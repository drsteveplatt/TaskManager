#ifndef TASKMANAGERRFCORE_H_INCLUDED
#define TASKMANAGERRFCORE_H_INCLUDED

#include <TaskManagerCore.h>

//#include "Streaming.h"
//#include "ring.h"
//#include <setjmp.h>

/*! \file TaskManagerCore.h
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



/**********************************************************************************************************/

/*! \class TaskManagerRF
    \brief Adds RF24 abilities to TaskManager

    Manages a set of cooperative tasks.  This includes round-robin scheduling, yielding, and inter-task
    messaging and signaling.  It also replaces the loop() function in standard Arduino programs.  Nominally,
    there is a single instance of TaskManager called TaskMgr.  TaskMgr is used for all actual task control.
	Each node has a nodeID.  nodeID=0 is 'the current node'.
    Each task has a taskID.  By convention, user tasks' taskID values are in the range [0 127].
*/

class TaskManagerRF: public TaskManager {

private:
	RF24*	m_rf24;				// Our radio (dynamically allocated)
	int		m_myNodeId;			// radio node number. 0 if radio not enabled.

public:
	// Constructor and destructor
	// Not included in doxygen documentation because the
	// user never constructs or destructs a TaskManager object
	/*! \brief Constructor,  Creates an empty task
	*/
    TaskManagerRF();
    /*!  \brief Destructor.  Destroys the TaskManager.

	    After calling this, any operations based on the object will fail.  For normal purpoases, destroying the
	    TaskMgr instance will have serious consequences for the standard loop() routine.
	*/
    ~TaskManagerRF();

public:

	/*!	@name Sending Signals and Messages

		These methods send signals or messages to other tasks running on this or other nodes
		@note If the nodeID is 0 on any routine that sends signals/messages to other nodes,
		the signal/message will be sent to this node.
	*/
	/*! @{ */

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

	void sendSignalAll(byte nodeId, byte sigNum);

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

		Resumes a task on any node.  If nodeID==0, it resumes a task on this node.  If the node or task
		do not exist, nothing happens.  If the task had not been suspended, nothing happens.
		\param nodeId The node containnig the task
		\param taskId The task to be resumed
		\note Not implemented.
		\sa suspend()
	*/
	void resume(byte nodeId, byte taskId);			// node, task
	/*! @} */


private:
	void radioSender(byte);	// generic packet sender

    // status requests/
    //void yieldPingNode(byte);					// node -> status (responding/not responding)
    //void yieldPingTask(byte, byte);				// node, task -> status
    //bool radioFree();						// Is the radio available for use?

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

	/*! @name Miscellaneous and Informational Routines */
	/*! @{ */

	/*!	\brief Return the node ID of this system.
		\return The byte value that is the current node's radio ID.  If the radio has not been
		enabled, returns 0.
	*/
    byte myNodeId();
    /*! @} */
};

//
// Defining our global TaskMgr
//
/*!	\brief The global object used to access TaskManager functionality
*/
//#if !defined(TASKMANAGER_MAIN)
//extern TaskManager TaskMgr;
//#else
//TaskManager TaskMgr;
//#endif

//
// Inline stuff
//

inline byte TaskManagerRF::myNodeId() {
	return m_myNodeId;
}


#endif // TASKMANAGERRFCORE_H_INCLUDED


