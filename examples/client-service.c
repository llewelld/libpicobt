/**
 * A test program that opens a connection to specific address and port, 
 * defined at REMOTE_ADDRESS and CHANNEL, then write the string "Hello"
 * and expect to read the same string back.
 * 
 */
#include <picobt/bt.h>
#include <stdio.h>

#define REMOTE_ADDRESS "00:1a:7d:da:72:00"
#define SERVICE_UUID "465dbfb2-68a2-11e7-907b-a6006ad3dba0"

int main() {
	bt_addr_t remote_addr;
	bt_socket_t sock;
	bt_err_t e;
	int ret = -1;	
	bt_uuid_t uuid;

	printf("Initialising Bluetooth\n");
	e = bt_init();
	if (e != BT_SUCCESS) {
		printf("Error initialising Bluetooth\n");
		goto cleanup;
	}

	bt_str_to_addr(REMOTE_ADDRESS, &remote_addr);
	bt_str_to_uuid(SERVICE_UUID, &uuid);
	printf("Connecting to %s : %s\n", REMOTE_ADDRESS, SERVICE_UUID);
	
	e = bt_connect_to_service(&remote_addr, &uuid, &sock);
	if (e != BT_SUCCESS) {
		printf("Error (%d) connecting to service", e);
		goto cleanup;
	}

	printf("Connected\n");
	
	char buffer[6];
	size_t len = 6;

	printf("Writing \"Hello\"\n");

	e = bt_write(&sock, "Hello", 6);
	if (e != BT_SUCCESS) {
		printf("Error writing to server\n");
		goto cleanup;
	}

	e = bt_read(&sock, buffer, &len);
	if (e != BT_SUCCESS) {
		printf("Error reading from server\n");
		goto cleanup;
	}

	printf("%zd %s\n", len, buffer);

	ret = 0;
cleanup:
	bt_disconnect(&sock);

	bt_exit(); 

	return ret;
}
