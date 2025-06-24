#ifndef ENC28J60_NET_INTERFACE_H
#define ENC28J60_NET_INTERFACE_H

#include <stdbool.h>

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "task.h"

void pxFillInterfaceDescriptor(NetworkInterface_t *netInterface);

bool netInterfaceIsUp(void);

#endif
