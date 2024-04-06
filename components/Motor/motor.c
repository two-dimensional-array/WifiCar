#include "motor.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_log.h"

#define MOTOR_GPIO_PIN_MASK \
	( \
		BIT(MOTOR_DRIVER_LEFT_FORWARD_PIN) | \
		BIT(MOTOR_DRIVER_LEFT_BACKWARD_PIN) | \
		BIT(MOTOR_DRIVER_RIGHT_FORWARD_PIN) | \
		BIT(MOTOR_DRIVER_RIGHT_BACKWARD_PIN) | \
		BIT(MOTOR_DRIVER_STBY_PIN) \
	) \

static const uint32_t pwmPins[2] = {MOTOR_DRIVER_LEFT_PWM_PIN, MOTOR_DRIVER_RIGHT_PWM_PIN};
static uint32_t pwmDuties[2] = {0, 0};
static float pwmPhases[2] = {0.0, 0.0};
static motor_driver_t motorDriver;

static motor_state_t motorDirection = MOTOR_STATE_STOP;
static TimerHandle_t motorStopTimer;

static const char *TAG = "motor";

static void MotorStopCallback(TimerHandle_t xTimer)
{
	MotorDriverStop(&motorDriver);
	motorDirection = MOTOR_STATE_STOP;
}

void MotorInit(void)
{
	motorDriver.left.backward = MOTOR_DRIVER_LEFT_BACKWARD_PIN;
	motorDriver.left.forward = MOTOR_DRIVER_LEFT_FORWARD_PIN;
	motorDriver.left.pwm = MOTOR_DRIVER_LEFT_PWM_CHANNEL;
	motorDriver.right.backward = MOTOR_DRIVER_RIGHT_BACKWARD_PIN;
	motorDriver.right.forward = MOTOR_DRIVER_RIGHT_FORWARD_PIN;
	motorDriver.right.pwm = MOTOR_DRIVER_RIGHT_PWM_CHANNEL;
	motorDriver.stby = MOTOR_DRIVER_STBY_PIN;
	gpio_config_t gpioConfig;
	gpioConfig.pin_bit_mask = MOTOR_GPIO_PIN_MASK;
	gpioConfig.mode = GPIO_MODE_OUTPUT;
	gpioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
	gpioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpioConfig.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&gpioConfig);
	pwm_init(MOTOR_DRIVER_PWM_PERIOD, pwmDuties, 2, pwmPins);
	pwm_set_phases(pwmPhases);
	pwm_start();
	MotorDriverStop(&motorDriver);
	motorStopTimer = xTimerCreate("MotorStop", pdMS_TO_TICKS(700), false, NULL, MotorStopCallback);
}

void MotorControl(motor_state_t direction)
{
	xTimerReset(motorStopTimer, pdMS_TO_TICKS(0));
	if (motorDirection != direction)
	{
		motorDirection = direction;
		switch (motorDirection)
		{
			case MOTOR_STATE_FORWARD_LEFT:
				MotorRunForward(&motorDriver.left, MOTOR_DRIVER_PWM_PERIOD);
			    MotorRunBackward(&motorDriver.right, MOTOR_DRIVER_PWM_PERIOD / 2);
			    MOTOR_DRIVER_SET_GPIO_STATE(&motorDriver.stby, true);
			    break;
			case MOTOR_STATE_FORWARD:
				MotorDriverRunForward(&motorDriver, MOTOR_DRIVER_PWM_PERIOD);
				break;
			case MOTOR_STATE_FORWARD_RIGHT:
				MotorRunBackward(&motorDriver.left, MOTOR_DRIVER_PWM_PERIOD / 2);
				MotorRunForward(&motorDriver.right, MOTOR_DRIVER_PWM_PERIOD);
				MOTOR_DRIVER_SET_GPIO_STATE(&motorDriver.stby, true);
				break;
			case MOTOR_STATE_BACKWARD_LEFT:
				MotorRunBackward(&motorDriver.left, MOTOR_DRIVER_PWM_PERIOD);
				MotorRunForward(&motorDriver.right, MOTOR_DRIVER_PWM_PERIOD / 2);
				MOTOR_DRIVER_SET_GPIO_STATE(&motorDriver.stby, true);
				break;
			case MOTOR_STATE_BACKWARD:
				MotorDriverRunBackward(&motorDriver, MOTOR_DRIVER_PWM_PERIOD);
				break;
			case MOTOR_STATE_BACKWARD_RIGHT:
				MotorRunForward(&motorDriver.left, MOTOR_DRIVER_PWM_PERIOD / 2);
				MotorRunBackward(&motorDriver.right, MOTOR_DRIVER_PWM_PERIOD);
				MOTOR_DRIVER_SET_GPIO_STATE(&motorDriver.stby, true);
				break;
			case MOTOR_STATE_LEFT:
				MotorDriverRunLeft(&motorDriver, MOTOR_DRIVER_PWM_PERIOD);
				break;
			case MOTOR_STATE_RIGHT:
				MotorDriverRunRight(&motorDriver, MOTOR_DRIVER_PWM_PERIOD);
				break;
			default:
				xTimerStop(motorStopTimer, pdMS_TO_TICKS(0));
				MotorDriverStop(&motorDriver);
				motorDirection = MOTOR_STATE_STOP;
				break;
		}
	}
}

void MotorDriverSetPWMPower(motor_pwm_t* pwmHandler, uint8_t percent)
{
	pwm_set_duty(*pwmHandler, percent);
	pwm_start();
}
