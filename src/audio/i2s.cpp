#include "i2s.h"

static __nocache uint32_t dma_buff[64 * 2];

I2S_HandleTypeDef hi2s1;
DMA_HandleTypeDef hdma_i2s1;

// From ST driver
static unsigned int div_round_closest(uint32_t dividend, uint32_t divisor) {
    return (dividend + (divisor / 2U)) / divisor;
}

void dma_isr(void*) {
    HAL_DMA_IRQHandler(&hdma_i2s1);
}

void spi1_isr(void*) {
    HAL_I2S_IRQHandler(&hi2s1);
}

void clock_setup_i2s() {
    RCC_PeriphCLKInitTypeDef clk_init{0};
    clk_init.PeriphClockSelection = RCC_PERIPHCLK_SPI1;
    clk_init.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PIN;
    HAL_RCCEx_PeriphCLKConfig(&clk_init);

    __HAL_RCC_SPI1_CLK_ENABLE();
}

void gpio_setup() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gpio_init;
    gpio_init.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    gpio_init.Pin = GPIO_PIN_9;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOC, &gpio_init);
}

void dma_setup() {
    __HAL_RCC_DMA1_CLK_ENABLE();

    hdma_i2s1.Instance = DMA1_Stream0;
    hdma_i2s1.Init.Request = DMA_REQUEST_SPI1_RX;
    hdma_i2s1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_i2s1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_i2s1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_i2s1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_i2s1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_i2s1.Init.Mode = DMA_CIRCULAR;
    hdma_i2s1.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_i2s1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_i2s1);

    __HAL_LINKDMA(&hi2s1, hdmarx, hdma_i2s1);
}

void init_i2s_periph(SPI_TypeDef *hdl) {
    hi2s1.Instance = hdl;
    hi2s1.Init.Mode = I2S_MODE_MASTER_RX;
    hi2s1.Init.Standard = I2S_STANDARD_MSB;
    hi2s1.Init.DataFormat = I2S_DATAFORMAT_24B;
    hi2s1.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
    hi2s1.Init.AudioFreq = I2S_AUDIOFREQ_192K;
    hi2s1.Init.CPOL = I2S_CPOL_LOW;
    hi2s1.Init.FirstBit = I2S_FIRSTBIT_MSB;
    hi2s1.Init.WSInversion = I2S_WS_INVERSION_DISABLE;
    hi2s1.Init.Data24BitAlignment = I2S_DATA_24BIT_ALIGNMENT_RIGHT;
    hi2s1.Init.MasterKeepIOState = I2S_MASTER_KEEP_IO_STATE_DISABLE;

    HAL_I2S_Init(&hi2s1);
}

void ll_i2s_clock_setup(SPI_TypeDef *hdl) {
    const int channel_count = 2;
    int bit_clk_target_freq = I2SBoardConfig::sample_rate * 32 * channel_count;
    uint32_t freq_in = I2SBoardConfig::audio_clk_freq; // 49.152 MHz

    uint8_t i2s_div = div_round_closest(freq_in, bit_clk_target_freq);
    uint8_t i2s_odd = (i2s_div & 0x1) ? 1 : 0;
    i2s_div >>= 2; // Multiply by 4 because what the fuck

    LL_I2S_SetPrescalerLinear(hdl, i2s_div);
    LL_I2S_SetPrescalerParity(hdl, i2s_odd);
}

void ll_i2s_start(SPI_TypeDef *hdl) {
    volatile auto err = HAL_I2S_Receive_DMA(&hi2s1, (uint16_t*)dma_buff, 64*4);
}

void irq_setup() {
    IRQ_CONNECT(DMA1_Stream0_IRQn, 0, dma_isr, nullptr, 0);
    irq_enable(DMA1_Stream0_IRQn);

    IRQ_CONNECT(SPI1_IRQn, 0, spi1_isr, nullptr, 0);
    irq_enable(SPI1_IRQn);
}

void configure_board_i2s() {
    clock_setup_i2s();
    gpio_setup();
    dma_setup();
    irq_setup();

    init_i2s_periph(SPI1);
    ll_i2s_clock_setup(SPI1);
}
