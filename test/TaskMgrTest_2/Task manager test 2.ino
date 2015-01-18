#include <Arduino.h>
#include <TaskManager/Streaming.h>
#include <TaskManager/TaskManager.h>

/*
  Iteration 2 test of Arduino Task Manager
  Tasks:
    1. Blink on/off every 5000 ms (autorepeat)
    2. Blink on/off every 5000 ms (autorepeat, delayed)
    3. Blink on/off every 1333 ms (yieldDelay)
    4. Blink on/off every 1500 ms (yieldUntil)
*/

unsigned long clock0;   // time at start of run

void task0() {
    // just run
    Serial << "T0 at " << millis()-clock0 << "\n";
}

void task1() {
    // autorepeat
    Serial << "T1 now " << millis()-clock0 <<  " plain autoreschedule(2) at " << millis()+2000-clock0 << "\n";
}
void task2() {
    // autorepeat with delayed start
    Serial << "T2 now " << millis()-clock0 <<  " delayed(5) autoreschedule(5) at " << millis()+ 5000 -clock0 << "\n";
}
void task3() {
    // yieldDelay
    static int nIter = 0;
    Serial << "T3 now " << millis()-clock0 <<  " yield " << nIter << "/5 times (3) then stop) at " << millis()-clock0 << "\n";
    nIter++;
    if(nIter<5) TaskMgr.yieldDelay(5000); else TaskMgr.yieldDelay(1000000);
}
void task4() {
    // yieldUntil with limited number of runs
    static int execs=0;
    unsigned long nextRun;
    execs++;
    nextRun = millis() + 3000;
    Serial << "T4 now " << millis()-clock0 <<  " yieldUntil(now+3) at " << millis()-clock0
        << " next at " << nextRun - clock0 << " iter " << execs << "\n";
    TaskMgr.yieldUntil(nextRun);
}
void task5() {
    // addDelay
    // yieldDelay
    Serial << "T5 now " << millis()-clock0 <<  " yieldDelay(4) at " << millis()-clock0 << "\n";
    TaskMgr.yieldDelay(4000);
}

void setup()
{
	Serial.begin(9600);

	Serial << "hello\n";

    //TaskMgr.add(0, task0);
    TaskMgr.addAutoWaitDelay(1, task1, 2000);
    TaskMgr.addAutoWaitDelay(2, task2, 5000, true);
    TaskMgr.add(3, task3);
    TaskMgr.add(4, task4);
    //TaskMgr.addDelayed(5, task5, 4000);
    // set clock0 as our base for future messaging
    clock0 = millis();

    Serial << "setup done, clock0 is " << clock0 << "\n";
    Serial << TaskMgr << '\n';
}


