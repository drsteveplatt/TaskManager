#include <Arduino.h>

#include "../../Streaming.h"
#include "../../TaskManager.h"

/*
  Turns on an LED on for one second, then off for one second, repeatedly.
*/

/* The test builds the following task list (adds done in the opposite order)
    sig_1 - middle of task list
    sig_2 - right before signaller
    signaller
    sig_3 - right after signaller
    sig_4 - middle of task list
    sig_4a - middle of task list, same signal as sig4, will only get "all"
    null task
    sig_6 - middle of task list, has timeout, timeout not hit (gets signal 6)
    sig_9 - will timeout after 5 seconds

    The signaller will signal sig_1 sig_2 sig_3 Sig_4 sig_all none in sequence, one action per second

    This is similar to TaskMgrTest_3 except it uses addAutoSignal

*/
void sig_9() {
    Serial << "Sig_9 called, timeout = " << (TaskMgr.timedOut()?"Yes ":"No ") <<  "t=" << TaskMgr.runtime() << '\n';
}
void sig_6() {
    Serial << "Sig_6 called, timeout = " << (TaskMgr.timedOut()?"Yes ":"No ") <<  "t=" << TaskMgr.runtime() << '\n';
}
void sig_4a() {
    Serial << "Sig_4a sig received, t=" << TaskMgr.runtime() << '\n';
}
void sig_4() {
    Serial << "Sig_4 sig received, t=" << TaskMgr.runtime() << '\n';
}
void sig_3() {
    Serial << "Sig_3 signal received, t=" << TaskMgr.runtime() << '\n';
}
void sig_2() {
    // this also tests out override
    static bool override = false;
    Serial << "Sig_2 signal received, t=" << TaskMgr.runtime() <<
        " override=" << (override?"true\n":"false\n");
    if(override) {
        override = false;
        TaskMgr.yieldDelay(10000);
    } else override=true;
}
void sig_1() {
    Serial << "Sig_1 signal received, t=" << TaskMgr.runtime() << '\n';
}
void signaller() {
    static int next = 1;
    Serial << "Signaller, about to signal " << next << " t=" << TaskMgr.runtime() << '\n';
    if(next==5) TaskMgr.sendSignalAll(4);
    else TaskMgr.sendSignal(next);
    next++;
    // we send signal 7 nowhere
    if(next>7) next=1;
}
void setup()
{
	Serial.begin(9600);
	TaskMgr.addAutoWaitSignal(1, sig_1, 1);
	TaskMgr.addAutoWaitSignal(2, sig_2, 2);
	TaskMgr.addAutoWaitDelay(10, signaller, 1000);  // send a signal a second
    TaskMgr.addAutoWaitSignal(3, sig_3, 3);
	TaskMgr.addAutoWaitSignal(4, sig_4, 4);         // hit with signal(4) and signalAll(4)
	TaskMgr.addAutoWaitSignal(5, sig_4a, 4);        // just hit with signalAll(4)
	TaskMgr.addAutoWaitSignal(6, sig_6, 6, 10000);  // called before timeout
	TaskMgr.addAutoWaitSignal(9, sig_9, 9, 10000);  // times out

    Serial << "Tasks Added\n" << TaskMgr << '\n  ';
}



