#ifndef NET_CONFIG_H_
#define NET_CONFIG_H_

// Shared by the trees and the controller.

// NODE_ID only exists on tree builds (set in platformio.ini)
#ifdef NODE_ID
#define NODE_IS_MASTER (NODE_ID == 1)
#endif

#define AP_SSID       "tree-lights"
#define AP_PASS       "*SantaPaws321"   // WPA2: 8+ characters
#define WIFI_CHANNEL  6               // AP channel == ESP-NOW channel for everyone
#define OTA_PASSWORD  "*SantaPaws321"

// Master:    192.168.4.1
// Receivers: 192.168.4.(10 + NODE_ID)  -> node 2 = .12, node 3 = .13

#endif
