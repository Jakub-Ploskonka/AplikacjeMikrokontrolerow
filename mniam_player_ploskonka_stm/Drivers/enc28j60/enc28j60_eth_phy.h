/*
 * enc28j60_eth_phy.h
 *
 *  Created on: Mar 31, 2025
 *      Author: kf
 */

#ifndef ENC28J60_ENC28J60_ETH_PHY_H_
#define ENC28J60_ENC28J60_ETH_PHY_H_

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef __clang__
//  #define __rbit(v)     __builtin_arm_rbit(v)
  #pragma clang diagnostic ignored "-Wpadded"
  #pragma clang diagnostic ignored "-Wconversion"
  #pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include "enc28j60.h"
#include "stm32f4xx_hal.h"

void softReset(void);
void writeOperation(uint8_t operation, uint8_t address, uint8_t data);
uint8_t readOperation(uint8_t operation, uint8_t address);
void setBank(uint8_t address);
uint8_t readControlRegister (uint8_t reg);
void writeControlRegister8 (uint8_t reg, uint8_t value);
void writeControlRegister16(uint8_t reg, uint16_t value);



uint16_t readPhyRegister(uint8_t address);

void writePhyRegister(uint8_t address, uint16_t value);

void writeBuffer(const uint8_t* data, uint32_t len);
void readBuffer(uint16_t len, uint8_t* data);
uint16_t readBufferWord();
uint32_t readId(void);


/**
  \fn          int32_t Initialize (ARM_ETH_MAC_SignalEvent_t cb_event)
  \brief       Initialize Ethernet MAC Device.
  \param[in]   cb_event  Pointer to \ref ARM_ETH_MAC_SignalEvent
  \return      \ref execution_status
*/
void MAC_Initialize (void* cb_event);


/**
  \fn          int32_t SendFrame (const uint8_t *frame, uint32_t len, uint32_t flags)
  \brief       Send Ethernet frame.
  \param[in]   frame  Pointer to frame buffer with data to send
  \param[in]   len    Frame buffer length in bytes
  \param[in]   flags  Frame transmit flags (see ARM_ETH_MAC_TX_FRAME_...)
  \return      \ref execution_status
*/
bool MAC_SendFrame (const uint8_t *frame, uint32_t len, uint32_t flags);

/**
  \fn          int32_t ReadFrame (uint8_t *frame, uint32_t len)
  \brief       Read data of received Ethernet frame.
  \param[in]   frame  Pointer to frame buffer for data to read into
  \param[in]   len    Frame buffer length in bytes
  \return      number of data bytes read or execution status
                 - value >= 0: number of data bytes read
                 - value < 0: error occurred, value is execution status as defined with \ref execution_status
*/
int32_t MAC_ReadFrame (uint8_t *frame, uint32_t maxlen);

/**
  \fn          uint32_t GetRxFrameSize (void)
  \brief       Get size of received Ethernet frame.
  \return      number of bytes in received frame
*/
uint32_t MAC_GetRxFrameSize (void);


void MAC_SetMACAddress(uint8_t* macAddr);

/**
  \fn          ARM_ETH_LINK_INFO GetLinkInfo (void)
  \brief       Get Ethernet PHY Device Link information.
  \return      current link parameters \ref ARM_ETH_LINK_INFO
*/
bool PHY_IsLinkUp (void);


#endif /* ENC28J60_ENC28J60_ETH_PHY_H_ */
