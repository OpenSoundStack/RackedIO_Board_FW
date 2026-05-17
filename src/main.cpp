#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/ethernet.h>

#include <memory>
#include <thread>

#include "ihm/rgb.h"
#include "net/EthernetRouter.h"
#include "audio/i2s.h"

#include "OpenAudioNetwork/common/NetworkMapper.h"

#define THREAD_DEF(name, stack_size) K_THREAD_STACK_DEFINE(name##_stack, (stack_size)); \
    k_tid_t name##_th_id; \
    k_thread name##_thread;

#define THREAD_START(name, entry, a, b, c, prio) name##_th_id = k_thread_create( \
    & name##_thread,\
    name##_stack, K_THREAD_STACK_SIZEOF(name##_stack), \
    & entry, \
    (a), (b), (c), \
    prio, 0, K_NO_WAIT);

THREAD_DEF(mapper_rx, 2048);
THREAD_DEF(mapper_tx, 2048);
THREAD_DEF(control_handler, 1024);
THREAD_DEF(life, 512);

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

void process_gain(const LowLatPacket<ControlPacket>* pck) {
    if (pck->payload.packet_data.channel >= I2SBoardConfig::max_channels) {
        return;
    }

    set_led_color(LedColor::WHITE);
}

void control_handler_entry(void* self_conf, void*, void*) {
    auto* pconf = reinterpret_cast<PeerConf*>(self_conf);
    auto* router = EthernetRouter::get_eth_router();
    static LowLatPacket<ControlPacket> local_packet;

    while (true) {
        router->wait_control_ev();

        // We received one control packet
        // Read it and dispatch it
        //
        // We skip the whole Socket infrastructure
        // beacause we only need to read those
        // packets

        router->read_control_packet(&local_packet);
        if ((local_packet.llhdr.dest_uid == pconf->uid) &&
            (local_packet.payload.header.type == PacketType::CONTROL) &&
            (local_packet.payload.packet_data.control_type == DataTypes::FLOAT)
        ) {
            process_gain(&local_packet);
        }
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

    THREAD_START(mapper_rx, mapper_rx_entry, &nmapper, nullptr, nullptr, 5);
    THREAD_START(mapper_tx, mapper_tx_entry, &nmapper, nullptr, nullptr, 5);
    THREAD_START(control_handler, control_handler_entry, &pconf, nullptr, nullptr, 2);
    THREAD_START(life, life_entry, nullptr, nullptr, nullptr, 10);

    auto* pre1_pipe = get_stream(0);
    float data[64] = {0};
    int idx = 0;

    while (true) {
        k_pipe_read(pre1_pipe, (uint8_t*)(data + idx), sizeof(float), K_FOREVER);
        idx++;
        if (idx == 64) {
            idx = 0;
        }
    }

    return 0;
}