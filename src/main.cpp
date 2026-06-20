#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/ethernet.h>

#include <memory>
#include <thread>

#include "ihm/rgb.h"
#include "net/EthernetRouter.h"

#include "audio/i2s.h"
#include "audio/pre.h"
#include "audio/pre_phydef.h"

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

THREAD_DEF(mapper, 2048);
THREAD_DEF(control_handler, 1024);
THREAD_DEF(life, 512);

phy_pre_t pre1 = DT_GET_PREAMP(pre1);
phy_pre_t pre2 = DT_GET_PREAMP(pre2);
phy_pre_t pre3 = DT_GET_PREAMP(pre3);
phy_pre_t pre4 = DT_GET_PREAMP(pre4);
phy_pre_t pre5 = DT_GET_PREAMP(pre5);
phy_pre_t pre6 = DT_GET_PREAMP(pre6);
phy_pre_t pre7 = DT_GET_PREAMP(pre7);
phy_pre_t pre8 = DT_GET_PREAMP(pre8);

std::vector<Preamp> preamps_control;

void mapper_entry(void* mapper, void*, void*) {
    NetworkMapper* nmapper = reinterpret_cast<NetworkMapper*>(mapper);

    uint64_t last_rx = NetworkMapper::local_now();
    uint64_t last_tx = NetworkMapper::local_now();
    while (true) {
        auto now = NetworkMapper::local_now();
        if (now - last_tx > 5000) {
            nmapper->packet_send_update();
            last_tx = now;
        }

        if (now - last_rx > 100) {
            nmapper->packet_recv_update();
            nmapper->mapper_update();
            last_rx = now;
        }


        k_msleep(100);
    }
}

void life_entry(void*, void*, void*) {
    while (true) {
        set_led_color(LedColor::CYAN);
        k_msleep(500);
        set_led_color(LedColor::PINK);
        k_msleep(500);
    }
}

void process_gain(const LowLatPacket<ControlPacket>* pck) {
    if (pck->payload.packet_data.channel >= I2SBoardConfig::max_channels) {
        return;
    }

    volatile float raw_gain = 0.0f;
    memcpy((void*)&raw_gain, &pck->payload.packet_data.data[0], sizeof(float));

    preamps_control[pck->payload.packet_data.channel].update_gain(raw_gain);
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
        // because we only need to read those
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

    std::shared_ptr<NetworkMapper> nmapper = std::make_shared<NetworkMapper>(pconf);
    if (!nmapper->init_mapper("")) {
        set_led_color(LedColor::RED);

        return 0;
    }

    auto audio_socket = std::make_shared<LowLatSocket>(pconf.uid, nmapper);
    audio_socket->init_socket("", EthProtocol::ETH_PROTO_OANAUDIO);

    preamps_control.emplace_back(AnalogDigitalGain{GainValue::GAIN_1, 1.0f}, &pre1, get_stream(0), audio_socket, 0);
    preamps_control.emplace_back(AnalogDigitalGain{GainValue::GAIN_1, 1.0f}, &pre2, get_stream(1), audio_socket, 1);
    preamps_control.emplace_back(AnalogDigitalGain{GainValue::GAIN_1, 1.0f}, &pre3, get_stream(2), audio_socket, 2);
    preamps_control.emplace_back(AnalogDigitalGain{GainValue::GAIN_1, 1.0f}, &pre4, get_stream(3), audio_socket, 3);
    preamps_control.emplace_back(AnalogDigitalGain{GainValue::GAIN_1, 1.0f}, &pre5, get_stream(4), audio_socket, 4);
    preamps_control.emplace_back(AnalogDigitalGain{GainValue::GAIN_1, 1.0f}, &pre6, get_stream(5), audio_socket, 5);
    preamps_control.emplace_back(AnalogDigitalGain{GainValue::GAIN_1, 1.0f}, &pre7, get_stream(6), audio_socket, 6);
    preamps_control.emplace_back(AnalogDigitalGain{GainValue::GAIN_1, 1.0f}, &pre8, get_stream(7), audio_socket, 7);

    configure_board_i2s(&preamps_control);
    set_led_color(LedColor::GREEN);

    THREAD_START(mapper, mapper_entry, nmapper.get(), nullptr, nullptr, 15);
    THREAD_START(control_handler, control_handler_entry, &pconf, nullptr, nullptr, 5);
    THREAD_START(life, life_entry, nullptr, nullptr, nullptr, 20);

    start_i2s_all();
    while (true) {
        for (auto& pre : preamps_control) {
            pre.process_stream();
        }

        k_usleep(100);
    }

    return 0;
}