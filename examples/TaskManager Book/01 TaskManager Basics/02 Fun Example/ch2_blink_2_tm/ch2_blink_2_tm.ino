
//
// Blink two LEDs at different rates
// Respond to a switch on the third LED
//

#define size_t int
#include <Streaming.h>
#include <TaskManager.h>

// first LED
int led_1 = 2;
bool led_1_state;

// second LED
int led_2 = 3;
bool led_2_state;

// third LED and its switch
int led_3 = 4;
int switchPort = 5;
bool led_3_state;
bool switchIsPressed;

// declare our tasks
void loop_led_1();
void loop_led_2();
void loop_led_3();

void setup() {
  // set up first LED
  pinMode(led_1, OUTPUT);
  digitalWrite(led_1, LOW);
  led_1_state = LOW;

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
  
  // add our tasks
  TaskMgr.add(1, loop_led_1);
  TaskMgr.add(2, loop_led_2);
  TaskMgr.add(3, loop_led_3);
}

void loop_led_1() {
  // just invert the LED
  led_1_state = (led_1_state==LOW) ? HIGH : LOW;
  digitalWrite(led_1, led_1_state);
  TaskMgr.yieldDelay(500);  
}

void loop_led_2() {
  // just invert the LED
  led_2_state = (led_2_state==LOW) ? HIGH : LOW;
  digitalWrite(led_2, led_2_state);
  TaskMgr.yieldDelay(100);  
}

void loop_led_3() {
  // if the switch has been pressed, invert the LED and debounce
  // if it has been released, just debounce
  if(digitalRead(switchPort)==LOW/*==pressed/closed*/) {
    if(!switchIsPressed) {
      // someone pressed the switch
      led_3_state = (led_3_state==LOW) ? HIGH : LOW;
      digitalWrite(led_3, led_3_state);
      switchIsPressed = true;
      TaskMgr.yieldDelay(50);  // debounce
    }
  } else if(switchIsPressed /* we know switchPort==HIGH/open */) {
    switchIsPressed = false;
    TaskMgr.yieldDelay(50);  // debounce
  }
}
