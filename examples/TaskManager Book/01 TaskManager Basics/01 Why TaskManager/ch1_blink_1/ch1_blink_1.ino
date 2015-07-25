//
// Standard "blink" program

int led_1=2;
bool led_1_state;

void setup() {
  pinMode(led_1, OUTPUT);
  led_1_state = LOW;
  digitalWrite(led_1, led_1_state);
}

void loop() {
  led_1_state = (led_1_state==LOW) ? HIGH : LOW;
  digitalWrite(led_1, led_1_state);
  delay(500);
}

