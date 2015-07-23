#include <Arduino.h>

#include <TaskManager/TaskManager.h>

/*
  Turns on an LED on for one second, then off for one second, repeatedly.
*/

void loopx();

void setup()
{
	Serial.begin(9600);

	// initialize the digital pin as an output.
	// Pin 13 has an LED connected on most Arduino boards:
	pinMode(13, OUTPUT);
	TaskMgr.add(1, loopx);
}

void loopx()
{
	Serial.println("Hello world!");

	delay(1000);              // wait for a second
	digitalWrite(13, HIGH);   // set the LED on
	delay(1000);              // wait for a second
	digitalWrite(13, LOW);    // set the LED off
}
