.. SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
..
.. SPDX-License-Identifier: CC-BY-4.0

:hide-toc:

USB Device Class
================

The i2c-tiny-usb protocol is implemented as a USB device class for the Zephyr ``device_next`` USB
stack, in ``subsys/usb/class/``. With this repository added as a module, see :doc:`building`, the
class (``CONFIG_USBD_I2C_TINY_USB_CLASS``) is enabled by default when
``CONFIG_USB_DEVICE_STACK_NEXT``, ``CONFIG_I2C`` and an ``i2c-tiny-usb`` devicetree node are
present.

The class is registered under the name ``i2c_tiny_usb`` like any other class, and registers its
vendor requests itself when ``usbd_init()`` runs:

.. code-block:: c

   err = usbd_register_class(&usbd, "i2c_tiny_usb", USBD_SPEED_FS, 1);

The Linux driver sends its commands to interface 0, so the class must be registered before any
other class of the configuration. ``usbd_register_all_classes()`` registers classes in alphabetical
order and is only suitable when the i2c-tiny-usb class comes first. See ``app/src/main.c`` for a
complete example.

The class supports a single instance, as the host addresses I2C transfers to the device rather than
to the interface.

.. list-table::
   :header-rows: 1

   * - Kconfig
     - Purpose
   * - ``CONFIG_USBD_I2C_TINY_USB_CLASS``
     - i2c-tiny-usb USB device class
   * - ``CONFIG_USBD_I2C_TINY_USB_LED``
     - Activity LED
   * - ``CONFIG_USBD_I2C_TINY_USB_LED_BLINK_MS``
     - Time the activity LED stays off after an I2C transaction
   * - ``CONFIG_USBD_I2C_TINY_USB_LOG_LEVEL_*``
     - Log level of the class
