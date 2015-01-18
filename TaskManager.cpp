#include <Arduino.h>

#define TASKMANAGER_MAIN
#include "TaskManager.h"

/*! \file TaskManager.cpp
    Implementation file for Arduino Task Manager
*/

TaskManager TaskMgr;

static void nullTask() {
}

// This is the replacement for the normal loop().
void loop() {
    TaskMgr.loop();
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
    //Serial << "isRunnable, examining " << m_id << (ret ? " is " : " is not " ) << "runnable\n";
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

/*! \brief Constructor,  Creates an empty task
*/
TaskManager::TaskManager() {
    add(0xff, nullTask);
    m_startTime = millis();
}
/*!  \brief Destructor.  Destroys the TaskManager.

    After calling this, any operations based on the object will fail.  For normal purpoases, destroying the
    TaskMgr instance will have serious consequences for the standard loop() routine.
*/
TaskManager::~TaskManager() {
}

//
//  Add a new task
//

/*!  \brief Add a simple task.

    The task will execute once each cycle through the task list.  Unless the task itself forces itself into a different scheduling
    model (e.g., through YieldSignal), it will execute again at the next available opportunity
    \param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [0 127].
    System tasks have taskId values in the range [128 255].
    \param fn -- this is a void function with no arguments.  This is the procedure that is called every time
    the task is invoked.
    \sa addWaitDelay, addWaitUntil, addAutoWaitDelay
*/
void TaskManager::add(byte taskId, void (*fn)()) {
    _TaskManagerTask newTask(taskId, fn);
    m_theTasks.push_back(newTask);
}

/*! \brief Add a task that will be delayed before its first invocation

    This task will execute once each cycle.  Its first execution will be delayed for a set time.  After this,
    unless the task forces itself into a different scheduling model (e.g., through yieldSignal), it will
    execute agaion at the next available opportunity.
    \param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [0 127].
    System tasks have taskId values in the range [128 255].
    \param fn -- this is a void function with no arguments.  This is the procedure that is called every time
    the task is invoked.
    \param msDelay -- the initial delay, in milliseconds
    \sa add, addWaitUntil, addAutoWaitDelay
    */
void TaskManager::addWaitDelay(byte taskId, void(*fn)(), unsigned int msDelay) {
    addWaitUntil(taskId, fn, millis() + msDelay);
}
/*! \brief Add a task that will be delayed until a set system clock time before its first invocation

    This task will execute once each cycle.  Its first execution will be delayed until a set system clock time.  After this,
    unless the task forces itself into a different scheduling model (e.g., through yieldSignal), it will
    execute agaion at the next available opportunity.
    \param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [0 127].
    System tasks have taskId values in the range [128 255].
    \param fn -- this is a void function with no arguments.  This is the procedure that is called every time
    the task is invoked.
    \param msWhen -- the initial delay, in milliseconds
    \sa add, addDelayed, addAutoReachedule
    */
void TaskManager::addWaitUntil(byte taskId, void(*fn)(), unsigned long msWhen) {
    _TaskManagerTask newTask(taskId, fn);
    newTask.setWaitUntil(msWhen);
    m_theTasks.push_back(newTask);
}
/*! \brief Add a task that will automatically reschedule itself with a delay

    This task will execute once each cycle.  The task will automatically reschedule itself to not execute
    until the given delay has passed. THe first execution may be delayed using the optional fourth parameter startDelayed.
    This delay, if used, will be the same as the period.

    Note that
    yielding for messages or signals may extend this delay.  However, if a signal/message is received during the
    delay period., the procedure will still wait until the end of the delay period.
    \param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [0 127].
    System tasks have taskId values in the range [128 255].
    \param fn -- this is a void function with no arguments.  This is the procedure that is called every time
    the task is invoked.
    \param period -- the schedule, in milliseconds
    \param startWaiting -- for the first execution, start immediately (false), or delay its start for one period (true)
    \sa add, addDelayed, addWaitUntil
    */
void TaskManager::addAutoWaitDelay(byte taskId, void(*fn)(), unsigned int period, bool startWaiting /*=false*/) {
    _TaskManagerTask newTask(taskId, fn);
    if(startWaiting) newTask.setWaitDelay(period);
    newTask.setAutoDelay(period);
    m_theTasks.push_back(newTask);
}

/*! \brief Add a task that will wait until a signal arrives or (optionally) a timeout occurs.

    The task will be added, but will be set to be waiting for the listed signal.  If a nonzero timeout is
    specified and ifthe signal does
    not arrive before the timeout period (in milliseconds), then the routine will be activated.  The
    routine may use TaskManager::timeout() to determine whether it timed our or received the signal.

    Note that calling addWaitSignal() with no timeout or startWaiting parmeter, the task will start in a wait
    state with no timeout.  To start the task in a non-wait-state (active) with no timeout, the routine must
    be called as addWaitSignal(id, fn, 0, false).
    \param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [0 127].
    System tasks have taskId values in the range [128 255].
    \param fn -- this is a void function with no arguments.  This is the procedure that is called every time
    the task is invoked.
    \param sigNum -- the signal the task will be waiting for
    \param timeout -- the maximum time to wait (in ms) before timing out.  The default is zero (no timeout).
    \sa addAutoWaitSignal
*/
void TaskManager::addWaitSignal(byte taskId, void(*fn)(), byte sigNum, unsigned int timeout/*=0*/){
    _TaskManagerTask newTask(taskId, fn);
    newTask.setWaitSignal(sigNum, timeout);
    m_theTasks.push_back(newTask);
}

/*! \brief Add a task that waits until a signal arrives or (optionally) timeout occurs. The task
    automatically reschedules itself

    The task will be added, but will be set to be waiting for the listed signal. If the task exits normally
    (normal return, fall out of the bottom, or yield()), it will reschedule itself to wait for the same
    signal.  If a timeout (>0) is specified and the signal does
    not arrive before the timeout period (in milliseconds), then the routine will be activated.  The
    routine may use TaskManager::timeout() to determine whether it timed our or received the signal.
    \param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [0 127].
    System tasks have taskId values in the range [128 255].
    \param fn -- this is a void function with no arguments.  This is the procedure that is called every time
    the task is invoked.
    \param sigNum -- the signal the task will be waiting for
    \param timeout -- the maximum time to wait (in ms) before timing out.
    \param startWaiting -- tells whether the routine will start waiting for the signal (true) or will execute
    immediately (false).
    \sa addAutoWaitSignal
*/
void TaskManager::addAutoWaitSignal(byte taskId, void(*fn)(), byte sigNum, unsigned int timeout/*=0*/, bool startWaiting/*=true*/) {
    _TaskManagerTask newTask(taskId, fn);
    if(startWaiting) {
        newTask.setWaitSignal(sigNum, timeout);
        if(timeout>0) newTask.setWaitUntil(millis()+timeout);
    }
    newTask.setAutoSignal(sigNum, timeout);
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
void TaskManager::addWaitMessage(byte taskId, void (*fn)(), unsigned int timeout/*=0*/) {
    _TaskManagerTask newTask(taskId, fn);
    newTask.setWaitMessage(timeout);
    m_theTasks.push_back(newTask);
}
/*! \brief Add a task that is waiting for a message or until a timeout occurs

    The task will be added, but will be set to be waiting for the listed signal.  If the signal does
    not arrive before the timeout period (in milliseconds), then the routine will be activated.  The
    routine may use TaskManager::timeout() to determine whether it timed our or received the signal.
    \param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [0 127].
    System tasks have taskId values in the range [128 255].
    \param fn -- this is a void function with no arguments.  This is the procedure that is called every time
    the task is invoked.
    \param timeout -- the maximum time to wait (in ms) before timing out.
    \param startWaiting -- tells whether the routine will start waiting for the signal (true) or will execute
    immediately (false).
    \sa addAutoWaitMessage
*/
void TaskManager::addAutoWaitMessage(byte taskId, void (*fn)(), unsigned int timeout/*=0*/, bool startWaiting/*=true*/) {
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
/*! \brief Exit from this task and return control to the task manager

    This exits from the current task, and returns control to the task manager.  Functionally, it is similar to a
    return statement.  The next time the task gains control, it will resume from the TOP of the routine.  Note that
    if the task was an Auto task, it will be automatically rescheduled according to its Auto specifications.
    \sa yieldDelay(), yieldUntil(), yieldSignal(), yieldMessage(), addAutoWaitDelay(), addAutoWaitSignal(), addAutoWaitMessage()
*/
void TaskManager::yield() {
    longjmp(taskJmpBuf, YtYield);
}
/*! \brief Exit from the task manager and do not restart this task until after a specified period.

    This exits from the current task and returns control to the task manager.  This task will not be rescheduled until
    at least the stated number of milliseconds has passed.  Note that yieldDelay _overrides_ any of the Auto
    specifications.  That is, the next rescheduling will occur _solely_ after the stated time period, and will
    not be constrained by AutoWaitSignal, AutoWaitMessage, or a different AutoWaitDelay value.  The Auto specification will
    be retained, and will be applied on future executions where yield() or a normal return are used.
    \param ms -- the delay in milliseconds.  Note the next call may exceed this constraint depending on time taken by other tasks.
    \sa yield(), yieldUntil(), yieldSignal(), yieldMessage(), addAutoWaitDelay(), addAutoWaitSignal(), addAutoWaitMessage()
*/
void TaskManager::yieldDelay(int ms) {
    yieldUntil(millis()+ms);
}
/*! \brief Exit from the task manager and do not restart this task until (after) a specified CPU clock time.

    This exits from the current task and returns control to the task manager.  This task will not be rescheduled until
    the CPU clock (millis()) has exceeded the given time.  Note that yieldUntil _overrides_ any of the Auto
    specifications.  That is, the next rescheduling will occur _solely_ after the stated clock time has passed, and will
    not be constrained by AutoWaitSignal, AutoWaitMessage, or a different AutoWaitDelay value.  The Auto specification will
    be retained, and will be applied on future executions where yield() or a normal return are used.
    \param when -- The target CPU time.  Note the next call may exceed this constraint depending on time taken by other tasks.
    \sa yield(), yieldDelay(), yieldSignal(), yieldMessage(), addAutoWaitDelay(), addAutoWaitSignal(), addAutoWaitMessage()
*/
void TaskManager::yieldUntil(unsigned long when) {
    // mark it as waiting
    m_theTasks.front().setWaitUntil(when);
    longjmp(taskJmpBuf, YtYieldUntil);
}

/*! \brief Exit from the task manager and do not restart this task until a signal has been received or a stated time period has passed.

    This exits from the current task and returns control to the task manager.  This task will not be rescheduled until
    the given signal has been received or a stated time period has passed (the timeout period). The TaskManager::timeOut()
    function will tell whether or not the timeout had been triggered.

    Note that yieldForSignal _overrides_ any of the Auto
    specifications.  That is, the next rescheduling will occur _solely_ after the signal has been received, and will
    not be constrained by AutoWaitDelay, AutoWaitMessage, or a different AutoWaitSignal value.  The Auto specification will
    be retained, and will be applied on future executions where yield() or a normal return are used.
    \param sigNum -- the value of the signal the task will wait for.
    \param timeout -- The timeout period, in milliseconds.
    \sa yield(), yieldDelay(), yieldForMessage(), addAutoWaitDelay(), addAutoWaitSignal(), addAutoWaitMessage(), timeOut()
*/
void TaskManager::yieldForSignal(byte sigNum, unsigned int timeout/*=0*/) {
    m_theTasks.front().setWaitSignal(sigNum, timeout);
    longjmp(taskJmpBuf, YtYieldSignalTimeout);
}

/*! \brief Exit from the task manager and do not restart this task until a message has been received or a stated time period has passed.

    This exits from the current task and returns control to the task manager.  This task will not be rescheduled until
    a message has been received or a stated time period has passed (the timeout period). The TaskManager::timeOut()
    function will tell whether or not the timeout had been triggered.

    Note that yieldForMessage _overrides_ any of the Auto
    specifications.  That is, the next rescheduling will occur _solely_ after the signal has been received, and will
    not be constrained by AutoWaitDelay, AutoWaitSignal, or a different AutoWaitMessage value.  The Auto specification will
    be retained, and will be applied on future executions where yield() or a normal return are used.
    \param timeout -- The timeout period, in milliseconds.
    \sa yield(), yieldDelay(), yieldForSignal(), addAutoWaitDelay(), addAutoWaitSignal(), addAutoWaitMessage(), timeOut()
*/
void TaskManager::yieldForMessage(unsigned int timeout/*=0*/) {
    m_theTasks.front().setWaitMessage(timeout);
    longjmp(taskJmpBuf, YtYieldMessageTimeout);
}
//
// Signal functions
//
/*! \brief  Sends a signal to a task

    Sends a signal to a task.  The signal will go to only one task.  If there are several tasks waiting on the signal, it will go to
    the first task found that is waiting for this particular signal.  Note that once a task is signalled, it will not be waiting for
    other instances of the same siggnal number.
    \param sigNum -- the value of the signal to be sent
    \sa yieldForSignal(), sendSignalAll(), addWaitSignal, addAutoWaitSignal()
*/
void TaskManager::sendSignal(byte sigNum) {
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
            tmt->signal();
            found = true;
        }
        tmpTasks.move_next();
    }
    if(!found && last->stateTestBit(_TaskManagerTask::WaitSignal) && last->m_sigNum==sigNum) {
        // the last task is the one we want to signal
        last->signal();
    }
}
/*! \brief Send a signal to all tasks that are waiting for this particular signal.

    Signals all tasks that are waiting for signal <i>sigNum</i>.
    \param sigNum -- the signal number to be sent
    \sa sendSignal(), yieldForSiganl(), addWaitSignal(), addAutoWaitSignal()
*/
void TaskManager::sendSignalAll(byte sigNum) {
    ring<_TaskManagerTask> tmpTasks;
    _TaskManagerTask* tmt;
    _TaskManagerTask* last;

    tmpTasks = m_theTasks;
    last = &(m_theTasks.back());

    // scan for all things that will receive the listed signal.
    while(&(tmpTasks.front())!=last) {
        tmt = &(tmpTasks.front());
        if(tmt->stateTestBit(_TaskManagerTask::WaitSignal) && tmt->m_sigNum==sigNum) {
            tmt->signal();
        }
        tmpTasks.move_next();
    }
    if(last->stateTestBit(_TaskManagerTask::WaitSignal) && last->m_sigNum==sigNum) {
        // the last task is the one we want to signal
        last->signal();
    }
}
/*! \brief  Sends a string message to a task

    Sends a message to a task.  The message will go to only one task.

    Note that once a task has been sent a message, it will not be waiting for
    other instances of the same siggnal number.
    Note that additional messages sent prior to the task executing will overwrite any prior messages.
    \param taskId -- the ID number of the task
    \param message -- the character string message.  It is restricted in length to 24 characters.  Longer messages
    will be ignored.
    \sa yieldForMessage()
*/
void TaskManager::sendMessage(byte taskId, char* message) {
    _TaskManagerTask* tsk;
    if(strlen(message)>TASKMGR_MESSAGE_SIZE) return;
    tsk = findTaskById(taskId);
    if(tsk==NULL) return;
    tsk->putMessage((void*)message, strlen(message)+1);
}
/*! \brief Send a binary message to a task

    Sends a message to a task.  The message will go to only one task.
    Note that once a task has been sent a message, it will not be waiting for
    other instances of the same siggnal number.

    Note that additional messages sent prior to the task executing will overwrite any prior messages.
    \param taskId -- the ID number of the task
    \param buf -- A pointer to the structure that is to be passed to the task
    \param len -- The length of the buffer.  Buffers over 24 bytes long will be ignored -- no message will be sent.
    \sa yieldForMessage()
*/
void TaskManager::sendMessage(byte taskId, void* buf, int len) {
    _TaskManagerTask* tsk;
    if(len>TASKMGR_MESSAGE_SIZE) return;
    tsk = findTaskById(taskId);
    if(tsk==NULL) return;
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
    //Serial << "FindNextRunnable: scanning at " << millis() << "\n";
    //Serial << TaskMgr << "\n";
    //Serial << "...before move_next, front is " << m_theTasks.front().m_id << "\n";
    m_theTasks.move_next(); // go past the current one
    //Serial << "...after move_next, front is " << m_theTasks.front().m_id << "\n";
    tmt = &(m_theTasks.front());
    while(!tmt->isRunnable()) {
        m_theTasks.move_next();
        tmt = &(m_theTasks.front());
    }
    //Serial << "... found " << tmt->m_id << "\n";
    //Serial << "...TaskMgr is " << TaskMgr << "\n";
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
        if(nextTask->stateTestBit(_TaskManagerTask::AutoReWaitUntil)) {
           nextTask->setWaitUntil(millis()+nextTask->m_period);
        }
        // Note that timeout autorestarts will have AutoReUntil and AutoReSignal/Message set
        // So to handle this, we process the bits separately and set both if needed.
        if(nextTask->stateTestBit(_TaskManagerTask::AutoReWaitSignal)) {
           nextTask->setWaitSignal(nextTask->m_restartSignal);
        } else if(nextTask->stateTestBit(_TaskManagerTask::AutoReWaitMessage)) {
           nextTask->setWaitMessage();
        }
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
