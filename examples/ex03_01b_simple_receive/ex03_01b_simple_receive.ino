// TaskManager 2.0 test 3
// Simple receive, a partner of simple send
// During basic testing, be sure to set the ESP/AVR _RF flag appropriately

#if defined(ARDUINO_ARCH_AVR)
#include <RF24.h>
#include <TaskManager_2.h>
#define CE_PIN   9
#define CSN_PIN 10
#else
#include <TaskManager_2.h>
#endif

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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Receiver starting.");
  pinMode(HEARTBEATLED, OUTPUT);

#if defined(ARDUINO_ARCH_AVR)
  TaskMgr.radioBegin(2, CE_PIN, CSN_PIN);
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  TaskMgr.radioBegin(2);
#endif

  TaskMgr.addAutoWaitDelay(1, heartbeatTask, 500);
  TaskMgr.addAutoWaitMessage(2, receiverTask);
}

// Heartbeat task
void heartbeatTask() {
  static bool isLow = true;
  digitalWrite(HEARTBEATLED, isLow ? HIGH : LOW);
  isLow = !isLow;
}

void receiverTask() {
  char* theMsg;
  theMsg = (char*)TaskMgr.getMessage();
  Serial.print("Received ["); Serial.print(theMsg); Serial.println("]");
}
