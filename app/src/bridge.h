/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BRIDGE_H
#define BRIDGE_H

#include <zephyr/usb/usbd.h>

/*
 * Add the device level descriptors shared by the runtime and the DFU mode
 * device contexts. Only one context is initialized at a time, the descriptor
 * nodes are released by usbd_shutdown().
 */
int bridge_add_descriptors(struct usbd_context *const uds_ctx);

/* Add the BOS descriptors and the Microsoft OS 2.0 descriptor set */
int bridge_msosv2_add(struct usbd_context *const uds_ctx);

/* Shut down the runtime device context and enumerate in DFU mode */
void bridge_dfu_switch(struct usbd_context *const runtime_ctx);

/* Confirm the running image, so that MCUboot does not revert it */
void bridge_dfu_confirm(void);

#endif /* BRIDGE_H */
