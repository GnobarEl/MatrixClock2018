// Using PAROLA Library
// https://majicdesigns.github.io/MD_MAX72XX/page_hardware.html
//


//Headers
#include <ESP8266WiFi.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <Button.h>                      // Button library by Alexander Brevig

#include "Font_Data.h"
#include "Font_Data_STD.h"


// Turn on debug statements to the serial output
#define  DEBUG  1

#if  DEBUG
#define PRINT(s, x) { Serial.print(F(s)); Serial.print(x); }
#define PRINTS(x) Serial.print(F(x))
#define PRINTX(x) Serial.println(x, HEX)
#else
#define PRINT(s, x)
#define PRINTS(x)
#define PRINTX(x)
#endif

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers are for ESO8266 hardware SPI and will probably not
// work with your hardware and may need to be adapted
#define MAX_DEVICES 4
#define CLK_PIN   D5 // or SCK
#define DATA_PIN  D7 // or MOSI
#define CS_PIN    D8 // or SS

#define DEMO_WAIT_TIME  1000
#define SPEED_TIME  75
#define PAUSE_TIME  0
#define INTENSITY = 7;                      // Default intensity/brightness (0-15)
#define PAUSE_TIME    1000
#define SCROLL_SPEED  50

// HARDWARE SPI
MD_Parola P = MD_Parola(CS_PIN, MAX_DEVICES);
// SOFTWARE SPI
//MD_Parola P = MD_Parola(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

RTC_DS1307 rtc;



//global variables

char szTime[9];    // mm:ss\0
char daysOfTheWeekFull[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
char daysOfTheWeekStiped[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
char *vers[] = {"ver2.1 Beta"};
char *menu[] = {"Alarm","Bright","Date","Exit",};
char *Bright[] = {"Brg:1","Brg:2","Brg:3","Brg:4","Brg:5","Brg:6","Brg:7","Brg:8","Brg:9","Brg:10","Brg:11","Brg:12","Brg:13","Brg:14","Brg:15","Brg:16",};

byte clock_mode = 0;                     // Default clock mode. Default = 0 (basic_mode)

//Setting up controller//
int UD = 0;
int LR = 0; 
Button buttonA = Button(2, BUTTON_PULLUP);      // Setup button A (using button library)
Button buttonB = Button(3, BUTTON_PULLUP);      // Setup button B (using button library)


// Setup Wi.FI
// WiFi login parameters - network name and password
const char* ssid = "osquadrados";
const char* password = "laraegaspar";
// WiFi Server object and parameters
WiFiServer server(80);

// Scrolling parameters
uint8_t frameDelay = 25;  // default frame delay value
textEffect_t  scrollEffect = PA_SCROLL_LEFT;

// Global message buffers shared by Wifi and Scrolling functions
#define BUF_SIZE  512
char curMessage[BUF_SIZE];
char newMessage[BUF_SIZE];
bool newMessageAvailable = false;

const char WebResponse[] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";

char WebPage[] =
"<!DOCTYPE html>" \
"<html>" \
"<head>" \
"<title>MajicDesigns Test Page</title>" \

"<script>" \
"strLine = \"\";" \

"function SendData()" \
"{" \
"  nocache = \"/&nocache=\" + Math.random() * 1000000;" \
"  var request = new XMLHttpRequest();" \
"  strLine = \"&MSG=\" + document.getElementById(\"data_form\").Message.value;" \
"  strLine = strLine + \"/&SD=\" + document.getElementById(\"data_form\").ScrollType.value;" \
"  strLine = strLine + \"/&I=\" + document.getElementById(\"data_form\").Invert.value;" \
"  strLine = strLine + \"/&SP=\" + document.getElementById(\"data_form\").Speed.value;" \
"  request.open(\"GET\", strLine + nocache, false);" \
"  request.send(null);" \
"}" \
"</script>" \
"</head>" \

"<body>" \
"<p><b>MD_Parola set message</b></p>" \

"<form id=\"data_form\" name=\"frmText\">" \
"<label>Message:<br><input type=\"text\" name=\"Message\" maxlength=\"255\"></label>" \
"<br><br>" \
"<input type = \"radio\" name = \"Invert\" value = \"0\" checked> Normal" \
"<input type = \"radio\" name = \"Invert\" value = \"1\"> Inverse" \
"<br>" \
"<input type = \"radio\" name = \"ScrollType\" value = \"L\" checked> Left Scroll" \
"<input type = \"radio\" name = \"ScrollType\" value = \"R\"> Right Scroll" \
"<br><br>" \
"<label>Speed:<br>Fast<input type=\"range\" name=\"Speed\"min=\"10\" max=\"200\">Slow"\
"<br>" \
"</form>" \
"<br>" \
"<input type=\"submit\" value=\"Send Data\" onclick=\"SendData()\">" \
"</body>" \
"</html>";


// Error handling
char *err2Str(wl_status_t code)
{
  switch (code)
  {
  case WL_IDLE_STATUS:    return("IDLE");           break; // WiFi is in process of changing between statuses
  case WL_NO_SSID_AVAIL:  return("NO_SSID_AVAIL");  break; // case configured SSID cannot be reached
  case WL_CONNECTED:      return("CONNECTED");      break; // successful connection is established
  case WL_CONNECT_FAILED: return("CONNECT_FAILED 1"); break; // password is incorrect
  case WL_DISCONNECTED:   return("CONNECT_FAILED 2"); break; // module is not configured in station mode
  default: return("??");
  }
}


uint8_t htoi(char c)
{
  c = toupper(c);
  if ((c >= '0') && (c <= '9')) return(c - '0');
  if ((c >= 'A') && (c <= 'F')) return(c - 'A' + 0xa);
  return(0);
}

void getData(char *szMesg, uint8_t len)
// Message may contain data for:
// New text (/&MSG=)
// Scroll direction (/&SD=)
// Invert (/&I=)
// Speed (/&SP=)
{
  char *pStart, *pEnd;      // pointer to start and end of text

  // check text message
  pStart = strstr(szMesg, "/&MSG=");
  if (pStart != NULL)
  {
    char *psz = newMessage;

    pStart += 6;  // skip to start of data
    pEnd = strstr(pStart, "/&");

    if (pEnd != NULL)
    {
      while (pStart != pEnd)
      {
        if ((*pStart == '%') && isdigit(*(pStart + 1)))
        {
          // replace %xx hex code with the ASCII character
          char c = 0;
          pStart++;
          c += (htoi(*pStart++) << 4);
          c += htoi(*pStart++);
          *psz++ = c;
        }
        else
          *psz++ = *pStart++;
      }

      *psz = '\0'; // terminate the string
      newMessageAvailable = (strlen(newMessage) != 0);
      PRINT("\nNew Msg: ", newMessage);
    }
  }

  // check scroll direction
  pStart = strstr(szMesg, "/&SD=");
  if (pStart != NULL)
  {
    pStart += 5;  // skip to start of data

    PRINT("\nScroll direction: ", *pStart);
    scrollEffect = (*pStart == 'R' ? PA_SCROLL_RIGHT : PA_SCROLL_LEFT);
    P.setTextEffect(scrollEffect, scrollEffect);
    P.displayReset();
  }

  // check invert
  pStart = strstr(szMesg, "/&I=");
  if (pStart != NULL)
  {
    pStart += 4;  // skip to start of data

    PRINT("\nInvert mode: ", *pStart);
    P.setInvert(*pStart == '1');
  }

  // check speed
  pStart = strstr(szMesg, "/&SP=");
  if (pStart != NULL)
  {
    pStart += 5;  // skip to start of data

    int16_t speed = atoi(pStart);
    PRINT("\nSpeed: ", P.getSpeed());
    P.setSpeed(speed);
    frameDelay = speed;
  }
}

void handleWiFi(void)
{
  static enum { S_IDLE, S_WAIT_CONN, S_READ, S_EXTRACT, S_RESPONSE, S_DISCONN } state = S_IDLE;
  static char szBuf[1024];
  static uint16_t idxBuf = 0;
  static WiFiClient client;
  static uint32_t timeStart;

  switch (state)
  {
  case S_IDLE:   // initialise
    PRINTS("\nS_IDLE");
    idxBuf = 0;
    state = S_WAIT_CONN;
    break;

  case S_WAIT_CONN:   // waiting for connection
  {
    client = server.available();
    if (!client) break;
    if (!client.connected()) break;

#if DEBUG
    char szTxt[20];
    sprintf(szTxt, "%03d:%03d:%03d:%03d", client.remoteIP()[0], client.remoteIP()[1], client.remoteIP()[2], client.remoteIP()[3]);
    PRINT("\nNew client @ ", szTxt);
#endif

    timeStart = millis();
    state = S_READ;
  }
  break;

  case S_READ: // get the first line of data
    PRINTS("\nS_READ ");

    while (client.available())
    {
      char c = client.read();

      if ((c == '\r') || (c == '\n'))
      {
        szBuf[idxBuf] = '\0';
        client.flush();
        PRINT("\nRecv: ", szBuf);
        state = S_EXTRACT;
      }
      else
        szBuf[idxBuf++] = (char)c;
    }
    if (millis() - timeStart > 1000)
    {
      PRINTS("\nWait timeout");
      state = S_DISCONN;
    }
    break;

  case S_EXTRACT: // extract data
    PRINTS("\nS_EXTRACT");
    // Extract the string from the message if there is one
    getData(szBuf, BUF_SIZE);
    state = S_RESPONSE;
    break;

  case S_RESPONSE: // send the response to the client
    PRINTS("\nS_RESPONSE");
    // Return the response to the client (web page)
    client.print(WebResponse);
    client.print(WebPage);
    state = S_DISCONN;
    break;

  case S_DISCONN: // disconnect client
    PRINTS("\nS_DISCONN");
    client.flush();
    client.stop();
    state = S_IDLE;
    break;

  default:  state = S_IDLE;
  }
}


void setup()
{
  Serial.begin(57600);
  PRINTS("\n[MD_Parola WiFi Message Display]\nType a message for the scrolling display from your internet browser");

  P.begin();
  P.displayClear();
  P.displaySuspend(false);

  P.displayScroll(curMessage, PA_LEFT, scrollEffect, frameDelay);

  curMessage[0] = newMessage[0] = '\0';

  // Connect to and initialise WiFi network
  PRINT("\nConnecting to ", ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    PRINT("\n", err2Str(WiFi.status()));
    delay(500);
  }
  PRINTS("\nWiFi connected");

  // Start the server
  server.begin();
  PRINTS("\nServer started");

  // Set up first message as the IP address
  //sprintf(curMessage, "%03d:%03d:%03d:%03d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  PRINT("\nAssigned IP ", curMessage);


  // Intro
  // Display version
  //delay(DEMO_WAIT_TIME);
  //void MD_Parola::displayText  (pText,textPosition_t ,speed,pause,effectIn,effectOut = PA_NO_EFFECT)   
  sprintf(curMessage, vers[0]);
 // P.displayText(vers[0], PA_CENTER, 5, 1000, PA_PRINT, PA_NO_EFFECT);
 //while (!P.displayAnimate()) yield();


if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  } else {
    Serial.println("RTC Found");
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  } else {
    Serial.println("RTC is running");
  }
  
  digitalWrite(2, HIGH);                 // turn on pullup resistor for button on pin 2
  digitalWrite(3, HIGH);                 // turn on pullup resistor for button on pin 3
  
}

/*
void setup() 
{

  Serial.begin(57600);

  P.begin();
  P.displayClear();
  P.displaySuspend(false);
  P.setInvert(false);

  // Intro
  // Display version
  delay(DEMO_WAIT_TIME);
  //void MD_Parola::displayText  (pText,textPosition_t ,speed,pause,effectIn,effectOut = PA_NO_EFFECT)   
  P.displayText(vers[0], PA_CENTER, 5, 1000, PA_PRINT, PA_NO_EFFECT);
  while (!P.displayAnimate()) yield();
 
  PRINTS("\n[MD_Parola WiFi Message Display]\nType a message for the scrolling display from your internet browser");

// Connect to and initialise WiFi network
  PRINT("\nConnecting to ", ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    PRINT("\n", err2Str(WiFi.status()));
    delay(500);
  }
  PRINTS("\nWiFi connected");

  // Start the server
  server.begin();
  PRINTS("\nServer started");

  // Set up first message as the IP address
  sprintf(curMessage, "%03d:%03d:%03d:%03d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  PRINT("\nAssigned IP ", curMessage);


  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  } else {
    Serial.println("RTC Found");
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  } else {
    Serial.println("RTC is running");
  }
  
  digitalWrite(2, HIGH);                 // turn on pullup resistor for button on pin 2
  digitalWrite(3, HIGH);                 // turn on pullup resistor for button on pin 3


}
*/
void GetCurrentTime(char *psz, bool f = true){
  String CurrentTimeNow; 
  DateTime now = rtc.now();

  String CurrentHour = String (now.hour(), DEC);
  if (CurrentHour.length() < 2){
    CurrentHour = "0" + CurrentHour;
  }

  String CurrentMinute = String (now.minute(), DEC);
  if (CurrentMinute.length() < 2){
    CurrentMinute = "0" + CurrentMinute;
  }

  CurrentTimeNow = CurrentHour + (f ? ':' : ' ') + CurrentMinute;

  const char* CharTime = CurrentTimeNow.c_str();
  char CurrentTime [10] = "";
  strcat(CurrentTime, CharTime);
  //P.displayClear();
  P.setFont(0, fixedFont);
  P.displayZoneText(0, CurrentTime, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
  while (!P.displayAnimate()) yield();  
}

void GetCurrentDate(){

  String CurrentDateNow; 
  DateTime now = rtc.now();

  String CurrentDay = String (now.day(), DEC);
  String CurrentMonth = String (now.month(), DEC);
  String CurrentYear = String (now.year(), DEC);

  CurrentDateNow = CurrentDay + '/' + CurrentMonth + '/' + CurrentYear;

  const char* CharDate = CurrentDateNow.c_str();
  char CurrentDate [10] = "";
  strcat(CurrentDate, CharDate);

  P.displayClear();
  P.displayZoneText(0, CurrentDate, PA_CENTER, SPEED_TIME, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  while (!P.displayAnimate()) yield();
}


void loop()
{
Serial.println("Loop Start");
// RTC Debug
#if DEBUG  
  DisplaySerialMonitorTime();
#endif

   switch (clock_mode){        
    case 0: 
   basic_mode();
    break; 
    case 1: 
    small_mode(); 
    break;
    case 2: 
    setup_menu(); 
    break;
  } 
}

/*
void loop(void)
{

// RTC Debug
#if DEBUG  
  DisplaySerialMonitorTime();
#endif


handleWiFi();

  if (P.displayAnimate())
  {
    if (newMessageAvailable)
    {
      strcpy(curMessage, newMessage);
      newMessageAvailable = false;
    }
    P.displayReset();
  }

/*
  switch (clock_mode){        
    case 0: 
    basic_mode();
    break; 
    case 1: 
    small_mode(); 
    break;
    case 2: 
    setup_menu(); 
    break;
  }
*/

//} */




// basic_mode()
// show the time in 5x7 characters
void basic_mode()
{

  static uint32_t lastTime = 0; // millis() memory
  static uint8_t  display = 0;  // current display mode
  static bool flasher = false;  // seconds passing flasher
  
//  P.displayAnimate();
 if (millis() - lastTime >= 1000)
  {
    lastTime = millis();
    GetCurrentTime(szTime, flasher);
    flasher = !flasher;
    //P.displayReset(0);
  }
  
  
  if (buttonA.uniquePress()) {
    setup_menu();
    return;
  }
  if (buttonB.uniquePress()) {
    display_date();
    return;
  }
}

void small_mode()
{}

void menu_brightness()
{
  int MenuExit = 0;
  P.displayClear();

  do
    {
      int val = analogRead(3);
      val = map(val, 0, 1023, 0, 20);
      P.setIntensity(val);
      P.displayZoneText(0, Bright[val], PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);
      while (!P.displayAnimate());

      if (buttonA.uniquePress()) {
        MenuExit = 1;
      }
    
    delay(100);        // delay in between reads for stability

    } while (MenuExit != 1);
}

void resetMatrix(void)
{}

void setup_menu()
{
  int MenuExit = 0;
  int currentMenuSelected = 0;

  do
    {
      int val = analogRead(3);
      val = map(val, 0, 1023, 0, 4);
      Serial.println(val);
      P.displayZoneText(0, menu[val], PA_LEFT, SCROLL_SPEED, 1, PA_PRINT, PA_NO_EFFECT);
      while (!P.displayAnimate());

      if (buttonA.uniquePress()) {
        switch (val){        
          case 0: 
          break; 
          case 1: 
          menu_brightness();
          break;
          case 3: 
          MenuExit = 1;
          break;
        }
    }
      delay(100);        // delay in between reads for stability
  
    } while (MenuExit != 1);
}

void display_date()
{GetCurrentDate();}

void DisplaySerialMonitorTime(){
  DateTime now = rtc.now();

  //Serial.println("Date:");
  //Serial.print(now.year(), DEC);
  //Serial.print('/');
  //Serial.print(now.month(), DEC);
  //Serial.print('/');
  //Serial.print(now.day(), DEC);
  //Serial.print(" (");
  //Serial.print(daysOfTheWeekFull[now.dayOfTheWeek()]);
  //Serial.print(") ");
  //Serial.println("");
  //Serial.println("Time:");
  //Serial.print(now.hour(), DEC);
  //Serial.print(':');
  //Serial.print(now.minute(), DEC);
  //Serial.print(':');
  //Serial.print(now.second(), DEC);
  //Serial.println();

  //Serial.print(" since midnight 1/1/1970 = ");
  //Serial.print(now.unixtime());
  //Serial.print("s = ");
  //Serial.print(now.unixtime() / 86400L);
  //Serial.println("d");

    // calculate a date which is 7 days and 30 seconds into the future
  //DateTime future (now + TimeSpan(7,12,30,6));

  //Serial.print(" now + 7d + 30s: ");
  //Serial.print(future.year(), DEC);
  //Serial.print('/');
  //Serial.print(future.month(), DEC);
  //Serial.print('/');
  //Serial.print(future.day(), DEC);
  //Serial.print(' ');
  //Serial.print(future.hour(), DEC);
  //Serial.print(':');
  //Serial.print(future.minute(), DEC);
  //Serial.print(':');
  //Serial.print(future.second(), DEC);
  //Serial.println();

  Serial.println();
  delay(3000);
}
