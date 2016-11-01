#ifndef TASKMANAGER_H_INCLUDED
#define TASKMANAGER_H_INCLUDED


#include "Streaming.h"
#include "ring.h"
#include <setjmp.h>

/*! \file TaskManager.h
    Header for Arduino TaskManager library
*/


/*! \def TASKMGR_MESSAGE_SIZE
    The maximum size of a message passed between tasks

    This defines the maximum size for an inter-task message.  It is constrained, in part,
    by plans for RFI communication between tasks running on different devices.

    Note that the NRF24's max payload size is 32.  Message overhead is 3 bytes.
*/

#define TASKMGR_MESSAGE_SIZE (32-3)


/*!	\struct	_TaskManagerRadioPacket
	A packet of information being sent by radio between two TaskManager nodes
*/
struct _TaskManagerRadioPacket {
	byte	m_cmd;							//!< Command information
	byte	m_fromNodeId;						// source node
	byte	m_fromTaskId;						// source task
	byte	m_data[TASKMGR_MESSAGE_SIZE];	//! The data being transmitted.
};

/*! \class _TaskManagerTask
    \brief Internal class to manage a single active task

    This class is used by the TaskManager class to manage a single task.
    It should not be used by the end-user.
*/
class TaskManager;

class _TaskManagerTask: public Printable {
    friend class TaskManager;
protected:
	/*! \name Public Member Constants and Fields
	*/
	/* \@ */
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
	/* \} */
protected:
    byte m_stateFlags; //!< The task's state information

    // Active delay information.  If a task is waiting, here is the reason (or in the
    // case of messaging, the response)
    unsigned long m_restartTime;  //!< Used by WaitUntil to determine the restart time.
                                    //!< This is compared to the absolute ms clock maintained by the processor
    byte m_sigNum;      //!< The signal this task is waiting for (if waiting for a signal).

	byte	m_fromNodeId;		// where the signal/message came from
	byte	m_fromTaskId;

    char m_message[TASKMGR_MESSAGE_SIZE];   //!<  The message sent to this task (if waiting for a message)

    unsigned int m_reTimeout;   //!< The timeout to use during auto restarts.  0 means no timeout.

    // Autorestart information.  If a task has autorestart, here is the information to use at the restart
    unsigned long m_period; //!< If it is auto-reschedule, the rescheduling period
    byte m_restartSignal;   //!< If it is auto-reseschedule, the signal to wait for upon rescheduling

    byte    m_id; //!< The task ID
    void    (*m_fn)(); //!< The procedure to be invoked each cycle

public:
	/*!	\name	Constructors and Destructor
	*/
	/*! \( */
    // Constructors and destructors
    _TaskManagerTask();
    _TaskManagerTask(byte id, void (*fn)());
    ~_TaskManagerTask();
	/*! \) */
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

/**********************************************************************************************************/

/*! \class TaskManager
    \brief A cooperative multitasking manager

    Manages a set of cooperative tasks.  This includes round-robin scheduling, yielding, and inter-task
    messaging and signaling.  It also replaces the loop() function in standard Arduino programs.  Nominally,
    there is a single instance of TaskManager called TaskMgr.  TaskMgr is used for all actual task control.

    Each task has a taskID.  By convention, user tasks' taskID values are in the range [0 127].
*/
class TaskManager: public Printable {

public:
    ring<_TaskManagerTask> m_theTasks; //!< The ring of all tasks.  For internal use only.

private:
    unsigned long m_startTime;  // Start clock time.  Used to calcualte runtime. For internal use only.
	RF24*	m_rf24;				// Our radio (dynamically allocated)
	int		m_myNodeId;			// radio node number. 0 if radio not enabled.
	//byte	m_radioFromNode;	// if receiving something by radio, where it came from
	//byte	m_radioFromTask;
public:
	// Constructor and destructor
	// Not included in doxygen documentation because the
	// user never constructs or destructs a TaskManager object
	/*! \brief Constructor,  Creates an empty task
	*/
    TaskManager();
    /*!  \brief Destructor.  Destroys the TaskManager.

	    After calling this, any operations based on the object will fail.  For normal purpoases, destroying the
	    TaskMgr instance will have serious consequences for the standard loop() routine.
	*/
    ~TaskManager();


    // Things used by yield
private:
    /* Defines the different methods a process may yield control.

        This is the return value passsed back by the different yield*() routines to the control `loop()`.
        It indicates which form of yield*() was called.

        This is for internal use only.
    */
    enum YieldTypes { YtYield,
        YtYieldUntil,
        YtYieldSignal,
        YtYieldSignalTimeout,
        YtYieldMessage,
        YtYieldMessageTimeout,
        YtYieldSuspend,
        YtYieldKill
        };
    jmp_buf  taskJmpBuf;    // Jump buffer used by yield.  For internal use only.

public:
	/*!	@name Adding a New Task

		These methods are used to add new tasks to the task list
	*/
	/*! @{ */

	/*!  \brief Add a simple task.

		The task will execute once each cycle through the task list.  Unless the task itself forces itself into a different scheduling
		model (e.g., through YieldSignal), it will execute again at the next available opportunity
		\param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [0 127].
		System tasks have taskId values in the range [128 255].
		\param fn -- this is a void function with no arguments.  This is the procedure that is called every time
		the task is invoked.
		\sa addWaitDelay, addWaitUntil, addAutoWaitDelay
	*/
    void add(byte taskId, void (*fn)());

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
    void addWaitDelay(byte taskId, void(*fn)(), unsigned long msDelay);

	/*! \brief Add a task that will be delayed until a set system clock time before its first invocation

		This task will execute once each cycle.  Its first execution will be delayed until a set system clock time.  After this,
		unless the task forces itself into a different scheduling model (e.g., through yieldSignal), it will
		execute agaion at the next available opportunity.
		\param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [0 127].
		System tasks have taskId values in the range [128 255].
		\param fn -- this is a void function with no arguments.  This is the procedure that is called every time
		the task is invoked.
		\param msWhen -- the initial delay, in milliseconds
		\sa add, addWaitDelay, addAutoWaitDelay
    */
    void addWaitUntil(byte taskId, void(*fn)(), unsigned long msWhen);

	/*! \brief Add a task that is waiting for a message

		The task will be added, but will be waiting for a message.
		\param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [0 127].
		System tasks have taskId values in the range [128 255].
		\param fn -- this is a void function with no arguments.  This is the procedure that is called every time
		the task is invoked.
		\param timeout -- the maximum time to wait (in ms) before timing out.
	*/
    void addWaitMessage(byte taskId, void(*fn)(), unsigned long timeout=0);

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
    void addWaitSignal(byte taskId, void(*fn)(), byte sigNum, unsigned long timeout=0);

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
		\param startDelayed -- for the first execution, start immediately (false), or delay its start for one period (true)
		\sa add, addDelayed, addWaitUntil
    */
    void addAutoWaitDelay(byte taskId, void(*fn)(), unsigned long period, bool startDelayed=false);

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
    void addAutoWaitSignal(byte taskId, void(*fn)(), byte sigNum, unsigned long timeout=0, bool startWaiting=true);

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
		\sa addWaitMessage
	*/
	void addAutoWaitMessage(byte taskId, void(*fn)(), unsigned long timeout=0, bool startWaiting=true);
	/*! @} */

    /*!	@name Yield

    	These methods are used to yield control back to the task manager core.
    	@note Upon next invocation, execution will start at the TOP of
    	the routine, not at the statement following the yield.
    	@note A <i>yield</i> call will override any of the <i>addAuto...</i> automatic
    	rescheduling.  This will be a one-time override; later (non-<i>yield</i>) returns
    	will resume automatic rescheduling.
    */ /*! @{ */

    /*! \brief Exit from this task and return control to the task manager

	    This exits from the current task, and returns control to the task manager.  Functionally, it is similar to a
	    return statement.  The next time the task gains control, it will resume from the TOP of the routine.  Note that
	    if the task was an Auto task, it will be automatically rescheduled according to its Auto specifications.
	    \sa yieldDelay(), yieldUntil(), yieldSignal(), yieldMessage(), addAutoWaitDelay(), addAutoWaitSignal(), addAutoWaitMessage()
	*/
    void yield();

    /*! \brief Exit from the task manager and do not restart this task until after a specified period.

	    This exits from the current task and returns control to the task manager.  This task will not be rescheduled until
	    at least the stated number of milliseconds has passed.  Note that yieldDelay _overrides_ any of the Auto
	    specifications.  That is, the next rescheduling will occur _solely_ after the stated time period, and will
	    not be constrained by AutoWaitSignal, AutoWaitMessage, or a different AutoWaitDelay value.  The Auto specification will
	    be retained, and will be applied on future executions where yield() or a normal return are used.
	    \param ms -- the delay in milliseconds.  Note the next call may exceed this constraint depending on time taken by other tasks.
	    \sa yield(), yieldUntil(), yieldSignal(), yieldMessage(), addAutoWaitDelay(), addAutoWaitSignal(), addAutoWaitMessage()
	*/
    void yieldDelay(unsigned long ms);

	/*! \brief Exit from the task manager and do not restart this task until (after) a specified CPU clock time.

		This exits from the current task and returns control to the task manager.  This task will not be rescheduled until
		the CPU clock (millis()) has exceeded the given time.  Note that yieldUntil _overrides_ any of the Auto
		specifications.  That is, the next rescheduling will occur _solely_ after the stated clock time has passed, and will
		not be constrained by AutoWaitSignal, AutoWaitMessage, or a different AutoWaitDelay value.  The Auto specification will
		be retained, and will be applied on future executions where yield() or a normal return are used.
		\param when -- The target CPU time.  Note the next call may exceed this constraint depending on time taken by other tasks.
		\sa yield(), yieldDelay(), yieldSignal(), yieldMessage(), addAutoWaitDelay(), addAutoWaitSignal(), addAutoWaitMessage()
	*/
    void yieldUntil(unsigned long when);

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
    void yieldForSignal(byte sigNum, unsigned long timeout=0);

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
    void yieldForMessage(unsigned long timeout=0);
    /*! @} */


	/*!	@name Sending Signals and Messages

		These methods send signals or messages to other tasks running on this or other nodes
		@note If the nodeID is 0 on any routine that sends signals/messages to other nodes,
		the signal/message will be sent to this node.
	*/
	/*! @{ */

	/*!	\brief  Sends a signal to a task

	    Sends a signal to a task.  The signal will go to only one task.  If there are several tasks waiting on the signal, it will go to
	    the first task found that is waiting for this particular signal.  Note that once a task is signalled, it will not be waiting for
	    other instances of the same siggnal number.

	    \param sigNum -- the value of the signal to be sent
	    \sa yieldForSignal(), sendSignalAll(), addWaitSignal, addAutoWaitSignal()
	*/
    void sendSignal(byte sigNum);

	/*!	\brief  Sends a signal to a task

		Sends a signal to a task running on a different node.  The signal will go to only one task.
		If there are several tasks on the node waiting on the signal, it will go to
		the first task found that is waiting for this particular signal.  Note that once a task is signalled, it will not be waiting for
		other instances of the same siggnal number.

		\param nodeId -- The node that is to receive the signal
		\param sigNum -- The value of the signal to be sent
		\sa yieldForSignal(), sendSignalAll(), addWaitSignal, addAutoWaitSignal()
	*/
	void sendSignal(byte nodeId, byte sigNum);

	/*! \brief Send a signal to all tasks that are waiting for this particular signal.

		Signals all tasks that are waiting for signal <i>sigNum</i>.
		\param sigNum -- The signal number to be sent
		\sa sendSignal(), yieldForSiganl(), addWaitSignal(), addAutoWaitSignal()
	*/
    void sendSignalAll(byte sigNum);

	/*! \brief Send a signal to all tasks that are waiting for this particular signal.

		Signals all tasks that are waiting for signal <i>sigNum</i>.
		\param nodeId -- The node that is to receive the signal
		\param sigNum -- the signal number to be sent

		\sa sendSignal(), yieldForSiganl(), addWaitSignal(), addAutoWaitSignal()
	*/
	void sendSignalAll(byte nodeId, byte sigNum);

    /*! \brief  Sends a string message to a task

	    Sends a message to a task.  The message will go to only one task.

	    Note that once a task has been sent a message, it will not be waiting for
	    other instances of the same siggnal number.
	    Note that additional messages sent prior to the task executing will overwrite any prior messages.
	    Messages that are too large are ignored.  Remember to account for the trailing '\n'
	    when considering the string message size.

	    \param taskId -- the ID number of the task
	    \param message -- the character string message.  It is restricted in length to
	    TASKMGR_MESSAGE_LENGTH-1 characters.
	    \sa yieldForMessage()
	*/
    void sendMessage(byte taskId, char* message);       // string

	/*! \brief  Sends a string message to a task

		Sends a message to a task.  The message will go to only one task.

		Note that once a task has been sent a message, it will not be waiting for
		other instances of the same siggnal number.
		Note that additional messages sent prior to the task executing will overwrite any prior messages.
		Messages that are too large are ignored.  Remember to account for the trailing '\n'
		when considering the string message size.

		\param nodeId -- the node the message is sent to
		\param taskId -- the ID number of the task
		\param message -- the character string message.  It is restricted in length to
		TASKMGR_MESSAGE_LENGTH-1 characters.
		\sa yieldForMessage()
	*/
	void sendMessage(byte nodeId, byte taskId, char* message);

	/*! \brief Send a binary message to a task

		Sends a message to a task.  The message will go to only one task.
		Note that once a task has been sent a message, it will not be waiting for
		other instances of the same signal number.  Messages that are too large are
		ignored.

		Note that additional messages sent prior to the task executing will overwrite any prior messages.
		\param taskId -- the ID number of the task
		\param buf -- A pointer to the structure that is to be passed to the task
		\param len -- The length of the buffer.  Buffers can be at most TASKMGR_MESSAGE_LENGTH
		bytes long.
		\sa yieldForMessage()
	*/
    void sendMessage(byte taskId, void* buf, int len);

	/*! \brief Send a binary message to a task

		Sends a message to a task.  The message will go to only one task.
		Note that once a task has been sent a message, it will not be waiting for
		other instances of the same signal number.  Messages that are too large are
		ignored.

		Note that additional messages sent prior to the task executing will overwrite any prior messages.

		\param nodeId -- the node the message is sent to
		\param taskId -- the ID number of the task
		\param buf -- A pointer to the structure that is to be passed to the task
		\param len -- The length of the buffer.  Buffers can be at most TASKMGR_MESSAGE_LENGTH
		bytes long.
		\sa yieldForMessage()
	*/
	void sendMessage(byte nodeId, byte taskId, void* buf, int len);

	/*!	\brief Get source node/task of last message/signal

		Returns the nodeId and taskId of the node/task that last sent a signal or message
		to the current task.  If the current task has never received a signal, returns [0 0].
		If the last message/signal was from "this" node, returns fromNodeId=0.

		\param[out] fromNodeId -- the nodeId that sent the last message or signal
		\param[out] fromTaskId -- the taskId that sent the last message or signal
	*/
	void getSource(byte& fromNodeId, byte& fromTaskId);

	/*! @} */

    /*!	@name Task Management */
    /*! @{ */

	/*!	\brief Suspend the given task

		The given task will be suspended until it is resumed.  It will not be allowed to run, nor will it receive
		messages or signals.
		\param taskId The task to be suspended

		\note Not implemented.

		\sa receive
	*/
    void suspend(byte taskId);					// task

	/*!	\brief Suspend the given task on the given node

		Suspends a task on any node.  If nodeID==0, it suspends a task on this node. If the node or task
		do not exist, nothing happens.  If the task was already suspended, it remains suspended.
		\param nodeId The node containing the task
		\param taskId The task to be suspended

		\note Not implemented.
		\sa resume()
	*/
	void suspend(byte nodeId, byte taskId);			// node, task

	/*!	\brief Resume the given task on the given node

		Resumes a task.  If the task
		do not exist, nothing happens.  If the task had not been suspended, nothing happens.
		\param taskId The task to be resumed

		\note Not implemented.
		\sa suspend()
	*/
	void resume(byte taskId);					// task

	/*!	\brief Resume the given task on the given node

		Resumes a task on any node.  If nodeID==0, it resumes a task on this node.  If the node or task
		do not exist, nothing happens.  If the task had not been suspended, nothing happens.
		\param nodeId The node containnig the task
		\param taskId The task to be resumed
		\note Not implemented.
		\sa suspend()
	*/
	void resume(byte nodeId, byte taskId);			// node, task
	/*! @} */

	/*! @name Miscellaneous and Informational Routines */
	/*! @{ */
	/*! \brief Return the time since the start of the run, in milliseconds
	*/
    unsigned long runtime() const;

    size_t printTo(Print& p) const;
    /*! \brief Tell if the current task has timed out
	*/
	bool timedOut();

	/*! \brief Get a task's message buffer
		\return A pointer to the actual message buffer.  Use the contents of the buffer but do NOT modify it.
		If a task is killed, this pointer becomes invalid.
	*/
	void* getMessage();

	/*! \brief Return the task ID of the currently running task
		\return The byte value that represents the current task's ID.
	*/
    byte myId();

    /*!	\brief Return the node ID of this system.
    	\return The byte value that is the current node's radio ID.  If the radio has not been
    	enabled, returns 0.
    */
    byte myNodeId();
    /*! @} */

    // We need a publicly available TaskManager::loop() so our global loop() can use it
    void loop();

private:
	void internalSendSignal(byte fromNodeId, byte fromTaskId, byte sigNum);
	void internalSendSignalAll(byte fromNodeId, byte fromTaskId, byte sigNum);
	void internalSendMessage(byte fromNodeId, byte fromTaskId, byte taskId, char* message);
	void internalSendMessage(byte fromNodeId, byte fromTaskId, byte taskId, void* buf, int len);

private:
	void radioSender(byte);	// generic packet sender

    // status requests/
    //void yieldPingNode(byte);					// node -> status (responding/not responding)
    //void yieldPingTask(byte, byte);				// node, task -> status
    //bool radioFree();						// Is the radio available for use?

private:
    // Find the next active task
    // Note: there will always be a runnable task (tne null task) on the list.
    _TaskManagerTask* FindNextRunnable();

    // internal utility
public:
    _TaskManagerTask* findTaskById(byte id);

    // radio
private:
	// notes on parameters to the commands
	//	signal, signalAll: m_data[0] = sigNum
	//  message: m_data[0] = taskID, m_data[1+] = message
	//  suspend, resume: m_data[0] = taskID
	enum RadioCmd {
		tmrNoop,			//!<	Do nothing
		tmrStatus,			//!<	Request status of this node.
		tmrAck,				//!<	Node status returned from a tmrStatus request
		tmrTaskStatus,		//!<	Request status of a task on this node
		tmrTaskAck,			//!<	Task status returned from a tmrTaskStatus request
		tmrSignal,			//!<	Send a signal
		tmrSignalAll,		//!<	Send a signal to all on this node
		tmrMessage,			//!<	Send a message
		tmrSuspend,			//!<	Suspend a task
		tmrResume			//!<	Resume a task
	};
	_TaskManagerRadioPacket	radioBuf;
	bool	m_radioReceiverRunning;

public:
	/*! \brief Radio receiver task for inter-node communication

		This receives radio messages and processes them.  It will transmit messages and signals as needed.

		This routine is for internal use only.
	*/
	void tmRadioReceiverTask();

	/*! \brief Create the radio and start it receiving

		Create an RF24 radio instance and set our radio node ID.

		Note that additional messages sent prior to the task executing will overwrite any prior messages.

		\param nodeId -- the node the message is sent to
		\param cePin -- Chip Enable pin
		\param csPin -- Chip Select pin
	*/
	void radioBegin(byte nodeId, byte cePin, byte csPin);

};

//
// Defining our global TaskMgr
//
/*!	\brief The global object used to access TaskManager functionality
*/
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
inline _TaskManagerTask::_TaskManagerTask(): m_id(0), m_fn(NULL), m_stateFlags(0)
{
}

/*! \brief Construct a _TaskManager task object with given parameters
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

inline byte TaskManager::myId() {
	return m_theTasks.front().m_id;
};

inline byte TaskManager::myNodeId() {
	return m_myNodeId;
}

inline bool TaskManager::timedOut() {
    return m_theTasks.front().stateTestBit(_TaskManagerTask::TimedOut);
}

inline void TaskManager::sendSignal(byte sigNum) {
	internalSendSignal(0, myId(), sigNum);
}

inline void TaskManager::sendSignalAll(byte sigNum) {
	internalSendSignalAll(0, myId(), sigNum);
}

inline void TaskManager::sendMessage(byte taskId, char* message) {
	internalSendMessage(0, myId(), taskId, message);
}

inline void TaskManager::sendMessage(byte taskId, void* buf, int len) {
	internalSendMessage(0, myId(), taskId, buf, len);
}

inline void* TaskManager::getMessage() {
    return (void*)(&(m_theTasks.front().m_message));
}

inline unsigned long TaskManager::runtime() const { return millis()-m_startTime; }
#endif // TASKMANAGER_H_INCLUDED

/*!
	\mainpage TaskManger - Cooperative Multitasking System for Arduino

	\section Overview

	TaskManager is a cooperative task manager for the Arduino family of processors.
	It replaces the single user \c loop() routine with an environment in which the user
	can write many independent \c loop() -style routines.  These user routines are run in a
	round-robin manner.  In addition, routines can
	@li Delay -- suspending their operation for a specified period while allowing
	other routines to make use of the time
	@li Signal -- suspend action until they receive a signal, or send a signal allowing
	a different task to resume
	@li Message -- suspend action until a message has been received, or send messages to
	different tasks to pass them information.

	\section News

	2015/11/13: Release 1.0: Initial full release.
	<br>More code cleanup, improved documentation.  Added routines so tasks could ID where messages/signals came from.


	2015/10/15: Prerelease 0.2.1:  RF24 routines added.  Code cleaned up.
	<br>Routines include \c beginRadio() and versions of \c sendSignal(), \c sendSignalAll(),
	and \c sendMessage(). The \c send*()
	routines add a parameter to specify the \c nodeId the signal/message is being sent to.
	<br>Additionally, the code was refactored for clarity.

	2015/07/30: Prerelease 0.2:  Updated functionality of \c addAutoWaitDelay().
	<br>The \c addAutoWaitDelay()
	routine will operate on a repeat time based on the original start time, not on the end time.  For example,
	if the task starts at 1000 with addAutoDelay(500) and it takes 100 (all ms), it will start at 1000, 1500,
	2000, etc.  If it used yieldDelay, the yield is based on the end-time of the task, so it will start at
	1000, 1600 (1000+100(run)+500(delay)), 2200 (1600+100(run)+500(delay)), etc.
	<br>An excellent suggestion from the alpha user group.

	2015/01/15: Internal (SA-28) Pre-release:  Core functionality complete.
	<br>This is an alpha release for a small group of users.  Most of the non-RF functionality has been implemeented.
	<br>\c suspend() and \c resume() not implemented yet.  It
	may be a while until they are...

	\section Summary
	TaskManager is a cooperative multitasking task-swapper.  It allows the developer to create many independent
	tasks, which are called in a round-robin manner.
	<br>TaskManager offers the following:
	\li Any number of tasks.
	\li Extends the Arduino "setup/loop" paradigm -- the programmer creates several "loop" routines (tasks)
	instead of one.
	So programming is simple and straightforward.
	\li Tasks can communicate through signals or messages.  A signal is an information-free "poke" sent to
	whatever task is waiting for the poke.  A message has information (string or data), and is passed to
	a particular task.
	\li TaskManager programs can use RF24 2.4GHz radios to communicate between nodes.  So tasks running on
	different nodes can communicate through signals and messages in the same manner as if they were on the
	same node.

	\section Example
	The following is a TaskManager program.
		\code
			//
			// Blink two LEDs at different rates
			//

			#include <SPI.h>
			#include <RF24.h>
			#include <TaskManager.h>

			#define LED_1_PORT  2
			bool led_1_state;

			#define LED_2_PORT  3
			bool led_2_state;

			void setup() {
			  pinMode(LED_1_PORT, OUTPUT);
			  digitalWrite(LED_1_PORT, LOW);
			  led_1_state = LOW;

			  pinMode(LED_2_PORT, OUTPUT);
			  digitalWrite(LED_2_PORT, LOW);
			  led_2_state = LOW;

			  TaskMgr.add(1, loop_led_1);
			  TaskMgr.add(2, loop_led_2);
			}

			void loop_led_1() {
				led_1_state = (led_1_state==LOW) ? HIGH : LOW;
				digitalWrite(LED_1_PORT, led_1_state);
				TaskMgr.yieldDelay(500);
			}

			void loop_led_2() {
				led_2_state = (led_2_state==LOW) ? HIGH : LOW;
				digitalWrite(LED_2_PORT, led_2_state);
				TaskMgr.yieldDelay(100);
			}
		\endcode

		Note the following; this is all that is needed for TaskManager:
		\li You need to '\#include'  three files.  Since TaskManager (which is a class) uses RF,
		you need to include them whether or not you use the RF/radio routines.
		\li TaskManager routines are accessed through the 'TaskMgr' object.
		\li There is no 'void  loop()'. Instead, you write a routine for each independent task as
		if it were its own 'loop()'.
		\li You tell 'TaskMgr' about your routines through the 'void TaskManager::add(byte
		taskId, void (*) task);' method.  This is shown in the two calls
		to 'TaskMgr.add(...);'.
		\li You do not use 'delay();'.  Never use 'delay();'.  'delay();' delays all
		things; nothing will run.  Instead use TaskManager::yieldDelay()'.  'yieldDelay()' will return from
		the current routine and guarantee it won't be restarted for the specified time.  However, other routines
		will be allowed to run during this time.

	\section future Future Work
	Here are the upcoming/future plans for TaskManager.  Some are short term, some are longer term.
	\li TaskManager: The Book.  I have a draft of a more in-depth set of tutorials, examples, and programmer's
	documentation.  This needs to be finalized and added to the TaskManager release tree.
	\li RF Extension.  Add a method 'getStatus(nodeId)' that returns the status (including whether or not
	exists) for a given node.  This will also require the creation of a task in the system task group.
	\li RF Extension.  Right now, 'nodeId' values of 1..254 are available for general use.  'nodeId' value
	255 has been reserved for "all".  I need to add a routine 'findNodes()' that finds all available nodes.
	After this, specifying 'nodeId' of 255 in a 'sendSignal()', 'sendSignalAll()', or 'sendMessage()' will
	send the signal/message to every node that has been found. 'getStatus()' will be needed for this.
	\li SPI investigation/certification.  Running RF in a multi-SPI environment is "fraught with peril".  The SPI routines
	\c beginTransaction() and \c endTransaction() allow different SPI devices to share the MOSI/MISO interface even
	if they use different serial settings.  However, most SPI libraries do not use currently use these routines.
	(Note that the RF library recommended for TaskManager does.)  We need to investigate the different SPI
	libraries and identify the transaction-safety of each.
	\li Suspend, resume, and kill.  These routines haven't been implemented.
*/
