#include "EthernetRouter.h"

/*
 *  NET FILTERING
 */

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
    return 0;
}

EthernetRouter::EthernetRouter() {

}

void EthernetRouter::install_enet_filters() {
    eth_filtering_setup();
}

void EthernetRouter::init_router() {
    install_enet_filters();
    init_raw_sock();
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
