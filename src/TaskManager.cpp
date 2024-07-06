//
//	Implementation file for TaskManager 
//

//	version 2.1 2024-0701 SMP
//

//!	\ignore
#define TASKMANAGER_MAIN
//!	\endignore

#include <arduino.h>
#include <TaskManagerCore.h>
#include <TaskManagerMacros.h>
#include <Streaming.h>

//!	\ignore
#define DEBUG false
//!	\endignore



/*! \file TaskManager.cpp
    Implementation file for Arduino Task Manager
*/

/*x	\addtogroup _TaskManagerTask _TaskManagerTask
*/

/*x \addtogroup TaskManager TaskManager
*/

// Globals used to support network clock synchronization
// TmClockOffset -- the offset between this system's clock and the network server's clock
// Defined as netMillis - ::millis(), depending on the value of the network millisecond timer
// when clock resyncing is performed
// Set to 0 at the start.
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
//!	\ignore
unsigned long TmClockOffset = 0;	
//!	\endignore

/*!	\brief Return the network-adjusted time

	When network clock synchronization is used, TmMillis() is the internal routine used to 
	access the network-adjusted (network-standard) ms time.
	
	This should not be called by used tasks.  TaskMgr.millis() should be used instead.
*/
unsigned long TmMillis() {
	return ::millis() + TmClockOffset;
}

/*!	\brief Adjust the network-adjusted time

	This is an internal test routine.  It modifies the network time adjustment.  It should not be used by user tasks.
*/
void TmAdjustClockOffset(unsigned long offsetDelta) {
	TmClockOffset += offsetDelta;
}
#endif // static global network clock

/*!	\brief An empty task to be called by the TaskManager null task.

	This task does nothing, however, since TaskManager always needs something to run,
	a task referencing nullTask is created when the TaskManager object is created.
*/
static void nullTask() {
}


// **********************************************************
// *     _TASKMANAGERTASK IMPLEMENTATION					*
// **********************************************************

/*x \defgroup clocksync Cross-Node Clock Synchronization
	\ingroup _TaskManagerTask
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
	//
	// millisNow is a passed value to allow TaskManager to work with virtual/network clocks.
    bool ret;
    if(stateTestBit(Suspended)) {
		ret = false;
	} else if(stateTestBit(WaitMessage)) {
		if(stateTestBit(WaitUntil)) {
			// waiting for message or timeout, act based on timeout
			if(m_restartTime<TmMillis()) {
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
		if(m_restartTime<TmMillis()) {
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

	In a debugging environment (TaskManager debugging is enabled), this will print out information on a single task.
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
/*x	@}
*/

// ******************************************************
// *     TASKMANAGER IMPLEMENTATION					*
// ******************************************************

// Constructor and Destructor

/*x	\ingroup TaskManager
	@{
*/

/*! \brief Create a new TaskManager task control object.

	Creates an empty TaskManager control object
*/
TaskManager::TaskManager() {
    add(TASKMGR_NULL_TASK, nullTask);
    m_startTime = millis();
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
	TmClockOffset = 0;
#endif

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

/*! \brief Destroy an existing TaskManager task control object.

	Destroys a TaskManager object.  We do not expect this will ever be called, however,
	it  is  included for completeness.  For normal purpoases, destroying the
	TaskMgr instance will have serious consequences for the standard loop() routine.
*/
TaskManager::~TaskManager() {
#if TM_USING_RADIO
#if (defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) 
	if(m_rf24!=NULL) delete m_rf24;
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
	vSemaphoreDelete(m_TaskManagerMessageQueueSemaphore);
#endif // architecture selection
#endif // TM_USING_RADIO
}

/*!  \brief Add a simple task.

	The task will execute once each cycle through the task list.  Unless the task itself forces itself into a different scheduling
	model (e.g., through YieldMessage), it will execute again at the next available opportunity
	\param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [1 239].
	System tasks have taskId values in the range [240 255].
	\param fn -- this is a void function with no arguments.  This is the procedure that is called every time
	the task is invoked.
	\sa addWaitDelay, addWaitUntil, addAutoWaitDelay
*/
void TaskManager::add(tm_taskId_t taskId, void (*fn)()) {
    _TaskManagerTask newTask(taskId, fn);
    m_theTasks.push_back(newTask);
}

/*! \brief Add a task that will be delayed before its first invocation

	This task will execute once each cycle.  Its first execution will be delayed for a set time.  After this,
	unless the task forces itself into a different scheduling model (e.g., through yieldMessage), it will
	execute agaion at the next available opportunity.
	\param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [0 127].
	System tasks have taskId values in the range [128 255].
	\param fn -- this is a void function with no arguments.  This is the procedure that is called every time
	the task is invoked.
	\param msDelay -- the initial delay, in milliseconds
	\sa add, addWaitUntil, addAutoWaitDelay
*/
void TaskManager::addWaitDelay(tm_taskId_t taskId, void(*fn)(), unsigned long msDelay) {
    addWaitUntil(taskId, fn, millis() + msDelay);
}

/*! \brief Add a task that will be delayed until a set system clock time before its first invocation

	This task will execute once each cycle.  Its first execution will be delayed until a set system clock time.  After this,
	unless the task forces itself into a different scheduling model (e.g., through yieldMessage), it will
	execute agaion at the next available opportunity.
	\param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [0 127].
	System tasks have taskId values in the range [128 255].
	\param fn -- this is a void function with no arguments.  This is the procedure that is called every time
	the task is invoked.
	\param msWhen -- the initial delay, in milliseconds
	\sa add, addWaitDelay, addAutoWaitDelay
*/
void TaskManager::addWaitUntil(tm_taskId_t taskId, void(*fn)(), unsigned long msWhen) {
    _TaskManagerTask newTask(taskId, fn);
    newTask.setWaitUntil(msWhen);
    m_theTasks.push_back(newTask);
}

/*! \brief Add a task that will automatically reschedule itself with a delay

	This task will execute once each cycle.  The task will automatically reschedule itself to not execute
	until the given delay has passed. The first execution may be delayed using the optional fourth parameter startDelayed.
	This delay, if used, will be the same as the period.

	Note that
	yielding for messages may extend this delay.  However, if a message is received during the
	delay period., the procedure will still wait until the end of the delay period.
	\param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [0 127].
	System tasks have taskId values in the range [128 255].
	\param fn -- this is a void function with no arguments.  This is the procedure that is called every time
	the task is invoked.
	\param period -- the schedule, in milliseconds
	\param startWaiting -- for the first execution, start immediately (false), or delay its start for one period (true)
	\sa add, addDelayed, addWaitUntil
*/
void TaskManager::addAutoWaitDelay(tm_taskId_t taskId, void(*fn)(), unsigned long period, bool startWaiting /*=false*/) {
    _TaskManagerTask newTask(taskId, fn);
    if(startWaiting) newTask.setWaitDelay(period); else newTask.m_restartTime = millis();
    newTask.setAutoDelay(period);
    m_theTasks.push_back(newTask);
}

/*! \brief Add a task that is waiting for a message

	The task will be added, but will be waiting for a message.
	\param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [0 127].
	System tasks have taskId values in the range [128 255].
	\param fn -- this is a void function with no arguments.  This is the procedure that is called every time
	the task is invoked.
	\param timeout -- the maximum time to wait (in ms) before timing out.
*/
void TaskManager::addWaitMessage(tm_taskId_t taskId, void (*fn)(), unsigned long timeout/*=0*/) {
    _TaskManagerTask newTask(taskId, fn);
    newTask.setWaitMessage(timeout);
    m_theTasks.push_back(newTask);
}

/*! \brief Add a task that is waiting for a message or until a timeout occurs

	The task will be added, but will be set to be waiting for a message.  If the message does
	not arrive before the timeout period (in milliseconds), then the routine will be activated.  The
	routine may use TaskManager::
	() to determine whether it timed our or received a message.
	\param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [0 127].
	System tasks have taskId values in the range [128 255].
	\param fn -- this is a void function with no arguments.  This is the procedure that is called every time
	the task is invoked.
	\param timeout -- the maximum time to wait (in ms) before timing out.
	\param startWaiting -- tells whether the routine will start waiting for a message (true) or will execute
	immediately (false).
	\sa addWaitMessage
*/
void TaskManager::addAutoWaitMessage(tm_taskId_t taskId, void (*fn)(), unsigned long timeout/*=0*/, bool startWaiting/*=true*/) {
    _TaskManagerTask newTask(taskId, fn);
    if(startWaiting) {
        newTask.setWaitMessage(timeout);
        if(timeout>0) newTask.setWaitUntil(millis()+timeout);
    }
    newTask.setAutoMessage(timeout);
    m_theTasks.push_back(newTask);
}

/*! \brief Exit from this task and return control to the task manager

	This exits from the current task, and returns control to the task manager.  Functionally, it is similar to a
	return statement.  The next time the task gains control, it will resume from the TOP of the routine.  Note that
	if the task was an Auto task, it will be automatically rescheduled according to its Auto specifications.
	\sa yieldDelay(), yieldUntil(), yieldMessage(), addAutoWaitDelay(), addAutoWaitMessage()
*/
void TaskManager::yield() {
    longjmp(taskJmpBuf, YtYield);
}

/*! \brief Exit from the task manager and do not restart this task until after a specified period.

	This exits from the current task and returns control to the task manager.  This task will not be rescheduled until
	at least the stated number of milliseconds has passed.  Note that yieldDelay _overrides_ any of the Auto
	specifications.  That is, the next rescheduling will occur _solely_ after the stated time period, and will
	not be constrained by AutoWaitMessage, or a different AutoWaitDelay value.  The Auto specification will
	be retained, and will be applied on future executions where yield() or a normal return are used.
	\param ms -- the delay in milliseconds.  Note the next call may exceed this constraint depending on time taken by other tasks.
	\sa yield(), yieldUntil(), yieldMessage(), addAutoWaitDelay(), addAutoWaitMessage()
*/
void TaskManager::yieldDelay(unsigned long ms) {
    yieldUntil(millis()+ms);
}

/*! \brief Exit from the task manager and do not restart this task until (after) a specified CPU clock time.

	This exits from the current task and returns control to the task manager.  This task will not be rescheduled until
	the CPU clock (millis()) has exceeded the given time.  Note that yieldUntil _overrides_ any of the Auto
	specifications.  That is, the next rescheduling will occur _solely_ after the stated clock time has passed, and will
	not be constrained by AutoWaitMessage, or a different AutoWaitDelay value.  The Auto specification will
	be retained, and will be applied on future executions where yield() or a normal return are used.
	\param when -- The target CPU time.  Note the next call may exceed this constraint depending on time taken by other tasks.
	\sa yield(), yieldDelay(), yieldMessage(), addAutoWaitDelay(), addAutoWaitMessage()
*/
void TaskManager::yieldUntil(unsigned long when) {
    // mark it as waiting
    m_theTasks.front().setWaitUntil(when);
    longjmp(taskJmpBuf, YtYieldUntil);
}

/*! \brief Exit from the task manager and do not restart this task until a message has been received or a stated time period has passed.

	This exits from the current task and returns control to the task manager.  This task will not be rescheduled until
	a message has been received or a stated time period has passed (the timeout period). The TaskManager::timeOut()
	function will tell whether or not the timeout had been triggered.

	\note The yieldForMessage call _overrides_ any of the Auto
	specifications.  That is, the next rescheduling will occur _solely_ after the message has been received, and will
	not be constrained by AutoWaitDelay, or a different AutoWaitMessage value.  The Auto specification will
	be retained, and will be applied on future executions where yield() or a normal return are used.
	\param timeout -- The timeout period, in milliseconds.
	\sa yield(), yieldDelay(), addAutoWaitDelay(), addAutoWaitMessage(), timeOut()
*/
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
	
	/param millisNow -- the unsigned long time to use to check for waitUntil events.
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
void TaskManager::loop() {
    int jmpVal; // return value from setjmp, indicates longjmp type
    _TaskManagerTask* nextTask;
    nextTask = /*TaskMgr.*/FindNextRunnable();
    // pre-stage the next startup time based on the current time.  This'll be overwritten if a Yield*(time)
    // is encountered.  It'll be ignored anyway unless we are auto-yielddelay, which is the only one
    // that focuses on the start-start measurement of the period.  (All others are end-start.)
    nextTask->m_restartTime = millis() + nextTask->m_period;
	//Serial << "About to run task " << nextTask->m_id << endl;
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
/*!	\brief Suspend the given task on this node.

	The given task will be suspended until it is resumed.  It will not be allowed to run, nor will it receive
	messages.
	\param taskId The task to be suspended
	\returns true if the task could be suspended, false otherwise
	\sa receive
*/
bool TaskManager::suspend(tm_taskId_t taskId) {
    _TaskManagerTask* tsk;
    tsk = findTaskById(taskId);
    tsk->setSuspended();
	return true;
}

/*!	\brief Resume the given task on this node

	Resumes a task.  If the task
	do not exist, nothing happens.  If the task had not been suspended, nothing happens.
	\param taskId The task to be resumed
	\returns true if the task could be resumed, false otherwise

	\note Not implemented.
	\sa suspend()
*/
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
/*! \brief  Sends a string message to a task on a different node

	Sends a message to a task.  The message will go to only one task.

	Note that once a task has been sent a message, it will not be waiting for
	other instances of the same siggnal number.
	Note that additional messages sent prior to the task executing will overwrite any prior messages.
	Messages that are too large are ignored.  Remember to account for the trailing '\n'
	when considering the string message size.

	\note This routine is only available on ESP and RF24-enabled AVR environments.

	\param nodeId -- the node the message is sent to
	\param taskId -- the ID number of the task
	\param message -- the character string message.  It is restricted in length to
	TASKMGR_MESSAGE_LENGTH-1 characters.
	\sa yieldForMessage()
*/
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
	return radioSender(nodeId);
}

/*! \brief Send a binary message to a task on a different node

	Sends a message to a task.  The message will go to only one task.
	Messages that are too large are ignored.

	\note Additional messages sent prior to the task executing will overwrite any prior messages.
	
	\note This routine is only available on ESP and RF24-enabled AVR environments.
	
	\param nodeId -- the node the message is sent to
	\param taskId -- the ID number of the task
	\param buf -- A pointer to the structure that is to be passed to the task
	\param len -- The length of the buffer.  Buffers can be at most TASKMGR_MESSAGE_LENGTH
	bytes long.
	\sa yieldForMessage()
*/
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

/*!	\brief Suspend the given task on the given node

	Suspends a task on any node.  If nodeID==0, it suspends a task on this node. If the node or task
	do not exist, nothing happens.  If the task was already suspended, it remains suspended.
	
	\param nodeId The node containing the task
	\param taskId The task to be suspended
	\returns true if the task could be suspended, false otherwise

	\note Not implemented.
	\note This routine is only available on ESP and RF24-enabled AVR environments.
	\sa resume()
*/
bool TaskManager::suspend(tm_nodeId_t nodeId, tm_taskId_t taskId) {
	if(nodeId==0 || nodeId==myNodeId()) { TaskManager::suspend(taskId); return true; }
	radioBuf.m_cmd = tmrSuspend;
	radioBuf.m_fromNodeId = myNodeId();
	radioBuf.m_fromTaskId = myId();
	radioBuf.m_data[0] = taskId;
	return radioSender(nodeId);
}

/*!	\brief Resume the given task on the given node

	Resumes a task on any node.  If nodeID==0, it resumes a task on this node.  If the node or task
	do not exist, nothing happens.  If the task had not been suspended, nothing happens.
	\param nodeId The node containnig the task
	\param taskId The task to be resumed
	\note Not implemented.
	\returns true if the task could be suspended, false otherwise
	\note This routine is only available on ESP and RF24-enabled AVR environments.
	\sa suspend()
*/
bool TaskManager::resume(tm_nodeId_t nodeId, tm_taskId_t taskId) {
	if(nodeId==0 || nodeId==myNodeId()) { TaskManager::resume(taskId); return true; }
	radioBuf.m_cmd = tmrResume;
	radioBuf.m_fromNodeId = myNodeId();
	radioBuf.m_fromTaskId = myId();
	radioBuf.m_data[0] = taskId;
	return radioSender(nodeId);
}

/*!	\brief Get source node/task ID of last message

	Returns the nodeId and taskId of the node/task that last sent a message
	to the current task.  If the current task has never received a message, returns [0 0].
	If the last message was from "this" node, returns fromNodeId=0.

	\note This routine is only available on ESP and RF24-enabled AVR environments.

	\param[out] fromNodeId -- the nodeId that sent the last message
	\param[out] fromTaskId -- the taskId that sent the last message
*/
void TaskManager::getSource(tm_nodeId_t& fromNodeId, tm_taskId_t& fromTaskId) {
	fromNodeId = m_theTasks.front().m_fromNodeId;
	fromTaskId = m_theTasks.front().m_fromTaskId;
}
#endif // USING_RADIO && architecture

// Internals for network clock resyncing
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
void TaskManager::resync(unsigned long int remoteMillis) {
	// offsetDelta is the difference between the old offset and the new offset.
	// It is how far the server clock is ahead of the local clock.  So we add this value to
	// ::millis to get the network millis().
	// It is used to update all m_restartTime values on the task ring, so they will
	// be in sync with the new clock after the update
	// Synchronization will be m_restartTime += offsetDelta
	unsigned long int oldOffset, offsetDelta;
	// calc oldOffset and offsetDelta; calc new TmClockOffset
	oldOffset = TmClockOffset;
	TmClockOffset = remoteMillis - ::millis();
	offsetDelta = TmClockOffset - oldOffset;
	// now update the things on the task ring
	// Step through all of the  tasks.  Anything that has stateTestBit(AutoReWaitUntil) 
	// will have its m_restartTime adjusted
	ring<_TaskManagerTask> tmpTasks;
	_TaskManagerTask* tmt;
	_TaskManagerTask* last;
	_TaskManagerTask* theTask;
	if(m_theTasks.isNull()) return;
	tmpTasks = m_theTasks;
	last = &(m_theTasks.back());
	//Serial.printf("\tfirst task is %d, last task is %d\n", int(tmt->m_id), 0/*int(last->m_id)*/);
	// do all of the tasks up to [last]
	while(&(tmpTasks.front())!=last) {
		tmt = &(tmpTasks.front());
		if(tmt->stateTestBit(_TaskManagerTask::WaitUntil) || tmt->m_id==TASKMGR_CLOCK_SYNC_CLIENT_TASK) {
			tmt->m_restartTime += offsetDelta;
		} else {
		}
		tmpTasks.move_next();
	}
	// do [last]
	if(last->stateTestBit(_TaskManagerTask::WaitUntil) /*|| last->m_id==TASKMGR_CLOCK_SYNC_CLIENT_TASK*/ ) {
		last->m_restartTime += offsetDelta;
	} else {
	}
}

unsigned long TaskManager::millis() const {
		return ::millis() + TmClockOffset;
	}
#endif // ESP resync


/*x	@}  end ingroup TaskManager
*/


