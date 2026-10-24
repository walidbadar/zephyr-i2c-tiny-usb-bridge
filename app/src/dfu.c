/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-FileCopyrightText: Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dfu/mcuboot.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/usb/usbd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dfu, CONFIG_APP_LOG_LEVEL);

#include "bridge.h"

/*
 * In DFU mode the device enumerates with a configuration that only has the
 * DFU interface of the built-in flash backend, serving the slot-1 partition.
 */
USBD_DEVICE_DEFINE(dfu_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), CONFIG_APP_USBD_DFU_VID,
		   CONFIG_APP_USBD_DFU_PID);

USBD_DESC_CONFIG_DEFINE(dfu_fs_cfg_desc, "DFU FS Configuration");
USBD_DESC_CONFIG_DEFINE(dfu_hs_cfg_desc, "DFU HS Configuration");

#define DFU_CFG_ATTRIB (IS_ENABLED(CONFIG_APP_USBD_SELF_POWERED) ? USB_SCD_SELF_POWERED : 0)

USBD_CONFIGURATION_DEFINE(dfu_fs_config, DFU_CFG_ATTRIB, CONFIG_APP_USBD_MAX_POWER,
			  &dfu_fs_cfg_desc);
USBD_CONFIGURATION_DEFINE(dfu_hs_config, DFU_CFG_ATTRIB, CONFIG_APP_USBD_MAX_POWER,
			  &dfu_hs_cfg_desc);

static void dfu_reboot(struct k_work *work)
{
	sys_reboot(SYS_REBOOT_COLD);
}

static K_WORK_DELAYABLE_DEFINE(dfu_reboot_work, dfu_reboot);

static void dfu_msg_cb(struct usbd_context *const usbd_ctx, const struct usbd_msg *const msg)
{
	int err;

	LOG_INF("DFU USBD message: %s", usbd_msg_type_string(msg->type));

	if (usbd_can_detect_vbus(usbd_ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY && usbd_enable(usbd_ctx) != 0) {
			LOG_ERR("Failed to enable device support");
		}

		if (msg->type == USBD_MSG_VBUS_REMOVED && usbd_disable(usbd_ctx) != 0) {
			LOG_ERR("Failed to disable device support");
		}
	}

	if (msg->type == USBD_MSG_DFU_DOWNLOAD_COMPLETED) {
		/* MCUboot reverts the image unless it confirms itself after booting */
		err = boot_request_upgrade(BOOT_UPGRADE_TEST);
		if (err != 0) {
			LOG_ERR("Failed to request upgrade (%d)", err);
			return;
		}

		LOG_INF("Download completed, rebooting");
		k_work_schedule(&dfu_reboot_work, K_MSEC(CONFIG_APP_DFU_REBOOT_DELAY));
	}
}

static int dfu_add_configuration(const enum usbd_speed speed,
				 struct usbd_config_node *const cfg)
{
	int err;

	err = usbd_add_configuration(&dfu_usbd, speed, cfg);
	if (err != 0) {
		return err;
	}

	err = usbd_register_class(&dfu_usbd, "dfu_dfu", speed, 1);
	if (err != 0) {
		return err;
	}

	usbd_device_set_code_triple(&dfu_usbd, speed, 0, 0, 0);

	return 0;
}

static int dfu_setup_device(void)
{
	int err;

	err = bridge_add_descriptors(&dfu_usbd);
	if (err != 0) {
		return err;
	}

	if (USBD_SUPPORTS_HIGH_SPEED && usbd_caps_speed(&dfu_usbd) == USBD_SPEED_HS) {
		err = dfu_add_configuration(USBD_SPEED_HS, &dfu_hs_config);
		if (err != 0) {
			return err;
		}
	}

	err = dfu_add_configuration(USBD_SPEED_FS, &dfu_fs_config);
	if (err != 0) {
		return err;
	}

	err = usbd_msg_register_cb(&dfu_usbd, dfu_msg_cb);
	if (err != 0) {
		return err;
	}

	return usbd_init(&dfu_usbd);
}

void bridge_dfu_switch(struct usbd_context *const runtime_ctx)
{
	int err;

	LOG_INF("Detach, switching to DFU mode");

	(void)usbd_disable(runtime_ctx);
	(void)usbd_shutdown(runtime_ctx);

	err = dfu_setup_device();
	if (err != 0) {
		LOG_ERR("Failed to set up DFU mode (%d)", err);
		return;
	}

	if (!usbd_can_detect_vbus(&dfu_usbd)) {
		err = usbd_enable(&dfu_usbd);
		if (err != 0) {
			LOG_ERR("Failed to enable DFU mode (%d)", err);
		}
	}
}

void bridge_dfu_confirm(void)
{
	int err;

	if (boot_is_img_confirmed()) {
		return;
	}

	err = boot_write_img_confirmed();
	if (err != 0) {
		LOG_ERR("Failed to confirm image (%d)", err);
		return;
	}

	LOG_INF("Image confirmed");
}
