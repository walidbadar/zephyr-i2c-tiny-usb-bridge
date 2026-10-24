# SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
# SPDX-License-Identifier: Apache-2.0
#
# Entry point of the Zephyr module (see zephyr/module.yml); the application
# lives in app/ and is built like any Zephyr application.

add_subdirectory_ifdef(CONFIG_USBD_I2C_TINY_USB_CLASS subsys/usb/class)
