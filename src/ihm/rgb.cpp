#include "rgb.h"

static const gpio_dt_spec ledr = GPIO_DT_SPEC_GET(LEDR_NODE, gpios);
static const gpio_dt_spec ledg = GPIO_DT_SPEC_GET(LEDG_NODE, gpios);
static const gpio_dt_spec ledb = GPIO_DT_SPEC_GET(LEDB_NODE, gpios);

void init_leds() {
    gpio_pin_configure_dt(&ledr, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ledg, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ledb, GPIO_OUTPUT_INACTIVE);
}

void set_led_color(LedColor color) {
    gpio_pin_set_dt(&ledr, (color & LedColorMask::MSK_RED));
    gpio_pin_set_dt(&ledg, (color & LedColorMask::MSK_GREEN));
    gpio_pin_set_dt(&ledb, (color & LedColorMask::MSK_BLUE));
}
