#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/ethernet.h>

#include <memory>
#include <thread>

#include "ihm/rgb.h"
#include "net/EthernetRouter.h"
#include "audio/i2s.h"

#include "OpenAudioNetwork/common/NetworkMapper.h"

K_THREAD_STACK_DEFINE(mapper_rx_stack, 2048);
k_tid_t mapper_rx_th_id;
k_thread mapper_rx_thread;

K_THREAD_STACK_DEFINE(mapper_tx_stack, 2048);
k_tid_t mapper_tx_th_id;
k_thread mapper_tx_thread;

K_THREAD_STACK_DEFINE(life_stack, 512);
k_tid_t life_th_id;
k_thread life_thread;

void mapper_rx_entry(void* mapper, void*, void*) {
    NetworkMapper* nmapper = reinterpret_cast<NetworkMapper*>(mapper);

    while (true) {
        nmapper->packet_recv_update();
        k_msleep(100);
    }
}

void mapper_tx_entry(void* mapper, void*, void*) {
    NetworkMapper* nmapper = reinterpret_cast<NetworkMapper*>(mapper);

    while (true) {
        nmapper->packet_send_update();
        k_msleep(5000);
    }
}

void life_entry(void*, void*, void*) {
    while (true) {
        set_led_color(LedColor::CYAN);
        k_msleep(250);
        set_led_color(LedColor::PINK);
        k_msleep(250);
    }
}

void dummy() {
    k_usleep(100);
}

int main() {
    init_leds();
    set_led_color(LedColor::YELLOW);

    configure_board_i2s();
    ll_i2s_start(SPI1);

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

    mapper_tx_th_id = k_thread_create(
        &mapper_tx_thread,
        mapper_tx_stack,
        K_THREAD_STACK_SIZEOF(mapper_tx_stack),
        &mapper_tx_entry,
        &nmapper, nullptr, nullptr,
        5, 0, K_NO_WAIT
    );

    life_th_id = k_thread_create(
        &life_thread,
        life_stack,
        K_THREAD_STACK_SIZEOF(life_stack),
        &life_entry,
        nullptr, nullptr, nullptr,
        5, 0, K_NO_WAIT
    );

    auto* pre1_pipe = get_stream(0);
    float data[64] = {0};
    int idx = 0;
    int color = 0;

    while (true) {
        k_pipe_read(pre1_pipe, (uint8_t*)(data + idx), sizeof(float), K_FOREVER);
        idx++;
        if (idx == 64) {
            idx = 0;
        }
    }

    return 0;
}