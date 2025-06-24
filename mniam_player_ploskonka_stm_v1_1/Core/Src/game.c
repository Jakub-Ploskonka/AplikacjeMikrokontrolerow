#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include "cmsis_os2.h"
#include "game.h"
#include <string.h>

#include "amcom.h"
#include "amcom_packets.h"

#include <FreeRTOS.h>
#include <FreeRTOS_IP.h>
#include <FreeRTOS_Sockets.h>

#define MAX_DISTANCE 100000
#define AMCOM_ObjectState_size 12
#define MAX_NUMBER_OF_RECORDS 100 // moze byc max 100 tranzystorow
#define PI 3.14159f


volatile AMCOM_ObjectState playerJakub = {0, 255, 0, 0.0f, 0.0f};
 
volatile size_t counts[4] = {0};
volatile AMCOM_ObjectState objects[4][MAX_NUMBER_OF_RECORDS]; // 4 typy obiektow, max 16 rekordow 

volatile const float bezpiecznaOdlegloscOdIskry = 90.0f;


void aktualizacjaListyObiektow(AMCOM_ObjectState *objectArray, size_t *count, const AMCOM_ObjectState *newObject) {
    for (size_t i = 0; i < *count; i++) {
        if (objectArray[i].objectNo == newObject->objectNo) {
            if (newObject->hp == 0) {
                for (size_t j = i; j < *count - 1; j++) {
                    objectArray[j] = objectArray[j + 1];
                }
                (*count)--;
                printf("Removed object type %u, no=%u: 0 hp\n", newObject->objectType, newObject->objectNo);
            } else {
                objectArray[i] = *newObject;
                printf("Updated object type %u, no=%u\n", newObject->objectType, newObject->objectNo);
            }
            return;
        }
    }

    if (newObject->hp != 0) {
        if (*count < MAX_NUMBER_OF_RECORDS) {
            objectArray[*count] = *newObject;
            (*count)++;
            printf("Added new object type %u, no=%u\n", newObject->objectType, newObject->objectNo);
        }
    }
}

void obslugaAktualizacjiObiektu(const uint8_t *payload, size_t payload_length) {
    if (payload_length % AMCOM_ObjectState_size != 0) {
        printf("Invalid payload length: %zu\n", payload_length);
        return;
    }

    size_t basic_count = payload_length / AMCOM_ObjectState_size;

    for (size_t i = 0; i < basic_count; i++) {
        const uint8_t *record = payload + i * AMCOM_ObjectState_size;

        uint8_t type = record[0];
        uint8_t no = (uint16_t)record[1] | ((uint16_t)record[2] << 8);
        uint8_t hp = record[3];
        float x, y;
        memcpy(&x, record + 4, sizeof(float));
        memcpy(&y, record + 8, sizeof(float));

        AMCOM_ObjectState obj = { type, no, hp, x, y };

        bool my_playerJakub = (type == 0 && no == playerJakub.objectNo);
        if (my_playerJakub) {
            playerJakub.x = x;
            playerJakub.y = y;
            playerJakub.hp = hp;
        }

        //printf("Received object: type=%u no=%u hp=%u x=%.2f y=%.2f%s\n", type, no, hp, x, y, my_playerJakub ? "Gracz Jakub" : "");

        if (type < 4) {
            aktualizacjaListyObiektow(objects[type], &counts[type], &obj);
        } else {
            printf("Unknown object type: %u\n", type);
        }
    }
}

float obliczOdlegloscEuklidesowa(float wsp_x1, float wsp_y1, float wsp_x2, float wsp_y2) {
    float dx = wsp_x2 - wsp_x1;
    float dy = wsp_y2 - wsp_y1;
    return sqrtf(dx*dx + dy*dy);
}

float obliczKat(float wsp_x1, float wsp_y1, float wsp_x2, float wsp_y2) {
    return atan2f(wsp_y2 - wsp_y1, wsp_x2 - wsp_x1);
}

float normalizujKat(float angle) {
	while (angle < 0.0f) {
	    angle += 2 * PI;
	}
	while (angle >= 2 * PI) {
	    angle -= 2 * PI;
	}

    return angle;
}

float katIskry(float threatAngle) {
    return normalizujKat(threatAngle + PI);  // zmiana kata o 180 stopni
}

float obliczKatRuchu(float myWspX, float myWspY) {
    float closestTransistorDist = MAX_DISTANCE;
    float closestTransistorAngle = 0.0f;

    float closestSparkDist = MAX_DISTANCE;
    float closestSparkAngle = 0.0f;

    // Szukaj najbliższego tranzystora
    for (size_t i = 0; i < counts[1]; i++) {
        if (objects[1][i].hp == 0) continue;
        float distance = obliczOdlegloscEuklidesowa(myWspX, myWspY, objects[1][i].x, objects[1][i].y);
        if (distance < closestTransistorDist) {
            closestTransistorDist = distance;
            closestTransistorAngle = obliczKat(myWspX, myWspY, objects[1][i].x, objects[1][i].y);
        }
    }

    // Szukaj najbliższej iskry
    for (size_t i = 0; i < counts[2]; i++) {
        if (objects[2][i].hp == 0) continue;
        float distance = obliczOdlegloscEuklidesowa(myWspX, myWspY, objects[2][i].x, objects[2][i].y);
        if (distance < closestSparkDist) {
            closestSparkDist = distance;
            closestSparkAngle = obliczKat(myWspX, myWspY, objects[2][i].x, objects[2][i].y);
        }
    }

    // Jeśli iskra jest za blisko – uciekaj
    if (closestSparkDist < bezpiecznaOdlegloscOdIskry) {
        return katIskry(closestSparkAngle);
    }

    // W przeciwnym razie – idź do najbliższego transistora
    if (closestTransistorDist < MAX_DISTANCE) {
        return closestTransistorAngle;
    }

    // Jeśli nic nie ma – kręć się losowo
    return ((float)(rand() % 123) / 100.0f); // losowy kąt 0 - ~1.23 rad
}


void amcomPacketHandler(const AMCOM_Packet* packet, void* userContext) {
    uint8_t buf[AMCOM_MAX_PACKET_SIZE];              // buffer used to serialize outgoing packets
    size_t toSend = 0;                               // size of the outgoing packet
    Socket_t socket = (Socket_t)userContext;    // socket used for communication with the server


    switch (packet->header.type) {
    case AMCOM_IDENTIFY_REQUEST:
        printf("Got IDENTIFY.request. Responding with IDENTIFY.response\n");
        AMCOM_IdentifyResponsePayload identifyResponse;
        sprintf(identifyResponse.playerName, "Jakub Ploskonka");
        toSend = AMCOM_Serialize(AMCOM_IDENTIFY_RESPONSE, &identifyResponse, sizeof(identifyResponse), buf);
        break;

    case AMCOM_NEW_GAME_REQUEST: {
        printf("Got NEW_GAME.request. Responding with NEW_GAME.response\n");
        playerJakub.objectNo = packet->payload[0];
        AMCOM_NewGameResponsePayload newGameResponse;
        strcpy(newGameResponse.helloMessage, "Hello everyone!");
        toSend = AMCOM_Serialize(AMCOM_NEW_GAME_RESPONSE, &newGameResponse, sizeof(newGameResponse), buf);
        break;
    }

    case AMCOM_MOVE_REQUEST: {
        printf("Got MOVE.request. Responding with MOVE.response\n");
        AMCOM_MoveResponsePayload moveResponse;
        moveResponse.angle = obliczKatRuchu(playerJakub.x, playerJakub.y);
        toSend = AMCOM_Serialize(AMCOM_MOVE_RESPONSE, &moveResponse, sizeof(moveResponse), buf);
        break;
    }

    case AMCOM_GAME_OVER_REQUEST: {
        printf("Got GAME_OVER.request. Responding with GAME_OVER.response\n");
        AMCOM_GameOverResponsePayload gameOverResponse;
        strcpy(gameOverResponse.endMessage, "Bye!");
        toSend = AMCOM_Serialize(AMCOM_GAME_OVER_RESPONSE, &gameOverResponse, sizeof(gameOverResponse), buf);
        break;
    }

        case AMCOM_OBJECT_UPDATE_REQUEST: {
        printf("Got OBJECT_UPDATE.request.\n");
        const uint8_t *payload = packet->payload;
        size_t payload_length = packet->header.length;
        obslugaAktualizacjiObiektu(payload, payload_length);
        break;
        }

    default:
		break;
	}

	// if there is something to send back - do it
	if (toSend > 0) {
		FreeRTOS_send(socket, buf, toSend, 0);
	}
}
