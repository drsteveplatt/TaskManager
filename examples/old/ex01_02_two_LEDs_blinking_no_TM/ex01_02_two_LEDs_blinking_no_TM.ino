// Example 01_01
// More complex blinking
// Two LEDs blinking at different rates
// Written without TaskManager

#define LED_1   3
#define LED_2   9

void setup() {
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
}

// LEDxTIME is how long the LED stays on (or off)
#define LED1TIME 1000
#define LED2TIME 700
void loop() { 
  static bool led1LOW = true;
  static bool led2LOW = true;
  static long led1LastTime = millis();
  static long led2LastTime = millis();
  // See if LED1 needs to be flipped, and if so, do it
  if(millis()>led1LastTime+LED1TIME) {
    digitalWrite(LED1, LED1Low ? HIGH : LOW);   // turn the LED on or off, depending on previous state
    LED1Low = !LED1Low;                    // set the new state
    led1LastTime = millis();
  }
  // see if LED2 needs to be flipped, and if so, do it
  if(millis()>led2LastTime+LED2TIME) {
    digitalWrite(LED2, LED2Low ? HIGH : LOW);   // turn the LED on or off, depending on previous state
    LED2Low = !LED2Low;                    // set the new state
    led2LastTime = millis();
  }
}
