#ifndef WIFI_OTA_H_
#define WIFI_OTA_H_

#include <stdint.h>

// Master: starts the access point. Receivers: join the master's AP.
// Then starts a task on core 0 that serves ArduinoOTA.
// Call before espnowControlBegin().
void wifiOtaBegin();

bool    wifiOtaInProgress();
uint8_t wifiOtaPercent();

#endif
