<!--
SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
SPDX-FileCopyrightText: Copyright (c) 2024-2026 Henrik Brix Andersen <henrik@brixandersen.dk>
SPDX-License-Identifier: Apache-2.0
-->

[![Build](https://github.com/walidbadar/zephyr-i2c-tiny-usb-bridge/actions/workflows/build.yml/badge.svg)](https://github.com/walidbadar/zephyr-i2c-tiny-usb-bridge/actions/workflows/build.yml?query=branch%3Amain)
[![Docs](https://github.com/walidbadar/zephyr-i2c-tiny-usb-bridge/actions/workflows/docs.yml/badge.svg)](https://github.com/walidbadar/zephyr-i2c-tiny-usb-bridge/actions/workflows/docs.yml?query=branch%3Amain)

# zephyr-i2c-tiny-usb-bridge

## Overview

zephyr-i2c-tiny-usb-bridge is an open source firmware for Universal Serial Bus
(USB) to Inter-Integrated Circuit (I2C) adapters.

The firmware implements the [i2c-tiny-usb](https://github.com/harbaum/I2C-Tiny-USB)
device protocol. This protocol is supported by the Linux kernel
[i2c-tiny-usb driver](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/i2c/busses/i2c-tiny-usb.c),
which exposes the adapter as a regular `/dev/i2c-N` bus to `i2c-tools` and any
other program using `i2c-dev`.

The firmware, which is based on the [Zephyr RTOS](https://www.zephyrproject.org),
allows turning your favorite microcontroller development board into a USB to
I2C adapter.

The full documentation is available at
<https://walidbadar.github.io/zephyr-i2c-tiny-usb-bridge/>.

zephyr-i2c-tiny-usb-bridge is licensed under the [Apache-2.0 license](LICENSE).
The documentation is licensed under the [CC BY 4.0 license](doc/LICENSE).

## Firmware Features

The firmware supports the following features, some of which depend on hardware
support:

- Plain I2C transfers, including repeated starts between the messages of a
  transaction
- SMBus emulation
- Hardware I2C controllers and bit-banged GPIO buses (`gpio-i2c`)
- Bus speed selection by the host (standard, fast and fast-plus mode)
- Full-speed and high-speed USB
- Unique USB serial number per device
- Activity LED
- Uses the in-tree Linux i2c-tiny-usb driver, no driver installation needed
- Automatic i2c-dev loading, custom USB ID binding and user access under Linux
  using custom [udev rules](70-i2c-tiny-usb.rules)
- Automatic WinUSB driver installation under Microsoft Windows 8.1 and newer
- USB Device Firmware Upgrade (DFU) mode
- The i2c-tiny-usb protocol is implemented as an out-of-tree USB device class,
  reusable in other Zephyr applications

## Hardware Requirements

Since the firmware is based on the Zephyr RTOS, it requires Zephyr support for
the board it is to run on. The board configuration must support a USB device
driver for the `device_next` USB stack and either an I2C controller or two
GPIOs for a bit-banged bus.

Check the list of [supported boards](https://docs.zephyrproject.org/latest/boards/index.html)
in the Zephyr documentation to see if your board is already supported. If not,
have a look at the instructions in the
[board porting guide](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html).

## Board Configuration

The firmware relies on an `i2c-tiny-usb` [devicetree](https://docs.zephyrproject.org/latest/build/dts/index.html)
node for specifying the I2C controller to use, and on the `zephyr_udc0` node
label for the USB device controller. Most boards already define `zephyr_udc0`,
so a devicetree overlay with the `i2c-tiny-usb` node is often all that is
needed:

```dts
/ {
	i2c-tiny-usb {
		compatible = "i2c-tiny-usb";
		i2c-bus = <&i2c1>;
	};
};
```

Any free GPIO pair can serve as a bit-banged bus instead, with a `gpio-i2c`
node:

```dts
#include <zephyr/dt-bindings/i2c/i2c.h>

/ {
	i2c-tiny-usb {
		compatible = "i2c-tiny-usb";
		i2c-bus = <&gpio_i2c>;
	};

	gpio_i2c: i2c {
		compatible = "gpio-i2c";
		clock-frequency = <I2C_BITRATE_STANDARD>;
		/* Adjust to the GPIOs of your board */
		scl-gpios = <&gpioa 3 GPIO_OPEN_DRAIN>;
		sda-gpios = <&gpioa 4 GPIO_OPEN_DRAIN>;
		#address-cells = <1>;
		#size-cells = <0>;
	};
};
```

The bus needs pull-ups, unless the I2C target or the board provides them.
The build fails with an explicit error when the `i2c-tiny-usb` node is missing.

If a devicetree alias for `led0` is present, it is used as activity LED: it is
on while the host has configured the device and goes off briefly for each I2C
transaction. The `activity-led` property selects a different LED. Check the
[i2c-tiny-usb](dts/bindings/usb/i2c-tiny-usb.yaml) devicetree binding.

Board configurations placed in `app/boards/<board>.overlay` and
`app/boards/<board>.conf` are picked up automatically. Check the
[example board configurations](app/boards), which include both a hardware I2C
controller and a bit-banged bus. Boards with an Arduino header can use
[app/overlays/arduino_i2c.overlay](app/overlays/arduino_i2c.overlay).

## Workspace Setup

Building the firmware requires a proper Zephyr development environment. This
repository can be used as a stand-alone Zephyr workspace, linked into an
existing workspace as its manifest, or added to an existing workspace as a
module.

### Stand-alone Workspace

Follow the official [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
until the "Get the Zephyr source code" step.

Replace the instructions for getting the Zephyr source code with the command
below. This will clone the firmware repository and download the necessary
Zephyr modules:

```shell
west init -m https://github.com/walidbadar/zephyr-i2c-tiny-usb-bridge --mr main <workspace directory>
cd <workspace directory>
west update
```

After performing the step above, continue with installing the Zephyr SDK
according to the instructions found in the Zephyr Getting Started Guide.

### Linked into an Existing Workspace

An existing workspace, e.g. `~/zephyrproject`, can use a local clone of the
repository as its manifest instead:

```shell
cd ~/zephyrproject
ln -s /path/to/zephyr-i2c-tiny-usb-bridge zephyr-i2c-tiny-usb-bridge
west config manifest.path zephyr-i2c-tiny-usb-bridge
west update zephyr
```

`west update zephyr` checks out the Zephyr revision pinned in
[west.yml](west.yml). Run `west update` without arguments to also fetch the
HALs and modules listed there.

### As a Zephyr Module

The firmware repository is a [Zephyr module](https://docs.zephyrproject.org/latest/develop/modules.html),
which allows for adding it to an existing Zephyr workspace, either for building
the firmware as part of an existing workspace, or for reuse of the
i2c-tiny-usb USB device class outside of the firmware application.

To pull it in as a Zephyr module, either add it as a West project in the
`west.yml` file or pull it in by adding a submanifest (e.g.
`zephyr/submanifests/i2c-tiny-usb.yaml`) file with the following content and
run `west update`:

```yaml
manifest:
  projects:
    - name: zephyr-i2c-tiny-usb-bridge
      url: https://github.com/walidbadar/zephyr-i2c-tiny-usb-bridge.git
      revision: main
      path: custom/zephyr-i2c-tiny-usb-bridge # adjust the path as needed
```

### Building and Running

After setting up a workspace, build the firmware for your board using `west`,
passing the devicetree overlay from [Board Configuration](#board-configuration):

```shell
west build -b <board> zephyr-i2c-tiny-usb-bridge/app -- -DEXTRA_DTC_OVERLAY_FILE=<board>.overlay
```

Board-specific Kconfig options can be passed the same way with
`-DEXTRA_CONF_FILE=<board>.conf`. Boards with a configuration in
[app/boards](app/boards) need neither:

```shell
west build -b <board> zephyr-i2c-tiny-usb-bridge/app
```

The default configuration is a release build without logging. To use the
`debug` configuration, which has logging and a boot banner with the firmware
version enabled, set `FILE_SUFFIX=debug`:

```shell
west build -b <board> zephyr-i2c-tiny-usb-bridge/app -- -DFILE_SUFFIX=debug
```

After building, the firmware can be flashed to the board by running the
`west flash` command.

The firmware version is set in [app/VERSION](app/VERSION) and reported to the
host in the USB `bcdDevice` field, which the Linux driver logs when the
adapter is plugged in.

## USB Device Firmware Upgrade (DFU) Mode

The firmware supports USB [Device Firmware Upgrade](https://docs.zephyrproject.org/latest/services/device_mgmt/dfu.html)
(DFU) via the [MCUboot](https://www.trustedfirmware.org/projects/mcuboot/)
bootloader. This is intended for use with boards without an on-board
programmer. The board needs `slot0_partition` and `slot1_partition` flash
partitions.

To build the firmware with MCUboot integration for USB DFU use
[sysbuild](https://docs.zephyrproject.org/latest/build/sysbuild/index.html):

```shell
west build -b <board> --sysbuild zephyr-i2c-tiny-usb-bridge/app
```

After building, MCUboot and the firmware can be flashed to the board by running
the `west flash` command. Subsequent firmware updates can be applied via USB
DFU using [dfu-util](https://dfu-util.sourceforge.net/):

```shell
dfu-util -d 1209:0001 -D build/app/zephyr/zephyr.signed.bin
```

The `dfu-util` command switches the firmware into DFU mode and downloads the
image. The device then reboots into MCUboot, which swaps in the new image. The
new firmware confirms itself once its USB device support is up, otherwise
MCUboot reverts to the previous image on the next reset.

MCUboot signs images with its development key by default. Set
`SB_CONFIG_BOOT_SIGNATURE_KEY_FILE` to your own key before distributing
devices.

## Usage

### Linux

The Linux kernel includes the i2c-tiny-usb driver, but it does not know the
[USB IDs](#usb-device-configuration) of the firmware. The
[udev rules](70-i2c-tiny-usb.rules) bind the driver to the adapter, load
`i2c-dev`, and give the logged-in user and the `plugdev` group access to the
bus. Install them with:

```shell
sudo cp 70-i2c-tiny-usb.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```

The bus can then be used without `sudo`:

```shell
i2cdetect -l | grep i2c-tiny-usb
i2cdetect -y <bus number>
```

Without the udev rules, bind the driver manually after plugging in the adapter:

```shell
sudo modprobe -a i2c-tiny-usb i2c-dev
echo 1209 0001 ff | sudo tee /sys/bus/usb/drivers/i2c-tiny-usb/new_id
```

### Microsoft Windows

The firmware reports Microsoft OS 2.0 descriptors, so Windows 8.1 and newer
install the WinUSB driver automatically. Windows has no i2c-tiny-usb driver,
but libusb based tools can access the device without a driver installation.

## USB Device Configuration

| Kconfig                     | Default                      | Purpose                      |
|-----------------------------|------------------------------|------------------------------|
| `APP_USBD_VID`              | `0x1209`                     | USB vendor ID                |
| `APP_USBD_PID`              | `0x0001`                     | USB product ID               |
| `APP_USBD_MANUFACTURER`     | `zephyr-i2c-tiny-usb-bridge` | Manufacturer string          |
| `APP_USBD_PRODUCT`          | `i2c-tiny-usb bridge`        | Product string               |
| `APP_USBD_SELF_POWERED`     | `n`                          | Self-powered device          |
| `APP_USBD_MAX_POWER`        | `50`                         | Maximum current, in 2 mA units |
| `APP_USBD_WINUSB`           | `y`                          | Microsoft OS 2.0 descriptors |
| `APP_USBD_DFU_VID`, `APP_USBD_DFU_PID` | Runtime IDs       | USB IDs in DFU mode          |

The default USB IDs are the [pid.codes](https://pid.codes) test IDs, which are
only meant for testing and private use. Set your own IDs before distributing a
device: open source projects can get a free product ID under the vendor ID
`0x1209` from pid.codes. Replace `1209` and `0001` in the
[udev rules](70-i2c-tiny-usb.rules) with them, and the rules bind the driver to
the i2c-tiny-usb interface of the adapter.

The IDs of the original i2c-tiny-usb adapter, which the Linux driver knows,
belong to its vendor and are not used. With them, the driver would also bind to
the DFU interface.

## USB Device Class

The i2c-tiny-usb protocol is implemented as a USB device class for the Zephyr
`device_next` USB stack, in [subsys/usb/class](subsys/usb/class). With this
repository added as a module, the class (`CONFIG_USBD_I2C_TINY_USB_CLASS`) is
enabled by default when `CONFIG_USB_DEVICE_STACK_NEXT`, `CONFIG_I2C` and an
`i2c-tiny-usb` devicetree node are present.

The class is registered under the name `i2c_tiny_usb` like any other class,
and registers its vendor requests itself when `usbd_init()` runs. The Linux
driver sends its commands to interface 0, so the class must be registered
before any other class of the configuration. See [app/src/main.c](app/src/main.c)
for a complete example.

The class supports a single instance, as the host addresses I2C transfers to
the device rather than to the interface.

## Testing

The firmware configurations are built, and the USB device class is tested on
`native_sim` against an emulated I2C target, with
[Twister](https://docs.zephyrproject.org/latest/develop/test/twister.html):

```shell
west twister -T zephyr-i2c-tiny-usb-bridge/app --integration
west twister -T zephyr-i2c-tiny-usb-bridge/tests --integration
```

The pytest suite in [app/pytest](app/pytest) tests a bridge attached to the
Linux host, through `i2c-dev` and sysfs:

```shell
cd zephyr-i2c-tiny-usb-bridge/app
pytest pytest/test_bridge.py --target=<addr>:<reg>:<value> --absent=<addr>
```

## License

zephyr-i2c-tiny-usb-bridge is licensed under the [Apache-2.0 license](LICENSE).
The documentation is licensed under the [CC BY 4.0 license](doc/LICENSE). The
USB device class is derived from the Zephyr `samples/subsys/usb/i2c_tiny_usb`
sample. The project structure, documentation and tooling are adapted from
[CANnectivity](https://github.com/CANnectivity/cannectivity).
