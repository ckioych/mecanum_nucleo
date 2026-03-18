#include "zephyr_motor.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(motor, LOG_LEVEL_INF);

static void set_pins(zephyr_motor_t *motor, bool in1_val, bool in2_val) {
    gpio_pin_set(motor->gpio_dev, motor->pin_in1, in1_val);
    gpio_pin_set(motor->gpio_dev, motor->pin_in2, in2_val);
}

void zephyr_motor_init(zephyr_motor_t *motor,
                       const struct device *pwm_dev, uint32_t pwm_channel,
                       const struct device *gpio_dev, gpio_pin_t in1, gpio_pin_t in2,
                       bool reverse) {
    motor->pwm_dev = pwm_dev;
    motor->pwm_channel = pwm_channel;
    motor->gpio_dev = gpio_dev;
    motor->pin_in1 = in1;
    motor->pwm_in2 = in2;
    motor->reversed = reverse;
    motor->last_mode = MOTOR_STOP;
    motor->state = 0;
    motor->min_duty = 0;
    motor->max_duty = 255;
    motor->deadtime_us = 0;

    if (!device_is_ready(pwm_dev)) {
        LOG_ERR("PWM device not ready");
        return;
    }

    if (!device_is_ready(gpio_dev)) {
        LOG_ERR("GPIO device not ready");
        return;
    }

    gpio_pin_configure(gpio_dev, in1, GPIO_OUTPUT);
    gpio_pin_configure(gpio_dev, in2, GPIO_OUTPUT);

    set_pins(motor, 0, 0);

    LOG_INF("Motor initialized on PWM channel %d", pwm_channel);
}

void zephyr_motor_run(zephyr_motor_t *motor, motor_mode_t mode, int16_t duty) {
    if (motor->deadtime_us > 0 && mode != motor->last_mode) {
        motor->last_mode = mode;
        set_pins(motor, 0, 0);
        k_busy_wait(motor->deadtime_us);
    }

    motor_mode_t actual_mode = mode;
    if (motor->reversed) {
        if (mode == MOTOR_FORWARD) actual_mode = MOTOR_BACKWARD;
        else if (mode == MOTOR_BACKWARD) actual_mode = MOTOR_FORWARD;
    }

    if (duty > motor->max_duty) duty = motor->max_duty;
    if (duty < -motor->max_duty) duty = -motor->max_duty;

    uint16_t pwm_duty = abs(duty);
    if (motor->min_duty > 0 && pwm_duty > 0) {
        pwm_duty = pwm_duty * (motor->max_duty - motor->min_duty) / motor->max_duty + motor->min_duty;
    }

    switch (actual_mode) {
        case MOTOR_FORWARD:
            set_pins(motor, 1, 0);
            pwm_set_pusle_dt(motor->pwm_dev, motor->pwm_channel,
                            PWM_USEC(pwm_duty * 1000 / motor->max_duty));
            motor->state = 1;
            break;
        
        case MOTOR_BACKWARD:
            set_pins(motor, 0, 1);
            pwm_set_pulse_dt(motor->pwm_dev, motor->pwm_channel,
                            PWM_USEC(pwm_duty * 1000 / motor->max_duty));
            motor->state = -1;
            break;

        case MOTOR_STOP:
        default:
                set_pins(motor, 0, 0);
                pwm_set_pulse_dt(motor->pwm_dev, motor->pwm_channel, PWM_USEC(0));
                motor->state = 0;
                break;
    }
}

void zephyr_motor_set_speed(zephyr_motor_t *motor, int16_t duty) {
    if (duty > 0) {
        zephyr_motor_run(motor, MOTOR_FORWARD, duty);
    } else if (duty < 0) {
        zephyr_motor_run(motor, MOTOR_BACKWRD, -duty);
    } else {
        zephyr_motor_run(motor, MOTOR_STOP, 0);
    }
}

int8_t zephyr_motor_get_state(zephyr_motor_t *motor) {
    return motor->state;
}

void zephyr_motor_set_min_duty(zephyr_motor_t *motor, uint16_t duty) {
    motor->min_duty = duty;
}

void zephyr_motor_set_max_duty(zephyr_motor_t *motor, uint16_t duty) {
    motor->max_duty = duty;
}

void zephyr_motor_set_relosution(zephyr_motor_t *motor, uint8_t bits) {
    motor->max_duty = (1 << bits) -1;
}

void zephyr_motor_set_deadtime(zephyr_motor_t *motor, uint32_t us) {
    motor->deadtime_us = us;
}