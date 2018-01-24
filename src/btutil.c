/**
 * @file btutil.c
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
 * @brief Handy utility functions.
 * 
 * Handy utility functions. Most of these are for converting between the
 * cross-platform `bt_*` types defined in bttypes.h and the platform's
 * native Bluetooth types.
 */

#include <stdio.h>
#include "picobt/bt.h"
#include "picobt/log.h"

/// Format string for converting Bluetooth addresses to and from text.
#define BT_ADDRESS_FORMAT	"%02x:%02x:%02x:%02x:%02x:%02x"
/// Format string for converting UUIDs to and from text.
#define BT_UUID_FORMAT		"%02x%02x%02x%02x-%02x%02x-%02x%02x-" \
								"%02x%02x-%02x%02x%02x%02x%02x%02x"

/** Format string for converting Bluetooth addresses to and from text, without
    any colons. */
#define BT_ADDRESS_FORMAT_COMPACT "%02X%02X%02X%02X%02X%02X"


/******************************************************************************\
 * COMPARISONS                                                                *
\******************************************************************************/

/**
 * Compare two Bluetooth addresses to see whether they are equal.
 * 
 * @param a1 The first address.
 * @param a2 The second address.
 * 
 * @return `true` if the addresses are equal, `false` otherwise.
 */
bool bt_addr_equals(const bt_addr_t *a1, const bt_addr_t *a2) {
	int i;
	for (i = 0; i < 6; i++)
		if (a1->b[i] != a2->b[i])
			return false;
	return true;
}


/******************************************************************************\
 * ADDRESS CONVERSIONS                                                        *
\******************************************************************************/

/**
 * Turn a Bluetooth address into a hex string, formatted as xx:xx:xx:xx:xx:xx
 * using #BT_ADDRESS_FORMAT.
 * 
 * @param addr Device address to convert.
 * @param str  String to write to. Must be at least 18 chars long (including
 *             terminating nil).
 */
void bt_addr_to_str(const bt_addr_t *addr, char *str) {
	sprintf(str, BT_ADDRESS_FORMAT,
			(unsigned int) addr->b[5],
			(unsigned int) addr->b[4],
			(unsigned int) addr->b[3],
			(unsigned int) addr->b[2],
			(unsigned int) addr->b[1],
			(unsigned int) addr->b[0]);
}

/**
 * Turn a string representation of a Bluetooth address into a {@link bt_addr_t}.
 * 
 * @param str  String to read address from.
 * @param addr {@link bt_addr_t} to write address to.
 * 
 * @return `BT_SUCCESS` if successful, or `BT_ERR_BAD_PARAM` if the
 *         input is malformed
 */
bt_err_t bt_str_to_addr(const char *str, bt_addr_t *addr) {
	unsigned int x[6];
	if (6 != sscanf(str, BT_ADDRESS_FORMAT,
			&x[0], &x[1], &x[2], &x[3], &x[4], &x[5]))
		return BT_ERR_BAD_PARAM;
	addr->b[5] = x[0];
	addr->b[4] = x[1];
	addr->b[3] = x[2];
	addr->b[2] = x[3];
	addr->b[1] = x[4];
	addr->b[0] = x[5];
	return BT_SUCCESS;
}

/**
 * Turn a Bluetooth address into a hex string, formatted as xxxxxxxxxxxx using
 * {@link BT_ADDRESS_FORMAT_COMPACT}.
 * @param addr Device address to convert
 * @param str  String to write to. Must be at least 13 chars long (including
 *             terminating nil).
 */
void bt_addr_to_str_compact(const bt_addr_t *addr, char *str) {
	sprintf(str, BT_ADDRESS_FORMAT_COMPACT,
			(unsigned int) addr->b[5],
			(unsigned int) addr->b[4],
			(unsigned int) addr->b[3],
			(unsigned int) addr->b[2],
			(unsigned int) addr->b[1],
			(unsigned int) addr->b[0]);
}

/**
 * Turn a string representation of a Bluetooth address into a bt_addr_t.
 * @param str	String to read address from
 * @param addr	bt_addr_t to write address to
 * @return BT_SUCCESS if successful, or BT_ERR_BAD_PARAM if input is malformed
 */
bt_err_t bt_str_compact_to_addr(const char *str, bt_addr_t *addr) {
    unsigned int x[6];
    if (6 != sscanf(str, BT_ADDRESS_FORMAT_COMPACT,
            &x[0], &x[1], &x[2], &x[3], &x[4], &x[5]))
        return BT_ERR_BAD_PARAM;
    addr->b[5] = x[0];
    addr->b[4] = x[1];
    addr->b[3] = x[2];
    addr->b[2] = x[3];
    addr->b[1] = x[4];
    addr->b[0] = x[5];
    return BT_SUCCESS;
}

/******************************************************************************\
 * UUID CONVERSIONS                                                           *
\******************************************************************************/

/**
 * Turn a UUID into a string.
 * Format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
 * @param uuid UUID to convert.
 * @param str  String to write to. Must be at least 37 chars long (including
 *             terminating nil).
 */
void bt_uuid_to_str(const bt_uuid_t *uuid, char *str) {
	sprintf(str, BT_UUID_FORMAT,
			uuid->b[ 0], uuid->b[ 1], uuid->b[ 2], uuid->b[ 3],
			uuid->b[ 4], uuid->b[ 5], uuid->b[ 6], uuid->b[ 7],
			uuid->b[ 8], uuid->b[ 9], uuid->b[10], uuid->b[11],
			uuid->b[12], uuid->b[13], uuid->b[14], uuid->b[15]);
}

/**
 * Turn a string representation of a UUID address into a {@link bt_uuid_t}.
 * @param str  String to read UUID from.
 * @param uuid The {@link bt_uuid_t} to write to.
 * @return `BT_SUCCESS` if successful, or `BT_ERR_BAD_PARAM` if the
 *         input is malformed.
 */
bt_err_t bt_str_to_uuid(const char *str, bt_uuid_t *uuid) {
	unsigned int x[16], i;
	if (16 != sscanf(str, BT_UUID_FORMAT,
			&x[ 0], &x[ 1], &x[ 2], &x[ 3],
			&x[ 4], &x[ 5], &x[ 6], &x[ 7],
			&x[ 8], &x[ 9], &x[10], &x[11],
			&x[12], &x[13], &x[14], &x[15]))
		return BT_ERR_BAD_PARAM;
	for (i = 0; i < 16; i++)
		uuid->b[i] = (uint8_t) x[i];
	return BT_SUCCESS;
}


/******************************************************************************\
 * PLATFORM-SPECIFIC CONVERSIONS                                              *
\******************************************************************************/

#ifdef WINDOWS

/**
 * Windows-specific function to convert {@link bt_uuid_t} into a Windows `GUID`.
 * @param uuid Pointer to the libpicobt UUID to convert.
 * @param guid Pointer to the target Windows `GUID` to convert to.
 */
void bt_uuid_to_guid(const bt_uuid_t *uuid, GUID *guid) {
	uint8_t *a = (uint8_t*) uuid;
	uint8_t *b = (uint8_t*) guid;
	// need to do some endian-swapping because Windows uses int fields
	b[0] = a[3];	// |
	b[1] = a[2];	// |_ swap
	b[2] = a[1];	// |
	b[3] = a[0];	// /
	b[4] = a[5];	// \_ swap
	b[5] = a[4];	// /
	b[6] = a[7];	// \_ swap
	b[7] = a[6];	// /
	b[8] = a[8];	// remaining bytes are the same
	b[9] = a[9];
	b[10] = a[10];
	b[11] = a[11];
	b[12] = a[12];
	b[13] = a[13];
	b[14] = a[14];
	b[15] = a[15];
}

/**
 * Windows-specific function to convert a {@link bt_addr_t} into a Windows
 * `BTH_ADDR`.
 * @param addr   Pointer to the libpicobt Bluetooth address to convert.
 * @param bdAddr Pointer to the target Windows `BTH_ADDR` to convert to.
 */
void bt_addr_to_bdaddr(const bt_addr_t *addr, BTH_ADDR *bdAddr) {
	*bdAddr = 0;
	// works on little-endian systems
	memcpy(bdAddr, addr, 6);
}

/**
 * Windows-specific function to convert a Windows `BTH_ADDR` into a
 * {@link bt_addr_t}.
 * @param bdaddr Pointer to the target Windows `BTH_ADDR` to convert to.
 * @param addr   Pointer to the libpicobt Bluetooth address to convert.
 */
void bt_bdaddr_to_addr(const BTH_ADDR bdaddr, bt_addr_t *addr) {
	// again, on Linux they're the same
	memcpy(addr, &bdaddr, 6);
}

#else

/**
 * Linux-specific function to convert a {@link bt_uuid_t} into a BlueZ
 * `uuid_t`.
 * @param uuid  Pointer to the libpicobt {@link bt_uuid_t} to convert from.
 * @param uuidt Pointer to the BlueZ `uuid_t` to convert to.
 */
void bt_uuid_to_uuid(const bt_uuid_t *uuid, uuid_t *uuidt) {
	// on Linux the uuid has a type as well
	uuidt->type = SDP_UUID128;
	memcpy(&uuidt->value, uuid, 16);
}

/**
 * Linux-specific function to convert a BlueZ `uuid_t` into a
 * {@link bt_uuid_t}
 * @param uuidt Pointer to the BlueZ `uuid_t` to convert from.
 * @param uuid  Pointer to the libpicobt {@link bt_uuid_t} to convert to.
 */
void bt_uuidt_to_uuid(const uuid_t *uuidt, bt_uuid_t *uuid) {
	// check the type
	if (uuidt->type == SDP_UUID128) {
		memcpy(uuid, &uuidt->value, 16);
	} else {
		// use the base UUID
		const uint8_t base[] = {0x00, 0x00, 0x10, 0x00, 0x80, 0x00,
								0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb};
		uint32_t value;
		if (uuidt->type == SDP_UUID16) {
			value = uuidt->value.uuid16; // extend with zeros
		} else if (uuidt->type == SDP_UUID32) {
			value = uuidt->value.uuid32;
		} else {
			LOG("bt_uuidt_to_uuid: erroneous uuid_t passed\n");
			value = 0;
		}
		uuid->b[0] = (uint8_t) ((value >> 24) & 0xff);
		uuid->b[1] = (uint8_t) ((value >> 16) & 0xff);
		uuid->b[2] = (uint8_t) ((value >> 8) & 0xff);
		uuid->b[3] = (uint8_t) (value & 0xff);
		memcpy(&uuid->b[4], base, 12);
	}
}

/**
 * Linux-specific function to convert a {@link bt_addr_t} into a BlueZ
 * `bdaddr_t`.
 * @param addr   Pointer to the libpicobt Bluetooth address to convert from.
 * @param bdaddr Pointer to the target `bdaddr_t` to convert to.
 */
void bt_addr_to_bdaddr(const bt_addr_t *addr, bdaddr_t *bdaddr) {
	// again, on Linux they're the same
	memcpy(bdaddr, addr, 6);
}

/**
 * Linux-specific function to convert a BlueZ `bdaddr_t` into a
 * {@link bt_addr_t}.
 * @param bdaddr Pointer to the `bdaddr_t` to convert from.
 * @param addr   Pointer to the target libpicobt Bluetooth address to convert to.
 */
void bt_bdaddr_to_addr(const bdaddr_t *bdaddr, bt_addr_t *addr) {
	// again, on Linux they're the same
	memcpy(addr, bdaddr, 6);
}

#endif

