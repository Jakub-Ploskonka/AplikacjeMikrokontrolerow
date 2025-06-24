#include <stdint.h>
#include "cmsis_os2.h"
#include <FreeRTOS.h>
#include <FreeRTOS_IP.h>
#include <FreeRTOS_Sockets.h>
#include <enc28j60_net_interface.h>
#include <enc28j60_eth_phy.h>
#include "amcom.h"
#include "game.h"

void NET_APPThread(void* arg) {

	// Set the IP address and port of the server
	struct freertos_sockaddr serverAddress;
	memset( &serverAddress, 0, sizeof(serverAddress) );
	serverAddress.sin_port = FreeRTOS_htons( 2001 );
	serverAddress.sin_address.ulIP_IPv4 = FreeRTOS_inet_addr_quick( 192, 168, 40, 1 );
	serverAddress.sin_family = FREERTOS_AF_INET4;

	osDelay(1000);

	while (1) {
		// Create a socket
		Socket_t socket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
		configASSERT( socket != FREERTOS_INVALID_SOCKET );

		// Connect to the remote socket.
		if( FreeRTOS_connect( socket, &serverAddress, sizeof( serverAddress ) ) == 0 )
		{
			printf("Connected to server!\n");

			static AMCOM_Receiver amcomReceiver;		// AMCOM receiver structure
			static char buf[512];						// buffer for incoming data
			int receivedBytesCount;				        // holds the number of bytes received via socket

			// Initialize AMCOM receiver
			AMCOM_InitReceiver(&amcomReceiver, amcomPacketHandler, (void*)socket);

			// Receive data from socket until the peer shuts down the connection
			do {
				// Fetch received bytes from socket into buf
				receivedBytesCount = FreeRTOS_recv(socket, buf, sizeof(buf), 0);
				if (receivedBytesCount > 0) {
					printf("Received %d bytes in socket\n", receivedBytesCount);
					// Try to deserialize the incoming data
					AMCOM_Deserialize(&amcomReceiver, buf, receivedBytesCount);
				} else if (receivedBytesCount < 0) {
					// Negative result indicates that there was socket communication error
					// Initiate graceful shutdown
					FreeRTOS_shutdown( socket, FREERTOS_SHUT_RDWR );
					break;
				}
			} while (receivedBytesCount > 0);
		}

		/* The socket has shut down and is safe to close. */
		FreeRTOS_closesocket( socket );
	}
}

