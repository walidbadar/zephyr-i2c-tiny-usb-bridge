/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-FileCopyrightText: Copyright (c) 2016-2019 Intel Corporation
 * SPDX-FileCopyrightText: Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/bos.h>
#include <zephyr/usb/msos_desc.h>
#include <zephyr/usb/usbd.h>

#include "bridge.h"

/*
 * Microsoft OS 2.0 descriptor set without function subsets, so that Windows
 * binds WinUSB to the whole device, in runtime and in DFU mode. See the
 * Microsoft OS 2.0 Descriptors Specification for reference.
 */
#define MSOSV2_OS_VERSION 0x06030000UL /* Windows 8.1 */

/* {5C0A3FB5-29BD-401A-AB9B-6D69EF4EF7B0} */
#define BRIDGE_DEVICE_INTERFACE_GUID                                                       \
	'{', 0x00, '5', 0x00, 'C', 0x00, '0', 0x00, 'A', 0x00, '3', 0x00, 'F', 0x00,         \
	'B', 0x00, '5', 0x00, '-', 0x00, '2', 0x00, '9', 0x00, 'B', 0x00, 'D', 0x00,         \
	'-', 0x00, '4', 0x00, '0', 0x00, '1', 0x00, 'A', 0x00, '-', 0x00, 'A', 0x00,         \
	'B', 0x00, '9', 0x00, 'B', 0x00, '-', 0x00, '6', 0x00, 'D', 0x00, '6', 0x00,         \
	'9', 0x00, 'E', 0x00, 'F', 0x00, '4', 0x00, 'E', 0x00, 'F', 0x00, '7', 0x00,         \
	'B', 0x00, '0', 0x00, '}', 0x00, 0x00, 0x00, 0x00, 0x00

struct msosv2_descriptor {
	struct msosv2_descriptor_set_header header;
	struct msosv2_compatible_id compatible_id;
	struct msosv2_guids_property guids_property;
} __packed;

static const struct msosv2_descriptor msosv2_desc = {
	.header = {
		.wLength = sys_cpu_to_le16(sizeof(struct msosv2_descriptor_set_header)),
		.wDescriptorType = sys_cpu_to_le16(MS_OS_20_SET_HEADER_DESCRIPTOR),
		.dwWindowsVersion = sys_cpu_to_le32(MSOSV2_OS_VERSION),
		.wTotalLength = sys_cpu_to_le16(sizeof(struct msosv2_descriptor)),
	},
	.compatible_id = {
		.wLength = sys_cpu_to_le16(sizeof(struct msosv2_compatible_id)),
		.wDescriptorType = sys_cpu_to_le16(MS_OS_20_FEATURE_COMPATIBLE_ID),
		.CompatibleID = {'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00},
	},
	.guids_property = {
		.wLength = sys_cpu_to_le16(sizeof(struct msosv2_guids_property)),
		.wDescriptorType = sys_cpu_to_le16(MS_OS_20_FEATURE_REG_PROPERTY),
		.wPropertyDataType = sys_cpu_to_le16(MS_OS_20_PROPERTY_DATA_REG_MULTI_SZ),
		.wPropertyNameLength = sys_cpu_to_le16(42),
		.PropertyName = {DEVICE_INTERFACE_GUIDS_PROPERTY_NAME},
		.wPropertyDataLength = sys_cpu_to_le16(80),
		.bPropertyData = {BRIDGE_DEVICE_INTERFACE_GUID},
	},
};

struct bos_msosv2_descriptor {
	struct usb_bos_platform_descriptor platform;
	struct usb_bos_capability_msos cap;
} __packed;

static const struct bos_msosv2_descriptor bos_msosv2_desc = {
	.platform = {
		.bLength = sizeof(struct usb_bos_platform_descriptor) +
			   sizeof(struct usb_bos_capability_msos),
		.bDescriptorType = USB_DESC_DEVICE_CAPABILITY,
		.bDevCapabilityType = USB_BOS_CAPABILITY_PLATFORM,
		.bReserved = 0,
		/* D8DD60DF-4589-4CC7-9CD2-659D9E648A9F */
		.PlatformCapabilityUUID = {
			0xDF, 0x60, 0xDD, 0xD8,
			0x89, 0x45,
			0xC7, 0x4C,
			0x9C, 0xD2,
			0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,
		},
	},
	.cap = {
		.dwWindowsVersion = sys_cpu_to_le32(MSOSV2_OS_VERSION),
		.wMSOSDescriptorSetTotalLength = sys_cpu_to_le16(sizeof(msosv2_desc)),
		.bMS_VendorCode = CONFIG_APP_USBD_WINUSB_VENDOR_CODE,
		.bAltEnumCode = 0x00,
	},
};

static const struct usb_bos_capability_lpm bos_cap_lpm = {
	.bLength = sizeof(struct usb_bos_capability_lpm),
	.bDescriptorType = USB_DESC_DEVICE_CAPABILITY,
	.bDevCapabilityType = USB_BOS_CAPABILITY_EXTENSION,
	.bmAttributes = 0UL,
};

static struct net_buf *msosv2_to_host(const struct usbd_context *const uds_ctx,
				      const struct usb_setup_packet *const setup)
{
	struct net_buf *buf;
	uint16_t len;

	if (setup->wIndex != MS_OS_20_DESCRIPTOR_INDEX) {
		return NULL;
	}

	len = MIN(setup->wLength, sizeof(msosv2_desc));
	buf = usbd_ep_ctrl_data_in_alloc(uds_ctx, len);
	if (buf == NULL) {
		return NULL;
	}

	net_buf_add_mem(buf, &msosv2_desc, len);

	return buf;
}

USBD_DESC_BOS_DEFINE(bridge_bos_lpm, sizeof(bos_cap_lpm), &bos_cap_lpm);
USBD_DESC_BOS_VREQ_DEFINE(bridge_bos_msosv2, sizeof(bos_msosv2_desc), &bos_msosv2_desc,
			  CONFIG_APP_USBD_WINUSB_VENDOR_CODE, msosv2_to_host, NULL);

int bridge_msosv2_add(struct usbd_context *const uds_ctx)
{
	int err;

	/* The host only reads the BOS descriptor from USB 2.01 devices */
	err = usbd_device_set_bcd_usb(uds_ctx, USBD_SPEED_FS, USB_SRN_2_0_1);
	if (err != 0) {
		return err;
	}

	if (USBD_SUPPORTS_HIGH_SPEED && usbd_caps_speed(uds_ctx) == USBD_SPEED_HS) {
		err = usbd_device_set_bcd_usb(uds_ctx, USBD_SPEED_HS, USB_SRN_2_0_1);
		if (err != 0) {
			return err;
		}
	}

	err = usbd_add_descriptor(uds_ctx, &bridge_bos_lpm);
	if (err != 0) {
		return err;
	}

	return usbd_add_descriptor(uds_ctx, &bridge_bos_msosv2);
}
