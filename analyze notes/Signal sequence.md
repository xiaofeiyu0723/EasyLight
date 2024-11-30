# Binary sequence analysis of signals
Editor: Lan HUANG    Nov 2024

Collector: Xiaofei YU

## Signal types
### Kinetic Switch 🕹️
Main Components:
- switch_ID (24 bits)
- button_ID (8 bits)

Signal Sequence:
```
// A packet sent from a kinetic switch to the controller(light)
HEX: 54 21 A4 23 01 02 03 04 04 00 00

Premable:     1010.... 01010100
Sync Word:    00100001 10100100 00100011
SwitchID:     00000001 00000010 00000011 00000100
ButtonID:     00000100
CRC:          00000000 00000000
```
- Premable (32 bits?)
  - fixed 0x55, ..., 0x55, 0x54
- Sync Word (24 bits?)
  - fixed 0x21, 0x24, 0x23
- Data (40 bits)
  - switch_ID 0x01, 0x02, 0x03, 0x04
  - button_ID 0x05
    - One key switch:
      - Press: 0x04 = 00000100
      - Release: 0xC0 = 11000000
    - Two key switch:
      - Left Press: 0x00 = 00000000
      - Right Press: 0x02 = 00000010
      - Release: 0xC0 = 11000000
    - Three key switch:
      - Left Press: 0x01 = 00000001
      - Middle Press: 0x04 = 00000100
      - Right Press: 0x02 = 00000010
      - Release: 0xC0 = 11000000

- CRC (16 bits)
  - Algorithm: CRC-16/SPI-FUJITSU
  - Data Segment: switch_ID, button_ID (40 bits)




Minimum Effective Signal Length (When replaying): 
11 bytes
```
0x54,0x21,0xA4,0x23, 0x01,0x02,0x03,0x04, 0x04, 0x00, 0x00
```
### Controller (in light) 💡

Main Components:
- Device Type (8 bits)
- controller_ID (24 bits)

Signal Sequence:
```
// A packet sent from controller when receiving a signal from a kinetic switch(sometime)
HEX: 01 0C 00 36 F9 C6 90 B7 D8 01 00 41

Premable:       1010.... 01010100
Sync Word:      00100001 10100100 00100011
Device Type     00000001
Length:         00001100
TBD:            00000000
ControllerID_1: 00110110 11111001 = 0x36F9
TBD:            11000110 10010000 10110111 (CRC?)
ControllerID_2: 10001101 = 0xD8
TBD:            00000001 (parameter?)
Status:         00000000
TBD:            01000001 (tail?)
```

- Premable (32 bits?)
  - fixed 0x55, ..., 0x55, 0x54
- Sync Word (24 bits?)
  - fixed 0x21, 0x24, 0x23
- Data (12 bytes)
  - type 0x01
    - controller=0x01
    - gateway=0x03
  - TBD 0x0C (length?)
  - TBD 0X00
  - controller_ID part 1 0x36F9
  - TBD 0XC6 90 B7 (CRC?)
  - controller_ID part 2 0xD8
  - TBD 0x01 (parameter?)
  - status 0x00 (different when on/off)
  - TBD 0x41/0x81 (tail?)



### Gateway 🌐

Main Components:
- Device Type (8 bits)
- gateway_ID?
- operation type (8 bits)
- controller_ID (from controller signal 24 bits)
- command
- parameter
- status

Signal Sequence:
```
// A packet sent from gateway to controller to turn on the light
Premable:     1010.... 01010100
Sync Word:    00100001 10100100 00100011
Device Type   00000011
Length:       00010000
TBD:          00111010 100101101 0011101 (gateway_ID?)
Operation:    11100111
CRC:          10111101 00100111
TBD:          10011011 00000000
ControllerID: 00110110 10101111 01101100
Command:      00000100
Parameter:    00000010
Status:       00000001
```
- Premable (32 bits?)
  - fixed 0x55, ..., 0x55, 0x54
- Sync Word (24 bits?)
  - fixed 0x21, 0x24, 0x23
- Data (14/16 bytes)
  - type 0x03 
    - controller=0x01
    - gateway=0x03
  - length 0x00
  - TBD 0x00 0x00 0x00
  - operation type 0x01
    - on/off: 11100111 = 0xE7
    - add/delete: 01100000 = 0x60
  - CRC16-SPI-FUJITSU 0x00 0x00
  - TBD 0x0000 (gateway_ID?)
  - controller_ID 0x36 F9 D8
  - command 0x01
    - on/off: 00000100 = 0x04
    - add: 00000001 = 0x01
    - remove: 00000010 = 0x02
  - parameter 0x00 (only on/off)
    - TBD: 00000010
  - status/value 0x00 (different when on/off)
    - on: 00000001 = 0x01
    - off: 00000000 = 0x00


# Others Test Notes

```
Try different error code rates in preamble and sync word
// 54 = 0101 0100 YES
// 14 = 0001 0100 YES
// 24 = 0011 0100 NO
// 04 = 0000 0100 NO
// C4 = 1100 0100 NO
// D4 = 1101 0100 NO

-------------------------------------------------
Different Count of Keys
-------------------------------------------------
One Key Switch
// Press  00000100 = 4

Three Keys Switch
// Left   00000001 = 1
// Middle 00000100 = 4
// Right  00000010 = 2

Two Keys Switch
// Left   00000000 = 0
// Right  00000010 = 2

-------------------------------------------------
Kinetic Switch Signal and Controller Response
-------------------------------------------------

$$$$$$$$$$$$$$$$$$$$$$ Press $$$$$$$$$$$$$$$$$$$$$$

Kinetic Switch SENT PACKET:
//23 AA BB CC DD 04 00 00 (PRESS 7 bytes)
//   __ __ __ __ ++ [   ]
//**                ** ** (CRC from 5 bytes)

Controller Response:
//23 01 0C 00 36 F9 C6 A3 86 D8 01 01 41 (RES_ON 12 bytes)
//23 01 0C 00 36 F9 C6 90 B7 D8 01 00 41 (RES_OFF 12 bytes)
//            __ __    [   ] __    ==
//**                ** ** **             (CRC from 9 bytes)

Other switch example
//23 01 0C 00 36 E6 27 90 69 08 01 00 41 
//            __ __    [   ] __    ==

//23 01 0C 00 37 40 C1 5A 66 71 01 00 81 (why 81?)
//23 01 0C 00 37 40 C1 5A 66 71 01 00 81
//23 01 0C 00 37 40 C1 69 57 71 01 01 81
//            __ __    [   ] __    ==

//23 01 0C 00 37 07 F9 31 43 8B 01 03 41 (error)
//23 01 0C 00 37 07 F9 02 70 8B 01 00 41
//            __ __    [   ] __    ==

Resp Structure:
//   CC LL ?? AA AA ?? RR RR AA MM VV ??
- CC: Message Type?
- LL: Length
- AA: Controller ID (24 bits)
- RR: CRC16-SPI-FUJITSU (16 bits)
- MM: Operation Type (8 bits)
- VV: Value (8 bits)

-------------------------------------------------
Gateway Signal and Controller Response
-------------------------------------------------

$$$$$$$$$$$$$$$$$$$$$$ ON/OFF(04) $$$$$$$$$$$$$$$$$$$$$$

Gateway SENT PACKET:
//23 03 10 3A 96 9D E7 00 00 9B 00 36 F9 D8 04 02 01 (ON 16 bytes)
//         __ __ __                __ __ __ ++ ++ ++
//**                ** ** **                         (CRC from 13 bytes)
//   CC LL GG GG GG ?? RR RR ?? ?? AA AA AA MM QQ VV
//                                             00error
//                                             01ok
//                                             02ok
//                                             03error
//                                             04error

Controller Response:
//23 04 0B 00 36 F9 B8 BD E5 D8 04 00 (RES_ON 11 bytes)
//23 04 0B 00 36 F9 B8 BD E5 D8 04 00 (RES_ON 11 bytes)
//                     [   ]    ==
//**                ** ** **          (CRC from 8 bytes)

//23 04 0B 00 36 F9 B8 9D A7 8D 04 02 (REJECT 11 bytes)!!!!

$$$$$$$$$$$$$$$$$$$$$$ ADD(01)/DELETE(02) $$$$$$$$$$$$$$$$$$$$$$

Gateway SENT PACKET:
//23 03 0E 3A 96 9D 60 00 00 9B 00 36 F9 D8 01 (ADD 14 bytes)
//         __ __ __                __ __ __ ++

Controller Response:
//23 04 0B 00 36 F9 B8 42 10 8D 01 00 (RES_SUCCESS     11 bytes) add always success
//23 04 0B 00 36 F9 B8 17 43 8D 02 00 (RES_DEL_SUCCESS 11 bytes)
//23 04 0B 00 36 F9 B8 37 01 8D 02 02 (REJECT          11 bytes) !!!!
because we already delete this gateway in the controller, so the next add will be rejected

$$$$$$$$$$$$$$$$$$$$$$ RESET(03) $$$$$$$$$$$$$$$$$$$$$$

Gateway SENT PACKET:
//23 03 0E 3A 96 9D 60 00 00 9B 00 36 F9 D8 03 (ADD 14 bytes)

Controller Response:
//23 04 0B 00 36 F9 B8 24 72 8D 03 00 (RES_SUCCESS 11 bytes)
//23 04 0B 00 36 F9 B8 04 30 8D 03 02 (REJECT       11 bytes)!!!! 
because we already clear all switches and gateways in the controller, so the next reset will be rejected
 
$$$$$$$$$$$$$$$$$$$$$$ PING(05) $$$$$$$$$$$$$$$$$$$$$$

Gateway SENT PACKET:
//23 03 0F 3A 96 9D 2A 00 00 9B 00 36 F9 D8 05 00 (PING 15 bytes)
//         __ __ __                __ __ __ ++
//**                ** ** **                      (CRC from 12 bytes)
//   CC LL GG GG GG ?? RR RR KK ?? AA AA AA MM ??

// GG GG GG can not be changed, otherwise, the controller not response
// ?? ?? between RR and AA, if changed, the controller will reject
// KK is kind of Gateway ID

Controller Response:
//23 04 0D 00 36 F9 91 82 32 D8 05 00 01 41 (RES_ON 13 bytes)
//23 04 0D 00 36 F9 91 B1 03 D8 05 00 00 41 (RES_OFF 13 bytes)
//23 04 0D 00 36 F9 91 92 13 8D 05 00 01 40 (RES_ON 13 bytes) !!!! WHY 40 OR 41
                       [   ]    ==    ==
//**                ** ** **                (CRC from 10 bytes)

Resp Structure:
//   CC LL LL AA AA ?? RR RR AA MM ?? VV ??
- CC: Message Type?
- LL: Length (16 bits)
- AA: Address (24 bits)
- RR: CRC16-SPI-FUJITSU (16 bits)
- MM: 
- VV: Value (8 bits)

The ?? Between RR and AA seems not used

-------------------------------------------------

Bind Switch to Gateway

1010101010101010101010101010101010101010000100001 10100100 00100011
00000011 00001111 
00111010 10010110 10011101 
00101010 01100001 11010101 
10011011 00000000 
00110110 10101111 01101100
00001000 00000001

//23 03 0F 3A 96 9D 2A 61 D5 9B 00 36 AF 6C 08 01
//         __ __ __                __ __ __ ++ ++


-------------------------------------------------

Clear

100111111111111010101010101010101010101010101010101010000100001 10100100 00100011
00000011 00001111                     // 03 0F
00111010 10010110 10011101            // 3A 96 9D
00101010 01010001 10110110            // 2A 51 B6
10011011 00000000                     // 9B 00       
00110110 10101111 01101100            // 36 AF 6C
00001000 00000010                     // 08 02

//23 03 0F 3A 96 9D 2A 51 B6 9B 00 36 AF 6C 08 02


Message Type:
- 01: Controller response to the switch
- 04: Controller response to the gateway
- 03: Gateway send to the controller

Command:
- 01: ADD
- 02: DELETE
- 03: CLEAR ALL SWITCHES and GATEWAYS
- 04: ON/OFF
- 05: STATUS
- 08: BIND SWITCH TO GATEWAY

If this gateway is not in the controller, the controller will reject the command, including unknown message type
- 06: *Have response but not sure what it is
- 07: *Have response but not sure what it is

Know Switch ID:
//23 01 0C 00 36 C8 E4 76 DB 44 01 00 41

```


### Operations
1. Add gateway to controller
   - ID: 0x01
2. Delete gateway from controller
   - ID: 0x02
3. Clear all switches and gateways
   - ID: 0x03
4. Turn on/off light
   - ID: 0x04
   - Parameter: 0x02 (01 also works, but 00, 03, 04 not)
   - Status: 0x01 (01 on, 00 off)
5. Get status of light
   - ID: 0x05
   - TBD: 0x00 (no matter what)
8. Bind/Unbind switch to gateway
   - ID: 0x08
   - Parameter: 0x01 (bind), 0x02 (unbind)