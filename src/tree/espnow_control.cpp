#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "espnow_control.h"

static QueueHandle_t commandQueue = nullptr;

// Runs in the WiFi task's context
static void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(ControlPacket) || commandQueue == nullptr) return;
  ControlPacket pkt;
  memcpy(&pkt, data, sizeof(pkt));
  xQueueOverwrite(commandQueue, &pkt);   // keep only the latest command
}

bool espnowControlBegin() {
  if (commandQueue == nullptr) {
    commandQueue = xQueueCreate(1, sizeof(ControlPacket));
    if (commandQueue == nullptr) return false;
  }
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_recv_cb(onDataRecv);
  return true;
}

bool espnowControlGetCommand(ControlPacket &out) {
  if (commandQueue == nullptr) return false;
  return xQueueReceive(commandQueue, &out, 0) == pdTRUE;
}
