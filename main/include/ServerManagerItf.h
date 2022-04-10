/*****************************************************************************************//**
 * @file     ServerManagerItf.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     10/03/2018
 *
 * @brief    A ServerManager manages a collection of ServerConnectors (i.e. devices for
 *           connection to the network used to reach the LoRa Network Server from the gateway).
 *
 * @details  This class is the interface exposed by objects managing a collection of network
 *           devices used for packet transmissions between the gateway and LoRa Network Server.\n
 *
 * @note     This class IS THREAD SAFE.
*********************************************************************************************/

#ifndef SERVERMANAGERITF_H
#define SERVERMANAGERITF_H

//#include "ServerConnectorItf.h"

// Forward declaration to interface object (used as this on interface methods)
typedef struct _IServerManager * IServerManager;


/********************************************************************************************* 
  Public definitions used by methods of 'IServerManager' interface
*********************************************************************************************/



/********************************************************************************************* 
  Objects used as parameter on methods of 'IServerManager' interface
*********************************************************************************************/

// Forward declarations (objects used to exchange data on interface)
typedef struct _CServerManagerItf_InitializeParams * CServerManagerItf_InitializeParams;
typedef struct _CServerManagerItf_AttachParams * CServerManagerItf_AttachParams;
typedef struct _CServerManagerItf_StartParams * CServerManagerItf_StartParams;
typedef struct _CServerManagerItf_StopParams * CServerManagerItf_StopParams;

typedef struct _CServerManagerItf_LoraSessionPacket * CServerManagerItf_LoraSessionPacket;


// Configuration for a 'ServerConnector'
// Note: 
//  - This object is used in 'params' object of 'IServerConnector_Initialize' method
//  - Typically, the gateway configuration file contains a similar structure for settings definition
// 
typedef struct _CServerManagerItf_ConnectorSettings
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

} CServerManagerItf_ConnectorSettingsOb;

typedef struct _CServerManagerItf_ConnectorSettings * CServerManagerItf_ConnectorSettings;


// Configuration for a 'ServerManager'
// Note: 
//  - This object is used in 'params' object of 'IServerManager_Initialize' method
//  - Typically, the gateway configuration file contains a similar structure for settings definition
//  - This object includes configuration settings for embedded objects (i.e. access via 
//    'IServerConnector' and 'INetworkServerProtocol' interfaces)
typedef struct _CServerManagerItf_LoraServerSettings
{
  // Public
  // Note: One or more 'ServerConnector' used with a single 'Lora Network Server'

  // Network access
  CServerManagerItf_ConnectorSettingsOb ConnectorSettings[GATEWAY_MAX_SERVERCONNECTORS];

  // Network Server access
  WORD m_wNetworkServerProtocol;               // Allowed values are 'SERVERMANAGER_PROTOCOL_xxx'
  char m_szNetworkServerUrl[64];
  DWORD m_dwNetworkServerPort;
  char m_szNetworkServerUser[32];
  char m_szNetworkServerPassword[32];
  char m_szGatewayIDToken[17];

  // SNTP Server access (optionnal = if no hardware RTC available)
  DWORD m_dwSNTPServerPeriodSec;               // The 0 value indicates not required 
  char m_szSNTPServerUrl[32];

  // Gateway MAC Address is required in UDP transport and messages sent to Network Server
  BYTE m_GatewayMACAddr[6];

} CServerManagerItf_LoraServerSettingsOb;

typedef struct _CServerManagerItf_LoraServerSettings * CServerManagerItf_LoraServerSettings;

// Values for 
#define SERVERMANAGER_PROTOCOL_UNKNOWN     0
#define SERVERMANAGER_PROTOCOL_SEMTECH     1


typedef struct _CServerManagerItf_InitializeParams
{
  // Public
  // Configuration settings
  bool m_bUseBuiltinSettings;    
  CServerManagerItf_LoraServerSettingsOb LoraServerSettings;

  // Transceiver to connect to ServerManager (i.e. Node side of the gateway)
  // The 'Attach' method on this 'ITransceiverManager' interface is invoked to provide the
  // ServerManager's task to notify when new packets are received
  void * pTransceiverManagerItf; 

} CServerManagerItf_InitializeParamsOb;

// Administrative commands
typedef struct _CServerManagerItf_AttachParams
{
  // Public
  TaskFunction_t m_hNodeManagerTask;
} CServerManagerItf_AttachParamsOb;


typedef struct _CServerManagerItf_StartParams
{
  // Public
  bool m_bForce;
} CServerManagerItf_StartParamsOb;


typedef struct _CServerManagerItf_StoptParams
{
  // Public
  bool m_bForce;
} CServerManagerItf_StopParamsOb;


// Uplink packets are transmitted using direct RTOS notification (see 'Attach' method)
// The notification variable is a pointer to 'CServerManagerItf_LoraSessionPacket' object

typedef struct _CServerManagerItf_LoraSessionPacket
{
  // Public
  void *m_pLoraPacket;                // 'CLoraTransceiverItf_LoraPacket' object
  void *m_pLoraPacketInfo;            // 'CLoraTransceiverItf_ReceivedLoraPacketInfo' object                    
  void *m_pSession;                   // Associated session for LoraPacket 
                                      // Significant only for calling object
                                      // NOTE: NO ACCESS ALLOWED
  DWORD m_dwSessionId;                // Unique identifier
} CServerManagerItf_LoraSessionPacketOb;



typedef struct _CServerManagerItf_Event
{
  // Public
  void *m_pLoraPacket;                // 'CLoraTransceiverItf_LoraPacket' object
  void *m_pSession;                   // Associated session for LoraPacket 
                                      // Significant only for calling object
} CServerManagerItf_EventOb;


/********************************************************************************************* 
  CServerManagerItf_ServerMessageEvent object

  Used to notify the 'ServerManager' of status changes of the 'LoraServerMesssage' in gateway
  (i.e. messages exchanged between gateway and LoRa Network Server).
  Typically, the 'ServerManager' uses these events to apply LoRa Network Server protocol rules
  for the message (i.e. 'LoraServerMessage' life cycle)
*********************************************************************************************/

typedef struct _CServerManagerItf_ServerMessageEvent
{
  // Public
  WORD m_wEventType;             // The event ('SERVERMANAGER_MESSAGEEVENT_xxx')
  void *m_pMessage;              // Pointer to 'CLoraServerUpMessage' 
  DWORD m_dwParam;               // Additional parameter (depends on message)
                                 
//  DWORD m_dwMessageId;           // Identifier of message (index of 'CLoraServerUpMessage' in
                                 // MemoryBlockArray)
                                 // Note: DWORD to map generic message in task queue of
                                 //       'LoraServerManager' main automaton
} CServerManagerItf_ServerMessageEventOb;

typedef CServerManagerItf_ServerMessageEventOb * CServerManagerItf_ServerMessageEvent;

// Note: Base for separation from other commands or internal messages in 'ServerManager'
#define SERVERMANAGER_MESSAGEEVENT_BASE                0x00001000

#define SERVERMANAGER_MESSAGEEVENT_UPLINK_RECEIVED     (SERVERMANAGER_MESSAGEEVENT_BASE)
#define SERVERMANAGER_MESSAGEEVENT_UPLINK_PREPARED     (SERVERMANAGER_MESSAGEEVENT_BASE + 1)
#define SERVERMANAGER_MESSAGEEVENT_UPLINK_SEND_FAILED  (SERVERMANAGER_MESSAGEEVENT_BASE + 2)
#define SERVERMANAGER_MESSAGEEVENT_UPLINK_SENT         (SERVERMANAGER_MESSAGEEVENT_BASE + 3)
#define SERVERMANAGER_MESSAGEEVENT_UPLINK_TERMINATED   (SERVERMANAGER_MESSAGEEVENT_BASE + 4)
#define SERVERMANAGER_MESSAGEEVENT_DOWNLINK_RECEIVED   (SERVERMANAGER_MESSAGEEVENT_BASE + 5)
#define SERVERMANAGER_MESSAGEEVENT_DOWNLINK_SENT       (SERVERMANAGER_MESSAGEEVENT_BASE + 6)


/********************************************************************************************* 
  Public methods of 'IServerManager' interface
 
  These methods are available to any object which has obtained a reference to the
  'IServerManager' interface (i.e. this reference is required with all interface methods)
*********************************************************************************************/

uint32_t IServerManager_AddRef(IServerManager this);
uint32_t IServerManager_ReleaseItf(IServerManager this);

bool IServerManager_Initialize(IServerManager this, CServerManagerItf_InitializeParams pParams);
bool IServerManager_Attach(IServerManager this, CServerManagerItf_AttachParams pParams);
bool IServerManager_Start(IServerManager this, CServerManagerItf_StartParams pParams);
bool IServerManager_Stop(IServerManager this, CServerManagerItf_StopParams pParams);
bool IServerManager_ServerMessageEvent(IServerManager this, CServerManagerItf_ServerMessageEvent pEvent);



#ifdef SERVERMANAGERITF_IMPL

/********************************************************************************************* 
  Classes and definition used by objects which expose the 'IServerManager' interface and
  by the interface implementation itself
 
  Note: The implementation file of these objects must define 'SERVERMANAGERITF_IMPL'
        (i.e. in order to include the following block of code) 
*********************************************************************************************/

#include "ServerManagerItfImpl.h"

/********************************************************************************************* 
  Forward declarations
*********************************************************************************************/
           

/********************************************************************************************* 
  Interface class instanced by objects implementing the interface methods                                                              .
                                                                                                                                        .
  This object is instanced by 'IServerManager_New' method (protected constructor)                                                                           .
*********************************************************************************************/

// Interface class data
typedef struct _IServerManager
{
  // Private attributes
  void *m_pOwnerObject;                          // Instance of the object exposing the interface
                                                 // This pointer is provided as argument on interface methods
  CServerManagerItfImpl m_pOwnerItfImpl;         // Pointers to implementation functions on owner object
} IServerManagerOb;



/********************************************************************************************* 
  Protected methods used by objects implementing the 'IServerManager' interface
*********************************************************************************************/
 
IServerManager IServerManager_New(void *pOwnerObject, CServerManagerItfImpl pOwnerItfImpl);
void IServerManager_Delete(IServerManager this);


#endif
#endif


