//
// Blink two LEDs at different rates
// Respond to a switch on the third LED
//

// first LED
#define LED_1_PORT  2
long int last_1_time;
bool led_1_state;

// second LED
#define LED_2_PORT  3
long int last_2_time;
bool led_2_state;

// third LED and its switch
#define LED_3_PORT   4
#define SWITCH_PORT  5
bool led_3_state;
bool switchIsPressed;


void setup() {
  // set up first LED
  pinMode(LED_1_PORT, OUTPUT);
  digitalWrite(LED_1_PORT, LOW);
  led_1_state = LOW;
  last_1_time = millis();
  
  // set up second LED
  pinMode(LED_2_PORT, OUTPUT);
  digitalWrite(LED_2_PORT, LOW);
  led_2_state = LOW;

  // set up switch and third LED
  pinMode(LED_3_PORT, OUTPUT);
  pinMode(SWITCH_PORT, INPUT_PULLUP);  
  digitalWrite(LED_3_PORT, LOW);
  led_3_state = LOW;
  switchIsPressed = false;
  
  // log the start times of the first two LEDs
  last_1_time = millis();
  last_2_time = last_1_time;
}

void loop() {
  // get the current time
  long int now = millis();
  
  // see if enough time has passed to flip the first LED
  if(now-last_1_time > 500) {
    led_1_state = (led_1_state==LOW) ? HIGH : LOW;
    digitalWrite(LED_1_PORT, led_1_state);
    last_1_time = millis();
  }
  
  // see if enough time has passed to flip the second LED
  if(now-last_2_time > 100) {
    led_2_state = (led_2_state==LOW) ? HIGH : LOW;
    digitalWrite(LED_2_PORT, led_2_state);
    last_2_time = millis();
  }

  // if the switch has been pressed, invert the LED and debounce
  // if it has been released, just debounce
  if(digitalRead(SWITCH_PORT)==LOW/*==pressed/closed*/) {
    if(!switchIsPressed) {
      // someone pressed the switch
      led_3_state = (led_3_state==LOW) ? HIGH : LOW;
      digitalWrite(LED_3_PORT, led_3_state);
      switchIsPressed = true;  // debounce
      delay(50);
    }
  } else if(switchIsPressed /* we know switchPort==HIGH/open */) {
    switchIsPressed = false;
    delay(50);  // debounce
  }
}

