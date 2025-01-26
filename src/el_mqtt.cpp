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
String mqttClientID = "mqtt_" + String(ESP.getEfuseMac(), HEX); // Unique ID 'mqtt_xx9fxxefxxc0'
String mqttRootTopic = "mqtt/" + mqttClientID;
bool mqttInitialized = false;
MQTTClientCallbackSimple mqttCallback = NULL;

// #======================== Prototypes ========================#

int Mqtt_init();
int Mqtt_connect_blocking();
int Mqtt_disconnect();
int Mqtt_handle();
int Mqtt_publish(String topic, String payload, bool retained, int qos);
bool Mqtt_isInitialized();
bool Mqtt_isConnected();
MQTTClient *Mqtt_getClient();
int Mqtt_setClientID(String id);
int Mqtt_setRootTopic(String topic);
int Mqtt_setCallback(MQTTClientCallbackSimple cb);
int Mqtt_setLoggerOutput(Stream *s);

int mqtt_log_print(String message);
int mqtt_connectedCallback();
void mqtt_messageReceivedCallback(String &topic, String &payload);

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
    mqttClient.onMessage(mqtt_messageReceivedCallback);

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
    mqttCallback = cb;
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

    mqttClient.setWill((mqttRootTopic + "/$state").c_str(), "lost", true, 2); // LWT set brfore connect
    while (!mqttClient.connect(mqttClientID.c_str(), MQTT_USERNAME, MQTT_PASSWORD))
    {
        mqtt_log_print(".");
        delay(1000);
    }
    mqtt_log_print("\n");

    mqtt_connectedCallback();
    return 0;
}

int Mqtt_disconnect()
{
    mqttClient.publish((mqttRootTopic + "/$state").c_str(), "disconnected", true, 2);
    mqttClient.disconnect();
    return 0;
}

int Mqtt_publish(String topic, String payload, bool retained, int qos)
{
    if (mqttClient.connected())
    {
        mqttClient.publish((mqttRootTopic + "/" + topic).c_str(), payload.c_str(), retained, qos);
    }
    return 0;
}

bool Mqtt_isConnected()
{
    return mqttClient.connected();
}

MQTTClient *Mqtt_getClient()
{
    return &mqttClient;
}

int Mqtt_setClientID(String id)
{
    mqttClientID = id;
    return 0;
}

int Mqtt_setRootTopic(String topic) // DO NOT ADD '/' at the end
{
    mqttRootTopic = topic;
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

// #======================== Callbacks ========================#

int mqtt_connectedCallback()
{
    mqtt_log_print("[MQTT] Connected!\n");
    mqttClient.publish((mqttRootTopic + "/$state").c_str(), "init", true, 2);
    mqttClient.subscribe((mqttRootTopic + "/test").c_str(), 2);
    // TODO: Add more subscriptions here
    mqttClient.publish((mqttRootTopic + "/$state").c_str(), "ready", true, 2);
    return 0;
}

void mqtt_messageReceivedCallback(String &topic, String &payload){

    mqtt_log_print("[MQTT] Receive: " + topic + " - " + payload + "\n");
    mqtt_log_print("[MQTT] Root: " + mqttRootTopic + "\n");

    //validate and remove root topic here
    if(topic.startsWith(mqttRootTopic)){
        topic = topic.substring(mqttRootTopic.length() + 1);
    }else {
        return;
    }

    mqtt_log_print("[MQTT] SubTopic: " + topic + "\n");

    // Call the callback
    if (mqttCallback)
    {
        mqttCallback(topic, payload);
    }
}


// #======================== Interrupt ========================#

// #======================== End ========================#
