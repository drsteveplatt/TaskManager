#define TASKMANAGER_MAIN

#include <arduino.h>
#include <TaskManagerCore.h>
#include <Streaming.h>

#define DEBUG false



/*! \file TaskManager.cpp
    Implementation file for Arduino Task Manager
*/

/*!	\addtogroup _TaskManagerTask _TaskManagerTask
*/

/*! \addtogroup TaskManager TaskManager
*/

static void nullTask() {
}

// *******************************************************************
// Implementation of _TaskManagerTask
//
/*! \ingroup _TaskManagerTask
	@{
*/
/*! \brief Determine whether or not a task can be run NOW

    Determine whether or not a task is runnable.  A task can be run if and only if all of
    the following are true:
        (a) it is not suspended,
        (b) it is not waiting for a message, and
        (c) either it is not waiting for a time or that time has passed.
    */
bool _TaskManagerTask::isRunnable()  {
    // if isRunnable returns TRUE, it should not be called again on the
    // task until the task is run.  Otherwise, internal state descriptions
    // for the task may be mangled.
    //
    // Note that WaitMessage will have been cleared if a
    // process has received a message.  We will clear WaitUntil here.
    // Also, if a process has been (WaitUntil+WaitMessage) and it
    // times out, the accompanying WaitMessage will be cleared.
    bool ret;
    if(stateTestBit(Suspended)) {
		ret = false;
	} else if(stateTestBit(WaitMessage)) {
		if(stateTestBit(WaitUntil)) {
			// waiting for message or timeout, act based on timeout
			if(m_restartTime<millis()) {
				// timed out
				stateClear(WaitUntil);
				stateSet(TimedOut);
				ret = true;
			} else {
				// neither timeout nor message
				ret = false;
			}
		} else {
			// just waiting for message, no timeout
			ret = false;
		}
	} else if(stateTestBit(WaitUntil)) {
		if(m_restartTime<millis()) {
			// yes waiting for a time and the time has passed
			stateClear(WaitUntil);
			ret = true;
		} else {
			// waiting for a time, time hasn't passed yet
			ret = false;
		}
	} else {
		// was not waiting for signal or message or time passage
		//if(DEBUG && (my_Id==1 || my_Id==20)) Serial << "not waiting for anything\n";
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
    memcpy(m_message, rhs.m_message, TASKMGR_MESSAGE_SIZE+1);

    m_period = rhs.m_period;

    m_id = rhs.m_id;
    m_fn = rhs.m_fn;
	return *this;
}

/*! \brief Compare the data contents of two tasks

    Returns true if and only if all of the data values (id, function, etc.) are the same.
*/
bool _TaskManagerTask::operator==(_TaskManagerTask& rhs) const {
    return m_id==rhs.m_id && m_fn==rhs.m_fn;
}

/*! \brief Print out information about the task
*/
#if defined(TASKMANAGER_DEBUG)
size_t _TaskManagerTask::printTo(Print& p) const {
    size_t ret = 35;
    p.print(F("    [task: "));
    ret += p.print(this->m_id);
    p.print(F(" flgs:"));
    ret += p.print(this->m_stateFlags, HEX);
    p.print(F(" rstTim:"));
    ret += p.print(this->m_restartTime);
    p.print(F(" period:"));
    ret += p.print(this->m_period);
    p.print(F(" fn:"));
    ret += p.print((int)(this->m_fn), HEX);
    p.print(F(" now: "));
    ret += p.print(millis());
    p.print(F("]\n"));
    return ret;
}
#endif
/*!	@}
*/

// ***********************************************************************
// Implementation of TaskManager
//

// Constructor and Destructor

/*!	\ingroup TaskManager
	@{
*/

TaskManager::TaskManager() {
    add(TASKMGR_NULL_TASK, nullTask);
    m_startTime = millis();

#if TM_USING_RADIO
#if (defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) 
	m_rf24 = NULL;
	m_myNodeId = 0;
	m_radioReceiverRunning = false;
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
	m_myNodeId = 0;
	m_radioReceiverRunning = false;
	m_TaskManagerMessageQueueSemaphore = xSemaphoreCreateBinary();
#endif	// which architecture
#endif // TM_USING_RADIO

}

TaskManager::~TaskManager() {
#if TM_USING_RADIO
#if (defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) 
	if(m_rf24!=NULL) delete m_rf24;
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
	vSemaphoreDelete(m_TaskManagerMessageQueueSemaphore);
#endif // architecture selection
#endif // TM_USING_RADIO
}

//
//  Add a new task
//
void TaskManager::add(tm_taskId_t taskId, void (*fn)()) {
    _TaskManagerTask newTask(taskId, fn);
    m_theTasks.push_back(newTask);
}

void TaskManager::addWaitDelay(tm_taskId_t taskId, void(*fn)(), unsigned long msDelay) {
    addWaitUntil(taskId, fn, millis() + msDelay);
}

void TaskManager::addWaitUntil(tm_taskId_t taskId, void(*fn)(), unsigned long msWhen) {
    _TaskManagerTask newTask(taskId, fn);
    newTask.setWaitUntil(msWhen);
    m_theTasks.push_back(newTask);
}

void TaskManager::addAutoWaitDelay(tm_taskId_t taskId, void(*fn)(), unsigned long period, bool startWaiting /*=false*/) {
    _TaskManagerTask newTask(taskId, fn);
    if(startWaiting) newTask.setWaitDelay(period); else newTask.m_restartTime = millis();
    newTask.setAutoDelay(period);
    m_theTasks.push_back(newTask);
}

void TaskManager::addWaitMessage(tm_taskId_t taskId, void (*fn)(), unsigned long timeout/*=0*/) {
    _TaskManagerTask newTask(taskId, fn);
    newTask.setWaitMessage(timeout);
    m_theTasks.push_back(newTask);
}

void TaskManager::addAutoWaitMessage(tm_taskId_t taskId, void (*fn)(), unsigned long timeout/*=0*/, bool startWaiting/*=true*/) {
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

void TaskManager::yieldForMessage(unsigned long timeout/*=0*/) {
    m_theTasks.front().setWaitMessage(timeout);
    longjmp(taskJmpBuf, YtYieldMessageTimeout);
}

//
// Message functions
//	Note that TaskManager uses these with nodeID of 0 (=="self") and
//	inter-node functions for TaskManagerRF and ESP uses either local or non-local nodeIDs.
//

void TaskManager::internalSendMessage(tm_nodeId_t fromNodeId, tm_taskId_t fromTaskId, tm_taskId_t taskId, char* message) {
    _TaskManagerTask* tsk;
    if(strlen(message)>TASKMGR_MESSAGE_SIZE-1) return;
    tsk = findTaskById(taskId);
    if(tsk==NULL) return;
    tsk->m_fromNodeId = fromNodeId;
    tsk->m_fromTaskId = fromTaskId;
    tsk->putMessage((void*)message, strlen(message)+1);
}

void TaskManager::internalSendMessage(tm_nodeId_t fromNodeId, tm_taskId_t fromTaskId, tm_taskId_t taskId, void* buf, int len) {
    _TaskManagerTask* tsk;
	//Serial.printf("ism: from n/t %d/%d to task %d, msg len %d\n", fromNodeId, fromTaskId, taskId, len);
    if(len>TASKMGR_MESSAGE_SIZE) return;
	//Serial.printf("len is okay");
    tsk = findTaskById(taskId);
	//Serial.printf("back from findTaskById, %s\n",tsk==NULL?"did not find":"found");
    if(tsk==NULL) return;
	//Serial.printf(" task was found\n");
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
	
    This routine is for internal use only.
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

#if defined(TASKMANAGER_DEBUG)
// printTo
/*! \brief Support routine to allow the printing of the contents of a TaskManager object.

    This prints a TaskManager object.  It should only be used for debugging purposes.
*/
size_t TaskManager::printTo(Print& p) const {
    size_t ret;
    ret = p.print(m_theTasks);
    return ret;
}
#endif

/*! \brief Find a task by its ID.  

    This routine is for internal use only.

    \param id: the ID of the task
    \return A pointer to the _TaskManagerTask or NULL if not found
*/
_TaskManagerTask* TaskManager::findTaskById(tm_taskId_t id) {
     ring<_TaskManagerTask> tmpTasks;
    _TaskManagerTask* tmt;
    _TaskManagerTask* last;
    _TaskManagerTask* theTask = NULL;
	//Serial.printf("Entering ftbId\n");

    tmpTasks = m_theTasks;
	//Serial.printf("\t1\n");
    bool found = false;
	//Serial.printf("\t2\n");
    last = &(m_theTasks.back());
	//Serial.printf("\t3\n");

    // scan for the first thing that has the given id
	//Serial.printf("ftbId: looking for %d\n", id);
    while(&(tmpTasks.front())!=last && !found) {
        tmt = &(tmpTasks.front());
		//Serial.printf("\tloop: comparing with %d\n", tmt->m_id);
        if(tmt->m_id==id) {
            found = true;
            theTask = tmt;
        }
        tmpTasks.move_next();
    }
	//Serial.printf("\tpast loop: %sfound\n", found?"":"not ");
    // check the last one if we haven't found it yet
    if(!found && last->m_id==id) {
		//Serial.printf("\tbut found it at the end.\n");
        found = true;
        theTask = last;
    }
	//if(theTask==NULL) Serial.printf("\tat end, not found\n");
	//else Serial.printf("\tat end, found\n");
    return theTask;
}

/*! \brief Implements a single pass for the system loop() routine.

    This performs a single iteration for the system loop() routine.  It finds the next
    runnable task and runs it.  It processes any yield*() operations that the user routine
    may have executed.

    This routine is for internal use only.
*/
//??delete??  #define T1	20
//??delete??  #define	T2	20

void TaskManager::loop() {
    int jmpVal; // return value from setjmp, indicates longjmp type
    _TaskManagerTask* nextTask;
    nextTask = /*TaskMgr.*/FindNextRunnable();
    // pre-stage the next startup time based on the current time.  This'll be overwritten if a Yield*(time)
    // is encountered.  It'll be ignored anyway unless we are auto-yielddelay, which is the only one
    // that focuses on the start-start measurement of the period.  (All others are end-start.)
    nextTask->m_restartTime = millis() + nextTask->m_period;
	//Serial << "About to run task " << nextTask->m_id << endl;
    //??delete??if(DEBUG && (nextTask->m_id==T1 || nextTask->m_id==T2)) Serial << "-->TaskManager::loop\n";
    if((jmpVal=setjmp(/*TaskMgr.*/taskJmpBuf))==0) {
        // this is the normal path we use to invoke and process "normal" returns
    	//??delete??if(DEBUG && (nextTask->m_id==T1 || nextTask->m_id==T2)) Serial << "about to run task " << nextTask->m_id
    	//??delete??  << " at " << millis() << " status is " << _HEX(nextTask->m_stateFlags) << endl;
        (nextTask->m_fn)();
        //??delete??if(DEBUG && (nextTask->m_id==T1 || nextTask->m_id==T2))
        //??delete??	Serial << "normal return at " << millis() << " pre-set status is " << _HEX(nextTask->m_stateFlags) << endl;
		// If we've gotten here, we got here through a normal "fall out the bottom or 'return'" return.
		// As such, we reset according to the auto bits.
        // Process auto bits
        // AutoReWaitUntil has two interpretations:  If alone, it is periodic and is
        // incremented based on the previous restartTime.  If in conjunction with
        // AutoReMessage then it is a timeout and is based on "now"
        if(nextTask->anyStateSet(_TaskManagerTask::AutoReWaitMessage)
        	&& nextTask->stateTestBit(_TaskManagerTask::AutoReWaitUntil)) {
			nextTask->setWaitUntil(millis()+nextTask->m_period);
		}
		nextTask->resetCurrentStateBits();
		//??delete??if(DEBUG && (nextTask->m_id==T1 || nextTask->m_id==T2))
		//??delete??	Serial << "post-set status is " << _HEX(nextTask->m_stateFlags) << endl;
    } else {
        // this is the path executed if a yield was called
        // Yield types (jmpVal values) are from YtYield
        //
        // AutoRestart is tricky here.  It needs to be handled separately
        // in each case.
        //
        // In each case, the yield*(...) routine will have set the appropriate flag bit(s)
        // and if needed, stuffed a m_restartTime value if a delay/timeout was specified.
        //??delete??if(DEBUG && (nextTask->m_id==T1 || nextTask->m_id==T2)) Serial << "exited with yield type " << jmpVal << endl;
        switch(jmpVal) {
            case YtYield:
                // normal yield, just exit cleanly
                // Autorestart is ignored
                break;
            case YtYieldUntil:
                // yieldUntil/yieldDelay will have marked the next completion time so exit cleanly
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
                /*TaskMgr.*/m_theTasks.pop_front();
                break;
            default:
                // ignore invalid yields
                // YOU SHOULD NOT BE GETTING HERE!!!
                break;
        }
     }
     //??delete??if(DEBUG && (nextTask->m_id==T1 || nextTask->m_id==T2)) Serial << "<--TaskManager::loop\n";
}

// Status tasks

bool TaskManager::suspend(tm_taskId_t taskId) {
    _TaskManagerTask* tsk;
    tsk = findTaskById(taskId);
    tsk->setSuspended();
	return true;
}

bool TaskManager::resume(tm_taskId_t taskId) {
    _TaskManagerTask* tsk;
    tsk = findTaskById(taskId);
    tsk->clearSuspended();
	return true;
}

//
// Network/Mesh tasks
//

#if TM_USING_RADIO && ((defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32))

bool TaskManager::sendMessage(tm_nodeId_t nodeId, tm_taskId_t taskId, char* message) {
	if(nodeId==0 || nodeId==myNodeId()) { TaskManager::sendMessage(taskId, message); return true; }
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
	Serial.printf("sendmessage: calling radiosender\n");
	return radioSender(nodeId);
}

bool TaskManager::sendMessage(tm_nodeId_t nodeId, tm_taskId_t taskId, void* buf, int len) {
	if(nodeId==0 || nodeId==myNodeId()) {
		TaskManager::sendMessage(taskId, buf, len);
		return true;
	}
	if(len>TASKMGR_MESSAGE_SIZE) {
		return false;	// reject too-long messages
	}
	radioBuf.m_cmd = tmrMessage;
	radioBuf.m_fromNodeId = myNodeId();
	radioBuf.m_fromTaskId = myId();
	radioBuf.m_data[0] = taskId;	// who we are sending it to
	memcpy(&radioBuf.m_data[1], buf, len);
	bool ret = radioSender(nodeId);
	return ret;
}

bool TaskManager::suspend(tm_nodeId_t nodeId, tm_taskId_t taskId) {
	if(nodeId==0 || nodeId==myNodeId()) { TaskManager::suspend(taskId); return true; }
	radioBuf.m_cmd = tmrSuspend;
	radioBuf.m_fromNodeId = myNodeId();
	radioBuf.m_fromTaskId = myId();
	radioBuf.m_data[0] = taskId;
	return radioSender(nodeId);
}

bool TaskManager::resume(tm_nodeId_t nodeId, tm_taskId_t taskId) {
	if(nodeId==0 || nodeId==myNodeId()) { TaskManager::resume(taskId); return true; }
	radioBuf.m_cmd = tmrResume;
	radioBuf.m_fromNodeId = myNodeId();
	radioBuf.m_fromTaskId = myId();
	radioBuf.m_data[0] = taskId;
	return radioSender(nodeId);
}

void TaskManager::getSource(tm_nodeId_t& fromNodeId, tm_taskId_t& fromTaskId) {
	fromNodeId = m_theTasks.front().m_fromNodeId;
	fromTaskId = m_theTasks.front().m_fromTaskId;
}
#endif // USING_RADIO && architecture

/*!	@}
*/

