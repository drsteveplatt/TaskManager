//  TaskManagerMainpage.h
//
// This file contains the body of the mainpage for Doxygen
//

//!	\file TaskManagerMainpage.h

/*!
	\mainpage TaskManger - Cooperative Multitasking System for Arduino

	\section Overview

	TaskManager is a cooperative task manager for the Arduino family of processors.
	It replaces the single user \c loop() routine with an environment in which the user
	can write many independent \c loop() -style routines.  These user routines are run in a
	round-robin manner.  In addition, routines can
	@li Delay -- suspending their operation for a specified period while allowing
	other routines to make use of the time
	@li Message -- suspend action until a message has been received, or send messages to
	different tasks to pass them information.

	\section News

	2022/05/31: Release 2.0: Merging Atmel and ESP32 branches.  
	<br>ESP32 will include multi-node (mesh) routines
	to send/receive messages between nodes.  Atmel systems will need to use TaskManagerRF version 2.0 to 
	support this functionality.
	2015/11/13: Release 1.0: Initial full release.
	<br>More code cleanup, improved documentation.  Added routines so tasks could ID where messages came from.
	may be a while until they are...

	\section Summary
	TaskManager is a cooperative multitasking task-swapper.  It allows the developer to create many independent
	tasks, which are called in a round-robin manner.
	<br>TaskManager offers the following:
	\li Any number of tasks.
	\li Extends the Arduino "setup/loop" paradigm -- the programmer creates several "loop" routines (tasks)
	instead of one.
	So programming is simple and straightforward.
	\li Tasks can communicate through messages.  A message has information (string or data), 
	and is passed to a particular task.
	\li TaskManager programs can use RF24 2.4GHz radios to communicate between nodes.  So tasks running on
	different nodes can communicate through messages in the same manner as if they were on the
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
