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


// Node IDs
int thisNode, otherNode;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Sender starting.");

  // Set up our data lines
  pinMode(LED1, OUTPUT);
#if defined(ARDUINO_ARCH_AVR)
  pinMode(LED2, OUTPUT);
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  ledcAttachPin(LED2, LED2PWMCHANNEL);
  ledcSetup(LED2PWMCHANNEL, 4000, 8); // channel, frequency, bits
#endif
  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);
  pinMode(HEARTBEATLED, OUTPUT);
  pinMode(BOARDSEL, INPUT_PULLUP);

  // set the addressing
  if(digitalRead(BOARDSEL)==HIGH) {
    thisNode = 1;
    otherNode = 2;
  } else {
    thisNode = 2;
    otherNode = 1;
  }
  Serial << "My nodeId is " << thisNode << " and the other node is " << otherNode << "\n";
  
#if defined(ARDUINO_ARCH_AVR)
  TaskMgr.radioBegin(thisNode, CE_PIN, CSN_PIN);
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  TaskMgr.radioBegin(thisNode);
  TaskMgr.registerPeer(otherNode);
#endif

  TaskMgr.addAutoWaitDelay(HEARTBEATTASK, heartbeatTask,500);
  TaskMgr.add(SW1TASK, sw1Task);
  //TaskMgr.addAutoWaitDelay(SW2TASK, sw2Task, 50);
  TaskMgr.addAutoWaitMessage(LED1TASK, led1Task);
  TaskMgr.addAutoWaitMessage(LED2TASK, led2Task);
  //TaskMgr.addAutoWaitDelay(LOCALSENDERTASK, localSenderTask, 5000);
  //TaskMgr.addAutoWaitMessage(LOCALRECEIVERTASK, localReceiverTask);
}

// Heartbeat task
void heartbeatTask() {
  static bool isLow = true;
  digitalWrite(HEARTBEATLED, isLow ? HIGH : LOW);
  isLow = !isLow;
}

void sw1Task() {
  static bool butPressed = false;
  TM_BEGIN();
  if(digitalRead(SW1)==LOW && !butPressed) {
    // just pressed, display status
    butPressed = true;
    // PUT ON-PRESS CODE HERE
    TaskMgr.sendMessage(/*otherNode, */LED1TASK, NULL);
    Serial << "SW1TASK: Sending message\n";
    // END OF ON-RELEASE STUFF
    TM_YIELDDELAY(1,10);  // debounce
  } else if(digitalRead(SW1)==HIGH && butPressed) {
    // was pressed, now released
    butPressed = false;
    // PUT ON-RELEASE CODE HERE
    // END OF ON-RELEASE CODE
    TM_YIELDDELAY(2,10);
  }
  TM_END();
}

struct Led2Info {
  int16_t m_level;
  Led2Info() { m_level = 0; };
};
void sw2Task() {
  // This task autorepeats
  static Led2Info msg;
  TM_BEGIN();
  if(digitalRead(SW1)==LOW) {
    // button pressed
    // PUT ON-PRESS CODE HERE
    TaskMgr.sendMessage(/*otherNode, */LED2TASK, (void*)&msg, sizeof(Led2Info));
    msg.m_level += 5;    // END OF ON-RELEASE STUFF
    if(msg.m_level>255) msg.m_level=255;
    TM_YIELDDELAY(1,10);  // debounce
  } else if(digitalRead(SW1)==HIGH) {
    // buton released
    // PUT ON-RELEASE CODE HERE
    TaskMgr.sendMessage(/*otherNode, */LED2TASK, (void*)&msg, sizeof(Led2Info));
    msg.m_level -= 5;    // END OF ON-RELEASE STUFF
    if(msg.m_level<0) msg.m_level=0;
    // END OF ON-RELEASE CODE
    TM_YIELDDELAY(2,10);
  }
  TM_END();
}

void led1Task() {
  // flips the LED when it receives a message (empty message)
  static bool isOn = false;
  Serial << "LED1TASK: received message, setting LED to " << (isOn ? "HIGH" : "LOW") << endl;
  digitalWrite(LED2, isOn ? LOW : HIGH);
  isOn = !isOn;
}

void led2Task() {
  Led2Info msg;
  memcpy(&msg, TaskMgr.getMessage(), sizeof(Led2Info));
  Serial << "LED2TASK: received msg " << msg.m_level << endl;
#if defined(ARDUINO_ARCH_AVR)
  analogWrite(LED2, msg.m_level);
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  ledcWrite(LED2PWMCHANNEL, msg.m_level);
#else
#endif
}

void localSenderTask() {
  
}

void localReceiverTask() {

}
