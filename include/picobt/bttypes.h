/**
 * @file bttypes.h
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
 * @brief Defines all the types used by libpicobt.
 * 
 * Note: no associated C file.
 * Defines the types used in this library. Note that some are OS-specific, thus
 * for portable code members should not be accessed directly.
 */

#ifndef __BTTYPES_H__
#define __BTTYPES_H__

#ifdef WINDOWS
#include <winsock2.h>
#include <ws2bth.h>
#include <stdint.h>

#else // LINUX
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#endif

#include "bterror.h"

#define DEVICE_NAME_BUFFER_SIZE 256
#define SERVICE_NAME_BUFFER_SIZE 256
#define SERVICE_DESCRIPTION_BUFFER_SIZE 256

/// Types of Bluetooth inquiry, used in the type field of `bt_inquiry_t`.
enum bt_inquiry_type {
	/// A device inquiry.
	BT_INQUIRY_DEVICES,
	/// A service inquiry.
	BT_INQUIRY_SERVICES,
};

/// Represents a 48-bit Bluetooth hardware (MAC) address.
#ifdef WINDOWS
typedef struct {
#else // LINUX
typedef struct __attribute__((packed)) {
#endif
	uint8_t b[6];
} bt_addr_t;

/// Represents a remote Bluetooth device.
typedef struct {
	/// The device's Bluetooth hardware address
	bt_addr_t address;
	/// The device's human-readable name
	const char *name;
	/**
	 * The device's class-of-device bits.
	 * Use the BT_COD_* macros and constants defined in btmain.h to interpret.
	 */
	uint32_t cod;
} bt_device_t;

/**
 * Generic structure that stores state for a variety of Bluetooth inquiry
 * sessions -- currently device discovery and service discovery.
 * The contents of this structure should be manipulated only through the
 * `bt_inquiry_*` functions.
 */
typedef struct {
	/// The type of inquiry -- see `enum bt_inquiry_type`.
	int type;
	/// Error status indicator.
	int error;
	
#ifdef WINDOWS
	WSAQUERYSET *qs;
	DWORD qsSize;
	ULONG flags;
	HANDLE hLookup;
	
#else // LINUX
	union {
		/// members for device inquiry
		struct {
			int dev_id;
			int socket;
			int flags;
			int count;
			inquiry_info *info;
			inquiry_info *current;
		} dev;
		// members for service discovery
		struct {
			sdp_session_t *session;
			sdp_list_t *response;
		} sdp;
	};
	char *nameBuffer;
	char *descBuffer;
#endif
} bt_inquiry_t;

/// Represent a UUID, used to identify Bluetooth services.
#ifdef WINDOWS
typedef struct {
#else // LINUX
typedef struct __attribute__((packed)) {
#endif
	uint8_t b[16];
} bt_uuid_t;

struct _bt_sdp_sequence_t;

/// An element in an SDP record.
typedef struct {
	uint8_t type, sizeDesc;
	uint32_t size, recordSize;
	union {
		uint8_t u8;
		uint16_t u16;
		uint32_t u32;
		uint64_t u64;
		int8_t i8;
		int16_t i16;
		int32_t i32;
		int64_t i64;
		uint16_t uuid16;
		uint32_t uuid32;
		bt_uuid_t uuid;
		char *text;
		uint8_t boolean;
		char *url;
		struct _bt_sdp_sequence_t *seq;
	} value;
} bt_sdp_element_t;

/// A sequence element in an SDP record, which contains further elements itself.
typedef struct _bt_sdp_sequence_t {
	bt_sdp_element_t head;
	struct _bt_sdp_sequence_t *tail;
} bt_sdp_sequence_t;

/// Represents an SDP record in an easy-to-parse structure.
typedef struct {
	bt_sdp_element_t root;
} bt_sdp_record_t;

/// Represents a Bluetooth service on a remote device.
typedef struct {
	/// The service's human-readable name.
	const char *name;
	/// The service's human-readable description.
	const char *description;
	/// The service's UUID.
	bt_uuid_t uuid;
	/// Which RFCOMM port this service is bound to.
	int port;
	/// The parsed SDP record for this service.
	bt_sdp_record_t *record;
} bt_service_t;

/// A cross-platform type for Bluetooth sockets.
typedef struct {
#ifdef WINDOWS
	SOCKET s;
	
#else // LINUX
	int s;
	
#endif
} bt_socket_t;

#endif //__BTTYPES_H__
