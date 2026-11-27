#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>

#include "control_packet.h"   // same file as the trees: presets + packet
#include "net_config.h"       // WIFI_CHANNEL: master tree's AP channel

// Saved settings (NVS flash) -- survives power loss and firmware updates
Preferences prefs;

// =======================
// RADIO CONFIG
// =======================
#define RESEND_MS    1000     // periodic re-sync so missed/rebooted trees catch up

// =======================
// OLED CONFIG
// =======================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// =======================
// BUTTON CONFIG
// =======================
// XIAO ESP32S3: GPIO0 is the onboard "B" (BOOT) button and isn't on the pads.
// For an external button to GND, use a pad instead, e.g. D1 = GPIO2.
#define BUTTON_PIN 2

// =======================
// I2C PINS (XIAO ESP32S3 default I2C pads)
// =======================
#define I2C_SDA 5   // D4
#define I2C_SCL 6   // D5

// =======================
// GLOBALS
// =======================
uint8_t currentPreset = PRESET_CANDYCANE;

uint8_t broadcastAddress[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// =======================
// PRESET NAME LOOKUP
// =======================
const char* presetName(uint16_t preset) {
  switch (preset) {
    case PRESET_RED:          return "Red";
    case PRESET_BLUE:         return "Blue";
    case PRESET_GREEN:        return "Green";
    case PRESET_LIGHTGREEN:   return "Light Green";
    case PRESET_WHITE:        return "White";
    case PRESET_WARMWHITE:    return "Warm White";
    case PRESET_PASTELSWEEP:  return "Pastel Sweep";
    case PRESET_COLORSWEEP:   return "Color Sweep";
    case PRESET_RETROBULBS:   return "Retro Bulbs";
    case PRESET_AURORA:       return "Aurora";
    case PRESET_FIREWORKS:    return "Fireworks";
    case PRESET_METEORRAIN:   return "Meteor Rain";
    case PRESET_CANDYCANE:    return "Candy Cane";
    case PRESET_RAINBOW:      return "Rainbow";
    case PRESET_LSU:          return "LSU";
    case PRESET_SNOW:         return "Snow Sparkle";
    case PRESET_SHOOTINGSTAR: return "Shooting Star";
    case PRESET_BERRYTREE:    return "Berry Tree";
    default:                  return "Unknown";
  }
}

// =======================
// OLED UPDATE
// =======================
void updateDisplay() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ESP32 Light Control");

  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 22);
  display.println(presetName(currentPreset));

  display.setTextSize(1);
  display.setCursor(0, 56);
  display.printf("Channel %d", WIFI_CHANNEL);

  display.display();
}

// =======================
// SEND ESP-NOW PACKET
// =======================
static unsigned long lastBroadcast = 0;

void sendPreset() {
  ControlPacket pkt;
  pkt.preset    = currentPreset;
  pkt.startTime = millis();   // trees use their own clock; kept for packet compatibility

  esp_now_send(broadcastAddress, (uint8_t*)&pkt, sizeof(pkt));
  lastBroadcast = millis();
}

// =======================
// BUTTON HANDLER
// =======================
void handleButton() {
  static bool lastState = HIGH;
  static unsigned long lastDebounce = 0;

  bool state = digitalRead(BUTTON_PIN);

  if (state != lastState && millis() - lastDebounce > 50) {
    lastDebounce = millis();

    if (state == LOW) {
      currentPreset++;

      if (currentPreset >= PRESET_COUNT)     // wraps after the last preset, whatever it is
        currentPreset = PRESET_RED;

      prefs.putUChar("preset", currentPreset);   // save after wrapping
      sendPreset();
      updateDisplay();
    }
  }

  lastState = state;
}

// =======================
// SETUP
// =======================
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Restore last preset (default on first boot or if the saved value is out of range)
  prefs.begin("controller", false);
  currentPreset = prefs.getUChar("preset", PRESET_CANDYCANE);
  if (currentPreset >= PRESET_COUNT) currentPreset = PRESET_CANDYCANE;

  Wire.begin(I2C_SDA, I2C_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED init failed");   // keep running: trees still get presets
  } else {
    display.setTextColor(SSD1306_WHITE);
    updateDisplay();
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setSleep(false);

  // Lock the radio to the master tree's AP channel
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) delay(1000);
  }

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, broadcastAddress, 6);
  peer.channel = WIFI_CHANNEL;
  peer.ifidx   = WIFI_IF_STA;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  sendPreset(); // Initial broadcast
}

// =======================
// LOOP
// =======================
void loop() {
  handleButton();

  if (millis() - lastBroadcast >= RESEND_MS) {
    sendPreset();
  }
}