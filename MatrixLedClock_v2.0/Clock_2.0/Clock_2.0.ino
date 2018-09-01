// Using PAROLA Library
// https://majicdesigns.github.io/MD_MAX72XX/page_hardware.html
//


//Headers
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <Button.h>                      // Button library by Alexander Brevig

#include "Font_Data.h"
#include "Font_Data_STD.h"


// Turn on debug statements to the serial output
#define  DEBUG  0

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
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define MAX_DEVICES 4
#define CLK_PIN   10
#define DATA_PIN  12
#define CS_PIN    11

#define DEMO_WAIT_TIME  1000
#define SPEED_TIME  75
#define PAUSE_TIME  0
#define INTENSITY = 7;                      // Default intensity/brightness (0-15)
#define PAUSE_TIME    1000
#define SCROLL_SPEED  50

// Hardware SPI connection
//  MD_Parola P = MD_Parola(CS_PIN, MAX_DEVICES);
// Arbitrary output pins
 MD_Parola P = MD_Parola(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);


RTC_DS1307 rtc;



//global variables

char szTime[9];    // mm:ss\0
char daysOfTheWeekFull[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
char daysOfTheWeekStiped[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
char *vers[] = {"ver2.0 Beta"};
char *menu[] = {"Alarm", "Bright", "Date", "Set Time", "Set Date", "Exit",};
char *Bright[] = {"Brg:1","Brg:2","Brg:3","Brg:4","Brg:5","Brg:6","Brg:7","Brg:8","Brg:9","Brg:10","Brg:11","Brg:12","Brg:13","Brg:14","Brg:15","Brg:16",};

byte clock_mode = 0;                     // Default clock mode. Default = 0 (basic_mode)

//Setting up controller//
int UD = 0;
int LR = 0; 
Button buttonA = Button(2, BUTTON_PULLUP);      // Setup button A (using button library)
Button buttonB = Button(3, BUTTON_PULLUP);      // Setup button B (using button library)



void setup() 
{
  Serial.begin(57600);
  
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
}

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

  Serial.println(CurrentTimeNow);
   
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

void loop(void)
{

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
    P.displayReset(0);
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
      // Desenhar uma linha
      
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

  Serial.println("Date:");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  //Serial.print(" (");
  //Serial.print(daysOfTheWeekFull[now.dayOfTheWeek()]);
  //Serial.print(") ");
  Serial.println("");
  Serial.println("Time:");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

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
