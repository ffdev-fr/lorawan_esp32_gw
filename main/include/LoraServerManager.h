/*****************************************************************************************//**
 * @file     LoraServerManager.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     19/03/2018
 *
 * @brief    Manages LoRaWan packet exchanges between gateway and the Network Server.
 *
 * @details  For the uplink path, the 'LoraServerManager" receives LoRaWan packets from the
 *           'LoraNodeManager' and send them to the Network Server using proprietary messages.
 *           The same mechanism is used for the downlink path.
 *           The 'LoraServerManager' maintains a collection of 'ServerConnectors' which can be
 *           used to reach the Network Server. These 'ServerConnectors' can be seen as 
 *           different routes available to exchange LoRaWan packets with the Network Server.
 *           The 'LoraServerManager' can be configurated for different routing strategies
 *           (typically a prefered route and a failover route).\n
 *           The 'LoraServerManager' is associated with a 'ServerProtocolEngine' used to encode
 *           and decode messages according to Network Server protocol. In other words the
 *           'LoraServerManager' can be configured to communicate with a particular Network
 *           Server by changing the associated 'ServerProtocolEngine'.
 *
 * @note     The default Network Server protocol implemented in this "LoRaWAN ESP32 Gateway
 *           V1.x" version is the "Semtech Nerwork Server Protocol" (LoRaWan packet 
 *           encapsulated in a JSON stream with Base64 encoding).
 *
 * @note     The "LoRaWAN ESP32 Gateway V1.x" project is designed for execution on ESP32 
 *           Module (Dev.C kit).\n
 *           The implementation uses the Espressif IDF V3.0 framework (with RTOS)              
*********************************************************************************************/

#ifndef LORASERVERMANAGER_H_
#define LORASERVERMANAGER_H_

/*********************************************************************************************
  Includes
*********************************************************************************************/

#include "Utilities.h"


/********************************************************************************************* 
  Definitions for debug traces
  The debug level is specified with 'LORASERVERMANAGER_DEBUG_LEVEL' in Definitions.h file
*********************************************************************************************/

#define LORASERVERMANAGER_DEBUG_LEVEL0 ((LORASERVERMANAGER_DEBUG_LEVEL & 0x01) > 0)
#define LORASERVERMANAGER_DEBUG_LEVEL1 ((LORASERVERMANAGER_DEBUG_LEVEL & 0x02) > 0)
#define LORASERVERMANAGER_DEBUG_LEVEL2 ((LORASERVERMANAGER_DEBUG_LEVEL & 0x04) > 0)


/********************************************************************************************* 
  Definitions (implementation)
*********************************************************************************************/

// Maximum size of encoded data stream for a 'LoraServerUpMessage'
// Notes:
//  - Typically, the encoded stream is encoded using JSON and Base64
//  - The number of fields in JSON stream depends on LoRa Network Server protocol
//  - The maximum number of bytes for the original LoRa packet is 'LORA_MAX_PAYLOAD_LENGTH'
#define LORASERVERMANAGER_MAX_UPMESSAGE_LENGTH    ((LORA_MAX_PAYLOAD_LENGTH * 2) + 1024)


// Number of items in memory array for 'CLoraServerUpMessageOb' (uplink)
// Typically, messages should be transmitted to Network Server quite quickly.
// This small buffer is only in case of the reception of another LoRa packet before previous
// packet is forwarded to Network Server
#define LORASERVERMANAGER_MAX_SERVERUPMESSAGES     3

// Number of items in memory array for 'CLoraServerDownMessageOb' (downlink)
// Typically, messages should be transmitted to NodeManager quite quickly.
// This small buffer is only in case of the reception of another Network Server message
// before previous packet is forwarded to Node
#define LORASERVERMANAGER_MAX_SERVERDOWNMESSAGES     3




/********************************************************************************************* 
  Structures 
*********************************************************************************************/






/********************************************************************************************* 
 LoraServerMessage Class

 This class describes an uplink message to send to the Network Server.

 Different types of uplink messages could be referenced in this object:
 - Encoded LoRa packet received from node
 - 'heartbeat' (periodical push from gateway and/or periodical pull request send by gateway)
*********************************************************************************************/

// Class data
typedef struct _CLoraServerUpMessage
{
  // Current state of 'LoraServerUpMessage'
  // NOTE: Must be atomic because used by the different tasks or 'CLoraServerManager' object
  //       without mutex
  DWORD m_dwMessageState;

  // Idenfifier of 'CLoraServerUpMessage'
  // Note: This identifier is the index in the 'MemoryBlockArray' used to maintain the
  //       'CLoraServerUpMessage' collection (i.e. messages to send via 'ServerConnetors')
  // Note: This identifier is not used for 'heartbeat' message (i.e. data maintained in a
  //       dedicated memory block). 
  //       For 'heartbeat' message the value of 'm_usMessageId' is 0xFF
  //
  // IMPORTANT NOTE: The 'm_usMessageId == 0xFF' expression in used in implementation to
  //                 check if message is a 'LoRa packet' from node or a 'Heartbeat'
  BYTE m_usMessageId;

  // Identifier of message in both 'CLoraServerManager' and associated 'ProtocolEngine'
  // The identifier format is:
  //  - LOWORD = Identifier in 'ProtocolEngine'. 
  //             This identifer depends on'ProtocolEngine' implementation and it is used to 
  //             retrieve the protocol session associated to the message.
  //  - HIWORD = Identifier in 'CLoraServerManager'.
  //             This identifier is 'm_usMessageId' (see above)
  DWORD m_dwProtocolMessageId;

  // Identifier of last 'ServerConnector' used to send the message
  BYTE m_usLastConnectorId;

  // 'LoraPacketSession' data (received via 'ServerManagerItf_LoraSessionPacket' object)
  // Note: These objects live in 'CLoraNodeManager'
  // Note: The 'm_pLoraPacket' object is not valid anymore as soon as the 'ACCEPTED' event
  //       is sent to the owner ('CLoraNodeManager' via 'ITransceiverManager' interface)
  // Note: The following variables are not used for 'heartbeat' messages
  void *m_pSession;
  void *m_pLoraPacket;
  void *m_pLoraPacketInfo;
  DWORD m_dwSessionId;
  
  //
  // Encoded data to send to LoRa Network Server
  //

  // Stream length
  WORD m_wDataLength;

  // Data bytes
  BYTE m_usData[LORASERVERMANAGER_MAX_UPMESSAGE_LENGTH];

} CLoraServerUpMessageOb;

typedef struct _CLoraServerUpMessage * CLoraServerUpMessage;


/********************************************************************************************* 
 LoraServerDownMessage Class

 This class describes a downlink message received from the Network Server.

 Different types of downlink messages could be referenced in this object:
 - Encoded LoRa packet to transmit to node
 - An 'ACK' message sent in response to an uplink message
*********************************************************************************************/

// Class data
typedef struct _CLoraServerDownMessage
{
  // Current state of 'LoraServerDownMessage'
  // NOTE: Must be atomic because used by the different tasks or 'CLoraServerManager' object
  //       without mutex
  DWORD m_dwMessageState;

  // Idenfifier of 'CLoraServerUpMessage'
  // Note: This identifier is the index in the 'MemoryBlockArray' used to maintain the
  //       'CLoraServerUpMessage' collection (i.e. messages to send via 'ServerConnetors')
  // Note: This identifier is not used for 'heartbeat' message (i.e. data maintained in a
  //       dedicated memory block). 
  //       For 'heartbeat' message the value of 'm_usMessageId' is 0xFF
  //
  // IMPORTANT NOTE: The 'm_usMessageId == 0xFF' expression in used in implementation to
  //                 check if message is a 'LoRa packet' from node or a 'Heartbeat'
  BYTE m_usMessageId;

  // Identifier of message in both 'CLoraServerManager' and associated 'ProtocolEngine'
  // The identifier format is:
  //  - LOWORD = Identifier in 'ProtocolEngine'. 
  //             This identifer depends on'ProtocolEngine' implementation and it is used to 
  //             retrieve the protocol session associated to the message.
  //  - HIWORD = Identifier in 'CLoraServerManager'.
  //             This identifier is 'm_usMessageId' (see above)
  DWORD m_dwProtocolMessageId;

  // Identifier of last 'ServerConnector' used to send the message
  BYTE m_usLastConnectorId;

  // 'LoraPacketSession' data (received via 'ServerManagerItf_LoraSessionPacket' object)
  // Note: These objects live in 'CLoraNodeManager'
  // Note: The 'm_pLoraPacket' object is not valid anymore as soon as the 'ACCEPTED' event
  //       is sent to the owner ('CLoraNodeManager' via 'ITransceiverManager' interface)
  // Note: The following variables are not used for 'heartbeat' messages
  void *m_pSession;
  void *m_pLoraPacket;
  void *m_pLoraPacketInfo;
  DWORD m_dwSessionId;
  
  //
  // Encoded data to send to LoRa Network Server
  //

  // Stream length
  WORD m_wDataLength;

  // Data bytes
  BYTE m_usData[LORASERVERMANAGER_MAX_UPMESSAGE_LENGTH];

} CLoraServerDownMessageOb;

typedef struct _CLoraServerDownMessage * CLoraServerDownMessage;



/********************************************************************************************* 
 Helper macros to split 'm_dwProtocolMessageId' identifier defined in 'CLoraServerDownMessageOb'
 and 'CLoraServerDownMessageOb' objects
*********************************************************************************************/

#define LORASERVERMANAGER_PROTOCOLENGINE_MESSAGEID(id) (id & 0xFFFF)
#define LORASERVERMANAGER_SERVERMANAGER_MESSAGEID(id)  (id >> 16)

#define LORASERVERMANAGER_SERVERMANAGER_IS_HEARTBEAT(BlockIdx)  (BlockIdx == 0xFF)


/********************************************************************************************* 
 ConnectorDescr Class
*********************************************************************************************/

typedef struct _CConnectorDescr
{
  // Interface to associated 'ServerConnector'
  IServerConnector m_pServerConnectorItf;
  
  // In current version, only one connector is active (i.e. first reachable network defined
  // during boot)
  bool m_bActive; 

} CConnectorDescrOb;

typedef struct _CConnectorDescr * CConnectorDescr;


/********************************************************************************************* 
  CLoraServerManager_Message object

  Objects queued in 'LoraSession' Task (main automaton) for external commands received via 
  'ITransceiverManager' interface and for internal comman sent by other tasks
*********************************************************************************************/

// The event message (i.e. posted to event queue of owner object)
typedef struct _CLoraServerManager_Message
{
  // Public
  WORD m_wMessageType;                            // The message
  DWORD m_dwMessageData;                          // Depends on message type
                                                  // Value or pointer to object
  DWORD m_dwMessageData2;                         // Depends on message type
} CLoraServerManager_MessageOb;

typedef struct _CLoraServerManager_Message * CLoraServerManager_Message;


/********************************************************************************************* 
 LoraServerManager Class
*********************************************************************************************/

// Class data
typedef struct _CLoraServerManager
{
  // Interface
  IServerManager m_pServerManagerItf;

  uint32_t m_nRefCount;      // The 'CLoraServerManager' object is reference counted (number of client 
                             // objects owning one of the public interfaces exposed by 'CLoraServerManager')

  // Object main state
  DWORD m_dwCurrentState;

  //
  // Collection of 'ServerConnector'
  //

  // Actual number of 'ServerConnector' present in the Gateway
  BYTE m_usConnectorNumber;

  // 'ServerConnector' descriptor array
  // Note: The 'ServerConnector' identifier is the index in this array
  CConnectorDescrOb m_ConnectorDescrArray[GATEWAY_MAX_SERVERCONNECTORS];


  //
  // Tasks and associated queues
  //

  // 'ServerManager' task (main automaton)
  // Used for internal messages and external commands via 'IServerManager' interface
  TaskFunction_t m_hServerManagerTask;
  QueueHandle_t m_hServerManagerQueue;

  // For command processing by 'ServerManager' task
  SemaphoreHandle_t m_hCommandMutex;
  SemaphoreHandle_t m_hCommandDone;
  DWORD m_dwCommand;
  void *m_pCommandParams;


  // 'NodeManager' task (automaton for exchange with 'LoraNodeManager')
  // Used to process uplink packets received from 'LoraNodeManager'
  // This 'Task' is known by the 'LoraNodeManager' (i.e. attached) and is direcly notified
  TaskFunction_t m_hNodeManagerTask;

  // 'Connector' task (automaton for exchange with 'ServerConnector' objects)
  // Used to process notifications for downlink packet received from 'NetworkServer (by 'ServerConnectors')
  TaskFunction_t m_hConnectorTask;
  QueueHandle_t m_hConnectorNotifQueue;

  //
  // Transmission of downlink packets to 'TransceiverManager'
  // Event notification to 'TransceiverManager' (typically downlink packet processing)
  //

  // The 'ITransceiverManager' interface of 'CLoraNodeManager' object to use for event notification.
  // A NULL value indicates that 'LoraNodeManager' is not attached (see 'ILoraNodeManager_Initialize' method)
  ITransceiverManager m_pTransceiverManagerItf;

  // Task in 'CLoraNodeManager' object to ask for sending (transmit) downlink packet to node device
  // A NULL value indicates that 'LoraNodeManager' is not attached (see 'ILoraNodeManager_Attach' method)
  TaskFunction_t m_hTransceiverManagerTask;


  //
  // Uplink message management
  //

  // Memory block array for 'LoraServerUpMessages' (i.e. uplink messages for Network Server)
  CMemoryBlockArray m_pLoraServerUpMessageArray;

  // Memory for 'heartbeat' last uplink message
  // Note: 
  //  - The 'heartbeat' messages are periodically sent by main task (i.e. serialized)
  //  - Only the associated transaction is checked (i.e. not required to keep message data in order to 
  //    process associated 'ack')
  // TO CHECK -> may be required to use a MessageOb from Array (and keep it locked)
  CLoraServerUpMessageOb m_HeartbeatMessageOb;


  //
  // Downlink message management
  //

  // Memory block array for 'LoraServerDownMessages' (i.e. descriptor of downlink messages from Network Server)
  CMemoryBlockArray m_pLoraServerDownMessageArray;

  // Memory block array for downlink Network Server message stream (i.e. raw data received from Network Server)
  CMemoryBlockArray m_pDownlinkMessageStreamArray;

  // Memory block array for downlink LoRa packets (i.e. whole payload to use with 'LoraTransceiver')
  CMemoryBlockArray m_pDownlinkLoraPacketArray;



  // Uplink packet currently sent to the 'LoraNodeManager'
  // Note: This object is owned by the 'CLoraServerManager' object. It must be explicitly released by the
  //       'CLoraNodeManager' when it is no more required for the send operation (i.e. 'Send' method on 
  //       'ITransceiverManager' interface) 
//  CServerManagerItf_LoraSessionPacketOb m_ForwardedUplinkPacket;


  // Interface to 'NetworkServerProtocol' engine
  INetworkServerProtocol m_pNetworkServerProtocolItf;

  // Access to 'NetworkServer'
  char m_szNetworkServerUrl[64];
  char m_szNetworkServerUser[32];
  char m_szNetworkServerPassword[32];
  

  // Properties
//DWORD m_dwMissedUplinkPacketdNumber;

} CLoraServerManager;

// Class constants and definitions


// Methods for 'IServerManager' interface implementation on 'LoraServerManager' object
// The 'CServerManagerItfImpl' structure provided by 'LoraServerManager' object contains pointers to these methods 
uint32_t CLoraServerManager_AddRef(void *this);
uint32_t CLoraServerManager_ReleaseItf(void *this);

bool CLoraServerManager_Initialize(void *this, void *pParams);
bool CLoraServerManager_Attach(void *this, void *pParams);
bool CLoraServerManager_Start(void *this, void *pParams);
bool CLoraServerManager_Stop(void *this, void *pParams);
bool CLoraServerManager_ServerMessageEvent(void *this, void *pEvent);


// Construction
CLoraServerManager * CLoraServerManager_New();
void CLoraServerManager_Delete(CLoraServerManager *this);


// Task functions
// Note: The CLoraServerManager object has 3 automatons
void CLoraServerManager_ServerManagerAutomaton(CLoraServerManager *this);
void CLoraServerManager_TransceiverAutomaton(CLoraServerManager *this);
void CLoraServerManager_ForwarderAutomaton(CLoraServerManager *this);


// Main Automaton (SessionManager task)
// Note: Numbering order MUST match object's life cycle
#define LORASERVERMANAGER_AUTOMATON_STATE_CREATING      0
#define LORASERVERMANAGER_AUTOMATON_STATE_CREATED       1
#define LORASERVERMANAGER_AUTOMATON_STATE_INITIALIZED   2
#define LORASERVERMANAGER_AUTOMATON_STATE_IDLE          3
#define LORASERVERMANAGER_AUTOMATON_STATE_RUNNING       4
#define LORASERVERMANAGER_AUTOMATON_STATE_STOPPING      5
#define LORASERVERMANAGER_AUTOMATON_STATE_TERMINATED    6
#define LORASERVERMANAGER_AUTOMATON_STATE_ERROR         7


#define LORASERVERMANAGER_AUTOMATON_MSG_NONE               0x00000000
#define LORASERVERMANAGER_AUTOMATON_MSG_COMMAND            0x00000001
//#define LORASERVERMANAGER_AUTOMATON_MSG_NOTIFY             0x00000002

#define LORASERVERMANAGER_AUTOMATON_MAX_CMD_DURATION       2000
#define LORASERVERMANAGER_AUTOMATON_MAX_SYNC_CMD_DURATION  120000

#define LORASERVERMANAGER_AUTOMATON_CMD_NONE         0x00000000
#define LORASERVERMANAGER_AUTOMATON_CMD_INITIALIZE   0x00000001
#define LORASERVERMANAGER_AUTOMATON_CMD_ATTACH       0x00000002
#define LORASERVERMANAGER_AUTOMATON_CMD_START        0x00000003
#define LORASERVERMANAGER_AUTOMATON_CMD_STOP         0x00000004


bool CLoraServerManager_NotifyAndProcessCommand(CLoraServerManager *this, DWORD dwCommand, DWORD dwTimeout, void *pCmdParams);
bool CLoraServerManager_ProcessAutomatonNotifyCommand(CLoraServerManager *this);

bool CLoraServerManager_ProcessInitialize(CLoraServerManager *this, CServerManagerItf_InitializeParams pParams);
bool CLoraServerManager_ProcessAttach(CLoraServerManager *this, CServerManagerItf_AttachParams pParams);
bool CLoraServerManager_ProcessStart(CLoraServerManager *this, CServerManagerItf_StartParams pParams);
bool CLoraServerManager_ProcessStop(CLoraServerManager *this, CServerManagerItf_StopParams pParams);

void CLoraServerManager_ProcessServerMessageEventUplinkReceived(CLoraServerManager *this, CLoraServerUpMessage pLoraServerMessage);
void CLoraServerManager_ProcessServerMessageEventUplinkPrepared(CLoraServerManager *this, CLoraServerUpMessage pLoraServerMessage); 
void CLoraServerManager_ProcessServerMessageEventUplinkSent(CLoraServerManager *this, CLoraServerUpMessage pLoraServerMessage);
void CLoraServerManager_ProcessServerMessageEventUplinkSendFailed(CLoraServerManager *this, CLoraServerUpMessage pLoraServerMessage);
void CLoraServerManager_ProcessServerMessageEventUplinkFailed(CLoraServerManager *this, CLoraServerUpMessage pLoraServerMessage);
void CLoraServerManager_ProcessServerMessageEventUplinkTerminated(CLoraServerManager *this, CLoraServerUpMessage pLoraServerMessage, DWORD dwProtocolState);

bool CLoraServerManager_SendServerMessage(CLoraServerManager *this, CLoraServerUpMessage pLoraServerMessage, bool bFirstConnector);


// LoraServerUpMessage state 
#define LORANODEMANAGER_SERVERUPMESSAGE_STATE_CREATED      0
#define LORANODEMANAGER_SERVERUPMESSAGE_STATE_PREPARED     1
#define LORANODEMANAGER_SERVERUPMESSAGE_STATE_SENDING      2



// LoraTransceiver Automaton (Transceiver task)

//#define LORASERVERMANAGER_TRANSCEIVER_MSG_NONE             0x00000000
//#define LORASERVERMANAGER_TRANSCEIVER_MSG_UPLINK_RECEIVED  0x00000001
//#define LORASERVERMANAGER_TRANSCEIVER_MSG_DOWNLINK_SENT    0x00000002


//bool CLoraServerManager_ProcessTransceiverUplinkReceived(CLoraServerManager *this, CLoraTransceiverItf_Event pEvent);
//bool CLoraServerManager_ProcessTransceiverDownlinkSent(CLoraServerManager *this, CLoraTransceiverItf_Event pEvent);


// Class public methods






// Class private methods (implementation helpers)



#endif





