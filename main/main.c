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

static const char *TAG = "main";

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
		char* msgData = xRingbufferReceive(udpReciveData, &msgSize, 0);
		if (msgData != NULL && msgSize > 0)
		{
			MotorControl(msgData[0]); // Need implement decoding msg's
			vRingbufferReturnItem(udpReciveData, msgData);
		}
		esp_task_wdt_reset();
	}

	vTaskDelete(NULL);
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
    MotorTaskStart();
    UDPServerStart(udpReciveData);
    xTaskCreate(DispatcherTask, "dispatcher", 4096, udpReciveData, 5, NULL);
}
