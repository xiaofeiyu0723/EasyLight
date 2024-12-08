#include "el_protocol.h"
#include <Arduino.h>

// #======================== Definitions ========================#

// #======================== Global Variables ========================#

Stream *Protocol_Logger;
bool protocol_initialized = false;

// #======================== Prototypes ========================#

int protocol_init();
int protocol_handle();
bool protocol_isInitialized();

int protocol_log_print(String message);

// #======================== Initialization ========================#

int protocol_init()
{
    protocol_log_print("[Protocol] Initializing ...\n");

    protocol_log_print("[Protocol] Initialized\n");
    protocol_initialized = true;
    return 0;
}

// #======================== Main ========================#

int protocol_handle()
{
    return 0;
}

// #======================== Functions ========================#

bool protocol_isInitialized()
{
    return protocol_initialized;
}

int protocol_setLoggerOutput(Stream *s)
{
    Protocol_Logger = s;
    return 0;
}

// private
int protocol_log_print(String message)
{
    if (Protocol_Logger)
    {
        Protocol_Logger->print(message);
    }
    return 0;
}

// #======================== Callbacks ========================#

// #======================== Interrupt ========================#

// #======================== End ========================#
