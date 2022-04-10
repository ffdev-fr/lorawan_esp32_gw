/*****************************************************************************************//**
 * @file     LoraNodeManager.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     04/03/2018
 *
 * @brief    Manages LoRaWan sessions for packets received by associated 'LoraTransceivers'.
 *
 * @details  This object maintains a collection of 'LoraTransceivers' and manages LoRaWAN
 *           sessions with  nodes.\n
 *           The 'LoraPacketSession' implements the state machine of LoRaWAN protocole for
 *           uplink and downlink transmissions (i.e. concepts of 'downlink window', 'packet
 *           acknowledgement', 'OTA Activation' and 'MAC commands).
 *
 * @note     The "LoRaWAN ESP32 Gateway V1.x" project is designed for execution on ESP32 
 *           Module (Dev.C kit).\n
 *           The implementation uses the Espressif IDF V3.0 framework (with RTOS)              
*********************************************************************************************/

#ifndef LORANODEMANAGER_H_
#define LORANODEMANAGER_H_

/*********************************************************************************************
  Includes
*********************************************************************************************/

#include "Utilities.h"


/********************************************************************************************* 
  Definitions for debug traces
  The debug level is specified with 'LORANODEMANAGER_DEBUG_LEVEL' in Definitions.h file
*********************************************************************************************/

#define LORANODEMANAGER_DEBUG_LEVEL0 ((SX1276_DEBUG_LEVEL & 0x01) > 0)
#define LORANODEMANAGER_DEBUG_LEVEL1 ((SX1276_DEBUG_LEVEL & 0x02) > 0)
#define LORANODEMANAGER_DEBUG_LEVEL2 ((SX1276_DEBUG_LEVEL & 0x04) > 0)


/********************************************************************************************* 
  Definitions (implementation)
*********************************************************************************************/

// Storage size for packet data and session management objects
// TO DO: define the values using description of nodes (number and properties)

// Number of items in memory array for 'CLoraPacketSessionOb' (uplink sessions)
// Typically, session life time depends on number of nodes (and speed of forwarder)
// Note: Maximum value is 255
#define LORANODEMANAGER_MAX_UP_LORASESSIONS (GATEWAY_MAX_LORATRANSCEIVERS * 3)

// Number of items in memory array for 'CLoraDownPacketSessionOb' (downlink sessions)
// Typically, session life time depends on LoRa configuration for 'receive windows' (Class A)
// and strategy of Network Server for downlink message
// Note: Maximum value is 255
#define LORANODEMANAGER_MAX_DOWN_LORASESSIONS (GATEWAY_MAX_LORATRANSCEIVERS * 5)

// Number of items in memory array for 'CLoraTransceiverItf_LoraPacketOb' (uplink and downlink)
// Note: Uplink packets have a short life time in this storage area (i.e. time required by
//       the gateway to transfer packet data from one side to the other)
// Note: Maximum value is 255
#define LORANODEMANAGER_MAX_LORAPACKETS (LORANODEMANAGER_MAX_UP_LORASESSIONS + LORANODEMANAGER_MAX_DOWN_LORASESSIONS)


// Constants for 'MessageType' in 'MAC Header' (MHDR field) of Lora packet
#define LORANODEMANAGER_MSG_TYPE_BASE             0
#define LORANODEMANAGER_MSG_TYPE_JOIN_REQUEST     LORANODEMANAGER_MSG_TYPE_BASE
#define LORANODEMANAGER_MSG_TYPE_JOIN_ACCEPT      (LORANODEMANAGER_MSG_TYPE_BASE + 1)
#define LORANODEMANAGER_MSG_TYPE_UNCONF_UPLINK    (LORANODEMANAGER_MSG_TYPE_BASE + 2)
#define LORANODEMANAGER_MSG_TYPE_UNCONF_DOWNLINK  (LORANODEMANAGER_MSG_TYPE_BASE + 3)
#define LORANODEMANAGER_MSG_TYPE_CONF_UPLINK      (LORANODEMANAGER_MSG_TYPE_BASE + 4)
#define LORANODEMANAGER_MSG_TYPE_CONF_DOWNLINK    (LORANODEMANAGER_MSG_TYPE_BASE + 5)
#define LORANODEMANAGER_MSG_TYPE_RFU              (LORANODEMANAGER_MSG_TYPE_BASE + 6)
#define LORANODEMANAGER_MSG_TYPE_PROPRIETARY      (LORANODEMANAGER_MSG_TYPE_BASE + 7)

// Time constants for LoRaWAN protocol (LoRaWAN specification for EU863-870)
// Values are in milliseconds

// Constants for standard RX Windows ('RECEIVE_DELAYx') 
#define LORANODEMANAGER_LORAWAN_RECEIVE_DELAY1   1000
#define LORANODEMANAGER_LORAWAN_RECEIVE_DELAY2   (LORANODEMANAGER_LORAWAN_RECEIVE_DELAY1 + 1000)

// Duration allowed to detect packet preamble
// Note: The gateway waits the maximum duration (delay between start of next minus time required 
//       to change radio settings). 
//       The endpoint may wait a smaller duration
#define LORANODEMANAGER_LORAWAN_RX_WINDOW_LENGTH  (LORANODEMANAGER_LORAWAN_RECEIVE_DELAY2 - LORANODEMANAGER_LORAWAN_RECEIVE_DELAY1 - 100)

// Constants for standard join accept delays (JOIN_ACCEPT_DELAYx)
#define LORANODEMANAGER_LORAWAN_JOIN_ACCEPT_DELAY1   5000
#define LORANODEMANAGER_LORAWAN_JOIN_ACCEPT_DELAY2   6000

// Constants for standard ACK timeout (ACK_TIMEOUT = random from 1s to 3s)
#define LORANODEMANAGER_LORAWAN_ACK_TIMEOUT_MIN      1000
#define LORANODEMANAGER_LORAWAN_ACK_TIMEOUT_MAX      3000


/********************************************************************************************* 
  Structures 
*********************************************************************************************/




/********************************************************************************************* 
 LoraPacketSession Class
 
 Manages session for uplink Lora packet
*********************************************************************************************/

// Class data
typedef struct _CLoraPacketSession
{
  // Current state of 'LoraPacketSession'
  // NOTE: Must be atomic because used by the different tasks or 'CLoraNodeManager' object
  //       without mutex
  DWORD m_dwSessionState;

  // Unique idenfifier of 'CLoraPacketSession'
  DWORD m_dwSessionId;

  // Interface to associated 'LoraTransceiver'
  ILoraTransceiver m_pLoraTransceiverItf;

  // Device Addr
  DWORD m_dwDeviceAddr;

  // Frame counter associated to received packet (i.e. depends on sender)
  WORD m_dwFrameCounter;

  // Message type
  BYTE m_usMHDR;
  BYTE m_usMessageType;

  // Packet timestamp
  DWORD m_dwTimestamp;

  // Additional information for received LoRa packet (uplink)
  CLoraTransceiverItf_ReceivedLoraPacketInfoOb m_ReceivedPacketInfo;

  // Access to this 'LoraPacketSession' object in 'm_pLoraPacketSessionArray' of parent 'CLoraNodeManager'
  CMemoryBlockArrayEntryOb m_LoraSessionEntry;

  // Access to associated LoRa packet in 'm_pLoraPacketArray' of parent 'CLoraNodeManager'
  CMemoryBlockArrayEntryOb m_LoraPacketEntry;


} CLoraPacketSessionOb;

typedef struct _CLoraPacketSession * CLoraPacketSession;


/********************************************************************************************* 
 LoraDownPacketSession Class
 
 Manages session for downlink Lora packet
*********************************************************************************************/

// Class data
typedef struct _CLoraDownPacketSession
{
  // Current state of 'LoraDownPacketSession'
  // NOTE: Must be atomic because used by the different tasks or 'CLoraNodeManager' object
  //       without mutex
  DWORD m_dwSessionState;

  // Unique idenfifier of 'CLoraDownPacketSession'
  DWORD m_dwSessionId;

  // Interface to 'LoraTransceiver' used to send packet
  ILoraTransceiver m_pLoraTransceiverItf;

  // Device Addr
  DWORD m_dwDeviceAddr;

  // Message type
  BYTE m_usMessageType;

  // Access to this 'LoraDownPacketSession' object in 'm_pLoraDownPacketSessionArray' of parent 'CLoraNodeManager'
  CMemoryBlockArrayEntryOb m_LoraSessionEntry;

  // Access to associated LoRa packet in 'm_pLoraPacketArray' of parent 'CLoraNodeManager'
  CMemoryBlockArrayEntryOb m_LoraPacketEntry;


} CLoraDownPacketSessionOb;

typedef struct _CLoraDownPacketSession * CLoraDownPacketSession;


// Class constants and definitions


// Class public methods


// Class private methods




/********************************************************************************************* 
 LoraTransceiverDescr Class
*********************************************************************************************/

typedef struct _CTransceiverDescr
{
  // Interface to associated 'LoraTransceiver'
  ILoraTransceiver m_pLoraTransceiverItf;

} CTransceiverDescrOb;

typedef struct _CTransceiverDescr * CTransceiverDescr;


/********************************************************************************************* 
  CLoraNodeManager_Message object

  Objects queued in 'LoraSession' Task (main automaton) for external commands received via 
  'ITransceiverManager' interface and for internal comman sent by other tasks
*********************************************************************************************/

// The event message (i.e. posted to event queue of owner object)
typedef struct _CLoraNodeManager_Message
{
  // Public
  WORD m_wMessageType;                            // The message
  DWORD m_dwMessageData;                          // Depends on message type
                                                  // Value or pointer to object
  DWORD m_dwMessageData2;                         // Depends on message type
} CLoraNodeManager_MessageOb;

typedef struct _CLoraNodeManager_Message * CLoraNodeManager_Message;


/********************************************************************************************* 
 LoraNodeManager Class
*********************************************************************************************/

// Class data
typedef struct _CLoraNodeManager
{
  // Interface
  ITransceiverManager m_pTransceiverManagerItf;

  uint32_t m_nRefCount;      // The 'CLoraNodeManager' object is reference counted (number of client 
                             // objects owning one of the public interfaces exposed by 'CLoraNodeManager')

  // Object main state
  DWORD m_dwCurrentState;

  //
  // Collection of 'LoraTransceiver'
  //

  // Actual number of 'LoraTransceiver' present in the Gateway
  BYTE m_usTransceiverNumber;

  // 'LoraTransceiver' descriptor array
  // Note: The 'LoraTransceiver' identifier is the index in this array
  CTransceiverDescrOb m_TransceiverDescrArray[GATEWAY_MAX_LORATRANSCEIVERS];


  //
  // Tasks and associated queues
  //

  // 'SessionManager' task (main automaton)
  // Used for internal messages and external commands via 'ITransceiverManager' interface
  TaskFunction_t m_hSessionManagerTask;
  QueueHandle_t m_hSessionManagerQueue;

  // For command processing by 'SessionManager' task
  SemaphoreHandle_t m_hCommandMutex;
  SemaphoreHandle_t m_hCommandDone;
  DWORD m_dwCommand;
  void *m_pCommandParams;


  // 'Transceiver' task (automaton for exchange with 'LoraTransceivers')
  // Used to process uplink packets received from 'LoraTransceiver' and notification for downlink packets
  // 'Sent' by 'LoraTransceiver' (i.e. via 'ILoraTransceiverItf')
  TaskFunction_t m_hTransceiverTask;
  QueueHandle_t m_hTransceiverNotifQueue;

  // 'Server' task (automaton for exchange with 'ServerManager')
  // Used to process downlink packets received from 'ServerManager' and notification for uplink packets
  // 'Sent' by the 'ServerManager' (i.e. via 'IServerManagerItf')
  TaskFunction_t m_hServerTask;
  QueueHandle_t m_hServerNotifQueue;

  //
  // Transmission of uplink packets to 'ServerManager' (i.e. the forward function of 'ServerManager')
  //

  // Task in 'CServerManager' object to ask for sending (forward) uplink packets to Network Server
  // A NULL value indicates that 'ServerManager' is not attached (see 'IServerManager_Attach' method)
  TaskFunction_t m_hPacketForwarderTask;

  // Packet currently sent in 'SENDING_UPLINK' 'LoraSession' state
  // Note: This object is owned by the 'CLoraNodeManager' object. It must be explicitly released by the
  //       'CServerManager' when it is no more required for the send operation (i.e. 'Send' method on 
  //       'IServerManager' interface) 
  CServerManagerItf_LoraSessionPacketOb m_ForwardedUplinkPacket;

  // LoRa Packet Session are uniquely identified
  // Counters for unique ID generation (uplink and downlink sessions)
  DWORD m_dwLastUpSessionId;
  DWORD m_dwLastDownSessionId;

  //
  // Transmission of downlink packets to 'Transceiver' (i.e. send of LoRa packets to nodes)
  //

  // Interface to dedicated object used to send LoRa packet just in time
  ILoraRealtimeSender m_pRealtimeSenderItf;

  //
  // Session management
  //

  // Memory block array for LoRa packets (i.e. whole payload used with 'LoraTransceiver')
  // This array contains both uplink and downlink LoRa packets
  CMemoryBlockArray m_pLoraPacketArray;

  // Memory block array for 'LoraPacketSession' (uplink)
  CMemoryBlockArray m_pLoraPacketSessionArray;

  // Memory block array for downlink sessions (items = 'LoraDownPacketSession')
  CMemoryBlockArray m_pLoraDownPacketSessionArray;


  // Properties
  DWORD m_dwMissedUplinkPacketdNumber;

} CLoraNodeManager;

// Class constants and definitions


// Methods for 'ITransceiverManager' interface implementation on 'LoraNodeManager' object
// The 'CTransceiverManagerItfImpl' structure provided by 'LoraNodeManager' object contains pointers to these methods 
uint32_t CLoraNodeManager_AddRef(void *this);
uint32_t CLoraNodeManager_ReleaseItf(void *this);

bool CLoraNodeManager_Initialize(void *this, void *pParams);
bool CLoraNodeManager_Attach(void *this, void *pParams);
bool CLoraNodeManager_Start(void *this, void *pParams);
bool CLoraNodeManager_Stop(void *this, void *pParams);
bool CLoraNodeManager_SessionEvent(void *this, void *pEvent);


// Construction
CLoraNodeManager * CLoraNodeManager_New();
void CLoraNodeManager_Delete(CLoraNodeManager *this);


// Task functions
// Note: The CLoraNodeManager object has 3 automatons
void CLoraNodeManager_SessionManagerAutomaton(CLoraNodeManager *this);
void CLoraNodeManager_TransceiverAutomaton(CLoraNodeManager *this);
void CLoraNodeManager_ServerAutomaton(CLoraNodeManager *this);


// Main Automaton (SessionManager task)
// Note: Numbering order MUST match object's life cycle
#define LORANODEMANAGER_AUTOMATON_STATE_CREATING      0
#define LORANODEMANAGER_AUTOMATON_STATE_CREATED       1
#define LORANODEMANAGER_AUTOMATON_STATE_INITIALIZED   2
#define LORANODEMANAGER_AUTOMATON_STATE_IDLE          3
#define LORANODEMANAGER_AUTOMATON_STATE_RUNNING       4
#define LORANODEMANAGER_AUTOMATON_STATE_STOPPING      5
#define LORANODEMANAGER_AUTOMATON_STATE_TERMINATED    6
#define LORANODEMANAGER_AUTOMATON_STATE_ERROR         7


#define LORANODEMANAGER_AUTOMATON_MSG_NONE               0x00000000
#define LORANODEMANAGER_AUTOMATON_MSG_COMMAND            0x00000001
//#define LORANODEMANAGER_AUTOMATON_MSG_NOTIFY             0x00000002

#define LORANODEMANAGER_AUTOMATON_MAX_CMD_DURATION       2000

#define LORANODEMANAGER_AUTOMATON_CMD_NONE         0x00000000
#define LORANODEMANAGER_AUTOMATON_CMD_INITIALIZE   0x00000001
#define LORANODEMANAGER_AUTOMATON_CMD_ATTACH       0x00000002
#define LORANODEMANAGER_AUTOMATON_CMD_START        0x00000003
#define LORANODEMANAGER_AUTOMATON_CMD_STOP         0x00000004


bool CLoraNodeManager_NotifyAndProcessCommand(CLoraNodeManager *this, DWORD dwCommand, void *pCmdParams);
bool CLoraNodeManager_ProcessAutomatonNotifyCommand(CLoraNodeManager *this);

bool CLoraNodeManager_ProcessInitialize(CLoraNodeManager *this, CTransceiverManagerItf_InitializeParams pParams);
bool CLoraNodeManager_ProcessAttach(CLoraNodeManager *this, CTransceiverManagerItf_AttachParams pParams);
bool CLoraNodeManager_ProcessStart(CLoraNodeManager *this, CTransceiverManagerItf_StartParams pParams);
bool CLoraNodeManager_ProcessStop(CLoraNodeManager *this, CTransceiverManagerItf_StopParams pParams);

void CLoraNodeManager_ProcessSessionEventUplinkAccepted(CLoraNodeManager *this, 
                                                        CTransceiverManagerItf_SessionEvent pEvent);
void CLoraNodeManager_ProcessSessionEventUplinkRejected(CLoraNodeManager *this, 
                                                        CTransceiverManagerItf_SessionEvent pEvent);
void CLoraNodeManager_ProcessSessionEventUplinkProgressing(CLoraNodeManager *this, 
                                                           CTransceiverManagerItf_SessionEvent pEvent);
void CLoraNodeManager_ProcessSessionEventUplinkSent(CLoraNodeManager *this, 
                                                    CTransceiverManagerItf_SessionEvent pEvent);
void CLoraNodeManager_ProcessSessionEventUplinkFailed(CLoraNodeManager *this, 
                                                      CTransceiverManagerItf_SessionEvent pEvent);
void CLoraNodeManager_ProcessSessionEventDownlinkScheduled(CLoraNodeManager *this, 
                                                           CTransceiverManagerItf_SessionEvent pEvent);
void CLoraNodeManager_ProcessSessionEventDownlinkSending(CLoraNodeManager *this, 
                                                         CTransceiverManagerItf_SessionEvent pEvent);
void CLoraNodeManager_ProcessSessionEventDownlinkSent(CLoraNodeManager *this, 
                                                      CTransceiverManagerItf_SessionEvent pEvent);
void CLoraNodeManager_ProcessSessionEventDownlinkFailed(CLoraNodeManager *this, 
                                                        CTransceiverManagerItf_SessionEvent pEvent, DWORD dwErrorCode);


// LoraSession state (uplink session)
#define LORANODEMANAGER_SESSION_STATE_CREATED             0
#define LORANODEMANAGER_SESSION_STATE_SENDING_UPLINK      1
#define LORANODEMANAGER_SESSION_STATE_PROGRESSING_UPLINK  2
#define LORANODEMANAGER_SESSION_STATE_UPLINK_SENT         3
#define LORANODEMANAGER_SESSION_STATE_UPLINK_FAILED       4



// LoraTransceiver Automaton (Transceiver task)

#define LORANODEMANAGER_TRANSCEIVER_MSG_NONE             0x00000000
#define LORANODEMANAGER_TRANSCEIVER_MSG_UPLINK_RECEIVED  0x00000001
#define LORANODEMANAGER_TRANSCEIVER_MSG_DOWNLINK_SENT    0x00000002


bool CLoraNodeManager_ProcessTransceiverUplinkReceived(CLoraNodeManager *this, CLoraTransceiverItf_Event pEvent);
bool CLoraNodeManager_ProcessTransceiverDownlinkSent(CLoraNodeManager *this, CLoraTransceiverItf_Event pEvent);


// Downlink session management

#define LORANODEMANAGER_DOWNSESSION_TYPE_ACK          0x0001
#define LORANODEMANAGER_DOWNSESSION_TYPE_DATA         0x0002

// LoraSession state (downlink session)
#define LORANODEMANAGER_DOWNSESSION_STATE_CREATED     0
#define LORANODEMANAGER_DOWNSESSION_STATE_SCHEDULING  1
#define LORANODEMANAGER_DOWNSESSION_STATE_SCHEDULED   2
#define LORANODEMANAGER_DOWNSESSION_STATE_SENDING     3
#define LORANODEMANAGER_DOWNSESSION_STATE_SENT        4
#define LORANODEMANAGER_DOWNSESSION_STATE_FAILED      5

typedef struct _CLoraNodeManager_ProcessServerDownlinkReceivedParams
{
  // Public
  DWORD m_dwSessionType;
  DWORD m_dwTimestamp;
  DWORD m_dwPayloadSize;
  BYTE *m_pPayload;
  DWORD m_dwDeviceAddr;
  ILoraTransceiver m_pLoraTransceiverItf;
} CLoraNodeManager_ProcessServerDownlinkReceivedParamsOb;

typedef struct _CLoraNodeManager_ProcessServerDownlinkReceivedParams * CLoraNodeManager_ProcessServerDownlinkReceivedParams;

bool CLoraNodeManager_ProcessServerDownlinkReceived(CLoraNodeManager *this, CLoraNodeManager_ProcessServerDownlinkReceivedParams pParams);
void CLoraNodeManager_ReleaseDownlinkSession(CLoraNodeManager *this, CLoraDownPacketSession pLoraPacketSession);



// Class public methods






// Class private methods (implementation helpers)



#endif




