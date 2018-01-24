/**
 * A test program that opens an echo server on a specific port
 * This code should be used with the client counterpart client-port.c
 * in a different machine.
 *
 * It will basically open a Bluetooth listening socket on the port defined
 * by CHANNEL, read exactly 6 bytes (client-port send the string "Hello")
 * and reply with the same string. 
 * 
 */

#include <picobt/bt.h>
#include <stdio.h>

#define CHANNEL 15

int main() {
	unsigned char buffer[6];
	bt_err_t e;
	bt_addr_t local_address;
	char bt_mac_address[BT_ADDRESS_LENGTH];
	bt_socket_t listener;
	bt_socket_t sock;
	int ret = -1;

 	printf("Initialising Bluetooth\n");
	e = bt_init();
	if (e != BT_SUCCESS) {
		printf("Error initialising Bluetooth\n");
		goto cleanup;
	}

	bt_get_device_name(&local_address);
	bt_addr_to_str(&local_address, bt_mac_address);
	printf("Local bluetooth address: %s\n", bt_mac_address);

	e = bt_bind_to_channel(&listener, CHANNEL);
	if (e != BT_SUCCESS) {
		printf("Error binding to channel %d\n", CHANNEL);
		goto cleanup;
	}

	e = bt_listen(&listener);
	if (e != BT_SUCCESS) {
		printf("Error setting socket to listen\n");
		goto cleanup;
	}

	printf("Waiting for client...\n");
	e = bt_accept(&listener, &sock);
	if (e != BT_SUCCESS) {
		printf("Error accepting connection\n");
		goto cleanup;
	}

	printf("Client connected\n");

	size_t len = 6;

	e = bt_read(&sock, buffer, &len);
	if (e != BT_SUCCESS) {
		printf("Error reading from client\n");
		goto cleanup;
	}

	printf("Read %zd bytes\n", len);
	printf("Data: %s\n", buffer);

	e = bt_write(&sock, buffer, 6);
	if (e != BT_SUCCESS) {
		printf("Error writing back\n");
		goto cleanup;
	}

	ret = 0;
cleanup:
	bt_disconnect(&sock);

	bt_exit(); 

	return ret;
}
