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

/**
 *
 * TAKING OVER ZEPHYR DRIVERS AND USING I2S&DMA BARE METAL
 * NEED TO IMPL PROPERLY IN ZEPHYR AND MAKE A FUTURE PR ON GITHUB
 *
 */
namespace I2SBoardConfig {
    constexpr int audio_clk_freq = 49152000; // 49.152 MHz
    constexpr int sample_rate = 96000;
    constexpr int block_size = AUDIO_DATA_SAMPLES_PER_PACKETS * sizeof(uint32_t);
}

void gpio_setup();
void clock_setup_i2s();
void dma_setup();
void irq_setup();
void configure_board_i2s();

void init_i2s_periph(SPI_TypeDef* hdl);
void ll_i2s_clock_setup(SPI_TypeDef* hdl);
void ll_i2s_start(SPI_TypeDef* hdl);

#endif //RACKEDIO_BOARD_FIRMWARE_I2S_H
