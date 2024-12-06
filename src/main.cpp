#include <WiFiClientSecure.h>
#include <MQTT.h>

// #======================== Definitions ========================#

#define SERIAL_BAUD 115200
#define EASYLIGHT_ENABLE_TLS // Enable TLS

// Use private network configuration
#include ".networkConfig/networkConfig.h" // <- Not included in the repository.

#ifndef USE_PRIVATE_NETWORK_CONFIG // -----
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#define MQTT_HOST "YOUR_MQTT_HOST"
#define MQTT_PORT 8883
#define MQTT_USERNAME "YOUR_MQTT_USERNAME"
#define MQTT_PASSWORD "YOUR_MQTT_PASSWORD"

#ifdef EASYLIGHT_ENABLE_TLS
const char *root_ca = R"literal(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh...
-----END CERTIFICATE-----
)literal";
#endif // EASYLIGHT_ENABLE_TLS
#endif // USE_PRIVATE_NETWORK_CONFIG ----- copy above to .networkConfig/networkConfig.h and fill in your data

// #======================== Global Variables ========================#

WiFiClientSecure wifi_client_s;
MQTTClient mqtt_client;

unsigned long lastMillis = 0;

// #======================== Prototypes ========================#

void connect();
void cb_mqttMessageReceived(String &topic, String &payload);

// #======================== Initialization ========================#

void setup()
{
  Serial.begin(SERIAL_BAUD);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  mqtt_client.begin(MQTT_HOST, MQTT_PORT, wifi_client_s);
  mqtt_client.onMessage(cb_mqttMessageReceived); // cb

  connect();
}

// #======================== Main Loop ========================#

void loop()
{
  mqtt_client.loop();
  delay(10); // <- fixes some issues with WiFi stability

  if (!mqtt_client.connected())
  {
    connect();
  }

  // publish a message roughly every second.
  if (millis() - lastMillis > 1000)
  {
    lastMillis = millis();
    mqtt_client.publish("/hello", "world");
  }
}

// #======================== Functions ========================#

void connect()
{
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }

#if defined(EASYLIGHT_ENABLE_TLS)
  wifi_client_s.setCACert(root_ca); // TLS
#endif

  char clientId[20];
  sprintf(clientId, "EL-%06X", ESP.getEfuseMac()); // The MAC address of the ESP32

  Serial.print("\nconnecting...");
  while (!mqtt_client.connect(clientId, MQTT_USERNAME, MQTT_PASSWORD))
  {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  mqtt_client.subscribe("/hello");
}

// #======================== Callbacks ========================#

void cb_mqttMessageReceived(String &topic, String &payload)
{
  Serial.println("incoming: " + topic + " - " + payload);

  // Note: Do not use the mqtt_client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `mqtt_client.loop()`.
}

// #======================== Interrupt ========================#

// #======================== End ========================#