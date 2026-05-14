#ifndef RACKEDIO_BOARD_FIRMWARE_RGB_H
#define RACKEDIO_BOARD_FIRMWARE_RGB_H

#include <cstdint>
#include <zephyr/drivers/gpio.h>

#define LEDR_NODE DT_ALIAS(ledr)
#define LEDG_NODE DT_ALIAS(ledg)
#define LEDB_NODE DT_ALIAS(ledb)

enum LedColor : uint8_t {
    OFF = 0x00,
    WHITE = 0x07,
    RED = 0x01,
    GREEN = 0x02,
    BLUE = 0x04,
    YELLOW = 0x03,
    CYAN = 0x06,
    PINK = 0x05
};

enum LedColorMask : uint8_t {
    MSK_RED = 0x01,
    MSK_GREEN = 0x02,
    MSK_BLUE = 0x04
};

void init_leds();
void set_led_color(LedColor color);

#endif //RACKEDIO_BOARD_FIRMWARE_RGB_H
