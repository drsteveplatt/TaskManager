//
// TaskManager Macros
//
// Used to implement in-process calls to TaskManager routines
// such as Yield, and continuing on the next statement
//
// Based on a complete abuse of the switch(){} statement, akin to the Duff Device
//

/*!	\file TaskManagerMacros.h
	Header for TaskManager Macros
*/

#if !defined(TASKMANAGERMACROS_DEFINED)
#define TASKMANAGERMACROS_DEFINED

/*!	@name Task Manager Macros

	These macros allow a task to yield in the middle, resuming
	at the next statement.  They should be used only under very precise
	control.
*/
/* @{ */

/*!	\brief Prepare the procedure for the TM macros

	TM_BEGIN() creates the initial code enabling the macros to execute correctly.
	It should be placed at/near the start of the procedure.  No TM_YIELD*() calls
	can come before it
*/
#define TM_BEGIN()							\
	static unsigned int __tmNext__ = 0;			\
	switch(__tmNext__) {					\
		case 0:
// for compatibility with older code
#define TM_INIT() TM_BEGIN()

/*!	\brief	Close out the TM macro component

	TM_END() balances TM_BEGIN(), closing the control structures TM_INIT() had
	created.  It should be placed near the bottom of the procedure.  No TM_YIELD*()
	calls can come after it.
*/
#define TM_END()						\
		default:	break;					\
	}										\
	__tmNext__ = 0;
// Compatibility with older code
#define	TM_CLEANUP() TM_END()

/*!	\brief	Yield and return to the next statement

	Perform a yield() operation.  Upon return to this task, execution will continue at
	the next statement.
	\param n -- an integer label.  The label should be unique for all of the TM_YIELD*()
	routines in this task.
*/
#define TM_YIELD(n)	{						\
			__tmNext__ = n;					\
			TaskMgr.yield();				\
		case n:  ;   }

/*!	\brief	Yield with a delay and return to the next statement

	Perform a yieldDelay(ms) operation.  Upon return to this task, execution will continue at
	the next statement.
	\param n -- an integer label.  The label should be unique for all of the TM_YIELD*()
	routines in this task.
	\param ms -- a long integer value representing the time (in ms) for the delay.
*/
#define TM_YIELDDELAY(n,ms)	{				\
			__tmNext__ = n;					\
			TaskMgr.yieldDelay(ms);			\
		case n: ; }

/*!	\brief	Yield until a signal is received, and then return to the next statement

	Perform a yieldForSignal(sigNum) operation.  Upon return to this task, execution will continue at
	the next statement.
	\param n -- an integer label.  The label should be unique for all of the TM_YIELD*()
	routines in this task.
	\param sigNum -- a byte value representing the signal the task will be waiting for.
*/
#define TM_YIELDSIGNAL(n,sig)	{			\
			__tmNext__ = n;					\
			TaskMgr.yieldForSignal(sig);	\
		case n: ; }

/*!	\brief	Yield until a message has been received, and then return to the next statement

	Perform a yieldForMessage() operation.  Upon return to this task, execution will continue at
	the next statement.
	\param n -- an integer label.  The label should be unique for all of the TM_YIELD*()
	routines in this task.
*/


#define TM_YIELDMESSAGE(n)	{				\
			__tmNext__ = n;					\
			TaskMgr.yieldForMessage();		\
		case n: ; }

/*!	\brief	Yield until a signal is received or a time has passed, and then return to the next statement

	Perform a yieldForSignal(sigNum, timeout) operation.  Upon return to this task, execution will continue at
	the next statement.
	\param n -- an integer label.  The label should be unique for all of the TM_YIELD*()
	routines in this task.
	\param sigNum -- a byte value representing the signal the task will be waiting for.
	\param msTimeout -- the maximal amount of time the task will wait for the signal.
*/
#define TM_YIELDSIGNALTIMEOUT(n,sig,msto)	{   \
			__tmNext__ = n;					    \
			TaskMgr.yieldForSignal(sig,msto);	\
		case n: ; }

/*!	\brief	Yield until a message is received or a time has passed, and then return to the next statement

	Perform a yieldForMessage(timeout) operation.  Upon return to this task, execution will continue at
	the next statement.
	\param n -- an integer label.  The label should be unique for all of the TM_YIELD*()
	routines in this task.
	\param msTimeout -- the maximal amount of time the task will wait for the signal.
*/
#define TM_YIELDMESSAGETIMEOUT(n,msto)	{	\
			__tmNext__ = n;					\
			TaskMgr.yieldForMessage(msto);	\
		case n:  ; }

/*! @} */

/*!	@name Task Manager Callable Subtasks
	These macros allow for the definition of a TM subtask -- a procedure
	that can be called from task.

	Each subtask is identified by a unique ID.  This ID is both a signal ID and a task ID.
	It should not be used by any other subtask or for any other signalling purposes
*/
/*! @{ */

/*!	\brief	Define a callable subtask
	TM_ADDSUBTASK() defines the task and prepares it to wait for activation.
	It should be used instead of any of the TaskManager.add*() routines.

	Note that the 'void subtask(){...}' will need to be defined elsewhere.
	Note that this is an internal function and may not be available in later releases
*/
#define TM_ADDSUBTASK(id, task) TaskMgr.addAutoWaitMessage(id, task);

/*!	\brief Procedure definition header for subtask
	TM_BEGINSUB() is used at the start of a subtask procedure.
	This procedure should be called using TM_CALL().

*/
#define TM_BEGINSUB()						\
	static byte __callingTask__; 			\
	TM_BEGIN();								\
	TaskMgr.getSource(__callingTask__);

/*!	\brief Procedure definition header for a subtask with parameters.
	TM_BEGINSUB_P(vtype, vlocal) is used at the start of a subtask procedure that is
	expecting a parameter of type vtype.  This procedure should be called using
	TM_CALL_P().
	\param vtype - the type of the parameter (normally a struct/class)
	\param vlocal - the name of the procedure-local variable of type vtype.

	Note that TM_BEGINSUB_P() will define a variable vlocal of type vtype.
*/
#define TM_BEGINSUB_P(vtype, vlocal)		\
	static vtype vlocal;					\
	TM_BEGINSUB();							\
	memcpy((void*)&vlocal, TaskMgr.getMessage(), sizeof(vtype));

/*!	\brief Return from a subtask
	TM_RETURNSUB() is used to return from a procedure.  Note that all subtasks MUST use
	TM_RETURNSUB() -- bare returns are not allowed.  TM_RETURNSUB() can be used
	from multiple places within a subtask
*/
#define TM_RETURNSUB()  { TaskMgr.sendMessage(__callingTask__, NULL, 0); return; }

/*!	\brief Call a subtask
	TM_CALL() calls a subtask.  When the subtask has been completed (via TM_SUBTASK_RETURN()), the
	calling routine will resume.
	\param n - a unique (within the procedure) value
	\param taskId - the taskId that is to be called
*/
#define TM_CALL(n, taskId) { TaskMgr.sendMessage(taskId, NULL, 0); TM_YIELDMESSAGE(n); }

/*!	\brief Call a subtask, passing a parameter block
	TM_CALL_P() calls a subtask.  When the subtask has been completed (via TM_SUBTASK_RETURN()), the
	calling routine will resume.  This version of CALL allows a single parameter, normally a
	struct/class object.  The size of the object must be less than or equal to the size of
	a message.

	Both input and output parameters may be specified.  Output parameters (return values) can be
	specified by including a pointer to a different object within the passed parameter, and manipulating
	this object through the pointer.
	\param n - a unique (within the procedure) value
	\param taskId - the taskId that is to be called
	\param vparam - the object being passed
*/
#define TM_CALL_P(n, taskId, vparam)										\
	{ 	TaskMgr.sendMessage(taskId, (void*)&vparam, sizeof(vparam));		\
		TM_YIELDMESSAGE(n);													\
	}

/*! \brief End a subtask
	TM_ENDSUB()
	Used at the bottom of a subtask.
*/
#define TM_ENDSUB() 								\
		default:	break;							\
	}												\
	TaskMgr.sendMessage(__callingTask__, NULL, 0);	\
	__tmNext__ = 0;
/*! @} */
#endif