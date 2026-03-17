#include "zephyr_accelmotor.h"
#include <zephyr/logging/log.h>
#include <math.h>
#include <string.h>

LOG_MODULE_REGISTER(accel_motor, LOG_LEVEL_INF);

#define SIGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define ABS(x) ((x) > 0 ? (x) : -(x))

bool AccelMotor::tick(long pos) {
	_currentPos = pos;
	if (millis() - _tmr2 > _dt) {
		_tmr2 += _dt;
		_curSpeed = (long)(_currentPos - _lastPos) / _dts;
		_curSpeed = filter(_curSpeed);
		_lastPos = _currentPos;
		
		switch (_runMode) {		
		case ACCEL_POS:
			{
				long err = _targetPos - controlPos;
				if (err != 0) {
					float accelDt = _accel * _dts;
					float accel = accelDt;
					if (abs(err) < _stopzone && abs(_lastSpeed - controlSpeed) < 2) {
						controlPos = _targetPos;
						controlSpeed = 0;
						accel = 0;
					}
					if (abs(err) < (float)(controlSpeed * controlSpeed) / (2.0 * accelDt)) {
						err = -err;
						accel = (float)(controlSpeed * controlSpeed) / (2.0 * abs(err));
						if (_sign(controlSpeed) == _sign(err)) err = -err;
					}
					controlSpeed += accel * _sign(err);
					controlSpeed = constrain(controlSpeed, -_maxSpeed*_dts, _maxSpeed*_dts);
					controlPos += controlSpeed;
					_lastSpeed = controlSpeed;	
					
				}
				PIDcontrol(controlPos, _currentPos, true);
			}
			break;
		case PID_POS:				
			PIDcontrol(_targetPos, _currentPos, true);
			break;
		case ACCEL_SPEED:
			{
				int err = _targetSpeed - _curSpeed;
				float reducer = min(abs(err) / _accel, 1.0);
				_dutyF += (float)_sign(err) * _accel * _dts * reducer;
				_dutyF = constrain(_dutyF, -_maxDuty, _maxDuty);
				setSpeed(_dutyF);
			}			
			break;
		case PID_SPEED:			
			PIDcontrol(_targetSpeed, _curSpeed, false);			
			break;
		}		
	}
	if (_runMode > 1) return (getState() != 0);
	else return (getState() != 0 || abs(_targetPos - _currentPos) > _stopzone);
}

void AccelMotor::PIDcontrol(long target, long current, bool cutoff) {
	
	long err = target - current;
	_dutyF = err * kp;	
	_dutyF += (float)(_prevInput - current) * kd / _dts;
	_prevInput = current;
	integral += (float)err * ki * _dts;
	_dutyF += integral;	
	if (cutoff) {
		if (abs(err) > _stopzone) _dutyF += _sign(err)*_minDuty;
		else integral = 0;		
	} else {
		if (err == 0 && target == 0) integral = 0;
		else _dutyF += _sign(err)*_minDuty;
	}
	_dutyF = constrain(_dutyF, -_maxDuty, _maxDuty);
	setSpeed(_dutyF);
}

bool AccelMotor::isBlocked() {
	return (abs(_dutyF) > _minDuty && _curSpeed == 0);
}

// settings
void AccelMotor::setRatio(float ratio) {
	_ratio = ratio;
}
void AccelMotor::setDt(int dt) {
	_dt = dt;
	_dts = dt / 1000.0;
}
void AccelMotor::setCurrent(long pos) {	
	_currentPos = pos;
}
void AccelMotor::setRunMode(runMode mode) {
	_runMode = mode;
	if (mode == ACCEL_POS) controlPos = _currentPos;
}

// current position
long AccelMotor::getCurrent() {
	return _currentPos;
}
long AccelMotor::getCurrentDeg() {
	return (long)_currentPos * 360 / _ratio;
}

// current speed
int AccelMotor::getSpeed() {
	return _curSpeed;
}
int AccelMotor::getSpeedDeg() {
	return (long)_curSpeed * 360 / _ratio;
}
float AccelMotor::getDuty() {
	return _dutyF;
}

// target position mode
void AccelMotor::setTarget(long pos) {
	_targetPos = pos;
}
void AccelMotor::setTargetDeg(long pos) {
	_targetPos = (long)pos * _ratio / 360;	
}
long AccelMotor::getTarget() {
	return _targetPos;
}
long AccelMotor::getTargetDeg() {
	return (long)_targetPos * 360 / _ratio;
}

void AccelMotor::setStopZone(int zone) {
	_stopzone = zone;
}

// target speed mode
void AccelMotor::setTargetSpeed(int speed) {
	_targetSpeed = speed;
}
void AccelMotor::setTargetSpeedDeg(int speed) {
	_targetSpeed = (long)speed * _ratio / 360;	
}
int AccelMotor::getTargetSpeed() {
	return _targetSpeed;
}
int AccelMotor::getTargetSpeedDeg() {
	return (long)_targetSpeed * 360 / _ratio;
}

// max speed / acceleration
void AccelMotor::setMaxSpeed(int speed) {
	_maxSpeed = speed;
}
void AccelMotor::setMaxSpeedDeg(int speed) {
	_maxSpeed = (long)speed * _ratio / 360;
}
void AccelMotor::setAcceleration(float accel) {
	_accel = accel;
}
void AccelMotor::setAccelerationDeg(float accel) {
	_accel = (float)accel * _ratio / 360.0;
}


int AccelMotor::filter(int newVal) {
	_buf[_count] = newVal;
	if (++_count >= 3) _count = 0;
	int middle = 0;
	if ((_buf[0] <= _buf[1]) && (_buf[0] <= _buf[2])) {
		middle = (_buf[1] <= _buf[2]) ? _buf[1] : _buf[2];
	} else {
		if ((_buf[1] <= _buf[0]) && (_buf[1] <= _buf[2])) {
			middle = (_buf[0] <= _buf[2]) ? _buf[0] : _buf[2];
		}
		else {
			middle = (_buf[0] <= _buf[1]) ? _buf[0] : _buf[1];
		}
	}
	_middle_f += (middle-_middle_f) * 0.7;
	return _middle_f;
}
