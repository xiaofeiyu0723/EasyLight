# Binary sequence analysis of signals
Editor: Lan HUANG    Nov 2024

Collector: Xiaofei YU

## Signal types
### Kinetic Switch
Main Components:
- switch_ID
- button_ID

Signal Sequence:
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


```
1010.... 01010100
00100001 10100100 00100011
00000001 00000010 00000011 00000100 00000101
00000000 00000000
```
Minimum Effective Signal Length (When replaying): 
11 bytes
```
0x54,0x21,0xA4,0x23, 0x01,0x02,0x03,0x04, 0x04, 0x00, 0x00
```