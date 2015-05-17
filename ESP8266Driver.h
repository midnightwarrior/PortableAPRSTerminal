// ESP8266 Helper Functions and Driver for PIC32

//#include "variableDefs.h"


// Copyleft midnightwarrior, 20 February 2015
// Initial public release: v15.02b20a
// Version v15.02b20a

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
//    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

// USAGE: This header file contains all of the functions you need to read and write data to and from
// the ESP8266/a network.

int isWiFiResponding(void) {
    char WiFiResponse[50];
    // Issue AT command and checks to see if an OK is given
    clearWiFiBuffer();
    printf("AT\r\n");
    while(!is_WiFi_data_ready()) {
        receiveWiFiData();
    }
    snprintf(WiFiResponse,10,("%s", WiFiBuffer));
    snprintf(str,10,"%d", strlen(WiFiResponse));
    GLCD_RenderText_writeBytes(60,7,str,1);
    if(strcmp(WiFiResponse, "OK\r\n") == 0) {
        return 1;
    }
    return 0;
}

int retrieveNetworkList(void) {
    char *temp_token;
    char stuff[50];
    int networkNumber = 0;
    //while(1) {

    if(isWiFiResponding) {
        // The ESP8266 is responding to an AT command, so it's awake and ready to go
        // Send AT+CWLAP to get network list
        memset(visibleNetworks,0,sizeof(WiFiNetwork));
        //receiveWiFiData();
        //scrollingTerminal(WiFiBuffer);
        printf("AT+CWLAP\r\n");
        while(is_WiFi_data_ready()) {
           receiveWiFiData();
           //scrollingTerminal(WiFiBuffer);
           // Parse data - see if it's valid
           char *temp_token = strtok_single(WiFiBuffer, "(");
           snprintf(str,10,"%s",temp_token);
           //scrollingTerminal("This is what split:");
           //scrollingTerminal(str);
           //scrollingTerminal(WiFiBuffer);
           if(strcmp(str, "+CWLAP:") == 0) {
               // There was a network!
               // Do more splitting; separate out actual information
               temp_token = strtok_single(NULL, ",");
               snprintf(str,10,"%s",temp_token);
               visibleNetworks[networkNumber].security = atoi(str);
               temp_token = strtok_single(NULL, ",");
               snprintf(str,33,"%s",temp_token);
               strcpy(visibleNetworks[networkNumber].APName, str);
               temp_token = strtok_single(NULL, ",");
               snprintf(str,10,"%s",temp_token);
               visibleNetworks[networkNumber].RSSI = atoi(str);
               // Get MAC address
               temp_token = strtok_single(NULL, "\",");
               snprintf(str,18,"%s",temp_token);
               strcpy(visibleNetworks[networkNumber].MACAddress, str);
               // Get channel number
               temp_token = strtok_single(NULL, ",)");
               snprintf(str,10,"%s",temp_token);
               visibleNetworks[networkNumber].channelNumber = atoi(str);

               scrollingTerminal(visibleNetworks[networkNumber].MACAddress);
               networkNumber++;
               scrollingTerminal(("BOOM! %d", networkNumber));
           }
//           else if(strcmp(str, "busy p...") == 0) {
//               return;
//           }
        }
        snprintf(stuff, 20, "Networks visible: %d", networkNumber);
        scrollingTerminal(stuff);

    }
    else {
        return 0;
    }
}
//}