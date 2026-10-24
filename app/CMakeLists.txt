# SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(i2c_tiny_usb_bridge)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_APP_USBD_WINUSB app PRIVATE src/msosv2.c)
target_sources_ifdef(CONFIG_APP_DFU app PRIVATE src/dfu.c)

if(CONFIG_APP_DFU)
  math(EXPR usbd_vid "${CONFIG_APP_USBD_VID}")
  math(EXPR usbd_pid "${CONFIG_APP_USBD_PID}")
endif()

if(CONFIG_APP_DFU AND usbd_vid EQUAL 1027 AND usbd_pid EQUAL 50737) # 0403:c631
  message(WARNING "The Linux i2c-tiny-usb driver binds to all interfaces of the USB IDs "
    "of the original i2c-tiny-usb adapter and also creates an I2C adapter for the DFU "
    "interface. Use other USB IDs for DFU support.")
endif()
