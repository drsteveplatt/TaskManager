#ifndef TASKMANAGER_H_INCLUDED
#define TASKMANAGER_H_INCLUDED

#include "ring.h"
#include <setjmp.h>



/*! \file TaskManager.h
    Header for Arduino TaskManager library
*/

/*! \def TASKMGR_MESSAGE_SIZE
    The maximum size of a message passed between tasks

    This defines the maximum size for an inter-task message.  It is constrained, in part,
    by plans for RFI communication between tasks running on different devices.
*/
#define TASKMGR_MESSAGE_SIZE 30

//  Radio code -- ensure definition
#if !defined(TASKMANAGER_RADIO)
#define TASKMANAGER_RADIO 0
#endif



/*! \class _TaskManagerTask
    \brief Internal class to manage a single active task

    This class is used by the TaskManager class to manage a single task.
    It should not be used by the end-user.
*/
class TaskManager;

class _TaskManagerTask: public Printable {
    friend class TaskManager;
protected:
    /*! \enum TaskStates
        \brief Defines the various flag-bits describing the state of a task
    */
    enum TaskStates {WaitSignal=0x01,   //!< Task is waiting for a signal.
        WaitMessage=0x02,               //!< Task is waiting for a message.
        WaitUntil=0x04,                 //!< Task is waiting until a time has passed.
        AutoReWaitSignal = 0x08,        //!< After normal completion, task will be set to wait for a signal.
        AutoReWaitMessage = 0x10,       //!< After normal completion, task will be set to wait for a message.
        AutoReWaitUntil = 0x20,         //!< After normal completion, task will be set to wait until a time has passed.
        TimedOut = 0x40,                //!< Marker that the task had a timeout with a signal/message, and the timeout happened.
        Suspended=0x80                  //!< Task is suspended and will not receive messages, signals, or timeouts.
        };

protected:
    byte m_stateFlags; //!< The task's state information

    // Active delay information.  If a task is waiting, here is the reason (or in the
    // case of messaging, the response)
    unsigned long m_restartTime;  //!< Used by WaitUntil to determine the restart time.
                                    //!< This is compared to the absolute ms clock maintained by the processor
    byte m_sigNum;      //!< The signal this task is waiting for (if waiting for a signal).
    char m_message[TASKMGR_MESSAGE_SIZE];   //!<  The message sent to this task (if waiting for a message)
    unsigned int m_reTimeout;   //!< The timeout to use during auto restarts.  0 means no timeout.

    // Autorestart information.  If a task has autorestart, here is the information to use at the restart
    unsigned long m_period; //!< If it is auto-reschedule, the rescheduling period
    byte m_restartSignal;   //!< If it is auto-reseschedule, the signal to wait for upon rescheduling

    byte    m_id; //!< The task ID
    void    (*m_fn)(); //!< The procedure to be invoked each cycle

public:
    // Constructors and destructors
    _TaskManagerTask();
    _TaskManagerTask(byte id, void (*fn)());
    ~_TaskManagerTask();

protected:
    // State bit manipulation methods
    bool anyStateSet(byte);
    bool allStateSet(byte);
    bool stateTestBit(byte);
    void stateSet(byte);
    void stateClear(byte bits);

    // Querying its state and other info
    bool isRunnable();

    // Setting its state
    void setRunnable();
    void setWaitUntil(unsigned long);
    void setWaitDelay(unsigned int);
    void setAutoDelay(unsigned int);
    void setWaitSignal(byte, unsigned int msTimeout=0);
    void setAutoSignal(byte, unsigned int msTimeout=0);
    void setWaitMessage(unsigned int msTimeout=0);
    void setAutoMessage(unsigned int msTimeout=0);

    // How to receive a signal/message
    void signal();
    void putMessage(void* buf, int len);

public:
    // Things that make the ring of _TaskManagerTask work
    _TaskManagerTask& operator=(_TaskManagerTask& rhs);
    bool operator==(_TaskManagerTask& rhs) const;

    // Miscellaneous methods
    size_t printTo(Print& p) const;
};

/*! \class TaskManager
    \brief Manages a set of cooperative tasks

    Manages a set of cooperative tasks.  This includes round-robin scheduling, yielding, and inter-task
    messaging and signaling.  It also replaces the loop() function in standard Arduino programs.  Nominally,
    there is a single instance of TaskManager called TaskMgr.  TaskMgr is used for all actual task control.

    Each task has a taskID.  By convention, user tasks' taskID values are in the range [0 127].
*/
class TaskManager: public Printable {

public:
    ring<_TaskManagerTask> m_theTasks; //!< The ring of all tasks.  For internal use only.
private:
    unsigned long m_startTime;  //!< Start clock time.  Used to calcualte runtime. For internal use only.
#if TASKMANAGER_RADIO>0
#if TASJNABAGER_RADIO==1
	Radio*	m_rf24;
#endif
#endif

public:
    TaskManager();
    ~TaskManager();

    // Things used by yield
private:
    /*! \enum YieldTypes
        \brief Defines the different methods a process may yield control.

        This is the return value passsed back by the different yield*() routines to the control `loop()`.
        It indicates which form of yield*() was called.

        This is for internal use only.
    */
    enum YieldTypes { YtYield,  //!<  Normal yield()
        YtYieldUntil,           //!<  yieldUntil() or yieldDelay()
        YtYieldSignal,          //!<  yieldSignal() with no specified timeout
        YtYieldSignalTimeout,   //!<  yieldSignal() with a specified timeout
        YtYieldMessage,         //!<  yieldMessage()
        YtYieldMessageTimeout,  //!<  yieldMessage() with a specified timeout
        YtYieldSuspend,         //!<  suspend()
        YtYieldKill 			//!<  killMe()
        };
    jmp_buf  taskJmpBuf;    //!< Jump buffer used by yield.  For internal use only.

public:
    // Add a new task
    void add(byte, void (*)());
    void addWaitDelay(byte, void(*)(), unsigned int);      // delay so many ms
    void addWaitUntil(byte, void(*)(), unsigned long);  // delay until the clock reaches a time
    void addAutoWaitDelay(byte, void(*)(), unsigned int, bool startDelayed=false);
    void addWaitSignal(byte, void(*)(), byte, unsigned int timeout=0);
    void addAutoWaitSignal(byte, void(*)(), byte, unsigned int timeout=0, bool startWaiting=true);
    void addWaitMessage(byte, void(*)(), unsigned int timeout=0);
    void addAutoWaitMessage(byte, void(*)(), unsigned int timeout=0, bool startWaiting=true);

    // Yield
    void yield();
    void yieldDelay(int);
    void yieldUntil(unsigned long);
    void yieldForSignal(byte, unsigned int timeout=0);
    void yieldForMessage(unsigned int timeout=0);

    // info about this run
    bool timedOut();
    void* getMessage();
    byte myID();

    // Signals
    void sendSignal(byte);
    void sendSignalAll(byte);
    void sendMessage(byte, char*);       // string
    void sendMessage(byte, void*, int);  // arbitrary buffer
#if TASKMANAGER_RADIO>0
    void sendSignal(byte, byte);			//	remote, sigID
    void sendSignalAll(byte, byte);			//	remote, sigID
    void sendMessage(byte, byte, char*);	//	remote, taskID, msg text
    void sendMessage(byte, byte, void*, int);	// remote, taskID, data, len)
#endif

private:
    // Find the next active task
    // Note: there will always be a runnable task (tne null task) on the list.
    _TaskManagerTask* FindNextRunnable();

    // Manage tasks

    // Misc
public:
    unsigned long runtime() const;
    size_t printTo(Print& p) const;
    void loop();

    // internal utility
public:
    _TaskManagerTask* findTaskById(byte id);

    // radio
#if TASKMANAGER_RADIO>0
public:
	void radioReceiverTask();
#if TASKMANAGER_RADIO==TASKMANAGER_RADIO_RF24
	void radioBegin();
#endif
#endif
};

//
// Defining our global TaskMgr
//
#if !defined(TASKMANAGER_MAIN)
extern TaskManager TaskMgr;
#else
TaskManager TaskMgr;
#endif

//
// Inline stuff
//

// _TaskManagerTask Things

// Constructor and destructor
/*! \brief Construct an empty _TaskManager task object.

    By default, the taskId is set to 0 and the routine is NULL.  This will not run, and the routine must be
    set prior to invoking the main loop.
*/
inline _TaskManagerTask::_TaskManagerTask(): m_id(0), m_fn(NULL), m_stateFlags(0){
}
/*! \brief COnstruct a _TaskManager task object with given parameters
    \param taskId: The taskId.  User tasks in the range [0 127], system tasks [128 255].  Does not have to be unique.
    \param fn: The routine that is called to perform the process.
*/
inline _TaskManagerTask::_TaskManagerTask(byte taskId, void (*fn)()): m_id(taskId), m_fn(fn), m_stateFlags(0) {
}
/*! \brief Standard destructor.
*/
inline _TaskManagerTask::~_TaskManagerTask() {
}

// Setting the task's state
/*! \brief Set the task to a runnable state.  That is, it will not be suspended or waiting for anything
*/
inline void _TaskManagerTask::setRunnable() {
    stateClear(WaitSignal+WaitMessage+WaitUntil+Suspended);
}
/*! \brief Setting the task to wait until a specific time

    This sets the wakeup time and sets the appropriate flag to wait on the clock.  The task ill not
    reactive until after the CPU clock has passed the given time.
    \param when -- the CPU clock time to wait until/after, in ms.
    \sa setWaitUntil
*/
inline void _TaskManagerTask::setWaitUntil(unsigned long when) {
    m_restartTime=when;
    stateSet(WaitUntil);
}
/*! \brief Setting the task to wait for a specific number of milliseconds

    Thsi sets the task so it will not awaken until at least the specified number of miliseconds have passed.
    \param ms -- The number of milliseconds to wait for.
    \sa setWaitUntil
*/
inline void _TaskManagerTask::setWaitDelay(unsigned int ms)  {
    setWaitUntil(millis()+ms);
}
/*! \brief Sets the task to automatically restart with the given delay whenever it exits normally (non-yield).

    Sets the task so it will automatically restart with the given delay whenever it exits through either
    a return or by dropping through the bottom of the routine.  Using any form of yield() will override
    the auto-restart.  If the auto-restart runs, the next execution will be delayed by the
    given number of milliseconds.

    Note that this will not set any delay for the first invocation of the task.  That will need to be done
    separately.  Note that this can be combined with setAutoMessage or setAutoSignal to create a task
    that will auto-restart with a wait-for-signal/message-or-timeout.
    \param ms -- The number of milliseconds that must pass before the task restarts.  A value of zero
    will cause the auto-delay to be ignored.
    \sa setWaitDelay, setAutoSignal, setAutoMessage
*/
inline void _TaskManagerTask::setAutoDelay(unsigned int ms) {
    if(ms>0) {
        m_period = ms;
        stateSet(AutoReWaitUntil);
    }
}
/*! \brief Set the task to wait for a signal, with an optional timeout

    Sets the task to wait for a signal.  If a timeout (in ms) is specified and is greater than 0, the task will
    reactivate after that time if the signal has not been received.  If no timeout (or a zero timeout) was specified,
    the task will wait forever until the signal is received.
    \param sigNum -- The signal number to wait for.
    \param msTimeout -- The timeout on the signal.  Optional, default is zero.  If zero, then there is no timeout.
*/
inline void _TaskManagerTask::setWaitSignal(byte sigNum, unsigned int msTimeout/*=0*/) {
    m_sigNum = sigNum;
    stateSet(WaitSignal);
    if(msTimeout>0)setWaitDelay(msTimeout);
}
/*! \brief Sets the task to automatically restart waiting on the given signal
    whenever it exits normally (non-yield).

    Sets the task so it will automatically restart waiting on the given signal whenever it exits through either
    a return or by dropping through the bottom of the routine.  Using any form of yield() will override
    the auto-restart.  If the auto-restart runs, the next execution will not happen until the given signal arrives

    Note that this will not set the task as being active or in a wait-state
    for the first invocation of the task.  That will need to be done
    separately.

    Note that this can be combined with setAutoDelay to create a task
    that will auto-restart with a wait-for-signal-or-timeout.
    \param sigNum -- the signal to wait for.
    \param msTimeout -- The optional timeout.  Zero means no timeout.
    \sa setWaitDelay, setAutoSignal, setAutoMessage
*/
inline void _TaskManagerTask::setAutoSignal(byte sigNum, unsigned int msTimeout/*=0*/)  {
    m_restartSignal = sigNum;
    stateSet(AutoReWaitSignal);
    setAutoDelay(msTimeout);
}
/*! \brief Set the task to wait for a message, with an optional timeout.

    Sets the task to wait for a message.  If a timeout (in ms) is specified and is greater than 0, the task will
    reactivate after that time if a message has not been received.  If no timeout (or a zero timeout) was specified,
    the task will wait forever until the message is received.
    \param msTimeout -- The timeout on the message.  Optional, default is zero.  If zero, then there is no timeout.
*/
inline void _TaskManagerTask::setWaitMessage(unsigned int msTimeout/*=0*/)  {
    stateSet(WaitMessage);
    if(msTimeout>0) setWaitDelay(msTimeout);
}
/*! \brief Sets the task to automatically restart waiting for a message
    whenever it exits normally (non-yield).

    Sets the task so it will automatically restart waiting for a message whenever it exits through either
    a return or by dropping through the bottom of the routine.  Using any form of yield() will override
    the auto-restart.  If the auto-restart runs, the next execution will not happen until a message arrives.

    Note that this will not set the task as being active or in a wait-state
    for the first invocation of the task.  That will need to be done
    separately.

    Note that this can be combined with setAutoDelay to create a task
    that will auto-restart with a wait-for-signal-or-timeout.
    \param msTimeout -- The timeout on th emssage.  Optional, defau
    \sa setWaitDelay, setAutoSignal, setAutoMessage
*/
inline void _TaskManagerTask::setAutoMessage(unsigned int msTimeout/*=0*/)  {
    stateSet(AutoReWaitMessage);
    setAutoDelay(msTimeout);
}

//
// Sending signals or messages to a task
//
/*! \brief Tell the task that it has been signalled.

    Tells the task that it has been signalled.  Presuably, the caller has checked the m_sigNum to
    ensure that it _should_ be signalled.  The timeout flag is cleared.
*/
inline void _TaskManagerTask::signal() {
    stateClear(WaitSignal + WaitUntil + TimedOut);
}
/*! \brief Send an actual message to the task

    Sends a message to the task.  The timeout and other flags are cleared.
    Buffers up to TASKMGR_MESSAGE_SIZE are supported; others are ignored.
    This is the only form; sending a string will need to call it with the
    string address and length+1 (null byte at end...).
*/
inline void _TaskManagerTask::putMessage(void* buf, int len) {
    if(len<=TASKMGR_MESSAGE_SIZE) {
        memcpy(m_message, buf, len);
        stateClear(WaitMessage+WaitUntil+TimedOut);
    }
}

//
// Testing the status bits of the task
//
/*! \brief Tells if any of the specified bits are set (1)
    \param bits -- The bits to test
    \return false if all of the bits are clear (0), true if any of the bits are set (1)
*/
inline bool _TaskManagerTask::anyStateSet(byte bits) {
    return (m_stateFlags&bits) != 0;
}
/*! \brief Tell if all of the flag bits are set
    \param bits -- The bits to test
    \return true if all of the bits are set (1), false if any are not set (0)
*/
inline bool _TaskManagerTask::allStateSet(byte bits) {
    return (m_stateFlags&bits) == bits;
}
/*! \brief Test a single bit
    \param  bit -- bit to test
    \return true if the bit is set(1), false if it is not set (0)
*/
inline bool _TaskManagerTask::stateTestBit(byte bit) {
    return anyStateSet(bit);
}
/*! \brief Set a bit or bits
    \param bits -- the bits to set
*/
inline void _TaskManagerTask::stateSet(byte bits) {
    m_stateFlags |= bits;
}
/*! \brief Clear a bit or bits
    \param bits - the bits to clear
*/
inline void _TaskManagerTask::stateClear(byte bits) {
    m_stateFlags &= (~bits);
}

/*! \brief Return the task ID of the currently running task
	\return The byte value that represents the current task's ID.
*/
inline byte TaskManager::myID() { return m_theTasks.front().m_id; };

/*! \brief Tell if the current task has timed out
*/
inline bool TaskManager::timedOut() {
    return m_theTasks.front().stateTestBit(_TaskManagerTask::TimedOut);
}

/*! \brief Get a task's message buffer
    \return A pointer to the actual message buffer.  Use the contents of the buffer but do NOT modify it.
    If a task is killed, this pointer becomes invalid.
*/
inline void* TaskManager::getMessage() {
    return (void*)(&(m_theTasks.front().m_message));
}
/*! \brief Return the time since the start of the run, in milliseconds
*/
inline unsigned long TaskManager::runtime() const { return millis()-m_startTime; }
#endif // TASKMANAGER_H_INCLUDED
