#include "udp_server.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "esp_log.h"
#include "esp_task_wdt.h"

#include <stddef.h>

static const char *TAG = "udp_server";

static void UDPServerTask(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    RingbufHandle_t udpReciveData = pvParameters;

    if (pvParameters == NULL)
    {
    	ESP_LOGE(TAG, "Error udpReciveData is NULL.");
    	vTaskDelete(NULL);
    	return;
    }

    for(;;)
    {
    	esp_task_wdt_reset();
        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = bind(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err < 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGD(TAG, "Socket binded");

        for(;;)
        {
        	esp_task_wdt_reset();
            ESP_LOGD(TAG, "Waiting for data");
            struct sockaddr_in sourceAddr;
            socklen_t socklen = sizeof(sourceAddr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&sourceAddr, &socklen);

            // Error occured during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                // Get the sender's ip address as string
                inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
                ESP_LOGD(TAG, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGD(TAG, "%s", rx_buffer);

//                int err = sendto(sock, rx_buffer, len, 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr));
//                if (err < 0) {
//                    ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
//                    break;
//                }
                xRingbufferSend(udpReciveData, rx_buffer, len, 1000);
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void UDPServerStart(RingbufHandle_t udpReciveData)
{
	xTaskCreate(UDPServerTask, "UDPServerTask", 4096, udpReciveData, 5, NULL);
}

