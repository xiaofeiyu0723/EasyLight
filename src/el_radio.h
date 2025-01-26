#ifndef EL_RADIO_H
#define EL_RADIO_H

#include <Arduino.h>

struct RequestToController{
    byte preamble;
    byte syncWord[3];

    byte msgType;
    byte length;
    byte fixedTBD_0[3];
    byte dynamicTBD;          // Not in CRC
    byte crc[2];              // Not in CRC
    byte fixedTBD_1[2];
    byte controllerID[3];
    byte command;
    byte param1;
    byte param2;
};

int Radio_init();
int Radio_handle();
int Radio_sendByte(byte *packet, size_t len);
int Radio_sendString(String str);
int Radio_sendRequestToController(String controllerID, byte msgType, byte dynamicTBD, byte command, byte param1 = 0x00, byte param2 = 0x00);
bool Radio_isInitialized();
int Radio_setReceiveCallback(void (*cb)(byte *packet, size_t len));
int Radio_setTransmitDoneCallback(void (*cb)());
int Radio_setLoggerOutput(Stream *s);

#endif // EL_RADIO_H