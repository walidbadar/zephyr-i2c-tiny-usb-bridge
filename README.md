<!--
SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
SPDX-FileCopyrightText: Copyright (c) 2024-2026 Henrik Brix Andersen <henrik@brixandersen.dk>
SPDX-License-Identifier: Apache-2.0
-->

[![Build](https://github.com/walidbadar/zephyr-i2c-tiny-usb-bridge/actions/workflows/build.yml/badge.svg)](https://github.com/walidbadar/zephyr-i2c-tiny-usb-bridge/actions/workflows/build.yml?query=branch%3Amain)
[![Docs](https://github.com/walidbadar/zephyr-i2c-tiny-usb-bridge/actions/workflows/docs.yml/badge.svg)](https://github.com/walidbadar/zephyr-i2c-tiny-usb-bridge/actions/workflows/docs.yml?query=branch%3Amain)

# zephyr-i2c-tiny-usb-bridge

Open source [Zephyr RTOS](https://www.zephyrproject.org) firmware that turns a
microcontroller development board into a USB to I2C adapter. It implements the
[i2c-tiny-usb](https://github.com/harbaum/I2C-Tiny-USB) protocol, so the
in-tree Linux driver exposes the adapter as a regular `/dev/i2c-N` bus.

**Documentation: <https://walidbadar.github.io/zephyr-i2c-tiny-usb-bridge/>**

## Quick Start

```shell
west init -m https://github.com/walidbadar/zephyr-i2c-tiny-usb-bridge --mr main <workspace directory>
cd <workspace directory>
west update
west build -b <board> zephyr-i2c-tiny-usb-bridge/app
west flash
```

Boards without a configuration in [app/boards](app/boards) need a devicetree
overlay selecting the I2C bus, see the
[board porting guide](https://walidbadar.github.io/zephyr-i2c-tiny-usb-bridge/porting.html).

On Linux, install the [udev rules](70-i2c-tiny-usb.rules), plug in the adapter
and use the bus:

```shell
sudo cp 70-i2c-tiny-usb.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
i2cdetect -l | grep i2c-tiny-usb
```

To build the documentation locally:

```shell
pip install -r doc/requirements.txt --require-hashes
make -C doc html
```

## License

zephyr-i2c-tiny-usb-bridge is licensed under the [Apache-2.0 license](LICENSE).
The documentation is licensed under the [CC BY 4.0 license](doc/LICENSE).
