.. SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
.. SPDX-FileCopyrightText: Copyright (c) 2024-2026 Henrik Brix Andersen <henrik@brixandersen.dk>
..
.. SPDX-License-Identifier: CC-BY-4.0

:hide-toc:

Workspace Setup
===============

Building the firmware requires a proper Zephyr development environment. This repository can be
used as a stand-alone Zephyr workspace, linked into an existing workspace as its manifest, or added
to an existing workspace as a module.

Stand-alone Workspace
---------------------

Follow the official :external+zephyr:ref:`Getting Started Guide <getting_started>` until the "Get
the Zephyr source code" step.

Replace the instructions for getting the Zephyr source code with the command below. This will clone
the firmware repository and download the necessary Zephyr modules:

.. code-block:: console

   west init -m https://github.com/walidbadar/zephyr-i2c-tiny-usb-bridge --mr main <workspace directory>
   cd <workspace directory>
   west update

After performing the step above, continue with installing the Zephyr SDK according to the
instructions found in the Zephyr Getting Started Guide.

Linked into an Existing Workspace
---------------------------------

An existing workspace, e.g. ``~/zephyrproject``, can use a local clone of the repository as its
manifest instead:

.. code-block:: console

   cd ~/zephyrproject
   ln -s /path/to/zephyr-i2c-tiny-usb-bridge zephyr-i2c-tiny-usb-bridge
   west config manifest.path zephyr-i2c-tiny-usb-bridge
   west update

``west update`` checks out the Zephyr revision pinned in ``west.yml`` and the modules listed there.

As a Zephyr Module
------------------

The firmware repository is a :external+zephyr:ref:`Zephyr module <modules>`, which allows for adding
it to an existing Zephyr workspace, either for building the firmware as part of an existing
workspace, or for reuse of the i2c-tiny-usb USB device class outside of the firmware application.

To pull it in as a Zephyr module, either add it as a West project in the ``west.yml`` file or pull
it in by adding a submanifest (e.g. ``zephyr/submanifests/i2c-tiny-usb.yaml``) file with the
following content and run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: zephyr-i2c-tiny-usb-bridge
         url: https://github.com/walidbadar/zephyr-i2c-tiny-usb-bridge.git
         revision: main
         path: custom/zephyr-i2c-tiny-usb-bridge # adjust the path as needed

Building and Running
--------------------

After setting up a workspace, build the firmware for your board using ``west``. Boards with a
configuration in ``app/boards/`` (here ``stm32f3_disco``) need no further options:

.. code-block:: console

   west build -b stm32f3_disco zephyr-i2c-tiny-usb-bridge/app

Other boards need a devicetree overlay selecting the I2C bus, see :doc:`porting`:

.. code-block:: console

   west build -b <board> zephyr-i2c-tiny-usb-bridge/app -- -DEXTRA_DTC_OVERLAY_FILE=<board>.overlay

The default configuration is a release build without logging. To use the ``debug`` configuration,
which has logging and a boot banner with the firmware version enabled, set ``FILE_SUFFIX=debug``:

.. code-block:: console

   west build -b stm32f3_disco zephyr-i2c-tiny-usb-bridge/app -- -DFILE_SUFFIX=debug

After building, the firmware can be flashed to the board by running the ``west flash`` command.

The firmware version is set in ``app/VERSION`` and reported to the host in the USB ``bcdDevice``
field, which the Linux driver logs when the adapter is plugged in.

USB Device Firmware Upgrade (DFU) Mode
--------------------------------------

The firmware supports USB :external+zephyr:ref:`Device Firmware Upgrade <dfu>` (DFU) via the
`MCUboot`_ bootloader. This is intended for use with boards without an on-board programmer. The
board needs ``slot0_partition`` and ``slot1_partition`` flash partitions.

To build the firmware with MCUboot integration for USB DFU use :external+zephyr:ref:`sysbuild
<sysbuild>` when building for your board (here ``stm32f3_disco``):

.. code-block:: console

   west build -b stm32f3_disco --sysbuild zephyr-i2c-tiny-usb-bridge/app

After building, MCUboot and the firmware can be flashed to the board by running the ``west flash``
command.

Subsequent firmware updates can be applied via USB DFU using `dfu-util`_:

.. code-block:: console

   dfu-util -d 1209:0001 -D build/app/zephyr/zephyr.signed.bin

The ``dfu-util`` command switches the firmware into DFU mode and downloads the image into the
slot-1 partition. The device then reboots into MCUboot, which swaps in the new image. The new
firmware confirms itself once its USB device support is up, otherwise MCUboot reverts to the
previous image on the next reset.

MCUboot signs images with its development key by default. Set ``SB_CONFIG_BOOT_SIGNATURE_KEY_FILE``
to your own key before distributing devices.

.. _MCUboot:
   https://www.trustedfirmware.org/projects/mcuboot/

.. _dfu-util:
   https://dfu-util.sourceforge.net/
