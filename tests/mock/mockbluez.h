#include <picobt/bt.h>
#include <picobt/devicelist.h>
#include <sys/socket.h>

typedef struct {
	int (*hci_get_route) (bdaddr_t *bdaddr);
	int (*hci_open_dev) (int dev_id);
	int (*hci_devba) (int dev_id, bdaddr_t *bdaddr);
	int (*hci_inquiry) (int dev_id, int len, int num_rsp, const uint8_t *lap, inquiry_info **ii, long flags);
	int (*hci_read_remote_name) (int sock, const bdaddr_t *ba, int len, char *name, int timeout);
	sdp_session_t* (*sdp_connect) (const bdaddr_t *src, const bdaddr_t *dst, uint32_t flags);
	int (*sdp_service_search_attr_req) (sdp_session_t *session, const sdp_list_t *search, sdp_attrreq_type_t reqtype, const sdp_list_t *attrid_list, sdp_list_t **rsp_list);
	int (*sdp_close) (sdp_session_t *session);
	int (*sdp_record_register) (sdp_session_t *session, sdp_record_t *rec, uint8_t flags);

	int (*socket) (int domain, int type, int protocol);
	int (*connect) (int sockfd, const struct sockaddr *addr, socklen_t addrlen);
	int (*bind) (int sockfd, const struct sockaddr *addr, socklen_t addrlen);
	int (*listen) (int sockfd, int backlog);
	int (*select) (int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
	int (*accept) (int sockfd, struct sockaddr *addr, socklen_t *addrlen);
	ssize_t (*recv) (int sockfd, void *buf, size_t len, int flags);
	ssize_t (*send) (int sockfd, const void *buf, size_t len, int flags);
	int (*close) (int sockfd);
	int (*getsockname) (int sockfd, struct sockaddr *addr, socklen_t *addrlen);
	int (*getsockopt) (int sockfd, int level, int optname, void *optval, socklen_t *optlen);

} BluezFunctions;

extern BluezFunctions bz_funcs;

