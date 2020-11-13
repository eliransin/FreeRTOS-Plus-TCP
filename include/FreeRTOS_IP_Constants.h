#ifndef FREERTOS_IP_CONSTANTS_H
#define FREERTOS_IP_CONSTANTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Some constants defining the sizes of several parts of a packet */
#define ipSIZE_OF_ETH_HEADER			14u
#define ipSIZE_OF_IP_HEADER_IPv4		20u
#define ipSIZE_OF_IP_HEADER_IPv6		40u
#define ipSIZE_OF_IGMP_HEADER			8u
#define ipSIZE_OF_ICMP_HEADER			8u
#define ipSIZE_OF_UDP_HEADER			8u
#define ipSIZE_OF_TCP_HEADER			20u


/* The number of octets in the MAC and IP addresses respectively. */
#define ipMAC_ADDRESS_LENGTH_BYTES ( 6 )
#define ipIP_ADDRESS_LENGTH_BYTES ( 4 )

/* IP protocol definitions. */
#define ipPROTOCOL_ICMP			( 1 )
#define ipPROTOCOL_IGMP         ( 2 )
#define ipPROTOCOL_TCP			( 6 )
#define ipPROTOCOL_UDP			( 17 )

#define ipPROTOCOL_ICMP_IPv6	( 58u )

#define ipTYPE_IPv4				( 0x40u )
#define ipTYPE_IPv6				( 0x60u )

/* Some IPv6 ICMP requests. */
#define ipICMP_PING_REQUEST_IPv6			( ( uint8_t ) 128 )
#define ipICMP_PING_REPLY_IPv6				( ( uint8_t ) 129 )
#define ipICMP_NEIGHBOR_SOLICITATION_IPv6	( ( uint8_t ) 135 )
#define ipICMP_NEIGHBOR_ADVERTISEMENT_IPv6	( ( uint8_t ) 136 )

/* Dimensions the buffers that are filled by received Ethernet frames. */
#define ipSIZE_OF_ETH_CRC_BYTES					( 4UL )
#define ipSIZE_OF_ETH_OPTIONAL_802_1Q_TAG_BYTES	( 4UL )

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* FREERTOS_IP_CONSTANTS_H */
