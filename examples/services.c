/**
 * A small test app to make sure your Bluetooth is working.
 * Lists all the Bluetooth services registered on a remote device.
 * 
 * Command line:   test <address>
 */

#include <stdio.h>
#include "picobt/bt.h"

#define PHONE "64:bc:0c:f9:e8:6c"

int main(int argc, char **argv) {
	char *addressStr;
	bt_addr_t address;
	bt_inquiry_t inquiry;
	bt_service_t service;
	bt_err_t e;
	char uuid[BT_UUID_LENGTH];
	
	if (argc > 1) {
		addressStr = argv[1];
	} else {
		addressStr = PHONE;
	}
	
	printf("libpicobt service inquiry test\n");
	
	e = bt_init();
	if (e) {
		printf("Error: bt_init failed, error number %d\n", e);
		return 1;
	}
	
	if (BT_SUCCESS != bt_str_to_addr(addressStr, &address)) {
		printf("Error: malformed address\n");
		return 1;
	}
	printf("Remote device address: %s\n", addressStr);
	
	e = bt_services_begin(&inquiry, &address, NULL, 0);
	if (e) {
		printf("Error: bt_services_begin failed, error number %d\n", e);
	} else {
		e = bt_services_next(&inquiry, &service);
		while (e == BT_SUCCESS) {
			bt_uuid_to_str(&service.uuid, uuid);
			printf("Service: %s - %s\n", uuid, service.name);
			e = bt_services_next(&inquiry, &service);
		}
		if (e != BT_ERR_END_OF_ENUM) {
			printf("Error: bt_services_next failed, error number %d\n", e);
		}
		bt_services_end(&inquiry);
	}
	
	bt_exit();
	
	return 0;
}
