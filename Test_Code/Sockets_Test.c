Code to be added to the bottom of FreeRTOS_Sockets.c


DHCP test cases

No response to DHCP discover
Invalid reply to DHCP discover
No response to DHCP request
DHCP complete, no response to renewal
DHCP complete, response to renewal gives a different IP address.

#if( ipconfigINCLUDE_TEST_CODE == 1 )

#define ustMAX_CREATED_SOCKETS		30000
#define sockNUM_TEST_ITEMS			50

static void prvSocketTestTask( void *pvParameters );

/* The IP address is declared externally, probably in main(). */
static const uint8_t ucIPAddress[ 4 ] = { ipconfigIP_ADDR0, ipconfigIP_ADDR1, ipconfigIP_ADDR2, ipconfigIP_ADDR3 };

static uint32_t ulErrors = 0UL;

static xSocket_t xCreatedSockets[ ustMAX_CREATED_SOCKETS ];

	configASSERT( FreeRTOS_inet_addr( ( uint8_t * ) "000.000.000.1" ) == FreeRTOS_inet_addr_quick( 0,0,0,1 ) );
	configASSERT( FreeRTOS_inet_addr( ( uint8_t * ) "100.000.000.1" ) == FreeRTOS_inet_addr_quick( 1,0,0,1 ) );
	configASSERT( FreeRTOS_inet_addr( ( uint8_t * ) "000.1.000.1" ) == FreeRTOS_inet_addr_quick( 0,1,0,1 ) );
	configASSERT( FreeRTOS_inet_addr( ( uint8_t * ) "000.100.10.1" ) == FreeRTOS_inet_addr_quick( 0,100,10,1 ) );
	configASSERT( FreeRTOS_inet_addr( ( uint8_t * ) "1000.100.10.1" ) == FreeRTOS_inet_addr_quick( 0,0,0,0 ) );
	configASSERT( FreeRTOS_inet_addr( ( uint8_t * ) "200.500.10.1" ) == FreeRTOS_inet_addr_quick( 0,0,0,0 ) );
	configASSERT( FreeRTOS_inet_addr( ( uint8_t * ) "192.168.0.100" ) == FreeRTOS_inet_addr_quick( 192,168,0,100 ) );
	configASSERT( FreeRTOS_inet_addr( ( uint8_t * ) "192.168.0." ) == FreeRTOS_inet_addr_quick( 0,0,0,0 ) );
	configASSERT( FreeRTOS_inet_addr( ( uint8_t * ) "192.168.0" ) == FreeRTOS_inet_addr_quick( 0,0,0,0 ) );
	configASSERT( FreeRTOS_inet_addr( ( uint8_t * ) "192.168.0001" ) == FreeRTOS_inet_addr_quick( 0,0,0,0 ) );
	configASSERT( FreeRTOS_inet_addr( ( uint8_t * ) "192.168.." ) == FreeRTOS_inet_addr_quick( 0,0,0,0 ) );
	configASSERT( FreeRTOS_inet_addr( ( uint8_t * ) "..168.0.100" ) == FreeRTOS_inet_addr_quick( 0,0,0,0 ) );
	configASSERT( FreeRTOS_inet_addr( ( uint8_t * ) "192..0.100" ) == FreeRTOS_inet_addr_quick( 0,0,0,0 ) );
	configASSERT( FreeRTOS_inet_addr( ( uint8_t * ) "192.168.0.100 " ) == FreeRTOS_inet_addr_quick( 0,0,0,0 ) );
/*-----------------------------------------------------------*/

void vStartSocketTestTask( uint16_t usStackSize, uint32_t ulPort, UBaseType_t uxPriority )
{
	/* Create the tasks. */
	xTaskCreate( prvSocketTestTask, ( const char * const ) "SockTest", usStackSize, ( void * ) ulPort, uxPriority, NULL );
}
/*-----------------------------------------------------------*/

static void prvErrorOccurred( uint32_t ulLine )
{
	( void ) ulLine;
	ulErrors++;
}
/*-----------------------------------------------------------*/

/* This tests parts of the FreeRTOS_sendto() functionality that is not
tested elsewhere. */
static void prvCheckFreeRTOSSendToWithoutFragmentation( void )
{
xSocket_t xSocket;
BaseType_t xBytesSent;
UBaseType_t uxAvailableBuffers, uxIndex;
struct freertos_sockaddr xAddress;
const uint16_t usBoundPort = 5000;
const uint32_t ulBoundIPAddress = 0x87654321;
UBaseType_t uxOriginalPriority;
const char *pcStringToSend1 = "This is the first string that will be sent to the socket.";
const char *pcStringToSend2 = "This is the second string that will be sent to the socket.";
extern QueueHandle_t xNetworkEventQueue;
xIPStackEvent_t xEvent;
xNetworkBufferDescriptor_t *pxNetworkBuffer;
xZeroCopyBufferDescriptor_t xBufferDescriptor;

	/* Check start condition. */
	if( uxQueueMessagesWaiting( xNetworkEventQueue ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}

	xSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );
	configASSERT( xSocket );

	/* Bind the socket as the unbound case has already been tested. */
	xAddress.sin_port = usBoundPort - 1;
	xAddress.sin_addr = ulBoundIPAddress - 1;
	if( FreeRTOS_bind( xSocket, &xAddress, sizeof( xAddress ) ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Change the address in xAddress as it will be used again in calls to
	FreeRTOS_sendto. */
	xAddress.sin_port = usBoundPort;
	xAddress.sin_addr = ulBoundIPAddress;

	{
		/* Try sending more bytes than will fit in the UDP payload with both
		normal and zero copy semantics. */
		xBytesSent = FreeRTOS_sendto( xSocket, pcStringToSend1, ipconfigMAX_UDP_DATA_LENGTH + 1, 0, &xAddress, sizeof( xAddress ) );
		if( xBytesSent != 0 )
		{
			prvErrorOccurred( __LINE__ );
		}
		xBytesSent = FreeRTOS_sendto( xSocket, pcStringToSend1, ipconfigMAX_UDP_DATA_LENGTH + 1, FREERTOS_ZERO_COPY, &xAddress, sizeof( xAddress ) );
		if( xBytesSent != 0 )
		{
			prvErrorOccurred( __LINE__ );
		}
	}

	/* Raise the priority of this task so the IP task cannot process the
	event queue. */
	uxOriginalPriority = uxTaskPriorityGet( NULL );
	vTaskPrioritySet( NULL, configMAX_PRIORITIES - 1 );

	/* Note how many buffers are available so leaks can be detected. */
	uxAvailableBuffers = uxGetNumberOfFreeNetworkBuffers();

	for( uxIndex = 0; uxIndex < uxAvailableBuffers; uxIndex++ )
	{
		/* Send legitimate data, using normal calling semantics. */
		xBytesSent = FreeRTOS_sendto( xSocket, pcStringToSend1, strlen( pcStringToSend1 ), 0, &xAddress, sizeof( xAddress ) );
		if( xBytesSent != ( int32_t ) strlen( pcStringToSend1 ) )
		{
			prvErrorOccurred( __LINE__ );
		}

		/* There should now be one less buffer as the buffer will have been
		queued. */
		if( uxGetNumberOfFreeNetworkBuffers() != ( uxAvailableBuffers - ( uxIndex + 1 ) ) )
		{
			prvErrorOccurred( __LINE__ );
		}

		/* Check the packet was queued. */
		if( uxQueueMessagesWaiting( xNetworkEventQueue ) != ( uxIndex + 1 ) )
		{
			prvErrorOccurred( __LINE__ );
		}
	}

	/* Attempting to send another packet should result in failure. */
	xBytesSent = FreeRTOS_sendto( xSocket, pcStringToSend1, strlen( pcStringToSend1 ), 0, &xAddress, sizeof( xAddress ) );
	if( xBytesSent != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Release all the buffers. */
	for( uxIndex = 0; uxIndex < uxAvailableBuffers; uxIndex++ )
	{
		if( xQueueReceive( xNetworkEventQueue, &xEvent, 0 ) != pdPASS )
		{
			prvErrorOccurred( __LINE__ );
		}
		else
		{
			pxNetworkBuffer = ( xNetworkBufferDescriptor_t * ) xEvent.pvData;

			/* Check the buffer holds the expected data before freeing it. */
			if( memcmp( ( void * ) pcStringToSend1, ( void * ) &( pxNetworkBuffer->pucBuffer[ ipUDP_PAYLOAD_OFFSET_IPv4 ] ), strlen( pcStringToSend1 ) ) != 0 )
			{
				prvErrorOccurred( __LINE__ );
			}

			if( pxNetworkBuffer->ulIPAddress != ulBoundIPAddress )
			{
				prvErrorOccurred( __LINE__ );
			}

			if( pxNetworkBuffer->xDataLength != strlen( pcStringToSend1 ) )
			{
				prvErrorOccurred( __LINE__ );
			}

			vReleaseNetworkBufferAndDescriptor( pxNetworkBuffer );
		}
	}

	/* The number of buffers should now be back to its original value. */
	if( uxGetNumberOfFreeNetworkBuffers() != uxAvailableBuffers )
	{
		prvErrorOccurred( __LINE__ );
	}

	/*-----------------------------------------------------------*/

	for( uxIndex = 0; uxIndex < uxAvailableBuffers; uxIndex++ )
	{
		/* Send legitimate data, using zero copy calling semantics. */
		if( FreeRTOS_GetZeroCopyBuffer( &xBufferDescriptor, strlen( pcStringToSend2 ), 0 ) == pdFALSE )
		{
			prvErrorOccurred( __LINE__ );
		}
		strcpy( ( char * ) xBufferDescriptor.pucBuffer, ( const char * ) pcStringToSend2 );
		xBytesSent = FreeRTOS_sendto( xSocket, &xBufferDescriptor, strlen( pcStringToSend2 ), FREERTOS_ZERO_COPY, &xAddress, sizeof( xAddress ) );
		if( xBytesSent != ( int32_t ) strlen( pcStringToSend2 ) )
		{
			prvErrorOccurred( __LINE__ );
		}

		/* There should now be one less buffer as the buffer will have been
		queued. */
		if( uxGetNumberOfFreeNetworkBuffers() != ( uxAvailableBuffers - ( uxIndex + 1 ) ) )
		{
			prvErrorOccurred( __LINE__ );
		}

		/* Check the packet was queued. */
		if( uxQueueMessagesWaiting( xNetworkEventQueue ) != ( uxIndex + 1 ) )
		{
			prvErrorOccurred( __LINE__ );
		}
	}

	/* Attempting to obtain another zero copy buffer, this should result in
	failure. */
	if( FreeRTOS_GetZeroCopyBuffer( &xBufferDescriptor, 0, 0 ) != pdFALSE )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Release all the buffers. */
	for( uxIndex = 0; uxIndex < uxAvailableBuffers; uxIndex++ )
	{
		if( xQueueReceive( xNetworkEventQueue, &xEvent, 0 ) != pdPASS )
		{
			prvErrorOccurred( __LINE__ );
		}
		else
		{
			pxNetworkBuffer = ( xNetworkBufferDescriptor_t * ) xEvent.pvData;

			/* Check the buffer holds the expected data before freeing it. */
			if( memcmp( ( void * ) pcStringToSend2, ( void * ) &( pxNetworkBuffer->pucBuffer[ ipUDP_PAYLOAD_OFFSET_IPv4 ] ), strlen( pcStringToSend2 ) ) != 0 )
			{
				prvErrorOccurred( __LINE__ );
			}

			if( pxNetworkBuffer->ulIPAddress != ulBoundIPAddress )
			{
				prvErrorOccurred( __LINE__ );
			}

			if( pxNetworkBuffer->xDataLength != strlen( pcStringToSend2 ) )
			{
				prvErrorOccurred( __LINE__ );
			}

			vReleaseNetworkBufferAndDescriptor( pxNetworkBuffer );
		}
	}

	/* The number of buffers should now be back to its original value. */
	if( uxGetNumberOfFreeNetworkBuffers() != uxAvailableBuffers )
	{
		prvErrorOccurred( __LINE__ );
	}

	/*-----------------------------------------------------------*/

	/* Now set the priority to its minimum. */
	vTaskPrioritySet( NULL, tskIDLE_PRIORITY );

	/* Perform the same sequence of operations, except this time the IP
	task should remove the items from the IP event queue and pass the
	buffers to the network interface, where they will get freed. */

	for( uxIndex = 0; uxIndex < uxAvailableBuffers; uxIndex++ )
	{
		/* Send legitimate data, using zero copy calling semantics. */
		if( FreeRTOS_GetZeroCopyBuffer( &xBufferDescriptor, strlen( pcStringToSend2 ), 0 ) == pdFALSE )
		{
			prvErrorOccurred( __LINE__ );
		}
		strcpy( ( char * ) xBufferDescriptor.pucBuffer, ( const char * ) pcStringToSend2 );
		xBytesSent = FreeRTOS_sendto( xSocket, &xBufferDescriptor, strlen( pcStringToSend2 ), FREERTOS_ZERO_COPY, &xAddress, sizeof( xAddress ) );
		if( xBytesSent != ( int32_t ) strlen( pcStringToSend2 ) )
		{
			prvErrorOccurred( __LINE__ );
		}

		/* The IP task should have removed the buffer from the queue
		already as it has the higher priority. */
		if( uxQueueMessagesWaiting( xNetworkEventQueue ) != 0 )
		{
			prvErrorOccurred( __LINE__ );
		}
	}

	/* The number of buffers should now be back to its original value. */
	if( uxGetNumberOfFreeNetworkBuffers() != uxAvailableBuffers )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Reset the priority back to its original value. */
	vTaskPrioritySet( NULL, uxOriginalPriority );
	FreeRTOS_closesocket( xSocket );
}
/*-----------------------------------------------------------*/

static void prvCheckSetSocketOptions( void )
{
FreeRTOS_Socket_t *pxSocket;
TickType_t xBlockTime;

	pxSocket = ( FreeRTOS_Socket_t * ) FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );
	configASSERT( pxSocket );

	/* By default, the socket should be using UDP checksums. */
	if( pxSocket->ucSocketOptions != FREERTOS_SO_UDPCKSUM_OUT )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* By default, the receive timeout should be infinite, and the send block
	time should be zero. */
	if( pxSocket->xReceiveBlockTime != portMAX_DELAY )
	{
		prvErrorOccurred( __LINE__ );
	}

	if( pxSocket->xSendBlockTime != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Ensure setting the send block time is working as expected. */
	xBlockTime = 0x01;
	if( FreeRTOS_setsockopt( ( xSocket_t ) pxSocket, 0, FREERTOS_SO_SNDTIMEO, &xBlockTime, sizeof( xBlockTime ) ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( pxSocket->xSendBlockTime != 0x01 )
	{
		prvErrorOccurred( __LINE__ );
	}
	xBlockTime = 0x00;
	if( FreeRTOS_setsockopt( ( xSocket_t ) pxSocket, 0, FREERTOS_SO_SNDTIMEO, &xBlockTime, sizeof( xBlockTime ) ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( pxSocket->xSendBlockTime != 0x00 )
	{
		prvErrorOccurred( __LINE__ );
	}
	xBlockTime = 0x0f;
	if( FreeRTOS_setsockopt( ( xSocket_t ) pxSocket, 0, FREERTOS_SO_SNDTIMEO, &xBlockTime, sizeof( xBlockTime ) ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( pxSocket->xSendBlockTime != 0x0f )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Note block times should be capped to ipconfigUDP_MAX_SEND_BLOCK_TIME_TICKS. */
	xBlockTime = portMAX_DELAY;
	if( FreeRTOS_setsockopt( ( xSocket_t ) pxSocket, 0, FREERTOS_SO_SNDTIMEO, &xBlockTime, sizeof( xBlockTime ) ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( pxSocket->xSendBlockTime != ipconfigUDP_MAX_SEND_BLOCK_TIME_TICKS )
	{
		prvErrorOccurred( __LINE__ );
	}
	xBlockTime = 0xffff0000;
	if( FreeRTOS_setsockopt( ( xSocket_t ) pxSocket, 0, FREERTOS_SO_SNDTIMEO, &xBlockTime, sizeof( xBlockTime ) ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( pxSocket->xSendBlockTime != ipconfigUDP_MAX_SEND_BLOCK_TIME_TICKS )
	{
		prvErrorOccurred( __LINE__ );
	}
	xBlockTime = ipconfigUDP_MAX_SEND_BLOCK_TIME_TICKS;
	if( FreeRTOS_setsockopt( ( xSocket_t ) pxSocket, 0, FREERTOS_SO_SNDTIMEO, &xBlockTime, sizeof( xBlockTime ) ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( pxSocket->xSendBlockTime != ipconfigUDP_MAX_SEND_BLOCK_TIME_TICKS )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Ensure setting the receive block time is working as expected. */
	xBlockTime = 0x01;
	if( FreeRTOS_setsockopt( ( xSocket_t ) pxSocket, 0, FREERTOS_SO_RCVTIMEO, &xBlockTime, sizeof( xBlockTime ) ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( pxSocket->xReceiveBlockTime != 0x01 )
	{
		prvErrorOccurred( __LINE__ );
	}
	xBlockTime = 0x00;
	if( FreeRTOS_setsockopt( ( xSocket_t ) pxSocket, 0, FREERTOS_SO_RCVTIMEO, &xBlockTime, sizeof( xBlockTime ) ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( pxSocket->xReceiveBlockTime != 0x00 )
	{
		prvErrorOccurred( __LINE__ );
	}
	xBlockTime = portMAX_DELAY;
	if( FreeRTOS_setsockopt( ( xSocket_t ) pxSocket, 0, FREERTOS_SO_RCVTIMEO, &xBlockTime, sizeof( xBlockTime ) ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( pxSocket->xReceiveBlockTime != portMAX_DELAY )
	{
		prvErrorOccurred( __LINE__ );
	}
	xBlockTime = 0xffff0000;
	if( FreeRTOS_setsockopt( ( xSocket_t ) pxSocket, 0, FREERTOS_SO_RCVTIMEO, &xBlockTime, sizeof( xBlockTime ) ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( pxSocket->xReceiveBlockTime != 0xffff0000 )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Ensure the use of the checksum can be turned on and off. */
	if( FreeRTOS_setsockopt( ( xSocket_t ) pxSocket, 0, FREERTOS_SO_UDPCKSUM_OUT, ( void * ) 0, 0 ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( pxSocket->ucSocketOptions != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}

	if( FreeRTOS_setsockopt( ( xSocket_t ) pxSocket, 0, FREERTOS_SO_UDPCKSUM_OUT, ( void * ) 1, 0 ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( pxSocket->ucSocketOptions != FREERTOS_SO_UDPCKSUM_OUT )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Invalid commands should return a non-zero value indicating an error. */
	if( FreeRTOS_setsockopt( ( xSocket_t ) pxSocket, 0, -1, NULL, 0 ) != -FREERTOS_ERRNO_ENOPROTOOPT )
	{
		prvErrorOccurred( __LINE__ );
	}
}
/*-----------------------------------------------------------*/

static void prvEnsureNoRAMLeakageOnSocketCreateAndDelete( void )
{
BaseType_t xNumCreated, xRepetitions, xTotalCreated;
struct freertos_sockaddr xAddress;

	xTotalCreated = 0;

	for( xRepetitions = 0; xRepetitions < 5; xRepetitions++ )
	{
		/* Create sockets until the RAM is exhausted. */
		for( xNumCreated = 0; xNumCreated < ustMAX_CREATED_SOCKETS; xNumCreated++ )
		{
			xCreatedSockets[ xNumCreated ] = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );
			if( xCreatedSockets[ xNumCreated ] == FREERTOS_INVALID_SOCKET )
			{
				break;
			}
			else
			{
				/* Also create the semaphore in the socket by binding. */
				xAddress.sin_port = ( uint16_t ) ( xNumCreated + 1 );
				FreeRTOS_bind( xCreatedSockets[ xNumCreated ], &xAddress, sizeof( xAddress ) );
			}
		}

		if( xTotalCreated == 0 )
		{
			/* Remember how many sockets were created. */
			xTotalCreated = xNumCreated;
		}
		else
		{
			/* Ensure the same number of sockets were created this time as last
			time round. */
			if( xNumCreated != xTotalCreated )
			{
				prvErrorOccurred( __LINE__ );
			}
		}

		/* Delete the sockets again. */
		xNumCreated = 0;
		while( xCreatedSockets[ xNumCreated ] != FREERTOS_INVALID_SOCKET )
		{
			FreeRTOS_closesocket( xCreatedSockets[ xNumCreated ] );
			xNumCreated++;
		}
	}
}
/*-----------------------------------------------------------*/

static void prvCheckQueueingPacketsOnBoundSockets( void )
{
UBaseType_t uxIndex, uxExpected;
BaseType_t xReturned;
struct freertos_sockaddr xAddress;
xSocket_t xAdditionalSocket;
xNetworkBufferDescriptor_t *pxNetworkBuffer;
FreeRTOS_Socket_t *pxSocket;

	for( uxIndex = 0; uxIndex < sockNUM_TEST_ITEMS; uxIndex++ )
	{
		/* Create the test sockets. */
		xCreatedSockets[ uxIndex ] = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );

		/* Bind the sockets.  Binding to zero should not be possible. */
		xAddress.sin_port = ( uint16_t ) uxIndex;
		xReturned = FreeRTOS_bind( xCreatedSockets[ uxIndex ], &xAddress, sizeof( xAddress ) );

		if( uxIndex == 0 )
		{
			if( xReturned != -FREERTOS_ERRNO_EADDRNOTAVAIL )
			{
				prvErrorOccurred( __LINE__ );
			}
		}
		else
		{
			if( xReturned != 0 )
			{
				prvErrorOccurred( __LINE__ );
			}
		}
	}

	/* Create another socket to try and bind to an already bound address. */
	xAdditionalSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );
	xAddress.sin_port = sockNUM_TEST_ITEMS >> 2;
	if( FreeRTOS_bind( xAdditionalSocket, &xAddress, sizeof( xAddress ) ) != -FREERTOS_ERRNO_EADDRINUSE )
	{
		prvErrorOccurred( __LINE__ );
	}
	FreeRTOS_closesocket( xAdditionalSocket );

	xReturned = ( BaseType_t ) uxGetNumberOfFreeNetworkBuffers();

	/* Send two network buffers to the socket bound to port 1. */
	pxNetworkBuffer = pxGetNetworkBufferWithDescriptor( 0, 0 );
	configASSERT( pxNetworkBuffer );
	if( xProcessReceivedUDPPacket( pxNetworkBuffer, 1 ) != pdPASS )
	{
		prvErrorOccurred( __LINE__ );
	}

	pxNetworkBuffer = pxGetNetworkBufferWithDescriptor( 0, 0 );
	configASSERT( pxNetworkBuffer );
	if( xProcessReceivedUDPPacket( pxNetworkBuffer, 1 ) != pdPASS )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Send three network buffers to the socket bound to port
	sockNUM_TEST_ITEMS - 1. */
	pxNetworkBuffer = pxGetNetworkBufferWithDescriptor( 0, 0 );
	configASSERT( pxNetworkBuffer );
	if( xProcessReceivedUDPPacket( pxNetworkBuffer, sockNUM_TEST_ITEMS - 1 ) != pdPASS )
	{
		prvErrorOccurred( __LINE__ );
	}

	pxNetworkBuffer = pxGetNetworkBufferWithDescriptor( 0, 0 );
	configASSERT( pxNetworkBuffer );
	if( xProcessReceivedUDPPacket( pxNetworkBuffer, sockNUM_TEST_ITEMS - 1 ) != pdPASS )
	{
		prvErrorOccurred( __LINE__ );
	}

	pxNetworkBuffer = pxGetNetworkBufferWithDescriptor( 0, 0 );
	configASSERT( pxNetworkBuffer );
	if( xProcessReceivedUDPPacket( pxNetworkBuffer, sockNUM_TEST_ITEMS - 1 ) != pdPASS )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Send a network buffer to the socket bound to port 10. */
	pxNetworkBuffer = pxGetNetworkBufferWithDescriptor( 0, 0 );
	configASSERT( pxNetworkBuffer );
	if( xProcessReceivedUDPPacket( pxNetworkBuffer, 10 ) != pdPASS )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Try sneding to a port that does not have a socket bound to it. */
	pxNetworkBuffer = pxGetNetworkBufferWithDescriptor( 0, 0 );
	configASSERT( pxNetworkBuffer );
	if( xProcessReceivedUDPPacket( pxNetworkBuffer, sockNUM_TEST_ITEMS ) != pdFAIL )
	{
		prvErrorOccurred( __LINE__ );
	}
	vReleaseNetworkBufferAndDescriptor( pxNetworkBuffer );

	/* Now determine if all the sockets are in their expected states. */
	for( uxIndex = 0; uxIndex < sockNUM_TEST_ITEMS; uxIndex++ )
	{
		switch( uxIndex )
		{
			case 1:
				/* Expect to have 2 packets queued on the socket. */
				uxExpected = 2;
				break;

			case sockNUM_TEST_ITEMS - 1:
				/* Expect to have 3 packets queued on the socket. */
				uxExpected = 3;
				break;

			case 10:
				/* Expect to have 1 packet queued on the socket. */
				uxExpected = 1;
				break;

			default:
				/* Expect to have no packets queued on the socket. */
				uxExpected = 0;
				break;
		}

		pxSocket = ( FreeRTOS_Socket_t * ) xCreatedSockets[ uxIndex ];
		if( listCURRENT_LIST_LENGTH( &( pxSocket->u.xUdp.xWaitingPacketsList ) ) != uxExpected )
		{
			prvErrorOccurred( __LINE__ );
		}

		/* Socket 0 was never bound. */
		if( uxIndex == 0 )
		{
//			if( pxSocket->xWaitingPacketSemaphore != NULL )
			if( listLIST_ITEM_CONTAINER( & ( pxSocket )->xBoundSocketListItem ) == NULL )
			{
				prvErrorOccurred( __LINE__ );
			}
		}
//		else
//		{
//			if( uxQueueMessagesWaiting( pxSocket->xWaitingPacketSemaphore ) != uxExpected )
//			{
//				prvErrorOccurred( __LINE__ );
//			}
//		}

		/* Free the resources. */
		FreeRTOS_closesocket( xCreatedSockets[ uxIndex ] );
	}

	/* Ensure all the packet buffers have been freed, even the ones that were
	queued on sockets. */
	if( ( BaseType_t ) uxGetNumberOfFreeNetworkBuffers() != xReturned )
	{
		prvErrorOccurred( __LINE__ );
	}
}
/*-----------------------------------------------------------*/

static void prvCheckFindingListItemWithValue( void )
{
List_t xTestList;
ListItem_t xTestListItems[ sockNUM_TEST_ITEMS ], *pxFoundListItem;
BaseType_t xIndex;

	/* Initialise all items. */
	vListInitialise( &xTestList );

	for( xIndex = 0; xIndex < sockNUM_TEST_ITEMS; xIndex++ )
	{
		vListInitialiseItem( &( xTestListItems[ xIndex ] ) );
		listSET_LIST_ITEM_VALUE( &( xTestListItems[ xIndex ] ), xIndex );
		vListInsert( &xTestList, &( xTestListItems[ xIndex ] ) );
	}

	/* The values of the list items are equal to their position in the list. */
	for( xIndex = 0; xIndex < sockNUM_TEST_ITEMS; xIndex++ )
	{
		pxFoundListItem = pxListFindListItemWithValue( &xTestList, xIndex );

		if( pxFoundListItem != &( xTestListItems[ xIndex ] ) )
		{
			prvErrorOccurred( __LINE__ );
		}
	}

	/* Try to find some values that are not in the list. */
	if( pxListFindListItemWithValue( &xTestList, sockNUM_TEST_ITEMS ) != NULL )
	{
		prvErrorOccurred( __LINE__ );
	}

	if( pxListFindListItemWithValue( &xTestList, portMAX_DELAY ) != NULL )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Try first and last items without walking the values in between, and
	something in the middle for good measure. */
	if( pxListFindListItemWithValue( &xTestList, 0 ) != &xTestListItems[ 0 ] )
	{
		prvErrorOccurred( __LINE__ );
	}

	if( pxListFindListItemWithValue( &xTestList, sockNUM_TEST_ITEMS - 1 ) != &xTestListItems[ sockNUM_TEST_ITEMS - 1 ] )
	{
		prvErrorOccurred( __LINE__ );
	}

	if( pxListFindListItemWithValue( &xTestList, 10 ) != &xTestListItems[ 10 ] )
	{
		prvErrorOccurred( __LINE__ );
	}
}
/*-----------------------------------------------------------*/

static void prvCheckAutomaticBinding( void )
{
const uint16_t usFirstPortNumber = socketAUTO_PORT_ALLOCATION_START_NUMBER, usFirstWrapPortNumber = socketAUTO_PORT_ALLOCATION_RESET_NUMBER;
uint16_t usExpectedPortNumber;
TickType_t xTicksToBlock = 0, xTimeBefore, xTimeNow;
struct freertos_sockaddr xAddress;
BaseType_t xBytesSent;
uint32_t ul;

	usExpectedPortNumber = usFirstPortNumber;

	for( ul = 0; ul < ( uint32_t ) ( usFirstPortNumber * 3U ); ul++ )
	{
		/* Create a socket. */
		xCreatedSockets[ 0 ] = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );

		if( xCreatedSockets[ 0 ] == FREERTOS_INVALID_SOCKET )
		{
			prvErrorOccurred( __LINE__ );
			break;
		}

		/* Set to non-blocking. */
		FreeRTOS_setsockopt( xCreatedSockets[ 0 ], 0, FREERTOS_SO_RCVTIMEO, &xTicksToBlock, 0 );

		/* Try sending without first binding. */
		xTimeBefore = xTaskGetTickCount();
		xBytesSent = FreeRTOS_sendto( xCreatedSockets[ 0 ], &usExpectedPortNumber, 0, 0, &xAddress, 0 );
		xTimeNow = xTaskGetTickCount();

		if( ( xTimeNow - xTimeBefore ) > 1 )
		{
			/* Should not have blocked. */
			prvErrorOccurred( __LINE__ );
		}

		/* Should not have sent any bytes. */
		if( xBytesSent != 0 )
		{
			prvErrorOccurred( __LINE__ );
		}

		/* Check the bound port is as expected. */
		if( socketGET_SOCKET_ADDRESS( ( ( FreeRTOS_Socket_t * ) xCreatedSockets[ 0 ] ) ) != FreeRTOS_htons( usExpectedPortNumber ) )
		{
			prvErrorOccurred( __LINE__ );
		}

		usExpectedPortNumber++;
		if( usExpectedPortNumber == 0 )
		{
			usExpectedPortNumber = usFirstWrapPortNumber;
		}

		/* Attempting to bind the socket manually now should result in an error. */
		xAddress.sin_port = 10;
		if( FreeRTOS_bind( xCreatedSockets[ 0 ], &xAddress, sizeof( xAddress ) ) != FREERTOS_EINVAL )
		{
			prvErrorOccurred( __LINE__ );
		}

		FreeRTOS_closesocket( xCreatedSockets[ 0 ] );
	}
}
/*-----------------------------------------------------------*/

static xNetworkBufferDescriptor_t *prvQueueNetworkBufferOnSocket( xSocket_t xSocket, const char *pcString, struct freertos_sockaddr *pxAddress, uint32_t uxAvailableBuffers )
{
extern QueueHandle_t xNetworkEventQueue;
xNetworkBufferDescriptor_t *pxNetworkBuffer = NULL;
xIPStackEvent_t xEvent;

	/* Use FreeRTOS_sendto() to create a network buffer. */
	if( FreeRTOS_sendto( xSocket, ( void * ) pcString, strlen( pcString ), 0, pxAddress, sizeof( *pxAddress ) ) != ( int32_t ) strlen( pcString ) )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* There should now be one fewer network buffer. */
	if( uxGetNumberOfFreeNetworkBuffers() != ( uxAvailableBuffers - 1 ) )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Obtain the created network buffer from the network event queue. */
	if( xQueueReceive( xNetworkEventQueue, &xEvent, 0 ) != pdPASS )
	{
		prvErrorOccurred( __LINE__ );
	}
	else
	{
		pxNetworkBuffer = ( xNetworkBufferDescriptor_t * ) xEvent.pvData;
	}

	/* Queue the network buffer on the socket. */
	xProcessReceivedUDPPacket( pxNetworkBuffer, pxAddress->sin_port );

	return pxNetworkBuffer;
}
/*-----------------------------------------------------------*/

static void prvCheckReceiveFrom( void )
{
xSocket_t xSocket;
const uint16_t usBoundPort = 10001U;
const uint32_t ulIPAddress = 0x11223344UL;
struct freertos_sockaddr xAddress;
const char *pcString = "String for recvfrom() test";
UBaseType_t uxOriginalPriority;
xNetworkBufferDescriptor_t *pxNetworkBuffer;
extern QueueHandle_t xNetworkEventQueue;
int32_t iReturned;
uint8_t ucReceiveBuffer[ ipconfigMAX_UDP_DATA_LENGTH ];
socklen_t xAddressSize;
TickType_t xBlockTime = 0;
uint32_t uxAvailableBuffers;
xZeroCopyBufferDescriptor_t xBufferDescriptor;

	/* Remember how many buffers are available at the start. */
	uxAvailableBuffers = uxGetNumberOfFreeNetworkBuffers();

	/* Raise the priority of this task so the IP task cannot process the
	event queue. */
	uxOriginalPriority = uxTaskPriorityGet( NULL );
	vTaskPrioritySet( NULL, configMAX_PRIORITIES - 1 );

	/* Create the socket to use in this test. */
	xSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );
	configASSERT( xSocket );

	/* An attempt to read from the socket before it is bound should return the
	appropriate error code. */
	iReturned = FreeRTOS_recvfrom( xSocket, ( void * ) ucReceiveBuffer, ipconfigMAX_UDP_DATA_LENGTH, 0, &xAddress, &xAddressSize );
	if( iReturned != FREERTOS_EINVAL )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Bind the socket so it can receive data. */
	xAddress.sin_port = usBoundPort;
	xAddress.sin_addr = ulIPAddress;
	if( FreeRTOS_bind( xSocket, &xAddress, sizeof( xAddress ) ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Ensure there is no block time. */
	if( FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_RCVTIMEO, &xBlockTime, sizeof( TickType_t ) ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Attempting a read now should just result in a timeout. */
	iReturned = FreeRTOS_recvfrom( xSocket, ( void * ) ucReceiveBuffer, ipconfigMAX_UDP_DATA_LENGTH, 0, &xAddress, &xAddressSize );
	if( iReturned != -FREERTOS_ERRNO_EWOULDBLOCK )
	{
		prvErrorOccurred( __LINE__ );
	}

	pxNetworkBuffer = prvQueueNetworkBufferOnSocket( xSocket, pcString, &xAddress, uxAvailableBuffers );

	/* Receiving now should result in the data being received, and the network
	buffer being returned to the free list. */
	memset( ucReceiveBuffer, 0x00, sizeof( ucReceiveBuffer ) );
	xAddress.sin_addr = 0;
	xAddress.sin_port = 0;
	iReturned = FreeRTOS_recvfrom( xSocket, ( void * ) ucReceiveBuffer, ipconfigMAX_UDP_DATA_LENGTH, 0, &xAddress, &xAddressSize );
	if( iReturned != ( int32_t ) strlen( pcString ) )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( strcmp( ( const char * ) ucReceiveBuffer, pcString ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( uxGetNumberOfFreeNetworkBuffers() != uxAvailableBuffers )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( xAddress.sin_addr != ulIPAddress )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( xAddress.sin_port != usBoundPort )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Do it again, but this time call recvfrom will a NULL xAddress parameter. */
	pxNetworkBuffer = prvQueueNetworkBufferOnSocket( xSocket, pcString, &xAddress, uxAvailableBuffers );
	memset( ucReceiveBuffer, 0x00, sizeof( ucReceiveBuffer ) );
	xAddress.sin_addr = 0;
	xAddress.sin_port = 0;
	iReturned = FreeRTOS_recvfrom( xSocket, ( void * ) ucReceiveBuffer, ipconfigMAX_UDP_DATA_LENGTH, 0, NULL, &xAddressSize );
	if( iReturned != ( int32_t ) strlen( pcString ) )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( strcmp( ( const char * ) ucReceiveBuffer, pcString ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( uxGetNumberOfFreeNetworkBuffers() != uxAvailableBuffers )
	{
		prvErrorOccurred( __LINE__ );
	}
	/* Note xAddress should not have changed this time. */
	if( xAddress.sin_addr != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( xAddress.sin_port != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Do it again, but this time change the network buffer so it contains
	too much data to fit in the receive buffer. */
	xAddress.sin_addr = ulIPAddress;
	xAddress.sin_port = usBoundPort;
	pxNetworkBuffer = prvQueueNetworkBufferOnSocket( xSocket, pcString, &xAddress, uxAvailableBuffers );
	pxNetworkBuffer->xDataLength = ipconfigMAX_UDP_DATA_LENGTH * 5;
	memset( ucReceiveBuffer, 0x00, sizeof( ucReceiveBuffer ) );
	iReturned = FreeRTOS_recvfrom( xSocket, ( void * ) ucReceiveBuffer, ipconfigMAX_UDP_DATA_LENGTH, 0, NULL, &xAddressSize );
	if( iReturned != ipconfigMAX_UDP_DATA_LENGTH )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( strncmp( ( const char * ) ucReceiveBuffer, pcString, strlen( pcString ) ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( uxGetNumberOfFreeNetworkBuffers() != uxAvailableBuffers )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* Do it again, but this time receive using zero copy semantics. */
	pxNetworkBuffer = prvQueueNetworkBufferOnSocket( xSocket, pcString, &xAddress, uxAvailableBuffers );
	iReturned = FreeRTOS_recvfrom( xSocket, ( void * ) &xBufferDescriptor, 0, FREERTOS_ZERO_COPY, NULL, &xAddressSize );
	if( iReturned != ( int32_t ) strlen( pcString ) )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( xBufferDescriptor.pvPrivate != pxNetworkBuffer )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( strncmp( ( const char * ) xBufferDescriptor.pucBuffer, pcString, strlen( pcString ) ) != 0 )
	{
		prvErrorOccurred( __LINE__ );
	}
	if( xBufferDescriptor.xBufferLengthBytes != ipconfigMAX_UDP_DATA_LENGTH )
	{
		prvErrorOccurred( __LINE__ );
	}

	/* The buffer should not have been freed yet. */
	if( uxGetNumberOfFreeNetworkBuffers() != ( uxAvailableBuffers - 1 ) )
	{
		prvErrorOccurred( __LINE__ );
	}


	/* Free the buffer manuall. */
	vReleaseNetworkBufferAndDescriptor( pxNetworkBuffer );
	if( uxGetNumberOfFreeNetworkBuffers() != uxAvailableBuffers )
	{
		prvErrorOccurred( __LINE__ );
	}


	/* Free up the socket. */
	FreeRTOS_closesocket( xSocket );

	/* Set the task priority back down to its original priority. */
	vTaskPrioritySet( NULL, uxOriginalPriority );
}
/*-----------------------------------------------------------*/

static void prvSocketTestTask( void *pvParameters )
{
	/* Remove compiler warning about unused parameters. */
	( void ) pvParameters;

	prvCheckReceiveFrom();

	prvCheckFreeRTOSSendToWithoutFragmentation();
	prvCheckSetSocketOptions();
	prvCheckQueueingPacketsOnBoundSockets();
	prvCheckFindingListItemWithValue();
	prvCheckAutomaticBinding();

	/* Run this last as it fragments the memory. */
	prvEnsureNoRAMLeakageOnSocketCreateAndDelete();

	vTaskSuspend( NULL );
}
/*-----------------------------------------------------------*/


#endif /* ipconfigINCLUDE_TEST_CODE */


