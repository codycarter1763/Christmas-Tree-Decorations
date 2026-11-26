#ifndef ESPNOW_CONTROL_H_
#define ESPNOW_CONTROL_H_

#include "control_packet.h"

// Call after WiFi.mode(WIFI_STA). Returns false if ESP-NOW failed to start.
bool espnowControlBegin();

// Non-blocking. Returns true and fills `out` if a new command has arrived.
bool espnowControlGetCommand(ControlPacket &out);

#endif
