#include "mockbluez.h"
#include <stdlib.h>

int hci_get_route_default (bdaddr_t *bdaddr) {
	return 0;
}

BluezFunctions bz_funcs = {
	.hci_get_route = hci_get_route_default,
	.hci_open_dev = NULL,
	.hci_devba = NULL,
	.hci_inquiry = NULL,
	.hci_read_remote_name = NULL,
	.sdp_connect = NULL,
	.sdp_service_search_attr_req = NULL,
	.sdp_close = NULL,
	.socket = NULL,
	.connect = NULL,
	.bind = NULL,
	.listen = NULL,
	.select = NULL,
	.accept = NULL,
	.recv = NULL,
	.send = NULL,
	.getsockname = NULL,
	.sdp_record_register = NULL,
	.getsockopt = NULL,
};

#define FUNCTION_BODY(name, ...)\
{ \
	if (!bz_funcs.name) { \
		printf("UNEXPECTED NULL Function pointer %s\n", #name); \
	} \
	return bz_funcs.name (__VA_ARGS__); \
}

#define FUNCTION1(ret, mockedfunc, A1) \
	ret mockedfunc (A1 _1) \
	FUNCTION_BODY(mockedfunc, _1)

#define FUNCTION2(ret, mockedfunc, A1, A2) \
	ret mockedfunc (A1 _1, A2 _2) \
	FUNCTION_BODY(mockedfunc, _1, _2)

#define FUNCTION3(ret, mockedfunc, A1, A2, A3) \
	ret mockedfunc (A1 _1, A2 _2, A3 _3) \
	FUNCTION_BODY(mockedfunc, _1, _2, _3)

#define FUNCTION4(ret, mockedfunc, A1, A2, A3, A4) \
	ret mockedfunc (A1 _1, A2 _2, A3 _3, A4 _4) \
	FUNCTION_BODY(mockedfunc, _1, _2, _3, _4)

#define FUNCTION5(ret, mockedfunc, A1, A2, A3, A4, A5) \
	ret mockedfunc (A1 _1, A2 _2, A3 _3, A4 _4, A5 _5) \
	FUNCTION_BODY(mockedfunc, _1, _2, _3, _4, _5)

#define FUNCTION6(ret, mockedfunc, A1, A2, A3, A4, A5, A6) \
	ret mockedfunc (A1 _1, A2 _2, A3 _3, A4 _4, A5 _5, A6 _6) \
	FUNCTION_BODY(mockedfunc, _1, _2, _3, _4, _5, _6)

FUNCTION1(int, hci_get_route, bdaddr_t*)
FUNCTION1(int, hci_open_dev, int)
FUNCTION2(int, hci_devba, int, bdaddr_t*)
FUNCTION6(int, hci_inquiry, int, int, int, const uint8_t *, inquiry_info **, long)
FUNCTION5(int, hci_read_remote_name, int, const bdaddr_t*, int, char*, int)
FUNCTION3(sdp_session_t*, sdp_connect, const bdaddr_t*, const bdaddr_t*, uint32_t)
FUNCTION5(int, sdp_service_search_attr_req, sdp_session_t*, const sdp_list_t *, sdp_attrreq_type_t, const sdp_list_t*, sdp_list_t**)
FUNCTION1(int, sdp_close, sdp_session_t*)

FUNCTION3(int, socket, int, int, int)
FUNCTION3(int, connect, int, const struct sockaddr*, socklen_t)
FUNCTION3(int, bind, int, const struct sockaddr*, socklen_t)
FUNCTION2(int, listen ,int, int)
FUNCTION5(int, select, int, fd_set*, fd_set*, fd_set*, struct timeval*) 
FUNCTION3(int, accept, int, struct sockaddr*, socklen_t*)
FUNCTION4(ssize_t, recv, int, void*, size_t, int)
FUNCTION4(ssize_t, send, int, const void*, size_t, int)
FUNCTION1(int, close, int)
FUNCTION3(int, getsockname, int, struct sockaddr*, socklen_t*)
FUNCTION3(int, sdp_record_register, sdp_session_t*, sdp_record_t*, uint8_t);
FUNCTION5(int, getsockopt, int, int, int, void*, socklen_t*);

