
//
// Low level drivers for ESP radio communications
//  This includes everything from radioBegin down.
//  Primarily, this includes radio initialization and control
//  and the low-level sending and receiving of messages (everything
//  from transmitting the raw data to the library routines to
//	send data to the other node, to polling/responding to interrupts
//  for received data, buffering the data, and placing it into
//  the targeted task's message buffer).

// Only use this in an ESP device

#define NEWAPI true

#if defined(DOXYGEN_ALL) || (defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32))

#include <arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include <Streaming.h>

#include <TaskManagerCore.h>
#include "radioDriverESP.h"

// Get rid of DEBUG at some point
#define DEBUG false

/*!	\defgroup TaskManagerRadioESP TaskManager ESP Network/Mesh Drivers
	@{
*/
/*!	\defgroup internal Internal components
	@{
	@}
*/

/*! \cond DO_NOT_PROCESS */
extern TaskManager TaskMgr;
/*! \endcond */

/*!	\brief General radio receiver task.
	Each driver is required to provide a radio receiver task.  The radio receiver task is 
	a full fledged TaskManager task, and is called on schedule with all other tasks.
	
	The radio receiver task should empty the incoming message queue of all messages and distribute them
	to the appropriate target tasks.
*/
static void radioReceiverTask() {
	TaskMgr.tmRadioReceiverTask();
}

// Semaphore to coordinate queue calls (so we don't call add (from the receive callback) while we
// are in the middle of a remove).
//static SemaphoreHandle_t _TaskManagerMessageQueueSemaphore;


// Configuration
//	Which WiFi channel to use
//! \cond EXCLUDE_ME
#define WIFI_CHANNEL 1
//! \endcond

//static void radioReceiverTask() {
//	TaskMgr.tmRadioReceiverTask();
//}

static void dumpBuf(const uint8_t* buf, short len) {
	for(int i=0; i<16/*len*/; i++) {
		Serial << _HEX(buf[i]) << ' ';
		if(i%16 == 15) Serial << endl;
	}
}
static void dumpMac(const uint8_t* buf) {
	Serial << _HEX(buf[0]);
	for(int i=1; i<6; i++) Serial << ":" << _HEX(buf[i]);
}

static char* espErrText(esp_err_t err) {
	static char buf[20];
	if(err==ESP_OK) return "ESP_OK";
	else if(err==ESP_ERR_ESPNOW_ARG) return "ESP_ERR_ESPNOW_ARG";
	else if(err==ESP_ERR_ESPNOW_INTERNAL) return "ESP_ERR_ESPNOW_INTERNAL";
	else if(err==ESP_ERR_ESPNOW_IF) return "ESP_ERR_ESPNOW_IF";
	else if(err==ESP_ERR_ESPNOW_NOT_FOUND) return "ESP_ERRNOW_NOT_FOUND";
	else {
		sprintf(buf,"ESP_ERR_UNKN_%d",(int)err);
		return buf;
	}
}

//! \ignore
//
// Incoming message queue
//
// Design note:  TaskManager will be using _TaskManagerRadioPacket objects to transport data between
// nodes.  The message queue will send full packets and just absorb whatever returns.
// Also, the HAL will read/write arbitrary buffers.
// So:  The HAL will expect a uint8_t* buffer and a size.  It will send or receive it.  The high level
// routine will pass in the uint8_t* buffer and a size of sizeof(_TaskManagerRadioPacket).  When polling
// the message queue, it will provide a buffer and a short* in return, even though the short* will always
// receive sizeof(_TaskManagerRadioPacket)
// Note that the radio packet will contain a short nodeID and a byte taskID.
//

/*!	\class MessageQueue
	The queue of incoming messages.  Messages are added by the interrupt handler and removed by the
	radio receiver task.
	
	If the buffer is full, new messages are discarded.
*/
class MessageQueue {
  private:
	_TaskManagerRadioPacket m_packets[TASKMGR_MESSAGE_QUEUE_SIZE];
	short m_lengths[TASKMGR_MESSAGE_QUEUE_SIZE];
	bool m_isEmpty;
	short m_head;	// oldest entry
	short m_tail;	// newest entry
  public:
  	MessageQueue(): m_isEmpty(true), m_head(0), m_tail(0) {};
	//! \brief Tells whether or not the message queue is empty.
	bool isEmpty() { return m_isEmpty; }
	bool add(const uint8_t* dat, const byte len);
	bool remove(uint8_t* dat, byte* len);
	/*!	\brief Return the size of the message queue.  Return 0 if the queue is empty.
	*/
	short size() {
		if(isEmpty()) return 0;
		else if(m_tail<=m_head) return m_head-m_tail+1;
		else return (m_head+TASKMGR_MESSAGE_QUEUE_SIZE)-m_tail;
	}
};

#if false	// old ff debug stuff
struct tmpStruct { byte cmd; byte fromTaskId; tm_nodeId_t fromNodeId; byte ffcmd; uint32_t seq; };
#endif


/*! \brief Add a message to the message queue.
	The given message is added to the message queue at the end of the queue.
	If the queue is full, the message is discarded.
	\param dat - a pointer to the byte array of the message.
	\param len - the length of the message, in bytes
	\returns true if the message was added, false if discarded.
*/
bool MessageQueue::add(const uint8_t* dat, const byte len) {
	// if we can't grab the semaphore in a ms, just ignore the message.
	bool ret;
#if false	// old ff debug stuff
	tmpStruct* ts;
	ts = (tmpStruct*)dat;
	if(DEBUG) Serial << "-->MessageQueue::add cmd: " << ts->cmd
		<< " from: " << ts->fromNodeId << ' ' << ts->fromTaskId
		<< " ffcmd: " << ts->ffcmd << " ffseq: " << ts->seq << endl;
#endif
	if(xSemaphoreTake(TaskMgr.m_TaskManagerMessageQueueSemaphore,1000)==pdFALSE) {
		return false;
	}
    if(m_isEmpty) {
        m_isEmpty = false;
        memcpy(&m_packets[m_tail], dat, len);
        m_lengths[m_tail] = len;
		ret = true;
    } else if (m_head==((m_tail+1)%TASKMGR_MESSAGE_QUEUE_SIZE)) {
        Serial.print("<receive buffer full, incoming message ignored>\n");
		ret = false;
    } else {
        m_tail = (m_tail+1)%TASKMGR_MESSAGE_QUEUE_SIZE;
        memcpy(&m_packets[m_tail], dat, len);
        m_lengths[m_tail] = len;
		ret = true;
    }
    xSemaphoreGive(TaskMgr.m_TaskManagerMessageQueueSemaphore);
    if(DEBUG) Serial << "<--MessageQueue::add\n";
	return ret;
};
/*! \brief Remove a message from the queue.
	Removes a message from the queue.  Returns true if a message could be removed;
	false if the queue was empty.
	Note that the buffer should be sufficiently large to hold any sized message.
	\param dat - a pointer to a message buffer
	\param len - the number of bytes returned.
	\returns true if the message was saved, false if not saved.
*/
bool MessageQueue::remove(uint8_t* dat, byte* len) {
	int t_at, t_len;
	bool ret;
	if(xSemaphoreTake(TaskMgr.m_TaskManagerMessageQueueSemaphore,1000)==pdFALSE) return false;
    if(m_isEmpty) {
		xSemaphoreGive(TaskMgr.m_TaskManagerMessageQueueSemaphore);
        printf("<empty>");
        return false;;
    } else if(m_head==m_tail) {
        memcpy(dat, &m_packets[m_head], sizeof(_TaskManagerRadioPacket));
        *len = m_lengths[m_head];
        t_at = m_head; t_len = *len;
        m_isEmpty = true;
		ret = true;
    } else {
        memcpy(dat, &m_packets[m_head], sizeof(_TaskManagerRadioPacket));
        *len = m_lengths[m_head];
        t_at = m_head; t_len = *len;
        m_head = (m_head+1) % TASKMGR_MESSAGE_QUEUE_SIZE;
		ret = true;
    }
    xSemaphoreGive(TaskMgr.m_TaskManagerMessageQueueSemaphore);
    return ret;
};

static MessageQueue _TaskManagerIncomingMessages;

// shared buf for MAC address; last two bytes are set to nodeID
static byte nodeMac[6] = { 0xA6, 'T', 'M', 0, 0, 0};

//
// Callbacks
//
static void msg_send_cb(const uint8_t* mac, esp_now_send_status_t sendStatus) {
	// We sent a message to the designated mac.  The message was sent with
	// sendStatus status.

	// for now, do nothing.
}

#if defined(NEWAPI)
static void msg_recv_cb(const esp_now_recv_info_t *mac, const uint8_t* data, int len) {
#else
static void msg_recv_cb(const uint8_t *mac, const uint8_t* data, int len) {
#endif
	// We have received a message from the given MAC with the accompanying data.
	// Save the data in the "incoming message" queue
	// We don't use taskENTER_CRITICAL here because 'add' does it as needed.
	if(DEBUG) Serial << "-->msg_recv_cb\nreceived message len=" << len << "\n";
	_TaskManagerIncomingMessages.add(data, len&0x0ff);
	if(DEBUG) Serial << "Queue is now " << (_TaskManagerIncomingMessages.isEmpty() ? " " : "not ") << "empty\n";
	if(DEBUG) Serial << "Queue size is now " << _TaskManagerIncomingMessages.size() << endl;
	if(DEBUG) Serial << "<--msg_recv_cb\n";
}

// General purpose receiver.  Checks the message queue for delivered messages and processes the first one
void TaskManager::tmRadioReceiverTask() {
	static byte len;
	// polled receiver -- if there is a packet waiting, grab and process it
	// receive packet from ESP radio mgmt..  Poll and process messages
	// We need to find the destination task and save the fromNode and fromTask.
	// They are saved on the task instead of the TaskManager object in case several
	// messages/signals have been received.
	while(true) {	// process all messages
		if(_TaskManagerIncomingMessages.isEmpty()) {
			break;
//			return;
		}
		if(DEBUG) Serial << "-->TaskManagerESP::tmRadioReceiverTask\n";
		// read a packet
		//m_rf24->read((void*)(&radioBuf), sizeof(radioBuf));
		_TaskManagerIncomingMessages.remove((uint8_t*)&radioBuf, &len);
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
				if(DEBUG) Serial << "radioReceiverTask: msg from n/t " << radioBuf.m_fromNodeId << "/" << radioBuf.m_fromTaskId 
					<< " for task " << radioBuf.m_data[0] << endl;
				internalSendMessage(radioBuf.m_fromNodeId, radioBuf.m_fromTaskId,
					radioBuf.m_data[0], &radioBuf.m_data[1], TASKMGR_MESSAGE_SIZE);
				break;
			case tmrSuspend:
				TaskManager::suspend(radioBuf.m_data[0]);
				break;
			case tmrResume:
				TaskManager::resume(radioBuf.m_data[0]);
				break;
		} // end switch
		if(DEBUG) Serial << "<--TaskManager:tmRadioReceiverTask finished a message\n";
		if(DEBUG) Serial << "   Queue is now " << (_TaskManagerIncomingMessages.isEmpty() ? " " : "not ") << "empty\n";
		if(DEBUG) Serial << "   Queue size is now " << _TaskManagerIncomingMessages.size() << endl;
	}  // end while true
}

// General purpose sender.  Sends a message somewhere (varying with the kind of radio)
/*!	\brief Generic packet sender
	TaskManager.radioBuf contains a complete formatted packet of information to 
	send to the designated node.  radioSender(node) sends the packet to the node.
	
	This routine is only present in radio-enabled environments.  It is implemented 
	in the driver module.
	\param node - The target node
	\returns - boolean, true if the remote node received the packet (low-level receive).  This
	does not guarantee that the task has received the packet.
*/
bool TaskManager::radioSender(tm_nodeId_t destNodeID) {
//Serial << "radioSender, sending to node " << destNodeID << " from n/t " 
//<< radioBuf.m_fromNodeId << "/" << radioBuf.m_fromTaskId 
//<< " for t " << radioBuf.m_data[0] << " swarm cmd " << radioBuf.m_data[1] << endl;
	nodeMac[4] = (destNodeID>>8)&0x0ff;
	nodeMac[5] = destNodeID&0x0ff;
	m_lastESPError = esp_now_send(nodeMac, (byte*)&radioBuf, sizeof(radioBuf));
//Serial << "radioSender, error code is " << m_lastESPError << endl;
	if(m_lastESPError!=ESP_OK) Serial << "***ERR " << espErrText(m_lastESPError) << " sending to node " << destNodeID << "***\n";
	return m_lastESPError == ESP_OK;
}

// If we have different radio receivers, they will have different instantiation routines.

bool TaskManager::radioBegin(tm_nodeId_t nodeID, const char* ssid, const char* pw) {
	// Initialize ESP-NOW and WiFi system
	if(DEBUG) { Serial << "-->radioBegin  nodeID=" << nodeID <<"\n"; }
	WiFi.mode(ssid==NULL ? WIFI_STA : WIFI_AP_STA);
	if(DEBUG) { Serial << "\tradioBegin: past Wifi.mode()\n"; }
	nodeMac[4] = (nodeID>>8)&0x0ff;
	nodeMac[5] = nodeID & 0x0ff;
#if defined(NEWAPI)
	m_lastESPError = esp_wifi_set_mac(WIFI_IF_STA, nodeMac);
#else
	m_lastESPError = esp_wifi_set_mac(ESP_IF_WIFI_STA, nodeMac);
#endif
	if(DEBUG) Serial << "\tradioBegin: past esp_wifi_set_mac()\n";
	if(m_lastESPError!=ESP_OK) {
		if(DEBUG) Serial << "<--radioBegin error, can't set_mac()\n";
		return false;
	}
	if(ssid==NULL) {
		// ESP-NOW only
		if(DEBUG) Serial << "\tradioBegin: esp-now only, about to WiFi.disconnect()\n";
		WiFi.disconnect();
		if(DEBUG) Serial << "\tradioBegin esp-now only: past Wifi.disconnect()\n";
	} else {
		// ESP-NOW, but project uses WiFi
		// Get channel
		int32_t channel;
		channel = 0;
	  	if (int32_t n = WiFi.scanNetworks()) {
		  for (uint8_t i=0; i<n; i++) {
			if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
			  channel = WiFi.channel(i);
			} // end if
		  } // end for each found network entry
	  	} // end if there are any network entries
		// Set our channel
		esp_wifi_set_promiscuous(true);
		esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  		esp_wifi_set_promiscuous(false);
		if(pw!=NULL) {
			// we need to configure the WiFi link to the access point
			WiFi.begin(ssid, pw);
			for(int i=0; i<10 && WiFi.status()!=WL_CONNECTED; i++) delay(500);
			if(WiFi.status()!=WL_CONNECTED) {
				if(DEBUG) Serial << "<--radioBegin error, Wifi.begin(ssid,pw) failed.\n";
				return false;
			}
		} else {
			WiFi.disconnect();
		}
	}
	/////

	/////
	m_lastESPError = esp_now_init();
	if(m_lastESPError!=ESP_OK) {
		if(DEBUG) Serial << "<--radioBegin error, esp_now_init() failed\n";
		return false;
	}

	delay(10);

	// register callbacks
	m_lastESPError = esp_now_register_recv_cb(msg_recv_cb);
	if(m_lastESPError!=ESP_OK) {
		Serial << "<--radioBegin error, esp_now_register_recv_cb() failed\n";
		return false;
	}
	m_lastESPError = esp_now_register_send_cb(msg_send_cb);
	if(m_lastESPError!=ESP_OK) {
		Serial << "<--radioBegin error, esp_now_register_send_cb() failed\n";
		return false;
	}

	// create our semaphore
	m_TaskManagerMessageQueueSemaphore = xSemaphoreCreateMutex();

	// start our handler
	TaskMgr.add(TASKMGR_RF_MONITOR_TASK, radioReceiverTask);

	// final cleanup
	m_myNodeId = nodeID;
	m_radioReceiverRunning = true;
	if(DEBUG) Serial << "<--radioBegin successful return\n";
	return true;
}

bool TaskManager::registerPeer(tm_nodeId_t nodeId) {
	// register the partner nodeID as a peer
	esp_now_peer_info_t peer;
	nodeMac[4] = (nodeId>>8)&0x0ff;
	nodeMac[5] = nodeId & 0x0ff;
	memcpy(peer.peer_addr, &nodeMac, 6);
	peer.channel = WIFI_CHANNEL;
#if defined(NEWAPI)
	peer.ifidx = WIFI_IF_STA;
#else
	peer.ifidx = ESP_IF_WIFI_STA;
#endif
	peer.encrypt=false;
	m_lastESPError = esp_now_add_peer(&peer);
	return m_lastESPError==ESP_OK;
}

bool TaskManager::unRegisterPeer(tm_nodeId_t nodeId){
	// unregister the partner nodeID as a peer
	nodeMac[4] = (nodeId>>8)&0x0ff;
	nodeMac[5] = nodeId & 0x0ff;
	m_lastESPError = esp_now_del_peer(nodeMac);
	return m_lastESPError==ESP_OK;
}

const char* TaskManager::lastESPError() {
	return espErrText(m_lastESPError);
}
/*! @} */ // end TaskManagerRadioESP
#endif // ESP radio


