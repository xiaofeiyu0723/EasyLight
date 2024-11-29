# EasyLight 🕹️💡🌐
A Project to help you intergrate the
- Kinetic energy switch
- Light
- Gateway

Leverage the Possibility of intergration with
- MQTT bridge
- Home Assistant
- other Smart Home System...

We provide the abstraction of the Kinetic energy switch, Light, and Gateway. You can easily implement your own logic on top of it. 🤘🤓


## Folder Structure
now... this is temporary
- rf_receive_pro Get the `switch_ID` and `button_ID` from the Kinetic energy switch
- rf_transmit Replay switch signal (Just toggle)
- rf_transmit_pro Imitate gateway signals

## The Machnism

Three Roles:

### Kinetic Switch 🕹️
- Send a signal to the controller when `pressed/released`
- Only Press will change the state of the light
- **DO NOT MAINTAIN THE STATE (on/off)** 
### Controller (In Light) 💡
- Two ways to change the state of the light
  - Kinetic Switch (Toggle)
  - Gateway (On/Off and query the state)
- Any device want to change the state of the light need to be bound to the light in advance
  - Kinetic Switch: Pree the button on the Controller
  - Gateway: Send a signal to the Controller
- Send a signal to the gateway when receiving a signal from a kinetic switch (hard to catch)
### Gateway 🌐
- Send Specific signal to the controller to `ON/OFF` the light
- Periodically **query the state** of the light
- Proactively send the binding signal to the controller

## Different Signals
Details in [Signal Sequence](analyze%20notes/Signal%20sequence.md)

## Hardware
- ESP8266-12E (NodeMCU/WeMos D1 Mini)
- CC1101 (433MHz)

Connection:
Details in File
