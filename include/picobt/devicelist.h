/**
 * @file devicelist.h
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
 * @brief Header for devicelist.c
 * 
 * Declarations for the Bluetooth device list functions.
 */

#ifndef __LIBPICOBT_DEVICELIST_H__
#define __LIBPICOBT_DEVICELIST_H__

#include "picobt/bttypes.h"
#include "stdbool.h"

/// A list of Bluetooth devices. This is a simple linked list structure.
typedef struct bt_device_list_t {
	/// Pointer to the next item in the list.
	struct bt_device_list_t *next;
	/// The Bluetooth address of this item.
	bt_addr_t address;
} bt_device_list_t;

/// An iterator for Bluetooth device lists.
typedef struct {
	/// The list being iterated through by this iterator.
	const bt_device_list_t *list;
	/// The current position in the list.
	const bt_device_list_t *item;
} bt_iterator_t;

bt_device_list_t *bt_list_new(void);
void bt_list_delete(bt_device_list_t *list);
bt_err_t bt_list_load(bt_device_list_t *list, const char *filename);
bt_err_t bt_list_save(bt_device_list_t *list, const char *filename);
void bt_list_add_device(bt_device_list_t *list, bt_addr_t *address);
bool bt_list_is_empty(bt_device_list_t *list);
int bt_get_list_size(const bt_device_list_t *list);

void bt_iterate_list(bt_iterator_t *iterator, const bt_device_list_t *list);
void bt_iterate_rewind(bt_iterator_t *iterator);
bt_err_t bt_get_next_device(bt_iterator_t *iterator, bt_addr_t *address);
void bt_send_to_list(const bt_device_list_t *list, const bt_uuid_t *service, const void *message, size_t length);

#endif //__LIBPICOBT_DEVICELIST_H__
