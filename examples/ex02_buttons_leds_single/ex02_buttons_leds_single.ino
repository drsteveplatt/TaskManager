// Example 01_03
// More complex blinking
// Two LEDs blinking at different rates
// Written with TaskManager

#include <TaskManager.h>

#if defined(ARDUINO_ARCH_AVR)
#define LED1    3
#define LED2    5
#define SW1     9
#define SW2     10
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#define LED1   19
#define LED2   21
#endif

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
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
