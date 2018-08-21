	#include <18F6622.h>
#device ADC=10
#device *=16
#DEVICE HIGH_INTS=TRUE
#include <stdlib.h>

#define FASTOSC 0

#FUSES INTRC_IO,NOPROTECT,PUT,NOLVP,BROWNOUT,NOMCLR,WDT8192
#use delay(clock=8000000)

#use rs232(stream=stream_wireless,baud=57600,xmit=PIN_C6,rcv=PIN_C7,ERRORS)	
#use rs232(stream=stream_sd,baud=9600,xmit=PIN_G1,rcv=PIN_G2,ERRORS)
#use i2c(master, sda=PIN_C4, scl=PIN_C3, FAST)

#use standard_io(A)
#use standard_io(B)
#use standard_io(C)
#use standard_io(D)
#use standard_io(E)
#use standard_io(F)

/* location where serial number is stored */
#define EE_SERIAL_PREFIX         0x10
#define EE_SERIAL_MSB            0x11
#define EE_SERIAL_LSB            0x12
#define EE_HW_TYPE               0x13
#define EE_ANEMOMETER_TYPE       0x14
#define EE_SD_LOG_RATE           0x15
#define EE_LIVE_TYPE             0x16
#define EE_WIND_DIRECTION_SOURCE 0x17

//#define ANEMOMETER_TIMEOUT 2000
#define MODEM_TIMEOUT_SECONDS 240
#define BACKLIGHT_TIMEOUT_SECONDS 30
#define BUTTON_0_BIT  5
#define BUTTON_1_BIT  6
#define BUTTON_2_BIT  7
#define BUTTON_BOUNCE 2

/* I2C bus has RTC, FRAM, and UART */
#define I2C_SDA       PIN_C4
#define I2C_SCL       PIN_C3

#define BUTTON_0      PIN_B5
#define BUTTON_1      PIN_B6
#define BUTTON_2      PIN_B7

/* LCD, character, 16x2 */
#define LCD_RS PIN_F1	// Register select
#define LCD_RW PIN_F2
#define LCD_EN PIN_F3	// Enable
#define LCD_D4 PIN_F4	// Data bits
#define LCD_D5 PIN_F5	// Data bits
#define LCD_D6 PIN_F6   // Data bits
#define LCD_D7 PIN_F7   // Data bits
#define LCD_BACKLIGHT PIN_F0

/* XBee Pro modem */
#define MODEM_SLEEP        PIN_E5
#define MODEM_CTS          PIN_E4
#define MODEM_SLEEP_STATUS PIN_E3
#define MODEM_RTS          PIN_E2

/* Dataflash AT45DB161D */
#define DATAFLASH_CS       PIN_D2
#define DATAFLASH_RESET    PIN_D3
#define DATAFLASH_SI       PIN_D4
#define DATAFLASH_SO       PIN_D5
#define DATAFLASH_SCK      PIN_D6

/* status LEDs (present on rdLoggerUniversalXTC */
#define LED_LOGGING        PIN_B2 /* green */
#define LED_WIRELESS       PIN_B3 /* red */
#define LED_ANEMOMETER     PIN_D7 /* orange */

/* SD daughter board */
#define MMC_STATUS_TO_HOST PIN_E1
#define MMC_STATUS_TO_SD   PIN_E0


/* GPS */
#define GPS_EN             PIN_E6

/* analog inputs */
#define AN_IN_VOLTS        PIN_A0
#define AN0_FILTERED       PIN_B0
#define AN1_FILTERED       PIN_B1
#define WV0_FILTERED       PIN_A1
#define WV1_FILTERED       PIN_A2
#define ADC_AN_IN_VOLTS    0
#define ADC_WV0_FILTERED   1
#define ADC_WV1_FILTERED   2

#define HARDWARE_TYPE_RDLOGGER          0
//#define HARDWARE_TYPE_RDLOGGERCELL      1
#define HARDWARE_TYPE_RDLOGGERUNIVERSAL 2

#define ANEMOMETER_TYPE_40HC  0
#define ANEMOMETER_TYPE_THIES 1

#define SD_LOG_RATE_60 0
#define SD_LOG_RATE_10 1

#define LIVE_TYPE_SHORT 0
#define LIVE_TYPE_FULL  1

#define WIND_DIRECTION_SOURCE_ADC    0
#define WIND_DIRECTION_SOURCE_CMPS12 1

const int16 int16_tens[]={10000,1000,100,10,1};

#define I2C_ADDR_CMPS12 0xC0
#define I2C_ADDR_MS5611 0x77
#define I2C_ADDR_DS1307 0x68
#define I2C_ADDR_NVRAM  0xA0
