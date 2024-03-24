#include "wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include <string.h>

#define AP_STAIPASSIGNED BIT(0)
#define AP_STACONNECTED BIT(1)

static const char *TAG = "wifi softAP";

static EventGroupHandle_t wifiEventGroup;
static const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
static wifi_config_t wifi_config =
{
	.ap =
	{
		.ssid = EXAMPLE_ESP_WIFI_SSID,
		.ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
		.password = EXAMPLE_ESP_WIFI_PASS,
		.max_connection = EXAMPLE_MAX_STA_CONN,
		.authmode = WIFI_AUTH_WPA_WPA2_PSK
	},
};

static void onAPSTAConnected(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	if (arg != NULL) {
	    system_event_ap_staconnected_t *event = (system_event_ap_staconnected_t *)arg;
	    ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
	                     MAC2STR(event->mac), event->aid);
	}

    xEventGroupSetBits(wifiEventGroup, AP_STACONNECTED);
}

static void onAPSTAIDAssigned(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	if (arg != NULL) {
		ip_event_ap_staipassigned_t *event = (ip_event_ap_staipassigned_t *)arg;
		ESP_LOGI(TAG, "IPv4 address: " IPSTR, IP2STR(&event->ip));
	}
    xEventGroupSetBits(wifiEventGroup, AP_STAIPASSIGNED);
}

void WifiAPInit(void)
{
    wifiEventGroup = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &onAPSTAConnected, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &onAPSTAIDAssigned, NULL));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

    xEventGroupWaitBits(wifiEventGroup, AP_STAIPASSIGNED | AP_STACONNECTED, true, true, portMAX_DELAY);
}
