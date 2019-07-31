// Example 01.01
// Simple blink
// One LED blinking
//
// This is similar to blinky; just modified to make the transition to simple TaskManager programs
// clearer.

#define LED_RED 3

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, LOW);
}

// the loop function runs over and over again forever
// LEDTIME is how long the LED stays on (or off).
#define LEDDTIME 1000
void loop() {
  static bool isLow = true;
  digitalWrite(LED_RED, isLow ? HIGH : LOW);   // turn the LED on or off, depending on previous state
  isLow = !isLow;                    // set the new state
  delay(LEDTIME);                    // wait for a bit
}
