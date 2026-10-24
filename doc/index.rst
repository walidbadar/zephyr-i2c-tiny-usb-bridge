.. SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
.. SPDX-FileCopyrightText: Copyright (c) 2024-2026 Henrik Brix Andersen <henrik@brixandersen.dk>
..
.. SPDX-License-Identifier: CC-BY-4.0

:hide-toc:

Introduction
============

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: Contents:

   self
   building
   porting
   usage
   class
   testing

zephyr-i2c-tiny-usb-bridge is an open source firmware for Universal Serial Bus (USB) to
Inter-Integrated Circuit (I2C) adapters.

The firmware implements the `i2c-tiny-usb`_ device protocol. This protocol is supported by the Linux
kernel `i2c-tiny-usb driver`_, which exposes the adapter as a regular ``/dev/i2c-N`` bus to
``i2c-tools`` and any other program using ``i2c-dev``.

The firmware, which is based on the `Zephyr RTOS`_, allows turning your favorite microcontroller
development board into a USB to I2C adapter.

zephyr-i2c-tiny-usb-bridge is licensed under the `Apache-2.0 license`_. The documentation is
licensed under the `CC BY 4.0 license`_.

This documentation is adapted from the `CANnectivity documentation`_ by Henrik Brix Andersen,
licensed under the `CC BY 4.0 license`_, and has been modified for this project.

Firmware Features
-----------------

The firmware supports the following features, some of which depend on hardware support:

* Plain I2C transfers, including repeated starts between the messages of a transaction
* SMBus emulation
* Hardware I2C controllers and bit-banged GPIO buses (``gpio-i2c``)
* Bus speed selection by the host (standard, fast and fast-plus mode)
* Full-speed and high-speed USB
* Unique USB serial number per device
* Activity LED
* Uses the in-tree Linux i2c-tiny-usb driver, no driver installation needed
* Automatic ``i2c-dev`` loading, custom USB ID binding and user access under Linux using custom
  udev rules
* Automatic WinUSB driver installation under Microsoft Windows 8.1 and newer
* USB Device Firmware Upgrade (DFU) mode
* The i2c-tiny-usb protocol is implemented as an out-of-tree USB device class, reusable in other
  Zephyr applications

.. _i2c-tiny-usb:
   https://github.com/harbaum/I2C-Tiny-USB

.. _i2c-tiny-usb driver:
   https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/i2c/busses/i2c-tiny-usb.c

.. _Zephyr RTOS:
   https://www.zephyrproject.org

.. _Apache-2.0 license:
   http://www.apache.org/licenses/LICENSE-2.0

.. _CANnectivity documentation:
   https://github.com/CANnectivity/cannectivity/tree/main/doc

.. _CC BY 4.0 license:
   https://creativecommons.org/licenses/by/4.0/
