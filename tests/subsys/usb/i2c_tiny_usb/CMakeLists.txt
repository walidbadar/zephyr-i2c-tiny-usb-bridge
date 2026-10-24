# SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_usbd_i2c_tiny_usb)

# USB host stack internals used to issue requests to the device
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/usb/host)

target_sources(app PRIVATE src/main.c)
