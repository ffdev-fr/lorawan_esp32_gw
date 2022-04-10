/*****************************************************************************************//**
 * @file     ESP32WifiConnector.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     22/04/2018
 *
 * @brief    The 'CESP32WifiConnector' object uses the native WIFI hardware/software of ESP32 
 *           device to implement the 'IServerConnector' interface.
 *
 * @details  The 'ESP32WifiConnector' is used by the gateway to access the Lora Network Server
 *           via WIFI.\n 
 *           The 'IServerConnector' interface exposes generic functions for connection and data
 *           transfer (assuming that distant server is connected to the network with TCP/IP
 *           protocol). The 'ESP32WifiConnector' implements these functions using the ESP32 WIFI
 *           hardware and software.\n
 *           Once instantiated the 'CESP32WifiConnector' object is publicly accessed using the
 *           'IServerConnector' interface.
 *
 * @note     The client object must call the 'ReleaseItf' method on 'IServerConnector' interface 
 *           to destroy the 'CESP32WifiConnector' object created by 'CreateInstance'.
 *
 * @note     The "LoRaWAN ESP32 Gateway V1.x" project is designed for execution on ESP32 
 *           Module (Dev.C kit).\n
 *           The implementation uses the Espressif IDF V3.0 framework (with RTOS)              
*********************************************************************************************/

#ifndef ESP32WIFICONNECTOR_H_
#define ESP32WIFICONNECTOR_H_

/*********************************************************************************************
  Includes
*********************************************************************************************/

#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_system.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "apps/sntp/sntp.h"

//#include <sys/socket.h>


#include "Utilities.h"


/********************************************************************************************* 
  Definitions for debug traces
  The debug level is specified with 'ESP32WIFICONNECTOR_DEBUG_LEVEL' in Definitions.h file
*********************************************************************************************/

#define ESP32WIFICONNECTOR_DEBUG_LEVEL0 ((ESP32WIFICONNECTOR_DEBUG_LEVEL & 0x01) > 0)
#define ESP32WIFICONNECTOR_DEBUG_LEVEL1 ((ESP32WIFICONNECTOR_DEBUG_LEVEL & 0x02) > 0)
#define ESP32WIFICONNECTOR_DEBUG_LEVEL2 ((ESP32WIFICONNECTOR_DEBUG_LEVEL & 0x04) > 0)


/********************************************************************************************* 
  Definitions (implementation)
*********************************************************************************************/

// Maximum size of meroy block used to receive a message from the Network Server.
// Notes:
//  - Typically, the message is a JSON stream (with LoRa packet data encoded in Base64)
//  - The format (and length) of JSON stream depends on LoRa Network Server protocol
//  - The maximum number of bytes for the LoRa packet to transmit is 'LORA_MAX_PAYLOAD_LENGTH'
#define ESP32WIFICONNECTOR_MAX_MESSAGELENGTH    ((LORA_MAX_PAYLOAD_LENGTH * 2) + 1024)


// Number of items in memory array for 'CServerConnectorItf_ServerDownlinkMessageOb' (downlink)
// Typically, messages should be processed by 'ProtocolEngine' quite quickly.
// This small buffer is only in case of the reception of another message from Network Server
// before previous message is decoded by 'ProtocolEngine'.
#define ESP32WIFICONNECTOR_MAX_SERVERMESSAGES     4


/********************************************************************************************* 
  CESP32WifiConnector_Message object

  Objects queued in 'WifiConnector' Task (main automaton) for external commands received via 
  'IServerConnector' interface and for internal comman sent by other tasks
*********************************************************************************************/

// The event message (i.e. posted to event queue of owner object)
typedef struct _CESP32WifiConnector_Message
{
  // Public
  WORD m_wMessageType;                            // The message
  DWORD m_dwMessageData;                          // Depends on message type
                                                  // Value or pointer to object
  DWORD m_dwMessageData2;                         // Depends on message type
} CESP32WifiConnector_MessageOb;

typedef struct _CESP32WifiConnector_Message * CESP32WifiConnector_Message;


/********************************************************************************************* 
 ESP32WifiConnector Class
*********************************************************************************************/

// Class data
typedef struct _CESP32WifiConnector
{
  // Interface
  IServerConnector m_pServerConnectorItf;

  uint32_t m_nRefCount;      // The 'CESP32WifiConnector' object is reference counted (number of client 
                             // objects owning one of the public interfaces exposed by 'CESP32WifiConnector')

  // Object main state
  DWORD m_dwCurrentState;

  // Object connection state
  DWORD m_dwConnectionState;
  SemaphoreHandle_t m_hConnectionStateMutex;

  //
  // Tasks and associated queues
  //

  // 'WifiConnector' task (main automaton)
  // Used for internal messages and external commands via 'IServerConnector' interface
  TaskFunction_t m_hWifiConnectorTask;
  QueueHandle_t m_hWifiConnectorQueue;

  // For command processing by 'WifiConnector' task
  SemaphoreHandle_t m_hCommandMutex;
  SemaphoreHandle_t m_hCommandDone;
  DWORD m_dwCommand;
  void *m_pCommandParams;


  // Task used to receive messages from Network Server (downlink)
  TaskFunction_t m_hReceiveTask;

  // Storage of received downlink messages
  CMemoryBlockArray m_pServerMessageArray;
  
  // Interface to 'ServerManager' object using the 'ServerConnector' object:
  //  - The 'Connector' notify the 'ServerManager' using 'IServerConnectorItf' interface
  //  - The 'IServerConnectorItf' interface uses direct event notification to specific task on 
  //    'ServerManager' via a dedicated queue
  // 
  // The notifications are used for the following purposes:
  //  - To notify the result of uplink operations asked by the ServerManager (i.e. 'IServerManager_ServerMessageEvent')
  //  - To transmit a downlink messages to 'ServerManager' ('CServerConnectorItf_ServerDownlinkMessageOb' objects)
  QueueHandle_t m_hServerManagerNotifyQueue;


  // The 'ServerConnector' uses this interface to notify the result of uplink operations asked by the 
  // ServerManager (i.e. 'IServerManager_ServerMessageEvent' method)
//  IServerManager m_pServerManagerItf;


  // Access to Wifi network
  char m_szWifiSSID[64];
  char m_szWifiPassword[32];
  DWORD m_dwWifiJoinTimeoutMillisec;

  // EventGroup for events received on Wifi event handler
  EventGroupHandle_t m_hWifiEventGroup;

  // Access to NetworkServer
  char m_szNetworkServerUrl[48];
  DWORD m_dwNetworkServerPort;
  DWORD m_dwNetworkServerTimeoutMillisec;

  int m_hServerSocket;
  struct sockaddr_in m_ServerSockAddr;
  char m_szNetworkServerIP[16];

} CESP32WifiConnector;

// Class constants and definitions


// Methods for 'IServerConnector' interface implementation on 'ESP32WifiConnector' object
// The 'CServerConnectorItfImpl' structure provided by 'ESP32WifiConnector' object contains pointers to these methods 
uint32_t CESP32WifiConnector_AddRef(void *this);
uint32_t CESP32WifiConnector_ReleaseItf(void *this);

bool CESP32WifiConnector_Initialize(void *this, void *pParams);
bool CESP32WifiConnector_Start(void *this, void *pParams);
bool CESP32WifiConnector_Stop(void *this, void *pParams);
bool CESP32WifiConnector_Send(void *this, void *pParams);
bool CESP32WifiConnector_SendReceive(void *this, void *pParams);
bool CESP32WifiConnector_DownlinkReceived(void *this, void *pParams);


// Task functions
// Note: The CESP32WifiConnector object has 2 automatons
void CESP32WifiConnector_WifiConnectorAutomaton(CESP32WifiConnector *this);
void CESP32WifiConnector_ReceiveAutomaton(CESP32WifiConnector *this);


// Main Automaton (ESP32WifiConnector task)
// Note: Numbering order MUST match object's life cycle
#define ESP32WIFICONNECTOR_AUTOMATON_STATE_CREATING      0
#define ESP32WIFICONNECTOR_AUTOMATON_STATE_CREATED       1
#define ESP32WIFICONNECTOR_AUTOMATON_STATE_INITIALIZED   2
#define ESP32WIFICONNECTOR_AUTOMATON_STATE_IDLE          3
#define ESP32WIFICONNECTOR_AUTOMATON_STATE_RUNNING       4
#define ESP32WIFICONNECTOR_AUTOMATON_STATE_STOPPING      5
#define ESP32WIFICONNECTOR_AUTOMATON_STATE_TERMINATED    6
#define ESP32WIFICONNECTOR_AUTOMATON_STATE_ERROR         7


#define ESP32WIFICONNECTOR_AUTOMATON_MSG_NONE            0x00000000
#define ESP32WIFICONNECTOR_AUTOMATON_MSG_COMMAND         0x00000001
//#define LORASERVERMANAGER_AUTOMATON_MSG_NOTIFY           0x00000002

#define ESP32WIFICONNECTOR_AUTOMATON_MAX_CMD_DURATION       2000
#define ESP32WIFICONNECTOR_AUTOMATON_MAX_SYNC_CMD_DURATION  40000

#define ESP32WIFICONNECTOR_AUTOMATON_CMD_NONE              0x00000000
#define ESP32WIFICONNECTOR_AUTOMATON_CMD_INITIALIZE        0x00000001
#define ESP32WIFICONNECTOR_AUTOMATON_CMD_ATTACH            0x00000002
#define ESP32WIFICONNECTOR_AUTOMATON_CMD_START             0x00000003
#define ESP32WIFICONNECTOR_AUTOMATON_CMD_STOP              0x00000004
#define ESP32WIFICONNECTOR_AUTOMATON_CMD_SEND              0x00000005
#define ESP32WIFICONNECTOR_AUTOMATON_CMD_SENDRECEIVE       0x00000006
#define ESP32WIFICONNECTOR_AUTOMATON_CMD_DOWNLINKRECEIVED  0x00000007


// Connection state
// Note: Compound state for access to Wifi network and Lora Network Server.
//       Typically adjusted by asynchronous events (Wifi event handler) 
#define ESP32WIFICONNECTOR_CONNECTION_STATE_DISCONNECTED         0
#define ESP32WIFICONNECTOR_CONNECTION_STATE_CONNECTING_WIFI      1
#define ESP32WIFICONNECTOR_CONNECTION_STATE_WIFI_CONNECTED       2
#define ESP32WIFICONNECTOR_CONNECTION_STATE_SERVER_DISCONNECTED  3
#define ESP32WIFICONNECTOR_CONNECTION_STATE_CONNECTING_SERVER    4
#define ESP32WIFICONNECTOR_CONNECTION_STATE_SERVER_CONNECTED     5

// Connection state change events
// These events are used with 'CESP32WifiConnector_UpdateConnectionState' method
#define ESP32WIFICONNECTOR_CONNECTION_EVENT_WIFI_DISCONNECTED    0x00000001
#define ESP32WIFICONNECTOR_CONNECTION_EVENT_WIFI_CONNECTED       0x00000002
#define ESP32WIFICONNECTOR_CONNECTION_EVENT_SOCKET_OPENED        0x00000010
#define ESP32WIFICONNECTOR_CONNECTION_EVENT_SERVER_DISCONNECTED  0x00000100
#define ESP32WIFICONNECTOR_CONNECTION_EVENT_SERVER_CONNECTED     0x00000200


bool CESP32WifiConnector_NotifyAndProcessCommand(CESP32WifiConnector *this, DWORD dwCommand, DWORD dwTimeout, void *pCmdParams);
bool CESP32WifiConnector_ProcessAutomatonNotifyCommand(CESP32WifiConnector *this);

bool CESP32WifiConnector_ProcessInitialize(CESP32WifiConnector *this, CESP32WifiConnectorItf_InitializeParams pParams);
//bool CESP32WifiConnector_ProcessAttach(CESP32WifiConnector *this, CServerConnectorItf_AttachParams pParams);
bool CESP32WifiConnector_ProcessStart(CESP32WifiConnector *this, CESP32WifiConnectorItf_StartParams pParams);
bool CESP32WifiConnector_ProcessStop(CESP32WifiConnector *this, CESP32WifiConnectorItf_StopParams pParams);
bool CESP32WifiConnector_ProcessSend(CESP32WifiConnector *this, CESP32WifiConnectorItf_SendParams pParams);
bool CESP32WifiConnector_ProcessSendReceive(CESP32WifiConnector *this, CESP32WifiConnectorItf_SendReceiveParams pParams);
bool CESP32WifiConnector_ProcessDownlinkReceived(CESP32WifiConnector *this, CESP32WifiConnectorItf_DownlinkReceivedParams pParams);

bool CESP32WifiConnector_UpdateConnectionState(CESP32WifiConnector *this, DWORD dwConnectionEvent);

// Construction
CESP32WifiConnector * CESP32WifiConnector_New();
void CESP32WifiConnector_Delete(CESP32WifiConnector *this);

// Wifi event handler
static esp_err_t CESP32WifiConnector_WifiEventHandler(void *pCtx, system_event_t *pEvent);

// Low level Wifi
#define WIFI_EVENT_GROUP_CONNECTED_BIT     BIT0
#define WIFI_EVENT_GROUP_DISCONNECTED_BIT  BIT1

bool CESP32WifiConnector_JoinWifi(CESP32WifiConnector *this, bool bReconnect);

// Low level UDP transport
bool CESP32WifiConnector_BindNetworkServer(CESP32WifiConnector *this);

// Misc
bool CESP32WifiConnector_ConnectSNTPServer(CESP32WifiConnector *this, char *pSNTPServerUrl, DWORD dwSNTPServerPeriodSec);


#endif

