// PIC32-based APRS terminal project
// Copyleft 2015, midnightwarrior

//    This program is free software; you can redistribute it and/or
//    modify it under the terms of the GNU General Public License
//    as published by the Free Software Foundation; either version 2
//    of the License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <p32xxxx.h>
#include <plib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "font.h"
#include "pindefs.h"
#include "variableDefs.h"
#include "prototypes.h"
#include "PIC32GraphicsLibrary.h"
#include "PIC32GraphicsLibrary_pins.h"

// Where am I?
// Expanding the NMEA parser so I can pull out all of the information I need, and developing the
// menu system a little bit.
// I'm trying to get $GPGSV stuff out by mallocing a dynamic 2d array.  It's not working currently :(
// EDIT:  v15.01b21a statically assigns the satellite array - malloc seems too fraught with dragons and fire to be of use.

// SET VERSION NUMBER HERE!
char versionNumber[11] = "v15.02b04b";

void init(void) {
    SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);
    OpenCoreTimer(CORE_TICK_RATE);
    // Set up timer 1 as the basis for the delay_us() function
    OpenTimer1(T1_ON | T1_IDLE_CON | T1_PS_1_8 | T1_SOURCE_INT,  5000);
    // Set up timer 2 as the screen refresh clock (200ms)
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_256,  31250);
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_1);

    // Set up pushbutton interrupt
    init_userinterface();
    // Initialise LCD
    initLCD();
    // Initialise power LED
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


void _mon_putc (char c)
 {
    // Replacement printf() function
   while (U3STAbits.UTXBF); //Wait till transmission is complete
   U3TXREG = c;
 }

void init_userinterface(void) {
    // Initialises hardware for UI
    // Currently just uses bootloader pushbutton
    mCNOpen(CN_ON | CN_IDLE_CON, CN1_ENABLE, CN_PULLUP_DISABLE_ALL); //See plib documentation and PIC32 datasheet
    ConfigIntCN(CHANGE_INT_ON | CHANGE_INT_PRI_3);
    INTEnableSystemMultiVectoredInt();
}

int main(void) {
    // Main routine - do something!

    // This routine will handle getting GPS info, carting it away to a parser
    // then figuring out the relevant stuff to be fed to the display

    init();

    CS1 = 0;
    CS2 = 0;
    RST = 1;

    GLCD_ON();
    GLCD_CLR();

    Set_Start_Line(0);

    // Initialise satellites in view array
    //satellitesInView = malloc(sizeof *satellitesInView * 4);

    printf("PortableAPRS unit starting...\r\n");

    loadScreenPage();

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
                GPSBufferPos++;

                // The first part of the string is valid
                // Keep receiving!

                while(rxbuffer[GPSBufferPos-1] != 0x0D) {
                    if(is_uart_data_ready()) {
                        rxbuffer[GPSBufferPos] = uart_getchar();
                        GPSBufferPos++;
                    }
                }

                GPSDataReady = 1;
                rxbuffer[GPSBufferPos] = '\0';
                if(NMEAChecksum() == 0) {
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
    }
    return (EXIT_SUCCESS);
}

void forceDataRewrite() {
    // Set all 'changed' variables to 1 to tell the software to redraw them
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
    GLCD_ON();
    GLCD_CLR();
    Set_Start_Line(0);
    forceDataRewrite();
    snprintf(str,12," Page %d/%d ",screenPage+1, numberOfScreenPages+1);
    GLCD_RenderText_writeBytes(67,7,str,0);
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
        case 4: GLCD_DrawCircle(24, 29, 24, 1);

        // Draw SNR graph grid
                GLCD_DrawLine(64,2,64,20,0);
                GLCD_DrawLine(64,20,127,20,0);
                GLCD_DrawLine(64,11,127,11,1);
                GLCD_RenderText(52,20,"0dB");
                GLCD_RenderText(56,9,"50");
                GLCD_RenderText(56,2,"99");

                // Range from 0 - 99dB = 18 pixels
                // 6 sats per line: 64 pixels so 10 pix each
                // Lines at y=24, y=48
                GLCD_DrawLine(64,48,127,48,0);
                GLCD_DrawLine(64,30,64,48,0);
                GLCD_DrawLine(64,39,127,39,1);
                GLCD_RenderText(52,48,"0dB");
                GLCD_RenderText(56,37,"50");
                GLCD_RenderText(56,30,"99");
                break;

        case 5: GLCD_DrawCircle(24, 29, 24, 1);
                // Draw SNR graph grid
                GLCD_DrawLine(64,2,64,20,0);
                GLCD_DrawLine(64,20,127,20,0);
                GLCD_DrawLine(64,11,127,11,1);
                GLCD_RenderText(52,20,"0dB");
                GLCD_RenderText(56,9,"50");
                GLCD_RenderText(56,2,"99");

                // Range from 0 - 99dB = 18 pixels
                // 6 sats per line: 64 pixels so 10 pix each
                // Lines at y=24, y=48
                GLCD_DrawLine(64,48,127,48,0);
                GLCD_DrawLine(64,30,64,48,0);
                GLCD_DrawLine(64,39,127,39,1);
                GLCD_RenderText(52,48,"0dB");
                GLCD_RenderText(56,37,"50");
                GLCD_RenderText(56,30,"99");
                break;

    }
}

void displayGPSData(void) {
        int x, y;
        switch(screenPage) {
            case 1: if(time_Changed) {
                    time_Changed = 0;
                    GLCD_RenderText_writeBytes(0+31,0,time[0], 1);
                    GLCD_RenderText_writeBytes(18+31, 0, time[1], 1);
                    GLCD_RenderText_writeBytes(36+31, 0, time[2], 1);
                }

                    if(date_Changed) {
                        date_Changed = 0;
                        GLCD_RenderText_writeBytes(0+31,1, date[0],1);
                        GLCD_RenderText_writeBytes(18+31, 1, date[1],1);
                        GLCD_RenderText_writeBytes(36+31, 1, date[2],1);
                    }
                    if(latitude_Changed) {
                        latitude_Changed = 0;
                        snprintf(str,16,"%.0f~ %2.4f'",latitude[0],latitude[1]);
                        GLCD_RenderText_writeBytes(31,2,str,1);
                        printf(str);
                        printf("\r\n");
                    }
                    if(longitude_Changed) {
                        longitude_Changed = 0;
                        snprintf(str,116,"%.0f~ %2.4f'",longitude[0],longitude[1]);
                        GLCD_RenderText_writeBytes(31,3,str,1);
                        printf(str);
                        printf("\r\n");
                    }
                    if(altitude_msl_Changed) {
                        altitude_msl_Changed = 0;
                        snprintf(str,10,"%.1f m ", altitude_msl);
                        GLCD_RenderText_writeBytes(56,4,str,1);
                    }
                    if(numberOfSatellites_Changed) {
                        numberOfSatellites_Changed = 0;
                        snprintf(str,3,"%d ",numberOfSatellites);
                        GLCD_RenderText_writeBytes(31, 5, str,1);
                }
            break;
            case 2: if(hdop_Changed) {
                        hdop_Changed = 0;
                        snprintf(str,6,"%f",hdop);
                        GLCD_RenderText_writeBytes(70,2,str,1);
                    }
                    if(vdop_Changed) {
                        vdop_Changed = 0;
                        snprintf(str,6,"%f",vdop);
                        GLCD_RenderText_writeBytes(60,3,str,1);
                    }
                    if(pdop_Changed) {
                        pdop_Changed = 0;
                        snprintf(str,6,"%f",pdop);
                        GLCD_RenderText_writeBytes(70,4,str,1);
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
            case 4: // Display currently viewable satellites and their SNRs

                if(satellitesInView_Changed) {
                    GLCD_ON();
                    GLCD_CLR();
                    loadScreenPage();
                    satellitesInView_Changed = 0;
                    satellitesTracked = 0;
                    // Display what satellites are in view
                    for(a=0; a<satellitesInViewLength; a++) {
//                        y = (int)((float)(24/90)*(sin(satellitesInView[a][2] * PI/180) * (90 - satellitesInView[a][1])) + 32);
//                        x = (int)((float)(24/90)*(cos(satellitesInView[a][2] * PI/180) * (90 - satellitesInView[a][1])) + 64);
                        y = 29 + ((90 - satellitesInView[a][1])*sin((270+satellitesInView[a][2])*(PI/180))/4);
                        x = 24 + ((90 - satellitesInView[a][1])*cos((270+satellitesInView[a][2])*(PI/180))/4);

                        if(satellitesInView[a][0] < 10) {
                            snprintf(str,3,"0%d",satellitesInView[a][0]);

                            // Add satellite number to plot
                            if(satellitesInView[a][3] > 0) {
                            GLCD_RenderText(((satellitesTracked % 6 * 10) + 68), 22+((satellitesTracked > 5)*28), str);
                                // Do SNR bars
                                linelength = 18*((double)satellitesInView[a][3]/100);
                                GLCD_DrawBox(((satellitesTracked % 6 * 10) + 72)-1, 20+((satellitesTracked > 5)*28), ((satellitesTracked % 6 * 10) + 72)+1, 20+((satellitesTracked > 5)*28) - (int)linelength, 1);
                                satellitesTracked += 1;
                        }
                            if(!fixValid) {
                                snprintf(str,3,"??");
                            }
                            GLCD_RenderText(x, y, str);
                            //Draw_Point(x,y,1);
                        }
                        else {
                            snprintf(str,3,"%d",satellitesInView[a][0]);

                            // Add satellite number to plot
                            if(satellitesInView[a][3] > 0) {
                            GLCD_RenderText(((satellitesTracked % 6 * 10) + 68), 22+((satellitesTracked > 5)*28), str);
                            // Do SNR bars
                                linelength = 18*((double)satellitesInView[a][3]/100);
                                GLCD_DrawBox(((satellitesTracked % 6 * 10) + 72)-1, 20+((satellitesTracked > 5)*28), ((satellitesTracked % 6 * 10) + 72)+1, 20+((satellitesTracked > 5)*28) - (int)linelength, 1);
                                satellitesTracked += 1;
                        }
                            // Make each satellite number a question mark if invalid
                            if(!fixValid) {
                                snprintf(str,3,"??");
                            }
                            GLCD_RenderText(x, y, str);
                            //Draw_Point(x,y,1);
                        }
                    }
                    snprintf(str,15,"Tracking: %d",satellitesTracked);
                    GLCD_RenderText(0,56,str);
                }
                break;
            case 5: // Display currently viewable satellites and their SNRs

                if(satellitesInView_Changed) {
                    GLCD_ON();
                    GLCD_CLR();
                    loadScreenPage();
                    satellitesInView_Changed = 0;
                    satellitesTracked = 0;
                    // Display what satellites are in view (and SNRs!)
                    // Range from 0 - 99dB = 18 pixels
                    // 6 sats per line: 64 pixels so 10 pix each
                    // Lines at y=20, y=44
                    for(a=0; a<satellitesInViewLength; a++) {
                        y = 29 + ((90 - satellitesInView[a][1])*sin((270+satellitesInView[a][2])*(PI/180))/4);
                        x = 24 + ((90 - satellitesInView[a][1])*cos((270+satellitesInView[a][2])*(PI/180))/4);
                        Draw_Point(x, y, 1);
                        //printf("SN: %d, X: %d, Y: %d\r\n", satellitesInView[a][0],x, y);

                        // Add satellite number to plots
                        snprintf(str,3,"%d",satellitesInView[a][0]);
                        if(satellitesInView[a][0] < 10) {
                             snprintf(str,3,"0%d",
                                     satellitesInView[a][0]);
                        }
                        if(satellitesInView[a][3] > 0) {
                            GLCD_RenderText(((satellitesTracked % 6 * 10) + 68), 22+((satellitesTracked > 5)*28), str);
                            // Do SNR bars
                            //if(satellitesInView[a][3] != 0) {
                            //}
                                linelength = 18*((double)satellitesInView[a][3]/100);
                                GLCD_DrawBox(((satellitesTracked % 6 * 10) + 72)-1, 20+((satellitesTracked > 5)*28), ((satellitesTracked % 6 * 10) + 72)+1, 20+((satellitesTracked > 5)*28) - (int)linelength, 1);
                                satellitesTracked += 1;
                        }
                    }
                    snprintf(str,15,"Tracking: %d",satellitesTracked);
                    GLCD_RenderText(0,56,str);
                }
                break;
        }
}

void NMEAParser(void) {
    // This is the NMEA sentence parser
    char tokens[50][50];
    memset(tokens, 0, 50);
//    printf(NMEAString);
//    printf("\r\n");

    int tokenPosition = 0;

    // Get first token
    char *temp_token = strtok_single(NMEAString, ",*");
    if(temp_token == NULL) {
        //printf("No token found!\r\n");
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

        if(tokens[2][0] == 'A') {
            // Fix is valid!
            fixValid = 1;
        }
        else {
            fixValid = 0;
        }


        if(fixValid) {
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


            if(oldLatitude[0] != latitude[0] || oldLatitude[1] != latitude[1]) {
                latitude_Changed = 1;
                //printf("New latitude! - %f %f\r\n", latitude[0], latitude[1]);
            }

            if(oldLongitude[0] != longitude[0] || oldLongitude[1] != longitude[1]) {
                longitude_Changed = 1;
                //printf("New longitude! - %f %f\r\n", longitude[0], longitude[1]);
            }
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
        unsigned int numberOfMessages = atoi(tokens[1]);

        int sv, a;

        for(sv=0;sv<4;sv++) {
            if(satellitesInViewLength + sv > numberOfSatellites) {
                //break;
            }
            if(tokens[(sv + 1) * 4] != NULL) {
                svNumber[sv] = atoi(tokens[(sv + 1) * 4]);
            }
            else{
                svNumber[sv] = 0;
            }
            if(tokens[((sv + 1) * 4) + 1] != NULL)  {
                elevation[sv] = atoi(tokens[((sv + 1) * 4) + 1]);
            }
            else{
                elevation[sv] = 0;
            }
            if(tokens[((sv + 1) * 4) + 2] != NULL)  {
                azimuth[sv] = atoi(tokens[((sv + 1) * 4) + 2]);
            }
            else{
                azimuth[sv] = 0;
            }
            if(tokens[((sv + 1) * 4) + 3] != NULL)  {
                snr[sv] = atoi(tokens[((sv + 1) * 4) + 3]);
            }
            else{
                snr[sv] = 0;
            }
        }

        // Cool. Now we have our 'seen' SV list, search through the existing array
        // to see if those satellites have already been seen.  If not, then check
        // for empty space in the array and write there.
        // If the message number is 1, *clear* the array (edit: not strictly necessary)
        // and shrink it to the number of visible satellites.
        if(messageNumber == 1) {
            for(sv=0;sv<32;sv++) {
                satellitesInView[sv][0] = 0;
                satellitesInView[sv][1] = 0;
                satellitesInView[sv][2] = 0;
                satellitesInView[sv][3] = 0;
            }
            satellitesInViewLength = 0;
            satelliteSentencesReceived = 0;
        }

        // Iterate through the satellites we just got - then increment satellitesInViewLength by
        // one every time a non-NULL value for satellite SNR/SV number/yadda yadda yadda is found, and
        // add said value to the array of satellites in view

        for(sv=0;sv<4;sv++) {
            if(svNumber[sv] != NULL) {
                // There's a value here!
                satellitesInView[satellitesInViewLength][0] = svNumber[sv];
                satellitesInView[satellitesInViewLength][1] = elevation[sv];
                satellitesInView[satellitesInViewLength][2] = azimuth[sv];
                satellitesInView[satellitesInViewLength][3] = snr[sv];
                satellitesInViewLength += 1;
                //printf("Satellite: %d, SNR: %d dB\r\n", svNumber[sv], snr[sv]);
            }
            else {
                // Nothing to see here, break!
                break;
            }
        }
        satelliteSentencesReceived += 1;
        if(numberOfMessages == messageNumber) {
            // This data has been updated - push it!
            if(satelliteSentencesReceived == numberOfMessages) {
                satellitesInView_Changed = 1;
                printf("PUSH!\r\n");
            }
            else {
                printf("Satellite positional data was invalid, held back\r\n");
                satelliteSentencesReceived = 0;
                satellitesInView_Changed = 0;
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

    printf("Checksum:\r\nCalculated: %d, real: %d\r\n", checksum, number_checksum);
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
        if(screenRefreshTimerPeriod == screenRefreshRate) {
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