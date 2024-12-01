# Hardware
Editor: Lan HUANG Dec 2024


### ESP-S3 + SX126x 🛰️
The SX126x is a kind of LoRa transceiver chip. 

And it not only supports LoRa modulation, but also `FSK`
- [Heltec WiFi LoRa 32(V3)](https://heltec.org/project/wifi-lora-32-v3/)
- [Ebyte EoRa-S3-400TB](https://www.ebyte.com/product/2123.html)

Some Demo of FSK modulation in SX126x (RadioLib)
- [FSK Modem Example](https://github.com/jgromes/RadioLib/tree/master/examples/SX126x/SX126x_FSK_Modem)
- [FSK Modem Configuration](https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx126x---fsk-modem)

This kind of board is very suitable

beacuse it has a built-in LoRa transceiver and a powerful ESP32 chip.

The potential protocol stack is as follows:
- [LoRa](https://en.wikipedia.org/wiki/LoRa)
- [FSK](https://en.wikipedia.org/wiki/Frequency-shift_keying)
-
- [BLE 5.0](https://en.wikipedia.org/wiki/Bluetooth_Low_Energy)
- [BLE Mesh](https://en.wikipedia.org/wiki/Bluetooth_Low_Energy)
- [WiFi](https://en.wikipedia.org/wiki/Wi-Fi)
- [802.11 b/g/n](https://en.wikipedia.org/wiki/IEEE_802.11)
- [802.11 mc](https://en.wikipedia.org/wiki/IEEE_802.11)