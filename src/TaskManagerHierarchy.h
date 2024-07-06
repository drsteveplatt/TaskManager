// This is the TaskManager general hierarchy for groups and subgroups
// This file contains \defgroup commands defining all of our groups,
// \ingroup commands defining the group hierarchies, \brief giving summaries, 
// and general text defining each group (for Detailed Description sections).
//
// No other files should define groups.  They should just refer to these group names.

/*x
	\defgroup TaskManager TaskManager
	\brief A lightweight task swapping system for Atmel and ESP Arduino environments.
	
	TaskManager provides a lightweight task swapping environment.  Instead of a single large loop(), the
	developer creates individual independent loop-like tasks that manage distinct activities.  Tasks may
	auto-schedule themselves around the system clock and communicate using simple messaging.
	
	Basic networking is also supported, allowing meshes of Atmel or ESP nodes to communicate (including messaging)
	using RF24 or ESP-NOW protocols.
	
	\defgroup Globals Global Values 
	\ingroup TaskManager
	\brief A summary of available global values
	
	This lists the reserved/global constants within TaskManager.  Globals are divided into major sections:
    - System constants, such as maximum message size
	- Types, such as tm_taskId, a variable to hold a task identifier
	- Reserved task IDs, the task IDs that should not be used in a running environment
	- Reserved node IDs, the node IDs that should not be used in a running mesh.
	- TaskMgr, the global TaskManager object used to manage the task swapper and its environment.
	
	\defgroup Constants Constants
	\ingroup TaskManager
	\brief TaskManager system-wide constants
	
	\defgroup Classes Classes
	\ingroup TaskManager
	\brief Classes used to implement the TaskManager core
	
	\defgroup Ring The ring class
	\ingroup Classes
	\brief A simple STL-like ring paralleling a subset of list and similar STL collections
	
	\defgroup TaskManagerTask TaskManager Core Tasks
	\ingroup Classes
	\brief These tasks contain the information to manage individual tasks and the set of all tasks.
	
	\defgroup General General Methods
	\ingroup TaskManager
	\brief General-purpose routines
	
	\defgroup Setup Setting up TaskManager
	\ingroup General
	\brief Routines to define the overall TaskManager environment
	
	\defgroup Add Adding Tasks
	\ingroup General
	\brief Adding new tasks
	
	\defgroup Yield Yielding
	\ingroup General
	\brief Yielding control to other tasks
	
	\defgroup Message Messaging Other Tasks
	\ingroup General
	\brief Sending messages to other tasks
	
	\defgroup Control Controlling Tasks
	\ingroup General
	\brief Pausing, resuming, and killing tasks
	
	\defgroup Internal Internal Member Elements
	\ingroup General
	\brief Various internal objects
	
	\defgroup Misc Miscellaneous functions
	\ingroup General
	\brief Various functions that will be sorted out later
	
	\defgroup Network Setting up a Mesh Networking
	\ingroup General
	\brief Configuring a mesh networking
	
	\defgroup Macros TaskManager Macros 
	\ingroup General
	\brief Macros to enhance complex tasks
	
	\defgroup Modules Other Useful Modules
	\ingroup TaskManager
	\brief Other modules that can be added into an operating environment
	
	\defgroup ClockSync Synchronizing millis() Clocks in a Mesh
	\ingroup Modules
	\brief Allows different nodes in a mesh to operate on the same millis() value as a master system.
	
	\defgroup Shell A UI Command Line Shell
	\ingroup Modules
	\brief An extensible command line shell for console-like interaction

*/