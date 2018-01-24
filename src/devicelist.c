/**
 * @file devicelist.c
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
 * @brief Defines functions for working with lists of Bluetooth devices.
 * 
 * Defines functions for working with Bluetooth device lists, used by Pico for
 * maintaining and sending messages to Bluetooth-enabled Pico-paired devices.
 */

#include <stdio.h>
#include <stdlib.h>

#include "picobt/bt.h"
#include "picobt/devicelist.h"
#include "picobt/log.h"

/**
 * Create a new empty device list. Free using {@link bt_list_delete}.
 * 
 * @return Pointer to the new list.
 */
bt_device_list_t *bt_list_new(void) {
	bt_device_list_t *list = malloc(sizeof(bt_device_list_t));
	list->next = NULL;
	return list;
}

/**
 * Free memory associated with a device list.
 * After calling this function the list is empty.
 * 
 * @param list Pointer to the list to free.
 */
void bt_list_delete(bt_device_list_t *list) {
	// validate pointer
	if (list == NULL)
		return;
	
	// free the tail
	if (list->next != NULL) {
		bt_list_delete(list->next);
		list->next = NULL;
	}
	free(list);
}

/**
 * Load a device list from a file.
 * 
 * @param filename The file to load from.
 * @param list Pointer to the device list to load into.
 */
bt_err_t bt_list_load(bt_device_list_t *list, const char *filename) {
	bt_addr_t address;
	char line[100];
	FILE *f;
	
	// validate pointers
	if (filename == NULL || list == NULL)
		return BT_ERR_BAD_PARAM;
	
	// open the file
	f = fopen(filename, "r");
	if (f == NULL) {
		list->next = NULL;
		return BT_ERR_FILE_NOT_FOUND;
	}
	
	// read it line-by-line and add to the list
	while (fgets(line, sizeof(line), f)) {
		// check for and remove the terminating new-line character
		if (line[BT_ADDRESS_LENGTH-1] != '\n')
			continue;
		line[BT_ADDRESS_LENGTH-1] = 0;
		
		// convert the string record to an address
		if (BT_SUCCESS == bt_str_to_addr(line, &address)) {
			// valid, add it to the list
			bt_list_add_device(list, &address);
		}
	}
	
	// close the file
	fclose(f);
	
	return BT_SUCCESS;
}

/**
 * Store a device list in a file.
 * @param filename The file to write to.
 * @param list Pointer to the device list to save.
 */
bt_err_t bt_list_save(bt_device_list_t *list, const char *filename) {
	char str[BT_ADDRESS_LENGTH];
	bt_iterator_t iterator;
	bt_addr_t address;
	FILE *f;
	
	// validate pointers
	if (filename == NULL || list == NULL)
		return BT_ERR_BAD_PARAM;
	
	// open the file
	f = fopen(filename, "w+");
	if (f == NULL)
		return BT_ERR_FILE_NOT_FOUND;
	
	// iterate over each list item
	bt_iterate_list(&iterator, list);
	while (BT_SUCCESS == bt_get_next_device(&iterator, &address)) {
		// convert the address to string form and output as a line
		bt_addr_to_str(&address, str);
		fprintf(f, "%s\n", str);
	}
	
	// close the file
	fclose(f);
	
	return BT_SUCCESS;
}

/**
 * Add a device to the given device list. If it is already in the list, it will
 * not be added again.
 * 
 * @param list Pointer to the device list to add to.
 * @param address Pointer to the Bluetooth address of the device to add.
 */
void bt_list_add_device(bt_device_list_t *list, bt_addr_t *address) {
	// validate pointers
	if (list == NULL || address == NULL)
		return;
	
	// check it isn't already in there
	while (list->next != NULL) {
		if (bt_addr_equals(&list->address, address))
			return;
		list = list->next;
	}
	
	// add the new item on the end
	list->next = malloc(sizeof(bt_device_list_t));
	list->address = *address;
	list->next->next = NULL;
}

/**
 * See whether a list is empty.
 * 
 * @return true if empty, false otherwise.
 */
bool bt_list_is_empty(bt_device_list_t *list) {
	return (list->next == NULL);
}

/**
 * Get the size of the device list. This has to traverse the entire linked-list
 * to find out, so is slow (scales linearly with the number of devices in the
 * list).
 *
 * @param list The device linked list.
 * @return The number of items in the linked list.
 */
int bt_get_list_size(const bt_device_list_t *list) {
	bt_iterator_t iterator;
	int list_size = 0;
	bt_err_t e;
	bt_addr_t address;

	bt_iterate_list(&iterator, list);
	e = bt_get_next_device(&iterator, &address);
	while (e == BT_SUCCESS) {
		list_size++;
		e = bt_get_next_device(&iterator, &address);
	}

	return list_size;
}

/**
 * Begin iteration of a list. Follow with calls to {@link bt_get_next_device}.
 * 
 * @param iterator Pointer to an iterator. Current state will be overwritten.
 * @param list Pointer to the list to iterate through.
 */
void bt_iterate_list(bt_iterator_t *iterator, const bt_device_list_t *list) {
	if (iterator == NULL)
		return;
	iterator->list = list;
	iterator->item = list;
}

/**
 * Rewind iteration of a list. The next call to {@link bt_get_next_device} will
 * obtain the first item.
 * 
 * @param iterator Pointer to an iterator. If the iterator is in use its current
 *                 position will be lost.
 */
void bt_iterate_rewind(bt_iterator_t *iterator) {
	if (iterator == NULL)
		return;
	iterator->item = iterator->list;
}

/**
 * Get the next item (Bluetooth address) from a device iterator, and advance
 * the iterator's position.
 * 
 * @param iterator Pointer to the iterator.
 * @param address Pointer to an address. Will be written to.
 */
bt_err_t bt_get_next_device(bt_iterator_t *iterator, bt_addr_t *address) {
	// validate parameters
	if (iterator == NULL || address == NULL)
		return BT_ERR_BAD_PARAM;
	if (iterator->item == NULL)
		return BT_ERR_BAD_PARAM;
	// check if we're at the end of the list
	if (iterator->item->next == NULL)
		return BT_ERR_END_OF_ENUM;
	*address = iterator->item->address;
	iterator->item = iterator->item->next;
	return BT_SUCCESS;
}

/**
 * Helper function for Pico, to send a message to all devices in the given list.
 * 
 * @param list Pointer to the list of devices to send to.
 * @param service Pointer to the service UUID to send to.
 * @param message Pointer to the message to send.
 * @param length Length of the message to send.
 */
void bt_send_to_list(const bt_device_list_t *list, const bt_uuid_t *service,
						const void *message, size_t length) {
	bt_iterator_t iterator;
	bt_addr_t address;
	bt_socket_t socket;
	char addressStr[BT_ADDRESS_LENGTH];
	bt_err_t e;
	
	// validate parameters
	if (list == NULL || service == NULL || message == NULL || length == 0)
		return;
	
	// try each device in turn
	bt_iterate_list(&iterator, list);
	while (BT_SUCCESS == bt_get_next_device(&iterator, &address)) {
		bt_addr_to_str(&address, addressStr);
		LOG("Trying bluetooth device %s\n", addressStr);
		// connect to the Pico
		if (BT_SUCCESS == (e = bt_connect_to_service(&address, service, &socket))) {
			bt_write(&socket, message, length);
			// close the connection
			bt_disconnect(&socket);
		} else {
			if (e == BT_ERR_CONNECTION_FAILURE) {
				bt_disconnect(&socket);
			}
			LOG("error %d\n", e);
		}
	}
	
}


