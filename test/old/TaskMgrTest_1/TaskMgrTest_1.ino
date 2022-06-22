#include <Arduino.h>

#include <TaskManager/Streaming.h>
#include <TaskManager/TaskManager.h>

/*
  TaskManager test 1
  Should output alternating 'a' and 'b'
*/

void task1() {
    Serial << "a";
}
void task2() {
    Serial << "b";
}

void setup()
{
	Serial.begin(9600);
	TaskMgr.add(1, task1);
	TaskMgr.add(2, task2);
}



