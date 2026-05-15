#include "RingBuffer.h"

#include <cstring>

RingBuffer::RingBuffer(uint16_t buffer_size, uint16_t ring_size, EthProtocol filt_proto) {
    alloc_buffer(buffer_size, ring_size);

    m_ring_size = ring_size;
    m_buffer_cursor = 0;
    m_write_cursor = 0;
    m_filt_proto = filt_proto;
}

void RingBuffer::push_on_buffer(uint8_t *data, size_t len) {
    memcpy(m_buffers[m_write_cursor].ptr, data, len);
    m_buffers[m_write_cursor].sz = len;
    m_write_cursor = (m_write_cursor + 1) % m_ring_size;
}

DataPtr RingBuffer::pop_from_buffer() {
    auto data =  m_buffers[m_buffer_cursor];
    m_buffer_cursor = (m_buffer_cursor + 1) % m_ring_size;

    return data;
}

void RingBuffer::alloc_buffer(uint16_t buffer_size, uint16_t ring_size) {
    m_buffers.reserve(ring_size);
    for (int i = 0; i < ring_size; i++) {
        DataPtr dptr{};
        dptr.ptr = new uint8_t[buffer_size];
        dptr.sz = 0;

        m_buffers.emplace_back(dptr);
    }
}

EthProtocol RingBuffer::get_filt_proto() {
    return m_filt_proto;
}

bool RingBuffer::can_pop() {
    return m_buffer_cursor != m_write_cursor;
}
