// Example program demonstrating TM macros
// Blinks an LED once (1000ms)
#include <TaskManager.h>
#define LED 13

void setup() {
  // put your setup code here, to run once:
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  TaskMgr.add(1, ledTask);
}

void ledTask() {
  TM_BEGIN();
  digitalWrite(LED, HIGH);
  TM_YIELDDELAY(1, 1000);
  digitalWrite(LED, LOW);
  TM_YIELDDELAY(2, 1000);
  TM_END();
}
