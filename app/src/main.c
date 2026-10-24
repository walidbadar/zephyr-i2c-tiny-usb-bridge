/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-FileCopyrightText: Copyright (c) 2024-2026 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/app_version.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usbd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

#include "bridge.h"

BUILD_ASSERT(DT_HAS_COMPAT_STATUS_OKAY(i2c_tiny_usb),
	     "Add an i2c-tiny-usb devicetree node selecting the I2C bus to bridge");

#if defined(APP_BUILD_VERSION) && !IS_EMPTY(APP_BUILD_VERSION)
#define BANNER_VERSION APP_VERSION_STRING " (" STRINGIFY(APP_BUILD_VERSION) ")"
#else
#define BANNER_VERSION APP_VERSION_STRING
#endif

/* The Linux i2c-tiny-usb driver reports bcdDevice as the firmware version */
#define BRIDGE_BCD_DEVICE                                                                  \
	(USB_DEC_TO_BCD(APP_VERSION_MAJOR) << 8 | USB_DEC_TO_BCD(APP_VERSION_MINOR))

USBD_DEVICE_DEFINE(bridge_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), CONFIG_APP_USBD_VID,
		   CONFIG_APP_USBD_PID);

USBD_DESC_LANG_DEFINE(bridge_lang);
USBD_DESC_MANUFACTURER_DEFINE(bridge_mfr, CONFIG_APP_USBD_MANUFACTURER);
USBD_DESC_PRODUCT_DEFINE(bridge_product, CONFIG_APP_USBD_PRODUCT);
#ifdef CONFIG_HWINFO_HAS_DRIVER
/* Unique per device, lets the host tell several bridges apart */
USBD_DESC_SERIAL_NUMBER_DEFINE(bridge_sn);
#endif
USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");
USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "HS Configuration");

#define BRIDGE_CFG_ATTRIB (IS_ENABLED(CONFIG_APP_USBD_SELF_POWERED) ? USB_SCD_SELF_POWERED : 0)

USBD_CONFIGURATION_DEFINE(fs_config, BRIDGE_CFG_ATTRIB, CONFIG_APP_USBD_MAX_POWER, &fs_cfg_desc);
USBD_CONFIGURATION_DEFINE(hs_config, BRIDGE_CFG_ATTRIB, CONFIG_APP_USBD_MAX_POWER, &hs_cfg_desc);

static void msg_cb(struct usbd_context *const usbd_ctx, const struct usbd_msg *const msg)
{
	LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

	if (IS_ENABLED(CONFIG_APP_DFU) && msg->type == USBD_MSG_DFU_APP_DETACH) {
		bridge_dfu_switch(usbd_ctx);
		return;
	}

	if (!usbd_can_detect_vbus(usbd_ctx)) {
		return;
	}

	if (msg->type == USBD_MSG_VBUS_READY && usbd_enable(usbd_ctx) != 0) {
		LOG_ERR("Failed to enable device support");
	}

	if (msg->type == USBD_MSG_VBUS_REMOVED && usbd_disable(usbd_ctx) != 0) {
		LOG_ERR("Failed to disable device support");
	}
}

static int add_configuration(const enum usbd_speed speed, struct usbd_config_node *const cfg)
{
	int err;

	err = usbd_add_configuration(&bridge_usbd, speed, cfg);
	if (err != 0) {
		return err;
	}

	/*
	 * The Linux driver sends its commands to interface 0, so the
	 * i2c-tiny-usb interface is registered first.
	 */
	err = usbd_register_class(&bridge_usbd, "i2c_tiny_usb", speed, 1);
	if (err != 0) {
		return err;
	}

	if (IS_ENABLED(CONFIG_APP_DFU)) {
		err = usbd_register_class(&bridge_usbd, "dfu_runtime", speed, 1);
		if (err != 0) {
			return err;
		}
	}

	usbd_device_set_code_triple(&bridge_usbd, speed, 0, 0, 0);

	return 0;
}

int bridge_add_descriptors(struct usbd_context *const uds_ctx)
{
	int err;

	err = usbd_add_descriptor(uds_ctx, &bridge_lang);
	if (err != 0) {
		return err;
	}

	err = usbd_add_descriptor(uds_ctx, &bridge_mfr);
	if (err != 0) {
		return err;
	}

	err = usbd_add_descriptor(uds_ctx, &bridge_product);
	if (err != 0) {
		return err;
	}

#ifdef CONFIG_HWINFO_HAS_DRIVER
	err = usbd_add_descriptor(uds_ctx, &bridge_sn);
	if (err != 0) {
		return err;
	}
#endif

	if (IS_ENABLED(CONFIG_APP_USBD_WINUSB)) {
		err = bridge_msosv2_add(uds_ctx);
		if (err != 0) {
			return err;
		}
	}

	return usbd_device_set_bcd_device(uds_ctx, BRIDGE_BCD_DEVICE);
}

static int setup_device(void)
{
	int err;

	err = bridge_add_descriptors(&bridge_usbd);
	if (err != 0) {
		return err;
	}

	if (USBD_SUPPORTS_HIGH_SPEED && usbd_caps_speed(&bridge_usbd) == USBD_SPEED_HS) {
		err = add_configuration(USBD_SPEED_HS, &hs_config);
		if (err != 0) {
			return err;
		}
	}

	err = add_configuration(USBD_SPEED_FS, &fs_config);
	if (err != 0) {
		return err;
	}

	return usbd_msg_register_cb(&bridge_usbd, msg_cb);
}

int main(void)
{
	int err;

#ifdef CONFIG_APP_BOOT_BANNER
	printk("*** i2c-tiny-usb bridge " BANNER_VERSION " ***\n");
#endif

	err = setup_device();
	if (err != 0) {
		LOG_ERR("Failed to set up USB device (%d)", err);
		return err;
	}

	err = usbd_init(&bridge_usbd);
	if (err != 0) {
		LOG_ERR("Failed to initialize device support (%d)", err);
		return err;
	}

	if (!usbd_can_detect_vbus(&bridge_usbd)) {
		err = usbd_enable(&bridge_usbd);
		if (err != 0) {
			LOG_ERR("Failed to enable device support (%d)", err);
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_APP_DFU)) {
		bridge_dfu_confirm();
	}

	LOG_INF("i2c-tiny-usb bridge ready");

	return 0;
}
