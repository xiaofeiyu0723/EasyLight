#include <WiFiClientSecure.h>
#include <MQTT.h>

#include "el_wifi.h"
#include "el_mqtt.h"

// #======================== Definitions ========================#

#define SERIAL_BAUD 115200

// #======================== Global Variables ========================#

unsigned long LastMillis = 0;

// #======================== Prototypes ========================#

void cb_mqttMessageReceived(String &topic, String &payload);

// #======================== Initialization ========================#

void setup()
{
  Serial.begin(SERIAL_BAUD);

  wifi_init();
  mqtt_init();
  mqtt_setCallback(cb_mqttMessageReceived);

  wifi_connect_blocking();
  mqtt_connect_blocking();
}

// #======================== Main Loop ========================#

void loop()
{
  mqtt_handle();
  delay(10); // <- fixes some issues with WiFi stability

  // if (!MQTT_Client.connected())
  // {
  //   startConnect();
  // }

  // publish a message roughly every second.
  // if (millis() - LastMillis > 3000)
  // {
  //   LastMillis = millis();
  //   MQTT_Client.publish("hello", "world");
  // }
}

// #======================== Functions ========================#

// #======================== Callbacks ========================#

void cb_mqttMessageReceived(String &topic, String &payload)
{
  Serial.println("[MQTT]incoming: " + topic + " - " + payload);

  // Note: Do not use the MQTT_Client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `MQTT_Client.loop()`.
}

// #======================== Interrupt ========================#

// #======================== End ========================#