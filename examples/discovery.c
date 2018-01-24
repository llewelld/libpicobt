/**
 * A test program that performs a Bluetooth device discovery
 * and lists the results.
 * 
 * Command line:   no args
 */

#include <stdio.h>
#include "picobt/bt.h"

int main() {
	bt_err_t e;
	bt_inquiry_t inquiry;
	bt_device_t device;
	char addr[BT_ADDRESS_LENGTH];
	
	printf("libpicobt device discovery test\n");
	
	// initialise
	e = bt_init();
	if (e) {
		printf("Error: bt_init failed, error number %d\n", e);
		return 1;
	}
	
	// start a device inquiry
	printf("Finding nearby devices...\n");
	e = bt_inquiry_begin(&inquiry, 0);
	if (e) {
		printf("Error: Could not enumerate cached devices, error number %d\n\n", e);
	} else {
		e = bt_inquiry_next(&inquiry, &device);
		if (e == BT_ERR_END_OF_ENUM) {
			printf("- none\n");
		} else {
			while (e == BT_SUCCESS) {
				bt_addr_to_str(&device.address, addr);
				printf("- %s - %s\n", addr, device.name);
				e = bt_inquiry_next(&inquiry, &device);
			}
			if (e != BT_ERR_END_OF_ENUM) {
				printf("Error: bt_inquiry_next failed, error number %d\n", e);
			}
		}
		bt_inquiry_end(&inquiry);
		printf("\n");
	}
	
	// clean up	
	bt_exit();
	
	return 0;
}
