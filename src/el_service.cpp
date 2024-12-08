#include "el_service.h"
#include <Arduino.h>
#include "el_mqtt.h"

// #======================== Definitions ========================#

// #======================== Global Variables ========================#

Stream *serviceLogger;
bool serviceInitialized = false;

// #======================== Prototypes ========================#

int Service_init();
int Service_handle();
bool Service_isInitialized();

int service_log_print(String message);

// #======================== Initialization ========================#

int Service_init()
{
    service_log_print("[Service] Initializing ...\n");

    service_log_print("[Service] Initialized\n");
    serviceInitialized = true;
    return 0;
}

// #======================== Main ========================#

int Service_handle()
{
    return 0;
}

// #======================== Functions ========================#

bool Service_isInitialized()
{
    return serviceInitialized;
}

int Service_setLoggerOutput(Stream *s)
{
    serviceLogger = s;
    return 0;
}


// int mqtt_publish_light_state(String controller_id, String light_id, String state)
// {
//     String topic = "easylight/" + mqttClientID + "/controller/" + controller_id + "/light/" + light_id + "/state";
//     mqttClient.publish(topic.c_str(), state.c_str(), true, 2);
//     return 0;
// }

int Service_updateLightState(String controller_id, String light_id, String state)
{
    if (Mqtt_isInitialized() && Mqtt_isConnected())
    {
        Mqtt_publish("controller/" + controller_id + "/light/" + light_id + "/state", state);
    }
}

int service_log_print(String message)
{
    if (serviceLogger)
    {
        serviceLogger->print(message);
    }
    return 0;
}

// #======================== Callbacks ========================#

// #======================== Interrupt ========================#

// #======================== End ========================#
