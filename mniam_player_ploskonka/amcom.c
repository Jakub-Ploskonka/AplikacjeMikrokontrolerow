#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "amcom.h"

const uint8_t  AMCOM_SOP         = 0xA1;
const uint16_t AMCOM_INITIAL_CRC = 0xFFFF;

static uint16_t AMCOM_UpdateCRC(uint8_t byte, uint16_t crc)
{
    byte ^= (uint8_t)(crc & 0x00ff);
    byte ^= (uint8_t)(byte << 4);
    return ((((uint16_t)byte << 8) | (uint8_t)(crc >> 8)) ^ (uint8_t)(byte >> 4) ^ ((uint16_t)byte << 3));
}

void AMCOM_InitReceiver(AMCOM_Receiver* receiver, AMCOM_PacketHandler packetHandlerCallback, void* userContext) {
    receiver->payloadCounter = 0;
    receiver->receivedPacketState = AMCOM_PACKET_STATE_EMPTY;
    receiver->packetHandler = packetHandlerCallback;
    receiver->userContext = userContext;
}

size_t AMCOM_Serialize(uint8_t packetType, const void* payload, size_t payloadSize, uint8_t* destinationBuffer) {
    if (!destinationBuffer || (payloadSize > 0 && !payload) || payloadSize > AMCOM_MAX_PAYLOAD_SIZE)
        return 0;

    AMCOM_PacketHeader header;
    header.sop = AMCOM_SOP;
    header.type = packetType;
    header.length = (uint8_t)payloadSize;

    uint16_t crc = AMCOM_INITIAL_CRC;
    crc = AMCOM_UpdateCRC(header.type, crc);
    crc = AMCOM_UpdateCRC(header.length, crc);

    if (payload && payloadSize > 0) {
        for (size_t i = 0; i < payloadSize; ++i)
            crc = AMCOM_UpdateCRC(((uint8_t*)payload)[i], crc);
    }

    header.crc = crc;

    // Copy header
    destinationBuffer[0] = header.sop;
    destinationBuffer[1] = header.type;
    destinationBuffer[2] = header.length;
    destinationBuffer[3] = (uint8_t)(header.crc & 0xFF);
    destinationBuffer[4] = (uint8_t)((header.crc >> 8) & 0xFF);

    // Copy payload
    if (payload && payloadSize > 0)
        memcpy(destinationBuffer + 5, payload, payloadSize);

    return 5 + payloadSize;
}

void AMCOM_Deserialize(AMCOM_Receiver* receiver, const void* data, size_t dataSize) {
    const uint8_t* bytes = (const uint8_t*)data;

    for (size_t i = 0; i < dataSize; ++i) {
        uint8_t byte = bytes[i];

        switch (receiver->receivedPacketState) {
            case AMCOM_PACKET_STATE_EMPTY:
                if (byte == AMCOM_SOP) {
                    receiver->receivedPacket.header.sop = byte;
                    receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_SOP;
                }
                break;

            case AMCOM_PACKET_STATE_GOT_SOP:
                receiver->receivedPacket.header.type = byte;
                receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_TYPE;
                break;

            case AMCOM_PACKET_STATE_GOT_TYPE:
                receiver->receivedPacket.header.length = byte;
                if (byte <= AMCOM_MAX_PAYLOAD_SIZE)
                    receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_LENGTH;
                else
                    receiver->receivedPacketState = AMCOM_PACKET_STATE_EMPTY;
                break;

            case AMCOM_PACKET_STATE_GOT_LENGTH:
                receiver->receivedPacket.header.crc = byte;
                receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_CRC_LO;
                break;

            case AMCOM_PACKET_STATE_GOT_CRC_LO:
                receiver->receivedPacket.header.crc |= (uint16_t)(byte << 8);
                receiver->payloadCounter = 0;

                if (receiver->receivedPacket.header.length == 0) {
                    // Validate CRC
                    uint16_t crc = AMCOM_INITIAL_CRC;
                    crc = AMCOM_UpdateCRC(receiver->receivedPacket.header.type, crc);
                    crc = AMCOM_UpdateCRC(receiver->receivedPacket.header.length, crc);

                    if (crc == receiver->receivedPacket.header.crc && receiver->packetHandler) {
                        receiver->packetHandler(&receiver->receivedPacket, receiver->userContext);
                    }

                    receiver->receivedPacketState = AMCOM_PACKET_STATE_EMPTY;
                } else {
                    receiver->receivedPacketState = AMCOM_PACKET_STATE_GETTING_PAYLOAD;
                }
                break;

            case AMCOM_PACKET_STATE_GETTING_PAYLOAD:
                receiver->receivedPacket.payload[receiver->payloadCounter++] = byte;
                if (receiver->payloadCounter >= receiver->receivedPacket.header.length) {
                    // Validate CRC
                    uint16_t crc = AMCOM_INITIAL_CRC;
                    crc = AMCOM_UpdateCRC(receiver->receivedPacket.header.type, crc);
                    crc = AMCOM_UpdateCRC(receiver->receivedPacket.header.length, crc);
                    for (size_t j = 0; j < receiver->receivedPacket.header.length; ++j)
                        crc = AMCOM_UpdateCRC(receiver->receivedPacket.payload[j], crc);

                    if (crc == receiver->receivedPacket.header.crc && receiver->packetHandler) {
                        receiver->packetHandler(&receiver->receivedPacket, receiver->userContext);
                    }

                    receiver->receivedPacketState = AMCOM_PACKET_STATE_EMPTY;
                }
                break;

            case AMCOM_PACKET_STATE_GOT_WHOLE_PACKET: {
                // Validate CRC
                uint16_t crc = AMCOM_INITIAL_CRC;
                crc = AMCOM_UpdateCRC(receiver->receivedPacket.header.type, crc);
                crc = AMCOM_UpdateCRC(receiver->receivedPacket.header.length, crc);
                for (size_t j = 0; j < receiver->receivedPacket.header.length; ++j)
                    crc = AMCOM_UpdateCRC(receiver->receivedPacket.payload[j], crc);

                if (crc == receiver->receivedPacket.header.crc && receiver->packetHandler) {
                    receiver->packetHandler(&receiver->receivedPacket, receiver->userContext);
                }

                receiver->receivedPacketState = AMCOM_PACKET_STATE_EMPTY;
                break;
            }

            default:
                receiver->receivedPacketState = AMCOM_PACKET_STATE_EMPTY;
                break;
        }
    }
}