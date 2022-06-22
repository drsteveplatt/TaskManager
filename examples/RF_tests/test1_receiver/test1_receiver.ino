// RF24 TEST1 receiver
// SMP
// 2015-1003 Ver. 1.0
// 2019-1018 Nano/Mega version

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// Pick a board -- uncomment ONE of these
#define NANO
//#define MEGA

#define PRINTING 1 

// which pin the LED is on
#if defined(NANO)
#define LED  3
#elif defined(MEGA)
#define LED 13
#endif

// and where the radio is
#if defined(NANO)
#define RF24_CE 7
#define RF24_CSN  8
#elif defined(MEGA)
#define RF24_CE 31
#define RF24_CSN 30
#endif
#define PIPE_ID  "TEST1\0\0\0"

RF24 radio(RF24_CE, RF24_CSN);

int count;  // how many radio reads

struct Packet {
  int count;
  int val;
};

Packet packet;

void setup() {
  // put your setup code here, to run once:
#if PRINTING>0
  Serial.begin(115200);
  Serial.println("v1.2 Receiver starting");
#endif

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  delay(1000);
  digitalWrite(LED, LOW);
  
  long realPipeID;
  memcpy(&realPipeID, PIPE_ID, sizeof(long));
  radio.begin();
    //radio.setDataRate( RF24_250KBPS );
  radio.openReadingPipe(1, realPipeID);
  radio.startListening();
  
  count = 0;
}

void loop() {
  //Serial.println("hello");
  int radioData;
  if(radio.available()) {
    Serial.println("Data received");
    radio.read(&packet, sizeof(Packet));
    radioData = packet.val;
    digitalWrite(LED, radioData==0?LOW:HIGH);
    
#if PRINTING>0
    Serial.print(count++);
    Serial.print(" -> [");
    Serial.print(packet.count);
    Serial.print(", ");
    Serial.print(packet.val);
    Serial.println("]");
#endif
  }

}
