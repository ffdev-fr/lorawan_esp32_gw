/*****************************************************************************************//**
 * @file     SemtechProtocolEngine.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     26/03/2018
 *
 * @brief    Encodes and decodes messages exchanged with a Network Server using the Semtech
 *           protocol.
 *
 * @details  The Semtech protocol is the legacy protocol used on many LoRaWAN network.
 *           The main functions are for encoding and decoding LoRaWAN frames ('CLoraPacket')
 *           to/from proprietary messages used by the Semtech Network Server protocol.
 *           The "Semtech Network Server Protocol" uses JSON messages. In JSON message, the
 *           the variable for LoRaWan packet contains packet's bytes encoded in Base64.
 *
 * @note     The "LoRaWAN ESP32 Gateway V1.x" project is designed for execution on ESP32 
 *           Module (Dev.C kit).\n
 *           The implementation uses the Espressif IDF V3.0 framework (with RTOS)              
*********************************************************************************************/

#ifndef SEMTECHPROTOCOLENGINE_H_
#define SEMTECHPROTOCOLENGINE_H_

/*********************************************************************************************
  Includes
*********************************************************************************************/

#include "Utilities.h"


/********************************************************************************************* 
  Definitions for debug traces
  The debug level is specified with 'SEMTECHPROTOCOLENGINE_DEBUG_LEVEL' in Definitions.h file
*********************************************************************************************/

#define SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0 ((SEMTECHPROTOCOLENGINE_DEBUG_LEVEL & 0x01) > 0)
#define SEMTECHPROTOCOLENGINE_DEBUG_LEVEL1 ((SEMTECHPROTOCOLENGINE_DEBUG_LEVEL & 0x02) > 0)
#define SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2 ((SEMTECHPROTOCOLENGINE_DEBUG_LEVEL & 0x04) > 0)


/********************************************************************************************* 
  Definitions (implementation)
*********************************************************************************************/


// Number of items in memory array for 'CSemtechMessageTransactionOb'
// Typically, the ACK message associated to a Semtech transaction should be sent/received
// quickly (i.e. just a confirmation that data are taken in account by the server or gateway).
// 
// IMPORTANT NOTE:
//  - For optimization, allowed values are a power of 2 (2, 4, 8, 16 ...)
//  - For details, see how Semtech Message identifier is generated
//  - The Semtech Mesage identified is a WORD (16 bits)
#define SEMTECHPROTOCOLENGINE_MAX_TRANSACTION_BITS   3
#define SEMTECHPROTOCOLENGINE_MAX_TRANSACTIONS       ((0x01 << SEMTECHPROTOCOLENGINE_MAX_TRANSACTION_BITS) * 2)
#define SEMTECHPROTOCOLENGINE_TRANSACTION_ID_MASK    (0xFFFF >> (16 - SEMTECHPROTOCOLENGINE_MAX_TRANSACTION_BITS))


// States for Semtech message transaction
// The sequence of states depends on message type:
//  - For uplink messages: 
//     .. SENDING = The message will be transmited to the transport layer
//     .. SENT = The message has been sent to NetworkServer. Waiting for 'ACK'
//     .. The 'Transaction' object is deleted when send failed, 'ACK' received or timeout
#define SEMTECHPROTOCOLENGINE_TRANSACTION_STATE_UNKNOWN    0
#define SEMTECHPROTOCOLENGINE_TRANSACTION_STATE_SENDING    0x0001
#define SEMTECHPROTOCOLENGINE_TRANSACTION_STATE_SENT       0x0002


// Constants for Semtech protocol

// The protocol version is 2
#define SEMTECHPROTOCOLENGINE_SEMTECH_PROTOCOL_VERSION    2

// Message types (i.e. the Semtech protocol values)
#define SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_PUSH_DATA   0
#define SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_PUSH_ACK    1
#define SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_PULL_DATA   2
#define SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_PULL_RESP   3
#define SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_PULL_ACK    4
#define SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_TX_ACK      5



/********************************************************************************************* 
  Structures 
*********************************************************************************************/






/********************************************************************************************* 
 SemtechMessageTransaction Class

 This class maintains a message transaction executed with the Semtech Network Server.
 The Semtech Network Server Protocol uses the "Message / Acknowledge" paradigm.
 The 'SemtechMessageTransaction' object is used to wait, receive and process 'Acknowledge' for
 each 'Message' exchanged with the Network Server (i.e. uplink message sent or downlink message
 received).
 
 Note: This object is exclusively used by 'SemtechProtocolEngine' (private).
       The 'SemtechMessageTransaction' maintains these object in a 'MemoryBlockArray'.
       The 'SemtechMessage' identifier is the index of 'SemtechMessageTransaction' in this
       'MemoryBlockArray'.
*********************************************************************************************/

// Class data
typedef struct _CSemtechMessageTransaction
{
  // Idenfifier of the 'SemtechMessageTransaction'
  // Note: This identifier is the index in the 'MemoryBlockArray' used to maintain the
  //       'SemtechMessageTransaction' collection (i.e. Semtech Protocol messages)
  BYTE m_usTransactionId;

  // Type of Semtech protocol transaction
  // Values are 'SEMTECHMESSAGETRANSACTION_TYPE_xxx'
  // Typically defined by accoridng to operation which initiate the transaction
  BYTE m_usTransactionType;

  // Random identifier used for the associated Semtech Protocol message
  // Used to retrieve the transaction associated to a Semtech message (i.e. this identifier is
  // part of Semtech protocol)
  // Note: This identifer is optimized in order to contain the 'm_usTransactionId' for quick
  //       retrieval of transaction in 'MemoryBlockArray' 
  WORD m_wMessageId;

  // Identifier of message in both 'CLoraServerManager' and associated 'ProtocolEngine'
  // The identifier format is:
  //  - LOWORD = Identifier in 'ProtocolEngine'. 
  //             This identifer depends on 'ProtocolEngine' implementation and it is used to 
  //             retrieve the protocol session associated to the message.
  //             For 'CSemtechMessageTransaction' object the identifier is 'm_wMessageId'
  //  - HIWORD = Identifier in 'CLoraServerManager'.
  //             This identifier is provided by 'CLoraServerManager'
  //
  // Note: This field is set on first call to the interface methode which instanciate the transaction
  DWORD m_dwProtocolMessageId;

  // Message type (SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_xxx)
  // Type of last sent/received Semtech message
  WORD m_wMessageType;

  // Message is heartbeat (i.e. not a forwarded LoRa packet)
  bool m_bHeartbeat;

  // State of Semtech message transaction ('SEMTECHPROTOCOLENGINE_TRANSACTION_STATE_xxx')
  WORD m_wTransactionState;

  // Tick count for transaction start
  TickType_t m_dwTransactionStartTicks;

  // Tick count for last event
  TickType_t m_dwLastEventTicks;

} CSemtechMessageTransactionOb;

typedef struct _CSemtechMessageTransaction * CSemtechMessageTransaction;


// Class constants and definitions

// Types of transaction
#define SEMTECHMESSAGETRANSACTION_TYPE_UNKOWNN      0
#define SEMTECHMESSAGETRANSACTION_TYPE_PUSHDATA     1             // PUSH_DATA message sent to Network Server
#define SEMTECHMESSAGETRANSACTION_TYPE_PULLDATA     2             // PULL_DATA message sent to Network Server
#define SEMTECHMESSAGETRANSACTION_TYPE_PULLRESP     3             // PULL_RESP message received from Network Server

// Class public methods


// Class private methods





/********************************************************************************************* 
 SemtechProtocolEngine Class
*********************************************************************************************/

// Class data
typedef struct _CSemtechProtocolEngine
{
  // Interface
  INetworkServerProtocol m_pNetworkServerProtocolItf;

  uint32_t m_nRefCount;      // The 'CSemtechProtocolEngine' object is reference counted (number of client 
                             // objects owning one of the public interfaces exposed by 'CSemtechProtocolEngine')

  // Object main state
//DWORD m_dwCurrentState;

  // Collection of 'SemtechMessageTransaction' currently active
  // Memory block array for 'CSemtechMessageTransactionOb' objects
  CMemoryBlockArray m_pTransactionArray;

  // Counter for generation of message identifier
  // Note: To avoid identifier with a zero value, the first value for this counter is 1
  WORD m_wMessageIdCounter; 

  // Gateway identifier
  // By specification (Semtech Protocol) this identifier is the MAC address of the gateway with 2 additional bytes
  // (value depending of NetworkServer).
  // NOTE: This variable contains the byte value (i.e. not a string representation)
  // NOTE: The gateway may have more than one 'Connector' and so more than one MAC address.
  //       May be just an identifier for the Network Server and not related to UDP messages.
  BYTE m_GatewayMACAddr[8];
                     
  // Actual number of 'ServerConnector' present in the Gateway
  BYTE m_usConnectorNumber;

  // Counters used in messages for Semtech protocol
  // Note:
  //  - These counters are updated on protocol events (i.e. sent and received messages) processed
  //    by the CSemtechProtocolEngine
  //  - These counters are used to build the 'stat' block in PUSH_DATA messages
  DWORD m_dwRxnbCount;             // Number of LoRa packets received by gateway
  DWORD m_dwRxokCount;             // Number of LoRa packets received by gateway with valid CRC
  DWORD m_dwRxfwCount;             // Number of LoRa packets forwarded to Network Server
  DWORD m_dwDwnbCount;             // Number of PULL_RESP received by gateway (from Network Server)
  DWORD m_dwTxnbCount;             // Number of packets transmited to LoRa nodes by gateway

  DWORD m_dwUpnbCount;             // Number of uplink messages sent by gateway to Network Server 
                                   // for any kinds of messages (i.e. not only for Lora packets)
  DWORD m_dwAckrCount;             // Number of ACK received by gateway (from Network Server) 
                                   // for any kinds of messages (i.e. not only for Lora packets)

  // Gateway GPS coordinates
  // Typically provided during initialization (from configuration or GPS data)
  // Optimized for execution time in message formating functions 
  BYTE m_strGatewayLatitude[9];
  BYTE m_strGatewayLongitude[10];
  BYTE m_strGatewayAltitude[5];
  WORD m_wGatewayLatitudeLength;
  WORD m_wGatewayLongitudeLength;
  WORD m_wGatewayAltitudeLength;

  // Timestamps for periodical treatments
  TickType_t m_dwLastPushDataTicks;               // Last uplink 'PUSH_DATA' message (heartbeart or Lora data)
  TickType_t m_dwLastPullDataTicks;               // Last uplink 'PULL_DATA' message

  // Number of pending uplink transactions (i.e. number of ACK expected)
  // This counter is for debug: 
  //  - For LoRa packet message, the LoraServerManager invokes 'CANCEL' when received window RX2 has expired
  //  - For Heartbeat, the LoraServerManager invokes 'CANCEL' using a timeout rule
  WORD m_wPendingUpTransactionCount;


/*
  // 'ServerConnector' descriptor array
  // Note: The 'ServerConnector' identifier is the index in this array
//CConnectorDescrOb m_ConnectorDescrArray[GATEWAY_MAX_SERVERCONNECTORS];


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
  //

  // Task in 'CLoraNodeManager' object to ask for sending (forward) downlink packet to node device
  // A NULL value indicates that 'LoraNodeManager' is not attached (see 'ILoraNodeManager_Attach' method)
//TaskFunction_t m_hPacketForwarderTask;

  // Uplink packet currently sent to the 'LoraNodeManager'
  // Note: This object is owned by the 'CSemtechProtocolEngine' object. It must be explicitly released by the
  //       'CLoraNodeManager' when it is no more required for the send operation (i.e. 'Send' method on 
  //       'ITransceiverManager' interface) 
//CServerManagerItf_LoraSessionPacketOb m_ForwardedDownlinkPacket;

  //
  // Server message management
  //

  // Memory block array for 'LoraServerMessages' (i.e. uplink messages for Network Server)
  CMemoryBlockArray m_pLoraServerUpMessageArray;

  // Interface to 'NetworkServerProtocol' engine
//INetworkServerProtocol m_pNetworkServerProtocolItf;

  // Properties
//DWORD m_dwMissedUplinkPacketdNumber;

*/

} CSemtechProtocolEngine;

// Class constants and definitions


// Methods for 'INetworkServerProtocol' interface implementation on 'SemtechProtocolEngine' object
// The 'CNerworkServerProtocolItfImpl' structure provided by 'SemtechProtocolEngine' object contains pointers to these methods 
uint32_t CSemtechProtocolEngine_AddRef(void *this);
uint32_t CSemtechProtocolEngine_ReleaseItf(void *this);

bool CSemtechProtocolEngine_BuildUplinkMessage(void *this, CNetworkServerProtocolItf_BuildUplinkMessageParams pParams);
DWORD CSemtechProtocolEngine_ProcessServerMessage(void *this, CNetworkServerProtocolItf_ProcessServerMessageParams pParams);
DWORD CSemtechProtocolEngine_ProcessSessionEvent(void *this, CNetworkServerProtocolItf_ProcessSessionEventParams pParams);


// Construction
CSemtechProtocolEngine * CSemtechProtocolEngine_New();
void CSemtechProtocolEngine_Delete(CSemtechProtocolEngine *this);



// Class public methods



// Class private methods (implementation helpers)
WORD CSemtechProtocolEngine_GetNewMessageId(CSemtechProtocolEngine *this, BYTE usTransactionId);
BYTE * CSemtechProtocolEngine_GetStatStream(CSemtechProtocolEngine *this, BYTE *pStreamData);
DWORD CSemtechProtocolEngine_GetElapsedTicks(DWORD dwCurrentTicks, DWORD dwPreviousTicks);


#endif






