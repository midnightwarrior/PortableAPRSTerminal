//This is the string that will hold the data sentence from the GPS
char NMEAString[82];
// This will hold the data from the GPS serial port
char rxbuffer[82];


int ypos=0;

int GPSDataReady = 0;
int GPSBufferPos = 0;
int GPSDataReading = 0;

double linelength = 0;
int satelliteSentencesReceived = 0;

// Screen refresh rate in multiples of 50ms
int screenRefreshRate = 20;

#define QUEUE_SIZE 8192
typedef struct   // C method of creating new types
{
	char a[QUEUE_SIZE];
	int start;
	int end;
} Queue;

// Set up struct to hold WiFi network information
// It looks like APs are sorted by channel and values are:
// (security, "name", signal strength, MAC, channel)
// where for security: 0 = open, 1 = WEP, 3 = WPA, 4 = WPA2
typedef struct
{
    BYTE security;
    char APName[33];
    int RSSI;
    char MACAddress[18];
    BYTE channelNumber;
} WiFiNetwork;

// This will hold the data from the ESP8266 WiFi module
char WiFiBuffer[QUEUE_SIZE];
int WiFiDataReady = 0;
int WiFiBufferPos = 0;

// Create an array that can handle 64 simultaneous networks
WiFiNetwork visibleNetworks[64];

Queue RXQ, TXQ;
Queue RXQ_WiFi, TXQ_WiFi;

BYTE screenRefreshTimerPeriod = 0;
BYTE screenPage = 5;
BYTE loadNewScreenPage = 0;
BYTE numberOfScreenPages = 6;

int time[3];
int date[3];
float latitude[2];
float longitude[2];
int a;
int satellitesTracked = 0;

int fixValid = 0;
int numberOfSatellites = 0;

//int** satellitesInView;
int satellitesInView[32][4];

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

BYTE instructionCount = 0x00;
char str[50];

// Global array of positions to be averaged
#define positionListSize 180
float positionsList[positionListSize][2];
BYTE averagingReady = 0;
BYTE positionsInList = 0;
float averageLat = 0;
float averageLong = 0;
int avFuncResult = 0;

// Defines for haversine function from rosettacode.org

#define R 6371
#define TO_RAD (3.1415926536 / 180)

double nsDistance = 0;
double weDistance = 0;
