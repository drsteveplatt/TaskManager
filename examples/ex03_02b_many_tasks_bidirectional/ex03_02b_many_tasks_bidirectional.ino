// TaskManager 2.0 test 3 subtest 2
// Many tasks and bidirectional traffic
// 1. Two nodes with identical LED and switch organization.
// 2. One node has a BOARDSEL pin grounded.  This is used to differentiate boards
//    for the purpose of addressing
// 3. Heartbeat task.
// 4. Switch 1 task, send an empty message when pressed.
// 5. Switch 2 task, while the switch is down it sends increasing values to the other node. While
//    the switch is up it sends decreasing values.  The values are in the range [0 255]
// 6. LED 1 task.  Receive a message.  Change the LED from high to low, or low to high.
// 7. LED 2 task.  Receive values and set the PWM to the values.
// 8. Task that sends a periodic message to a local receiver.
// 9. Task that receives a periodic message from a local sender.
//
// This corresponds to ex02_01 except the LEDs are on the other node.
// This demonstrates bidirectional messaging, empty and data messages, local and nonlocal messages, 
// and different kinds of scheduling.
//
// During basic testing, be sure to set the ESP/AVR _RF flag appropriately

#if defined(ARDUINO_ARCH_AVR)
#include <RF24.h>
#include <TaskManager_2.h>
#else
#include <TaskManager_2.h>
#endif

#include <Streaming.h>

#if defined(ARDUINO_ARCH_AVR)
#define LED1    3
#define LED2    5
#define SW1     7
#define SW2     8
#define CE_PIN   9
#define CSN_PIN 10
// Note:  D14 is also A0 on Nano, Uno.
#define HEARTBEATLED 14
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
#define HEARTBEATTASK 1
#define SW1TASK 2
#define SW2TASK 3
#define LED1TASK 4
#define LED2TASK 5
#define LOCALSENDERTASK 6
#define LOCALRECEIVERTASK 7

// ms between SW2 polls
#define SW2POLL 50

// Node IDs
int thisNode, otherNode;

void setup() {
  pinMode(LED1, OUTPUT);
#if defined(ARDUINO_ARCH_AVR)
  pinMode(LED2, OUTPUT);
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  ledcAttachPin(LED2, LED2PWMCHANNEL);
  ledcSetup(LED2PWMCHANNEL, 4000, 8);
#else
#endif

  pinMode(BOARDSEL, INPUT_PULLUP);
  // set the addressing
  if(digitalRead(BOARDSEL)==HIGH) {
    thisNode = 1;
    otherNode = 2;
  } else {
    thisNode = 2;
    otherNode = 1;
  }
  Serial.begin(115200);
  Serial << "My nodeId is " << thisNode << " and the other node is " << otherNode << "\n";
  
#if defined(ARDUINO_ARCH_AVR)
  TaskMgr.radioBegin(thisNode, CE_PIN, CSN_PIN);
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  TaskMgr.radioBegin(thisNode);
  TaskMgr.registerPeer(otherNode);
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
  Serial << "Receiving SW1\n";
}

// Task for the second LED
// It auto-waits for a message, then gets the passed value and sets the LED to that value.
void led2Task() {
  MessageInfo theMessage;
  memcpy(&theMessage, TaskMgr.getMessage(), sizeof(MessageInfo));
  Serial << "Receiving SW2 " << int(theMessage.ledLevel) << endl;
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
      TaskMgr.sendMessage(otherNode, LED1TASK, NULL, 0);
      Serial << "Sw1: sending\n";
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
  bool sendMe;
  sendMe = false;
  if(digitalRead(SW2)==LOW && theMessage.ledLevel<255) {
    // went from unpressed to pressed
    theMessage.ledLevel += 5;
    sendMe = true;
  } else if(digitalRead(SW2)==HIGH && theMessage.ledLevel>0) {
    // went from pressed to unpressed
    theMessage.ledLevel -= 5;
    sendMe = true;
  }   // else no change so don't do anything
  // Send the message
  if(sendMe) {
    Serial << "Sw2: sending " << theMessage.ledLevel << endl;
    if(theMessage.ledLevel>255) theMessage.ledLevel=255;
    else if(theMessage.ledLevel<0) theMessage.ledLevel=0;
    TaskMgr.sendMessage(otherNode, LED2TASK, &theMessage, sizeof(MessageInfo));
  }
}
