
#define TASKMANAGER_MAIN

#include <SPI.h>
#include <RF24.h>
#include "TaskManager.h"

/*! \file TaskManager.cpp
    Implementation file for Arduino Task Manager
*/

static void nullTask() {
}

// This is the replacement for the normal loop().
//!	\private
void loop() {
    TaskMgr.loop();
}

static void radioReceiverTask() {
	TaskMgr.tmRadioReceiverTask();
}

//
// Implementation of _TaskManagerTask
//

/*! \brief Determine whether or not a task can be run NOW

    Determine whether or not a task is runnable.  A task can be run if and only if all of
    the following are true:
        (a) it is not suspended,
        (b) it is not waiting for a signal,
        (c) it is not waiting for a message, and
        (d) either it is not waiting for a time or
    that time has passed.
    */
bool _TaskManagerTask::isRunnable()  {
    // if isRunnable returns TRUE, it should not be called again on the
    // task until the task is run.  Otherwise, internal state descriptions
    // for the task may be mangled.
    //
    // Note that WaitMessage and WaitSignal will have been cleared if a
    // process has received a message/signal.  We will clear WaitUntil here.
    // Also, if a process has been (WaitUntil+WaitMessage/Signal) and it
    // times out, the accompanying WaitMessage/Signal will be cleared.
    bool ret;
    // new isRunnable
    // handles message/signal with timeout
    stateClear(TimedOut);
    if(stateTestBit(Suspended)) {
            ret = false;
    } else if(stateTestBit(WaitUntil) && m_restartTime<millis()) {
        // complex -- either a straight WaitUntil or a signal/message timeout
        stateClear(WaitUntil);
        if(anyStateSet(WaitSignal+WaitMessage)) {
            // timeout
            stateClear(WaitSignal); // to be sure
            stateClear(WaitMessage); // to be sure
            stateSet(TimedOut);    // used if signalled, others should ignore
        }
        ret = true;
    } else if(anyStateSet(WaitUntil+WaitMessage+WaitSignal)) {
        ret = false;
    } else {
        ret = true;
    }
    return ret;
}

/*! \brief Assign the value of one task to another

    Assign all values from one task to another.  The LHS becomes a copy of the data values of
    the RHS.
    \param rhs - The source task
*/
_TaskManagerTask& _TaskManagerTask::operator=(_TaskManagerTask& rhs){
    // makes a complete copy of all of the values
    m_stateFlags = rhs.m_stateFlags;

    m_restartTime = rhs.m_restartTime;
    m_sigNum = rhs.m_sigNum;
    memcpy(m_message, rhs.m_message, TASKMGR_MESSAGE_SIZE);

    m_period = rhs.m_period;
    m_restartSignal = rhs.m_restartSignal;

    m_id = rhs.m_id;
    m_fn = rhs.m_fn;
}

/*! \brief Compare the data contents of two tasks

    Returns true if and only if all of the data values (id, function, etc.) are the same.
*/
bool _TaskManagerTask::operator==(_TaskManagerTask& rhs) const {
    return m_id==rhs.m_id && m_fn==rhs.m_fn;
}

/*! \brief Print out information about the task
*/
size_t _TaskManagerTask::printTo(Print& p) const {
    size_t ret = 35;
    p.print("    [task: ");
    ret += p.print(this->m_id);
    p.print(" flgs:");
    ret += p.print(this->m_stateFlags, HEX);
    p.print(" rstTim:");
    ret += p.print(this->m_restartTime);
    p.print(" period:");
    ret += p.print(this->m_period);
    p.print(" sig:");
    ret += p.print(this->m_sigNum);
    p.print(" fn:");
    ret += p.print((int)(this->m_fn), HEX);
    p.print(" now: ");
    ret += p.print(millis());
    p.print("]\n");
    return ret;
}

//
// Implementation of TaskManager
//

// Constructor and Destructor


TaskManager::TaskManager() {
    add(0xff, nullTask);
	m_rf24 = NULL;
	m_myNodeId = 0;
	m_radioReceiverRunning = false;
    m_startTime = millis();
}

TaskManager::~TaskManager() {
	if(m_rf24!=NULL) delete m_rf24;
}

//
//  Add a new task
//


void TaskManager::add(byte taskId, void (*fn)()) {
    _TaskManagerTask newTask(taskId, fn);
    m_theTasks.push_back(newTask);
}


void TaskManager::addWaitDelay(byte taskId, void(*fn)(), unsigned long msDelay) {
    addWaitUntil(taskId, fn, millis() + msDelay);
}

void TaskManager::addWaitUntil(byte taskId, void(*fn)(), unsigned long msWhen) {
    _TaskManagerTask newTask(taskId, fn);
    newTask.setWaitUntil(msWhen);
    m_theTasks.push_back(newTask);
}

void TaskManager::addAutoWaitDelay(byte taskId, void(*fn)(), unsigned long period, bool startWaiting /*=false*/) {
    _TaskManagerTask newTask(taskId, fn);
    if(startWaiting) newTask.setWaitDelay(period); else newTask.m_restartTime = millis();
    newTask.setAutoDelay(period);
    m_theTasks.push_back(newTask);
}


void TaskManager::addWaitSignal(byte taskId, void(*fn)(), byte sigNum, unsigned long timeout/*=0*/){
    _TaskManagerTask newTask(taskId, fn);
    newTask.setWaitSignal(sigNum, timeout);
    m_theTasks.push_back(newTask);
}


void TaskManager::addAutoWaitSignal(byte taskId, void(*fn)(), byte sigNum, unsigned long timeout/*=0*/, bool startWaiting/*=true*/) {
    _TaskManagerTask newTask(taskId, fn);
    if(startWaiting) {
        newTask.setWaitSignal(sigNum, timeout);
        if(timeout>0) newTask.setWaitUntil(millis()+timeout);
    }
    newTask.setAutoSignal(sigNum, timeout);
    m_theTasks.push_back(newTask);
}

void TaskManager::addWaitMessage(byte taskId, void (*fn)(), unsigned long timeout/*=0*/) {
    _TaskManagerTask newTask(taskId, fn);
    newTask.setWaitMessage(timeout);
    m_theTasks.push_back(newTask);
}

void TaskManager::addAutoWaitMessage(byte taskId, void (*fn)(), unsigned long timeout/*=0*/, bool startWaiting/*=true*/) {
    _TaskManagerTask newTask(taskId, fn);
    if(startWaiting) {
        newTask.setWaitMessage(timeout);
        if(timeout>0) newTask.setWaitUntil(millis()+timeout);
    }
    newTask.setAutoMessage(timeout);
    m_theTasks.push_back(newTask);
}
//
// Yield functions
//

void TaskManager::yield() {
    longjmp(taskJmpBuf, YtYield);
}

void TaskManager::yieldDelay(unsigned long ms) {
    yieldUntil(millis()+ms);
}

void TaskManager::yieldUntil(unsigned long when) {
    // mark it as waiting
    m_theTasks.front().setWaitUntil(when);
    longjmp(taskJmpBuf, YtYieldUntil);
}

void TaskManager::yieldForSignal(byte sigNum, unsigned long timeout/*=0*/) {
    m_theTasks.front().setWaitSignal(sigNum, timeout);
    longjmp(taskJmpBuf, YtYieldSignalTimeout);
}


void TaskManager::yieldForMessage(unsigned long timeout/*=0*/) {
    m_theTasks.front().setWaitMessage(timeout);
    longjmp(taskJmpBuf, YtYieldMessageTimeout);
}
//
// Signal functions
//

void TaskManager::internalSendSignal(byte fromNodeId, byte fromTaskId, byte sigNum) {
    ring<_TaskManagerTask> tmpTasks;
    _TaskManagerTask* tmt;
    _TaskManagerTask* last;

    tmpTasks = m_theTasks;
    bool found = false;
    last = &(m_theTasks.back());

    // scan for the first thing that will receive the listed signal.
    while(&(tmpTasks.front())!=last && !found) {
        tmt = &(tmpTasks.front());
        if(tmt->stateTestBit(_TaskManagerTask::WaitSignal) && tmt->m_sigNum==sigNum) {
			tmt->m_fromNodeId = fromNodeId;
			tmt->m_fromTaskId = fromTaskId;
            tmt->signal();
            found = true;
        }
        tmpTasks.move_next();
    }
    if(!found && last->stateTestBit(_TaskManagerTask::WaitSignal) && last->m_sigNum==sigNum) {
        // the last task is the one we want to signal
        last->m_fromNodeId = fromNodeId;
        last->m_fromTaskId = fromTaskId;
        last->signal();
    }
}

void TaskManager::internalSendSignalAll(byte fromNodeId, byte fromTaskId, byte sigNum) {
    ring<_TaskManagerTask> tmpTasks;
    _TaskManagerTask* tmt;
    _TaskManagerTask* last;

    tmpTasks = m_theTasks;
    last = &(m_theTasks.back());

    // scan for all things that will receive the listed signal.
    while(&(tmpTasks.front())!=last) {
        tmt = &(tmpTasks.front());
        if(tmt->stateTestBit(_TaskManagerTask::WaitSignal) && tmt->m_sigNum==sigNum) {
			tmt->m_fromNodeId = fromNodeId;
			tmt->m_fromTaskId = fromTaskId;
            tmt->signal();
        }
        tmpTasks.move_next();
    }
    if(last->stateTestBit(_TaskManagerTask::WaitSignal) && last->m_sigNum==sigNum) {
        // the last task is the one we want to signal
        last->m_fromNodeId = fromNodeId;
        last->m_fromTaskId = fromTaskId;
        last->signal();
    }
}

void TaskManager::internalSendMessage(byte fromNodeId, byte fromTaskId, byte taskId, char* message) {
    _TaskManagerTask* tsk;
    if(strlen(message)>TASKMGR_MESSAGE_SIZE-1) return;
    tsk = findTaskById(taskId);
    if(tsk==NULL) return;
    tsk->m_fromNodeId = fromNodeId;
    tsk->m_fromTaskId = fromTaskId;
    tsk->putMessage((void*)message, strlen(message)+1);
}

void TaskManager::internalSendMessage(byte fromNodeId, byte fromTaskId, byte taskId, void* buf, int len) {
    _TaskManagerTask* tsk;
    if(len>TASKMGR_MESSAGE_SIZE) return;
    tsk = findTaskById(taskId);
    if(tsk==NULL) return;
    tsk->m_fromNodeId = fromNodeId;
    tsk->m_fromTaskId = fromTaskId;
    tsk->putMessage(buf, len);
}

// FindNextRunnable
// Relies on the null task being present and always runnable.
/*! \brief Find tne next runnable task.  Internal routine.

    Finds the next runnaable task on the task ring.  This routine assumes that the task ring is on the
    most-recently-run task.  It moves the task ring to the next task that is ready to run.  Since there
    is a null task and it is always runnable, FindNextRunnable() is guaranteed to find a runnable task.
    For internal use only.
*/
_TaskManagerTask* TaskManager::FindNextRunnable() {
    // Note:  This is the ONLY routine that should modify the m_theTasks ring's position pointer.
    // (Except for add and kill)
    //
    // In: m_theTasks points to the most recently executed task.
    // Out: m_theTasks points to the next task to be executed (the first one AFTER the current one
    //      that is runnable).  Note that it may end up on the same spot it started at, but to
    //      do so, it would have to check all other tasks.
    // Note: m_theTasks must NEVER be empty.  It must ALWAYS have a NULL task that is runnable.
    _TaskManagerTask* tmt;
    m_theTasks.move_next(); // go past the current one
    tmt = &(m_theTasks.front());
    while(!tmt->isRunnable()) {
        m_theTasks.move_next();
        tmt = &(m_theTasks.front());
    }
    return tmt;
}


// printTo
/*! \brief Support routine to allow the printing of the contents of a TaskManager object.

    This prints a TaskManager object.  It should only be used for debugging purposes.
*/
size_t TaskManager::printTo(Print& p) const {
    size_t ret;
    ret = p.print(m_theTasks);
    return ret;
}
/*! \brief Find a task by its ID

    \param id: the ID of the task
    \return A pointer to the _TaskManagerTask or NULL if not found
*/
_TaskManagerTask* TaskManager::findTaskById(byte id) {
     ring<_TaskManagerTask> tmpTasks;
    _TaskManagerTask* tmt;
    _TaskManagerTask* last;
    _TaskManagerTask* theTask = NULL;

    tmpTasks = m_theTasks;
    bool found = false;
    last = &(m_theTasks.back());

    // scan for the first thing that has the given id
    while(&(tmpTasks.front())!=last && !found) {
        tmt = &(tmpTasks.front());
        if(tmt->m_id==id) {
            found = true;
            theTask = tmt;
        }
        tmpTasks.move_next();
    }
    // check the last one if we haven't found it yet
    if(!found && last->m_id==id) {
        found = true;
        theTask = last;
    }
    return theTask;
}

/*! \brief Implements a single pass for the system loop() routine.

    This performs a single iteration for the system loop() routine.  It finds the next
    runnable task and runs it.  It processes any yield*() operations that the user routine
    may have executed.

    This routine is for internal use only.
*/
void TaskManager::loop() {
    int jmpVal; // return value from setjmp, indicates longjmp type
    _TaskManagerTask* nextTask;
    nextTask = TaskMgr.FindNextRunnable();
    if((jmpVal=setjmp(TaskMgr.taskJmpBuf))==0) {
        // this is the normal path we use to invoke and process "normal" returns
        (nextTask->m_fn)();
        // Process auto bits
        // AutoReWaitUntil has two interpretations:  If alone, it is periodic and is
        // incremented based on the previous restartTime.  If in conjunction with
        // AutoReSignal/AutoReMessage then it is a timeout and is based on "now"
#if 0
        if(nextTask->stateTestBit(_TaskManagerTask::AutoReWaitUntil)) {
		   unsigned long newRestart = nextTask->m_restartTime;	// the time this one started
		   unsigned long now = millis();
		   while(newRestart<now) {
                newRestart += nextTask->m_period;	// keep bumping til the future
		   }
		   nextTask->setWaitUntil(newRestart);
        }
        // Note that timeout autorestarts will have AutoReUntil and AutoReSignal/Message set
        // So to handle this, we process the bits separately and set both if needed.
        if(nextTask->stateTestBit(_TaskManagerTask::AutoReWaitSignal)) {
           nextTask->setWaitSignal(nextTask->m_restartSignal);
        } else if(nextTask->stateTestBit(_TaskManagerTask::AutoReWaitMessage)) {
           nextTask->setWaitMessage();
        }
#else
		// new code to handle different interpretations
		if(nextTask->stateTestBit(_TaskManagerTask::AutoReWaitSignal) ||
			nextTask->stateTestBit(_TaskManagerTask::AutoReWaitMessage)) {
			// set up next signal/message
			if(nextTask->stateTestBit(_TaskManagerTask::AutoReWaitSignal)) {
			   nextTask->setWaitSignal(nextTask->m_restartSignal);
			} else {
			   nextTask->setWaitMessage();
			}
			// set up next timing
			nextTask->setWaitUntil(millis()+nextTask->m_period);
		} else {
			// just check for AutoReWaitUntil and process periodic timing
			if(nextTask->stateTestBit(_TaskManagerTask::AutoReWaitUntil)) {
			   unsigned long newRestart = nextTask->m_restartTime;	// the time this one started
			   unsigned long now = millis();
			   while(newRestart<now) {
					newRestart += nextTask->m_period;	// keep bumping til the future
			   }
			   nextTask->setWaitUntil(newRestart);
			}
        }
#endif
    } else {
        // this is the path executed if a yield was called
        // Yield types (jmpVal values) are from YtYield
        //
        // AutoRestart is tricky here.  It needs to be handled separately
        // in each case.
        //
        // In each case, the yield*(...) routine will have set the appropriate flag bit(s)
        // and if needed, stuffed a m_restartTime value if a delay/timeout was specified.
        switch(jmpVal) {
            case YtYield:
                // normal yield, just exit cleanly
                // Autorestart is ignored
                break;
            case YtYieldUntil:
                // yieldUntil/yieldDelay will have marked the next completion time so exit cleanly
                // Autorestart is ignored
                break;
            case YtYieldSignal:
                // yieldSignal will have stuffed the signal marker and flag so exit cleanly
                // Autorestart is ignored
                break;
            case YtYieldSignalTimeout:
                // yieldSignalTimeout will have stuffed the signal marker and timeout and flag so exit cleanly
                // Autorestart is ignored
                break;
            case YtYieldMessage:
                // yieldMessage will have stuffed the message marker so exit cleanly
                // Autorestart is ignored
                break;
            case YtYieldMessageTimeout:
                // yieldMessageTimeout will have stuffed the message marker and timeout so exit cleanly
                // Autorestart is ignored
                break;
            case YtYieldSuspend:
                // yieldSuspend will have stuffed the suspend flag so exit cleanly
                // AutoRestart:  We're suspended.  AutoRestart is ignored.
                break;
            case YtYieldKill:
                // kill: we need to remove the current task from the task ring.  It is gone.
                // AutoRestart:  The task is being killed.  It will never AutoRestart
                //**** MEMORY LEAK:  NEED TO DISPOSE OF THE TASK *****
                TaskMgr.m_theTasks.pop_front();
                break;
            default:
                // ignore invalid yields
                // YOU SHOULD NOT BE GETTING HERE!!!
                break;
        }
     }
}

// Status tasks

void TaskManager::suspend(byte taskId) {
}

void TaskManager::resume(byte taskId) {
}

// ***************************
//  All the world's radio code
// ***************************


// General purpose receiver.  Gets a message (varying with the kind of radio)
// then parses and delivers it
void TaskManager::tmRadioReceiverTask() {
	static int cnt=0;
	// polled receiver -- if there is a packet waiting, grab and process it
	// receive packet from NRF24.  Poll and process messages
	// We need to find the destination task and save the fromNode and fromTask.
	// They are saved on the task instead of the TaskManager object in case several
	// messages/signals have been received.
	while(m_rf24->available()) {
		// read a packet
		m_rf24->read((void*)(&radioBuf), sizeof(radioBuf));
		// process it
		switch(radioBuf.m_cmd) {
			case tmrNoop:
				break;
			case tmrStatus:	// NYI
				break;
			case tmrAck:	// NYI
				break;
			case tmrTaskStatus:	// NYI
				break;
			case tmrTaskAck:	// NYI
				break;
			case tmrSignal:
				internalSendSignal(radioBuf.m_fromNodeId, radioBuf.m_fromTaskId, radioBuf.m_data[0]);
				break;
			case tmrSignalAll:
				sendSignalAll(radioBuf.m_data[0]);
				break;
			case tmrMessage:
				internalSendMessage(radioBuf.m_fromNodeId, radioBuf.m_fromTaskId,
					radioBuf.m_data[0], &radioBuf.m_data[1], TASKMGR_MESSAGE_SIZE);
				break;
			case tmrSuspend:
				suspend(radioBuf.m_data[0]);
				break;
			case tmrResume:
				resume(radioBuf.m_data[0]);
				break;
		}
	}
}

// General purpose sender.  Sends a message somewhere (varying with the kind of radio)
void TaskManager::radioSender(byte destNodeID) {
	// send packet to NRF24 node "TMGR"+nodeID
	static byte nodeName[6] = "xTMGR";
	m_rf24->stopListening(); delay(50);
	nodeName[0] = destNodeID;
	m_rf24->openWritingPipe(nodeName);
	for(int i=0; i<5; i++) {
		if(!m_rf24->write(&radioBuf, sizeof(radioBuf))) {
			Serial.print("write fail "); Serial.println(i);
		}
		else {
			break;
		}
	}
	m_rf24->startListening();
}

// If we have different radio receivers, they will have different instantiation routines.

void TaskManager::radioBegin(byte nodeId, byte cePin, byte csPin) {
	uint8_t pipeName[6];
	m_myNodeId = nodeId;
	m_rf24 = new RF24(cePin, csPin);
	strcpy((char*)pipeName,"xTMGR");	// R for read
	pipeName[0] = myNodeId();	// (read pipe preconfigure) not printable, who cares...
	m_rf24->begin();
	m_rf24->openReadingPipe(1, pipeName);
	m_rf24->startListening();
	pipeName[3] = 'W';	// write
	m_rf24->openWritingPipe(pipeName);
	if(!m_radioReceiverRunning) { add(0xfe, radioReceiverTask); m_radioReceiverRunning = true; }
}


void TaskManager::sendSignal(byte nodeId, byte sigNum) {
	if(nodeId==0 || nodeId==myNodeId()) { sendSignal(sigNum); return; }
	radioBuf.m_cmd = tmrSignal;
	radioBuf.m_fromNodeId = myNodeId();
	radioBuf.m_fromTaskId = myId();
	radioBuf.m_data[0] = sigNum;
	radioSender(nodeId);
}

void TaskManager::sendSignalAll(byte nodeId, byte sigNum) {
	if(nodeId==0 || nodeId==myNodeId()) { sendSignalAll(sigNum); return; }
	radioBuf.m_cmd = tmrSignalAll;
	radioBuf.m_fromNodeId = myNodeId();
	radioBuf.m_fromTaskId = myId();
	radioBuf.m_data[0] = sigNum;
	radioSender(nodeId);
}

void TaskManager::sendMessage(byte nodeId, byte taskId, char* message) {
	if(nodeId==0 || nodeId==myNodeId()) { sendMessage(taskId, message); return; }
	radioBuf.m_cmd = tmrMessage;
	radioBuf.m_fromNodeId = myNodeId();
	radioBuf.m_fromTaskId = myId();
	radioBuf.m_data[0] = taskId;	// who we are sending it to
	if(strlen(message)>TASKMGR_MESSAGE_SIZE-2) {
		memcpy(&radioBuf.m_data[1], message, TASKMGR_MESSAGE_SIZE-2);
		radioBuf.m_data[TASKMGR_MESSAGE_SIZE-2]='\0';
	} else {
		strcpy((char*)&radioBuf.m_data[1], message);
	}
	radioSender(nodeId);
}

void TaskManager::sendMessage(byte nodeId, byte taskId, void* buf, int len) {
	if(nodeId==0 || nodeId==myNodeId()) { sendMessage(taskId, buf, len); return; }
	if(len>TASKMGR_MESSAGE_SIZE) return;	// reject too-long messages
	radioBuf.m_cmd = tmrMessage;
	radioBuf.m_fromNodeId = myNodeId();
	radioBuf.m_fromTaskId = myId();
	radioBuf.m_data[0] = taskId;	// who we are sending it to
	memcpy(&radioBuf.m_data[1], buf, len);
	radioSender(nodeId);
}

void TaskManager::suspend(byte nodeId, byte taskId) {
	if(nodeId==0 || nodeId==myNodeId()) { suspend(taskId); return; }
	radioBuf.m_cmd = tmrSuspend;
	radioBuf.m_fromNodeId = myNodeId();
	radioBuf.m_fromTaskId = myId();
	radioBuf.m_data[0] = taskId;
	radioSender(nodeId);
}

void TaskManager::resume(byte nodeId, byte taskId) {
	if(nodeId==0 || nodeId==myNodeId()) { resume(taskId); return; }
	radioBuf.m_cmd = tmrResume;
	radioBuf.m_fromNodeId = myNodeId();
	radioBuf.m_fromTaskId = myId();
	radioBuf.m_data[0] = taskId;
	radioSender(nodeId);
}

void TaskManager::getSource(byte& fromNodeId, byte& fromTaskId) {
	fromNodeId = m_theTasks.front().m_fromNodeId;
	fromTaskId = m_theTasks.front().m_fromTaskId;
}


