#include <Arduino.h>
#include "../../TaskManager.h"

/*
  Turns on an LED on for one second, then off for one second, repeatedly.
*/

void delayLoop();

long startMs;

void setup()
{
    Serial.begin(9600);
	startMs = millis();
	TaskMgr.addAutoWaitDelay(1, delayLoop, 1000, false);
	Serial.println("Start");
}

void delayLoop()
{
	// print now and then amount of work time
	// Since we run every 1 seconds, we "work" 300 to 1500ms
	int work = 100*random(3, 15);
	Serial.print("now: ");
	Serial.print(millis()-startMs);
	Serial.print(" work: ");
	Serial.println(work);
	delay(work);
}
