// Example 01_03
// More complex blinking
// Two LEDs blinking at different rates
// Written with TaskManager

#include <TaskManager.h>

#define LED1  3
#define LED2  4

void setup() {
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
  TaskMgr.add(1, led1Loop);   // Tell TaskManager about the two loop-like procedure tasks
  TaskMgr.add(2, led2Loop);
}
//
// note what is missing:  void loop() {}
//

// Task for the first LED
#define LED1TIME 1000
void led1Loop() {
  static bool isLow = true;
  digitalWrite(LED1, isLow ? HIGH : LOW);
  isLow = !isLow;
  TaskMgr.yieldDelay(LED1TIME);
}

// Task for the second LED
#define LED2TIME 700
void led2Loop() {
  static bool isLow = true;
  digitalWrite(LED2, isLow ? HIGH : LOW);
  isLow = !isLow;
  TaskMgr.yieldDelay(LED2TIME);
}
