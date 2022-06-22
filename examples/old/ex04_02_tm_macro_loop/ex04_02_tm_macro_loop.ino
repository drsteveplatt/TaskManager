// Example program demonstrating the use of TM macros and loops
// Blinks an LED once (1000ms), then four times (100ms)
#include <TaskManager.h>
#define LED 13

void setup() {
  // put your setup code here, to run once:
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  TaskMgr.add(1, ledTask);
}

void ledTask() {
  static int i;
  TM_BEGIN();
  digitalWrite(LED, HIGH);
  TM_YIELDDELAY(1, 1000);
  digitalWrite(LED, LOW);
  TM_YIELDDELAY(2, 1000);

  for(i=0; i<5; i++) {
    digitalWrite(LED, HIGH);
    TM_YIELDDELAY(3, 100);
    digitalWrite(LED, LOW);
    TM_YIELDDELAY(4, 100);
  }
  TM_END();
}
