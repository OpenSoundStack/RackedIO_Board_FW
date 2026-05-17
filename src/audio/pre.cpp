#include "pre.h"

Preamp::Preamp(
    AnalogDigitalGain init_gain,
    const phy_pre_t* pre_io,
    k_pipe* stream,
    std::shared_ptr<LowLatSocket> audio_socket,
    uint8_t channel
) {
    m_ad_gain_value = init_gain;

    m_pre_io = pre_io;
    gpio_pin_configure_dt(&pre_io->b0, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&pre_io->b1, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&pre_io->en_48v, GPIO_OUTPUT_INACTIVE);

    m_stream = stream;

    m_sample_index = 0;
    m_buffer_index = 0;

    m_audio_socket = audio_socket;
    m_channel = channel;

    init_audio_packets();
}

AnalogDigitalGain Preamp::find_gain_from_value(float raw_gain) {
    AnalogDigitalGain adg{};

    if (raw_gain < 10.0f) {
        adg.analog_gain = GainValue::GAIN_1;
        adg.digital_gain = raw_gain;
    } else if (raw_gain < 100.0f) {
        adg.analog_gain = GainValue::GAIN_10;
        adg.digital_gain = raw_gain / 10.0f;
    } else if (raw_gain < 1000.0f) {
        adg.analog_gain = GainValue::GAIN_100;
        adg.digital_gain = raw_gain / 100.0f;
    } else if (raw_gain >= 1000.0f) {
        adg.analog_gain = GainValue::GAIN_1000;
        adg.digital_gain = raw_gain / 1000.0f;
    }

    return adg;
}

void Preamp::update_gain(float raw_gain) {
    m_ad_gain_value = find_gain_from_value(raw_gain);
    apply_to_io();
}

float Preamp::get_digital_gain() const {
    return m_ad_gain_value.digital_gain;
}

void Preamp::apply_to_io() {
    gpio_pin_set_dt(&m_pre_io->b0, m_ad_gain_value.analog_gain & 0x01);
    gpio_pin_set_dt(&m_pre_io->b1, m_ad_gain_value.analog_gain & 0x02);
}

void Preamp::process_stream() {
    auto ret = k_pipe_read(
        m_stream,
        reinterpret_cast<uint8_t*>(m_packets[m_buffer_index].packet_data.samples + m_sample_index),
        AUDIO_DATA_SAMPLES_PER_PACKETS * sizeof(float),
        K_NO_WAIT
    );

    if (ret > 0) {
        int read_sample_count = ret / 4;
        m_sample_index = (m_sample_index + read_sample_count) % AUDIO_DATA_SAMPLES_PER_PACKETS;
        if (m_sample_index == 0) {
            send_audio_packet(m_buffer_index, 100);
            m_buffer_index = (m_buffer_index + 1) % 2;
        }
    }
}

void Preamp::init_audio_packets() {
    for (int i = 0; i < 2; i++) {
        m_packets[i].header.type = PacketType::AUDIO;
        m_packets[i].header.version = 0;
        m_packets[i].header.timestamp = 0;
        m_packets[i].header.prev_delay = 0;
        m_packets[i].header.flags = 0;

        m_packets[i].packet_data.channel = m_channel;
        m_packets[i].packet_data.source_channel = 0;
    }
}

void Preamp::send_audio_packet(int idx, uint8_t dest) {
    m_audio_socket->send_data(m_packets[idx], dest);
}
