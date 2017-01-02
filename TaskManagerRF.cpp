
#define TASKMANAGER_MAIN

#include <SPI.h>
#include <RF24.h>
#include "TaskManagerCore.h"
#include "TaskManagerRFCore.h"

extern TaskManagerRF TaskMgr;

/*! \file TaskManagerRF.cpp
    Implementation file for Arduino Task Manager
*/

static void radioReceiverTask() {
	TaskMgr.tmRadioReceiverTask();
}



//
// Implementation of TaskManagerRF
//

// Constructor and Destructor


TaskManagerRF::TaskManagerRF() {
	m_rf24 = NULL;
	m_myNodeId = 0;
	m_radioReceiverRunning = false;
}

TaskManagerRF::~TaskManagerRF() {
	if(m_rf24!=NULL) delete m_rf24;
}

// ***************************
//  All the world's radio code
// ***************************


// General purpose receiver.  Gets a message (varying with the kind of radio)
// then parses and delivers it
void TaskManagerRF::tmRadioReceiverTask() {
	static int cnt=0;
	// polled receiver -- if there is a packet waiting, grab and process it
	// receive packet from NRF24.  Poll and process messages
	// We need to find the destination task and save the fromNode and fromTask.
	// They are saved on the task instead of the TaskManager object in case several
	// messages/signals have been received.
	while(m_rf24->available()) {
		// read a packet
		m_rf24->read((void*)(&radioBuf), sizeof(radioBuf));
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
			case tmrSignal:
				internalSendSignal(radioBuf.m_fromNodeId, radioBuf.m_fromTaskId, radioBuf.m_data[0]);
				break;
			case tmrSignalAll:
				TaskManager::sendSignalAll(radioBuf.m_data[0]);
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
void TaskManagerRF::radioSender(byte destNodeID) {
	// send packet to NRF24 node "TMGR"+nodeID
	//static byte nodeName[6] = F("xTMGR");
	m_rf24->stopListening(); delay(50);
	nodeName[0] = destNodeID;
	nodeName[4] = 'R'; // was [3]
	m_rf24->openWritingPipe(nodeName);
	for(int i=0; i<5; i++) {
		if(!m_rf24->write(&radioBuf, sizeof(radioBuf))) {
			Serial.print(F("write fail ")); Serial.println(i);
		}
		else {
			break;
		}
	}
	m_rf24->startListening();
}

// If we have different radio receivers, they will have different instantiation routines.

void TaskManagerRF::radioBegin(byte nodeId, byte cePin, byte csPin) {
	//uint8_t pipeName[6];
	m_myNodeId = nodeId;
	m_rf24 = new RF24(cePin, csPin);
	//strcpy((char*)pipeName,"xTMGR");	// R for read
	nodeName[0] = myNodeId();	// (read pipe preconfigure) not printable, who cares...
	m_rf24->begin();
	m_rf24->openReadingPipe(1, nodeName);
	m_rf24->startListening();
	nodeName[4] = 'R';	// write -- was [3] and 'R'
	m_rf24->openWritingPipe(nodeName);
	if(!m_radioReceiverRunning) { add(0xfe, radioReceiverTask); m_radioReceiverRunning = true; }
}


void TaskManagerRF::sendSignal(byte nodeId, byte sigNum) {
	if(nodeId==0 || nodeId==myNodeId()) { TaskManager::sendSignal(sigNum); return; }
	radioBuf.m_cmd = tmrSignal;
	radioBuf.m_fromNodeId = myNodeId();
	radioBuf.m_fromTaskId = myId();
	radioBuf.m_data[0] = sigNum;
	radioSender(nodeId);
}

void TaskManagerRF::sendSignalAll(byte nodeId, byte sigNum) {
	if(nodeId==0 || nodeId==myNodeId()) { TaskManager::sendSignalAll(sigNum); return; }
	radioBuf.m_cmd = tmrSignalAll;
	radioBuf.m_fromNodeId = myNodeId();
	radioBuf.m_fromTaskId = myId();
	radioBuf.m_data[0] = sigNum;
	radioSender(nodeId);
}

void TaskManagerRF::sendMessage(byte nodeId, byte taskId, char* message) {
	if(nodeId==0 || nodeId==myNodeId()) { TaskManager::sendMessage(taskId, message); return; }
	radioBuf.m_cmd = tmrMessage;
	radioBuf.m_fromNodeId = myNodeId();
	radioBuf.m_fromTaskId = myId();
	radioBuf.m_data[0] = taskId;	// who we are sending it to
	if(strlen(message)>TASKMGR_MESSAGE_SIZE-2) {
		memcpy(&radioBuf.m_data[1], message, TASKMGR_MESSAGE_SIZE-2);
		radioBuf.m_data[TASKMGR_MESSAGE_SIZE-2]='\0';
	} else {
		strcpy((char*)&radioBuf.m_data[1], message);
	}
	radioSender(nodeId);
}

void TaskManagerRF::sendMessage(byte nodeId, byte taskId, void* buf, int len) {
	if(nodeId==0 || nodeId==myNodeId()) { TaskManager::sendMessage(taskId, buf, len); return; }
	if(len>TASKMGR_MESSAGE_SIZE) return;	// reject too-long messages
	radioBuf.m_cmd = tmrMessage;
	radioBuf.m_fromNodeId = myNodeId();
	radioBuf.m_fromTaskId = myId();
	radioBuf.m_data[0] = taskId;	// who we are sending it to
	memcpy(&radioBuf.m_data[1], buf, len);
	radioSender(nodeId);
}

void TaskManagerRF::suspend(byte nodeId, byte taskId) {
	if(nodeId==0 || nodeId==myNodeId()) { TaskManager::suspend(taskId); return; }
	radioBuf.m_cmd = tmrSuspend;
	radioBuf.m_fromNodeId = myNodeId();
	radioBuf.m_fromTaskId = myId();
	radioBuf.m_data[0] = taskId;
	radioSender(nodeId);
}

void TaskManagerRF::resume(byte nodeId, byte taskId) {
	if(nodeId==0 || nodeId==myNodeId()) { TaskManager::resume(taskId); return; }
	radioBuf.m_cmd = tmrResume;
	radioBuf.m_fromNodeId = myNodeId();
	radioBuf.m_fromTaskId = myId();
	radioBuf.m_data[0] = taskId;
	radioSender(nodeId);
}

void TaskManagerRF::getSource(byte& fromNodeId, byte& fromTaskId) {
	fromNodeId = m_theTasks.front().m_fromNodeId;
	fromTaskId = m_theTasks.front().m_fromTaskId;
}


