#include "LedControl.h"
int devices = 3; //moduliu skaicius
LedControl lc = LedControl(12, 11, 10, 3); // DIN, CLK, CS, NRDEV
 
unsigned long delaytime = 50;
int row;
int col;
int address;
 
void setup (){
    for (int address = 0; address < devices; address++){
        lc.shutdown (address, false);
        lc.setIntensity (address, 12);
        lc.clearDisplay (address);}
    }
 
void clr_disp (int dev){
    delay (1000);
    for (address = 0; address < dev; address++){
        lc.clearDisplay (address);}
    }
 
void fill_pixel (int dev){
    for (row = 0; row < 8; row++){
        for (address = 0; address < dev; address++){
            for (col = 0; col < 8; col++){
                delay (delaytime);
                lc.setLed (address, row, col, true);}
            }
        }
    }
 
void fill_col_right (int dev, int b){
    for (address = 0; address < dev; address++){
        for (col = 0; col < 8; col++){
            delay (delaytime);
            lc.setColumn (address, col, b);}
        }
    }
 
void fill_col_left (int dev, int b){
    for (address = dev-1; address >= 0; address--){
        for (col = 7; col >= 0; col--){
            delay (delaytime);
            lc.setColumn (address, col, b);}
        }
    }
 
void fill_row_down (int dev, int b){
    for (row = 0; row < 8; row++){
        for (address = 0; address < dev; address++){
            lc.setRow (address, row, b);}
            delay (delaytime);
        }
    }
 
void fill_row_up (int dev, int b){
    for (row = 7; row >= 0; row--){
        for( address = 0; address < dev; address++){
            lc.setRow (address, row, b);}
            delay (delaytime);
        }
    }
 
void loop (){
    clr_disp (devices);
    fill_col_right (devices, 255);
    fill_row_down (devices, 0);
    fill_row_up (devices, 255);
    fill_col_left (devices, 0);
    }
