#include <FreeRTOS.h>
#include <FreeRTOS_IP.h>
#include <FreeRTOS_Sockets.h>
#include <stdbool.h>
#include <task.h>
#include "enc28j60_net_interface.h"

/**
 * This function reacts to network events.
 */
void vApplicationIPNetworkEventHook_Multi(eIPCallbackEvent_t eNetworkEvent, struct xNetworkEndPoint *pxEndPoint) {
  (void)eNetworkEvent;
  (void)pxEndPoint;
  switch(eNetworkEvent) {
  case eNetworkUp:
    // network comes up immediately at Ethernet driver initialization independently on the link status
    printf("Network up\n");
    break;
  case eNetworkDown:
    printf("Network down\n");
    break;
  }
}

BaseType_t xApplicationGetRandomNumber(uint32_t *pulNumber) {
  static uint32_t cnt = 0;
  *pulNumber = ++cnt;
  return pdTRUE;
}

uint32_t ulApplicationGetNextSequenceNumber(uint32_t ulSourceAddress, uint16_t usSourcePort, uint32_t ulDestinationAddress,
                                            uint16_t usDestinationPort) {
  (void)ulSourceAddress;
  (void)usSourcePort;
  (void)ulDestinationAddress;
  (void)usDestinationPort;
  uint32_t seq = 123;
  return seq++;
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName) {
  (void)pxTask;
  (void)pcTaskName;
  printf("Stack overflow in task %s\n", pcTaskName);
  while(1)
    ;
}

