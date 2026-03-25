#include "zephyr_ps2.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ps2, LOG_LEVEL_INF);

// Команды конфигурации
static const uint8_t enter_config[] = {0x01, 0x43, 0x00, 0x01, 0x00};
static const uint8_t set_mode[] = {0x01, 0x44, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00};
static const uint8_t set_bytes_large[] = {0x01, 0x4F, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00};
static const uint8_t exit_config[] = {0x01, 0x43, 0x00, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A};
static const uint8_t enable_rumble[] = {0x01, 0x4D, 0x00, 0x00, 0x01};
static const uint8_t type_read[] = {0x01, 0x45, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A};

// Задержки
#define CTRL_CLK_DELAY     5
#define CTRL_BYTE_DELAY    18

// Низкоуровневые макросы
#define CLK_SET(ps2)    gpio_pin_set((ps2)->gpio_dev, (ps2)->clk_pin, 1)
#define CLK_CLR(ps2)    gpio_pin_set((ps2)->gpio_dev, (ps2)->clk_pin, 0)
#define CMD_SET(ps2)    gpio_pin_set((ps2)->gpio_dev, (ps2)->cmd_pin, 1)
#define CMD_CLR(ps2)    gpio_pin_set((ps2)->gpio_dev, (ps2)->cmd_pin, 0)
#define ATT_SET(ps2)    gpio_pin_set((ps2)->gpio_dev, (ps2)->att_pin, 1)
#define ATT_CLR(ps2)    gpio_pin_set((ps2)->gpio_dev, (ps2)->att_pin, 0)
#define DAT_CHK(ps2)    (gpio_pin_get((ps2)->gpio_dev, (ps2)->dat_pin) > 0)

static uint8_t gamepad_shiftinout(ps2_t *ps2, uint8_t data) {
    uint8_t ret = 0;

    for (int i = 0; i < 8; i++) {
        if (data & (1 << i)) {
            CMD_SET(ps2);
        } else {
            CMD_CLR(ps2);
        }

        CLK_CLR(ps2);
        k_busy_wait(CTRL_CLK_DELAY);

        if (DAT_CHK(ps2)) {
            ret |= (1 << i);
        }

        CLK_SET(ps2);
        k_busy_wait(CTRL_CLK_DELAY);
    }

    CMD_SET(ps2);
    k_busy_wait(CTRL_BYTE_DELAY);

    return ret;
}

static void send_command_string(ps2_t *ps2, const uint8_t *string, uint8_t len) {
    ATT_CLR(ps2);
    k_busy_wait(CTRL_BYTE_DELAY);

    for (int i = 0; i < len; i++) {
        gamepad_shiftinout(ps2, string[i]);
    }

    ATT_SET(ps2);
    k_sleep(K_MSEC(ps2->read_delay));
}

bool ps2_init(ps2_t *ps2,
              const struct device *gpio_dev,
              gpio_pin_t clk_pin, gpio_pin_t cmd_pin,
              gpio_pin_t att_pin, gpio_pin_t dat_pin,
              bool pressures, bool rumble) {

    ps2->gpio_dev = gpio_dev;
    ps2->clk_pin = clk_pin;
    ps2->cmd_pin = cmd_pin;
    ps2->att_pin = att_pin;
    ps2->dat_pin = dat_pin;
    ps2->buttons = 0;
    ps2->last_buttons = 0;
    ps2->last_read = 0;
    ps2->read_delay = 1;
    ps2->controller_type = 0;
    ps2->en_rumble = false;
    ps2->en_pressures = false;

    if (!device_is_ready(gpio_dev)) {
        LOG_ERR("GPIO device not ready");
        return false;
    }

    gpio_pin_configure(gpio_dev, clk_pin, GPIO_OUTPUT);
    gpio_pin_configure(gpio_dev, cmd_pin, GPIO_OUTPUT);
    gpio_pin_configure(gpio_dev, att_pin, GPIO_OUTPUT);
    gpio_pin_configure(gpio_dev, dat_pin, GPIO_INPUT | GPIO_PULL_UP);

    CMD_SET(ps2);
    CLK_SET(ps2);
    ATT_SET(ps2);

    // Первичное чтение
    ps2_read_gamepad(ps2, false, 0);
    ps2_read_gamepad(ps2, false, 0);

    if (ps2->data[1] != 0x41 && ps2->data[1] != 0x42 &&
        ps2->data[1] != 0x73 && ps2->data[1] != 0x79) {
        LOG_ERR("No PS2 controller found. Response: 0x%02X", ps2->data[1]);
        return false;
    }

    ps2->read_delay = 1;
    bool configured = false;

    for (int y = 0; y <= 10; y++) {
        send_command_string(ps2, enter_config, sizeof(enter_config));

        k_busy_wait(CTRL_BYTE_DELAY);
        CMD_SET(ps2);
        CLK_SET(ps2);
        ATT_CLR(ps2);
        k_busy_wait(CTRL_BYTE_DELAY);

        uint8_t temp[9];
        for (int i = 0; i < 9; i++) {
            temp[i] = gamepad_shiftinout(ps2, type_read[i]);
        }
        ATT_SET(ps2);

        ps2->controller_type = temp[3];

        send_command_string(ps2, set_mode, sizeof(set_mode));

        if (rumble) {
            send_command_string(ps2, enable_rumble, sizeof(enable_rumble));
            ps2->en_rumble = true;
        }

        if (pressures) {
            send_command_string(ps2, set_bytes_large, sizeof(set_bytes_large));
            ps2->en_pressures = true;
        }

        send_command_string(ps2, exit_config, sizeof(exit_config));

        ps2_read_gamepad(ps2, false, 0);

        if (pressures) {
            if (ps2->data[1] == 0x79) {
                configured = true;
                break;
            }
            if (ps2->data[1] == 0x73) {
                LOG_ERR("Controller doesn't support pressures");
                return false;
            }
        }

        if (ps2->data[1] == 0x73) {
            configured = true;
            break;
        }

        ps2->read_delay++;
        k_sleep(K_MSEC(1));
    }

    if (!configured) {
        LOG_ERR("Failed to configure PS2 controller");
        return false;
    }

    LOG_INF("PS2 controller initialized. Type: 0x%02X", ps2->controller_type);
    return true;
}

bool ps2_read_gamepad(ps2_t *ps2, bool motor1, uint8_t motor2) {
    uint64_t now = k_uptime_get();

    if (now - ps2->last_read > 1500) {
        ps2_reconfig(ps2);
    }

    if (now - ps2->last_read < ps2->read_delay) {
        k_sleep(K_MSEC(ps2->read_delay - (now - ps2->last_read)));
    }

    if (motor2 != 0) {
        motor2 = (motor2 * 0xBF / 255) + 0x40;
    }

    uint8_t dword[9] = {0x01, 0x42, 0, motor1 ? 0x01 : 0, motor2, 0, 0, 0, 0};
    uint8_t dword2[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    bool success = false;
    for (int retry = 0; retry < 5; retry++) {
        CMD_SET(ps2);
        CLK_SET(ps2);
        ATT_CLR(ps2);
        k_busy_wait(CTRL_BYTE_DELAY);

        for (int i = 0; i < 9; i++) {
            ps2->data[i] = gamepad_shiftinout(ps2, dword[i]);
        }

        if (ps2->data[1] == 0x79) {
            for (int i = 0; i < 12; i++) {
                ps2->data[i + 9] = gamepad_shiftinout(ps2, dword2[i]);
            }
        }

        ATT_SET(ps2);

        if ((ps2->data[1] & 0xF0) == 0x70) {
            success = true;
            break;
        }

        ps2_reconfig(ps2);
        k_sleep(K_MSEC(ps2->read_delay));
    }

    ps2->last_buttons = ps2->buttons;
    ps2->buttons = (uint16_t)(ps2->data[4] << 8) | ps2->data[3];
    ps2->last_read = k_uptime_get();

    return success;
}

bool ps2_button(ps2_t *ps2, uint16_t button) {
    return (~ps2->buttons & button) != 0;
}

bool ps2_button_pressed(ps2_t *ps2, uint16_t button) {
    return ps2_new_button_state_mask(ps2, button) && ps2_button(ps2, button);
}

bool ps2_button_released(ps2_t *ps2, uint16_t button) {
    return ps2_new_button_state_mask(ps2, button) &&
           (~ps2->last_buttons & button) != 0;
}

bool ps2_new_button_state(ps2_t *ps2) {
    return (ps2->last_buttons ^ ps2->buttons) != 0;
}

bool ps2_new_button_state_mask(ps2_t *ps2, uint16_t button) {
    return ((ps2->last_buttons ^ ps2->buttons) & button) != 0;
}

uint8_t ps2_analog(ps2_t *ps2, uint8_t channel) {
    if (channel < 21) {
        return ps2->data[channel];
    }
    return 0;
}

void ps2_reconfig(ps2_t *ps2) {
    send_command_string(ps2, enter_config, sizeof(enter_config));
    send_command_string(ps2, set_mode, sizeof(set_mode));

    if (ps2->en_rumble) {
        send_command_string(ps2, enable_rumble, sizeof(enable_rumble));
    }

    if (ps2->en_pressures) {
        send_command_string(ps2, set_bytes_large, sizeof(set_bytes_large));
    }

    send_command_string(ps2, exit_config, sizeof(exit_config));
}

uint8_t ps2_read_type(ps2_t *ps2) {
    return ps2->controller_type;
}