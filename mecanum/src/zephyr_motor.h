#ifndef ZEPHYR_MOTOR_H
#define ZEPHYR_MOTOR_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>

#ifndef __cplusplus
extern "C" {
#endif

// режимы работы моторов
typedef enum {
    MOTOR_STOP,
    MOTOR_FORWARD,
    MOTOR_BACKWARD,
    MOTOR_BRAKE
} motor_mode_t;

typedef struct {
    const struct device *pwm_dev;
    uint32_t pwm_channel;
    const struct device *gpio_dev;
    gpio_pin_t pin_in1;
    gpio_pin_t pin_in2;

    bool reversed;
    motor_mode_t last_mode;
    int8_t state;

    uint16_t min_duty;
    uint16_t max_duty;
    uint16_t deadtime_us;
} zephyr_motor_t;

void zephyr_motor_init(zephyr_motor_t *motor,
                       const struct device *pwm_dev, uint32_t pwm_channel,
                       const struct device *gpio_dev, gpio_pin_t in1, gpio_pin_t in2,
                       bool reverse);

void zephyr_motor_set_speed(zephyr_motor_t *motor, int16_t duty);
void zephyr_motor_run(zephyr_motor_t *motor, motor_mode_t mode, int16_t duty);
int8_t zephyr_motor_get_state(zephyr_motor_t *motor);

void zephyr_motor_set_min_duty(zephyr_motor_t *motor, uint16_t duty);
void zephyr_motor_set_max_duty(zephyr_motor_t *motor, uint16_t duty);
void zephyr_motor_set_resolution(zephyr_motor_t *motor, uint8_t bits);
void zephyr_motor_set_deadtime(zephyr_motor_t *motor, uint32_t us);


#ifdef __cplusplus
}
#endif

#endif