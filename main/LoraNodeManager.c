/*****************************************************************************************//**
 * @file     LoraNodeManager.c
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


/*********************************************************************************************
  Espressif framework includes
*********************************************************************************************/

#include <Common.h>


/*********************************************************************************************
  Includes for object implementation
*********************************************************************************************/

// The CLoraNodeManager object implements the 'ITransceiverManager' interface
#define TRANSCEIVERMANAGERITF_IMPL

#include "SX1276Itf.h"
#include "TransceiverManagerItf.h"
#include "ServerManagerItf.h"
#include "LoraRealtimeSenderItf.h"

// Object's definitions and methods
#include "LoraNodeManager.h"
         
    
/*********************************************************************************************
  Instantiate global static objects used by module implementation
*********************************************************************************************/

// 'ITransceiverManager' interface function pointers
CTransceiverManagerItfImplOb g_TransceiverManagerItfImplOb = { .m_pAddRef = CLoraNodeManager_AddRef,
                                                               .m_pReleaseItf = CLoraNodeManager_ReleaseItf,
                                                               .m_pInitialize = CLoraNodeManager_Initialize,
                                                               .m_pAttach = CLoraNodeManager_Attach,
                                                               .m_pStart = CLoraNodeManager_Start,
                                                               .m_pStop = CLoraNodeManager_Stop,
                                                               .m_pSessionEvent = CLoraNodeManager_SessionEvent
                                                             };

// The CLoraNodeManager object implements the global configuration object
#define NODEMANAGERCONFIG_IMPL
#include "Configuration.h"


/*  To delete -> now in Configuration.h


// Gateway configuration and radio settings
//
// Note: 
//  - In this early version, the gateway hardware configuration and radio settings are
//    statically defined.
//  - In the final version, a similar object will be created in the main program using a 
//    configuration file and provided to 'CLoraNodeManager' object
//  - The operation mode for the gateway is:
//     .. Each CSX1276 is used for both uplink and downlink on a configured channel frequency
//     .. The CSX1276 is waiting for uplink LoRa packets (i.e. continuous receive mode by default)
//     .. The CSX1276 operation mode is changed to 'Send' mode only when a downlink packet is
//        received from the Network Server (typically LoRa 'ACK' message)
CTransceiverManagerItf_InitializeParamsOb g_LoraNodeManagerSettings = 
  { 
    .m_bUseBuiltinSettings = true,

    .pLoraTransceiverSettings = 
    { 
      [0] =
      {
        .LoraMAC = 
        {
          .m_wPreambleLength = LORATRANSCEIVERITF_PREAMBLE_LENGTH_NONE,
          .m_usSyncWord = LORATRANSCEIVERITF_SYNCWORD_NONE,
          .m_usHeader = LORATRANSCEIVERITF_HEADER_NONE,
          .m_usCRC = LORATRANSCEIVERITF_CRC_NONE,
          .m_bForce = false
        },
        .LoraMode =
        { 
          .m_usLoraMode = LORATRANSCEIVERITF_LORAMODE_NONE,
          .m_usCodingRate = LORATRANSCEIVERITF_CR_5,
          .m_usSpreadingFactor = LORATRANSCEIVERITF_SF_7,
          .m_usBandwidth = LORATRANSCEIVERITF_BANDWIDTH_125,
          .m_bForce = false
        },
        .PowerMode = 
        {
          .m_usPowerMode = LORATRANSCEIVERITF_POWER_MODE_LOW,
          .m_usPowerLevel = LORATRANSCEIVERITF_POWER_LEVEL_NONE,
          .m_usOcpRate = LORATRANSCEIVERITF_OCP_NONE,
          .m_bForce = false
        },
        .FreqChannel = 
        {
          .m_usFreqChannel = LORATRANSCEIVERITF_FREQUENCY_CHANNEL_18,
          .m_bForce = false
        },
      },
      [1] =
      {
        .LoraMAC = 
        {
          .m_wPreambleLength = LORATRANSCEIVERITF_PREAMBLE_LENGTH_NONE,
          .m_usSyncWord = LORATRANSCEIVERITF_SYNCWORD_NONE,
          .m_usHeader = LORATRANSCEIVERITF_HEADER_NONE,
          .m_usCRC = LORATRANSCEIVERITF_CRC_NONE,
          .m_bForce = false
        },
        .LoraMode =
        { 
          .m_usLoraMode = LORATRANSCEIVERITF_LORAMODE_NONE,
          .m_usCodingRate = LORATRANSCEIVERITF_CR_5,
          .m_usSpreadingFactor = LORATRANSCEIVERITF_SF_7,
          .m_usBandwidth = LORATRANSCEIVERITF_BANDWIDTH_125,
          .m_bForce = false
        },
        .PowerMode = 
        {
          .m_usPowerMode = LORATRANSCEIVERITF_POWER_MODE_LOW,
          .m_usPowerLevel = LORATRANSCEIVERITF_POWER_LEVEL_NONE,
          .m_usOcpRate = LORATRANSCEIVERITF_OCP_NONE,
          .m_bForce = false
        },
        .FreqChannel = 
        {
          .m_usFreqChannel = LORATRANSCEIVERITF_FREQUENCY_CHANNEL_17,
          .m_bForce = false
        }
      }
    }
  };
 
*/

/********************************************************************************************* 

 CLoraNodeManager Class

*********************************************************************************************/



/********************************************************************************************* 
  Public methods of CLoraNodeManager object
 
  These methods are exposed on object's public interfaces
*********************************************************************************************/

/*********************************************************************************************
  Object instance factory
 
  The factory contains one method used to create a new object instance.
  This method provides the 'ITransceiverManager' interface object for object's use and destruction.
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         ITransceiverManager CLoraNodeManager_CreateInstance(BYTE usTransceiverNumber)
 * 
 * @brief      Creates a new instance of CLoraNodeManager object.
 * 
 * @details    A new instance of CLoraNodeManager object is created and its 'ITransceiverManager'
 *             interface is returned. The owner object uses this interface to configure and 
 *             control associated 'LoraTransceiver'.
 *
 * @param      usTransceiverNumber
 *             The number of 'LoraTransceiver' in the gateway (i.e. hardware configuration).
 * 
 * @return     A 'ITransceiverManager' interface object.\n
 *             The reference count for returned 'ILoraTransceiver' interface is set to 1.
 *
 * @note       The CLoraNodeManager object and associated 'LoraTransceiver' objects are
 *             created but nothing is initialized (i.e. only object allocations).
 *             The 'ITransceiverManager_Initialize' method must be called to configure the 
 *             'LoraNodeManager' and associated 'LoraTransceiver'.
 *
 * @note       The CLoraNodeManager object is destroyed when the last reference to 
 *             'ITransceiverManager' is released (i.e. call to 'ITransceiverManager_ReleaseItf' 
 *             method).
 * 
 * @note       The 'LoraTransceiver' is assumed to be implemented by a 'CSX1276' object.
*********************************************************************************************/
ITransceiverManager CLoraNodeManager_CreateInstance(BYTE usTransceiverNumber)
{
  CLoraNodeManager * pLoraNodeManager;
  ILoraTransceiver pLoraTransceiverItf;
     
  // Create the object
  if ((pLoraNodeManager = CLoraNodeManager_New()) != NULL)
  {
    // Create 'LoraTransceiver' ojects (implemented by 'CSX1276' object)
    for (BYTE i = 0; i < usTransceiverNumber; i++)
    {
      if ((pLoraTransceiverItf = CSX1276_CreateInstance()) == NULL)
      {
        CLoraNodeManager_Delete(pLoraNodeManager);
        return NULL;
      }
      pLoraNodeManager->m_TransceiverDescrArray[i].m_pLoraTransceiverItf = pLoraTransceiverItf;
      ++(pLoraNodeManager->m_usTransceiverNumber);
    }

    // Create 'LoraRealtimeSender' object
    if ((pLoraNodeManager->m_pRealtimeSenderItf = CLoraRealtimeSender_CreateInstance()) == NULL)
    {
      CLoraNodeManager_Delete(pLoraNodeManager);
      return NULL;
    }

    // Create the 'ITransceiverManager' interface object
    if ((pLoraNodeManager->m_pTransceiverManagerItf = 
        ITransceiverManager_New(pLoraNodeManager, &g_TransceiverManagerItfImplOb)) != NULL)
    {
      ++(pLoraNodeManager->m_nRefCount);
    }
    return pLoraNodeManager->m_pTransceiverManagerItf;
  }

  return NULL;
}

/*********************************************************************************************
  Public methods exposed on 'ITransceiverManager' interface
 
  The static 'CLoraNodeManagerItfImplOb' object is initialized with pointers to these functions.
  The static 'CLoraNodeManagerItfImplOb' object is referenced in the 'ITransceiverManager'
  interface provided by 'CreateInstance' method (object factory).
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         uint32_t CLoraNodeManager_AddRef(void *this)
 * 
 * @brief      Increments the object's reference count.
 * 
 * @details    This function increments object's global reference count.\n
 *             The reference count is used to track the number of existing external references
 *             to 'ITransceiverManager' interface implemented by CLoraNodeManager object.
 * 
 * @param      this
 *             The pointer to CLoraNodeManager object.
 *  
 * @return     The value of reference count once incremented.
*********************************************************************************************/
uint32_t CLoraNodeManager_AddRef(void *this)
{
  return ++((CLoraNodeManager *)this)->m_nRefCount;
}

/*****************************************************************************************//**
 * @fn         uint32_t CLoraNodeManager_ReleaseItf(void *this)
 * 
 * @brief      Decrements the object's reference count.
 * 
 * @details    This function decrements object's global reference count and destroy the object
 *             when count reaches 0.\n
 *             The reference count is used to track the number of existing external references
 *             to 'ITransceiverManager' interface implemented by CLoraNodeManager object.
 * 
 * @param      this
 *             The pointer to CLoraNodeManager object.
 *  
 * @return     The value of reference count once decremented.
*********************************************************************************************/
uint32_t CLoraNodeManager_ReleaseItf(void *this)
{
  // Delete the object if its interface reference count reaches zero
  if (((CLoraNodeManager *)this)->m_nRefCount == 1)
  {
    // TO DO -> Stop the object master task which will delete the object on exit
    CLoraNodeManager_Delete((CLoraNodeManager *)this);
    return 0;
  }
  return --((CLoraNodeManager *)this)->m_nRefCount;
}

/*****************************************************************************************//**
 * @fn         bool CLoraNodeManager_Initialize(void *this, void *pParams)
 * 
 * @brief      Initializes the 'LoraNodeManager' and associated 'LoraTransceiver' objects.
 * 
 * @details    This function prepares the collection of 'LoraTransceiver' for radio transmissions.\n
 *             The default LoRa parameters are set and the 'LoraTransceivers' are is waiting
 *             ready in 'StandBy' mode.
 * 
 * @param      this
 *             The pointer to CLoraNodeManager object.
 *  
 * @param      pParams
 *             The method parameters (see 'NodeManagerItf.h' for details).
 *
 * @return     The returned value is 'true' if 'LoraNodeManager' has initialized all its
 *             associated 'LoraTransceivers' or 'false' in case of error.
*********************************************************************************************/
bool CLoraNodeManager_Initialize(void *this, void *pParams)
{
//  return CSX1276_NotifyAndProcessCommand((CSX1276 *) this, SX1276_AUTOMATON_CMD_INITIALIZE, pParams);


  // Debug
  DEBUG_PRINT_LN("[INFO] CLoraNodeManager_Initialize, calling CLoraNodeManager_NotifyAndProcessCommand");

  CLoraNodeManager_NotifyAndProcessCommand((CLoraNodeManager *) this, 
                                           LORANODEMANAGER_AUTOMATON_CMD_INITIALIZE, pParams);

  DEBUG_PRINT_LN("[INFO] CLoraNodeManager_Initialize, return from CLoraNodeManager_NotifyAndProcessCommand");

  return true;
}

bool CLoraNodeManager_Attach(void *this, void *pParams)
{
  return CLoraNodeManager_NotifyAndProcessCommand((CLoraNodeManager *) this, 
                                                  LORANODEMANAGER_AUTOMATON_CMD_ATTACH, pParams);
}

bool CLoraNodeManager_Start(void *this, void *pParams)
{
  return CLoraNodeManager_NotifyAndProcessCommand((CLoraNodeManager *) this, 
                                                  LORANODEMANAGER_AUTOMATON_CMD_START, pParams);
}

bool CLoraNodeManager_Stop(void *this, void *pParams)
{
  return CLoraNodeManager_NotifyAndProcessCommand((CLoraNodeManager *) this, 
                                                  LORANODEMANAGER_AUTOMATON_CMD_STOP, pParams);
}

bool CLoraNodeManager_SessionEvent(void *this, void *pEvent)
{
  CLoraNodeManager_MessageOb QueueMessage;

  // Insert the notification in queue used by 'SessionManager' task
  // Note: The 'CLoraNodeManager_MessageOb' object inserted in queue is a 
  //       'CTransceiverManagerItf_SessionEventOb' object
  QueueMessage.m_wMessageType = ((CTransceiverManagerItf_SessionEvent) pEvent)->m_wEventType;
  QueueMessage.m_dwMessageData = (DWORD)((CTransceiverManagerItf_SessionEvent) pEvent)->m_pSession;
  QueueMessage.m_dwMessageData2 = ((CTransceiverManagerItf_SessionEvent) pEvent)->m_dwSessionId;

  if (xQueueSend(((CLoraNodeManager *)this)->m_hSessionManagerQueue, &QueueMessage, 
      pdMS_TO_TICKS(LORANODEMANAGER_AUTOMATON_MAX_CMD_DURATION / 2)) != pdPASS)
  {
    // Message queue is full
    // Should never occur
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_Notify - Message queue full");
    #endif
    return false;
  }
  return true;
}

/********************************************************************************************* 
  Private methods of CLoraNodeManager object
 
  The following methods CANNOT be called by another object
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         bool CLoraNodeManager_NotifyAndProcessCommand(CLoraNodeManager *this, 
 *                                                           DWORD dwCommand, void *pCmdParams)
 * 
 * @brief      Process a command issued by a method of 'ILoraNodeManager' interface.
 * 
 * @details    The CLoraNodeManager main automaton asynchronously executes the commands generated
 *             by calls on 'ILoraNodeManager' interface methods.\n
 *             This method transmits commands to main automaton and wait for end of execution
 *             (i.e. from client point of view, the interface method is synchronous).
 * 
 * @param      this
 *             The pointer to CLoraNodeManager object.
 *  
 * @param      dwCommand
 *             The command to execute (typically same name than corresponding method on
 *             'ILoraNodeManager' interface. See 'LORANODEMANAGER_AUTOMATON_CMD_xxx' in 
 *             LoraNodeManager.h.
 *  
 * @param      pCmdParams
 *             A pointer to command parameters. The object pointed by 'pCmdParams' depends
 *             on method invoked on 'ILoraTransmiter' interface. 
 *             See 'LoraNodeManagerItf.h'.
 *  
 * @return     The returned value is 'true' if the command is properly executed or 'false'
 *             if command execution has failed or is still pending.
 *
 * @note       This function assumes that commands are quickly processed by main automaton.\n
 *             The maximum execution time can be configured and a mechanism is implemented in
 *             order to ignore client commands sent when a previous command is still pending.
*********************************************************************************************/
bool CLoraNodeManager_NotifyAndProcessCommand(CLoraNodeManager *this, DWORD dwCommand, void *pCmdParams)
{
  // Automaton commands are serialized (and should be quickly processed)
  // Note: In current design, there is only one client object for a single CLoraNodeManager
  //       instance (the main task on program startup and for operation control). In other 
  //       words, commands are serialized here and there is no concurrency on the 
  //       'CLoraNodeManager_NotifyAndProcessCommand' method.
  if (xSemaphoreTake(this->m_hCommandMutex, pdMS_TO_TICKS(LORANODEMANAGER_AUTOMATON_MAX_CMD_DURATION)) == pdFAIL)
  {
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_NotifyAndProcessCommand - Failed to take mutex");
    #endif

    return false;
  }

  // Make sure that previous command has been processed by the automaton
  if (this->m_dwCommand != LORANODEMANAGER_AUTOMATON_CMD_NONE)
  {
    // Previous call to this function has returned before end of command's execution
    // Check if done now
    if (xSemaphoreTake(this->m_hCommandDone, 0) == pdFAIL)
    {
      // Still not terminated
      #if (LORANODEMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_NotifyAndProcessCommand - Previous command still pending");
      #endif

      xSemaphoreGive(this->m_hCommandMutex);
      return false;
    }
  }

  // Post the command to main automaton
  this->m_dwCommand = dwCommand;
  this->m_pCommandParams = pCmdParams;
  CLoraNodeManager_MessageOb QueueMessage;
  QueueMessage.m_wMessageType = LORANODEMANAGER_AUTOMATON_MSG_COMMAND;

  if (xQueueSend(this->m_hSessionManagerQueue, &QueueMessage, pdMS_TO_TICKS(LORANODEMANAGER_AUTOMATON_MAX_CMD_DURATION / 2)) != pdPASS)
  {
    // Message queue is full
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_NotifyAndProcessCommand - Message queue full");
    #endif

    xSemaphoreGive(this->m_hCommandMutex);
    return false;
  }

  // Wait for command execution by main automaton
  BaseType_t nCommandDone = xSemaphoreTake(this->m_hCommandDone, pdMS_TO_TICKS(LORANODEMANAGER_AUTOMATON_MAX_CMD_DURATION -
                                           (LORANODEMANAGER_AUTOMATON_MAX_CMD_DURATION / 5)));

  // If the command has been processed, clear 'm_dwCommand' attribute
  if (nCommandDone == pdPASS)
  {
    this->m_dwCommand = LORANODEMANAGER_AUTOMATON_CMD_NONE;
  }
  else
  {
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_NotifyAndProcessCommand - Exiting before end of command execution");
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
  'SessionManager' task
 
  This RTOS 'Task' is the main automaton of 'CLoraNodeManager' object:
   - It processes messages sent by other tasks (typically when uplink or downlink LoRa packets
     are received or sent). In other words, it implements the LoRaWAN protocole.
   - It processes messages sent via 'ITransceiverManager' interface for comfiguration and 
     operation of associated radio transceivers.
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         void CLoraNodeManager_SessionManagerAutomaton(CLoraNodeManager *this)
 * 
 * @brief      Executes the 'SessionManager' automaton.
 * 
 * @details    This function is the RTOS task implementing the CLoraNodeManager main automaton.\n
 *             This automation processes the following messages:\n
 *               - Messages sent by other tasks (typically when uplink or downlink LoRa packets
 *                 are received or sent). 
 *               - Messages sent via 'ITransceiverManager' interface for comfiguration and 
 *                 operation of associated radio transceivers.
 *             This automaton manages the 'CLoraPacketSession' object collection:
 *               - The 'CLoraPacketSession' object is created by another task (i.e. the task
 *                 which  receives the packet)
 *               - The 'CLoraPacketSession' object content (typically 'state') is modified
 *                 only by the 'SessionManager' automaton
 *               - The 'CLoraPacketSession' object is destroyed only by the 'SessionManager'
 *                 automaton
 *
 * @param      this
 *             The pointer to CLoraNodeManager object.
 *  
 * @return     The RTOS task terminates when object is deleted (typically on main program
 *             exit).
 *
 * @note       The automaton's main loop waits for messages received through an RTOS queue.
*********************************************************************************************/
void CLoraNodeManager_SessionManagerAutomaton(CLoraNodeManager *this)
{
  CLoraNodeManager_MessageOb QueueMessage;
  BYTE i;
  CMemoryBlockArray pSessionArray = this->m_pLoraPacketSessionArray;
  CLoraPacketSession pLoraPacketSession;
  DWORD dwSessionEndTime;
  bool bReleaseSession;

  while (this->m_dwCurrentState != LORANODEMANAGER_AUTOMATON_STATE_TERMINATED)
  {
    if (this->m_dwCurrentState >= LORANODEMANAGER_AUTOMATON_STATE_CREATED)
    {
      #if (LORANODEMANAGER_DEBUG_LEVEL2)
        DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_SessionManagerAutomaton, waiting message");
      #endif
  
      // Wait for messages
      if (xQueueReceive(this->m_hSessionManagerQueue, &QueueMessage, pdMS_TO_TICKS(500)) == pdPASS)
      {
        // Process message
        #if (LORANODEMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_CR;
          DEBUG_PRINT("[INFO] CLoraNodeManager_SessionManagerAutomaton, message received: ");
          DEBUG_PRINT_HEX(QueueMessage.m_wMessageType);
          DEBUG_PRINT_CR;
        #endif
  
        if (QueueMessage.m_wMessageType == LORANODEMANAGER_AUTOMATON_MSG_COMMAND)
        {
          // Command message (i.e. external message via 'ITransceiverManager' interface
          // Note: The command is defined by 'm_dwCommand' and 'm_pCommandParams' variables
          //       (i.e. commands are serialized, never more than one command waiting in
          //       automaton's queue) 
          CLoraNodeManager_ProcessAutomatonNotifyCommand(this);
        }
        else if (QueueMessage.m_wMessageType >= TRANSCEIVERMANAGER_SESSIONEVENT_BASE)
        {
          // The message is a 'SessionEvent' sent via ''ITransceiverManager' interface
          // Note: The 'CLoraNodeManager_MessageOb' object retrieved in queue is a 
          //       'CTransceiverManagerItf_SessionEventOb' object
          switch (QueueMessage.m_wMessageType)
          {
            case TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_ACCEPTED:
              CLoraNodeManager_ProcessSessionEventUplinkAccepted(this, (CTransceiverManagerItf_SessionEvent) &QueueMessage);
              break;
            case TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_REJECTED:
              CLoraNodeManager_ProcessSessionEventUplinkRejected(this, (CTransceiverManagerItf_SessionEvent) &QueueMessage);
              break;
            case TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_PROGRESSING:
              CLoraNodeManager_ProcessSessionEventUplinkProgressing(this, (CTransceiverManagerItf_SessionEvent) &QueueMessage);
              break;
            case TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_SENT:
              CLoraNodeManager_ProcessSessionEventUplinkSent(this, (CTransceiverManagerItf_SessionEvent) &QueueMessage);
              break;
            case TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_FAILED:
              CLoraNodeManager_ProcessSessionEventUplinkFailed(this, (CTransceiverManagerItf_SessionEvent) &QueueMessage);
              break;
            case TRANSCEIVERMANAGER_SESSIONEVENT_DOWNLINK_SCHEDULED:
              CLoraNodeManager_ProcessSessionEventDownlinkScheduled(this, (CTransceiverManagerItf_SessionEvent) &QueueMessage);
              break;
            case TRANSCEIVERMANAGER_SESSIONEVENT_DOWNLINK_SENDING:
              CLoraNodeManager_ProcessSessionEventDownlinkSending(this, (CTransceiverManagerItf_SessionEvent) &QueueMessage);
              break;
            case TRANSCEIVERMANAGER_SESSIONEVENT_DOWNLINK_SENT:
              CLoraNodeManager_ProcessSessionEventDownlinkSent(this, (CTransceiverManagerItf_SessionEvent) &QueueMessage);
              break;
            case TRANSCEIVERMANAGER_SESSIONEVENT_DOWNLINK_FAILED:
              CLoraNodeManager_ProcessSessionEventDownlinkFailed(this, (CTransceiverManagerItf_SessionEvent) &QueueMessage, 0);
              break;
          }
        }
      }
      else
      {
        // No pending message for a while, look for expired UPLINK sessions
        #if (LORANODEMANAGER_DEBUG_LEVEL2)
          DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_SessionManagerAutomaton, checking expired sessions");
        #endif

        // IMPORTANT NODE: 
        // The 'LoraPacketSession' objects MUST be destroyed only by code below
        //  - The 'LoraPacketSession' cannot be destroyed while enumerated by the loop (i.e. no task synchronization required)
        //  - Some 'LoraPacketSession' may be missed if they become 'ready' during the loop (not an issue, checked on next iteration)
        for (i = 0; i < pSessionArray->m_usArraySize; i++)
        {
          if (CMemoryBlockArray_IsBlockReady(pSessionArray, i) == true)
          {
            #if (LORANODEMANAGER_DEBUG_LEVEL2)
              DEBUG_PRINT("[DEBUG] CLoraNodeManager_SessionManagerAutomaton, Enumerator, session block ready, index: ");
              DEBUG_PRINT_HEX((unsigned int) i);
              DEBUG_PRINT_CR;
            #endif

            bReleaseSession = false;
            pLoraPacketSession = (CLoraPacketSession) CMemoryBlockArray_BlockPtrFromIndex(pSessionArray, i);

            // Session can be released if terminated
            if ((pLoraPacketSession->m_dwSessionState == LORANODEMANAGER_SESSION_STATE_UPLINK_SENT) ||
                (pLoraPacketSession->m_dwSessionState == LORANODEMANAGER_SESSION_STATE_UPLINK_FAILED))
            {
              #if (LORANODEMANAGER_DEBUG_LEVEL0)
                DEBUG_PRINT("[INFO] CLoraNodeManager_SessionManagerAutomaton, LoraPacketSession terminated, destroying session, SessionId: ");
                DEBUG_PRINT_HEX(pLoraPacketSession->m_dwSessionId);
                DEBUG_PRINT_CR;
              #endif

              bReleaseSession = true;
            }
            else
            {
              // Compute time limit of 'Node' receive period
              if ((pLoraPacketSession->m_usMessageType == LORANODEMANAGER_MSG_TYPE_UNCONF_UPLINK) ||
                  (pLoraPacketSession->m_usMessageType == LORANODEMANAGER_MSG_TYPE_CONF_UPLINK))
              {
                dwSessionEndTime = pLoraPacketSession->m_dwTimestamp + LORANODEMANAGER_LORAWAN_RECEIVE_DELAY2 +
                                     LORANODEMANAGER_LORAWAN_RX_WINDOW_LENGTH;
              }
              else if (pLoraPacketSession->m_usMessageType == LORANODEMANAGER_MSG_TYPE_JOIN_REQUEST)
              {
                dwSessionEndTime = pLoraPacketSession->m_dwTimestamp + LORANODEMANAGER_LORAWAN_JOIN_ACCEPT_DELAY2 +
                                    LORANODEMANAGER_LORAWAN_RX_WINDOW_LENGTH;
              }
              else
              {
                // Sessions created by other packet types are not supported in this version
                dwSessionEndTime = 0;
              }

              // Session can be released at the end of 'Node' receive period in some cases 
              if ((dwSessionEndTime != 0) && (dwSessionEndTime <= xTaskGetTickCount() * portTICK_RATE_MS))
              {
                // Session can be destroyed only if: 
                //  - 'PacketForwarder' does not need 'LoraPacket' (i.e. it has encoded data)
                //  - The 'Node' packet does not require confirmation
                if ((pLoraPacketSession->m_dwSessionState == LORANODEMANAGER_SESSION_STATE_PROGRESSING_UPLINK) &&
                    ((pLoraPacketSession->m_usMessageType == LORANODEMANAGER_MSG_TYPE_UNCONF_UPLINK) ||
                    (pLoraPacketSession->m_usMessageType == LORANODEMANAGER_MSG_TYPE_JOIN_REQUEST)))
                {
                  #if (LORANODEMANAGER_DEBUG_LEVEL0)
                    DEBUG_PRINT("[INFO] CLoraNodeManager_SessionManagerAutomaton, LoraPacketSession expired, destroying session, SessionId: ");
                    DEBUG_PRINT_HEX(pLoraPacketSession->m_dwSessionId);
                    DEBUG_PRINT_CR;
                  #endif

                  bReleaseSession = true;
                }
                else
                {
                  #if (LORANODEMANAGER_DEBUG_LEVEL0)
                    DEBUG_PRINT_LN("[WARNING] CLoraNodeManager_SessionManagerAutomaton, session expired and ServerManager still 'Sending'");
                  #endif
                }
              }
            }

            if (bReleaseSession)
            {
              // TO DO: Make sure that 'LoraPacket' has been removed from its MemoryBlockArray (typically on 'SENT' or 'ACK' message)
              // Destroy 'CLoraPacket' if still allocated
              if (pLoraPacketSession->m_LoraPacketEntry.m_pDataBlock != NULL)
              {
                CMemoryBlockArray_ReleaseBlock(this->m_pLoraPacketArray, pLoraPacketSession->m_LoraPacketEntry.m_usBlockIndex);

                #if (LORANODEMANAGER_DEBUG_LEVEL2)
                  DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_SessionManagerAutomaton, LoraPacket destroyed");
                #endif
              }

              // Destroy 'CLoraPacketSession'
              CMemoryBlockArray_ReleaseBlock(pSessionArray, i);

              #if (LORANODEMANAGER_DEBUG_LEVEL2)
                DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_SessionManagerAutomaton, LoraPacketSession destroyed");
              #endif
            }
          }
          else
          {
//          #if (LORANODEMANAGER_DEBUG_LEVEL2)
//            DEBUG_PRINT("[DEBUG] CLoraNodeManager_SessionManagerAutomaton, Enumerator, session block not ready, index: ");
//            DEBUG_PRINT_HEX((unsigned int) i);
//            DEBUG_PRINT_CR;
//          #endif
          }
        }
      }
    }
    else
    {
      // Parent object not ready, wait for end of object's construction
      #if (LORANODEMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_CR;
        DEBUG_PRINT("CLoraNodeManager_SessionManagerAutomaton, waiting, state: ");
        DEBUG_PRINT_HEX(this->m_dwCurrentState);
        DEBUG_PRINT_CR;
      #endif

      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }

  // Main automaton terminated (typically 'CLoraNodeManager' being deleted)
  vTaskDelete(NULL);
  this->m_hSessionManagerTask = NULL;
}


/********************************************************************************************* 
  'Transceiver' task
 
  This RTOS 'Task' is used to process uplink packets received from 'LoraTransceiver' and 
  downlink packets 'Sent' by 'LoraTransceiver' (i.e. via 'ILoraTransceiverItf')

*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         void CLoraNodeManager_TransceiverAutomaton(CLoraNodeManager *this)
 * 
 * @brief      Processes packets exchanged with 'LoraTransceiver' objects associated to
 *             'CLoraNodeManager'.
 * 
 * @details    This function is the RTOS task used to process uplink packets received from
 *             'LoraTransceiver' and  downlink packets 'Sent' by 'LoraTransceiver' (i.e. via
 *             'ILoraTransceiverItf') :
 *              - The task collects uplink packets from different 'LoraTransceivers'. When a
 *                packet is received, the task initiates a LoRa session and transmits packets
 *                to the 'ServerManager'.
 *              - The task receives confirmations that downlink packets have been sent to
 *                endpoint by the 'LoraTransceiver'. When a confirmation is received, the task
 *                retrieves and terminates the session.
 * 
 * @param      this
 *             The pointer to CLoraNodeManager object.
 *  
 * @return     The RTOS task terminates when object is deleted (typically on main program
 *             exit).
 *
 * @note       The automaton's main loop waits for messages received through an RTOS queue.
 *             This queue contains messages related to packets exchanged with 'LoraTransceivers'
 *             (i.e. 'PacketReceived' and 'PacketSent' notifications).
*********************************************************************************************/
void CLoraNodeManager_TransceiverAutomaton(CLoraNodeManager *this)
{
  CLoraTransceiverItf_EventOb QueueMessage;

  while (this->m_dwCurrentState != LORANODEMANAGER_AUTOMATON_STATE_TERMINATED)
  {
    if (this->m_dwCurrentState >= LORANODEMANAGER_AUTOMATON_STATE_INITIALIZED)
    {
      #if (LORANODEMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("CLoraNodeManager_TransceiverAutomaton, waiting message");
      #endif

      // Wait for messages
      if (xQueueReceive(this->m_hTransceiverNotifQueue, &QueueMessage, pdMS_TO_TICKS(500)) == pdPASS)
      {
        // Process message
        #if (LORANODEMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_CR;
          DEBUG_PRINT("CLoraNodeManager_TransceiverAutomaton, message received: ");
          DEBUG_PRINT_HEX(QueueMessage.m_wEventType);
          DEBUG_PRINT_CR;
        #endif
  
        switch (QueueMessage.m_wEventType)
        {
          case LORATRANSCEIVERITF_EVENT_PACKETRECEIVED:
            CLoraNodeManager_ProcessTransceiverUplinkReceived(this, &QueueMessage);
            break;
  
          case LORATRANSCEIVERITF_EVENT_PACKETSENT:
            CLoraNodeManager_ProcessTransceiverDownlinkSent(this, &QueueMessage);
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

  // Main automaton terminated (typically 'CLoraNodeManager' being deleted)
  vTaskDelete(NULL);
  this->m_hTransceiverTask = NULL;
}


/********************************************************************************************* 
  'Forwarder' task
 
  This RTOS 'Task' is used to process downlink packets received from 'ServerManager' and 
  uplink packets 'Sent' by 'ServerManager' (i.e. via 'IServerManagerItf')

*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         void CLoraNodeManager_ServerAutomaton(CLoraNodeManager *this)
 * 
 * @brief      Processes packets exchanged with 'ServerManager' object associated to
 *             'CLoraNodeManager'.
 *
 * @details    This function is the RTOS task used to process  uplink packets 'Sent' by 
 *             'ServerManager' and downlink packets received from 'ServerManager' (i.e. via 
 *             'IPacketForwarderItf'):
 *              - The task receives confirmation that uplink packets have been sent to the
 *                Network Server by the 'ServerManager'. When a confirmation is received,
 *                the task retrieves the session and updates its state.
 *              - The task collects downlink packets sent from 'ServerManager' (typically 
 *                response to an uplink packet). When a packet is received, the task retrieves
 *                the session in order to identify the 'LoraTransceiver' associated to the 
 *                destination (endpoint) and asks the 'LoraTransceiver' to send the packet. 
 * 
 * @param      this
 *             The pointer to CLoraNodeManager object.
 *  
 * @return     The RTOS task terminates when object is deleted (typically on main program
 *             exit).
 *
 * @note       The automaton's main loop waits for messages received through an RTOS queue.
 *             This queue contains messages related to packets exchanged with 'ServerManager'
 *             (i.e. 'PacketReceived' and 'PacketSent' notifications).
*********************************************************************************************/
void CLoraNodeManager_ServerAutomaton(CLoraNodeManager *this)
{
//  CPacketForwarderItf_EventOb QueueMessage;

  while (this->m_dwCurrentState < LORANODEMANAGER_AUTOMATON_STATE_TERMINATED)
  {
/*
    if (this->m_dwCurrentState >= LORANODEMANAGER_AUTOMATON_STATE_INITIALIZED)
    {
      #if (LORANODEMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("CLoraNodeManager_ForwarderAutomaton, waiting message");
      #endif

      // Wait for messages
      if (xQueueReceive(this->m_hForwarderNotifQueue, &QueueMessage, pdMS_TO_TICKS(500)) == pdPASS)
      {
        // Process message
        #if (LORANODEMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_CR;
          DEBUG_PRINT("CLoraNodeManager_ForwarderAutomaton, message received: ");
          DEBUG_PRINT_HEX(QueueMessage.m_);
          DEBUG_PRINT_CR;
        #endif
  
        switch (QueueMessage.m_)
        {
          case :
            CLoraNodeManager_ProcessForwarderMsgXxx(this);
            break;
  
          case :
            CLoraNodeManager_ProcessForwarderMsgXxx(this);
            break;
        }
      }
    }
    else
    {
      // Parent object not ready, wait for end of object's initialization
      vTaskDelay(pdMS_TO_TICKS(100));
    }
*/
  }

  // Main automaton terminated (typically 'CLoraNodeManager' being deleted)
  vTaskDelete(NULL);
  this->m_hTransceiverTask = NULL;
}


/*********************************************************************************************
  Construction

  Protected methods : must be called only object factory and 'ITransceiverManager' interface
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         CLoraNodeManager * CLoraNodeManager_New()
 * 
 * @brief      Object construction.
 * 
 * @details    
 * 
 * @return     The function returns the pointer to the CLoraNodeManager instance.
 *
 * @note       This function only creates the object and its dependencies (RTOS objects).\n
*********************************************************************************************/
CLoraNodeManager * CLoraNodeManager_New()
{
  CLoraNodeManager *this;

#if LORANODEMANAGER_DEBUG_LEVEL2
  printf("CLoraNodeManager_New -> Debug level 2 (DEBUG)\n");
#elif LORANODEMANAGER_DEBUG_LEVEL1
  printf("CLoraNodeManager_New -> Debug level 1 (INFO)\n");
#elif LORANODEMANAGER_DEBUG_LEVEL0
  printf("CLoraNodeManager_New -> Debug level 0 (NORMAL)\n");
#endif 

  if ((this = (void *) pvPortMalloc(sizeof(CLoraNodeManager))) != NULL)
  {
    // The 'CLoraNodeObject' is under construction (i.e. embedded tasks must wait the 'INITIALIZED' state
    this->m_dwCurrentState = LORANODEMANAGER_AUTOMATON_STATE_CREATING;

    // Embedded objects are not defined (i.e. created below)
    this->m_pLoraPacketArray = this->m_pLoraPacketSessionArray = this->m_pLoraDownPacketSessionArray = NULL;
    this->m_hCommandMutex = this->m_hCommandDone = this->m_hSessionManagerTask = 
      this->m_hTransceiverTask = this->m_hSessionManagerQueue = 
      this->m_hTransceiverNotifQueue = this->m_hServerNotifQueue = 
      this->m_hPacketForwarderTask = NULL;
    this->m_pRealtimeSenderItf = NULL;


    #if (LORANODEMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_New Entering: create object 1");
    #endif

    // Allocate memory blocks for internal collections
    if ((this->m_pLoraPacketArray = CMemoryBlockArray_New(sizeof(CLoraTransceiverItf_LoraPacketOb) + 
        LORA_MAX_PAYLOAD_LENGTH - 1, LORANODEMANAGER_MAX_LORAPACKETS)) == NULL)
    {
      CLoraNodeManager_Delete(this);
      return NULL;
    }

    #if (LORANODEMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_New Entering: create object 2");
    #endif

    if ((this->m_pLoraPacketSessionArray = CMemoryBlockArray_New(sizeof(CLoraPacketSessionOb),
        LORANODEMANAGER_MAX_UP_LORASESSIONS)) == NULL)
    {
      CLoraNodeManager_Delete(this);
      return NULL;
    }

    #if (LORANODEMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_New Entering: create object 3");
    #endif

    if ((this->m_pLoraDownPacketSessionArray = CMemoryBlockArray_New(sizeof(CLoraDownPacketSessionOb),
        LORANODEMANAGER_MAX_DOWN_LORASESSIONS)) == NULL)
    {
      CLoraNodeManager_Delete(this);
      return NULL;
    }

    #if (LORANODEMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_New Entering: create object 4");
    #endif

    // Create SessionManager automaton task
    if (xTaskCreate((TaskFunction_t) CLoraNodeManager_SessionManagerAutomaton, "CLoraNodeManager_SessionManagerAutomaton", 
        2048, this, 5, &(this->m_hSessionManagerTask)) == pdFAIL)
    {
      CLoraNodeManager_Delete(this);
      return NULL;
    }

    #if (LORANODEMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_New Entering: create object 5");
    #endif

    if ((this->m_hCommandMutex = xSemaphoreCreateMutex()) == NULL)
    {
      CLoraNodeManager_Delete(this);
      return NULL;
    }

    #if (LORANODEMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_New Entering: create object 6");
    #endif

    if ((this->m_hCommandDone = xSemaphoreCreateBinary()) == NULL)
    {
      CLoraNodeManager_Delete(this);
      return NULL;
    }


    #if (LORANODEMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_New Entering: create object 7");
    #endif

    // Create Transceiver automaton task
    if (xTaskCreate((TaskFunction_t) CLoraNodeManager_TransceiverAutomaton, "CLoraNodeManager_TransceiverAutomaton", 
        2048, this, 5, &(this->m_hTransceiverTask)) == pdFAIL)
    {
      CLoraNodeManager_Delete(this);
      return NULL;
    }


    #if (LORANODEMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_New Entering: create object 8");
    #endif

    // Create Forwarder automaton task
    if (xTaskCreate((TaskFunction_t) CLoraNodeManager_ServerAutomaton, "CLoraNodeManager_ServerAutomaton", 
        2048, this, 5, &(this->m_hServerTask)) == pdFAIL)
    {
      CLoraNodeManager_Delete(this);
      return NULL;
    }


    #if (LORANODEMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_New Entering: create object 9");
    #endif

    // Message queue associated to 'SessionManager' task
    // Used for internal messages and external commands via 'ITransceiverManager' interface
    if ((this->m_hSessionManagerQueue = xQueueCreate(10, sizeof(CLoraNodeManager_MessageOb))) == NULL)
    {
      CLoraNodeManager_Delete(this);
      return NULL;
    }


    #if (LORANODEMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_New Entering: create object 10");
    #endif

    // Event queue associated to 'Transceiver' task
    // Used to process uplink packets received from 'LoraTransceiver' and downlink packets 'Sent' by
    // 'LoraTransceiver' (i.e. via 'ILoraTransceiverItf')
    if ((this->m_hTransceiverNotifQueue = xQueueCreate(10, sizeof(CLoraTransceiverItf_EventOb))) == NULL)
    {
      CLoraNodeManager_Delete(this);
      return NULL;
    }


    #if (LORANODEMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_New Entering: create object 11");
    #endif

    // Event queue associated to 'Forwarder' task
    // Used to process downlink packets received from 'ServerManager' and uplink packets 'Sent' by
    // 'ServerManager' (i.e. via 'IServerManagerItf')
    if ((this->m_hServerNotifQueue = xQueueCreate(10, sizeof(CServerManagerItf_EventOb))) == NULL)
    {
      CLoraNodeManager_Delete(this);
      return NULL;
    }

    // Initialize object's properties
    this->m_nRefCount = 0;
    this->m_dwCommand = LORANODEMANAGER_AUTOMATON_CMD_NONE;
    this->m_usTransceiverNumber = 0;
    this->m_dwMissedUplinkPacketdNumber = 0;

    this->m_ForwardedUplinkPacket.m_pLoraPacket = NULL;
    this->m_ForwardedUplinkPacket.m_pSession = NULL;
    this->m_ForwardedUplinkPacket.m_dwSessionId = 0;
    this->m_dwLastUpSessionId = 0;
    this->m_dwLastDownSessionId = 0;

    // Enter the 'CREATED' state
    this->m_dwCurrentState = LORANODEMANAGER_AUTOMATON_STATE_CREATED;
  }
  return this;
}

/*****************************************************************************************//**
 * @fn         void CLoraNodeManager_Delete(CSX1276 *this)
 * 
 * @brief      Object destruction.
 * 
 * @details    Destroys the CLoraNodeManager object.\n
 *             The associated RTOS objects are destroyed and the memory used by CLoraNodeManager
 *             object are released.

 * @param      this
 *             The pointer to CLoraNodeManager object.
 *  
 * @return     None.
*********************************************************************************************/
void CLoraNodeManager_Delete(CLoraNodeManager *this)
{
  // Ask all tasks for termination
  // TO DO (also check how to delete task object)

  // Delete queues (TO DO)

  // Free memory
  if (this->m_pLoraPacketArray != NULL)
  {
    CMemoryBlockArray_Delete(this->m_pLoraPacketArray);
  }
  if (this->m_pLoraPacketSessionArray != NULL)
  {
    CMemoryBlockArray_Delete(this->m_pLoraPacketSessionArray);
  }
  if (this->m_pLoraDownPacketSessionArray != NULL)
  {
    CMemoryBlockArray_Delete(this->m_pLoraDownPacketSessionArray);
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
  'LORANODEMANAGER_AUTOMATON_MSG_COMMAND' notification is received.

  Note: These functions may change automaton state. There is no protection against concurrency
        for automaton state because only functions called from automaton RTOS task are allowed
        to modify state (by design).
*********************************************************************************************/



/*****************************************************************************************//**
 * @fn         bool CLoraNodeManager_ProcessAutomatonNotifyCommand(CLoraNodeManager *this)
 * 
 * @brief      Process 'ILoraNodeManager' command currently waiting in automaton.
 * 
 * @details    This function is invoked by 'SessionManager' automaton when it receives a 
 *             'COMMAND' notification.\n
 *             These commands are issued via calls on 'ILoraNodeManager' interface.\n
 *             The command and its parameters are available in member variables of 
 *             CLoraSessionManager object.\n
 *             By design, commands are serialized when transmited to CLoraNodeManager automaton.
 *             In other words, the processing of one command cannot be interrupted by the 
 *             reception of another command.
 * 
 * @param      this
 *             The pointer to CLoraNodeManager object.
 *  
 * @return     The returned value is 'true' if command execution is successful or 'false' in
 *             case of error.
*********************************************************************************************/
bool CLoraNodeManager_ProcessAutomatonNotifyCommand(CLoraNodeManager *this)
{
  bool bResult;

  switch (this->m_dwCommand)
  {
    case LORANODEMANAGER_AUTOMATON_CMD_INITIALIZE:
      bResult = CLoraNodeManager_ProcessInitialize(this, (CTransceiverManagerItf_InitializeParams) this->m_pCommandParams);
      break;

    case LORANODEMANAGER_AUTOMATON_CMD_ATTACH:
      bResult = CLoraNodeManager_ProcessAttach(this, (CTransceiverManagerItf_AttachParams) this->m_pCommandParams);
      break;

    case LORANODEMANAGER_AUTOMATON_CMD_START:
      bResult = CLoraNodeManager_ProcessStart(this, (CTransceiverManagerItf_StartParams) this->m_pCommandParams);
      break;

    case LORANODEMANAGER_AUTOMATON_CMD_STOP:
      bResult = CLoraNodeManager_ProcessStop(this, (CTransceiverManagerItf_StopParams) this->m_pCommandParams);
      break;

    default:
      #if (LORANODEMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_ProcessAutomatonNotifyCommand, unknown command");
      #endif
      bResult = false;
      break;
  }

  // Command executed, allow next command for calling task (i.e. command execution by automaton is asynchronous
  // but has synchronous behavior for calling task)
  this->m_dwCommand = LORANODEMANAGER_AUTOMATON_CMD_NONE;
  xSemaphoreGive(this->m_hCommandDone);

  return bResult;
}


/*****************************************************************************************//**
 * @fn         bool CLoraNodeManager_ProcessInitialize(CLoraNodeManager *this, 
 *                                            CTransceiverManagerItf_InitializeParams pParams)
 * 
 * @brief      Initializes the object and configure associated 'LoraTransceivers'.
 * 
 * @details    This function reads the gateway configuration and initializes the 
 *             'LoraTransceivers' (i.e. prepares the transceiver devices for radio transmissions).\n
 *             The radio devices are configured and are waiting in 'StandBy' mode. 
 *             The 'START' and 'STOP' commands are then available to control the radio
 *             activity of the gateway.\n
 *             This function must be called one time in 'CREATED' automaton state.\n
 *             On exit, possible automaton states are:\n
 *              - 'LORANODEMANAGER_AUTOMATON_STATE_INITIALIZED' = the object and transceiver 
 *                devices initialization is successfully completed. The LoRa radio side of the 
 *                gateway is ready for operation.
 *              - 'LORANODEMANAGER_AUTOMATON_STATE_ERROR' = failed to initialize LoRa radio side
 *                of the gateway.
 * 
 * @param      this
 *             The pointer to CLoraNodeManager object.
 *  
 * @param      pParams
 *             The interface method parameters (see 'LoraNodeManagerItf.h' for details).
 *
 * @return     The returned value is 'true' if the LoraNodeManager is properly initialized or
 *             'false' in case of error.
*********************************************************************************************/
bool CLoraNodeManager_ProcessInitialize(CLoraNodeManager *this, CTransceiverManagerItf_InitializeParams pParams)
{
  CLoraTransceiverItf_InitializeParamsOb LoraTransceiverInitializeParams;

  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraNodeManager_ProcessInitialize'");
  #endif

  // The 'Initialize' method is allowed only in 'CREATED' and 'ERROR' automaton state
  if ((this->m_dwCurrentState != LORANODEMANAGER_AUTOMATON_STATE_CREATED) &&
      (this->m_dwCurrentState != LORANODEMANAGER_AUTOMATON_STATE_ERROR))
  {
    // By design, should never occur
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  // Early version: always configuration not provided, use builtin settings (i.e. statically defined in firmware) 
  if (pParams->m_bUseBuiltinSettings != true)
  {
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function MUST be called with 'UseBuiltinSettings'");
    #endif
    return false;
  }


  // Step 1: Apply the LoRa transceiver configuration
  // 
  // The number of 'LoraTransceiver' present in the gateway was indicated on object's construction.
  // The specified configuration must contain settings for at least this number of 'LoraTransceivers'.
  LoraTransceiverInitializeParams.m_hEventNotifyQueue = this->m_hTransceiverNotifQueue;
  for (BYTE i = 0; i < this->m_usTransceiverNumber; i++)
  {
    // Early version: configuration not provided use builtin settings (i.e. statically defined in firmware) 
    LoraTransceiverInitializeParams.pLoraMAC = &(g_LoraNodeManagerSettings.pLoraTransceiverSettings[i].LoraMAC);
    LoraTransceiverInitializeParams.pLoraMode = &(g_LoraNodeManagerSettings.pLoraTransceiverSettings[i].LoraMode);
    LoraTransceiverInitializeParams.pPowerMode = &(g_LoraNodeManagerSettings.pLoraTransceiverSettings[i].PowerMode);
    LoraTransceiverInitializeParams.pFreqChannel = &(g_LoraNodeManagerSettings.pLoraTransceiverSettings[i].FreqChannel);

    if (ILoraTransceiver_Initialize(this->m_TransceiverDescrArray[i].m_pLoraTransceiverItf, &LoraTransceiverInitializeParams) == false)
    {
      // By design, should never occur
      this->m_dwCurrentState = LORANODEMANAGER_AUTOMATON_STATE_ERROR;
      #if (LORANODEMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_ProcessInitialize, failed to initialize CSX1276");
      #endif
      return false;
    }
  }

  // Step 2: Attach the 'LoraRealtimeSender'
  //
  // The 'LoraRealtimeSender' sends asynchronous notifications using the 'SessionEvent' method of
  // ITransceiverManager' interface
  CLoraRealtimeSenderItf_InitializeParamsOb SenderInitParams;
  SenderInitParams.m_pTransceiverManagerItf = this->m_pTransceiverManagerItf;
  ILoraRealtimeSender_Initialize(this->m_pRealtimeSenderItf, &SenderInitParams);

  
  // Step 3: Attach the 'ServerManager'
  //
  // The 'ServerManager' will directly notify the 'NodeManagerAutomaton' task of 'LoraNodeManager'
  // when a new Lora Packet is received (Downlink = to forward to Node)
  CServerManagerItf_AttachParamsOb ServerAttachParams;
  ServerAttachParams.m_hNodeManagerTask = this->m_hServerTask;

  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_ProcessInitialize, calling ITransceiverManager_Attach");
  #endif

  IServerManager_Attach((IServerManager) pParams->m_pServerManagerItf, &ServerAttachParams);




  // All LoraTransceivers are ready in 'StandBy' mode:
  //  - Enter the 'IDLE' state if the 'ServerManager' is already attached
  //  - Otherwise enter the 'INITIALIZED' state (i.e. the 'IDLE' state will be entered when 'ServerManager' 
  //    will invoke the 'Attach' method)
  // Note: By design, no concurrency on automaton state variable
  if (this->m_hPacketForwarderTask == NULL)
  {
    this->m_dwCurrentState = LORANODEMANAGER_AUTOMATON_STATE_INITIALIZED;
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] CLoraNodeManager automaton state changed: 'INITIALIZED'");
    #endif
  }
  else
  {
    this->m_dwCurrentState = LORANODEMANAGER_AUTOMATON_STATE_IDLE;
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] CLoraNodeManager automaton state changed: 'IDLE'");
    #endif
  }

  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CLoraNodeManager successfully initialized for LoRA");
  #endif
  return true;
}



bool CLoraNodeManager_ProcessAttach(CLoraNodeManager *this, CTransceiverManagerItf_AttachParams pParams)
{
  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraNodeManager_ProcessAttach'");
  #endif

  // The 'Attach' method is allowed only in 'CREATED' and 'INITIALIZED' automaton state
  if ((this->m_dwCurrentState != LORANODEMANAGER_AUTOMATON_STATE_CREATED) &&
      (this->m_dwCurrentState != LORANODEMANAGER_AUTOMATON_STATE_INITIALIZED))
  {
    // By design, should never occur
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  // Check that no 'ServerManager' already attached
  // Command executed once at the end of startup process
  if (this->m_hPacketForwarderTask != NULL)
  {
    // By design, should never occur
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Packet forwarder already attached");
    #endif
    return false;
  }

  // Note: Task in 'CServerManager' object
  this->m_hPacketForwarderTask = pParams->m_hPacketForwarderTask;

  // Enter the 'IDLE' state if current state is 'INITIALIZED' 
  // Note: By design, no concurrency on automaton state variable
  if (this->m_dwCurrentState == LORANODEMANAGER_AUTOMATON_STATE_INITIALIZED)
  {
    this->m_dwCurrentState = LORANODEMANAGER_AUTOMATON_STATE_IDLE;
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] CLoraNodeManager automaton state changed: 'IDLE'");
    #endif
  }

  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CLoraNodeManager successfully attached to forwarder");
  #endif
  return true;
}


bool CLoraNodeManager_ProcessStart(CLoraNodeManager *this, CTransceiverManagerItf_StartParams pParams)
{
  CLoraTransceiverItf_ReceiveParamsOb ReceiveParams;
  CLoraRealtimeSenderItf_StartParamsOb StartParams;

  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraNodeManager_ProcessStart'");
  #endif

  // The 'Start' method is allowed only in 'IDLE' automaton state
  if (this->m_dwCurrentState != LORANODEMANAGER_AUTOMATON_STATE_IDLE)
  {
    // By design, should never occur
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  // Switch all 'LoraTransceivers' to continuous receive mode (i.e. wait for uplink packets)
  ReceiveParams.m_bForce = false;
  for (BYTE i = 0; i < this->m_usTransceiverNumber; i++)
  {
    if (ILoraTransceiver_Receive(this->m_TransceiverDescrArray[i].m_pLoraTransceiverItf, &ReceiveParams) == false)
    {
      // By design, should never occur
      this->m_dwCurrentState = LORANODEMANAGER_AUTOMATON_STATE_ERROR;
      #if (LORANODEMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_ProcessStart, failed to start receive on CSX1276");
      #endif
      return false;
    }
  }

  // Start the 'LoraRealtimeSender'
  StartParams.m_bForce = false;
  if (ILoraRealtimeSender_Start(this->m_pRealtimeSenderItf, &StartParams) == false)
  {
    // By design, should never occur
    this->m_dwCurrentState = LORANODEMANAGER_AUTOMATON_STATE_ERROR;
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_ProcessStart, failed to start CLoraRealtimeSender");
    #endif
    return false;
  }

  // Enter the 'RUNNING' state
  // Note: By design, no concurrency on automaton state variable
  this->m_dwCurrentState = LORANODEMANAGER_AUTOMATON_STATE_RUNNING;
  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CLoraNodeManager automaton state changed: 'RUNNING'");
  #endif

  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CLoraNodeManager successfully started (ready to create sessions)");
  #endif
  return true;
}

bool CLoraNodeManager_ProcessStop(CLoraNodeManager *this, CTransceiverManagerItf_StopParams pParams)
{
  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraNodeManager_ProcessStop'");
  #endif

  // The 'Stop' method is allowed only in 'RUNNING' automaton state
  if (this->m_dwCurrentState != LORANODEMANAGER_AUTOMATON_STATE_RUNNING)
  {
    // By design, should never occur
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  // Enter the 'STOPPING' state
  // Note: By design, no concurrency on automaton state variable
  this->m_dwCurrentState = LORANODEMANAGER_AUTOMATON_STATE_STOPPING;
  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CLoraNodeManager automaton state changed: 'STOPPING'");
  #endif

  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CLoraNodeManager currently stopping (no more sessions created)");
  #endif
  return true;
}


/*********************************************************************************************
  Private methods (implementation)

  Session Event processing

  These functions are called by 'SessionManager' automaton when one of the 
  'TRANSCEIVERMANAGER_SESSIONEVENT_xxx' notification is received.

  Note: These functions may change automaton state. There is no protection against concurrency
        for automaton state because only functions called from automaton RTOS task are allowed
        to modify state (by design).
*********************************************************************************************/

// The 'ServerManager' has copied the 'CLoraPacket' and 'CLoraPacketSession' references
// The exchange buffer used for notification can be released (i.e. allows notification of 
// next uplink LoRa packet)
void CLoraNodeManager_ProcessSessionEventUplinkAccepted(CLoraNodeManager *this, 
                                                        CTransceiverManagerItf_SessionEvent pEvent)
{
  CLoraPacketSession pLoraPacketSession;

  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraNodeManager_ProcessSessionEventUplinkAccepted'");
  #endif

  // Access Session
  pLoraPacketSession = (CLoraPacketSession) pEvent->m_pSession;

  // Make sure that message is for the last packet transmitted to 'ServerManager' and allow
  // next transmission (i.e. exchange buffer for only one 'LoraPacket')
  if (this->m_ForwardedUplinkPacket.m_pSession == pLoraPacketSession)
  {
    this->m_ForwardedUplinkPacket.m_pLoraPacket = NULL;
  }
  else
  {
    // By design shoud never occur (i.e. events are serialized)
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_ProcessSessionEventUplinkAccepted - Wrong session");
    #endif
  }
}

// The 'ServerManager' cannot process the 'CLoraPacket' (typically message queue full)
// Release the exchange buffer used for notification and destroy the 'CLoraPacketSession'
void CLoraNodeManager_ProcessSessionEventUplinkRejected(CLoraNodeManager *this, 
                                                        CTransceiverManagerItf_SessionEvent pEvent)
{
  CLoraPacketSession pLoraPacketSession;
  CLoraPacketSession pSessionCheck;

  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraNodeManager_ProcessSessionEventUplinkRejected'");
  #endif

  // Access Session
  pLoraPacketSession = (CLoraPacketSession) pEvent->m_pSession;

  // Step 1: Make sure that message is for the last packet transmitted to 'ServerManager' and allow
  //         next transmission (i.e. exchange buffer for only one 'LoraPacket')
  if (this->m_ForwardedUplinkPacket.m_pSession == pLoraPacketSession)
  {
    this->m_ForwardedUplinkPacket.m_pLoraPacket = NULL;
  }
  else
  {
    // By design shoud never occur (i.e. events are serialized)
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_ProcessSessionEventUplinkRejected - Wrong session");
    #endif
  }

  // Step 2: Destroy session

  // By design the 'REJECTED' event occurs before session has expired
  // Check for consistency

  pSessionCheck = (CLoraPacketSession) CMemoryBlockArray_BlockPtrFromIndex(this->m_pLoraPacketSessionArray, 
                                         pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex);

  if (pSessionCheck->m_dwSessionId == pLoraPacketSession->m_dwSessionId)
  {
    // MemoryBlock still associated to the session, make sure it is valid (i.e. the 'ready'
    // flag is set only when session is alive)
    if (CMemoryBlockArray_IsBlockReady(this->m_pLoraPacketSessionArray, 
        pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex) == true)
    {
      #if (LORANODEMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT("[INFO] CLoraNodeManager_ProcessSessionEventUplinkRejected, LoraPacketSession destroying session, SessionId: ");
        DEBUG_PRINT_HEX(pLoraPacketSession->m_dwSessionId);
        DEBUG_PRINT_CR;
      #endif

      if (pLoraPacketSession->m_LoraPacketEntry.m_pDataBlock != NULL)
      {
        CMemoryBlockArray_ReleaseBlock(this->m_pLoraPacketArray, pLoraPacketSession->m_LoraPacketEntry.m_usBlockIndex);

        #if (LORANODEMANAGER_DEBUG_LEVEL2)
          DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_ProcessSessionEventUplinkRejected, LoraPacket destroyed");
        #endif
      }

      // Destroy 'CLoraPacketSession'
      CMemoryBlockArray_ReleaseBlock(this->m_pLoraPacketSessionArray, pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex);

      #if (LORANODEMANAGER_DEBUG_LEVEL2)
        DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_ProcessSessionEventUplinkRejected, LoraPacketSession destroyed");
      #endif
    }
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      else
      {
        // Should never occur (session must be alive)
        DEBUG_PRINT_LN("[ERROR] 'CLoraNodeManager_ProcessSessionEventUplinkRejected' Session not found 0");
      }
    #endif
  }
  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    else
    {
      // Should never occur (session must be alive)
      DEBUG_PRINT_LN("[ERROR] 'CLoraNodeManager_ProcessSessionEventUplinkRejected' Session not found 1");
    }
  #endif
}

// The 'ServerManager' has encoded the 'CLoraPacket' and does not need it anymore
// Release the MemoryBlock used to store the 'CLoraPacket'
void CLoraNodeManager_ProcessSessionEventUplinkProgressing(CLoraNodeManager *this, 
                                                           CTransceiverManagerItf_SessionEvent pEvent)
{
  CLoraPacketSession pLoraPacketSession;
  CLoraPacketSession pSessionCheck;

  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraNodeManager_ProcessSessionEventUplinkProgressing'");
  #endif

  // Access Session
  pLoraPacketSession = (CLoraPacketSession) pEvent->m_pSession;

  // The preparation of Network Server message with 'CLoraPacket' may take a significant duration
  // Normally, the session is still alive but it is necessary to check

  pSessionCheck = (CLoraPacketSession) CMemoryBlockArray_BlockPtrFromIndex(this->m_pLoraPacketSessionArray, 
                                         pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex);

  if (pSessionCheck->m_dwSessionId == pLoraPacketSession->m_dwSessionId)
  {
    // MemoryBlock still associated to the session, make sure it is valid (i.e. the 'ready'
    // flag is set only when session is alive)
    if (CMemoryBlockArray_IsBlockReady(this->m_pLoraPacketSessionArray, 
        pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex) == true)
    {
      #if (LORANODEMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT("[INFO] CLoraNodeManager_ProcessSessionEventUplinkProgressing, LoraPacketSession releasing LoRa packet, SessionId: ");
        DEBUG_PRINT_HEX(pLoraPacketSession->m_dwSessionId);
        DEBUG_PRINT_CR;
      #endif

      if (pLoraPacketSession->m_LoraPacketEntry.m_pDataBlock != NULL)
      {
        CMemoryBlockArray_ReleaseBlock(this->m_pLoraPacketArray, pLoraPacketSession->m_LoraPacketEntry.m_usBlockIndex);
        pLoraPacketSession->m_LoraPacketEntry.m_pDataBlock = NULL;

        #if (LORANODEMANAGER_DEBUG_LEVEL2)
          DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_ProcessSessionEventUplinkProgressing, LoraPacket destroyed");
        #endif
      }
      #if (LORANODEMANAGER_DEBUG_LEVEL0)
        else
        {
          // Should never occur (if the session is alive the 'CLoraPacket' should exist)
          DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_ProcessSessionEventUplinkProgressing, LoraPacket already destroyed");
        }
      #endif

      pLoraPacketSession->m_dwSessionState = LORANODEMANAGER_SESSION_STATE_PROGRESSING_UPLINK;

      #if (LORANODEMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[INFO] CLoraNodeManager_ProcessSessionEventUplinkProgressing, Session state updated 'SENDING_UPLINK'");
      #endif
    }
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      else
      {
        // May occur (not an issue)
        DEBUG_PRINT_LN("[INFO] CLoraNodeManager_ProcessSessionEventUplinkProgressing, Session expired 0");
      }
    #endif
  }
  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    else
    {
      // May occur (not an issue)
      DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_ProcessSessionEventUplinkProgressing, Session expried 1");
    }
  #endif
}


void CLoraNodeManager_ProcessSessionEventUplinkSent(CLoraNodeManager *this, 
                                                    CTransceiverManagerItf_SessionEvent pEvent)
{
  CLoraPacketSession pLoraPacketSession;
  bool bSessionAlive;
  BYTE usAckPayload[10];
  CLoraNodeManager_ProcessServerDownlinkReceivedParamsOb DownlinkReceivedParams;

  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraNodeManager_ProcessSessionEventUplinkSent'");
  #endif

  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] 'CLoraNodeManager_ProcessSessionEventUplinkSent' - ticks: ");
    DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
    DEBUG_PRINT_CR;
  #endif

  // Access Session
  pLoraPacketSession = (CLoraPacketSession) pEvent->m_pSession;

  // The MemoryBlock for the specified session is in MemoryBlockArray but now it may contain
  // nothing or another session (i.e. depends on uplink LoRa packet type)
  bSessionAlive = false;
  if (pLoraPacketSession->m_dwSessionId == pEvent->m_dwSessionId)
  {
    // MemoryBlock still associated to the session, make sure it is valid (i.e. the 'ready'
    // flag is set only when session is alive)
    if (CMemoryBlockArray_IsBlockReady(this->m_pLoraPacketSessionArray, 
        pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex) == true)
    {
      // The session is alive
      bSessionAlive = true;

      #if (LORANODEMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[INFO] 'CLoraNodeManager_ProcessSessionEventUplinkSent' Session is alive");
      #endif
    }
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      else
      {
        DEBUG_PRINT_LN("[INFO] 'CLoraNodeManager_ProcessSessionEventUplinkSent' Session is NOT alive 0");
      }
    #endif
  }
  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    else
    {
      DEBUG_PRINT_LN("[INFO] 'CLoraNodeManager_ProcessSessionEventUplinkSent' Session is NOT alive 1");
    }
  #endif

  // The uplink has been sent (i.e. Network Server has acknowledged)
  // If a confirmation is requested by Node, prepare downlink LoRa packed and send it

  // IMPORTANT NOTE: For debug, send ACK evn in case of 'unconfirmed' uplink message 
  //                 TO DO: Remove in final version

  if (pLoraPacketSession->m_usMessageType == LORANODEMANAGER_MSG_TYPE_CONF_UPLINK ||
      pLoraPacketSession->m_usMessageType == LORANODEMANAGER_MSG_TYPE_UNCONF_UPLINK)
  {
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[WARNING] 'CLoraNodeManager_ProcessSessionEventUplinkSent' - TO DO: Update code in final version - Only for confirmed messages");
    #endif

    // The MemoryBlock for the specified session must be in MemoryBlockArray if session is for a
    // 'confirmed' uplink packet
    if (bSessionAlive == true)
    {
      // Build the payload for 'confirmation' message
      *((BYTE *)(usAckPayload)) = pLoraPacketSession->m_usMHDR;
      *((DWORD *)(usAckPayload + 1)) = pLoraPacketSession->m_dwDeviceAddr;
      *((BYTE *)(usAckPayload + 5)) = 0x10; // bits: 0010000 = ACK in FHDR.FCtrl field
      *((DWORD *)(usAckPayload + 6)) = pLoraPacketSession->m_dwFrameCounter;

      DownlinkReceivedParams.m_dwSessionType = LORANODEMANAGER_DOWNSESSION_TYPE_ACK;
      DownlinkReceivedParams.m_dwPayloadSize = 10;
      DownlinkReceivedParams.m_pPayload = usAckPayload;
      DownlinkReceivedParams.m_dwDeviceAddr = pLoraPacketSession->m_dwDeviceAddr;
      DownlinkReceivedParams.m_pLoraTransceiverItf = pLoraPacketSession->m_pLoraTransceiverItf;
      DownlinkReceivedParams.m_dwTimestamp = xTaskGetTickCount() * portTICK_RATE_MS;

      // Invoke the generic method for scheduling of a new downlink session
      if (CLoraNodeManager_ProcessServerDownlinkReceived(this, &DownlinkReceivedParams) == true)
      {
        #if (LORANODEMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[INFO] 'CLoraNodeManager_ProcessSessionEventUplinkSent' downlink LoRa session scheduled for ACK");
        #endif
      }
      else
      {
        #if (LORANODEMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[ERROR] 'CLoraNodeManager_ProcessSessionEventUplinkSent' Unable to schedule LoRa session for ACK");
        #endif
      }
    }
    else
    {
      #if (LORANODEMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] 'CLoraNodeManager_ProcessSessionEventUplinkSent' - Session must be alive for confirmed packet");
      #endif
    }
  }
  
  // If the session is alive, update its state
  if (bSessionAlive == true)
  {
    // The uplink session is terminated, set state to 'UPLINK_SENT'
    // Session will be released by Main automaton task
    // Note: The downlink session for ACK does not requires the uplink session (i.e. no ACK for ACK)
    pLoraPacketSession->m_dwSessionState = LORANODEMANAGER_SESSION_STATE_UPLINK_SENT;

    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] 'CLoraNodeManager_ProcessSessionEventUplinkSent' Session state updated 'UPLINK_SENT'");
    #endif
  }
}

void CLoraNodeManager_ProcessSessionEventUplinkFailed(CLoraNodeManager *this, 
                                                      CTransceiverManagerItf_SessionEvent pEvent)
{
  CLoraPacketSession pLoraPacketSession;

  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraNodeManager_ProcessSessionEventUplinkFailed'");
  #endif

  // Access Session
  pLoraPacketSession = (CLoraPacketSession) pEvent->m_pSession;

  // Update session state
  // 
  // The MemoryBlock for the specified session is in MemoryBlockArray but now it may contain
  // nothing or another session
  if (pLoraPacketSession->m_dwSessionId == pEvent->m_dwSessionId)
  {
    // MemoryBlock still associated to the session, make sure it is valid (i.e. the 'ready'
    // flag is set only when session is alive)
    if (CMemoryBlockArray_IsBlockReady(this->m_pLoraPacketSessionArray, 
        pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex) == true)
    {
      // The session is alive, update state in 'LoraPacketSession' object
      // Note: The 'NodeManager' main automaton will terminate the session immediatly (i.e. if the uplink 
      //       LoRa packet is not sent, it is not necessary to wait for a downlink packet)
      pLoraPacketSession->m_dwSessionState = LORANODEMANAGER_SESSION_STATE_UPLINK_FAILED;

      #if (LORANODEMANAGER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[INFO] 'CLoraNodeManager_ProcessSessionEventUplinkFailed' Session state updated 'UPLINK_SENT'");
      #endif
    }
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      else
      {
        DEBUG_PRINT_LN("[INFO] 'CLoraNodeManager_ProcessSessionEventUplinkFailed' Session not found 0");
      }
    #endif
  }
  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    else
    {
      DEBUG_PRINT_LN("[INFO] 'CLoraNodeManager_ProcessSessionEventUplinkFailed' Session not found 1");
    }
  #endif
}

//
// LoRaWAN acknowledgement rules
//  - When a 'confirmed' packet is sent (uplink or downlink), the destination must send back an ACK 
//    packet for confirmation (i.e. to indicates that packet is properly received, typically used for
//    retry management)
//  - When the received packet is an ACK packet, no reply is required (i.e. no ACK for ACK packet)
//  
// Some Network Server protocoles require that gateway notifies if it has accepted to transmit a
// downlink packet (i.e. message indicating if LoRa packet has been scheduled for send)
// 
// Note: According to LoRaWAN rules, this notification is not required for downlink ACK packets.
//

// The downlink packet is scheduled for send
// Notify the Network Server if necessary (i.e. according to packet type and protocol)
void CLoraNodeManager_ProcessSessionEventDownlinkScheduled(CLoraNodeManager *this, 
                                                           CTransceiverManagerItf_SessionEvent pEvent)
{
  CLoraDownPacketSession pLoraPacketSession;
  CLoraDownPacketSession pSessionCheck;

  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraNodeManager_ProcessSessionEventDownlinkScheduled'");
  #endif

  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] 'CLoraNodeManager_ProcessSessionEventDownlinkScheduled' - ticks: ");
    DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
    DEBUG_PRINT_CR;
  #endif

  // Access Session
  pLoraPacketSession = (CLoraDownPacketSession) pEvent->m_pSession;

  // Normally, the session is still alive but it is necessary to check
  pSessionCheck = (CLoraDownPacketSession) CMemoryBlockArray_BlockPtrFromIndex(this->m_pLoraDownPacketSessionArray, 
                                            pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex);
                    
  if (pSessionCheck->m_dwSessionId == pLoraPacketSession->m_dwSessionId)
  {
    // MemoryBlock still associated to the session, make sure it is valid (i.e. the 'ready'
    // flag is set only when session is alive)
    if (CMemoryBlockArray_IsBlockReady(this->m_pLoraDownPacketSessionArray, 
        pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex) == true)
    {

      // Check if a notification may be required by Network Server protocol
      if (pLoraPacketSession->m_usMessageType != LORANODEMANAGER_DOWNSESSION_TYPE_ACK)
      {
        // Notify the 'LoraServerManager'
        // The 'LoraServerManager' will ask the 'ProtocolEngine' to know if a message must be sent to the
        // Network Server
        // TO DO
        #if (LORANODEMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[ERROR] 'CLoraNodeManager_ProcessSessionEventDownlinkScheduled'- TO DO Implementation required");
        #endif
      }
    }
  }
}

// The transceiver is sending the LoRa packet
// Adjust session state
void CLoraNodeManager_ProcessSessionEventDownlinkSending(CLoraNodeManager *this, 
                                                         CTransceiverManagerItf_SessionEvent pEvent)
{
  CLoraDownPacketSession pLoraPacketSession;
  CLoraDownPacketSession pSessionCheck;

  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraNodeManager_ProcessSessionEventDownlinkSending'");
  #endif

  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] 'CLoraNodeManager_ProcessSessionEventDownlinkSending' - ticks: ");
    DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
    DEBUG_PRINT_CR;
  #endif

  // Access Session
  pLoraPacketSession = (CLoraDownPacketSession) pEvent->m_pSession;

  // Normally, the session is still alive but it is necessary to check
  pSessionCheck = (CLoraDownPacketSession) CMemoryBlockArray_BlockPtrFromIndex(this->m_pLoraDownPacketSessionArray, 
                                            pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex);
                    
  if (pSessionCheck->m_dwSessionId == pLoraPacketSession->m_dwSessionId)
  {
    // MemoryBlock still associated to the session, make sure it is valid (i.e. the 'ready'
    // flag is set only when session is alive)
    if (CMemoryBlockArray_IsBlockReady(this->m_pLoraDownPacketSessionArray, 
        pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex) == true)
    {
      // Adjust session state
      pLoraPacketSession->m_dwSessionState = LORANODEMANAGER_DOWNSESSION_STATE_SENDING;
    }
  }
}

// The downlink session is terminated with success
// Release session object and associated packet data
void CLoraNodeManager_ProcessSessionEventDownlinkSent(CLoraNodeManager *this, 
                                                      CTransceiverManagerItf_SessionEvent pEvent)
{
  CLoraDownPacketSession pLoraPacketSession;
  CLoraDownPacketSession pSessionCheck;

  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraNodeManager_ProcessSessionEventDownlinkSent'");
  #endif

  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] 'CLoraNodeManager_ProcessSessionEventDownlinkSent' - ticks: ");
    DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
    DEBUG_PRINT_CR;
  #endif

  // Access Session
  pLoraPacketSession = (CLoraDownPacketSession) pEvent->m_pSession;

  // Normally, the session is still alive but it is necessary to check
  pSessionCheck = (CLoraDownPacketSession) CMemoryBlockArray_BlockPtrFromIndex(this->m_pLoraDownPacketSessionArray, 
                                            pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex);
 
  if (pSessionCheck->m_dwSessionId == pLoraPacketSession->m_dwSessionId)
  {
    // MemoryBlock still associated to the session, make sure it is valid (i.e. the 'ready'
    // flag is set only when session is alive)
    if (CMemoryBlockArray_IsBlockReady(this->m_pLoraDownPacketSessionArray, 
        pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex) == true)
    {
      // Release the 'CLoraDownPacketSession' object
      CLoraNodeManager_ReleaseDownlinkSession(this, pLoraPacketSession);
    }
  }
  else
  {
    // Should never occur
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_ProcessSessionEventDownlinkSent: Unable to retrieve session");
    #endif
  }
}

// The downlink packet cannot be scheduled for send or a send error has occurred:
//  - If 'dwErrorCode' is 'LORAREALTIMESENDER_SCHEDULESEND_NONE', the packet has been scheduled for send but
//    an error occured with the transceiver
//  - If 'dwErrorCode' is NOT 'LORAREALTIMESENDER_SCHEDULESEND_NONE', the packet cannot be scheduled for send.
//    The error code indicates the reason and it may be necessary to notify the Network Server.
// In all cases, the downlink session is terminated and associated objects are released
void CLoraNodeManager_ProcessSessionEventDownlinkFailed(CLoraNodeManager *this, 
                                                        CTransceiverManagerItf_SessionEvent pEvent, DWORD dwErrorCode)
{
  CLoraDownPacketSession pLoraPacketSession;
  CLoraDownPacketSession pSessionCheck;

  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering 'CLoraNodeManager_ProcessSessionEventDownlinkFailed'");
  #endif

  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] 'CLoraNodeManager_ProcessSessionEventDownlinkFailed' - ticks: ");
    DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
    DEBUG_PRINT_CR;
  #endif

  // Access Session
  pLoraPacketSession = (CLoraDownPacketSession) pEvent->m_pSession;

  // Normally, the session is still alive but it is necessary to check
  pSessionCheck = (CLoraDownPacketSession) CMemoryBlockArray_BlockPtrFromIndex(this->m_pLoraDownPacketSessionArray, 
                                            pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex);
                    
  if (pSessionCheck->m_dwSessionId == pLoraPacketSession->m_dwSessionId)
  {
    // MemoryBlock still associated to the session, make sure it is valid (i.e. the 'ready'
    // flag is set only when session is alive)
    if (CMemoryBlockArray_IsBlockReady(this->m_pLoraDownPacketSessionArray, 
        pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex) == true)
    {
      // Check if a notification may be required by Network Server protocol
      if ((dwErrorCode != LORAREALTIMESENDER_SCHEDULESEND_NONE) &&
          (pLoraPacketSession->m_usMessageType != LORANODEMANAGER_DOWNSESSION_TYPE_ACK))
      {
        // Notify the 'LoraServerManager'
        // The 'LoraServerManager' will ask the 'ProtocolEngine' to know if a message must be sent to the
        // Network Server
        // TO DO
        #if (LORANODEMANAGER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[ERROR] 'CLoraNodeManager_ProcessSessionEventDownlinkFailed'- TO DO Implementation required");
        #endif
      }

      // Release the 'CLoraDownPacketSession' object
      CLoraNodeManager_ReleaseDownlinkSession(this, pLoraPacketSession);
    }
  }
  else
  {
    // Should never occur
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_ProcessSessionEventDownlinkFailed: Unable to retrieve session");
    #endif
  }
}

/*********************************************************************************************
  Private methods (implementation)

  Processing of events received by 'Transceiver' Task

  These functions are called by 'Transceiver' automaton when 'LORATRANSCEIVERITF_EVENT_xxx'
  event are received via 'ILoraTransceiver' interface.

  Note: These functions may change automaton state. There is no protection against concurrency
        for automaton state because only functions called from automaton RTOS task are allowed
        to modify state (by design).
*********************************************************************************************/


bool CLoraNodeManager_ProcessTransceiverUplinkReceived(CLoraNodeManager *this, CLoraTransceiverItf_Event pEvent)
{
  CMemoryBlockArrayEntryOb MemBlockEntry;
  BYTE *pMemBlock;
  CLoraTransceiverItf_LoraPacket pReceivedPacket;
  CLoraPacketSession pLoraPacketSession;
  BYTE *pPayload;
  CLoraTransceiverItf_GetReceivedPacketInfoParamsOb PacketInfoParams;
  CLoraRealtimeSenderItf_RegisterNodeRxWindowsParamsOb RegisterWindowsParams;

  // Received packet
  pReceivedPacket = (CLoraTransceiverItf_LoraPacket) (pEvent->m_pEventData);

  // New uplink 'LoraPacket' allowed only in 'RUNNING' automaton state
  if (this->m_dwCurrentState != LORANODEMANAGER_AUTOMATON_STATE_RUNNING)
  {
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT("[WARNING] LoraPacket received in wrong state: ");
      DEBUG_PRINT_DEC(this->m_dwCurrentState);
      DEBUG_PRINT_CR;
    #endif

    // Release packet in source 'LoraTransceiver' (i.e.set packet read semaphore)
    pReceivedPacket->m_dwDataSize = 0;
    return false;
  }

  // Step 1 - Obtain a 'MemoryBlock' to store the new 'LoraSession' associated with this uplink packet

  if ((pLoraPacketSession = CMemoryBlockArray_GetBlock(this->m_pLoraPacketSessionArray, &MemBlockEntry)) == NULL)
  {
    // Should never occur. Buffer for 'LoraPacketSession'exhausted
    // Note: No recovery mechanism = for stress test in current version
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] LoraPacketSession buffer exhausted. Entering 'ERROR' state");
      this->m_dwCurrentState = LORANODEMANAGER_AUTOMATON_STATE_ERROR;
    #endif

    // Release packet in source 'LoraTransceiver' (i.e.set packet read semaphore)
    pReceivedPacket->m_dwDataSize = 0;
    return false;
  }

  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CLoraNodeManager_ProcessTransceiverUplinkReceived: LoraPacketSession MemBlock, index: ");
    DEBUG_PRINT_HEX((unsigned int) MemBlockEntry.m_usBlockIndex);
    DEBUG_PRINT(", ptr: ");
    DEBUG_PRINT_HEX((unsigned int) MemBlockEntry.m_pDataBlock);
    DEBUG_PRINT_CR;
  #endif

  pLoraPacketSession->m_LoraSessionEntry.m_pDataBlock = MemBlockEntry.m_pDataBlock;
  pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex = MemBlockEntry.m_usBlockIndex;
  pLoraPacketSession->m_dwSessionState = LORANODEMANAGER_SESSION_STATE_CREATED;
  pLoraPacketSession->m_dwSessionId = ++this->m_dwLastUpSessionId;
  pLoraPacketSession->m_pLoraTransceiverItf = pEvent->m_pLoraTransceiverItf;

  // Step 2 - Store the 'CLoraPacket' in 'MemoryBlock' buffer
  //          This will allow to the 'LoraTransceiver' to receive a new packet

  if ((pMemBlock = CMemoryBlockArray_GetBlock(this->m_pLoraPacketArray, &(pLoraPacketSession->m_LoraPacketEntry))) == NULL)
  {
    // Should never occur. Buffer for 'LoraPacket' exhausted
    // Note: No recovery mechanism = for stress test in current version
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] LoraPacket buffer exhausted. Entering 'ERROR' state");
      this->m_dwCurrentState = LORANODEMANAGER_AUTOMATON_STATE_ERROR;
    #endif

    // Release packet in source 'LoraTransceiver' (i.e.set packet read semaphore)
    pReceivedPacket->m_dwDataSize = 0;

    // Release 'MemoryBlock' of 'LoraPacketSession'
    CMemoryBlockArray_ReleaseBlock(this->m_pLoraPacketSessionArray, pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex);

    #if (LORANODEMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_ProcessTransceiverUplinkReceived, LoraPacketSession destroyed");
    #endif

    return false;
  }

  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CLoraNodeManager_ProcessTransceiverUplinkReceived: LoraPacket MemBlock, index: ");
    DEBUG_PRINT_HEX((unsigned int) pLoraPacketSession->m_LoraPacketEntry.m_usBlockIndex);
    DEBUG_PRINT(", ptr: ");
    DEBUG_PRINT_HEX((unsigned int) pLoraPacketSession->m_LoraPacketEntry.m_pDataBlock);
    DEBUG_PRINT_CR;
  #endif


  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_ProcessTransceiverUplinkReceived: Copying packet to MemBlock");
  #endif

  memcpy(pMemBlock, pReceivedPacket, sizeof(CLoraTransceiverItf_LoraPacketOb) + LORA_MAX_PAYLOAD_LENGTH - 1);

  // Retrieve addtional information for received packet (SNR, RSSI...)
  PacketInfoParams.m_pPacketInfo = &pLoraPacketSession->m_ReceivedPacketInfo;
  ILoraTransceiver_GetReceivedPacketInfo(pLoraPacketSession->m_pLoraTransceiverItf, &PacketInfoParams);
                  
  // Release packet in source 'LoraTransceiver' (i.e.set packet read semaphore)
  // Note: From now, use 'ReceivedPacket' copy in MemoryBlock
  pReceivedPacket->m_dwDataSize = 0;
  pReceivedPacket = (CLoraTransceiverItf_LoraPacket) pMemBlock;

  // Store some properties of 'LoraPacket' in 'LoraPacketSession' (i.e. required to manage session life cycle later)
  pLoraPacketSession->m_dwTimestamp = ((CLoraTransceiverItf_LoraPacket) pMemBlock)->m_dwTimestamp;
  pPayload = (BYTE *) &(((CLoraTransceiverItf_LoraPacket) pMemBlock)->m_usData);
  pLoraPacketSession->m_usMHDR = *pPayload;
  pLoraPacketSession->m_usMessageType = LORANODEMANAGER_MSG_TYPE_BASE + (*pPayload >> 5);
  pLoraPacketSession->m_dwDeviceAddr = *((DWORD *)(pPayload + 1));
  pLoraPacketSession->m_dwFrameCounter = *((WORD *)(pPayload + 6));
  
  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CLoraNodeManager_ProcessTransceiverUplinkReceived: Packet session created, SessionId: ");
    DEBUG_PRINT_HEX(pLoraPacketSession->m_dwSessionId);
    DEBUG_PRINT(", Timestamp: ");
    DEBUG_PRINT_HEX(pLoraPacketSession->m_dwTimestamp);
    DEBUG_PRINT(", DeviceAddr: ");
    DEBUG_PRINT_HEX(pLoraPacketSession->m_dwDeviceAddr);
    DEBUG_PRINT(", FrameCounter: ");
    DEBUG_PRINT_HEX(pLoraPacketSession->m_dwFrameCounter);
    DEBUG_PRINT(", MessageType: ");
    DEBUG_PRINT_HEX((DWORD) pLoraPacketSession->m_usMessageType);
    DEBUG_PRINT(", Packet length: ");
    DEBUG_PRINT_HEX(pReceivedPacket->m_dwDataSize);
    DEBUG_PRINT_CR;
  #endif

  // The 'LoraPacketSession' object is fully defined in MemoryBlocks (i.e. it is 'CREATED')
  // Set the 'Ready' flag to allow other tasks to use it
  CMemoryBlockArray_SetBlockReady(this->m_pLoraPacketSessionArray, pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex);

  // Step 3 - Transmit the received packed to 'PacketForward' for send to network server

  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_ProcessTransceiverUplinkReceived: Transmitting packet to Forwarder");
  #endif

  // The 'CLoraNodeManager' uses a single dedicated 'CServerManagerItf_LoraSessionPacketOb' for exchange
  // with 'CServerManager'.
  // Check that there is no pending operation on this object
  if (this->m_ForwardedUplinkPacket.m_pLoraPacket != NULL)
  {
    // Previous notification to 'CServerManager' still processing
    // Wait a bit (but not too much)
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[WARNING] CLoraNodeManager_ProcessTransceiverUplinkReceived: Previous packet still in buffer");
    #endif

    vTaskDelay(pdMS_TO_TICKS(50));
  }

  if (this->m_ForwardedUplinkPacket.m_pLoraPacket != NULL)
  {
    // Miss this packet
    ++this->m_dwMissedUplinkPacketdNumber;

    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT("[ERROR] CLoraNodeManager_ProcessTransceiverUplinkReceived: Previous packet still in buffer, total missed: ");
      DEBUG_PRINT_DEC(this->m_dwMissedUplinkPacketdNumber);
      DEBUG_PRINT_CR;
    #endif

    // Release 'MemoryBlock' used for 'LoraPacket' and 'LoraPacketSession'
    CMemoryBlockArray_ReleaseBlock(this->m_pLoraPacketArray, pLoraPacketSession->m_LoraPacketEntry.m_usBlockIndex);
    CMemoryBlockArray_ReleaseBlock(this->m_pLoraPacketSessionArray, pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex);

    #if (LORANODEMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_ProcessTransceiverUplinkReceived, LoraPacket destroyed");
      DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_ProcessTransceiverUplinkReceived, LoraPacketSession destroyed");
    #endif

    return false;
  }

  // Send buffer available
  this->m_ForwardedUplinkPacket.m_dwSessionId = pLoraPacketSession->m_dwSessionId;
  this->m_ForwardedUplinkPacket.m_pSession = pLoraPacketSession;
  this->m_ForwardedUplinkPacket.m_pLoraPacket = pReceivedPacket;
  this->m_ForwardedUplinkPacket.m_pLoraPacketInfo = &pLoraPacketSession->m_ReceivedPacketInfo;

  pLoraPacketSession->m_dwSessionState = LORANODEMANAGER_SESSION_STATE_SENDING_UPLINK;

  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CLoraNodeManager_ProcessTransceiverUplinkReceived: Notifying task: ");
    DEBUG_PRINT_HEX((unsigned int) this->m_hPacketForwarderTask);
    DEBUG_PRINT_CR;
  #endif

  xTaskNotify(this->m_hPacketForwarderTask, (uint32_t) &(this->m_ForwardedUplinkPacket), eSetValueWithOverwrite);
  
  // Register the received uplink packet for downlink processing
  // Note:
  //  - For a quick processing of received packet, the registration is done after notification to 'ForwarderTask'
  //  - The duration required for packet roundtrip to Network Server is large enough to allow registration processing
  //    before an eventual downlink packet is received
  RegisterWindowsParams.m_dwDeviceAddr = pLoraPacketSession->m_dwDeviceAddr;
  RegisterWindowsParams.m_usDeviceClass = LORAREALTIMESENDER_DEVICECLASS_A;
  RegisterWindowsParams.m_pLoraTransceiverItf = pLoraPacketSession->m_pLoraTransceiverItf;
  RegisterWindowsParams.m_dwRXTimestamp = pReceivedPacket->m_dwTimestamp;
  if (ILoraRealtimeSender_RegisterNodeRxWindows(this->m_pRealtimeSenderItf, &RegisterWindowsParams) == false)
  {
    // Should never occur
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_ProcessTransceiverUplinkReceived: Unable to register node RX windows");
    #endif
  }

  return true;
}


// The 'LoraPacket' has been transmited to destination node, release associated downlink session
bool CLoraNodeManager_ProcessTransceiverDownlinkSent(CLoraNodeManager *this, CLoraTransceiverItf_Event pEvent)
{
  CTransceiverManagerItf_SessionEventOb SessionEvent;
  CLoraDownPacketSessionOb LoraPacketSession;
  CMemoryBlockArrayEnumItemOb EnumItem;
  bool bEntryFound;

  // Retrieve the downlink session associated to sent LoRa packet
  EnumItem.m_bByValue = true;
  EnumItem.m_pItemData = (void*) &LoraPacketSession;
  
  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_ProcessTransceiverDownlinkSent: Enumerator loop:");
  #endif

  bEntryFound = CMemoryBlockArray_EnumStart(this->m_pLoraDownPacketSessionArray, &EnumItem);
  while (bEntryFound == true)
  {
    // The 'm_pEventData' variable of 'pEvent' is the 'CLoraTransceiverItf_LoraPacket' sent
    // This packet is referenced by the 'CLoraDownPacketSession' and is stored in the 
    // 'm_pLoraDownPacketSessionArray' array

    #if (LORANODEMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT("Event packet: ");
      DEBUG_PRINT_HEX(pEvent->m_pEventData);
      DEBUG_PRINT(", Session packet: ");
      DEBUG_PRINT_HEX(LoraPacketSession.m_LoraPacketEntry.m_pDataBlock);
      DEBUG_PRINT_CR;
    #endif

    if (LoraPacketSession.m_LoraPacketEntry.m_pDataBlock == pEvent->m_pEventData)
    {
      // Downlink session retrieved
      // Sanity check: by design MemoryBlock used for LoraPacket cannot be released before end of session
      #if (LORANODEMANAGER_DEBUG_LEVEL1)
        CLoraTransceiverItf_LoraPacket pSendLoraPacket = (CLoraTransceiverItf_LoraPacket) pEvent->m_pEventData;
        CLoraTransceiverItf_LoraPacket pSessionLoraPacket = (CLoraTransceiverItf_LoraPacket) LoraPacketSession.m_LoraPacketEntry.m_pDataBlock;
        if ((pSessionLoraPacket->m_dwTimestamp != pSendLoraPacket->m_dwTimestamp) ||
            (pSessionLoraPacket->m_dwDataSize != pSendLoraPacket->m_dwDataSize))
        {
          DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_ProcessTransceiverDownlinkSent: Invalid LoRa packet found for session");
          return false;
        }
      #endif

      // Notify the parent 'LoraNodeManager'
      // Note: A session event is required for correct automaton state sequence of downlink session
      SessionEvent.m_pSession = LoraPacketSession.m_LoraSessionEntry.m_pDataBlock;
      SessionEvent.m_dwSessionId = LoraPacketSession.m_dwSessionId;
      SessionEvent.m_wEventType = TRANSCEIVERMANAGER_SESSIONEVENT_DOWNLINK_SENT;
      ITransceiverManager_SessionEvent(this->m_pTransceiverManagerItf, &SessionEvent);
      return true;
    }
    bEntryFound = CMemoryBlockArray_EnumNext(this->m_pLoraDownPacketSessionArray, &EnumItem);
  }

  // Should never occur: downlink session must be alive until packet is sent by transceiver
  DEBUG_PRINT_LN("[ERROR] CLoraNodeManager_ProcessTransceiverDownlinkSent: Unable to retrieve the downlink session associated to LoRa packet");
  return false;
}


/*********************************************************************************************
  Private methods (implementation)

  Processing of downlink sessions
*********************************************************************************************/

// Creates a new downlink session to send a LoRa packet and schedules it
// Two types of LoRa packets are supported:
//  - Packet for ACK message in response of an uplink LoRa session.
//    In this case the method is invoked by the Main automaton (UPLINK_SENT event processing)
//  - Packet for downlink data sent by Network Server
//    In this case the method is invoked by 'Server' automaton (downlink notification)
bool CLoraNodeManager_ProcessServerDownlinkReceived(CLoraNodeManager *this,
                                                    CLoraNodeManager_ProcessServerDownlinkReceivedParams pParams)
{
  CMemoryBlockArrayEntryOb MemBlockEntry;
  BYTE *pMemBlock;
  CLoraDownPacketSession pLoraPacketSession;
  DWORD dwResult;
  CLoraRealtimeSenderItf_ScheduleSendNodePacketParamsOb ScheduleSendParams;
  CTransceiverManagerItf_SessionEventOb SessionEvent;

  // Step 1 - Obtain a 'MemoryBlock' to store the new 'DownLoraSession' to manage send of downlink packet

  if ((pLoraPacketSession = CMemoryBlockArray_GetBlock(this->m_pLoraDownPacketSessionArray, &MemBlockEntry)) == NULL)
  {
    // Should never occur. Buffer for 'LoraDownPacketSession'exhausted
    // Note: No recovery mechanism = for stress test in current version
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] LoraDownPacketSession buffer exhausted. Entering 'ERROR' state");
      this->m_dwCurrentState = LORANODEMANAGER_AUTOMATON_STATE_ERROR;
    #endif

    return false;
  }

  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CLoraNodeManager_ProcessServerDownlinkReceived: LoraPacketSession MemBlock, index: ");
    DEBUG_PRINT_HEX((unsigned int) MemBlockEntry.m_usBlockIndex);
    DEBUG_PRINT(", ptr: ");
    DEBUG_PRINT_HEX((unsigned int) MemBlockEntry.m_pDataBlock);
    DEBUG_PRINT_CR;
  #endif

  pLoraPacketSession->m_LoraSessionEntry.m_pDataBlock = MemBlockEntry.m_pDataBlock;
  pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex = MemBlockEntry.m_usBlockIndex;
  pLoraPacketSession->m_dwSessionState = LORANODEMANAGER_DOWNSESSION_STATE_CREATED;
  pLoraPacketSession->m_dwSessionId = ++this->m_dwLastDownSessionId;
  pLoraPacketSession->m_usMessageType = (BYTE) pParams->m_dwSessionType;

  // Step 2 - Build the 'CLoraPacket' to send in a 'MemoryBlock' buffer
  //          The payload data are copied from memory provided in params

  if ((pMemBlock = CMemoryBlockArray_GetBlock(this->m_pLoraPacketArray, &(pLoraPacketSession->m_LoraPacketEntry))) == NULL)
  {
    // Should never occur. Buffer for 'LoraPacket' exhausted
    // Note: No recovery mechanism = for stress test in current version
    #if (LORANODEMANAGER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] LoraPacket buffer exhausted. Entering 'ERROR' state");
      this->m_dwCurrentState = LORANODEMANAGER_AUTOMATON_STATE_ERROR;
    #endif

    // Release 'MemoryBlock' of 'LoraPacketSession'
    CMemoryBlockArray_ReleaseBlock(this->m_pLoraPacketSessionArray, pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex);

    #if (LORANODEMANAGER_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_ProcessServerDownlinkReceived, LoraDownPacketSession destroyed");
    #endif

    return false;
  }

  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CLoraNodeManager_ProcessServerDownlinkReceived: LoraPacket MemBlock, index: ");
    DEBUG_PRINT_HEX((unsigned int) pLoraPacketSession->m_LoraPacketEntry.m_usBlockIndex);
    DEBUG_PRINT(", ptr: ");
    DEBUG_PRINT_HEX((unsigned int) pLoraPacketSession->m_LoraPacketEntry.m_pDataBlock);
    DEBUG_PRINT_CR;
  #endif


  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_ProcessServerDownlinkReceived: Copying packet to MemBlock");
  #endif

  // The 'MemoryBlock' is used to store the received packet (i.e. 'CLoraTransceiverItf_LoraPacketOb' object)
  ((CLoraTransceiverItf_LoraPacket) pMemBlock)->m_dwDataSize = pParams->m_dwPayloadSize;
  ((CLoraTransceiverItf_LoraPacket) pMemBlock)->m_dwTimestamp = pParams->m_dwTimestamp;
  memcpy(((CLoraTransceiverItf_LoraPacket) pMemBlock)->m_usData, pParams->m_pPayload, pParams->m_dwPayloadSize);

  pLoraPacketSession->m_pLoraTransceiverItf = pParams->m_pLoraTransceiverItf;

  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CLoraNodeManager_ProcessServerDownlinkReceived: Packet session created, SessionId: ");
    DEBUG_PRINT_HEX(pLoraPacketSession->m_dwSessionId);
    DEBUG_PRINT(", DeviceAddr: ");
    DEBUG_PRINT_HEX(pParams->m_dwDeviceAddr);
    DEBUG_PRINT(", Packet length: ");
    DEBUG_PRINT_HEX(((CLoraTransceiverItf_LoraPacket) pMemBlock)->m_dwDataSize);
    DEBUG_PRINT_CR;
  #endif

  // The 'LoraPacketSession' object is fully defined in MemoryBlocks (i.e. it is 'CREATED')
  // Set the 'Ready' flag to allow other tasks to use it
  CMemoryBlockArray_SetBlockReady(this->m_pLoraDownPacketSessionArray, pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex);

  // Step 3 - Transmit the received packed to 'PacketForward' for send to network server

  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT_LN("[DEBUG] CLoraNodeManager_ProcessServerDownlinkReceived: Transmitting packet to RealtimeLoraSender");
  #endif

  pLoraPacketSession->m_dwSessionState = LORANODEMANAGER_DOWNSESSION_STATE_SCHEDULING;

  // Ask the 'RealtimeLoraSender' to schedule packet for send 
  ScheduleSendParams.m_dwDeviceAddr = pParams->m_dwDeviceAddr;
  ScheduleSendParams.m_dwDownlinkSessionId = pLoraPacketSession->m_dwSessionId;
  ScheduleSendParams.m_pDownlinkSession = pLoraPacketSession;
  ScheduleSendParams.m_pPacketToSend = (CLoraTransceiverItf_LoraPacket) pMemBlock;
  dwResult = ILoraRealtimeSender_ScheduleSendNodePacket(this->m_pRealtimeSenderItf, &ScheduleSendParams);

  // An error code is returned if the packet cannot be scheduled
  // Note: If the packet is scheduled, a notification is sent (for correct serialization of session events)
  if (dwResult != LORAREALTIMESENDER_SCHEDULESEND_NONE)
  {
    // Process the session error (use standard error event processing)
    SessionEvent.m_dwSessionId = pLoraPacketSession->m_dwSessionId;
    SessionEvent.m_pSession = pLoraPacketSession;
    SessionEvent.m_wEventType = TRANSCEIVERMANAGER_SESSIONEVENT_DOWNLINK_FAILED;
    CLoraNodeManager_ProcessSessionEventDownlinkFailed(this, &SessionEvent, dwResult);
    return false;
  }
  else
  {
    // Adjust session state
    pLoraPacketSession->m_dwSessionState = LORANODEMANAGER_DOWNSESSION_STATE_SCHEDULED;
  }
  return true;
}

// Releases the 'CLoraDownPacketSession' object and associated LoRa packet data
void CLoraNodeManager_ReleaseDownlinkSession(CLoraNodeManager *this, CLoraDownPacketSession pLoraPacketSession)
{
  #if (LORANODEMANAGER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Entering CLoraNodeManager_ReleaseDownlinkSession");
  #endif

  // Release 'MemoryBlock' of 'CLoraTransceiverItf_LoraPacketOb'
  CMemoryBlockArray_ReleaseBlock(this->m_pLoraPacketArray, pLoraPacketSession->m_LoraPacketEntry.m_usBlockIndex);

  // Release 'MemoryBlock' of 'LoraPacketSession'
  CMemoryBlockArray_ReleaseBlock(this->m_pLoraDownPacketSessionArray, pLoraPacketSession->m_LoraSessionEntry.m_usBlockIndex);

  #if (LORANODEMANAGER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] 'CLoraNodeManager_ReleaseDownlinkSession' - Released - ticks: ");
    DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
    DEBUG_PRINT_CR;
  #endif
}

