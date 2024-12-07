#include <Arduino.h>
#include <RadioLib.h>

// #======================== Definitions ========================#

// #======================== Global Variables ========================#

Stream *Radio_Logger;
bool radio_initialized = false;

// #======================== Prototypes ========================#

int radio_init();
int radio_handle();

bool radio_isInitialized();

int radio_setLoggerOutput(Stream *s);

// private
int radio_log_print(String message);

// #======================== Initialization ========================#

int radio_init()
{
    radio_initialized = true;
    return 0;
}

// #======================== Main ========================#

int radio_handle()
{
    return 0;
}

// #======================== Functions ========================#

bool radio_isInitialized()
{
    return radio_initialized;
}


int radio_setLoggerOutput(Stream *s)
{
    Radio_Logger = s;
    return 0;
}

// private
int radio_log_print(String message)
{
    if (Radio_Logger)
    {
        Radio_Logger->print(message);
    }
    return 0;
}

// #======================== Callbacks ========================#

// #======================== Interrupt ========================#

// #======================== End ========================#
