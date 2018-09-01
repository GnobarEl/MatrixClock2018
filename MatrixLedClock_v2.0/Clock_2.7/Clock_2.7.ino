      /**************
       * Inspired: "Multi-Mode Digi Uhr" by joergeli
       * Ringtone: https://github.com/acristoffers/RingtonePlayer/blob/master/examples/RingtonePlayerTest/RingtonePlayerTest.ino
       * 

      Notes:
          - Eliminar a introdução caso seja necessário libertar espaço
          - setBright é apenas para o brilho automático? Se sim, eliminar!
          - analisar se vale apena ter as funcoes
            - fade_high();
            - fade_low();
          - Corrigir BUG ID: 0001
          - remover linha ID:0002
          - clock_mode 6 = Big Font (sem animacao // --> elliminar)
       */

      /* Include libraries */
        #include "LedControl.h"                  // For assigning LED's
        #include <fontDigiClock.h>               // Font library
        #include <Wire.h>                        // DS1307 clock
        #include "RTClib.h"                      // DS1307 clock, works also with DS3231 clock
        #include <Button.h>                      // Button library by Alexander Brevig
        #include <OneWire.h>                     // This library allows you to communicate with I2C
        #include <RingtonePlayer.h>

      /* Define constants */
        #define NUM_DISPLAY_MODES 8            // Number of clock-modes  (counting zero as the first mode)
        #define NUM_SETTINGS_MODES 3           // Number of settings modes = 3 (conting zero as the first mode)
        #define SLIDE_DELAY 55                 // The time in milliseconds for the slide effect per character in slide mode. Make this higher for a slower effect
        #define cls   clear_display              // Clear display
        int pin = 8;
         
      /* Setup LED Matrix */
        // pin 12 is connected to the DataIn (DIN) on the display
        // pin 11 is connected to the CLK on the display
        // pin 10 is connected to LOAD (CS) on the display 
        
        //sets the 3 pins as 12, 11 & 10 and then sets 4 displays (max is 8 displays)
        LedControl lc = LedControl(12, 11, 10, 4);

      /* global variables */
        bool debug = true;                       // For debugging only, starts serial output (true/false)
        byte intensity = 10;                     // Startup intensity/brightness (0-15)
        byte NightIntensity = 1;                     // Startup intensity/brightness (0-15)
        bool ampm = false;                       // Define 12 or 24 hour time. false = 24 hour. true = 12 hour
        bool show_date = true;                   // Show date? - Display date approx. every 2 minutes (default = true)
        bool circle = false;                     // Define circle mode - changes the clock-mode approx. every 2 minutes. Default = true (on)
        byte clock_mode = 5;                     // Default clock mode.
                                                 // clock_mode 0 = basic mode (done)
                                                 // clock_mode 1 = small mode
                                                 // clock_mode 2 = slide mode
                                                 // clock_mode 3 = smallslide mode                                          
                                                 // clock_mode 4 = word clock
                                                 // clock_mode 5 = SuperBigFont (com animacao)
                                                 // clock_mode 6 = Big Font (sem animacao // --> elliminar)
                                                 // clock_mode 7 = setup menu
      /* ________________________________________________________________________________________ */

      /* Please don't change the following variables */                                                                                                                      
        byte old_mode = clock_mode;              // Stores the previous clock mode, so if we go to date or whatever, we know what mode to go back.
        short DN;                                // Returns the number of day in the year
        short WN;                                // Returns the number of the week in the year
        bool date_state = true;                  // Holds state of displaying date 
        int devices, dev;                        // Number of LED Matrix-Displays (dev = devices-1)
        int rtc[7];                              // Array that holds complete real time clock output
       // char dig[7];                             // Holds time-chars for shift-mode
       // char shiftChar[8];                       // Holds chars to display in shift-mode
      /*________________________________________________________________________________________*/

      //day array  (The DS1307/DS3231 outputs 1-7 values for day of week)
        char days[7][4] = {"Sun", "Mon", "Tues", "Wed", "Thur", "Fri", "Sat"};
        char daysfull[7][9] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
        char suffix[1] = {'.'};                  //date suffix "." , used in slide, basic and jumble modes - e.g. date = 25.

        RTC_DS1307 ds1307;                       // Create RTC object - works also with DS3231
        
        Button buttonA = Button(2, BUTTON_PULLUP);      // Setup button A (using button library)
        Button buttonB = Button(3, BUTTON_PULLUP);      // Setup button B (using button library)

      /*________________________________________________________________________________________*/
      void setup() {

        //playRingtone(pin,"100 8- 16#a1 16#a1 16#a1 8#f1 8#d1 8#g1 8#g1 16#g1 16c2 16c2 16#c2 16#d2 16#c2 16#c2 16#c2 8#g1 8#f1 8#a1 8#a1 16#a1 16#g1 16#g1 16#a1 16#g1 16#a1 16#a1 16#a1 8#f1 8#d1 8#g1 8#g1 16#g1 16c2 16c2 16#c2 16#d2 16#c2 16#c2 16#c2 8#g1 8#f1 8#a1 8#a1");
      
        digitalWrite(2, HIGH);                 // turn on pullup resistor for button on pin 2
        digitalWrite(3, HIGH);                 // turn on pullup resistor for button on pin 3
       
        if(debug){
        Serial.begin(9600); //start serial
        Serial.println("Debugging activated ... ");
        }
        
        //initialize the 4 matrix panels
        //we have already set the number of devices when we created the LedControl
        devices = lc.getDeviceCount();
        dev = devices-1;
        
        //we have to init all devices in a loop
        for (int address = 0; address < devices; address++) {    /*The MAX72XX is in power-saving mode on startup*/
          lc.shutdown(address, false);                           /* Set the brightness to a medium values */
          lc.setIntensity(address, intensity);                   /* and clear the display */
          lc.clearDisplay(address);
        }

      //Setup DS1307/DS3231 RTC
        #ifdef AVR
          Wire.begin();    // start I2C communication
        #else
          Wire1.begin();   // Shield I2C pins connect to alt I2C bus on Arduino
        #endif
          ds1307.begin();  //start RTC Clock - works also with DS3231
       
       //Show intro ?
       //if(show_intro){ intro(); }
       wipeBottom();

      // Show state of displaying date. toggleDateState() must! run once at startup, otherwise it shows opposite information.
      // toggleDateState();

      } // end of setup
      /*________________________________________________________________________________________*/


      /*________________________________________________________________________________________*/
      void loop() {

// start by checking the time, if hh > 21h < 07 them decrease the intensity to minimum

get_time();

byte hours = rtc[2];

          Serial.println("check");
          Serial.println("hours: ");
          Serial.println(hours);


        if (hours >= 21 || hours <= 23) { // Night time activated
           for (int address = 0; address < devices; address++) {    /*The MAX72XX is in power-saving mode on startup*/
           
            lc.setIntensity(address, 1);                   /* and clear the display */
           
        }
      }else { Serial.println("NOT check"); }

      
        if (hours >= 1 || hours <= 7) { // Night time activated
            for (int address = 0; address < devices; address++) {    /*The MAX72XX is in power-saving mode on startup*/
           
            lc.setIntensity(address, 1);                   /* and clear the display */
           
        } 
        }else { Serial.println("NOT check"); }
      

      //run the clock with whatever mode is set by clock_mode - the default is set at top of code.
        switch (clock_mode){       
        case 0: 
          basic();
          break; 
        case 1: 
          small(); 
          break;
        case 2: 
         slide(); 
          break;
        case 3: 
          //smallslide(); 
          break;  
        case 4: 
          //word_clock(); 
          break;
        case 5: 
          SuperBigFont(); 
          break;
        case 6: 
          BigChar();  
          break;
        case 7: 
          setup_menu();
          break;
        }

        
      } // end of loop
      /*________________________________________________________________________________________*/


      /* basic(= mode 0): simple mode shows the time in 5x7 characters
       ________________________________________________________________________________________*/
      
      void basic(){
        cls();            // Lets start by clearing the screen

        char buffer[3];   //for int to char conversion to turn rtc values into chars we can print on screen
        byte offset = 0;  //used to offset the x postition of the digits and centre the display when we are in 12 hour mode and the clock shows only 3 digits. e.g. 3:21
        byte x, y;        //used to draw a clear box over the left hand "1" of the display when we roll from 12:59 -> 1:00am in 12 hour mode.

        //do 12/24 hour conversion if ampm set to 1
        byte hours = rtc[2];

        if (hours > 12) {
          hours = hours - ampm * 12;
        }
        if (hours < 1) {
          hours = hours + ampm * 12;
        }

        //do offset conversion
        if (ampm && hours < 10) {
          offset = 2;
        }
        else{
          offset = 0;
        }
        
        //set the next minute we show the date at
        //set_next_date();
        
        // initially set mins to value 100 - so it wll never equal rtc[1] on the first loop of the clock, meaning we draw the clock display when we enter the function
        byte secs = 100;
        byte mins = 100;
        int count = 0;
        
        //run clock main loop as long as run_mode returns true
        while (run_mode()) {

          //get the time from the clock chip
          get_time();
         
          //check for button press
          if (buttonA.uniquePress()) { switch_mode(); return;  }
          if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }


          // display temp, when second=40 and minute=even and date_state=true
          //if(rtc[0]==40 && rtc[1] % 2 == 0 && date_state){
          //   wipeBottom();
          //   //display_temp();
          //   wipeTop();
          //   return; 
          //  }
            
          // display date, when second=40 and minute=odd and date_state = true
          //if(rtc[0]==40 && rtc[1] % 2 == 1 && date_state){
          //   display_date();
          //   return;    
          //  }

          //draw the flashing colon on/off if the secs have changed.
          if (secs != rtc[0]) {     
             secs = rtc[0];  //update secs with new value
         
            //Blink ":"
            if(secs % 2 == 0){
              plot(15, 2, 1);
              plot(15, 4, 1 );
            }
            else {
              plot(15, 2, 0);
              plot(15, 4, 0 );
            }
          }
         
          //redraw the display if button pressed or if mins != rtc[1]
          if (mins != rtc[1]) {

            //update mins and hours with the new values
            mins = rtc[1];
            hours = rtc[2];

            //adjust hours of ampm set to 12 hour mode
            if (hours > 12) { hours = hours - ampm * 12; }
            if (hours < 1) { hours = hours + ampm * 12;  }

            itoa(hours, buffer, 10);

            //if hours < 10 the num e.g. "3" hours, itoa coverts this to chars with space "3 " which we dont want
            if (hours < 10) {
              buffer[1] = buffer[0];
              buffer[0] = '0';
            }

            //print hours
            //***********
            //if we in 12 hour mode and hours < 10, then don't print the leading zero, and set the offset so we centre the display with 3 digits.
            if (ampm && hours < 10) {
              offset = 2;

              //if the time is 1:00am clear the entire display as the offset changes at this time and we need to blank out the old 12:59
              if ((hours == 1 && mins == 0) ) {
                cls();
              }
            }
            else {
              //else no offset and print hours tens digit
              offset = 0;
              
                    //if the time is 10:00am clear the entire display as the offset changes at this time and we need to blank out the old 9:59
                    if (hours == 10 && mins == 0) {
                    cls();
                    }    
               putnormalchar(3,  0, buffer[0]); // first hour digit
            }
            
            //print hours ones digit
            putnormalchar(9 - offset, 0, buffer[1]); // second hour digit

            //print mins
            //**********
            //add leading zero if mins < 10 
            itoa (mins, buffer, 10);
            if (mins < 10) {
              buffer[1] = buffer[0];
              buffer[0] = '0';
            }
            //print mins tens and mins ones digits
            putnormalchar(17 - offset, 0, buffer[0]);
            putnormalchar(23 - offset, 0, buffer[1]);
          } // end of if (mins != rtc[1]
        } // end of while run_mode
      }
      
      
      /*________________________________________________________________________________________*/


    // small(=mode 1): show the time in small 3x5 characters with seconds-dots at bottom-line
    void small() {
      char textchar[8]; // the 16 characters on the display
      byte mins = 100; //mins
      byte secs = rtc[0]; //seconds
      byte old_secs = secs; //holds old seconds value - from last time seconds were updated o display - used to check if seconds have changed
      
      cls();

      //run clock main loop as long as run_mode returns true
      while (run_mode()) {
      get_time();
      secs = rtc[0];

      //check for button presses
      if (buttonA.uniquePress()) { switch_mode();  return; }
      if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }


      //if secs changed then update them on the display
      if (secs != old_secs) {

      bottomleds(secs); // plot seconds-dots at bottomline

          char buffer[3];
          itoa(secs, buffer, 10);

          //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ".
          if (secs < 10) {
            buffer[1] = buffer[0];
            buffer[0] = '0';
          }

          puttinychar( 20, 1, ':'); //seconds colon
          puttinychar( 24, 1, buffer[0]); //seconds
          puttinychar( 28, 1, buffer[1]); //seconds
          old_secs = secs;
        }

        //if minute changes change time
        if (mins != rtc[1]) {

          //reset these for comparison next time
          mins = rtc[1];
          byte hours = rtc[2];
          if (hours > 12) {
            hours = hours - ampm * 12;
          }
          if (hours < 1) {
            hours = hours + ampm * 12;
          }


          //byte dow  = rtc[3]; // the DS1307/DS3231 outputs 0 - 6 where 0 = Sunday0 - 6 where 0 = Sunday.
          //byte date = rtc[4];

          //set characters
          char buffer[3];
          itoa(hours, buffer, 10);

          //fix - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 ".
          if (hours < 10) {
            buffer[1] = buffer[0];
            //if we are in 12 hour mode blank the leading zero.
            if (ampm) {
              buffer[0] = ' ';
            }
            else {
              buffer[0] = '0';
            }
          }
          //set hours chars
          textchar[0] = buffer[0];
          textchar[1] = buffer[1];
          textchar[2] = ':';

          itoa (mins, buffer, 10);
          if (mins < 10) {
            buffer[1] = buffer[0];
            buffer[0] = '0';
          }
          //set mins characters
          textchar[3] = buffer[0];
          textchar[4] = buffer[1];

          //do seconds
          textchar[5] = ':';
          buffer[3];
          secs = rtc[0];
          itoa(secs, buffer, 10);

          //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ".
          if (secs < 10) {
            buffer[1] = buffer[0];
            buffer[0] = '0';
          }
          //set seconds
          textchar[6] = buffer[0];
          textchar[7] = buffer[1];

          byte x = 0;
          byte y = 0;

          //print each char
          for (byte x = 0; x < 6 ; x++) {
            puttinychar( x * 4, 1, textchar[x]);
          }
        }
        delay(50);
      } // end of while run_mode
    }


    ////////////////////////////////////////////////////////////////////////////////////////

    /* BIG Char(= mode 0): simple mode shows the time in 5x7 characters
       ________________________________________________________________________________________*/
      
      void BigChar(){
        cls();            // Lets start by clearing the screen

        char buffer[3];   //for int to char conversion to turn rtc values into chars we can print on screen
        byte offset = 0;  //used to offset the x postition of the digits and centre the display when we are in 12 hour mode and the clock shows only 3 digits. e.g. 3:21
        byte x, y;        //used to draw a clear box over the left hand "1" of the display when we roll from 12:59 -> 1:00am in 12 hour mode.

        //do 12/24 hour conversion if ampm set to 1
        byte hours = rtc[2];

        if (hours > 12) {
          hours = hours - ampm * 12;
        }
        if (hours < 1) {
          hours = hours + ampm * 12;
        }

        //do offset conversion
        if (ampm && hours < 10) {
          offset = 2;
        }
        else{
          offset = 0;
        }
        
        //set the next minute we show the date at
        //set_next_date();
        
        // initially set mins to value 100 - so it wll never equal rtc[1] on the first loop of the clock, meaning we draw the clock display when we enter the function
        byte secs = 100;
        byte mins = 100;
        int count = 0;
        
        //run clock main loop as long as run_mode returns true
        while (run_mode()) {

          //get the time from the clock chip
          get_time();
         
          //check for button press
          if (buttonA.uniquePress()) { switch_mode(); return;  }
          if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }


          // display temp, when second=40 and minute=even and date_state=true
          //if(rtc[0]==40 && rtc[1] % 2 == 0 && date_state){
          //   wipeBottom();
          //   //display_temp();
          //   wipeTop();
          //   return; 
          //  }
            
          // display date, when second=40 and minute=odd and date_state = true
          //if(rtc[0]==40 && rtc[1] % 2 == 1 && date_state){
          //   display_date();
          //   return;    
          //  }

          //draw the flashing colon on/off if the secs have changed.
          if (secs != rtc[0]) {     
             secs = rtc[0];  //update secs with new value
         
            //Blink ":"
            if(secs % 2 == 0){
              plot(15, 2, 1);
              plot(15, 4, 1 );
            }
            else {
              plot(15, 2, 0);
              plot(15, 4, 0 );
            }
          }
         
          //redraw the display if button pressed or if mins != rtc[1]
          if (mins != rtc[1]) {

            //update mins and hours with the new values
            mins = rtc[1];
            hours = rtc[2];

            //adjust hours of ampm set to 12 hour mode
            if (hours > 12) { hours = hours - ampm * 12; }
            if (hours < 1) { hours = hours + ampm * 12;  }

            itoa(hours, buffer, 10);

            //if hours < 10 the num e.g. "3" hours, itoa coverts this to chars with space "3 " which we dont want
            if (hours < 10) {
              buffer[1] = buffer[0];
              buffer[0] = '0';
            }

            //print hours
            //***********
            //if we in 12 hour mode and hours < 10, then don't print the leading zero, and set the offset so we centre the display with 3 digits.
            if (ampm && hours < 10) {
              offset = 2;

              //if the time is 1:00am clear the entire display as the offset changes at this time and we need to blank out the old 12:59
              if ((hours == 1 && mins == 0) ) {
                cls();
              }
            }
            else {
              //else no offset and print hours tens digit
              offset = 0;
              
                    //if the time is 10:00am clear the entire display as the offset changes at this time and we need to blank out the old 9:59
                    if (hours == 10 && mins == 0) {
                    cls();
                    }    
               putbigchar(1,  0, buffer[0]); // first hour digit
            }
            
            //print hours ones digit
            putbigchar(8 - offset, 0, buffer[1]); // second hour digit

            //print mins
            //**********
            //add leading zero if mins < 10 
            itoa (mins, buffer, 10);
            if (mins < 10) {
              buffer[1] = buffer[0];
              buffer[0] = '0';
            }
            //print mins tens and mins ones digits
            putbigchar(17 - offset, 0, buffer[0]);
            putbigchar(24 - offset, 0, buffer[1]);
          } // end of if (mins != rtc[1]
        } // end of while run_mode
      }
      
      
      /*________________________________________________________________________________________*/


/* Super BIG Char(= mode 0): simple mode shows the time in 5x7 characters
       ________________________________________________________________________________________*/
      
      void SuperBigFont(){
       byte digits_old[4] = {99, 99, 99, 99}; //old values  we store time in. Set to somthing that will never match the time initially so all digits get drawn wnen the mode starts
        byte digits_new[4]; //new digits time will slide to reveal
        byte digits_x_pos[4] = {24, 17, 8, 1}; //x pos for which to draw each digit at

        char old_char[2]; //used when we use itoa to transpose the current digit (type byte) into a char to pass to the animation function
        char new_char[2]; //used when we use itoa to transpose the new digit (type byte) into a char to pass to the animation function

        //old_chars - stores the 5 day and date suffix chars on the display. e.g. "mon" and "st". We feed these into the slide animation as the current char when these chars are updated.
        //We sent them as A initially, which are used when the clocl enters the mode and no last chars are stored.
        //char old_chars[6] = "AAAAA";

        cls();
        
        // plot the clock colon on the display
        //  putnormalchar( 13, 0, ':');

        byte old_secs = rtc[0]; //store seconds in old_secs. We compare secs and old secs. WHen they are different we redraw the display

        //run clock main loop as long as run_mode returns true
        while (run_mode()) {

          get_time();
          byte secs =rtc[0];


        // display date, when second=40 and date_state = true
        // BUG ID: 0001
        if(rtc[0]==85 && date_state){ //This is a bug /* REVIEW */
          // display_date();
          // return;    
          }
            

          //check for button press
          if (buttonA.uniquePress()) { switch_mode();  return; }
          if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }

          //if secs have changed then update the display
          if (rtc[0] != old_secs) {
            //Blink ":"
            if(old_secs % 2 == 0){
              plot(15, 2, 1);
              plot(15, 4, 1 );
            }
            else {
              plot(15, 2, 0);
              plot(15, 4, 0 );
            }
           
            old_secs = rtc[0];

            //do 12/24 hour conversion if ampm set to 1
            byte hours = rtc[2];
            if (hours > 12) {
              hours = hours - ampm * 12;
            }
            if (hours < 1) {
              hours = hours + ampm * 12;
            }

            //split all date and time into individual digits - stick in digits_new array

            //rtc[0] = secs                        //array pos and digit stored
            //digits_new[0] = (rtc[0]%10);           //0 - secs ones
            //digits_new[1] = ((rtc[0]/10)%10);      //1 - secs tens
            //rtc[1] = mins
            digits_new[0] = (rtc[1] % 10);         //2 - mins ones
            digits_new[1] = ((rtc[1] / 10) % 10);  //3 - mins tens
            //rtc[2] = hours
            digits_new[2] = (hours % 10);         //4 - hour ones
            digits_new[3] = ((hours / 10) % 10);  //5 - hour tens
            //rtc[4] = date
            //digits_new[6] = (rtc[4]%10);           //6 - date ones
            //digits_new[7] = ((rtc[4]/10)%10);      //7 - date tens

            //draw initial screen of all chars. After this we just draw the changes.

            //compare digits 0 to 3 (mins and hours)
            for (byte i = 0; i <= 3; i++) {
              //see if digit has changed...
              if (digits_old[i] != digits_new[i]) {

                //run 9 step animation sequence for each in turn
                for (byte seq = 0; seq <= 8 ; seq++) {

                  //convert digit to string
                  itoa(digits_old[i], old_char, 10);
                  itoa(digits_new[i], new_char, 10);

                  //if set to 12 hour mode and we're on digit 2 (hours tens mode) then check to see if this is a zero. If it is, blank it instead so we get 2.00pm not 02.00pm
                  if (ampm && i == 3) {
                    if (digits_new[3] == 0) {
                      new_char[0] = ' ';
                    }
                    if (digits_old[3] == 0) {
                      old_char[0] = ' ';
                    }
                  }
                  //draw the animation frame for each digit
                  Superslideanim(digits_x_pos[i], 0, seq, old_char[0], new_char[0]);
                  delay(SLIDE_DELAY);
                }
              }
            }

       
            //save digita array tol old for comparison next loop
            for (byte i = 0; i <= 3; i++) {
              digits_old[i] =  digits_new[i];
            }
          }// end of secs/oldsecs
        }// end of while run_mode
      }
      
      
      /*________________________________________________________________________________________*/

      
      /* Big-Slide mode (=mode 2): like basic-mode, but with sliding digits top-down
      /*________________________________________________________________________________________*/
      void slide() {
        byte digits_old[4] = {99, 99, 99, 99}; //old values  we store time in. Set to somthing that will never match the time initially so all digits get drawn wnen the mode starts
        byte digits_new[4]; //new digits time will slide to reveal
        byte digits_x_pos[4] = {23, 17, 9, 3}; //x pos for which to draw each digit at

        char old_char[2]; //used when we use itoa to transpose the current digit (type byte) into a char to pass to the animation function
        char new_char[2]; //used when we use itoa to transpose the new digit (type byte) into a char to pass to the animation function

        //old_chars - stores the 5 day and date suffix chars on the display. e.g. "mon" and "st". We feed these into the slide animation as the current char when these chars are updated.
        //We sent them as A initially, which are used when the clocl enters the mode and no last chars are stored.
        //char old_chars[6] = "AAAAA";

        cls();
        
        // plot the clock colon on the display
        //  putnormalchar( 13, 0, ':');

        byte old_secs = rtc[0]; //store seconds in old_secs. We compare secs and old secs. WHen they are different we redraw the display

        //run clock main loop as long as run_mode returns true
        while (run_mode()) {

          get_time();
          byte secs =rtc[0];


        // display date, when second=40 and date_state = true
        // BUG ID: 0001
        if(rtc[0]==85 && date_state){ //This is a bug /* REVIEW */
           display_date();
           return;    
          }
            

          //check for button press
          if (buttonA.uniquePress()) { switch_mode();  return; }
          if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }

          //if secs have changed then update the display
          if (rtc[0] != old_secs) {
            //Blink ":"
            if(old_secs % 2 == 0){
              plot(15, 2, 1);
              plot(15, 4, 1 );
            }
            else {
              plot(15, 2, 0);
              plot(15, 4, 0 );
            }
           
            old_secs = rtc[0];

            //do 12/24 hour conversion if ampm set to 1
            byte hours = rtc[2];
            if (hours > 12) {
              hours = hours - ampm * 12;
            }
            if (hours < 1) {
              hours = hours + ampm * 12;
            }

            //split all date and time into individual digits - stick in digits_new array

            //rtc[0] = secs                        //array pos and digit stored
            //digits_new[0] = (rtc[0]%10);           //0 - secs ones
            //digits_new[1] = ((rtc[0]/10)%10);      //1 - secs tens
            //rtc[1] = mins
            digits_new[0] = (rtc[1] % 10);         //2 - mins ones
            digits_new[1] = ((rtc[1] / 10) % 10);  //3 - mins tens
            //rtc[2] = hours
            digits_new[2] = (hours % 10);         //4 - hour ones
            digits_new[3] = ((hours / 10) % 10);  //5 - hour tens
            //rtc[4] = date
            //digits_new[6] = (rtc[4]%10);           //6 - date ones
            //digits_new[7] = ((rtc[4]/10)%10);      //7 - date tens

            //draw initial screen of all chars. After this we just draw the changes.

            //compare digits 0 to 3 (mins and hours)
            for (byte i = 0; i <= 3; i++) {
              //see if digit has changed...
              if (digits_old[i] != digits_new[i]) {

                //run 9 step animation sequence for each in turn
                for (byte seq = 0; seq <= 8 ; seq++) {

                  //convert digit to string
                  itoa(digits_old[i], old_char, 10);
                  itoa(digits_new[i], new_char, 10);

                  //if set to 12 hour mode and we're on digit 2 (hours tens mode) then check to see if this is a zero. If it is, blank it instead so we get 2.00pm not 02.00pm
                  if (ampm && i == 3) {
                    if (digits_new[3] == 0) {
                      new_char[0] = ' ';
                    }
                    if (digits_old[3] == 0) {
                      old_char[0] = ' ';
                    }
                  }
                  //draw the animation frame for each digit
                  slideanim(digits_x_pos[i], 0, seq, old_char[0], new_char[0]);
                  delay(SLIDE_DELAY);
                }
              }
            }

       
            //save digita array tol old for comparison next loop
            for (byte i = 0; i <= 3; i++) {
              digits_old[i] =  digits_new[i];
            }
          }// end of secs/oldsecs
        }// end of while run_mode
      }
      /*________________________________________________________________________________________*/



         

      /* plot a point on the display */
      void plot (byte x, byte y, byte val) {

        //select which matrix depending on the x coord
        byte address;
        if (x >= 0 && x <= 7)   {
          address = 3;
        }
        if (x >= 8 && x <= 15)  {
          address = 2;
          x = x - 8;
        }
        if (x >= 16 && x <= 23) {
          address = 1;
          x = x - 16;
        }
        if (x >= 24 && x <= 31) {
          address = 0;
          x = x - 24;
        }

        if (val == 1) {
          lc.setLed(address, y, x, true);
        } else {
          lc.setLed(address, y, x, false);
        }
      }
      /*________________________________________________________________________________________*/

      //clear screen
      void clear_display() {
        for (byte address = 0; address < 4; address++) {
          lc.clearDisplay(address);
        }
      }


      //display date - show dayname, date, month, year, week of year in 4 steps
      void display_date(){
        int date_delay = 70;  // delay between displaying next character
          
        wipeBottom();     //wipe out devices  

        //read the date from the DS1307/DS3231
        byte dow = rtc[3]; // day of week 0 = Sunday
        byte date = rtc[4];
        byte month = rtc[5] - 1;
        byte year = rtc[6]-2000;

        //array of month names to print on the display. Some are shortened as we only have 8 characters across to play with
        //  char monthnames[12][9] = {
        //    "Januar", "Februar", "Maerz", "April", "Mai", "Juni", "Juli", "August", "Septemb.", "Oktober", "November", "Dezember"
        //  };

        char monthnames[12][4] = {
          "Jan", "Feb", "Mrz", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"
        };

        //----------- print the day name ----------- //
        //get length of text in pixels, that way we can centre it on the display by divindin the remaining pixels b2 and using that as an offset
        byte len = 0;
        while(daysfull[dow][len]) { 
          len++; 
        }; 
        byte offset = (31 - ((len-1)*4)) / 2; //our offset to centre up the text
         
        int i = 0;
        while(daysfull[dow][i]){
          puttinychar((i*4) + offset , 1, daysfull[dow][i]);
          delay(date_delay); 
          i++;
        }
              //hold display but check for button presses
              int counter = 1000;
              while (counter > 0){
              if (buttonA.uniquePress()) { switch_mode();  return; }
              if (buttonB.uniquePress()) { toggleDateState();  return; }
              delay(1);
              counter--;
              }
        cls();

      
        //----------- print date numerals ----------- //
        char buffer[3];
        //if date < 10 add a 0
        itoa(date,buffer,10);
           if (date < 10) {
             buffer[1] = buffer[0];
             buffer[0] = '0';
            }
        offset = 5;      
        puttinychar(0+offset, 1, buffer[0]);  //print the 1st date number
        delay(date_delay);
        puttinychar(4+offset, 1, buffer[1]);  //print the 2nd date number
        delay(date_delay);
        puttinychar(8+offset, 1, suffix[0]);  //print suffix -  char suffix[1]={'.'}; is defined at top of code
        delay(90);

        
        //----------- print month name ----------- // 
        //get length of text in pixels, that way we can centre it on the display by divindin the remaining pixels b2 and using that as an offset
        len = 0;
        while(monthnames[month][len]) { 
          len++; 
        }; 
        //offset = (31 - ((len-1)*4)) / 2; //our offset to centre up the text
        offset = 17;
        i = 0;
        while(monthnames[month][i]){
          puttinychar((i*4) +offset, 1, monthnames[month][i]);
          delay(date_delay); 
          i++; 
        }
              //hold display but check for button presses
              counter = 1000;
              while (counter > 0){
              if (buttonA.uniquePress()) { switch_mode();  return; }
              if (buttonB.uniquePress()) { toggleDateState();  return; }
              delay(1);
              counter--;
              }
        cls();


        //----------- print year ----------- //
        offset = 9; //offset to centre text - e.g. 2016
        char buffer_y[3] = "20";
        puttinychar(0+offset , 1, buffer_y[0]);   //print the 1st year number: 2
        delay(date_delay);
        puttinychar(4+offset , 1, buffer_y[1]);   //print the 2nd year number: 0 
        delay(date_delay);
        itoa(year,buffer,10);                     //if year < 10 add a 0
         if (year < 10) {
             buffer[1] = buffer[0];
             buffer[0] = '0';
            }
        puttinychar(8+offset, 1, buffer[0]);      //print the 1st year number
        delay(date_delay);
        puttinychar(12+offset, 1, buffer[1]);     //print the 2nd year number
        delay(1000);
        cls();

        //----------- print week of year ----------- //
        offset = 1;
        char buffer_w[6] = "Woche";
        puttinychar(0+offset , 1, buffer_w[0]);   //print "W"
        delay(date_delay);
        puttinychar(4+offset , 1, buffer_w[1]);   //print "o"
        delay(date_delay);
        puttinychar(8+offset , 1, buffer_w[2]);   //print "c"
        delay(date_delay);
        puttinychar(12+offset , 1, buffer_w[3]);   //print "h"
        delay(date_delay);
        puttinychar(16+offset , 1, buffer_w[4]);   //print "e"
        delay(date_delay);
        itoa(WN,buffer,10);                        //if week < 10 add a 0
         if (WN < 10) {
             buffer[1] = buffer[0];
             buffer[0] = '0';
            }
        puttinychar(23+offset, 1, buffer[0]);      //print the 1st week number
        delay(date_delay);
        puttinychar(27+offset, 1, buffer[1]);      //print the 2nd week number
              //hold display but check for button presses
              counter = 1000;
              while (counter > 0){
              if (buttonA.uniquePress()) { switch_mode();  return; }
              if (buttonB.uniquePress()) { toggleDateState();  return; }
              delay(1);
              counter--;
              }
        wipeTop();  //wipe out devices

      } // end of display_date

      ////////////////////////////////////////////////////////////////////////////////////////

      //putnormalchar:
      //Copy a 5x7 character glyph from the myfont data structure to display memory
      void putnormalchar(byte x, byte y, char c){
        byte dots;
        if (c >= 'A' && c <= 'Z' ) {
          c &= 0x1F;   // A-Z maps to 1-26
        }
        else if (c >= 'a' && c <= 'z') {
          c = (c - 'a') + 41;   // A-Z maps to 41-67
        }
        else if (c >= '0' && c <= '9') {
          c = (c - '0') + 31;
        }
        else if (c == ' ') {
          c = 0; // space
        }
        else if (c == '.') {
          c = 27; // full stop
        }
        else if (c == '\'') {
          c = 28; // single quote mark
        }
        else if (c == ':') {
          c = 29; // colon
        }
        else if (c == '>') {
          c = 30; // clock_mode selector arrow
        }
        else if (c == '=') {
          c = 79; // equal sign
        }

        
        else if (c >= -80 && c <= -67) {
          c *= -1;
        }

        for (char col = 0; col < 5; col++) {
          dots = pgm_read_byte_near(&myfont[c][col]);
          for (char row = 0; row < 7; row++) {
            //check coords are on screen before trying to plot
            //if ((x >= 0) && (x <= 31) && (y >= 0) && (y <= 7)){

            if (dots & (64 >> row)) {   // only 7 rows.
              plot(x + col, y + row, 1);
            } else {
              plot(x + col, y + row, 0);
            }
            //}
          }
        }
      }

      ////////////////////////////////////////////////////////////////////////////////////////


      //putbigchar:
      //Copy a 6x8 character glyph from the myfont data structure to display memory
      void putbigchar(byte x, byte y, char c){
        byte dots;
        if (c >= 'A' && c <= 'Z' ) {
          c &= 0x1F;   // A-Z maps to 1-26
        }
        else if (c >= 'a' && c <= 'z') {
          c = (c - 'a') + 41;   // A-Z maps to 41-67
        }
        else if (c >= '0' && c <= '9') {
          c = (c - '0') + 31;
        }
        else if (c == ' ') {
          c = 0; // space
        }
        else if (c == '.') {
          c = 27; // full stop
        }
        else if (c == '\'') {
          c = 28; // single quote mark
        }
        else if (c == ':') {
          c = 29; // colon
        }
        else if (c == '>') {
          c = 30; // clock_mode selector arrow
        }
        else if (c == '=') {
          c = 79; // equal sign
        }
        else if (c >= -80 && c <= -67) {
          c *= -1;
        }

        for (char col = 0; col < 6; col++) {
          dots = pgm_read_byte_near(&myBigFont[c][col]);
          for (char row = 0; row < 8; row++) {
            //check coords are on screen before trying to plot
            //if ((x >= 0) && (x <= 31) && (y >= 0) && (y <= 7)){

            if (dots & (128 >> row)) {   // only 7 rows.
              plot(x + col, y + row, 1);
            } else {
              plot(x + col, y + row, 0);
            }
            //}
          }
        }
      }

      ////////////////////////////////////////////////////////////////////////////////////////

    /*
      //putbigchar:
      //Copy a 5x7 character glyph from the myfont data structure to display memory
      void putbigchar(byte x, byte y, char c){
        Serial.print("Char C: ");
        Serial.println(c);
        byte dots;
      /*
        if (c >= 'A' && c <= 'Z' ) {
          c &= 0x1F;   // A-Z maps to 1-26
        }
        else if (c >= 'a' && c <= 'z') {
          c = (c - 'a') + 41;   // A-Z maps to 41-67
        }
        else if (c >= '0' && c <= '9') {
          c = (c - '0') + 31;
        }
        else if (c == ' ') {
          c = 0; // space
        }
        else if (c == '.') {
          c = 27; // full stop
        }
        else if (c == '\'') {
          c = 28; // single quote mark
        }
        else if (c == ':') {
          c = 29; // colon
        }
        else if (c == '>') {
          c = 30; // clock_mode selector arrow
        }
        else if (c == '=') {
          c = 79; // equal sign
        }

        if (c == '1') {
          c = 1; // equal sign
        }
        
        else if (c >= -80 && c <= -67) {
          c *= -1;
        }

        for (int col = 0; col < 8; col++) {
          dots = pgm_read_byte_near(&myBigFont[c][col]);
            Serial.print("COL: ");
            Serial.println(col);
            Serial.print("DOTS: ");
            Serial.println(dots);
          for (int row = 0; row < 8; row++) {
              Serial.print("ROW: ");
              Serial.println(row);
            //check coords are on screen before trying to plot
            //if ((x >= 0) && (x <= 31) && (y >= 0) && (y <= 7)){

            if (dots & (128 >> row)) {   // only 7 rows.
              plot(x + col, y + row, 1);
            } else {
              plot(x + col, y + row, 0);
            }
           // }
          }
        }
      }
    */
      ////////////////////////////////////////////////////////////////////////////////////////

      // puttinychar:
      // Copy a 3x5 character glyph from the myfont data structure to display memory, with its upper left at the given coordinate
      // This is unoptimized and simply uses plot() to draw each dot.
      void puttinychar(byte x, byte y, char c){
        byte dots;
        if (c >= 'A' && c <= 'Z' || (c >= 'a' && c <= 'z') ) {
          c &= 0x1F;   // A-Z maps to 1-26
        }
        else if (c >= '0' && c <= '9') {
          c = (c - '0') + 32;
        }
        else if (c == ' ') {
          c = 0; // space
        }
        else if (c == '.') {
          c = 27; // full stop
        }
        else if (c == ':') {
          c = 28; // colon
        }
        else if (c == '\'') {
          c = 29; // single quote mark
        }
        else if (c == '!') {
          c = 30; // exclamation mark
        }
        else if (c == '?') {
          c = 31; // question mark
        }
        else if (c == '-') {
          c = 42; // hyphen
        }
        else if (c == '#') {
          c = 43; // degree-symbol
        }
        else if (c == '>') {
          c = 44; // selector-arrow
        }
        else if (c == '~') {
          c = 45; // Ãœ
        }
        else if (c == '*') {
          c = 46; // Ã–
        }


        
        
        for (byte col = 0; col < 3; col++) {
          dots = pgm_read_byte_near(&mytinyfont[c][col]);
          for (char row = 0; row < 5; row++) {
            if (dots & (16 >> row))
              plot(x + col, y + row, 1);
            else
              plot(x + col, y + row, 0);
          }
        }
      }

      ////////////////////////////////////////////////////////////////////////////////////////


      ////////////////////////////////////////////////////////////////////////////////////////

      //run clock main loop as long as run_mode returns true
      byte run_mode() {
        setBright();  // 
        return 1;
      }

      ////////////////////////////////////////////////////////////////////////////////////////


      ////////////////////////////////////////////////////////////////////////////////////////

      // setBright: set the brightness to a value between 0 and 15 (= 16 steps, in dependence of LDR)
      int setBright(){
        // map LDR-values from 0 to 15 and set the brightness of devices
        int brightness = 10;

        //we have to init all devices in a loop
        for (int address = 0; address < devices; address++) {
          lc.setIntensity(address, brightness);
        }
        return brightness;
      }

      ////////////////////////////////////////////////////////////////////////////////////////

      ////////////////////////////////////////////////////////////////////////////////////////

      //wipeTop: wipe-effect from top to bottom
      void wipeTop(){
        for(int r=0; r<=8; r++){
          for(int c=0; c<32; c++){
              plot (c, r, 1);
              plot (c, r-1, 0);        
              }
        }
      } // end of wipeTop


      ////////////////////////////////////////////////////////////////////////////////////////

      //wipeBottom: wipe-effect from bottom to top
      void wipeBottom(){
      //bottom to top
        for(int r=7; r>=(-1); r--){
          for(int c=0; c<32; c++){
              plot (c, r, 1);
              plot (c, r+1, 0);        
              }
        }
      } // end of wipeBottom


      ////////////////////////////////////////////////////////////////////////////////////////

      //wipeMiddle: wipe-effect from left and right to the middle
      void wipeMiddle(){
       for(int c=0; c<=31; c++){   
            for(int r=7; r>=0; r--){
                plot (c, r, 1);
                plot (32-c, r, 1);    
            }
       delay(10);
           
        for(int r=7; r>=0; r--){ 
            plot (c, r, 0);
                  if(c != 16){
                      plot (32-c, r, 0);
                  }
                  else{
                      plot (c, 0, 0); delay(50);
                      plot (c, 7, 0); delay(50);
                      plot (c, 1, 0); delay(50);
                      plot (c, 6, 0); delay(50);
                      plot (c, 2, 0); delay(50);
                      plot (c, 5, 0); delay(50);
                      plot (c, 3, 0); delay(50);
                      plot (c, 4, 0); delay(600);                 
                      return;
                  }
        }
       }
      } // end of wipeMiddle

      ////////////////////////////////////////////////////////////////////////////////////////

      //wipeOutside: wipe-effect from both sides over the middle to the other sides
      void wipeOutside(){
        for(int c=0; c<32; c++){
          for(int r=7; r>=0; r--){
              plot (c, r, 1);
              plot (32-c, r, 1);    
              }
           delay(5);
           for(int r=7; r>=0; r--){      
              plot (c, r, 0);
                if(c != 16){
                  plot (32-c, r, 0);
                  }
           }
        }
      delay(300);
      } // end of wipeOutside

      ////////////////////////////////////////////////////////////////////////////////////////


      // wipeInside - looks like random-clearing of dots 
      // (for testing set all dots to 1)
      void wipeInside(){

       int verz=5;  // delay between plotting each dot

       int rh=7;
       int rl=0;
       for(int row=0; row<4; row++){
            for(int col=0; col<8; col++){      
              plot(col,    rh, 0); delay(verz);
              plot(col,    rl, 0); delay(verz); 
              plot(31-col, rh, 0); delay(verz);
              plot(31-col, rl, 0); delay(verz);           
            }
           rh--;
           rl++;
       }

       rh=7;
       rl=0;
       for(int row=0; row<4; row++){        
            for(int col=0; col<8; col++){
              plot(8+col,  rh, 0); delay(verz);
              plot(8+col,  rl, 0); delay(verz);
              plot(23-col, rh, 0); delay(verz); 
              plot(23-col, rl, 0); delay(verz);
            }
            rh--;
            rl++;      
       }

       delay(300);
          
      } // end of wipeInside

      ////////////////////////////////////////////////////////////////////////////////////////


      // fade_high: fade intensity from 0 to brightness (in dependence of LDR)
      void fade_high() {

        // map LDR-values from 0 to 15
        int brightness = 10;
        
        //fade from intensity 0 to brightness and set the brightness of devices
        for (byte f=0; f<=brightness; f++) {
          for (byte address = 0; address < 4; address++) {
            lc.setIntensity(address, f);
          }
          delay(120); //change this to alter fade-up speed
        }
        return;
      }

      ////////////////////////////////////////////////////////////////////////////////////////
      // fade_low: fade intensity from brightness (in dependence of LDR) to 0
      void fade_low() {

        // map LDR-values from 0 to 15
        int brightness = 10;
       
        //fade from brightness to 1 and set the brightness of devices
        for (byte f=brightness; f>0; f--) {
          for (byte address = 0; address < 4; address++) {
            lc.setIntensity(address, f);
          }
          delay(120); //change this to alter fade-low speed
        }  
        for (byte address = 0; address < 4; address++) {
          lc.setIntensity(address, 0);  // set intensity to lowest level
        }
        return;
      }

      ////////////////////////////////////////////////////////////////////////////////////////


      //called by slide
      //this draws the animation of one char sliding on and the other sliding off. There are 8 steps in the animation, we call the function to draw one of the steps from 0-7
      //inputs are are char x and y, animation frame sequence (0-7) and the current and new chars being drawn.
      void slideanim(byte x, byte y, byte sequence, char current_c, char new_c) {

        //  To slide one char off and another on we need 9 steps or frames in sequence...

        //  seq# 0123456 <-rows of the display
        //   |   |||||||
        //  seq0 0123456  START - all rows of the display 0-6 show the current characters rows 0-6
        //  seq1  012345  current char moves down one row on the display. We only see it's rows 0-5. There are at display positions 1-6 There is a blank row inserted at the top
        //  seq2 6 01234  current char moves down 2 rows. we now only see rows 0-4 at display rows 2-6 on the display. Row 1 of the display is blank. Row 0 shows row 6 of the new char
        //  seq3 56 0123
        //  seq4 456 012  half old / half new char
        //  seq5 3456 01
        //  seq6 23456 0
        //  seq7 123456
        //  seq8 0123456  END - all rows show the new char

        //from above we can see...
        //currentchar runs 0-6 then 0-5 then 0-4 all the way to 0. starting Y position increases by 1 row each time.
        //new char runs 6 then 5-6 then 4-6 then 3-6. starting Y position increases by 1 row each time.

        //if sequence number is below 7, we need to draw the current char
        if (sequence < 7) {
          byte dots;
          if (current_c >= 'A' && current_c <= 'Z' ) {
            current_c &= 0x1F;   // A-Z maps to 1-26
          }
          else if (current_c >= 'a' && current_c <= 'z') {
            current_c = (current_c - 'a') + 41;   // a-z maps to 41-66
          }
          else if (current_c >= '0' && current_c <= '9') {
            current_c = (current_c - '0') + 31;
          }
          else if (current_c == ' ') {
            current_c = 0; // space
          }
          else if (current_c == '.') {
            current_c = 27; // full stop
          }
          else if (current_c == '\'') {
            current_c = 28; // single quote mark
          }
          else if (current_c == ':') {
            current_c = 29; //colon
          }
          else if (current_c == '>') {
            current_c = 30; // clock_mode selector arrow
          }

          byte curr_char_row_max = 7 - sequence; //the maximum number of rows to draw is 6 - sequence number
          byte start_y = sequence; //y position to start at - is same as sequence number. We inc this each loop

          //plot each row up to row maximum (calculated from sequence number)
          for (byte curr_char_row = 0; curr_char_row <= curr_char_row_max; curr_char_row++) {
            for (byte col = 0; col < 5; col++) {
              dots = pgm_read_byte_near(&myfont[current_c][col]);
              if (dots & (64 >> curr_char_row))
                plot(x + col, y + start_y, 1); //plot led on
              else
                plot(x + col, y + start_y, 0); //else plot led off
            }
            start_y++;//add one to y so we draw next row one down
          }
        }

        //draw a blank line between the characters if sequence is between 1 and 7. If we don't do this we get the remnants of the current chars last position left on the display
        if (sequence >= 1 && sequence <= 8) {
          for (byte col = 0; col < 5; col++) {
            plot(x + col, y + (sequence - 1), 0); //the y position to draw the line is equivalent to the sequence number - 1
          }
        }

        //if sequence is above 2, we also need to start drawing the new char
        if (sequence >= 2) {

          //work out char
          byte dots;
          //if (new_c >= 'A' && new_c <= 'Z' || (new_c >= 'a' && new_c <= 'z') ) {
          //  new_c &= 0x1F;   // A-Z maps to 1-26
          //}
          if (new_c >= 'A' && new_c <= 'Z' ) {
            new_c &= 0x1F;   // A-Z maps to 1-26
          }
          else if (new_c >= 'a' && new_c <= 'z') {
            new_c = (new_c - 'a') + 41;   // A-Z maps to 41-67
          }
          else if (new_c >= '0' && new_c <= '9') {
            new_c = (new_c - '0') + 31;
          }
          else if (new_c == ' ') {
            new_c = 0; // space
          }
          else if (new_c == '.') {
            new_c = 27; // full stop
          }
          else if (new_c == '\'') {
            new_c = 28; // single quote mark
          }
          else if (new_c == ':') {
            new_c = 29; // clock_mode selector arrow
          }
          else if (new_c == '>') {
            new_c = 30; // clock_mode selector arrow
          }

          byte newcharrowmin = 6 - (sequence - 2); //minimumm row num to draw for new char - this generates an output of 6 to 0 when fed sequence numbers 2-8. This is the minimum row to draw for the new char
          byte start_y = 0; //y position to start at - is same as sequence number. we inc it each row

          //plot each row up from row minimum (calculated by sequence number) up to 6
          for (byte newcharrow = newcharrowmin; newcharrow <= 6; newcharrow++) {
            for (byte col = 0; col < 5; col++) {
              dots = pgm_read_byte_near(&myfont[new_c][col]);
              if (dots & (64 >> newcharrow))
                plot(x + col, y + start_y, 1); //plot led on
              else
                plot(x + col, y + start_y, 0); //else plot led off
            }
            start_y++;//add one to y so we draw next row one down
          }
        }
      }

      ////////////////////////////////////////////////////////////////////////////////////////


//called by slide
      //this draws the animation of one char sliding on and the other sliding off. There are 8 steps in the animation, we call the function to draw one of the steps from 0-7
      //inputs are are char x and y, animation frame sequence (0-7) and the current and new chars being drawn.
      void Superslideanim(byte x, byte y, byte sequence, char current_c, char new_c) {

        //  To slide one char off and another on we need 9 steps or frames in sequence...

        //  seq# 0123456 <-rows of the display
        //   |   |||||||
        //  seq0 0123456  START - all rows of the display 0-6 show the current characters rows 0-6
        //  seq1  012345  current char moves down one row on the display. We only see it's rows 0-5. There are at display positions 1-6 There is a blank row inserted at the top
        //  seq2 6 01234  current char moves down 2 rows. we now only see rows 0-4 at display rows 2-6 on the display. Row 1 of the display is blank. Row 0 shows row 6 of the new char
        //  seq3 56 0123
        //  seq4 456 012  half old / half new char
        //  seq5 3456 01
        //  seq6 23456 0
        //  seq7 123456
        //  seq8 0123456  END - all rows show the new char

        //from above we can see...
        //currentchar runs 0-6 then 0-5 then 0-4 all the way to 0. starting Y position increases by 1 row each time.
        //new char runs 6 then 5-6 then 4-6 then 3-6. starting Y position increases by 1 row each time.

        //if sequence number is below 7, we need to draw the current char
        if (sequence < 7) {
          byte dots;
          if (current_c >= 'A' && current_c <= 'Z' ) {
            current_c &= 0x1F;   // A-Z maps to 1-26
          }
          else if (current_c >= 'a' && current_c <= 'z') {
            current_c = (current_c - 'a') + 41;   // a-z maps to 41-66
          }
          else if (current_c >= '0' && current_c <= '9') {
            current_c = (current_c - '0') + 31;
          }
          else if (current_c == ' ') {
            current_c = 0; // space
          }
          else if (current_c == '.') {
            current_c = 27; // full stop
          }
          else if (current_c == '\'') {
            current_c = 28; // single quote mark
          }
          else if (current_c == ':') {
            current_c = 29; //colon
          }
          else if (current_c == '>') {
            current_c = 30; // clock_mode selector arrow
          }

          byte curr_char_row_max = 8 - sequence; //the maximum number of rows to draw is 6 - sequence number
          byte start_y = sequence; //y position to start at - is same as sequence number. We inc this each loop

          //plot each row up to row maximum (calculated from sequence number)
          for (byte curr_char_row = 0; curr_char_row <= curr_char_row_max; curr_char_row++) {
            for (byte col = 0; col < 6; col++) {
              dots = pgm_read_byte_near(&myBigFont[current_c][col]);
              if (dots & (128 >> curr_char_row))
                plot(x + col, y + start_y, 1); //plot led on
              else
                plot(x + col, y + start_y, 0); //else plot led off
            }
            start_y++;//add one to y so we draw next row one down
          }
        }

        //draw a blank line between the characters if sequence is between 1 and 7. If we don't do this we get the remnants of the current chars last position left on the display
        if (sequence >= 1 && sequence <= 8) {
          for (byte col = 0; col < 6; col++) {
            plot(x + col, y + (sequence - 1), 0); //the y position to draw the line is equivalent to the sequence number - 1
          }
        }

        //if sequence is above 2, we also need to start drawing the new char
        if (sequence >= 2) {

          //work out char
          byte dots;
          //if (new_c >= 'A' && new_c <= 'Z' || (new_c >= 'a' && new_c <= 'z') ) {
          //  new_c &= 0x1F;   // A-Z maps to 1-26
          //}
          if (new_c >= 'A' && new_c <= 'Z' ) {
            new_c &= 0x1F;   // A-Z maps to 1-26
          }
          else if (new_c >= 'a' && new_c <= 'z') {
            new_c = (new_c - 'a') + 41;   // A-Z maps to 41-67
          }
          else if (new_c >= '0' && new_c <= '9') {
            new_c = (new_c - '0') + 31;
          }
          else if (new_c == ' ') {
            new_c = 0; // space
          }
          else if (new_c == '.') {
            new_c = 27; // full stop
          }
          else if (new_c == '\'') {
            new_c = 28; // single quote mark
          }
          else if (new_c == ':') {
            new_c = 29; // clock_mode selector arrow
          }
          else if (new_c == '>') {
            new_c = 30; // clock_mode selector arrow
          }

          byte newcharrowmin = 6 - (sequence - 2); //minimumm row num to draw for new char - this generates an output of 6 to 0 when fed sequence numbers 2-8. This is the minimum row to draw for the new char
          byte start_y = 0; //y position to start at - is same as sequence number. we inc it each row

          //plot each row up from row minimum (calculated by sequence number) up to 6
          for (byte newcharrow = newcharrowmin; newcharrow <= 8; newcharrow++) {
            for (byte col = 0; col < 6; col++) {
              dots = pgm_read_byte_near(&myBigFont[new_c][col]);
              if (dots & (128 >> newcharrow))
                plot(x + col, y + start_y, 1); //plot led on
              else
                plot(x + col, y + start_y, 0); //else plot led off
            }
            start_y++;//add one to y so we draw next row one down
          }
        }
      }

      ////////////////////////////////////////////////////////////////////////////////////////

    // bottomleds: plot seconds-dots at bottomline
    void bottomleds(byte secs){

          //switch on bottomleds from 1 to 30
          if(secs >=1 && secs <=30){                
            for(int i=0; i<=secs-1; i++){
                 plot(i, 7, 1);
            }
          }
          //switch off bottomleds from 30 to 1   
          if(secs>=31){
            for(int i=0; i<=(30-(secs-30)); i++){
                 plot(i, 7, 1);
            }
             plot(30-(secs-30), 7, 0);
           }


          //switch off bottomled 1      
          if(secs == 0){               
             plot(0, 7, 0);           
          }   
    }



    // setup menu(=mode6): display menu to change the clock settings
    void setup_menu() {

      //char* set_modes[] = { //depecated
      const char *set_modes[] = {
         "Circl", "=24Hr","Set >", "Exit"};   
      if (ampm == 0) { 
        set_modes[1] = ("=12Hr"); 
      }

      byte setting_mode = 0;
      byte next_setting_mode;
      byte firstrun = 1;

      //loop waiting for button (timeout after 35 loops to return to mode X)
      for(int count=0; count < 35 ; count++) {
        //if user hits button, change the clock_mode
        if(buttonA.uniquePress() || firstrun == 1){
          count = 0;
          cls();

          if (firstrun == 0) { 
            setting_mode++; 
          } 
          if (setting_mode > NUM_SETTINGS_MODES) { 
            setting_mode = 0; 
          }

          //print arrown and current clock_mode name on line one and print next clock_mode name on line two
          char str_top[9];
        
          strcpy (str_top, set_modes[setting_mode]);

          next_setting_mode = setting_mode + 1;
          if (next_setting_mode > NUM_SETTINGS_MODES) { 
            next_setting_mode = 0; 
          }
          
          byte i = 0;
          while(str_top[i]) {
            putnormalchar(i*6, 0, str_top[i]); 
            i++;
          }

          firstrun = 0;
        }
        delay(50); 
      }
      
      //pick the mode 
      switch(setting_mode){
        case 0: 
    //      set_circle(); 
          break;
        case 1: 
    //       set_ampm(); 
          break;
        case 2: 
    //      set_time(); 
          break;
          case 3: 
          //exit form menu
          break;
      }
        
      //change the mode from mode 6 (=settings) back to the one it was in before 
      clock_mode=old_mode;
    }

    ////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////

    // toggleDateState: toggle Show date : On/Off
    void toggleDateState(){ 
                if (show_date == true ) {
                    show_date = false;
                    date_state = true;
                        if(debug){
                           Serial.println("Show date = On");
                            }
                    //cls();
                    wipeTop();
                    //display state of date
                    char dateOn[8] = "DATE:ON";
                    int len=7;  // length of dateOn               
                    byte offset_top = (31 - ((len - 1) * 4)) / 2; 
                    byte i = 0;
                    while (dateOn[i]) {
                     puttinychar((i * 4) + offset_top, 1, dateOn[i]);
                     i++;
                    }
                        //hold display but check for button presses
                        int counter = 1000;
                        while (counter > 0){
                              if (buttonA.uniquePress()) { switch_mode();  return; }
                              if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }
                              delay(1);
                              counter--;
                        }
                   wipeBottom();     
                  }
                  
                else{
                  show_date = true;
                  date_state = false;
                      if(debug){
                        Serial.println("Show date = Off");
                        }
                    //cls();
                    wipeTop();
                    //display state of date
                    char dateOff[9] = "DATE:OFF";
                    int len=8;  // length of dateOn               
                    byte offset_top = (31 - ((len - 1) * 4)) / 2; 
                    byte i = 0;
                    while (dateOff[i]) {
                     puttinychar((i * 4) + offset_top, 1, dateOff[i]);
                     i++;
                    }
                        //hold display but check for button presses
                        int counter = 1000;
                        while (counter > 0){
                              if (buttonA.uniquePress()) { switch_mode();  return; }
                              if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }
                              delay(1);
                              counter--;
                        }
                   wipeBottom();               
                }
    }

    ////////////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////

    //display menu to change the clock-mode
    void switch_mode() {

      //remember mode we are in. We use this value if we go into settings mode, so we can change back from settings mode (6) to whatever mode we were in.
      old_mode = clock_mode;

      const char *modes[] = {    
        "Basic", "Small", "Big S", "Sml S", "Words", "Shift", "Setup", "last",
      };

      byte next_clock_mode;
      byte firstrun = 1;

      //loop waiting for button (timeout after 35 loops to return to mode X)
      for (int count = 0; count < 35 ; count++) {

        //if user hits button, change the clock_mode
        if (buttonA.uniquePress() || firstrun == 1) {

          count = 0;
          cls();

          if (firstrun == 0) {
            clock_mode++;
          }
          if (clock_mode > NUM_DISPLAY_MODES + 1 ) {
            clock_mode = 0;
          }

          //print arrown and current clock_mode name on line one and print next clock_mode name on line two
          char str_top[9];

          //strcpy (str_top, "-");
          strcpy (str_top, modes[clock_mode]);

          next_clock_mode = clock_mode + 1;
          if (next_clock_mode >  NUM_DISPLAY_MODES + 1 ) {
            next_clock_mode = 0;
          }

          byte i = 0;
          while (str_top[i]) {
            putnormalchar(i * 6, 0, str_top[i]);
            i++;
          }
          firstrun = 0;
        }
        delay(50);
      }
    }

    ////////////////////////////////////////////////////////////////////////////////////////

      void play_alarm() {

// REMOVE this line, debug only ID:0002 //
int selected_alarm = 8;
///////////////////////////////////////


      switch(selected_alarm){
          case 0: 
          //Abba: Mamma Mia 
          playRingtone(pin,"40 32f2 32#d2 32f2 8#d2 32#d2 32#d2 32f2 32g2 32f2 16.#d2 32- 16f2 8#d2 16#g2 32#g2 32#g2 32#g2 16g2 16.#d2 32- 8#a2 32#a2 32#a2 16#a2 16f2 16g2 8#g2 16g2 16g2 32g2 16g2 16d2 16#d2 8f2 16f2 8#d2 16#g2 32#g2 32#g2 32#g2 32g2 32#d2 32f2 16#d2");
          break;
          
          case 1: 
          //Abba: Money Money Money v1
          playRingtone(pin,"112 8e3 8e3 8e3 8e3 8e3 8e3 16e2 16a2 16c3 16e3 8#d3 8#d3 8#d3 8#d3 8#d3 8#d3 16f2 16a2 16c3 16#d3 4d3 8c3 8a2 8c3 4c3 2a2 32a2 32c3 32e3 8a3"); 
          break;

          case 2: 
          //Abba: Money Money Money v2
          playRingtone(pin,"200 4a1 8b1 4c2 8a1 4b1 4c2 4- 4c2 8a1 4b1 4c2 4- 4b1 8a1 4c2 4.c2 16- 1a1");
          break;
          
          case 3: 
          //Aqua: Lollipop
          playRingtone(pin,"140 8#d1 8c2 8f1 8c2 8f1 8c2 8f1 8#d2 8f1 8#c2 8#a1 8#c2 8#a1 8#c2 8#g1 16#c2 16#d2 16e2 16f2 8c2 8f1 8c2 8f1 8c2 8f1 8#d2 8f1 8#c2 8#a1 8#c2 8#a1 8#c2 8#g1 8#a1 8f1 8c2 8f1 8c2 8f1 8c2 8f1 8#d2 8f1 8#c2 8#a1 8#c2 8#a1 8#c2 8#g1 16#c2");
          break;
      
          case 4: 
         //Alice DJ: Back In My Life
          playRingtone(pin,"140 1c2 4- 8c2 8c2 8e2 8c2 8b1 1c2 4- 8- 8c2 8c2 8e2 8c2 8b1 1c2 8g1 4b1 8b1 4b1 4c2 1c2 2- 4- 8c2 8d2 8c2 16e2 16d2 1c2");
          break;
          
          case 5: 
          //Alice DJ: Better off Alone
          playRingtone(pin,"140 8c2 8- 8c2 8a1 8- 8c2 8- 8c2 8- 8b1 8- 8g1 8g2 16- 8g2 16- 8e2 8c2 8- 8c2 8a1 8- 4c2 8c2 8- 8b1 8- 8g1 8f2 16- 8f2");
          break;
         
          case 6: 
          //Airwolf 
          playRingtone(pin,"100 4e1 16a1 16b1 16d2 4e2 16g2 16#f2 16d2 4e2 16g2 16#f2 16d2 4e2 8d2 16#f2 4b1 4a1 8g1 16a1 8#f1 16d1 4g1 16c2 16d2 16f2 4g2 16c2 16b2 16f2 4g2 16c2 16b2 16f2 4g2 8f2 16a2 4d2 4c2 8b1 16d2 8a1 16f1 4g1 16c2 16d2 16f2 4g2 16c2 16b2 16f2");  
          break;

          case 7: 
          //Aqua: Barbie Girl
          playRingtone(pin,"125 8#g2 8e2 8#g2 8#c3 4a2 4- 8#f2 8#d2 8#f2 8b2 4#g2 8#f2 8e2 4- 8e2 8#c2 4#f2 4#c2 4- 8#f2 8e2 4#g2 4#f2");
          break;
        
          case 8: 
          //Ann Lee: Two Times
          playRingtone(pin,"63 8#a1 16#a1 16#c2 16#a1 16#c2 8#a1 8#a1 16#a1 16b1 16#a1 16b1 8#a1 8#a1 16#a1 16#c2 16#a1 16#c2 8#a1 8#a1 16#a1 16b1 16#a1 16b1 8#a1 8#a1 16#a1 16#c2 16#a1 16#c2 8#a1 8#a1 16#a1 16b1 16#a1 16b1 8#a1 8#a1 16#a1 16#c2 16#a1 16#c2 8#a1 8#a1 16#a1 16b1 16#a1 16b1 8#a1 8#a1 16#a1");
          break;
          
          case 9: 
          //Aha: Take on me 
          playRingtone(pin,"100 8- 16#a1 16#a1 16#a1 8#f1 8#d1 8#g1 8#g1 16#g1 16c2 16c2 16#c2 16#d2 16#c2 16#c2 16#c2 8#g1 8#f1 8#a1 8#a1 16#a1 16#g1 16#g1 16#a1 16#g1 16#a1 16#a1 16#a1 8#f1 8#d1 8#g1 8#g1 16#g1 16c2 16c2 16#c2 16#d2 16#c2 16#c2 16#c2 8#g1 8#f1 8#a1 8#a1");
          break;

          case 10: 
          //Adams Family v2 
          playRingtone(pin,"160 8c2 4f2 8a2 4f2 8c2 4b1 2g2 8f2 4e2 8g2 4e2 8e1 4a1 2f2 8c2 4f2 8a2 4f2 8c2 4b1 2g2 8f2 4e2 8c2 4d2 8e2 1f2 8c2 8d2 8e2 8f2 1- 8d2 8e2 8#f2 8g2 1- 8d2 8e2 8#f2 8g2 4- 8d2 8e2 8#f2 8g2 4- 4c2 8e2 1f2");
          break;

          case 11: 
          //Adams Family v1
          playRingtone(pin,"70 8#f1 16#f1 16#f1 16#f1 8b1 32#d2 8b1 32#g1 8e1 8#c2 32a1 8#a1 32#c2 8#a1 32#f1 8#d1 8b1 32#f1 8b1 32#d2 8b1 32#g1 8e1 8#c2 32b1 8#a1 32#f1 8#g1 32#a1 4b1 32#f1 8b1 32#d2 8b1 32#g1 8e1 8#c2 32a1 8#a1 32#c2 8#a1 32#f1 8#d1 8b1 32#f1 8b1 32#d2 8b1 32#g1 8e1");
          break;
          
          case 12: 
          //Aerosmith: I Don't Wanna Miss A Thing
          playRingtone(pin,"125 2- 16a1 16- 16a1 16- 8a1 16- 4a2 16g2 16- 2g2 16- 4- 8- 16g2 16- 16g2 16- 16g2 8g2 16- 4c2 16#a1 16- 4a2 8g2 4f2 4g2 8d2 8f2 16- 16f2 16- 16c2 8c2 16- 4a2 8g2 16f2 16- 8f2 16- 16c2 16- 4g2 4f2");
          break;
     
          case 13: 
          //ATB: 9PM
          playRingtone(pin,"140 8a1 8a1 8b1 4a1 16c2 8- 16- 16c2 8- 16- 8c2 8e2 8d2 4f1 4- 8g1 8g1 8a1 4g1 16a1 8- 16- 8c2 8- 8c2 8d2 8c2 4g1");
          break;
          
          case 14: 
          //A-Team
          playRingtone(pin,"100 1d2 4d2 4e2 2#f2 1g2 4a2 4a1 4d2 4#c2 8#f2 8#f2 8e2 4#f2 4e2 4#f2 4e2 8e2 4b2 4a2 8#f2 8#f2 8e2 4#f2 4e2 4d2 4d2 8e2 2e2 8#f2 8#f2 8e2 4#f2 4e2 4#f2 4e2 8e2 4b2 4a2 8#f2 8#f2 8e2 4#f2 4e2 4d2 4d2 8e2 2e2");
          break;
          
          case 15: 
          //Auld Lang Synez
          playRingtone(pin,"100 4g1 4.c2 8c2 4c2 4e2 4.d2 8c2 4d2 8e2 8d2 4.c2 8c2 4e2 4g2 2.a2 4a2 4.g2 8e2 4e2 4c2 4.d2 8c2 4d2 8e2 8d2 4.c2 8a1 4a1 4g1 2c2");
          break;
          
          case 16:
          //Austin Powers
          playRingtone(pin,"100 4b1 8#c2 4#c2 4e2 8#c2 8#f2 4e2 4#c2 4e2 8g2 8e2 8a2 8g2 8e2 4e2 8e2 8e2 8b1 4d2 8e2 8b1 4d2 8e2 8b1 4d2 8e2 4d2 8e2 8e2 8d2 8e2 8g2 8a2 8g2 4b2 4b2 8d2 4b2 8d2 8e2 8e2 8e2 8d2 8b2 8a2 8g2 8d2 8e2 8b1 8d2");
          break;

          case 17:
         //Avril Lavigne: Sk8er Boy
          playRingtone(pin,"160 8a1 8a1 8a1 8#a1 8a1 4f1 8a1 8a1 8a1 8a1 8#a1 8a1 4f1 8f1 8f1 8f1 4e1 4e1 4g1 4e1 4.f1 8- 8a1 8a1 8a1 8#a1 8a1 4f1 8a1 8a1 8a1 8a1 8#a1 8a1 4f1 8f1 8f1 8f1 4e1 4e1 4g1 4e1 4.f1");
          break;

          case 18:
          //Axel F
          playRingtone(pin,"100 4g2 8.#a2 16g2 16- 16g2 8c3 8g2 8f2 4g2 8.d3 16g2 16- 16g2 8#d3 8d3 8#a2 8g2 8d3 8g3 16g2 16f2 16- 16f2 8d2 8a2 2g2");
          break;
      }
  }

      // get_time: get the current time from the RTC
      void get_time()
      {
        //get time
        DateTime now = ds1307.now();
        //save time to array
        rtc[6] = now.year();
        rtc[5] = now.month();
        rtc[4] = now.day();
      //  rtc[3] = now.dayOfWeek(); //returns 0-6 where 0 = Sunday
        //rtc[2] = now.hour();  depends on boolean summertime_EU
        rtc[1] = now.minute();
        rtc[0] = now.second();

       // Calculate if summer- or wintertime
      //  if(summertime_EU(now.year(), now.month(), now.day(), now.hour(), 1)){
      //    if(debug){Serial.print("Sommerzeit =  true"); }
      //    rtc[2] = now.hour() +1;   // wintertime: add 1 hour
      //      if(rtc[2] > 23){        // if hour > 23, we have next day between 00:00:00 and 01:00:00
      //        rtc[2] = 0;           // hour = 0
      //        rtc[3] = rtc[3] +1;   // dayOfWeek +1 
      //        rtc[4] = rtc[4] +1;   // day +1 
      //      }
      //  }
      //  else{
          //  if(debug){Serial.print("Sommerzeit = false"); }
            rtc[2] = now.hour();    // summertime = "normal" hour
      //  }


       // Calculate day of year and week of year 
      // DayWeekNumber(rtc[6],rtc[5],rtc[4],rtc[3]);


     /*   if(debug){
        //print the time to the serial port - for debuging
        Serial.print("     ");
        Serial.print(rtc[2]);
        Serial.print(":");
        Serial.print(rtc[1]);
        Serial.print(":");
        Serial.print(rtc[0]);

        Serial.print("   ");
        Serial.print(rtc[4]);
        Serial.print(".");
        Serial.print(rtc[5]);
        Serial.print(".");
        Serial.print(rtc[6]);

        Serial.print("    Wochentag: ");
        Serial.print(rtc[3]);    

        Serial.print("      Tag ");
        Serial.print(DN);
        Serial.print("  in Woche ");
        Serial.print(WN);
        Serial.print(" in ");
        Serial.print(rtc[6]);

        Serial.print("  clock_mode: ");
        Serial.println(clock_mode);
        }
        */
      }
