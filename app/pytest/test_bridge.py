# SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
# SPDX-License-Identifier: Apache-2.0

"""Tests of a bridge attached to the Linux host, through i2c-dev and sysfs."""

import ctypes
import errno
import fcntl
import os
import re
from pathlib import Path

import pytest

# From linux/i2c-dev.h and linux/i2c.h
I2C_FUNCS = 0x0705
I2C_RDWR = 0x0707
I2C_M_RD = 0x0001

I2C_FUNC_I2C = 0x00000001
I2C_FUNC_SMBUS_EMUL = 0x0eff0008


class I2cMsg(ctypes.Structure):
    _fields_ = [("addr", ctypes.c_uint16), ("flags", ctypes.c_uint16),
                ("len", ctypes.c_uint16), ("buf", ctypes.POINTER(ctypes.c_uint8))]


class I2cRdwrData(ctypes.Structure):
    _fields_ = [("msgs", ctypes.POINTER(I2cMsg)), ("nmsgs", ctypes.c_uint32)]


def i2c_transfer(dev, addr, msgs):
    """Run messages, given as bytes to write or an int to read, in one transaction."""
    bufs = []
    cmsgs = (I2cMsg * len(msgs))()
    for cmsg, msg in zip(cmsgs, msgs):
        if isinstance(msg, int):
            buf = (ctypes.c_uint8 * msg)()
            cmsg.flags = I2C_M_RD
        else:
            buf = (ctypes.c_uint8 * len(msg))(*msg)
        bufs.append(buf)
        cmsg.addr = addr
        cmsg.len = len(buf)
        cmsg.buf = ctypes.cast(buf, ctypes.POINTER(ctypes.c_uint8))

    data = I2cRdwrData(cmsgs, len(msgs))
    fd = os.open(dev, os.O_RDWR)
    try:
        fcntl.ioctl(fd, I2C_RDWR, data)
    finally:
        os.close(fd)

    return [bytes(buf) for msg, buf in zip(msgs, bufs) if isinstance(msg, int)]


def app_version():
    text = (Path(__file__).parent.parent / "VERSION").read_text()
    fields = dict(re.findall(r"^(\w+)\s*=\s*(\S*)", text, re.MULTILINE))
    return int(fields["VERSION_MAJOR"]), int(fields["VERSION_MINOR"])


def test_usb_strings(bridge):
    assert bridge.usb_attr("manufacturer") == "zephyr-i2c-tiny-usb-bridge"
    assert bridge.usb_attr("product") == "i2c-tiny-usb bridge"
    assert re.fullmatch(r"[0-9A-F]+", bridge.usb_attr("serial"))


def test_usb_version(bridge):
    major, minor = app_version()
    assert bridge.usb_attr("bcdDevice") == f"{major:02d}{minor:02d}"


def test_functionality(bridge):
    fd = os.open(bridge.dev, os.O_RDWR)
    try:
        funcs = ctypes.c_ulong()
        fcntl.ioctl(fd, I2C_FUNCS, funcs)
    finally:
        os.close(fd)

    assert funcs.value == I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL


def test_target_register(bridge, target):
    addr, reg, value = target

    # Register address and read with a repeated start
    data, = i2c_transfer(bridge.dev, addr, [bytes([reg]), 1])
    assert data[0] == value


def test_absent_target(bridge, request):
    absent = request.config.getoption("--absent")
    if absent is None:
        pytest.skip("No --absent address given")

    with pytest.raises(OSError) as exc:
        i2c_transfer(bridge.dev, int(absent, 0), [1])
    assert exc.value.errno in (errno.ENXIO, errno.EREMOTEIO, errno.EIO)
