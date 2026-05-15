#ifndef IO_BOARD_FW_RINGBUFFER_H
#define IO_BOARD_FW_RINGBUFFER_H

#include <cstdint>
#include <cstdlib>
#include <vector>

#include <netutils/LowLatSocket.h>

struct DataPtr {
    uint8_t* ptr;
    size_t sz;
};

class RingBuffer {
public:
    RingBuffer(uint16_t buffer_size, uint16_t ring_size, EthProtocol filtered_proto);

    void push_on_buffer(uint8_t* data, size_t len);
    DataPtr pop_from_buffer();

    EthProtocol get_filt_proto();

    bool can_pop();
private:
    void alloc_buffer(uint16_t buffer_size, uint16_t ring_size);

    std::vector<DataPtr> m_buffers;
    int m_buffer_cursor;
    int m_write_cursor;
    int m_ring_size;
    EthProtocol m_filt_proto;
};

#endif //IO_BOARD_FW_RINGBUFFER_H