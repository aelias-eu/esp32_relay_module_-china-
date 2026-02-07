# esp32_relay_module_-china-
Custom firmware for chinese esp32 relay module\
![ESP32 Relay X8 Module](files/esp32_modbus_rs485_relay_module_8ch.png)\
Marking on the bottom side of the PCB: **ESP32_Relay_X8_Modbus (303E32DC812)**
The module can be found as "ESP32 RS485 Modbus Wifi Bluetooth Relay Optocoupler Isolation Module Tyep C 4/8 Channel Relay Module DC 9-24V".\
I ordered from [aliexpress](https://www.aliexpress.com/item/1005010518108874.html)\

This repository is focused to the 8-channel version of that product.\
Marking on the bottom side of the PCB: **ESP32_Relay_X8_Modbus (303E32DC812)**

## Status on arival:
The module works as Modbus Slave with slave ID=1. According to all sources I managed to gather, this looks to be hardcoded. The bus connection parameters are also hardcoded.
No visible SSID and no new Wireless channel was identified - WiFi on the ESP32 is probably off.

```
Modbus ID: 1
Bus parameters: 115200 8N1
```

The inputs 1-8 are software-tied to the relays.
Working Modbus functions:
 - 01 - Read coils [0-7] bool
 - 02 - ReadDiscreteInputs [0-7] bool
 - 05 - WriteSingleCoil [0-7] bool
 - 15 - WriteMultipleCoils Address 0, count 8, number 0x00-0xFF

for easy function check, you can use the [esp32_Relay_x8_modbus_default_view.xml]

## Schematics
I managed to get this schematics file [ESP32_Relay_X8_Modbus_303E32DC812.pdf](files/ESP32_Relay_X8_Modbus_303E32DC812.pdf), not sure about author & license. If this somehow offend authors rights, please contact me and I can remove that document. 
