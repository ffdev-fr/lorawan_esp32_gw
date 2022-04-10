/*****************************************************************************************//**
 * @file     LoraRealtimeSender.c
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     24/12/2018
 *
 * @brief    Sends downlink LoRa packets to nodes when their RX windows are open.
 *
 * @details  The downlink packets can be sent to nodes only when they are listening (i.e. the
 *           radio device may activate RX mode only for a short period of time).\n
 *           The owner object transmits to 'LoraRealtimeSender' the downlink LoRa packets to send.
 *           When a LoRa packet is enqueued for send, the 'LoraRealtimeSender' computes the time
 *           when Lora packet can be send to destination node.
 *           The 'LoraRealtimeSender' automatically sends waiting packets at the right time.\n
 *
 * @note     In current version only LoRaWAN class A is implemented.
 *
 * @note     This object is only used by 'LoraNodeManager' object (i.e. child object).
 *           The 'LoraNodeManager' delegates the downlink send operation to 'LoraRealTimeSender'.
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

// The CLoraRealtimeSender object implements the 'ILoraRealtimeSender' interface
#define LORAREALTIMESENDERITF_IMPL

#include "LoraTransceiverItf.h"
#include "TransceiverManagerItf.h"
#include "LoraRealtimeSenderItf.h"
#include "Configuration.h"

// Object's definitions and methods
#include "LoraRealtimeSender.h"
         
    
/*********************************************************************************************
  Instantiate global static objects used by module implementation
*********************************************************************************************/

// 'ILoraRealtimeSender' interface function pointers
struct _CLoraRealtimeSenderItfImpl g_LoraRealtimeSenderItfImplOb = { .m_pAddRef = CLoraRealtimeSender_AddRef,
                                                                     .m_pReleaseItf = CLoraRealtimeSender_ReleaseItf,
                                                                     .m_pInitialize = CLoraRealtimeSender_Initialize,
                                                                     .m_pStart = CLoraRealtimeSender_Start,
                                                                     .m_pStop = CLoraRealtimeSender_Stop,
                                                                     .m_pRegisterNodeRxWindows = CLoraRealtimeSender_RegisterNodeRxWindows,
                                                                     .m_pScheduleSendNodePacket = CLoraRealtimeSender_ScheduleSendNodePacket
                                                                    };



/********************************************************************************************* 

 CLoraRealtimeSender Class

*********************************************************************************************/



/********************************************************************************************* 
  Public methods of CLoraRealtimeSender object
 
  These methods are exposed on object's public interfaces
*********************************************************************************************/

/*********************************************************************************************
  Object instance factory
 
  The factory contains one method used to create a new object instance.
  This method provides the 'ILoraRealtimeSender' interface object for object's use and destruction.
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         ILoraRealtimeSender CLoraRealtimeSender_CreateInstance()
 * 
 * @brief      Creates a new instance of CLoraRealtimeSender object.
 * 
 * @details    A new instance of CLoraRealtimeSender object is created and its 
 *             'ILoraRealtimeSender' interface is returned. The owner object invokes methods
 *             of this interface to send LoRa downlink packets when nodes are in receive mode.
 *
 * @return     A 'ILoraRealtimeSender' interface object.\n
 *             The reference count for returned 'ILoraRealtimeSender' interface is set to 1.
 *
 * @note       The CLoraRealtimeSender object is destroyed when the last reference to 
 *             'ILoraRealtimeSender' is released (i.e. call to 'ILoraRealtimeSender_ReleaseItf' 
 *             method).
*********************************************************************************************/
ILoraRealtimeSender CLoraRealtimeSender_CreateInstance()
{
  CLoraRealtimeSender * pLoraRealtimeSender;
     
  // Create the object
  if ((pLoraRealtimeSender = CLoraRealtimeSender_New()) != NULL)
  {
    // Create the 'ILoraRealtimeSender' interface object
    if ((pLoraRealtimeSender->m_pLoraRealtimeSenderItf =
        ILoraRealtimeSender_New(pLoraRealtimeSender, &g_LoraRealtimeSenderItfImplOb)) != NULL)
    {
      ++(pLoraRealtimeSender->m_nRefCount);
    }
    return pLoraRealtimeSender->m_pLoraRealtimeSenderItf;
  }

  return NULL;
}

/*********************************************************************************************
  Public methods exposed on 'ILoraRealtimeSender' interface
 
  The static 'CLoraRealtimeSenderItfImplOb' object is initialized with pointers to these functions.
  The static 'CLoraRealtimeSenderItfImplOb' object is referenced in the 'ILoraRealtimeSender'
  interface provided by 'CreateInstance' method (object factory).
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         uint32_t CLoraRealtimeSender_AddRef(void *this)
 * 
 * @brief      Increments the object's reference count.
 * 
 * @details    This function increments object's global reference count.\n
 *             The reference count is used to track the number of existing external references
 *             to 'ILoraRealtimeSender' interface implemented by CLoraRealtimeSender object.
 * 
 * @param      this
 *             The pointer to CLoraRealtimeSender object.
 *  
 * @return     The value of reference count once incremented.
*********************************************************************************************/
uint32_t CLoraRealtimeSender_AddRef(void *this)
{
  return ++((CLoraRealtimeSender *)this)->m_nRefCount;
}

/*****************************************************************************************//**
 * @fn         uint32_t CLoraRealtimeSender_ReleaseItf(void *this)
 * 
 * @brief      Decrements the object's reference count.
 * 
 * @details    This function decrements object's global reference count and destroy the object
 *             when count reaches 0.\n
 *             The reference count is used to track the number of existing external references
 *             to 'ILoraRealtimeSender' interface implemented by CLoraRealtimeSender object.
 * 
 * @param      this
 *             The pointer to CLoraRealtimeSender object.
 *  
 * @return     The value of reference count once decremented.
*********************************************************************************************/
uint32_t CLoraRealtimeSender_ReleaseItf(void *this)
{
  // Delete the object if its interface reference count reaches zero
  if (((CLoraRealtimeSender *)this)->m_nRefCount == 1)
  {
    CLoraRealtimeSender_Delete((CLoraRealtimeSender *)this);
    return 0;
  }
  return --((CLoraRealtimeSender *)this)->m_nRefCount;
}


bool CLoraRealtimeSender_Initialize(void *this, CLoraRealtimeSenderItf_InitializeParams pParams)
{
  #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[DEBUG] CLoraRealtimeSender_Initialize - Entering funcion");
  #endif

  if (((CLoraRealtimeSender *)this)->m_dwCurrentState == LORAREALTIMESENDER_AUTOMATON_STATE_CREATED)
  {
    ((CLoraRealtimeSender *)this)->m_pTransceiverManagerItf = pParams->m_pTransceiverManagerItf;
    ((CLoraRealtimeSender *)this)->m_dwCurrentState = LORAREALTIMESENDER_AUTOMATON_STATE_INITIALIZED;
    return true;
  }
  return false;
}

bool CLoraRealtimeSender_Start(void *this, CLoraRealtimeSenderItf_StartParams pParams)
{
  #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[DEBUG] CLoraRealtimeSender_Start - Entering funcion");
  #endif

  if ((((CLoraRealtimeSender *)this)->m_dwCurrentState == LORAREALTIMESENDER_AUTOMATON_STATE_INITIALIZED) ||
      (((CLoraRealtimeSender *)this)->m_dwCurrentState == LORAREALTIMESENDER_AUTOMATON_STATE_IDLE))
  {
    ((CLoraRealtimeSender *)this)->m_dwCurrentState = LORAREALTIMESENDER_AUTOMATON_STATE_RUNNING;

    #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] CLoraRealtimeSender_Start - Automaton state changed: 'RUNNING'");
    #endif
    return true;
  }
  return false;
}

bool CLoraRealtimeSender_Stop(void *this, CLoraRealtimeSenderItf_StopParams pParams)
{
  #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[DEBUG] CLoraRealtimeSender_Stop - Entering funcion");
  #endif

  if (((CLoraRealtimeSender *)this)->m_dwCurrentState == LORAREALTIMESENDER_AUTOMATON_STATE_RUNNING)
  {
    ((CLoraRealtimeSender *)this)->m_dwCurrentState = LORAREALTIMESENDER_AUTOMATON_STATE_STOPPING;
    return true;
  }
  return false;
}

/*****************************************************************************************//**
 * @fn         bool CLoraRealtimeSender_RegisterNodeRxWindows(void *this, 
 *                                  CLoraRealtimeSenderItf_RegisterNodeRxWindowsParams pParams)
 * 
 * @brief      Computes and registers the start times of RX windows for a received uplink 
 *             LoRa packet.
 * 
 * @details    This function behaves as follows:\n
 *              - The 'NodeManager' invokes the function when it receives a Lora packet from
 *                Node.\n
 *              - The function computes the start times of RX windows and insert an entry in
 *                'm_pNodeReceiveWindowArray' array.\n
 *              - When a downlink packet is received, the entry is retrieved and used to
 *                program the send operation when a node's RX window is active 
 *                (see 'CLoraRealtimeSender_ScheduleSendNodePacket' method for details).\n
 *              - The 'SenderTask' periodically checks for entries with elapsed RX windows
 *                and removes them from 'm_pNodeReceiveWindowArray' array (i.e. uplink packet
 *                without or with too late downlink reply).\n
 * 
 * @param      this
 *             The pointer to CLoraRealtimeSender object.
 *  
 * @return     The 'true' value is returned if node's RX windows are registered or false if
 *             an error has occurred (typically current time past the end of last RX2 window).
*********************************************************************************************/
bool CLoraRealtimeSender_RegisterNodeRxWindows(void *this, 
                                               CLoraRealtimeSenderItf_RegisterNodeRxWindowsParams pParams)
{
  CNodeReceiveWindow pNodeReceiveWindow;
  CMemoryBlockArrayEntryOb MemBlockEntry;

  // Obtain an entry in 'm_pNodeReceiveWindowArray' array
  // The RX window definition depends on LoRa class of device
  if (pParams->m_usDeviceClass == LORAREALTIMESENDER_DEVICECLASS_A)
  {
    // Sanity check: 
    //  - For LoRa class A, a given node cannot send another uplink packet until duration for
    //    RX windows is elapsed.
    //  - The periodical cleanup for expired entries in 'm_pNodeReceiveWindowArray' array may
    //    have not processed the entry yet.
    if ((pNodeReceiveWindow = CLoraRealtimeSender_FindNodeReceiveWindow((CLoraRealtimeSender *) this, pParams->m_dwDeviceAddr, false)) != NULL)
    {
      // The new uplink packet must be received after last RX window duration of previous packet is elapsed
      if (pParams->m_dwRXTimestamp < pNodeReceiveWindow->m_dwRX2WindowTimestamp + LORAREALTIMESENDER_LORAWAN_RX_WINDOW_LENGTH)
      {
        // Should never occur
        // Maybe adjust 'LORAREALTIMESENDER_CLASSA_RX_PREAMBLE_RATIO' (some nodes may have very small
        // width for RX windows = time to receive LoRa preamble)
        #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[ERROR] CLoraRealtimeSender_RegisterNodeRxWindows - Uplink packet received too early");
        #endif
        return false;
      }

      // Note: 
      //  - By design, the periodical cleanup must remove this entry before the downlink packet is received
      //  - A new entry is always used for the received uplink packet
    }

    if ((pNodeReceiveWindow = (CNodeReceiveWindow) CMemoryBlockArray_GetBlock(((CLoraRealtimeSender *) this)->m_pNodeReceiveWindowArray, &MemBlockEntry)) == NULL)
    {
      // Should never occur
      #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] CLoraRealtimeSender_RegisterNodeRxWindows - NodeReceiveWindow array full");
      #endif
      return false;
    }

    // Compute start times of RX windows
    pNodeReceiveWindow->m_dwRX1WindowTimestamp = pParams->m_dwRXTimestamp + LORAREALTIMESENDER_CLASSA_RECEIVE_DELAY1;
    pNodeReceiveWindow->m_dwRX2WindowTimestamp = pParams->m_dwRXTimestamp + LORAREALTIMESENDER_CLASSA_RECEIVE_DELAY2;

    pNodeReceiveWindow->m_usDeviceClass = pParams->m_usDeviceClass;
    pNodeReceiveWindow->m_dwDeviceAddr = pParams->m_dwDeviceAddr;
    pNodeReceiveWindow->m_pLoraTransceiverItf = pParams->m_pLoraTransceiverItf;

    // Allow other tasks to use this entry
    CMemoryBlockArray_SetBlockReady(((CLoraRealtimeSender *) this)->m_pNodeReceiveWindowArray, MemBlockEntry.m_usBlockIndex);
  }
  else
  {
    // Class C devices not supported in this version
    #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraRealtimeSender_RegisterNodeRxWindows - Downlink for class C device not supported");
    #endif
    return false;
  }

  return true;
}

/*****************************************************************************************//**
 * @fn         DWORD CLoraRealtimeSender_ScheduleSendNodePacket(void *this, 
 *                            CLoraRealtimeSenderItf_ScheduleSendNodePacketParams pParams)
 * 
 * @brief      Schedules a downlink LoRa packet for send when an RX window is open on 
 *             destination node.
 * 
 * @details    This function behaves as follows:\n
 *              - The 'NodeManager' invokes the function when it receives a downlink LoRa
 *                packet from Network Server.\n
 *              - The function checks when an RX window will be active on destination node
 *                by looking in 'm_pNodeReceiveWindowArray' array.\n
 *              - If an active RX window is retrieved and no schedule collision is detected
 *                on the transceiver, a new entry is inserted in the 'm_pRealtimeLoraPacketArray'
 *                array.\n
 *              - The 'SenderTask' looks in 'm_pNodeReceiveWindowArray' array and send the
 *                LoRa packets at the scheduled time.\n
 * 
 * @param      this
 *             The pointer to CLoraRealtimeSender object.
 *  
 * @return     The function returns a code to indicate if the LoRa packet is scheduled for
 *             send (= 'LORAREALTIMESENDER_SCHEDULESEND_xxx'. The semantic of returned codes
 *             is specified from codes used by Semtech protocol for TX_ACK message.
 *
 * @note       For Semtech protocol the Network Server must be notified if the downlink packet 
 *             can be sent at expected time (i.e. not a confirmation that node has received
 *             the packet.
*********************************************************************************************/
DWORD CLoraRealtimeSender_ScheduleSendNodePacket(void *this, 
                            CLoraRealtimeSenderItf_ScheduleSendNodePacketParams pParams)
{
  CNodeReceiveWindow pNodeReceiveWindow;
  CRealtimeLoraPacket pRealtimeLoraPacket;
  CMemoryBlockArrayEntryOb MemBlockEntry;
  DWORD dwCurrentTimestamp;
  bool bScheduled;
  CTransceiverManagerItf_SessionEventOb SessionEvent;

  #if (LORAREALTIMESENDER_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] Entering 'CLoraRealtimeSender_ScheduleSendNodePacket' - ticks: ");
    DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
    DEBUG_PRINT_CR;
  #endif

  // Step 1: Retrieve the receive windows for the destination device
  if ((pNodeReceiveWindow = CLoraRealtimeSender_FindNodeReceiveWindow((CLoraRealtimeSender *) this, pParams->m_dwDeviceAddr, true)) == NULL)
  {
    // No receive window descriptor registered
    // Probably a too late downlink message for a Class A device
    #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[WARNING] CLoraRealtimeSender_ScheduleSendNodePacket - No window descriptor found (maybe too late)");
    #endif
    return LORAREALTIMESENDER_SCHEDULESEND_TOO_LATE;
  }

  // Step 2: Obtain an entry in the schedule queue
  if ((pRealtimeLoraPacket = CMemoryBlockArray_GetBlock(((CLoraRealtimeSender *) this)->m_pRealtimeLoraPacketArray, &MemBlockEntry)) == NULL)
  {
    // Should never occur
    #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraRealtimeSender_ScheduleSendNodePacket - RealtimeLoraPacket array full");
    #endif
    return LORAREALTIMESENDER_SCHEDULESEND_COLLISION_PACKET;
  }

  // Step 3: Define when packet can be sent

  // Check if it not too late to schedule the send operation
  dwCurrentTimestamp = xTaskGetTickCount() * portTICK_RATE_MS;
  bScheduled = false;
  if (pNodeReceiveWindow->m_usDeviceClass == LORAREALTIMESENDER_DEVICECLASS_A)
  {
    if (dwCurrentTimestamp < pNodeReceiveWindow->m_dwRX1WindowTimestamp + 
        (LORAREALTIMESENDER_LORAWAN_RX_WINDOW_LENGTH - LORAREALTIMESENDER_GATEWAY_TX_DELAY))
    {
      // Lora packet can be send on RX1 window, look for collision
      bScheduled = true;
      pRealtimeLoraPacket->m_bASAP = true;
      pRealtimeLoraPacket->m_dwSendTimestamp = pNodeReceiveWindow->m_dwRX1WindowTimestamp + 
        (LORAREALTIMESENDER_LORAWAN_RX_WINDOW_LENGTH - LORAREALTIMESENDER_GATEWAY_TX_DELAY);
    }

    // If packet cannot be scheduled on RX1 window, check if possible on RX2 window
    if (bScheduled == false)
    {
      if (dwCurrentTimestamp < pNodeReceiveWindow->m_dwRX2WindowTimestamp + 
          (LORAREALTIMESENDER_LORAWAN_RX_WINDOW_LENGTH - LORAREALTIMESENDER_GATEWAY_TX_DELAY))
      {
        // Lora packet can be send on RX1 window, look for collision
        pRealtimeLoraPacket->m_bASAP = true;
        pRealtimeLoraPacket->m_dwSendTimestamp = pNodeReceiveWindow->m_dwRX2WindowTimestamp + 
          (LORAREALTIMESENDER_LORAWAN_RX_WINDOW_LENGTH - LORAREALTIMESENDER_GATEWAY_TX_DELAY);
      }
      else
      {
        // Too late to schedule send of LoRa packet
        #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[WARNING] CLoraRealtimeSender_ScheduleSendNodePacket - No RX window available (too late)");
        #endif
        CMemoryBlockArray_ReleaseBlock(((CLoraRealtimeSender *) this)->m_pRealtimeLoraPacketArray, MemBlockEntry.m_usBlockIndex);
        return LORAREALTIMESENDER_SCHEDULESEND_TOO_LATE;
      }
    }

    // Note: The 'NodeReceiveWindow' checked here (Class A = generated by uplink LoRa packet) will be
    //       removed later by the 'SenderTask'.
  }
  else
  {
    // TO DO: Only Class A supported in this version
    #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraRealtimeSender_ScheduleSendNodePacket - Only Class A devices supported");
    #endif
    CMemoryBlockArray_ReleaseBlock(((CLoraRealtimeSender *) this)->m_pRealtimeLoraPacketArray, MemBlockEntry.m_usBlockIndex);
    return LORAREALTIMESENDER_SCHEDULESEND_TOO_LATE;
  }

  // Step 4: Schedule send for the LoRa packet
  //
  // NOTE: When reaching this point, 'bScheduled' is always true
  pRealtimeLoraPacket->m_pLoraTransceiverItf = pNodeReceiveWindow->m_pLoraTransceiverItf;
  pRealtimeLoraPacket->m_dwDownlinkSessionId = pParams->m_dwDownlinkSessionId;
  pRealtimeLoraPacket->m_pDownlinkSession = pParams->m_pDownlinkSession;
  pRealtimeLoraPacket->m_pPacketToSend = pParams->m_pPacketToSend;

  #if (LORAREALTIMESENDER_DEBUG_LEVEL1)
    DEBUG_PRINT_LN("[INFO] CLoraRealtimeSender_ScheduleSendNodePacket - Scheduling downlink packet for send");
  #endif

  // Notify the parent 'LoraNodeManager'
  // IMPORTANT NOTE: The session event is required when packet is scheduled because the 'SenderTask'
  //                 may process it before the return of this function (i.e. required for correct 
  //                 automaton state sequence of downlink session)
  SessionEvent.m_pSession = pParams->m_pDownlinkSession;
  SessionEvent.m_dwSessionId = pParams->m_dwDownlinkSessionId;
  SessionEvent.m_wEventType = TRANSCEIVERMANAGER_SESSIONEVENT_DOWNLINK_SCHEDULED;
  ITransceiverManager_SessionEvent(((CLoraRealtimeSender *) this)->m_pTransceiverManagerItf, &SessionEvent);

  CMemoryBlockArray_SetBlockReady(((CLoraRealtimeSender *) this)->m_pRealtimeLoraPacketArray, MemBlockEntry.m_usBlockIndex);
  xSemaphoreGive(((CLoraRealtimeSender *) this)->m_hPacketWaiting);

  return LORAREALTIMESENDER_SCHEDULESEND_NONE;
}


/********************************************************************************************* 
  Private methods of CLoraRealtimeSender object
 
  The following methods CANNOT be called by another object
*********************************************************************************************/


/********************************************************************************************* 
  RTOS task functions
*********************************************************************************************/


/********************************************************************************************* 
  'PacketSender' task
 
  This RTOS 'Task' is used to send downlink packets to nodes just in time.

*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         void CLoraRealtimeSender_PacketSenderAutomaton(CLoraRealtimeSender *this)
 * 
 * @brief      Periodically checks if it is time to send next LoRa packet to its destination node.
 * 
 * @details    This function is the RTOS task used to send downlink packets at their progammed
 *             time (i.e. when the receive window of destination node is open):
 * 
 * @param      this
 *             The pointer to CLoraRealtimeSender object.
 *  
 * @return     The RTOS task terminates when object is deleted (typically on main program
 *             exit).
*********************************************************************************************/
void CLoraRealtimeSender_PacketSenderAutomaton(CLoraRealtimeSender *this)
{
  DWORD dwCurrentTicks;
  CLoraTransceiverItf_SendParamsOb SendParams;
  CTransceiverManagerItf_SessionEventOb SessionEvent;
  CRealtimeLoraPacket pRealtimeLoraPacket;
  bool bSendingPacket;
  DWORD dwSemaphoreWaitMs;

  dwSemaphoreWaitMs = 500;
  while (this->m_dwCurrentState != LORAREALTIMESENDER_AUTOMATON_STATE_TERMINATED)
  {
    if (this->m_dwCurrentState == LORAREALTIMESENDER_AUTOMATON_STATE_RUNNING)
    {
      #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[DEBUG] CLoraRealtimeSender_PacketSenderAutomaton, waiting message");
      #endif

      // Wait for signal that a new LoRa packet is programmed for realtime send
      if (xSemaphoreTake(this->m_hPacketWaiting, pdMS_TO_TICKS(dwSemaphoreWaitMs)) == pdPASS)
      {
        // Process message
        #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[INFO] CLoraRealtimeSender_PacketSenderAutomaton, next packet scheduled: ");
        #endif
  
        #if (LORAREALTIMESENDER_DEBUG_LEVEL2)
          DEBUG_PRINT("[DEBUG] CLoraRealtimeSender_PacketSenderAutomaton - ticks: ");
          DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
          DEBUG_PRINT_CR;
        #endif

        // Retrieve the next packet to send
        // 
        // NOTE:
        //  - In current version only Class A nodes are implemented
        //  - The packets are inserted in queue as a FIFO scheme (i.e. no packet can be scheduled
        //    before packet referenced by 'm_pNextRealtimeLoraPacket')
        //  - The task loop MUST be adjusted to allow management of packets with absolute send time

        // Sanity check: 'm_pNextRealtimeLoraPacket' MUST be NULL when 'm_hPacketWaiting' semaphore is detected
        if (this->m_pNextRealtimeLoraPacket != NULL)
        {
          #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
            DEBUG_PRINT_LN("[ERROR] CLoraRealtimeSender_PacketSenderAutomaton - Packet signal inconsistency");
          #endif
        }

        if ((this->m_pNextRealtimeLoraPacket = CLoraRealtimeSender_GetNextRealtimePacket(this)) == NULL)
        {
          // Should never occur, the automaton was too slow to process scheduled packets
          // Maybe adjust 'LORAREALTIMESENDER_GATEWAY_TX_DELAY'
          #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
            DEBUG_PRINT_LN("[ERROR] CLoraRealtimeSender_PacketSenderAutomaton - Scheduled packet expired");
          #endif
        }
        else
        {
          // Send packet at the scheduled time
          pRealtimeLoraPacket = this->m_pNextRealtimeLoraPacket;
          if (pRealtimeLoraPacket->m_bASAP == false)
          {
            dwCurrentTicks = xTaskGetTickCount();
            vTaskDelay(pdMS_TO_TICKS(pRealtimeLoraPacket->m_dwSendTimestamp - (dwCurrentTicks * portTICK_RATE_MS)));
          }

          SendParams.m_pPacketToSend = pRealtimeLoraPacket->m_pPacketToSend;
          if ((bSendingPacket = ILoraTransceiver_Send(pRealtimeLoraPacket->m_pLoraTransceiverItf, &SendParams)) == true)
          {
            #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
              DEBUG_PRINT_LN("[INFO] CLoraRealtimeSender_PacketSenderAutomaton - LoRa packet currently sent by transceiver");
            #endif

            #if (LORAREALTIMESENDER_DEBUG_LEVEL2)
              DEBUG_PRINT("[DEBUG] CLoraRealtimeSender_PacketSenderAutomaton - transceiver sending... - ticks: ");
              DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
              DEBUG_PRINT_CR;
            #endif
          }
          else
          {
            // Should never occur
            // Maybe check if transceiver is not currently sending a previous packet (i.e. adjust schedule strategy)
            #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
              DEBUG_PRINT_LN("[ERROR] CLoraRealtimeSender_PacketSenderAutomaton - Transceiver cannot send LoRa packet");
            #endif
          }

          // Downlink packet transmission is started
          // Notify the parent 'LoraNodeManager'
          // Note: The 'LoraNodeManager' will be directly notified when packet is sent (i.e. event received on its
          //       'TransceiverAutomaton' task)
          SessionEvent.m_pSession = pRealtimeLoraPacket->m_pDownlinkSession;
          SessionEvent.m_dwSessionId = pRealtimeLoraPacket->m_dwDownlinkSessionId;  
          SessionEvent.m_wEventType = bSendingPacket == true ? TRANSCEIVERMANAGER_SESSIONEVENT_DOWNLINK_SENDING :
                                                                TRANSCEIVERMANAGER_SESSIONEVENT_DOWNLINK_FAILED;
          ITransceiverManager_SessionEvent(this->m_pTransceiverManagerItf, &SessionEvent);

          // Ready for next LoRa packet
          this->m_pNextRealtimeLoraPacket = NULL;
        }

        // Configure to trigger immediate cleanup if no more packet is registered in realtime queue
        dwSemaphoreWaitMs = 10;
      }
      else
      {
        // No packet waiting in realtime queue, performs some cleanup in collections 
        dwSemaphoreWaitMs = 500;
        CLoraRealtimeSender_RemoveExpiredNodeReceiveWindows(this);
      }
    }
    else if (this->m_dwCurrentState == LORAREALTIMESENDER_AUTOMATON_STATE_STOPPING)
    {
      // TO DO: cancel all programmed downlink sends
      this->m_dwCurrentState = LORAREALTIMESENDER_AUTOMATON_STATE_IDLE; 
    }
    else
    {
      // Other states:
      //  - Parent object not ready, wait for end of object's initialization
      //  - Object in 'INITIALIZED' or 'IDLE' state, wait for 'Start' command 
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }

  // Main automaton terminated (typically 'CLoraNodeManager' being deleted)
  vTaskDelete(NULL);
  this->m_hPacketSenderTask = NULL;
}


/*********************************************************************************************
  Construction

  Protected methods : must be called only object factory and 'ILoraRealtimeSender' interface
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         CLoraRealtimeSender * CLoraRealtimeSender_New()
 * 
 * @brief      Object construction.
 * 
 * @details    
 * 
 * @return     The function returns the pointer to the CLoraRealtimeSender instance.
 *
 * @note       This function only creates the object and its dependencies (RTOS objects).\n
*********************************************************************************************/
CLoraRealtimeSender * CLoraRealtimeSender_New()
{
  CLoraRealtimeSender *this;

#if LORAREALTIMESENDER_DEBUG_LEVEL2
  printf("CLoraRealtimeSender_New -> Debug level 2 (DEBUG)\n");
#elif LORAREALTIMESENDER_DEBUG_LEVEL1
  printf("CLoraRealtimeSender_New -> Debug level 1 (INFO)\n");
#elif LORAREALTIMESENDER_DEBUG_LEVEL0
  printf("CLoraRealtimeSender_New -> Debug level 0 (NORMAL)\n");
#endif 

  if ((this = (void *) pvPortMalloc(sizeof(CLoraRealtimeSender))) != NULL)
  {
    // The 'CLoraRealtimeSender' is under construction (i.e. embedded tasks must wait the 'INITIALIZED' state
    this->m_dwCurrentState = LORAREALTIMESENDER_AUTOMATON_STATE_CREATING;

    // Embedded objects are not defined (i.e. created below)
    this->m_pNodeReceiveWindowArray = NULL;
    this->m_pRealtimeLoraPacketArray = NULL;
    this->m_hPacketSenderTask = NULL;
    this->m_hPacketArrayMutex = NULL;
    this->m_hPacketWaiting = NULL;

    // Allocate memory blocks for internal collections
    if ((this->m_pNodeReceiveWindowArray = CMemoryBlockArray_New(sizeof(CNodeReceiveWindowOb),
        CONFIG_NODE_MAX_NUMBER)) == NULL)
    {
      CLoraRealtimeSender_Delete(this);
      return NULL;
    }

    if ((this->m_pRealtimeLoraPacketArray = CMemoryBlockArray_New(sizeof(CRealtimeLoraPacketOb),
        CONFIG_NODE_MAX_NUMBER)) == NULL)
    {
      CLoraRealtimeSender_Delete(this);
      return NULL;
    }

    if ((this->m_hPacketArrayMutex = xSemaphoreCreateMutex()) == NULL)
    {
      CLoraRealtimeSender_Delete(this);
      return NULL;
    }

    if ((this->m_hPacketWaiting = xSemaphoreCreateCounting(CONFIG_NODE_MAX_NUMBER, 0)) == NULL)
    {
      CLoraRealtimeSender_Delete(this);
      return NULL;
    }

    // Create PacketSender automaton task
    if (xTaskCreate((TaskFunction_t) CLoraRealtimeSender_PacketSenderAutomaton, "CLoraRealtimeSender_PacketSenderAutomaton", 
        2048, this, 5, &(this->m_hPacketSenderTask)) == pdFAIL)
    {
      CLoraRealtimeSender_Delete(this);
      return NULL;
    }

    // Initialize object's properties
    this->m_nRefCount = 0;
    this->m_pNextRealtimeLoraPacket = NULL;

    // Enter the 'CREATED' state
    this->m_dwCurrentState = LORAREALTIMESENDER_AUTOMATON_STATE_CREATED;
  }
  return this;
}

/*****************************************************************************************//**
 * @fn         void CLoraRealtimeSender_Delete(CLoraRealtimeSender *this)
 * 
 * @brief      Object destruction.
 * 
 * @details    Destroys the CLoraRealtimeSender object.\n
 *             The associated RTOS objects are destroyed and the memory used by CLoraRealtimeSender
 *             object are released.

 * @param      this
 *             The pointer to CLoraRealtimeSender object.
 *  
 * @return     None.
*********************************************************************************************/
void CLoraRealtimeSender_Delete(CLoraRealtimeSender *this)
{
  // Ask all tasks for termination
  // TO DO (also check how to delete task object)

  // Free memory
  if (this->m_pNodeReceiveWindowArray != NULL)
  {
    CMemoryBlockArray_Delete(this->m_pNodeReceiveWindowArray);
  }

  if (this->m_pRealtimeLoraPacketArray != NULL)
  {
    CMemoryBlockArray_Delete(this->m_pRealtimeLoraPacketArray);
  }

  if (this->m_hPacketArrayMutex != NULL)
  {
    vSemaphoreDelete(this->m_hPacketArrayMutex);
  }

  if (this->m_hPacketWaiting != NULL)
  {
    vSemaphoreDelete(this->m_hPacketWaiting);
  }
  
  vPortFree(this);
}




/*********************************************************************************************
  Private methods (implementation)

  Utility functions
*********************************************************************************************/


CNodeReceiveWindow CLoraRealtimeSender_FindNodeReceiveWindow(CLoraRealtimeSender *this, DWORD dwDeviceAddr, bool bCheckExpired)
{
  bool bEntryFound;
  CMemoryBlockArrayEnumItemOb EnumItem;
  CNodeReceiveWindowOb NodeReceiveWindow;
  DWORD dwCurrentTimestamp;

  // Enumerate the array containing 'CNodeReceiveWindow' objects for nodes with active RX windows
  EnumItem.m_pItemData = (void *) &NodeReceiveWindow;
  EnumItem.m_bByValue = true;
  bEntryFound = CMemoryBlockArray_EnumStart(this->m_pNodeReceiveWindowArray, &EnumItem);
  while (bEntryFound == true)
  {
    if (NodeReceiveWindow.m_dwDeviceAddr == dwDeviceAddr)
    {
      // The caller may ask to provide object only if not expired
      if ((bCheckExpired == true) && (NodeReceiveWindow.m_usDeviceClass == NODERECEIVEWINDOW_DEVICECLASS_A))
      {
        dwCurrentTimestamp = xTaskGetTickCount() * portTICK_RATE_MS;

        if (dwCurrentTimestamp > NodeReceiveWindow.m_dwRX2WindowTimestamp + 
            (LORAREALTIMESENDER_LORAWAN_RX_WINDOW_LENGTH - LORAREALTIMESENDER_GATEWAY_TX_DELAY))
        {
          #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
            DEBUG_PRINT_LN("[INFO] CLoraRealtimeSender_FindNodeReceiveWindow - Expired RX windows found for device");
          #endif
          return NULL;
        }
      }
      return (CNodeReceiveWindow) CMemoryBlockArray_BlockPtrFromIndex(this->m_pNodeReceiveWindowArray, EnumItem.m_usBlockIndex);
    }
    bEntryFound = CMemoryBlockArray_EnumNext(this->m_pNodeReceiveWindowArray, &EnumItem);
  }
  return NULL;
}


CRealtimeLoraPacket CLoraRealtimeSender_GetNextRealtimePacket(CLoraRealtimeSender *this)
{
  CMemoryBlockArrayEnumItemOb EnumItem;
  CRealtimeLoraPacketOb RealtimeLoraPacket;
  WORD wCount;
  bool bAsapFound;
  DWORD dwAsapTimestamp;
  BYTE usAsapEntryIndex;
  bool bAbsoluteFound;
  DWORD dwAbsoluteTimestamp;
  BYTE usAbsoluteEntryIndex;
  DWORD dwCurrentTimestamp;
  BYTE usEntryIndex;

  bAsapFound = bAbsoluteFound = false;
  usAbsoluteEntryIndex = usAsapEntryIndex = 0;     // Compiler error (should be warning)
  dwAbsoluteTimestamp = dwAsapTimestamp = 0;       // Compiler error (should be warning)
  wCount = 0;

  // Enumerate the array containing 'CRealtimeLoraPacket' for scheduled send
  EnumItem.m_pItemData = (void *) &RealtimeLoraPacket;
  EnumItem.m_bByValue = true;
  if (CMemoryBlockArray_EnumStart(this->m_pRealtimeLoraPacketArray, &EnumItem) == true)
  {
    do
    {
      // Retrieve the first packet to send:
      //  - For packet programmed with absolute time, select it if time is nearer than 'LORAREALTIMESENDER_GATEWAY_TX_DELAY'
      //    (i.e. duration required to send another packet before) or if there is no ASAP packet.
      //    If there is more than one absolute time packet the previous rules are applied to the first packet to send
      //    regarding programmed time.
      //  - For ASAP packet, select the first packet to send regarding maximum allowed time
      if (RealtimeLoraPacket.m_bASAP == true)
      {
        if (bAsapFound == true)
        {
          if (RealtimeLoraPacket.m_dwSendTimestamp >= dwAsapTimestamp)
          {
            continue;
          }
        }
        else
        {
          bAsapFound = true;
        }
        dwAsapTimestamp = RealtimeLoraPacket.m_dwSendTimestamp;
        usAsapEntryIndex = EnumItem.m_usBlockIndex;
      }
      else
      {
        if (bAbsoluteFound == true)
        {
          if (RealtimeLoraPacket.m_dwSendTimestamp >= dwAbsoluteTimestamp)
          {
            continue;
          }
        }
        else
        {
          bAbsoluteFound = true;
        }
        dwAbsoluteTimestamp = RealtimeLoraPacket.m_dwSendTimestamp;
        usAbsoluteEntryIndex = EnumItem.m_usBlockIndex;
      }
      wCount++;
    } 
    while (CMemoryBlockArray_EnumNext(this->m_pRealtimeLoraPacketArray, &EnumItem) == true);
  }
  else
  {
    // By design the array MUST contain at least one entry:
    //  - Each time an entry is inserted a 'count' semaphore is incremented
    //  - The 'SenderTask' waits on 'count' semaphore and invokes this function (i.e. one function call
    //    for each programmed packet)
    #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraRealtimeSender_GetNextRealtimePacket - FATAL: no entry in realtime queue");
    #endif
    return NULL;
  }

  // Check for too late
  // Current version assumes that automaton is fast enough too send all programmed packets and downlink 
  // packets with absolute time are scheduled by Network Server in order of arrival (should be OK because
  // this implementation is supporting only Class A devices)
  dwCurrentTimestamp = xTaskGetTickCount() * portTICK_RATE_MS;

  if ((bAsapFound == false) && (bAbsoluteFound == false))
  {
    // Should never occur, always something selected even if only expired objects in queue
    #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraRealtimeSender_GetNextRealtimePacket - FATAL: no entry selected in realtime queue");
    #endif
    return NULL;
  }

  if (((bAsapFound == true) && (dwAsapTimestamp < dwCurrentTimestamp)) ||
      ((bAbsoluteFound == true) && (dwAbsoluteTimestamp < dwCurrentTimestamp)))
  {
    // Invalid entry found
    // TO DO: update 'SenderTask' algorithme for cleanup of expired 'CRealtimeLoraPacket'
    #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CLoraRealtimeSender_GetNextRealtimePacket - Expired entry found realtime queue");
    #endif
  }
   
  // Select the next packet to send
  if ((bAbsoluteFound == false) || (dwAbsoluteTimestamp > dwCurrentTimestamp + LORAREALTIMESENDER_GATEWAY_TX_DELAY))
  {
    usEntryIndex = usAsapEntryIndex;
  }
  else
  {
    usEntryIndex = usAbsoluteEntryIndex;
  }

  return (CRealtimeLoraPacket) CMemoryBlockArray_BlockPtrFromIndex(this->m_pRealtimeLoraPacketArray, usEntryIndex);
}

void CLoraRealtimeSender_RemoveExpiredNodeReceiveWindows(CLoraRealtimeSender *this)
{
  CMemoryBlockArrayEnumItemOb EnumItem;
  DWORD dwCurrentTimestamp;

  // Enumerate the array containing 'CNodeReceiveWindow' objects for nodes with active RX windows
  dwCurrentTimestamp = xTaskGetTickCount() * portTICK_RATE_MS;

  EnumItem.m_bByValue = false;
  if (CMemoryBlockArray_EnumStart(this->m_pNodeReceiveWindowArray, &EnumItem) == true)
  {
    do
    {
      if (((CNodeReceiveWindow) EnumItem.m_pItemData)->m_usDeviceClass == NODERECEIVEWINDOW_DEVICECLASS_A)
      {
        if (dwCurrentTimestamp > ((CNodeReceiveWindow) EnumItem.m_pItemData)->m_dwRX2WindowTimestamp + 
            (LORAREALTIMESENDER_LORAWAN_RX_WINDOW_LENGTH - LORAREALTIMESENDER_GATEWAY_TX_DELAY))
        {
          #if (LORAREALTIMESENDER_DEBUG_LEVEL0)
            DEBUG_PRINT_LN("[INFO] CLoraRealtimeSender_RemoveExpiredNodeReceiveWindows - Removed expired RX windows");
          #endif
          CMemoryBlockArray_ReleaseBlock(this->m_pNodeReceiveWindowArray, EnumItem.m_usBlockIndex);
        }
      }
    } 
    while (CMemoryBlockArray_EnumNext(this->m_pNodeReceiveWindowArray, &EnumItem) == true);
  }
}

