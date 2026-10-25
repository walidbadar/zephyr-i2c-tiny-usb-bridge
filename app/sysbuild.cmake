# SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
# SPDX-License-Identifier: Apache-2.0

set(board_overlay ${APP_DIR}/boards/${BOARD}.overlay)
if(EXISTS ${board_overlay})
  # The board overlay selects the bus of the i2c-tiny-usb node of app.overlay,
  # which Zephyr only uses for boards without an overlay of their own
  set(${DEFAULT_IMAGE}_DTC_OVERLAY_FILE
      "${APP_DIR}/app.overlay;${board_overlay}"
      CACHE INTERNAL "Board overlay after the i2c-tiny-usb node of app.overlay"
  )

  # Boards whose devicetree has no MCUboot flash layout get one from their
  # board overlay, which MCUboot then uses as well
  file(STRINGS ${board_overlay} board_layout REGEX "boot_partition:")
  if(board_layout)
    set(mcuboot_EXTRA_DTC_OVERLAY_FILE
        "${APP_DIR}/app.overlay;${board_overlay};${APP_DIR}/sysbuild/boot_partition.overlay"
        CACHE INTERNAL "MCUboot flash layout from the board overlay"
    )
  endif()
endif()
