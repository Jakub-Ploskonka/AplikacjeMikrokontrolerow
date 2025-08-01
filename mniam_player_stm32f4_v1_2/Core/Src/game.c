#include "cmsis_os2.h"
#include "game.h"
#include "amcom.h"
#include "amcom_packets.h"

#include <stdio.h>
#include <string.h>

#include <FreeRTOS.h>
#include <FreeRTOS_IP.h>
#include <FreeRTOS_Sockets.h>

/**
 * This function is used when AMCOM_MOVE_REQUEST is received
 */
float computeMoveDirection(const GameState* state) {
	// relative coordinates to closest target(best) and danger(worst)
    float bestDx = 0, bestDy = 0;
    float worstDx = 0, worstDy = 0;
    float bestDist = 1e9f;
    float worstDist = 1e9f;
	// used for determining danger's acceptable distance
	float dangerOffset = 50.0f;

	// checking all objects on map
    for (size_t i = 0; i < state->objectCount; i++) {
        AMCOM_ObjectState obj = state->objects[i];
		
		// checking only relevant objects (that exist)
        if (obj.hp <= 0)
            continue;

		// relative coordinates and distance calculation
        float dx = obj.x - state->myX;
        float dy = obj.y - state->myY;
        float dist2 = dx * dx + dy * dy;

		// my player object not taken into account 
        if (dist2 < 1e-3)
            continue;

        float danger_zone = 25.0f;
        int isDanger = 0;
        int isTarget = 0;

		// determining behaviour based on type and proximity
        switch (obj.objectType) {
            case 0: // player
                danger_zone = 12.5f + obj.hp/2.0f + 12.5f + state->myHp/2.0f + dangerOffset;
                if (obj.hp >= state->myHp && dist2 <= danger_zone * danger_zone)
                    isDanger = 1;
                else if (obj.hp < state->myHp)
                    isTarget = 1;
                break;
            case 1: // transistor
                isTarget = 1;
                break;
            case 2: // spark
                danger_zone = 12.5f + 12.5f + state->myHp/2.0f + dangerOffset;
                if (dist2 <= danger_zone * danger_zone)
                    isDanger = 1;
                break;
			// glue not taken into account as transistor location may overlap with glue's surface and thus be unapproachable
        }

		// closest Target and closest possible danger relative location update
        if (isTarget && dist2 < bestDist) {
            bestDx = dx;
            bestDy = dy;
            bestDist = dist2;
        }
        if (isDanger && dist2 < worstDist) {
            worstDx = dx;
            worstDy = dy;
            worstDist = dist2;
        }
    }

    // no targets left - go right
    if (bestDx == 0 && bestDy == 0)
        return 0.0f;

    // target's direction normalization
    float bestLen = sqrtf(bestDx * bestDx + bestDy * bestDy);
    float goalX = bestDx / bestLen;
    float goalY = bestDy / bestLen;

	// danger's direction normalization
    float dangerX = 0.0f, dangerY = 0.0f;
    if (worstDx != 0 || worstDy != 0) {
        float worstLen = sqrtf(worstDx * worstDx + worstDy * worstDy);
        dangerX = worstDx / worstLen;
        dangerY = worstDy / worstLen;
    }

    // target's and danger's directions combination, altered by fear
    float fear = 1.5f;
    float moveX = goalX - dangerX * fear;
    float moveY = goalY - dangerY * fear;

    return atan2f(moveY, moveX);
}


/**
 * This function will be called each time a valid AMCOM packet is received
 */
void amcomPacketHandler(const AMCOM_Packet* packet, void* userContext) {
	static uint8_t buf[AMCOM_MAX_PACKET_SIZE];  // buffer used to serialize outgoing packets
	size_t toSend = 0;                          // size of the outgoing packet
	Socket_t socket = (Socket_t)userContext;    // socket used for communication with the server

	// gameState init
	static GameState gameState = {0};

	switch (packet->header.type) {
	case AMCOM_IDENTIFY_REQUEST:
		printf("Got IDENTIFY.request. Responding with IDENTIFY.response\n");
		AMCOM_IdentifyResponsePayload identifyResponse;
		sprintf(identifyResponse.playerName, "JakubPloskonka");
		toSend = AMCOM_Serialize(AMCOM_IDENTIFY_RESPONSE, &identifyResponse, sizeof(identifyResponse), buf);
		break;
	
	case AMCOM_NEW_GAME_REQUEST:
        const AMCOM_NewGameRequestPayload* newGameRequest = (const void*)packet->payload;
        gameState.playerNumber = newGameRequest->playerNumber;
        printf("NEW_GAME: I'm player #%d of %d\n", newGameRequest->playerNumber, newGameRequest->numberOfPlayers);

        AMCOM_NewGameResponsePayload newGameResponse;
        sprintf(newGameResponse.helloMessage, "Fine dining, here I come!");
        toSend = AMCOM_Serialize(AMCOM_NEW_GAME_RESPONSE, &newGameResponse, sizeof(newGameResponse), buf);
        break;
	
	case AMCOM_OBJECT_UPDATE_REQUEST:
        const AMCOM_ObjectUpdateRequestPayload* updateRequest = (const void*)packet->payload;
        int count = packet->header.length / sizeof(AMCOM_ObjectState);

        for (size_t i = 0; i < count && gameState.objectCount < MAX_ALL_OBJECTS; i++) {
            AMCOM_ObjectState obj = updateRequest->objectState[i];

			uint8_t swapped = 0;
			for (size_t j = 0; j < gameState.objectCount; j++)
			{
				AMCOM_ObjectState obj2 = gameState.objects[j];
				if(obj2.objectNo == obj.objectNo && obj2.objectType == obj.objectType)
				{
					gameState.objects[j] = obj;
					swapped = 1;
				}
			}
			if(swapped == 0) gameState.objects[gameState.objectCount++] = obj;

            // save my coordinates in variables separate from other players
            if (obj.objectType == 0 && obj.objectNo == gameState.playerNumber) {
                gameState.myX = obj.x;
                gameState.myY = obj.y;
                gameState.myHp = obj.hp;
            }
        }
        break;

    case AMCOM_MOVE_REQUEST:
        const AMCOM_MoveRequestPayload* moveRequest = (const void*)packet->payload;
        printf("MOVE.request (time=%u) - processing %d objects\n", moveRequest->gameTime, gameState.objectCount);

        float angle = computeMoveDirection(&gameState);

        AMCOM_MoveResponsePayload moveResponse = { .angle = angle };
        toSend = AMCOM_Serialize(AMCOM_MOVE_RESPONSE, &moveResponse, sizeof(moveResponse), buf);

        break;

    case AMCOM_GAME_OVER_REQUEST:
        printf("GAME_OVER.request received. Sending final message.\n");
        AMCOM_GameOverResponsePayload gameOverResponse;
        sprintf(gameOverResponse.endMessage, "Thanks for the meal");
        toSend = AMCOM_Serialize(AMCOM_GAME_OVER_RESPONSE, &gameOverResponse, sizeof(gameOverResponse), buf);
        break;
		
	default:
		break;
	}

	// if there is something to send back - do it
	if (toSend > 0) {
		FreeRTOS_send(socket, buf, toSend, 0);
	}
}
