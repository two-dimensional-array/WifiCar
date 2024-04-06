#ifndef __MOTOR_MOTOR_H__
#define __MOTOR_MOTOR_H__

#include "motor_driver.h"

typedef enum
{
	MOTOR_STATE_STOP = 0,
	MOTOR_STATE_FORWARD_LEFT,
	MOTOR_STATE_FORWARD,
	MOTOR_STATE_FORWARD_RIGHT,
	MOTOR_STATE_LEFT,
	MOTOR_STATE_RIGHT,
	MOTOR_STATE_BACKWARD_LEFT,
	MOTOR_STATE_BACKWARD,
	MOTOR_STATE_BACKWARD_RIGHT,
	MOTOR_STATE_AMOUNT
} motor_state_t;

void MotorInit(void);
void MotorControl(motor_state_t direction);

#endif // __MOTOR_MOTOR_H__
