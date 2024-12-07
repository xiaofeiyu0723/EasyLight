#include <el_mqtt.h>
#include <el_wifi.h>
#include <MQTT.h>

// #======================== Definitions ========================#

// Use private network configuration
#include ".networkConfig.h" // <- Not included in the repository.

#ifndef USE_PRIVATE_NETWORK_CONFIG
#define MQTT_HOST "YOUR_MQTT_HOST"
#define MQTT_PORT 8883
#define MQTT_USERNAME "YOUR_MQTT_USERNAME"
#define MQTT_PASSWORD "YOUR_MQTT_PASSWORD"
#endif // USE_PRIVATE_NETWORK_CONFIG

// #======================== Global Variables ========================#

MQTTClient MQTT_Client;
String Client_ID = "el_" + String(ESP.getEfuseMac(), HEX); // Unique ID 'el_xx9fxxefxxc0'

// #======================== Prototypes ========================#

int mqtt_init();
int mqtt_connect_blocking();
int mqtt_disconnect();
int mqtt_setCallback(MQTTClientCallbackSimple cb);

int cb_mqttConnected();

// #======================== Initialization ========================#

int mqtt_init()
{
    // check if wifi is initialized
    if (!wifi_isInitialized())
    {
        Serial.println("[MQTT] WiFi not initialized!");
        return -1;
    }

    // TODO: Check if wifi is connected

    MQTT_Client.begin(MQTT_HOST, MQTT_PORT, *wifi_getClient());
    return 0;
}

// #======================== Main ========================#

int mqtt_handle()
{
    MQTT_Client.loop();
    return 0;
}

// #======================== Functions ========================#

int mqtt_setCallback(MQTTClientCallbackSimple cb)
{
    MQTT_Client.onMessage(cb);
    return 0;
}

int mqtt_connect_blocking()
{
    Serial.println("[MQTT] Client ID: " + Client_ID);
    // Serial.print("[MQTT] Connecting to Host: [" + String(MQTT_HOST) + "] ");
    Serial.print("[MQTT] Connecting ");

    MQTT_Client.setWill(("easylight/" + Client_ID + "/$state").c_str(), "lost", true, 2); // LWT set brfore connect
    while (!MQTT_Client.connect(Client_ID.c_str(), MQTT_USERNAME, MQTT_PASSWORD))
    {
        Serial.print(".");
        delay(1000);
    }

    cb_mqttConnected();
    return 0;
}

int mqtt_disconnect()
{
    MQTT_Client.publish(("easylight/" + Client_ID + "/$state").c_str(), "disconnected", true, 2);
    MQTT_Client.disconnect();
    return 0;
}

// #======================== Callbacks ========================#

int cb_mqttConnected()
{
    MQTT_Client.publish(("easylight/" + Client_ID + "/$state").c_str(), "init", true, 2);
    MQTT_Client.subscribe("hello");
    // TODO: Add more subscriptions here
    MQTT_Client.publish(("easylight/" + Client_ID + "/$state").c_str(), "ready", true, 2);
    return 0;
}

// #======================== Interrupt ========================#

// #======================== End ========================#



