/**
 * @file test_btutil.c
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
 * @brief Test the functions in btutil.c
 */

#include <stdlib.h>
#include <ctype.h>
#include <check.h>
#include "picobt/devicelist.h"
#include "picobt/btmain.h"
#include "picobt/btutil.h"

#define ADDR1 "11:22:33:44:55:66"
#define ADDR2 "aa:bb:cc:dd:ee:ff"
#define FILE_TO_SAVE "devicelist.txt"

START_TEST (base_device_list)
{
    char str[BT_ADDRESS_LENGTH];
    bt_device_list_t *list;
    bt_addr_t addr1;
    bt_addr_t addr2;
    bt_addr_t device;
    bt_iterator_t iterator;
    
    bt_str_to_addr(ADDR1, &addr1);
    bt_str_to_addr(ADDR2, &addr2);
    
    list = bt_list_new();
    ck_assert(bt_list_is_empty(list));

    bt_iterate_list(&iterator, list);
	ck_assert(bt_get_next_device(&iterator, &device) == BT_ERR_END_OF_ENUM);
	ck_assert(bt_get_next_device(&iterator, &device) == BT_ERR_END_OF_ENUM);

	// Adding one device
    bt_list_add_device(list, &addr1);
	bt_iterate_rewind(&iterator);   
	ck_assert(bt_get_next_device(&iterator, &device) == BT_SUCCESS);
    bt_addr_to_str(&device, str);
	ck_assert_str_eq(str, ADDR1);
	ck_assert(bt_get_next_device(&iterator, &device) == BT_ERR_END_OF_ENUM);


    // Adding same item
	bt_list_add_device(list, &addr1);
	bt_iterate_rewind(&iterator);   
	ck_assert(bt_get_next_device(&iterator, &device) == BT_SUCCESS);
    bt_addr_to_str(&device, str);
	ck_assert_str_eq(str, ADDR1);
	ck_assert(bt_get_next_device(&iterator, &device) == BT_ERR_END_OF_ENUM);
    
    // Adding new item
	bt_list_add_device(list, &addr2);
	bt_iterate_rewind(&iterator);   
	ck_assert(bt_get_next_device(&iterator, &device) == BT_SUCCESS);
    bt_addr_to_str(&device, str);
	ck_assert_str_eq(str, ADDR1);
	ck_assert(bt_get_next_device(&iterator, &device) == BT_SUCCESS);
    bt_addr_to_str(&device, str);
	ck_assert_str_eq(str, ADDR2);
	ck_assert(bt_get_next_device(&iterator, &device) == BT_ERR_END_OF_ENUM);
   
   	// Adding same item	
    bt_list_add_device(list, &addr2);
	bt_iterate_rewind(&iterator);   
	ck_assert(bt_get_next_device(&iterator, &device) == BT_SUCCESS);
    bt_addr_to_str(&device, str);
	ck_assert_str_eq(str, ADDR1);
	ck_assert(bt_get_next_device(&iterator, &device) == BT_SUCCESS);
    bt_addr_to_str(&device, str);
	ck_assert_str_eq(str, ADDR2);
	ck_assert(bt_get_next_device(&iterator, &device) == BT_ERR_END_OF_ENUM);
    
    bt_list_delete(list);
    
}
END_TEST

START_TEST (device_list_save_load)
{
    char str[BT_ADDRESS_LENGTH];
    
	bt_device_list_t *list;
    bt_device_list_t *loadedlist;
    bt_addr_t addr1;
    bt_addr_t addr2;
    bt_addr_t device;
    bt_iterator_t iterator;
    
    bt_str_to_addr(ADDR1, &addr1);
    bt_str_to_addr(ADDR2, &addr2);
    
    list = bt_list_new();
    bt_list_add_device(list, &addr1);
	bt_list_add_device(list, &addr2);

	remove(FILE_TO_SAVE);

	bt_list_save(list, FILE_TO_SAVE);

	bt_list_delete(list);

	loadedlist = bt_list_new();
	bt_list_load(loadedlist, FILE_TO_SAVE);
	bt_iterate_list(&iterator, loadedlist);   
	ck_assert(bt_get_next_device(&iterator, &device) == BT_SUCCESS);
    bt_addr_to_str(&device, str);
	ck_assert_str_eq(str, ADDR1);
	ck_assert(bt_get_next_device(&iterator, &device) == BT_SUCCESS);
    bt_addr_to_str(&device, str);
	ck_assert_str_eq(str, ADDR2);
	ck_assert(bt_get_next_device(&iterator, &device) == BT_ERR_END_OF_ENUM);

	bt_list_delete(loadedlist);

}
END_TEST

TCase *libpicobt_devicelist_testcase(void) {
    TCase *tcase = tcase_create("devicelist");
    
    tcase_add_test(tcase, base_device_list);
    tcase_add_test(tcase, device_list_save_load);
    
    return tcase;
}

