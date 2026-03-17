#include "zephyr_ps2.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(ps2, LOG_LEVEL_INF);

// Команды конфигурации (из оригинальной библиотеки)
static const uint8_t enter_config[] = {0x01, 0x43, 0x00, 0x01, 0x00};
static const uint8_t set_mode[] = {0x01, 0x44, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00};
static const uint8_t set_bytes_large[] = {0x01, 0x4F, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00};
static const uint8_t exit_config[] = {0x01, 0x43, 0x00, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A};
static const uint8_t enable_rumble[] = {0x01, 0x4D, 0x00, 0x00, 0x01};
static const uint8_t type_read[] = {0x01, 0x45, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A};

PS2X::PS2X() : gpio_dev(nullptr), buttons(0), last_buttons(0), 
               last_read(0), read_delay(1), controller_type(0),
               en_Rumble(false), en_Pressures(false) {}

bool PS2X::init(const struct device *dev, 
                gpio_pin_t clk, gpio_pin_t cmd, 
                gpio_pin_t att, gpio_pin_t dat,
                bool pressures, bool rumble) {
    
    gpio_dev = dev;
    clk_pin = clk;
    cmd_pin = cmd;
    att_pin = att;
    dat_pin = dat;
    
    if (!device_is_ready(gpio_dev)) {
        LOG_ERR("GPIO device not ready");
        return false;
    }
    
    // Настройка пинов
    gpio_pin_configure(gpio_dev, clk_pin, GPIO_OUTPUT);
    gpio_pin_configure(gpio_dev, cmd_pin, GPIO_OUTPUT);
    gpio_pin_configure(gpio_dev, att_pin, GPIO_OUTPUT);
    gpio_pin_configure(gpio_dev, dat_pin, GPIO_INPUT | GPIO_PULL_UP);
    
    // Установка начальных состояний
    cmd_set();
    clk_set();
    att_set();
    
    // Первичное чтение для проверки связи
    read_gamepad();
    read_gamepad();
    
    // Проверка, отвечает ли контроллер
    if (PS2data[1] != 0x41 && PS2data[1] != 0x42 && 
        PS2data[1] != 0x73 && PS2data[1] != 0x79) {
        LOG_ERR("No PS2 controller found. Response: 0x%02X", PS2data[1]);
        return false;
    }
    
    // Конфигурация с увеличивающейся задержкой
    read_delay = 1;
    bool configured = false;
    
    for (int y = 0; y <= 10; y++) {
        sendCommandString((uint8_t*)enter_config, sizeof(enter_config));
        
        // Чтение типа
        k_busy_wait(CTRL_BYTE_DELAY);
        cmd_set();
        clk_set();
        att_clr();
        k_busy_wait(CTRL_BYTE_DELAY);
        
        uint8_t temp[9];
        for (int i = 0; i < 9; i++) {
            temp[i] = gamepad_shiftinout(type_read[i]);
        }
        att_set();
        
        controller_type = temp[3];
        
        sendCommandString((uint8_t*)set_mode, sizeof(set_mode));
        
        if (rumble) {
            sendCommandString((uint8_t*)enable_rumble, sizeof(enable_rumble));
            en_Rumble = true;
        }
        
        if (pressures) {
            sendCommandString((uint8_t*)set_bytes_large, sizeof(set_bytes_large));
            en_Pressures = true;
        }
        
        sendCommandString((uint8_t*)exit_config, sizeof(exit_config));
        
        read_gamepad();
        
        if (pressures) {
            if (PS2data[1] == 0x79) {
                configured = true;
                break;
            }
            if (PS2data[1] == 0x73) {
                LOG_ERR("Controller doesn't support pressures");
                return false;
            }
        }
        
        if (PS2data[1] == 0x73) {
            configured = true;
            break;
        }
        
        read_delay++;
        k_sleep(K_MSEC(1));
    }
    
    if (!configured) {
        LOG_ERR("Failed to configure PS2 controller");
        return false;
    }
    
    LOG_INF("PS2 controller initialized. Type: 0x%02X", controller_type);
    return true;
}

uint8_t PS2X::gamepad_shiftinout(uint8_t data) {
    uint8_t ret = 0;
    
    for (int i = 0; i < 8; i++) {
        // Установка бита команды
        if (data & (1 << i)) {
            cmd_set();
        } else {
            cmd_clr();
        }
        
        // Тактовый импульс
        clk_clr();
        k_busy_wait(CTRL_CLK_DELAY);
        
        // Чтение бита данных
        if (dat_chk()) {
            ret |= (1 << i);
        }
        
        clk_set();
        k_busy_wait(CTRL_CLK_DELAY);
    }
    
    cmd_set();
    k_busy_wait(CTRL_BYTE_DELAY);
    
    return ret;
}

void PS2X::sendCommandString(uint8_t *string, uint8_t len) {
    att_clr();
    k_busy_wait(CTRL_BYTE_DELAY);
    
    for (int i = 0; i < len; i++) {
        gamepad_shiftinout(string[i]);
    }
    
    att_set();
    k_sleep(K_MSEC(read_delay));
}

bool PS2X::read_gamepad(bool motor1, uint8_t motor2) {
    uint64_t now = k_uptime_get();
    
    if (now - last_read > 1500) {
        reconfig_gamepad();
    }
    
    if (now - last_read < read_delay) {
        k_sleep(K_MSEC(read_delay - (now - last_read)));
    }
    
    if (motor2 != 0) {
        // Преобразование значения для вибромотора
        motor2 = (motor2 * 0xBF / 255) + 0x40;
    }
    
    uint8_t dword[9] = {0x01, 0x42, 0, motor1 ? 0x01 : 0, motor2, 0, 0, 0, 0};
    uint8_t dword2[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    
    // Несколько попыток чтения
    bool success = false;
    for (int retry = 0; retry < 5; retry++) {
        cmd_set();
        clk_set();
        att_clr();
        k_busy_wait(CTRL_BYTE_DELAY);
        
        for (int i = 0; i < 9; i++) {
            PS2data[i] = gamepad_shiftinout(dword[i]);
        }
        
        if (PS2data[1] == 0x79) {
            for (int i = 0; i < 12; i++) {
                PS2data[i + 9] = gamepad_shiftinout(dword2[i]);
            }
        }
        
        att_set();
        
        if ((PS2data[1] & 0xF0) == 0x70) {
            success = true;
            break;
        }
        
        reconfig_gamepad();
        k_sleep(K_MSEC(read_delay));
    }
    
    last_buttons = buttons;
    buttons = (uint16_t)(PS2data[4] << 8) | PS2data[3];
    last_read = k_uptime_get();
    
    return success;
}

bool PS2X::Button(uint16_t button) {
    return (~buttons & button) != 0;
}

bool PS2X::ButtonPressed(uint16_t button) {
    return NewButtonState(button) && Button(button);
}

bool PS2X::ButtonReleased(uint16_t button) {
    return NewButtonState(button) && (~last_buttons & button) != 0;
}

bool PS2X::NewButtonState() {
    return (last_buttons ^ buttons) != 0;
}

bool PS2X::NewButtonState(uint16_t button) {
    return ((last_buttons ^ buttons) & button) != 0;
}

uint8_t PS2X::Analog(uint8_t button) {
    return PS2data[button];
}

void PS2X::reconfig_gamepad() {
    sendCommandString((uint8_t*)enter_config, sizeof(enter_config));
    sendCommandString((uint8_t*)set_mode, sizeof(set_mode));
    
    if (en_Rumble) {
        sendCommandString((uint8_t*)enable_rumble, sizeof(enable_rumble));
    }
    
    if (en_Pressures) {
        sendCommandString((uint8_t*)set_bytes_large, sizeof(set_bytes_large));
    }
    
    sendCommandString((uint8_t*)exit_config, sizeof(exit_config));
}

uint8_t PS2X::readType() {
    return controller_type;
}