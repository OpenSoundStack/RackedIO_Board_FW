#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include "ihm/rgb.h"

int main() {
    init_leds();
    set_led_color(LedColor::YELLOW);

    while (true) {
        k_msleep(1000);
    }
}