#include "el_protocol.h"
#include <Arduino.h>

// #======================== Definitions ========================#

// #======================== Global Variables ========================#

Stream *protocolLogger;
bool protocolInitialized = false;

// #======================== Prototypes ========================#

int Protocol_init();
int Protocol_handle();
bool Protocol_isInitialized();

int protocol_log_print(String message);

// #======================== Initialization ========================#

int Protocol_init()
{
    protocol_log_print("[Protocol] Initializing ...\n");

    protocol_log_print("[Protocol] Initialized\n");
    protocolInitialized = true;
    return 0;
}

// #======================== Main ========================#

int Protocol_handle()
{
    return 0;
}

// #======================== Functions ========================#

bool Protocol_isInitialized()
{
    return protocolInitialized;
}

int Protocol_setLoggerOutput(Stream *s)
{
    protocolLogger = s;
    return 0;
}

int protocol_log_print(String message)
{
    if (protocolLogger)
    {
        protocolLogger->print(message);
    }
    return 0;
}

// #======================== Callbacks ========================#

// #======================== Interrupt ========================#

// #======================== End ========================#
