.. SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
..
.. SPDX-License-Identifier: CC-BY-4.0

:hide-toc:

Usage
=====

Linux
-----

The Linux kernel includes the i2c-tiny-usb driver, but it does not know the USB IDs of the
firmware. The ``70-i2c-tiny-usb.rules`` udev rules in the repository bind the driver to the
adapter, load ``i2c-dev``, and give the logged-in user and the ``plugdev`` group access to the bus.
Install them with:

.. code-block:: console

   sudo cp 70-i2c-tiny-usb.rules /etc/udev/rules.d/
   sudo udevadm control --reload-rules
   sudo udevadm trigger

The rules file must keep a number below 73, where systemd applies the ``uaccess`` tag. The bus can
then be used without ``sudo``:

.. code-block:: console

   i2cdetect -l | grep i2c-tiny-usb
   i2cdetect -y <bus number>

Without the udev rules, bind the driver manually after plugging in the adapter:

.. code-block:: console

   sudo modprobe -a i2c-tiny-usb i2c-dev
   echo 1209 0001 ff | sudo tee /sys/bus/usb/drivers/i2c-tiny-usb/new_id

Microsoft Windows
-----------------

The firmware reports Microsoft OS 2.0 descriptors, so Windows 8.1 and newer install the WinUSB
driver automatically. Windows has no i2c-tiny-usb driver, but libusb based tools can access the
device without a driver installation. The descriptors can be disabled with
``CONFIG_APP_USBD_WINUSB=n``.

USB Device Configuration
------------------------

.. list-table::
   :header-rows: 1

   * - Kconfig
     - Default
     - Purpose
   * - ``CONFIG_APP_USBD_VID``
     - ``0x1209``
     - USB vendor ID
   * - ``CONFIG_APP_USBD_PID``
     - ``0x0001``
     - USB product ID
   * - ``CONFIG_APP_USBD_MANUFACTURER``
     - ``zephyr-i2c-tiny-usb-bridge``
     - Manufacturer string
   * - ``CONFIG_APP_USBD_PRODUCT``
     - ``i2c-tiny-usb bridge``
     - Product string
   * - ``CONFIG_APP_USBD_SELF_POWERED``
     - ``n``
     - Self-powered device
   * - ``CONFIG_APP_USBD_MAX_POWER``
     - ``50``
     - Maximum current, in 2 mA units
   * - ``CONFIG_APP_USBD_DFU_VID``, ``CONFIG_APP_USBD_DFU_PID``
     - Runtime IDs
     - USB IDs in DFU mode

The default USB IDs are the `pid.codes`_ test IDs, which are only meant for testing and private
use. Set your own IDs before distributing a device: open source projects can get a free product ID
under the vendor ID ``0x1209`` from pid.codes. Replace ``1209`` and ``0001`` in the udev rules with
them, and the rules bind the driver to the i2c-tiny-usb interface of the adapter.

The IDs of the original i2c-tiny-usb adapter, which the Linux driver knows, belong to its vendor and
are not used. With them, the driver would also bind to the DFU interface.

.. _pid.codes:
   https://pid.codes
