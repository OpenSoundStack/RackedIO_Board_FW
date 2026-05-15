#include <zephyr/net/socket_service.h>

extern void sock_rx_service_handler(struct net_socket_service_event*);
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(sock_rx_svc, sock_rx_service_handler, 1);