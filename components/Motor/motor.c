#include "motor.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_task_wdt.h"

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

static char motorDirection = 0x00;
static uint32_t motorTimeout;
static SemaphoreHandle_t motorMutex;

static const char *TAG = "motor";

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
}

static void MotorControlTask(void *pvParameters)
{
	motorDirection = 0x00;
	motorTimeout = xTaskGetTickCount();

	for(;;)
	{
		esp_task_wdt_reset();
		if (xSemaphoreTake(motorMutex, 0))
		{
			uint32_t current_time = xTaskGetTickCount();
			uint32_t delta = (current_time < motorTimeout) ? ((UINT32_MAX - motorTimeout) + current_time) : (current_time - motorTimeout);
			if (delta > (500 / portTICK_PERIOD_MS))
			{
				if (motorDirection != 0x00)
				{
					ESP_LOGD(TAG, "End state: %c\n\r", motorDirection);
					MotorDriverStop(&motorDriver);
					motorDirection = 0x00;
				}
				motorTimeout = xTaskGetTickCount();
			}
			xSemaphoreGive(motorMutex);
		}
	}

	vTaskDelete(NULL);
}

void MotorTaskStart(void)
{
	motorMutex = xSemaphoreCreateMutex();
	xTaskCreate(MotorControlTask, "MotorControlTask", 4096, NULL, 5, NULL);
}

void MotorControl(char direction)
{
	if (xSemaphoreTake(motorMutex, portMAX_DELAY))
	{
		if (motorDirection != direction)
		{
			if (motorDirection != 0x00)
			{
				ESP_LOGD(TAG, "End state: %c\n\r", motorDirection);
			}
			motorDirection = direction;
			ESP_LOGD(TAG, "Start state: %c\n\r", motorDirection);
			switch (motorDirection)
			{
				case 'f':
					MotorDriverRunForward(&motorDriver, MOTOR_DRIVER_PWM_PERIOD);
					break;
				case 'b':
					MotorDriverRunBackward(&motorDriver, MOTOR_DRIVER_PWM_PERIOD);
					break;
				case 'l':
					MotorDriverRunLeft(&motorDriver, MOTOR_DRIVER_PWM_PERIOD);
					break;
				case 'r':
					MotorDriverRunRight(&motorDriver, MOTOR_DRIVER_PWM_PERIOD);
					break;
				default:
					break;
			}
		}
		motorTimeout = xTaskGetTickCount();
		xSemaphoreGive(motorMutex);
	}
}

void MotorDriverSetPWMPower(motor_pwm_t* pwmHandler, uint8_t percent)
{
	pwm_set_duty(*pwmHandler, percent);
	pwm_start();
}
