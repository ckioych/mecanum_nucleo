#ifndef ZEPHYR_ACCELMOTOR_H
#define ZEPHYR_ACCELMOTOR_H

#include "zephyr_motor.h"
#include "zephyr_encoder.h"

typedef enum {
    ACCEL_POS,      // позиционирование с ускорением
    PID_POS,        // ПИД-позиционирование
    ACCEL_SPEED,    // скорость с ускорением
    PID_SPEED,      // ПИД-скорость
    IDLE_RUN
} accel_run_mode_t;

typedef struct {
    zephyr_motor_t *motor;
    zephyur_encoder_t *encoder;

    float ratio;           // передаточное число
    uint32_t dt_ms;        // период (мс)
    float dt_sec;          // пероид (сек)
    uint32_t stopzone;     // зона остановки
    uint16_ max_speed;     // макс. скорость
    float accelerarion;    // ускорение

    accel_run_mode_t run_mode;
    int32_t current_pos;
    int32_t last_pos;
    int32_t target_pos;
    int16_t current_speed;
    int16_t target_speed;
    float duty_f;
    uint32_t last_tick_time;

    int16_t speed_buf[3];
    uint8_t speed_buf_idx;
    float filtered_speed;

    float kp, ki, kd;
    float integral;
    int32_t prev_input;
    float last_speed;

    float control_pos;
    float control_speed;
} zephyr_accelmotor_c;

void zephyr_accelmotor_init(zephyr_accelmotor_t *accel,
                            zephyr_motor_t *motor,
                            zephyr_encoder_t *encoder);

bool zephyr_accelmotor_tick(zephyr_accelmotor_t *accel, int32_t pos);

void zephyr_accelmotor_set_dt(zephyr_accelmotor_t *accel, uint32_t dt_ms);
void zephyr_accelmotor_set_ratio(zephyr_accelmotor_t *accel, float ratio);
void zephyr_accelmotor_set_stopzone(zephyr_accelmotor_t *accel, uint32_t zone);
void zephyr_accelmotor_set_run_mode(zephyr_accelmotor_t *accel, accel_run_mode_t mode);
accel_run_mode_t zephyr_accelmotor_get_run_mode(zephyr_accelmotor_t *accel);

void zephyr_accelmotor_set_target_pos(zephyr_accelmotor_t *accel int32_pos);
void zephyr_accelmotor_set_target_pos_deg(zephyr_accelmotor_t *accel, int32_t deg);
int32_t zephyr_accelmotor_get_target_pos(zephyr_accelmotor_t *accel);
int32_t zephyr_accelmotor_get_target_pos_deg(zephyr_accelmotor_t *accel);

void zephyr_accelmotor_set_target_speed(zephyr_accelmotor_t *accel, int16_t speed);
void zephyr_accelmotor_set_target_speed_deg(zephyr_accelmotor_t *accel, int16_t speed);
int16_t zephyr_accelmotor_get_target_speed(zephyr_accelmotor_t *accel);
int16_t zephyr_accelmotor_get_target_speed_deg(zephyr_accelmotor_t *accel);

int32_t zephyr_accelmotor_get_current_pos(zephyr_accelmotor_t *accel);
int32_t zephyr_accelmotor_get_current_pos_deg(zephyr_accelmotor_t *accel);
int16_t zephyr_accelmotor_get_current_speed(zephyr_accelmotor_t *accel);
int16_t zephyr_accelmotor_get_current_speed_deg(zephyr_accelmotor_t *accel);
float zephyr_accelmotor_get_duty(zephyr_accelmotor_t *accel);

void zephyr_accelmotor_set_max_speed(zephyr_accelmotor_t *accel, uint16_t speed);
void zephyr_accelmotor_set_max_speed_deg(zephyr_accelmotor_t *accel, uint16_t speed);
void zephyr_accelmotor_set_acceleration(zephyr_accelmotor_t *accel, float accel_val);
void zephyr_accelmotor_set_acceleration_deg(zephyr_accelmotor_t *accel, float accel_val);

bool zephyr_accelmotor_is_blocked(zephyr_accelmotor_t *accel);

#endif // ZEPHYR_ACCELMOTOR_H