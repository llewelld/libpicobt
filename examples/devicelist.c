/**
 * Test the device list functions
 */

#include <stdio.h>
#include "picobt/bt.h"
#include "picobt/devicelist.h"

#define TEST1 "11:22:33:44:55:66"
#define TEST2 "aa:bb:cc:dd:ee:ff"
#define TESTFILE "devicelist.txt"

void dumpListContents(bt_device_list_t *list);


int main(void) {
	bt_device_list_t *list;
	bt_addr_t addr1, addr2;
	
	bt_str_to_addr(TEST1, &addr1);
	bt_str_to_addr(TEST2, &addr2);
	
	printf("Creating new device list\n");
	list = bt_list_new();
	dumpListContents(list);
	
	printf("\nAdding item %s\n", TEST1);
	bt_list_add_device(list, &addr1);
	dumpListContents(list);
	
	printf("\nAdding item again %s\n", TEST1);
	bt_list_add_device(list, &addr1);
	dumpListContents(list);
	
	printf("\nAdding item %s\n", TEST2);
	bt_list_add_device(list, &addr2);
	dumpListContents(list);
	
	printf("\nAdding item again %s\n", TEST2);
	bt_list_add_device(list, &addr2);
	dumpListContents(list);
	
	printf("\nWriting to %s\n", TESTFILE);
	bt_list_save(list, TESTFILE);
	
	printf("\nFreeing list\n");
	bt_list_delete(list);
	
	printf("\nLoading from %s\n", TESTFILE);
	list = bt_list_new();
	bt_list_load(list, TESTFILE);
	dumpListContents(list);
	bt_list_delete(list);
	
	printf("\nDone.\n");

	return 0;
}

void dumpListContents(bt_device_list_t *list) {
	char str[BT_ADDRESS_LENGTH];
	bt_iterator_t iterator;
	bt_addr_t address;
	
	printf("- list contents:\n");
	bt_iterate_list(&iterator, list);
	while (BT_SUCCESS == bt_get_next_device(&iterator, &address)) {
		bt_addr_to_str(&address, str);
		printf("  - %s\n", str);
	}
}
