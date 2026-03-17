#ifndef ZEPHYR_MOTOR_H
#define ZEPHYR_MOTOR_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>

#ifndef __cplusplus
extern "C" {
#endif

typedef enum {
    DRIVER2WIRE,    
    DRIVER2WIRE_NO_INVERT,
    DRIVER2WIRE_PWM,
    DRIVER3WIRE,
    RELAY2WIRE,
} GM_driverType;

typedef enum {
    FORWARD,
    BACKWARD,
    BRAKE,
    STOP
} GM_workMode;

#define GM_NC 255

struct GMotor {

    GM_driverType type;

    struct pwm_dt_spec pwm_a;
    struct pwm_dt_spec pwm_b;
    struct gpio_dt_spec dig_a;
    struct gpio_dt_spec dig_b; 

    int8_t state;
    int16_t duty;
    int16_t target_duty;
    
    int16_t min_duty;
    int16_t max_duty;
    bool direction_reverse;
    uint16_t deadtime_us;
    
    bool smooth_enabled;
    int16_t smooth_step;
    struct k_timer smooth_timer;
    uint32_t last_smooth_tick;
    
    GM_workMode current_mode;
    GM_workMode last_mode;
    
    uint8_t resolution;
};

void GMotor_init(struct GMotor *motor, GM_driverType type,
                 const char *pwm_dev_name, uint32_t pwm_pin_a, uint32_t pwm_pin_b,
                 bool active_high);

void GMotor_setSpeed(struct GMotor *motor, int16_t dutu);

void GMotor_run(struct GMotor *motor, GM_workMode mode, int16_t duty);

void GMotor_stop(struct GMotor *motor);

void GMotor_brake(struct GMotor *motor);

void GMotor_setMinDuty(struct GMotor *motor, int16_t duty);

void GMotor_setResolution(struct GMotor *motor, uint8_t bits);

void GMotor_setDirection(struct GMotor *motor, bool reverse);

void GMotor_setDeadtime(struct GMotor *motor, uint16_t us);

void GMotor_enableSmooth(struct GMotor *motor, uint8_t step);

void GMotor_disableSmooth(struct GMotor *motor);

int8_t GMotor_getState(struct GMotor *motor);

int16_t GMotor_getDuty(struct GMotor *motor);

void GMotor_tick(struct GMotor *motor);

#ifdef __cplusplus
}
#endif

#endif