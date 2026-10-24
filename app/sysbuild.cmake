# SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
# SPDX-License-Identifier: Apache-2.0

# Boards whose devicetree has no MCUboot flash layout get one from their board
# overlay, which MCUboot then uses as well
set(board_overlay ${APP_DIR}/boards/${BOARD}.overlay)
if(EXISTS ${board_overlay})
  file(STRINGS ${board_overlay} board_layout REGEX "boot_partition:")
  if(board_layout)
    set(mcuboot_EXTRA_DTC_OVERLAY_FILE
        "${board_overlay};${APP_DIR}/sysbuild/boot_partition.overlay"
        CACHE INTERNAL "MCUboot flash layout from the board overlay"
    )
  endif()
endif()
