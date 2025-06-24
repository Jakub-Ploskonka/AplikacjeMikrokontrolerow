#include "amcom.h"
#include <stdint.h>
#include "amcom_packets.h"


void aktualizacjaListyObiektow(AMCOM_ObjectState *objectArray, size_t *count, const AMCOM_ObjectState *newObject);

void obslugaAktualizacjiObiektu(const uint8_t *payload, size_t payload_length);

float obliczOdlegloscEuklidesowa(float wsp_x1, float wsp_y1, float wsp_x2, float wsp_y2);

float obliczKat(float wsp_x1, float wsp_y1, float wsp_x2, float wsp_y2);

float normalizujKat(float angle);

float katIskry(float threatAngle);

float obliczKatRuchu(float myWspX, float myWspY);

/** AMCOM packet handler handling game packets. */
void amcomPacketHandler(const AMCOM_Packet* packet, void* userContext);
