#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "amcom.h"

/// Start of packet character
const uint8_t  AMCOM_SOP         = 0xA1;
const uint16_t AMCOM_INITIAL_CRC = 0xFFFF;

static uint16_t AMCOM_UpdateCRC(uint8_t byte, uint16_t crc)
{
	byte ^= (uint8_t)(crc & 0x00ff);
	byte ^= (uint8_t)(byte << 4);
	return ((((uint16_t)byte << 8) | (uint8_t)(crc >> 8)) ^ (uint8_t)(byte >> 4) ^ ((uint16_t)byte << 3));
}


void AMCOM_InitReceiver(AMCOM_Receiver* receiver, AMCOM_PacketHandler packetHandlerCallback, void* userContext) {
	memset(&receiver->receivedPacket, 0, sizeof(receiver->receivedPacket));
	receiver->receivedPacketState = AMCOM_PACKET_STATE_EMPTY;
    receiver->packetHandler = packetHandlerCallback;
    receiver->userContext = userContext;
    receiver->payloadCounter = 0;
}

size_t AMCOM_Serialize(uint8_t packetType, const void* payload, size_t payloadSize, uint8_t* destinationBuffer) {
	
	if(!(destinationBuffer) || payloadSize>AMCOM_MAX_PAYLOAD_SIZE || (payloadSize > 0 && payload == NULL)){
	    return 0;
	}
	
	//initial CRC
	uint16_t crc = AMCOM_INITIAL_CRC;
    crc = AMCOM_UpdateCRC(packetType, crc);
    crc = AMCOM_UpdateCRC((uint8_t)payloadSize, crc);
	
	//CRC calculation
	const uint8_t* payloadData = (const uint8_t*)payload;
	for (size_t i = 0; i < payloadSize; i++) {
        crc = AMCOM_UpdateCRC(*(payloadData+i), crc);
    }
	
	//HEADER
	*(destinationBuffer) = AMCOM_SOP;
	*(destinationBuffer+1) = packetType;
	*(destinationBuffer+2) = (uint8_t)payloadSize;
    *(destinationBuffer+3) = (uint8_t)(crc);
	*(destinationBuffer+4) = (uint8_t)(crc >> 8);
    
	if (payloadSize > 0) {
        memcpy(destinationBuffer+5, payload, payloadSize);
    }
	
	//return payloadSize + headerSize
	return payloadSize + 5;
}

void AMCOM_Deserialize(AMCOM_Receiver* receiver, const void* data, size_t dataSize) {
    
    const uint8_t* receivedData = (const uint8_t*)data;
    
    uint8_t curr;
    //uint16_t crc;
    
    for (size_t i = 0; i < dataSize; i++) {
        curr = *(receivedData+i);
        switch (receiver->receivedPacketState) {
            
            case AMCOM_PACKET_STATE_EMPTY:
                if (curr == AMCOM_SOP) {
                    //memset(&receiver->receivedPacket, 0, sizeof(receiver->receivedPacket));
                    //crc = AMCOM_INITIAL_CRC;
                    //receiver->payloadCounter = 0;
                    receiver->receivedPacket.header.sop = curr;
                    receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_SOP;
                }
                break;
                
            case AMCOM_PACKET_STATE_GOT_SOP:
                receiver->receivedPacket.header.type = curr;
                //crc = AMCOM_UpdateCRC(curr, crc);
                receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_TYPE;
                break;
                
            case AMCOM_PACKET_STATE_GOT_TYPE:
                if (curr > AMCOM_MAX_PAYLOAD_SIZE) {
                    receiver->receivedPacketState = AMCOM_PACKET_STATE_EMPTY;
                    memset(&receiver->receivedPacket, 0, sizeof(receiver->receivedPacket));
                }
                else {
                    receiver->receivedPacket.header.length = curr;
                    //crc = AMCOM_UpdateCRC(curr, crc);
                    receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_LENGTH;
                }
                break;
                
            case AMCOM_PACKET_STATE_GOT_LENGTH:
                receiver->receivedPacket.header.crc = curr;
                receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_CRC_LO;
                break;
                
            case AMCOM_PACKET_STATE_GOT_CRC_LO:
                receiver->receivedPacket.header.crc |= ((uint16_t)curr << 8);
                if (receiver->receivedPacket.header.length == 0) {
                    uint16_t crc = AMCOM_INITIAL_CRC;
                    crc = AMCOM_UpdateCRC(receiver->receivedPacket.header.type, crc);
                    crc = AMCOM_UpdateCRC(receiver->receivedPacket.header.length, crc);
                    if (crc == receiver->receivedPacket.header.crc) {
                        receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_WHOLE_PACKET;
                    }
                    else{
                        receiver->receivedPacketState = AMCOM_PACKET_STATE_EMPTY;
                        memset(&receiver->receivedPacket, 0, sizeof(receiver->receivedPacket));
                    }
                }
                else{
                    receiver->payloadCounter = 0;
                    receiver->receivedPacketState = AMCOM_PACKET_STATE_GETTING_PAYLOAD;
                }
                break;
                
            case AMCOM_PACKET_STATE_GETTING_PAYLOAD:
				if (receiver->payloadCounter < AMCOM_MAX_PAYLOAD_SIZE) {
					receiver->receivedPacket.payload[receiver->payloadCounter] = curr;
					receiver->payloadCounter += 1;
					
					if (receiver->payloadCounter == receiver->receivedPacket.header.length) {
					    
					    uint16_t crc = AMCOM_INITIAL_CRC;
                        crc = AMCOM_UpdateCRC(receiver->receivedPacket.header.type, crc);
                        crc = AMCOM_UpdateCRC(receiver->receivedPacket.header.length, crc);
						for (size_t j = 0; j < (size_t)receiver->receivedPacket.header.length; j++) {
							crc = AMCOM_UpdateCRC(receiver->receivedPacket.payload[j], crc);
						}
						if (crc == receiver->receivedPacket.header.crc) {
                        receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_WHOLE_PACKET;
                        }
                        else{
                            receiver->receivedPacketState = AMCOM_PACKET_STATE_EMPTY;
                            memset(&receiver->receivedPacket, 0, sizeof(receiver->receivedPacket));
                        }
					}
				}
				else {
                    receiver->receivedPacketState = AMCOM_PACKET_STATE_EMPTY;
                    memset(&receiver->receivedPacket, 0, sizeof(receiver->receivedPacket));
                }
                break;
        }
        if(receiver->receivedPacketState == AMCOM_PACKET_STATE_GOT_WHOLE_PACKET){
            if ( receiver->packetHandler != NULL /* && crc == receiver->receivedPacket.header.crc */) {
                receiver->packetHandler(&receiver->receivedPacket, receiver->userContext);
            }
            receiver->receivedPacketState = AMCOM_PACKET_STATE_EMPTY;
            memset(&receiver->receivedPacket, 0, sizeof(receiver->receivedPacket));
        }
    }
}