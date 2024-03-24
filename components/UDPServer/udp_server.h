#ifndef __UDPSERVER_UDP_SERVER_H__
#define __UDPSERVER_UDP_SERVER_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ringbuf.h"

#define PORT 3333

void UDPServerStart(RingbufHandle_t udpReciveData);

#endif /* __UDPSERVER_UDP_SERVER_H__ */
