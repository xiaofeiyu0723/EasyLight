#include "el_service.h"
#include <Arduino.h>

// #======================== Definitions ========================#

// #======================== Global Variables ========================#

Stream *Service_Logger;
bool service_initialized = false;

// #======================== Prototypes ========================#

int service_init();
int service_handle();
bool service_isInitialized();

int service_log_print(String message);

// #======================== Initialization ========================#

int service_init()
{
    service_log_print("[Service] Initializing ...\n");

    service_log_print("[Service] Initialized\n");
    service_initialized = true;
    return 0;
}

// #======================== Main ========================#

int service_handle()
{
    return 0;
}

// #======================== Functions ========================#

bool service_isInitialized()
{
    return service_initialized;
}

int service_setLoggerOutput(Stream *s)
{
    Service_Logger = s;
    return 0;
}

// private
int service_log_print(String message)
{
    if (Service_Logger)
    {
        Service_Logger->print(message);
    }
    return 0;
}

// #======================== Callbacks ========================#

// #======================== Interrupt ========================#

// #======================== End ========================#
