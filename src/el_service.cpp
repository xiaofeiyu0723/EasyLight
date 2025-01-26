#include "el_service.h"
#include <Arduino.h>
#include "el_mqtt.h"
#include "el_radio.h"

// #======================== Definitions ========================#

// #======================== Global Variables ========================#

Stream *serviceLogger;
bool serviceInitialized = false;

// #======================== Prototypes ========================#

int Service_init();
int Service_handle();
bool Service_isInitialized();

int Service_mqttReportControllerResponse(String controller_id, String req_code, String res_value, String type);
int Service_radioSendControllerStateQuery(String controller_id);

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

int Service_mqttReportControllerResponse(String controller_id, String req_code, String res_value, String type)
{
    if (Mqtt_isInitialized() && Mqtt_isConnected())
    {
        Mqtt_publish("controller/" + controller_id + "/response", req_code + " " + res_value + " " + type);
    }
    return 0;
}

int Service_radioSendControllerStateQuery(String controller_id)
{
    if (controller_id.length() != 6)
    {
        return -1;
    }

    if (Radio_isInitialized())
    {
        Radio_sendRequestToController(controller_id, 0x03, 0x2A, 0X05, 0x00);
    }
    return 0;
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
