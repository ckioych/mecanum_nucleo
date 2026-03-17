#ifndef ZEPHYR_PS2_H
#define ZEPHYR_PS2_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

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

// Задержки (подобраны под STM32 80MHz)
#define CTRL_CLK_DELAY     5  // микросекунд
#define CTRL_BYTE_DELAY     18 // микросекунд

class PS2X {
public:
    PS2X();
    
    // Инициализация с указанием устройств и пинов
    bool init(const struct device *gpio_dev, 
              gpio_pin_t clk_pin, gpio_pin_t cmd_pin, 
              gpio_pin_t att_pin, gpio_pin_t dat_pin,
              bool pressures = false, bool rumble = false);
    
    // Чтение состояния геймпада
    bool read_gamepad(bool motor1 = false, uint8_t motor2 = 0);
    
    // Проверка кнопок
    bool Button(uint16_t button);
    bool ButtonPressed(uint16_t button);
    bool ButtonReleased(uint16_t button);
    bool NewButtonState();
    bool NewButtonState(uint16_t button);
    
    // Аналоговые значения
    uint8_t Analog(uint8_t button);
    
    // Переконфигурация
    void reconfig_gamepad();
    
    // Тип контроллера
    uint8_t readType();

private:
    // Низкоуровневые операции с GPIO
    void clk_set() { gpio_pin_set(gpio_dev, clk_pin, 1); }
    void clk_clr() { gpio_pin_set(gpio_dev, clk_pin, 0); }
    void cmd_set() { gpio_pin_set(gpio_dev, cmd_pin, 1); }
    void cmd_clr() { gpio_pin_set(gpio_dev, cmd_pin, 0); }
    void att_set() { gpio_pin_set(gpio_dev, att_pin, 1); }
    void att_clr() { gpio_pin_set(gpio_dev, att_pin, 0); }
    bool dat_chk() { return gpio_pin_get(gpio_dev, dat_pin) > 0; }
    
    // Отправка/прием байта
    uint8_t gamepad_shiftinout(uint8_t data);
    
    // Отправка командной строки
    void sendCommandString(uint8_t *string, uint8_t len);
    
    // Устройства и пины
    const struct device *gpio_dev;
    gpio_pin_t clk_pin, cmd_pin, att_pin, dat_pin;
    
    // Данные контроллера
    uint8_t PS2data[21];
    uint16_t buttons;
    uint16_t last_buttons;
    uint64_t last_read;
    uint8_t read_delay;
    uint8_t controller_type;
    bool en_Rumble;
    bool en_Pressures;
};

#endif
