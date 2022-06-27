/* AVR Networking (RF24) Note:
   Uncomment this #define ONLY IF you wish the ACR RF24 networking code to be included.
   This will incorporate RF24 components into AVR compilations.
   Any AVR program using this will also have to #include <RF24.h> prior #include <TaskManagerRF24.h>
*/
//#define TASKMGR_AVR_RF24

#ifndef TASKMANAGERCORE_H_INCLUDED
#define TASKMANAGERCORE_H_INCLUDED


//#include <Streaming.h>
#include "ring.h"

#if defined(ARDUINO_ARCH_AVR)
typedef uint8_t tm_nodeId_t;
typedef uint8_t tm_taskId_t;
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
typedef uint16_t tm_nodeId_t;
typedef uint8_t tm_taskId_t;
#else
#endif

#include <setjmp.h>

/*! \file TaskManagerCore.h
    Header for Arduino TaskManager library
*/

/*! \defgroup global Global Values
	@{
*/
/*! \def TASKMGR_MESSAGE_SIZE
    The maximum size of a message passed between tasks

    This defines the maximum size for an inter-task message.  It is constrained, in part,
    by plans for RFI communication between tasks running on different devices.

    Note that the NRF24's max payload size is 32.  Message overhead is 3 bytes for cmd, src node, src task 
	and 1 byte for target task.
	
    ESP-NOW max payload is 250, overhead is 4 for cmd/src and 1 for target.  Message overhead is 4 bytes for 
	cmd, src node, src task, and 1 byte for target task.  THIS MAY CHANGE IF WE ALLOW MORE THAN 256 TASKS
	
*/
/*!	\def TASKMGR_MAX_PAYLOAD
	The maximum size of the payload (packet plus overhead) that the particular radio 
	architecture supports.  This differs for ESP and RF24 systems.
*/
#if defined(ARDUINO_ARCH_AVR)
#define TASKMGR_MAX_PAYLOAD (32)
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#define TASKMGR_MAX_PAYLOAD (250)
#else
#endif
#define TASKMGR_MESSAGE_SIZE (TASKMGR_MAX_PAYLOAD-1-sizeof(tm_nodeId_t)-2*sizeof(tm_taskId_t))
/*! @} */ // end global

// Process includes for networking code
#if defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)
#include "radioDriverRF.h"
#elif  defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#include "radioDriverESP.h"
#endif


/*! \defgroup TaskManager TaskManager
 * @{
*/

/*!	\defgroup fixed Predefined TaskIDs
	\ingroup TaskManager
	@{
*/

/*  FIXED TASKS */
/*!	\def TASKMGR_MAX_TASK
*/
#if defined(ARDUINO_ARCH_AVR)
#define TASKMGR_MAX_TASK 255
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#define TASKMGR_MAX_TASK 255
#endif
/*! \def TASKMGR_NULL_TASK
	The TaskID of the null task.  At present, 255 for both AVR and ESP.  We
	may increase the max ESP task value along with changing tm_taskId_t.
*/
#define TASKMGR_NULL_TASK TASKMGR_MAX_TASK

/*! \def TASKMGR_RF_MONITOR_TASK
	The TaskID of the task that monitors the inter-node radio link
*/
#define TASKMGR_RF_MONITOR_TASK (TASKMGR_NULL_TASK-1)

/*! \def TASKMGR_PING_MONITOR_TASK
	The TaskID that monitors RF PING requests
*/
#define TASKMGR_PING_MONITOR_TASK (TASKMGR_NULL_TASK-2)

/*! \def TASKMGR_MENU_MONITOR_TASK
	The task that monitors button presses for the LCD menu subsystem
*/
#define TASKMGR_MENU_MONITOR_TASK (TASKMGR_NULL_TASK-3)
	/*! @} */ // group fixed
 /*! @} */ // group TaskManager
 
/*! \defgroup TaskManagerTask _TaskManagerTask
   @{
*/ 

class TaskManager;	// forward declaration

/*! \class _TaskManagerTask
    \brief Internal class to manage a single active task

    This class is used by the TaskManager class to manage a single task.
    It should not be used by the end-user.
*/
#if defined(TASKMANAGER_DEBUG)
class _TaskManagerTask: public Printable {
#else
class _TaskManagerTask {
#endif
    friend class TaskManager;
protected:
	/*! \defgroup public Public Member Constants and Fields
		\ingroup TaskManagerTask
	*/
	/* @{ */
    /*! \enum TaskStates
        \brief Defines the various flag-bits describing the state of a task
    */
    enum TaskStates {UNUSED01=0x01,   	// RFU
        WaitMessage=0x02,               //!< Task is waiting for a message.
        WaitUntil=0x04,                 //!< Task is waiting until a time has passed.
        UNUSED08 = 0x08,        		// RFU
        AutoReWaitMessage = 0x10,       //!< After normal completion, task will be set to wait for a message.
        AutoReWaitUntil = 0x20,         //!< After normal completion, task will be set to wait until a time has passed.
        TimedOut = 0x40,                //!< Marker that the task had a timeout with a signal/message, and the timeout happened.
        Suspended=0x80                  //!< Task is suspended and will not receive messages, signals, or timeouts.
        };
	/* @} */ // end public
public:
	tm_nodeId_t	m_fromNodeId;		// where the signal/message came from
	tm_taskId_t	m_fromTaskId;
protected:
    uint8_t m_stateFlags; //!< The task's state information

    // Active delay information.  If a task is waiting, here is the reason (or in the
    // case of messaging, the response)
    unsigned long m_restartTime;  //!< Used by WaitUntil to determine the restart time.
                                    //!< This is compared to the absolute ms clock maintained by the processor


    char m_message[TASKMGR_MESSAGE_SIZE+1];   //!<  The message sent to this task (if waiting for a message)
    uint16_t m_messageLength;

    //NOT USED??? unsigned int m_reTimeout;   //!< The timeout to use during auto restarts.  0 means no timeout.

    // Autorestart information.  If a task has autorestart, here is the information to use at the restart
    unsigned long m_period; //!< If it is auto-reschedule, the rescheduling period

    tm_taskId_t    m_id; //!< The task ID
    void    (*m_fn)(); //!< The procedure to be invoked each cycle

public:
	/*!	\defgroup constructors	Constructors and Destructor
		\ingroup TaskManagerTask
	*/
	/*! @{ */
    // Constructors and destructors
    _TaskManagerTask();
    _TaskManagerTask(tm_taskId_t id, void (*fn)());
    ~_TaskManagerTask();
	/*! @} */ // end constructors
protected:
    // State bit manipulation methods
    bool anyStateSet(byte);
    bool allStateSet(byte);
    bool stateTestBit(byte);
    void stateSet(byte);
    void stateClear(byte bits);
    void resetCurrentStateBits();

    // Querying its state and other info
    bool isRunnable();

    // Setting its state
    void setRunnable();
	void setSuspended();
	void clearSuspended();
    void setWaitUntil(unsigned long);
    void setWaitDelay(unsigned int);
    void setAutoDelay(unsigned int);
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
#if defined(TASKMANAGER_DEBUG)
	size_t printTo(Print& p) const;
#endif
};
/*! @} */ // end TaskManagerTask

/**********************************************************************************************************/

/*! \ingroup TaskManager
 * @{
*/
/*! \class TaskManager
    \brief A cooperative multitasking manager

    Manages a set of cooperative tasks.  This includes round-robin scheduling, yielding, and inter-task
    messaging and signaling.  It also replaces the loop() function in standard Arduino programs.  Nominally,
    there is a single instance of TaskManager called TaskMgr.  TaskMgr is used for all actual task control.

    Each task has a taskID.  By convention, user tasks' taskID values are in the range [0 127].
*/
#if defined(TASKMANAGER_PRINTABLE)
class TaskManager: public Printable {
#else
class TaskManager {
#endif
#if (defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24))
private:
	RF24*	m_rf24;				// Our radio (dynamically allocated)	
#endif
public:
    ring<_TaskManagerTask> m_theTasks; //!< The ring of all tasks.  For internal use only.

private:
    unsigned long m_startTime;  // Start clock time.  Used to calculate runtime. For internal use only.
public:
	/*! \defgroup constructor Constructor and Destructor
		\ingroup TaskManager
		@{
	*/
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
	/*! @} */ // end constructor

    // Things used by yield
private:
    /* Defines the different methods a process may yield control.

        This is the return value passsed back by the different yield*() routines to the control `loop()`.
        It indicates which form of yield*() was called.

        This is for internal use only.
    */
    enum YieldTypes { YtYield,
        YtYieldUntil,
        YtYieldMessage,
        YtYieldMessageTimeout,
        YtYieldSuspend,
        YtYieldKill
        };
    jmp_buf  taskJmpBuf;    // Jump buffer used by yield.  For internal use only.

public:
	/*!	\defgroup add Adding Tasks
		\ingroup TaskManager
		@{
	*/
	/*	\name add

		These methods are used to add new tasks to the task list
	*/

	/*!  \brief Add a simple task.

		The task will execute once each cycle through the task list.  Unless the task itself forces itself into a different scheduling
		model (e.g., through YieldSignal), it will execute again at the next available opportunity
		\param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [1 239].
		System tasks have taskId values in the range [240 255].
		\param fn -- this is a void function with no arguments.  This is the procedure that is called every time
		the task is invoked.
		\sa addWaitDelay, addWaitUntil, addAutoWaitDelay
	*/
    void add(tm_taskId_t taskId, void (*fn)());

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
    void addWaitDelay(tm_taskId_t taskId, void(*fn)(), unsigned long msDelay);

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
    void addWaitUntil(tm_taskId_t taskId, void(*fn)(), unsigned long msWhen);

	/*! \brief Add a task that is waiting for a message

		The task will be added, but will be waiting for a message.
		\param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [0 127].
		System tasks have taskId values in the range [128 255].
		\param fn -- this is a void function with no arguments.  This is the procedure that is called every time
		the task is invoked.
		\param timeout -- the maximum time to wait (in ms) before timing out.
	*/
    void addWaitMessage(tm_taskId_t taskId, void(*fn)(), unsigned long timeout=0);

    /*! \brief Add a task that will automatically reschedule itself with a delay

		This task will execute once each cycle.  The task will automatically reschedule itself to not execute
		until the given delay has passed. The first execution may be delayed using the optional fourth parameter startDelayed.
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
    void addAutoWaitDelay(tm_taskId_t taskId, void(*fn)(), unsigned long period, bool startDelayed=false);

	/*! \brief Add a task that is waiting for a message or until a timeout occurs

		The task will be added, but will be set to be waiting for the listed signal.  If the signal does
		not arrive before the timeout period (in milliseconds), then the routine will be activated.  The
		routine may use TaskManager::timedOut() to determine whether it timed our or received the signal.
		\param taskId - the task's ID.  For normal user tasks, this should be a byte value in the range [0 127].
		System tasks have taskId values in the range [128 255].
		\param fn -- this is a void function with no arguments.  This is the procedure that is called every time
		the task is invoked.
		\param timeout -- the maximum time to wait (in ms) before timing out.
		\param startWaiting -- tells whether the routine will start waiting for the signal (true) or will execute
		immediately (false).
		\sa addWaitMessage
	*/
	void addAutoWaitMessage(tm_taskId_t taskId, void(*fn)(), unsigned long timeout=0, bool startWaiting=true);
	/*! @} */ // end add
	
	/*! \defgroup yield Yield Methods
		\ingroup TaskManager
		@{
	*/
    /* Yield

    	These methods are used to yield control back to the task manager core.
    	@note Upon next invocation, execution will start at the TOP of
    	the routine, not at the statement following the yield.
    	@note A <i>yield</i> call will override any of the <i>addAuto...</i> automatic
    	rescheduling.  This will be a one-time override; later (non-<i>yield</i>) returns
    	will resume automatic rescheduling.
    */

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

    /*! \brief Exit from the task manager and do not restart this task until a message has been received or a stated time period has passed.

	    This exits from the current task and returns control to the task manager.  This task will not be rescheduled until
	    a message has been received or a stated time period has passed (the timeout period). The TaskManager::timeOut()
	    function will tell whether or not the timeout had been triggered.

	    \note The yieldForMessage call _overrides_ any of the Auto
	    specifications.  That is, the next rescheduling will occur _solely_ after the signal has been received, and will
	    not be constrained by AutoWaitDelay, AutoWaitSignal, or a different AutoWaitMessage value.  The Auto specification will
	    be retained, and will be applied on future executions where yield() or a normal return are used.
	    \param timeout -- The timeout period, in milliseconds.
	    \sa yield(), yieldDelay(), yieldForSignal(), addAutoWaitDelay(), addAutoWaitSignal(), addAutoWaitMessage(), timeOut()
	*/
    void yieldForMessage(unsigned long timeout=0);
    /*! @} */ // end yield

	/*! \defgroup send Sending Messages
		\ingroup TaskManager
		@{
	*/
	/*  Sending Messages

		These methods send messages to other tasks running on this or other nodes
		@note If the nodeID is 0 on any routine that sends messages to other nodes,
		the signal/message will be sent to this node.
	*/

    /*! \brief  Sends a string message to a task

	    Sends a message to a task.  The message will go to only one task.

	    Note that once a task has been sent a message, it will not be waiting for
	    other instances of the same siggnal number.
	    Note that additional messages sent prior to the task executing will overwrite any prior messages.
	    Messages that are too large are ignored.  Remember to account for the trailing '\n'
	    when considering the string message size.

		\note In networked TaskManager environments, this will send the message to a task
		on the current node.

	    \param taskId -- the ID number of the task
	    \param message -- the character string message.  It is restricted in length to
	    TASKMGR_MESSAGE_LENGTH-1 characters.
		\returns true if message successfully sent, false otherwise
	    \sa yieldForMessage()
	*/
    bool sendMessage(tm_taskId_t taskId, char* message);       // string

	/*! \brief Send a binary message to a task

		Sends a message to a task.  The message will go to only one task.
		Note that once a task has been sent a message, it will not be waiting for
		other instances of the same signal number.  Messages that are too large are
		ignored.

		\note Additional messages sent prior to the task executing will overwrite any prior messages.
		
		\note In networked TaskManager environments, this will send the message to a task
		on the current node.
		
		\param taskId -- the ID number of the task
		\param buf -- A pointer to the structure that is to be passed to the task
		\param len -- The length of the buffer.  Buffers can be at most TASKMGR_MESSAGE_LENGTH
		bytes long.
		\returns true if message successfully sent, false otherwise
		\sa yieldForMessage()
	*/
    bool sendMessage(tm_taskId_t taskId, void* buf, int len);
	/*!	@} */ // end send
	
#if (defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
	/*! \brief  Sends a string message to a task
		\ingroup send

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
	bool sendMessage(tm_nodeId_t nodeId, tm_taskId_t taskId, char* message);


	/*! \brief Send a binary message to a task
		\ingroup send

		Sends a message to a task.  The message will go to only one task.
		Note that once a task has been sent a message, it will not be waiting for
		other instances of the same signal number.  Messages that are too large are
		ignored.

		\note Additional messages sent prior to the task executing will overwrite any prior messages.
		
		\note This routine is only available on ESP and RF24-enabled AVR environments.
		
		\param nodeId -- the node the message is sent to
		\param taskId -- the ID number of the task
		\param buf -- A pointer to the structure that is to be passed to the task
		\param len -- The length of the buffer.  Buffers can be at most TASKMGR_MESSAGE_LENGTH
		bytes long.
		\sa yieldForMessage()
	*/
	bool sendMessage(tm_nodeId_t nodeId, tm_taskId_t taskId, void* buf, int len);


#endif

	/*!	\defgroup receive Receiving Messages
		\ingroup TaskManager
		@{
	*/

	/*!	\brief Get task ID of last message/signal

		Returns the taskId of the task that last sent a signal or message
		to the current task.  If the current task has never received a signal, returns [0].

		\param[out] fromTaskId -- the taskId that sent the last message or signal
	*/
	void getSource(tm_taskId_t& fromTaskId);
#if (defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
	/*!	\brief Get source node/task ID of last message/signal

		Returns the nodeId and taskId of the node/task that last sent a signal or message
		to the current task.  If the current task has never received a signal, returns [0 0].
		If the last message/signal was from "this" node, returns fromNodeId=0.

		\note This routine is only available on ESP and RF24-enabled AVR environments.

		\param[out] fromNodeId -- the nodeId that sent the last message or signal
		\param[out] fromTaskId -- the taskId that sent the last message or signal
	*/
	void getSource(tm_nodeId_t& fromNodeId, tm_taskId_t& fromTaskId);
#endif

	/*! @} */ // end receive

	/*  @name Changing Task Scheduing Method */
	/*  @{ */
	/*  \brief Change task types

		These routines change the scheduling method for the given task.  FOr example, SetAutoWaitDelay() will change a
		task so it will automatically wait for the set delay period after yielding (instead of using whatever
		rescheduling process was defined when it was Add()ed).

		The change will take place immediately.  If it is being performed on the currernt task, it will impact the next
		yield/return.  If it is performed on a suspended task, the suspended tasks's reawakening constraints will be
		reset to the new method.

	*/

	/*  @} */

    /*!	@defgroup task Task Management
		\ingroup TaskManager
    /*! @{ */

	/*!	\brief Suspend the given task on this node.

		The given task will be suspended until it is resumed.  It will not be allowed to run, nor will it receive
		messages or signals.
		\param taskId The task to be suspended
		\returns true if the task could be suspended, false otherwise
		\sa receive
	*/
    bool suspend(tm_taskId_t taskId);					// task
	
#if (defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
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
	bool suspend(tm_nodeId_t nodeId, tm_taskId_t taskId);			// node, task
#endif	

	/*!	\brief Resume the given task on this node

		Resumes a task.  If the task
		do not exist, nothing happens.  If the task had not been suspended, nothing happens.
		\param taskId The task to be resumed
		\returns true if the task could be resumed, false otherwise

		\note Not implemented.
		\sa suspend()
	*/
	bool resume(tm_taskId_t taskId);					// task
	
#if (defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
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
	bool resume(tm_nodeId_t nodeId, tm_taskId_t taskId);			// node, task
#endif

	/*! @} */	// end task */
	
	/* **** Mesh/Radio Internal Routines */
#if (defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
private:
	tm_nodeId_t m_myNodeId;
	/*!	\defgroup internal Internal components
		\ingroup TaskManager
		@{
	*/
	/*!	\brief Generic packet sender
		TaskManager.radioBuf contains a complete formatted packet of information to 
		send to the designated node.  radioSender(node) sends the packet to the node.
		
		This routine is only present in radio-enabled environments.  It is implemented 
		in the driver module.
		\param node - The target node
		\returns - boolean, true if the remote node received the packet (low-level receive).  This
		does not guarantee that the task has received the packet.
	*/
	bool radioSender(tm_nodeId_t);	// generic packet sender
	/* @} */ // end internal
    // status requests/
    //void yieldPingNode(byte);					// node -> status (responding/not responding)
    //void yieldPingTask(byte, byte);				// node, task -> status
    //bool radioFree();						// Is the radio available for use?
	/*! @} */	// end internal
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
		tmrMessage,			//!<	Send a message
		tmrSuspend,			//!<	Suspend a task
		tmrResume			//!<	Resume a task
	};
	_TaskManagerRadioPacket	radioBuf;
	bool	m_radioReceiverRunning;
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
public:
/*!	\ignore */
	SemaphoreHandle_t m_TaskManagerMessageQueueSemaphore;
/*! \endignore */
#endif
public:
	/*!	\brief General radio receiver task.
		Each driver is required to provide a radio receiver task.  The radio receiver task is 
		a full fledged TaskManager task, and is called on schedule with all other tasks.
		
		The radio receiver task should empty the incoming message queue of all messages and distribute them
		to the appropriate target tasks.
	*/
	void tmRadioReceiverTask();

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
private:
	esp_err_t m_lastESPError;
public:
	/*! \brief Return a textual description of the last ESP error message.
		\returns A const char* with text describing the last error.
	*/
	const char* lastESPError();
#endif
#endif

	/*! \defgroup radio Network/mesh functions
		\ingroup TaskManager
	
		These functions are available in the ESP and the RF24-enabled versions of TaskManager.
		They allow the creation of networks of AVR or ESP based nodes.  Tasks running on nodes
		may send/receive messages to/from tasks either on the local node or to any node on the 
		network.  Any operation performable on a task may also be performed on a remote task.
		
		Note that ESP and AVR nodes may not be intermixed on the same network.
		
		Note that there is one unified network space for each of ESP and AVR.  A node may reach
		any other node within radio range.
		@{
	*/
#if (defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24))
public:
	/*! \brief Create the radio and start it receiving

		Create an RF24 radio instance and set our radio node ID.

		Note that additional messages sent prior to the task executing will overwrite any prior messages.

		\param nodeId -- the node the message is sent to
		\param cePin -- Chip Enable pin
		\param csPin -- Chip Select pin
		\note This routine is only available on ESP and RF24-enabled AVR environments.
	*/
	void radioBegin(tm_nodeId_t nodeId, byte cePin, byte csPin);
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
public:
	/*! \brief Create the radio and start it receiving

		Set up the ESP-32 ESP-Now configuration set our radio node ID.

		There are three variants:  radioBegin(nodeId), radioBegin(nodeId, ssId), radioBegin(nodeId, ssid, pw).

		The first is used when all of the nodes of a project are only using ESP-NOW.
		It closes the WiFi connection after ESP-NOW has been configured.

		The second is used if this node is ESP-NOW-only, but another node in the project (which this node is communicating with)
		is using WiFi.  It configures ESP-NOW to use the same channel as the WiFi node, but closes WiFi for this node.

		The third is used if this node is using both ESP-NOW and WiFi.  It configures both ESP-NOW and WiFi, acquires an IP address,
		and leaves WiFi open for later use.

		Note that additional messages sent prior to the task executing will overwrite any prior messages.

		\param nodeId -- the node the message is sent to
		\param ssid -- the ssid of the local WiFi node (if using WiFi in a project).
		\param pw -- the password of the local WiFi node (if using WiFi from this node).
		\note This routine is only available on ESP environments.
	*/
	bool radioBegin(tm_nodeId_t nodeId, const char* ssid=NULL, const char* pw=NULL);

	/*! \brief Add a peer for ESP-Now communications

		\param nodeId -- A peer node for future communications.
		\note This routine is only available on ESP environments.
	*/
	bool registerPeer(tm_nodeId_t nodeId);

	/*!	\brief Remove a peer from ESP-Now communications
		\param nodeId -- A peer node that will no longer be usable as a peer
		\note This routine is only available on ESP environments.
	*/
	bool unRegisterPeer(tm_nodeId_t nodeId);
#endif

#if (defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
	/*!	\brief Return the node ID of this system.
		\return The byte value that is the current node's radio ID.  If the radio has not been
		enabled, returns 0.
		\note This routine is only available on ESP environments.
	*/
    tm_nodeId_t myNodeId();
#endif
	/*! @} */ // end radio

	/*! \defgroup misc Miscellaneous and Informational Routines
		\ingroup TaskManager
	    @{ */
		
	/*! \brief Return the time since the start of the run, in milliseconds
	*/
    unsigned long runtime() const;
#if defined(TASKMANAGER_DEBUG)
/*!	\cond DO_NOT_PROCESS */
    size_t printTo(Print& p) const;
/*!	\endcond */
#endif
    /*! \brief Tell if the current task has timed out while waiting for a signal or message
    	\return true if the task started due to timing out while waiting for a signal or message; false otherwise.
	*/
	bool timedOut();


	/*! @} */ // end misc
	
	/*! \brief Get a task's message buffer
		\ingroup receive
		\return A pointer to the actual message buffer.  Use the contents of the buffer but do NOT modify it.
		If a task is killed, this pointer becomes invalid.
	*/	void* getMessage();

	/*!	\brief Get the length of the message in the buffer
		\ingroup receive
		\return The size of the data block in the buffer.  Note that if the content is a string, the
		size will be one greater than the string length to account for the trailing null.
	*/
	uint16_t getMessageLength();
	
	/*! \brief Return the task ID of the currently running task
		\ingroup task
		\return The byte value that represents the current task's ID.
	*/
    tm_taskId_t myId();

    // We need a publicly available TaskManager::loop() so our global loop() can use it
    void loop();

protected:
	/*!	\brief Sends a string message to a task on this system.
		Send a null-terminated (string) message to a task running on this system.  This is the internal
		gateway used by both sendMessage("this node") and from the radio receiver module (messages from
		other nodes).
		\ingroup internal
		\param fromNodeId - the source node of the message.  0 means "this node".
		\param fromTaskId - the source task of the message.
		\param taskId - which task is to receive the message.
		\param message - a null-terminated string message.
	*/
	void internalSendMessage(tm_nodeId_t fromNodeId, tm_taskId_t fromTaskId, tm_taskId_t taskId, char* message);

	/*!	\brief Sends a binary message to a task on this system.
		Send a raw data (binary) message to a task running on this system.  This is the internal
		gateway used by both sendMessage("this node") and from the radio receiver module (messages from
		other nodes).
		\ingroup internal
		\param fromNodeId - the source node of the message.  0 means "this node".
		\param fromTaskId - the source task of the message.
		\param taskId - which task is to receive the message.
		\param buf - the buffer with the message.
		\param len - the size of the buf (in bytes).
	*/
	void internalSendMessage(tm_nodeId_t fromNodeId, tm_taskId_t fromTaskId, tm_taskId_t taskId, void* buf, int len);

private:
    // Find the next active task
    // Note: there will always be a runnable task (tne null task) on the list.
    _TaskManagerTask* FindNextRunnable();

    // internal utility
public:
    _TaskManagerTask* findTaskById(tm_taskId_t id);

};
/*! @} */ // end TaskManager

//
// Inline stuff
//

/*! \ingroup TaskManagerTask
 * @{
*/
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
inline _TaskManagerTask::_TaskManagerTask(tm_taskId_t taskId, void (*fn)()): m_id(taskId), m_fn(fn), m_stateFlags(0) {
}
/*! \brief Standard destructor.
*/
inline _TaskManagerTask::~_TaskManagerTask() {
}

// Setting the task's state
/*! \brief Set the task to a runnable state.  That is, it will not be suspended or waiting for anything
*/
inline void _TaskManagerTask::setRunnable() {
    stateClear(WaitMessage+WaitUntil+Suspended);
}
/*! \brief Set the task to a generic suspended state.  That is, it will not be run until it is set runnable
*/
inline void _TaskManagerTask::setSuspended() {
	stateSet(Suspended);
}
/*! \brief Clear the task's suspended'ness, if it was suspended.  Note, if it is still waiting on a messagel etc.,
	it will keep waiting.
*/
inline void _TaskManagerTask::clearSuspended() {
	stateClear(Suspended);
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
// Sending messages to a task
//

/*! \brief Send an actual message to the task

    Sends a message to the task.  The timeout and other flags are cleared.
    Buffers up to TASKMGR_MESSAGE_SIZE are supported; others are ignored.
    This is the only form; sending a string will need to call it with the
    string address and length+1 (null byte at end...).
*/
inline void _TaskManagerTask::putMessage(void* buf, int len) {
    if(len<=TASKMGR_MESSAGE_SIZE) {
        memcpy(m_message, buf, len);
        m_messageLength = len;
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
/*! \brief Resets bits (note: not sure of purpose)
*/
inline void _TaskManagerTask::resetCurrentStateBits() {
	m_stateFlags = (m_stateFlags&0xf8) + ((m_stateFlags>>3)&0x07);
}
 /*! @} */ // end TaskManagerTask
 
 /*************** TaskManager Implementation ****************/
/*! \ingroup TaskManager
 * @{
*/

/* \ingroup  task 
	@{
*/
inline tm_taskId_t TaskManager::myId() {
	return m_theTasks.front().m_id;
};
/* @} */ // end task
/*! \ingroup receive
	@{
*/
inline bool TaskManager::timedOut() {
    return m_theTasks.front().stateTestBit(_TaskManagerTask::TimedOut);
}
	/*! @} */ // end receive
/*! \ingroup send
	@{
*/
inline bool TaskManager::sendMessage(tm_taskId_t taskId, char* message) {
	internalSendMessage(0, myId(), taskId, message);
	return true;
}

inline bool TaskManager::sendMessage(tm_taskId_t taskId, void* buf, int len) {
	internalSendMessage(0, myId(), taskId, buf, len);
	return true;
}
	/*!	@} */ // end send
/*! \ingroup receive
	@{
*/
inline void* TaskManager::getMessage() {
    return (void*)(&(m_theTasks.front().m_message));
}

inline uint16_t TaskManager::getMessageLength() {
	return m_theTasks.front().m_messageLength;
}

inline void TaskManager::getSource(tm_taskId_t& fromTaskId) {
	fromTaskId = m_theTasks.front().m_fromTaskId;
}
	/*! @} */ // end receive
/*! \ingroup misc		
	@{
*/
inline unsigned long TaskManager::runtime() const { return millis()-m_startTime; }
	/*! @} */ // end misc
/*! @} */	// end taskmanager
 
/**************** Network ************************/
/*! \ingroup TaskManager
 *	@{
*/
#if (defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
inline tm_nodeId_t TaskManager::myNodeId() {
	return m_myNodeId;
}
#endif

/*! @}
*/
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

	2022/05/31: Release 2.0: Merging Atmel and ESP32 branches.  
	<br>ESP32 will include multi-node (mesh) routines
	to send/receive messages between nodes.  Atmel systems will need to use TaskManagerRF version 2.0 to 
	support this functionality.
	2015/11/13: Release 1.0: Initial full release.
	<br>More code cleanup, improved documentation.  Added routines so tasks could ID where messages/signals came from.
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
		\li You need to '\#include <TaskManager.h>'.
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
	\li SPI investigation/certification.  Running RF in a multi-SPI environment is "fraught with peril".  The SPI routines
	\c beginTransaction() and \c endTransaction() allow different SPI devices to share the MOSI/MISO interface even
	if they use different serial settings.  However, most SPI libraries do not use currently use these routines.
	(Note that the RF library recommended for TaskManager does.)  We need to investigate the different SPI
	libraries and identify the transaction-safety of each.
	\li Suspend, resume, and kill.  These routines haven't been fully tested.
*/
