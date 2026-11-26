#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "net_config.h"
#include "wifi_ota.h"

#ifndef NODE_ID
#error "Set NODE_ID in platformio.ini build_flags (1 = master)"
#endif

static volatile bool    otaActive  = false;
static volatile uint8_t otaPct     = 0;
static char             hostname[24];

bool    wifiOtaInProgress() { return otaActive; }
uint8_t wifiOtaPercent()    { return otaPct; }

static void startOTA() {
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA
    .onStart([]()  { otaPct = 0; otaActive = true; })
    .onProgress([](unsigned int done, unsigned int total) {
      otaPct = (uint8_t)(done * 100 / total);
    })
    .onEnd([]()    { otaPct = 100; })                // reboots right after
    .onError([](ota_error_t e) {
      Serial.printf("OTA error %u\n", e);
      otaActive = false;
    });
  ArduinoOTA.begin();

#if NODE_IS_MASTER
  Serial.printf("[%s] OTA ready at %s, channel %d\n",
                hostname, WiFi.softAPIP().toString().c_str(), WIFI_CHANNEL);
#else
  Serial.printf("[%s] OTA ready at %s, channel %d\n",
                hostname, WiFi.localIP().toString().c_str(), WiFi.channel());
#endif
}

static void otaTask(void *) {
  bool started = false;
  for (;;) {
#if NODE_IS_MASTER
    bool netUp = true;                               // our own AP is always up
#else
    bool netUp = (WiFi.status() == WL_CONNECTED);
#endif
    if (netUp) {
      if (!started) { startOTA(); started = true; }
      ArduinoOTA.handle();
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void wifiOtaBegin() {
  snprintf(hostname, sizeof(hostname), "tree-lights-%d", NODE_ID);

#if NODE_IS_MASTER
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1),
                    IPAddress(255, 255, 255, 0));
  // ssid, pass, channel, hidden, max clients (2 receivers + laptop + spare)
  WiFi.softAP(AP_SSID, AP_PASS, WIFI_CHANNEL, 0, 4);
#else
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname);
  WiFi.config(IPAddress(192, 168, 4, 10 + NODE_ID), IPAddress(192, 168, 4, 1),
              IPAddress(255, 255, 255, 0));
  WiFi.setAutoReconnect(true);
  WiFi.begin(AP_SSID, AP_PASS, WIFI_CHANNEL);
#endif

  WiFi.setSleep(false);   // modem sleep makes ESP-NOW miss packets

  xTaskCreatePinnedToCore(otaTask, "ota", 4096, nullptr, 1, nullptr, 0);
}
