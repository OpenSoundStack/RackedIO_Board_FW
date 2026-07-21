#ifndef RACKEDIO_BOARD_FIRMWARE_PRE_H
#define RACKEDIO_BOARD_FIRMWARE_PRE_H

#include <cstdint>
#include <span>

#include <zephyr/drivers/gpio.h>
#include <zephyr/net/net_pkt.h>

#include "pre_phydef.h"
#include "src/net/EthernetRouter.h"

#include <OpenAudioNetwork/common/packet_structs.h>
#include <OpenAudioNetwork/common/ClockSlave.h>
#include <OpenAudioNetwork/common/NetworkMapper.h>
#include <OpenAudioNetwork/netutils/LowLatSocket.h>

enum GainValue : uint8_t {
    GAIN_1 = 0x00,
    GAIN_10 = 0x01,
    GAIN_100 = 0x02,
    GAIN_1000 = 0x03
};

struct AnalogDigitalGain {
    GainValue analog_gain;
    float digital_gain;
};

class Preamp {
public:
    Preamp(
        AnalogDigitalGain init_gain,
        const phy_pre_t* pre_io,
        std::shared_ptr<LowLatSocket> audio_socket,
        uint8_t channel,
        std::shared_ptr<ClockSlave> clk_slave
    );

    ~Preamp() = default;

    void update_gain(float raw_gain);
    float get_digital_gain() const;

    void fire_audio_packet();
    float* get_audio_buffer();
private:
    AnalogDigitalGain find_gain_from_value(float raw_gain);
    void apply_to_io();
    void init_audio_packets();
    void send_audio_packet(int idx, uint8_t dest);

    std::shared_ptr<ClockSlave> m_clk_slave;
    std::shared_ptr<LowLatSocket> m_audio_socket;
    EthernetRouter* m_router;

    AnalogDigitalGain m_ad_gain_value;
    const phy_pre_t* m_pre_io;

    LowLatPacket<AudioPacket> m_packets[2];
    int m_sample_index;
    int m_buffer_index;

    uint8_t m_channel;
};

#endif //RACKEDIO_BOARD_FIRMWARE_PRE_H
