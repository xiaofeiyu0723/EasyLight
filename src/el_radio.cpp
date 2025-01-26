#include "el_radio.h"
#include <Arduino.h>
#include <RadioLib.h>
#include <AceCRC.h>

// #======================== Definitions ========================#

using namespace ace_crc::crc16ccitt_byte; // Select CRC algorithm

// For ESP32 ONLY !!!
#define PIN_CS 5
#define PIN_GDO0 21
#define PIN_RST RADIOLIB_NC
#define PIN_GDO2 22

/*
// For ESP32-C3
#define PIN_CS 7
#define PIN_GDO0 3
#define PIN_RST RADIOLIB_NC
#define PIN_GDO2 2
*/

// RF Configurations
#define RADIO_CARRIER_FREQUENCY 433.3   // MHz
#define RADIO_BIT_RATE 250.0            // kbps
#define RADIO_FREQUENCY_DEVIATION 125.0 // kHz
#define RADIO_RX_BANDWIDTH 270.0        // kHz
#define RADIO_OUTPUT_POWER 10           // dBm
#define RADIO_PREAMBLE_LENGTH 32        // bits

// Packet
#define PACKET_RSSI_THRESHOLD -100  // dBm (RSSI threshold for packet reception)
#define PACKET_LENGTH 24            // The Bytes you want to receive (after the sync word)
uint8_t SYNC_WORD[] = {0x21, 0xA4}; // The [21 A4] would be better, [A4 23] is not working, I don't know why
uint8_t SYNC_WORD_LENGTH = 2;
#define PACKET_THIRD_SYNC_WORD 0x23

// #======================== Global Variables ========================#

CC1101 radio = new Module(PIN_CS, PIN_GDO0, PIN_RST, PIN_GDO2);
Stream *radioLogger;
bool radioInitialized = false;

// Interrupts flags
volatile bool receivedFlag = false; // flag to indicate that a packet was received
volatile bool transmitFlag = false; // flag to indicate that a packet was transmitted
int transmissionState = RADIOLIB_ERR_NONE;

void (*radioReceiveCallback)(byte *packet, size_t len);
void (*radioTransmitDoneCallback)();

// #======================== Prototypes ========================#

int Radio_init();
int Radio_handle();
int Radio_sendByte(byte *packet, size_t len);
int Radio_sendString(String str);
int Radio_sendRequestToController(String controllerID, byte msgType, byte dynamicTBD, byte command, byte param1, byte param2);
bool Radio_isInitialized();
int Radio_setReceiveCallback(void (*cb)(byte *packet, size_t len));
int Radio_setLoggerOutput(Stream *s);

int radio_log_print(String message);
int radio_message_validate(byte *packet, size_t len);

// interrupt
ICACHE_RAM_ATTR void setReceivedFlag(void);
ICACHE_RAM_ATTR void setTransmitFlag(void);

// #======================== Initialization ========================#

int Radio_init()
{
    radio_log_print("[Radio] Initializing ...\n");
    radio.reset();
    int state = radio.begin(RADIO_CARRIER_FREQUENCY, RADIO_BIT_RATE, RADIO_FREQUENCY_DEVIATION, RADIO_RX_BANDWIDTH, RADIO_OUTPUT_POWER, RADIO_PREAMBLE_LENGTH);
    radio.setCrcFiltering(false);
    radio.fixedPacketLengthMode(PACKET_LENGTH);
    radio.setSyncWord(SYNC_WORD, SYNC_WORD_LENGTH);

    if (state != RADIOLIB_ERR_NONE)
    {
        radio_log_print("[Radio] initialization failed: " + String(state) + "\n");
        return -1;
    }

    radio.setPacketReceivedAction(setReceivedFlag); // Callback on packet received
    radio.setPacketSentAction(setTransmitFlag);     // Callback on packet sent

    // TODO: Manually start the receiver
    radio_log_print("[Radio] Starting receiver ...\n");
    state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE)
    {
        radio_log_print("[Radio] Start receive failed: " + String(state) + "\n");
        return -1;
    }

    // TODO: Figure out why there is a deadlock.
    //radio.startTransmit("Hello World", 11); // Must send something here to avoid deadlock, otherwise the transmitter always call TransmitDoneCallback (I don't know why)

    radio_log_print("[Radio] Initialized\n");
    radioInitialized = true;
    return 0;
}

// #======================== Main ========================#

int Radio_handle()
{
    // Transmit
    if (transmitFlag)
    {
        transmitFlag = false;
        radio.finishTransmit(); // Must be called after
        if (transmissionState == RADIOLIB_ERR_NONE)
        {
            if (radioTransmitDoneCallback)
            {
                radioTransmitDoneCallback();
            }
        }
        else
        {
            radio_log_print("[Radio] Packet transmit failed: " + String(transmissionState) + "\n");
        }

        radio.startReceive(); // Restart the receiver !important!
    }

    // Receive
    if (receivedFlag)
    {
        receivedFlag = false;
        byte packet[PACKET_LENGTH];
        int state = radio.readData(packet, PACKET_LENGTH);

        if (state == RADIOLIB_ERR_NONE)
        {
            // Print the raw received packet

            radio_log_print("[Radio] Received: ");
            for (int i = 0; i < PACKET_LENGTH; i++)
            {
                radio_log_print(String(packet[i], HEX) + " ");
            }
            radio_log_print("\n");

            if (packet[0] == PACKET_THIRD_SYNC_WORD) // if not fixedPacketLengthMode(), the 0x23 will be ignored
            {

                // Validate the packet, and extract the effective part
                byte effectivePacket[PACKET_LENGTH];
                int effectivePacketLength = radio_message_validate(packet, PACKET_LENGTH);
                if (effectivePacketLength > 0)
                {
                    for (int i = 1; i <= effectivePacketLength; i++)
                    {
                        effectivePacket[i - 1] = packet[i];
                    }
                }

                // If the packet is valid, call the callback
                if (radioReceiveCallback && effectivePacketLength > 0)
                {
                    radioReceiveCallback(effectivePacket, effectivePacketLength);
                }
            }
        }
        else
        {
            radio_log_print("[Radio] Packet receive failed: " + String(state) + "\n");
        }

        radio.startReceive(); // Restart the receiver !important!
    }


    return 0;
}

// #======================== Functions ========================#

int Radio_sendByte(byte *packet, size_t len)
{
    // transmissionState = radio.startTransmit(packet, len);
    int state = radio.transmit(packet, len);
    radio_log_print("[Radio] Block sending, state: " + String(state) + "\n");
    radio.standby();
    return 0;
}

int Radio_sendString(String str)
{
    // transmissionState = radio.startTransmit(str);
    return 0;
}

int Radio_sendRequestToController(String controllerID, byte msgType, byte dynamicTBD, byte command, byte param1, byte param2)
{
    // Convert the controllerID to byte array
    byte controllerIDBytes[3];
    for (int i = 0; i < 3; i++)
    {
        controllerIDBytes[i] = (byte)strtol(controllerID.substring(i * 2, i * 2 + 2).c_str(), NULL, 16);
    }

    RequestToController req;
    req.preamble = 0x54;

    req.syncWord[0] = 0x21;
    req.syncWord[1] = 0xA4;
    req.syncWord[2] = 0x23;

    req.msgType = msgType;

    // req.length = 0x00;
    req.length = 0x0F;

    req.fixedTBD_0[0] = 0x3A;
    req.fixedTBD_0[1] = 0x96;
    req.fixedTBD_0[2] = 0x9D;

    req.dynamicTBD = dynamicTBD;

    // req.crc[0] = 0x00;
    // req.crc[1] = 0x00;

    req.fixedTBD_1[0] = 0x9B;
    req.fixedTBD_1[1] = 0x00;

    req.controllerID[0] = controllerIDBytes[0];
    req.controllerID[1] = controllerIDBytes[1];
    req.controllerID[2] = controllerIDBytes[2];

    req.command = command;
    req.param1 = param1;
    req.param2 = param2;

    // Place CRC
    byte payload[] = {
        req.msgType, req.length,
        req.fixedTBD_0[0], req.fixedTBD_0[1], req.fixedTBD_0[2],
        req.fixedTBD_1[0], req.fixedTBD_1[1],
        req.controllerID[0], req.controllerID[1], req.controllerID[2],
        req.command, req.param1}; // req.param2

    crc_t crc = crc_init();
    crc = crc_update(crc, payload, sizeof(payload));
    crc = crc_finalize(crc);
    req.crc[0] = (crc >> 8) & 0xFF;
    req.crc[1] = crc & 0xFF;

    byte packet[sizeof(req)];
    memcpy(packet, &req, sizeof(req));

    // Debug
    radio_log_print("[Radio] Sending Request to Controller: ");
    for (int i = 0; i < sizeof(req); i++)
    {
        radio_log_print(String(packet[i], HEX) + " ");
    }
    radio_log_print("\n");

    return Radio_sendByte(packet, sizeof(packet));
}

bool Radio_isInitialized()
{
    return radioInitialized;
}

int Radio_setReceiveCallback(void (*cb)(byte *packet, size_t len))
{
    radioReceiveCallback = cb;
    return 0;
}

int Radio_setTransmitDoneCallback(void (*cb)())
{
    radioTransmitDoneCallback = cb;
    return 0;
}

int Radio_setLoggerOutput(Stream *s)
{
    radioLogger = s;
    return 0;
}

int radio_log_print(String message)
{
    if (radioLogger)
    {
        radioLogger->print(message);
    }
    return 0;
}

int radio_message_validate(byte *packet, size_t len)
{
    byte effectivePacket[PACKET_LENGTH];
    uint8_t effectivePacketLength = PACKET_LENGTH;

    // Different types of CRC validation
    byte firstByte = packet[1];
    if (firstByte == 0x01 || firstByte == 0x02 || firstByte == 0x04)
    {
        // 23 01 0C 00 12 34 C6 A3 86 56 01 01 41
        //    __ __ __ __ __          __ __ __ __
        effectivePacketLength = packet[2];
        uint8_t payloadLength = effectivePacketLength - 3; // used to calculate the CRC

        // radio_log_print("[Radio] efPkt Length: " + String(effectivePacketLength) + "\n");
        // radio_log_print("[Radio] efPkt: ");
        // for (int i = 0; i < effectivePacketLength; i++)
        // {
        //     radio_log_print(String(packet[i + 1], HEX) + " ");
        // }
        // radio_log_print("\n");

        // Get the payload, two parts [1-5],[9-length]
        byte msg_payload[PACKET_LENGTH];
        for (int i = 1; i <= 5; i++) // [1,2,3,4,5]
        {
            msg_payload[i - 1] = packet[i];
        }
        for (int i = 9; i <= effectivePacketLength; i++) // [9,10,11,efPktLength]
        {
            msg_payload[i - 4] = packet[i];
        }

        // radio_log_print("[Radio] Payload: ");
        // for (int i = 0; i < payloadLength; i++)
        // {
        //     radio_log_print(String(msg_payload[i], HEX) + " ");
        // }
        // radio_log_print("\n");

        crc_t crc = crc_init();
        crc = crc_update(crc, msg_payload, payloadLength);
        crc = crc_finalize(crc);
        unsigned long messageCRC = (packet[7] << 8) | packet[8];
        // radio_log_print("[Radio] CRC: " + String(crc) + " Message CRC: " + String(messageCRC) + "\n");
        if ((unsigned long)crc != messageCRC)
        {
            return 0;
        }
    }
    else
    {
        // maybe switch packet
        byte switch_payload[] = {packet[1], packet[2], packet[3], packet[4], packet[5]}; // SwitchID 4 + ButtonID 1

        crc_t crc = crc_init();
        crc = crc_update(crc, switch_payload, 5);
        crc = crc_finalize(crc);
        unsigned long messageCRC = (packet[6] << 8) | packet[7];
        effectivePacketLength = 7; // 4 + 1 + 2

        if ((unsigned long)crc != messageCRC)
        {
            return 0;
        }
    }

    return effectivePacketLength;
}

// #======================== Callbacks ========================#

// #======================== Interrupt ========================#

ICACHE_RAM_ATTR void setReceivedFlag(void)
{
    receivedFlag = true;
}

ICACHE_RAM_ATTR void setTransmitFlag(void)
{
    transmitFlag = true;
}

// #======================== End ========================#
