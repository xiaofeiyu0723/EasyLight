#include <el_mqtt.h>
#include <Arduino.h>
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

MQTTClient mqttClient;
Stream *mqttLogger;
String mqttClientID = "el_" + String(ESP.getEfuseMac(), HEX); // Unique ID 'el_xx9fxxefxxc0'
bool mqttInitialized = false;

// #======================== Prototypes ========================#

int Mqtt_init();
int Mqtt_connect_blocking();
int Mqtt_disconnect();
int Mqtt_handle();
bool Mqtt_isInitialized();
int Mqtt_setCallback(MQTTClientCallbackSimple cb);
int Mqtt_setLoggerOutput(Stream *s);

int mqtt_log_print(String message);
int cb_mqttConnected();

int mqtt_publish_light_state(String controller_id, String light_id, String state);

// #======================== Initialization ========================#

int Mqtt_init()
{
    mqtt_log_print("[MQTT] Initializing ...\n");
    // check if wifi is initialized
    if (!Wifi_isInitialized())
    {
        mqtt_log_print("[MQTT] WiFi not initialized");
        return -1;
    }

    // TODO: Check if wifi is connected

    mqttClient.begin(MQTT_HOST, MQTT_PORT, *Wifi_getClient());

    mqtt_log_print("[MQTT] Initialized\n");
    mqttInitialized = true;
    return 0;
}

// #======================== Main ========================#

int Mqtt_handle()
{
    if (Wifi_isInitialized() && Wifi_isConnected() && !mqttClient.connected())
    {
        // Wifi is connected but MQTT is not connected
        mqtt_log_print("[MQTT] Reconnecting...\n");
        Mqtt_connect_blocking();
    }

    mqttClient.loop();

    return 0;
}

// #======================== Functions ========================#

bool Mqtt_isInitialized()
{
    return mqttInitialized;
}

int Mqtt_setCallback(MQTTClientCallbackSimple cb)
{
    mqttClient.onMessage(cb);
    return 0;
}

int Mqtt_setLoggerOutput(Stream *s)
{
    mqttLogger = s;
    return 0;
}

int Mqtt_connect_blocking()
{
    mqtt_log_print("[MQTT] Client ID: " + mqttClientID + "\n");
    // mqtt_log_print("[MQTT] Connecting to Host: [" + String(MQTT_HOST) + "] ");
    mqtt_log_print("[MQTT] Connecting ");

    mqttClient.setWill(("easylight/" + mqttClientID + "/$state").c_str(), "lost", true, 2); // LWT set brfore connect
    while (!mqttClient.connect(mqttClientID.c_str(), MQTT_USERNAME, MQTT_PASSWORD))
    {
        mqtt_log_print(".");
        delay(1000);
    }
    mqtt_log_print("\n");

    cb_mqttConnected();
    return 0;
}

int Mqtt_disconnect()
{
    mqttClient.publish(("easylight/" + mqttClientID + "/$state").c_str(), "disconnected", true, 2);
    mqttClient.disconnect();
    return 0;
}

int mqtt_log_print(String message)
{
    if (mqttLogger)
    {
        mqttLogger->print(message);
    }
    return 0;
}

// Pubublish
int mqtt_publish_light_state(String controller_id, String light_id, String state)
{
    String topic = "easylight/" + mqttClientID + "/controller/" + controller_id + "/light/" + light_id + "/state";
    mqttClient.publish(topic.c_str(), state.c_str(), true, 2);
    return 0;
}

// #======================== Callbacks ========================#

int cb_mqttConnected()
{
    mqtt_log_print("[MQTT] Connected!\n");
    mqttClient.publish(("easylight/" + mqttClientID + "/$state").c_str(), "init", true, 2);
    mqttClient.subscribe("hello");
    // TODO: Add more subscriptions here
    mqttClient.publish(("easylight/" + mqttClientID + "/$state").c_str(), "ready", true, 2);
    return 0;
}

// #======================== Interrupt ========================#

// #======================== End ========================#
