// TaskManagerClockSync standard header
// 

//!	\file TaskManagerClockSync.h

#if !defined(__TASKMANAGERCLOCKSYNC_H__)
#define __TASKMANAGERCLOCKSYNC_H__

//#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)

/*!	\defgroup ClockSync Mesh Clock Synchronization
	@{
*/
/*!	\def TASKMGR_CLOCK_SYNC_SERVER_NODE
	If clock synchronization is used, then the clock sync server must run on this nodeId.
*/
#define TASKMGR_CLOCK_SYNC_SERVER_NODE (TASKMGR_MAX_NODE-1)

/*!	\class _TaskManagerClockSyncInfo
	\brief Information passed between the clocksync client and clocksync server
*/
class _TaskManagerClockSyncInfo {
	public:
		unsigned long int m_id;	//!< Verifies server response matches client request
		unsigned long int m_serverTime; //!< Absolute time as reported by the clock sync server
};

/*!	\brief User-invokable task to act as a clock synchronization server
	
	This task should be invoked on one node (a project agreed-upon time server node, note it
	can also supply other services but should not do anything time-intensive).  It should be
	invoked with TaskMgr.addAutoWaitMessage(TASKMGR_CLOCK_SYNC_SERVER_TASK, TmClockSyncServerTask);
	It will wait for messages from clients requesting the master clock and respond with the master
	clock (sys::millis()).
	
	Note that the nominal time is around 3ms in each direction, so the recipient can normally add 3
	to the reply value to get a better estimate of what the server time actually is when the message
*/
void TmClockSyncServerTask();

/*!	\brief User-invokable task on client systems to request synchronization with a server
	
	This task should be invoked on each node requesting synchronization with a master node.
	Normally, it is invoke with code such as
	`	#define SYNCDELAY (60000L)
	`	TaskMgr.addAutoWaitDelay(TASKMGR_CLOCK_SYNC_CLIENT_TASK, TmClockSyncClientTask, SYNCDELAY);
*/
void TmClockSyncClientTask();
/*!	@} */ // ingroup ClockSync
//#endif // arch is esp
#endif // __TASKMANAGERCLOCKSYNC_H__
