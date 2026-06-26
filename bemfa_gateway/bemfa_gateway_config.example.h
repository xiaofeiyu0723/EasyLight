#pragma once

// Copy these values into .config/bemfa_gateway_config.h.
// Do not commit your real UID or WiFi password.

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Bemfa user private key / UID from the Bemfa console.
#define BEMFA_UID "YOUR_BEMFA_UID"

// Optional: required only for Bemfa batch topic creation APIs.
// Get these after real-name verification in the Bemfa console.
#define BEMFA_SECRET_ID "YOUR_BEMFA_SECRET_ID"
#define BEMFA_SECRET_KEY "YOUR_BEMFA_SECRET_KEY"

// Optional bootstrap binding for one known controller.
// Create this MQTT topic in Bemfa. Suffix 006 makes it a switch device.
// Set both values to "" if you want to use pairing-only discovery.
#define BEMFA_TOPIC "rfswitch006"

// Target controller ID, 3 bytes / 6 hex chars, for example "36F98D".
#define EASYLIGHT_CONTROLLER_ID "36F98D"

// Optional static mappings for manually created Bemfa topics.
// Fill with your own topic/controller pairs. Entries with an empty controller ID are ignored.
#define STATIC_LIGHT_BINDINGS \
  {"livingroom006", "36F98D"}, \
  {"bedroom006", "36AF6C"}

// GPIO0 is the BOOT button on many ESP32 dev boards.
#define PAIR_BUTTON_PIN 0

// Long-press duration to enter pairing mode.
#define PAIR_HOLD_MS 3000

// Pairing scan window after long press.
#define PAIR_WINDOW_MS 60000

// Maximum number of persisted controller/topic bindings.
#define MAX_LIGHT_BINDINGS 12
