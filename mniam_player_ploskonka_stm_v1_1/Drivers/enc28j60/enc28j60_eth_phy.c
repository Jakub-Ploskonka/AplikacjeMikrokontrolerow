/*
 * Copyright (c) AGH UST. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * -----------------------------------------------------------------------
 *
 * $Date:        25. March 2022
 * $Revision:    V6.7
 *
 * Driver:       Driver_ETH_MACn (default: Driver_ETH_MAC0),
                 Driver_ETH_PHYn (default: Driver_ETH_PHY0)
 * Project:      Ethernet Media Access (MAC) Driver and
                 Ethernet Physical Layer Transceiver (PHY) Driver
                 for ENC28J60
 * -----------------------------------------------------------------------
 * Use the following configuration settings in the middleware component
 * to connect to this driver.
 *
 *   Configuration Setting                     Value
 *   ---------------------                     -----
 *   Connect to hardware via Driver_ETH_MAC# = n (default: 0)
 *                           Driver_ETH_PHY# = n (default: 0)
 * -------------------------------------------------------------------- */


#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stm32f4xx_hal.h>
#include <main.h>

#ifdef __clang__
//  #define __rbit(v)     __builtin_arm_rbit(v)
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include "enc28j60.h"
#include "enc28j60_eth_phy.h"
#include "stm32f4xx_hal.h"

extern SPI_HandleTypeDef hspi4;
extern TIM_HandleTypeDef htim14;
static const TIM_HandleTypeDef* tim_handle = &htim14;

static inline void delay_us(uint32_t us) {
	__HAL_TIM_SetCounter(tim_handle, 0);
	volatile uint16_t count_to = us;
	volatile uint16_t counter = 0;
	while (1) {
		counter = __HAL_TIM_GetCounter(tim_handle);
		if(counter >= count_to) {
			break;
		}
	}
}

static inline void SPI_CS_Assert(void) {
	HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_RESET);
}

static inline void SPI_CS_Deassert(void) {
	HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_SET);
	delay_us(1);
}


static uint16_t gNextPacketPtr;
static uint8_t  erxfcon;
static uint8_t  macaddr[6] = {0xaa, 0xbb, 0xcc, 0xee, 0x11, 0x22};


void softReset(void) {
	uint8_t cmd = 0xFF;
	//  ptrSPI->Control (ARM_SPI_CONTROL_SS, ARM_SPI_SS_ACTIVE)
	SPI_CS_Assert();
	;
	HAL_SPI_Transmit(&hspi4, &cmd, 1, HAL_MAX_DELAY);

	SPI_CS_Deassert();
}



void writeOperation(uint8_t operation, uint8_t address, uint8_t data) {
	uint8_t out[2];
	out[0] = operation | (address & ADDR_MASK);
	out[1] = data;

	SPI_CS_Assert();
	//  ptrSPI->Send (&out, 2);
	HAL_SPI_Transmit(&hspi4, out, 2, HAL_MAX_DELAY);

	while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY) { // may be BUSY_TX as well
		;
	}

	SPI_CS_Deassert();
	delay_us(1);
}

uint8_t readOperation(uint8_t operation, uint8_t address) {
	uint8_t result;
	uint8_t out = (operation | (address & ADDR_MASK));
	uint8_t in[2];

	SPI_CS_Assert();

	// issue read command
	if (address & 0x80) {
		HAL_SPI_Transmit(&hspi4, &out, 1, HAL_MAX_DELAY);
		while (HAL_SPI_GetState(&hspi4) == HAL_SPI_STATE_BUSY_TX) {
			;
		}
		HAL_SPI_Receive(&hspi4, in, 2, HAL_MAX_DELAY);
		while (HAL_SPI_GetState(&hspi4) == HAL_SPI_STATE_BUSY_TX) {
			;
		}
		result = in[1];
	} else{
		HAL_SPI_Transmit(&hspi4, &out, 1, HAL_MAX_DELAY);
		while (HAL_SPI_GetState(&hspi4) == HAL_SPI_STATE_BUSY_TX) {
			;
		}
		HAL_SPI_Receive(&hspi4, in, 1, HAL_MAX_DELAY);
		while (HAL_SPI_GetState(&hspi4) == HAL_SPI_STATE_BUSY_TX) {
			;
		}
		result = in[0];
	}
	SPI_CS_Deassert();
	delay_us(1);
	return result;
}

void setBank(uint8_t address) {
	static int currentBank = 0;
	// set the bank (if needed)
	if ((address & BANK_MASK) != currentBank) {
		// set the bank
		writeOperation(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1 | ECON1_BSEL0));
		writeOperation(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK) >> 5);
		currentBank = (address & BANK_MASK);
	}
}

uint8_t readControlRegister (uint8_t reg) {

	// set the bank (if needed)
	setBank(reg);
	// do the read
	return readOperation(ENC28J60_READ_CTRL_REG, reg);
}

void writeControlRegister8 (uint8_t reg, uint8_t value) {
	// set the bank (if needed)
	setBank(reg);
	// do the write
	writeOperation(ENC28J60_WRITE_CTRL_REG, reg, value);
}

void writeControlRegister16(uint8_t reg, uint16_t value) {
	// set the bank (if needed)
	setBank(reg);
	// do the write
	writeOperation(ENC28J60_WRITE_CTRL_REG, reg, value & 0xff);
	writeOperation(ENC28J60_WRITE_CTRL_REG, reg+1, (uint8_t)(value >> 8));
}


uint16_t readPhyRegister(uint8_t address) {
	uint16_t value = 0xFFFF;

	writeControlRegister8(MIREGADR, address);
	writeControlRegister8(MICMD, MICMD_MIIRD);
	// wait until the PHY read completes
	while (readControlRegister(MISTAT) & MISTAT_BUSY) {
		;
	}
	writeControlRegister8(MICMD, 0);
	value = (readControlRegister(MIRDL) | readControlRegister(MIRDH) << 8);
	return value;
}

void writePhyRegister(uint8_t address, uint16_t value) {
	// set the PHY register address
	writeControlRegister8(MIREGADR, address);
	// write the PHY data
	writeControlRegister16(MIWRL, value);
	// wait until the PHY write completes
	while (readControlRegister(MISTAT) & MISTAT_BUSY) {
		;
	}
}

void writeBuffer(const uint8_t* data, uint32_t len)
{
	uint8_t cmd = ENC28J60_WRITE_BUF_MEM;
	//	ptrSPI->Control (ARM_SPI_CONTROL_SS, ARM_SPI_SS_ACTIVE);
	SPI_CS_Assert();
	HAL_SPI_Transmit(&hspi4, &cmd, 1, HAL_MAX_DELAY);
	while (HAL_SPI_GetState(&hspi4) == HAL_SPI_STATE_BUSY_TX) {
		;
	}
	//	ptrSPI->Send (data, len);
	HAL_SPI_Transmit(&hspi4, data, len, HAL_MAX_DELAY);
	while (HAL_SPI_GetState(&hspi4) == HAL_SPI_STATE_BUSY_TX) {
		;
	}
	//  ptrSPI->Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);
	SPI_CS_Deassert();
}

void readBuffer(uint16_t len, uint8_t* data)
{
	uint8_t cmd = ENC28J60_READ_BUF_MEM;
	//	ptrSPI->Control (ARM_SPI_CONTROL_SS, ARM_SPI_SS_ACTIVE);
	SPI_CS_Assert();
	//  ptrSPI->Send (&cmd, 1);
	HAL_SPI_Transmit(&hspi4, &cmd, 1, HAL_MAX_DELAY);
	//	while (ptrSPI->GetStatus().busy) {
	//		;
	//	}
	//	ptrSPI->Receive(data, len);
	HAL_SPI_Receive(&hspi4, data, len, 100);
	while (HAL_SPI_GetState(&hspi4) == HAL_SPI_STATE_BUSY_RX) {
		;
	}
	//  ptrSPI->Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);
	SPI_CS_Deassert();
}

uint16_t readBufferWord() {
	uint16_t result;
	readBuffer(2, (uint8_t*) &result);
	return result;
}

uint32_t readId(void) {
	uint16_t phid1;
	uint16_t phid2;

	phid1 = readPhyRegister(PHID1);
	phid2 = readPhyRegister(PHID2);

	return ((((uint32_t)phid1) << 6) | (phid2 >> 10));
}


/* Ethernet Driver functions */


void MAC_Initialize (void* cb_event) {
	(void)cb_event;

	softReset();
	uint8_t estat = readControlRegister(ESTAT);
	printf("ESTAT:   0x%x\n", (unsigned int)estat);
	uint16_t phcon1 = readPhyRegister(PHCON1);
	printf("PHCON1:  0x%x\n", (unsigned int)phcon1);
	uint16_t phcon2 = readPhyRegister(PHCON2);
	printf("PHCON2:  0x%x\n", (unsigned int)phcon2);
	uint16_t phstat1 = readPhyRegister(PHSTAT1);
	printf("PHSTAT1: 0x%x\n", (unsigned int)phstat1);
	uint16_t phstat2 = readPhyRegister(PHSTAT2);
	printf("PHSTAT2: 0x%x\n", (unsigned int)phstat2);
	uint16_t phid1 = readPhyRegister(PHID1);
	printf("PHID1:   0x%x\n", (unsigned int)phid1);
	uint16_t phid2 = readPhyRegister(PHID2);
	printf("PHID2:   0x%x\n", (unsigned int)phid2);
	uint16_t phie = readPhyRegister(PHIE);
	printf("PHIE:    0x%x\n", (unsigned int)phie);
	uint16_t phir = readPhyRegister(PHIR);
	printf("PHIR:    0x%x\n", (unsigned int)phir);
	uint16_t phlcon = readPhyRegister(PHLCON);
	printf("PHLCON:  0x%x\n", (unsigned int)phlcon);
	uint32_t phid = readId();
	printf("PHID:    0x%x\n", (unsigned int)phid);

	// set receive buffer start address
	gNextPacketPtr = RXSTART_INIT;
	// Rx start
	writeControlRegister16(ERXSTL, RXSTART_INIT);
	// set receive pointer address
	writeControlRegister16(ERXRDPTL, RXSTART_INIT);
	// RX end
	writeControlRegister16(ERXNDL, RXSTOP_INIT);
	// TX start
	writeControlRegister16(ETXSTL, TXSTART_INIT);
	// TX end
	writeControlRegister16(ETXNDL, TXSTOP_INIT);
	// do bank 1 stuff, packet filter:
	// For broadcast packets we allow only ARP packtets
	// All other packets should be unicast only for our mac (MAADR)
	//
	// The pattern to match on is therefore
	// Type     ETH.DST
	// ARP      BROADCAST
	// 06 08 -- ff ff ff ff ff ff -> ip checksum for theses bytes=f7f9
	// in binary these poitions are:11 0000 0011 1111
	// This is hex 303F->EPMM0=0x3f,EPMM1=0x30

	erxfcon =  ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN|ERXFCON_BCEN;
	writeControlRegister8(ERXFCON, erxfcon );
	writeControlRegister16(EPMM0, 0x303f);
	writeControlRegister16(EPMCSL, 0xf7f9);

	// do bank 2 stuff
	// enable MAC receive
	writeControlRegister8(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	// bring MAC out of reset
	writeControlRegister8(MACON2, 0x00);
	// enable automatic padding to 60bytes and CRC operations
	writeOperation(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);  //|MACON3_FULDPX);
	// set inter-frame gap (non-back-to-back)
	writeControlRegister16(MAIPGL, 0x0C12);
	// set inter-frame gap (back-to-back)
	writeControlRegister8(MABBIPG, 0x12);
	// Set the maximum packet size which the controller will accept
	// Do not send packets longer than MAX_FRAMELEN:
	writeControlRegister16(MAMXFLL, MAX_FRAMELEN);
	// do bank 3 stuff
	// write MAC address
	// NOTE: MAC address in ENC28J60 is byte-backward
	writeControlRegister8(MAADR5, macaddr[0]);
	writeControlRegister8(MAADR4, macaddr[1]);
	writeControlRegister8(MAADR3, macaddr[2]);
	writeControlRegister8(MAADR2, macaddr[3]);
	writeControlRegister8(MAADR1, macaddr[4]);
	writeControlRegister8(MAADR0, macaddr[5]);
	// no loopback of transmitted frames
	writePhyRegister(PHCON2, PHCON2_HDLDIS);
	// switch to bank 0
	setBank(ECON1);
	// enable interrutps
	writeOperation(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
	// enable packet reception
	writeOperation(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);

	// do bank 2 stuff
	// enable MAC receive
	writeControlRegister8(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	// bring MAC out of reset
	writeControlRegister8(MACON2, 0x00);
	// enable automatic padding to 60bytes and CRC operations
	writeOperation(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);  //|MACON3_FULDPX);
	// set inter-frame gap (non-back-to-back)
	writeControlRegister16(MAIPGL, 0x0C12);
	// set inter-frame gap (back-to-back)
	writeControlRegister8(MABBIPG, 0x12);
	// Set the maximum packet size which the controller will accept
	// Do not send packets longer than MAX_FRAMELEN:
	writeControlRegister16(MAMXFLL, MAX_FRAMELEN);

	// do bank 3 stuff
	// write MAC address
	// NOTE: MAC address in ENC28J60 is byte-backward
	writeControlRegister8(MAADR5, macaddr[0]);
	writeControlRegister8(MAADR4, macaddr[1]);
	writeControlRegister8(MAADR3, macaddr[2]);
	writeControlRegister8(MAADR2, macaddr[3]);
	writeControlRegister8(MAADR1, macaddr[4]);
	writeControlRegister8(MAADR0, macaddr[5]);

	// switch to bank 0
	setBank(ECON1);
	// enable interrupt when packet is received
	writeOperation(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
	// enable packet reception
	writeOperation(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
}



/**
  \fn          int32_t SendFrame (const uint8_t *frame, uint32_t len, uint32_t flags)
  \brief       Send Ethernet frame.
  \param[in]   frame  Pointer to frame buffer with data to send
  \param[in]   len    Frame buffer length in bytes
  \param[in]   flags  Frame transmit flags (see ARM_ETH_MAC_TX_FRAME_...)
  \return      \ref execution_status
 */
bool MAC_SendFrame (const uint8_t *frame, uint32_t len, uint32_t flags) {
	//printf("MAC_SendFrame called: %d\n", (int)len);
	if ((frame == NULL) || (len == 0U)) {
		return false;
	}
	// Check no transmit in progress
	while (readOperation(ENC28J60_READ_CTRL_REG, ECON1) & ECON1_TXRTS)
	{
		// Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
		if( (readControlRegister(EIR) & EIR_TXERIF) ) {
			writeOperation(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRST);
			writeOperation(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRST);
		}
	}

	// Set the write pointer to start of transmit buffer area
	writeControlRegister16(EWRPTL, TXSTART_INIT);
	// Set the TXND pointer to correspond to the packet size given
	writeControlRegister16(ETXNDL, (TXSTART_INIT+len));
	// write per-packet control byte (0x00 means use macon3 settings)
	writeOperation(ENC28J60_WRITE_BUF_MEM, 0, 0x00);
	// copy the packet into the transmit buffer
	writeBuffer(frame, len);
	// send the contents of the transmit buffer onto the network
	writeOperation(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
	// Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
	return true;
}

/**
  \fn          int32_t ReadFrame (uint8_t *frame, uint32_t len)
  \brief       Read data of received Ethernet frame.
  \param[in]   frame  Pointer to frame buffer for data to read into
  \param[in]   len    Frame buffer length in bytes
  \return      number of data bytes read or execution status
                 - value >= 0: number of data bytes read
                 - value < 0: error occurred, value is execution status as defined with \ref execution_status 
 */
int32_t MAC_ReadFrame (uint8_t *frame, uint32_t maxlen) {

	if(readControlRegister(EPKTCNT) == 0) {
		return 0;
	}
	// Set the read pointer to the start of the received packet
	writeControlRegister16(ERDPTL, gNextPacketPtr);

	struct __attribute__((packed)) {
		uint16_t nextPtr;
		uint16_t len;
		uint16_t status;
	} header;
	readBuffer(6, (uint8_t*)&header);		
	// calculate next read pointer
	gNextPacketPtr = header.nextPtr;

	//printf("header.nextPtr: %x\n", (unsigned)header.nextPtr);
	//printf("header.status: %x\n", (unsigned)header.status);

	// limit retrieve length
	if (header.len > maxlen - 1){
		header.len = maxlen - 1;
	}
	// check CRC and symbol errors (see datasheet page 44, table 7-3):
	// The ERXFCON.CRCEN is set by default. Normally we should not
	// need to check this.
	if ((header.status & 0x80)==0){
		// invalid
		header.len=0;
	}else{
		// copy the packet from the receive buffer
		readBuffer(header.len, frame);
	}

	// Move the RX read pointer to the start of the next received packet
	if ((gNextPacketPtr < RXSTART_INIT)	|| (gNextPacketPtr > RXSTOP_INIT)) {
		gNextPacketPtr = RXSTOP_INIT;
	}
	writeControlRegister16(ERXRDPTL, gNextPacketPtr);

	// decrement the packet counter indicate we are done with this packet
	writeOperation(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
	//  printf("ETHMAC: Got packet with len: %d\n", (unsigned)header.len);
	return header.len;
}

/**
  \fn          uint32_t GetRxFrameSize (void)
  \brief       Get size of received Ethernet frame.
  \return      number of bytes in received frame
 */
uint32_t MAC_GetRxFrameSize (void) {

	uint8_t pktCnt = readControlRegister(EPKTCNT);

	if(pktCnt == 0) {
		// no packets in receive queue
		return 0;
	}
	//printf("EPKTCNT = %d\n", (int)pktCnt);
	//printf("gNextPacketPtr0: %x\n", (unsigned)gNextPacketPtr);
	// Set the read pointer to the start of the received packet
	writeControlRegister16(ERDPTL, gNextPacketPtr);
	// read the next packet pointer
	struct __attribute__((packed)) {
		uint16_t nextPtr;
		uint16_t len;
		uint16_t status;
	} header;
	header.len = 0;
	header.nextPtr = 0;
	readBuffer(6, (uint8_t*)&header);	
	return header.len;
}


void MAC_SetMACAddress(uint8_t* macAddr) {
	memcpy(macaddr, macAddr, 6);
}

bool PHY_IsLinkUp (void) {
	if (readPhyRegister(PHSTAT2) & PHSTAT2_LSTAT) {
		return true;
	}
	return false;
}
