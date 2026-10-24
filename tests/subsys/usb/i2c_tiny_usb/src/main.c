/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-FileCopyrightText: Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/ztest.h>

#include <usbh_ch9.h>
#include <usbh_device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_i2c_tiny_usb, LOG_LEVEL_INF);

/*
 * The requests below are the ones the Linux i2c-tiny-usb driver sends: all
 * of them are vendor requests to interface 0, except CMD_I2C_IO, which
 * carries the target address in wIndex.
 */
#define CMD_ECHO         0
#define CMD_GET_FUNC     1
#define CMD_SET_DELAY    2
#define CMD_GET_STATUS   3
#define CMD_I2C_IO       4
#define CMD_I2C_IO_BEGIN BIT(0)
#define CMD_I2C_IO_END   BIT(1)

#define STATUS_ADDRESS_ACK 1
#define STATUS_ADDRESS_NAK 2

#define TINY_USB_M_RD BIT(0)

#define REQTYPE_VENDOR_IF_IN                                                              \
	((USB_REQTYPE_DIR_TO_HOST << 7) | (USB_REQTYPE_TYPE_VENDOR << 5) |                   \
	 USB_REQTYPE_RECIPIENT_INTERFACE)
#define REQTYPE_VENDOR_IF_OUT                                                             \
	((USB_REQTYPE_DIR_TO_DEVICE << 7) | (USB_REQTYPE_TYPE_VENDOR << 5) |                 \
	 USB_REQTYPE_RECIPIENT_INTERFACE)

/* AT24 EEPROM emulator on the bridged bus, see boards/native_sim.overlay */
#define EEPROM_ADDR 0x50
#define ABSENT_ADDR 0x10

USBD_DEVICE_DEFINE(test_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), 0x1209, 0x0001);

USBD_DESC_LANG_DEFINE(test_lang);
USBD_CONFIGURATION_DEFINE(test_fs_config, 0, 50, NULL);
USBD_CONFIGURATION_DEFINE(test_hs_config, 0, 50, NULL);

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

static int vendor_in(const uint8_t bRequest, const uint16_t wValue, const uint16_t wIndex,
		     uint8_t *const data, const uint16_t wLength)
{
	struct usb_device *const udev = usbh_device_get_any(&uhs_ctx);
	struct net_buf *buf;
	int err;

	zassert_not_null(udev, "No USB device available");

	buf = usbh_xfer_buf_alloc(udev, wLength);
	zassert_not_null(buf, "Failed to allocate buffer");

	err = k_mutex_lock(&udev->mutex, K_MSEC(200));
	zassert_equal(err, 0, "Failed to lock device");

	err = usbh_req_setup(udev, REQTYPE_VENDOR_IF_IN, bRequest, wValue, wIndex, wLength, buf);
	if (err == 0) {
		zassert_equal(buf->len, wLength, "Short transfer, %u bytes", buf->len);
		memcpy(data, buf->data, wLength);
	}

	k_mutex_unlock(&udev->mutex);
	usbh_xfer_buf_free(udev, buf);

	return err;
}

static int vendor_out(const uint8_t bRequest, const uint16_t wValue, const uint16_t wIndex,
		      const uint8_t *const data, const uint16_t wLength)
{
	struct usb_device *const udev = usbh_device_get_any(&uhs_ctx);
	struct net_buf *buf = NULL;
	int err;

	zassert_not_null(udev, "No USB device available");

	if (wLength != 0U) {
		buf = usbh_xfer_buf_alloc(udev, wLength);
		zassert_not_null(buf, "Failed to allocate buffer");
		net_buf_add_mem(buf, data, wLength);
	}

	err = k_mutex_lock(&udev->mutex, K_MSEC(200));
	zassert_equal(err, 0, "Failed to lock device");

	err = usbh_req_setup(udev, REQTYPE_VENDOR_IF_OUT, bRequest, wValue, wIndex, wLength, buf);

	k_mutex_unlock(&udev->mutex);

	if (buf != NULL) {
		usbh_xfer_buf_free(udev, buf);
	}

	return err;
}

static uint8_t get_status(void)
{
	uint8_t status;

	zassert_ok(vendor_in(CMD_GET_STATUS, 0, 0, &status, sizeof(status)));

	return status;
}

ZTEST(i2c_tiny_usb, test_echo)
{
	uint8_t data[2];

	zassert_ok(vendor_in(CMD_ECHO, 0x1234, 0, data, sizeof(data)));
	zassert_equal(sys_get_le16(data), 0x1234, "Echo mismatch, 0x%04x", sys_get_le16(data));
}

ZTEST(i2c_tiny_usb, test_get_func)
{
	uint8_t data[4];

	zassert_ok(vendor_in(CMD_GET_FUNC, 0, 0, data, sizeof(data)));
	/* I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL */
	zassert_equal(sys_get_le32(data), 0x0eff0009, "Unexpected functionality 0x%08x",
		      sys_get_le32(data));
}

ZTEST(i2c_tiny_usb, test_set_delay)
{
	/* Fast mode, a half-period of 1 us */
	zassert_ok(vendor_out(CMD_SET_DELAY, 1, 0, NULL, 0));
	/* Back to standard mode */
	zassert_ok(vendor_out(CMD_SET_DELAY, 10, 0, NULL, 0));
}

ZTEST(i2c_tiny_usb, test_write_read)
{
	const uint8_t write[] = {0x10, 0xde, 0xad, 0xbe, 0xef};
	const uint8_t reg = write[0];
	uint8_t read[sizeof(write) - 1];

	/* One message: register address followed by the data */
	zassert_ok(vendor_out(CMD_I2C_IO | CMD_I2C_IO_BEGIN | CMD_I2C_IO_END, 0, EEPROM_ADDR,
			      write, sizeof(write)));
	zassert_equal(get_status(), STATUS_ADDRESS_ACK);

	/* Two messages with a repeated start: register address, then a read */
	zassert_ok(vendor_out(CMD_I2C_IO | CMD_I2C_IO_BEGIN, 0, EEPROM_ADDR, &reg, sizeof(reg)));
	zassert_equal(get_status(), STATUS_ADDRESS_ACK);

	zassert_ok(vendor_in(CMD_I2C_IO | CMD_I2C_IO_END, TINY_USB_M_RD, EEPROM_ADDR, read,
			     sizeof(read)));
	zassert_equal(get_status(), STATUS_ADDRESS_ACK);

	zassert_mem_equal(read, &write[1], sizeof(read));
}

ZTEST(i2c_tiny_usb, test_absent_target)
{
	uint8_t data;

	/* The device answers with the requested length, the status reports the NAK */
	zassert_ok(vendor_in(CMD_I2C_IO | CMD_I2C_IO_BEGIN | CMD_I2C_IO_END, TINY_USB_M_RD,
			     ABSENT_ADDR, &data, sizeof(data)));
	zassert_equal(get_status(), STATUS_ADDRESS_NAK);
}

static void *i2c_tiny_usb_setup(void)
{
	int err;

	err = usbh_init(&uhs_ctx);
	zassert_ok(err, "Failed to initialize USB host");

	err = usbh_enable(&uhs_ctx);
	zassert_ok(err, "Failed to enable USB host");

	err = uhc_bus_reset(uhs_ctx.dev);
	zassert_ok(err, "Failed to signal bus reset");

	err = uhc_bus_resume(uhs_ctx.dev);
	zassert_ok(err, "Failed to signal bus resume");

	err = uhc_sof_enable(uhs_ctx.dev);
	zassert_ok(err, "Failed to enable SoF generator");

	err = usbd_add_descriptor(&test_usbd, &test_lang);
	zassert_ok(err, "Failed to add language descriptor");

	if (USBD_SUPPORTS_HIGH_SPEED && usbd_caps_speed(&test_usbd) == USBD_SPEED_HS) {
		err = usbd_add_configuration(&test_usbd, USBD_SPEED_HS, &test_hs_config);
		zassert_ok(err, "Failed to add configuration");

		err = usbd_register_class(&test_usbd, "i2c_tiny_usb", USBD_SPEED_HS, 1);
		zassert_ok(err, "Failed to register class");
	}

	err = usbd_add_configuration(&test_usbd, USBD_SPEED_FS, &test_fs_config);
	zassert_ok(err, "Failed to add configuration");

	err = usbd_register_class(&test_usbd, "i2c_tiny_usb", USBD_SPEED_FS, 1);
	zassert_ok(err, "Failed to register class");

	err = usbd_init(&test_usbd);
	zassert_ok(err, "Failed to initialize device support");

	err = usbd_enable(&test_usbd);
	zassert_ok(err, "Failed to enable device support");

	/* Allow the host time to enumerate the device */
	k_msleep(200);

	return NULL;
}

static void i2c_tiny_usb_teardown(void *f)
{
	zassert_ok(usbd_disable(&test_usbd));
	zassert_ok(usbd_shutdown(&test_usbd));
	zassert_ok(usbh_disable(&uhs_ctx));
}

ZTEST_SUITE(i2c_tiny_usb, NULL, i2c_tiny_usb_setup, NULL, NULL, i2c_tiny_usb_teardown);
