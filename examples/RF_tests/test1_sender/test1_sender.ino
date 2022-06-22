// RF24 TEST1 sender
// SMP
// 2015-1004 Ver. 1.0

// Sends 1s and 0s to anything listening on "TEST1"
// One button, one switch.  The switch controls how the button is interpreted.
// If the switch is up, it is STREAM.  If the switch is down, the button is PULSE
// When STREAM, each loop will cause the state of the button (open=1, closed=0) to be
// sent.  When PULSE, a button press (open->closed) will cause alternating 1s and 0s
// to be sent.

#define PRINTING 1

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// define the switches
#define THE_BUTTON 9
#define THE_SWITCH 10

// and where the radio is
#define RF24_CE 7
#define RF24_CSN  8
#define PIPE_ID  "TEST1\0\0\0"

RF24 radio(RF24_CE, RF24_CSN);

int lastButtonVal;  // for pulse mode, what was the last button
int lastButtonSend;  // pulse mode, what was sent

struct Packet {
  int seq;
  int val;
};

Packet packet;  // the global packet to send

void sendDump(struct Packet p) {
  static int counter=0;
  Serial.print(counter++);
  Serial.print(" -> [");
  Serial.print(p.seq);
  Serial.print(", ");
  Serial.print(p.val);
  Serial.println("]");
}

void setup() {
  
  lastButtonVal = 0; 
  lastButtonSend = 0;
  //packet.seq = 0;

//#if PRINTING!=0
  Serial.begin(115200);
  Serial.println("Sender starting");
//#endif
  pinMode(THE_BUTTON, INPUT_PULLUP);
  pinMode(THE_SWITCH, INPUT_PULLUP);
  
  long realPipeID;
  memcpy(&realPipeID, PIPE_ID, sizeof(long));
  radio.begin();
    //radio.setDataRate( RF24_250KBPS );
  radio.setRetries(10,100);
  radio.setAutoAck(true);
  radio.openWritingPipe(realPipeID);
}

void loop() {
  // put your main code here, to run repeatedly:
  int buttonVal, switchVal;
  buttonVal = digitalRead(THE_BUTTON)==HIGH ? 0 : 1;
  switchVal = digitalRead(THE_SWITCH)==HIGH ? 0 : 1;
  Serial.print(buttonVal); Serial.print(" "); Serial.println(switchVal);
  
  if(switchVal==1) {
    int failCount;
    // stream, just send the button
    packet.val = buttonVal;
    failCount = 0;
    while(!radio.write(&packet, sizeof(packet)) && failCount<5) {
      failCount++;
      Serial.print("Stream: Write "); Serial.print(failCount); Serial.print(" failed.");
      delay(100);
    };
    packet.seq++;
#if PRINTING!=0
    Serial.print("(Stream) ");
    sendDump(packet);
#endif
  } else {
    // pulse, if button has gone 0->1 then flip, send lastButtonVal, debounce
    //        if button has gone 1->0, just flip and debounce
    // Debounce is defined as a 50ms delay
    if(lastButtonVal==0 && buttonVal==1) {
      int failCount;
      //delay(50);
      lastButtonVal = buttonVal;
      lastButtonSend = lastButtonSend==0 ? 1 : 0;
      packet.val = lastButtonSend;
      failCount = 0;
      while(!radio.write(&packet, sizeof(packet)) && failCount<5) {
        failCount++;
        Serial.print("Press: Write "); Serial.print(failCount); Serial.print(" failed.");
        delay(100);
      };
      packet.seq++;
#if PRINTING!=0
      Serial.print("Pulse+ ");
      sendDump(packet);
#endif
    } else if (lastButtonVal==1 && buttonVal==0) {
      lastButtonVal = buttonVal;
#if PRINTING!=0
      Serial.println("Pulse-");
#endif
      //delay(50);
    } // else do nothing
  }
}
