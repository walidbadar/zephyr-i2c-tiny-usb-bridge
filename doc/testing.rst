.. SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
..
.. SPDX-License-Identifier: CC-BY-4.0

:hide-toc:

Testing
=======

Build Tests
-----------

The firmware configurations in ``app/sample.yaml`` are built with :external+zephyr:ref:`Twister
<twister_script>`:

.. code-block:: console

   west twister -T zephyr-i2c-tiny-usb-bridge/app --integration

USB Class Tests
---------------

The tests in ``tests/subsys/usb/i2c_tiny_usb/`` run the USB device class on ``native_sim``,
connected through virtual USB host and device controllers. They issue the requests of the Linux
driver against an emulated AT24 EEPROM on the bridged bus:

.. code-block:: console

   west twister -T zephyr-i2c-tiny-usb-bridge/tests --integration

Hardware Tests
--------------

The pytest suite in ``app/pytest/`` tests a bridge attached to the Linux host running the tests,
through ``i2c-dev`` and sysfs. Twister flashes the board and runs the suite when the
``i2c_tiny_usb_host`` fixture is available:

.. code-block:: console

   west twister -T zephyr-i2c-tiny-usb-bridge/app -p stm32f3_disco --device-testing \
       --device-serial /dev/ttyACM0 --fixture i2c_tiny_usb_host

The suite can also run on its own against an attached bridge. ``--target`` gives an I2C register
with a known value on the bridged bus, ``--absent`` an address without a target:

.. code-block:: console

   cd zephyr-i2c-tiny-usb-bridge/app
   pytest pytest/test_bridge.py --target=0x1e:0x0a:0x48 --absent=0x10

Use ``--bridge-serial`` to select a bridge when several are attached.
