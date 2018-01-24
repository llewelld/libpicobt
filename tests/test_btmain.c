/**
 * @file test_btmain.c
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
#include "picobt/bt.h"
#include "picobt/bttypes.h"
#include "mock/mockbluez.h"

START_TEST (test_bt_init)
{
	int route_return;

	int get_route (bdaddr_t *bdaddr) {
		ck_assert(bdaddr == NULL);
		return route_return;
	}
	bz_funcs.hci_get_route = get_route;

	route_return = 0;
	ck_assert(bt_init() == BT_SUCCESS);

	route_return = -1;
	ck_assert(bt_init() == BT_ERR_UNSUPPORTED);
	
	route_return = 5;
	ck_assert(bt_is_present() == BT_SUCCESS);

	route_return = -10;
	ck_assert(bt_is_present() == BT_ERR_UNSUPPORTED);

	bt_exit();
}
END_TEST

START_TEST (test_bt_get_device_name)
{
	bt_err_t e;
	bt_addr_t address;
	char addr[BT_ADDRESS_LENGTH];
	
	const int mock_dev_id = 12;

	int get_route (bdaddr_t *bdaddr) {
		ck_assert(bdaddr == NULL);
		return mock_dev_id;
	}
	bz_funcs.hci_get_route = get_route;

	int devba(int dev_id, bdaddr_t *bdaddr) {
		// Check if the same device number was sent
		ck_assert(dev_id == mock_dev_id);
		memcpy(bdaddr->b, "\x13\x71\xda\x7d\x1a\x00", 6);
		return 0;	
	}
	bz_funcs.hci_devba = devba;

	e = bt_get_device_name(&address);
	ck_assert(e == BT_SUCCESS);
		
	bt_addr_to_str(&address, addr);
	ck_assert_str_eq(addr, "00:1a:7d:da:71:13");
}
END_TEST

START_TEST (test_bt_inquiry)
{
	bt_err_t e;
	bt_inquiry_t inquiry;
	bt_device_t device;
	char addr[BT_ADDRESS_LENGTH];
	
	const int mock_dev_id = 12;
	const int mock_sock_number = 555;

	const inquiry_info mock_info[2] = {
		{
			.bdaddr.b = {0x13, 0x71, 0xda, 0x7d, 0x1a, 0x00},
			.pscan_rep_mode = 0x1,
			.pscan_period_mode = 0x2,
			.dev_class = {0x04, 0x01, 0x0c},
			.clock_offset = 0x113e
		},
		{
			.bdaddr.b = {0xa9, 0xaf, 0xbe, 0xae, 0xf8, 0xfc},
			.pscan_rep_mode = 0x1,
			.pscan_period_mode = 0x2,
			.dev_class = {0x04, 0x01, 0x7e},
			.clock_offset = 0x1479
		}
	};

	int get_route (bdaddr_t *bdaddr) {
		ck_assert(bdaddr == NULL);
		return mock_dev_id;
	}
	bz_funcs.hci_get_route = get_route;

	int open_dev(int dev_id) {
		// Check if the same device number was sent
		ck_assert(dev_id == mock_dev_id);
		return mock_sock_number;
		
	}
	bz_funcs.hci_open_dev = open_dev;

	int inquiry_func(int dev_id, int len, int num_rsp, const uint8_t *lap, inquiry_info **ii, long flags) {
		ck_assert(len > 0);
		ck_assert(num_rsp == 256);
		ck_assert(dev_id == mock_dev_id);
		ck_assert(lap == NULL);

		memcpy(*ii, mock_info, 2 * sizeof(inquiry_info));

		return 2;
	}
	bz_funcs.hci_inquiry = inquiry_func;
	
	int read_remote_name(int sock, const bdaddr_t *ba, int len, char *name, int timeout) {
		ck_assert(sock == mock_sock_number);
		if (!memcmp(ba->b, "\x13\x71\xda\x7d\x1a\x00", 6)) {
			strncpy(name, "ACHILLES", len);
			return 0;
		} else if (!memcmp(ba->b, "\xa9\xaf\xbe\xae\xf8\xfc", 6)) {
			strncpy(name, "ARCHIMEDES", len);
			return 0;
		} else {
			return -1;
		}
	}
	bz_funcs.hci_read_remote_name = read_remote_name;
	
	int close_local(int sockfd) {
		ck_assert_int_eq(sockfd, 555);
		return 0;
	}
	bz_funcs.close = close_local;
	
	printf("Finding nearby devices...\n");
	e = bt_inquiry_begin(NULL, 0);
	ck_assert(e == BT_ERR_BAD_PARAM);
	e = bt_inquiry_begin(&inquiry, 0);
	ck_assert(e == BT_SUCCESS);
		
	e = bt_inquiry_next(NULL, &device);
	ck_assert(e == BT_ERR_BAD_PARAM);
	e = bt_inquiry_next(&inquiry, NULL);
	ck_assert(e == BT_ERR_BAD_PARAM);
	bt_service_t service;
	e = bt_services_next(&inquiry, &service);
	ck_assert(e == BT_ERR_BAD_PARAM);
	
	e = bt_inquiry_next(&inquiry, &device);
	ck_assert(e == BT_SUCCESS);
	bt_addr_to_str(&device.address, addr);
	ck_assert_str_eq(addr, "00:1a:7d:da:71:13");
	ck_assert(device.cod == 0x4010c);
	ck_assert_str_eq(device.name, "ACHILLES");
	
	e = bt_inquiry_next(&inquiry, &device);
	ck_assert(e == BT_SUCCESS);
	bt_addr_to_str(&device.address, addr);
	ck_assert_str_eq(addr, "fc:f8:ae:be:af:a9");
	ck_assert(device.cod == 0x4017e);
	ck_assert_str_eq(device.name, "ARCHIMEDES");
	
	e = bt_inquiry_next(&inquiry, &device);
	ck_assert(e == BT_ERR_END_OF_ENUM);
	
	bt_inquiry_end(&inquiry);
	bt_inquiry_end(NULL);
}
END_TEST

START_TEST (test_bt_services)
{
	char *addressStr = "64:bc:0c:f9:e8:6c";
	bt_addr_t address;
	bt_inquiry_t inquiry;
	bt_service_t service;
	bt_err_t e;
	char uuid[BT_UUID_LENGTH];
	
	e = bt_str_to_addr(addressStr, &address);
	ck_assert(e == BT_SUCCESS);

	const int mocksock = 342; 
		
	uint8_t channel = 10;

	sdp_session_t * sdp_connect_local(const bdaddr_t *src, const bdaddr_t *dst, uint32_t flags) {
		sdp_session_t* ret = malloc(sizeof(sdp_session_t));
		
		ck_assert(memcmp(dst, "\x6c\xe8\xf9\x0c\xbc\x64", 6) == 0);
		ck_assert(memcmp(src, BDADDR_ANY, 6) == 0);
		ck_assert(flags == SDP_RETRY_IF_BUSY);

		ret->sock = mocksock;
		ret->state = 0;
		ret->local = 0;
		ret->flags = 1;
		ret->tid = 0;
		ret->priv = (void*) 0x53C937;

		return ret;
	}
	bz_funcs.sdp_connect = sdp_connect_local;

	int search_attr_req(sdp_session_t *session, const sdp_list_t *search, sdp_attrreq_type_t reqtype, const sdp_list_t *attrid_list, sdp_list_t **rsp_list) {
		// Check if the session is the same returned by connect
		ck_assert(session->sock == mocksock);
		ck_assert(session->priv == (void*) 0x53C937);

		// Check parameters
		// For bt_services_begin we are expecting a full search
		ck_assert(reqtype = SDP_ATTR_REQ_RANGE);
		ck_assert(sdp_list_len(attrid_list) == 1);
		ck_assert(sdp_list_len(search) == 1);

		ck_assert(*(uint32_t*) attrid_list->data == 0xffff);
		bt_uuid_t localuuid;
		bt_uuidt_to_uuid(search->data, &localuuid);
		bt_uuid_to_str(&localuuid, uuid);
		ck_assert_str_eq(uuid, "00001002-0000-1000-8000-00805f9b34fb");

		// Write the results.
		// This was collected from a real search on my phone
		sdp_record_t* record;
		uuid_t uuid;
		sdp_list_t* svclass_list;
		sdp_data_t* data;

		// First record
		record = sdp_record_alloc();
		sdp_uuid16_create(&uuid, 0x1801);
		svclass_list = sdp_list_append(NULL, &uuid);
		sdp_set_service_classes(record, svclass_list);

		*rsp_list = sdp_list_append(NULL, record);

		// Seconds record
		record = sdp_record_alloc();
		sdp_uuid16_create(&uuid, 0x1112);
		svclass_list = sdp_list_append(NULL, &uuid);
		sdp_set_service_classes(record, svclass_list);
		data = sdp_data_alloc_with_length(SDP_TEXT_STR8, (void*) "Headset Gateway", 16);
		sdp_attr_add(record, SDP_ATTR_SVCNAME_PRIMARY, data);
	
		*rsp_list = sdp_list_append(*rsp_list, record);
		
		// Third record
		record = sdp_record_alloc();
		sdp_uuid128_create(&uuid, "\xed\x99\x5e\x5a\xc7\xe7\x44\x42\xa6\xee\x7b\xb7\x6d\xf4\x3b\xd");
		svclass_list = sdp_list_append(NULL, &uuid);
		sdp_set_service_classes(record, svclass_list);
		data = sdp_data_alloc_with_length(SDP_TEXT_STR8, (void*) "Pico", 5);
		sdp_attr_add(record, SDP_ATTR_SVCNAME_PRIMARY, data);

		// Access Protocols for third record
		sdp_list_t *aproto, *proto[2], *apseq;
		uuid_t l2cap, rfcomm;

		sdp_uuid16_create(&l2cap, L2CAP_UUID);
		proto[0] = sdp_list_append(0, &l2cap);
		apseq = sdp_list_append(0, proto[0]);
		
		sdp_uuid16_create(&rfcomm, RFCOMM_UUID);
		proto[1] = sdp_list_append(0, &rfcomm);
		proto[1] = sdp_list_append(proto[1], sdp_data_alloc(SDP_UINT8, &channel));
		apseq = sdp_list_append(apseq, proto[1]);

		aproto = sdp_list_append(0, apseq);
		sdp_set_access_protos(record, aproto);

		*rsp_list = sdp_list_append(*rsp_list, record);
		

		return 0;
	}
	bz_funcs.sdp_service_search_attr_req = search_attr_req;
	
	int close (sdp_session_t *session) {
		// Check if the session is the same returned by connect
		ck_assert(session->sock == mocksock);
		ck_assert(session->priv == (void*) 0x53C937);
		return 0;
	}
	bz_funcs.sdp_close = close;

	e = bt_services_begin(&inquiry, NULL, NULL, 0);
	ck_assert(e == BT_ERR_BAD_PARAM);
	e = bt_services_begin(NULL, &address, NULL, 0);
	ck_assert(e == BT_ERR_BAD_PARAM);
	e = bt_services_begin(&inquiry, &address, NULL, 0);
	ck_assert(e == BT_SUCCESS);

	e = bt_services_next(NULL, &service);
	ck_assert(e == BT_ERR_BAD_PARAM);
	e = bt_services_next(&inquiry, NULL);
	ck_assert(e == BT_ERR_BAD_PARAM);
	bt_device_t device;
	e = bt_inquiry_next(&inquiry, &device);
	ck_assert(e == BT_ERR_BAD_PARAM);
	e = bt_services_next(&inquiry, &service);
	ck_assert(e == BT_SUCCESS);
	bt_uuid_to_str(&service.uuid, uuid);
	printf("Service: %s - %s\n", uuid, service.name);
	ck_assert_str_eq(uuid, "00001801-0000-1000-8000-00805f9b34fb");
   	ck_assert_str_eq(service.name, "<no name>");

	e = bt_services_next(&inquiry, &service);
	ck_assert(e == BT_SUCCESS);
	bt_uuid_to_str(&service.uuid, uuid);
	printf("Service: %s - %s\n", uuid, service.name);
	ck_assert_str_eq(uuid, "00001112-0000-1000-8000-00805f9b34fb");
   	ck_assert_str_eq(service.name, "Headset Gateway");

	e = bt_services_next(&inquiry, &service);
	ck_assert(e == BT_SUCCESS);
	bt_uuid_to_str(&service.uuid, uuid);
	printf("Service: %s - %s\n", uuid, service.name);
	ck_assert_str_eq(uuid, "ed995e5a-c7e7-4442-a6ee-7bb76df43b0d");
   	ck_assert_str_eq(service.name, "Pico");
	ck_assert(service.port == 10);

	e = bt_services_next(&inquiry, &service);
	ck_assert(e == BT_ERR_END_OF_ENUM);
	bt_services_end(&inquiry);
	bt_services_end(NULL);
}
END_TEST

START_TEST (test_connect_to_service)
{
	char *addressStr = "64:bc:0c:f9:e8:6c";
	bt_addr_t address;
	bt_err_t e;
	char uuid[BT_UUID_LENGTH];
	bt_uuid_t pico_service_uuid;
	bt_str_to_uuid("ed995e5a-c7e7-4442-a6ee-7bb76df43b0d", &pico_service_uuid);
	e = bt_str_to_addr(addressStr, &address);
	ck_assert(e == BT_SUCCESS);
	bt_socket_t sock;

	const int mocksock = 342; 
		
	uint8_t channel = 15;

	sdp_session_t * sdp_connect_local(const bdaddr_t *src, const bdaddr_t *dst, uint32_t flags) {
		sdp_session_t* ret = malloc(sizeof(sdp_session_t));
		
		ck_assert(memcmp(dst, "\x6c\xe8\xf9\x0c\xbc\x64", 6) == 0);
		ck_assert(memcmp(src, BDADDR_ANY, 6) == 0);
		ck_assert(flags == SDP_RETRY_IF_BUSY);

		ret->sock = mocksock;
		ret->state = 0;
		ret->local = 0;
		ret->flags = 1;
		ret->tid = 0;
		ret->priv = (void*) 0x53C937;

		return ret;
	}
	bz_funcs.sdp_connect = sdp_connect_local;

	int search_attr_req(sdp_session_t *session, const sdp_list_t *search, sdp_attrreq_type_t reqtype, const sdp_list_t *attrid_list, sdp_list_t **rsp_list) {
		// Check if the session is the same returned by connect
		ck_assert(session->sock == mocksock);
		ck_assert(session->priv == (void*) 0x53C937);

		// Check parameters
		// For bt_connect_to_service we are expecting the uuid that was requested
		ck_assert(reqtype = SDP_ATTR_REQ_RANGE);
		ck_assert(sdp_list_len(attrid_list) == 1);
		ck_assert(sdp_list_len(search) == 1);

		ck_assert(*(uint32_t*) attrid_list->data == 0xffff);
		bt_uuid_t localuuid;
		bt_uuidt_to_uuid(search->data, &localuuid);
		bt_uuid_to_str(&localuuid, uuid);
		ck_assert_str_eq(uuid, "ed995e5a-c7e7-4442-a6ee-7bb76df43b0d");

		// Write the results.
		// This was collected from a real search on my phone
		sdp_record_t* record;
		uuid_t uuid;
		sdp_list_t* svclass_list;
		sdp_data_t* data;

		record = sdp_record_alloc();
		sdp_uuid128_create(&uuid, "\xed\x99\x5e\x5a\xc7\xe7\x44\x42\xa6\xee\x7b\xb7\x6d\xf4\x3b\xd");
		svclass_list = sdp_list_append(NULL, &uuid);
		sdp_set_service_classes(record, svclass_list);
		data = sdp_data_alloc_with_length(SDP_TEXT_STR8, (void*) "Pico", 5);
		sdp_attr_add(record, SDP_ATTR_SVCNAME_PRIMARY, data);

		// Access Protocols for third record
		sdp_list_t *aproto, *proto[2], *apseq;
		uuid_t l2cap, rfcomm;

		sdp_uuid16_create(&l2cap, L2CAP_UUID);
		proto[0] = sdp_list_append(0, &l2cap);
		apseq = sdp_list_append(0, proto[0]);
		
		sdp_uuid16_create(&rfcomm, RFCOMM_UUID);
		proto[1] = sdp_list_append(0, &rfcomm);
		proto[1] = sdp_list_append(proto[1], sdp_data_alloc(SDP_UINT8, &channel));
		apseq = sdp_list_append(apseq, proto[1]);

		aproto = sdp_list_append(0, apseq);
		sdp_set_access_protos(record, aproto);

		*rsp_list = sdp_list_append(*rsp_list, record);
		

		return 0;
	}
	bz_funcs.sdp_service_search_attr_req = search_attr_req;
	
	int close (sdp_session_t *session) {
		// Check if the session is the same returned by connect
		ck_assert(session->sock == mocksock);
		ck_assert(session->priv == (void*) 0x53C937);
		return 0;
	}
	bz_funcs.sdp_close = close;

	int socket_local (int domain, int type, int protocol) {
		ck_assert(domain == AF_BLUETOOTH);
		ck_assert(type == SOCK_STREAM);
		ck_assert(protocol == BTPROTO_RFCOMM);
		return 666;
	}
	bz_funcs.socket = socket_local;

	bool connect_called = false;
	int connect_local (int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
		ck_assert(sockfd == 666);
		ck_assert(addrlen == sizeof(struct sockaddr_rc));
		const struct sockaddr_rc* addr_rc = (const struct sockaddr_rc *)addr;
		ck_assert(addr_rc->rc_family == AF_BLUETOOTH);
		ck_assert(addr_rc->rc_channel == channel);
		connect_called = true;
		return 0;
	}
	bz_funcs.connect = connect_local;

	e = bt_connect_to_service(&address, &pico_service_uuid, &sock);
	ck_assert(e == BT_SUCCESS);
	ck_assert(connect_called);
}
END_TEST

START_TEST (test_connect_to_service_and_sdp_connect_fails)
{
	bt_addr_t address;
	bt_err_t e;
	bt_uuid_t pico_service_uuid;
	bt_socket_t sock;
	
	bt_str_to_uuid("ed995e5a-c7e7-4442-a6ee-7bb76df43b0d", &pico_service_uuid);
	bt_str_to_addr("64:bc:0c:f9:e8:6c", &address);

	sdp_session_t * sdp_connect_local(const bdaddr_t *src, const bdaddr_t *dst, uint32_t flags) {
		return NULL;
	}
	bz_funcs.sdp_connect = sdp_connect_local;

	e = bt_connect_to_service(&address, &pico_service_uuid, &sock);
	ck_assert(e == BT_ERR_DEVICE_NOT_FOUND);
}
END_TEST

START_TEST (test_connect_to_service_and_service_doesnt_exist)
{
	bt_addr_t address;
	bt_err_t e;
	char uuid[BT_UUID_LENGTH];
	bt_uuid_t pico_service_uuid;
	bt_str_to_uuid("ed995e5a-c7e7-4442-a6ee-7bb76df43b0d", &pico_service_uuid);
	bt_str_to_addr("64:bc:0c:f9:e8:6c", &address);
	bt_socket_t sock;
	
	const int mocksock = 342; 

	sdp_session_t * sdp_connect_local(const bdaddr_t *src, const bdaddr_t *dst, uint32_t flags) {
		sdp_session_t* ret = malloc(sizeof(sdp_session_t));
		
		ck_assert(memcmp(dst, "\x6c\xe8\xf9\x0c\xbc\x64", 6) == 0);
		ck_assert(memcmp(src, BDADDR_ANY, 6) == 0);
		ck_assert(flags == SDP_RETRY_IF_BUSY);

		ret->sock = mocksock;
		ret->state = 0;
		ret->local = 0;
		ret->flags = 1;
		ret->tid = 0;
		ret->priv = (void*) 0x53C937;

		return ret;
	}
	bz_funcs.sdp_connect = sdp_connect_local;

	int search_attr_req(sdp_session_t *session, const sdp_list_t *search, sdp_attrreq_type_t reqtype, const sdp_list_t *attrid_list, sdp_list_t **rsp_list) {
		// Check if the session is the same returned by connect
		ck_assert(session->sock == mocksock);
		ck_assert(session->priv == (void*) 0x53C937);

		// Check parameters
		// For bt_connect_to_service we are expecting the uuid that was requested
		ck_assert(reqtype = SDP_ATTR_REQ_RANGE);
		ck_assert(sdp_list_len(attrid_list) == 1);
		ck_assert(sdp_list_len(search) == 1);

		ck_assert(*(uint32_t*) attrid_list->data == 0xffff);
		bt_uuid_t localuuid;
		bt_uuidt_to_uuid(search->data, &localuuid);
		bt_uuid_to_str(&localuuid, uuid);
		ck_assert_str_eq(uuid, "ed995e5a-c7e7-4442-a6ee-7bb76df43b0d");

		*rsp_list = NULL;
		
		// TODO Not sure if this returns zero in real life
		return 0;
	}
	bz_funcs.sdp_service_search_attr_req = search_attr_req;
	
	int close (sdp_session_t *session) {
		// Check if the session is the same returned by connect
		ck_assert(session->sock == mocksock);
		ck_assert(session->priv == (void*) 0x53C937);
		return 0;
	}
	bz_funcs.sdp_close = close;

	e = bt_connect_to_service(&address, &pico_service_uuid, &sock);
	ck_assert(e == BT_ERR_SERVICE_NOT_FOUND);
}
END_TEST

START_TEST (test_bt_bind)
{
	bool bound = false;

	int socket_local (int domain, int type, int protocol) {
		ck_assert(domain == AF_BLUETOOTH);
		ck_assert(type == SOCK_STREAM);
		ck_assert(protocol == BTPROTO_RFCOMM);
		return 666;
	}
	bz_funcs.socket = socket_local;
	
	int bind_only_to_15 (int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
		ck_assert_int_eq(sockfd, 666);
		ck_assert(addrlen == sizeof(struct sockaddr_rc));
		const struct sockaddr_rc* addr_rc = (const struct sockaddr_rc *)addr;
		ck_assert(addr_rc->rc_family == AF_BLUETOOTH);
		ck_assert(!memcmp(&addr_rc->rc_bdaddr, BDADDR_ANY, 6));

		if (addr_rc->rc_channel == 15) {
			bound = true;
			return 0;
		} else {
			return -1;
		}
	}
	bz_funcs.bind = bind_only_to_15;

	int getsockname_local(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
		ck_assert_int_eq(sockfd, 666);
		ck_assert(*addrlen == sizeof(struct sockaddr_rc));
		struct sockaddr_rc* addr_rc = (struct sockaddr_rc *)addr;

		if (bound) {
			addr_rc->rc_channel = 15;
			return 0;
		} else {
			return -1;
		}
	}
	bz_funcs.getsockname = getsockname_local;

	bt_err_t e;
	bt_socket_t sock;
	e = bt_bind(&sock);
	ck_assert(e == BT_SUCCESS);
	ck_assert_int_eq(bt_get_socket_channel(sock), 15);

	bound = false;
	ck_assert_int_eq(bt_get_socket_channel(sock), 0);
	e = bt_bind_to_channel(&sock, 10);
	ck_assert(e == BT_ERR_UNKNOWN);
	e = bt_bind_to_channel(&sock, 15);
	ck_assert(e == BT_SUCCESS);
	ck_assert_int_eq(bt_get_socket_channel(sock), 15);
}
END_TEST

START_TEST (test_bt_listen)
{
	bt_err_t e;
	bt_socket_t sock;
	sock.s = 123;
	bool called = false;

	int listen_local(int sockfd, int backlog) {
		ck_assert(backlog == 2);
		ck_assert_int_eq(sockfd, 123);
		called = true;
		return 0;
	}
	bz_funcs.listen = listen_local;

	e = bt_listen(&sock);
	ck_assert(e == BT_SUCCESS);
	ck_assert(called);
}
END_TEST

START_TEST (test_bt_accept)
{
	bt_err_t e;
	bt_socket_t sock, connect_sock;
	sock.s = 123;
	bool called = false;

	int getsockopt_local(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
		ck_assert(sockfd == 123);
		ck_assert(level == SOL_SOCKET);
		ck_assert(optname == SO_RCVTIMEO);
		ck_assert(*optlen == sizeof(struct timeval));

		struct timeval* timeout = (struct timeval*) optval;
		timeout->tv_sec = 10;
		timeout->tv_usec = 500000;

		return 0;
	}
	bz_funcs.getsockopt = getsockopt_local;

	int select_local(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
		ck_assert(nfds > 123);
		ck_assert(FD_ISSET(123, readfds));
		ck_assert(writefds == NULL);
		ck_assert(exceptfds == NULL);
		ck_assert(timeout->tv_sec == 10);
		ck_assert(timeout->tv_usec == 500000);
		return 1;
	}
	bz_funcs.select = select_local;

	int accept_local(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
		ck_assert_int_eq(sockfd, 123);
		called = true;
		return 456;
	}
	bz_funcs.accept = accept_local;

	e = bt_accept(&sock, &connect_sock);
	ck_assert(e == BT_SUCCESS);
	ck_assert(called);
	ck_assert(connect_sock.s == 456);
}
END_TEST

START_TEST (test_bt_read)
{
	bt_err_t e;
	bt_socket_t sock;
	sock.s = 123;
	int num_called = 0;
	char buf[20];

	ssize_t recv_local(int sockfd, void *buf, size_t len, int flags) {
		ck_assert_int_eq(sockfd, 123);
		ck_assert_int_eq(flags, 0);
		size_t ret = 0;
		if (num_called == 0) {
			ck_assert(len >= 5);
			memcpy(buf, "Pico ", 5);
			ret = 5;
		} else if (num_called == 1) {
			ck_assert(len >= 15);
			memcpy(buf, "Authentication", 15);
			ret = 15;
		} else {
			ck_assert(false);
		}
		num_called++;
		return ret;
	}
	bz_funcs.recv = recv_local;

	size_t len = 20;
	e = bt_read(&sock, buf, &len);
	ck_assert(e == BT_SUCCESS);
	ck_assert(num_called == 2);
	ck_assert(len == 20);
	ck_assert_str_eq(buf, "Pico Authentication");
}
END_TEST

START_TEST (test_bt_read_close_socket)
{
	bt_err_t e;
	bt_socket_t sock;
	sock.s = 123;
	int num_called = 0;
	char buf[20];

	ssize_t recv_local(int sockfd, void *buf, size_t len, int flags) {
		ck_assert_int_eq(sockfd, 123);
		ck_assert_int_eq(flags, 0);
		size_t ret = 0;
		if (num_called == 0) {
			ck_assert(len >= 5);
			memcpy(buf, "Pico ", 5);
			ret = 5;
		} else if (num_called == 1) {
			ret = 0;
		} else {
			ck_assert(false);
		}
		num_called++;
		return ret;
	}
	bz_funcs.recv = recv_local;

	size_t len = 20;
	e = bt_read(&sock, buf, &len);
	ck_assert(e == BT_SOCKET_CLOSED);
	ck_assert(num_called == 2);
	ck_assert(len == 5);
	buf[5] = '\0'; // Writing the null terminator just so str_eq works
	ck_assert_str_eq(buf, "Pico ");
}
END_TEST

START_TEST (test_bt_read_error)
{
	bt_err_t e;
	bt_socket_t sock;
	sock.s = 123;

	ssize_t recv_local(int sockfd, void *buf, size_t len, int flags) {
		ck_assert_int_eq(sockfd, 123);
		ck_assert_int_eq(flags, 0);
		return -1;
	}
	bz_funcs.recv = recv_local;

	char buf[20];
	size_t len = 20;
	e = bt_read(&sock, buf, &len);
	ck_assert(e == BT_ERR_UNKNOWN);
}
END_TEST


START_TEST (test_bt_write)
{
	bt_err_t e;
	bt_socket_t sock;
	sock.s = 123;
	int num_called = 0;

	ssize_t send_local(int sockfd, const void *buf, size_t len, int flags) {
		ck_assert_int_eq(sockfd, 123);
		ck_assert_int_eq(flags, 0);
		size_t ret = 0;
		if (num_called == 0) {
			ck_assert(len >= 5);
			ck_assert(!memcmp(buf, "Pico ", 5));
			ret = 5;
		} else if (num_called == 1) {
			ck_assert(len >= 15);
			ck_assert(!memcmp(buf, "Authentication ", 5));
			ret = 15;
		} else {
			ck_assert(false);
		}
		num_called++;
		return ret;
	}
	bz_funcs.send = send_local;

	e = bt_write(NULL, "Pico Authentication", 20);
	ck_assert(e == BT_ERR_BAD_PARAM);
	e = bt_write(&sock, NULL, 20);
	ck_assert(e == BT_ERR_BAD_PARAM);
	e = bt_write(&sock, "Pico Authentication", 20);
	ck_assert(e == BT_SUCCESS);
	ck_assert(num_called == 2);
}
END_TEST

START_TEST (test_bt_write_error)
{
	bt_err_t e;
	bt_socket_t sock;
	sock.s = 123;
	int num_called = 0;

	ssize_t send_local(int sockfd, const void *buf, size_t len, int flags) {
		ck_assert_int_eq(sockfd, 123);
		ck_assert_int_eq(flags, 0);
		size_t ret = 0;
		if (num_called == 0) {
			ck_assert(len >= 5);
			ck_assert(!memcmp(buf, "Pico ", 5));
			ret = 5;
		} else if (num_called == 1) {
			ret = -1; // Force error
		} else {
			ck_assert(false);
		}
		num_called++;
		return ret;
	}
	bz_funcs.send = send_local;

	e = bt_write(&sock, "Pico Authentication", 20);
	ck_assert(e == BT_ERR_UNKNOWN);
	ck_assert(num_called == 2);
}
END_TEST

START_TEST (test_bt_disconnect)
{
	bt_socket_t sock;
	sock.s = 123;
	int num_called = 0;

	int close_local(int sockfd) {
		ck_assert_int_eq(sockfd, 123);
		num_called++;
		return 0;
	}
	bz_funcs.close = close_local;

	bt_disconnect(&sock);
	ck_assert(sock.s == -1);
	ck_assert(num_called == 1);
}
END_TEST

START_TEST (test_bt_register_service)
{
	bt_uuid_t uuid;
	bt_str_to_uuid("0af56906-6623-11e7-907b-a6006ad3dba0", &uuid);
	bt_socket_t sock;
	sock.s = 222;
	bt_err_t e;
	bool called = false;
	
	int getsockname_local(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
		ck_assert_int_eq(sockfd, 222);
		ck_assert(*addrlen == sizeof(struct sockaddr_rc));
		struct sockaddr_rc* addr_rc = (struct sockaddr_rc *)addr;
		addr_rc->rc_channel = 3;
		return 0;
	}
	bz_funcs.getsockname = getsockname_local;
	
	sdp_session_t * sdp_connect_local(const bdaddr_t *src, const bdaddr_t *dst, uint32_t flags) {
		sdp_session_t* ret = malloc(sizeof(sdp_session_t));
		
		ck_assert(memcmp(dst, BDADDR_LOCAL, 6) == 0);
		ck_assert(memcmp(src, BDADDR_ANY, 6) == 0); 
		ck_assert(flags == SDP_RETRY_IF_BUSY);

		ret->sock = 231;
		ret->state = 0;
		ret->local = 0;
		ret->flags = 1;
		ret->tid = 0;
		ret->priv = (void*) 0x53C937;

		return ret;
	}
	bz_funcs.sdp_connect = sdp_connect_local;
	
	int sdp_record_register_local (sdp_session_t *session, sdp_record_t *rec, uint8_t flags) {
		ck_assert(session->sock == 231);
		ck_assert(session->priv == (void*) 0x53C937);
		ck_assert(flags == 0);

		char nameBuffer[50] = {0};
		char descBuffer[50] = {0};
		char uuidstr[BT_UUID_LENGTH];
		int err;

		err = sdp_get_service_name(rec, nameBuffer, 50);
		ck_assert_int_eq(err, 0);
		ck_assert_str_eq(nameBuffer, "Service Name");
		
		err = sdp_get_service_desc(rec, descBuffer, 50);
		ck_assert_int_eq(err, 0);
		ck_assert_str_eq(descBuffer, "");

		sdp_list_t* browse_groups;
		sdp_list_t* svc_class;
		sdp_list_t* protos;

		err = sdp_get_browse_groups(rec, &browse_groups);
		ck_assert_int_eq(err, 0);
		uuid_t* groups_uuid = (uuid_t*) browse_groups->data;
		ck_assert_int_eq(groups_uuid->type, SDP_UUID16);
		ck_assert_int_eq(groups_uuid->value.uuid16, PUBLIC_BROWSE_GROUP);

		err = sdp_get_service_classes(rec, &svc_class);
		ck_assert_int_eq(err, 0);
		bt_uuid_t localuuid;
		bt_uuidt_to_uuid((uuid_t*) svc_class->data, &localuuid);
		bt_uuid_to_str(&localuuid, uuidstr);
		ck_assert_str_eq("0af56906-6623-11e7-907b-a6006ad3dba0", uuidstr);
	
		err = sdp_get_access_protos(rec, &protos);
		ck_assert_int_eq(err, 0);
		int port = sdp_get_proto_port(protos, RFCOMM_UUID);
		ck_assert_int_eq(port, 3);

		sdp_list_free(browse_groups, 0);
		sdp_list_free(svc_class, 0);
		sdp_list_free(protos, 0);
		called = true;
		return 0;
	}
	bz_funcs.sdp_record_register = sdp_record_register_local;

	e = bt_register_service(&uuid, "Service Name", &sock);
	ck_assert(e == BT_SUCCESS);
	ck_assert(called);
}
END_TEST


TCase *libpicobt_btmain_testcase(void) {
	TCase *tcase = tcase_create("btmain");
	
	tcase_add_test(tcase, test_bt_init);
	tcase_add_test(tcase, test_bt_get_device_name);
	tcase_add_test(tcase, test_bt_inquiry);
	tcase_add_test(tcase, test_bt_services);
	tcase_add_test(tcase, test_connect_to_service);
	tcase_add_test(tcase, test_connect_to_service_and_sdp_connect_fails);
	tcase_add_test(tcase, test_connect_to_service_and_service_doesnt_exist);
	tcase_add_test(tcase, test_bt_bind);
	tcase_add_test(tcase, test_bt_listen);
	tcase_add_test(tcase, test_bt_accept);
	tcase_add_test(tcase, test_bt_read);
	tcase_add_test(tcase, test_bt_read_close_socket);
	tcase_add_test(tcase, test_bt_read_error);
	tcase_add_test(tcase, test_bt_write);
	tcase_add_test(tcase, test_bt_write_error);
	tcase_add_test(tcase, test_bt_disconnect);
	tcase_add_test(tcase, test_bt_register_service);
	
	return tcase;
}

