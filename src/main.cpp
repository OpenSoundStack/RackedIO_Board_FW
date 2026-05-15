#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/ethernet.h>

#include <memory>
#include <thread>

#include "ihm/rgb.h"
#include "net/EthernetRouter.h"

#include "OpenAudioNetwork/common/NetworkMapper.h"

K_THREAD_STACK_DEFINE(mapper_rx_stack, 2048);
k_tid_t mapper_rx_th_id;
k_thread mapper_rx_thread;

void mapper_rx_entry(void* mapper, void*, void*) {
    NetworkMapper* nmapper = reinterpret_cast<NetworkMapper*>(mapper);

    while (true) {
        nmapper->packet_recv_update();
        k_msleep(10);
    }
}

int main() {
    init_leds();
    set_led_color(LedColor::YELLOW);

    EthernetRouter* router = EthernetRouter::get_eth_router();
    router->init_router();

    char dev_name[32] = "Core I Racked IO";

    PeerConf pconf{};
    pconf.ck_type = CKTYPE_SLAVE;
    pconf.dev_type = DeviceType::AUDIO_IO_INTERFACE;
    pconf.iface = "";
    pconf.sample_rate = SamplingRate::SAMPLING_96K;
    pconf.topo.phy_in_count = 8;
    pconf.topo.phy_out_count = 0;
    pconf.topo.pipes_count = 0;
    pconf.uid = 10;
    memcpy(pconf.dev_name, dev_name, 32);

    NetworkMapper nmapper = NetworkMapper(pconf);
    if (!nmapper.init_mapper("")) {
        set_led_color(LedColor::RED);

        return 0;
    }

    set_led_color(LedColor::GREEN);

    mapper_rx_th_id = k_thread_create(
        &mapper_rx_thread,
        mapper_rx_stack,
        K_THREAD_STACK_SIZEOF(mapper_rx_stack),
        &mapper_rx_entry,
        &nmapper, nullptr, nullptr,
        5, 0, K_NO_WAIT
    );

    while (true) {
        nmapper.packet_send_update();
        k_msleep(5000);
    }

    return 0;
}