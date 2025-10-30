#ifndef _LOOPBACK_H_
#define _LOOPBACK_H_

#include <stdint.h>
#ifdef BSERIES_EN // only includes once it is in a B-series project
#include "../../../../../../include/custom_cmd.h"
#endif

/* Loopback test debug message printout enable */
//#define	_LOOPBACK_DEBUG_

#ifndef count_of
#define count_of(a) (sizeof(a)/sizeof((a)[0])) //! was out of scope here and I needed it in loopback_udps()
#endif

/* DATA_BUF_SIZE define for Loopback example */
#ifndef DATA_BUF_SIZE
	#define DATA_BUF_SIZE			2048
#endif

/* MUST BE DEFINED HERE TO BE SEEN BY wiznet_app.c */
int32_t loopback_udps(uint8_t sn, uint8_t* buf, uint16_t port, uint8_t loopback_mode);
int32_t loopback_tcps(uint8_t sn, uint8_t* buf, uint16_t port, uint8_t loopback_mode);
int32_t loopback_tcpc(uint8_t sn, uint8_t* buf, uint8_t* destip, uint16_t destport, uint8_t loopback_mode);
int32_t custom_tcps(uint8_t sn, uint8_t* buf, uint16_t port, uint8_t loopback_mode);

#endif
/**
 * @file loopback.h
 * @brief Header file for network loopback functionality
 *
 * This file contains declarations for loopback testing functions including TCP server/client
 * and UDP server implementations. It also defines buffer sizes and debug options.
 *
 * @note The DATA_BUF_SIZE can be redefined but defaults to 2048 bytes
 *
 * @defgroup loopback Network Loopback
 * @{
 *
 * @def DATA_BUF_SIZE 
 * Default buffer size for loopback data (2048 bytes)
 *
 * @fn int32_t loopback_udps(uint8_t sn, uint8_t* buf, uint16_t port, uint8_t loopback_mode)
 * @brief UDP Server loopback function
 * @param sn Socket number
 * @param buf Buffer for data
 * @param port UDP port number
 * @param loopback_mode Loopback operation mode
 * @return Status code
 *
 * @fn int32_t loopback_tcps(uint8_t sn, uint8_t* buf, uint16_t port, uint8_t loopback_mode)
 * @brief TCP Server loopback function
 * @param sn Socket number
 * @param buf Buffer for data
 * @param port TCP port number
 * @param loopback_mode Loopback operation mode
 * @return Status code
 *
 * @fn int32_t loopback_tcpc(uint8_t sn, uint8_t* buf, uint8_t* destip, uint16_t destport, uint8_t loopback_mode)
 * @brief TCP Client loopback function
 * @param sn Socket number
 * @param buf Buffer for data
 * @param destip Destination IP address
 * @param destport Destination port number
 * @param loopback_mode Loopback operation mode
 * @return Status code
 *
 * @fn int32_t custom_tcps(uint8_t sn, uint8_t* buf, uint16_t port, uint8_t loopback_mode)
 * @brief Custom TCP Server implementation
 * @param sn Socket number
 * @param buf Buffer for data
 * @param port TCP port number
 * @param loopback_mode Operation mode
 * @return Status code
 *
 * @}
 */