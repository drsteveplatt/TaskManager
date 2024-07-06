/*! \file TaskManagerCore.h
    Header for Arduino TaskManager library
*/

/* AVR Networking (RF24) Note:
   Uncomment this #define ONLY IF you wish the ACR RF24 networking code to be included.
   This will incorporate RF24 components into AVR compilations.
   Any AVR program using this will also have to #include <RF24.h> prior #include <TaskManagerRF24.h>
*/

#ifndef TASKMANAGERCORE_H_INCLUDED
#define TASKMANAGERCORE_H_INCLUDED

// Are we including the radio routines in this build?
// comment out to no use radio
/*!	\def TM_USING_RADIO
	Determines whether or not radio (networking) routines are included in the build.
	
	Normally set to true for AVR systems.  Should always be true for ESP systems.
	
	This adds a byte to the general data structures and makes the max packet size one byte smaller.
*/
#define TM_USING_RADIO true

//#define TASKMANAGER_DEBUG
//#define TASKMGR_AVR_RF24

//#include <Streaming.h>
#include "ring.h"

#if defined(ARDUINO_ARCH_AVR)
typedef uint8_t tm_nodeId_t;	//!<	Storage for Node ID (Atmel architecture)
typedef uint8_t tm_taskId_t;	//!<	Storage for Task ID (Atmel architecture)
typedef uint8_t tm_netId_t;		//!<	Storage for Net ID (Atmel architecture)
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
typedef uint16_t tm_nodeId_t;	//!<	Storage for Node ID (ESP architecture)
typedef uint8_t tm_taskId_t;	//!<	Storage for Task ID (ESP architecture)
typedef uint8_t tm_netId_t;		//!<	Storage for Net ID (ESP architecture)
#else
#endif

#include <setjmp.h>



/*x \ingroup Globals
	@{
*/


#if defined(ARDUINO_ARCH_AVR)
/*!	\def TASKMGR_MAX_PAYLOAD
	The maximum size of the payload (packet plus overhead) that the particular radio 
	architecture supports.  This differs for ESP and RF24 systems.
*/
#define TASKMGR_MAX_PAYLOAD (32)
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#define TASKMGR_MAX_PAYLOAD (250)
#else
#endif

/*! \def TASKMGR_MESSAGE_SIZE
    The maximum size of a message passed between tasks

    This defines the maximum size for an inter-task message.  It is constrained, in part,
    by plans for RFI communication between tasks running on different devices.

    Note that the NRF24's max payload size is 32.  Message overhead is 3 bytes for cmd, src node, src task 
	and 1 byte for target task.
	
    ESP-NOW max payload is 250, overhead is 4 for cmd/src and 1 for target.  Message overhead is 4 bytes for 
	cmd, src node, src task, and 1 byte for target task.  THIS MAY CHANGE IF WE ALLOW MORE THAN 256 TASKS
	
*/
#define TASKMGR_MESSAGE_SIZE (TASKMGR_MAX_PAYLOAD-1-sizeof(tm_nodeId_t)-2*sizeof(tm_taskId_t))
/*x @} */ // ingroup Globals

// Process includes for networking code
#if TM_USING_RADIO
#if defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)
#include "radioDriverRF.h"
#elif  defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#include "radioDriverESP.h"
#endif // architecture selection
#endif // TM_USING_RADIO

/*x	\ingroup ClockSync
	@{
*/
// Clock adjustment routines
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
void TmAdjustClockOffset(unsigned long offsetDelta);
unsigned long TmMillis();
#endif
/*x	@} */ // ingroup ClockSync

/*x	\ingroup Globals
	@{
*/

/*  FIXED TASKS */
/*	\def TASKMGR_MAX_TASK
	\def TASKMGR_MAx_NODE
	\def TASKMGR_MAX_NET
*/
#if defined(ARDUINO_ARCH_AVR)
#define TASKMGR_MAX_TASK 255	//!< Maximum task ID (Atmel AVR architecture)
#define TASKMGR_MAX_NODE 255	//!< Maximum node ID (Atmel AVR architecture)
#define TASKMGR_MAX_NET 255		//!< Maximum network ID (Atmel AVR architecture)
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#define TASKMGR_MAX_TASK 255	//!< Maximum task ID (ESP architecture)
#define TASKMGR_MAX_NODE 65535	//!< Maximum node ID (ESP architecture)
#define TASKMGR_MAX_NET	 255	//!< Maximum network ID (ESP architecture)
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

/*!	\def TASKMGR_CLOCK_SYNC_SERVER_TASK
	The task that receives clock synchronization time requests.
	This runs on the clock server and receives the requests from clients.
	Available on ESP only.
*/
#define	TASKMGR_CLOCK_SYNC_SERVER_TASK (TASKMGR_NULL_TASK-4)

/*!	\def TASKMGR_CLOCK_SYNC_CLIENT_TASK
	The task that receives clock synchronization messages from a clock server.
	This runs on clients.
	Available on ESP only.
*/
#define TASKMGR_CLOCK_SYNC_CLIENT_TASK (TASKMGR_NULL_TASK-5)

/*x @} */ // ingroup Globals

/*x \ingroup TaskManagerTask
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
	/*x \defgroup public Public Member Constants and Fields
		\ingroup TaskManagerTask
	*/
	/*x @{ */
	//!	\name Enums
    /*! \enum TaskStates
        Defines the various flag-bits describing the state of a task
    */
    enum TaskStates {UNUSED01=0x01,   	// RFU
        WaitMessage=0x02,               //!< Task is waiting for a message.
        WaitUntil=0x04,                 //!< Task is waiting until a time has passed.
        UNUSED08 = 0x08,        		// RFU
        AutoReWaitMessage = 0x10,       //!< After normal completion, task will be set to wait for a message.
        AutoReWaitUntil = 0x20,         //!< After normal completion, task will be set to wait until a time has passed.
        TimedOut = 0x40,                //!< Marker that the task had a timeout with a message, and the timeout happened.
        Suspended=0x80                  //!< Task is suspended and will not receive messages or timeouts.
        };
	/*x @} */ // end public
public:
	//!	\name Member Variables
	tm_nodeId_t	m_fromNodeId;		//!< Source node for a message
	tm_taskId_t	m_fromTaskId;		//!< Source task for a message
protected:
    uint8_t m_stateFlags; //!< The task's state information

    // Active delay information.  If a task is waiting, here is the reason (or in the
    // case of messaging, the response)
    unsigned long m_restartTime;  //!< Used by WaitUntil to determine the restart time.
                                    //!< This is compared to the absolute ms clock maintained by the processor
									//!< Updated for net clock: compared to TaskManager.millis() ms clock


    char m_message[TASKMGR_MESSAGE_SIZE+1];   //!<  The message sent to this task (if waiting for a message)
    uint16_t m_messageLength;		//!< The length of a received message

    //NOT USED??? unsigned int m_reTimeout;   //!< The timeout to use during auto restarts.  0 means no timeout.

    // Autorestart information.  If a task has autorestart, here is the information to use at the restart
    unsigned long m_period; //!< If it is auto-reschedule, the rescheduling period

    tm_taskId_t    m_id; //!< This task's task ID
    void    (*m_fn)(); //!< The procedure to be invoked each cycle

public:
	/*x	\defgroup constructors	Constructors and Destructor
		\ingroup TaskManagerTask
	*/
	/*x @{ */
    //!	\name Constructors and destructors
    _TaskManagerTask();
    _TaskManagerTask(tm_taskId_t, void (*)());
    ~_TaskManagerTask();
	
	/*x @} */ // end constructors
protected:
    //!	\name State Tests and Manipulators
    bool anyStateSet(byte);
    bool allStateSet(byte);
    bool stateTestBit(byte);
    void stateSet(byte);
    void stateClear(byte);
    void resetCurrentStateBits();
	
    // Querying its state and other info
    bool isRunnable();

    // Setting its state
    void setRunnable();
	void setSuspended();
	void clearSuspended();
    void setWaitUntil(unsigned long);
    void setWaitDelay(unsigned long);
    void setAutoDelay(unsigned long);
    void setWaitMessage(unsigned long msTimeout=0);
    void setAutoMessage(unsigned long msTimeout=0);

    //!	\name Messaging
    void putMessage(void* buf, int len);

public:
    // Things that make the ring of _TaskManagerTask work
	//!	\name Operators
    _TaskManagerTask& operator=(_TaskManagerTask& rhs);
    bool operator==(_TaskManagerTask& rhs) const;

    // Miscellaneous methods
	//!	\ignore
#if defined(TASKMANAGER_DEBUG)
	size_t printTo(Print& p) const;
#endif
	//!	\endignore
};
/*x @} */ // end TaskManagerTask


/**********************************************************************************************************/

/*x \ingroup TaskManager
 * @{
*/
/*! \class TaskManager
    \brief A cooperative multitasking manager

    Manages a set of cooperative tasks.  This includes round-robin scheduling, yielding, and inter-task
    messaging.  It also replaces the loop() function in standard Arduino programs.  Nominally,
    there is a single instance of TaskManager called TaskMgr.  TaskMgr is used for all actual task control.

    Each task has a taskID.  By convention, user tasks' taskID values are in the range [0 223].
	Task IDs [224 255] are reserved for system and common module tasks.
*/
#if defined(TASKMANAGER_PRINTABLE)
class TaskManager: public Printable {
#else
class TaskManager {
#endif

#if (defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24) && TM_USING_RADIO)
private:
	RF24*	m_rf24;				// Our radio (dynamically allocated)	
#endif
public:
	//! \name Member Variables
    ring<_TaskManagerTask> m_theTasks; //!< The ring of all tasks.  For internal use only.

private:
    unsigned long m_startTime;  // Start clock time.  Used to calculate runtime. For internal use only.

public:
	/*x \ingroup Setup
		@{
	*/
	// Constructor and destructor
	//!	\name Constructors and Destructors
    TaskManager();
    ~TaskManager();
	/*x @} */ // end Setup

    // Things used by yield

private:
    /* Defines the different methods a process may yield control.

        This is the return value passsed back by the different yield*() routines to the control `loop()`.
        It indicates which form of yield*() was called.

        This is for internal use only.
    */
	/*!	\enum YieldTypes
		Different methods a task may use when yielding
	*/		
    enum YieldTypes { YtYield,
        YtYieldUntil,
        YtYieldMessage,
        YtYieldMessageTimeout,
        YtYieldSuspend,
        YtYieldKill
        };
	//!	\ignore
    jmp_buf  taskJmpBuf;    // Jump buffer used by yield.  For internal use only.
	//!	\endignore

public:
	/*x	\ingroup Add
		@{
	*/
	/*	\name Add a Task

		These methods are used to add new tasks to the task list
	*/

    void add(tm_taskId_t taskId, void (*fn)());
    void addWaitDelay(tm_taskId_t taskId, void(*fn)(), unsigned long msDelay);
    void addWaitUntil(tm_taskId_t taskId, void(*fn)(), unsigned long msWhen);
    void addAutoWaitDelay(tm_taskId_t taskId, void(*fn)(), unsigned long period, bool startDelayed=false);
    void addWaitMessage(tm_taskId_t taskId, void(*fn)(), unsigned long timeout=0);
	void addAutoWaitMessage(tm_taskId_t taskId, void(*fn)(), unsigned long timeout=0, bool startWaiting=true);
	/*x @} */ // ingroup Add
	
	/*x \defgroup ingroup Yield
		@{
	*/
    /*!	\name Yield

    	These methods are used to yield control back to the task manager core.
    	@note Upon next invocation, execution will start at the TOP of
    	the routine, not at the statement following the yield.
    	@note A <i>yield</i> call will override any of the <i>addAuto...</i> automatic
    	rescheduling.  This will be a one-time override; later (non-<i>yield</i>) returns
    	will resume automatic rescheduling.
    */
    void yield();
    void yieldDelay(unsigned long ms);
    void yieldUntil(unsigned long when);
    void yieldForMessage(unsigned long timeout=0);
    /*x @} */ // ingroup Yield

	/*x \ingroup Message
		@{
	*/
	/*  \name Sending Messages

		These methods send messages to other tasks running on this or other nodes
		@note If the nodeID is 0 on any routine that sends messages to other nodes,
		the message will be sent to this node.
	*/


    bool sendMessage(tm_taskId_t taskId, char* message);       // string
    bool sendMessage(tm_taskId_t taskId, void* buf, int len);
	/*x	@} */ // end send
#if TM_USING_RADIO && ((defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32))
	bool sendMessage(tm_nodeId_t nodeId, tm_taskId_t taskId, char* message);
	bool sendMessage(tm_nodeId_t nodeId, tm_taskId_t taskId, void* buf, int len);
#endif // using radio && (atmel || esp)


	void getSource(tm_taskId_t& fromTaskId);
#if TM_USING_RADIO && ((defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32))
	void getSource(tm_nodeId_t& fromNodeId, tm_taskId_t& fromTaskId);
#endif // using radio && (atmel || esp)

	/*x @} */ // ingroup Message
	/*  @} */


    /*x	\ingroup Control
		@{ */

	/*!	\name Task Control
		Methods for suspending and resuming tasks
	*/
    bool suspend(tm_taskId_t taskId);					// task
#if TM_USING_RADIO && ((defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32))
	bool suspend(tm_nodeId_t nodeId, tm_taskId_t taskId);			// node, task
#endif	// using radio && (atmel || esp)
	bool resume(tm_taskId_t taskId);					// task
#if TM_USING_RADIO && ((defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32))
	bool resume(tm_nodeId_t nodeId, tm_taskId_t taskId);			// node, task
#endif // using radio && (atmel || esp)

	/*x @} */	// ingroup Control

	/* **** Mesh/Radio Internal Routines */
#if TM_USING_RADIO && ((defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32) )
private:
	tm_nodeId_t m_myNodeId;	//!< The node ID of the current system.  Only available on radio-enabled environments.
	/*x	\ingroup Internal
		@{
	*/

	bool radioSender(tm_nodeId_t);	// generic packet sender

    // status requests/
    //void yieldPingNode(byte);					// node -> status (responding/not responding)
    //void yieldPingTask(byte, byte);				// node, task -> status
    //bool radioFree();						// Is the radio available for use?

	/*x @} */	// ingroup Internal
    // radio
private:
	// notes on parameters to the commands
	//  message: m_data[0] = taskID, m_data[1+] = message
	//  suspend, resume: m_data[0] = taskID
	/*!	\enum RadioCmd
		Operations that are passed in a single byte in the radio packet indicating how the receiving node
		will process the remaining packet data
	*/
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
#endif // esp
public:
	void tmRadioReceiverTask();

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
private:
	esp_err_t m_lastESPError;
public:
	/*! \brief Return a textual description of the last ESP error message.
		\returns A const char* with text describing the last error.
	*/
	const char* lastESPError();
#endif // esp: ESP error info 
#endif // using radio && (atmel || esp) :: mesh/radio internal routines

	/*x \ingroup Network

		These functions are available in the ESP and the RF24-enabled versions of TaskManager.
		They allow the creation of networks of AVR or ESP based nodes.  Tasks running on nodes
		may send/receive messages to/from tasks either on the local node or to any node on the 
		network.  Any operation performable on a task may also be performed on a remote task.
		
		Note that ESP and AVR nodes may not be intermixed on the same network.
		
		Note that there is one unified network space for each of ESP and AVR.  A node may reach
		any other node within radio range.
		@{
	*/
#if defined(DOXYGEN_ALL) || (TM_USING_RADIO && (defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)))
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
#endif
#if defined(DOXYGEN_ALL) || (defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32))
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
#endif // using radio: radio start/stop

#if TM_USING_RADIO && ((defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32) )
	/*!	\brief Return the node ID of this system.
		\return The byte value that is the current node's radio ID.  If the radio has not been
		enabled, returns 0.
		\note This routine is only available on ESP environments.
	*/
    tm_nodeId_t myNodeId();
#endif // using radio && (atmel || esp)
	/*x @} */ // ingroup Network

	/*x \ingroup Setup
	    @{ */

	/*! \brief TaskManager initialization
	*/
	void begin() {}
	/*x @} */ // ingroup Setup
	
	/*x \ingroup Misc
		@{ */
		
	/*! \brief Return the time since the start of the run, in milliseconds
	*/
    unsigned long runtime() const;
	/*x	@) */ // ingroup Misc
	
#if DEBUG
#if defined(TASKMANAGER_DEBUG)
/*!	\ignore DO_NOT_PROCESS */
    size_t printTo(Print& p) const;
/*!	\endignore */
#endif // defined(TASKMANAGER_DEBUG)
#endif // debug

	/*x	\ingroup Message
		@{
	*/
    /*! \brief Tell if the current task has timed out while waiting for a message
    	\return true if the task started due to timing out while waiting for a smessage; false otherwise.
	*/
	bool timedOut();
	
	/*! \brief Get a task's message buffer
		\return A pointer to the actual message buffer.  Use the contents of the buffer but do NOT modify it.
		If a task is killed, this pointer becomes invalid.
	*/
	void* getMessage();

	/*!	\brief Get the length of the message in the buffer
		\return The size of the data block in the buffer.  Note that if the content is a string, the
		size will be one greater than the string length to account for the trailing null.
	*/
	uint16_t getMessageLength();
	
	/*! \brief Return the task ID of the currently running task
		\return The byte value that represents the current task's ID.
	*/
    tm_taskId_t myId();

    // We need a publicly available TaskManager::loop() so our global loop() can use it
	/*x	@} */	// ingroup Message
	
	/*x	\ingroup Misc 
		@{
	*/
    void loop();
	/*x	@} */ // ingroup Misc

	/*x	\ingroup ClockSync
		@{
	*/
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
	/*!	\brief resync Synchronize this node's millis() timer to a remote time server node's millis()
		\param remoteMillis -- the millis() value from a master time server.
	*/
	void resync(unsigned long remoteMillis);
	
	/*!	\brief Synchronized millisecond clock
		This is the virtual network clock value.  It is the local clock, adjusted by the recorded
		offset between the local clock and the network clock.
		\return unsigned long int millisecond clock that is synchronized with the millis()of a clock server.
		Note that if no clock server is defined, TaskManager::millis() returns ::millis().
	*/
	unsigned long int millis() const;
	/*x	@} */ // ingroup ClockSync
#endif // ESP

protected:
	/*x	\ingroup Internal
		@{
	*/
	/*!	\brief Sends a string message to a task on this system.
		Send a null-terminated (string) message to a task running on this system.  This is the internal
		gateway used by both sendMessage("this node") and from the radio receiver module (messages from
		other nodes).
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
	
	/*x	@} */ // ingroup Internal


};
/*x @} */ // end TaskManager

/* **************************************************************************************************
   *    IMPLEMENTATION   																			*
   **************************************************************************************************
*/

/* ******************************************************************************
   *   TASKMANAGERTASK 															*
   ******************************************************************************
*/

/*x \ingroup TaskManagerTask
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

/*!	\brief Standard destructor.

	At this time, this does nothing.
*/
inline _TaskManagerTask::~_TaskManagerTask() {
}

// Setting the task's state

/*!	\brief Set the task to a runnable state.  That is, it will not be suspended or waiting for anything

	Clears the WaitMessage, WaitUntil, and Suspended bits.
*/
inline void _TaskManagerTask::setRunnable() {
    stateClear(WaitMessage+WaitUntil+Suspended);
}

/*!	\brief Set the task to a generic suspended state.  That is, it will not be run until it is set runnable

	Sets the suspended bit in the task flag set.
*/
inline void _TaskManagerTask::setSuspended() {
	stateSet(Suspended);
}

/*!	\brief Clear the task's suspended'ness, if it was suspended.  Note, if it is still waiting on a messagel etc.,
	it will keep waiting.
	
	Clears the Suspended bit in the task flag set.
*/
inline void _TaskManagerTask::clearSuspended() {
	stateClear(Suspended);
}

/*!	\brief Setting the task to wait until a specific time

    This sets the wakeup time and sets the appropriate flag to wait on the clock.  The task ill not
    reactive until after the CPU clock has passed the given time.
    \param when -- the CPU clock time to wait until/after, in ms.
    \sa setWaitUntil
*/
inline void _TaskManagerTask::setWaitUntil(unsigned long when) {
    m_restartTime=when;
    stateSet(WaitUntil);
}

/*!	\brief Setting the task to wait for a specific number of milliseconds

    Thsi sets the task so it will not awaken until at least the specified number of miliseconds have passed.
    \param ms -- The number of milliseconds to wait for.
    \sa setWaitUntil
*/
inline void _TaskManagerTask::setWaitDelay(unsigned long ms)  {
    setWaitUntil(TmMillis()+ms);
}

/*!	\brief Sets the task to automatically restart with the given delay whenever it exits normally (non-yield).

    Sets the task so it will automatically restart with the given delay whenever it exits through either
    a return or by dropping through the bottom of the routine.  Using any form of yield() will override
    the auto-restart.  If the auto-restart runs, the next execution will be delayed by the
    given number of milliseconds.

    Note that this will not set any delay for the first invocation of the task.  That will need to be done
    separately.  Note that this can be combined with setAutoMessage to create a task
    that will auto-restart with a wait-for-message-or-timeout.
    \param ms -- The number of milliseconds that must pass before the task restarts.  A value of zero
    will cause the auto-delay to be ignored.
    \sa setWaitDelay, setAutoMessage
*/
inline void _TaskManagerTask::setAutoDelay(unsigned long ms) {
    if(ms>0) {
        m_period = ms;
        stateSet(AutoReWaitUntil);
    }
}

/*!	\brief Set the task to wait for a message, with an optional timeout.

    Sets the task to wait for a message.  If a timeout (in ms) is specified and is greater than 0, the task will
    reactivate after that time if a message has not been received.  If no timeout (or a zero timeout) was specified,
    the task will wait forever until the message is received.
    \param msTimeout -- The timeout on the message.  Optional, default is zero.  If zero, then there is no timeout.
*/
inline void _TaskManagerTask::setWaitMessage(unsigned long msTimeout/*=0*/)  {
    stateSet(WaitMessage);
    if(msTimeout>0) setWaitDelay(msTimeout);
}

/*!	\brief Sets the task to automatically restart waiting for a message
    whenever it exits normally (non-yield).

    Sets the task so it will automatically restart waiting for a message whenever it exits through either
    a return or by dropping through the bottom of the routine.  Using any form of yield() will override
    the auto-restart.  If the auto-restart runs, the next execution will not happen until a message arrives.

    Note that this will not set the task as being active or in a wait-state
    for the first invocation of the task.  That will need to be done
    separately.

    Note that this can be combined with setAutoDelay to create a task
    that will auto-restart with a wait-for-message-or-timeout.
    \param msTimeout -- The timeout on th emssage.  Optional, defau
    \sa setWaitDelay, setAutoMessage
*/
inline void _TaskManagerTask::setAutoMessage(unsigned long msTimeout/*=0*/)  {
    stateSet(AutoReWaitMessage);
    setAutoDelay(msTimeout);
}

//
// Sending messages to a task
//

/*!	\brief Send an actual message to the task

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
/*!	\brief Tells if any of the specified bits are set (1)
    \param bits -- The bits to test
    \return false if all of the bits are clear (0), true if any of the bits are set (1)
*/
inline bool _TaskManagerTask::anyStateSet(byte bits) {
    return (m_stateFlags&bits) != 0;
}

/*!	\brief Tell if all of the flag bits are set
    \param bits -- The bits to test
    \return true if all of the bits are set (1), false if any are not set (0)
*/
inline bool _TaskManagerTask::allStateSet(byte bits) {
    return (m_stateFlags&bits) == bits;
}

/*!	\brief Test a single bit
    \param  bit -- bit to test
    \return true if the bit is set(1), false if it is not set (0)
*/
inline bool _TaskManagerTask::stateTestBit(byte bit) {
    return anyStateSet(bit);
}

/*!	\brief Set a bit or bits
    \param bits -- the bits to set
*/
inline void _TaskManagerTask::stateSet(byte bits) {
    m_stateFlags |= bits;
}

/*!	\brief Clear a bit or bits
    \param bits - the bits to clear
*/
inline void _TaskManagerTask::stateClear(byte bits) {
    m_stateFlags &= (~bits);
}

/*!	\brief Resets bits (note: not sure of purpose)

	Clears many of the state bits.  This needs to be rewritten in terms of the enum values in 
	case things shift around internally.
*/
inline void _TaskManagerTask::resetCurrentStateBits() {
	m_stateFlags = (m_stateFlags&0xf8) + ((m_stateFlags>>3)&0x07);
}

 /*x @} */ // ingroup TaskManagerTask
 

/* **********************************************
   *   TASKMANAGER IMPLEMENTATION				* 															*
   **********************************************
*/



/*x \ingroup TaskManager
 * @{
*/

/* \ingroup  Message 
	@{
*/
inline tm_taskId_t TaskManager::myId() {
	return m_theTasks.front().m_id;
};
/* @} */ // end task
/*x \ingroup Message
	@{
*/
inline bool TaskManager::timedOut() {
    return m_theTasks.front().stateTestBit(_TaskManagerTask::TimedOut);
}
/*x @} */ // end Message
/*x \ingroup Message
	@{
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
inline bool TaskManager::sendMessage(tm_taskId_t taskId, char* message) {
	internalSendMessage(0, myId(), taskId, message);
	return true;
}

/*! \brief Send a binary message to a task

	Sends a message to a task.  The message will go to only one task.
	Messages that are too large are	ignored.

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
inline bool TaskManager::sendMessage(tm_taskId_t taskId, void* buf, int len) {
	internalSendMessage(0, myId(), taskId, buf, len);
	return true;
}
/*x	@} */ // end Message
/*x \ingroup Message
	@{
*/
inline void* TaskManager::getMessage() {
    return (void*)(&(m_theTasks.front().m_message));
}

inline uint16_t TaskManager::getMessageLength() {
	return m_theTasks.front().m_messageLength;
}

/*!	\brief Get task ID of last message's sender

	Returns the taskId of the task that last sent a message
	to the current task.  If the current task has never received a message, returns [0].

	\param[out] fromTaskId -- the taskId that sent the last message.
*/
inline void TaskManager::getSource(tm_taskId_t& fromTaskId) {
	fromTaskId = m_theTasks.front().m_fromTaskId;
}
/*x @} */ // end Message
/*x \ingroup Misc		
	@{
*/
/*!	\brief Return the time (in ms) since the system has been restarted.

	Note this time is in absolute ms, and is independent of the network synchronization clock (should
	the network synchronization clock be used).
*/
inline unsigned long TaskManager::runtime() const { return ::millis()-m_startTime; }
/*x @} */ // end Misc
 
/**************** Network ************************/
/*x \ingroup TaskManager
 *	@{
*/
#if TM_USING_RADIO && ((defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32) )
inline tm_nodeId_t TaskManager::myNodeId() {
	return m_myNodeId;
}
#endif // using radio && (atmel || esp)

/*x @} */ // End ingroup TaskManager


#endif // __TASKMANAGERCORE_H_INCLUDED__
