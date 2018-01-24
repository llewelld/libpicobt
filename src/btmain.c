/**
 * @file btmain.c
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
 * @brief Core Bluetooth stuff.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "picobt/bt.h"
#ifdef WINDOWS
#include <WinSock2.h>
#include <winsock.h>
#include <BluetoothAPIs.h>
#include <Windows.h>
#include <cguid.h>
// nothing further to include
#else // LINUX
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp_lib.h>
#endif

#include "picobt/log.h"


/* only Windows defines this, and it's useful to have something to initialise
   sockets to */
#ifndef WINDOWS
/// A value that represents an uninitialised or invalid socket.
#define INVALID_SOCKET -1
#endif

#ifdef WINDOWS
#else // LINUX
int dynamic_bind_rc(int sock, struct sockaddr_rc * sockaddr, socklen_t addrlen, uint8_t * port);
#endif

#ifdef WINDOWS
/**
 * This function does the same as a socket connect, but using select instead.
 * Also, there is the difference of returning a bt_err_t instead of int
 * 
 * WARNING: This must be called with a BLOCKING socket! After running this function,
 * the socket will be set as blocking
 *
 * @param s Socket as in connect
 * @param name sockaddr structure as in connect
 * @param namelen size of name as in connect
 * @return `BT_SUCCESS` if successful, or one of the following:
 *    `BT_ERR_DEVICE_NOT_FOUND` - Timeout searching for device.
 *    `BT_ERR_UNKNWON`          - unhelpfully generic failure
 */
bt_err_t async_connect(SOCKET s, const struct sockaddr * name, int namelen) {
	bt_err_t ret = BT_SUCCESS;
	int sockResult;
	int wsaError;
	int mode;

	// Set socket to be non-blocking
	mode = 1;
	sockResult = ioctlsocket(s, FIONBIO, &mode);
	if (sockResult != NO_ERROR) {
		LOG("ioctlsocket failed setting 1 with error %d\n", sockResult);
		ret = BT_ERR_UNKNOWN;
	}

	// connect to the remote device
	LOG("Calling connect %d\n", s);
	sockResult = connect(s, name, namelen);
	if (sockResult == SOCKET_ERROR) {
		wsaError = WSAGetLastError();
		if (wsaError == WSAEWOULDBLOCK) {
			fd_set write, err;
			TIMEVAL timeout;

			FD_ZERO(&write);
			FD_ZERO(&err);
			FD_SET(s, &write);
			FD_SET(s, &err);

			timeout.tv_sec = 5;
			timeout.tv_usec = 0;

			sockResult = select(0, NULL, &write, &err, &timeout);
			if (sockResult == 0) {
				LOG("Connect timed out %d\n", s);
				ret = BT_ERR_DEVICE_NOT_FOUND;
			} else if (sockResult > 0) {
				if (FD_ISSET(s, &err)) {
					LOG("select failed with %d\n", WSAGetLastError());
					ret = BT_ERR_UNKNOWN;
				}
			} else {
				LOG("select failed with %d\n", WSAGetLastError());
				ret = BT_ERR_UNKNOWN;
			}
		} else {
			LOG("connect failed with %d\n", wsaError);
			ret = BT_ERR_UNKNOWN;
		}
	} else {
		LOG("Connected to socket %d without waiting\n", s);
	}

	// Set socket to be blocking again, as the rest of the code
	// expects this.
	mode = 0;
	sockResult = ioctlsocket(s, FIONBIO, &mode);
	if (sockResult != NO_ERROR) {
		LOG("ioctlsocket failed setting 1 with error %d\n", sockResult);
		ret = BT_ERR_UNKNOWN;
	}

	return ret;
}
#endif

/**
 * Initialise use of Bluetooth. Call at start of program.
 * 
 * @return `BT_SUCCESS` if successful, or one of the following if there's an
 *         error:
 *    `BT_ERR_UNSUPPORTED`      - Bluetooth is not supported on this machine.
 *    `BT_ERR_UNKNWON`          - unhelpfully generic failure
 */
bt_err_t bt_init() {
#ifdef WINDOWS
	// initialise the Winsock library, requesting version 2.2
	WSADATA WSAData = {0};
	int result = WSAStartup(MAKEWORD(2, 2), &WSAData);
	switch (result) {
	case 0:
		return BT_SUCCESS;
	case WSAVERNOTSUPPORTED:
		return BT_ERR_UNSUPPORTED;
	/*case WSASYSNOTREADY:
	case WSAEINPROGRESS:
	case WSAEPROCLIM:
	case WSAEFAULT:
		return -1;*/
	default:
		return BT_ERR_UNKNOWN;
	}
	
#else // LINUX
	// check that bluetooth exists
	if (hci_get_route(NULL) < 0)
		return BT_ERR_UNSUPPORTED;
	
	return BT_SUCCESS;
#endif
}

/**
 * Free Bluetooth resources. Call at end of program.
 */
void bt_exit() {
#ifdef WINDOWS
	WSACleanup();
	
#else // LINUX
	// nothing as of yet
#endif
}

/**
 * Determine whether Bluetooth can be used on the device.
 * 
 * @return `BT_SUCCESS` if Bluetooth is present.
 */
int bt_is_present() {
	int present;

	present = BT_ERR_UNSUPPORTED;
#ifdef WINDOWS
// See http://stackoverflow.com/a/20240790
	SOCKET s;
	s = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
	if (s != INVALID_SOCKET) {
		present = BT_SUCCESS;
	}
	closesocket(s);
#else
	if (hci_get_route(NULL) >= 0) {
		present = BT_SUCCESS;
	}
#endif

	return present;
}

/******************************************************************************\
 * DEVICE DISCOVERY                                                           *
\******************************************************************************/

/**
 * Start a device inquiry. The call will block for a number of seconds, and
 * the result is a list of discoverable Bluetooth devices within range. Devices
 * are then enumerated using {@link bt_inquiry_next}.
 * 
 * @param inquiry Pointer to an uninitialised {@link bt_inquiry_t} object.
 * @param cached  Include cached devices (that have been paired?)
 * 
 * @return `BT_SUCCESS` if successful, or one of the following if there's an
 *         error:
 *    `BT_ERR_BAD_PARAM`        - you passed in a `NULL` pointer
 *    `BT_ERR_UNINITIALISED`    - you forgot to call bt_init()
 *    `BT_ERR_DEVICE_NOT_FOUND` - no devices found during inquiry
 *    `BT_ERR_UNKNWON`          - unhelpfully generic failure
 */
bt_err_t bt_inquiry_begin(bt_inquiry_t *inquiry, int cached) {
	// check parameters
	if (inquiry == NULL)
		return BT_ERR_BAD_PARAM;
	
	// set inquiry header fields
	inquiry->type = BT_INQUIRY_DEVICES;
	inquiry->error = 0;
	
#ifdef WINDOWS
	// set up the queryset record
	inquiry->qs = calloc(1, sizeof(WSAQUERYSET));
	inquiry->qs->dwSize = sizeof(WSAQUERYSET);
	inquiry->qs->dwNameSpace = NS_BTH;
	
	// set up flags for finding devices rather than services
	inquiry->flags = LUP_CONTAINERS | LUP_RETURN_NAME |
			LUP_RETURN_ADDR | LUP_RETURN_TYPE;
	if (!cached)
		inquiry->flags |= LUP_FLUSHCACHE;
	
	// misc
	inquiry->qsSize = inquiry->qs->dwSize;
	inquiry->hLookup = NULL;
	
	// start the inquiry
	if (WSALookupServiceBegin(inquiry->qs, inquiry->flags, &inquiry->hLookup)) {
		int e = WSAGetLastError();
		switch (e) {
		case WSA_NOT_ENOUGH_MEMORY:
			LOG("bt_services_begin: WSALookupServiceBegin failed: WSA_NOT_ENOUGH_MEMORY\n");
			break;
		case WSAEINVAL:
			LOG("bt_services_begin: WSALookupServiceBegin failed: WSAEINVAL\n");
			break;
		case WSANO_DATA:
			LOG("bt_services_begin: WSALookupServiceBegin failed: WSANO_DATA\n");
			break;
		case WSANOTINITIALISED:
			LOG("bt_services_begin: WSALookupServiceBegin failed: WSANOTINITIALISED\n");
			return BT_ERR_UNINITIALISED;
		case WSASERVICE_NOT_FOUND:
			LOG("bt_services_begin: WSALookupServiceBegin failed: WSASERVICE_NOT_FOUND\n");
			return BT_ERR_DEVICE_NOT_FOUND;
		case WSAEFAULT:
			LOG("bt_services_begin: WSALookupServiceBegin failed: WSAEFAULT\n");
			break;
		default:
			LOG("bt_services_begin: WSALookupServiceBegin failed: wtf %d\n", e);
		}
		return BT_ERR_UNKNOWN;
	}
	// no error, but the MSDN example checks the handle anyway
	if (NULL == inquiry->hLookup)
		return BT_ERR_WTF; // this should never happen, right?
	
	return BT_SUCCESS;
	
#else // LINUX
	inquiry->dev.dev_id = hci_get_route(NULL);
	if (inquiry->dev.dev_id < 0)
		return BT_ERR_UNSUPPORTED;
	
	inquiry->dev.socket = hci_open_dev(inquiry->dev.dev_id);
	inquiry->dev.flags = 0;
	if (!cached)
		inquiry->dev.flags |= IREQ_CACHE_FLUSH;
	
	#define MAX_INQUIRY_RESULTS 256
	inquiry->dev.info = malloc(MAX_INQUIRY_RESULTS * sizeof(inquiry_info));
	inquiry->dev.current = inquiry->dev.info;
	
	inquiry->dev.count = hci_inquiry(inquiry->dev.dev_id, 8, MAX_INQUIRY_RESULTS, NULL,
			&inquiry->dev.info, inquiry->dev.flags);
	if (inquiry->dev.count < 0) {
		free(inquiry->dev.info);
		if (inquiry->dev.socket != -1)
			close(inquiry->dev.socket);
		return BT_ERR_UNKNOWN;
	}
	
	inquiry->nameBuffer = malloc(DEVICE_NAME_BUFFER_SIZE);
	
	return BT_SUCCESS;
	
#endif
}

/**
 * Get the next device in a device inquiry. Returns `BT_ERR_END_OF_ENUM` when
 * the end of the list is reached.
 * 
 * @param inquiry Pointer to bt_inquiry_t object previously passed to
 *                {@link bt_inquiry_begin}
 * @param device  Pointer to bt_device_t object. If the function returns
 *                `BT_SUCCESS`, this will contain info about the Bluetooth
 *                device. Otherwise it remains unmodified.
 * 
 * @return `BT_SUCCESS` if successful, or one of the following if there's an
 *         error:
 *    `BT_ERR_BAD_PARAM`        - you passed in a `NULL` pointer, or inquiry isn't a device inquiry
 *    `BT_ERR_UNINITIALISED`    - you forgot to call {@link bt_init()}
 *    `BT_ERR_END_OF_ENUM`      - there is no next item
 *    `BT_ERR_UNKNOWN`          - unhelpfully generic failure
 */
bt_err_t bt_inquiry_next(bt_inquiry_t *inquiry, bt_device_t *device) {
#ifdef WINDOWS
	// watchdog to make sure we don't end up in an infinite retry loop
	int sanity;
#endif
	
	// check parameters
	if (inquiry == NULL || device == NULL)
		return BT_ERR_BAD_PARAM;
	if (inquiry->type != BT_INQUIRY_DEVICES)
		return BT_ERR_BAD_PARAM;
	
#ifdef WINDOWS
	if (inquiry->hLookup == NULL)
		return BT_ERR_BAD_PARAM;
	
	// try to get a queryset record
	for (sanity = 1; sanity >= 0; sanity--) {
		if (WSALookupServiceNext(inquiry->hLookup, inquiry->flags,
				&inquiry->qsSize, inquiry->qs)) {
			int e = WSAGetLastError();
			switch (e) {
			case WSAEFAULT:
				// record too small?
				//LOG("bt_inquiry_next: too small, expanding to %d bytes\n", (int) inquiry->qsSize);
				inquiry->qs = realloc(inquiry->qs, inquiry->qsSize);
				// try again
				if (sanity == 0)
					return BT_ERR_UNKNOWN;
				break;
				
			case WSA_E_NO_MORE:
				// this was the last one; not really an error
				return BT_ERR_END_OF_ENUM;
				
				// properly bad errors
			case WSANOTINITIALISED:
				LOG("bt_inquiry_next: WSALookupServiceNext failed with error WSANOTINITIALISED\n");
				return BT_ERR_UNINITIALISED;
			default:
				LOG("bt_inquiry_next: WSALookupServiceNext failed with error %d\n", e);
				inquiry->error = e;
				return BT_ERR_UNKNOWN;
			}
		} else {
			break;
		}
	}
	
	// populate the bt_device_t record
	device->name = inquiry->qs->lpszServiceInstanceName;
	device->cod = inquiry->qs->lpServiceClassId->Data1;
	// note: this only works on little-endian systems
	// I don't think Windows runs on any BE systems so it should be fine
	memcpy(&device->address,
			&((PSOCKADDR_BTH) inquiry->qs->lpcsaBuffer->RemoteAddr.lpSockaddr)->btAddr, 6);
	// todo: replace with bt_btaddr_to_addr(&((PSOCKADDR_BTH) inquiry->qs->lpcsaBuffer->RemoteAddr.lpSockaddr)->btAddr, &device->address);
	
	return BT_SUCCESS;
	
#else // LINUX
	
	// check for end of enum
	if (inquiry->dev.count == 0)
		return BT_ERR_END_OF_ENUM;
	inquiry_info *info = inquiry->dev.current;
	
	// get teh device name
	if (hci_read_remote_name(inquiry->dev.socket, &info->bdaddr,
			DEVICE_NAME_BUFFER_SIZE, inquiry->nameBuffer, 0) < 0)
		strcpy(inquiry->nameBuffer, "<unavailable>");
	
	// populate the bt_device_t record
	device->cod = ((uint32_t) info->dev_class[0] << 16) |
			((uint32_t) info->dev_class[1] << 8) |
			((uint32_t) info->dev_class[2]);
	memcpy(&device->address, &info->bdaddr, 6);
	device->name = inquiry->nameBuffer;
	
	// next
	inquiry->dev.current++;
	inquiry->dev.count--;
	
	return BT_SUCCESS;
	
#endif
}

/**
 * Finish off a device inquiry (initiated with {@link bt_inquiry_begin}) and
 * free its resources.
 * 
 * @param inquiry The inquiry to finish.
 */
void bt_inquiry_end(bt_inquiry_t *inquiry) {
	if (inquiry == NULL)
		return;
	
#ifdef WINDOWS
	if (inquiry->hLookup != NULL) {
		WSALookupServiceEnd(inquiry->hLookup);
		inquiry->hLookup = NULL;
	}
	if (inquiry->qs != NULL) {
		free(inquiry->qs);
		inquiry->qs = NULL;
	}
	
#else // LINUX
	if (inquiry->dev.dev_id < 0)
		return;
	if (inquiry->dev.info != NULL) {
		free(inquiry->dev.info);
		inquiry->dev.info = NULL;
	}
	if (inquiry->nameBuffer != NULL) {
		free(inquiry->nameBuffer);
		inquiry->nameBuffer = NULL;
	}
	if (inquiry->dev.socket != -1) {
		close(inquiry->dev.socket);
		inquiry->dev.socket = -1;
	}
	
#endif
}


/******************************************************************************\
 * SERVICE DISCOVERY                                                          *
\******************************************************************************/

/**
 * Start a service inquiry. For a given device, this will produce a list of
 * services with SDP records registered on that device. Results are then
 * enumerated using {@link bt_services_next}.
 * 
 * @param inquiry       Pointer to an uninitialised {@link bt_inquiry_t} object.
 * @param device        Device to query services from.
 * @param service_class Service class UUID. You may pass `NULL` to get all
 *                      (public) services back.
 * @param cached        Find only services in cache (generally you always want
 *                      to pass `0` here).
 * 
 * @return `BT_SUCCESS` if successful, or one of the following if there's an
 *         error:
 *    `BT_ERR_BAD_PARAM`         - you passed in a `NULL` pointer
 *    `BT_ERR_UNINITIALISED`     - you forgot to call {@link bt_init()}
 *    `BT_ERR_SERVICE_NOT_FOUND` - no services found during inquiry, or device not available
 *    `BT_ERR_UNKNWON`           - unhelpfully generic failure
 */
bt_err_t bt_services_begin(bt_inquiry_t *inquiry,
						const bt_addr_t *device,
						const bt_uuid_t *service_class,
						int cached) {
	bt_uuid_t temp;
	int e;
#ifdef WINDOWS
	SOCKADDR_BTH sa = {0};
	DWORD length = 40;
#else
	bdaddr_t addr;
	uuid_t uuid;
	sdp_list_t *search_list, *attrid_list;
	sdp_list_t *response_list = NULL;
	uint32_t range = 0xffff;
#endif
	
	// check parameters
	if (inquiry == NULL || device == NULL)
		return BT_ERR_BAD_PARAM;
	
	// a NULL class means we use the public browse group UUID
	if (service_class == NULL) {
		bt_str_to_uuid("00001002-0000-1000-8000-00805f9b34fb", &temp);
		service_class = &temp;
	}
	
	// set inquiry header fields
	inquiry->type = BT_INQUIRY_SERVICES;
	inquiry->error = 0;
	
#ifdef WINDOWS
	// set up the queryset record
	inquiry->qs = calloc(1, sizeof(WSAQUERYSET));
	inquiry->qs->dwSize = sizeof(WSAQUERYSET);
	inquiry->qs->dwNameSpace = NS_BTH;
	inquiry->qs->dwNumberOfCsAddrs = 0;
	inquiry->qs->lpServiceClassId = malloc(sizeof(GUID));
	bt_uuid_to_guid(service_class, inquiry->qs->lpServiceClassId);
	// set the target device string (why can't they just use a BD_ADDR??)
	sa.addressFamily = AF_BTH;
	memcpy(&sa.btAddr, device, 6);
	inquiry->qs->lpszContext = malloc(length);
	e = WSAAddressToString((SOCKADDR*) &sa, sizeof(SOCKADDR_BTH), NULL,
			inquiry->qs->lpszContext, &length);
	if (e == WSAEFAULT) {
		// maybe buffer is too small; make it bigger and try again
		inquiry->qs->lpszContext = realloc(inquiry->qs->lpszContext, length);
		e = WSAAddressToString((SOCKADDR*) &sa, sizeof(SOCKADDR_BTH), NULL,
				inquiry->qs->lpszContext, &length);
		if (e) {
			free(inquiry->qs->lpszContext);
			LOG("bt_services_begin: WSAAddressToString failed (%d)\n", e);
			if (e == WSANOTINITIALISED)
				return BT_ERR_UNINITIALISED;
			else
				return BT_ERR_UNKNOWN;
		}
	} else if (e) {
		free(inquiry->qs->lpszContext);
		LOG("bt_services_begin: WSAAddressToString failed (%d)\n", e);
		if (e == WSANOTINITIALISED)
			return BT_ERR_UNINITIALISED;
		else
			return BT_ERR_UNKNOWN;
	}
	
	// set up flags for finding devices rather than services
	inquiry->flags = LUP_RETURN_ALL | LUP_RETURN_COMMENT;
	if (!cached)
		inquiry->flags |= LUP_FLUSHCACHE;
	
	// misc
	inquiry->qsSize = inquiry->qs->dwSize;
	inquiry->hLookup = NULL;
	
	// start the inquiry
	if (WSALookupServiceBegin(inquiry->qs, inquiry->flags, &inquiry->hLookup)) {
		int e = WSAGetLastError();
		switch (e) {
		case WSA_NOT_ENOUGH_MEMORY:
			LOG("bt_services_begin: WSALookupServiceBegin failed: WSA_NOT_ENOUGH_MEMORY\n");
			break;
		case WSAEINVAL:
			LOG("bt_services_begin: WSALookupServiceBegin failed: WSAEINVAL\n");
			break;
		case WSANO_DATA:
			LOG("bt_services_begin: WSALookupServiceBegin failed: WSANO_DATA\n");
			break;
		case WSANOTINITIALISED:
			LOG("bt_services_begin: WSALookupServiceBegin failed: WSANOTINITIALISED\n");
			return BT_ERR_UNINITIALISED;
		case WSASERVICE_NOT_FOUND:
			LOG("bt_services_begin: WSALookupServiceBegin failed: WSASERVICE_NOT_FOUND\n");
			return BT_ERR_SERVICE_NOT_FOUND;
		case WSAEFAULT:
			LOG("bt_services_begin: WSALookupServiceBegin failed: WSAEFAULT\n");
			break;
		default:
			LOG("bt_services_begin: WSALookupServiceBegin failed: wtf %d\n", e);
		}
		return BT_ERR_UNKNOWN;
	}
	// no error, but the MSDN example checks the handle anyway
	if (NULL == inquiry->hLookup)
		return BT_ERR_WTF; // this should never happen, right?
	
	return BT_SUCCESS;
	
#else // LINUX
	bt_addr_to_bdaddr(device, &addr);
	bt_uuid_to_uuid(service_class, &uuid);
	
	// connect to the remote SDP server
	inquiry->sdp.session = sdp_connect(BDADDR_ANY, &addr, SDP_RETRY_IF_BUSY);
	if (inquiry->sdp.session == NULL)
		return BT_ERR_DEVICE_NOT_FOUND;
	
	// get SDP records
	search_list = sdp_list_append(NULL, &uuid);
	attrid_list = sdp_list_append(NULL, &range);
	e = sdp_service_search_attr_req(inquiry->sdp.session, search_list,
			SDP_ATTR_REQ_RANGE, attrid_list, &response_list);
	// don't need these any more
	sdp_list_free(search_list, 0);
	sdp_list_free(attrid_list, 0);
	// check for error
	if (e < 0) {
		sdp_close(inquiry->sdp.session);
		inquiry->sdp.session = NULL;
		return BT_ERR_DEVICE_NOT_FOUND;
	}
	
	inquiry->sdp.response = response_list;
	inquiry->nameBuffer = malloc(SERVICE_NAME_BUFFER_SIZE);
	inquiry->descBuffer = malloc(SERVICE_DESCRIPTION_BUFFER_SIZE);
	
	return BT_SUCCESS;
	
#endif
}

/**
 * Get the next service in an inquiry. Returns `BT_ERR_END_OF_ENUM` when end of
 * list is reached.
 * 
 * @param inquiry Pointer to {@link bt_inquiry_t} object previously passed to
 *                {@link bt_services_begin}.
 * @param service Pointer to {@link bt_service_t} object. If the function
 *                returns `BT_SUCCESS`, this will contain info about the
 *                discovered service. Otherwise it remains unmodified.
 * 
 * @return `BT_SUCCESS` if successful, or one of the following if there's an
 *         error:
 *    `BT_ERR_BAD_PARAM`        - you passed in a `NULL` pointer, or inquiry isn't a device inquiry
 *    `BT_ERR_UNINITIALISED`    - you forgot to call {@link bt_init()}
 *    `BT_ERR_END_OF_ENUM`      - there is no next item
 *    `BT_ERR_UNKNOWN`          - unhelpfully generic failure
 */
bt_err_t bt_services_next(bt_inquiry_t *inquiry, bt_service_t *service) {
#ifdef WINDOWS
	// watchdog to make sure we don't end up in an infinite retry loop
	int sanity;
#else
	sdp_list_t *response;
	sdp_record_t *record;
	sdp_list_t *list = NULL;
	uuid_t *uuid;
	int e;
#endif
	
	// check parameters
	if (inquiry == NULL || service == NULL)
		return BT_ERR_BAD_PARAM;
	if (inquiry->type != BT_INQUIRY_SERVICES)
		return BT_ERR_BAD_PARAM;
	
#ifdef WINDOWS
	if (inquiry->hLookup == NULL)
		return BT_ERR_BAD_PARAM;
	
	inquiry->flags &= ~LUP_FLUSHCACHE;
	
	// try to get a queryset record
	for (sanity = 1; sanity >= 0; sanity--) {
		if (WSALookupServiceNext(inquiry->hLookup, inquiry->flags,
				&inquiry->qsSize, inquiry->qs)) {
			int e = WSAGetLastError();
			if (e == WSAEFAULT) {
				// record too small?
				//LOG("hLookup=%08x, flags=%08x, qs=%08x\n", (int) inquiry->hLookup, (int) inquiry->flags, (int) inquiry->qs);
				//LOG("bt_services_next: too small, expanding to %d bytes\n", (int) inquiry->qsSize);
				inquiry->qs = realloc(inquiry->qs, inquiry->qsSize);
				// try again
				if (sanity == 0) {
					LOG("bt_services_next: WSALookupServiceNext failed twice with WSAEFAULT\n");
					return BT_ERR_UNKNOWN;
				}
				
			} else if (e == WSA_E_NO_MORE) {
				// this was the last one; not really an error
				return BT_ERR_END_OF_ENUM;
				
			} else {
				// bad error happened
				LOG("bt_services_next: WSALookupServiceNext failed with error %d\n", e);
				inquiry->error = e;
				return BT_ERR_UNKNOWN;
			}
		} else {
			break;
		}
	}
	
	// todo: parse SDP record
	LOG("\n");
	
	// populate the bt_service_t record
	SOCKADDR_BTH *addr = (SOCKADDR_BTH*) inquiry->qs->lpcsaBuffer->RemoteAddr.lpSockaddr;
	service->name = inquiry->qs->lpszServiceInstanceName;
	service->description = inquiry->qs->lpszComment;
	memcpy(&service->uuid, &addr->serviceClassId, 16);
	service->port = (int) addr->port;
	
	return BT_SUCCESS;
	
#else // LINUX
	if (inquiry->sdp.session == NULL)
		return BT_ERR_BAD_PARAM;
	
	// get the current response item if there is one
	response = inquiry->sdp.response;
	if (response == NULL)
		return BT_ERR_END_OF_ENUM;
	// get the SDP record
	record = (sdp_record_t*) response->data;
	// move to next
	inquiry->sdp.response = response->next;
	
	// get the service's name
	e = sdp_get_service_name(record, inquiry->nameBuffer,
			SERVICE_NAME_BUFFER_SIZE);
	if (e) strcpy(inquiry->nameBuffer, "<no name>");
	// get its description
	e = sdp_get_service_desc(record, inquiry->descBuffer,
			SERVICE_DESCRIPTION_BUFFER_SIZE);
	if (e) strcpy(inquiry->descBuffer, "<no description>");
	// put in the bt_service_t record
	service->name = inquiry->nameBuffer;
	service->description = inquiry->descBuffer;
	
	// get its class UUID
	if (sdp_get_service_classes(record, &list) == 0) {
		uuid = (uuid_t*) list->data;
		bt_uuidt_to_uuid(uuid, &service->uuid);
		sdp_list_free(list, 0);
	}
	
	// get the service's channel number
	if (sdp_get_access_protos(record, &list) == 0) {
		service->port = sdp_get_proto_port(list, RFCOMM_UUID);
		sdp_list_free(list, 0);
	}
	
	// free this SDP record
	sdp_record_free(record);
	
	return BT_SUCCESS;
	
#endif
}

/**
 * Finish off a service inquiry (initiated with {@link bt_services_begin}) and
 * free its resources.
 * 
 * @param inquiry The inquiry to finish.
 */
void bt_services_end(bt_inquiry_t *inquiry) {
	if (inquiry == NULL)
		return;
	
#ifdef WINDOWS
	if (inquiry->hLookup != NULL)
		WSALookupServiceEnd(inquiry->hLookup);
	if (inquiry->qs != NULL)
		free(inquiry->qs);
	
#else // LINUX
	if (inquiry->sdp.session != NULL) {
		sdp_close(inquiry->sdp.session);
		inquiry->sdp.session = NULL;
	}
	if (inquiry->nameBuffer != NULL) {
		free(inquiry->nameBuffer);
		inquiry->nameBuffer = NULL;
	}
	if (inquiry->descBuffer != NULL) {
		free(inquiry->descBuffer);
		inquiry->descBuffer = NULL;
	}
	
#endif
}


/******************************************************************************\
 * CONNECTIONS                                                                *
\******************************************************************************/

#ifndef WINDOWS
/**
 * Look up a service and find out which RFCOMM channel it's bound to.
 * This is required only for the Linux build; Windows does it for us.
 * Largely copied from example from "An Introduction to Bluetooth Programming".
 * 
 * @return A channel number in [1..30] if successful. Otherwise, -1 if channel
 *         not found, -2 if device not found.
 * 
 * @see https://people.csail.mit.edu/albert/bluez-intro/x604.html
 */
static int bt_find_service_channel(const bdaddr_t *device, uuid_t *uuid) {
	sdp_list_t *response_list = NULL;
	sdp_list_t *search_list, *attrid_list;
	sdp_session_t *session;
	int channel = -1;
	
	// connect to the SDP server running on the remote machine
	session = sdp_connect(BDADDR_ANY, device, SDP_RETRY_IF_BUSY);
	if (session == NULL) {
		return -2;
	}
	
	// specify the UUID of the application we're searching for
	search_list = sdp_list_append(NULL, uuid);
	
	// specify that we want a list of all the matching applications' attributes
	uint32_t range = 0xffff;
	attrid_list = sdp_list_append(NULL, &range);
	
	// get a list of service records that have the given UUID
	sdp_service_search_attr_req(session, search_list,
			SDP_ATTR_REQ_RANGE, attrid_list, &response_list);
	sdp_list_t *r = response_list;
	
	// go through each of the service records
	for (; r; r = r->next) {
		sdp_record_t *rec = (sdp_record_t*) r->data;
		sdp_list_t *proto_list;
		// get a list of the protocol sequences
		if (sdp_get_access_protos(rec, &proto_list) == 0) {
			channel = sdp_get_proto_port(proto_list, RFCOMM_UUID);
			sdp_list_free(proto_list, 0);
		}
		sdp_record_free(rec);
	}
	sdp_close(session);
	return channel;
}
#endif

/**
 * Create an RFCOMM connection to the specified device and service.
 * 
 * @param address  Bluetooth address of the device to connect to
 * @param service  UUID of the remote service to connect to
 * @param sock     Pointer to a Bluetooth socket, that, if the operation is
 *                 successful, is connected to the remote service
 * 
 * @return `BT_SUCCESS` if successful, or one of the following error values:
 *    `BT_ERR_DEVICE_NOT_FOUND`  - the desired device could not be connected to
 *    `BT_ERR_SERVICE_NOT_FOUND` - the desired service was not found on the device
 *    `BT_ERR_UNKNOWN`           - unhelpfully generic failure
 */
bt_err_t bt_connect_to_service(const bt_addr_t *address,
							const bt_uuid_t *service,
							bt_socket_t *sock) {
#ifdef WINDOWS
	SOCKADDR_BTH addr;
	SOCKET s;
	bt_err_t ret = BT_SUCCESS;
	
	
	sock->s = INVALID_SOCKET;
	
	// populate the remote address structure
	addr.addressFamily = AF_BTH;
	addr.port = 0;
	bt_addr_to_bdaddr(address, &addr.btAddr);
	bt_uuid_to_guid(service, &addr.serviceClassId);
	
	// create an RFCOMM socket
	s = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
	if (s == INVALID_SOCKET) {
		LOG("bt_connect_to_service: could not create socket\n");
		ret = BT_ERR_UNKNOWN;
	}

	// connect to the remote device
	if (ret == BT_SUCCESS) {
		ret = async_connect(s, (SOCKADDR*)&addr, sizeof(addr));
	}

	if (ret == BT_SUCCESS) {
		// success, update the bt_socket_t
		sock->s = s;
	} else if (s != INVALID_SOCKET) {
		closesocket(s);
	}
	return ret;
	
#else // LINUX
	uuid_t uuid;
	int channel;
	bdaddr_t bdaddr;

	// first we have to find what channel to connect to
	bt_addr_to_bdaddr(address, &bdaddr);
	bt_uuid_to_uuid(service, &uuid);
	
	channel = bt_find_service_channel(&bdaddr, &uuid);
	if (channel == -2) {
		LOG("bt_connect_to_service: device unavailable\n");
		return BT_ERR_DEVICE_NOT_FOUND;
	} else if (channel == -1) {
		LOG("bt_connect_to_service: service not running\n");
		return BT_ERR_SERVICE_NOT_FOUND;
	}
	
	return bt_connect_to_port(address, channel, sock);
#endif
}

/**
 * Create an RFCOMM connection to the specified device and port.
 * 
 * @param address Bluetooth address of the device to connect to
 * @param port The RFCOMM port number to connect to
 * @param sock Pointer to a Bluetooth socket, that, if the operation is
 *             successful, is connected to the remote service
 * 
 * @return `BT_SUCCESS` if successful, or one of the following error values:
 *    `BT_ERR_DEVICE_NOT_FOUND`  - the desired device could not be connected to
 *    `BT_ERR_UNKNOWN`           - unhelpfully generic failure
 */
bt_err_t bt_connect_to_port(const bt_addr_t *address, unsigned char port, bt_socket_t *sock) {
#ifdef WINDOWS
	SOCKADDR_BTH addr;
	SOCKET s;
	bt_err_t ret = BT_SUCCESS;
	
	// be safe
	sock->s = INVALID_SOCKET;
	
	// populate the remote address structure
	addr.addressFamily = AF_BTH;
	addr.port = port;
	bt_addr_to_bdaddr(address, &addr.btAddr);
	
	// create an RFCOMM socket
	s = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
	if (s == INVALID_SOCKET) {
		LOG("bt_connect_to_service: could not create socket\n");
		ret = BT_ERR_UNKNOWN;
	}
	
	// connect to the remote device
	if (ret == BT_SUCCESS) {
		ret = async_connect(s, (SOCKADDR*)&addr, sizeof(addr));
	}

	if (ret == BT_SUCCESS) {
		// success, update the bt_socket_t
		sock->s = s;
	} else if (s != INVALID_SOCKET) {
		closesocket(s);
	}

	return BT_SUCCESS;

#else // LINUX
    struct sockaddr_rc target = { 0 };
    int result;
    char showaddress[256];
    int s;

    // be safe
    sock->s = INVALID_SOCKET;

    // first we have to find what channel to connect to
    bt_addr_to_bdaddr(address, &target.rc_bdaddr);
    bt_addr_to_str(address, showaddress);
    LOG("Connecting to: %s on port: %d\n", showaddress, port);

    // allocate a socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (s == INVALID_SOCKET) {
        LOG("bt_connect_to_service: could not create socket\n");
        return BT_ERR_ALLOCATING_SOCKET;
    }

    // set remaining connection parameters and connect
    target.rc_family = AF_BLUETOOTH;
    target.rc_channel = (uint8_t) port;
    result = connect(s, (struct sockaddr*) &target, sizeof(target));
    if (result) {
        LOG("bt_connect_to_service: could not connect socket (%d): %d\n", result, errno);
        close(s);
        return BT_ERR_CONNECTION_FAILURE;
    }

    // Success. Assign socket
    sock->s = s;

    return BT_SUCCESS;
#endif
}

/**
 * Listen on a bluetooth socket and return the first connection accepted.
 * @param service The UUID of the service.
 * @param service_name The name of the service.
 * @param sock Returns the socket of the accepted connection.
 * @param timeout Time to wait for a connection
 *        If NULL, will wait indefinitely
 * 
 * @return `BT_SUCCESS` if successful.
 */
bt_err_t bt_wait_for_connection(bt_uuid_t const * service, char const * service_name, bt_socket_t * sock, struct timeval* timeout) {
	bt_socket_t listener;
	bt_err_t err;
	
	err = BT_SUCCESS;

	if (err == BT_SUCCESS) {
		err = bt_bind(&listener);
	}

	if (err == BT_SUCCESS) {
		// Register service
		err = bt_register_service(service, service_name, &listener);
	}

	if (err == BT_SUCCESS) {
		err = bt_listen(&listener);
	}

	if (err == BT_SUCCESS) {
		err = bt_accept_with_timeout(&listener, sock, timeout);
	}
	
	return err;
}


/**
 * Bind a Bluetooth socket to an available address. This call does not block (a
 * subsequent call to {@link bt_accept} will block).
 * 
 * @param listener The socket structure to store the listener details in.
 * 
 * @return `BT_SUCCESS` if successful.
 */
bt_err_t bt_bind(bt_socket_t * listener) {
	bt_err_t err;
#ifdef WINDOWS
	SOCKADDR_BTH loc_addr;

	err = BT_SUCCESS;

	// Allocate socket
	listener->s = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
	bt_set_timeout(listener, 20);

	// Bind socket to random port of the first available local bluetooth adapter
	memset(&loc_addr, 0, sizeof(loc_addr));
	loc_addr.addressFamily = AF_BTH;
	loc_addr.btAddr = 0;
	loc_addr.serviceClassId = GUID_NULL;
	loc_addr.port = BT_PORT_ANY;
	if (bind(listener->s, (struct sockaddr *)&loc_addr, sizeof(loc_addr))) {
		DWORD e = WSAGetLastError();
		printf("Bind failed with error: %ld\n", e);
		err = BT_ERR_UNKNOWN;
		closesocket (listener->s);
	}
#else
	struct sockaddr_rc loc_addr = { 0 };
	int result;

	err = BT_SUCCESS;

	// Allocate socket
	listener->s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	bt_set_timeout(listener, 20);

	// Bind socket to random port of the first available local bluetooth adapter
	loc_addr.rc_family = AF_BLUETOOTH;
	loc_addr.rc_bdaddr = *BDADDR_ANY;
	loc_addr.rc_channel = 0;
	//result = bind(listener->s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
	result = dynamic_bind_rc(listener->s, & loc_addr, sizeof(loc_addr), NULL);

	LOG("Bound on %d", listener->s);

	if (result < 0) {
		LOG("Failed to bind socket");
		err = BT_ERR_UNKNOWN;
	}

#endif

	return err;
}

/**
 * Bind a Bluetooth socket to an available address. This call does not block (a
 * subsequent call to {@link bt_accept} will block).
 * 
 * @param listener The socket structure to store the listener details in
 * @param channel  Which RFCOMM channel to bind to.
 * 
 * @return `BT_SUCCESS` if successful.
 */
bt_err_t bt_bind_to_channel(bt_socket_t * listener, uint8_t channel) {
	bt_err_t err;
#ifdef WINDOWS
	SOCKADDR_BTH loc_addr;

	err = BT_SUCCESS;

	// Allocate socket
	listener->s = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
	bt_set_timeout(listener, 20);

	// Bind socket to random port of the first available local bluetooth adapter
	memset(&loc_addr, 0, sizeof(loc_addr));
	loc_addr.addressFamily = AF_BTH;
	loc_addr.btAddr = 0;
	loc_addr.serviceClassId = GUID_NULL;
	loc_addr.port = channel;
	if (bind(listener->s, (struct sockaddr *)&loc_addr, sizeof(loc_addr))) {
		DWORD e = WSAGetLastError();
		printf("Bind failed with error: %ld\n", e);
		err = BT_ERR_UNKNOWN;
		closesocket (listener->s);
	}
#else
	struct sockaddr_rc loc_addr = { 0 };
	int result;

	err = BT_SUCCESS;

	// Allocate socket
	listener->s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	bt_set_timeout(listener, 20);

	// Bind socket to random port of the first available local bluetooth adapter
	loc_addr.rc_family = AF_BLUETOOTH;
	loc_addr.rc_bdaddr = *BDADDR_ANY;
	loc_addr.rc_channel = channel;
	result = bind(listener->s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));

	LOG("Bound on %d", listener->s);

	if (result < 0) {
		LOG("Failed to bind socket");
		err = BT_ERR_UNKNOWN;
	}

#endif

	return err;
}

/**
 * Listen on a bound socket. This should be called after {@link bt_bind} to
 * start listening on the socket. This call does not block (a subsequent call
 * to {@link bt_accept} will block).
 * 
 * @param listener The socket to start listening on.
 * 
 * @return `BT_SUCCESS` if successful.
 */
bt_err_t bt_listen(bt_socket_t * listener) {
	bt_err_t err;
	int result;

	err = BT_SUCCESS;

	// Put socket into listening mode
	result = listen(listener->s, 2);
	if (result < 0) {
		LOG("Failed to listen on socket, error %d: %s", errno, strerror(errno));
		err = BT_ERR_UNKNOWN;
	}
	
	return err;
}

/**
 * Accept the next connection from the listening socket. This should be
 * called after {@link bt_listen} has been called (and consequently after
 * {@link bt_bind} has been called before this).
 * This call will block until a connection is made, or the listen times out.
 * 
 * @param listener The socket that's listening for connections.
 * @param sock The structure to store the details of the next connection
 *        made on the listening socket. This will create a different socket
 *        to the listening socket.
 * @param timeout Time to wait for a connection. If NULL, we will wait
 *        indefinitely
 *
 * @return `BT_SUCCESS` if successful.
 * 	       `BT_ERR_TIMEOUT` if the clock timed out
 */
bt_err_t bt_accept_with_timeout(bt_socket_t const * listener, bt_socket_t * sock, struct timeval* timeout) {
	bt_err_t err;
	int sock_result;
	fd_set rfds;

#ifdef WINDOWS
	SOCKADDR_BTH rem_addr;
	int opt = sizeof(rem_addr);
#else
	struct sockaddr_rc rem_addr = { 0 };
	socklen_t opt = sizeof(rem_addr);
#endif
	
	err = BT_SUCCESS;

	if (err == BT_SUCCESS) {
		FD_ZERO(&rfds);
		FD_SET(listener->s, &rfds);

		sock_result = select(listener->s + 1, &rfds, NULL, NULL, timeout);
		if (sock_result > 0) {
			err = BT_SUCCESS;
		}
		else if (sock_result == 0) {
			err = BT_ERR_TIMEOUT;
		}
		else {
			err = BT_ERR_UNKNOWN;
		}
	}

	if (err == BT_SUCCESS) {
		// Accept the first connection
		sock->s = accept(listener->s, (struct sockaddr *)&rem_addr, &opt);
		LOG("Accept on %d", sock->s);

		if (sock->s < 0 || sock->s == INVALID_SOCKET) {
			LOG("Failed to accept connection, errno = %d", errno);
			err = BT_ERR_UNKNOWN;
		}
		else {
			bt_set_timeout(sock, 20);
		}
	}

	return err;

}

/**
 * Accept the next connection from the listening socket. This should be
 * called after {@link bt_listen} has been called (and consequently after
 * {@link bt_bind} has been called before this).
 * This call will block until a connection is made, or the listen times out.
 * The timeout for this function will be the same that was set using
 * {@link bt_set_timeout}
 * 
 * @param listener The socket that's listening for connections.
 * @param sock The structure to store the details of the next connection
 *        made on the listening socket. This will create a different socket
 *        to the listening socket.
 * 
 * @return `BT_SUCCESS` if successful.
 * 	       `BT_ERR_TIMEOUT` if the clock timed out
 */
bt_err_t bt_accept(bt_socket_t const * listener, bt_socket_t * sock) {
	bt_err_t err;
	struct timeval tv;
	unsigned int size_timeout;

#ifdef WINDOWS
	DWORD timeout;
#else
	struct timeval timeout;
#endif
	
	size_timeout = sizeof(timeout);
	err = BT_SUCCESS;

	if (!getsockopt(listener->s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, &size_timeout)) {
#ifdef WINDOWS
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
#else
		tv.tv_sec = timeout.tv_sec;
		tv.tv_usec = timeout.tv_usec;
#endif
	} else {
		err = BT_ERR_UNKNOWN;
	}

	err = bt_accept_with_timeout(listener, sock, &tv);

	return err;
}

/**
 * Bind to the next available port.
 * 
 * @param sock     The socket to bind.
 * @param sockaddr Structure containing details of the socket to bind (same as
 *                 bind).
 * @param addrlen  The size allocated to the `sockaddr` structure.
 * @param port     Memory to return the port number (Bluetooth channel) that's
 *                 ultimately bound. If there's an error, this will be left
 *                 unchanged. It can be `NULL`.
 * 
 * @return `0` if successful.
 */
#ifdef WINDOWS
#else // LINUX
int dynamic_bind_rc(int sock, struct sockaddr_rc * sockaddr, socklen_t addrlen, uint8_t * port) {
	int err;
	uint8_t check;
	uint8_t found;

	err = -1;
	found = 0;
	for (check = 1; (check < 32) && (err != 0); check += 1) {
		sockaddr->rc_channel = check;
		err = bind(sock, (struct sockaddr *)sockaddr, addrlen);
		if (err == 0) {
			found = check;
		}
	}

	if (check == 31) {
		err = -1;
		errno = EINVAL;
	}
	if ((err == 0) && (port != NULL)) {
		*port = found;
	}

	return err;
}
#endif

/**
 * Get the port (Bluetooth channel) for the given socket.
 * 
 * @param sock The socket to get the port/channel for.
 * 
 * @return The channel number of the socket.
 */
uint8_t bt_get_socket_channel(bt_socket_t sock) {
	uint8_t channel;
#ifdef WINDOWS
	SOCKADDR_BTH src;
	int olen;

	olen = sizeof(SOCKADDR_BTH);
	if (getsockname(sock.s, (struct sockaddr*)&src, &olen) < 0) {
		DWORD e = WSAGetLastError();
		printf("getsockname failed with error: %ld\n", e);
		channel = 0;
	}
	else {
		channel = src.port;;
	}

#else
	struct sockaddr_rc src;
	socklen_t olen;

	memset(&src, 0, sizeof(src));
	olen = sizeof(src);
	if (getsockname(sock.s, (void *)&src, &olen) < 0) {
		channel = 0;
	}
	else {
		channel = src.rc_channel;
	}

#endif
	return channel;
}

/**
 * Get the local device address.
 * 
 * @param addr Structure to return the result in.
 * 
 * @return `BT_SUCCESS` if successful.
 */
bt_err_t bt_get_device_name(bt_addr_t * addr) {
#ifdef WINDOWS
	bt_err_t err = BT_SUCCESS;
	BLUETOOTH_FIND_RADIO_PARAMS btfrp;
	btfrp.dwSize = sizeof(btfrp);
	HANDLE hRadio;
	HBLUETOOTH_RADIO_FIND hFind = BluetoothFindFirstRadio(&btfrp, &hRadio);
	BLUETOOTH_RADIO_INFO radioInfo;

	if (hFind == NULL) {
		DWORD e = GetLastError();
		switch (e) {
		case ERROR_NO_MORE_ITEMS:
			err = BT_ERR_DEVICE_NOT_FOUND;
			break;
		default:
			err = BT_ERR_UNKNOWN;
		}
	}

	if (err == BT_SUCCESS) {
		radioInfo.dwSize = sizeof(radioInfo);
		DWORD e = BluetoothGetRadioInfo(hRadio, &radioInfo);
		if (e != ERROR_SUCCESS) {
			err = BT_ERR_UNKNOWN;
		} else {
			bt_bdaddr_to_addr(radioInfo.address.ullLong, addr);
		}
	}

	BluetoothFindRadioClose(hFind);
	return BT_SUCCESS;
#else
	//struct hci_dev_info devinfo;
	bdaddr_t bdaddr;
	int dev_id;
	int result;
	bt_err_t err;

	dev_id = hci_get_route(NULL);
	//result = hci_devinfo(dev_id, &devinfo);
	result = hci_devba(dev_id, &bdaddr);

	if (addr && (result == 0)) {
		bt_bdaddr_to_addr(&bdaddr, addr);
		err = BT_SUCCESS;
	}
	else {
		err = BT_ERR_UNKNOWN;
	}
	
	return err;
#endif
}

/**
 * Register a service with local SDP server.
 * 
 * @param service      The UUID of the service to register.
 * @param service_name The name of the service to register.
 * @param sock         The Bluetooth server socket used by the service.
 * 
 * @return `BT_SUCCESS` if the service was registered.
 */
bt_err_t bt_register_service(bt_uuid_t const * service, char const * service_name, bt_socket_t *sock) {
#ifdef WINDOWS
	bt_err_t ret;
	WSAQUERYSET _service;
	GUID guid;
	bt_uuid_to_guid(service, &guid);
	memset(&_service, 0, sizeof(_service));
	_service.dwSize = sizeof(_service);
	_service.lpszServiceInstanceName = service_name;
	_service.lpszComment = L"";
	GUID serviceID = guid;
	_service.lpServiceClassId = &serviceID;
	_service.dwNumberOfCsAddrs = 1; // This member is ignored for queries.
	_service.dwNameSpace = NS_BTH;
	SOCKADDR_BTH address;
	struct sockaddr *pAddr = (struct sockaddr*)&address;
	int length = sizeof(SOCKADDR_BTH);

	ret = BT_SUCCESS;

	getsockname(sock->s, pAddr, &length);

	CSADDR_INFO csAddr;
	memset(&csAddr, 0, sizeof(csAddr));
	csAddr.LocalAddr.iSockaddrLength = sizeof(SOCKADDR_BTH);
	csAddr.LocalAddr.lpSockaddr = pAddr;
	csAddr.iSocketType = SOCK_STREAM;
	csAddr.iProtocol = BTHPROTO_RFCOMM;
	_service.lpcsaBuffer = &csAddr;

	if (0 != WSASetService(&_service, RNRSERVICE_REGISTER, 0))
	{
		ret = BT_ERR_UNKNOWN;
	}

	return ret;
#else
	bt_err_t ret;
	uint8_t rfcomm_channel;
	char const *service_dsc = "";
	char const *service_prov = "";
	uuid_t root_uuid;
	uuid_t l2cap_uuid;
	uuid_t rfcomm_uuid;
	uuid_t svc_uuid;
	sdp_list_t *l2cap_list = 0;
	sdp_list_t *rfcomm_list = 0;
	sdp_list_t *root_list = 0;
	sdp_list_t *proto_list = 0;
	sdp_list_t *access_proto_list = 0;
	sdp_data_t *channel = 0;
	sdp_data_t *data = 0;

	ret = BT_SUCCESS;

	rfcomm_channel = bt_get_socket_channel(*sock);

	bt_uuid_to_uuid(service, & svc_uuid);

	sdp_record_t *record = sdp_record_alloc();

	// General service ID
	//sdp_uuid128_create(&svc_uuid, &svc_uuid_int);
	sdp_set_service_id(record, svc_uuid);
	sdp_list_t service_class = {NULL, &svc_uuid};
	sdp_set_service_classes(record, &service_class);

	// Make publicly browsable
	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root_list = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(record, root_list);

	// Set l2cap info
	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	l2cap_list = sdp_list_append(0, &l2cap_uuid);
	proto_list = sdp_list_append(0, l2cap_list);

	// Set RFCOMM info
	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	channel = sdp_data_alloc(SDP_UINT8, &rfcomm_channel);
	rfcomm_list = sdp_list_append(0, &rfcomm_uuid);
	sdp_list_append(rfcomm_list, channel);
	sdp_list_append(proto_list, rfcomm_list);

	// Attach protocol info to service record
	access_proto_list = sdp_list_append(0, proto_list);
	sdp_set_access_protos(record, access_proto_list);

	// Set name, provider, description
	data = sdp_data_alloc_with_length(SDP_TEXT_STR8, service_name, strlen(service_name) + 1);
	sdp_attr_add(record, SDP_ATTR_SVCNAME_PRIMARY, data);
	data = sdp_data_alloc_with_length(SDP_TEXT_STR8, service_prov, strlen(service_prov) + 1);
	sdp_attr_add(record, SDP_ATTR_PROVNAME_PRIMARY, data);
	data = sdp_data_alloc_with_length(SDP_TEXT_STR8, service_dsc, strlen(service_dsc) + 1);
	sdp_attr_add(record, SDP_ATTR_SVCDESC_PRIMARY, data);
	
	int err = 0;
	sdp_session_t *session = 0;

	// Connect to local SDP server, register service record and disconnect
	session = sdp_connect(BDADDR_ANY, BDADDR_LOCAL, SDP_RETRY_IF_BUSY);
	if (session) {
		err = sdp_record_register(session, record, 0);
		if (err < 0) {
			ret = BT_ERR_UNKNOWN;
		}
	} else {
		ret = BT_ERR_UNKNOWN;
	}

	// Cleanup
	sdp_data_free(channel);
	sdp_list_free(l2cap_list, 0);
	sdp_list_free(rfcomm_list, 0);
	sdp_list_free(root_list, 0);
	sdp_list_free(access_proto_list, 0);

	return ret;
#endif
}

/**
 * Set both the read and write timeout on a connection.
 * @param sock The socket to set the timeout on.
 * @param duration Timeout duration in seconds.
 * @return `BT_SUCCESS` if successful.
 */
bt_err_t bt_set_timeout(bt_socket_t *sock, int duration) {
	bt_err_t result;
	int sockresult;

	result = BT_SUCCESS;

#ifdef WINDOWS
	DWORD timeout = duration * 1000;
#else
	struct timeval timeout;
	timeout.tv_sec = duration;
	timeout.tv_usec = 0;
#endif

	sockresult = setsockopt (sock->s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	if (sockresult < 0) {
		result = BT_ERR_UNKNOWN;
	}

	sockresult = setsockopt (sock->s, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
	if (sockresult < 0) {
		result = BT_ERR_UNKNOWN;
	}

	return result;
}

/**
 * Close a Bluetooth socket.
 * @param socket The Bluetooth socket to close
 */
void bt_disconnect(bt_socket_t *socket) {
	if (socket == NULL)
		return;
	
#ifdef WINDOWS
	if (socket->s == INVALID_SOCKET)
		return;
	// close the socket
	closesocket(socket->s);
	socket->s = INVALID_SOCKET;
	
#else // LINUX
	if (socket->s == INVALID_SOCKET)
		return;
	// close the socket
	LOG("Close socket %d\n", socket->s);
	close(socket->s);
	socket->s = INVALID_SOCKET;
#endif
}

/**
 * Read data from a Bluetooth socket.
 * This functions is basically a proxy the socket recv function.
 * 
 * @param socket   The socket to read from.
 * @param buffer   Pointer to buffer in which to put received data.
 * @param numBytes Pointer to number of bytes to receive. On return, this will
 *                 be set to the actual number of bytes received.
 * 
 * @return `BT_SUCCESS` if successful,
 *    `BT_SOCKET_CLOSED` if the socket was closed, or one of the following if
 *     there's an error:
 *    `BT_SOCKET_CLOSED`       - Socket was closed during read
 *    `BT_ERR_UNKNOWN`         - unhelpfully generic failure
 *    `BT_ERR_BAD_PARAM`       - One of the parameters was NULL
 */
bt_err_t bt_recv(bt_socket_t *socket, void *buffer, size_t *numBytes) {
	// this is one of the few bits that is actually the same on Windows and Linux (for now)
	int n;
	size_t bytesToRead = *numBytes;

	// check parameters
	if (socket == NULL || buffer == NULL || (numBytes == NULL)) {
		LOG("bt_recv: error reading from socket: bad parameters\n");
		return BT_ERR_BAD_PARAM;
	}

	*numBytes = 0;
	n = recv(socket->s, buffer, bytesToRead, 0);
	if (n == 0) {
		// socket has been closed
		LOG("bt_recv: socket %d closed on write (returned 0 bytes)\n", socket->s);
		return BT_SOCKET_CLOSED;
	} else if (n < 0) {
		LOG("bt_recv: error %d reading from socket %d\n", ERRNO, socket->s);
		if (ERRNO == ECONNRESET) {
			return BT_SOCKET_CLOSED;
		}
		else {
			// error
			return BT_ERR_UNKNOWN;
		}
	}

	*numBytes = n;
	return BT_SUCCESS;
}

/**
 * Read data from a Bluetooth socket.
 * The exact semantics of this function are perhaps not as intuitive as they
 * might be: the call will block until either the socket gets closed, or the
 * desired number of bytes has been read. It will NOT return as soon as any
 * amount of data is available. The `bt_recv()` function should be used for
 * this.
 * 
 * @param socket   The socket to read from.
 * @param buffer   Pointer to buffer in which to put received data.
 * @param numBytes Pointer to number of bytes to receive. On return, this will
 *                 be set to the actual number of bytes received.
 * 
 * @return `BT_SUCCESS` if successful,
 *    `BT_SOCKET_CLOSED` if the socket was closed, or one of the following if
 *     there's an error:
 *    `BT_SOCKET_CLOSED`       - Socket was closed during read
 *    `BT_ERR_UNKNOWN`         - unhelpfully generic failure
 *    `BT_ERR_BAD_PARAM`       - One of the parameters was NULL
 */
bt_err_t bt_read(bt_socket_t *socket, void *buffer, size_t *numBytes) {
	size_t remaining = *numBytes;
	size_t received = 0;
	size_t n;

	bt_err_t e;
	
	// check parameters
	if (socket == NULL || buffer == NULL || (numBytes == NULL)) {
		LOG("bt_read: error reading from socket: bad parameters\n");
		return BT_ERR_BAD_PARAM;
	}

	while (remaining > 0) {
		n = remaining;
		e = bt_recv(socket, buffer, &n);

		if (e != BT_SUCCESS) {
			// Error log will be generated by bt_recv(), so no need to duplicate
			*numBytes = received;
			return e;
		}

		received += n;
		buffer = (char*) buffer + n;
		remaining -= n;
	}

	*numBytes = received;
	return BT_SUCCESS;
}

/**
 * Write data to a Bluetooth socket.
 * This functions is basically a proxy the socket send function.
 *
 * @param socket   The socket to write to.
 * @param buffer   Pointer to buffer containing data to send.
 * @param numBytes Pointer to number of bytes to send. On return, this will
 *                 be set to the actual number of bytes sent.
 *
 * @return `BT_SUCCESS` if successful,
 *    `BT_SOCKET_CLOSED`       - Socket was closed during read
 *     or one of the following if there's an error:
 *    `BT_ERR_UNKNOWN`         - unhelpfully generic failure
 *    `BT_ERR_BAD_PARAM`       - One of the parameters was NULL
 */
bt_err_t bt_send(bt_socket_t *socket, const void *buffer, size_t *numBytes) {
	// this is one of the few bits that is actually the same on Windows and Linux (for now)
	int n;
	size_t bytesToSend = *numBytes;

	// check parameters
	if (socket == NULL || buffer == NULL || (numBytes == NULL)) {
		LOG("bt_send: error writing to socket: bad parameters\n");
		return BT_ERR_BAD_PARAM;
	}

	*numBytes = 0;
	n = send(socket->s, buffer, bytesToSend, 0);
	if (n == 0) {
		// socket has been closed
		LOG("bt_send: socket %d closed on write (returned 0 bytes)\n", socket->s);
		return BT_SOCKET_CLOSED;
	} else if (n < 0) {
		LOG("bt_send: error %d writing to socket %d\n", ERRNO, socket->s);
		if (ERRNO == ECONNRESET) {
			return BT_SOCKET_CLOSED;
		}
		else {
			// error
			return BT_ERR_UNKNOWN;
		}
	}

	*numBytes = n;
	return BT_SUCCESS;
}

/**
 * Write data to a Bluetooth socket. This functions guarantees to write
 * the specified amount. Unless some error happens.
 *
 * @param socket The socket to send to.
 * @param buffer Pointer to the data to send.
 * @param numBytes Number of bytes to send.
 *
 * @return `BT_SUCCESS` if successful,
 *    `BT_SOCKET_CLOSED`       - Socket was closed during read
 *     or one of the following if there's an error:
 *    `BT_ERR_UNKNOWN`         - unhelpfully generic failure
 *    `BT_ERR_BAD_PARAM`       - One of the parameters was NULL
 */
bt_err_t bt_write(bt_socket_t *socket, const void *buffer, size_t numBytes) {
#ifdef WINDOWS
	size_t remaining = numBytes;
	size_t sent = 0;
	size_t n;
	bt_err_t e;
	
	// check parameters
	if (socket == NULL || buffer == NULL) {
		LOG("bt_write: error writing to socket: bad parameters\n");
		return BT_ERR_BAD_PARAM;
	}

	// the send function takes size as an int, so check the given size_t is safe
	if (numBytes > INT_MAX) {
		LOG("bt_write: error writing to socket %d: data too large\n", socket->s);
		return BT_ERR_BAD_PARAM;
	}
	
	// loop in case send doesn't send everything at once for whatever reason
	while (remaining > 0) {
		n = remaining;
		// try to send the data
		e = bt_send(socket, buffer, &n);

		if (e != BT_SUCCESS) {
			int error = WSAGetLastError();
			switch (error) {
			case WSANOTINITIALISED:
				LOG("bt_write: error WSANOTINITIALISED sending to socket %d\n", socket->s);
				return BT_ERR_UNINITIALISED;
			default:
				LOG("bt_write: error %d sending to socket %d\n", error, socket->s);
			}
			return e;
		}

		sent += n;
		buffer = (char*) buffer + n;
		remaining -= n;
	}

	return BT_SUCCESS;
	
#else // LINUX
	size_t remaining = numBytes;
	size_t sent = 0;
	size_t n;
	bt_err_t e;
	
	// check parameters
	if (socket == NULL || buffer == NULL) {
		LOG("bt_write: error writing to socket: bad parameters\n");
		return BT_ERR_BAD_PARAM;
	}

	// loop in case send doesn't send everything at once for whatever reason
	while (remaining > 0) {
		n = remaining;
		// try to send the data
		e = bt_send(socket, buffer, &n);

		if (e != BT_SUCCESS) {
			// Error log will be generated by bt_send(), so no need to duplicate
			return e;
		}

		sent += n;
		buffer = (char*) buffer + n;
		remaining -= n;
	}

	return BT_SUCCESS;
#endif
}


