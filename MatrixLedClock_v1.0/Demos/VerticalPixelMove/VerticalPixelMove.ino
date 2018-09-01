// Inladen van de ledcontrol bibliotheek.
#include "LedControl.h"
 
LedControl lc=LedControl(12, 11, 10, 1); // DIN, CLK, CS, NRDEV
 
//Een variabele voor het wachten voordat we het display updaten.
unsigned long delaytime = 50;
 
void setup() {
    // Haal het aantal apparaten op dat we hebben "gecreëerd" met Ledcontrol.
    int devices=lc.getDeviceCount();
    // Alle apparaten initialiseren (in een loop).
    for(int address=0;address<devices;address++) {
        // De MAX72XX IC is in slaapstand modus bij opstarten.
        lc.shutdown(address,false);
        // Zet de helderheid op een medium niveau.
        lc.setIntensity(address,8);
        // Maak de dot matrix leeg (clear display).
        lc.clearDisplay(address);
    }
}
 
void loop() { 
    // Lees het aantal apparaten uit.
    int devices=lc.getDeviceCount();
  
    // Laat de ledjes stuk voor stuk branden.
    for(int row=0;row<8;row++) {
        for(int col=0;col<8;col++) {
            for(int address=0;address<devices;address++) {
                lc.setLed(address,row,col,true); delay(delaytime);
            }
        }
    }
    // Zet de ledjes stuk voor stuk uit.
    for(int row=0;row<8;row++) {
        for(int col=0;col<8;col++) {
            for(int address=0;address<devices;address++) {
                lc.setLed(address,row,col,false); delay(delaytime);
            }
        }
    }
}

