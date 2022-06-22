//
// Standard "blink" program (slightly rewritten)

#define LED_1_PORT  2
bool led_1_state;

void setup() {
  pinMode(LED_1_PORT, OUTPUT);
  led_1_state = LOW;
  digitalWrite(LED_1_PORT, led_1_state);
}

void loop() {
  led_1_state = (led_1_state==LOW) ? HIGH : LOW;
  digitalWrite(LED_1_PORT, led_1_state);
  delay(500);
}

