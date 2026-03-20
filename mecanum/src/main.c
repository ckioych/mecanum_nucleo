#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "zephyr_motor.h"
#include "zephyr_encoder.h"
#include "zephyr_accelmotor.h"
#include "zephyr_ps2.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define PID_P 2.2f
#define PID_I 0.4f
#define PID_P 0.01f

#define PS2_DAT_PIN 11 // pc11
#define PS2_CMD_PIN 12 //pc12
#define PS2_SEL_PIN 10 //pc10
#define PS2_CLK_PIN 8 //pc8

// моторы
static zephyr_motor_t motor_fl, motor_fr, motor_bl, motor_br;

// энкодеры
static zephyr_encoder_t enc_fl, enc_fr, enc_bl, enc_br;

// моторы accel
static zephyr_accelmotor_t accel_fl, accel_fr, accel_bl, accel_br;

// ps2
static ps2_t ps2;

static bool position_mode = false;
static int32_t pos_fl = 0, pos_fr = 0, pos_bl = 0, pos_br = 0;

extern const struct device *pwm_dev1;
extern const struct device *pwm_dev2;
extern const struct device *pwm_dev17;
extern const struct device *gpio_dev;

static void update_target_positions(void) {
    zephyr_accelmotor_set_target_pos(&accel_fl, pos_fl);
    zephyr_accelmotor_set_target_pos(&accel_fr, pos_fr);
    zephyr_accelmotor_set_target_pos(&accel_bl, pos_bl);
    zephyr_accelmotor_set_target_pos(&accel_br, pos_br);
}

static void change_mode(bool mode) {
    position_mode = mode;
}