#include "i2s.h"

static __nocache uint32_t dma_buff[4][I2SBoardConfig::samples_per_dma_buff];
static k_event packet_process_event;

I2S_HandleTypeDef hi2s1;
I2S_HandleTypeDef hi2s2;
I2S_HandleTypeDef hi2s3;
I2S_HandleTypeDef hi2s6;

DMA_HandleTypeDef hdma_i2s1;
DMA_HandleTypeDef hdma_i2s2;
DMA_HandleTypeDef hdma_i2s3;
DMA_HandleTypeDef hdma_i2s4;

K_THREAD_STACK_DEFINE(audio_proc_stack, 2048);
k_tid_t audio_proc_th_id;
k_thread audio_proc_thread;

k_pipe channel_audio_streams[I2SBoardConfig::max_channels];
__nocache float channel_buffers[8][AUDIO_DATA_SAMPLES_PER_PACKETS * 8]; // Buffer up to 4 packets

// From ST driver
static unsigned int div_round_closest(uint32_t dividend, uint32_t divisor) {
    return (dividend + (divisor / 2U)) / divisor;
}

static inline int32_t sign_extend_24_32(uint32_t x) {
    const int bits = 24;
    uint32_t m = 1u << (bits - 1);
    return (x ^ m) - m;
}

void dma_isr(void*) {
    HAL_DMA_IRQHandler(&hdma_i2s1);
}

void spi1_isr(void*) {
    HAL_I2S_IRQHandler(&hi2s1);
}

extern "C" void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef* hdl) {
    if (hdl == &hi2s1) {
        k_event_post(&packet_process_event, I2SEvents::PRE12_EV_FULL);
    }
}

extern "C" void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef* hdl) {
    if (hdl == &hi2s1) {
        k_event_post(&packet_process_event, I2SEvents::PRE12_EV_HALF);
    }
}

void clock_setup_i2s() {
    RCC_PeriphCLKInitTypeDef clk_init{0};
    clk_init.PeriphClockSelection = RCC_PERIPHCLK_SPI1;
    clk_init.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PIN;
    HAL_RCCEx_PeriphCLKConfig(&clk_init);

    clk_init.PeriphClockSelection = RCC_PERIPHCLK_SPI2;
    HAL_RCCEx_PeriphCLKConfig(&clk_init);

    clk_init.PeriphClockSelection = RCC_PERIPHCLK_SPI3;
    HAL_RCCEx_PeriphCLKConfig(&clk_init);

    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_SPI3_CLK_ENABLE();
}

void gpio_setup() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*
     * Clock => PC9
     * I2S 1 => PA4, PA5, PA6
     * I2S 2 => PA11, PB10, PC2
     * I2S 3 => PC10, PC11, PA15
     */

    GPIO_InitTypeDef gpio_init;
    gpio_init.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_11;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    gpio_init.Pin = GPIO_PIN_15;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    gpio_init.Pin = GPIO_PIN_9|GPIO_PIN_2|GPIO_PIN_11|GPIO_PIN_10;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    gpio_init.Pin = GPIO_PIN_11|GPIO_PIN_10;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    gpio_init.Pin = GPIO_PIN_10;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOB, &gpio_init);
}

void init_link_dma(DMA_Stream_TypeDef* dma, int rq, DMA_HandleTypeDef& hdl, I2S_HandleTypeDef* i2s) {
    hdl.Instance = dma;
    hdl.Init.Request = rq;
    hdl.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdl.Init.PeriphInc = DMA_PINC_DISABLE;
    hdl.Init.MemInc = DMA_MINC_ENABLE;
    hdl.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdl.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdl.Init.Mode = DMA_CIRCULAR;
    hdl.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdl.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdl);

    __HAL_LINKDMA(i2s, hdmarx, hdl);
}

void dma_setup() {
    __HAL_RCC_DMA1_CLK_ENABLE();

    init_link_dma(DMA1_Stream0, DMA_REQUEST_SPI1_RX, hdma_i2s1, &hi2s1);
    init_link_dma(DMA1_Stream1, DMA_REQUEST_SPI2_RX, hdma_i2s2, &hi2s2);
    init_link_dma(DMA1_Stream2, DMA_REQUEST_SPI3_RX, hdma_i2s3, &hi2s3);

}

void init_i2s_periph(I2S_HandleTypeDef* i2s_hdl, SPI_TypeDef *hdl) {
    i2s_hdl->Instance = hdl;
    i2s_hdl->Init.Mode = I2S_MODE_MASTER_RX;
    i2s_hdl->Init.Standard = I2S_STANDARD_MSB;
    i2s_hdl->Init.DataFormat = I2S_DATAFORMAT_24B;
    i2s_hdl->Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
    i2s_hdl->Init.AudioFreq = I2S_AUDIOFREQ_96K;
    i2s_hdl->Init.CPOL = I2S_CPOL_LOW;
    i2s_hdl->Init.FirstBit = I2S_FIRSTBIT_MSB;
    i2s_hdl->Init.WSInversion = I2S_WS_INVERSION_DISABLE;
    i2s_hdl->Init.Data24BitAlignment = I2S_DATA_24BIT_ALIGNMENT_RIGHT;
    i2s_hdl->Init.MasterKeepIOState = I2S_MASTER_KEEP_IO_STATE_DISABLE;

    HAL_I2S_Init(i2s_hdl);
}

void ll_i2s_clock_setup(SPI_TypeDef *hdl) {
    const int channel_count = 2;
    int bit_clk_target_freq = I2SBoardConfig::sample_rate * 32 * channel_count;
    uint32_t freq_in = I2SBoardConfig::audio_clk_freq; // 49.152 MHz

    uint8_t i2s_div = div_round_closest(freq_in, bit_clk_target_freq);
    uint8_t i2s_odd = (i2s_div & 0x1) ? 1 : 0;
    i2s_div >>= 1;

    LL_I2S_SetPrescalerLinear(hdl, i2s_div);
    LL_I2S_SetPrescalerParity(hdl, i2s_odd);
}

void ll_i2s_start(I2S_HandleTypeDef *hdl, int index) {
    HAL_I2S_Receive_DMA(hdl, (uint16_t*)dma_buff[index], 64*4);
}

void start_i2s_all() {
    ll_i2s_start(&hi2s1, 0);
    ll_i2s_start(&hi2s2, 1);
    ll_i2s_start(&hi2s3, 2);
}

void irq_setup() {
    IRQ_CONNECT(DMA1_Stream0_IRQn, 0, dma_isr, nullptr, 0);
    irq_enable(DMA1_Stream0_IRQn);

    IRQ_CONNECT(SPI1_IRQn, 0, spi1_isr, nullptr, 0);
    irq_enable(SPI1_IRQn);
}

static void audio_processor_entry(void* ev, void* pre, void*) {
    constexpr size_t buffer_size = I2SBoardConfig::samples_per_dma_buff / 2;

    uint32_t evcode = 0;
    k_event* kev = reinterpret_cast<k_event *>(ev);
    const std::vector<Preamp>& preamps = *(reinterpret_cast<const std::vector<Preamp>*>(pre));

    while (true) {
        evcode = k_event_wait_safe(kev, I2SEvents::PRE12_EV_HALF | I2SEvents::PRE12_EV_FULL, false, K_FOREVER);
        if (evcode == I2SEvents::PRE12_EV_HALF) {
            process_buffer(dma_buff[0], buffer_size, 0, preamps);
        } else if (evcode == I2SEvents::PRE12_EV_FULL) {
            process_buffer(dma_buff[0] + buffer_size, buffer_size, 0, preamps);
        }
    }
}

void process_buffer(uint32_t *base, size_t len, int pre_idx, const std::vector<Preamp>& preamps_control) {
    constexpr float rerange_coef = 1.0f / (1 << 23);

    float digital_gain_1 = preamps_control[pre_idx].get_digital_gain();
    float digital_gain_2 = preamps_control[pre_idx + 1].get_digital_gain();

    for (int i = 0; i < len; i += 2) {
        float sample_a = static_cast<float>(sign_extend_24_32(base[i])) * rerange_coef * digital_gain_1;
        float sample_b = static_cast<float>(sign_extend_24_32(base[i + 1])) * rerange_coef * digital_gain_2;

        k_pipe_write(&channel_audio_streams[pre_idx], (uint8_t*)&sample_a, sizeof(float), K_NO_WAIT);
        k_pipe_write(&channel_audio_streams[pre_idx + 1], (uint8_t*)&sample_b, sizeof(float), K_NO_WAIT);
    }
}

void ev_setup(const std::vector<Preamp>* preamps_control) {
    int i = 0;
    for (auto& p : channel_audio_streams) {
        k_pipe_init(&p, (uint8_t*)channel_buffers[i], AUDIO_DATA_SAMPLES_PER_PACKETS * 4);
        i++;
    }

    k_event_init(&packet_process_event);

    audio_proc_th_id = k_thread_create(
        &audio_proc_thread,
        audio_proc_stack,
        K_THREAD_STACK_SIZEOF(audio_proc_stack),
        &audio_processor_entry,
        &packet_process_event, (void*)preamps_control, nullptr,
        -5, 0, K_NO_WAIT
    );
}

void configure_board_i2s(const std::vector<Preamp>* preamps_control) {
    clock_setup_i2s();
    gpio_setup();
    dma_setup();
    irq_setup();
    ev_setup(preamps_control);

    init_i2s_periph(&hi2s1, SPI1);
    init_i2s_periph(&hi2s2, SPI2);
    init_i2s_periph(&hi2s3, SPI3);

    ll_i2s_clock_setup(SPI1);
    ll_i2s_clock_setup(SPI2);
    ll_i2s_clock_setup(SPI3);
}

k_pipe *get_stream(const int idx) {
    return &channel_audio_streams[idx];
}
