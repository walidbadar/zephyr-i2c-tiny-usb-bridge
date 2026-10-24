.. SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
.. SPDX-FileCopyrightText: Copyright (c) 2024-2026 Henrik Brix Andersen <henrik@brixandersen.dk>
..
.. SPDX-License-Identifier: CC-BY-4.0

:hide-toc:

Board Porting Guide
===================

Hardware Requirements
---------------------

Since the firmware is based on the Zephyr RTOS, it requires Zephyr support for the board it is to
run on. The board configuration must support a USB device driver for the ``device_next`` USB stack
and either an I2C controller or two GPIOs for a bit-banged bus.

Check the list of :external+zephyr:ref:`supported boards <boards>` in the Zephyr documentation to
see if your board is already supported. If not, have a look at the instructions in the Zephyr
:external+zephyr:ref:`Board Porting Guide <board_porting_guide>`.

Board Configuration
-------------------

The firmware relies on an ``i2c-tiny-usb`` :external+zephyr:ref:`devicetree <devicetree>` node for
specifying the I2C controller to use, and on the ``zephyr_udc0`` node label for the USB device
controller. Most boards already define ``zephyr_udc0``, so a devicetree overlay with the
``i2c-tiny-usb`` node is often all that is needed:

.. code-block:: devicetree

   / {
           i2c-tiny-usb {
                   compatible = "i2c-tiny-usb";
                   i2c-bus = <&i2c1>;
           };
   };

Any free GPIO pair can serve as a bit-banged bus instead, with a ``gpio-i2c`` node:

.. code-block:: devicetree

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

The bus needs pull-ups, unless the I2C target or the board provides them. The build fails with an
explicit error when the ``i2c-tiny-usb`` node is missing.

Boards with an Arduino header can use ``app/overlays/arduino_i2c.overlay``, which bridges the
``arduino_i2c`` bus:

.. code-block:: console

   west build -b <board> zephyr-i2c-tiny-usb-bridge/app -- \
       -DEXTRA_DTC_OVERLAY_FILE=overlays/arduino_i2c.overlay

Activity LED
------------

If the board has a ``led0`` devicetree alias, it is used as activity LED. The LED is on while the
host has configured the device and goes off briefly for each I2C transaction. A different LED can be
selected with the ``activity-led`` property, which takes a child of an LED controller node such as
``gpio-leds``:

.. code-block:: devicetree

   / {
           i2c-tiny-usb {
                   compatible = "i2c-tiny-usb";
                   i2c-bus = <&i2c1>;
                   activity-led = <&green_led>;
           };
   };

Check the description of the ``i2c-tiny-usb`` devicetree binding in
``dts/bindings/usb/i2c-tiny-usb.yaml`` and the example board configurations under ``app/boards/``.
Board configurations placed in ``app/boards/<board>.overlay`` and ``app/boards/<board>.conf`` are
picked up automatically.
