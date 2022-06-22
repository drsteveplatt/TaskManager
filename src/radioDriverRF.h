//

// Low level drivers for RF24 radio communications
//  This includes everything from radioBegin down.
//  Primarily, this includes radio initialization and control
//  and the low-level sending and receiving of messages (everything
//  from transmitting the raw data to the library routines to
//	send data to the other node, to polling/responding to interrupts
//  for received data, buffering the data, and placing it into
//  the targeted task's message buffer).

#if !defined(__TASKMANAGER_RF24)
#define __TASKMANAGER_RF24)

// Only use this if AVR and using RF24
#if defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)

#include <RF24.h>

/*!	\defgroup TaskManagerRadioRF TaskManager AVR RF24 Network/Mesh Drivers
	@{
*/

/*!	_TaskManagerRadioPacket
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
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
// This is where we build MAC data for setting our MAC and pairing setup
// It has enough constant data that it is easier to just keep one around.
static byte _TaskManagerMAC[] = { 0xA6, 'T', 'M',  0, 0x00, 0x00 };
#endif

/*! @}
*/

#endif // AVR and RF24

#endif // __TASKMANAGER_RF24
