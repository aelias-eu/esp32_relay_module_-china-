# Base firmware verion for the ESP32 Relay X8 Modbus RS485 module
This module should allow everything, that worked in the original firmware and on top of that::
 - allows you to change RS485 communication parameters
 - allows you to change Modbus module address
 - can show debug messages
 - reset communication parameters by pressing BOOT button for more than 10 seconds in standard RUN mode

## First steps:
 * You need the [modbus-esp8266](https://github.com/emelianov/modbus-esp8266) for the Arduino IDE
 * ```git clone https://github.com/aelias-eu/esp32_relay_module_-china-.git ```
 * go to the **esp32_relay_module_-china-/ESP32_Relay_X8_module** directory \
   ```cd esp32_relay_module_-china-/ESP32_Relay_X8_module``` \
   copy the **UserSettings.h.example** to **UserSettings.h** \
   ```cp UserSettings.h.example UserSettings.h ``` \
   or in Windows: \
   ```copy UserSettings.h.example UserSettings.h ``` \
   that way your settings will be preserved if you run ```git pull``` to download updaded code from this repository.
                          
## What is working:
 - [x] Input handling 
 - [x] Output handling
 - [x] Output follows input (you can toggle this for each output in the future)
 - Modbus
   - [x] Reading/writing coils (outputs)
   - [x] Reading/writing inputs
   - [x] Reading/writing holding registers
     - [x] Store configuration to NVM
     - [x] Change Modbus address
     - [x] Change RS485 settings

## Changing the communication parameters
There are 6 main HREGs for this:

| Address | Description | Allowed values |
|:-------:|:------------|:--------------:|
| 0x0000  | [Action register / Firmware](#action-register--firmware) | - |
| 0x0001  | [RS485 Baudrate](#rs485-baudrate) | 0x00 - 0x0E |
| 0x0002  | RS485 Databits | 0x05 - 0x08 |
| 0x0003  | RS485 Parity | 0x00 = **None**; 0x01 = **Odd**, 0x02 = **Even** |
| 0x0004  | RS485 Stopbits | 0x01 - 0x02 |
| 0x0005  | Modbus address | 1 - 128 |

An attempt to set to a non-allowed value will leave the current value unchanged.

### Action register / Firmware
On read operation, by default, it returns current firmware version
Write operations are reserved for triggering various actions, based on the written value:
| Written value | Operation |
|:-------------:|:----------|
| 0x0F | Save the RS485 configuration and restart module |
| 0x10 | Reserved for future use |
| 0x11 | Reserved for future use |


### RS485 Baudrate
| Register value | Baudrate |
|:-------------:|:----------|
| 0x00 | 1200 |
| 0x01 | 1800 |
| 0x02 | 2400 |
| 0x03 | 4800 |
| 0x04 | 9600 |
| 0x05 | 19200 |
| 0x06 | 28800 |
| 0x07 | 38400 |
| 0x08 | 57600 |
| 0x09 | 76800 |
| 0x0A | 115200 |
| 0x0B | 230400 |
| 0x0C | 460800 |
| 0x0D | 576000 |
| 0x0E | 921600 |


## Debug messages
The code uses ESP based log messages from **esp_log.h** so instead of serial.print use ESP_LOGI() for info messages, ESP_LOGD() for debug messages, etc...\
Just toggle the compiler settings to change message level show to the serial console. \ 
In Arduino IDE Menu: **Tools** -> **Core Debug Level** -> From **None** up to **Verbose**. \
It's recommended to change it to **None** for production usage firmware compilation.