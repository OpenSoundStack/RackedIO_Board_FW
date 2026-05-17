#ifndef RACKEDIO_BOARD_FIRMWARE_PRE_H
#define RACKEDIO_BOARD_FIRMWARE_PRE_H

#include <cstdint>

#include <zephyr/drivers/gpio.h>

#include "pre_phydef.h"

#include <OpenAudioNetwork/common/packet_structs.h>
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
        k_pipe* stream,
        std::shared_ptr<LowLatSocket> audio_socket,
        uint8_t channel
    );

    ~Preamp() = default;

    void update_gain(float raw_gain);
    float get_digital_gain() const;

    void process_stream();
private:
    AnalogDigitalGain find_gain_from_value(float raw_gain);
    void apply_to_io();
    void init_audio_packets();
    void send_audio_packet(int idx, uint8_t dest);

    std::shared_ptr<LowLatSocket> m_audio_socket;

    AnalogDigitalGain m_ad_gain_value;
    const phy_pre_t* m_pre_io;
    k_pipe* m_stream;

    AudioPacket m_packets[2];
    int m_sample_index;
    int m_buffer_index;

    uint8_t m_channel;
};

#endif //RACKEDIO_BOARD_FIRMWARE_PRE_H
