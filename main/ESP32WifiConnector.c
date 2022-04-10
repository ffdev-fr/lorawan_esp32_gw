/*****************************************************************************************//**
 * @file     ESP32WifiConnector.c
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


/*********************************************************************************************
  Espressif framework includes
*********************************************************************************************/

#include <Common.h>


/*********************************************************************************************
  Includes for object implementation
*********************************************************************************************/

// The CESP32WifiConnector object implements the 'IServerConnector' interface
#define SERVERCONNECTORITF_IMPL

#include "ServerManagerItf.h"
#include "ServerConnectorItf.h"
#include "ESP32WifiConnectorItf.h"

// Object's definitions and methods
#include "ESP32WifiConnector.h"
         
    
/*********************************************************************************************
  Instantiate global static objects used by module implementation
*********************************************************************************************/

// 'IServerConnector' interface function pointers
CServerConnectorItfImplOb g_ServerConnectorItfImplOb = { .m_pAddRef = CESP32WifiConnector_AddRef,
                                                         .m_pReleaseItf = CESP32WifiConnector_ReleaseItf,
                                                         .m_pInitialize = CESP32WifiConnector_Initialize,
                                                         .m_pStart = CESP32WifiConnector_Start,
                                                         .m_pStop = CESP32WifiConnector_Stop,
                                                         .m_pSend = CESP32WifiConnector_Send,
                                                         .m_pSendReceive = CESP32WifiConnector_SendReceive,
                                                         .m_pDownlinkReceived = CESP32WifiConnector_DownlinkReceived
                                                       };


/********************************************************************************************* 

 CLoraServerManager Class

*********************************************************************************************/



/********************************************************************************************* 
  Public methods of CESP32WifiConnector object
 
  These methods are exposed on object's public interfaces
*********************************************************************************************/

/*********************************************************************************************
  Object instance factory
 
  The factory contains one method used to create a new object instance.
  This method provides the 'IServerConnector' interface object for object's use and destruction.
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         IServerConnector CESP32WifiConnector_CreateInstance()
 * 
 * @brief      Creates a new instance of CESP32WifiConnector object.
 * 
 * @details    A new instance of CESP32WifiConnector object is created and its 'IServerConnector'
 *             interface is returned. The owner object uses this interface to configure and 
 *             control associated 'ServerConnector'.
 *
 * @return     A 'IServerConnector' interface object.\n
 *             The reference count for returned 'IServerConnector' interface is set to 1.
 *
 * @note       The CESP32WifiConnector object and associated 'ServerConnector' objects are
 *             created but nothing is initialized (i.e. only object allocations).
 *             The 'IServerConnector_Initialize' method must be called to configure the 
 *             'ServerConnector'.
 *
 * @note       The CESP32WifiConnector object is destroyed when the last reference to 
 *             'IServerConnector' is released (i.e. call to 'IServerConnector_ReleaseItf' 
 *             method).
*********************************************************************************************/
IServerConnector CESP32WifiConnector_CreateInstance()
{
  CESP32WifiConnector * pESP32WifiConnector;
     
  // Create the object
  if ((pESP32WifiConnector = CESP32WifiConnector_New()) != NULL)
  {

    // Create the 'IServerConnector' interface object
    if ((pESP32WifiConnector->m_pServerConnectorItf = 
        IServerConnector_New(pESP32WifiConnector, &g_ServerConnectorItfImplOb)) != NULL)
    {
      ++(pESP32WifiConnector->m_nRefCount);
    }
    return pESP32WifiConnector->m_pServerConnectorItf;
  }

  return NULL;
}

/*********************************************************************************************
  Public methods exposed on 'IServerConnector' interface
 
  The static 'CESP32WifiConnectorItfImplOb' object is initialized with pointers to these functions.
  The static 'CESP32WifiConnectorItfImplOb' object is referenced in the 'IServerConnector'
  interface provided by 'CreateInstance' method (object factory).
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         uint32_t CESP32WifiConnector_AddRef(void *this)
 * 
 * @brief      Increments the object's reference count.
 * 
 * @details    This function increments object's global reference count.\n
 *             The reference count is used to track the number of existing external references
 *             to 'IServerConnector' interface implemented by CESP32WifiConnector object.
 * 
 * @param      this
 *             The pointer to CESP32WifiConnector object.
 *  
 * @return     The value of reference count once incremented.
*********************************************************************************************/
uint32_t CESP32WifiConnector_AddRef(void *this)
{
  return ++((CESP32WifiConnector *)this)->m_nRefCount;
}

/*****************************************************************************************//**
 * @fn         uint32_t CESP32WifiConnector_ReleaseItf(void *this)
 * 
 * @brief      Decrements the object's reference count.
 * 
 * @details    This function decrements object's global reference count and destroy the object
 *             when count reaches 0.\n
 *             The reference count is used to track the number of existing external references
 *             to 'IServerConnector' interface implemented by CESP32WifiConnector object.
 * 
 * @param      this
 *             The pointer to CESP32WifiConnector object.
 *  
 * @return     The value of reference count once decremented.
*********************************************************************************************/
uint32_t CESP32WifiConnector_ReleaseItf(void *this)
{
  // Delete the object if its interface reference count reaches zero
  if (((CESP32WifiConnector *)this)->m_nRefCount == 1)
  {
    // TO DO -> Stop the object master task which will delete the object on exit
    CESP32WifiConnector_Delete((CESP32WifiConnector *)this);
    return 0;
  }
  return --((CESP32WifiConnector *)this)->m_nRefCount;
}

/*****************************************************************************************//**
 * @fn         bool CESP32WifiConnector_Initialize(void *this, void *pParams)
 * 
 * @brief      Initializes the 'ESP32WifiConnector' object.
 * 
 * @details    This function prepares the 'ESP32WifiConnector' for radio transmissions.\n
 *             The default connection parameters are set and the 'ESP32WifiConnector' is waiting
 *             ready in 'StandBy' mode.
 * 
 * @param      this
 *             The pointer to CESP32WifiConnector object.
 *  
 * @param      pParams
 *             The method parameters (see 'ServerConnectorItf.h' for details).
 *
 * @return     The returned value is 'true' if the 'ESP32WifiConnector' is initialized or 
 *             'false' in case of error.
*********************************************************************************************/
bool CESP32WifiConnector_Initialize(void *this, void *pParams)
{
  return CESP32WifiConnector_NotifyAndProcessCommand((CESP32WifiConnector *) this,
                                                     ESP32WIFICONNECTOR_AUTOMATON_CMD_INITIALIZE,
                                                     ESP32WIFICONNECTOR_AUTOMATON_MAX_SYNC_CMD_DURATION, 
                                                     pParams);
}

bool CESP32WifiConnector_Start(void *this, void *pParams)
{
  return CESP32WifiConnector_NotifyAndProcessCommand((CESP32WifiConnector *) this,
                                                     ESP32WIFICONNECTOR_AUTOMATON_CMD_START, 0, pParams);
}

bool CESP32WifiConnector_Stop(void *this, void *pParams)
{
  return CESP32WifiConnector_NotifyAndProcessCommand((CESP32WifiConnector *) this,
                                                     ESP32WIFICONNECTOR_AUTOMATON_CMD_STOP, 0, pParams);
}

bool CESP32WifiConnector_Send(void *this, void *pParams)
{
  return CESP32WifiConnector_NotifyAndProcessCommand((CESP32WifiConnector *) this,
                                                     ESP32WIFICONNECTOR_AUTOMATON_CMD_SEND, 0, pParams);
}

bool CESP32WifiConnector_SendReceive(void *this, void *pParams)
{
  return CESP32WifiConnector_NotifyAndProcessCommand((CESP32WifiConnector *) this,
                                                     ESP32WIFICONNECTOR_AUTOMATON_CMD_SENDRECEIVE, 0, pParams);
}

bool CESP32WifiConnector_DownlinkReceived(void *this, void *pParams)
{
  return CESP32WifiConnector_NotifyAndProcessCommand((CESP32WifiConnector *) this,
                                                     ESP32WIFICONNECTOR_AUTOMATON_CMD_DOWNLINKRECEIVED, 0, pParams);
}

/********************************************************************************************* 
  Private methods of CESP32WifiConnector object
 
  The following methods CANNOT be called by another object
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         bool CESP32WifiConnector_NotifyAndProcessCommand(CESP32WifiConnector *this, 
 *                                                           DWORD dwCommand, void *pCmdParams)
 * 
 * @brief      Process a command issued by a method of 'IESP32WifiConnector' interface.
 * 
 * @details    The CESP32WifiConnector main automaton asynchronously executes the commands generated
 *             by calls on 'IServerConnector' interface methods.\n
 *             This method transmits commands to main automaton and wait for end of execution
 *             (i.e. from client point of view, the interface method is synchronous).
 * 
 * @param      this
 *             The pointer to CESP32WifiConnector object.
 *  
 * @param      dwCommand
 *             The command to execute (typically same name than corresponding method on
 *             'IServerConnector' interface. See 'ESP32WIFICONNECTOR_AUTOMATON_CMD_xxx' in 
 *             ESP32WifiConnector.h.
 *  
 * @param      dwTimeout
 *             Specific timeout for the command. If the value is 0, the standard timeout is
 *             used.
 *
 * @param      pCmdParams
 *             A pointer to command parameters. The object pointed by 'pCmdParams' depends
 *             on method invoked on 'IServerConnector' interface. 
 *             See 'ServerConnectorItf.h'.
 *  
 * @return     The returned value is 'true' if the command is properly executed or 'false'
 *             if command execution has failed or is still pending.
 *
 * @note       This function assumes that commands are quickly processed by main automaton.\n
 *             The maximum execution time can be configured and a mechanism is implemented in
 *             order to ignore client commands sent when a previous command is still pending.
*********************************************************************************************/
bool CESP32WifiConnector_NotifyAndProcessCommand(CESP32WifiConnector *this, DWORD dwCommand, DWORD dwTimeout, void *pCmdParams)
{
  // Automaton commands are serialized (and should be quickly processed)
  // Note: In current design, there is only one client object for a single CESP32WifiConnector
  //       instance (the main task on program startup and for operation control). In other 
  //       words, commands are serialized here and there is no concurrency on the 
  //       'CESP32WifiConnector_NotifyAndProcessCommand' method.
  if (xSemaphoreTake(this->m_hCommandMutex, pdMS_TO_TICKS(ESP32WIFICONNECTOR_AUTOMATON_MAX_CMD_DURATION)) == pdFAIL)
  {
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CESP32WifiConnector_NotifyAndProcessCommand - Failed to take mutex");
    #endif

    return false;
  }

  // Make sure that previous command has been processed by the automaton
  if (this->m_dwCommand != ESP32WIFICONNECTOR_AUTOMATON_CMD_NONE)
  {
    // Previous call to this function has returned before end of command's execution
    // Check if done now
    if (xSemaphoreTake(this->m_hCommandDone, 0) == pdFAIL)
    {
      // Still not terminated
      #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] CESP32WifiConnector_NotifyAndProcessCommand - Previous command still pending");
      #endif

      xSemaphoreGive(this->m_hCommandMutex);
      return false;
    }
  }

  // Post the command to main automaton
  this->m_dwCommand = dwCommand;
  this->m_pCommandParams = pCmdParams;
  CESP32WifiConnector_MessageOb QueueMessage;
  QueueMessage.m_wMessageType = ESP32WIFICONNECTOR_AUTOMATON_MSG_COMMAND;

  DEBUG_PRINT_LN("[DEBUG] CESP32WifiConnector_NotifyAndProcessCommand - Sending command (via ESP32WifiConnector' queue)");

  if (xQueueSend(this->m_hWifiConnectorQueue, &QueueMessage, pdMS_TO_TICKS(ESP32WIFICONNECTOR_AUTOMATON_MAX_CMD_DURATION / 2)) != pdPASS)
  {
    // Message queue is full
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CESP32WifiConnector_NotifyAndProcessCommand - Message queue full");
    #endif

    xSemaphoreGive(this->m_hCommandMutex);
    return false;
  }

  // Wait for command execution by main automaton
  if (dwTimeout == 0)
  {
    dwTimeout = ESP32WIFICONNECTOR_AUTOMATON_MAX_CMD_DURATION - (ESP32WIFICONNECTOR_AUTOMATON_MAX_CMD_DURATION / 5); 
  }
  else
  {
    dwTimeout -= ESP32WIFICONNECTOR_AUTOMATON_MAX_CMD_DURATION / 5;
  }
  BaseType_t nCommandDone = xSemaphoreTake(this->m_hCommandDone, pdMS_TO_TICKS(dwTimeout));

  // If the command has been processed, clear 'm_dwCommand' attribute
  if (nCommandDone == pdPASS)
  {
    this->m_dwCommand = ESP32WIFICONNECTOR_AUTOMATON_CMD_NONE;
  }
  else
  {
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CESP32WifiConnector_NotifyAndProcessCommand - Exiting before end of command execution");
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
  'WifiConnector' task
 
  This RTOS 'Task' is the main automaton of 'CESP32WifiConnector' object:
   - It processes messages sent via 'IServerConnector' interface for comfiguration and 
     operation execution on the ESP32 Wifi device.
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         void CESP32WifiConnector_WifiConnectorAutomaton(CESP32WifiConnector *this)
 * 
 * @brief      Executes the 'WifiConnector' automaton.
 * 
 * @details    This function is the RTOS task implementing the CESP32WIfiConnector main automaton.\n
 *             This automation processes the following messages:\n
 *               - Messages sent via 'IServerConnector' interface for configuration and 
 *                 operation execution on the ESP32 Wifi device.
 *
 * @param      this
 *             The pointer to CESP32WifiConnector object.
 *  
 * @return     The RTOS task terminates when object is deleted (typically on main program
 *             exit).
 *
 * @note       The automaton's main loop waits for messages received through an RTOS queue.
*********************************************************************************************/
void CESP32WifiConnector_WifiConnectorAutomaton(CESP32WifiConnector *this)
{
  CESP32WifiConnector_MessageOb QueueMessage;

  while (this->m_dwCurrentState != ESP32WIFICONNECTOR_AUTOMATON_STATE_TERMINATED)
  {
    if (this->m_dwCurrentState >= ESP32WIFICONNECTOR_AUTOMATON_STATE_CREATED)
    {
      #if (ESP32WIFICONNECTOR_DEBUG_LEVEL2)
        DEBUG_PRINT_LN("[DEBUG] CESP32WifiConnector_WifiConnectorAutomaton, waiting message");
      #endif
  
      // Wait for messages
      if (xQueueReceive(this->m_hWifiConnectorQueue, &QueueMessage, pdMS_TO_TICKS(500)) == pdPASS)
      {
        // Process message
        #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
          DEBUG_PRINT_CR;
          DEBUG_PRINT("[INFO] CESP32WifiConnector_WifiConnectorAutomaton, message received: ");
          DEBUG_PRINT_HEX(QueueMessage.m_wMessageType);
          DEBUG_PRINT_CR;
        #endif
  
        if (QueueMessage.m_wMessageType == ESP32WIFICONNECTOR_AUTOMATON_MSG_COMMAND)
        {
          // Command message (i.e. external message via 'IServerConnector' interface
          // Note: The command is defined by 'm_dwCommand' and 'm_pCommandParams' variables
          //       (i.e. commands are serialized, never more than one command waiting in
          //       automaton's queue) 
          CESP32WifiConnector_ProcessAutomatonNotifyCommand(this);
        }
      }
      else
      {
        // No pending message for a while, maybe something to do in background
        // 
        // TO CHECK: Connectors are event driven with timeout, normally nothing to do!
        #if (ESP32WIFICONNECTOR_DEBUG_LEVEL2)
          DEBUG_PRINT_LN("[DEBUG] CESP32WifiConnector_WifiConnectorAutomaton, idle - TO DO - maybe something in background");
        #endif
      }
    }
    else
    {
      // Parent object not ready, wait for end of object's construction
      #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
        DEBUG_PRINT_CR;
        DEBUG_PRINT("CESP32WifiConnector_WifiConnectorAutomaton, waiting, state: ");
        DEBUG_PRINT_HEX(this->m_dwCurrentState);
        DEBUG_PRINT_CR;
      #endif

      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }

  // Main automaton terminated (typically 'CLoraServerManager' being deleted)
  vTaskDelete(NULL);
  this->m_hWifiConnectorTask = NULL;
}

/********************************************************************************************* 
  'Receive' task
 
  This RTOS 'Task' is used to receive messages sent by Network Server and transmit them to
  'ServerManager' (direct notification using dedicated queue specified in 'IServerManagerItf')

*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         void CESP32WifiConnector_ReceiveAutomaton(CESP32WifiConnector *this)
 * 
 * @brief      Waits for messages sent by Network Server. Received messages are transmited
 *             to 'CLoraServerManager".
 * 
 * @details    This function is the RTOS task used to receive downlink messages sent by the
 *             Network Server.
 *             Received message are transmited to 'CLoraServerManager' by direct notification
 *             to a dedicated task (see 'IServerManagerItf'):
 *              - The object uses a small buffer to allow reception of some messages in case 
 *                of delayed processing by 'CLoraServerManager'
 *              - The 'CLoraServerManager' asks the 'ProtocolEngine' to process message's raw
 *                data in order to build a LoRa packet.
 *              - When message is decoded by 'ProtocolEngine', the 'CLoraServerManager' releases
 *                the message buffer in 'Connector' object (via 'IServerConnector_DownlinkReceived')
 * 
 * @param      this
 *             The pointer to CLoraNodeManager object.
 *  
 * @return     The RTOS task terminates when object is deleted (typically on main program
 *             exit).
*********************************************************************************************/
void CESP32WifiConnector_ReceiveAutomaton(CESP32WifiConnector *this)
{
  socklen_t addrLen = sizeof(this->m_ServerSockAddr); 
  int retCode;
  BYTE *pMessageData;
  CMemoryBlockArrayEntryOb MemBlockEntry;
  CServerConnectorItf_ConnectorEventOb ConnectorEvent;
  CServerConnectorItf_ServerDownlinkMessage pDownlinkMessage;

  ConnectorEvent.m_wConnectorEventType = SERVERCONNECTOR_CONNECTOREVENT_DOWNLINK_RECEIVED;
  pDownlinkMessage = &ConnectorEvent.m_DownlinkMessage;

  while (this->m_dwCurrentState != ESP32WIFICONNECTOR_AUTOMATON_STATE_TERMINATED)
  {
    if (this->m_dwCurrentState == ESP32WIFICONNECTOR_AUTOMATON_STATE_RUNNING)
    {
      #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[INFO] CESP32WifiConnector_ReceiveAutomaton, receiving message");
      #endif

      // Step 1 - Obtain a 'MemoryBlock' to store received message data
      if ((pMessageData = CMemoryBlockArray_GetBlock(this->m_pServerMessageArray, &MemBlockEntry)) == NULL)
      {
        // Should never occur. Buffer for messages exhausted
        // Note: No recovery mechanism = for stress test in current version
        #if (LORASERVERMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[ERROR] Message receive buffer exhausted. Entering 'ERROR' state");
          this->m_dwCurrentState = ESP32WIFICONNECTOR_AUTOMATON_STATE_ERROR;
        #endif

        continue;
      }

      // Step 2 - Receive data
      //
      // Note: Should use a non blocking mechanism (TO DO, check if possible)
      while (1)
      {
        #if (ESP32WIFICONNECTOR_DEBUG_LEVEL2)
          DEBUG_PRINT_LN("[DEBUG] CESP32WifiConnector_ReceiveAutomaton, calling recvfrom (blocking)");
        #endif

        retCode = recvfrom(this->m_hServerSocket, pMessageData, ESP32WIFICONNECTOR_MAX_MESSAGELENGTH, 0, 
                           (struct sockaddr *) &this->m_ServerSockAddr, &addrLen);
  
        #if (ESP32WIFICONNECTOR_DEBUG_LEVEL2)
          DEBUG_PRINT_LN("[DEBUG] 'CESP32WifiConnector_ReceiveAutomaton' - After recvfrom, ticks: ");
          DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
          DEBUG_PRINT_CR;
        #endif

        #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
          DEBUG_PRINT("[INFO] 'CESP32WifiConnector_ReceiveAutomaton' - Return from recvfrom, code(or length) = ");
          DEBUG_PRINT_DEC(retCode);
          DEBUG_PRINT_CR;
        #endif
  
        if (retCode < 0)
        {
          // No data received, wait until end of timeout if 'EWOULDBLOCK'
          if (retCode == EWOULDBLOCK)
          {
            #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
              DEBUG_PRINT("[INFO] 'CESP32WifiConnector_ReceiveAutomaton' - EWOULDBLOCK, still waiting");
            #endif
          }
          else
          {
            // Receive error
            #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
              DEBUG_PRINT_LN("[ERROR] 'CESP32WifiConnector_ReceiveAutomaton' - Unable to receive message, ignored, trying again");
            #endif
  
            break;
          }
        }
        else
        {
          // Data received from the NetworkServer, transmit it to 'ServerManager'
          pDownlinkMessage->m_pConnectorItf = this->m_pServerConnectorItf;
          pDownlinkMessage->m_dwMessageId = (DWORD) MemBlockEntry.m_usBlockIndex;
          pDownlinkMessage->m_dwTimestamp = xTaskGetTickCount() * portTICK_RATE_MS; 
          pDownlinkMessage->m_pData = pMessageData;
          pDownlinkMessage->m_wDataSize = (WORD) retCode;

          if (xQueueSend(this->m_hServerManagerNotifyQueue, &ConnectorEvent, 0) != pdPASS)
          {
            // Queue should be long enough to always accept messages received by 'Connector'
            // If no room available for notification, the message is discarded (and lost)
            // TO DO: possible to delay the notification and allow reception of next message
            #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
              DEBUG_PRINT_LN("[ERROR] CESP32WifiConnector_ReceiveAutomaton - ServerManager notification queue full, message lost!");
            #endif

            CMemoryBlockArray_ReleaseBlock(this->m_pServerMessageArray, MemBlockEntry.m_usBlockIndex);
          }

          // Receive next message
          break;
        }
      }
    }
    else
    {
      // Parent object not ready, wait for end of object's initialization
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }

  // Main automaton terminated (typically 'CESP32WifiConnector' being deleted)
  vTaskDelete(NULL);
  this->m_hReceiveTask = NULL;
}



/*********************************************************************************************
  Construction

  Protected methods : must be called only object factory and 'IServerConnector' interface
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         CESP32WifiConnector * CESP32WifiConnector_New()
 * 
 * @brief      Object construction.
 * 
 * @details    
 * 
 * @return     The function returns the pointer to the CESP32WifiConnector instance.
 *
 * @note       This function only creates the object and its dependencies (RTOS objects).\n
*********************************************************************************************/
CESP32WifiConnector * CESP32WifiConnector_New()
{
  CESP32WifiConnector *this;

#if ESP32WIFICONNECTOR_DEBUG_LEVEL2
  printf("CESP32WifiConnector_New -> Debug level 2 (DEBUG)\n");
#elif ESP32WIFICONNECTOR_DEBUG_LEVEL1
  printf("CESP32WifiConnector_New -> Debug level 1 (INFO)\n");
#elif ESP32WIFICONNECTOR_DEBUG_LEVEL0
  printf("CESP32WifiConnector_New -> Debug level 0 (NORMAL)\n");
#endif 

  if ((this = (void *) pvPortMalloc(sizeof(CESP32WifiConnector))) != NULL)
  {
    // The 'CESP32WifiConnector' is under construction (i.e. embedded tasks must wait the 'INITIALIZED' state
    this->m_dwCurrentState = ESP32WIFICONNECTOR_AUTOMATON_STATE_CREATING;
    this->m_dwConnectionState = ESP32WIFICONNECTOR_CONNECTION_STATE_DISCONNECTED;

    // Embedded objects are not defined (i.e. created below)
    this->m_pServerMessageArray = this->m_hCommandMutex = this->m_hCommandDone = this->m_hWifiConnectorTask = 
      this->m_hWifiConnectorQueue = this->m_hWifiEventGroup = this->m_hConnectionStateMutex = 
      this->m_hReceiveTask = NULL;


    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CESP32WifiConnector_New Entering: create object 1");
    #endif

    if ((this->m_pServerMessageArray = CMemoryBlockArray_New(ESP32WIFICONNECTOR_MAX_MESSAGELENGTH,
        ESP32WIFICONNECTOR_MAX_SERVERMESSAGES)) == NULL)
    {
      CESP32WifiConnector_Delete(this);
      return NULL;
    }

    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CESP32WifiConnector_New Entering: create object 2");
    #endif

    // Create WifiConnector automaton task
    if (xTaskCreate((TaskFunction_t) CESP32WifiConnector_WifiConnectorAutomaton, "CESP32WifiConnector_WifiConnectorAutomaton", 
        2048, this, 5, &(this->m_hWifiConnectorTask)) == pdFAIL)
    {
      CESP32WifiConnector_Delete(this);
      return NULL;
    }

    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CESP32WifiConnector_New Entering: create object 3");
    #endif

    if ((this->m_hCommandMutex = xSemaphoreCreateMutex()) == NULL)
    {
      CESP32WifiConnector_Delete(this);
      return NULL;
    }

    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CESP32WifiConnector_New Entering: create object 4");
    #endif

    if ((this->m_hCommandDone = xSemaphoreCreateBinary()) == NULL)
    {
      CESP32WifiConnector_Delete(this);
      return NULL;
    }

    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CESP32WifiConnector_New Entering: create object 5");
    #endif

    // Message queue associated to 'WifiConnector' task
    // Used for internal messages and external commands via 'IServerConnector' interface
    if ((this->m_hWifiConnectorQueue = xQueueCreate(10, sizeof(CESP32WifiConnector_MessageOb))) == NULL)
    {
      CESP32WifiConnector_Delete(this);
      return NULL;
    }

    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CESP32WifiConnector_New Entering: create object 6");
    #endif

    // EventGroup for Wifi Event Handler function
    if ((this->m_hWifiEventGroup = xEventGroupCreate()) == NULL)
    {
      CESP32WifiConnector_Delete(this);
      return NULL;
    }

    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CESP32WifiConnector_New Entering: create object 7");
    #endif

    if ((this->m_hConnectionStateMutex = xSemaphoreCreateMutex()) == NULL)
    {
      CESP32WifiConnector_Delete(this);
      return NULL;
    }

    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CESP32WifiConnector_New Entering: create object 8");
    #endif
    
    // Create the task used to receive downlink messages
    if (xTaskCreate((TaskFunction_t) CESP32WifiConnector_ReceiveAutomaton, "CESP32WifiConnector_ReceiveAutomaton", 
        2048, this, 5, &(this->m_hReceiveTask)) == pdFAIL)
    {
      CESP32WifiConnector_Delete(this);
      return NULL;
    }

    // The owner object is not defined (i.e. done on 'Initialize' command)
    this->m_hServerManagerNotifyQueue = NULL;

    // Initialize object's properties
    this->m_nRefCount = 0;
    this->m_dwCommand = ESP32WIFICONNECTOR_AUTOMATON_CMD_NONE;
    this->m_hServerSocket = -1;

    // Enter the 'CREATED' state
    this->m_dwCurrentState = ESP32WIFICONNECTOR_AUTOMATON_STATE_CREATED;

  }
  return this;
}

/*****************************************************************************************//**
 * @fn         void CESP32WifiConnector_Delete(CESP32WifiConnector *this)
 * 
 * @brief      Object destruction.
 * 
 * @details    Destroys the CESP32WifiConnector object.\n
 *             The associated RTOS objects are destroyed and the memory used by CESP32WifiConnector
 *             object are released.

 * @param      this
 *             The pointer to CESP32WifiConnector object.
 *  
 * @return     None.
*********************************************************************************************/
void CESP32WifiConnector_Delete(CESP32WifiConnector *this)
{
  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL2)
    DEBUG_PRINT_LN("[DEBUG] CESP32WifiConnector_Delete Entering");
  #endif

  // Ask all tasks for termination
  // TO DO (also check how to delete task object)

  // Delete queues (TO DO)

  // Free memory
  if (this->m_pServerMessageArray != NULL)
  {
    CMemoryBlockArray_Delete(this->m_pServerMessageArray);
  }

  if (this->m_hCommandMutex != NULL)
  {
    vSemaphoreDelete(this->m_hCommandMutex);
  }
  if (this->m_hCommandDone != NULL)
  {
    vSemaphoreDelete(this->m_hCommandDone);
  }

  if (this->m_hWifiEventGroup != NULL)
  {   
    vEventGroupDelete(this->m_hWifiEventGroup);
  }
  if (this->m_hConnectionStateMutex != NULL)
  {
    vSemaphoreDelete(this->m_hConnectionStateMutex);
  }
  
  vPortFree(this);
}


/*********************************************************************************************
  Private methods (implementation)

  Command processing

  These functions are called by 'WifiConnector' automaton when the 
  'ESP32WIFICONNECTOR_AUTOMATON_MSG_COMMAND' notification is received.

  Note: These functions may change automaton state. There is no protection against concurrency
        for automaton state because only functions called from automaton RTOS task are allowed
        to modify state (by design).
*********************************************************************************************/



/*****************************************************************************************//**
 * @fn         bool CESP32WifiConnector_ProcessAutomatonNotifyCommand(CESP32WifiConnector *this)
 * 
 * @brief      Process 'IESP32WifiConnector' command currently waiting in automaton.
 * 
 * @details    This function is invoked by 'WifiConnector' automaton when it receives a 
 *             'COMMAND' notification.\n
 *             These commands are issued via calls on 'IServerConnector' interface.\n
 *             The command and its parameters are available in member variables of 
 *             CESP32WifiConnector object.\n
 *             By design, commands are serialized when transmited to CESP32WifiConnector automaton.
 *             In other words, the processing of one command cannot be interrupted by the 
 *             reception of another command.
 * 
 * @param      this
 *             The pointer to CESP32WifiConnector object.
 *  
 * @return     The returned value is 'true' if command execution is successful or 'false' in
 *             case of error.
*********************************************************************************************/
bool CESP32WifiConnector_ProcessAutomatonNotifyCommand(CESP32WifiConnector *this)
{
  bool bResult;

  switch (this->m_dwCommand)
  {
    case ESP32WIFICONNECTOR_AUTOMATON_CMD_INITIALIZE:
      bResult = CESP32WifiConnector_ProcessInitialize(this, (CESP32WifiConnectorItf_InitializeParams) this->m_pCommandParams);
      break;

//  case ESP32WIFICONNECTOR_AUTOMATON_CMD_ATTACH:
//    bResult = CESP32WifiConnector_ProcessAttach(this, (CESP32WifiConnectorItf_AttachParams) this->m_pCommandParams);
//    break;

    case ESP32WIFICONNECTOR_AUTOMATON_CMD_START:
      bResult = CESP32WifiConnector_ProcessStart(this, (CESP32WifiConnectorItf_StartParams) this->m_pCommandParams);
      break;

    case ESP32WIFICONNECTOR_AUTOMATON_CMD_STOP:
      bResult = CESP32WifiConnector_ProcessStop(this, (CESP32WifiConnectorItf_StopParams) this->m_pCommandParams);
      break;

    case ESP32WIFICONNECTOR_AUTOMATON_CMD_SEND:
      bResult = CESP32WifiConnector_ProcessSend(this, (CESP32WifiConnectorItf_SendParams) this->m_pCommandParams);
      break;

    case ESP32WIFICONNECTOR_AUTOMATON_CMD_SENDRECEIVE:
      bResult = CESP32WifiConnector_ProcessSendReceive(this, (CESP32WifiConnectorItf_SendReceiveParams) this->m_pCommandParams);
      break;

    case ESP32WIFICONNECTOR_AUTOMATON_CMD_DOWNLINKRECEIVED:
      bResult = CESP32WifiConnector_ProcessDownlinkReceived(this, (CESP32WifiConnectorItf_DownlinkReceivedParams) this->m_pCommandParams);
      break;

    default:
      #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] CESP32WifiConnector_ProcessAutomatonNotifyCommand, unknown command");
      #endif
      bResult = false;
      break;
  }

  // Command executed, allow next command for calling task (i.e. command execution by automaton is asynchronous
  // but has synchronous behavior for calling task)
  this->m_dwCommand = ESP32WIFICONNECTOR_AUTOMATON_CMD_NONE;
  xSemaphoreGive(this->m_hCommandDone);

  return bResult;
}

/*****************************************************************************************//**
 * @fn         bool CESP32WifiConnector_ProcessInitialize(CESP32WifiConnector *this, 
 *                                            CESP32WifiConnectorItf_InitializeParams pParams)
 * 
 * @brief      Initializes the object and configure associated 'ServerConnectors'.
 * 
 * @details    This function reads the connector configuration and initializes the 
 *             'ServerConnector' (i.e. connects the Wifi network and creates UDP socket to
 *             the LoRa Network Server).\n
 *             The connection device is connected and is waiting in 'Idle' state. 
 *             The 'START' and 'STOP' commands are then available to control the activity of
 *             the object (i.e. processing of 'Send' and 'Receive' operations).\n
 *             This function must be called one time in 'CREATED' automaton state.\n
 *             On exit, possible automaton states are:\n
 *              - 'ESP32WIFICONNECTOR_AUTOMATON_STATE_INITIALIZED' = the object and connection 
 *                devices initialization is successfully completed. The Wifi connector is ready
 *                for send and receive operations with Network Server.
 *              - 'ESP32WIFICONNECTOR_AUTOMATON_STATE_ERROR' = failed to initialize Wifi connection
 *                to LoRa Network Server.
 *
 * @note       By design, the 'Initialize' method of 'ServerConnector' interface must be called 
 *             once. In case of failure, the 'CESP32WifiConnector' enter the 'TERMINATED' state 
 *             (i.e. fatal error and no possible recovery until gateway reboot).
 *             This simplified behavior is used because the failover feature for Network Server
 *             link is not required for first version of the gateway. In current version, the
 *             gateway uses the first successful link detected on boot and reboots if the link
 *             cannot be established back after failure (typically to use backup connector).
 * 
 * @param      this
 *             The pointer to CESP32WifiConnector object.
 *  
 * @param      pParams
 *             The interface method parameters (see 'ESP32WifiConnectorItf.h' for details).
 *
 * @return     The returned value is 'true' if the ESP32WifiConnector is properly initialized or
 *             'false' in case of error.
*********************************************************************************************/
bool CESP32WifiConnector_ProcessInitialize(CESP32WifiConnector *this, CESP32WifiConnectorItf_InitializeParams pParams)
{
  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CESP32WifiConnector_ProcessInitialize'");
  #endif

  // The 'Initialize' method is allowed only in 'CREATED' and 'ERROR' automaton state
  if ((this->m_dwCurrentState != ESP32WIFICONNECTOR_AUTOMATON_STATE_CREATED) &&
      (this->m_dwCurrentState != ESP32WIFICONNECTOR_AUTOMATON_STATE_ERROR))
  {
    // By design, should never occur
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  CServerManagerItf_ConnectorSettings pConnectorSettings = pParams->m_pConnectorSettings;

  // Interface for notifications to owner ServerManager object (direct notification using dedicated queue)
  this->m_hServerManagerNotifyQueue = pParams->m_hEventNotifyQueue;

  // DEBUG
  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT("[INFO] 'CESP32WifiConnector_ProcessInitialize' - Event group before config copy:");
    DEBUG_PRINT_HEX(this->m_hWifiEventGroup);
    DEBUG_PRINT_CR;
  #endif


  // Step 1: Store configuration for access to Wifi Network
  strcpy(this->m_szWifiSSID, pConnectorSettings->m_szNetworkName);
  strcpy(this->m_szWifiPassword, pConnectorSettings->m_szNetworkPassword);
  this->m_dwWifiJoinTimeoutMillisec = pConnectorSettings->m_dwNetworkJoinTimeout;

  // DEBUG
  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT("[INFO] 'CESP32WifiConnector_ProcessInitialize' - Event group after config copy:");
    DEBUG_PRINT_HEX(this->m_hWifiEventGroup);
    DEBUG_PRINT_CR;
  #endif

  // Step 2: Initialize ESP-IDF Wifi module for 'Station' mode
  //
  nvs_flash_init();            // Required for Wifi firmware

  // Gateway MAC Address is explicitly set (i.e. checked in UDP packet by Network Server) 
  esp_base_mac_addr_set(pParams->m_pConnectorSettings->m_GatewayMACAddr);

  tcpip_adapter_init();

  wifi_init_config_t InitConfig = WIFI_INIT_CONFIG_DEFAULT();
  bool bError = false;

  if (esp_event_loop_init(CESP32WifiConnector_WifiEventHandler, this) != ESP_OK) { bError = true; }
  else if (esp_wifi_init(&InitConfig) != ESP_OK) { bError = true; }
  else if (esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK) { bError = true; }
  else if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) { bError = true; }
  else if (esp_wifi_start() != ESP_OK) { bError = true; }

  if (bError == true)
  {
    // Fatal error, should never occur
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CESP32WifiConnector_ProcessInitialize, unable to initialize ESP Wifi layer");
    #endif
    this->m_dwCurrentState = ESP32WIFICONNECTOR_AUTOMATON_STATE_TERMINATED;
    return false;
  }

  // Step 3: Connect the Wifi station to network
  //
  // This step blocks until connection is established (or refused)
  if (CESP32WifiConnector_JoinWifi(this, false) == false)
  {
    // Access Point not reachable, this connector cannot be used by 'ServerManager' at the moment
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CESP32WifiConnector_ProcessInitialize, unable to join Wifi Access Point");
    #endif
    this->m_dwCurrentState = ESP32WIFICONNECTOR_AUTOMATON_STATE_TERMINATED;
    return false;
  }

  // Step 4: Update RTC using SNTP server if required
  //
  // This step blocks until RTC current time is updated (or failed)
  if (pConnectorSettings->m_dwSNTPServerPeriodSec != 0)
  {
    if (CESP32WifiConnector_ConnectSNTPServer(this, pConnectorSettings->m_szSNTPServerUrl, pConnectorSettings->m_dwSNTPServerPeriodSec) == false)
    {
      // SNTP Server not reachable, this connector cannot be used by 'ServerManager' at the momment
      #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] CESP32WifiConnector_ProcessInitialize, unable to update RTC using SNTP Server");
      #endif
      this->m_dwCurrentState = ESP32WIFICONNECTOR_AUTOMATON_STATE_TERMINATED;
      return false;
    }
  }

  // Step 5: Store configuration for access to Network Server
  strcpy(this->m_szNetworkServerUrl, pConnectorSettings->m_szNetworkServerUrl);
  this->m_dwNetworkServerPort = pConnectorSettings->m_dwNetworkServerPort;
  this->m_dwNetworkServerTimeoutMillisec = pConnectorSettings->m_dwNetworkServerTimeout;

  // Step 6: Check access to Network Server (retrieve NetworkServer IP and create UDP socket)
  //
  // This step blocks until DNS has retrieved IP (or failed)
  if (CESP32WifiConnector_BindNetworkServer(this) == false)
  {
    // Unable to retrieve NetworkServer IP, this connector cannot be used by 'ServerManager' at the momment
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CESP32WifiConnector_ProcessInitialize, unable retrieve Network Server IP");
    #endif
    this->m_dwCurrentState = ESP32WIFICONNECTOR_AUTOMATON_STATE_TERMINATED;
    return false;
  }



/* TO CHECK / FINISH

  // Step 5: Attach the 'ServerManager'
  //
  // The 'ServerManager' will directly notify the 'UplinkSenderAutomaton' task of 'ESP32WifiConnector'
  // when a new Lora Packet is received (to forward to Network Server)
  CTransceiverManagerItf_AttachParamsOb AttachParams;
  AttachParams.m_hPacketForwarderTask = this->m_hNodeManagerTask;

  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[DEBUG] CESP32WifiConnector_ProcessInitialize, calling ITransceiverManager_Attach");
  #endif

  ITransceiverManager_Attach((ITransceiverManager) pParams->pTransceiverManagerItf, &AttachParams);
  this->m_pTransceiverManagerItf = (ITransceiverManager) pParams->pTransceiverManagerItf;

  // Adjust state of 'ESP32WifiConnector':
  //  - Enter the 'IDLE' state if the 'TransceiverManager' is already attached
  //  - Otherwise enter the 'INITIALIZED' state (i.e. the 'IDLE' state will be entered when 
  //    'TransceiverManager' will invoke the 'Attach' method)
  // Note: By design, no concurrency on automaton state variable
  if (this->m_hTransceiverManagerTask == NULL)
  {
    this->m_dwCurrentState = ESP32WIFICONNECTOR_AUTOMATON_STATE_INITIALIZED;
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] CESP32WifiConnector automaton state changed: 'INITIALIZED'");
    #endif
  }
  else
  {
    this->m_dwCurrentState = ESP32WIFICONNECTOR_AUTOMATON_STATE_IDLE;
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] CESP32WifiConnector automaton state changed: 'IDLE'");
    #endif
  }
      
*/


  // The 'ESP32WifiConnector' is initialized and ready to send/receive UDP packets to/from Network Server
  this->m_dwCurrentState = ESP32WIFICONNECTOR_AUTOMATON_STATE_INITIALIZED;

  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CESP32WifiConnector initialized for Wifi Network access (INITIALIZED state)");
  #endif
  return true;
}

/*

bool CESP32WifiConnector_ProcessAttach(CESP32WifiConnector *this, CESP32WifiConnectorItf_AttachParams pParams)
{
  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CESP32WifiConnector_ProcessAttach'");
  #endif

  // The 'Attach' method is allowed only in 'CREATED' and 'INITIALIZED' automaton state
  if ((this->m_dwCurrentState != ESP32WIFICONNECTOR_AUTOMATON_STATE_CREATED) &&
      (this->m_dwCurrentState != ESP32WIFICONNECTOR_AUTOMATON_STATE_INITIALIZED))
  {
    // By design, should never occur
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  // Check that no 'TransceiverManager' already attached
  // Command executed once at the end of startup process
  if (this->m_hTransceiverManagerTask != NULL)
  {
    // By design, should never occur
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Node transceiver already attached");
    #endif
    return false;
  }

  // Note: Task in 'CLoraNodeManager' object
  this->m_hTransceiverManagerTask = pParams->m_hNodeManagerTask;

  // Enter the 'IDLE' state if current state is 'INITIALIZED' 
  // Note: By design, no concurrency on automaton state variable
  if (this->m_dwCurrentState == ESP32WIFICONNECTOR_AUTOMATON_STATE_INITIALIZED)
  {
    this->m_dwCurrentState = ESP32WIFICONNECTOR_AUTOMATON_STATE_IDLE;
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] CESP32WifiConnector automaton state changed: 'IDLE'");
    #endif
  }

  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CESP32WifiConnector successfully attached to forwarder");
  #endif
  return true;
}

*/

bool CESP32WifiConnector_ProcessStart(CESP32WifiConnector *this, CESP32WifiConnectorItf_StartParams pParams)
{
//CLoraServerItf_ReceiveParamsOb ReceiveParams;

  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CESP32WifiConnector_ProcessStart'");
  #endif

  // The 'Start' method is allowed only in 'IDLE' automaton state
  if (this->m_dwCurrentState != ESP32WIFICONNECTOR_AUTOMATON_STATE_IDLE)
  {
    // By design, should never occur
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  // Enter the 'RUNNING' state
  // Note: By design, no concurrency on automaton state variable
  this->m_dwCurrentState = ESP32WIFICONNECTOR_AUTOMATON_STATE_RUNNING;
  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CESP32WifiConnector automaton state changed: 'RUNNING'");
  #endif


  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CESP32WifiConnector successfully started (ready to create sessions)");
  #endif
  return true;
}

bool CESP32WifiConnector_ProcessStop(CESP32WifiConnector *this, CESP32WifiConnectorItf_StopParams pParams)
{
  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CESP32WifiConnector_ProcessStop'");
  #endif

  // The 'Stop' method is allowed only in 'RUNNING' automaton state
  if (this->m_dwCurrentState != ESP32WIFICONNECTOR_AUTOMATON_STATE_RUNNING)
  {
    // By design, should never occur
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  // Enter the 'STOPPING' state
  // Note: By design, no concurrency on automaton state variable
  this->m_dwCurrentState = ESP32WIFICONNECTOR_AUTOMATON_STATE_STOPPING;
  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CESP32WifiConnector automaton state changed: 'STOPPING'");
  #endif

  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CESP32WifiConnector currently stopping (no more sessions created)");
  #endif
  return true;
}

bool CESP32WifiConnector_ProcessSend(CESP32WifiConnector *this, CESP32WifiConnectorItf_SendParams pParams)
{
  bool bResult = false;

  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering 'CESP32WifiConnector_ProcessSend'");
  #endif

  // The 'Send' method is allowed only in 'RUNNING' automaton state
  if (this->m_dwCurrentState == ESP32WIFICONNECTOR_AUTOMATON_STATE_RUNNING)
  {
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT("[INFO] CESP32WifiConnector - trying to send message (sendto: ");
      DEBUG_PRINT_DEC((DWORD)((CServerConnectorItf_SendParams) pParams)->m_wDataLength);
      DEBUG_PRINT_LN(" bytes)");
    #endif

    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL2)
      DEBUG_PRINT("[DEBUG] CESP32WifiConnector_ProcessSend - Before sendto, ticks: ");
      DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
      DEBUG_PRINT_CR;
    #endif

    int nBytesSent = sendto(this->m_hServerSocket,
                            ((CServerConnectorItf_SendParams) pParams)->m_pData,
                            ((CServerConnectorItf_SendParams) pParams)->m_wDataLength, 0, 
                            (struct sockaddr *) &this->m_ServerSockAddr, sizeof(this->m_ServerSockAddr));
    if (nBytesSent != ((CServerConnectorItf_SendParams) pParams)->m_wDataLength)
    {
      #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
        DEBUG_PRINT("[ERROR] 'CESP32WifiConnector_ProcessSend' - Unable to sent message, 'sendto' failed (code: ");
        DEBUG_PRINT_DEC(nBytesSent);
        DEBUG_PRINT_CR;
      #endif
    }
    else
    {
      #if (ESP32WIFICONNECTOR_DEBUG_LEVEL2)
        DEBUG_PRINT("[DEBUG] CESP32WifiConnector_ProcessSend - After sendto, ticks: ");
        DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
        DEBUG_PRINT_CR;
      #endif

      bResult = true;

      #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[INFO] CESP32WifiConnector_ProcessSend - message sent");
      #endif
    }
  }
  else
  {
    // By design, should never occur
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CESP32WifiConnector_ProcessSend - Function called in invalid automaton state, message not sent");
    #endif
  }

  // Notify result via 'IServerManager' interface (i.e. asynchronous commnad processing)
  CServerConnectorItf_ConnectorEventOb ConnectorEvent;
  ConnectorEvent.m_wConnectorEventType = SERVERCONNECTOR_CONNECTOREVENT_SERVERMSG_EVENT;
  CServerManagerItf_ServerMessageEvent pServerMessageEvent = &ConnectorEvent.m_ServerMessageEvent;

  pServerMessageEvent->m_wEventType = bResult == true ? SERVERMANAGER_MESSAGEEVENT_UPLINK_SENT: 
                                                        SERVERMANAGER_MESSAGEEVENT_UPLINK_SEND_FAILED;
  pServerMessageEvent->m_pMessage = ((CServerConnectorItf_SendParams) pParams)->m_pMessage;
  pServerMessageEvent->m_dwParam = NULL;
//ServerMessageEvent.m_dwMessageId = ((CServerConnectorItf_SendParams) pParams)->m_dwMessageId;

  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering 'CESP32WifiConnector_ProcessSend' - Notifying ServerManager for send result");
  #endif

/*
  IServerManager *pDebug = ((CESP32WifiConnector *) this)->m_pServerManagerItf;

  DEBUG_PRINT("[INFO] Entering 'CESP32WifiConnector_ProcessSend' - Debug itf = ");
  DEBUG_PRINT_HEX(pDebug);
  DEBUG_PRINT_CR;
*/

//IServerManager_ServerMessageEvent(((CESP32WifiConnector *) this)->m_pServerManagerItf, &ServerMessageEvent);

  if (xQueueSend(this->m_hServerManagerNotifyQueue, &ConnectorEvent, 0) != pdPASS)
  {
    // Queue should be long enough to always accept notifications sent by 'Connector'
    // If no room available for notification, the ServerManager automaton should abort the associated session
    // (typically on timeout)
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CESP32WifiConnector_ProcessSend - ServerManager notification queue full, session may fail");
    #endif
  }

  return bResult;
}

// This method is called after 'Connector' initialization to check the access to Network Server.
// If this first exchange with the Network Server is successful, the 'Connector' enters the 'IDLE' state.
bool CESP32WifiConnector_ProcessSendReceive(CESP32WifiConnector *this, CESP32WifiConnectorItf_SendReceiveParams pParams)
{
  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CESP32WifiConnector_ProcessSendReceive'");
  #endif

  // The 'SendReceive' method is allowed only when automaton state is 'INITIALIZED' and connection state is 'SERVER_DISCONNECTED'
  // NOTE: The owner object calls this function to establish session with Network Server prior starting the 'Connector' object
  //       (i.e. method called once during object's life)
  if ((this->m_dwCurrentState != ESP32WIFICONNECTOR_AUTOMATON_STATE_INITIALIZED) ||
      (this->m_dwConnectionState != ESP32WIFICONNECTOR_CONNECTION_STATE_SERVER_DISCONNECTED))
  {
    // By design, should never occur
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }




  // /////// DEBUG
//int rc = connect(this->m_hServerSocket, (struct sockaddr *) &this->m_ServerSockAddr, sizeof(this->m_ServerSockAddr));
//DEBUG_PRINT("[INFO] 'CESP32WifiConnector_ProcessSendReceive' - Connect, rc: ");
//DEBUG_PRINT_DEC(rc);
//DEBUG_PRINT_CR;
//
//rc = send(this->m_hServerSocket, pParams->m_pData, pParams->m_wDataLength, 0);
//DEBUG_PRINT("[INFO] 'CESP32WifiConnector_ProcessSendReceive' - Send, rc: ");
//DEBUG_PRINT_DEC(rc);
//DEBUG_PRINT_CR;






///*  RESTORE


  // Step 1: Send the 'ping' message to initialize Network Server session
  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] 'CESP32WifiConnector_ProcessSendReceive' - Sending PING message...");
  #endif

// Debug - sendto X99 (wireshark capture)
//  this->m_ServerSockAddr.sin_addr.s_addr = ipaddr_addr("192.168.1.15");

  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL2)
    DEBUG_PRINT_LN("[DEBUG] 'CESP32WifiConnector_ProcessSendReceive' - Before sendto, ticks: ");
    DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
    DEBUG_PRINT_CR;
  #endif

  int nBytesSent = sendto(this->m_hServerSocket, pParams->m_pData, pParams->m_wDataLength, 0,
                          (struct sockaddr *) &this->m_ServerSockAddr, sizeof(this->m_ServerSockAddr));
  if (nBytesSent != pParams->m_wDataLength)
  {
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] 'CESP32WifiConnector_ProcessSendReceive' - Unable to sent PING message, connector disabled");
    #endif

    close(this->m_hServerSocket);
    this->m_dwCurrentState = ESP32WIFICONNECTOR_AUTOMATON_STATE_TERMINATED;
    return false;
  }


//*/




  // TO DO -> Possible only if not using the CMD Main automaton (i.e. currently blocked in synchronous execution of THIS Command!)

  // Message sent, notify the 'ServerManager' (i.e. transmited to 'ProtocolEngine' if required) 
//CServerManagerItf_ServerMessageEventOb ServerMessageEvent;
//
//ServerMessageEvent.m_wEventType = SERVERMANAGER_MESSAGEEVENT_UPLINK_SENT;
//ServerMessageEvent.m_pMessage = pSendParams->m_pMessage;
//ServerMessageEvent.m_dwMessageId = pSendParams->m_dwMessageId;
//
//#if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
//  DEBUG_PRINT_CR;
//  DEBUG_PRINT_LN("[INFO] Entering 'CESP32WifiConnector_ProcessSendReceive' - Notifying ServerManager for message sent");
//#endif
//
//
//IServerManager_ServerMessageEvent(((CESP32WifiConnector *) this)->m_pServerManagerItf, &ServerMessageEvent);


  // Step 2: Wait for NetworkServer reply
  socklen_t addrLen = sizeof(this->m_ServerSockAddr); 
  int retCode;
  while (1)
  {
    retCode = recvfrom(this->m_hServerSocket, pParams->m_pReply, pParams->m_wReplyMaxLength, 0, 
                       (struct sockaddr *) &this->m_ServerSockAddr, &addrLen);

    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] 'CESP32WifiConnector_ProcessSendReceive' - After recvfrom, ticks: ");
      DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
      DEBUG_PRINT_CR;
    #endif

    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT("[INFO] 'CESP32WifiConnector_ProcessSendReceive' - Return from recvfrom, code(or length) = ");
      DEBUG_PRINT_DEC(retCode);
      DEBUG_PRINT_CR;
    #endif

    if (retCode < 0)
    {
      // No data received, wait until end of timeout if 'EWOULDBLOCK'
      if (retCode == EWOULDBLOCK)
      {
        #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
          DEBUG_PRINT("[INFO] 'CESP32WifiConnector_ProcessSendReceive' - EWOULDBLOCK, still waiting");
        #endif
      }
      else
      {
        // Receive error
        #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[ERROR] 'CESP32WifiConnector_ProcessSendReceive' - Unable to receive PING reply, connector disabled");
        #endif

        close(this->m_hServerSocket);
        this->m_dwCurrentState = ESP32WIFICONNECTOR_AUTOMATON_STATE_TERMINATED;
        return false;
      }
    }
    else
    {
      // Data received from the NetworkServer. It is assumed that is the reply we are waiting for 
      // (i.e. not checked here but by associated ProtocolEngine)
      pParams->m_wReplyLength = retCode;

      // The 'ESP32WifiConnector' has validated the access to Network Server
      this->m_dwCurrentState = ESP32WIFICONNECTOR_AUTOMATON_STATE_IDLE;

      #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[INFO] 'CESP32WifiConnector_ProcessSendReceive' - First send/receive with Network Server successful (IDLE state)");
      #endif
      break;
    }
  }
  return true;
}

// The downlink message data has been processed, release the MemoryBlock to allows later use in receiving another message
bool CESP32WifiConnector_ProcessDownlinkReceived(CESP32WifiConnector *this, CESP32WifiConnectorItf_DownlinkReceivedParams pParams)
{
  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CESP32WifiConnector_ProcessDownlinkReceived'");
  #endif

  CMemoryBlockArray_ReleaseBlock(this->m_pServerMessageArray, (WORD) pParams->m_dwMessageId);

  return true;
}


/*********************************************************************************************
  Private methods (implementation)

  Wifi network
*********************************************************************************************/

/*********************************************************************************************
  Event handler for ESP Wifi (static method)
*********************************************************************************************/

static esp_err_t CESP32WifiConnector_WifiEventHandler(void *this, system_event_t *pEvent)
{
  switch(pEvent->event_id)
  {
    case SYSTEM_EVENT_STA_GOT_IP:
      #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[INFO] 'CESP32WifiConnector_WifiEventHandler' - Event: SYSTEM_EVENT_STA_GOT_IP");
      #endif
      CESP32WifiConnector_UpdateConnectionState((CESP32WifiConnector *) this, ESP32WIFICONNECTOR_CONNECTION_EVENT_WIFI_CONNECTED);
      xEventGroupClearBits(((CESP32WifiConnector *) this)->m_hWifiEventGroup, WIFI_EVENT_GROUP_DISCONNECTED_BIT);
      xEventGroupSetBits(((CESP32WifiConnector *) this)->m_hWifiEventGroup, WIFI_EVENT_GROUP_CONNECTED_BIT);
      break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
      #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[INFO] 'CESP32WifiConnector_WifiEventHandler' - Event: SYSTEM_EVENT_STA_DISCONNECTED");
      #endif
      CESP32WifiConnector_UpdateConnectionState((CESP32WifiConnector *) this, ESP32WIFICONNECTOR_CONNECTION_EVENT_WIFI_DISCONNECTED);
      xEventGroupClearBits(((CESP32WifiConnector *) this)->m_hWifiEventGroup, WIFI_EVENT_GROUP_CONNECTED_BIT);
      xEventGroupSetBits(((CESP32WifiConnector *) this)->m_hWifiEventGroup, WIFI_EVENT_GROUP_DISCONNECTED_BIT);
      break;

    case SYSTEM_EVENT_STA_CONNECTED:
    case SYSTEM_EVENT_STA_LOST_IP:
    case SYSTEM_EVENT_STA_START:
    case SYSTEM_EVENT_STA_STOP:
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
    case SYSTEM_EVENT_WIFI_READY:
      #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
        DEBUG_PRINT("[INFO] 'CESP32WifiConnector_WifiEventHandler' - Event received: ");
        DEBUG_PRINT_DEC(pEvent->event_id);
        DEBUG_PRINT_CR;
      #endif
      break;

    default:
      break;
  }

  return ESP_OK;
}

bool CESP32WifiConnector_UpdateConnectionState(CESP32WifiConnector *this, DWORD dwConnectionEvent)
{
  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT("[INFO] 'CESP32WifiConnector_UpdateConnectionState' - Entering, current state: ");
    DEBUG_PRINT_DEC(this->m_dwConnectionState);
    DEBUG_PRINT_CR;
  #endif

  if (xSemaphoreTake(this->m_hConnectionStateMutex, pdMS_TO_TICKS(1000)) == pdPASS)
  {
    switch (dwConnectionEvent)
    {
      case ESP32WIFICONNECTOR_CONNECTION_EVENT_WIFI_CONNECTED:
      case ESP32WIFICONNECTOR_CONNECTION_EVENT_SERVER_DISCONNECTED:
        this->m_dwConnectionState = ESP32WIFICONNECTOR_CONNECTION_STATE_WIFI_CONNECTED;
        break;
  
      case ESP32WIFICONNECTOR_CONNECTION_EVENT_WIFI_DISCONNECTED:
        if (this->m_hServerSocket >= 0)
        {
          close(this->m_hServerSocket);
          this->m_hServerSocket = -1;
        }
        this->m_dwConnectionState = ESP32WIFICONNECTOR_CONNECTION_STATE_DISCONNECTED;
        break;
  
      case ESP32WIFICONNECTOR_CONNECTION_EVENT_SERVER_CONNECTED:
        this->m_dwConnectionState = ESP32WIFICONNECTOR_CONNECTION_STATE_SERVER_CONNECTED;
        break;

      case ESP32WIFICONNECTOR_CONNECTION_EVENT_SOCKET_OPENED:
        // UDP transport initialized, typically automaton will try to connect Network Server soon
        // (i.e. first connection or reconnection)
        this->m_dwConnectionState = ESP32WIFICONNECTOR_CONNECTION_STATE_SERVER_DISCONNECTED;
        break;
    }

    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT("[INFO] 'CESP32WifiConnector_UpdateConnectionState' - State updated: ");
      DEBUG_PRINT_DEC(this->m_dwConnectionState);
      DEBUG_PRINT_CR;
    #endif

    xSemaphoreGive(this->m_hConnectionStateMutex);
    return true;
  }

  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[ERROR] 'CESP32WifiConnector_UpdateConnectionState'- Failed: timed out waiting for mutex");
  #endif

  return false;
}

bool CESP32WifiConnector_JoinWifi(CESP32WifiConnector *this, bool bReconnect)
{
  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering 'CESP32WifiConnector_JoinWifi'");
  #endif

  // The caller can ask for reconnection to Wifi network
  if (bReconnect == true)
  {
    // Check if connected
    int nEventBits = xEventGroupWaitBits(this->m_hWifiEventGroup, WIFI_EVENT_GROUP_CONNECTED_BIT, 0, 1, 0);
    if (nEventBits & WIFI_EVENT_GROUP_CONNECTED_BIT)
    {
      #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[INFO] 'CESP32WifiConnector_JoinWifi' - Reconnection required, disconnecting now...");
      #endif
  
      xEventGroupClearBits(this->m_hWifiEventGroup, WIFI_EVENT_GROUP_CONNECTED_BIT);
      if (esp_wifi_disconnect() == ESP_OK)
      {
        xEventGroupWaitBits(this->m_hWifiEventGroup, WIFI_EVENT_GROUP_DISCONNECTED_BIT, 0, 1, portTICK_RATE_MS);
      }
      else
      {
        #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[ERROR] 'CESP32WifiConnector_JoinWifi' - Failed to disconnect from Wifi network");
        #endif
        return false;
      }
    }
  }

  // Connect station to Wifi network
  bool bError = false;

  wifi_config_t WifiConfig = { 0 };
  strcpy((char*) WifiConfig.sta.ssid, this->m_szWifiSSID);
  strcpy((char*) WifiConfig.sta.password, this->m_szWifiPassword);

  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] 'CESP32WifiConnector_JoinWifi' - Connecting station now...");
  #endif

  if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) { bError = true; }
  else if (esp_wifi_set_config(ESP_IF_WIFI_STA, &WifiConfig) != ESP_OK) { bError = true; }
  else if (esp_wifi_connect() != ESP_OK) { bError = true; }
  else 
  {
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT("[INFO] 'CESP32WifiConnector_JoinWifi' - Waiting event group:");
      DEBUG_PRINT_HEX(this->m_hWifiEventGroup);
      DEBUG_PRINT(", for: ");
      DEBUG_PRINT_DEC(this->m_dwWifiJoinTimeoutMillisec);
      DEBUG_PRINT_LN("ms");
    #endif

    xEventGroupWaitBits(this->m_hWifiEventGroup, WIFI_EVENT_GROUP_CONNECTED_BIT, 0, 1, 
                        this->m_dwWifiJoinTimeoutMillisec / portTICK_RATE_MS);

    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] 'CESP32WifiConnector_JoinWifi' - Wait done");
    #endif
  }

  if (bError == true ||
      (xEventGroupWaitBits(this->m_hWifiEventGroup, WIFI_EVENT_GROUP_DISCONNECTED_BIT, 0, 1, 0) & WIFI_EVENT_GROUP_DISCONNECTED_BIT))
  {
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] 'CESP32WifiConnector_JoinWifi' - Failed to connect station to Wifi");
    #endif
    return false;
  }

  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] 'CESP32WifiConnector_JoinWifi' - Station connected to Wifi");
  #endif
  return true;
}


/*********************************************************************************************
  Private methods (implementation)

  Access to Network Server
*********************************************************************************************/

bool CESP32WifiConnector_BindNetworkServer(CESP32WifiConnector *this)
{
  int hServerSocket;
  int opt;

  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering 'CESP32WifiConnector_BindNetworkServer'");
  #endif

  hServerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (hServerSocket < 0) 
  {
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] 'CESP32WifiConnector_BindNetworkServer' - Unable to create socket");
    #endif
    return false;
  }

  opt = 0;
  setsockopt(hServerSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
/*
  addr.sin_family = AF_INET;
  addr.sin_port = 0;
  addr.sin_addr.s_addr = s_iperf_ctrl.cfg.sip;
  if (bind(hServerSocket, (struct sockaddr*) &addr, sizeof(addr)) != 0) 
  {
    close(hServerSocket);
    return false;
  }
*/

  // Retrieve Network Server IP from URL
  const struct addrinfo hints = { .ai_family = AF_INET,
                                  .ai_socktype = SOCK_DGRAM,
                                  .ai_flags = 0,
//                                .ai_protocol = 0,
                                  .ai_protocol = IPPROTO_UDP,
                                };
  struct addrinfo *res;
  struct in_addr *paddr;
  char szPort[16];

  sprintf(szPort, "%d", this->m_dwNetworkServerPort); 
  int err = getaddrinfo(this->m_szNetworkServerUrl, szPort, &hints, &res);

  if (err != 0 || res == NULL) 
  {
    #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] 'CESP32WifiConnector_BindNetworkServer' - DNS lookup failed err=");
      DEBUG_PRINT_DEC(err);
      DEBUG_PRINT_CR;
    #endif
    return false;
  }

  // Store resolved IP.
  // Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
  paddr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
  strcpy(this->m_szNetworkServerIP, inet_ntoa(*paddr));
  freeaddrinfo(res);

  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT("[INFO] 'CESP32WifiConnector_BindNetworkServer' - Server IP address is ");
    DEBUG_PRINT_LN(this->m_szNetworkServerIP);
  #endif


  this->m_ServerSockAddr.sin_family = AF_INET;
  this->m_ServerSockAddr.sin_port = htons(this->m_dwNetworkServerPort);
  this->m_ServerSockAddr.sin_addr.s_addr = ipaddr_addr(this->m_szNetworkServerIP);

  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT("[INFO] 'CESP32WifiConnector_BindNetworkServer' - Server network address is ");
    DEBUG_PRINT_HEX(this->m_ServerSockAddr.sin_addr.s_addr);
    DEBUG_PRINT_CR;
  #endif


  // Network Server IP retrieved, UDP socket ready 
  this->m_hServerSocket = hServerSocket;
  CESP32WifiConnector_UpdateConnectionState(this, ESP32WIFICONNECTOR_CONNECTION_EVENT_SOCKET_OPENED);

  return true;
}

/*********************************************************************************************
  Private methods (implementation)

  SNTP Server activation and RTC time update
*********************************************************************************************/

bool CESP32WifiConnector_ConnectSNTPServer(CESP32WifiConnector *this, char *pSNTPServerUrl, DWORD dwSNTPServerPeriodSec)
{
  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering 'CESP32WifiConnector_ConnectSNTPServer'");
  #endif

  // Initialize SNTP utility
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, pSNTPServerUrl);
  sntp_init();

  // Wait for ESP32 RTC module update
  
  setenv("TZ", "CET-1", 1);
  tzset();

  time_t timeNow = 0;
  struct tm tmTimeinfo = { 0 };
  int nRetryCount = 0;

  while (tmTimeinfo.tm_year < (2017 - 1900) && ++nRetryCount < 10) 
  {
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    time(&timeNow);
    localtime_r(&timeNow, &tmTimeinfo);
  }

  #if (ESP32WIFICONNECTOR_DEBUG_LEVEL0)
    if (nRetryCount < 10)
    {
      DEBUG_PRINT("[INFO] 'CESP32WifiConnector_ConnectSNTPServer - System time: '");
      DEBUG_PRINT_DEC(timeNow);
      DEBUG_PRINT_CR;
    }
  #endif

  return nRetryCount < 10 ? true : false;
}

