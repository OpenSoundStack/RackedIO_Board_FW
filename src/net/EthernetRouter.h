#ifndef RACKEDIO_BOARD_FIRMWARE_ETHERNETROUTER_H
#define RACKEDIO_BOARD_FIRMWARE_ETHERNETROUTER_H

#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <OpenAudioNetwork/netutils/LowLatSocket.h>

extern "C" void eth_filtering_setup(); // C/C++ compat issue

class EthernetRouter {
public:
    EthernetRouter();
    ~EthernetRouter() = default;

    void init_router();

    static EthernetRouter* get_eth_router() {
        static EthernetRouter* eth_router = nullptr;

        if (eth_router == nullptr) {
            eth_router = new EthernetRouter();
        }

        return eth_router;
    }

    int send(uint8_t* data, size_t data_len);
private:
    void install_enet_filters();
    void init_raw_sock();
    void init_out_addr();

    int m_sock_eth_if;
    sockaddr_ll m_out_addr;
};


#endif //RACKEDIO_BOARD_FIRMWARE_ETHERNETROUTER_H
