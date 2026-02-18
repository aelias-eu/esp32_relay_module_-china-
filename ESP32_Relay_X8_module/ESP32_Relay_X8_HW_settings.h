/*
  Pin usage definition
*/

// Shift registers I/O
#define IO_74HC_EN    13   // HC595.EN(13) + HC165.^CE(15)
#define IO_74HC_IN    33   // SPI Out   - HC595.SIN(14)
#define IO_74HC_RCK   25   // Latch     - HC595.RCK(12) + HC165.PL(1)
#define IO_74HC_SCK   26   // SPI clock - HC595.SCK(11) + HC165.CP(2)
#define IO_74HC_OUT   27   // SPI In    - HC165.Q7(9)

// RS485 serial port
#define RS485_TXD     19
#define RS485_RXD     18
#define RS485_RE      32   // RE pin of SP3485EN-L/TR

//JP1 IO Header
#define JP1_1         22   // IO22 I2C_SCL
#define JP1_2         21   // IO21 I2C_SDA
#define JP1_3         23   // IO23  []
#define JP1_4         5    // IO5  [must be HIGH during boot]

//JP2 Serial port UART2
#define JP2_1_VCC          // VCC +5V from USB or internal PSU
#define JP2_2_GND          // Common ground
#define JP2_3_TX      16   // IO16 UART2_RX
#define JP2_4_RX      17   // IO17 UART2_TX

#define BTN_BOOT      0    // Boot Button - will be used to "Factory reset"
#define RESET_TIMEOUT 10   // Timeout in seconds to factory-reset the module
#define PREFS_RO    true
#define PREFS_RW    false

/*
  MODBUS Register address definition
*/

// Holding registers
#define HREG_DEFAULT_COUNT  6        // There are 6 pre-defined registers
#define HREG_ACTION         0x00     // MODBUS_ACTION_ 
#define HREG_RS485_BAUDRATE 0x01
#define HREG_RS485_DATABITS 0x02
#define HREG_RS485_PARITY   0x03
#define HREG_RS485_STOPBITS 0x04
#define HREG_MODBUS_ADDRESS 0x05

//#define HREG_O_FOLLOW_I     0x0A     // Binary mask to enable input following on output
/*
  Modbus actions
  - Actions from 0x01 - 0x5F should be left reserved for future usage in main firmware  
  - Custom actions should use addresses 0x60 - 0xFF
*/
#define MODBUS_ACTION_SAVE      0x0F  // Save settings to permanent memory
#define MODBUS_ACTION_FWUPGRADE 0X10  // Enable AP for firmware upgrade
#define MODBUS_ACTION_WIFI_DOWN 0x11  // Disable wifi (e.g. if no upgrade will be performed)
