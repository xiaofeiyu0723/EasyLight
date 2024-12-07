#include <WiFiClientSecure.h>
#include <MQTT.h>

#include "el_wifi.h"
#include "el_mqtt.h"

// #======================== Definitions ========================#

#define SERIAL_BAUD 115200

// #======================== Global Variables ========================#

// #======================== Prototypes ========================#

void cb_mqttMessageReceived(String &topic, String &payload);

// #======================== Initialization ========================#

void setup()
{
  Serial.begin(SERIAL_BAUD);

  wifi_init();
  wifi_setLoggerOutput(&Serial);

  mqtt_init();
  mqtt_setLoggerOutput(&Serial);
  mqtt_setCallback(cb_mqttMessageReceived);

  // wifi_connect_blocking();
  // mqtt_connect_blocking();
}

// #======================== Main Loop ========================#

void loop()
{
  wifi_handle();
  mqtt_handle();
  // delay(10); // <- fixes some issues with WiFi stability
}

// #======================== Functions ========================#

// #======================== Callbacks ========================#

void cb_mqttMessageReceived(String &topic, String &payload)
{
  Serial.println("MQTT Receive: " + topic + " - " + payload);

  // Note: Do not use the MQTT_Client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `MQTT_Client.loop()`.
}

// #======================== Interrupt ========================#

// #======================== End ========================#