#include <Arduino.h>

#include <TaskManager/TaskManager.h>

/*
  Turns on an LED on for one second, then off for one second, repeatedly.
*/

void task1(), task2(), task3();
void setup()
{
	Serial.begin(9600);
    TaskMgr.add(1, task1);
    TaskMgr.add(2, task2);
    TaskMgr.add(3, task3);
}

unsigned long int t0, t1;   // used to track task swap times

void task1()
{
    Serial.println("task 1");
    t0 = micros();
}
void task2()
{
    t1 = micros();
    Serial.print(t1-t0);
    Serial.println(" task 2");
}
void task3()
{
    Serial.println("task 3");
}
