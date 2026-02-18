#include <HardwareSerial.h>
#include <SPI.h>                          // SPI will be used to control the I/O shift registers 74HC165 and 74HC595
#include <ModbusRTU.h>                    // I'm using this version: https://github.com/emelianov/modbus-esp8266
#include <Preferences.h>                  
#include "esp_log.h"                      // For better logging/message handling
// Include module configuration files
#include "ESP32_Relay_X8_HW_settings.h"   // HW Pins & Modbus address constants
#include "UserSettings.h"                 // Custom settings

#define FIRMWARE_VERSION 0x0101           // 0x0101 == v1.01, 0x0102 == v1.02
#ifndef USER_RS485_SETTINGS               // This should be defined in the UserSettings.h, but if not...
  #define RS485_BAUDRATE 0x0A             // Default Baudrate
                                          // BAUDRATE Options: 0x00 = 1200, 0x01 = 1800, 0x02 = 2400, 0x03 = 4800, 0x04 = 9600, 0x05 = 19200, 0x06 = 28800, 0x07 = 38400, 0x08 = 57600, 0x09 = 76800, 0x0A = 115200, 0x0B = 230400, 0x0C = 460800, 0x0D = 576000, 0x0E = 921600
  #define RS485_DATABITS 8                // Default DataBits
  #define RS485_PARITY   0x00             // Default parity 0x00 == None; 0x01 == Odd; 0x02 == Even
  #define RS485_STOPBITS 1                // Default StopBits
  #define RS485_ADDRESS  1                // Default Modbus module address
#endif

Preferences settings;                      // Module settings stored in the NVS
HardwareSerial rs485(1);                   // Serial1 is used for RS485 / Modbus interface
ModbusRTU mb_slave;
byte modbus_slave_id = RS485_ADDRESS;      // Modbus slave address
uint32_t  rs485_serialconfig;              // Serial port settings (databits, parity, stopbits) for RS485
unsigned long  rs485_baudrate;             // Baudrate
uint8_t rs485_db;                          // Databits
uint8_t rs485_p;                           // Stopbits
uint8_t rs485_sb;

uint8_t outputsImage = 0x00;               // Stores the current outputs state

static const unsigned long baudrateMap[] = {1200, 1800, 2400, 4800, 9600, 19200, 28800, 38400, 57600, 76800, 115200, 230400, 460800, 576000, 921600};
static const uint8_t parityMap[] = {'N', 'O', 'E'};


void factoryReset() {
   ESP_LOGI("factoryReset", "Starting factory reset");
   clearConfiguration();
   loadConfiguration();
   ESP.restart();     // reboot ESP
}

void clearConfiguration() {
/*
clearConfiguration - clear saved configuration in non-volatile memory
*/
  ESP_LOGV("clearConfiguration", "Applying default configuration");
  //Serial.println("clearConfig();");
  settings.begin("modbus",PREFS_RW); 
  settings.putUChar("baud",RS485_BAUDRATE);
  settings.putUChar("D",RS485_DATABITS);   
  settings.putUChar("P",RS485_PARITY);     
  settings.putUChar("S",RS485_STOPBITS);   
  settings.putUChar("addr",RS485_ADDRESS); 
  settings.end();
}

void storeConfiguration() {
/*
storeConfiguration - save current configuration to non-volatile memory
*/
  ESP_LOGV("storeConfiguration", "Saving current configuration to NVM");
  //
  settings.begin("modbus",PREFS_RW); 
  rs485_baudrate = mb_slave.Hreg(HREG_RS485_BAUDRATE);        
  rs485_db = mb_slave.Hreg(HREG_RS485_DATABITS);
  rs485_p = mb_slave.Hreg(HREG_RS485_PARITY);
  rs485_sb = mb_slave.Hreg(HREG_RS485_STOPBITS);
  modbus_slave_id = mb_slave.Hreg(HREG_MODBUS_ADDRESS);
  ESP_LOGI("storeConfiguration", "Saving current configuration to NVM: %u %u %u %u %u",rs485_baudrate,rs485_db,rs485_p,rs485_sb,modbus_slave_id);
  settings.putUChar("baud",rs485_baudrate);
  settings.putUChar("D",rs485_db);   
  settings.putUChar("P",rs485_p);     
  settings.putUChar("S",rs485_sb);   
  settings.putUChar("addr",modbus_slave_id); 
  settings.end();
  delay(1);
  ESP.restart();
}

void loadConfiguration() {
/*
loadConfiguration - gets configuration from non-volatile memory, in case of first run stores default values
*/
  ESP_LOGV("loadConfiguration", "Loading configuration from NVM");
  settings.begin("modbus",PREFS_RO); 
  bool isInit = settings.isKey("baud"); 
  if (! isInit) {                             // initialize settings with default values
    settings.end();
    clearConfiguration();
    settings.begin("modbus",PREFS_RO); 
  }
  rs485_baudrate = settings.getUChar("baud");
  rs485_db = settings.getUChar("D");
  rs485_p  = settings.getUChar("P");  
  rs485_sb = settings.getUChar("S");
  rs485_serialconfig = getSerialConfig(rs485_db, parityMap[rs485_p],rs485_sb);
  modbus_slave_id  = settings.getUChar("addr");
  settings.end();
  ESP_LOGD("loadConfiguration","Serial settings: %u 0x%X, Modbus address: %u", baudrateMap[rs485_baudrate], rs485_serialconfig, modbus_slave_id);
}

uint32_t getSerialConfig(uint8_t dataBits, char parity, uint8_t stopBits) {
/*
getSerialConfig creates correct SerialConfig value from dataBits, parity and stopBits
look at SerialConfig in HardwareSerial.h
*/    
    uint32_t config = 0x8000000;    // SerialConfig base mask
    // Map dataBits
    if (dataBits < 5) dataBits = 5;
    if (dataBits > 8) dataBits = 8;
    config |= ((dataBits - 5) << 2);
    // Map parity
    switch (toupper(parity)) {
        case 'E': config |= 0x02; break;
        case 'O': config |= 0x03; break;
        case 'N': default:  config |= 0x00; break;
    }
    // Map stopBits
    if (stopBits == 2) {
        config |= 0x30;
    } else {
        config |= 0x10;
    }
    return config;
}

/*
delayCycles - delay specified amount of CPU cycles - better that waiting whole microseconds for speedy operations
*/
static inline void delayCycles(uint32_t cycles) {
    uint32_t start = esp_cpu_get_cycle_count();
    while ((esp_cpu_get_cycle_count() - start) < cycles);
}

/*
spiIO - read inputs and write outputs at once
Manages data interange between ESP and the I/O registers
*/
uint8_t spiIO(uint8_t outputs) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE2)); 
  digitalWrite(IO_74HC_RCK, LOW);               // "Latch for the 74HC165 - get inputs ready
  delayCycles(15);                              // wait about 60ns @240MHz
  digitalWrite(IO_74HC_RCK, HIGH);
  uint8_t inputs = SPI.transfer(outputs);       // Read inputs and write outputs at once
  digitalWrite(IO_74HC_RCK, LOW);               // "Latch" for the 74HC595 - send data to the outputs
  delayCycles(15);                              // wait about 60ns @240MHz
  digitalWrite(IO_74HC_RCK, HIGH);
  SPI.endTransaction();
  return inputs;
}

uint8_t getInputBit(uint8_t inputs, uint8_t bit) {
/*
getInputBit - return state of specific module input
*/
    static const uint8_t shiftMap[] = {4, 3, 5, 2, 6, 1, 7, 0}; // Shift table (because the input numbers on 74HC does not correspond to board input numbering)
    if (bit > 7) return 0;
    return (inputs >> shiftMap[bit]) & 0x01;
}

void setOutputBit(uint8_t* outputs, uint8_t bit, uint8_t state) {
/*
setOutputBit - set state of specific module output 
*/
    if (bit > 7) return; 
    *outputs = (*outputs & ~(1 << bit)) | ((state & 0x01) << bit);
}

uint16_t cbGetHreg(TRegister *reg, uint16_t val)
{ 
/*
cbGetHreg - Callback function for sending HREG values via Modbus 
            e.g. modify value before sending or send calculated values
*/
  // This is for future use
  return val;
}

uint16_t cbSetHreg(TRegister *reg, uint16_t val)
{
/*
cbSetHreg - Callback function for setting HREG value via MODBUS
Useful info/parameters:
  val                   // new Holding register value
  reg->address.address; // Holding register address
  reg->value;           // current Holding register value
*/
  ESP_LOGD("cbSetHReg","> [0x%X]: 0x%X", reg->address.address, val);
  if (val != reg->value) {
    switch (reg->address.address) {
      case HREG_ACTION:
              switch(val) {
                case MODBUS_ACTION_SAVE: 
                    storeConfiguration();
                    break;
                case MODBUS_ACTION_FWUPGRADE:
                    break;
                case MODBUS_ACTION_WIFI_DOWN:
                    break;
                default:
                    val=FIRMWARE_VERSION;   // return Firmware version
              }
              break;
      case HREG_RS485_BAUDRATE:
              if (( val >=0 ) && (val <= 0x0E )){
                ESP_LOGD("cbSetHreg","Set baudrate to %u bps",baudrateMap[val]);
              }
              else
              {
                ESP_LOGE("cbSetHreg","Set baudrate to unsupported value [%X]",val);
                val = reg->value;   // leave current setting
              }
              break;
      case HREG_RS485_DATABITS:
              if (( val >=5 ) && (val <= 8 )){
                ESP_LOGD("cbSetHreg","Set databits to [%u]",val);
              }
              else
              {
                ESP_LOGE("cbSetHreg","Set databits to unsupported value [%X]",val);
                val = reg->value;   // leave current setting
              }
              break;
      case HREG_RS485_PARITY:
              if (( val >=0 ) && (val <= 2 )){
                ESP_LOGD("cbSetHreg","Set parity to %c",parityMap[val]);
              }
              else
              {
                ESP_LOGE("cbSetHreg","Set parity to unsupported value [%X]",val);
                val = reg->value;   // leave current setting
              }
              break;
      case HREG_RS485_STOPBITS:
              if (( val >=1 ) && (val <= 2 )){
                ESP_LOGD("cbSetHreg","Set stopbits to [%u]",val);
              }
              else
              {
                ESP_LOGE("cbSetHreg","Set stopbits to unsupported value [%X]",val);
                val = reg->value;   // leave current setting
              }
              break;
      case HREG_MODBUS_ADDRESS:
              if (( val >=1 ) && (val <= 128 )){
                ESP_LOGD("cbSetHreg","Set Modbus address to [%u]",val);
              }
              else
              {
                ESP_LOGE("cbSetHreg","Set Modbus address unsupported value [%u]",val);
                val = reg->value;   // leave current setting
              }
              break;

    }
  }
  return val;
}


int checkBootButton() {
/*
checkBootButton - Handle the BOOT button press, 
  return values:
   * 0 by default
   * 1 if pressed more than 5 seconds
   * 2 if pressed more than 10 seconds
*/
  static bool lastState = true;       
  static unsigned long pressTime = 0; 
  int returnValue = 0;

  bool currentState = digitalRead(BTN_BOOT);
  if (lastState == true && currentState == false) {     // Button press edge
    pressTime = millis();
  }
  if (lastState == false && currentState == true) {     // Button release detect
    unsigned long duration = millis() - pressTime;
    if (duration >= 10000) {
      returnValue = 2; // More than 10 seconds
    } else if (duration >= 5000) {
      returnValue = 1; // 5-10 seconds
    }
  }
  lastState = currentState;
  return returnValue;
}

void setup() {
  //I/O setup
  Serial.begin(115200);           // Debug UART
  pinMode(IO_74HC_RCK, OUTPUT);   // Shift IO latch
  pinMode(IO_74HC_EN, OUTPUT);    // Shift IO enable
  pinMode(BTN_BOOT, INPUT);       // Boot button - for factory reset functionaloity
  loadConfiguration();
  // Setup the Input/Output shift registers
    digitalWrite(IO_74HC_RCK, LOW);
    SPI.begin(IO_74HC_SCK, IO_74HC_OUT, IO_74HC_IN); 
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE2)); // 
    digitalWrite(IO_74HC_EN, LOW);  
    SPI.endTransaction();
  // Setup RS485 & MODBUS
    rs485.begin(baudrateMap[rs485_baudrate], rs485_serialconfig, RS485_RXD, RS485_TXD);    
    mb_slave.begin(&rs485, RS485_RE);                                 
    mb_slave.slave(modbus_slave_id);                                 
    mb_slave.addCoil(0, false, 8);                // Add coils
    mb_slave.addIsts(0, false, 8);                // Add inputs
    mb_slave.addHreg(0, 0, HREG_DEFAULT_COUNT);   // Add holding registers
    // Set HReg values based on the loaded configuration
    mb_slave.Hreg(HREG_ACTION,FIRMWARE_VERSION);
    mb_slave.Hreg(HREG_RS485_BAUDRATE,rs485_baudrate);        
    mb_slave.Hreg(HREG_RS485_DATABITS,rs485_db);
    mb_slave.Hreg(HREG_RS485_PARITY,rs485_p);
    mb_slave.Hreg(HREG_RS485_STOPBITS,rs485_sb);
    mb_slave.Hreg(HREG_MODBUS_ADDRESS,modbus_slave_id);
    for (int i=0; i<HREG_DEFAULT_COUNT; i++) {    // Set callbacks for holding register operations            
      mb_slave.onGetHreg(i, cbGetHreg);       
      mb_slave.onSetHreg(i, cbSetHreg);
    }
  ESP_LOGI("Setup","Setup completed, Modbus Address: %u",modbus_slave_id);
}

void loop() {
  unsigned int bootBtnState = checkBootButton();
  if (bootBtnState == 2) {
      ESP_LOGI("loop","Proceed to total factory reset...");
      factoryReset();
  }
  else if (bootBtnState == 1) {
      ESP_LOGI("loop","Proceed to soft factory reset... (empty for now)");
  }
  else {
    for (uint8_t i = 0; i < 8; ++i) {                             // Get the state of outputs for further modification
        bool coilState = (mb_slave.Coil(i) == 0);      
        setOutputBit(&outputsImage, i, coilState ? HIGH : LOW);
    }
    uint8_t inputsImage = spiIO(outputsImage);                    // Read inputs, write outputs

    for (int i = 0; i < 8; i++) { 
      mb_slave.Ists(i, getInputBit(inputsImage, i) == LOW);       // Low level is active 
    } 

    // Output follows input
    for (int i = 0; i < 8; i++) { 
      mb_slave.Coil(i, getInputBit(inputsImage, i) == 0); 
    } 

    mb_slave.task(); // Process Modbus events
  }
}
