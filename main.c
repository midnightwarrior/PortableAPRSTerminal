/*
 * File:   main.c
 * Author: midnightwarrior
 *
 * Created on 10 November 2014, 12:50 AM
 * This version v15.01b05a
 */

#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <p32xxxx.h>
#include <plib.h>
#include <math.h>
#include "font.h"
#include <string.h>
#include <assert.h>

// W00t! The graphics code *mostly* works! The two simultaneous pixel writes issue has been
// fixed, but something funky is going on with positioning.

// EDIT: Positioning is correct, I've just misinterpreted it in the code - data supplied is ddmm.mmmm not dddd.dddd
// This issue fixed 19 January 2015

// Acknowledgements: font(s) are from http://jared.geek.nz/2014/jan/custom-fonts-for-microcontrollers
// courtesy of Jared Sanson.

// Where am I?
// Expanding the NMEA parser so I can pull out all of the information I need, and developing the 
// menu system a little bit.
// I'm trying to get $GPGSV stuff out by mallocing a dynamic 2d array.  It's not working currently :(

char versionNumber[11] = "v15.01b15a";

// Formula for delay: Tdelay = (Fpb) * 256 * DELAY

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

#define PI 3.141592653589793

// Set GPS baudrate - this is UART2
// Set up GPS input and output pins
#define GPSRX_DIR TRISFbits.TRISF4
#define GPSRX PORTFbits.RF4
#define GPSTX_DIR TRISFbits.TRISF5
#define GPSTX PORTFbits.RF5

#define DEBUGTX_DIR TRISGbits.TRISG8
#define DEBUGTX PORTGbits.RG8

#define GPS_BaudRate 9600

//This is the string that will hold the data sentence from the GPS
char NMEAString[82];
char rxbuffer[82];
int GPSDataReady = 0;
int GPSBufferPos = 0;
int GPSDataReading = 0;

#define QUEUE_SIZE 128
typedef struct   // C method of creating new types
{
	char a[QUEUE_SIZE];
	int start;
	int end;
} Queue;

Queue RXQ, TXQ;

BYTE screenRefreshTimerPeriod = 0;
BYTE screenPage = 0;
BYTE loadNewScreenPage = 0;
BYTE numberOfScreenPages = 3;

char time[3][3];
char date[3][3];
float latitude[2];
float longitude[2];


int fixquality = 0;
int numberOfSatellites = 0;

int** satellitesInView;



long satellitesInViewLength = 0;


// Accuracy/precision
float hdop = 0;
float pdop = 0;
float vdop = 0;

float altitude_msl = 0;
float altitude_wgs84 = 0;
float speed = 0;
float coursemadegood = 0;
float mag_variation = 0;
int fixmode = 0;

BYTE time_Changed = 0;
BYTE date_Changed = 0;
BYTE latitude_Changed = 0;
BYTE longitude_Changed = 0;
BYTE fixquality_Changed = 0;
BYTE numberOfSatellites_Changed = 0;
BYTE hdop_Changed = 0;
BYTE pdop_Changed = 0;
BYTE vdop_Changed = 0;
BYTE altitude_msl_Changed = 0;
BYTE altitude_wgs84_Changed = 0;
BYTE speed_Changed = 0;
BYTE coursemadegood_Changed = 0;
BYTE mag_variation_Changed = 0;
BYTE fixmode_Changed = 0;
BYTE satellitesInView_Changed = 0;

// Don't do anything about $GPGSV at the mo - define a struct or something?



// Function prototypes

int main(void);
void Enable_Pulse(void);
void Draw_Point(unsigned short, unsigned short, unsigned short);
void GLCD_CLR(void);
void GLCD_Clrln(unsigned short);
void GLCD_Data(BYTE);
void GLCD_FlashRapidly(void);
void GLCD_ON(void);
unsigned short GLCD_Read(unsigned short);
void GLCD_Write(BYTE);
void GOTO_COL(unsigned int);
void GOTO_ROW(unsigned int);
void Set_Start_Line(unsigned short);
void GOTO_XY(unsigned int, unsigned int);
void delay_us(WORD);
void init(void);
void setDataBusAsRead(void);
void initGPS(void);
int is_uart_data_ready(void);
char uart_getchar(void);
char** str_split(char* , const char);
char * strtok_single (char *, char const *);



BYTE instructionCount = 0x00;
int fooCounter = 0;
char str[15];

void init(void) {
    // Initialises the PIC32's GPIOs
    AD1PCFG = 0xFFFF;
    CS1_DIR = OUT;
    CS2_DIR = OUT;
    RST_DIR = OUT;
    RW_DIR = OUT;
    DB0_DIR = OUT;
    DB1_DIR = OUT;
    DB2_DIR = OUT;
    DB3_DIR = OUT;
    DB4_DIR = OUT;
    DB5_DIR = OUT;
    DB6_DIR = OUT;
    DB7_DIR = OUT;
    EN_DIR = OUT;
    DI_DIR = OUT;

    PowerLED_DIR = OUT;

    // Initialise the GPS!
    initGPS();
    initDebug();

}

void initGPS() {
    GPSTX_DIR = OUT;
    GPSRX_DIR = IN;

    // Set up U(S)ART
    UARTConfigure(UART2, (UART_ENABLE_HIGH_SPEED | UART_ENABLE_PINS_TX_RX_ONLY));
    UARTSetFifoMode(UART2, UART_INTERRUPT_ON_RX_NOT_EMPTY);
    UARTSetDataRate(UART2, 4e7, 9600);
    UARTSetLineControl(UART2, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    UARTEnable(UART2, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
    // Extra stuff for interrupts
    ConfigIntUART2(UART_INT_PR2 | UART_RX_INT_EN);
    INTEnableSystemMultiVectoredInt();
}

void initDebug() {
    DEBUGTX_DIR = OUT;
    UARTConfigure(UART3, (UART_ENABLE_HIGH_SPEED | UART_ENABLE_PINS_TX_RX_ONLY));
    UARTSetFifoMode(UART3, UART_INTERRUPT_ON_RX_NOT_EMPTY);
    UARTSetDataRate(UART3, 4e7, 115200);
    UARTSetLineControl(UART3, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    UARTEnable(UART3, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
}

void setDataBusAsRead(void) {
    // Set tristate registers so the LCD's data bus can get read
    // This function DOESN'T ACTUALLY WORK PROPERLY! - fix!
    DB0_DIR = IN;
    DB1_DIR = IN;
    DB2_DIR = IN;
    DB3_DIR = IN;
    DB4_DIR = IN;
    DB5_DIR = IN;
    DB6_DIR = IN;
    DB7_DIR = IN;
    //PowerLED = DB0_DIR;
    //delay_us(20000);
}

void _mon_putc (char c)
 {
   while (U3STAbits.UTXBF); //Wait till transmission is complete
   U3TXREG = c;
 }

void init_userinterface(void) {
    mCNOpen(CN_ON | CN_IDLE_CON, CN1_ENABLE, CN_PULLUP_DISABLE_ALL); //See plib documentation and PIC32 datasheet
    ConfigIntCN(CHANGE_INT_ON | CHANGE_INT_PRI_3);
    INTEnableSystemMultiVectoredInt();
}

int main(void) {
    // Main routine - do something!

    // This routine will handle getting GPS info, carting it away to a parser
    // then figuring out the relevant stuff to be fed to the display
    SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);
    OpenCoreTimer(CORE_TICK_RATE);
    init();


    // Set up timer 1 as the basis for the delay_us() function
    OpenTimer1(T1_ON | T1_IDLE_CON | T1_PS_1_8 | T1_SOURCE_INT,  5000);
    // Set up timer 2 as the screen refresh clock (200ms)
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_256,  31250);
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_1);

    // Set up pushbutton interrupt
    init_userinterface();

    INTEnableSystemMultiVectoredInt();


    CS1 = 0;
    CS2 = 0;
    RST = 1;

    GLCD_ON();
    GLCD_CLR();

    Set_Start_Line(0);

    // Initialise satellites in view array
    satellitesInView = malloc(sizeof *satellitesInView * 4);

    printf("PortableAPRS unit starting...\r\n");

    loadScreenPage();
//GLCD_RenderText(0,0,"UTC:");
//GLCD_RenderText(12+31, 0, ":");
//GLCD_RenderText(30+31, 0, ":");
//GLCD_RenderText(0,8,"Date:");
//GLCD_RenderText(12+31, 8, "/");
//GLCD_RenderText(30+31, 8, "/");
//GLCD_RenderText(0,16,"Lat:");
//GLCD_RenderText(0,24,"LonG:");
//GLCD_RenderText(0,32,"Sats:");
    while(1) {
        GPSBufferPos = 0;
        while(GPSDataReady == 0) {
            // This loop will only loop if data isn't ready
                // Data is ready - rejoice! ...ahem... I mean, buffer it
                while(rxbuffer[0] != '$') {
                    // This string is INVALID! WRONG!
                    // Get rid of it immediately
                    GPSBufferPos = 0;
                    memset(rxbuffer, 0, 82);
                    if(is_uart_data_ready()) {
                        rxbuffer[0] = uart_getchar();
                    }

                }
                //printf("First character rxed, keep reading...\r\n");

                GPSBufferPos++;

                // The first part of the string is valid
                // Keep receiving!

                while(rxbuffer[GPSBufferPos-1] != 0x0D) {
                    if(is_uart_data_ready()) {
                        rxbuffer[GPSBufferPos] = uart_getchar();
                        GPSBufferPos++;
                        //printf("Bam!\r\n");

                        //PowerLED = !PowerLED;
                    }
                }

                GPSDataReady = 1;
                rxbuffer[GPSBufferPos] = '\0';
//                printf("***String receiving has ended!\r\n");
//                printf(rxbuffer);
                if(NMEAChecksum() == 0) {
                    printf("\r\nChecksum is invalid - clear what's been rxed\r\n\r\n\r\n");
                    GPSDataReady = 0;
                    memset(rxbuffer, 0, 82);
                }
                break;
            }

            if(GPSDataReady == 1) {
                    strcpy(NMEAString, rxbuffer);
                    NMEAParser();
                }
                GPSDataReady = 0;
                GPSBufferPos = 0;
                memset(rxbuffer, 0, 82);
                //printf("Start again!\r\n");
    }
    return (EXIT_SUCCESS);
}

void forceDataRewrite() {
    time_Changed = 1;
    date_Changed = 1;
    latitude_Changed = 1;
    longitude_Changed = 1;
    fixquality_Changed = 1;
    numberOfSatellites_Changed = 1;
    hdop_Changed = 1;
    pdop_Changed = 1;
    vdop_Changed = 1;
    altitude_msl_Changed = 1;
    altitude_wgs84_Changed = 1;
    speed_Changed = 1;
    coursemadegood_Changed = 1;
    mag_variation_Changed = 1;
    fixmode_Changed = 1;
}
void loadScreenPage(void) {
    char string[15];
    GLCD_ON();
    GLCD_CLR();
    Set_Start_Line(0);
    forceDataRewrite();
    snprintf(string,12," Page %d/%d ",screenPage+1, numberOfScreenPages+1);
    GLCD_RenderText_writeBytes(67,7,string,0);
    switch(screenPage) {
       case 0:  GLCD_RenderText_writeBytes(0,0,"Portable APRS",1);
                GLCD_RenderText_writeBytes(0,1,"midnightwarrior",1);
                GLCD_RenderText_writeBytes(0,5,"Version:",1);
                GLCD_RenderText_writeBytes(56, 5, versionNumber, 1);
                displayGPSData();
                break;
        case 1:
                GLCD_RenderText_writeBytes(0,0,"UTC:",1);
                GLCD_RenderText_writeBytes(12+31, 0, ":",1);
                GLCD_RenderText_writeBytes(30+31, 0, ":",1);
                GLCD_RenderText_writeBytes(0,1,"Date:",1);
                GLCD_RenderText_writeBytes(12+31, 1, "/",1);
                GLCD_RenderText_writeBytes(30+31, 1, "/",1);
                GLCD_RenderText_writeBytes(0,2,"Lat:",1);
                GLCD_RenderText_writeBytes(0,3,"Long:",1);
                GLCD_RenderText_writeBytes(0,4,"Altitude:",1);
                GLCD_RenderText_writeBytes(0,5,"Sats:",1);
                displayGPSData();
                break;
        case 2: GLCD_RenderText_writeBytes(0,0,"Dilution of precision", 1);
                GLCD_RenderText_writeBytes(0,2,"Horizontal: ",1);
                GLCD_RenderText_writeBytes(0,3,"Vertical: ", 1);
                GLCD_RenderText_writeBytes(0,4,"Positional: ", 1);
                displayGPSData();
                break;
        case 3: GLCD_RenderText_writeBytes(0,0,"UTC Time", 1);
                GLCD_DrawCircle(64, 29, 24, 1);
                break;
    }
}

void displayGPSData(void) {
        char stringFromFloat[50];
        switch(screenPage) {
            case 1: if(time_Changed) {
                    time_Changed = 0;
                    GLCD_RenderText(0+31,0,time[0]);
                    GLCD_RenderText(18+31, 0, time[1]);
                    GLCD_RenderText(36+31, 0, time[2]);
                }

                    if(date_Changed) {
                        date_Changed = 0;
                        GLCD_RenderText(0+31,8, date[0]);
                        GLCD_RenderText(18+31, 8, date[1]);
                        GLCD_RenderText(36+31, 8, date[2]);
                    }
                    if(latitude_Changed) {
                        latitude_Changed = 0;
                        snprintf(stringFromFloat,16,"%.0f~ %f'",latitude[0],latitude[1]);
                        GLCD_RenderText_writeBytes(31,2,stringFromFloat,1);
                        printf(stringFromFloat);
                        printf("\r\n");
                    }
                    if(longitude_Changed) {
                        longitude_Changed = 0;
                        snprintf(stringFromFloat,116,"%.0f~ %f'",longitude[0],longitude[1]);
                        GLCD_RenderText_writeBytes(31,3,stringFromFloat,1);
                        printf(stringFromFloat);
                        printf("\r\n");
                    }
                    if(altitude_msl_Changed) {
                        altitude_msl_Changed = 0;
                        snprintf(stringFromFloat,10,"%.1f m ", altitude_msl);
                        GLCD_RenderText_writeBytes(56,4,stringFromFloat,1);
                    }
                    if(numberOfSatellites_Changed) {
                        numberOfSatellites_Changed = 0;
                        snprintf(stringFromFloat,3,"%d",numberOfSatellites);
                        GLCD_RenderText_writeBytes(31, 5, stringFromFloat,1);
                }
            break;
            case 2: if(hdop_Changed) {
                        hdop_Changed = 0;
                        snprintf(stringFromFloat,6,"%f",hdop);
                        GLCD_RenderText_writeBytes(70,2,stringFromFloat,1);
                    }
                    if(vdop_Changed) {
                        vdop_Changed = 0;
                        snprintf(stringFromFloat,6,"%f",vdop);
                        GLCD_RenderText_writeBytes(60,3,stringFromFloat,1);
                    }
                    if(pdop_Changed) {
                        pdop_Changed = 0;
                        snprintf(stringFromFloat,6,"%f",pdop);
                        GLCD_RenderText_writeBytes(70,4,stringFromFloat,1);
                    }
            break;
            case 3: // Draw analogue hands
                if(time_Changed) {
                    GLCD_CLR();
                    loadScreenPage();
                    time_Changed = 0;
                    // Get time from strings
                    int hour = atoi(time[0]);
                    int minute = atoi(time[1]);
                    int second = atoi(time[2]);

                    // Convert to 12-hour time
                    if(hour > 12) {
                        // Subtract 12 hours from time
                        hour -= 12;
                    }

                    // Hand angles!
                    // Each hour is 30 degrees
                    // This means each minute is 360 / 60 = 6 degrees
                    // Each second is also 6 degrees

                    // Hour angle = (hour)*30 + (minute) * (0.5) + (second) * (0.5/60)

                    // Draw the hours hand
                    // This is half the length of the minutes hand

                    int hour_x, hour_y = 0;
                    int minute_x, minute_y = 0;
                    int second_x, second_y = 0;
                    int r = 20;
                    int angle = ((hour)*30) + ((minute) * 0.5) + ((second) * (0.5/60)) + 270;
                    hour_x = 64 + (r/2)*cos(angle*(PI/180));
                    hour_y = 32 + (r/2)*sin(angle*(PI/180));
                    // minute position - minute*6 + second*(6/60)
                    angle = (minute*6) + (second*(6/60)) + 270;
                    minute_x = 64 + (r)*cos(angle*(PI/180));
                    minute_y = 29 + (r)*sin(angle*(PI/180));
                    // second position - second * 6
                    angle = (second*6) + 270;
                    second_x = 64 + (r)*cos(angle*(PI/180));
                    second_y = 29 + (r)*sin(angle*(PI/180));

                    GLCD_DrawLine(64, 29, hour_x, hour_y, 0);
                    GLCD_DrawLine(64, 29, minute_x, minute_y, 0);
                    GLCD_DrawLine(64, 29, second_x, second_y, 0);
                }
                break;
        }
}

void NMEAParser(void) {
    // This is the NMEA sentence parser
    char tokens[50][50];
    memset(tokens, 0, 50);
    printf(NMEAString);
    printf("\r\n");
    //strcpy(NMEAString,"$GPRMC,074008.574,V,4050.00,S,17553.57,E,0.00,0.00,090180,,,N*40");

    char str[15];
    int tokenPosition = 0;
//    printf(NMEAString);
//    printf("\r\n");
    // Get first token
    char *temp_token = strtok_single(NMEAString, ",*");
    if(temp_token == NULL) {
        printf("No token found!\r\n");
        strcpy(tokens[tokenPosition], "0");
    }
    else {
        strcpy(tokens[tokenPosition], temp_token);
    }
    tokenPosition++;
    while(temp_token != NULL) {
        temp_token = strtok_single(NULL, ",*");

        if(temp_token != NULL) {
            strcpy(tokens[tokenPosition], temp_token);
        }
        else {
            strcpy(tokens[tokenPosition], "0");
        }

        tokenPosition++;
    }

    int a, b;

    if(strcmp(tokens[0], "$GPGGA") == 0) {
        // String is $GPGGA
        // This contains altitude, accuracy etc.
        float old_altitude_msl = altitude_msl;
        altitude_msl = strtod(tokens[9], NULL);
        if(old_altitude_msl != altitude_msl) {
            // Set changed flag!
            altitude_msl_Changed = 1;
        }

        float old_hdop = hdop;
        hdop = strtod(tokens[8], NULL);
        if(old_hdop != hdop) {
            // Set changed flag!
            hdop_Changed = 1;
        }

    }
    if(strcmp(tokens[0], "$GPRMC") == 0) {
//        // String is $GPRMC
//
//        // Get time and date
        char oldTime[3][3];
        char oldDate[3][3];

        for(a=0;a<3;a++) {
            for(b=0;b<3;b++) {
               oldTime[a][b] = time[a][b];
               oldDate[a][b] = date[a][b];
            }
        }
        for(a=0;a<3;a++) {
            for(b=0;b<2;b++) {
                int offset = (a*2)+b;
                time[a][b] = tokens[1][offset];
                date[a][b] = tokens[9][offset];
                if(time[a][b] != oldTime[a][b]) {
                    time_Changed = 1;
                }
                if(date[a][b] != oldTime[a][b]) {
                    date_Changed = 1;
                }
            }
            
            time[a][2] = '\0';
            date[a][2] = '\0';
        }

        // Get latitude/longitude and sign it
        // NOTE!!! - lat and long are returned in ddmm.mmmm format, NOT dddd.dddd
        float oldLongitude[2];
        oldLongitude[0] = longitude[0];
        oldLongitude[1] = longitude[1];

        float oldLatitude[2];
        oldLatitude[0] = latitude[0];
        oldLatitude[1] = latitude[1];

        latitude[0] = (int)strtod(tokens[3], NULL) / 100;
        latitude[1] = (strtod(tokens[3], NULL) / 100 - latitude[0]) * 100;
        longitude[0] = (int)strtod(tokens[5], NULL) / 100;
        longitude[1] = (strtod(tokens[5], NULL) / 100 - longitude[0]) * 100;

        if(tokens[4][0] == 'S') {
            latitude[0]*= -1;
        }
        if(tokens[6][0] == 'W') {
            longitude[0]*=-1;
        }

        if(oldLongitude[0] != longitude[0] || oldLongitude[1] != longitude[1]) {
            longitude_Changed = 1;
        }
        if(oldLatitude[0] != latitude[0] || oldLatitude[1] != latitude[1]) {
            latitude_Changed = 1;
        }
    }
    else if(strcmp(tokens[0], "$GPGSV") == 0) {
        // String is $GPGSV
        // This contains satellites in view

        // Get number of satellites in view
        int oldnumberOfSatellites = numberOfSatellites;
        numberOfSatellites = atoi(tokens[3]);
        if(oldnumberOfSatellites != numberOfSatellites) {
            numberOfSatellites_Changed = 1;
        }


        // The fun part - parse the string and add satellites to the array!
        /* GPS Satellites in view

        eg. $GPGSV,3,1,11,03,03,111,00,04,15,270,00,06,01,010,00,13,06,292,00*74
            $GPGSV,3,2,11,14,25,170,00,16,57,208,39,18,67,296,40,19,40,246,00*74
            $GPGSV,3,3,11,22,42,067,42,24,14,311,43,27,05,244,00,,,,*4D


            $GPGSV,1,1,13,02,02,213,,03,-3,000,,11,00,121,,14,13,172,05*67


        1    = Total number of messages of this type in this cycle
        2    = Message number
        3    = Total number of SVs in view
        4    = SV PRN number
        5    = Elevation in degrees, 90 maximum
        6    = Azimuth, degrees from true north, 000 to 359
        7    = SNR, 00-99 dB (null when not tracking)
        8-11 = Information about second SV, same as field 4-7
        12-15= Information about third SV, same as field 4-7
        16-19= Information about fourth SV, same as field 4-7

        - Taken from http://aprs.gids.nl/nmea/#gsv
         */

        // Satellite number is at 4, 8, 12, 16...
        // Elevation in degrees is at 5, 9, 13, 17...
        // Azimuth in degrees from true north is at 6, 10, 14, 18...
        // SNR in dB is at 7, 11, 15, 19...
        // Up to 4 SV's in each sentence, values are NULL if not there
        unsigned int svNumber[4];
        unsigned int elevation[4];
        unsigned int azimuth[4];
        unsigned int snr[4];
        unsigned int messageNumber = atoi(tokens[2]);

        int sv, a;

        for(sv=0;sv<4;sv++) {
            svNumber[sv] = atoi(tokens[(sv + 1) * 4]);
            elevation[sv] = atoi(tokens[((sv + 1) * 4) + 1]);
            azimuth[sv] = atoi(tokens[((sv + 1) * 4) + 2]);
            snr[sv] = atoi(tokens[((sv + 1) * 4) + 3]);
        }

        // Cool. Now we have our 'seen' SV list, search through the existing array
        // to see if those satellites have already been seen.  If not, then check
        // for empty space in the array and write there.
        // If the message number is 1, *clear* the array (edit: not strictly necessary)
        // and shrink it to the number of visible satellites.
        if(messageNumber == 1) {
            // Kill the array - we're gonna fill it with new values!
            free(satellitesInView);

            satellitesInView = malloc(sizeof *satellitesInView * 4);
            if(satellitesInView) {
                for(a=0;a<4;a++) {
                    satellitesInView[a] = malloc(sizeof *satellitesInView[a] * numberOfSatellites);
                }
                PowerLED = !PowerLED;
            }
            satellitesInViewLength = 0;
        }

        // Iterate through the satellites we just got - then increment satellitesInViewLength by
        // one every time a non-NULL value for satellite SNR/SV number/yadda yadda yadda is found, and
        // add said value to the array of satellites in view

        for(sv=0;sv<4;sv++) {
            if(svNumber[sv] != NULL) {
                // There's a value here!
                satellitesInViewLength += 1;
//                satellitesInView[0][satellitesInViewLength] = svNumber[sv];
                
            }
        }


    }
    else if(strcmp(tokens[0], "$GPGSA") == 0) {
        // String is $GPGSA
        // This has more degree-of-precision stuff in it, as well as currently used
        // satellites

        float old_pdop = pdop;
        pdop = strtod(tokens[15], NULL);
        if(old_pdop != pdop) {
            // Set changed flag!
            pdop_Changed = 1;
        }

        float old_hdop = hdop;
        hdop = strtod(tokens[16], NULL);
        if(old_hdop != hdop) {
            // Set changed flag!
            hdop_Changed = 1;
        }

        float old_vdop = vdop;
        hdop = strtod(tokens[17], NULL);
        if(old_vdop != vdop) {
            // Set changed flag!
            vdop_Changed = 1;
        }

    }

    free(tokens);
}

int NMEAChecksum(void) {
    // Checksums the NMEA sentence
    // XOR everything between $ and * (but not inclusive)
    unsigned int a, checksum = 0;
    for(a=1;a<strlen(rxbuffer)-4;a++) {
        checksum ^= rxbuffer[a];
    }

    // Strip checksum from sentence
    char str_checksum[3];
    unsigned int number_checksum;
    str_checksum[0] = rxbuffer[strlen(rxbuffer) - 3];
    str_checksum[1] = rxbuffer[strlen(rxbuffer) - 2];
    str_checksum[2] = NULL;
    number_checksum = strtol(str_checksum, NULL, 16);

    if(number_checksum == checksum) {
        // Checksum is valid!
//        printf("Checksum is valid!\r\n");
        return 1;
    }
    else {
        // Checksum is invalid!
//        printf("Checksum is invalid!\r\n");
//        while (U3STAbits.UTXBF); //Wait till transmission is complete
//        U3TXREG = rxbuffer[strlen(rxbuffer) - 4];
        return 0;
    }

}

void __ISR(_TIMER_2_VECTOR, ipl1) IntTim2Handler(void) {
    //printf("Blink!\r\n");
    if(mT2GetIntFlag()) {
        if(screenRefreshTimerPeriod == 5) {
            displayGPSData();
            screenRefreshTimerPeriod = 0;
//            printf("DON'T BLINK.\r\n");
            mT2ClearIntFlag();
            return;
        }
        mT2ClearIntFlag();
        screenRefreshTimerPeriod++;
        if(loadNewScreenPage) {
            loadNewScreenPage = 0;
            loadScreenPage();
        }
//        printf("%d\r\n", screenRefreshTimerPeriod);
        return;
    }
}

void __ISR(_UART2_VECTOR, ipl2) IntUart2Handler(void)
{
	// Is this an RX interrupt?
	if(mU2RXGetIntFlag())
	{
	    mU2RXClearIntFlag();
            //PowerLED = !PowerLED;
            char c = ReadUART2();
            //PowerLED = !PowerLED;
            if ((RXQ.end+1)%QUEUE_SIZE != RXQ.start) {
		RXQ.a[RXQ.end] = c;
		RXQ.end = (RXQ.end + 1)%QUEUE_SIZE;
            }
            return;
        }
            
	// We don't care about TX interrupt
	if ( mU2TXGetIntFlag() )
	{
            mU2TXClearIntFlag();
	}
}

void __ISR(_CHANGE_NOTICE_VECTOR,ipl3) DetButtons(void) {
    // Determine what change notice vector was sent
    // Falling edge occurs when button is pressed!
    // No switch bounce for some strange reason, so I have been lazy and not implemented
    // a debouncing routine.
    mPORTCRead();
    if(!ButtonPress) {
        // Button was pressed!
        // Increment the screen page number
        printf("Interrupt!\r\n");
        if(screenPage < numberOfScreenPages) {
            screenPage++;
            loadNewScreenPage = 1;
        }
        else {
            screenPage = 0;
            loadNewScreenPage = 1;
        }
    }

    mCNClearIntFlag();
}

int is_uart_data_ready(void)
{
	return (RXQ.start != RXQ.end);
}

char uart_getchar(void)
{
	char c = RXQ.a[RXQ.start];
	RXQ.start = (RXQ.start + 1)%QUEUE_SIZE;
	return c;
}

void GLCD_DrawLine(unsigned short x0, unsigned short y0, unsigned short x1, unsigned short y1, unsigned short style) {
    // Draw a line on the screen!

    // Bresenham's Line Algorithm - from http://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C

  int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
  int err = (dx>dy ? dx : -dy)/2, e2;

  for(;;){
      switch(style) {
          case 0: // Solid line
              Draw_Point(x0,y0,1);
              break;
          case 1: // Dotted line - even pixels
              if(!(x0 % 2)) {
                   Draw_Point(x0,y0,1);
                   PowerLED = 1;
              }
              break;
          case 2: // Dotted line - odd pixels
              if(x0 % 2) {
                  Draw_Point(x0,y0,1);
              }
              break;
      }
    if (x0==x1 && y0==y1) break;
    e2 = err;
    if (e2 >-dx) {
        err -= dy;
        x0 += sx;
    }
    if (e2 < dy) {
        err += dx;
        y0 += sy;
    }
  }
}

void GLCD_DrawCircle(unsigned short x, unsigned short y, unsigned short r, unsigned short colour) {
    // Draw a circle!
    int angle;
    for(angle=0; angle<360; angle++) {
        Draw_Point(x + r*cos(angle*(PI/180)), y + r*sin(angle*(PI/180)), colour);
    }
}
void GLCD_DrawBox(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned short fillpattern) {
    // Code to draw a box and fill/unfill/hatch/whatever it
    int x, y;
    // Swap coords around if need be
    if(x1 > x2) {
        x = x1;
        x1 = x2;
        x2 = x;
    }

    if(y1 > y2) {
        y = y1;
        y1 = y2;
        y2 = y;
    }

    // Shading routine
    switch(fillpattern) {
        case 1: // Fill the box
            for(y=y1; y<y2+1; y++) {
                GLCD_DrawLine(x1, y, x2, y, 0);
            }
            break;
        case 2: // Dot teh box
            for(y=y1+1; y<y2; y+=2) {
                GLCD_DrawLine(x1+1, y, x2, y, 1);
                GLCD_DrawLine(x1+1, y+1, x2, y+1, 2);
            }
            break;
    }

    GLCD_DrawLine(x1, y1, x1, y2, 0);
    GLCD_DrawLine(x1, y1, x2, y1, 0);
    GLCD_DrawLine(x2, y1, x2, y2, 0);
    GLCD_DrawLine(x1, y2, x2, y2, 0);
}

void Enable_Pulse() {
    EN = 1;
    // Delay 5us
    delay_us(5);
    EN = 0;
    // Delay 5us
    delay_us(5);
}

void GLCD_ON() {
    // Switch on both chips
    CS1 = 1;
    CS2 = 1;
    DI = 0;
    //RW = 1;
    RW = 0;
    GLCD_Data(0x3F);
    Enable_Pulse();
    return;
}

void GLCD_OFF() {
    // Switch on both chips
    CS1 = 1;
    CS2 = 1;
    DI = 0;
    //RW = 1;
    RW = 0;
    GLCD_Data(0x3E);
    Enable_Pulse();
    return;
}

void Set_Start_Line(unsigned short line) {
    DI = 0;
    RW = 0;
    CS1 = 1;
    CS2 = 1;
    GLCD_Data(0xC0 | line);
    Enable_Pulse();
}

void GOTO_COL(unsigned int x)
{
    unsigned short Col_Data;

    DI = 0;
    RW = 0;
    if(x<64) {
        CS1 = 1;
        CS2 = 0;
        Col_Data = x;
    }
    else {
        CS1 = 0;
        CS2 = 1;
        Col_Data = x-64;
    }
    Col_Data = (Col_Data | 0x40) & 0x7F;
    GLCD_Data(Col_Data);
    Enable_Pulse();
}

void GOTO_ROW(unsigned int y) {
    unsigned short Row_Data;
    DI = 0;
    RW = 0;
    Row_Data = (y | 0xB8) & 0xBF;
    GLCD_Data(Row_Data);
    Enable_Pulse();
}

void GOTO_XY(unsigned int x, unsigned int y) {
    GOTO_ROW(y);
    GOTO_COL(x);
}

void GLCD_Write(BYTE b) {
    DI = 1;
    RW = 0;
    GLCD_Data(b);
    delay_us(1);
    Enable_Pulse();
}

unsigned short GLCD_Read(unsigned short column) {
    unsigned short read_data = 0;
    // Set ports as inputs
    // This requires magic with tristate buffers
    // EDIT: Magic has already been done in a routine!

    setDataBusAsRead();

    DI = 1;
    RW = 1;

    CS2 = (column>63);
    CS1 = !CS2;

    // Delay 1us
    delay_us(1);
    EN = 1;

    // Delay 1us
    delay_us(1);
    EN = 0;

    // Delay 5us
    delay_us(5);
    EN = 1;
    // Delay 1us
    delay_us(1);
    // Get data off the port!

    read_data = (DB7_Read * 128) + (DB6_Read * 64) + (DB5_Read * 32) + (DB4_Read * 16)
            + (DB3_Read * 8) + (DB2_Read * 4) + (DB1_Read * 2) + (DB0_Read);
    EN = 0;
    // Delay 1us
    delay_us(1);

    init();
    return read_data;
}

void GLCD_Clrln(unsigned short ln) {
    int i;
    GOTO_XY(0,ln); //At start of line of left side
    GOTO_XY(64,ln); //At start of line of right side (Problem)
    // Enable CS1
    CS1 = 1;
    for(i=0;i<65;i++) {
        GLCD_Write(0x00);
    }
}

void GLCD_CLR() {
    unsigned short m;
    for(m=0;m<8;m++){
        GLCD_Clrln(m);
    }
}

void Draw_Point(unsigned short x,unsigned short y, unsigned short color) {
    unsigned short Col_Data;
    GOTO_XY(x,(y/8));
    switch (color) {
    case 0: //Light spot
        Col_Data = ~(1<<(y%8)) & GLCD_Read(x);
        break;
    case 1: //Dark spot
        Col_Data = (1<<(y%8)) | GLCD_Read(x);
        break;
    }
    GOTO_XY(x,(y/8));
    GLCD_Write(Col_Data);
}

void writeByteToScreen(unsigned short x,unsigned short y, unsigned short color) {
    unsigned short Col_Data;
    switch (color) {
    case 0: //Light spot
        Col_Data = ~(1<<(y%8));
        break;
    case 1: //Dark spot
        Col_Data = (1<<(y%8));
        break;
    }
    GOTO_XY(x,(y/8));
    GLCD_Write(Col_Data);
}

void GLCD_Data(BYTE data) {

    // Write data!
    DB0 = ((data >> 0) & 0x01);
    DB1 = ((data >> 1) & 0x01);
    DB2 = ((data >> 2) & 0x01);
    DB3 = ((data >> 3) & 0x01);
    DB4 = ((data >> 4) & 0x01);
    DB5 = ((data >> 5) & 0x01);
    DB6 = ((data >> 6) & 0x01);
    DB7 = ((data >> 7) & 0x01);

    return;
}


void delay_us(WORD delay)
 {

    // New approach! - use timer 1
    // Timer 1 is configged to have a resolution of 1/5000 ms - that's 0.2us or
    // 200ns.  Probably a bit overkill, but meh.
    // This function will set the maximum value of the timer to the required period

    unsigned timerResetValue = delay * 5;
    unsigned int int_status;

    int_status = INTDisableInterrupts();
    OpenTimer1(T1_ON | T1_IDLE_CON | T1_PS_1_8 | T1_SOURCE_INT,  timerResetValue);
    INTRestoreInterrupts(int_status);

    WriteTimer1(0);

    while(1) {
        if(mT1GetIntFlag()) {
            mT1ClearIntFlag();
            return;
        }
    }
}

void GLCD_RenderText(int init_x, int init_y, char *str) {
    int a, b, x, y, offset = 0;
    GOTO_XY(init_x, init_y);

    // Loop through string and print chars
    while(offset < strlen(str)) {
        x = offset * 6 + init_x;
        y = init_y;
        GOTO_XY(x, y);
        for(a=0; a<5; a++) {
            for(b=7; b+1>0; b--) {
                Draw_Point(x + a, y + b, (((font[str[offset] - 0x20][a]) >> b) & 0x01));
            }
        }
        offset++;
    }
}

void GLCD_RenderText_writeBytes(int init_x, int init_y, char *str, int colour) {
    // NOTE: y position here is the page, not the y pixel position
    // Extra parameter - colour - 1 means text is white with black background
    // 0 means black text on white background
    // Runs like greased lightning in comparison to the other RenderText function!
    int a, b, x, y, offset = 0;
    GOTO_XY(init_x, init_y);
    GOTO_ROW(init_y);
    if(!colour) {
        GLCD_Write(0xFF);
    }

    // Loop through string and print chars
    while(offset < strlen(str)) {
        if(!colour) {
            x = offset * 6 + init_x + 1;
        }
        else if(colour) {
            x = offset * 6 + init_x;
        }
        y = init_y;
        GOTO_XY(x, y);
        GOTO_ROW(init_y);
        for(a=0; a<5; a++) {
            GOTO_XY(x + a, y);
            GOTO_ROW(init_y);
            if(colour) {
                GLCD_Write((font[str[offset] - 0x20][a]));
            }
            else if(!colour) {
                GLCD_Write(~(font[str[offset] - 0x20][a]));
            }
        }
        offset++;
        if(colour) {
            GLCD_Write(0x00);
        }
        else if(!colour) {
            GLCD_Write(0xFF);
        }
    }
}

char * strtok_single (char * str, char const * delims)
{
  static char  * src = NULL;

  char  *  p,  * ret = 0;

  if (str != NULL)
    src = str;

  if (src == NULL)
    return NULL;


  if ((p = strpbrk (src, delims)) != NULL) {
    *p  = 0;
    ret = src;
    src = ++p;
  }

  return ret;
}