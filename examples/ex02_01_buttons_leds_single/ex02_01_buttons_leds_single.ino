// Example 02_01
// Buttons and LEDs, single node
// Buttons controlling LEDs
//
// SW1 sends an empty message to LED1; LED1 inverts at each message.
// SW1 is continuously polled
// SW2 sends a message to LED2 with a level (0..255).  THe level 
// increases by 5 as long as SW2 is held down and decreases by 
// 5 as long as SW2 is open.  SW2 is polled every 50ms.  The vals 
// are constrained to [0 255].

#include <TaskManager_2.h>
#if defined(ARDUINO_ARCH_AVR)
#define LED1    3
#define LED2    5
#define SW1     9
#define SW2     10
#define HEARTBEATLED 6
#define BOARDSEL 4
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#define LED1   22
#define LED2   23
#define LED2PWMCHANNEL 0
#define SW1    5
#define SW2    18
#define HEARTBEATLED  32
#define BOARDSEL      33
#endif


// Task IDs
#define LED1TASK 1
#define LED2TASK 2
#define HEARTBEATTASK 3
#define SW1TASK  4
#define SW2TASK  5

// MS between SW2 polls
#define SW2POLL 50

void setup() {
  pinMode(LED1, OUTPUT);
#if defined(ARDUINO_ARCH_AVR)
  pinMode(LED2, OUTPUT);
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  ledcAttachPin(LED2, LED2PWMCHANNEL);
  ledcSetup(LED2PWMCHANNEL, 4000, 8);
#else
#endif
  pinMode(HEARTBEATLED, OUTPUT);
  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(HEARTBEATLED, LOW);
  TaskMgr.addAutoWaitMessage(LED1TASK, led1Task);   // Tell TaskManager about the two loop-like procedure tasks
  TaskMgr.addAutoWaitMessage(LED2TASK, led2Task);
  TaskMgr.addAutoWaitDelay(HEARTBEATTASK, heartbeatTask, 700);
  TaskMgr.add(SW1TASK, sw1Task);
  TaskMgr.addAutoWaitDelay(SW2TASK, sw2Task, SW2POLL);
}

struct MessageInfo {
  int16_t ledLevel;
  MessageInfo(): ledLevel(0) {}
};

// Task for the first LED
// It auto-waits for a message. It ignores the message, just flipping the LED each time.
void led1Task() {
  static bool isOn = false;
  digitalWrite(LED1, isOn ? LOW : HIGH);
  isOn = !isOn;
}

// Task for the second LED
// It auto-waits for a message, then gets the passed value and sets the LED to that value.
void led2Task() {
  MessageInfo theMessage;
  memcpy(&theMessage, TaskMgr.getMessage(), sizeof(MessageInfo));
#if defined(ARDUINO_ARCH_AVR)
  analogWrite(LED2, theMessage.ledLevel);
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  ledcWrite(LED2PWMCHANNEL, theMessage.ledLevel);
#else
#endif
}

// Heartbeat task
void heartbeatTask() {
  static bool isLow = true;
  digitalWrite(HEARTBEATLED, isLow ? HIGH : LOW);
  isLow = !isLow;
}

// Task for first switch.
// Just send an empty message to the LED1 task
void sw1Task() {
  static bool button_pressed = false;
  if(digitalRead(SW1)==LOW && !button_pressed) {
      // went from unpressed to pressed
      TaskMgr.sendMessage(LED1TASK, NULL, 0);
      button_pressed = true;
      TaskMgr.yieldDelay(50); // debounce
  } else if(digitalRead(SW1)==HIGH && button_pressed) {
      // went from pressed to unpressed
      button_pressed = false;
      TaskMgr.yieldDelay(50); // debounce
  }   // else no change so don't do anything
}

// Task for second switch.
// Send a message with an increasing/decreasing value
// We don't worry about whether this is an on-press or on-release
// We are just worred about the state of the switch.
// Hence, no bool button_pressed logic.
void sw2Task() {
  static struct MessageInfo theMessage;
  if(digitalRead(SW2)==LOW) {
    // went from unpressed to pressed
    theMessage.ledLevel += 5;
  } else if(digitalRead(SW2)==HIGH) {
    // went from pressed to unpressed
    theMessage.ledLevel -= 5;
  }   // else no change so don't do anything
  // Send the message
  if(theMessage.ledLevel>255) theMessage.ledLevel=255;
  else if(theMessage.ledLevel<0) theMessage.ledLevel=0;
  TaskMgr.sendMessage(LED2TASK, &theMessage, sizeof(MessageInfo));
}
