#include "zephyr_motor.h"
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(zephyr_motor, LOG_LEVEL_INF);

static void set_pins(struct GMotor *motor, bool a_state, bool b_state, uint32_t pwm_value)
{
    if (motor->dig_a.port != NULL) {
        gpio_pin_set_dt(&motor->dig_a, a_state);
    }
    if (motor->dig_b.port != NULL) {
        gpio_pin_set_dt(&motor->dig_b, b_state);
    }

    if (motor->type == DRIVER2WIRE || motor->type == DRIVER2WIRE_NO_INVERT) {
        if (motor->pwm_a.dev != NULL) {
            pwm_set_pulse_dt(&motor->pwm_a, pwm_value);
        }
        if (motor->pwm_b.dev != NULL && motor->type == DRIVER2WIRE) {
            pwm_set_pulse_dt(&motor->pwm_b, motor->max_duty * PWM_USEC(20) - pwm_value);
        }
    } else if (motor->type == DRIVER3WIRE) {
        if (motor->pwm_a.dev != NULL) {
            pwm_set_pulse_dt(&motor->pwm_a, pwm_value);
        }
    }
}

void GMotor_init(struct GMotor *motor, GM_driverType type,
                 const char *pwm_dev_name, uint32_t pwm_pin_a, uint32_t pwm_pin_b,
                 bool active_high) {

    memset(motor, 0, sizeof(struct GMotor));

    motor->type = type;
    motor->state = 0;
    motor->duty = 0;
    motor->target_duty = 0;
    motor->min_duty = 0;
    motor->max_duty = 255;
    motor->direction_reverse = false;
    motor->deadtime_us = 0;
    motor->smooth_enabled = false;
    motor->smooth_step = 20;
    motor->current_mode = STOP;
    motor->last_mode = STOP;
    motor->resolution = 8;

    if (pwm_dev_name != NULL) {
        if (pwm_pin_a != GM_NC) {
            motor->pwm_a.dev = device_get_binding(pwm_dev_name);
            motor->pwm_a.channel = pwm_pin_a;
            motor->pwm_a.period = PWM_USEC(20);
            if (!motor->pwm_a.dev) {
                LOG_ERR("PWM device %s not found", pwm_dev_name);
            }
        }

        if (pwm_pin_b != GM_NC && type != DRIVER3WIRE) {
            motor->pwm_b.dev = device_get_binding(pwm_dev_name);
            motor->pwm_b.channel = pwm_pin_b;
            motor->pwm_b.period = PWM_USEC(20);
        }
    }
    
    LOG_INF("Motor initialized, type: %d", type);
}




GMotor::GMotor(GM_driverType type, int8_t param1, int8_t param2, int8_t param3, int8_t param4) {
    _type = type;
    switch (_type) {
    case DRIVER2WIRE_NO_INVERT:    
    case DRIVER2WIRE:
        _digA = param1;
        _pwmC = param2;
        if (param3 != _GM_NC) _level = !param3;
        break;
    case DRIVER3WIRE:
        _digA = param1;
        _digB = param2;
        _pwmC = param3;    
        if (param4 != _GM_NC) _level = !param4;
        break;    
    case RELAY2WIRE:
        _digA = param1;
        _digB = param2;
        if (param3 != _GM_NC) _level = !param3;
        break;        
    }

    if (_digA != _GM_NC) pinMode(_digA, OUTPUT);
    if (_digB != _GM_NC) pinMode(_digB, OUTPUT);
    if (_pwmC != _GM_NC) pinMode(_pwmC, OUTPUT);
    
    setMode(STOP);
}

void GMotor::setSpeed(int16_t duty) {
    if (_mode < 2) {
        _duty = constrain(duty, -_maxDuty, _maxDuty);
        
        if (_maxDuty > 255 && abs(_duty) == 255) _duty++;
        
        if (duty == 0) run(STOP, 0);
        else {
            if (duty > 0) {
                if (_minDuty != 0) _duty = _duty * _k + _minDuty;
                run(_mode, _duty);
            } else {
                if (_minDuty != 0) _duty = _duty * _k - _minDuty;
                run(BACKWARD, -_duty);
            }
        }
    }
}

void GMotor::run(GM_workMode mode, int16_t duty) {
    if (_deadtime > 0 && _lastMode != mode) {
        _lastMode = mode;
        setPins(_level, _level, 0);
        delayMicroseconds(_deadtime);
    }

    if (_direction) {
        if (mode == FORWARD) mode = BACKWARD;
        else if (mode == BACKWARD) mode = FORWARD;
    }

    switch (mode) {
    case FORWARD:    setPins(_level, !_level, duty); _state = 1; break;        
    case BACKWARD:    setPins(!_level, _level, (_type == DRIVER2WIRE) ? (_maxDuty - duty) : (duty)); _state = -1; break;
    case BRAKE:        setPins(!_level, !_level, !_level * 255); _state = 0; break;
    case STOP:        setPins(_level, _level, _level * 255); _duty = _dutyS = 0; _state = 0; break;
    }
}

void GMotor::setPins(bool a, bool b, int c) {
    if (_digA != _GM_NC) digitalWrite(_digA, a);
    if (_digB != _GM_NC) digitalWrite(_digB, b);
    if (_pwmC != _GM_NC) analogWrite(_pwmC, c);
}

void GMotor::smoothTick(int16_t duty) {
    if (millis() - _tmr >= _SMOOTH_PRD) {
        _tmr = millis();
        if (abs(_dutyS - duty) > _speed) _dutyS += (_dutyS < duty) ? _speed : -_speed;
        else _dutyS = duty;
        setSpeed(_dutyS);
    }
}

int GMotor::getState() {
    return _state;
}

void GMotor::setResolution(byte bit) {
    _maxDuty = (1 << bit) - 1;
    setMinDuty(_minDuty);
}

void GMotor::setMinDuty(int duty) {
    _minDuty = duty;
    _k = 1.0 - (float)_minDuty / _maxDuty;
}

void GMotor::setMode(GM_workMode mode) {
    if (_mode == mode) return;
    _mode = mode;
    run(mode, _duty);
}

void GMotor::setSmoothSpeed(uint8_t speed) {
    _speed = speed;
}

void GMotor::setDirection(bool direction) {
    _direction = direction;
}

void GMotor::setDeadtime(uint16_t deadtime) {
    _deadtime = deadtime;
}

void GMotor::setLevel(int8_t level) {
    _level = !level;
}

void GMotor::set8bitMode() {
    setResolution(8);
}

void GMotor::set10bitMode() {
    setResolution(10);
}
