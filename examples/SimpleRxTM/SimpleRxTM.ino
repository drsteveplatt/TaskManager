// A rewrite of SimpleRX from https://forum.arduino.cc/t/simple-nrf24l01-2-4ghz-transceiver-demo/405123
// Uses TaskManager instead of raw RF24 calls

#if defined(ARDUINO_ARCH_AVR)
#include <RF24.h>
#include <TaskManager.h>
#define CE_PIN   9
#define CSN_PIN 10
#else
#include <TaskManager.h>
#endif

char dataReceived[10];

void setup() {
    Serial.begin(9600);

    Serial.println("SimpleRxTM Starting"); Serial.flush();
#if defined(ARDUINO_ARCH_AVR)
    TaskMgr.radioBegin(2, CE_PIN, CSN_PIN);
#else
    TaskMgr.radioBegin(2);
#endif

    TaskMgr.addAutoWaitMessage(1, receiveMessage);

}

void receiveMessage() {
  // put your main code here, to run repeatedly:
  strcpy(dataReceived, (char*)TaskMgr.getMessage());
  Serial.println(dataReceived); Serial.flush();
}
