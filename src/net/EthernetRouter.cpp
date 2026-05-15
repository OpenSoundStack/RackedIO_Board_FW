#include "EthernetRouter.h"

/*
 *  Eth Queues
 */

std::vector<RingBuffer> eth_global_queue_buffers{};

/*
 * NET Abstraction functions
 */

extern "C" IfaceMeta _fetch_iface_meta(std::string& name) {
    IfaceMeta meta{};

    net_if* iface = net_if_get_default();
    net_linkaddr* iface_addr = net_if_get_link_addr(iface);

    meta.idx = net_if_get_by_iface(iface);
    memcpy(meta.mac, iface_addr->addr, 6);

    return meta;
}

extern "C" int _send_data(uint8_t* data, size_t data_len) {
    auto* router = EthernetRouter::get_eth_router();
    return router->send(data, data_len);
}

extern "C" int _recv_data(uint8_t* data_out, size_t data_size, EthProtocol proto) {
    for (auto& q : eth_global_queue_buffers) {
        if (q.can_pop() && q.get_filt_proto() == proto) {
            DataPtr dptr = q.pop_from_buffer();
            size_t cpy_size = dptr.sz > data_size ? data_size : dptr.sz;

            memcpy(data_out, dptr.ptr, cpy_size);
            return cpy_size;
        }
    }

    return 0;
}

extern "C" net_socket_service_desc sock_rx_svc;
extern "C" void sock_rx_service_handler(net_socket_service_event* ev) {
    auto* router = EthernetRouter::get_eth_router();

    static char inbuffer[300];

    auto len = router->recv_data_raw(ev, inbuffer, 300);
    if (len > 0) {
        INT_LLP<300>* data = reinterpret_cast<INT_LLP<300>*>(inbuffer);
        for (auto& q : eth_global_queue_buffers) {
            if (ntohs(data->eth_header.h_proto) == q.get_filt_proto()) {
                q.push_on_buffer((uint8_t*)inbuffer, len);
            }
        }
    }
}

/*
 * Ethernet router code
 */

void EthernetRouter::install_enet_filters() {
    eth_filtering_setup();
}

void EthernetRouter::init_router() {
    install_enet_filters();
    init_queues();
    init_raw_sock();
    init_service();
    init_out_addr();
}

void EthernetRouter::init_out_addr() {
    std::string dummy = "";
    auto meta = _fetch_iface_meta(dummy);

    m_out_addr = {0};
    m_out_addr.sll_family = NET_AF_PACKET;
    m_out_addr.sll_ifindex = meta.idx;
    memcpy(m_out_addr.sll_addr, meta.mac, 6);
    m_out_addr.sll_halen = NET_ETH_ADDR_LEN;
}

void EthernetRouter::init_raw_sock() {
    m_sock_eth_if = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
}

int EthernetRouter::send(uint8_t *data, size_t data_len) {
    return sendto(
        m_sock_eth_if,
        data, data_len,
        MSG_DONTWAIT,
        (sockaddr*)&m_out_addr, sizeof(sockaddr_ll)
    );
}

void EthernetRouter::init_queues() {
    constexpr EthProtocol protocols[] = {
        EthProtocol::ETH_PROTO_OANDISCO,
        EthProtocol::ETH_PROTO_OANAUDIO,
        EthProtocol::ETH_PROTO_OANCONTROL,
        EthProtocol::ETH_PROTO_OANSYNC,
    };

    for (int i = 0; i < 4; i++) {
        eth_global_queue_buffers.emplace_back(300, 10, protocols[i]);
    }
}

void EthernetRouter::init_service() {
    auto sockfd_data = pollfd {
        m_sock_eth_if,
        POLLIN
    };

    net_socket_service_register(&sock_rx_svc, &sockfd_data, 1, NULL);
}

int EthernetRouter::recv_data_raw(net_socket_service_event* ev, char *data, size_t len) {
    int sock = ev->event.fd;
    [[maybe_unused]] net_sockaddr src_addr;
    [[maybe_unused]] net_socklen_t src_addr_len;

    return recvfrom(sock, data, len, 0, &src_addr, &src_addr_len);
}
