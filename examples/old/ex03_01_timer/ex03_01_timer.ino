// TimerTest
// Tests the time needed for various operations

#include <TaskManager.h>
#include <Streaming.h>

void workerSub(byte*);  // our external routine

void setup() {
  // timer variables
  long  tStart, tEnd;
  long tLoop1000, tLoop10000, tfLoop1000, tfLoop10000;
  //
  int i;
  byte b;

  Serial.begin(115200);

  tStart=micros();
  for(i=0; i<1000; i++) continue;
  tEnd=micros();
  tLoop1000 = tEnd-tStart;

  tStsart=micros();
  for(i=0; i<10000; i++) continue;
  tEnd=micros();
  tLoop10000 = tEnd-tStart;

  b=0;
  tStart=micros();
  for(i=0; i<1000; i++) workerSub(b);
  tEnd=micros();
  tfLoop1000 = tEnd-tStart;

  b=0;
  tStart=micros();
  for(i=0; i<10000; i++) workerSub(b);
  tEnd=micros();
  tfLoop10000 = tEnd-tStart;

  Serial << "No fn: " << tLoop1000 << " " << tLoop10000
    << " fn: " << tfLoop1000 << " " << tfLoop10000 << endl;
  
}

void loop() {
  // put your main code here, to run repeatedly:

}
