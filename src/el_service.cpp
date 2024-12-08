#include "el_service.h"
#include <Arduino.h>

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
