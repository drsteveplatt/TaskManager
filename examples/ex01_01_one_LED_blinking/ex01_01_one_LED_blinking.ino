// Example 01.01
// Simple blink
// One LED blinking
//
// This is similar to blinky; just modified to make the transition to simple TaskManager programs
// clearer.

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
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED1, OUTPUT);
  digitalWrite(LED1, LOW);
}

// the loop function runs over and over again forever
// LEDTIME is how long the LED stays on (or off).
#define LEDTIME 1000
void loop() {
  static bool isLow = true;
  digitalWrite(LED1, isLow ? HIGH : LOW);   // turn the LED on or off, depending on previous state
  isLow = !isLow;                    // set the new state
  delay(LEDTIME);                    // wait for a bit
}
