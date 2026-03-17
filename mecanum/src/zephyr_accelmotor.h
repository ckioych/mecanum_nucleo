#pragma once
#include <Arduino.h>
#include <Motor.h>

enum runMode {	
	ACCEL_POS,
	PID_POS,
	ACCEL_SPEED,
	PID_SPEED,
	IDLE_RUN,
};

class AccelMotor : public GMotor {
public:
	using GMotor::GMotor;

	bool tick(long pos);							
	
	void setRatio(float ratio);	

	void setDt(int dt);
	
	void setCurrent(long pos);
	long getCurrent();
	long getCurrentDeg();
	
	void setTarget(long pos);
	void setTargetDeg(long pos);
	long getTarget();
	long getTargetDeg();
	
	void setMaxSpeed(int speed);
	void setMaxSpeedDeg(int speed);
	
	void setAcceleration(float accel);
	void setAccelerationDeg(float accel);
	
	void setTargetSpeed(int speed);
	void setTargetSpeedDeg(int speed);
	int getTargetSpeed();
	int getTargetSpeedDeg();
	
	int getSpeed();
	int getSpeedDeg();	
	
	float getDuty();
	
	void setRunMode(runMode mode);
	
	bool isBlocked();
	
	float kp = 2.0;
	
	float ki = 0.9;
	
	float kd = 0.1;
	
	void setStopZone(int zone);
	
private:
	int filter(int newVal);
	int _buf[3];
	byte _count = 0;
	float _middle_f = 0;
	
	float _lastSpeed = 0;	
	void PIDcontrol(long target, long current, bool cutoff);
	float integral = 0;
	int _dt = 20;
	float _dts = 0.02;
	long _lastPos = 0, _currentPos = 0, _targetPos = 0;
	int _curSpeed = 0;
	int _maxSpeed = 0, _targetSpeed = 0;
	float _ratio = 1;
	uint32_t _tmr2 = 0;
	float _accel = 1;
	float _dutyF = 0;
	long controlPos = 0;
	float controlSpeed = 0;
	int _stopzone = 8;
	long _prevInput = 0;
	runMode _runMode = IDLE_RUN;
};

