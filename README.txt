
TaskManager is a cooperative multitasking task-swapper. It allows the developer to 
create many independent tasks, which are called in a round-robin manner. 

TaskManager offers the following: 
•	Any number of tasks. 
•	Extends the Arduino "setup/loop" paradigm – the programmer creates several 
	"loop" routines (tasks) instead of one. So programming is simple and  
	straightforward. 
•	Tasks can communicate through signals or messages. A signal is an information-free  
	"poke" sent to whatever task is waiting for the poke. A message has information  
	(string or data), and is passed to a particular task. 
•	TaskManager programs can use RF24 2.4GHz radios to communicate between nodes.  
	So tasks running on different nodes can communicate through signals and messages  
	in the same manner as if they were on the same node.

Release History:

	2017/1/2:  Release 1.2: Split RF code out of base Taskmanager.
	This simplifies its use and reduces some memory consumption.
	Use <TaskManager.h> for basic, single-node functionality
	Use <TaskManagerRF.h> to include the RF24 routines.
	The documentation still needs updating.

	2015/11/13: Release 1.0: Initial full release.
	More code cleanup, improved documentation.  Added routines so tasks could ID
	where messages/signals came from.

	2015/10/15: Prerelease 0.2.1:  RF24 routines added.  Code cleaned up.
	Routines include beginRadio() and versions of sendSignal(), sendSignalAll(),
	and sendMessage(). The send*() routines add a parameter to specify the nodeId 
	the signal/message is being sent to.
	
	Additionally, the code was refactored for clarity.

	2015/07/30: Prerelease 0.2:  Updated functionality of addAutoWaitDelay().
	The addAutoWaitDelay() routine will operate on a repeat time based on the original 
	start time, not on the end time.  For example, if the task starts at 1000 with 
	addAutoDelay(500) and it takes 100 (all ms), it will start at 1000, 1500,
	2000, etc.  If it used yieldDelay, the yield is based on the end-time of the task, 
	so it will start at 1000, 1600 (1000+100(run)+500(delay)), 2200 (1600+100(run)+500(delay)), 
	etc.
	
	An excellent suggestion from the alpha user group.

	2015/01/15: Internal (SA-28) Pre-release:  Core functionality complete.
	This is an alpha release for a small group of users.  Most of the non-RF functionality 
	has been implemeented. suspend() and resume() not implemented yet.  It	may be a 
	while until they are...