//This is the string that will hold the data sentence from the GPS
char NMEAString[82];
char rxbuffer[82];
int GPSDataReady = 0;
int GPSBufferPos = 0;
int GPSDataReading = 0;

double linelength = 0;
int satelliteSentencesReceived = 0;

// Screen refresh rate in multiples of 200ms
int screenRefreshRate = 5;

#define QUEUE_SIZE 128
typedef struct   // C method of creating new types
{
	char a[QUEUE_SIZE];
	int start;
	int end;
} Queue;

Queue RXQ, TXQ;

BYTE screenRefreshTimerPeriod = 0;
BYTE screenPage = 5;
BYTE loadNewScreenPage = 0;
BYTE numberOfScreenPages = 5;

char time[3][3];
char date[3][3];
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
int fooCounter = 0;
char str[50];