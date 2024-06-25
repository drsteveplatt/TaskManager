//
// TaskManagerClockSync implementation

#include <TaskManagerSub.h>
#include <TaskManagerMacros.h>
#include <TaskManagerClockSync.h>

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
/*!	\ingroup Clock Synchronization
	@{
*/
/*!	\brief TmClockSyncServerTask
	A simple server that responds to synchronization message requests.
	
	It must run on the node TASKMGR_CLOCK_SYNC_SERVER_TASK.  It should be started with
	AddAutoWaitMessage(TASKMGR_CLOCK_SYNC_SERVER_TASK, TmClockSyncServerTask, 50);
	
	50 is the number of ms to wait between calls; use larger or smaller values as needed.
*/
void TmClockSyncServerTask() {
	_TaskManagerClockSyncInfo myInfo;
	tm_nodeId_t fromNode;
	tm_taskId_t fromTask;
	TaskMgr.getSource(fromNode, fromTask);
	memcpy(&myInfo, TaskMgr.getMessage(), sizeof(_TaskManagerClockSyncInfo));
	myInfo.m_serverTime = ::millis();
	TaskMgr.registerPeer(fromNode);
	TaskMgr.sendMessage(fromNode, fromTask, &myInfo, sizeof(_TaskManagerClockSyncInfo));
	TaskMgr.unRegisterPeer(fromNode);
	//Serial.printf("Replying to node/task %d/%d, sending local time %ld seq %lu\n", fromNode, fromTask, myInfo.m_serverTime, myInfo.m_id);
}

/*!	\brief TmClockSyncClientTask
	A TaskManager task that will resync the TaskMgr::millis() clock every time TmClockSyncClientTask
	is invoked.
	
	To use: A client node schedules the task to run periodically: TaskMgr.addAutoWaitDelay(myTaskId, TmClockSyncClientTask, 60*1000);
	to have it run every minute.
	
	Note this task may consume up to 50ms or so -- if the reply ID doesn't sync or if the server times out, the client will pause
	and repeat.
*/
void TmClockSyncClientTask() {
	static _TaskManagerClockSyncInfo myInfo, theReply;
	static unsigned long int seq = 0;		// sequencer, to keep our request/replies straight if things get lost
	static int i;
	TM_BEGIN();
	// Flush the incoming message queue

	// Then five tries at a good transmit-receive
	for(i=0; i<5; i++) {
		while(true) {
			TM_YIELDMESSAGETIMEOUT(3, 5);
			if(TaskMgr.timedOut()) break;
			memcpy(&theReply, TaskMgr.getMessage(), sizeof(_TaskManagerClockSyncInfo));
			TM_YIELDDELAY(4, 5);
		}
		seq++;
		myInfo.m_id = seq;
		TaskMgr.registerPeer(TASKMGR_CLOCK_SYNC_SERVER_NODE);
		TaskMgr.sendMessage(TASKMGR_CLOCK_SYNC_SERVER_NODE, TASKMGR_CLOCK_SYNC_SERVER_TASK, &myInfo, sizeof(_TaskManagerClockSyncInfo));
		TaskMgr.unRegisterPeer(TASKMGR_CLOCK_SYNC_SERVER_NODE);
		// new code: keep eating messages until a timeout (msg queue exhausted) or until we get a matching message
		while(true) {
			TM_YIELDMESSAGETIMEOUT(1, 20);
			if(TaskMgr.timedOut()) {
				break;
			} else {
				memcpy(&theReply, TaskMgr.getMessage(), sizeof(_TaskManagerClockSyncInfo));
				if(theReply.m_id==seq) {
					// good response, process and exit the loop
					TaskMgr.resync(theReply.m_serverTime+3);
					break;
				} else {
					// bad seq number, ignore and try again
					TM_YIELDDELAY(2,10);
				}
			}
		}
#if false
		TM_YIELDMESSAGETIMEOUT(1, 20);
		if(!TaskMgr.timedOut()) {
			// check the reply for the correct seq, if so, done!)
			memcpy(&theReply, TaskMgr.getMessage(), sizeof(_TaskManagerClockSyncInfo));
			if(theReply.m_id==seq) {
				// we have a reply!! add 3 ms to the server's clock (to account for the unloaded/typical 2ms to send a message)
				// and then reset the offset.
				TaskMgr.resync(theReply.m_serverTime+3);
				break;
			} else {
			}// end good reply else(bogus) processing
		} else {
			// timed out
		}
#endif // false
	}	// end for i=0..4 try to get a message across
	// if we get here, we either did a correct request/reply/resync or we failed to connect.  Regardless, wait until next time
	// to try again.
	TM_END();
}
	/*!	@} */ // end clock synchronization group
#endif // ESP

