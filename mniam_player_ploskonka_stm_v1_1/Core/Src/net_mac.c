#include <stdint.h>
#include <cmsis_os2.h>
#include <FreeRTOS.h>
#include <FreeRTOS_IP.h>
#include <FreeRTOS_Sockets.h>
#include <enc28j60_net_interface.h>
#include <enc28j60_eth_phy.h>



void NET_MACThread(void *arg) {

	static NetworkInterface_t netInterface;
	static NetworkEndPoint_t netEndpoint;

	// The last 4 bytes of the MAC address will be read from the chip
	uint8_t ucMACAddress[6] = {0xaa, 0xbb, 0x00, 0x00, 0x00, 0x00};
	uint32_t uid = HAL_GetUIDw0();
	memcpy(ucMACAddress+2, &uid, 4);
	MAC_SetMACAddress(ucMACAddress);


	// Prepare the network interface
	pxFillInterfaceDescriptor(&netInterface);

	// These will only be used if DHCP fails:
	uint8_t ip[4] = {192, 168, 12, 2};
	uint8_t mask[4] = {255,255,255,0};
	uint8_t gateway[4] = {192,168,12,1};
	uint8_t dns[4] = {8,8,8,8};

	// Before initialization at least one endpoint must be set
	FreeRTOS_FillEndPoint(&netInterface, &netEndpoint, ip, mask, gateway, dns, ucMACAddress);
	#if ( ipconfigUSE_DHCP != 0 )
		{
			/* End-point 0 wants to use DHCPv4. */
		netEndpoint.bits.bWantDHCP = pdTRUE;
		}
	#endif /* ( ipconfigUSE_DHCP != 0 ) */
	FreeRTOS_IPInit_Multi();

	while(1) {
		size_t xBytesReceived = MAC_GetRxFrameSize();
		if (xBytesReceived > 0) {
			// There is received packet pending
			// Allocate buffer for packet
			xNetworkBufferDescriptor_t *pxBufferDescriptor;
			pxBufferDescriptor = pxGetNetworkBufferWithDescriptor( xBytesReceived, 0 );
			if( pxBufferDescriptor != NULL ) {
				// Read packet content
				MAC_ReadFrame(pxBufferDescriptor->pucEthernetBuffer, xBytesReceived);
				// Fill in the descriptor
				pxBufferDescriptor->xDataLength = xBytesReceived;
				pxBufferDescriptor->pxInterface = &netInterface;
				pxBufferDescriptor->pxEndPoint = &netEndpoint;

				// The event about to be sent to the TCP/IP is an Rx event
				xIPStackEvent_t xRxEvent;
				xRxEvent.eEventType = eNetworkRxEvent;

				// pvData is used to point to the network buffer descriptor that now references the received data.
				xRxEvent.pvData = ( void * ) pxBufferDescriptor;

				// Send the data to the TCP/IP stack.
				if( xSendEventStructToIPTask( &xRxEvent, 0 ) == pdFALSE ) {
					// The buffer could not be sent to the IP task so the buffer must be released.
					vReleaseNetworkBufferAndDescriptor( pxBufferDescriptor );
					// Make a call to the standard trace macro to log the occurrence.
					iptraceETHERNET_RX_EVENT_LOST();
				} else {
					// The message was successfully sent to the TCP/IP stack. Call the standard trace macro to log the occurrence.
					iptraceNETWORK_INTERFACE_RECEIVE();
				}
			} else {
				// The event was lost because a network buffer was not available. Call the standard trace macro to log the occurrence.
				iptraceETHERNET_RX_EVENT_LOST();
			}
		} else {
			osDelay(1);
		}
	}
}


