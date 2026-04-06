#include "zephyr_accelmotor.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(accelmotor, LOG_LEVEL_INF);

static int32_t iabs32(int32_t x) {
    return (x < 0) ? -x : x;
}

static float fabs32(float x) {
    return (x < 0.0f) ? -x : x;
}

static int8_t sign(float x) {
    return (x > 0) ? 1 : ((x < 0) ? -1 : 0);
}

void zephyr_accelmotor_init(zephyr_accelmotor_t *accel,
                            zephyr_motor_t *motor,
                            zephyr_encoder_t *encoder) {
    accel->motor = motor;
    accel->encoder = encoder;

    accel->ratio = 1.0f;
    accel->dt_ms = 20;
    accel->dt_sec = 0.02f;
    accel->stopzone = 8;
    accel->max_speed = 0;
    accel->acceleration = 1.0f;

    accel->run_mode = IDLE_RUN;
    accel->current_pos = 0;
    accel->last_pos = 0;
    accel->target_pos = 0;
    accel->current_speed = 0;
    accel->target_speed = 0;
    accel->duty_f = 0.0f;
    accel->last_tick_time = k_uptime_get();

    for (int i = 0; i < 3; i++) {
        accel->speed_buf[i] = 0;
    }
    accel->speed_buf_idx = 0;
    accel->filtered_speed = 0.0f;

    accel->kp = 2.0f;
    accel->ki = 0.9f;
    accel->kd = 0.1f;
    accel->integral = 0.0f;
    accel->prev_input = 0;
    accel->last_speed = 0.0f;

    accel->control_pos = 0.0f;
    accel->control_speed = 0.0f;
}

void zephyr_accelmotor_set_dt(zephyr_accelmotor_t *accel, uint32_t dt_ms) {
    accel->dt_ms = dt_ms;
    accel->dt_sec = dt_ms / 1000.0f;
}

void zephyr_accelmotor_set_ratio(zephyr_accelmotor_t *accel, float ratio) {
    accel->ratio = ratio;
}

void zephyr_accelmotor_set_stopzone(zephyr_accelmotor_t *accel, uint32_t zone) {
    accel->stopzone = zone;
}

void zephyr_accelmotor_set_run_mode(zephyr_accelmotor_t *accel, accel_run_mode_t mode) {
    accel->run_mode = mode;
    accel->integral = 0.0f;

    if (mode == ACCEL_POS) {
        accel->control_pos = (float)accel->current_pos;
        accel->control_speed = 0.0f;
    }
}

accel_run_mode_t zephyr_accelmotor_get_run_mode(zephyr_accelmotor_t *accel) {
    return accel->run_mode;
}

static int16_t filter_speed(zephyr_accelmotor_t *accel, int16_t new_speed) {
    accel->speed_buf[accel->speed_buf_idx] = new_speed;
    accel->speed_buf_idx = (accel->speed_buf_idx + 1) % 3;

    int16_t a = accel->speed_buf[0];
    int16_t b = accel->speed_buf[1];
    int16_t c = accel->speed_buf[2];

    int16_t median;
    if ((a <= b) && (a <= c)) {
        median = (b <= c) ? b : c;
    } else if ((b <= a) && (b <= c)) {
        median = (a <= c) ? a : c;
    } else {
        median = (a <= b) ? a : b;
    }

    accel->filtered_speed += (median - accel->filtered_speed) * 0.7f;
    return (int16_t)accel->filtered_speed;
}

static void pid_control(zephyr_accelmotor_t *accel, int32_t target, int32_t current, bool cutoff) {
    int32_t err = target - current;

    accel->duty_f = (float)err * accel->kp;
    accel->duty_f += (float)(accel->prev_input - current) * accel->kd / accel->dt_sec;
    accel->prev_input = current;

    accel->integral += (float)err * accel->ki * accel->dt_sec;
    accel->duty_f += accel->integral;

    uint16_t min_duty = 0;

    if (cutoff) {
        if (iabs32(err) > (int32_t)accel->stopzone) {
            accel->duty_f += sign(err) * min_duty;
        } else {
            accel->integral = 0.0f;
        }
    } else {
        if (err == 0 && target == 0) {
            accel->integral = 0.0f;
        } else {
            accel->duty_f += sign(err) * min_duty;
        }
    }

    if (accel->max_speed > 0) {
        if (accel->duty_f > accel->max_speed) accel->duty_f = (float)accel->max_speed;
        if (accel->duty_f < -accel->max_speed) accel->duty_f = -(float)accel->max_speed;
    }

    zephyr_motor_set_speed(accel->motor, (int16_t)accel->duty_f);
}

bool zephyr_accelmotor_tick(zephyr_accelmotor_t *accel, int32_t pos) {
    accel->current_pos = pos;
    uint32_t now = k_uptime_get();

    if (now - accel->last_tick_time >= accel->dt_ms) {
        accel->last_tick_time += accel->dt_ms;

        int32_t delta_pos = accel->current_pos - accel->last_pos;
        accel->current_speed = (int16_t)(delta_pos * 1000 / (int32_t)accel->dt_ms);
        accel->current_speed = filter_speed(accel, accel->current_speed);
        accel->last_pos = accel->current_pos;

        int8_t motor_dir = zephyr_motor_get_state(accel->motor);
        if (motor_dir != 0) {
            zephyr_encoder_set_direction(accel->encoder, motor_dir);
        }
        
        switch (accel->run_mode) {
            case ACCEL_POS: {
                float err = (float)(accel->target_pos - accel->control_pos);

                if (fabs32(err) > 0.1f) {
                    float accel_step = accel->acceleration * accel->dt_sec;

                    if (fabs32(err) < accel->stopzone &&
                        fabs32(accel->last_speed - accel->control_speed) < 2.0f) {
                        accel->control_pos = (float)accel->target_pos;
                        accel->control_speed = 0.0f;
                        accel_step = 0.0f;
                    }

                    if (fabs32(err) < (accel->control_speed * accel->control_speed) / (2.0f * accel->acceleration)) {
                        err = -err;
                        accel_step = (accel->control_speed * accel->control_speed) / (2.0f * fabs32(err));
                        if (sign(accel->control_speed) == sign(err)) err = -err;
                    }

                    accel->control_speed += accel_step * sign(err);

                    if (accel->max_speed > 0) {
                        float max_speed_dt = (float)accel->max_speed * accel->dt_sec;
                        if (accel->control_speed > max_speed_dt) accel->control_speed = max_speed_dt;
                        if (accel->control_speed < -max_speed_dt) accel->control_speed = -max_speed_dt;
                    }

                    accel->control_pos += accel->control_speed;
                    accel->last_speed = accel->control_speed;
                }

                pid_control(accel, (int32_t)accel->control_pos, accel->current_pos, true);
                break;
            }

            case PID_POS:
                pid_control(accel, accel->target_pos, accel->current_pos, true);
                break;

            case ACCEL_SPEED: {
                int16_t err = accel->target_speed - accel->current_speed;
                float reducer = (fabs32((float)err) < accel->acceleration) ? 1.0f : fabs32((float)err) / accel->acceleration;
                accel->duty_f += (float)sign(err) * accel->acceleration * accel->dt_sec * reducer;
                if (accel->max_speed > 0) {
                    if (accel->duty_f > accel->max_speed) accel->duty_f = (float)accel->max_speed;
                    if (accel->duty_f < -accel->max_speed) accel->duty_f = -(float)accel->max_speed;
                }
                zephyr_motor_set_speed(accel->motor, (int16_t)accel->duty_f);
                break;
            }

            case PID_SPEED:
                pid_control(accel, accel->target_speed, accel->current_speed, false);
                break;

            default:
                break;
        }
    }

    if (accel->run_mode > 1) {
        return zephyr_motor_get_state(accel->motor) != 0;
    } else {
        return (zephyr_motor_get_state(accel->motor) != 0) ||
               (iabs32(accel->target_pos - accel->current_pos) > (int32_t)accel->stopzone);
    }
}

void zephyr_accelmotor_set_target_pos(zephyr_accelmotor_t *accel, int32_t pos) {
    accel->target_pos = pos;
}

void zephyr_accelmotor_set_target_pos_deg(zephyr_accelmotor_t *accel, int32_t deg) {
    accel->target_pos = (int32_t)((float)deg * accel->ratio / 360.0f);
}

int32_t zephyr_accelmotor_get_target_pos(zephyr_accelmotor_t *accel) {
    return accel->target_pos;
}

int32_t zephyr_accelmotor_get_target_pos_deg(zephyr_accelmotor_t *accel) {
    return (int32_t)((float)accel->target_pos * 360.0f / accel->ratio);
}

void zephyr_accelmotor_set_target_speed(zephyr_accelmotor_t *accel, int16_t speed) {
    accel->target_speed = speed;
}

void zephyr_accelmotor_set_target_speed_deg(zephyr_accelmotor_t *accel, int16_t speed) {
    accel->target_speed = (int16_t)((float)speed * accel->ratio / 360.0f);
}

int16_t zephyr_accelmotor_get_target_speed(zephyr_accelmotor_t *accel) {
    return accel->target_speed;
}

int16_t zephyr_accelmotor_get_target_speed_deg(zephyr_accelmotor_t *accel) {
    return (int16_t)((float)accel->target_speed * 360.0f / accel->ratio);
}

int32_t zephyr_accelmotor_get_current_pos(zephyr_accelmotor_t *accel) {
    return accel->current_pos;
}

int32_t zephyr_accelmotor_get_current_pos_deg(zephyr_accelmotor_t *accel) {
    return (int32_t)((float)accel->current_pos * 360.0f / accel->ratio);
}

int16_t zephyr_accelmotor_get_current_speed(zephyr_accelmotor_t *accel) {
    return accel->current_speed;
}

int16_t zephyr_accelmotor_get_current_speed_deg(zephyr_accelmotor_t *accel) {
    return (int16_t)((float)accel->current_speed * 360.0f / accel->ratio);
}

float zephyr_accelmotor_get_duty(zephyr_accelmotor_t *accel) {
    return accel->duty_f;
}

void zephyr_accelmotor_set_max_speed(zephyr_accelmotor_t *accel, uint16_t speed) {
    accel->max_speed = speed;
}

void zephyr_accelmotor_set_max_speed_deg(zephyr_accelmotor_t *accel, uint16_t speed) {
    accel->max_speed = (uint16_t)((float)speed * accel->ratio / 360.0f);
}

void zephyr_accelmotor_set_acceleration(zephyr_accelmotor_t *accel, float accel_val) {
    accel->acceleration = accel_val;
}

void zephyr_accelmotor_set_acceleration_deg(zephyr_accelmotor_t *accel, float accel_val) {
    accel->acceleration = accel_val * accel->ratio / 360.0f;
}

bool zephyr_accelmotor_is_blocked(zephyr_accelmotor_t *accel) {
    uint16_t min_duty = 0;
    return (fabs32(accel->duty_f) > min_duty && accel->current_speed == 0);
}
