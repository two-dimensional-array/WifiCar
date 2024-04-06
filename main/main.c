#include "nvs.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_task_wdt.h"
#include "esp_netif.h"

#include "ringbuf.h"

#include "wifi.h"
#include "motor.h"
#include "udp_server.h"

#include <string.h>

static const char *TAG = "main";

typedef struct
{
	const char* str;
	motor_state_t state;
} dispatcher_motor_t;

static const dispatcher_motor_t dispatcherMotor[MOTOR_STATE_AMOUNT] =
{
	{"ss", MOTOR_STATE_STOP},
	{"fl", MOTOR_STATE_FORWARD_LEFT},
	{"ff", MOTOR_STATE_FORWARD},
	{"fr", MOTOR_STATE_FORWARD_RIGHT},
	{"ll", MOTOR_STATE_LEFT},
	{"rr", MOTOR_STATE_RIGHT},
	{"bl", MOTOR_STATE_BACKWARD_LEFT},
	{"bb", MOTOR_STATE_BACKWARD},
	{"br", MOTOR_STATE_BACKWARD_RIGHT},
};

static void DispatcherTask(void *pvParameters)
{
	RingbufHandle_t udpReciveData = pvParameters;

	if (pvParameters == NULL)
	{
		ESP_LOGE(TAG, "Error udpReciveData is NULL.");
		vTaskDelete(NULL);
		return;
	}

	for (;;)
	{
		size_t msgSize = 0;
		char* msgData = xRingbufferReceive(udpReciveData, &msgSize, portMAX_DELAY);
		if (msgData != NULL && msgSize > 0)
		{
			for (size_t i = 0; i < MOTOR_STATE_AMOUNT; i++)
			{
				if (strncmp(dispatcherMotor[i].str, msgData, (msgSize <= 2) ? msgSize : 2) == 0)
				{
					MotorControl(dispatcherMotor[i].state);
				}
			}
			vRingbufferReturnItem(udpReciveData, msgData);
		}
		esp_task_wdt_reset();
	}

	vTaskDelete(NULL);
}

static void IDLETask(void *pvParameters)
{
	for (;;)
	{
		esp_task_wdt_reset();
	}
}

void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_task_wdt_init());

    RingbufHandle_t udpReciveData = xRingbufferCreate(4096, RINGBUF_TYPE_ALLOWSPLIT);

    MotorInit();
    WifiAPInit();
    UDPServerStart(udpReciveData);
    xTaskCreate(DispatcherTask, "dispatcher", 4096, udpReciveData, 5, NULL);
    xTaskCreate(IDLETask, "idle", 1024, NULL, tskIDLE_PRIORITY, NULL);
}
