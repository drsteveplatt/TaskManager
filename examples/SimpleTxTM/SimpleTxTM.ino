// A rewrite of SimpleTX from https://forum.arduino.cc/t/simple-nrf24l01-2-4ghz-transceiver-demo/405123
// Uses TaskManager instead of raw RF24 calls



#if defined(ARDUINO_ARCH_AVR)
#include <RF24.h>
#include <TaskManager_2.h>
#define CE_PIN   9
#define CSN_PIN 10
#else
#include <TaskManager_2.h>
#endif

char dataToSend[10] = "Message 0";
char txNum = '0';

void setup() {
    Serial.begin(9600);

    Serial.println("SimpleTxTM Starting"); Serial.flush();

#if defined(ARDUINO_ARCH_AVR)
    TaskMgr.radioBegin(1, CE_PIN, CSN_PIN);
#else
    TaskMgr.radioBegin(1);
    TaskMgr.registerPeer(2);
#endif
    TaskMgr.addAutoWaitDelay(1, sendMessage, 2500);

}

void sendMessage() {
  // put your main code here, to run repeatedly:
    txNum += 1;
    if (txNum > '9') {
        txNum = '0';
    }
    dataToSend[8] = txNum;
    Serial.print("Data sent "); Serial.println(dataToSend); Serial.flush();
    TaskMgr.sendMessage(2, 1, dataToSend);
 }
