// Example 01_01
// More complex blinking
// Two LEDs blinking at different rates
// Written without TaskManager

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
}

// LEDxTIME is how long the LED stays on (or off)
#define LED1TIME 1000
#define LED2TIME 150
void loop() { 
  static bool led1Low = true;
  static bool led2Low = true;
  static long led1LastTime = millis();
  static long led2LastTime = millis();
  // See if LED1 needs to be flipped, and if so, do it
  if(millis()>led1LastTime+LED1TIME) {
    digitalWrite(LED1, led1Low ? HIGH : LOW);   // turn the LED on or off, depending on previous state
    led1Low = !led1Low;                    // set the new state
    led1LastTime = millis();
  }
  // see if LED2 needs to be flipped, and if so, do it
  if(millis()>led2LastTime+LED2TIME) {
    digitalWrite(LED2, led2Low ? HIGH : LOW);   // turn the LED on or off, depending on previous state
    led2Low = !led2Low;                    // set the new state
    led2LastTime = millis();
  }
}
