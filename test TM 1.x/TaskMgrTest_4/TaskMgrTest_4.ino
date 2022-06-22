#include <Arduino.h>

#include "../../Streaming.h"
#include "../../TaskManager.h"

/*
  Test out variants of AddAutoMessage
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
*/
class point {
public:
    point(): x(0), y(0) {}
    int x, y;
//    virtual size_t PrintTo(Print& p) const {
//        int ret = 4;
//        p << "[";
//        ret += p.print(x);
//        p << ", ";
//        ret += p.print(y);
//        p<<"]";
//    }
};
void msg_9() {
    Serial << "Msg_9 msg recieved [" << (char*)TaskMgr.getMessage() << "], timeout = " << (TaskMgr.timedOut()?"Yes ":"No ") <<  "t=" << TaskMgr.runtime() << '\n';
}
void msg_7() {
    Serial << "Msg_7 msg recieved [" << (char*)TaskMgr.getMessage() << "], timeout = " << (TaskMgr.timedOut()?"Yes ":"No ") <<  "t=" << TaskMgr.runtime() << '\n';
}
void msg_6() {
    // handler 6 recieves a point instead of a string.
    point* p;
    p = (point*)(TaskMgr.getMessage());
    Serial << "Msg_6 received [" << p->x << ", " << p->y <<  "], timeout = " << (TaskMgr.timedOut()?"Yes ":"No ") <<  "t=" << TaskMgr.runtime() << '\n';
}
void msg_4a() {
    Serial << "Msg_4a msg received [" << (char*)TaskMgr.getMessage() << "], t=" << TaskMgr.runtime() << '\n';
}
void msg_4() {
    Serial << "Msg_4 msg received [" << (char*)TaskMgr.getMessage() << "], t=" << TaskMgr.runtime() << '\n';
}
void msg_3() {
    Serial << "Msg_3 msg received [" << (char*)TaskMgr.getMessage() << "], t=" << TaskMgr.runtime() << '\n';
}
void msg_2() {
    Serial << "Msg_2 msg received [" << (char*)TaskMgr.getMessage() << "], t=" << TaskMgr.runtime() << '\n';
}
void msg_1() {
    Serial << "Msg_1 msg received [" << (char*)TaskMgr.getMessage() << "], t=" << TaskMgr.runtime() << '\n';
}
void signaller() {
    static int next = 1;
    static int x=0, y=0;
    point myP;
    myP.x = x;
    myP.y = y;
    x += next;
    y += 1;
    char buf[25];
    strcpy(buf,"my msg [x]");
    Serial << "Signaller, about to send message " << next << " t=" << TaskMgr.runtime() << '\n';
    // we send messages 4 and 5 at the same time to check for buffer overlaps.
    if(next==4) {
        buf[8] = '0'+next;
        TaskMgr.sendMessage(next, buf);
        buf[8] = '0'+next+1;
        TaskMgr.sendMessage(next+1, buf);
    } else if(next==6) {
        TaskMgr.sendMessage(next, (void*)&myP, sizeof(point));
    } else if(next!=5) {
        buf[8] = '0'+next;
        TaskMgr.sendMessage(next, buf);
    }

    next++;
    // we send signal 7 nowhere
    if(next>7) next=1;
}
void setup()
{
	Serial.begin(9600);
	TaskMgr.addAutoWaitMessage(1, msg_1);
	TaskMgr.addAutoWaitMessage(2, msg_2);
	TaskMgr.addAutoWaitDelay(10, signaller, 1000);  // send a signal a second
    TaskMgr.addAutoWaitMessage(3, msg_3);
	TaskMgr.addAutoWaitMessage(4, msg_4);
	TaskMgr.addAutoWaitMessage(5, msg_4a);
	TaskMgr.addAutoWaitMessage(6, msg_6, 10000);  // called before timeout
	TaskMgr.addAutoWaitMessage(7, msg_7, 5000, false); // called before timeout, but starts immediately
	TaskMgr.addAutoWaitMessage(9, msg_9, 10000);  // times out

    Serial << "Tasks Added\n" << TaskMgr << '\n';
}



