/*****************************************************************************************//**
 * @file     ServerConnectorItf.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     25/03/2018
 *
 * @brief    A ServerConnector manages a network interface (device) for connection to the 
 *           network used to reach the LoRa Network Server from the gateway.
 *
 * @details  This class is the interface exposed by objects managing a physical network device
 *           used for data transmissions between the gateway and LoRa Network Server.\n
 *
 * @note     This class IS THREAD SAFE.
*********************************************************************************************/

#ifndef SERVERCONNECTORITF_H
#define SERVERCONNECTORITF_H

#include "ServerManagerItf.h"

// Forward declaration to interface object (used as this on interface methods)
typedef struct _IServerConnector * IServerConnector;


/********************************************************************************************* 
  Public definitions used by methods of 'IServerConnector' interface
*********************************************************************************************/



/********************************************************************************************* 
  Objects used as parameter on methods of 'IServerConnector' interface
*********************************************************************************************/

// Forward declarations (objects used to exchange data on interface)
typedef struct _CServerConnectorItf_InitializeParams * CServerConnectorItf_InitializeParams;
typedef struct _CServerConnectorItf_StartParams * CServerConnectorItf_StartParams;
typedef struct _CServerConnectorItf_StopParams * CServerConnectorItf_StopParams;

typedef struct _CServerConnectorItf_SendParams * CServerConnectorItf_SendParams;
typedef struct _CServerConnectorItf_SendReceiveParams * CServerConnectorItf_SendReceiveParams;
typedef struct _CServerConnectorItf_DownlinkReceivedParams * CServerConnectorItf_DownlinkReceivedParams;

//typedef struct _CServerConnectorItf_LoraSessionPacket * CServerConnectorItf_LoraSessionPacket;

/*

// Configuration for a 'ServerConnector'
// Note: 
//  - This object is used in 'params' object of 'IServerConnector_Initialize' method
//  - Typically, the gateway configuration file contains a similar structure for settings definition
// 
typedef struct _CServerConnectorItf_ConnectorSettings
{
  // Public
  char m_szNetworkName[64];                  // SSID for Wifi Network
  char m_szNetworkUser[32];                  // Not used for Wifi Network
  char m_szNetworkPassword[32];              // Password for Network (Wifi or GPRS)
  DWORD m_dwNetworkJoinTimeout;

  char m_szNetworkServerUrl[64];
  DWORD m_dwNetworkServerPort;
  DWORD m_dwNetworkServerTimeout;

  char m_szSNTPServerUrl[32];
  DWORD m_dwSNTPServerPeriodSec;

  BYTE m_GatewayMACAddr[6];

} CServerConnectorItf_ConnectorSettingsOb;

typedef struct _CServerConnectorItf_ConnectorSettings * CServerConnectorItf_ConnectorSettings;

*/


typedef struct _CServerConnectorItf_InitializeParams
{
  // Public

  // Settings
  CServerManagerItf_ConnectorSettings m_pConnectorSettings;

  // Queue for direct notification of downlink events (i.e. data received from network)
  QueueHandle_t m_hEventNotifyQueue;

  // Interface of 'ServerManager' object using the 'Connector' object
  // The 'Connector' uses this interface to notify the result of uplink operations asked by the 
  // ServerManager (i.e. 'IServerManager_ServerMessageEvent' method)
  void * m_pServerManagerItf; 

} CServerConnectorItf_InitializeParamsOb;

// Administrative commands

typedef struct _CServerConnectorItf_StartParams
{
  // Public
  bool m_bForce;
} CServerConnectorItf_StartParamsOb;


typedef struct _CServerConnectorItf_StoptParams
{
  // Public
  bool m_bForce;
} CServerConnectorItf_StopParamsOb;



typedef struct _CServerConnectorItf_SendParams
{
  // Public
  WORD m_wDataLength;
  BYTE *m_pData;
  void *m_pMessage;                   // Not used by 'ServerConnector' (parameter for notification)
  DWORD m_dwMessageId;                // Not used by 'ServerConnector' (parameter for notification)
} CServerConnectorItf_SendParamsOb;


typedef struct _CServerConnectorItf_SendReceiveParams
{
  // Public
  BYTE *m_pData;
  WORD m_wDataLength;
  BYTE *m_pReply; 
  WORD m_wReplyMaxLength;
  WORD m_wReplyLength;
  DWORD m_dwTimeoutMillisec;
} CServerConnectorItf_SendReceiveParamsOb;


typedef struct _CServerConnectorItf_DownlinkReceivedParams
{
  // Public
  DWORD m_dwMessageId;
} CServerConnectorItf_DownlinkReceivedParamsOb;


/********************************************************************************************* 
  Objects used as parameter for direct notification to ServerManager event queue
*********************************************************************************************/


/********************************************************************************************* 
  CServerConnectorItf_ServerDownlinkMessage object

  Used to transmit to 'ServerManager' the downlink messages received from Network Server on the
  'Connector':
   - The 'Connector' maintains a memory block containing the received data
   - The 'ServerManager' invokes the 'DownlinkReceived' method on 'ServerConnectorItf' interface
     to indicate that memory block can be reused by the 'Connector'. 
     Typically, the 'ServerManager' reads the data and builds a LoRa packet in order to transmit
     it to target node.
*********************************************************************************************/
 
typedef struct _CServerConnectorItf_ServerDownlinkMessage
{
  // Public
  IServerConnector m_pConnectorItf;   // 'IServerConnector' interface to invoke when downlink
                                      // message is processed  
                                      // (i.e. to call 'IServerConnector_DownlinkReceived'
                                      // method)

  DWORD m_dwMessageId;           // Identifier of message in 'Connector' object (i.e. depends of
                                 // implementation = not a protocol identifier).
                                 // Typically the identifier of the MemoryBlock used to store
                                 // received message data.
                                 // The 'DownlinkReceived' method references the message using
                                 // this identifer.

  // System tick count when message data is received.
  // This is used by the 'ProtocolEngine' (and 'Transceiver') for LoRaWAN protocol time rules 
  // (i.e. uplink RX windows)
  DWORD m_dwTimestamp;

  // Size of received message (= number of significant bytes in m_pData)
  WORD m_wDataSize;

  // Received message
  BYTE *m_pData;

} CServerConnectorItf_ServerDownlinkMessageOb;

typedef CServerConnectorItf_ServerDownlinkMessageOb * CServerConnectorItf_ServerDownlinkMessage;


/********************************************************************************************* 
  CServerConnectorItf_ConnectorEvent object

  Used to notify events occurred on 'Connector'.
  Differents types of events are transmitted using this notification:
   - Events related to processing of Uplink messages
   - Events used to transmit Downlink messages received from Network Server

  Note: The 'ServerConnector' must always use the 'IServerConnectorItf' to send notifications 
        to 'ServerManager'. The 'IServerConnetorItf' mechanism for notification is direct post
        of 'CServerConnectorItf_ConnectorEvent' in 'ServerManager' notify queue.
        In other words, the 'Connector' is not allowed to use the 'IServerManagerItf' interface.
        This rule is required for event serialization in 'ServerManager' automaton.
*********************************************************************************************/
 
typedef struct _CServerConnectorItf_ConnectorEvent
{
  WORD m_wConnectorEventType;     // Values are 'SERVERCONNECTOR_CONNECTOREVENT_xxx'
  union
  {
    CServerManagerItf_ServerMessageEventOb m_ServerMessageEvent;
    CServerConnectorItf_ServerDownlinkMessageOb m_DownlinkMessage;
  };
} CServerConnectorItf_ConnectorEventOb;

typedef CServerConnectorItf_ConnectorEventOb * CServerConnectorItf_ConnectorEvent;

#define SERVERCONNECTOR_CONNECTOREVENT_SERVERMSG_EVENT      0x0001
#define SERVERCONNECTOR_CONNECTOREVENT_DOWNLINK_RECEIVED    0x0002


/*
// Note: Base for separation from other commands or internal messages in 'ServerConnector'
#define SERVERCONNECTOR_MESSAGEEVENT_BASE                0x00001000

#define SERVERCONNECTOR_MESSAGEEVENT_UPLINK_RECEIVED     (SERVERCONNECTOR_MESSAGEEVENT_BASE)
#define SERVERCONNECTOR_MESSAGEEVENT_UPLINK_PREPARED     (SERVERCONNECTOR_MESSAGEEVENT_BASE + 1)
#define SERVERCONNECTOR_MESSAGEEVENT_UPLINK_SEND_FAILED  (SERVERCONNECTOR_MESSAGEEVENT_BASE + 2)
#define SERVERCONNECTOR_MESSAGEEVENT_UPLINK_SENT         (SERVERCONNECTOR_MESSAGEEVENT_BASE + 3)
#define SERVERCONNECTOR_MESSAGEEVENT_UPLINK_FAILED       (SERVERCONNECTOR_MESSAGEEVENT_BASE + 4)
#define SERVERCONNECTOR_MESSAGEEVENT_DOWNLINK_RECEIVED   (SERVERCONNECTOR_MESSAGEEVENT_BASE + 5)
#define SERVERCONNECTOR_MESSAGEEVENT_DOWNLINK_SENT       (SERVERCONNECTOR_MESSAGEEVENT_BASE + 6)

*/

/********************************************************************************************* 
  Public methods of 'IServerConnector' interface
 
  These methods are available to any object which has obtained a reference to the
  'IServerConnector' interface (i.e. this reference is required with all interface methods)
*********************************************************************************************/

uint32_t IServerConnector_AddRef(IServerConnector this);
uint32_t IServerConnector_ReleaseItf(IServerConnector this);

bool IServerConnector_Initialize(IServerConnector this, CServerConnectorItf_InitializeParams pParams);
bool IServerConnector_Start(IServerConnector this, CServerConnectorItf_StartParams pParams);
bool IServerConnector_Stop(IServerConnector this, CServerConnectorItf_StopParams pParams);

bool IServerConnector_Send(IServerConnector this, CServerConnectorItf_SendParams pParams);
bool IServerConnector_SendReceive(IServerConnector this, CServerConnectorItf_SendReceiveParams pParams);
bool IServerConnector_DownlinkReceived(IServerConnector this, CServerConnectorItf_DownlinkReceivedParams pParams);



#ifdef SERVERCONNECTORITF_IMPL

/********************************************************************************************* 
  Classes and definition used by objects which expose the 'IServerConnector' interface and
  by the interface implementation itself
 
  Note: The implementation file of these objects must define 'SERVERCONNECTORITF_IMPL'
        (i.e. in order to include the following block of code) 
*********************************************************************************************/

#include "ServerConnectorItfImpl.h"

/********************************************************************************************* 
  Forward declarations
*********************************************************************************************/
           

/********************************************************************************************* 
  Interface class instanced by objects implementing the interface methods                                                              .
                                                                                                                                        .
  This object is instanced by 'IServerConnector_New' method (protected constructor)                                                                           .
*********************************************************************************************/

// Interface class data
typedef struct _IServerConnector
{
  // Private attributes
  void *m_pOwnerObject;                          // Instance of the object exposing the interface
                                                 // This pointer is provided as argument on interface methods
  CServerConnectorItfImpl m_pOwnerItfImpl;       // Pointers to implementation functions on owner object
} IServerConnectorOb;

typedef struct _IServerConnector * IServerConnector;


/********************************************************************************************* 
  Protected methods used by objects implementing the 'IServerConnector' interface
*********************************************************************************************/
 
IServerConnector IServerConnector_New(void *pOwnerObject, CServerConnectorItfImpl pOwnerItfImpl);
void IServerConnector_Delete(IServerConnector this);


#endif
#endif



