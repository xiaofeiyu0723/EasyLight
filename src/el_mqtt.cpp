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
Stream *MQTT_Logger;
String Client_ID = "el_" + String(ESP.getEfuseMac(), HEX); // Unique ID 'el_xx9fxxefxxc0'
bool mqtt_initialized = false;

// #======================== Prototypes ========================#

int mqtt_init();
int mqtt_connect_blocking();
int mqtt_disconnect();

int mqtt_handle();

bool mqtt_isInitialized();

int mqtt_setCallback(MQTTClientCallbackSimple cb);
int mqtt_setLoggerOutput(Stream *s);

// Pubublish
int mqtt_publish_light_state(String controller_id, String light_id, String state);

// private
int mqtt_log_print(String message);
int cb_mqttConnected();

// #======================== Initialization ========================#

int mqtt_init()
{
    mqtt_log_print("[MQTT] Initializing ...\n");
    // check if wifi is initialized
    if (!wifi_isInitialized())
    {
        mqtt_log_print("[MQTT] WiFi not initialized");
        return -1;
    }

    // TODO: Check if wifi is connected

    MQTT_Client.begin(MQTT_HOST, MQTT_PORT, *wifi_getClient());

    mqtt_log_print("[MQTT] Initialized\n");
    mqtt_initialized = true;
    return 0;
}

// #======================== Main ========================#

int mqtt_handle()
{
    if (wifi_isInitialized() && wifi_isConnected() && !MQTT_Client.connected())
    {
        // Wifi is connected but MQTT is not connected
        mqtt_log_print("[MQTT] Reconnecting...\n");
        mqtt_connect_blocking();
    }

    MQTT_Client.loop();

    return 0;
}

// #======================== Functions ========================#

bool mqtt_isInitialized()
{
    return mqtt_initialized;
}

int mqtt_setCallback(MQTTClientCallbackSimple cb)
{
    MQTT_Client.onMessage(cb);
    return 0;
}

int mqtt_setLoggerOutput(Stream *s)
{
    MQTT_Logger = s;
    return 0;
}

int mqtt_connect_blocking()
{
    mqtt_log_print("[MQTT] Client ID: " + Client_ID + "\n");
    // mqtt_log_print("[MQTT] Connecting to Host: [" + String(MQTT_HOST) + "] ");
    mqtt_log_print("[MQTT] Connecting ");

    MQTT_Client.setWill(("easylight/" + Client_ID + "/$state").c_str(), "lost", true, 2); // LWT set brfore connect
    while (!MQTT_Client.connect(Client_ID.c_str(), MQTT_USERNAME, MQTT_PASSWORD))
    {
        mqtt_log_print(".");
        delay(1000);
    }
    mqtt_log_print("\n");

    cb_mqttConnected();
    return 0;
}

int mqtt_disconnect()
{
    MQTT_Client.publish(("easylight/" + Client_ID + "/$state").c_str(), "disconnected", true, 2);
    MQTT_Client.disconnect();
    return 0;
}

// Pubublish
int mqtt_publish_light_state(String controller_id, String light_id, String state)
{
    String topic = "easylight/" + Client_ID + "/controller/" + controller_id + "/light/" + light_id + "/state";
    MQTT_Client.publish(topic.c_str(), state.c_str(), true, 2);
    return 0;
}

// private
int mqtt_log_print(String message)
{
    if (MQTT_Logger)
    {
        MQTT_Logger->print(message);
    }
    return 0;
}

// #======================== Callbacks ========================#

int cb_mqttConnected()
{
    mqtt_log_print("[MQTT] Connected!\n");
    MQTT_Client.publish(("easylight/" + Client_ID + "/$state").c_str(), "init", true, 2);
    MQTT_Client.subscribe("hello");
    // TODO: Add more subscriptions here
    MQTT_Client.publish(("easylight/" + Client_ID + "/$state").c_str(), "ready", true, 2);
    return 0;
}

// #======================== Interrupt ========================#

// #======================== End ========================#
