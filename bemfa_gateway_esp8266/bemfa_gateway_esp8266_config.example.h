#pragma once

// Copy this file to bemfa_gateway_esp8266_config.h.
// Do not commit your real UID or WiFi password.

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Bemfa user private key / UID.
#define BEMFA_UID "YOUR_BEMFA_UID"

// Static mappings for manually created Bemfa switch topics.
// Topic names must match Bemfa exactly. Controller IDs are 3 bytes / 6 hex chars.
// Empty controller IDs are useful for MQTT-only testing.
#define STATIC_LIGHT_BINDINGS \
  {"livingroom006", "36F98D", "Living Room"}, \
  {"bedroom006", "36AF6C", "Bedroom"}
