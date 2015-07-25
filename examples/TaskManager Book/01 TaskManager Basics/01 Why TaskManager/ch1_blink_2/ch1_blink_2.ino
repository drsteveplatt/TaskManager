//
// Blink two LEDs at different rates
// Respond to a switch on the third LED
//

// first LED
int led_1 = 2;
long int last_1_time;
bool led_1_state;

// second LED
int led_2 = 3;
long int last_2_time;
bool led_2_state;

// third LED and its switch
int led_3 = 4;
int switchPort = 5;
bool led_3_state;
bool switchIsPressed;


void setup() {
  // set up first LED
  pinMode(led_1, OUTPUT);
  digitalWrite(led_1, LOW);
  led_1_state = LOW;
  last_1_time = millis();
  
  // set up second LED
  pinMode(led_2, OUTPUT);
  digitalWrite(led_2, LOW);
  led_2_state = LOW;

  // set up switch and third LED
  pinMode(led_3, OUTPUT);
  pinMode(switchPort, INPUT_PULLUP);  
  digitalWrite(led_3, LOW);
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
    digitalWrite(led_1, led_1_state);
    last_1_time = millis();
  }
  
  // see if enough time has passed to flip the second LED
  if(now-last_2_time > 100) {
    led_2_state = (led_2_state==LOW) ? HIGH : LOW;
    digitalWrite(led_2, led_2_state);
    last_2_time = millis();
  }

  // if the switch has been pressed, invert the LED and debounce
  // if it has been released, just debounce
  if(digitalRead(switchPort)==LOW/*==pressed/closed*/) {
    if(!switchIsPressed) {
      // someone pressed the switch
      led_3_state = (led_3_state==LOW) ? HIGH : LOW;
      digitalWrite(led_3, led_3_state);
      switchIsPressed = true;  // debounce
      delay(50);
    }
  } else if(switchIsPressed /* we know switchPort==HIGH/open */) {
    switchIsPressed = false;
    delay(50);  // debounce
  }
}

