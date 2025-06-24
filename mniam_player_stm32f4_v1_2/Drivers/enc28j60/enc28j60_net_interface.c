#include "enc28j60_eth_phy.h"
#include "enc28j60_net_interface.h"
#include "enc28j60.h"

static BaseType_t netInit(NetworkInterface_t *pxDescriptor)
{
    (void)pxDescriptor;

    // Initialize the MAC
    MAC_Initialize(NULL);

    return pdPASS;
}

static BaseType_t netOut(NetworkInterface_t *pxDescriptor, NetworkBufferDescriptor_t *const pxNetworkBuffer, BaseType_t xReleaseAfterSend)
{
    (void)pxDescriptor;
    (void)xReleaseAfterSend;

    MAC_SendFrame(pxNetworkBuffer->pucEthernetBuffer, pxNetworkBuffer->xDataLength, 0);

    if(xReleaseAfterSend != pdFALSE) {
      /* It is assumed SendData() copies the data out of the FreeRTOS-Plus-TCP Ethernet
      buffer.  The Ethernet buffer is therefore no longer needed, and must be
      freed for re-use. */
      vReleaseNetworkBufferAndDescriptor(pxNetworkBuffer);
    }
    return pdTRUE;
}

static BaseType_t netGetPhyLinkStatus(struct xNetworkInterface *pxDescriptor)
{
    (void)pxDescriptor;
    return PHY_IsLinkUp() ? pdTRUE : pdFALSE;
}

bool netInterfaceIsUp(void) {
	return PHY_IsLinkUp() ? pdTRUE : pdFALSE;
}

void pxFillInterfaceDescriptor(NetworkInterface_t *netInterface)
{
    netInterface->pfInitialise = netInit;
    netInterface->pfOutput = netOut;
    netInterface->pfGetPhyLinkStatus = netGetPhyLinkStatus;
    netInterface->pfAddAllowedMAC = NULL;
    netInterface->pfRemoveAllowedMAC = NULL;
    netInterface->pcName = "ENC28J60";
    FreeRTOS_AddNetworkInterface(netInterface);
}
