# SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
# SPDX-License-Identifier: Apache-2.0

"""Fixtures for testing a bridge attached to the Linux host running pytest."""

import glob
import os
import time
from dataclasses import dataclass

import pytest

ENUMERATION_TIMEOUT = 10.0


def pytest_addoption(parser):
    parser.addoption("--bridge-serial", default=None,
                     help="USB serial number of the bridge to test, required "
                          "when several bridges are attached")
    parser.addoption("--target", action="append", default=[],
                     metavar="ADDR:REG:VALUE",
                     help="I2C target register with a known value on the "
                          "bridged bus, e.g. 0x1e:0x0a:0x48 (repeatable)")
    parser.addoption("--absent", default=None, metavar="ADDR",
                     help="I2C address with no target on the bridged bus")


@dataclass
class Bridge:
    bus: int
    usb_path: str

    @property
    def dev(self):
        return f"/dev/i2c-{self.bus}"

    def usb_attr(self, name):
        with open(os.path.join(self.usb_path, name)) as f:
            return f.read().strip()


def find_bridges():
    bridges = []
    for path in glob.glob("/sys/bus/i2c/devices/i2c-*"):
        with open(os.path.join(path, "name")) as f:
            if not f.read().startswith("i2c-tiny-usb"):
                continue
        # i2c-N -> USB interface -> USB device
        usb_path = os.path.dirname(os.path.dirname(os.path.realpath(path)))
        bridges.append(Bridge(int(path.rsplit("-", 1)[1]), usb_path))
    return bridges


@pytest.fixture(scope="session")
def bridge(request):
    serial = request.config.getoption("--bridge-serial")
    deadline = time.monotonic() + ENUMERATION_TIMEOUT

    # The device enumerates after it has been flashed and has booted
    while True:
        bridges = find_bridges()
        if serial is not None:
            bridges = [b for b in bridges if b.usb_attr("serial") == serial]
        if bridges or time.monotonic() > deadline:
            break
        time.sleep(0.5)

    if not bridges:
        pytest.fail("No i2c-tiny-usb adapter found, is i2c-dev loaded?")
    if len(bridges) > 1:
        pytest.fail("Several i2c-tiny-usb adapters found, select one with --bridge-serial")

    return bridges[0]


def parse_target(spec):
    addr, reg, value = (int(x, 0) for x in spec.split(":"))
    return addr, reg, value


def pytest_generate_tests(metafunc):
    if "target" in metafunc.fixturenames:
        specs = metafunc.config.getoption("--target")
        metafunc.parametrize("target", [parse_target(s) for s in specs], ids=specs)
