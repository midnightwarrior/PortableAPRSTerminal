// Pin definitions for the Maximite

/* Maximite|GPIO Pin
_____________________
  Pin 1 | RB4 | 12 | DI
  Pin 2 | RB3 | 13 | EN
  Pin 3 | RB6 | 17 | CS1
  Pin 4 | RB7 | 18 | CS2
  Pin 5 | RB9 | 22 | RST
  Pin 6 | RB10 | 23
  Pin 7 | RB11 | 24 |
  Pin 8 | RB12 | 27 |
  Pin 9 | RB13 | 28 |
 Pin 10 | RB15 | 30 | RW
 Pin 11 | RD8  | 42 | DB7 *
 Pin 12 | RD9  | 43 | DB1 *
 Pin 13 | RD10 | 44 | DB3 *
 Pin 14 | RD11 | 45 | DB6 *
 Pin 15 | RE2  | 62 | DB4 *
 Pin 16 | RE3  | 63 | DB2 *
 Pin 17 | RE4  | 64 | DB0 *
 Pin 18 | RE5  | 1  | DB5 *
 Pin 19 | RE6  | 2
 Pin 20 | RE7  | 3
 */

/* U(S)ART - finding!

 - I need 2 U(S)ARTs - one is available on the standard Maximite pinout, and
   one is available on SD card header

 Available pins for possible USART?

VIDEO - SDO2A - 6 - U3TX
VERT SYNC - RF1 - 59 -
SELECT COMPOSITE - RC14 - 48
SOUND - OC2 - 49 - U4TX
Kbd clock/data - RD6/7 - 54/55

Card enable - RE0 - 60
Data > card - SDO4 - 32 - U2TX
Clk > card - SCK4 - 29 - U5TX
Data < card - SDI4 - 31 - U2RX
Card present - RD4 - 52 -
Card write protect - RD5 - 53
SD activity - RE1 - 61

Solution! - Use SD card iface - 31&32 - RX/TX
 * Debug interface is U3TX
 *
 * User input - switch is on RC13

 */

// PreProcessors
#define SYS_FREQ            (80000000L)                     // 80MHz system clock
#define CORE_TICK_RATE	    (40)                            // Core ticks at 1MHz - that's a 1us delay!

// Define pins!

// Special function pins

// Make my life much easier
#define OUT 0
#define IN 1

#define PowerLED_DIR TRISFbits.TRISF0
#define PowerLED LATFbits.LATF0

#define RST_DIR TRISDbits.TRISD6
#define RST LATDbits.LATD6
#define CS1_DIR TRISBbits.TRISB6
#define CS1 LATBbits.LATB6
#define DB6_DIR TRISDbits.TRISD11
#define DB6 LATDbits.LATD11

#define DB4_DIR TRISEbits.TRISE2
#define DB4 LATEbits.LATE2
#define DB2_DIR TRISEbits.TRISE3
#define DB2 LATEbits.LATE3
#define DB0_DIR TRISEbits.TRISE4
#define DB0 LATEbits.LATE4
#define RW_DIR TRISBbits.TRISB15
#define RW LATBbits.LATB15

#define EN_DIR TRISBbits.TRISB3
#define EN LATBbits.LATB3
#define DI_DIR TRISBbits.TRISB4  //the RS pin
#define DI LATBbits.LATB4
#define DB1_DIR TRISDbits.TRISD9
#define DB1 LATDbits.LATD9
#define DB3_DIR TRISDbits.TRISD10
#define DB3 LATDbits.LATD10
#define DB5_DIR TRISEbits.TRISE5
#define DB5 LATEbits.LATE5
#define DB7_DIR TRISDbits.TRISD8
#define DB7 LATDbits.LATD8
#define CS2_DIR TRISBbits.TRISB7
#define CS2 LATBbits.LATB7

#define DB0_Read PORTEbits.RE4
#define DB1_Read PORTDbits.RD9
#define DB2_Read PORTEbits.RE3
#define DB3_Read PORTDbits.RD10
#define DB4_Read PORTEbits.RE2
#define DB5_Read PORTEbits.RE5
#define DB6_Read PORTDbits.RD11
#define DB7_Read PORTDbits.RD8

// Use change notification stuff (CNx) for this
// RC13 goes to CN0
#define ButtonPress PORTCbits.RC13
#define ButtonPressDir TRISCbits.TRISC13 = IN



// Set GPS baudrate - this is UART2
// Set up GPS input and output pins
#define GPSRX_DIR TRISFbits.TRISF4
#define GPSRX PORTFbits.RF4
#define GPSTX_DIR TRISFbits.TRISF5
#define GPSTX PORTFbits.RF5

// This will be deprecated once I can get USB to work
#define DEBUGTX_DIR TRISGbits.TRISG8
#define DEBUGTX PORTGbits.RG8

// ESP8266 pins
#define ESP8266RX_DIR TRISDbits.TRISD2
#define ESP8266RX PORTDbits.RD2
#define ESP8266TX_DIR TRISGbits.TRISG8
#define ESP8266TX PORTGbits.RG8
#define ESP8266EN_DIR TRISDbits.TRISD7
#define ESP8266EN LATDbits.LATD7

#define GPS_BaudRate 9600
#define PI 3.141592653589793