#ifndef COMPONENTS_UDPSERVER_UDP_SERVER_H_
#define COMPONENTS_UDPSERVER_UDP_SERVER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ringbuf.h"

#define PORT 3333

void UDPServerStart(RingbufHandle_t udpReciveData);

#endif /* COMPONENTS_UDPSERVER_UDP_SERVER_H_ */
