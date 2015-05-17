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
void initLCD(void);
int is_uart_data_ready(void);
char uart_getchar(void);
char** str_split(char* , const char);
char * strtok_single (char *, char const *);
void NMEAParser(void);
void displayGPSData(void);
void initDebug(void);
void init_userinterface(void);
void loadScreenPage(void);
void initESP8266(void);
void receiveGPSData(void);
char WiFi_getchar(void);
void displayScrollingTerminal(void);
void clearWiFiBuffer(void);
void receiveWiFiData(void);

// Averaging

void initAveraging(void);
int addToAveragingList(void);

// Haversine function from rosettacode.org
double dist(double, double, double, double);