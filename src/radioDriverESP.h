//
// Low level drivers for ESP radio communications
//  This includes everything from radioBegin down.
//  Primarily, this includes radio initialization and control
//  and the low-level sending and receiving of messages (everything
//  from transmitting the raw data to the library routines to
//	send data to the other node, to polling/responding to interrupts
//  for received data, buffering the data, and placing it into
//  the targeted task's message buffer).

#if !defined(__TASKMANAGER_ESP)
#define __TASKMANAGER_ESP

// Only use this if AVR and using RF24
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

/*!	\defgroup TaskManagerRadioESP TaskManager ESP Network/Mesh Drivers
	@{
*/
// How many messages can our interrupt manager save (between processing by the radio receiver task)
// before discarding messages
/*! \def TASKMGR_MESSAGE_QUEUE_SIZE
	The number of messages that can be buffered before the radio receiver task is called.
	Any more than this and messages are discarded.
*/
#define TASKMGR_MESSAGE_QUEUE_SIZE 50

/*!	\struct	_TaskManagerRadioPacket
	A packet of information being sent by radio between two TaskManager nodes
	
	Note this struct is always defined in ESP implementations, but is only defined in RF24-enabled
	implementations.
*/
struct _TaskManagerRadioPacket {
	byte	m_cmd;							//!< Command information
	tm_nodeId_t	m_fromNodeId;						// source node
	tm_taskId_t	m_fromTaskId;						// source task
	byte	m_data[TASKMGR_MESSAGE_SIZE+1];	//! The data being transmitted.
} __attribute__((packed));

// This is where we build MAC data for setting our MAC and pairing setup
// It has enough constant data that it is easier to just keep one around.
static byte _TaskManagerMAC[] = { 0xA6, 'T', 'M',  0, 0x00, 0x00 };


/*! @}
*/

#endif // ESP

#endif // __TASKMANAGER_ESP

