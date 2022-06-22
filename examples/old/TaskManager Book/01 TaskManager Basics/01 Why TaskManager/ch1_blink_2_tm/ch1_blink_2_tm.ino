//
// Blink two LEDs at different rates
// Respond to a switch on the third LED
//

#include <SPI.h>
#include <RF24.h>
#include <TaskManager.h>

#define LED_1_PORT  2
bool led_1_state;

#define LED_2_PORT  3
bool led_2_state;

#define LED_3_PORT   4
#define SWITCH_PORT  5
bool led_3_state;
bool switchIsPressed;

void setup() {
  pinMode(LED_1_PORT, OUTPUT);
  digitalWrite(LED_1_PORT, LOW);
  led_1_state = LOW;
  
  pinMode(LED_2_PORT, OUTPUT);
  digitalWrite(LED_2_PORT, LOW);
  led_2_state = LOW;

  pinMode(LED_3_PORT, OUTPUT);
  pinMode(SWITCH_PORT, INPUT_PULLUP);  
  digitalWrite(LED_3_PORT, LOW);
  led_3_state = LOW;
  switchIsPressed = false;
  
  TaskMgr.add(1, loop_led_1);
  TaskMgr.add(2, loop_led_2);
  TaskMgr.add(3, loop_led_3);
}

void loop_led_1() {
    led_1_state = (led_1_state==LOW) ? HIGH : LOW;
    digitalWrite(LED_1_PORT, led_1_state);
    TaskMgr.yieldDelay(500);  
}

void loop_led_2() {
    led_2_state = (led_2_state==LOW) ? HIGH : LOW;
    digitalWrite(LED_2_PORT, led_2_state);
    TaskMgr.yieldDelay(100);  
}

void loop_led_3() {
    if(digitalRead(SWITCH_PORT)==LOW/*==pressed/closed*/) {
    if(!switchIsPressed) {
      // someone pressed the switch
      led_3_state = (led_3_state==LOW) ? HIGH : LOW;
      digitalWrite(LED_3_PORT, led_3_state);
      switchIsPressed = true;
      TaskMgr.yieldDelay(50);  // debounce
    }
  } else if(switchIsPressed /* we know switchPort==HIGH/open */) {
    switchIsPressed = false;
    TaskMgr.yieldDelay(50); // debounce
  }
}

