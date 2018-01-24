/**
 * @file btmain.h
 * 
 * @section LICENSE
 *
 * (C) Copyright Cambridge Authentication Ltd, 2017
 *
 * This file is part of libtt.
 *
 * Libpicobt is free software: you can redistribute it and\/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Libpicobt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License along with libpicobt. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * @brief Header for btmain.c
 * 
 * Declares the core Bluetooth functions and constants, and also everything
 * related to the Bluetooth class-of-device fields.
 */

#ifndef __BTMAIN_H__
#define __BTMAIN_H__

#include "bttypes.h"

/// The length of a Bluetooth address string, xx:xx:xx:xx:xx:xx, including terminating nil.
#define BT_ADDRESS_LENGTH 18
/// The length of a UUID string, including terminating nil.
#define BT_UUID_LENGTH 37

// class-of-device constants and macros

/// Extract the service bits from a CoD value.
#define BT_COD_SERVICE(x)	(((x) >> 13) & 0x7ff)
/// Extract the major class bits from a CoD value.
#define BT_COD_MAJOR(x)		(((x) >> 8) & 0x1f)
/// Extract the minor class bits from a CoD value.
#define BT_COD_MINOR(x)		(((x) >> 2) & 0x3f)
/// Extract the format bits from a CoD value.
#define BT_COD_FORMAT(x)	((x) & 3)

#define BT_COD_SERVICE_INFORMATION		0x400
#define BT_COD_SERVICE_TELEPHONY		0x200
#define BT_COD_SERVICE_AUDIO			0x100
#define BT_COD_SERVICE_OBJECT_TRANSFER	0x080
#define BT_COD_SERVICE_CAPTURING		0x040
#define BT_COD_SERVICE_RENDERING		0x020
#define BT_COD_SERVICE_NETWORKING		0x010
#define BT_COD_SERVICE_POSITIONING		0x008

#define BT_COD_FORMAT_1					0x0

/// Enumeration of the Bluetooth CoD major class values.
enum bt_cod_major_class_t {
	BT_COD_MAJOR_MISCELLANEOUS = 0,
	BT_COD_MAJOR_COMPUTER,
	BT_COD_MAJOR_PHONE,
	BT_COD_MAJOR_NETWORK,
	BT_COD_MAJOR_AUDIO_VIDEO,
	BT_COD_MAJOR_PERIPHERAL,
	BT_COD_MAJOR_IMAGING,
	BT_COD_MAJOR_WEARABLE,
	BT_COD_MAJOR_TOY,
	BT_COD_MAJOR_HEALTH,
	BT_COD_MAJOR_UNCATEGORISED = 31
};

/**
 * Enumeration of the Bluetooth CoD minor class values.
 * Note that this contains many entries with the same value.
 */
enum bt_code_minor_class_t {
	BT_COD_MINOR_UNCATEGORISED = 0,
	// computers
	BT_COD_MINOR_DESKTOP = 1,
	BT_COD_MINOR_SERVER,
	BT_COD_MINOR_LAPTOP,
	BT_COD_MINOR_HANDHELD,
	BT_COD_MINOR_PALM_SIZED,
	BT_COD_MINOR_WATCH,
	// phones
	BT_COD_MINOR_CELLULAR = 1,
	BT_COD_MINOR_CORDLESS,
	BT_COD_MINOR_SMARTPHONE,
	BT_COD_MINOR_MODEM,
	BT_COD_MINOR_COMMON_ISDN_ACCESS,
	// network points
	BT_COD_MINOR_AVAILABLE = 0,
	BT_COD_MINOR_17_PERCENT,
	BT_COD_MINOR_33_PERCENT,
	BT_COD_MINOR_50_PERCENT,
	BT_COD_MINOR_67_PERCENT,
	BT_COD_MINOR_83_PERCENT,
	BT_COD_MINOR_99_PERCENT,
	BT_COD_MINOR_UNAVAILABLE,
	// audio/video
	BT_COD_MINOR_HEADSET = 1,
	BT_COD_MINOR_HANDS_FREE,
	BT_COD_MINOR_MICROPHONE = 4,
	BT_COD_MINOR_LOUDSPEAKER,
	BT_COD_MINOR_HEADPHONES,
	BT_COD_MINOR_PORTABLE_AUDIO,
	BT_COD_MINOR_CAR_AUDIO,
	BT_COD_MINOR_SET_TOP_BOX,
	BT_COD_MINOR_HIFI_AUDIO,
	BT_COD_MINOR_VCR,
	BT_COD_MINOR_VIDEO_CAMERA,
	BT_COD_MINOR_CAMCORDER,
	BT_COD_MINOR_VIDEO_MONITOR,
	BT_COD_MINOR_VIDEO_DISPLAY_AND_LOUDSPEAKER,
	BT_COD_MINOR_VIDEO_CONFERENCING,
	BT_COD_MINOR_GAMING = 18,
	// peripheral
	// imaging
	// wearable
	BT_COD_MINOR_WRIST_WATCH = 1,
	BT_COD_MINOR_PAGER,
	BT_COD_MINOR_JACKET,
	BT_COD_MINOR_HELMET,
	BT_COD_MINOR_GLASSES,
	// toy
	BT_COD_MINOR_ROBOT = 1,
	BT_COD_MINOR_VEHICLE,
	BT_COD_MINOR_DOLL,
	BT_COD_MINOR_CONTROLLER,
	BT_COD_MINOR_GAME,
	// health
	BT_COD_MINOR_BLOOD_PRESSURE = 1,
	BT_COD_MINOR_THERMOMETER,
	BT_COD_MINOR_WEIGHING_SCALE,
	BT_COD_MINOR_GLUCOSE_METER,
	BT_COD_MINOR_PULSE_OXIMETER,
	BT_COD_MINOR_HEART_RATE,
	BT_COD_MINOR_HEALTH_DATA_DISPLAY,
};


/* SETUP */

bt_err_t bt_init();
void bt_exit();
int bt_is_present();

/* DEVICE DISCOVERY */

bt_err_t bt_inquiry_begin(bt_inquiry_t *inquiry, int cached);
bt_err_t bt_inquiry_next(bt_inquiry_t *inquiry, bt_device_t *device);
void bt_inquiry_end(bt_inquiry_t *inquiry);
bt_err_t bt_get_device_name(bt_addr_t * addr);

/* SERVICE DISCOVERY */

bt_err_t bt_services_begin(bt_inquiry_t *inquiry, const bt_addr_t *device, const bt_uuid_t *service_class, int cached);
bt_err_t bt_services_next(bt_inquiry_t *inquiry, bt_service_t *service);
void bt_services_end(bt_inquiry_t *inquiry);
bt_err_t bt_register_service(bt_uuid_t const * service, char const * service_name, bt_socket_t *sock);

/* CONNECTIONS */

bt_err_t bt_connect_to_service(const bt_addr_t *address, const bt_uuid_t *service, bt_socket_t *sock);
bt_err_t bt_connect_to_port(const bt_addr_t *address, unsigned char port, bt_socket_t *sock);
void bt_disconnect(bt_socket_t *socket);
bt_err_t bt_recv(bt_socket_t *socket, void *buffer, size_t *numBytes);
bt_err_t bt_read(bt_socket_t *socket, void *buffer, size_t *numBytes);
bt_err_t bt_send(bt_socket_t *socket, const void *buffer, size_t *numBytes);
bt_err_t bt_write(bt_socket_t *socket, const void *buffer, size_t numBytes);

bt_err_t bt_bind(bt_socket_t * listener);
bt_err_t bt_bind_to_channel(bt_socket_t * listener, uint8_t channel);
bt_err_t bt_listen(bt_socket_t * listener);
bt_err_t bt_accept(bt_socket_t const * listener, bt_socket_t * sock);
bt_err_t bt_accept_with_timeout(bt_socket_t const * listener, bt_socket_t * sock, struct timeval* timeout);
bt_err_t bt_wait_for_connection(bt_uuid_t const * service, char const * service_name, bt_socket_t * sock, struct timeval* timeout);

bt_err_t bt_set_timeout(bt_socket_t *sock, int duration);
uint8_t bt_get_socket_channel(bt_socket_t sock);

#endif //__BTMAIN_H__
