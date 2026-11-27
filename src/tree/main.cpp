#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>
#include "effects.h"
#include "espnow_control.h"
#include "wifi_ota.h"

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Saved settings (NVS flash) -- survives power loss and OTA updates
static Preferences prefs;

// Only the LED task touches these
uint8_t  currentPreset   = PRESET_CANDYCANE;
uint32_t presetStartTime = 0;

// ---- Effects ----
static void runEffect(uint16_t p) {
  switch (p) {
    case PRESET_RED:          Red();          break;
    case PRESET_BLUE:         Blue();         break;
    case PRESET_GREEN:        Green();        break;
    case PRESET_LIGHTGREEN:   lightGreen();   break;
    case PRESET_WHITE:        White();        break;
    case PRESET_WARMWHITE:    warmWhite();    break;
    case PRESET_PASTELSWEEP:  pastelSweep();  break;
    case PRESET_COLORSWEEP:   colorSweep();   break;
    case PRESET_RETROBULBS:   retroBulbs();   break;
    case PRESET_AURORA:       aurora();       break;
    case PRESET_FIREWORKS:    fireworks();    break;
    case PRESET_METEORRAIN:   meteorRain(0xff, 0xff, 0xff, 30, 64, true, 60); break;
    case PRESET_CANDYCANE:    candyCane();    break;
    case PRESET_RAINBOW:      rainbow();      break;
    case PRESET_LSU:          LSU();          break;
    case PRESET_SNOW:         SnowSparkle(0x10, 0x10, 0x10, 20, 200); break;
    case PRESET_SHOOTINGSTAR: shootingStar(); break;
    case PRESET_BERRYTREE:    berryTree();    break;
  }
}

static void drawOtaProgress() {
  uint16_t lit = (uint32_t)wifiOtaPercent() * LED_COUNT / 100;
  for (uint16_t i = 0; i < LED_COUNT; i++)
    strip.setPixelColor(i, i < lit ? strip.Color(0, 0, 40) : 0);
  strip.show();
}

void ledTask(void *) {
  const TickType_t framePeriod = pdMS_TO_TICKS(10);   // ~100 fps cap
  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    ControlPacket pkt;
    // Controller resends every second; only act on a real, known preset change
    if (espnowControlGetCommand(pkt) && pkt.preset != currentPreset
        && pkt.preset < PRESET_COUNT) {
      currentPreset   = pkt.preset;
      presetStartTime = millis();               // receiver's clock, not the sender's
      prefs.putUChar("preset", currentPreset);  // remember across power loss
    }

    if (wifiOtaInProgress()) drawOtaProgress();
    else                     runEffect(currentPreset);

    vTaskDelayUntil(&lastWake, framePeriod);
  }
}

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.show();

  // Restore last preset (default on first boot or if the saved value is out of range)
  prefs.begin("tree", false);
  currentPreset = prefs.getUChar("preset", PRESET_CANDYCANE);
  if (currentPreset >= PRESET_COUNT) currentPreset = PRESET_CANDYCANE;
  presetStartTime = millis();

  wifiOtaBegin();   // AP (master) or join master (receiver), plus OTA task

  // Don't hang on failure: OTA must stay reachable even if ESP-NOW breaks
  if (!espnowControlBegin()) Serial.println("ESP-NOW init failed");

  xTaskCreatePinnedToCore(ledTask, "led", 4096, nullptr, 2, nullptr, 1);
}

void loop() {
  vTaskDelete(nullptr);   // all work happens in the tasks above
}