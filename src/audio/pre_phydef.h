#ifndef RACKEDIO_BOARD_FIRMWARE_PRE_PHYDEF_H
#define RACKEDIO_BOARD_FIRMWARE_PRE_PHYDEF_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

struct PḧyPre {
    struct gpio_dt_spec b0;
    struct gpio_dt_spec b1;
    struct gpio_dt_spec en_48v;
} typedef phy_pre_t;

#define DT_GET_PREAMP(name) phy_pre_t{ \
    GPIO_DT_SPEC_GET(DT_ALIAS(name), b0), \
    GPIO_DT_SPEC_GET(DT_ALIAS(name), b1), \
    GPIO_DT_SPEC_GET(DT_ALIAS(name), phantom) \
}

#endif //RACKEDIO_BOARD_FIRMWARE_PRE_PHYDEF_H
