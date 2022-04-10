/*****************************************************************************************//**
 * @file     LoraServerManager.c
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


/*********************************************************************************************
  Espressif framework includes
*********************************************************************************************/

#include <Common.h>


/*********************************************************************************************
  Includes for object implementation
*********************************************************************************************/

// The CLoraServerManager object implements the 'IServerManager' interface
#define SERVERMANAGERITF_IMPL

#include "ServerManagerItf.h"
#include "ESP32WifiConnectorItf.h"
#include "ServerConnectorItf.h"
#include "NetworkServerProtocolItf.h"
#include "SemtechProtocolEngineItf.h"
#include "TransceiverManagerItf.h"

// Object's definitions and methods
#include "LoraServerManager.h"
         
    
/*********************************************************************************************
  Instantiate global static objects used by module implementation
*********************************************************************************************/

// 'IServerManager' interface function pointers
CServerManagerItfImplOb g_ServerManagerItfImplOb = { .m_pAddRef = CLoraServerManager_AddRef,
                                                     .m_pReleaseItf = CLoraServerManager_ReleaseItf,
                                                     .m_pInitialize = CLoraServerManager_Initialize,
                                                     .m_pAttach = CLoraServerManager_Attach,
                                                     .m_pStart = CLoraServerManager_Start,
                                                     .m_pStop = CLoraServerManager_Stop,
                                                     .m_pServerMessageEvent = CLoraServerManager_ServerMessageEvent
                                                   };

// The CLoraServerManager object implements the global configuration object
#define SERVERMANAGERCONFIG_IMPL
#include "Configuration.h"

/*  To delete -> now in Configuration.h

// Gateway configuration and server connection settings
//
// Note: 
//  - In this early version, the gateway hardware configuration and connection settings are
//    statically defined.
//  - In the final version, a similar object will be created in the main program using a 
//    configuration file and provided to 'CLoraServerManager' object
//  - The operation mode for the gateway is:
//     .. Semtech protocol
//     .. No dynamic failover (first successful connector, in configuration order, is used until the next reboot)

#define CONFIG_WIFI_NETWORK_NETGEAR    1
//#define CONFIG_WIFI_NETWORK_ANDROID    1

#define CONFIG_NETWORK_SERVER_TTN      1
//#define CONFIG_NETWORK_SERVER_LORIOT   1

CServerManagerItf_InitializeParamsOb g_LoraServerManagerSettings = 
  { 
    .m_bUseBuiltinSettings = true,

    .LoraServerSettings = 
    {
      .ConnectorSettings =
      {
        [0] =
        {
          #if (CONFIG_WIFI_NETWORK_NETGEAR)
            // Netgear Paris
            .m_szNetworkName = "NETGEAR_11ng\0",           // SSID for Wifi Network
            .m_szNetworkUser = "\0",                       // Not used for Wifi Network
            .m_szNetworkPassword = "spaddeperdussin\0",    // Password for Network (Wifi or GPRS)
          #endif

          #if (CONFIG_WIFI_NETWORK_ANDROID)
            // Android Samsung
            .m_szNetworkName = "AndroidAP\0",         // SSID for Wifi Network
            .m_szNetworkUser = "\0",                  // Not used for Wifi Network
            .m_szNetworkPassword = "gubd3761\0",      // Password for Network (Wifi or GPRS)
          #endif
          .m_dwNetworkJoinTimeout = 60000,
          .m_dwNetworkServerTimeout = 5000
        },
      },
      .m_wNetworkServerProtocol = SERVERMANAGER_PROTOCOL_SEMTECH, 

      #if (CONFIG_NETWORK_SERVER_TTN)
        // TTN
        .m_szNetworkServerUrl = "router.eu.thethings.network\0",
        .m_dwNetworkServerPort = 1700,
        //.m_szGatewayIDToken = "FFFE\0",               // WIFI_MAC[:6] + "FFFE" + WIFI_MAC[6:12]
        .m_szGatewayIDToken = "240AC4FFFE0272B4\0",     // MAC Addr du LoPy NanoGateway (Registered with NS) 
      #endif

      #if (CONFIG_NETWORK_SERVER_LORIOT)
        // Loriot
        .m_szNetworkServerUrl = "eu1.loriot.io\0",
        .m_dwNetworkServerPort = 1780,
        //.m_szGatewayIDToken = "FFFF\0",               // WIFI_MAC[:6] + "FFFF" + WIFI_MAC[6:12]
        .m_szGatewayIDToken = "240AC4FFFF0272B4\0",     // MAC Addr du LoPy NanoGateway (Registered with NS)
      #endif

      .m_szNetworkServerUser = "\0",
      .m_szNetworkServerPassword = "\0",

      .m_dwSNTPServerPeriodSec = 3600,                  // The 0 value indicates SNTP time update not required
      .m_szSNTPServerUrl = "pool.ntp.org\0",

      .m_GatewayMACAddr = {0x24,0x0A,0xC4,0x02,0x72,0xB4}   // The WIFI_MAC (explicitly set on ESP32 during initialization)
    }
  };

*/

/********************************************************************************************* 

 CLoraServerManager Class

*********************************************************************************************/



/********************************************************************************************* 
  Public methods of CLoraServerManager object
 
  These methods are exposed on object's public interfaces
*********************************************************************************************/

/*********************************************************************************************
  Object instance factory
 
  The factory contains one method used to create a new object instance.
  This method provides the 'IServerManager' interface object for object's use and destruction.
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         IServerManager CLoraServerManager_CreateInstance(BYTE usWifiConnectorNumber,
 *                              BYTE usGPRSConnectorNumber, BYTE usNetworkServerProtocol)
 * 
 * @brief      Creates a new instance of CLoraServerManager object.
 * 
 * @details    A new instance of CLoraServerManager object is created and its 'IServerManager'
 *             interface is returned. The owner object uses this interface to configure and 
 *             control associated 'ServerConnectors'.
 *
 * @param      usWifiConnectorNumber
 *             The number of 'ServerConnector' for Wifi present in the gateway (i.e. hardware
 *              configuration).
 * 
 * @param      usGPRSConnectorNumber
 *             The number of 'ServerConnector' for GPRS present in the gateway (i.e. hardware
 *              configuration).
 *
 * @param      usNetworkServerProtocol
 *             The protocol to use with the associated 'Lora Network Server' (i.e. to encode
 *             and decode messages).
 *
 * @return     A 'IServerManager' interface object.\n
 *             The reference count for returned 'IServerManager' interface is set to 1.
 *
 * @note       The CLoraServerManager object and associated 'ServerConnector' objects are
 *             created but nothing is initialized (i.e. only object allocations).
 *             The 'IServerManager_Initialize' method must be called to configure the 
 *             'LoraServerManager' and associated 'ServerConnectors'.
 *
 * @note       The CLoraServerManager object is destroyed when the last reference to 
 *             'IServerManager' is released (i.e. call to 'IServerManager_ReleaseItf' 
 *             method).
 * 
 * @note       The 'ServerConnector' for Wifi is assumed to be implemented by a 
 *            'CESP32WifiConnector'
 *             object.\n
 *             The 'ServerConnector' for GPRS is assumed to be implemented by a 
 *             'CFONA808Connector' object.
*********************************************************************************************/
IServerManager CLoraServerManager_CreateInstance(BYTE usWifiConnectorNumber,
                                                 BYTE usGPRSConnectorNumber,
                                                 BYTE usNetworkServerProtocol)
{
  CLoraServerManager * pLoraServerManager;
  IServerConnector pServerConnectorItf;
     
  // Create the object
  if ((pLoraServerManager = CLoraServerManager_New()) != NULL)
  {
    // Create 'ServerConnector' ojects for Wifi connections (implemented by 'CESP32WifiConnector' object)
    for (BYTE i = 0; i < usWifiConnectorNumber; i++)
    {
      if ((pServerConnectorItf = CESP32WifiConnector_CreateInstance()) == NULL)
      {
        CLoraServerManager_Delete(pLoraServerManager);
        return NULL;
      }
      pLoraServerManager->m_ConnectorDescrArray[i].m_pServerConnectorItf = pServerConnectorItf;
      pLoraServerManager->m_ConnectorDescrArray[i].m_bActive = false;
      ++(pLoraServerManager->m_usConnectorNumber);
    }

    // Create 'ServerConnector' ojects for GPRS connections (implemented by 'CFONA808Connector' object)

    // TO DO

//  for (BYTE i = 0; i < usGPRSConnectorNumber; i++)
//  {
//    if ((pServerConnectorItf = CFONA808Connector_CreateInstance()) == NULL)
//    {
//      CLoraServerManager_Delete(pLoraServerManager);
//      return NULL;
//    }
//    pLoraServerManager->m_ConnectorDescrArray[usWifiConnectorNumber + i].m_pServerConnectorItf = pServerConnectorItf;
//    pLoraServerManager->m_ConnectorDescrArray[usWifiConnectorNumber + i].m_bActive = false;
//    ++(pLoraServerManager->m_usConnectorNumber);
//  }


    // Create the 'NetworkServerProtocol' engine
    if (usNetworkServerProtocol == SERVERMANAGER_PROTOCOL_SEMTECH)
    {
      if ((pLoraServerManager->m_pNetworkServerProtocolItf = CSemtechProtocolEngine_CreateInstance()) == NULL)
      {
        CLoraServerManager_Delete(pLoraServerManager);
        return NULL;
      }
    }
    else
    {
      DEBUG_PRINT_LN("[ERROR] CLoraServerManager_CreateInstance, only Semtech protocol engine supported in this version");
      CLoraServerManager_Delete(pLoraServerManager);
      return NULL;
    }

    // Create the 'IServerManager' interface object
    if ((pLoraServerManager->m_pServerManagerItf = 
        IServerManager_New(pLoraServerManager, &g_ServerManagerItfImplOb)) != NULL)
    {
      ++(pLoraServerManager->m_nRefCount);
    }

    return pLoraServerManager->m_pServerManagerItf;
  }

  return NULL;
}

/*********************************************************************************************
  Public methods exposed on 'IServerManager' interface
 
  The static 'CLoraServerManagerItfImplOb' object is initialized with pointers to these functions.
  The static 'CLoraServerManagerItfImplOb' object is referenced in the 'IServerManager'
  interface provided by 'CreateInstance' method (object factory).
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         uint32_t CLoraServerManager_AddRef(void *this)
 * 
 * @brief      Increments the object's reference count.
 * 
 * @details    This function increments object's global reference count.\n
 *             The reference count is used to track the number of existing external references
 *             to 'IServerManager' interface implemented by CLoraServerManager object.
 * 
 * @param      this
 *             The pointer to CLoraServerManager object.
 *  
 * @return     The value of reference count once incremented.
*********************************************************************************************/
uint32_t CLoraServerManager_AddRef(void *this)
{
  return ++((CLoraServerManager *)this)->m_nRefCount;
}

/*****************************************************************************************//**
 * @fn         uint32_t CLoraServerManager_ReleaseItf(void *this)
 * 
 * @brief      Decrements the object's reference count.
 * 
 * @details    This function decrements object's global reference count and destroy the object
 *             when count reaches 0.\n
 *             The reference count is used to track the number of existing external references
 *             to 'IServerManager' interface implemented by CLoraServerManager object.
 * 
 * @param      this
 *             The pointer to CLoraServerManager object.
 *  
 * @return     The value of reference count once decremented.
*********************************************************************************************/
uint32_t CLoraServerManager_ReleaseItf(void *this)
{
  // Delete the object if its interface reference count reaches zero
  if (((CLoraServerManager *)this)->m_nRefCount == 1)
  {
    // TO DO -> Stop the object master task which will delete the object on exit
    CLoraServerManager_Delete((CLoraServerManager *)this);
    return 0;
  }
  return --((CLoraServerManager *)this)->m_nRefCount;
}

/*****************************************************************************************//**
 * @fn         bool CLoraServerManager_Initialize(void *this, void *pParams)
 * 
 * @brief      Initializes the 'LoraServerManager' and associated 'ServerConnector' objects.
 * 
 * @details    This function prepares the collection of 'ServerConnector' for network access.\n
 *             The default connection parameters are set and the 'ServerConnectors' are waiting
 *             ready in 'StandBy' mode.
 * 
 * @param      this
 *             The pointer to CLoraServerManager object.
 *  
 * @param      pParams
 *             The method parameters (see 'ServerManagerItf.h' for details).
 *
 * @return     The returned value is 'true' if 'LoraServerManager' has initialized all its
 *             associated 'ServerConnectors' or 'false' in case of error.
*********************************************************************************************/
bool CLoraServerManager_Initialize(void *this, void *pParams)
{
  return CLoraServerManager_NotifyAndProcessCommand((CLoraServerManager *) this, 
                                                    LORASERVERMANAGER_AUTOMATON_CMD_INITIALIZE, 
                                                    LORASERVERMANAGER_AUTOMATON_MAX_SYNC_CMD_DURATION,
                                                    pParams);
}

bool CLoraServerManager_Attach(void *this, void *pParams)
{
  return CLoraServerManager_NotifyAndProcessCommand((CLoraServerManager *) this, 
                                                    LORASERVERMANAGER_AUTOMATON_CMD_ATTACH, 0, pParams);
}

bool CLoraServerManager_Start(void *this, void *pParams)
{
  return CLoraServerManager_NotifyAndProcessCommand((CLoraServerManager *) this, 
                                                    LORASERVERMANAGER_AUTOMATON_CMD_START, 0, pParams);
}

bool CLoraServerManager_Stop(void *this, void *pParams)
{
  return CLoraServerManager_NotifyAndProcessCommand((CLoraServerManager *) this, 
                                                    LORASERVERMANAGER_AUTOMATON_CMD_STOP, 0, pParams);
}

bool CLoraServerManager_ServerMessageEvent(void *this, void *pEvent)
{
  CLoraServerManager_MessageOb QueueMessage;

  #if (LORASERVERMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT_LN("[DEBUG] CLoraServerManager_ServerMessageEvent, Entering function");
  #endif

  // Insert the notification in queue used by 'SessionManager' task
  // Note: The 'CLoraServerManager_MessageOb' object inserted in queue is a 
  //       'CServerManagerItf_ServerMessageEventOb' object
  QueueMessage.m_wMessageType = ((CServerManagerItf_ServerMessageEvent) pEvent)->m_wEventType;
  QueueMessage.m_dwMessageData = (DWORD)((CServerManagerItf_ServerMessageEvent) pEvent)->m_pMessage;
  QueueMessage.m_dwMessageData2 = ((CServerManagerItf_ServerMessageEvent) pEvent)->m_dwParam;

  #if (LORASERVERMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CLoraServerManager_ServerMessageEvent, Writing message in queue, Msg Type: ");
    DEBUG_PRINT_HEX(((CServerManagerItf_ServerMessageEvent) pEvent)->m_wEventType);
    DEBUG_PRINT(", ticks: ");
    DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
//    DEBUG_PRINT(", Msg Addr: ");
//    DEBUG_PRINT_HEX((DWORD)((CServerManagerItf_ServerMessageEvent) pEvent)->m_pMessage);
//    DEBUG_PRINT(", Msg Param: ");
//    DEBUG_PRINT_HEX(((CServerManagerItf_ServerMessageEvent) pEvent)->m_dwParam);
    DEBUG_PRINT_CR;
  #endif

  if (xQueueSend(((CLoraServerManager *)this)->m_hServerManagerQueue, &QueueMessage, 
      pdMS_TO_TICKS(LORASERVERMANAGER_AUTOMATON_MAX_CMD_DURATION / 2)) != pdPASS)
  {
    // Message queue is full
    // Should never occur
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraServerManager_Notify - Message queue full");
    #endif
    return false;
  }
  return true;
}

/********************************************************************************************* 
  Private methods of CLoraServerManager object
 
  The following methods CANNOT be called by another object
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         bool CLoraServerManager_NotifyAndProcessCommand(CLoraServerManager *this, 
 *                                                           DWORD dwCommand, void *pCmdParams)
 * 
 * @brief      Process a command issued by a method of 'ILoraServerManager' interface.
 * 
 * @details    The CLoraServerManager main automaton asynchronously executes the commands generated
 *             by calls on 'ILoraServerManager' interface methods.\n
 *             This method transmits commands to main automaton and wait for end of execution
 *             (i.e. from client point of view, the interface method is synchronous).
 * 
 * @param      this
 *             The pointer to CLoraServerManager object.
 *  
 * @param      dwCommand
 *             The command to execute (typically same name than corresponding method on
 *             'ILoraServerManager' interface. See 'LORASERVERMANAGER_AUTOMATON_CMD_xxx' in 
 *             LoraServerManager.h.
 *  
 * @param      dwTimeout
 *             Specific timeout for the command. If the value is 0, the standard timeout is
 *             used.
 *
 * @param      pCmdParams
 *             A pointer to command parameters. The object pointed by 'pCmdParams' depends
 *             on method invoked on 'IServerManager' interface. 
 *             See 'LoraServerManagerItf.h'.
 *  
 * @return     The returned value is 'true' if the command is properly executed or 'false'
 *             if command execution has failed or is still pending.
 *
 * @note       This function assumes that commands are quickly processed by main automaton.\n
 *             The maximum execution time can be configured and a mechanism is implemented in
 *             order to ignore client commands sent when a previous command is still pending.
*********************************************************************************************/
bool CLoraServerManager_NotifyAndProcessCommand(CLoraServerManager *this, DWORD dwCommand, DWORD dwTimeout, void *pCmdParams)
{
  // Automaton commands are serialized (and should be quickly processed)
  // Note: In current design, there is only one client object for a single CLoraServerManager
  //       instance (the main task on program startup and for operation control). In other 
  //       words, commands are serialized here and there is no concurrency on the 
  //       'CLoraServerManager_NotifyAndProcessCommand' method.
  if (xSemaphoreTake(this->m_hCommandMutex, pdMS_TO_TICKS(LORASERVERMANAGER_AUTOMATON_MAX_CMD_DURATION)) == pdFAIL)
  {
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraServerManager_NotifyAndProcessCommand - Failed to take mutex");
    #endif

    return false;
  }

  // Make sure that previous command has been processed by the automaton
  if (this->m_dwCommand != LORASERVERMANAGER_AUTOMATON_CMD_NONE)
  {
    // Previous call to this function has returned before end of command's execution
    // Check if done now
    if (xSemaphoreTake(this->m_hCommandDone, 0) == pdFAIL)
    {
      // Still not terminated
      #if (LORASERVERMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] CLoraServerManager_NotifyAndProcessCommand - Previous command still pending");
      #endif

      xSemaphoreGive(this->m_hCommandMutex);
      return false;
    }
  }

  // Post the command to main automaton
  this->m_dwCommand = dwCommand;
  this->m_pCommandParams = pCmdParams;
  CLoraServerManager_MessageOb QueueMessage;
  QueueMessage.m_wMessageType = LORASERVERMANAGER_AUTOMATON_MSG_COMMAND;

  DEBUG_PRINT_LN("[DEBUG] CLoraServerManager_NotifyAndProcessCommand - Sending command (via LoraServerManager' queue)");

  if (xQueueSend(this->m_hServerManagerQueue, &QueueMessage, pdMS_TO_TICKS(LORASERVERMANAGER_AUTOMATON_MAX_CMD_DURATION / 2)) != pdPASS)
  {
    // Message queue is full
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraServerManager_NotifyAndProcessCommand - Message queue full");
    #endif

    xSemaphoreGive(this->m_hCommandMutex);
    return false;
  }

  // Wait for command execution by main automaton
  if (dwTimeout == 0)
  {
    dwTimeout = LORASERVERMANAGER_AUTOMATON_MAX_CMD_DURATION - (LORASERVERMANAGER_AUTOMATON_MAX_CMD_DURATION / 5); 
  }
  else
  {
    dwTimeout -= LORASERVERMANAGER_AUTOMATON_MAX_CMD_DURATION / 5;
  }
  BaseType_t nCommandDone = xSemaphoreTake(this->m_hCommandDone, pdMS_TO_TICKS(dwTimeout));

  // If the command has been processed, clear 'm_dwCommand' attribute
  if (nCommandDone == pdPASS)
  {
    this->m_dwCommand = LORASERVERMANAGER_AUTOMATON_CMD_NONE;
  }
  else
  {
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraServerManager_NotifyAndProcessCommand - Exiting before end of command execution");
    #endif
  }

  // Next command allowed
  xSemaphoreGive(this->m_hCommandMutex);

  return nCommandDone == pdPASS ? true : false;
}


/********************************************************************************************* 
  RTOS task functions
*********************************************************************************************/


/********************************************************************************************* 
  'ServerManager' task
 
  This RTOS 'Task' is the main automaton of 'CLoraServerManager' object:
   - It processes messages sent by other tasks (typically when uplink or downlink LoRa packets
     are received or sent). In other words, it implements the routing rules between gateway and
     the LoRaWAN Network Sever.
   - It processes messages sent via 'IServerManager' interface for comfiguration and 
     operation of associated server connectors.
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         void CLoraServerManager_ServerManagerAutomaton(CLoraServerManager *this)
 * 
 * @brief      Executes the 'ServerManager' automaton.
 * 
 * @details    This function is the RTOS task implementing the CLoraServerManager main automaton.\n
 *             This automation processes the following messages:\n
 *               - Messages sent by other tasks (typically when uplink or downlink data are 
 *                 received or sent). 
 *               - Messages sent via 'IServerManager' interface for configuration and 
 *                 operation of associated server connectors.
 *             For uplink path, the automaton manages a collection of 'CLoraServerUpMessage' objects:
 *               - A 'CLoraServerUpMessage' is created using the 'CLoraPacketSession' object
 *                 sent by the 'CLoraNodeNamager' (i.e. when a LoRaWAN session is initiated
 *                 by the endpoint device).
 *               - The encoding of 'CLoraServerUpMessage' is realized by a Server Protocol 
 *                 Engine object (i.e. object implementing the 'INetworkServerProtocol'
 *                 interface). 
 *               - The 'CLoraServerMessage' object is destroyed only by the 'ServerManager'
 *                 automaton.
 *               - An internal 'CLoraServerUpMessage' object is maintained in the 
 *                 'CLoraServerManager' and is used to manage heartbeat messages periodicaly
 *                 sent to the Network Server. This object is reused and never destroyed.

  TO CHECK
 *             For downlink path, the 'CloraServerMessage' is received from the Network
 *                 Server. This message is used to build the 'CLoraPacketSession' object to
 *                 transmit to the 'CLoraNodeManager'.
 * @param      this
 *             The pointer to CLoraServerManager object.
 *  
 * @return     The RTOS task terminates when object is deleted (typically on main program
 *             exit).
 *
 * @note       The automaton's main loop waits for messages received through an RTOS queue.
*********************************************************************************************/
void CLoraServerManager_ServerManagerAutomaton(CLoraServerManager *this)
{
  CLoraServerManager_MessageOb QueueMessage;
  CLoraServerUpMessage pLoraServerMessage;
  CNetworkServerProtocol_BuildUplinkMessageParamsOb ProtocolEncodeParams;

  // Initialize the parameter object for 'heartbeat' processing (i.e. same object used uring
  // whole life of task)
  this->m_HeartbeatMessageOb.m_usMessageId = 0xFF;
  this->m_HeartbeatMessageOb.m_dwProtocolMessageId = 0xFFFFFFFF;
  this->m_HeartbeatMessageOb.m_pLoraPacket = NULL;
  this->m_HeartbeatMessageOb.m_pLoraPacketInfo = NULL;
  this->m_HeartbeatMessageOb.m_pSession = NULL;

  ProtocolEncodeParams.m_wServerManagerMessageId = 0xFF;
  ProtocolEncodeParams.m_pMessageData = this->m_HeartbeatMessageOb.m_usData;
  ProtocolEncodeParams.m_wMessageType = NETWORKSERVERPROTOCOL_UPLINKMSG_HEARTBEAT;
  ProtocolEncodeParams.m_bForceHeartbeat = false;
  ProtocolEncodeParams.m_pLoraPacket = NULL;
  ProtocolEncodeParams.m_pLoraPacketInfo = NULL;
  ProtocolEncodeParams.m_wMaxMessageLength = LORASERVERMANAGER_MAX_UPMESSAGE_LENGTH;
   
  // Task loop
  while (this->m_dwCurrentState != LORASERVERMANAGER_AUTOMATON_STATE_TERMINATED)
  {
    if (this->m_dwCurrentState >= LORASERVERMANAGER_AUTOMATON_STATE_CREATED)
    {
      #if (LORASERVERMANAGER_DEBUG_LEVEL2)
        DEBUG_PRINT_LN("[DEBUG] CLoraServerManager_ServerManagerAutomaton, waiting message");
      #endif
  
      // Wait for messages
      if (xQueueReceive(this->m_hServerManagerQueue, &QueueMessage, pdMS_TO_TICKS(500)) == pdPASS)
      {
        // Process message
        #if (LORASERVERMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_CR;
          DEBUG_PRINT("[INFO] CLoraServerManager_ServerManagerAutomaton, message received (processing): ");
          DEBUG_PRINT_HEX(QueueMessage.m_wMessageType);
          DEBUG_PRINT_CR;
        #endif
  
        if (QueueMessage.m_wMessageType == LORASERVERMANAGER_AUTOMATON_MSG_COMMAND)
        {
          // Command message (i.e. external message via 'IServerManager' interface
          // Note: The command is defined by 'm_dwCommand' and 'm_pCommandParams' variables
          //       (i.e. commands are serialized, never more than one command waiting in
          //       automaton's queue) 
          CLoraServerManager_ProcessAutomatonNotifyCommand(this);
        }
        else if (QueueMessage.m_wMessageType >= SERVERMANAGER_MESSAGEEVENT_BASE)
        {
          // The message is a 'MessageEvent' sent via 'IServerManager' interface 
          // (i.e. notification for an event occurred on a 'LoraServerMessage' object)
          // 
          // Note: The 'CLoraServerManager_MessageOb' object retrieved in queue contains a pointer to a 
          //       'CServerManagerItf_ServerMessageEventOb' object (this object is allocated in CMemoryBlockArray)

          pLoraServerMessage = (CLoraServerUpMessage) QueueMessage.m_dwMessageData;

          #if (LORASERVERMANAGER_DEBUG_LEVEL2)
            DEBUG_PRINT("[DEBUG] CLoraServerManager_ServerManagerAutomaton, Event message received, Type: ");
            DEBUG_PRINT_HEX(QueueMessage.m_wMessageType);
            DEBUG_PRINT(", ticks: ");
            DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
            DEBUG_PRINT_CR;
          #endif

          switch (QueueMessage.m_wMessageType)
          {
            case SERVERMANAGER_MESSAGEEVENT_UPLINK_RECEIVED:
              CLoraServerManager_ProcessServerMessageEventUplinkReceived(this, pLoraServerMessage);
              break;

            case SERVERMANAGER_MESSAGEEVENT_UPLINK_PREPARED:
              CLoraServerManager_ProcessServerMessageEventUplinkPrepared(this, pLoraServerMessage);
              break;

            case SERVERMANAGER_MESSAGEEVENT_UPLINK_SENT:
              CLoraServerManager_ProcessServerMessageEventUplinkSent(this, pLoraServerMessage);
              break;

            case SERVERMANAGER_MESSAGEEVENT_UPLINK_SEND_FAILED:
              CLoraServerManager_ProcessServerMessageEventUplinkSendFailed(this, pLoraServerMessage);
              break;

            case SERVERMANAGER_MESSAGEEVENT_UPLINK_TERMINATED:
              CLoraServerManager_ProcessServerMessageEventUplinkTerminated(this, pLoraServerMessage, QueueMessage.m_dwMessageData2);
              break;
          }
        }
      }
      else
      {
        // No pending message for a while, maybe something to do in background
        
        // In 'RUNNING' state, ask the 'NetworkServerProtocol' for uplink message to send (typically 'heartbeat' or
        // 'pull' request)
        if (this->m_dwCurrentState == LORASERVERMANAGER_AUTOMATON_STATE_RUNNING)
        {
          ProtocolEncodeParams.m_wMessageLength = 0;
          ProtocolEncodeParams.m_wServerManagerMessageId = 0xFF;
          ProtocolEncodeParams.m_dwProtocolMessageId = 0xFFFFFFFF;
          if (INetworkServerProtocol_BuildUplinkMessage(this->m_pNetworkServerProtocolItf, &ProtocolEncodeParams) == true)
          {
            // Uplink 'heartbeat' message to sent provided 
            #if (LORASERVERMANAGER_DEBUG_LEVEL0)
              DEBUG_PRINT_LN("[INFO] 'CLoraServerManager_ServerManagerAutomaton', heartbeat provided by ProtocolEngine");
            #endif

            // Asynchronous processing of uplink 'heartbeat' message
            this->m_HeartbeatMessageOb.m_dwProtocolMessageId = ProtocolEncodeParams.m_dwProtocolMessageId;
            this->m_HeartbeatMessageOb.m_wDataLength = ProtocolEncodeParams.m_wMessageLength;
            CLoraServerManager_ProcessServerMessageEventUplinkPrepared(this, &this->m_HeartbeatMessageOb);
          }
          else
          {
            // No 'heartbeat' required
            #if (LORASERVERMANAGER_DEBUG_LEVEL2)
              DEBUG_PRINT_LN("[DEBUG] 'CLoraServerManager_ServerManagerAutomaton', No heartbeat required");
            #endif
          }
        }
        else
        {
          #if (LORASERVERMANAGER_DEBUG_LEVEL2)
            DEBUG_PRINT_LN("[DEBUG] CLoraServerManager_ServerManagerAutomaton, idle - TO DO - maybe something in background");
          #endif
        }
      }
    }
    else
    {
      // Parent object not ready, wait for end of object's construction
      #if (LORASERVERMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_CR;
        DEBUG_PRINT("CLoraServerManager_ServerManagerAutomaton, waiting, state: ");
        DEBUG_PRINT_HEX(this->m_dwCurrentState);
        DEBUG_PRINT_CR;
      #endif

      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }

  // Main automaton terminated (typically 'CLoraServerManager' being deleted)
  vTaskDelete(NULL);
  this->m_hServerManagerTask = NULL;
}


/********************************************************************************************* 
  'NodeManager' task
 
  This RTOS 'Task' is used to process uplink packets received from 'LoraNodeManager'.
  This 'Task' is known by the 'LoraNodeManager' (i.e. attached) and is direcly notified.
  
  Note: This 'Task' is used to quickly store received 'LoraPacket'. The goal is to release the
        exchange object in order to accept next 'LoraPacket' (i.e. to support bursts of few
        packets) 
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         void CLoraServerManager_NodeManagerAutomaton(CLoraServerManager *this)
 * 
 * @brief      Processes packets exchanged with 'LoraNodeManager' object associated to
 *             'CLoraServerManager'.
 * 
 * @details    This function is the RTOS task used to process uplink packets received from
 *             'LoraNodeManager'.
 *             When an uplink packet is received from 'LoraNodeManager':
 *              - A new 'CLoraServerUpMessage' object is created and inserted in the 
 *                'm_pLoraServerUpMessageArray' memory block array.
 *              - A 'UPLINK_ACCEPTED' event is sent to the 'LoraNodeManager' to inform it that
 *                next 'LoraPacket' can be transmitted (via 'ITransceiverManager' interface).
 *              - A 'UPLINK_RECEIVED' event is sent to main automaton ('ServerManager' task to
 *                inforn it that a new 'LoraPacket' is received. The 'ServerManager' task will
 *                generate a suitable message for the Network Server and send it.
 *             If there is no space in 'm_pLoraServerUpMessageArray' to accept the new 
 *             'Lorapacket' a 'UPLINK_REJECTED' event is sent to the 'LoraNodeManager'.
 * 
 * @param      this
 *             The pointer to CLoraServerManager object.
 *  
 * @return     The RTOS task terminates when object is deleted (typically on main program
 *             exit).
 *
 * @note       The automaton's main loop waits for direct notify.
 *             The notify value is a pointer to a 'CServerManagerItf_LoraSessionPacket' object.
 *             The contents of this 'CServerManagerItf_LoraSessionPacketOb' object may change 
 *             as soon as the 'UPLINK_ACCEPTED' or 'UPLINK_REJECTED' event is sent.
 *             The 'CLoraPacketSession' and 'CLoraPacket' referenced in the received
 *             'CServerManagerItf_LoraSessionPacketOb' object are still valid after sending
 *             'UPLINK_ACCEPTED' or 'UPLINK_REJECTED' event to 'CLoraNodeManager' (i.e. will
 *             be used asynchronously by main automaton).  
 *
 * @note       Packet reception in this task is decoupled from message encoding in main 
 *             automaton task to be able to receive immediatly a second Lora packet (i.e. 
 *             ability to process occasional small bursts) 
*********************************************************************************************/
void CLoraServerManager_NodeManagerAutomaton(CLoraServerManager *this)
{
  CServerManagerItf_LoraSessionPacket pLoraSessionPacket;

  CMemoryBlockArrayEntryOb MemBlockEntry;
  CLoraTransceiverItf_LoraPacket pReceivedPacket;
  CTransceiverManagerItf_SessionEventOb SessionEvent;
  CServerManagerItf_ServerMessageEventOb ServerMessageEvent;
  CLoraServerUpMessage pLoraServerMessage;


  while (this->m_dwCurrentState != LORASERVERMANAGER_AUTOMATON_STATE_TERMINATED)
  {
    if (this->m_dwCurrentState >= LORASERVERMANAGER_AUTOMATON_STATE_INITIALIZED)
    {
      #if (LORASERVERMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("CLoraServerManager_NodeManagerAutomaton, waiting notify");
      #endif

      // Wait for notify
      if (xTaskNotifyWait(0, 0xFFFFFFFF, (DWORD *) &pLoraSessionPacket, pdMS_TO_TICKS(500)) == pdTRUE)
      {
        // Process new 'LoraPacketSession' (i.e. uplink packet)
        #if (LORASERVERMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_CR;
          DEBUG_PRINT("CLoraServerManager_NodeManagerAutomaton, new uplink packet session received: ");
          DEBUG_PRINT_HEX(pLoraSessionPacket->m_dwSessionId);
          DEBUG_PRINT_CR;
        #endif

        // For event sent to 'LoraNodeManager' (i.e. LoRa packet 'ACCEPTED' or 'REJECTED')
        SessionEvent.m_pSession = pLoraSessionPacket->m_pSession;
        SessionEvent.m_dwSessionId = pLoraSessionPacket->m_dwSessionId;  

        // New uplink 'LoraPacket' allowed only in 'RUNNING' automaton state
        if (this->m_dwCurrentState != LORASERVERMANAGER_AUTOMATON_STATE_RUNNING)
        {
          #if (LORASERVERMANAGER_DEBUG_LEVEL0)
            DEBUG_PRINT("[WARNING] LoraPacket received in wrong state: ");
            DEBUG_PRINT_DEC(this->m_dwCurrentState);
            DEBUG_PRINT_CR;
          #endif

          // Notify 'LoraNodeManager' that packet is rejected
          SessionEvent.m_wEventType = TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_REJECTED;
          ITransceiverManager_SessionEvent(this->m_pTransceiverManagerItf, &SessionEvent);
          continue;
        }

        // Received LoRa packet
        pReceivedPacket = (CLoraTransceiverItf_LoraPacket) (pLoraSessionPacket->m_pLoraPacket);

        #if (LORASERVERMANAGER_DEBUG_LEVEL2)
          DEBUG_PRINT("[DEBUG] CLoraServerManager_NodeManagerAutomaton. Received packet, addr: ");
          DEBUG_PRINT_HEX((DWORD) pReceivedPacket);
          DEBUG_PRINT(", Timestamp: ");
          DEBUG_PRINT_DEC(pReceivedPacket->m_dwTimestamp);
          DEBUG_PRINT(", Data size: ");
          DEBUG_PRINT_DEC(pReceivedPacket->m_dwDataSize);
          DEBUG_PRINT(", Head data: ");
          DEBUG_PRINT_HEX(pReceivedPacket->m_usData[0]);
          DEBUG_PRINT(",");
          DEBUG_PRINT_HEX(pReceivedPacket->m_usData[1]);
          DEBUG_PRINT(",");
          DEBUG_PRINT_HEX(pReceivedPacket->m_usData[2]);
          DEBUG_PRINT(",");
          DEBUG_PRINT_HEX(pReceivedPacket->m_usData[3]);
          DEBUG_PRINT_CR;
        #endif

        // Step 1 - Obtain a 'MemoryBlock' to prepare a new 'LoraServerUpMessage' to process the received uplink packet

        if ((pLoraServerMessage = CMemoryBlockArray_GetBlock(this->m_pLoraServerUpMessageArray, &MemBlockEntry)) == NULL)
        {
          // Should never occur. Buffer for 'LoraServerUpMessage' exhausted
          // Note: No recovery mechanism = for stress test in current version
          #if (LORASERVERMANAGER_DEBUG_LEVEL0)
            DEBUG_PRINT_LN("[ERROR] LoraServerUpMessage buffer exhausted. Entering 'ERROR' state");
            this->m_dwCurrentState = LORASERVERMANAGER_AUTOMATON_STATE_ERROR;
          #endif

          // Notify 'LoraNodeManager' that packet is rejected
          SessionEvent.m_wEventType = TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_REJECTED;
          ITransceiverManager_SessionEvent(this->m_pTransceiverManagerItf, &SessionEvent);
          continue;
        }

        // Step 2 - Initialize 'LoraServerUpMessage'

        pLoraServerMessage->m_dwMessageState = LORANODEMANAGER_SERVERUPMESSAGE_STATE_CREATED;

        // The identifier of the 'LoraServerUpMessage' is the index in the 'm_pLoraServerUpMessageArray' MemoryBlockArray
        pLoraServerMessage->m_usMessageId = MemBlockEntry.m_usBlockIndex;

        // The 'CLoraPacket' and 'CLoraPacketSession' objects are owned by 'CLoraNodeManager'
        // The 'CLoraPacket' object is alive until 'TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_PROGRESSING' event is sent to
        // 'CLoraNodeManager'
        pLoraServerMessage->m_pLoraPacket = pLoraSessionPacket->m_pLoraPacket;
        pLoraServerMessage->m_pSession = pLoraSessionPacket->m_pSession;
        pLoraServerMessage->m_pLoraPacketInfo = pLoraSessionPacket->m_pLoraPacketInfo;
        pLoraServerMessage->m_dwSessionId = pLoraSessionPacket->m_dwSessionId;
        pLoraServerMessage->m_wDataLength = 0;

        // The 'LoraServerUpMessage' object is fully defined in MemoryBlocks (i.e. it is 'CREATED')
        // Set the 'Ready' flag to allow other tasks to use it
        CMemoryBlockArray_SetBlockReady(this->m_pLoraServerUpMessageArray, MemBlockEntry.m_usBlockIndex);

        // Step 3 - Notify 'LoraNodeManager' that packet is accepted

        #if (LORASERVERMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[INFO] CLoraServerManager_NodeManagerAutomaton, uplink packet accepted");
        #endif

        SessionEvent.m_wEventType = TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_ACCEPTED;
        ITransceiverManager_SessionEvent(this->m_pTransceiverManagerItf, &SessionEvent);

        // Step 4 - Inform the main automaton that a new LoRa packet must be prosessed (encoded) and sent
        //
        // Note: The processing of received LoRa packet is decoupled here to support small uplink packet bursts
        //       (i.e. building Network Server message from LoRa packet is a bit heavy)

        #if (LORASERVERMANAGER_DEBUG_LEVEL2)
          DEBUG_PRINT("[DEBUG] CLoraServerManager_NodeManagerAutomaton, Sending Event message, Addr: ");
          DEBUG_PRINT_HEX((DWORD) pLoraServerMessage);
          DEBUG_PRINT(", Id: ");
          DEBUG_PRINT_HEX((DWORD) pLoraServerMessage->m_usMessageId);
          DEBUG_PRINT(", Lora packet: ");
          DEBUG_PRINT_HEX((DWORD) pLoraServerMessage->m_pLoraPacket);
          DEBUG_PRINT(", Packet session: ");
          DEBUG_PRINT_HEX((DWORD) pLoraServerMessage->m_pSession);
          DEBUG_PRINT(", Packet Info: ");
          DEBUG_PRINT_HEX((DWORD) pLoraServerMessage->m_pLoraPacketInfo);
          DEBUG_PRINT_CR;
        #endif

        ServerMessageEvent.m_wEventType = SERVERMANAGER_MESSAGEEVENT_UPLINK_RECEIVED;
        ServerMessageEvent.m_pMessage = pLoraServerMessage;
        ServerMessageEvent.m_dwParam = NULL;
//      ServerMessageEvent.m_dwMessageId = (DWORD) pLoraServerMessage->m_usMessageId;
        IServerManager_ServerMessageEvent(this->m_pServerManagerItf, &ServerMessageEvent);
      }
    }
    else
    {
      // Parent object not ready, wait for end of object's initialization
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }

  // Main automaton terminated (typically 'CLoraServerManager' being deleted)
  vTaskDelete(NULL);
  this->m_hNodeManagerTask = NULL;
}


/********************************************************************************************* 
  'Connector' task
 
  This RTOS 'Task' is used to process notifications sent by 'ServerConnectors'.
  The notification have one of the following purposes:
   - To transmit downlink packets received on 'ServerConnectors' from Network Server.
     In this case, the 'Task' knows the 'LoraNodeManager' task to directly notify (i.e. attached) 
     when the 'LoraPacket' is constructed with the message received from the Network Server.
   - To notify 'ServerManager' of events occurred when the 'ServerConnector' sends an uplink
     message to Network Server.
     In this case, the received event is transmitted to the main task of 'ServerManager'.

  Note: The 'ServerConnectors' communicate with 'ServerManager' only via the 'IServerConnectorItf'
        interface (i.e. never use the 'IServerManagerItf' interface).
        This is required for event serialization in 'ServerManager' main automaton.
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         void CLoraServerManager_ConnectorAutomaton(CLoraServerManager *this)
 * 
 * @brief      Processes notifications sent by one of the associated 'LoraServerConnectors.
 *
 * @details    This function is the RTOS task used to process notifications sent by associated
 *             'Serverconnectors' (using a dedicated queue).
 *             The notification have one of the following purposes:
 *              - To transmit downlink packets received  on 'ServerConnectors' from Network Server.
 *                  .. The task invokes the associated 'ServerProtocolEngine' to decode the message
 *                      received form Network Server. 
 *                  .. If the message contains data for Lora node (i.e. 'PULL_RESP' in Semtech
 *                     protocol), the task transmits to the 'LoraNodeManager' the 'LoraPacket' built
 *                     by the 'ServerProtocolEngine'
 *              - To notify 'ServerManager' of events occurred when the 'ServerConnector' sends an 
 *                uplink message to Network Server.
 *                In this case, the received event is transmitted to the main task of 'ServerManager'.
 * 
 * @param      this
 *             The pointer to CLoraServerManager object.
 *  
 * @return     The RTOS task terminates when object is deleted (typically on main program
 *             exit).
 *
 * @note       The automaton's main loop waits for direct notify.
*********************************************************************************************/
void CLoraServerManager_ConnectorAutomaton(CLoraServerManager *this)
{
  CServerConnectorItf_ServerDownlinkMessageOb DownlinkMessage;
  CNetworkServerProtocol_ProcessServerMessageParamsOb ProcessMessageParams;
  DWORD dwResult;
  CServerConnectorItf_ConnectorEventOb ConnectorEvent;
  CServerConnectorItf_ServerDownlinkMessage pDownlinkMessage;
  CServerConnectorItf_DownlinkReceivedParamsOb DownlinkReceivedParams;
  BYTE usBlockIndex;
  CLoraServerUpMessage pLoraServerUpMessage;
  CServerManagerItf_ServerMessageEventOb ServerMessageEvent;
  CMemoryBlockArrayEntryOb MemBlockEntry;

  while (this->m_dwCurrentState < LORASERVERMANAGER_AUTOMATON_STATE_TERMINATED)
  {
    if (this->m_dwCurrentState >= LORASERVERMANAGER_AUTOMATON_STATE_INITIALIZED)
    {
      #if (LORASERVERMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("CLoraServerManager_ConnectorAutomaton, waiting message");
      #endif

      // Wait for messages
      if (xQueueReceive(this->m_hConnectorNotifQueue, &ConnectorEvent, pdMS_TO_TICKS(500)) == pdPASS)
      {
        #if (LORASERVERMANAGER_DEBUG_LEVEL2)
          DEBUG_PRINT("[DEBUG] CLoraServerManager_ConnectorAutomaton, Event message received (processing), Type: ");
          DEBUG_PRINT_HEX(ConnectorEvent.m_wConnectorEventType);
          DEBUG_PRINT(", ticks: ");
          DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
          DEBUG_PRINT_CR;
        #endif

        // Process notification according to its type
        if (ConnectorEvent.m_wConnectorEventType == SERVERCONNECTOR_CONNECTOREVENT_DOWNLINK_RECEIVED)
        {
          // Downlink message received from Network Server
          pDownlinkMessage = &ConnectorEvent.m_DownlinkMessage;

          #if (LORASERVERMANAGER_DEBUG_LEVEL0)
            DEBUG_PRINT_CR;
            DEBUG_PRINT("CLoraServerManager_ConnectorAutomaton, downlink message received, size: ");
            DEBUG_PRINT_DEC((DWORD) pDownlinkMessage->m_wDataSize);
            DEBUG_PRINT_CR;
          #endif
    
          // Step 1 - Invoke the 'ServerProtocolEngine' to process the message
          //
          // Note: The 'ServerProtocolEngine' will update the 'm_dwProtocolMessageId' in the specified 
          //       'ProcessMessageParams' object.
          //       In case of reply (ACK) to uplink message, this identifier is used below to retrieve
          //       the associated 'CLoraServerUpMessageOb' in'm_pLoraServerUpMessageArray'
          // 
          // Note: This operation is synchronous (i.e. Required to access received message data in 'Connector' memory) 

          // The 'ServerProtocolEngine' may encode a LoRa packet if some data must be transmitted to Node (i.e. PULL_RESP) 
          // Obtain a memory block to provide storage area to 'ServerProtocolEngine'
          if ((ProcessMessageParams.m_pData = CMemoryBlockArray_GetBlock(this->m_pDownlinkLoraPacketArray, &MemBlockEntry)) == NULL)
          {
            // Should never occur (max capacity of block area reached)
            // Let execute the program normaly (because memory is not required for some messages)
            #if (LORASERVERMANAGER_DEBUG_LEVEL0)
              DEBUG_PRINT_LN("[ERROR] CLoraServerManager_ConnectorAutomaton, no memory to encode LoRa packet, may fail later");
            #endif
          }
          ProcessMessageParams.m_wLoraPacketLength = 0;
          ProcessMessageParams.m_wMaxLoraPacketLength = LORA_MAX_PAYLOAD_LENGTH;

          ProcessMessageParams.m_wMessageLength = pDownlinkMessage->m_wDataSize;
          ProcessMessageParams.m_pMessageData = pDownlinkMessage->m_pData;
          dwResult = INetworkServerProtocol_ProcessServerMessage(this->m_pNetworkServerProtocolItf, &ProcessMessageParams);
  
          // Step 2 - Release the memory in 'Connector' object
          //          This operation is done asynchronously by 'Connector' object.
          DownlinkReceivedParams.m_dwMessageId = pDownlinkMessage->m_dwMessageId;
          IServerConnector_DownlinkReceived(pDownlinkMessage->m_pConnectorItf, &DownlinkReceivedParams);
  
          // Step 3 - Process received data if required (i.e. according to 'ProtocolEngine' reply)
  
          // Check for replies related to 'uplink' session
          if (NETWORKSERVERPROTOCOL_IS_UPLINKSESSIONEVENT(dwResult) == true)
          {
            if (dwResult != NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_PROGRESSING)
            {
              // Downlink message terminates a protocol 'Uplink' session, additional processing may be required
              // according to type of session (LoRa packet or heartbeat)
              #if (LORASERVERMANAGER_DEBUG_LEVEL2)
                DEBUG_PRINT_LN("[INFO] CLoraServerManager_ConnectorAutomaton, ProtocolEngine session terminated");
              #endif
  
              // Retrieve the associated 'CLoraServerUpMessageOb' in 'm_pLoraServerUpMessageArray'
              // Note: Specific 'CLoraServerUpMessageOb' for heartbeat(outside of array)
              usBlockIndex = (BYTE) LORASERVERMANAGER_SERVERMANAGER_MESSAGEID(ProcessMessageParams.m_dwProtocolMessageId);
              if (LORASERVERMANAGER_SERVERMANAGER_IS_HEARTBEAT(usBlockIndex))
              {
                pLoraServerUpMessage = &this->m_HeartbeatMessageOb;
              }
              else
              {
                pLoraServerUpMessage = CMemoryBlockArray_BlockPtrFromIndex(this->m_pLoraServerUpMessageArray, usBlockIndex);
              }

              // Consistency check
              if (pLoraServerUpMessage->m_dwProtocolMessageId == ProcessMessageParams.m_dwProtocolMessageId)
              {
                // Notify ServerManager 'Main' automaton:
                //  - The uplink session is terminated for the 'ServerManager'
                //  - If the ACK is received for a PUSH_DATA associated to an uplink LoRa packet, the 'confirmation'
                //    packet will generated and sent by 'NodeManager'
               
                // Session terminated
                ServerMessageEvent.m_wEventType = SERVERMANAGER_MESSAGEEVENT_UPLINK_TERMINATED;   
                ServerMessageEvent.m_pMessage = pLoraServerUpMessage;
                // Status: 'TERMINATED' or 'FAILED'
                ServerMessageEvent.m_dwParam = dwResult; 
                
                // Further 'session' processing done asynchronously by 'Main' automaton
                IServerManager_ServerMessageEvent(this->m_pServerManagerItf, &ServerMessageEvent);
              }
              else
              {
                // Should never occur (memory leak in array)
                #if (LORASERVERMANAGER_DEBUG_LEVEL0)
                  DEBUG_PRINT_LN("[ERROR] CLoraServerManager_ConnectorAutomaton, unable to retrieve LoraServerUpMessage (LEAK)");
                #endif
              }
            }
          }
          else if (NETWORKSERVERPROTOCOL_IS_DOWNLINKSESSIONEVENT(dwResult) == true)
          {
            // Check for replies related to 'downlink' session
            if (dwResult == NETWORKSERVERPROTOCOL_DOWNLINKSESSIONEVENT_PREPARED)
            {
              // The Network Server have provided downlink data
              // A LoRa packet has been prepared by the 'ServerProtocolEngine'
              // Ask the 'NodeManager' to forward downlink packet to node
              
              // TO DO 
              #if (LORASERVERMANAGER_DEBUG_LEVEL0)
                DEBUG_PRINT_LN("[ERROR] CLoraServerManager_ConnectorAutomaton, TO DO forward downlink packet");
              #endif
            }
            else
            {
              // TO DO 
              #if (LORASERVERMANAGER_DEBUG_LEVEL0)
                DEBUG_PRINT_LN("[ERROR] CLoraServerManager_ConnectorAutomaton, TO DO process received downlink packet");
              #endif
            }
          }
        }
        else if (ConnectorEvent.m_wConnectorEventType == SERVERCONNECTOR_CONNECTOREVENT_SERVERMSG_EVENT)
        {
          // Event related to send operation (by 'Connector') for uplink message
          // Transmit this event to main automaton (i.e. event serialization in main automaton)
          IServerManager_ServerMessageEvent(this->m_pServerManagerItf, &ConnectorEvent.m_ServerMessageEvent);
        }
        else
        {
          // By design should never occur
          #if (LORASERVERMANAGER_DEBUG_LEVEL0)
            DEBUG_PRINT_LN("[ERROR] CLoraServerManager_ConnectorAutomaton, TO DO unknown connector event type");
          #endif
        }
      }
    }
    else
    {
      // Parent object not ready, wait for end of object's initialization
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }

  // Main automaton terminated (typically 'CLoraServerManager' being deleted)
  vTaskDelete(NULL);
  this->m_hConnectorTask = NULL;
}


/*********************************************************************************************
  Construction

  Protected methods : must be called only object factory and 'IServerManager' interface
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         CLoraServerManager * CLoraServerManager_New()
 * 
 * @brief      Object construction.
 * 
 * @details    
 * 
 * @return     The function returns the pointer to the CLoraServerManager instance.
 *
 * @note       This function only creates the object and its dependencies (RTOS objects).\n
*********************************************************************************************/
CLoraServerManager * CLoraServerManager_New()
{
  CLoraServerManager *this;

#if LORASERVERMANAGER_DEBUG_LEVEL2
  printf("CLoraServerManager_New -> Debug level 2 (DEBUG)\n");
#elif LORASERVERMANAGER_DEBUG_LEVEL1
  printf("CLoraServerManager_New -> Debug level 1 (INFO)\n");
#elif LORASERVERMANAGER_DEBUG_LEVEL0
  printf("CLoraServerManager_New -> Debug level 0 (NORMAL)\n");
#endif 

  if ((this = (void *) pvPortMalloc(sizeof(CLoraServerManager))) != NULL)
  {
    // The 'CLoraNodeObject' is under construction (i.e. embedded tasks must wait the 'INITIALIZED' state
    this->m_dwCurrentState = LORASERVERMANAGER_AUTOMATON_STATE_CREATING;

    // Embedded objects are not defined (i.e. created below)
    this->m_pLoraServerUpMessageArray = NULL;
    this->m_pLoraServerDownMessageArray = NULL;
    this->m_pDownlinkMessageStreamArray = NULL;
    this->m_pDownlinkLoraPacketArray = NULL;

    this->m_hCommandMutex = this->m_hCommandDone = this->m_hServerManagerTask = 
      this->m_hNodeManagerTask = this->m_hConnectorTask = this->m_hServerManagerQueue = 
      this->m_hConnectorNotifQueue = this->m_hTransceiverManagerTask = NULL;
    this->m_pNetworkServerProtocolItf = NULL;

    // Allocate memory blocks for internal collections

    #if (LORASERVERMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraServerManager_New Entering: create object 1");
    #endif

    if ((this->m_pLoraServerUpMessageArray = CMemoryBlockArray_New(sizeof(CLoraServerUpMessageOb),
        LORASERVERMANAGER_MAX_SERVERUPMESSAGES)) == NULL)
    {
      CLoraServerManager_Delete(this);
      return NULL;
    }

    #if (LORASERVERMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraServerManager_New Entering: create object 2");
    #endif

    if ((this->m_pLoraServerDownMessageArray = CMemoryBlockArray_New(sizeof(CLoraServerDownMessageOb),
        LORASERVERMANAGER_MAX_SERVERDOWNMESSAGES)) == NULL)
    {
      CLoraServerManager_Delete(this);
      return NULL;
    }

    #if (LORASERVERMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraServerManager_New Entering: create object 3");
    #endif

    if ((this->m_pDownlinkMessageStreamArray = CMemoryBlockArray_New(LORASERVERMANAGER_MAX_UPMESSAGE_LENGTH,
        LORASERVERMANAGER_MAX_SERVERDOWNMESSAGES)) == NULL)
    {
      CLoraServerManager_Delete(this);
      return NULL;
    }

    #if (LORASERVERMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraServerManager_New Entering: create object 4");
    #endif

    if ((this->m_pDownlinkLoraPacketArray = CMemoryBlockArray_New(LORA_MAX_PAYLOAD_LENGTH,
        LORASERVERMANAGER_MAX_SERVERDOWNMESSAGES)) == NULL)
    {
      CLoraServerManager_Delete(this);
      return NULL;
    }

    #if (LORASERVERMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraServerManager_New Entering: create object 5");
    #endif

    // Create ServerManager automaton task
    if (xTaskCreate((TaskFunction_t) CLoraServerManager_ServerManagerAutomaton, "CLoraServerManager_ServerManagerAutomaton", 
        2048, this, 5, &(this->m_hServerManagerTask)) == pdFAIL)
    {
      CLoraServerManager_Delete(this);
      return NULL;
    }

    #if (LORASERVERMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraServerManager_New Entering: create object 6");
    #endif

    if ((this->m_hCommandMutex = xSemaphoreCreateMutex()) == NULL)
    {
      CLoraServerManager_Delete(this);
      return NULL;
    }

    #if (LORASERVERMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraServerManager_New Entering: create object 7");
    #endif

    if ((this->m_hCommandDone = xSemaphoreCreateBinary()) == NULL)
    {
      CLoraServerManager_Delete(this);
      return NULL;
    }


    #if (LORASERVERMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraServerManager_New Entering: create object 8");
    #endif

    // Create NodeManager automaton task
    if (xTaskCreate((TaskFunction_t) CLoraServerManager_NodeManagerAutomaton, "CLoraServerManager_NodeManagerAutomaton", 
        2048, this, 5, &(this->m_hNodeManagerTask)) == pdFAIL)
    {
      CLoraServerManager_Delete(this);
      return NULL;
    }


    #if (LORASERVERMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraServerManager_New Entering: create object 9");
    #endif

    // Create Connector automaton task
    if (xTaskCreate((TaskFunction_t) CLoraServerManager_ConnectorAutomaton, "CLoraServerManager_ForwarderAutomaton", 
        2048, this, 5, &(this->m_hConnectorTask)) == pdFAIL)
    {
      CLoraServerManager_Delete(this);
      return NULL;
    }


    #if (LORASERVERMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraServerManager_New Entering: create object 10");
    #endif

    // Message queue associated to 'ServerManager' task
    // Used for internal messages and external commands via 'IServerManager' interface
    if ((this->m_hServerManagerQueue = xQueueCreate(10, sizeof(CLoraServerManager_MessageOb))) == NULL)
    {
      CLoraServerManager_Delete(this);
      return NULL;
    }


    #if (LORASERVERMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraServerManager_New Entering: create object 11");
    #endif

    // Event queue associated to 'Connector' task
    // Used to process notifications received from 'ServerConnector' objects
    if ((this->m_hConnectorNotifQueue = xQueueCreate(10, sizeof(CServerConnectorItf_ConnectorEventOb))) == NULL)
    {
      CLoraServerManager_Delete(this);
      return NULL;
    }


    // Initialize object's properties
    this->m_nRefCount = 0;
    this->m_dwCommand = LORASERVERMANAGER_AUTOMATON_CMD_NONE;
    this->m_usConnectorNumber = 0;
//  this->m_dwMissedUplinkPacketdNumber = 0;

//  this->m_ForwardedUplinkPacket.m_pLoraPacket = NULL;
//  this->m_ForwardedUplinkPacket.m_pSession = NULL;
//  this->m_ForwardedUplinkPacket.m_dwSessionId = 0;
//  this->m_dwLastSessionId = 0;

    // Enter the 'CREATED' state
    this->m_dwCurrentState = LORASERVERMANAGER_AUTOMATON_STATE_CREATED;
  }
  return this;
}

/*****************************************************************************************//**
 * @fn         void CLoraServerManager_Delete(CLoraServerManager *this)
 * 
 * @brief      Object destruction.
 * 
 * @details    Destroys the CLoraServerManager object.\n
 *             The associated RTOS objects are destroyed and the memory used by CLoraServerManager
 *             object are released.

 * @param      this
 *             The pointer to CLoraServerManager object.
 *  
 * @return     None.
*********************************************************************************************/
void CLoraServerManager_Delete(CLoraServerManager *this)
{
  // Ask all tasks for termination
  // TO DO (also check how to delete task object)

  // Delete queues (TO DO)

  // Free memory
  if (this->m_pLoraServerUpMessageArray != NULL)
  {
    CMemoryBlockArray_Delete(this->m_pLoraServerUpMessageArray);
  }

  if (this->m_pLoraServerDownMessageArray != NULL)
  {
    CMemoryBlockArray_Delete(this->m_pLoraServerDownMessageArray);
  }

  if (this->m_pDownlinkMessageStreamArray != NULL)
  {
    CMemoryBlockArray_Delete(this->m_pDownlinkMessageStreamArray);
  }

  if (this->m_pDownlinkLoraPacketArray != NULL)
  {
    CMemoryBlockArray_Delete(this->m_pDownlinkLoraPacketArray);
  }

  if (this->m_hCommandMutex != NULL)
  {
    vSemaphoreDelete(this->m_hCommandMutex);
  }
  if (this->m_hCommandDone != NULL)
  {
    vSemaphoreDelete(this->m_hCommandDone);
  }

  vPortFree(this);
}


/*********************************************************************************************
  Private methods (implementation)

  Command processing

  These functions are called by 'SessionManager' automaton when the 
  'LORASERVERMANAGER_AUTOMATON_MSG_COMMAND' notification is received.

  Note: These functions may change automaton state. There is no protection against concurrency
        for automaton state because only functions called from automaton RTOS task are allowed
        to modify state (by design).
*********************************************************************************************/



/*****************************************************************************************//**
 * @fn         bool CLoraServerManager_ProcessAutomatonNotifyCommand(CLoraServerManager *this)
 * 
 * @brief      Process 'ILoraServerManager' command currently waiting in automaton.
 * 
 * @details    This function is invoked by 'ServerManager' automaton when it receives a 
 *             'COMMAND' notification.\n
 *             These commands are issued via calls on 'ILoraServerManager' interface.\n
 *             The command and its parameters are available in member variables of 
 *             CLoraServerManager object.\n
 *             By design, commands are serialized when transmited to CLoraServerManager automaton.
 *             In other words, the processing of one command cannot be interrupted by the 
 *             reception of another command.
 * 
 * @param      this
 *             The pointer to CLoraServerManager object.
 *  
 * @return     The returned value is 'true' if command execution is successful or 'false' in
 *             case of error.
*********************************************************************************************/
bool CLoraServerManager_ProcessAutomatonNotifyCommand(CLoraServerManager *this)
{
  bool bResult;

  switch (this->m_dwCommand)
  {
    case LORASERVERMANAGER_AUTOMATON_CMD_INITIALIZE:
      bResult = CLoraServerManager_ProcessInitialize(this, (CServerManagerItf_InitializeParams) this->m_pCommandParams);
      break;

    case LORASERVERMANAGER_AUTOMATON_CMD_ATTACH:
      bResult = CLoraServerManager_ProcessAttach(this, (CServerManagerItf_AttachParams) this->m_pCommandParams);
      break;

    case LORASERVERMANAGER_AUTOMATON_CMD_START:
      bResult = CLoraServerManager_ProcessStart(this, (CServerManagerItf_StartParams) this->m_pCommandParams);
      break;

    case LORASERVERMANAGER_AUTOMATON_CMD_STOP:
      bResult = CLoraServerManager_ProcessStop(this, (CServerManagerItf_StopParams) this->m_pCommandParams);
      break;

    default:
      #if (LORASERVERMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] CLoraServerManager_ProcessAutomatonNotifyCommand, unknown command");
      #endif
      bResult = false;
      break;
  }

  // Command executed, allow next command for calling task (i.e. command execution by automaton is asynchronous
  // but has synchronous behavior for calling task)
  this->m_dwCommand = LORASERVERMANAGER_AUTOMATON_CMD_NONE;
  xSemaphoreGive(this->m_hCommandDone);

  return bResult;
}


/*****************************************************************************************//**
 * @fn         bool CLoraServerManager_ProcessInitialize(CLoraServerManager *this, 
 *                                            CServerManagerItf_InitializeParams pParams)
 * 
 * @brief      Initializes the object and configure associated 'ServerConnectors'.
 * 
 * @details    This function reads the gateway configuration and initializes the 
 *             'ServerConnectors' (i.e. prepares the connection devices for transmission with
 *             the LoRa Network Server).\n
 *             The connection devices are configured and are waiting in 'StandBy' mode. 
 *             The 'START' and 'STOP' commands are then available to control the activity of
 *             the gateway.\n
 *             This function must be called one time in 'CREATED' automaton state.\n
 *             On exit, possible automaton states are:\n
 *              - 'LORASERVERMANAGER_AUTOMATON_STATE_INITIALIZED' = the object and connection 
 *                devices initialization is successfully completed. The LoRa Network Server side
 *                of the gateway is ready for operation.
 *              - 'LORASERVERMANAGER_AUTOMATON_STATE_ERROR' = failed to initialize LoRa Network
 *                Server side of the gateway.
 * 
 * @param      this
 *             The pointer to CLoraServerManager object.
 *  
 * @param      pParams
 *             The interface method parameters (see 'LoraServerManagerItf.h' for details).
 *
 * @return     The returned value is 'true' if the LoraServerManager is properly initialized or
 *             'false' in case of error.
*********************************************************************************************/
bool CLoraServerManager_ProcessInitialize(CLoraServerManager *this, CServerManagerItf_InitializeParams pParams)
{
  CServerManagerItf_LoraServerSettings pLoraServerSettings;
  CServerConnectorItf_InitializeParamsOb ServerConnectorInitializeParams;
  CNetworkServerProtocol_BuildUplinkMessageParamsOb ProtocolEncodeParams;

  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraServerManager_ProcessInitialize'");
  #endif

  // The 'Initialize' method is allowed only in 'CREATED' and 'ERROR' automaton state
  if ((this->m_dwCurrentState != LORASERVERMANAGER_AUTOMATON_STATE_CREATED) &&
      (this->m_dwCurrentState != LORASERVERMANAGER_AUTOMATON_STATE_ERROR))
  {
    // By design, should never occur
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  // Early version: always configuration not provided, use builtin settings (i.e. statically defined in firmware) 
  if (pParams->m_bUseBuiltinSettings != true)
  {
    pLoraServerSettings = &pParams->LoraServerSettings;

    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function MUST be called with 'UseBuiltinSettings'");
    #endif
    return false;
  }
  else
  {
    // Builtin configuration
    pLoraServerSettings = &g_LoraServerManagerSettings.LoraServerSettings;
  }

  // Step 1: Apply configuration on embedded 'ServerConnector' objects
  // 
  // The number of 'ServerConnector' present in the gateway was indicated on object's construction.
  // The specified configuration must contain settings for at least this number of 'ServerConnectors'.
  //
  // Note: Typically the 'ServerConnector' object establishes the initial connection to the associated
  //       network (e.g. joins a local Wifi network or a cellular network)
  ServerConnectorInitializeParams.m_pServerManagerItf = this->m_pServerManagerItf;
  ServerConnectorInitializeParams.m_hEventNotifyQueue = this->m_hConnectorNotifQueue;
  bool bServerConnected = false; 
  for (BYTE i = 0; i < this->m_usConnectorNumber; i++)
  {
    strcpy(pLoraServerSettings->ConnectorSettings[i].m_szNetworkServerUrl, pLoraServerSettings->m_szNetworkServerUrl);
    pLoraServerSettings->ConnectorSettings[i].m_dwNetworkServerPort = pLoraServerSettings->m_dwNetworkServerPort;

    strcpy(pLoraServerSettings->ConnectorSettings[i].m_szSNTPServerUrl, pLoraServerSettings->m_szSNTPServerUrl);
    pLoraServerSettings->ConnectorSettings[i].m_dwSNTPServerPeriodSec = pLoraServerSettings->m_dwSNTPServerPeriodSec;

    memcpy(pLoraServerSettings->ConnectorSettings[i].m_GatewayMACAddr, pLoraServerSettings->m_GatewayMACAddr, 6);

    ServerConnectorInitializeParams.m_pConnectorSettings = &pLoraServerSettings->ConnectorSettings[i];
    if (IServerConnector_Initialize(this->m_ConnectorDescrArray[i].m_pServerConnectorItf, 
        &ServerConnectorInitializeParams) == true)
    {
      // If the connector is properly initialized, open a session with the 'NetworkServer' 
      // Note: 
      //  - The mechanism for session initialization depends on protocol.
      //  - Typically, the protocols use the 'heartbeat' paradigm in order to keep alive the connection used by
      //    the transport layer (true for Semtech protocol and MQTT).

      // Ask the 'ProtocolEngine ' to provide the 'ping' uplink message
      ProtocolEncodeParams.m_pMessageData = (BYTE *) pvPortMalloc(LORASERVERMANAGER_MAX_UPMESSAGE_LENGTH);
      if (ProtocolEncodeParams.m_pMessageData == NULL)
      {
        // Should never occur, not enough memory to initialize gateway
        #if (LORASERVERMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[ERROR] 'CLoraServerManager_ProcessInitialize' Not enough memory!");
        #endif
        return false;
      }
      ProtocolEncodeParams.m_wMessageType = NETWORKSERVERPROTOCOL_UPLINKMSG_HEARTBEAT;
      ProtocolEncodeParams.m_bForceHeartbeat = true;
      ProtocolEncodeParams.m_pLoraPacket = NULL;
      ProtocolEncodeParams.m_pLoraPacketInfo = NULL;
      ProtocolEncodeParams.m_wMaxMessageLength = LORASERVERMANAGER_MAX_UPMESSAGE_LENGTH;
      ProtocolEncodeParams.m_wMessageLength = 0;
      ProtocolEncodeParams.m_dwProtocolMessageId = 0xFFFFFFFF;
      ProtocolEncodeParams.m_wServerManagerMessageId = 0xFF;

      if (INetworkServerProtocol_BuildUplinkMessage(this->m_pNetworkServerProtocolItf, &ProtocolEncodeParams) != true)
      {
        // Should never occur. Unable to obtain the first 'heartbeat' uplink message for to initialize session with NetworkServer
        #if (LORASERVERMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[ERROR] 'CLoraServerManager_ProcessInitialize' Failed to obtain first heatbeat message from ProtocolEngine");
        #endif

        vPortFree(ProtocolEncodeParams.m_pMessageData);
        return false;
      }

      // Ask the 'Connector' to send this message and to wait for the reply
      // Note: The first 'send / receive' exchange with the Network Server does not use the dedicated tasks of
      //       'Connector' and 'ServerManager' objects (i.e. these objects will enter their 'RUNNING' state only
      //       when initialization is completed)

      CServerConnectorItf_SendReceiveParamsOb SendReceiveParams;
      SendReceiveParams.m_pReply = (BYTE *) pvPortMalloc(LORASERVERMANAGER_MAX_UPMESSAGE_LENGTH);
      if (SendReceiveParams.m_pReply == NULL)
      {
        // Should never occur, not enough memory to initialize gateway
        #if (LORASERVERMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[ERROR] 'CLoraServerManager_ProcessInitialize' Not enough memory(2)!");
        #endif
        vPortFree(ProtocolEncodeParams.m_pMessageData);
        return false;
      }

      SendReceiveParams.m_pData = ProtocolEncodeParams.m_pMessageData;
      SendReceiveParams.m_wDataLength = ProtocolEncodeParams.m_wMessageLength;
      SendReceiveParams.m_wReplyMaxLength = LORASERVERMANAGER_MAX_UPMESSAGE_LENGTH;
      SendReceiveParams.m_wReplyLength = 0;
      SendReceiveParams.m_dwTimeoutMillisec = 60000;

      CNetworkServerProtocol_ProcessSessionEventParamsOb ProcessSessionEventParams;

      if (IServerConnector_SendReceive(this->m_ConnectorDescrArray[i].m_pServerConnectorItf, &SendReceiveParams) == true)
      {
        // Reply received from 'NetworkServer', forward it to 'ProtocolEngine'
        // 1. Notify 'ProtocolEngine' for 'heartbeat' message sent
        ProcessSessionEventParams.m_wSessionEvent = NETWORKSERVERPROTOCOL_SESSIONEVENT_SENT;
        ProcessSessionEventParams.m_dwProtocolMessageId = ProtocolEncodeParams.m_dwProtocolMessageId;

        if (INetworkServerProtocol_ProcessSessionEvent(this->m_pNetworkServerProtocolItf, &ProcessSessionEventParams) == 
            NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_PROGRESSING)
        {
          // 2. Transmit the reply to 'ProtocolEngine'
          CNetworkServerProtocol_ProcessServerMessageParamsOb ProcessServerMessageParams;
          ProcessServerMessageParams.m_pMessageData = SendReceiveParams.m_pReply;
          ProcessServerMessageParams.m_wMessageLength = SendReceiveParams.m_wReplyLength;

          if (INetworkServerProtocol_ProcessServerMessage(this->m_pNetworkServerProtocolItf, &ProcessServerMessageParams) == 
              NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_TERMINATED)
          {
            // If NetworkServer session is properly initialized, use this 'Connector' until reboot (i.e. no dynamic failover)
            this->m_ConnectorDescrArray[i].m_bActive = bServerConnected = true;

            // Release session in 'ProtocolEngine'
            ProcessSessionEventParams.m_wSessionEvent = NETWORKSERVERPROTOCOL_SESSIONEVENT_RELEASED;
            INetworkServerProtocol_ProcessSessionEvent(this->m_pNetworkServerProtocolItf, &ProcessSessionEventParams);
          }
          else
          {
            #if (LORASERVERMANAGER_DEBUG_LEVEL0)
              DEBUG_PRINT_LN("[ERROR] CLoraServerManager_ProcessInitialize, failed to initialize NetworkServer session (rejected 1)");
            #endif
          }
        }
        else
        {
          // Should never occur (simple notification)
          #if (LORASERVERMANAGER_DEBUG_LEVEL0)
            DEBUG_PRINT_LN("[ERROR] CLoraServerManager_ProcessInitialize, failed to initialize NetworkServer session (rejected 2)");
          #endif
        }
      }
      else
      {
        // Transport error (probably no reply from NestworkServer)
        #if (LORASERVERMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[ERROR] CLoraServerManager_ProcessInitialize, failed to initialize NetworkServer session (no reply)");
        #endif

        ProcessSessionEventParams.m_wSessionEvent = NETWORKSERVERPROTOCOL_SESSIONEVENT_CANCELED;
        ProcessSessionEventParams.m_dwProtocolMessageId = ProtocolEncodeParams.m_dwProtocolMessageId;
        INetworkServerProtocol_ProcessSessionEvent(this->m_pNetworkServerProtocolItf, &ProcessSessionEventParams);
      }

      // Always exit here (i.e. network reachable using the connector and NetworkServer session started or not)
      // If NetworkServer session not started, the 'ServerManager' initialization fails (i.e. no reason to be successful
      // with another connector)
      vPortFree(ProtocolEncodeParams.m_pMessageData);
      vPortFree(SendReceiveParams.m_pReply);
      break;
    }
    else
    {
      // Unable to initialize the 'Connector', try with next one if any
      #if (LORASERVERMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[INFO] CLoraServerManager_ProcessInitialize, failed to initialize Connector, checking for another one");
      #endif
    }

  }

  if (bServerConnected == false)
  {
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraServerManager_ProcessInitialize, Failed to initialize, cannot join any network");
    #endif
    return false;
  }

  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CLoraServerManager_ProcessInitialize, connected to Network Server");
  #endif


  // Step 2: Store configuration for access to Network Server
  strcpy(this->m_szNetworkServerUrl, pLoraServerSettings->m_szNetworkServerUrl);
  strcpy(this->m_szNetworkServerUser, pLoraServerSettings->m_szNetworkServerUser);
  strcpy(this->m_szNetworkServerPassword, pLoraServerSettings->m_szNetworkServerPassword);



  // Step 3: Attach the 'TransceiverManager'
  //
  // The 'TransceiverManager' will directly notify the 'NodeManagerAutomaton' task of 'LoraServerManager'
  // when a new Lora Packet is received (Uplink = to forward to Network Server)
  CTransceiverManagerItf_AttachParamsOb AttachParams;
  AttachParams.m_hPacketForwarderTask = this->m_hNodeManagerTask;

  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[DEBUG] CLoraServerManager_ProcessInitialize, calling ITransceiverManager_Attach");
  #endif

  ITransceiverManager_Attach((ITransceiverManager) pParams->pTransceiverManagerItf, &AttachParams);
  this->m_pTransceiverManagerItf = (ITransceiverManager) pParams->pTransceiverManagerItf;

  // Adjust state of 'LoraServerManager':
  //  - Enter the 'IDLE' state if the 'TransceiverManager' is already attached
  //  - Otherwise enter the 'INITIALIZED' state (i.e. the 'IDLE' state will be entered when 
  //    'TransceiverManager' will invoke the 'Attach' method)
  // Note: By design, no concurrency on automaton state variable
  if (this->m_hTransceiverManagerTask == NULL)
  {
    this->m_dwCurrentState = LORASERVERMANAGER_AUTOMATON_STATE_INITIALIZED;
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] CLoraServerManager automaton state changed: 'INITIALIZED'");
    #endif
  }
  else
  {
    this->m_dwCurrentState = LORASERVERMANAGER_AUTOMATON_STATE_IDLE;
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] CLoraServerManager automaton state changed: 'IDLE'");
    #endif
  }
      

  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CLoraServerManager successfully initialized for Network Server access");
  #endif
  return true;
}



bool CLoraServerManager_ProcessAttach(CLoraServerManager *this, CServerManagerItf_AttachParams pParams)
{
  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraServerManager_ProcessAttach'");
  #endif

  // The 'Attach' method is allowed only in 'CREATED' and 'INITIALIZED' automaton state
  if ((this->m_dwCurrentState != LORASERVERMANAGER_AUTOMATON_STATE_CREATED) &&
      (this->m_dwCurrentState != LORASERVERMANAGER_AUTOMATON_STATE_INITIALIZED))
  {
    // By design, should never occur
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  // Check that no 'TransceiverManager' already attached
  // Command executed once at the end of startup process
  if (this->m_hTransceiverManagerTask != NULL)
  {
    // By design, should never occur
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Node transceiver already attached");
    #endif
    return false;
  }

  // Note: Task in 'CLoraNodeManager' object
  this->m_hTransceiverManagerTask = pParams->m_hNodeManagerTask;

  // Enter the 'IDLE' state if current state is 'INITIALIZED' 
  // Note: By design, no concurrency on automaton state variable
  if (this->m_dwCurrentState == LORASERVERMANAGER_AUTOMATON_STATE_INITIALIZED)
  {
    this->m_dwCurrentState = LORASERVERMANAGER_AUTOMATON_STATE_IDLE;
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] CLoraServerManager automaton state changed: 'IDLE'");
    #endif
  }

  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CLoraServerManager successfully attached to forwarder");
  #endif
  return true;
}


bool CLoraServerManager_ProcessStart(CLoraServerManager *this, CServerManagerItf_StartParams pParams)
{
  BYTE usConnectorId;
  CConnectorDescr pConnectorDescr;
  CServerConnectorItf_StartParamsOb StartParams;

  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraServerManager_ProcessStart'");
  #endif

  // The 'Start' method is allowed only in 'IDLE' automaton state
  if (this->m_dwCurrentState != LORASERVERMANAGER_AUTOMATON_STATE_IDLE)
  {
    // By design, should never occur
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  // Start the active 'ServerConnectors'
  // Note: In current version only one 'ServerConnector' is activated (defined on Gateway initialization)
  for (usConnectorId = 0; usConnectorId < this->m_usConnectorNumber; usConnectorId++)
  {
    pConnectorDescr = this->m_ConnectorDescrArray + usConnectorId;

    if (pConnectorDescr->m_bActive == true)
    {
      StartParams.m_bForce = false;
  
      if (IServerConnector_Start(pConnectorDescr->m_pServerConnectorItf, &StartParams) == true)
      {
        // 'ServerConnector' has accepted the start command
        // This operation is executed asynchronously and is assumed successful (TO DO: active wait for result)
        #if (LORASERVERMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[INFO] CLoraServerManager_ProcessStart, Start command sent to active ServerConnector");
        #endif
  
        // Enter the 'RUNNING' state
        // Note: By design, no concurrency on automaton state variable
        this->m_dwCurrentState = LORASERVERMANAGER_AUTOMATON_STATE_RUNNING;
        #if (LORASERVERMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[INFO] CLoraServerManager automaton state changed: 'RUNNING'");
        #endif


        #if (LORASERVERMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[INFO] CLoraServerManager successfully started (ready to create sessions)");
        #endif
        return true;
      }
      else
      {
        // Should never occur
        #if (LORASERVERMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[ERROR] CLoraServerManager_ProcessStart, Active Server start command refused");
        #endif
      }
    }
  }

  // No active 'ServerConnector' found (should never occur)
  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[ERROR] CLoraServerManager_ProcessStart, Unable to start becaus no active ServerConnector found");
  #endif
  return false;
}

bool CLoraServerManager_ProcessStop(CLoraServerManager *this, CServerManagerItf_StopParams pParams)
{
  BYTE usConnectorId;
  CConnectorDescr pConnectorDescr;
  CServerConnectorItf_StopParamsOb StopParams;

  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraServerManager_ProcessStop'");
  #endif

  // The 'Stop' method is allowed only in 'RUNNING' automaton state
  if (this->m_dwCurrentState != LORASERVERMANAGER_AUTOMATON_STATE_RUNNING)
  {
    // By design, should never occur
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  // Enter the 'STOPPING' state
  // Note: By design, no concurrency on automaton state variable
  this->m_dwCurrentState = LORASERVERMANAGER_AUTOMATON_STATE_STOPPING;
  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CLoraServerManager automaton state changed: 'STOPPING'");
  #endif

  // Stop the active 'ServerConnectors'
  // Note: This is a forced 'stop' (i.e. it does not leave 'ServerConnectors' started in order to
  //       exchange remaining protocol messages associated to current sessions) 
  // Note: In current version only one 'ServerConnector' is activated (defined on Gateway initialization)
  for (usConnectorId = 0; usConnectorId < this->m_usConnectorNumber; usConnectorId++)
  {
    pConnectorDescr = this->m_ConnectorDescrArray + usConnectorId;

    if (pConnectorDescr->m_bActive == true)
    {
      StopParams.m_bForce = false;

      if (IServerConnector_Stop(pConnectorDescr->m_pServerConnectorItf, &StopParams) == true)
      {
        // 'ServerConnector' has accepted the stop command
        // This operation is executed asynchronously and is assumed successful (TO DO: active wait for result)
        #if (LORASERVERMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[INFO] CLoraServerManager_ProcessStop, Start command sent to active ServerConnector");
        #endif
        break;
      }
      else
      {
        // Should never occur
        #if (LORASERVERMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[ERROR] CLoraServerManager_ProcessStop, Active Server start command refused");
        #endif
      }
    }
  }

  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CLoraServerManager currently stopping (no more sessions created)");
  #endif
  return true;
}


/*********************************************************************************************
  Private methods (implementation)

  Processing of 'ServerMessageEvent' events (i.e. received when an event occurs on the 
  'LoraServerUpMessage' or 'LoraServerDownMessage' object during its life cycle)

  These functions are called by 'ServerManager' automaton when one of the 
  'SERVERMANAGER_MESSAGEEVENT_xxx' notification is received.

  Note: These functions may change automaton state. There is no protection against concurrency
        for automaton state because only functions called from automaton RTOS task are allowed
        to modify state (by design).

  Note for uplink messages:
    - Different types of uplink messages could be referenced in this object:
       .. Encoded LoRa packet received from node
       .. 'heartbeat' (periodical push from gateway and/or periodical pull request send by gateway)
    - The event can be transmitted (if required) to 'TransceiverManager' only if it is associated 
      to an uplink LoRa packet sent by a node.
    - The 'm_usMessageId == 0xFF' expression in used in implementation to check if message is
      associated to 'LoRa packet' from node or to a 'Heartbeat'
*********************************************************************************************/


// A 'CLoraPacket' just received from 'CLoraNodeManager':
//  - Build the message stream according to LoRa Network Server protocol
//  - Send the message to LoRa Network Server
// Note: This method is used only for LoRa packet (i.e. not for 'heartbeat')
void CLoraServerManager_ProcessServerMessageEventUplinkReceived(CLoraServerManager *this, 
                                                                CLoraServerUpMessage pLoraServerMessage)
{
  CTransceiverManagerItf_SessionEventOb SessionEvent;
  CServerManagerItf_ServerMessageEventOb ServerMessageEvent;
  CNetworkServerProtocol_BuildUplinkMessageParamsOb ProtocolEncodeParams;

  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraServerManager_ProcessServerMessageEventUplinkReceived'");
  #endif

  #if (LORASERVERMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] 'CLoraServerManager_ProcessServerMessageEventUplinkReceived' - ticks: ");
    DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
    DEBUG_PRINT_CR;
  #endif


  // Access 'LoraServerUpMessage'

  #if (LORASERVERMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CLoraServerManager_ProcessServerMessageEventUplinkReceived, Received message, Addr: ");
    DEBUG_PRINT_HEX((DWORD) pLoraServerMessage);
    DEBUG_PRINT(", Id: ");
    DEBUG_PRINT_HEX((DWORD) pLoraServerMessage->m_usMessageId);
    DEBUG_PRINT(", Lora packet: ");
    DEBUG_PRINT_HEX((DWORD) pLoraServerMessage->m_pLoraPacket);
    DEBUG_PRINT(", Packet session: ");
    DEBUG_PRINT_HEX((DWORD) pLoraServerMessage->m_pSession);
    DEBUG_PRINT(", Packet Info: ");
    DEBUG_PRINT_HEX((DWORD) pLoraServerMessage->m_pLoraPacketInfo);
    DEBUG_PRINT_CR;

    CLoraTransceiverItf_LoraPacket pReceivedPacket;
    pReceivedPacket = (CLoraTransceiverItf_LoraPacket) pLoraServerMessage->m_pLoraPacket;
    DEBUG_PRINT("[DEBUG] CLoraServerManager_ProcessServerMessageEventUplinkReceived. Received packet, addr: ");
    DEBUG_PRINT_HEX((DWORD) pReceivedPacket);
    DEBUG_PRINT(", Timestamp: ");
    DEBUG_PRINT_DEC(pReceivedPacket->m_dwTimestamp);
    DEBUG_PRINT(", Data size: ");
    DEBUG_PRINT_DEC(pReceivedPacket->m_dwDataSize);
    DEBUG_PRINT(", Head data: ");
    DEBUG_PRINT_HEX(pReceivedPacket->m_usData[0]);
    DEBUG_PRINT(",");
    DEBUG_PRINT_HEX(pReceivedPacket->m_usData[1]);
    DEBUG_PRINT(",");
    DEBUG_PRINT_HEX(pReceivedPacket->m_usData[2]);
    DEBUG_PRINT(",");
    DEBUG_PRINT_HEX(pReceivedPacket->m_usData[3]);
    DEBUG_PRINT_CR;
  #endif


  // Step 1: Build the message stream according to LoRa Network Server protocol
  ProtocolEncodeParams.m_pLoraPacket = pLoraServerMessage->m_pLoraPacket;
  ProtocolEncodeParams.m_pLoraPacketInfo = pLoraServerMessage->m_pLoraPacketInfo;
  ProtocolEncodeParams.m_wMessageType = NETWORKSERVERPROTOCOL_UPLINKMSG_LORADATA;
  ProtocolEncodeParams.m_wMaxMessageLength = LORASERVERMANAGER_MAX_UPMESSAGE_LENGTH;
  ProtocolEncodeParams.m_wMessageLength = 0;
  ProtocolEncodeParams.m_pMessageData = (BYTE *) &pLoraServerMessage->m_usData; 

  if (INetworkServerProtocol_BuildUplinkMessage(this->m_pNetworkServerProtocolItf, &ProtocolEncodeParams) != true)
  {
    // Should never occur. Unable to send LoRa packet, discard it.
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] 'CLoraServerManager_ProcessServerMessageEventUplinkReceived' Failed to encode LoRa packet");
    #endif

    CLoraServerManager_ProcessServerMessageEventUplinkFailed(this, pLoraServerMessage);
    return;
  }

  // ServerMessage prepared by ProtocolEngine, returned data are stream length and dwProtocolMessageId
  // Set them in LoraServerUpMessage (i.e. required to process it)
  pLoraServerMessage->m_dwMessageState = LORANODEMANAGER_SERVERUPMESSAGE_STATE_PREPARED;
  pLoraServerMessage->m_wDataLength = ProtocolEncodeParams.m_wMessageLength;
  pLoraServerMessage->m_dwProtocolMessageId = ProtocolEncodeParams.m_dwProtocolMessageId;

  // Step 2: Notify the 'LoraNodeManager' that LoRa packet is currently being sent
  //         The 'LoraNodeManager' may release the MemoryBlock used to store the 'CLoraPacket'
  //         (i.e. not required anymore because we are sending the encoded stream to Network Server) 
  pLoraServerMessage->m_pLoraPacket = NULL;

  SessionEvent.m_pSession = pLoraServerMessage->m_pSession;
  SessionEvent.m_dwSessionId = pLoraServerMessage->m_dwSessionId;  
  SessionEvent.m_wEventType = TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_PROGRESSING;
  ITransceiverManager_SessionEvent(this->m_pTransceiverManagerItf, &SessionEvent);

  // Step 3: Notify the 'LoraServerManager' that message can be sent to Network Server
  //         The send operation is triggered asynchronously to allow processing of events received
  //         during message preparation (i.e. encoding may take a significant duration)
  ServerMessageEvent.m_wEventType = SERVERMANAGER_MESSAGEEVENT_UPLINK_PREPARED;
  ServerMessageEvent.m_pMessage = pLoraServerMessage;
  ServerMessageEvent.m_dwParam = NULL;
//ServerMessageEvent.m_dwMessageId = (DWORD) pLoraServerMessage->m_usMessageId;
  IServerManager_ServerMessageEvent(this->m_pServerManagerItf, &ServerMessageEvent);
}


// The 'LoraServerUpMessage' is ready and can be sent to Network Server
// Retrieve the next (first) 'ServerConnector' and launch a send operation
void CLoraServerManager_ProcessServerMessageEventUplinkPrepared(CLoraServerManager *this, 
                                                                CLoraServerUpMessage pLoraServerMessage)
{
  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraServerManager_ProcessServerMessageEventUplinkPrepared'");
  #endif

  #if (LORASERVERMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] 'CLoraServerManager_ProcessServerMessageEventUplinkPrepared' - ticks: ");
    DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
    DEBUG_PRINT_CR;
  #endif

  if (CLoraServerManager_SendServerMessage(this, pLoraServerMessage, true) != true)
  {
    // No 'ServerConnector' available (typically network unreachable)
    // No recovery mechanism in this early version
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[WARNING] CLoraServerManager_ProcessServerMessageEventUplinkPrepared, network unreachable");
    #endif

    CLoraServerManager_ProcessServerMessageEventUplinkFailed(this, pLoraServerMessage);
  }
  else
  {
    // The 'ServerConnector' is going to launch the send operation
    // The result will be received asynchronously ('SERVERMANAGER_MESSAGEEVENT_UPLINK_SENT' or
    // 'SERVERMANAGER_MESSAGEEVENT_UPLINK_SEND_FAILED')
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] CLoraServerManager_ProcessServerMessageEventUplinkPrepared, connector has accepted to send message to Network Server (async)");
    #endif
  }
}


// The 'LoraServerUpMessage' has been successfully sent to the Network Server (transport layer)
//  - Notify the 'ProtocolEngine'
//  - If the 'ProtocolEngine' replies that transaction is terminated, notify the 'LoraNodeManager'
//    and terminate the 'LoraServerUpMessage' life.
//    Depending on the protocol, the 'ProtocolEngine' may ask to wait for completion (i.e. 'ACK'
//    message expected from Network Server)
void CLoraServerManager_ProcessServerMessageEventUplinkSent(CLoraServerManager *this, 
                                                            CLoraServerUpMessage pLoraServerMessage)
{
  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraServerManager_ProcessServerMessageEventUplinkSent'");
  #endif

  #if (LORASERVERMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] 'CLoraServerManager_ProcessServerMessageEventUplinkSent' - ticks: ");
    DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
    DEBUG_PRINT_CR;
  #endif

  // Notify the 'ProtocolEngine' that message is sent
  CNetworkServerProtocol_ProcessSessionEventParamsOb ProcessSessionEventParams;
  ProcessSessionEventParams.m_wSessionEvent = NETWORKSERVERPROTOCOL_SESSIONEVENT_SENT;
  ProcessSessionEventParams.m_dwProtocolMessageId = pLoraServerMessage->m_dwProtocolMessageId;

  switch (INetworkServerProtocol_ProcessSessionEvent(this->m_pNetworkServerProtocolItf, &ProcessSessionEventParams))
  {
    case NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_PROGRESSING:
      // The 'ProtocolEngine' is waiting more events to complete the transaction (typically 'ACK' reply)
      // The 'CLoraServerManager_ProcessServerMessageEventUplinkTerminated' method will be invoked when the
      // transaction is completed (typically 'ACK' received or transaction timed out) 
      #if (LORASERVERMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[INFO] 'CLoraServerManager_ProcessServerMessageEventUplinkSent' - ProtocolEngine asks to wait");
      #endif
      break;

    case NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_FAILED:
      // Should never occur (ProtocolEngine transaction expired before Connector successfully sends the message)
      // Note: optimistic behavior (i.e. Network Server should have received the message and taken it in account)
      #if (LORASERVERMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[WARNING] 'CLoraServerManager_ProcessServerMessageEventUplinkSent' - ProtocolEngine reports error");
      #endif
      
      // NOTE: Fallthrough

    case NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_TERMINATED:
      // The message is sent and transaction in ProtocolEngine is successfully terminated
      // Life of 'LoraServerMessage' is terminated
      CLoraServerManager_ProcessServerMessageEventUplinkTerminated(this, pLoraServerMessage, 
                                                                   NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_TERMINATED);
      break;
  }
}


// Current 'ServerConnector' cannot send the 'LoraServerUpMessage', try with next 'ServerConnector' 
// (if any)
void CLoraServerManager_ProcessServerMessageEventUplinkSendFailed(CLoraServerManager *this, 
                                                                  CLoraServerUpMessage pLoraServerMessage)
{
  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraServerManager_ProcessServerMessageEventUplinkSendFailed'");
  #endif

  #if (LORASERVERMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] 'CLoraServerManager_ProcessServerMessageEventUplinkSendFailed' - ticks: ");
    DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
    DEBUG_PRINT_CR;
  #endif

  if (CLoraServerManager_SendServerMessage(this, pLoraServerMessage, false) != true)
  {
    // No more 'ServerConnector' available
    // No recovery mechanism in this early version
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[WARNING] CLoraServerManager_ProcessServerMessageEventUplinkSendFailed, no more connector");
    #endif

    CLoraServerManager_ProcessServerMessageEventUplinkFailed(this, pLoraServerMessage);
  }
  else
  {
    // The 'ServerConnector' is going to launch the send operation
    // The result will be received asynchronously ('SERVERMANAGER_MESSAGEEVENT_UPLINK_SENT' of
    // 'SERVERMANAGER_MESSAGEEVENT_UPLINK_SEND_FAILED')
    #if (LORASERVERMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] CLoraServerManager_ProcessServerMessageEventUplinkSendFailed, next connector is sending message to Network Server");
    #endif
  }
}


// The 'LoraServerUpMessage' has been processed (successfully or not)
// The 'dwProtocolState' parameter indicates the status of the send operation
//  - NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_TERMINATED = Message successfully sent to Network Server
//  - NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_FAILED = Message not sent to or not acknowledged by the Network Server
// The function behaves has follows:
//  - If the uplink message is for a LoRa packet, the 'LoraNodeManager' is notified 
//  - The 'LoraServerMessage' is removed from MemoryBlockArray
void CLoraServerManager_ProcessServerMessageEventUplinkTerminated(CLoraServerManager *this, 
                                                                  CLoraServerUpMessage pLoraServerMessage, DWORD dwProtocolState)
{
  CTransceiverManagerItf_SessionEventOb SessionEvent;
  CNetworkServerProtocol_ProcessSessionEventParamsOb ProcessSessionEventParams;

  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraServerManager_ProcessServerMessageEventUplinkTerminated'");
  #endif

  #if (LORASERVERMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] 'CLoraServerManager_ProcessServerMessageEventUplinkTerminated' - ticks: ");
    DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
    DEBUG_PRINT_CR;
  #endif

  // If not a 'heartbeat', notify the 'LoraNodeManager' for the result of uplink LoRa packet send operation
  // The 'LoraNodeManager' may release the MemoryBlock used to store the 'CLoraPacketSession'
  // (do not access it from now)
  if (!LORASERVERMANAGER_SERVERMANAGER_IS_HEARTBEAT(pLoraServerMessage->m_usMessageId))
  {
    #if (LORASERVERMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraServerManager_ProcessServerMessageEventUplinkTerminated, processing session for LoRa packet send");
    #endif

    SessionEvent.m_pSession = pLoraServerMessage->m_pSession;
    SessionEvent.m_dwSessionId = pLoraServerMessage->m_dwSessionId;  
    SessionEvent.m_wEventType = dwProtocolState == NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_TERMINATED ?
                                 TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_SENT : TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_FAILED;
    ITransceiverManager_SessionEvent(this->m_pTransceiverManagerItf, &SessionEvent);

    // If not a 'heartbeat', release 'CLoraServerUpMessage' memory block
    #if (LORASERVERMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT("[DEBUG] CLoraServerManager_ProcessServerMessageEventUplinkTerminated, destroying LoraServerUpMessage, id: ");
      DEBUG_PRINT_HEX(pLoraServerMessage->m_usMessageId);
      DEBUG_PRINT_CR;
    #endif

    CMemoryBlockArray_ReleaseBlock(this->m_pLoraServerUpMessageArray, pLoraServerMessage->m_usMessageId);
  }

  // Confirm to the 'ProtocolEngine' that 'LoraServerManager' has finished with the protocol session
  ProcessSessionEventParams.m_wSessionEvent = NETWORKSERVERPROTOCOL_SESSIONEVENT_RELEASED;
  ProcessSessionEventParams.m_dwProtocolMessageId = pLoraServerMessage->m_dwProtocolMessageId;

  INetworkServerProtocol_ProcessSessionEvent(this->m_pNetworkServerProtocolItf, &ProcessSessionEventParams);
}


// The 'LoraServerUpMessage' cannot be sent (fatal error)
// Notify the 'ProtocolEngine' and terminate the uplink message send session
void CLoraServerManager_ProcessServerMessageEventUplinkFailed(CLoraServerManager *this, 
                                                              CLoraServerUpMessage pLoraServerMessage)
{
  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraServerManager_ProcessServerMessageEventUplinkFailed'");
  #endif

  #if (LORASERVERMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] 'CLoraServerManager_ProcessServerMessageEventUplinkFailed' - ticks: ");
    DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
    DEBUG_PRINT_CR;
  #endif

  // Notify the 'ProtocolEngine' that message is not sent
  CNetworkServerProtocol_ProcessSessionEventParamsOb ProcessSessionEventParams;
  ProcessSessionEventParams.m_wSessionEvent = NETWORKSERVERPROTOCOL_SESSIONEVENT_SENDFAILED;
  ProcessSessionEventParams.m_dwProtocolMessageId = pLoraServerMessage->m_dwProtocolMessageId;

  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DWORD dwResultCode =
  #endif

  INetworkServerProtocol_ProcessSessionEvent(this->m_pNetworkServerProtocolItf, &ProcessSessionEventParams);

  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    // The 'ProtocolEngine' should reply that transaction is terminated
    if ((dwResultCode != NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_TERMINATED) &&
        (dwResultCode != NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_FAILED))
    {
      DEBUG_PRINT_LN("[WARNING] 'CLoraServerManager_ProcessServerMessageEventUplinkFailed' - Wrong reply from ProtocolEngine");
    }
  #endif

  // The uplink message send session is terminated (failed state)
  CLoraServerManager_ProcessServerMessageEventUplinkTerminated(this, pLoraServerMessage,
                                                               NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_FAILED);
}

                   
/*********************************************************************************************
  Private methods (implementation)

  Access and management of 'ServerConnectors' 
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         bool CLoraServerManager_SendServerMessage(CLoraServerManager *this, 
 *                   CLoraServerUpMessage pLoraServerMessage, bool bFirstConnector)
 * 
 * @brief      Retrieves a 'ServerConnector' and sends data contained in specified 
 *             'LoraServerUpMessage'.
 * 
 * @details    This function iterates the 'ServerConnector' collection to retrieve the next
 *             connector connected to the network and asks it to send message data to associated
 *             Network Server.\n
 *             The send operation is asynchronous. The 'LoraServerManager' will be notified of
 *             the result by the 'ServerConnector' through its 'IServerManager' interface):\n
 *              - 'SERVERMANAGER_MESSAGEEVENT_UPLINK_SENT' = the 'ServerConnector' has 
 *                 successfully sent message data to Network Server.
 *              - 'SERVERMANAGER_MESSAGEEVENT_UPLINK_SEND_FAILED' = the 'ServerConnector' failed
 *                to send message data to Network Server
 * 
 * @param      this
 *             The pointer to CLoraServerManager object.
 *  
 * @param      pLoraServerMessage
 *             The 'LoraServerUpMessage' object containing the data to send to Network Server.\n
 *             The 'm_usLastConnectorId' member variable of 'LoraServerUpMessage' indicates the 
 *             index of the 'ServerConnector' used for last send operation (i.e. failover)
 *
 * @param      bFirstConnector
 *             A 'true' value starts a new iteration in 'ServerConnector' collection. If 'false'
 *             is specified, the iteration starts at 'm_usLastConnectorId + 1' index.
 *             
 * @return     The returned value is 'true' if a 'ServerConnector' has accepted to send to
 *             message.\n
 *             A 'false' value is returned when end of 'ServerConnector' collection is 
 *             reached an no 'ServerConnector' has accepted to send the message.
*********************************************************************************************/
bool CLoraServerManager_SendServerMessage(CLoraServerManager *this, CLoraServerUpMessage pLoraServerMessage, 
                                          bool bFirstConnector)
{
  BYTE usConnectorId;
  CConnectorDescr pConnectorDescr;
  CServerConnectorItf_SendParamsOb SendParams;

  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraServerManager_SendServerMessage'");
  #endif


  for (usConnectorId = bFirstConnector == true ? 0 : pLoraServerMessage->m_usLastConnectorId + 1;
       usConnectorId < this->m_usConnectorNumber; usConnectorId++)
  {
    pConnectorDescr = this->m_ConnectorDescrArray + usConnectorId;

    SendParams.m_wDataLength = pLoraServerMessage->m_wDataLength;
    SendParams.m_pData = (BYTE *) &pLoraServerMessage->m_usData;
    SendParams.m_pMessage = pLoraServerMessage;
    SendParams.m_dwMessageId = (DWORD) pLoraServerMessage->m_usMessageId;

    if (IServerConnector_Send(pConnectorDescr->m_pServerConnectorItf, &SendParams) == true)
    {
      // 'ServerConnector' has accepted the send data command (operation executed asynchronously, may
      // fail later)
      #if (LORASERVERMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[INFO] CLoraServerManager_SendServerMessage, Command posted to ServerConnector (executed later)");
      #endif

      pLoraServerMessage->m_usLastConnectorId = usConnectorId;
      return true;
    }
    else
    {
      #if (LORASERVERMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT("[INFO] CLoraServerManager_SendServerMessage, ServerConnector #");
        DEBUG_PRINT_DEC(usConnectorId);
        DEBUG_PRINT_LN(" cannot send data, trying with next connector");
      #endif
    }
  }

  // No 'ServerConnector' can send data
  #if (LORASERVERMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CLoraServerManager_SendServerMessage, no more ServerConnector");
  #endif

  return false;
}


