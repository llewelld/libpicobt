/**
 * @file btutil.h
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
 * @brief Header for btutil.c
 * 
 * These are utility functions for converting between types.
 */

#ifndef __BTUTIL_H__
#define __BTUTIL_H__

#include <stdbool.h>
#include "bttypes.h"

/// The maximum length of a string formatted with BT_ADDRESS_FORMAT.
#define BT_ADDRESS_FORMAT_MXSIZE (18)
/// The maximum length of a string formatted with BT_ADDRESS_FORMAT_COMPACT.
#define BT_ADDRESS_FORMAT_COMPACT_MAXSIZE (13)
/// The maximum length of a string formatted with BT_UUID_FORMAT.
#define BT_UUID_FORMAT_MAXSIZE (37)

bool bt_addr_equals(const bt_addr_t *a1, const bt_addr_t *a2);

void bt_addr_to_str(const bt_addr_t *addr, char *out);
bt_err_t bt_str_to_addr(const char *str, bt_addr_t *addr);
void bt_addr_to_str_compact(const bt_addr_t *addr, char *str);
bt_err_t bt_str_compact_to_addr(const char *str, bt_addr_t *addr);

void bt_uuid_to_str(const bt_uuid_t *uuid, char *str);
bt_err_t bt_str_to_uuid(const char *str, bt_uuid_t *uuid);

#ifdef WINDOWS
// Windows-specific stuff
void bt_addr_to_bdaddr(const bt_addr_t *addr, BTH_ADDR *bdAddr);
void bt_bdaddr_to_addr(const BTH_ADDR bdaddr, bt_addr_t *addr);
void bt_uuid_to_guid(const bt_uuid_t *uuid, GUID *guid);
#else
// Linux-specific stuff
void bt_uuid_to_uuid(const bt_uuid_t *uuid, uuid_t *uuidt);
void bt_uuidt_to_uuid(const uuid_t *uuidt, bt_uuid_t *uuid);
void bt_addr_to_bdaddr(const bt_addr_t *addr, bdaddr_t *bdaddr);
void bt_bdaddr_to_addr(const bdaddr_t *bdaddr, bt_addr_t *addr);
#endif

#endif //__BTUTIL_H__
