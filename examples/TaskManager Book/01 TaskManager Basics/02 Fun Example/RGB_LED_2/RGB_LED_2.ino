/*
  Manipulate an RGB LED on pins 9(R), 10(G), 11(B) using PWM.
  Ramps each color up and down at different speeds.
  Uses the TaskManager library to manage independent tasks for each pin.
  Also manipulates an RGB LED on pins 3(R), 5 (G), 6(B) using PWM.
  Ramps H:[0 255] S=255 V=255, converting to RGB.  A hue circle, essentially

  SM Platt, 2014-10-20

*/

#include <SPI.h>
#include <RF24.h>
#include <TaskManager.h>

#include <rgb_hsv.h>

#include "Streaming.h"

/* 10/21/14 Added code for second RGB LED that just traverses the H range */

// LEDs and pins
#define LED1R 9
#define LED1G 10
#define LED1B 11
#define LED2R 3
#define LED2G 5
#define LED2B 6

// KEEP RGB global so the reporter can access the values
int rLevel=1;
int gLevel=1;
int bLevel=1;

int hLevel=0;

// Task to manipulate the HSV cone on pins 3, 5, 6
void hsv() {
    // Note that S and V will always be 255 since we are going on the saturated outer
    // rim of the HSV cone
    byte r, g, b;
    byte h, s, v;
    hsv_to_rgb(hLevel, 255, 255, r, g, b);
    analogWrite(LED2R, r);
    analogWrite(LED2G, g);
    analogWrite(LED2B, b);
    hLevel = hLevel==255 ? 0 : hLevel+1;

//    if(hLevel>166)
//        Serial << "h: " << hLevel << " -> rgb: " << r << ' ' << g << ' ' << b << '\n';
}
// Task to report the current RGB levels
void reporter() {
    Serial << "rgb is [" << rLevel << ' ' << gLevel << ' ' << bLevel
        << "] hsl is [" << hLevel << " 255 255]\n";
}

// Common update procedure for each of the R, G, B tasks
// Note that this procedure takes around 512 cycles to complete.
void updater(unsigned int pin, int& val) {
    analogWrite(pin, val<256 ? val : 511-val);
    val = (val + 1);
    if(val>511) val=1;
}

// The R, G, B tasks.
void red() {
    updater(LED1R, rLevel);
}
void green() {
    updater(LED1G, gLevel);
}
void blue() {
    updater(LED1B, bLevel);
}

// Setup.
// Initialize the serial line.
// Add the tasks at their set rates
void setup()
{
  Serial.begin(9600);
  Serial << "Hello World xxx\n";

  pinMode(3,OUTPUT);
  pinMode(5,OUTPUT);
  pinMode(6,OUTPUT);
  pinMode(9,OUTPUT);
  pinMode(10,OUTPUT);
  pinMode(11,OUTPUT);

    // Red runs every 3 seconds.  Green every 5 seconds.  Blue every 7 seconds
    // There are 52 steps per the update procedure.
  TaskMgr.addAutoWaitDelay(1, red, (3*1000)/512);
  TaskMgr.addAutoWaitDelay(2, green, (5*1000)/512);
  TaskMgr.addAutoWaitDelay(3, blue, (7*1000)/512);
  TaskMgr.addAutoWaitDelay(4, hsv, (20*1000)/256); // 5 second loop
  //TaskMgr.addAutoWaitDelay(5, reporter, 1000);
}



