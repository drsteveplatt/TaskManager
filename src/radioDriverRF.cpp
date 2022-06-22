//
// Low level drivers for RF24 radio communications
//  This includes everything from radioBegin down.
//  Primarily, this includes radio initialization and control
//  and the low-level sending and receiving of messages (everything
//  from transmitting the raw data to the library routines to
//	send data to the other node, to polling/responding to interrupts
//  for received data, buffering the data, and placing it into
//  the targeted task's message buffer).

// Only use this if AVR and using RF24
#if defined(ARDUINO_ARCH_AVR) && defined(TASKMGR_AVR_RF24)

#include <arduino.h>
#include <TaskManagerCore_2.h>
#include "radioDriverRF.h"

extern TaskManager TaskMgr;
static void radioReceiverTask() {
	TaskMgr.tmRadioReceiverTask();
}

void TaskManager::tmRadioReceiverTask() {
	static int cnt=0;
	// polled receiver -- if there is a packet waiting, grab and process it
	// receive packet from NRF24.  Poll and process messages
	// We need to find the destination task and save the fromNode and fromTask.
	// They are saved on the task instead of the TaskManager object in case several
	// messages/signals have been received.
	static int packetCount = 1;
	while(m_rf24->available()) {
		// read a packet
		m_rf24->read((void*)(&radioBuf), sizeof(radioBuf));
		//Serial.print("Packet "); Serial.print(packetCount++); Serial.print(" ");
		//Serial.println((int)radioBuf.m_cmd);
		// process it
		switch(radioBuf.m_cmd) {
			case tmrNoop:
				break;
			case tmrStatus:	// NYI
				break;
			case tmrAck:	// NYI
				break;
			case tmrTaskStatus:	// NYI
				break;
			case tmrTaskAck:	// NYI
				break;
			case tmrMessage:
				internalSendMessage(radioBuf.m_fromNodeId, radioBuf.m_fromTaskId,
					radioBuf.m_data[0], &radioBuf.m_data[1], TASKMGR_MESSAGE_SIZE);
				break;
			case tmrSuspend:
				TaskManager::suspend(radioBuf.m_data[0]);
				break;
			case tmrResume:
				TaskManager::resume(radioBuf.m_data[0]);
				break;
		}
	}
}

// General purpose sender.  Sends a message somewhere (varying with the kind of radio)
static byte nodeName[6] = "xTMGR";	// shared buf
bool TaskManager::radioSender(tm_nodeId_t destNodeID) {
	// send packet to NRF24 node "TMGR"+nodeID
	//static byte nodeName[6] = F("xTMGR");
	bool ret;
	m_rf24->stopListening(); delay(50);
	nodeName[0] = destNodeID;
	nodeName[4] = 'R'; // was [3]
	m_rf24->openWritingPipe(nodeName);
	ret = false;
	for(int i=0; i<5; i++) {
		if(m_rf24->write(&radioBuf, sizeof(radioBuf))) {
			ret = true;
			break;
		}
		else {
			delay(5);
		}
	}
	m_rf24->startListening();
	return ret;
}

// If we have different radio receivers, they will have different instantiation routines.

void TaskManager::radioBegin(tm_nodeId_t nodeId, byte cePin, byte csPin) {
	//uint8_t pipeName[6];
	m_myNodeId = nodeId;
	m_rf24 = new RF24(cePin, csPin);
	//strcpy((char*)pipeName,"xTMGR");	// R for read
	nodeName[0] = myNodeId();	// (read pipe preconfigure) not printable, who cares...
	m_rf24->begin();
	m_rf24->setRetries(15,8); // 4ms betw retries, 8 retries
	m_rf24->openReadingPipe(1, nodeName);
	m_rf24->startListening();
	nodeName[4] = 'R';	// write -- was [3] and 'R'
	m_rf24->openWritingPipe(nodeName);
	if(!m_radioReceiverRunning) { add(0xfe, radioReceiverTask); m_radioReceiverRunning = true; }
}

#endif // AVR and RF24
