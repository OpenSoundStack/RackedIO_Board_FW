#ifndef RACKEDIO_BOARD_FIRMWARE_I2S_H
#define RACKEDIO_BOARD_FIRMWARE_I2S_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/gpio.h>

#include <stm32h723xx.h>
#include <stm32h7xx_ll_spi.h>
#include <stm32h7xx_ll_gpio.h>
#include <stm32h7xx_ll_bus.h>
#include <stm32h7xx_ll_dma.h>

#include <stm32_ll_spi.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_gpio.h>

#include "common/packet_structs.h"

#include "pre.h"

/**
 *
 * TAKING OVER ZEPHYR DRIVERS AND USING I2S&DMA BARE METAL
 * NEED TO IMPL PROPERLY IN ZEPHYR AND MAKE A FUTURE PR ON GITHUB
 *
 */
namespace I2SBoardConfig {
    constexpr int audio_clk_freq = 49152000; // 49.152 MHz
    constexpr int sample_rate = 96000;
    constexpr int samples_per_dma_buff = AUDIO_DATA_SAMPLES_PER_PACKETS * 2 * 2;
    constexpr int max_channels = 8;
}

enum I2SEvents : uint32_t {
    PRE12_EV_HALF = (1),
    PRE12_EV_FULL = (1 << 1)
};

void gpio_setup();
void clock_setup_i2s();
void dma_setup();
void irq_setup();
void ev_setup(const std::vector<Preamp>* preamps_control);
void configure_board_i2s(const std::vector<Preamp>* preamps_control);

void init_i2s_periph(I2S_HandleTypeDef* i2s_hdl, SPI_TypeDef* hdl);
void ll_i2s_clock_setup(SPI_TypeDef* hdl);
void ll_i2s_start(SPI_TypeDef* hdl);

void process_buffer(uint32_t* base, size_t len, int pre_idx, const std::vector<Preamp>& preamps_control);
k_pipe* get_stream(int idx);

#endif //RACKEDIO_BOARD_FIRMWARE_I2S_H
