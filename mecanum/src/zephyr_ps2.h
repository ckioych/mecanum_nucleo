#ifndef ZEPHYR_PS2_H
#define ZEPHYR_PS2_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Константы кнопок
#define PSB_SELECT      0x0001
#define PSB_L3          0x0002
#define PSB_R3          0x0004
#define PSB_START       0x0008
#define PSB_PAD_UP      0x0010
#define PSB_PAD_RIGHT   0x0020
#define PSB_PAD_DOWN    0x0040
#define PSB_PAD_LEFT    0x0080
#define PSB_L2          0x0100
#define PSB_R2          0x0200
#define PSB_L1          0x0400
#define PSB_R1          0x0800
#define PSB_GREEN       0x1000
#define PSB_RED         0x2000
#define PSB_BLUE        0x4000
#define PSB_PINK        0x8000

// Константы стиков
#define PSS_RX 5
#define PSS_RY 6
#define PSS_LX 7
#define PSS_LY 8

// Структура PS2 контроллера
typedef struct {
    const struct device *gpio_dev;
    gpio_pin_t clk_pin;
    gpio_pin_t cmd_pin;
    gpio_pin_t att_pin;
    gpio_pin_t dat_pin;

    uint8_t data[21];
    uint16_t buttons;
    uint16_t last_buttons;
    uint64_t last_read;
    uint8_t read_delay;
    uint8_t controller_type;
    bool en_rumble;
    bool en_pressures;
} ps2_t;

// Инициализация
bool ps2_init(ps2_t *ps2,
              const struct device *gpio_dev,
              gpio_pin_t clk_pin, gpio_pin_t cmd_pin,
              gpio_pin_t att_pin, gpio_pin_t dat_pin,
              bool pressures, bool rumble);

// Чтение состояния
bool ps2_read_gamepad(ps2_t *ps2, bool motor1, uint8_t motor2);

// Проверка кнопок
bool ps2_button(ps2_t *ps2, uint16_t button);
bool ps2_button_pressed(ps2_t *ps2, uint16_t button);
bool ps2_button_released(ps2_t *ps2, uint16_t button);
bool ps2_new_button_state(ps2_t *ps2);
bool ps2_new_button_state_mask(ps2_t *ps2, uint16_t button);

// Аналоговые значения
uint8_t ps2_analog(ps2_t *ps2, uint8_t channel);

// Переконфигурация
void ps2_reconfig(ps2_t *ps2);

// Тип контроллера
uint8_t ps2_read_type(ps2_t *ps2);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_PS2_H */