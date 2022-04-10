/*****************************************************************************************//**
 * @file     SX1276.c
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     16/02/2018
 *
 * @brief    Low level driver for Semtech SX1276 chip.  
 *
 * @details  This program has been validated with Modtronix inAir9 module (version with
 *           Semtech SX1276 chip).
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

// The CSX1276 object implements the 'ILoraTransceiver' interface
#define LORATRANSCEIVERITF_IMPL

#include "LoraTransceiverItf.h"

// Object's definitions and methods
#include "SX1276.h"
         
    
/*********************************************************************************************
  Instantiate global static objects used by module implementation
*********************************************************************************************/

// 'ILoraTransceiver' interface function pointers
CLoraTransceiverItfImplOb g_LoraTransceiverItfImplOb = { .m_pAddRef = CSX1276_AddRef,
                                                         .m_pReleaseItf = CSX1276_ReleaseItf,
                                                         .m_pInitialize = CSX1276_Initialize,
                                                         .m_pSetLoraMAC = CSX1276_SetLoraMAC,
                                                         .m_pSetLoraMode = CSX1276_SetLoraMode,
                                                         .m_pSetPowerMode = CSX1276_SetPowerMode,
                                                         .m_pSetFreqChannel = CSX1276_SetFreqChannel,
                                                         .m_pStandBy = CSX1276_StandBy,
                                                         .m_pReceive = CSX1276_Receive,
                                                         .m_pSend = CSX1276_Send,
                                                         .m_pGetReceivedPacketInfo = CSX1276_GetReceivedPacketInfo
                                                       };

const double SignalBwLog[] =
{
  5.0969100130080564143587833158265,
  5.397940008672037609572522210551,
  5.6989700043360188047862611052755
};

    
    
/********************************************************************************************* 
  Public methods of CSX1276 object
 
  These methods are exposed on object's public interfaces
*********************************************************************************************/

/*********************************************************************************************
  Object instance factory
 
  The factory contains one method used to create a new object instance.
  This method provides the 'ILoraTransceiver' interface object for object's use and destruction.
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         ILoraTransceiver CSX1276_CreateInstance()
 * 
 * @brief      Creates a new instance of CSX1276 object.
 * 
 * @details    A new instance of CSX1276 object is created and its 'ILoraTransceiver' interface
 *             is returned. The owner object uses this interface to receive and send data 
 *             packets over LoRa radio. 
 * 
 * @return     A 'ILoraTransceiver' interface object.\n
 *             The reference count for returned 'ILoraTransceiver' interface is set to 1.
 *
 * @note       The CSX1276 object is created but the SX1276 hardware is not initialized (i.e.
 *             the radio modem is not configured and does not reeive/send packets).
 *             The 'ILoraTransceiver_Initialize' method must be called to configure the LoRa
 *             transceiver.
 *
 * @note       The CSX1276 object is destroyed when the last reference to 'ILoraTransceiver'
 *             is released (i.e. call to 'ILoraTransceiver_ReleaseItf' method).
*********************************************************************************************/
ILoraTransceiver CSX1276_CreateInstance()
{
  CSX1276 * pSX1276;

  // Create the object
  if ((pSX1276 = CSX1276_New(0)) != NULL)
  {
    // Create the 'ILoraTransceiver' interface object
    if ((pSX1276->m_pLoraTransceiverItf = ILoraTransceiver_New(pSX1276, &g_LoraTransceiverItfImplOb)) != NULL)
    {
      ++(pSX1276->m_nRefCount);
    }
    return pSX1276->m_pLoraTransceiverItf;
  }

  return NULL;
}

/*********************************************************************************************
  Public methods exposed on 'ILoraTransceiver' interface
 
  The static 'CLoraTransceiverItfImplOb' object is initialized with pointers to these functions.
  The static 'CLoraTransceiverItfImplOb' object is referenced in the 'ILoraTransceiver'
  interface provided by 'CreateInstance' method (object factory).
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         uint32_t CSX1276_AddRef(void *this)
 * 
 * @brief      Increments the object's reference count.
 * 
 * @details    This function increments object's global reference count.\n
 *             The reference count is used to track the number of existing external references
 *             to 'ILoraTransceiver' interface implemented by CSX1276 object.
 * 
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @return     The value of reference count once incremented.
*********************************************************************************************/
uint32_t CSX1276_AddRef(void *this)
{
  return ++((CSX1276 *)this)->m_nRefCount;
}

/*****************************************************************************************//**
 * @fn         uint32_t CSX1276_ReleaseItf(void *this)
 * 
 * @brief      Decrements the object's reference count.
 * 
 * @details    This function decrements object's global reference count and destroy the object
 *             when count reaches 0.\n
 *             The reference count is used to track the number of existing external references
 *             to 'ILoraTransceiver' interface implemented by CSX1276 object.
 * 
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @return     The value of reference count once decremented.
*********************************************************************************************/
uint32_t CSX1276_ReleaseItf(void *this)
{
  // Delete the object if its interface reference count reaches zero
  if (((CSX1276 *)this)->m_nRefCount == 1)
  {
    // TO DO -> Stop the object automaton which will delete the object on exit
    CSX1276_Delete((CSX1276 *)this);
    return 0;
  }
  return --((CSX1276 *)this)->m_nRefCount;
}

/*****************************************************************************************//**
 * @fn         bool CSX1276_Initialize(void *this, void *pParams)
 * 
 * @brief      Initializes the Semtech SX1276 chip.
 * 
 * @details    This function prepares the SX1276 modem for radio transmissions.\n
 *             The default LoRa parameters are set and the modem is waiting ready in 'StandBy'
 *             mode.
 * 
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @param      pParams
 *             The method parameters (see 'LoraTransceiverItf.h' for details).
 *
 * @return     The returned value is 'true' if the SX1276 chip is properly initialized or
 *             'false' in case of error.
*********************************************************************************************/
bool CSX1276_Initialize(void *this, void *pParams)
{
//  return CSX1276_NotifyAndProcessCommand((CSX1276 *) this, SX1276_AUTOMATON_CMD_INITIALIZE, pParams);


  // Debug
  DEBUG_PRINT_LN("[INFO] CSX1276_Initialize, calling CSX1276_NotifyAndProcessCommand");

  CSX1276_NotifyAndProcessCommand((CSX1276 *) this, SX1276_AUTOMATON_CMD_INITIALIZE, pParams);

  DEBUG_PRINT_LN("[INFO] CSX1276_Initialize, return from CSX1276_NotifyAndProcessCommand");

  return true;
}

/*****************************************************************************************//**
 * @fn         bool CSX1276_SetLoraMAC(void *this, void *pParams)
 * 
 * @brief      Set LoRa MAC configuration in the Semtech SX1276 chip.
 * 
 * @details    This function allows configuration of 'Preamble length', 'Sync Word', 'Header'
 *             and 'CRC' for LoRa MAC layer.\n
 *             Default values for LoRa MAC settings are defined during initialization. This
 *             configuration is suitable for use on standard LoRaWAN networks.
 * 
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @param      pParams
 *             The method parameters (see 'LoraTransceiverItf.h' for details).
 *
 * @return     The returned value is 'true' if the settings are properly applied in SX1276
 *             device or 'false' in case of error.
*********************************************************************************************/
bool CSX1276_SetLoraMAC(void *this, void *pParams)
{
  return CSX1276_NotifyAndProcessCommand((CSX1276 *) this, SX1276_AUTOMATON_CMD_SETLORAMAC, pParams);
}

/*****************************************************************************************//**
 * @fn         bool CSX1276_SetLoraMode(void *this, void *pParams)
 * 
 * @brief      Set LoRa modem configuration in the Semtech SX1276 chip.
 * 
 * @details    This function allows configuration of 'Spreading factor', 'Coding Rate' and
 *             'Bandwidth' for LoRa radio PHY layer.\n
 *             Predefined combinations (modes) are also available for standard configurations.
 * 
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @param      pParams
 *             The method parameters (see 'LoraTransceiverItf.h' for details).
 *
 * @return     The returned value is 'true' if the settings are properly applied in SX1276
 *             device or 'false' in case of error.
*********************************************************************************************/
bool CSX1276_SetLoraMode(void *this, void *pParams)
{
  return CSX1276_NotifyAndProcessCommand((CSX1276 *) this, SX1276_AUTOMATON_CMD_SETLORAMODE, pParams);
}

/*****************************************************************************************//**
 * @fn         bool CSX1276_SetPowerMode(void *this, void *pParams)
 * 
 * @brief      Set LoRa modem power configuration and over current protection in the Semtech
 *             SX1276 chip.
 * 
 * @details    This function allows configuration of radio power and power supply management
 *             of SX1276 device.
 *             Predefined radio power modes are also available for 2dBm, 6dBm and 14dBm
 *             settings.
 * 
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @param      pParams
 *             The method parameters (see 'LoraTransceiverItf.h' for details).
 *
 * @return     The returned value is 'true' if the settings are properly applied in SX1276
 *             device or 'false' in case of error.
*********************************************************************************************/
bool CSX1276_SetPowerMode(void *this, void *pParams)
{
  return CSX1276_NotifyAndProcessCommand((CSX1276 *) this, SX1276_AUTOMATON_CMD_SETPOWERMODE, pParams);
}

/*****************************************************************************************//**
 * @fn         bool CSX1276_SetFreqChannel(void *this, void *pParams)
 * 
 * @brief      Set LoRa frequency channel used by Semtech SX1276 chip for radio transmissions.
 * 
 * @details    This function allows the selection of the frequency channel to use for radio
 *             transmissions.\n
 *             The channel numbers are defined according to standard EU868 plan.
 * 
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @param      pParams
 *             The method parameters (see 'LoraTransceiverItf.h' for details).
 *
 * @return     The returned value is 'true' if the settings are properly applied in SX1276
 *             device or 'false' in case of error.
*********************************************************************************************/
bool CSX1276_SetFreqChannel(void *this, void *pParams)
{
  return CSX1276_NotifyAndProcessCommand((CSX1276 *) this, SX1276_AUTOMATON_CMD_SETFREQCHANNEL, pParams);
}

/*****************************************************************************************//**
 * @fn         bool CSX1276_StandBy(void *this, void *pParams)
 * 
 * @brief      Set radio modem of Semtech SX1276 chip in 'Standby' mode.
 * 
 * @details    This function switches the radio modem in 'StandBy' mode. When in this mode,
 *             the radio function of SX1276 device is disabled (i.e. no send or receive 
 *             activity).
 * 
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @param      pParams
 *             The method parameters (see 'LoraTransceiverItf.h' for details).
 *
 * @return     The returned value is 'true' if the SX1276 device is in 'StandBy' operating
 *             mode or 'false' in case of error.
*********************************************************************************************/
bool CSX1276_StandBy(void *this, void *pParams)
{
  return CSX1276_NotifyAndProcessCommand((CSX1276 *) this, SX1276_AUTOMATON_CMD_STANDBY, pParams);
}

/*****************************************************************************************//**
 * @fn         bool CSX1276_Receive(void *this, void *pParams)
 * 
 * @brief      Set radio modem of Semtech SX1276 chip in 'Continuous Receive' mode.
 * 
 * @details    This function configures the radio modem for continuous receive. When in this
 *             mode, the device receives LoRa packets (according to its PHY radio settings)
 *             and send them to owner object (using the RTOS Queue configured via
 *             'LoraTransceiverItf' interface).
 * 
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @param      pParams
 *             The method parameters (see 'LoraTransceiverItf.h' for details).
 *
 * @return     The returned value is 'true' if the SX1276 device is in 'Receive' operating
 *             mode or 'false' in case of error.
*********************************************************************************************/
bool CSX1276_Receive(void *this, void *pParams)
{
  return CSX1276_NotifyAndProcessCommand((CSX1276 *) this, SX1276_AUTOMATON_CMD_RECEIVE, pParams);
}

/*****************************************************************************************//**
 * @fn         bool CSX1276_Send(void *this, void *pParams)
 * 
 * @brief      Asks the Semtech SX1276 chip to send a specified LoRa packet.
 * 
 * @details    This function transfers the specified LoRa packet in SX1276 device and activates
 *             the 'Send' operating mode.\n
 *             The SX1276 device sends the LoRa packet and returns to 'StandBy' mode.\n
 *             When the packet transmission terminates, a notification is sent to owner objet
 *             via 'LoraTransceiverItf' interface. Then, owner object can send another packet, 
 *             enter the 'Receive' state or stay in 'StandBy'.
 * 
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @param      pParams
 *             The method parameters (see 'LoraTransceiverItf.h' for details).
 *
 * @return     The returned value is 'true' if the SX1276 device is sending the LoRa packet
 *             or 'false' in case of error.
*********************************************************************************************/
bool CSX1276_Send(void *this, void *pParams)
{
  return CSX1276_NotifyAndProcessCommand((CSX1276 *) this, SX1276_AUTOMATON_CMD_SEND, pParams);
}


/*****************************************************************************************//**
 * @fn         bool CSX1276_GetReceivedPacketInfo(void *this, void *pParams)
 * 
 * @brief      Retrieve additional information for last received LoRa packet.
 * 
 * @details    This function copies in caller memory block the information associated to last
 *             received LoRa packet.\n
 *             The additional information for a received LoRa packet is available until the
 *             client object has not confirmed the transfer of received packet (see synchronization
 *             mechanism in 'LoraTransceiverItf' interface specification).
 * 
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @param      pParams
 *             The method parameters (see 'LoraTransceiverItf.h' for details).
 *
 * @return     The returned value is 'true' if the information for last received LoRa packet
 *             is provided or 'false' in case of error.
*********************************************************************************************/
bool CSX1276_GetReceivedPacketInfo(void *this, void *pParams)
{
  // Simple copy of last received packet info stored in CSX1276 object
  // 
  // Note: 
  //  - This function is synchronous (i.e. possible concurrency for client task and packet
  //    received IRQ in CSX1276 object for a new packet)
  //  - There is no protection here because the packet information in CSX1276 is written
  //    only after client object has confirmed the retrieval of LoRa packet (and so the
  //    associated additional information if required)

  #if (SX1276_DEBUG_LEVEL2)
    CReceivedLoraPacketInfo *pReceivedPacketInfo = &((CSX1276 *) this)->m_ReceivedPacketInfo;
    DEBUG_PRINT("[DEBUG] CSX1276_GetReceivedPacketInfo - Timestamp: ");
    DEBUG_PRINT_DEC(pReceivedPacketInfo->m_dwUTCSec);
    DEBUG_PRINT(", Freq: ");
    DEBUG_PRINT(pReceivedPacketInfo->m_szFrequency);
    DEBUG_PRINT(", DataRate: ");
    DEBUG_PRINT(pReceivedPacketInfo->m_szDataRate);
    DEBUG_PRINT(", SNR: ");
    DEBUG_PRINT(pReceivedPacketInfo->m_szSNR);
    DEBUG_PRINT(", RSSI: ");
    DEBUG_PRINT_LN(pReceivedPacketInfo->m_szRSSI);
  #endif
        
  memcpy(((CLoraTransceiverItf_GetReceivedPacketInfoParams) pParams)->m_pPacketInfo,
         &((CSX1276 *) this)->m_ReceivedPacketInfo, sizeof(CLoraTransceiverItf_ReceivedLoraPacketInfoOb));
  return true;
}


/********************************************************************************************* 
  Private methods of CSX1276 object
 
  The following methods CANNOT be called by another object
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         bool CSX1276_NotifyAndProcessCommand(CSX1276 *this, DWORD dwCommand, void *pCmdParams)
 * 
 * @brief      Process a command issued by a method of 'ILoraTransceiver' interface.
 * 
 * @details    The CSX1276 main automaton asynchronously executes the commands generated by
 *             calls on 'ILoraTransceiver' interface methods.\n
 *             This method transmits commands to main automaton and wait for end of execution
 *             (i.e. from client point of view, the interface method is synchronous).
 * 
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @param      dwCommand
 *             The command to execute (typically same name than corresponding method on
 *             'ILoraTransmiter' interface. See 'SX1276_AUTOMATON_CMD_xxx' in SX1276.h.
 *  
 * @param      pCmdParams
 *             A pointer to command parameters. The object pointed by 'pCmdParams' depends
 *             on method invoked on 'ILoraTransmiter' interface. 
 *             See 'LoraTransceiverItf.h'.
 *  
 * @return     The returned value is 'true' if the command is properly executed or 'false'
 *             if command execution has failed or is still pending.
 *
 * @note       This function assumes that commands are quickly processed by main automaton.\n
 *             The maximum execution time can be configured and a mechanism is implemented in
 *             order to ignore client commands sent when a previous command is still pending.
*********************************************************************************************/
bool CSX1276_NotifyAndProcessCommand(CSX1276 *this, DWORD dwCommand, void *pCmdParams)
{
  // Automaton commands are serialized (and should be quickly processed)
  // Note: In current design, there is only one client object for a single CSX1276 instance
  //       (the 'CLoraNodeManager'). In other words, commands are serialized here and there
  //       is no concurrency on the 'CSX1276_ProcessCommand' method.
  if (xSemaphoreTake(this->m_hCommandMutex, pdMS_TO_TICKS(SX1276_AUTOMATON_MAX_CMD_DURATION)) == pdFAIL)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CSX1276_ProcessCommand - Failed to take mutex");
    #endif

    return false;
  }

  // Make sure that previous command has been processed by the automaton
  if (this->m_dwCommand != SX1276_AUTOMATON_CMD_NONE)
  {
    // Previous call to this function has returned before end of command's execution
    // Check if done now
    if (xSemaphoreTake(this->m_hCommandDone, 0) == pdFAIL)
    {
      // Still not terminated
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] CSX1276_ProcessCommand - Previous command still pending");
      #endif

      xSemaphoreGive(this->m_hCommandMutex);
      return false;
    }
  }

  // Post the command to main automaton
  this->m_dwCommand = dwCommand;
  this->m_pCommandParams = pCmdParams;
  xTaskNotify(this->m_hAutomatonTask, SX1276_AUTOMATON_NOTIFY_COMMAND, eSetBits);

  // Wait for command execution by main automaton
  BaseType_t nCommandDone = xSemaphoreTake(this->m_hCommandDone, pdMS_TO_TICKS(SX1276_AUTOMATON_MAX_CMD_DURATION -
                                           (SX1276_AUTOMATON_MAX_CMD_DURATION / 5)));

  // If the command has been processed, clear 'm_dwCommand' attribute
  if (nCommandDone == pdPASS)
  {
    this->m_dwCommand = SX1276_AUTOMATON_CMD_NONE;
  }
  else
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CSX1276_ProcessCommand - Exiting before end of command execution");
    #endif
  }

  // Next command allowed
  xSemaphoreGive(this->m_hCommandMutex);

  return nCommandDone == pdPASS ? true : false;
}


/********************************************************************************************* 
  Main automaton
 
  The CSX1276 implements an RTOS 'Task' which executes the object's automaton
*********************************************************************************************/

/********************************************************************************************* 
  RTOS task function
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         void CSX1276_MainAutomaton(CSX1276 *this)
 * 
 * @brief      Executes the CSX1276 main automaton.
 * 
 * @details    This function is the RTOS task implementing the CSX1276 main automaton.\n
 *             The main automaton process the commands received from 'ILoraTransceiver'
 *             interface and the events sent by Semtech SX1276 chip.
 * 
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @return     None. The RTOS task terminates when object is deleted (typically on main program
 *             exit).
 *
 * @note       The automaton's main loop waits for events received through a bit flag variable.
 *             The commands and IRQs are sent using this notification variable.
*********************************************************************************************/
void CSX1276_MainAutomaton(CSX1276 *this)
{
  DWORD dwNotificationFlags;

  while (this->m_dwCurrentState != SX1276_AUTOMATON_STATE_TERMINATED)
  {
    // Wait for events
    // Rem: 'dwNotificationFlags' cleared on 'xTaskNotifyWait' exit
    if (xTaskNotifyWait(0, 0xFFFFFFFF, &dwNotificationFlags, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      // Process any events signaled in 'dwNotificationFlags'
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_CR;
        DEBUG_PRINT("CSX1276_MainAutomaton, notify received: ");
        DEBUG_PRINT_HEX(dwNotificationFlags);
        DEBUG_PRINT_CR;
      #endif

      // 1- Check and process 'Command'
      if (dwNotificationFlags & SX1276_AUTOMATON_NOTIFY_COMMAND)
      {
        // A 'Command' is waiting for processing (i.e. launched via 'ILoraTransceiver' interface)
        // Note: commands are serialized by design and only one command is associated to this signal
        CSX1276_ProcessAutomatonNotifyCommand(this);

        // Command executed, release calling task (i.e. command execution is synchronous)
        xSemaphoreGive(this->m_hCommandDone);
      }

      // 2- Check and process 'Packet Received' hardware interrupt raised by SX1276
      if (dwNotificationFlags & SX1276_AUTOMATON_NOTIFY_PACKET_RECEIVED)
      {
        CSX1276_ProcessAutomatonNotifyPacketReceived(this);
      }

      // 3- Check and process 'Packet Sent' hardware interrupt raised by SX1276
      if (dwNotificationFlags & SX1276_AUTOMATON_NOTIFY_PACKET_SENT)
      {
        CSX1276_ProcessAutomatonNotifyPacketSent(this);
      }
    }
    else
    {
      DEBUG_PRINT_LN("CSX1276_MainAutomaton, waiting notify");
    }
  }

  // Main automaton terminated (typically 'CSX1276' being deleted)
  vTaskDelete(NULL);
  this->m_hAutomatonTask = NULL;
}



/*********************************************************************************************
  Construction

  Protected methods : must be called only object factory and 'IloraTransceiver' interface
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         CSX1276 * CSX1276_New()
 * 
 * @brief      Object construction.
 * 
 * @details    This function is the RTOS task implementing the CSX1276 main automaton.\n
 *             The main automaton process the commands received from 'ILoraTransceiver'
 *             interface and the events sent by Semtech SX1276 chip.
 * 
 * @return     The function returns the pointer to the CSX1276 instance.
 *
 * @note       This function only creates the object and its dependencies (RTOS objects).\n
 *             The hardware interface with associated SX1276 chip is not initialized (i.e.
 *             SPI link not configured).
*********************************************************************************************/
CSX1276 * CSX1276_New()
{
  CSX1276 *this;

#if SX1276_DEBUG_LEVEL2
  printf("CSX1276_New -> Debug level 2 (DEBUG)\n");
#elif SX1276_DEBUG_LEVEL1
  printf("CSX1276_New -> Debug level 1 (INFO)\n");
#elif SX1276_DEBUG_LEVEL0
  printf("CSX1276_New -> Debug level 0 (NORMAL)\n");
#endif 

  if ((this = (void *) pvPortMalloc(sizeof(CSX1276))) != NULL)
  {
    // Allocate memory blocks for data transfers
    this->m_pPacketReceived = this->m_hCommandMutex = this->m_hCommandDone = 
      this->m_hAutomatonTask = this->m_hPacketReceivedIntOb = NULL;

    if ((this->m_pPacketReceived = (CLoraPacket *) pvPortMalloc(sizeof(CLoraPacket))) == NULL)
    {
      CSX1276_Delete(this);
      return NULL;
    }

    if ((this->m_hCommandMutex = xSemaphoreCreateMutex()) == NULL)
    {
      CSX1276_Delete(this);
      return NULL;
    }

    if ((this->m_hCommandDone = xSemaphoreCreateBinary()) == NULL)
    {
      CSX1276_Delete(this);
      return NULL;
    }

    // Create main automaton task
    if (xTaskCreate((TaskFunction_t) CSX1276_MainAutomaton, "CSX1276_Automaton", 4096, this, 5, &(this->m_hAutomatonTask)) == pdFAIL)
    {
      CSX1276_Delete(this);
      return NULL;
    }

    // Initialize GPIO interrupt handler
    if (gpio_install_isr_service(ESP_INTR_FLAG_IRAM) != ESP_OK)
    {
      CSX1276_Delete(this);
      return NULL;
    }

    // Initialize object's properties
    this->m_nRefCount = 0;
    this->m_dwCommand = SX1276_AUTOMATON_CMD_NONE;

    this->m_usBandwidth = SX1276_BW_UNDEFINED;
    this->m_usCodingRate = SX1276_CR_UNDEFINED;
    this->m_usSpreadingFactor = SX1276_SF_UNDEFINED;
    this->m_usFreqChannel = SX1276_FREQ_CH_UNDEFINED;
    this->m_dwRegFreqChannel = SX1276_REG_CH_UNDEFINED;
    this->m_usLoraMode = SX1276_LORAMODE_UNDEFINED;
    this->m_usHeader = SX1276_HEADER_UNDEFINED;
    this->m_usCRC = SX1276_CRC_UNDEFINED;
    this->m_usModemMode = MODEM_MODE_UNKNOWN;
    this->m_usPowerLevel = SX1276_POWER_LEVEL_UNDEFINED;
    this->m_usPowerMode = SX1276_POWER_MODE_UNDEFINED;
    this->m_usOcpRate = SX1276_OCP_UNDEFINED;
    this->m_wPreambleLength = SX1276_PREAMBLE_LENGTH_UNDEFINED;
    this->m_usSyncWord = SX1276_SYNCWORD_UNDEFINED;
    this->m_dwPacketReceivedNumber = 0;
    this->m_dwMissedPacketReceivedNumber = 0;
    this->m_dwPacketSentNumber = 0;
    this->m_pPacketToSend = NULL;
    this->m_usRetries = 0;
    this->m_usMaxRetries = 3;
    this->m_usSpiSlaveID = 0; //SPISlaveID;
    this->m_SpiDeviceHandle = NULL;

    this->m_hEventNotifyQueue = NULL;

    this->m_ReceivedPacketInfo.m_szDataRate[0] = 0;
    this->m_ReceivedPacketInfo.m_szFrequency[0] = 0;
    this->m_ReceivedPacketInfo.m_szRSSI[0] = 0;
    this->m_ReceivedPacketInfo.m_szSNR[0] = 0;

    // Enter the 'CREATED' state
    this->m_dwCurrentState = SX1276_AUTOMATON_STATE_CREATED;
  }
  return this;
}

/*****************************************************************************************//**
 * @fn         void CSX1276_Delete(CSX1276 *this)
 * 
 * @brief      Object destruction.
 * 
 * @details    Destroys the CSX1246 object.\n
 *             The associated RTOS objects are destroyed and the memory used by CSX176 object
 *             is released.

 * @param      this
 *             The pointer to CSX1246 object.
 *  
 * @return     None.
*********************************************************************************************/
void CSX1276_Delete(CSX1276 *this)
{
  if (this->m_pPacketReceived != NULL) 
  {
    vPortFree(this->m_pPacketReceived);
  }
  if (this->m_pLoraTransceiverItf != NULL)
  {
    ILoraTransceiver_Delete(this->m_pLoraTransceiverItf);
  }
  if (this->m_hCommandMutex != NULL)
  {
    vSemaphoreDelete(this->m_hCommandMutex);
  }
  if (this->m_hCommandDone != NULL)
  {
    vSemaphoreDelete(this->m_hCommandDone);
  }

  gpio_uninstall_isr_service();

  // Ask main automaton for termination
  // TO DO (also check how to delete task object)

  vPortFree(this);
}


/*********************************************************************************************
  Private methods (implementation)

  Command processing

  These functions are called by main automaton when the 'SX1276_AUTOMATON_NOTIFY_COMMAND'
  notification is received.

  Note: These functions may change automaton state. There is no protection against concurrency
        for automaton state because only functions called from automaton RTOS task are allowed
        to modify state (by design).
*********************************************************************************************/



/*****************************************************************************************//**
 * @fn         bool CSX1276_ProcessAutomatonNotifyCommand(CSX1276 *this)
 * 
 * @brief      Process 'ILoraTransceiver' command currently waiting in automaton.
 * 
 * @details    This function is invoked by main automaton when it receives a 'COMMAND' notification.\n
 *             These commands are issued via calls on 'ILoraTransceiver' interface.\n
 *             The command and its parameters are available in member variables of CSX1276 object.\n
 *             By design, commands are serialized when transmited to CSX1276 automaton. In other
 *             words, the processing of one command cannot be interrupted by the reception of
 *             another command.
 * 
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @return     The returned value is 'true' if command execution is successful or 'false' in
 *             case of error.
*********************************************************************************************/
bool CSX1276_ProcessAutomatonNotifyCommand(CSX1276 *this)
{
  switch (this->m_dwCommand)
  {
    case SX1276_AUTOMATON_CMD_INITIALIZE:
      return CSX1276_ProcessInitialize(this, (CLoraTransceiverItf_InitializeParams) this->m_pCommandParams);

    case SX1276_AUTOMATON_CMD_SETLORAMAC:
      return CSX1276_ProcessSetLoraMAC(this, (CLoraTransceiverItf_SetLoraMACParams) this->m_pCommandParams);

    case SX1276_AUTOMATON_CMD_SETLORAMODE:
      return CSX1276_ProcessSetLoraMode(this, (CLoraTransceiverItf_SetLoraModeParams) this->m_pCommandParams);

    case SX1276_AUTOMATON_CMD_SETPOWERMODE:
      return CSX1276_ProcessSetPowerMode(this, (CLoraTransceiverItf_SetPowerModeParams) this->m_pCommandParams);

    case SX1276_AUTOMATON_CMD_SETFREQCHANNEL:
      return CSX1276_ProcessSetFreqChannel(this, (CLoraTransceiverItf_SetFreqChannelParams) this->m_pCommandParams);

    case SX1276_AUTOMATON_CMD_STANDBY:
      return CSX1276_ProcessStandBy(this, (CLoraTransceiverItf_StandByParams) this->m_pCommandParams);

    case SX1276_AUTOMATON_CMD_RECEIVE:
      return CSX1276_ProcessReceive(this, (CLoraTransceiverItf_ReceiveParams) this->m_pCommandParams);

    case SX1276_AUTOMATON_CMD_SEND:
      return CSX1276_ProcessSend(this, (CLoraTransceiverItf_SendParams) this->m_pCommandParams);

    default:
      break;
  }
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[ERROR] CSX1276_ProcessAutomatonNotifyCommand, unknown command");
  #endif
  return false;
}


/*****************************************************************************************//**
 * @fn         bool CSX1276_ProcessInitialize(CSX1276 *this, 
 *                                            CLoraTransceiverItf_InitializeParams pParams)
 * 
 * @brief      Initializes the Semtech SX1276 chip.
 * 
 * @details    This function prepares the SX1276 modem for radio transmissions.\n
 *             The modem is set for LoRa radio and the 'StandBy' mode is entered.\n
 *             Whole LoRa configuration is possible using settings provided in 'pParams'.\n
 *             This function must be called one time in '' automaton state.\n
 *             On exit, depending of provided settings, possible automaton states are:\n
 *              - 'SX1276_AUTOMATON_STATE_INITIALIZED' = the LoRa radio configuration must
 *                 be completed to set SX1276 ready for transmission 
 *              - 'SX1276_AUTOMATON_STATE_STANDBY' = all LoRa radio settings are defined
 *                 and SX1276 is ready for transmission
 * 
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @param      pParams
 *             The interface method parameters (see 'LoraTransceiverItf.h' for details).
 *
 * @return     The returned value is 'true' if the SX1276 chip is properly initialized or
 *             'false' in case of error.
*********************************************************************************************/
bool CSX1276_ProcessInitialize(CSX1276 *this, CLoraTransceiverItf_InitializeParams pParams)
{
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CSX1276_ProcessInitialize'");
  #endif

  // The 'Initialize' method is allowed only in 'CREATED' automaton state
  if (this->m_dwCurrentState != SX1276_AUTOMATON_STATE_CREATED)
  {
    // By design, should never occur
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  // Step 1: Initialize SPI link and start LoRa in 'Standby' (with chip default configuration)
  // TO DO = Manage SPI Slave ID
  if (CSX1276_InitializeDevice(this, 0, pParams) != LORATRANSCEIVERITF_RESULT_SUCCESS)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Failed to initialize device");
    #endif
    return false;
  }

  // Set the queue to use for event notifications
  this->m_hEventNotifyQueue = pParams->m_hEventNotifyQueue ; 
  
  // Enter 'INITIALIZED' state if current state is still 'CREATED'
  // Note: By design, no concurrency on automaton state variable
  if (this->m_dwCurrentState == SX1276_AUTOMATON_STATE_CREATED)
  {
    this->m_dwCurrentState = SX1276_AUTOMATON_STATE_INITIALIZED;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] CSX1276 automaton state changed: 'INITIALIZED'");
    #endif
  }

  // Step 2: Additionnal configuration
  
  // Configure Lora Mode if settings provided
  if (pParams->pLoraMode != NULL)
  {
    if (CSX1276_ProcessSetLoraMode(this, pParams->pLoraMode) == false)
    {
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set Lora Mode");
      #endif
      return false;
    }
  }

  // Configure Lora MAC if settings provided
  if (pParams->pLoraMAC != NULL)
  {
    if (CSX1276_ProcessSetLoraMAC(this, pParams->pLoraMAC) == false)
    {
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set Lora MAC");
      #endif
      return false;
    }
  }

  // Configure Frequency Channel if settings provided
  if (pParams->pFreqChannel != NULL)
  {
    if (CSX1276_ProcessSetFreqChannel(this, pParams->pFreqChannel) == false)
    {
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set Frequency Channel");
      #endif
      return false;
    }
  }

  // Configure Power Mode if settings provided
  if (pParams->pPowerMode != NULL)
  {
    if (CSX1276_ProcessSetPowerMode(this, pParams->pPowerMode) == false)
    {
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set Power Mode");
      #endif
      return false;
    }
  }

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] SX1276 successfully initialized for LoRA");
  #endif
  return true;
}

// Note: 
//  - The 'InitializeDevice' method sets the LoRa MAC configuration for use on LoRaWAN public networks 
//  (setting values according to LoRa specification).
//  - The LoRa MAC configuration is required only for some kinds of private networks.

bool CSX1276_ProcessSetLoraMAC(CSX1276 *this, CLoraTransceiverItf_SetLoraMACParams pParams)
{
  uint8_t resultCode;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CSX1276_ProcessSetLoraMAC'");
  #endif

  // The 'SetLoraMAC' method is allowed only in 'INITIALIZED' and 'STANDBY' automaton states
  if ((this->m_dwCurrentState != SX1276_AUTOMATON_STATE_INITIALIZED) && 
      (this->m_dwCurrentState != SX1276_AUTOMATON_STATE_STANDBY))
  {
    // By design, should never occur
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  if ((pParams->m_usSyncWord != LORATRANSCEIVERITF_SYNCWORD_NONE) && 
      ((pParams->m_usSyncWord != this->m_usSyncWord) || (pParams->m_bForce == true)))
  {
    if (CSX1276_setSyncWord(this, pParams->m_usSyncWord) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set Sync Word");
      #endif
      return false;
    }
  }

  if ((pParams->m_wPreambleLength != LORATRANSCEIVERITF_PREAMBLE_LENGTH_NONE) && 
      ((pParams->m_wPreambleLength != this->m_wPreambleLength) || (pParams->m_bForce == true)))
  {
    if (CSX1276_setPreambleLength(this, pParams->m_wPreambleLength) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set Preamble Length");
      #endif
      return false;
    }
  }

  if ((pParams->m_usHeader != LORATRANSCEIVERITF_HEADER_NONE) && 
      ((pParams->m_usHeader != this->m_usHeader) || (pParams->m_bForce == true)))
  {
    resultCode = (pParams->m_usHeader == LORATRANSCEIVERITF_HEADER_ON ?
                  CSX1276_setHeaderON(this) : CSX1276_setHeaderOFF(this));
    if (resultCode != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set Header ON/OFF");
      #endif
      return false;
    }
  }

  if ((pParams->m_usCRC != LORATRANSCEIVERITF_CRC_NONE) && 
      ((pParams->m_usCRC != this->m_usCRC) || (pParams->m_bForce == true)))
  {
    resultCode = (pParams->m_usCRC == LORATRANSCEIVERITF_CRC_ON ?
                  CSX1276_setCRC_ON(this) : CSX1276_setCRC_OFF(this));
    if (resultCode != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set CRC ON/OFF");
      #endif
      return false;
    }
  }

  // If current automaton state is 'INITIALIZED', check if all required settings have been explicitly
  // provided by client object. In this case, the SX1276 is ready for radio transmission and 'STANDY'
  // automaton state is entered
  // Note: By design, no concurrency on automaton state variable
  if ((this->m_dwCurrentState == SX1276_AUTOMATON_STATE_INITIALIZED) && (CSX1276_IsDeviceConfigured(this) == true))
  {
    this->m_dwCurrentState = SX1276_AUTOMATON_STATE_STANDBY;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] CSX1276 automaton state changed: 'STANDBY'");
    #endif
  }
  return true;
}

bool CSX1276_ProcessSetLoraMode(CSX1276 *this, CLoraTransceiverItf_SetLoraModeParams pParams)
{
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CSX1276_ProcessSetLoraMode'");
  #endif

  // The 'SetLoraMode' method is allowed only in 'INITIALIZED' and 'STANDBY' automaton states
  if ((this->m_dwCurrentState != SX1276_AUTOMATON_STATE_INITIALIZED) && 
      (this->m_dwCurrentState != SX1276_AUTOMATON_STATE_STANDBY))
  {
    // By design, should never occur
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  if ((pParams->m_usLoraMode != LORATRANSCEIVERITF_LORAMODE_NONE) &&
      ((pParams->m_usLoraMode != this->m_usLoraMode) || (pParams->m_bForce == true)))
  {
    if (CSX1276_setLoRaMode(this, pParams->m_usLoraMode) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set Lora Mode");
      #endif
      return false;
    }
  }

  if ((pParams->m_usCodingRate != LORATRANSCEIVERITF_CR_NONE) &&
      ((pParams->m_usCodingRate != this->m_usCodingRate) || (pParams->m_bForce == true)))
  {
    if (CSX1276_setCR(this, pParams->m_usCodingRate) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set Coding Rate");
      #endif
      return false;
    }
  }

  if ((pParams->m_usSpreadingFactor != LORATRANSCEIVERITF_SF_NONE) &&
      ((pParams->m_usSpreadingFactor != this->m_usSpreadingFactor) || (pParams->m_bForce == true)))
  {
    if (CSX1276_setSF(this, pParams->m_usSpreadingFactor) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set Spreading Factor");
      #endif
      return false;
    }
  }

  if ((pParams->m_usBandwidth != LORATRANSCEIVERITF_BANDWIDTH_NONE) &&
      ((pParams->m_usBandwidth != this->m_usBandwidth) || (pParams->m_bForce == true)))
  {
    if (CSX1276_setBW(this, pParams->m_usBandwidth) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set Bandwidth");
      #endif
      return false;
    }
  }

  // Store current SpreadingFactor and Bandwidth information (i.e. text value retrieved by client object
  // when a LoraPacket is received)
  sprintf((char *) this->m_ReceivedPacketInfo.m_szDataRate, "SF%dBW", (int) this->m_usSpreadingFactor);
  switch (this->m_usBandwidth)
  {
    case LORATRANSCEIVERITF_BANDWIDTH_125:
      strcat((char *) this->m_ReceivedPacketInfo.m_szDataRate, "125");
      break;
    case LORATRANSCEIVERITF_BANDWIDTH_250:
      strcat((char *) this->m_ReceivedPacketInfo.m_szDataRate, "250");
      break;
    case LORATRANSCEIVERITF_BANDWIDTH_500:
      strcat((char *) this->m_ReceivedPacketInfo.m_szDataRate, "500");
      break;
    default:
      strcat((char *) this->m_ReceivedPacketInfo.m_szDataRate, "?");
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Unable to generate datarate string");
      #endif
  }

  #if (SX1276_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CSX1276_ProcessSetLoraMode - Data Rate: ");
    DEBUG_PRINT_LN((char *) this->m_ReceivedPacketInfo.m_szDataRate);
  #endif


  // If current automaton state is 'INITIALIZED', check if all required settings have been explicitly
  // provided by client object. In this case, the SX1276 is ready for radio transmission and 'STANDY'
  // automaton state is entered
  // Note: By design, no concurrency on automaton state variable
  if ((this->m_dwCurrentState == SX1276_AUTOMATON_STATE_INITIALIZED) && (CSX1276_IsDeviceConfigured(this) == true))
  {
    this->m_dwCurrentState = SX1276_AUTOMATON_STATE_STANDBY;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] CSX1276 automaton state changed: 'STANDBY'");
    #endif
  }
  return true;
}

bool CSX1276_ProcessSetPowerMode(CSX1276 *this, CLoraTransceiverItf_SetPowerModeParams pParams)
{
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CSX1276_ProcessSetPowerMode'");
  #endif

  // The 'SetPowerMode' method is allowed only in 'INITIALIZED' and 'STANDBY' automaton states
  if ((this->m_dwCurrentState != SX1276_AUTOMATON_STATE_INITIALIZED) && 
      (this->m_dwCurrentState != SX1276_AUTOMATON_STATE_STANDBY))
  {
    // By design, should never occur
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  if ((pParams->m_usPowerMode != LORATRANSCEIVERITF_POWER_MODE_NONE) &&
      ((pParams->m_usPowerMode != this->m_usPowerMode) || (pParams->m_bForce == true)))
  {
    if (CSX1276_setPowerMode(this, pParams->m_usPowerMode) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set Power Mode");
      #endif
      return false;
    }
  }

  if ((pParams->m_usPowerLevel != LORATRANSCEIVERITF_POWER_LEVEL_NONE) &&
      ((pParams->m_usPowerLevel != this->m_usPowerLevel) || (pParams->m_bForce == true)))
  {
    if (CSX1276_setPowerLevel(this, pParams->m_usPowerLevel) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set Power Level");
      #endif
      return false;
    }
  }

  if ((pParams->m_usOcpRate != LORATRANSCEIVERITF_OCP_NONE) &&
      ((pParams->m_usOcpRate != this->m_usOcpRate) || (pParams->m_bForce == true)))
  {
    if (CSX1276_setMaxCurrent(this, pParams->m_usOcpRate) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set Max Current (OCP)");
      #endif
      return false;
    }
  }

  // If current automaton state is 'INITIALIZED', check if all required settings have been explicitly
  // provided by client object. In this case, the SX1276 is ready for radio transmission and 'STANDY'
  // automaton state is entered
  // Note: By design, no concurrency on automaton state variable
  if ((this->m_dwCurrentState == SX1276_AUTOMATON_STATE_INITIALIZED) && (CSX1276_IsDeviceConfigured(this) == true))
  {
    this->m_dwCurrentState = SX1276_AUTOMATON_STATE_STANDBY;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] CSX1276 automaton state changed: 'STANDBY'");
    #endif
  }
  return true;
}

bool CSX1276_ProcessSetFreqChannel(CSX1276 *this, CLoraTransceiverItf_SetFreqChannelParams pParams)
{
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CSX1276_ProcessSetFreqChannel'");
  #endif

  // The 'SetFreqChannel' method is allowed only in 'INITIALIZED' and 'STANDBY' automaton states
  if ((this->m_dwCurrentState != SX1276_AUTOMATON_STATE_INITIALIZED) && 
      (this->m_dwCurrentState != SX1276_AUTOMATON_STATE_STANDBY))
  {
    // By design, should never occur
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  if ((pParams->m_usFreqChannel != LORATRANSCEIVERITF_FREQUENCY_CHANNEL_NONE) &&
      ((pParams->m_usFreqChannel != this->m_usFreqChannel) || (pParams->m_bForce == true)))
  {
    if (CSX1276_setChannel(this, pParams->m_usFreqChannel) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set Freq Channel");
      #endif
      return false;
    }
  }

  // Store current channel frequency information (i.e. text value retrieved by cleint object when a 
  // LoraPacket is received)
  strcpy((char *) this->m_ReceivedPacketInfo.m_szFrequency, CSX1276_getFreqTextValue(this->m_usFreqChannel));

  
  // If current automaton state is 'INITIALIZED', check if all required settings have been explicitly
  // provided by client object. In this case, the SX1276 is ready for radio transmission and 'STANDY'
  // automaton state is entered
  // Note: By design, no concurrency on automaton state variable
  if ((this->m_dwCurrentState == SX1276_AUTOMATON_STATE_INITIALIZED) && (CSX1276_IsDeviceConfigured(this) == true))
  {
    this->m_dwCurrentState = SX1276_AUTOMATON_STATE_STANDBY;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] CSX1276 automaton state changed: 'STANDBY'");
    #endif
  }
  return true;
}

bool CSX1276_ProcessStandBy(CSX1276 *this, CLoraTransceiverItf_StandByParams pParams)
{
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CSX1276_ProcessStandBy'");
  #endif

  // The 'StandBy' method is allowed only in 'STANDBY', 'RECEIVING' and 'SENDING' automaton states
  if ((this->m_dwCurrentState != SX1276_AUTOMATON_STATE_RECEIVING) && 
      (this->m_dwCurrentState != SX1276_AUTOMATON_STATE_SENDING) &&
      (this->m_dwCurrentState != SX1276_AUTOMATON_STATE_STANDBY))
  {
    // By design, should never occur
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  // If already in 'STANDBY' state, the reset must be explicitly forced
  if ((this->m_dwCurrentState == SX1276_AUTOMATON_STATE_STANDBY) && (pParams->m_bForce == false))
  {
    // Must be explicitly forced
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Already in STANDBY state, command must be explicitly forced");
    #endif
    return false;
  }

  // Set SX1276 device in standby mode
  if (CSX1276_startStandBy(this) != LORATRANSCEIVERITF_RESULT_SUCCESS)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Failed to set STANDBY mode in SX1276");
    #endif
    return false;
  }

  // The 'STANDBY' automaton state is entered
  // Note: By design, no concurrency on automaton state variable
  this->m_dwCurrentState = SX1276_AUTOMATON_STATE_STANDBY;
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CSX1276 automaton state changed: 'STANDBY'");
  #endif
  return true;
}


bool CSX1276_ProcessReceive(CSX1276 *this, CLoraTransceiverItf_ReceiveParams pParams)
{
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CSX1276_ProcessReceive'");
  #endif

  // The 'Receive' method is allowed only in 'STANDBY', 'SENDING' and 'RECEIVING' automaton states
  // Note: The owner object is responsible to be sure that no packet is currently 'SENDING' when asking
  //       for 'RECEIVING' mode 
  if ((this->m_dwCurrentState != SX1276_AUTOMATON_STATE_STANDBY) && 
      (this->m_dwCurrentState != SX1276_AUTOMATON_STATE_SENDING) &&
      (this->m_dwCurrentState != SX1276_AUTOMATON_STATE_RECEIVING))
  {
    // By design, should never occur
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  // If already in 'RECEIVING' state, the reset must be explicitly forced
  if ((this->m_dwCurrentState == SX1276_AUTOMATON_STATE_RECEIVING) && (pParams->m_bForce == false))
  {
    // Must be explicitly forced
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Already in RECEIVING state, command must be explicitly forced");
    #endif
    return false;
  }

  // Set SX1276 device in receive mode
  if (CSX1276_startReceive(this) != LORATRANSCEIVERITF_RESULT_SUCCESS)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Failed to set RECEIVE mode in SX1276");
    #endif
    return false;
  }

  // The 'RECEIVING' automaton state is entered
  // Note: By design, no concurrency on automaton state variable
  this->m_dwCurrentState = SX1276_AUTOMATON_STATE_RECEIVING;
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CSX1276 automaton state changed: 'RECEIVING'");
  #endif
  return true;
}

bool CSX1276_ProcessSend(CSX1276 *this, CLoraTransceiverItf_SendParams pParams)
{
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CSX1276_ProcessSend'");
  #endif

  // The 'Send' method is allowed only in 'STANDBY' and 'RECEIVING' automaton states
  // Note: By design, the SX1276 automatically returns to 'STANDBY' when packet is sent. This is
  //       detected by 'TX_DONE' IRQ and CSX1276 automaton state is adjusted accordingly.
  if ((this->m_dwCurrentState != SX1276_AUTOMATON_STATE_STANDBY) && 
      (this->m_dwCurrentState != SX1276_AUTOMATON_STATE_RECEIVING))
  {
    // By design, should never occur
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  // Enter 'STANDBY' mode (i.e. the SX1276 MUST be in 'STANDBY' mode to allow send operation)
  if (this->m_dwCurrentState != SX1276_AUTOMATON_STATE_STANDBY)
  {
    if (CSX1276_startStandBy(this) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Cannot set 'STANDBY' automaton state");
      #endif
      return false;
    }
  }

  // Copy packet in SX1276 and send it
  // Note: Packet payload is copied directly from 'CLoraTransceiverItf_LoraPacketOb'
  //       object provided by owner object
  if (CSX1276_startSend(this, pParams->m_pPacketToSend) != LORATRANSCEIVERITF_RESULT_SUCCESS)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Failed to start SEND in SX1276");
    #endif
    return false;
  }

  // The 'SENDING' automaton state is entered
  // Note: By design, no concurrency on automaton state variable
  this->m_dwCurrentState = SX1276_AUTOMATON_STATE_SENDING;
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CSX1276 automaton state changed: 'SENDING'");
  #endif
  return true;
}

/*********************************************************************************************
  Private methods (implementation)

  SX1276 event processing

  These functions are called by main automaton when 'SX1276_AUTOMATON_NOTIFY_xxx' notifications
  for events sent by SX1276 device are received.

  Note: Typically, the original event which invokes these functions is an IRQ triggered by
        SX1276 device.
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         bool CSX1276_ProcessAutomatonNotifyPacketReceived(CSX1276 *this)
 * 
 * @brief      Process the packet received event currently waiting in automaton.
 * 
 * @details    This function is invoked by main automaton when it receives a notification 
 *             signaling that a LoRa packet has been received by SX1276 and data are waiting
 *             in reception buffer.\n
 *             The function transfers the received bytes in the 'm_pPacketReceived' buffer of
 *             CSX1276 object and notifies the owner object.
 * 
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @return     The returned value is 'true' if packet data are successfully transfered and
 *             the owner object notified or 'false' otherwise.
*********************************************************************************************/
bool CSX1276_ProcessAutomatonNotifyPacketReceived(CSX1276 *this)
{
  CLoraTransceiverItf_EventOb EventOb;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CSX1276_ProcessAutomatonNotifyPacketReceived'");
  #endif

  // This function MUST be called only in 'RECEIVING' automaton state
  if (this->m_dwCurrentState != SX1276_AUTOMATON_STATE_RECEIVING)
  {
    // By design, should never occur
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  // Retrieve packet in SX1276 device
  if (CSX1276_getPacket(this) != LORATRANSCEIVERITF_RESULT_SUCCESS)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Failed to process PACKET_RECEIVED (receive error)");
    #endif
    return false;
  }

  // Packet is ready for use by owner object
  // Send an event on 'CLoraTransceiverItf' interface to notify owner object
  EventOb.m_wEventType = LORATRANSCEIVERITF_EVENT_PACKETRECEIVED;
  EventOb.m_pLoraTransceiverItf = this->m_pLoraTransceiverItf;
  EventOb.m_pEventData = this->m_pPacketReceived;
  if (xQueueSend(this->m_hEventNotifyQueue, &EventOb, 0) != pdPASS)
  {
    // Queue should be long enough to always accept packets received by LoRa devices
    // If no room available for notification, packet is discarded
    // TO DO: possible to delay and only discard packet if another one is received
    this->m_pPacketReceived->m_dwDataSize = 0;
    ++this->m_dwMissedPacketReceivedNumber;

    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[ERROR] Event notification queue full, total missed packets: ");
      DEBUG_PRINT_DEC(this->m_dwMissedPacketReceivedNumber);
      DEBUG_PRINT_CR;
    #endif
    return false;
  }
  ++this->m_dwPacketReceivedNumber;
  return true;
}


/*****************************************************************************************//**
 * @fn         bool CSX1276_ProcessAutomatonNotifyPacketSent(CSX1276 *this)
 * 
 * @brief      Process the packet sent event currently waiting in automaton.
 * 
 * @details    This function is invoked by main automaton when it receives a notification 
 *             signaling that a LoRa packet has been sent by SX1276 and SX1276 has returned
 *             to 'STANDBY' mode.\n
 *             The function changes main automaton state to 'STANDBY' and notifies the owner
 *             object (i.e. object calling 'Send' method of 'ILoraTransceiver' interface) that
 *             packet is sent.\n
 *             Typically, the owner object will send another packet or enter the continuous
 *             'RECEIVE' state.
 * 
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @return     The returned value is 'true' if the owner object is successfully notified or 
 *             'false' if function is called in wrong state or notification fails.
*********************************************************************************************/
bool CSX1276_ProcessAutomatonNotifyPacketSent(CSX1276 *this)
{
  CLoraTransceiverItf_EventOb EventOb;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Entering 'CSX1276_ProcessAutomatonNotifyPacketSent'");
  #endif

  // This function MUST be called only in 'SENDING' automaton state
  if (this->m_dwCurrentState != SX1276_AUTOMATON_STATE_SENDING)
  {
    // By design, should never occur
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Function called in invalid automaton state");
    #endif
    return false;
  }

  // The SX1276 has automatically returned to 'STANDBY' mode, update automaton state
  gpio_intr_disable(PIN_NUM_RX_TX_IRQ);
   
  this->m_dwCurrentState = SX1276_AUTOMATON_STATE_STANDBY;
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CSX1276 automaton state changed: 'STANDBY'");
  #endif


  // Packet is sent
  // Send an event on 'CLoraTransceiverItf' interface to notify owner object
  EventOb.m_wEventType = LORATRANSCEIVERITF_EVENT_PACKETSENT;
  EventOb.m_pLoraTransceiverItf = this->m_pLoraTransceiverItf;
  EventOb.m_pEventData = this->m_pPacketToSend;
  if (xQueueSend(this->m_hEventNotifyQueue, &EventOb, 0) != pdPASS)
  {
    // Queue should be long enough to always accept packets received by LoRa devices
    // If no room available for notification, sender may enter a dead lock
    // TO DO: possible to delay the notification and refuse send operation until notification done
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Event notification queue full, sender may dead lock!");
    #endif
    return false;
  }
  this->m_pPacketToSend = NULL;
  ++this->m_dwPacketSentNumber;
  return true;
}


/*********************************************************************************************
  Private methods (implementation)

  Access to SX1276 registers

  LoRa Configuration registers are accessed through the SPI interface. 
  Registers are readable in all device modes including Sleep. However, they should be written 
  only in 'Sleep' and 'StandBy' modes.
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         BYTE CSX1276_readRegister(spi_device_handle_t SPIDeviceHandle, BYTE address)
 * 
 * @brief      Reads value in a specified register.
 * 
 * @details    The function retrieves the byte value stored in the specified register.
 *
 * @param      SPIDeviceHandle
 *             The handle to the SPIDevice (ESP32 IDF object) associated to the SX1276 chip.
 *  
 * @param      address
 *             Register address to read from.
 *  
 * @return     The functions returns the content of the specified register.
*********************************************************************************************/
BYTE CSX1276_readRegister(spi_device_handle_t SPIDeviceHandle, BYTE address)
{
  BYTE value = 0x00;

  #if (SX1276_DEBUG_LEVEL2)
    DEBUG_PRINT_CR;
    DEBUG_PRINT("CSX1276_readRegister, dev: ");
    DEBUG_PRINT_HEX((uint32_t)SPIDeviceHandle);
    DEBUG_PRINT_CR;
  #endif

  spi_transaction_t t;

  // Zero out the transaction
  memset(&t, 0, sizeof(t));       

  // Bit 7 cleared to read from registers
  bitClear(address, 7);   
  t.addr = (uint64_t) address;

  // Use transaction RX buffer (declared as 4 bytes array)
  t.flags = SPI_TRANS_USE_RXDATA;

  // 8 bits of data to receive
  t.length = 8;                     

  // Start transaction and wait until completed
  esp_err_t ret = spi_device_transmit(SPIDeviceHandle, &t);
  assert(ret == ESP_OK);

  value = (BYTE) (*(uint32_t*)t.rx_data);

  #if (SX1276_DEBUG_LEVEL2)
    ret == ESP_OK ? DEBUG_PRINT("[OK] Register: ") : DEBUG_PRINT("[ERROR] Register: ");
    DEBUG_PRINT_HEX(address);
    DEBUG_PRINT(" -> ");
    DEBUG_PRINT_HEX(value);
    DEBUG_PRINT_CR;
  #endif
     
  return value;
}

/*****************************************************************************************//**
 * @fn         void CSX1276_writeRegister(spi_device_handle_t SPIDeviceHandle, BYTE address, BYTE data)
 * 
 * @brief      Writes in the specified register.
 * 
 * @details    The function writes a specified byte value in a specified register.
 *
 * @param      SPIDeviceHandle
 *             The handle to the SPIDevice (ESP32 IDF object) associated to the SX1276 chip.
 *  
 * @param      address
 *             Register address to write in.
 *  
 * @param      address
 *             Byte value to write in the register.
 *  
 * @return     None.
*********************************************************************************************/
void CSX1276_writeRegister(spi_device_handle_t SPIDeviceHandle, BYTE address, BYTE data)
{
  #if (SX1276_DEBUG_LEVEL2)
    DEBUG_PRINT_CR;
    DEBUG_PRINT("CSX1276_writeRegister, dev: ");
    DEBUG_PRINT_HEX((uint32_t)SPIDeviceHandle);
    DEBUG_PRINT_CR;
  #endif

  esp_err_t ret;
  spi_transaction_t t;

  // Zero out the transaction
  memset(&t, 0, sizeof(t));       

  // Bit 7 of address set to write to registers
  bitSet(address, 7);        
  t.addr = (uint64_t) address;

  // Use transaction TX buffer (declared as 4 bytes array)
  *(uint32_t*)t.tx_data = data;
  t.flags = SPI_TRANS_USE_TXDATA;

  // 8 bits of data to transmit
  t.length = 8;                     

  // Start transaction and wait until completed
  ret = spi_device_transmit(SPIDeviceHandle, &t);
  assert(ret == ESP_OK);                        //Should have had no issues.

  #if (SX1276_DEBUG_LEVEL2)
    ret == ESP_OK ? DEBUG_PRINT("[OK] Register: ") : DEBUG_PRINT("[ERROR] Register: ");
    bitClear(address, 7);
    DEBUG_PRINT_HEX(address);
    DEBUG_PRINT(" -> ");
    DEBUG_PRINT_HEX(data);
    DEBUG_PRINT_CR;
  #endif

}

/*****************************************************************************************//**
 * @fn         void CSX1276_clearFlags(CSX1276 *this)
 * 
 * @brief      Clears the IRQ flags register.
 * 
 * @details    The function clears the interrupt request flags register.\n
 *             This function can be called even if SX1276 is not in 'Sleep' or 'StandBy' mode.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @return     None.
*********************************************************************************************/
void CSX1276_clearFlags(CSX1276 *this)
{
  BYTE st0;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  // Save current mode
  st0 = CSX1276_readRegister(SPIDeviceHandle, REG_OP_MODE);    

  if (st0 != LORA_STANDBY_MODE)
  {
    // 'StandBy' mode to write in registers   
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_STANDBY_MODE);  
  }

  // Clear IRQ flags register
  CSX1276_writeRegister(SPIDeviceHandle, REG_IRQ_FLAGS, 0xFF); 

  if (st0 != LORA_STANDBY_MODE)
  {
    // Back to previous mode
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, st0);
  }

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("## LoRa IRQ flags cleared ##");
  #endif
}



/*********************************************************************************************
  Private methods (implementation)

  SX1276 configuration and management.

  These functions are invoked by the main automaton.
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_InitializeDevice(CSX1276 *this, BYTE SPISlaveID, 
 *                                              CLoraTransceiverItf_InitializeParams pParams)
 * 
 * @brief      Initializes the SX1276 device.
 * 
 * @details    The function establishes the SPI link to SX1276 device and configure the modem
 *             for the specified LoRa mode.\n
 *             On exit, the device is in 'StandBy' mode and ready for LoRa transmission (with
 *             LoRa MAC layer configured with default values for use on LoRaWAN public networks).
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @param      pParams
 *             The method parameters. See 'LoraTransceiverItf.h' for details.
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the SX1276 is ready
 *              - LORATRANSCEIVERITF_RESULT_NOTEXECUTED = the operation has not been executed
*********************************************************************************************/
uint8_t CSX1276_InitializeDevice(CSX1276 *this, BYTE SPISlaveID, CLoraTransceiverItf_InitializeParams pParams)
{
  uint8_t resultCode;
  
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("Entering CSX1276_InitializeDevice");
  #endif
    
  esp_err_t ret;

  spi_bus_config_t buscfg = {.miso_io_num = PIN_NUM_MISO,
                             .mosi_io_num = PIN_NUM_MOSI,
                             .sclk_io_num = PIN_NUM_CLK,
                             .quadwp_io_num = -1,
                             .quadhd_io_num = -1,
                             .max_transfer_sz = 512
                            };

  // TO DO : CS PIN according to SpiDeviceID
  spi_device_interface_config_t devcfg = {.command_bits = 0,                        // Amount of bits in command phase (0-16)
                                          .address_bits = 8,                        // Amount of bits in address phase (0-64)
                                          .clock_speed_hz = 5*1000*1000,            // Clock out at 5 MHz (Note: NOK at 10 Mhz on breadboard)
                                          .mode = 0,                                // SPI mode 0  (CPOL = 0, CPHA = 0)
                                          .spics_io_num = PIN_NUM_CS,               // CS pin
                                          .queue_size = 7,                          // We want to be able to queue 7 transactions at a time
                                          .pre_cb = NULL,                           // Possible to have pre-transfer callback 
                                          .post_cb = NULL,                          // Possible to have post-transfer callback 
                                         };

  // Initialize the SPI bus
  ret = spi_bus_initialize(HSPI_HOST, &buscfg, 1);
  assert(ret==ESP_OK);

  // Attach the SX1276 to the SPI bus
  ret = spi_bus_add_device(HSPI_HOST, &devcfg, &(this->m_SpiDeviceHandle));
  assert(ret==ESP_OK);
    
  // RX calibration (just after SX1276 reset)
  CSX1276_RxChainCalibration(this);

  // Frequency used by RxChainCalibration must be changed by explicit configuration
  this->m_usFreqChannel = SX1276_FREQ_CH_UNDEFINED;

  // Set Maximum Over Current Protection
  resultCode = CSX1276_setMaxCurrent(this, 0x1B);
  
  if (resultCode == LORATRANSCEIVERITF_RESULT_SUCCESS)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("Set LoRa mode with maximum current supply");
    #endif

    // This current (OCP) must be confirmed by explicit configuration
    this->m_usOcpRate = SX1276_OCP_UNDEFINED;
     
    // Set LoRa in Standby (i.e. other SX1276 registers with reset value)
    if ((resultCode = CSX1276_setLoRa(this)) == LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      // Set default 'SyncWord' for LoRaWAN public networks
      resultCode = CSX1276_setSyncWord(this, LORATRANSCEIVERITF_SYNCWORD_PUBLIC);

      if (resultCode == LORATRANSCEIVERITF_RESULT_SUCCESS)
      {
        // Set default 'Preamble Length' for LoRaWAN public networks
        resultCode = CSX1276_setPreambleLength(this, LORATRANSCEIVERITF_PREAMBLE_LENGTH_LORA);
      }
     
      if (resultCode == LORATRANSCEIVERITF_RESULT_SUCCESS)
      {
        // Set default 'Header ON' for LoRaWAN public networks
        resultCode = CSX1276_setHeaderON(this);
      }
       
      if (resultCode == LORATRANSCEIVERITF_RESULT_SUCCESS)
      {
        // Set default 'CRC ON' for LoRaWAN public networks
        resultCode = CSX1276_setCRC_ON(this);
      }

      // Set GPIO for Interrupt Requests from SX1276
      if (resultCode == LORATRANSCEIVERITF_RESULT_SUCCESS)
      {
        // PacketReceived and PacketSent IRQ
        // Note: Same PIN used for both RX_DONE and TX_DONE IRQs (i.e. software configuration of DIO on Sx1276
        //       according to OP mode) 
        gpio_set_direction(PIN_NUM_RX_TX_IRQ, GPIO_MODE_INPUT);
        gpio_set_pull_mode(PIN_NUM_RX_TX_IRQ, GPIO_PULLDOWN_ENABLE); //GPIO_FLOATING);
        gpio_set_intr_type(PIN_NUM_RX_TX_IRQ, GPIO_INTR_POSEDGE);
        gpio_intr_disable(PIN_NUM_RX_TX_IRQ); 

        if (gpio_isr_handler_add(PIN_NUM_RX_TX_IRQ, (gpio_isr_t) CSX1276_PacketRxTxIntHandler, this) != ESP_OK)
        {
          resultCode = LORATRANSCEIVERITF_RESULT_ERROR;
        }
      }

    }


   // DEBUG
//   CSX1276_CPhamInit(this);
  }
    
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         void CSX1276_RxChainCalibration(CSX1276 *this)
 * 
 * @brief      Performs the RX chain calibration for LF and HF bands.
 * 
 * @details    The RX calibration is required for SX1276 device.\n
 *             This function MUST be called just after the reset so all registers are at 
 *             their default values. 
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @return     None.
*********************************************************************************************/
void CSX1276_RxChainCalibration(CSX1276 *this)
{
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("Starting SX1276 LF/HF calibration");
  #endif

  // Cut the PA just in case, RFO output, power = -1 dBm
  CSX1276_writeRegister(SPIDeviceHandle, REG_PA_CONFIG, 0x00);

  // Launch Rx chain calibration for LF band
  CSX1276_writeRegister(SPIDeviceHandle, REG_IMAGE_CAL, (CSX1276_readRegister(SPIDeviceHandle, REG_IMAGE_CAL) & RF_IMAGECAL_IMAGECAL_MASK) | RF_IMAGECAL_IMAGECAL_START);
  while((CSX1276_readRegister(SPIDeviceHandle, REG_IMAGE_CAL) & RF_IMAGECAL_IMAGECAL_RUNNING) == RF_IMAGECAL_IMAGECAL_RUNNING)
  {
  }

  // Sets a Frequency in HF band
  CSX1276_setChannel(this, LORATRANSCEIVERITF_FREQUENCY_CHANNEL_17);

  // Launch Rx chain calibration for HF band
  CSX1276_writeRegister(SPIDeviceHandle, REG_IMAGE_CAL, (CSX1276_readRegister(SPIDeviceHandle, REG_IMAGE_CAL) & RF_IMAGECAL_IMAGECAL_MASK) | RF_IMAGECAL_IMAGECAL_START);
  while((CSX1276_readRegister(SPIDeviceHandle, REG_IMAGE_CAL) & RF_IMAGECAL_IMAGECAL_RUNNING) == RF_IMAGECAL_IMAGECAL_RUNNING)
  {
  }

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[OK] SX1276 LF/HF calibration");
  #endif
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_setLoRa(CSX1276 *this)
 * 
 * @brief      Sets the SX1276 in LoRa radio mode.
 * 
 * @details    The function sets the modem for LoRa mode and activates the 'StandBy' mode.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the SX1276 is set in LoRa 'StandBy' mode
 *              - LORATRANSCEIVERITF_RESULT_NOTEXECUTED = the operation has not been executed
*********************************************************************************************/
uint8_t CSX1276_setLoRa(CSX1276 *this)
{
  uint8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  BYTE st0;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("Starting 'setLORA'");
  #endif

  CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, FSK_SLEEP_MODE);     // Sleep mode (mandatory to set LoRa mode)
  CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_SLEEP_MODE);    // LoRa sleep mode
  CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_STANDBY_MODE);  // LoRa standby mode

  CSX1276_writeRegister(SPIDeviceHandle, REG_MAX_PAYLOAD_LENGTH, LORA_MAX_PAYLOAD_LENGTH);
    
  // Set RegModemConfig1 to Default values
  CSX1276_writeRegister(SPIDeviceHandle, REG_MODEM_CONFIG1, 0x08); 
  // Set RegModemConfig2 to Default values
  CSX1276_writeRegister(SPIDeviceHandle, REG_MODEM_CONFIG2, 0x74);   

  // Delay 100ms
  vTaskDelay(pdMS_TO_TICKS(100));

  st0 = CSX1276_readRegister(SPIDeviceHandle, REG_OP_MODE);  // Reading config mode
  if (st0 == LORA_STANDBY_MODE)
  { 
    // LoRa mode
    this->m_usModemMode = MODEM_MODE_LORA;
    resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[OK] LoRa mode set");
      DEBUG_PRINT_CR;
    #endif
  }
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_setLoRaMode(CSX1276 *this, uint8_t LoraMode)
 * 
 * @brief      Sets the SX1276 in a specified LoRa mode.
 * 
 * @details    The function sets the modem to LoRa radio mode and configure the specified
 *             bandwith, coding rate and spreading factor of the LoRa modulation.\n
 *             On exit the modem is in 'StandBy' mode.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @param      LoraMode
 *             The LoRa mode number as defined by 'LORATRANSCEIVERITF_LORAMODE_xxx' constants
 *             (see 'LoraTransceiverItf.h' for details).
 *  
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the SX1276 is set in LoRa 'StandBy' mode
 *              - LORATRANSCEIVERITF_RESULT_ERROR = error during configuration
 *              - LORATRANSCEIVERITF_RESULT_INVALIDPARAMS = invalid value of specified LoraMode
 *              - LORATRANSCEIVERITF_RESULT_NOTEXECUTED = the operation has not been executed
*********************************************************************************************/
uint8_t CSX1276_setLoRaMode(CSX1276 *this, uint8_t LoraMode)
{
  uint8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  BYTE st0;
  BYTE config1 = 0x00;
  BYTE config2 = 0x00;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("Entering 'setLoraMode'");
  #endif

  // 'SetMode' function only can be called in LoRa mode
  if (this->m_usModemMode != MODEM_MODE_LORA)
  {
    if ((resultCode = CSX1276_setLoRa(this)) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      return resultCode;
    }
    resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  }
  
  // Save the current OP mode
  st0 = CSX1276_readRegister(SPIDeviceHandle, REG_OP_MODE);    

  // LoRa standby mode
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_STANDBY_MODE);  
  }

  switch (LoraMode)
  {
    // Mode 1 (better reach, medium time on air)
    case LORATRANSCEIVERITF_LORAMODE_1:   
      CSX1276_setCR(this, LORATRANSCEIVERITF_CR_5);      // CR = 4/5
      CSX1276_setSF(this, LORATRANSCEIVERITF_SF_12);     // SF = 12
      CSX1276_setBW(this, LORATRANSCEIVERITF_BANDWIDTH_125);    // BW = 125 KHz
      break;

    // Mode 2 (medium reach, less time on air)
    case LORATRANSCEIVERITF_LORAMODE_2:   
      CSX1276_setCR(this, LORATRANSCEIVERITF_CR_5);      // CR = 4/5
      CSX1276_setSF(this, LORATRANSCEIVERITF_SF_12);     // SF = 12
      CSX1276_setBW(this, LORATRANSCEIVERITF_BANDWIDTH_250);    // BW = 250 KHz
      break;

    // Mode 3 (worst reach, less time on air)
    case LORATRANSCEIVERITF_LORAMODE_3: 
      CSX1276_setCR(this, LORATRANSCEIVERITF_CR_5);      // CR = 4/5
      CSX1276_setSF(this, LORATRANSCEIVERITF_SF_10);     // SF = 10
      CSX1276_setBW(this, LORATRANSCEIVERITF_BANDWIDTH_125);    // BW = 125 KHz
      break;

    // Mode 4 (better reach, low time on air)
    case LORATRANSCEIVERITF_LORAMODE_4:
      CSX1276_setCR(this, LORATRANSCEIVERITF_CR_5);      // CR = 4/5
      CSX1276_setSF(this, LORATRANSCEIVERITF_SF_12);     // SF = 12
      CSX1276_setBW(this, LORATRANSCEIVERITF_BANDWIDTH_500);    // BW = 500 KHz
      break;

    // Mode 5 (better reach, medium time on air)
    case LORATRANSCEIVERITF_LORAMODE_5:
      CSX1276_setCR(this, LORATRANSCEIVERITF_CR_5);      // CR = 4/5
      CSX1276_setSF(this, LORATRANSCEIVERITF_SF_10);     // SF = 10
      CSX1276_setBW(this, LORATRANSCEIVERITF_BANDWIDTH_250);    // BW = 250 KHz
      break;

    // Mode 6 (better reach, worst time-on-air)
    case LORATRANSCEIVERITF_LORAMODE_6:
      CSX1276_setCR(this, LORATRANSCEIVERITF_CR_5);      // CR = 4/5
      CSX1276_setSF(this, LORATRANSCEIVERITF_SF_11);     // SF = 11
      CSX1276_setBW(this, LORATRANSCEIVERITF_BANDWIDTH_500);    // BW = 500 KHz
      break;

    // Mode 7 (medium-high reach, medium-low time-on-air)
    case LORATRANSCEIVERITF_LORAMODE_7: 
      CSX1276_setCR(this, LORATRANSCEIVERITF_CR_5);      // CR = 4/5
      CSX1276_setSF(this, LORATRANSCEIVERITF_SF_9);      // SF = 9
      CSX1276_setBW(this, LORATRANSCEIVERITF_BANDWIDTH_250);    // BW = 250 KHz
      break;

    // Mode 8 (medium reach, medium time-on-air)
    case LORATRANSCEIVERITF_LORAMODE_8:
      CSX1276_setCR(this, LORATRANSCEIVERITF_CR_5);      // CR = 4/5
      CSX1276_setSF(this, LORATRANSCEIVERITF_SF_9);      // SF = 9
      CSX1276_setBW(this, LORATRANSCEIVERITF_BANDWIDTH_500);    // BW = 500 KHz
      break;

    // Mode 9 (medium-low reach, medium-high time-on-air)
    case LORATRANSCEIVERITF_LORAMODE_9:
      CSX1276_setCR(this, LORATRANSCEIVERITF_CR_5);      // CR = 4/5
      CSX1276_setSF(this, LORATRANSCEIVERITF_SF_8);      // SF = 8
      CSX1276_setBW(this, LORATRANSCEIVERITF_BANDWIDTH_500);    // BW = 500 KHz
      break;

    // Mode 10 (worst reach, less time_on_air)
    case LORATRANSCEIVERITF_LORAMODE_10:
      CSX1276_setCR(this, LORATRANSCEIVERITF_CR_5);      // CR = 4/5
      CSX1276_setSF(this, LORATRANSCEIVERITF_SF_7);      // SF = 7
      CSX1276_setBW(this, LORATRANSCEIVERITF_BANDWIDTH_500);    // BW = 500 KHz
      break;

    // test for LoRaWAN channel
    case LORATRANSCEIVERITF_LORAMODE_11:
    	CSX1276_setCR(this, LORATRANSCEIVERITF_CR_5);        // CR = 4/5
    	CSX1276_setSF(this, LORATRANSCEIVERITF_SF_12);       // SF = 12
    	CSX1276_setBW(this, LORATRANSCEIVERITF_BANDWIDTH_125);      // BW = 125 KHz
      break;

    default:
      resultCode = LORATRANSCEIVERITF_RESULT_INVALIDPARAMS; 
  };


  // Check proper register configuration
  if (resultCode == LORATRANSCEIVERITF_RESULT_INVALIDPARAMS)
  {
    // Don't change resultCode value
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("** The indicated mode doesn't exist, ");
      DEBUG_PRINT_LN("please select from 1 to 10 **");
    #endif
  }
  else
  {
    resultCode = LORATRANSCEIVERITF_RESULT_ERROR;
    config1 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG1);
    switch (LoraMode)
    { 
      //  Different way to check for each mode:
      // (config1 >> 1) ---> take out bits 7-1 from REG_MODEM_CONFIG1 (=_bandwidth & _codingRate together)
      // (config2 >> 4) ---> take out bits 7-4 from REG_MODEM_CONFIG2 (=_spreadingFactor)

      // mode 1: BW = 125 KHz, CR = 4/5, SF = 12.
      case LORATRANSCEIVERITF_LORAMODE_1:  
        if ((config1 >> 1) == 0x39)
        { 
          config2 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2);
          if ((config2 >> 4) == LORATRANSCEIVERITF_SF_12)
          {
            resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
          }
        }
        break;

      // mode 2: BW = 250 KHz, CR = 4/5, SF = 12.
      case LORATRANSCEIVERITF_LORAMODE_2:  
        if ((config1 >> 1) == 0x41)
        {
          config2 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2);
          if ((config2 >> 4) == LORATRANSCEIVERITF_SF_12)
          {
            resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
          }
        }
        break;

      // mode 3: BW = 125 KHz, CR = 4/5, SF = 10.
      case LORATRANSCEIVERITF_LORAMODE_3:  
        if ((config1 >> 1) == 0x39)
        {
          config2 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2);
          if ((config2 >> 4) == LORATRANSCEIVERITF_SF_10)
          {
            resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
          }
        }
        break;

      // mode 4: BW = 500 KHz, CR = 4/5, SF = 12.
      case LORATRANSCEIVERITF_LORAMODE_4:  
        if ((config1 >> 1) == 0x49)
        { 
          config2 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2);
          if ((config2 >> 4) == LORATRANSCEIVERITF_SF_12)
          {
            resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
          }
        }
        break;

      // mode 5: BW = 250 KHz, CR = 4/5, SF = 10.
      case LORATRANSCEIVERITF_LORAMODE_5:
        if ((config1 >> 1) == 0x41)
        {  
          config2 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2);
          if ((config2 >> 4) == LORATRANSCEIVERITF_SF_10)
          {
            resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
          }
        }
        break;

      // mode 6: BW = 500 KHz, CR = 4/5, SF = 11.
      case LORATRANSCEIVERITF_LORAMODE_6:  
        if ((config1 >> 1) == 0x49)
        {
          config2 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2);
          if ((config2 >> 4) == LORATRANSCEIVERITF_SF_11)
          {
            resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
          }
        }
        break;

      // mode 7: BW = 250 KHz, CR = 4/5, SF = 9.
      case LORATRANSCEIVERITF_LORAMODE_7:  
        if ((config1 >> 1) == 0x41)
        {  
          config2 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2);
          if ((config2 >> 4) == LORATRANSCEIVERITF_SF_9)
          {
            resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
          }
        }
        break;

      // mode 8: BW = 500 KHz, CR = 4/5, SF = 9.
      case LORATRANSCEIVERITF_LORAMODE_8: 
        if ((config1 >> 1) == 0x49)
        {
          config2 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2);
          if ((config2 >> 4) == LORATRANSCEIVERITF_SF_9)
          {
            resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
          }
        }
        break;

      // mode 9: BW = 500 KHz, CR = 4/5, SF = 8.
      case LORATRANSCEIVERITF_LORAMODE_9:  
        if ((config1 >> 1) == 0x49)
        {  
          config2 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2);
          if ((config2 >> 4) == LORATRANSCEIVERITF_SF_8)
          {
            resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
          }
        }
        break;

      // mode 10: BW = 500 KHz, CR = 4/5, SF = 7.
      case LORATRANSCEIVERITF_LORAMODE_10: 
        if ((config1 >> 1) == 0x49)
        {  
          config2 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2);
          if ((config2 >> 4) == LORATRANSCEIVERITF_SF_7)
          {
            resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
          }
        }
        break;

      // mode 11: BW = 125 KHz, CR = 4/5, SF = 7.
      case LORATRANSCEIVERITF_LORAMODE_11: 
        if ((config1 >> 1) == 0x39)
        {  
          config2 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2);
          if ((config2 >> 4) == LORATRANSCEIVERITF_SF_12)
          {
            resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
          }
        }
    }
  }
  
  #if (SX1276_DEBUG_LEVEL0)
    if (resultCode == LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      DEBUG_PRINT("[INFO] Mode ");
      DEBUG_PRINT_DEC(LoraMode);
      DEBUG_PRINT_LN(" configured with success");    
    }
    else
    {
      DEBUG_PRINT("[ERROR] There has been an error while configuring mode ");    
      DEBUG_PRINT_DEC(LoraMode);
    }
  #endif
  
  // Restore previous OP mode
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, st0);  
  }
  
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_setSyncWord(CSX1276 *this, uint8_t SyncWord)
 * 
 * @brief      Sets the 'Sync Word' for LoRa radio.
 * 
 * @details    The function sets the 'Sync Word' used by LoRa radio to detect packet preamble.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @param      SyncWord
 *             The value of 'Sync Word' to set for LoRa radio.
 *             In LoRa specification, values are:
 *              - 0x34 for public networks (i.e. compliant with LoRaWAN)
 *              - 0x12 for private networks (typically LoRa)
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the 'Sync Word' is set for LoRa radio
 *              - LORATRANSCEIVERITF_RESULT_ERROR = error during configuration
 *              - LORATRANSCEIVERITF_RESULT_NOTEXECUTED = the operation has not been executed
*********************************************************************************************/
uint8_t CSX1276_setSyncWord(CSX1276 *this, uint8_t SyncWord)
{
  BYTE st0;
  uint8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  BYTE config1;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("Starting CSX1276_setSyncWord");
  #endif

  // Should never occur
  if (this->m_usModemMode != MODEM_MODE_LORA)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[WARNING] Function called before LoRa mode activated - Switching to LoRa");
    #endif
    if ((resultCode = CSX1276_setLoRa(this)) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      return resultCode;
    }
  }

  // Save the current OP mode
  st0 = CSX1276_readRegister(SPIDeviceHandle, REG_OP_MODE);		

  // Set Standby mode to write in registers
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_STANDBY_MODE);		
  }
  CSX1276_writeRegister(SPIDeviceHandle, REG_SYNC_WORD, SyncWord);

  // Delay 100ms
  vTaskDelay(pdMS_TO_TICKS(100));

  config1 = CSX1276_readRegister(SPIDeviceHandle, REG_SYNC_WORD);

  if (config1 == SyncWord) 
  {
    this->m_usSyncWord = SyncWord;
    resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[OK] Sync word set: ");
      DEBUG_PRINT_HEX(SyncWord);
      DEBUG_PRINT_CR;
    #endif
  }
  else 
  {
    resultCode = LORATRANSCEIVERITF_RESULT_ERROR;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Unable to set Sync Word");
    #endif
  }

  if (st0 != LORA_STANDBY_MODE)
  {
    // Get back to previous OP mode
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, st0);	
  }

  // Delay 100ms
  vTaskDelay(pdMS_TO_TICKS(100));

  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_setHeaderON(CSX1276 *this)
 * 
 * @brief      Sets the SX1276 in explicit header mode (header is sent).
 * 
 * @details    The function configures the SX1276 to put the 'Header' field in LoRa messages.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the 'Header' is set to ON for LoRa radio
 *              - LORATRANSCEIVERITF_RESULT_ERROR = error during configuration
 *              - LORATRANSCEIVERITF_RESULT_NOTEXECUTED = the operation has not been executed
 *              - LORATRANSCEIVERITF_RESULT_INVALIDSTATE = Function called on invalid state
*********************************************************************************************/
uint8_t CSX1276_setHeaderON(CSX1276 *this)
{
  int8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  BYTE config1;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("Starting 'CSX1276_setHeaderON'");
  #endif

  if (this->m_usModemMode != MODEM_MODE_LORA)
  {
    // Should never occur: must be configured in LoRa mode before
    resultCode = LORATRANSCEIVERITF_RESULT_INVALIDSTATE;  
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Not configured in LoRa mode");
      DEBUG_PRINT_CR;
    #endif
  }
  else
  {
    // Save config1 to modify only the header bit
    config1 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG1);  
    if (this->m_usSpreadingFactor == 6)
    {
      // Mandatory headerOFF with SF = 6
      resultCode = LORATRANSCEIVERITF_RESULT_INVALIDSTATE;   
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Mandatory implicit header mode with spreading factor = 6");
      #endif
    }
    else
    {
      // Clear bit 0 from config1 (= headerON) and update config1
      config1 = config1 & 0b11111110;                                     
      CSX1276_writeRegister(SPIDeviceHandle, REG_MODEM_CONFIG1, config1);
    }
    if (this->m_usSpreadingFactor != 6 )
    {
      // Check headerON taking out bit 0 from REG_MODEM_CONFIG1
      config1 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG1);
      if (bitRead(config1, 0) == 0)
      {
        resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
        this->m_usHeader = SX1276_HEADER_ON;
        #if (SX1276_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[INFO] Header has been activated");
          DEBUG_PRINT_CR;
        #endif
      }
      else
      {
        resultCode = LORATRANSCEIVERITF_RESULT_ERROR;
        #if (SX1276_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[ERROR] Failed to activate header");
          DEBUG_PRINT_CR;
        #endif
      }
    }
  }
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_setHeaderOFF(CSX1276 *this)
 * 
 * @brief      Sets the SX1276 in implicit header mode (header is not sent).
 * 
 * @details    The function configures the SX1276 to not put the 'Header' field in LoRa messages.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the 'Header' is set to OFF for LoRa radio
 *              - LORATRANSCEIVERITF_RESULT_ERROR = error during configuration
 *              - LORATRANSCEIVERITF_RESULT_INVALIDSTATE = Function called on invalid state
*********************************************************************************************/
uint8_t  CSX1276_setHeaderOFF(CSX1276 *this)
{
  uint8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  BYTE config1;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("Starting 'CSX1276_setHeaderOFF'");
  #endif

  if (this->m_usModemMode != MODEM_MODE_LORA)
  { 
    // Should never occur: must be configured in LoRa mode before
    resultCode = LORATRANSCEIVERITF_RESULT_INVALIDSTATE;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Not configured in LoRa mode");
      DEBUG_PRINT_CR;
    #endif
  }
  else
  {
    // Read config1 to modify only the header bit
    config1 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG1);  
    
    // Set bit 0 from REG_MODEM_CONFIG1 (= headerOFF) and update Config1
    config1 = config1 | 0b00000001;    
    CSX1276_writeRegister(SPIDeviceHandle, REG_MODEM_CONFIG1, config1);   

    // Check register
    config1 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG1);
    if (bitRead(config1, 0) == SX1276_HEADER_OFF)
    { 
      // Checking headerOFF taking out bit 2 from REG_MODEM_CONFIG1
      resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
      this->m_usHeader = SX1276_HEADER_OFF;

      #if (SX1276_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[INFO] Header has been desactivated");
          DEBUG_PRINT_CR;
      #endif
    }
    else
    {
      resultCode = LORATRANSCEIVERITF_RESULT_ERROR;
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to desactivate header");
        DEBUG_PRINT_CR;
      #endif
    }
  }
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_setCRC_ON(CSX1276 *this)
 * 
 * @brief      Sets the SX1276 with CRC ON.
 * 
 * @details    The function configures the SX1276 to put the 'CRC' field in LoRa messages.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the 'CRC' is set to ON for LoRa radio
 *              - LORATRANSCEIVERITF_RESULT_ERROR = error during configuration
 *              - LORATRANSCEIVERITF_RESULT_INVALIDSTATE = Function called on invalid state
*********************************************************************************************/
uint8_t CSX1276_setCRC_ON(CSX1276 *this)
{
  uint8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  BYTE config1;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_setCRC_ON'");
  #endif

  if (this->m_usModemMode == MODEM_MODE_LORA)
  { 
    // LORA mode
    config1 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2);  // Save config2 to modify only the CRC bit
    config1 = config1 | 0b00000100;                                      // sets bit 2 from REG_MODEM_CONFIG2 = CRC_ON
    CSX1276_writeRegister(SPIDeviceHandle, REG_MODEM_CONFIG2, config1);

    config1 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2);
    if (bitRead(config1, 2) == SX1276_CRC_ON)
    {
      // take out bit 2 from REG_MODEM_CONFIG2 indicates RxPayloadCrcOn
      resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
      this->m_usCRC = SX1276_CRC_ON;
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[INFO] CRC has been activated");
        DEBUG_PRINT_CR;
      #endif
    }
    else
    {
      resultCode = LORATRANSCEIVERITF_RESULT_ERROR;
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set CRC ON");
        DEBUG_PRINT_CR;
      #endif
    }
  }
  else
  { 
    // Should never occur: must be configured in LoRa mode before
    resultCode = LORATRANSCEIVERITF_RESULT_INVALIDSTATE;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Not configured in LoRa mode");
      DEBUG_PRINT_CR;
    #endif
  }
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_setCRC_OFF(CSX1276 *this)
 * 
 * @brief      Sets the SX1276 with CRC OFF.
 * 
 * @details    The function configures the SX1276 to not put the 'CRC' field in LoRa messages.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the 'CRC' is set to OFF for LoRa radio
 *              - LORATRANSCEIVERITF_RESULT_ERROR = error during configuration
 *              - LORATRANSCEIVERITF_RESULT_INVALIDSTATE = Function called on invalid state
*********************************************************************************************/
uint8_t CSX1276_setCRC_OFF(CSX1276 *this)
{
  int8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  BYTE config1;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_setCRC_OFF'");
  #endif

  if (this->m_usModemMode == MODEM_MODE_LORA)
  { 
    // Save config2 to modify only the CRC bit
    config1 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2);  

    // Clear bit 2 from config2 = CRC_OFF
    config1 = config1 & 0b11111011;                                      
    CSX1276_writeRegister(SPIDeviceHandle, REG_MODEM_CONFIG2, config1);

    config1 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2);
    if ((bitRead(config1, 2)) == SX1276_CRC_OFF)
    {
      // Take out bit 1 from REG_MODEM_CONFIG1 indicates RxPayloadCrcOn
      resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
      this->m_usCRC = SX1276_CRC_OFF;
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[INFO] CRC has been desactivated");
        DEBUG_PRINT_CR;
      #endif
    }
    else
    {
      resultCode = LORATRANSCEIVERITF_RESULT_ERROR;
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Failed to set CRC OFF");
        DEBUG_PRINT_CR;
      #endif
    }
  }
  else
  { 
    // Should never occur: must be configured in LoRa mode before
    resultCode = LORATRANSCEIVERITF_RESULT_INVALIDSTATE;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Not configured in LoRa mode");
      DEBUG_PRINT_CR;
    #endif
  }
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_setSF(CSX1276 *this, uint8_t SpreadingFactor)
 * 
 * @brief      Sets the Spreading Factor in SX1276.
 * 
 * @details    The function sets the specified Spreading Factor for LoRa radio.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *
 * @param      SpreadingFactor
 *             The Spreading Factor value. Allowed values are 'LORATRANSCEIVERITF_SF_xxx'.
 *             See 'LoraTransceiverItf.h' for details.    
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the 'SF' is set for LoRa radio
 *              - LORATRANSCEIVERITF_RESULT_ERROR = error during configuration
 *              - LORATRANSCEIVERITF_RESULT_INVALIDPARAMS = invalid value specified for SF
 *              - LORATRANSCEIVERITF_RESULT_NOTEXECUTED = the operation has not been executed
*********************************************************************************************/
uint8_t CSX1276_setSF(CSX1276 *this, uint8_t SpreadingFactor)
{
  BYTE st0;
  uint8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  BYTE config1;
  BYTE config2;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_setSF'");
  #endif

  if (CSX1276_isSF(SpreadingFactor) == false)
  { 
    // Checking allowed values for SpreadingFactor
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[ERROR] Spreadin Factor ");
      DEBUG_PRINT_HEX(SpreadingFactor);
      DEBUG_PRINT_LN(" is not a correct value");
      DEBUG_PRINT_CR;
    #endif
    return LORATRANSCEIVERITF_RESULT_INVALIDPARAMS;
  }

  if (this->m_usModemMode != MODEM_MODE_LORA)
  {
    // Should never occur: Lora mode must be set first
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[WARNING] LoRa mode not set, activating now");
    #endif
    if ((resultCode = CSX1276_setLoRa(this)) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      return resultCode;
    }
  }

  resultCode = LORATRANSCEIVERITF_RESULT_ERROR;

  // Save the current OP mode
  st0 = CSX1276_readRegister(SPIDeviceHandle, REG_OP_MODE);  

  // LoRa standby mode
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_STANDBY_MODE);  
  }
  
  // Read config1 to modify only the LowDataRateOptimize
  config1 = (CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG1));  
  // Read config2 to modify SF value (bits 7-4)
  config2 = (CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2));  
  
  switch (SpreadingFactor)
  {
    case LORATRANSCEIVERITF_SF_6:
      config2 = config2 & 0b01101111;  // clears bits 7 & 4 from REG_MODEM_CONFIG2
      config2 = config2 | 0b01100000;  // sets bits 6 & 5 from REG_MODEM_CONFIG2
      break;

    case LORATRANSCEIVERITF_SF_7: 
      config2 = config2 & 0b01111111;  // clears bits 7 from REG_MODEM_CONFIG2
      config2 = config2 | 0b01110000;  // sets bits 6, 5 & 4
      break;
        
    case LORATRANSCEIVERITF_SF_8:  
      config2 = config2 & 0b10001111;  // clears bits 6, 5 & 4 from REG_MODEM_CONFIG2
      config2 = config2 | 0b10000000;  // sets bit 7 from REG_MODEM_CONFIG2
      break;
        
    case LORATRANSCEIVERITF_SF_9:  
      config2 = config2 & 0b10011111;  // clears bits 6, 5 & 4 from REG_MODEM_CONFIG2
      config2 = config2 | 0b10010000;  // sets bits 7 & 4 from REG_MODEM_CONFIG2
      break;
        
    case LORATRANSCEIVERITF_SF_10: config2 = config2 & 0b10101111;  // clears bits 6 & 4 from REG_MODEM_CONFIG2
      config2 = config2 | 0b10100000;  // sets bits 7 & 5 from REG_MODEM_CONFIG2
      break;
        
    case LORATRANSCEIVERITF_SF_11: 
      config2 = config2 & 0b10111111;  // clears bit 6 from REG_MODEM_CONFIG2
      config2 = config2 | 0b10110000;  // sets bits 7, 5 & 4 from REG_MODEM_CONFIG2
      CSX1276_getBW(this);
      if (this->m_usBandwidth == LORATRANSCEIVERITF_BANDWIDTH_125)
      { 
        // LowDataRateOptimize (Mandatory with LORATRANSCEIVERITF_SF_11 if LORATRANSCEIVERITF_BANDWIDTH_125)
        BYTE config3 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG3);
        config3 = config3 | 0b00001000;
        CSX1276_writeRegister(SPIDeviceHandle, REG_MODEM_CONFIG3, config3);
      }
      break;
        
    case LORATRANSCEIVERITF_SF_12: 
      config2 = config2 & 0b11001111;  // clears bits 5 & 4 from REG_MODEM_CONFIG2
      config2 = config2 | 0b11000000;  // sets bits 7 & 6 from REG_MODEM_CONFIG2
      CSX1276_getBW(this);
      if (this->m_usBandwidth == LORATRANSCEIVERITF_BANDWIDTH_125)
      { 
        // LowDataRateOptimize (Mandatory with LORATRANSCEIVERITF_SF_12 if LORATRANSCEIVERITF_BANDWIDTH_125)
        BYTE config3 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG3);
        config3 = config3 | 0b00001000;
        CSX1276_writeRegister(SPIDeviceHandle, REG_MODEM_CONFIG3, config3);
      }
      break;
  }
  
  // Check if it is neccesary to set special settings for SF=6
  if (SpreadingFactor == LORATRANSCEIVERITF_SF_6)
  {   
    // Mandatory headerOFF with SF = 6 (Implicit mode)
    CSX1276_setHeaderOFF(this); 
    
    // Set the bit field DetectionOptimize of 
    // register RegLoRaDetectOptimize to value "0b101".
    CSX1276_writeRegister(SPIDeviceHandle, REG_DETECT_OPTIMIZE, 0x05);
    
    // Write 0x0C in the register RegDetectionThreshold.            
    CSX1276_writeRegister(SPIDeviceHandle, REG_DETECTION_THRESHOLD, 0x0C);
  }
  else
  {
    // added by C. Pham
    CSX1276_setHeaderON(this);

    // LoRa detection Optimize: 0x03 --> SF7 to SF12
    CSX1276_writeRegister(SPIDeviceHandle, REG_DETECT_OPTIMIZE, 0x03);
    
    // LoRa detection threshold: 0x0A --> SF7 to SF12         
    CSX1276_writeRegister(SPIDeviceHandle, REG_DETECTION_THRESHOLD, 0x0A);   
  }
  
  // Set the AgcAutoOn in bit 2 of REG_MODEM_CONFIG3
  BYTE config3 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG3);
  config3 = config3 | 0b00000100;
  CSX1276_writeRegister(SPIDeviceHandle, REG_MODEM_CONFIG3, config3);

  // Update 'config2'
  CSX1276_writeRegister(SPIDeviceHandle, REG_MODEM_CONFIG2, config2);    
  
  // Read 'config1' and 'config2' to check update
  config1 = (CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG3));
  config2 = (CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2));
  
  // (config2 >> 4) ---> take out bits 7-4 from REG_MODEM_CONFIG2 (=_spreadingFactor)
  // bitRead(config1, 0) ---> take out bit 3 from REG_MODEM_CONFIG3 (=LowDataRateOptimize)
  // LowDataRateOptimize is in REG_MODEM_CONFIG3
  switch (SpreadingFactor)
  {
    case LORATRANSCEIVERITF_SF_6:
      if (((config2 >> 4) == SpreadingFactor) && (bitRead(config1, 2) == 1) && (this->m_usHeader == SX1276_HEADER_OFF))
      {
        resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
      }
      break;
    case LORATRANSCEIVERITF_SF_7:  
      if (((config2 >> 4) == 0x07) && (bitRead(config1, 2) == 1))
      {
        resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
      }
      break;
    case LORATRANSCEIVERITF_SF_8:  
      if (((config2 >> 4) == 0x08) && (bitRead(config1, 2) == 1))
      {
        resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
      }
      break;
    case LORATRANSCEIVERITF_SF_9:  
      if (((config2 >> 4) == 0x09) && (bitRead(config1, 2) == 1))
      {
        resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
      }
      break;
    case LORATRANSCEIVERITF_SF_10: 
      if (((config2 >> 4) == 0x0A) && (bitRead(config1, 2) == 1))
      {
        resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
      }
      break;
    case LORATRANSCEIVERITF_SF_11: 
      if (((config2 >> 4) == 0x0B) && (bitRead(config1, 2) == 1) && (bitRead(config1, 3) == 1))
      {
        resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
      }
      break;
    case LORATRANSCEIVERITF_SF_12: 
      if (((config2 >> 4) == 0x0C) && (bitRead(config1, 2) == 1) && (bitRead(config1, 3) == 1))
      {
        resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
      }
      break;
    default:  
      resultCode = LORATRANSCEIVERITF_RESULT_ERROR;
  }

  // Restore previous OP Mode
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, st0);  
  }

  if (resultCode == LORATRANSCEIVERITF_RESULT_SUCCESS)
  { 
    this->m_usSpreadingFactor = SpreadingFactor;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[INFO] Spreading factor ");
      DEBUG_PRINT_DEC(SpreadingFactor);
      DEBUG_PRINT_LN(" has been successfully set");
      DEBUG_PRINT_CR;
    #endif
  }
  else
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[ERROR] Failed to set spreading factor");
      DEBUG_PRINT_CR;
    #endif
  }
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t  CSX1276_setBW(CSX1276 *this, uint16_t BandWidth)
 * 
 * @brief      Sets the Bandwidth in SX1276.
 * 
 * @details    The function sets the specified Bandwidth for LoRa radio.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *
 * @param      BandWidth
 *             The bandwidth value. Allowed values are 'LORATRANSCEIVERITF_BANDWIDTH_xxx'.
 *             See 'LoraTransceiverItf.h' for details.    
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the 'BW' is set for LoRa radio
 *              - LORATRANSCEIVERITF_RESULT_ERROR = error during configuration
 *              - LORATRANSCEIVERITF_RESULT_INVALIDPARAMS = invalid bandwidth value specified
 *              - LORATRANSCEIVERITF_RESULT_NOTEXECUTED = the operation has not been executed
*********************************************************************************************/
uint8_t  CSX1276_setBW(CSX1276 *this, uint16_t BandWidth)
{
  BYTE st0;
  int8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  BYTE config1;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("Starting 'setBW'");
  #endif

  if (CSX1276_isBW(BandWidth) == false)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[ERROR] Bandwidth ");
      DEBUG_PRINT_HEX(BandWidth);
      DEBUG_PRINT_LN(" is not a correct value");
      DEBUG_PRINT_CR;
    #endif
    return LORATRANSCEIVERITF_RESULT_INVALIDPARAMS;
  }

  if (this->m_usModemMode != MODEM_MODE_LORA)
  {
    // Should never occur: Lora mode must be set first
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[WARNING] LoRa mode not set, activating now");
    #endif
    if ((resultCode = CSX1276_setLoRa(this)) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      return resultCode;
    }
  }

  resultCode = LORATRANSCEIVERITF_RESULT_ERROR;

  // Save the previous OP mode
  st0 = CSX1276_readRegister(SPIDeviceHandle, REG_OP_MODE);    

  // LoRa standby mode
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_STANDBY_MODE); 
  }

  // Save config1 to modify only the BW and clear bits 7 - 4 from REG_MODEM_CONFIG1
  config1 = (CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG1));   
  config1 = config1 & 0b00001111;	                                        
  switch (BandWidth)
  {
    case LORATRANSCEIVERITF_BANDWIDTH_125: 
      // 0111
      config1 = config1 | 0b01110000;
      CSX1276_getSF(this);
      if (this->m_usSpreadingFactor == 11 || this->m_usSpreadingFactor == 12)
      { 
        // LowDataRateOptimize (Mandatory with LORATRANSCEIVERITF_BANDWIDTH_125 if LORATRANSCEIVERITF_SF_11 or LORATRANSCEIVERITF_SF_12)
        BYTE config3 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG3);
        config3 = config3 | 0b00001000;
        CSX1276_writeRegister(SPIDeviceHandle, REG_MODEM_CONFIG3, config3);
      }
      break;
    case LORATRANSCEIVERITF_BANDWIDTH_250: 
      // 1000
      config1 = config1 | 0b10000000;
      break;
    case LORATRANSCEIVERITF_BANDWIDTH_500: 
      // 1001
      config1 = config1 | 0B10010000;
      break;
  }
  // Update config1
  CSX1276_writeRegister(SPIDeviceHandle, REG_MODEM_CONFIG1, config1);   

  config1 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG1);

  // (config1 >> 4) ---> take out bits 7-4 from REG_MODEM_CONFIG1 (=_bandwidth)
  switch (BandWidth)
  {
    case LORATRANSCEIVERITF_BANDWIDTH_125: 
      if ((config1 >> 4) == LORATRANSCEIVERITF_BANDWIDTH_125)
      {
        resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
        BYTE config3 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG3);

        if ((this->m_usSpreadingFactor == 11) || (this->m_usSpreadingFactor == 12))
        {
          if (bitRead(config3, 3) != 1)
          {
            // No LowDataRateOptimize
            resultCode = LORATRANSCEIVERITF_RESULT_ERROR;
          }
        }
      }
      break;
    case LORATRANSCEIVERITF_BANDWIDTH_250: 
      if ((config1 >> 4) == LORATRANSCEIVERITF_BANDWIDTH_250)
      {
        resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
      }
      break;
    case LORATRANSCEIVERITF_BANDWIDTH_500: 
      if ((config1 >> 4) == LORATRANSCEIVERITF_BANDWIDTH_500)
      {
        resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
      }
      break;
  }

  if (resultCode != LORATRANSCEIVERITF_RESULT_SUCCESS)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Failed to set bandwidth");
    #endif
  }
  else
  {
    this->m_usBandwidth = BandWidth;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[INFO] Bandwidth ");
      DEBUG_PRINT_HEX(BandWidth);
      DEBUG_PRINT_LN(" has been successfully set");
      DEBUG_PRINT_CR;
    #endif
  }

  // Restore previous OP mode
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, st0);
  }
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_setCR(CSX1276 *this, uint8_t CodingRate)
 * 
 * @brief      Sets the Coding Rate in SX1276.
 * 
 * @details    The function sets the specified Codding Rate for LoRa radio.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *
 * @param      CodingRate
 *             The coding rate value. Allowed values are 'LORATRANSCEIVERITF_CR_x'.
 *             See 'LoraTransceiverItf.h' for details.    
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the 'CR' is set for LoRa radio
 *              - LORATRANSCEIVERITF_RESULT_ERROR = error during configuration
 *              - LORATRANSCEIVERITF_RESULT_INVALIDPARAMS = invalid CR value specified
 *              - LORATRANSCEIVERITF_RESULT_NOTEXECUTED = the operation has not been executed
*********************************************************************************************/
uint8_t CSX1276_setCR(CSX1276 *this, uint8_t CodingRate)
{
  BYTE st0;
  int8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  BYTE config1;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_setCR'");
  #endif

  if (CSX1276_isCR(CodingRate) == false)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[ERROR] Coding Rate ");
      DEBUG_PRINT_HEX(CodingRate);
      DEBUG_PRINT_LN(" is not a correct value");
      DEBUG_PRINT_CR;
    #endif
    return LORATRANSCEIVERITF_RESULT_INVALIDPARAMS;
  }

  if (this->m_usModemMode != MODEM_MODE_LORA)
  {
    // Should never occur: Lora mode must be set first
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[WARNING] LoRa mode not set, activating now");
    #endif
    if ((resultCode = CSX1276_setLoRa(this)) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      return resultCode;
    }
  }

  resultCode = LORATRANSCEIVERITF_RESULT_ERROR;

  // Save the current OP mode
  st0 = CSX1276_readRegister(SPIDeviceHandle, REG_OP_MODE);

  // Set Standby mode to write in registers
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_STANDBY_MODE);
  }

  // Save config1 to modify only the CR and clear bits 3 - 1 from REG_MODEM_CONFIG1
  config1 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG1);  
  config1 = config1 & 0b11110001;	               

  switch (CodingRate)
  {
    case LORATRANSCEIVERITF_CR_5:
      config1 = config1 | 0b00000010;
      break;
    case LORATRANSCEIVERITF_CR_6:
      config1 = config1 | 0b00000100;
      break;
    case LORATRANSCEIVERITF_CR_7:
      config1 = config1 | 0b00000110;
      break;
    case LORATRANSCEIVERITF_CR_8:
      config1 = config1 | 0b00001000;
      break;
  }
  CSX1276_writeRegister(SPIDeviceHandle, REG_MODEM_CONFIG1, config1);    // Update config1

  config1 = CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG1);

  // ((config1 >> 1) & 0b0000111) ---> take out bits 3-1 from REG_MODEM_CONFIG1 (=_codingRate)
  switch (CodingRate)
  {
    case LORATRANSCEIVERITF_CR_5:
      if (((config1 >> 1) & 0b0000111) == 0x01)
      {
        resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
      }
      break;
    case LORATRANSCEIVERITF_CR_6:
      if (((config1 >> 1) & 0b0000111) == 0x02)
      {
        resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
      }
      break;
    case LORATRANSCEIVERITF_CR_7:
      if (((config1 >> 1) & 0b0000111) == 0x03)
      {
        resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
      }
      break;
    case LORATRANSCEIVERITF_CR_8:
      if (((config1 >> 1) & 0b0000111) == 0x04)
      {
        resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
      }
      break;
  }

  if (resultCode != LORATRANSCEIVERITF_RESULT_SUCCESS)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Failed to set Coding Rate");
    #endif
  }
  else
  {
    this->m_usCodingRate = CodingRate;

    // Store current coding rate information (i.e. text value retrieved by client object when a 
    // LoraPacket is received)
    strcpy((char *) this->m_ReceivedPacketInfo.m_szCodingRate, CSX1276_getCRTextValue(CodingRate));

    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[INFO] Coding Rate ");
      DEBUG_PRINT_HEX(CodingRate);
      DEBUG_PRINT_LN(" has been successfully set");
      DEBUG_PRINT_CR;
    #endif
  }

  // Restore previous OP mode
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, st0); 
  }
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_setChannel(CSX1276 *this, uint8_t FreqChannel)
 * 
 * @brief      Sets the Frequency Channel in SX1276.
 * 
 * @details    The function sets the specified Frequency Channel for LoRa radio.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *
 * @param      FreqChannel
 *             The frequency channel value. Allowed values are 'LORATRANSCEIVERITF_FREQUENCY_CHANNEL_xx'.
 *             See 'LoraTransceiverItf.h' for details.    
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the 'FrequencyChannel' is set for LoRa radio
 *              - LORATRANSCEIVERITF_RESULT_ERROR = error during configuration
 *              - LORATRANSCEIVERITF_RESULT_INVALIDPARAMS = invalid FreqChannel value specified
 *              - LORATRANSCEIVERITF_RESULT_NOTEXECUTED = the operation has not been executed
*********************************************************************************************/
uint8_t CSX1276_setChannel(CSX1276 *this, uint8_t FreqChannel)
{
  BYTE st0;
  int8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  uint32_t dwFreqRegValue;
  unsigned int freq3;
  unsigned int freq2;
  uint8_t freq1;
  uint32_t freq;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("Starting 'setChannel'");
  #endif

  if (CSX1276_isChannel(FreqChannel) == false)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[ERROR] Frequency Channel ");
      DEBUG_PRINT_HEX(FreqChannel);
      DEBUG_PRINT_LN(" is not a correct value");
      DEBUG_PRINT_CR;
    #endif
    return LORATRANSCEIVERITF_RESULT_INVALIDPARAMS;
  }

  if (this->m_usModemMode != MODEM_MODE_LORA)
  {
    // Should never occur: Lora mode must be set first
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[WARNING] LoRa mode not set, activating now");
    #endif
    if ((resultCode = CSX1276_setLoRa(this)) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      return resultCode;
    }
  }

  // Save the current OP mode
  st0 = CSX1276_readRegister(SPIDeviceHandle, REG_OP_MODE);  

  // LoRa Stdby mode in order to write in registers
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_STANDBY_MODE);
  }

  resultCode = LORATRANSCEIVERITF_RESULT_ERROR;

  dwFreqRegValue = CSX1276_getFreqRegValue(FreqChannel);

  freq3 = ((dwFreqRegValue >> 16) & 0x0FF);   // frequency channel MSB
  freq2 = ((dwFreqRegValue >> 8) & 0x0FF);    // frequency channel MIB
  freq1 = (dwFreqRegValue & 0xFF);            // frequency channel LSB

  CSX1276_writeRegister(SPIDeviceHandle, REG_FRF_MSB, freq3);
  CSX1276_writeRegister(SPIDeviceHandle, REG_FRF_MID, freq2);
  CSX1276_writeRegister(SPIDeviceHandle, REG_FRF_LSB, freq1);

  // Store MSB in freq channel value
  freq3 = (CSX1276_readRegister(SPIDeviceHandle, REG_FRF_MSB));
  freq = (freq3 << 8) & 0xFFFFFF;

  // Store MID in freq channel value
  freq2 = (CSX1276_readRegister(SPIDeviceHandle, REG_FRF_MID));
  freq = (freq << 8) + ((freq2 << 8) & 0xFFFFFF);

  // Store LSB in freq channel value
  freq = freq + ((CSX1276_readRegister(SPIDeviceHandle, REG_FRF_LSB)) & 0xFFFFFF);

  if (freq == dwFreqRegValue)
  {
    resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
    this->m_usFreqChannel = FreqChannel;
    this->m_dwRegFreqChannel = dwFreqRegValue;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[INFO] Frequency channel ");
      DEBUG_PRINT_HEX(FreqChannel);
      DEBUG_PRINT_LN(" has been successfully set");
      DEBUG_PRINT_CR;
    #endif
  }

  // Restore previous OP mode
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, st0);  
  }
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_setPowerMode(CSX1276 *this, uint8_t PowerMode)
 * 
 * @brief      Sets the Power Mode in SX1276.
 * 
 * @details    The function sets the specified predefined Power level for LoRa radio.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *
 * @param      PowerMode
 *             The power mode value. Allowed values are 'LORATRANSCEIVERITF_POWER_MODE_xx'.
 *             See 'LoraTransceiverItf.h' for details.    
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the 'PowerMode' is set for LoRa radio
 *              - LORATRANSCEIVERITF_RESULT_ERROR = error during configuration
 *              - LORATRANSCEIVERITF_RESULT_INVALIDPARAMS = invalid 'PowerMode' value specified
 *              - LORATRANSCEIVERITF_RESULT_NOTEXECUTED = the operation has not been executed
*********************************************************************************************/
uint8_t CSX1276_setPowerMode(CSX1276 *this, uint8_t PowerMode)
{
  BYTE st0;
  int8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  BYTE value = 0x00;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_setPowerMode'");
  #endif

  switch (PowerMode)
  {
    // LOW  = On SX1272/76: PA0 on RFO setting
    // HIGH = On SX1272/76: PA0 on RFO setting
    // MAX  = On SX1272/76: PA0 on RFO setting

    // BOOST  = extreme; added by C. Pham. On SX1272/76: PA1&PA2 PA_BOOST setting
    // BOOST2 = extreme; added by C. Pham. On SX1272/76: PA1&PA2 PA_BOOST setting + 20dBm settings
    case LORATRANSCEIVERITF_POWER_MODE_BOOST:
    case LORATRANSCEIVERITF_POWER_MODE_BOOST2:
    case LORATRANSCEIVERITF_POWER_MODE_MAX:  
      // SX1272/76: 14dBm
      value = 0x0F;
      break;

    case LORATRANSCEIVERITF_POWER_MODE_LOW:
      // SX1272/76: 2dBm
      value = 0x03;
      break;

    case LORATRANSCEIVERITF_POWER_MODE_HIGH:
      // SX1272/76: 6dBm
      value = 0x07;
      break;

    default:
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT("[ERROR] Power Mode ");
        DEBUG_PRINT_HEX(PowerMode);
        DEBUG_PRINT_LN(" is not a correct value");
        DEBUG_PRINT_CR;
      #endif
      return LORATRANSCEIVERITF_RESULT_INVALIDPARAMS;
  }

  if (this->m_usModemMode != MODEM_MODE_LORA)
  { 
    // Should never occur: Lora mode must be set first
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[WARNING] LoRa mode not set, activating now");
    #endif
    if ((resultCode = CSX1276_setLoRa(this)) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      return resultCode;
    }
  }

  // Save current OP modes
  st0 = CSX1276_readRegister(SPIDeviceHandle, REG_OP_MODE);    

  // LoRa Stdby mode in order to write in registers
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_STANDBY_MODE);
  }

  resultCode = LORATRANSCEIVERITF_RESULT_ERROR;

  // Set power value
  if (PowerMode == LORATRANSCEIVERITF_POWER_MODE_BOOST)
  {
    // we set only the PA_BOOST pin
    // limit to 14dBm
    value = 0x0C;
    value = value | 0b10000000;
    // set RegOcp for OcpOn and OcpTrim
    // 130mA
    CSX1276_setMaxCurrent(this, 0x10);
  }
  else if (PowerMode == LORATRANSCEIVERITF_POWER_MODE_BOOST2) 
  {
    // normally value = 0x0F;
    // we set the PA_BOOST pin
    value = value | 0b10000000;
    // and then set the high output power config with register REG_PA_DAC
    CSX1276_writeRegister(SPIDeviceHandle, 0x4D, 0x87);
    // set RegOcp for OcpOn and OcpTrim
    // 150mA
    CSX1276_setMaxCurrent(this, 0x12);
  }
  else 
  {
    // disable high power output in all other cases
    CSX1276_writeRegister(SPIDeviceHandle, 0x4D, 0x84);

    // Set default max current to 100mA
    CSX1276_setMaxCurrent(this, 0x0B);
  }

  // set MaxPower to 7 -> Pmax=10.8+0.6*MaxPower [dBm] = 15
  value = value | 0b01110000;

  // then Pout = Pmax-(15-_power[3:0]) if  PaSelect=0 (RFO pin for +14dBm)
  // so L=3dBm; H=7dBm; M=15dBm (but should be limited to 14dBm by RFO pin)

  // and Pout = 17-(15-_power[3:0]) if  PaSelect=1 (PA_BOOST pin for +14dBm)
  // so x= 14dBm (PA);
  // when p=='X' for 20dBm, value is 0x0F and RegPaDacReg=0x87 so 20dBm is enabled
  CSX1276_writeRegister(SPIDeviceHandle, REG_PA_CONFIG, value); // Setting output power value
  this->m_usPowerLevel = value;

  value = CSX1276_readRegister(SPIDeviceHandle, REG_PA_CONFIG);

  if (value == this->m_usPowerLevel)
  {
    this->m_usPowerMode = PowerMode;
    resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] Power Mode successfully set");
      DEBUG_PRINT_CR;
    #endif
  }
  else
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Failed to set Power Mode");
      DEBUG_PRINT_CR;
    #endif
  }

  // Restore previous OP mode
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, st0);  
  }
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_setPowerLevel(CSX1276 *this, uint8_t PowerLevel)
 * 
 * @brief      Sets the Power Level in SX1276.
 * 
 * @details    The function sets the specified Output Power Level for LoRa radio.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *
 * @param      PowerLevel
 *             The output power level value. The level values are in range from 0 to 14dBm.
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the 'PowerLevel' is set for LoRa radio
 *              - LORATRANSCEIVERITF_RESULT_ERROR = error during configuration
 *              - LORATRANSCEIVERITF_RESULT_INVALIDPARAMS = invalid 'PowerLevel' value specified
 *              - LORATRANSCEIVERITF_RESULT_NOTEXECUTED = the operation has not been executed
 *
 * @note       Typically, the maximum current (OCP) must adjusted according to required power
 *             level. This adjustment is made automatically by 'CSX1276_setPowerMode' method.
 *             When output power is set using 'CSX1276_setPowerLevel' method, an explicit
 *             call to 'CSX1276_setMaxCurrent' function is generaly required. 
*********************************************************************************************/
uint8_t CSX1276_setPowerLevel(CSX1276 *this, uint8_t PowerLevel)
{
  BYTE st0;
  uint8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  BYTE value = 0x00;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_setPowerLevel'");
  #endif

  // Maximum power 14dBm
  if (PowerLevel > 14)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[ERROR] Power Level ");
      DEBUG_PRINT_HEX(PowerLevel);
      DEBUG_PRINT_LN(" is not a correct value");
      DEBUG_PRINT_CR;
    #endif
    return LORATRANSCEIVERITF_RESULT_INVALIDPARAMS;
  }

  if (this->m_usModemMode != MODEM_MODE_LORA)
  { 
    // Should never occur: Lora mode must be set first
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[WARNING] LoRa mode not set, activating now");
    #endif
    if ((resultCode = CSX1276_setLoRa(this)) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      return resultCode;
    }
  }

  // Save the currenrt OP mode
  st0 = CSX1276_readRegister(SPIDeviceHandle, REG_OP_MODE);    

  // LoRa Standby mode to write in registers
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_STANDBY_MODE);
  }

  resultCode = LORATRANSCEIVERITF_RESULT_ERROR;

  this->m_usPowerLevel = PowerLevel;

  // Clear OutputPower, but keep current value of PaSelect and MaxPower
  value = CSX1276_readRegister(SPIDeviceHandle, REG_PA_CONFIG);
  value = value & 0b11110000;
  value = value + this->m_usPowerLevel;
  this->m_usPowerLevel = value;

  // Set output power value
  CSX1276_writeRegister(SPIDeviceHandle, REG_PA_CONFIG, this->m_usPowerLevel); 
  value = CSX1276_readRegister(SPIDeviceHandle, REG_PA_CONFIG);

  if (value == this->m_usPowerLevel)
  {
    this->m_usPowerMode = SX1276_POWER_MODE_CUSTOM;
    resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] Output power level has been successfully set");
      DEBUG_PRINT_CR;
    #endif
  }
  else
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Filed to set output power level");
      DEBUG_PRINT_CR;
    #endif
  }

  // Restore previous OP mode
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, st0);  
  }
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_setPreambleLength(CSX1276 *this, uint16_t PreambleLength)
 * 
 * @brief      Sets the Preamble Length in SX1276.
 * 
 * @details    The function sets the specified Preamble Length for LoRa radio.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *
 * @param      PreambleLength
 *             The value to set as preamble length. The standard value for LoRa network is 8.
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the 'PreambleLength' is set for LoRa radio
 *              - LORATRANSCEIVERITF_RESULT_ERROR = error during configuration
 *              - LORATRANSCEIVERITF_RESULT_NOTEXECUTED = the operation has not been executed
*********************************************************************************************/
uint8_t CSX1276_setPreambleLength(CSX1276 *this, uint16_t PreambleLength)
{
  BYTE st0;
  uint8_t p_length;
  int8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_setPreambleLength'");
  #endif

  if (this->m_usModemMode != MODEM_MODE_LORA)
  { 
    // Should never occur: Lora mode must be set first
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[WARNING] LoRa mode not set, activating now");
    #endif
    if ((resultCode = CSX1276_setLoRa(this)) != LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      return resultCode;
    }
  }

  // Save the current OP mode
  st0 = CSX1276_readRegister(SPIDeviceHandle, REG_OP_MODE); 
   
  resultCode = LORATRANSCEIVERITF_RESULT_ERROR;

  // Set Standby mode to write in registers
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_STANDBY_MODE);    
  }

  p_length = ((PreambleLength >> 8) & 0x0FF);

  // Store MSB preamble length for LoRa mode
  CSX1276_writeRegister(SPIDeviceHandle, REG_PREAMBLE_MSB_LORA, p_length);
  p_length = (PreambleLength & 0x0FF);

  // Store LSB preamble length for LoRa mode
  CSX1276_writeRegister(SPIDeviceHandle, REG_PREAMBLE_LSB_LORA, p_length);

  resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;

  this->m_wPreambleLength = PreambleLength;
   
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT("[INFO] Preamble length ");
    DEBUG_PRINT_HEX(PreambleLength);
    DEBUG_PRINT_LN(" has been successfully set");
    DEBUG_PRINT_CR;
  #endif

  // Restore previous OP mode
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, st0);  
  }
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_getSNR(CSX1276 *this)
 * 
 * @brief      Gets the SNR in SX1276.
 * 
 * @details    The function gets SNR from SX1276 register and store the value in 'm_SNR' member
 *             variable of CSX1276 object.\n
 *             The SNR is available only for LoRa radio.\n
 *             The LoRa radio mode must be configured before calling this function.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the 'SNR' is stored in member variable
 *              - LORATRANSCEIVERITF_RESULT_INVALIDSTATE = LoRa mode not configured before 
 *                                                         calling the function
*********************************************************************************************/
uint8_t CSX1276_getSNR(CSX1276 *this)
{ 
  int8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  BYTE value;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_getSNR'");
  #endif

  if (this->m_usModemMode == MODEM_MODE_LORA)
  { 
    // 'getSNR' exists only in LoRa mode
    value = CSX1276_readRegister(SPIDeviceHandle, REG_PKT_SNR_VALUE);

    // The SNR sign bit is 1
    if (value & 0x80) 
    {
      // Invert and divide by 4
      value = ((~value + 1) & 0xFF) >> 2;
      this->m_nSNRPacket = -value;
    }
    else
    {
      // Divide by 4
      this->m_nSNRPacket = (value & 0xFF) >> 2;
    }
    resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[INFO] SNR value is ");
      DEBUG_PRINT_DEC(this->m_nSNRPacket);
      DEBUG_PRINT_CR;
    #endif
  }
  else
  { 
    // LoRa mode must be set before calling this function
    resultCode = LORATRANSCEIVERITF_RESULT_INVALIDSTATE;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] LoRa mode must be configured before calling function");
      DEBUG_PRINT_CR;
    #endif
  }
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_getRSSI(CSX1276 *this)
 * 
 * @brief      Gets the current value of RSSI in SX1276.
 * 
 * @details    The function gets current value of RSSI from SX1276 register and store the 
 *             value in 'm_RSSI' member variable of CSX1276 object.\n
 *             The LoRa radio mode must be configured before calling this function.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the 'RSSI' is stored in member variable
 *              - LORATRANSCEIVERITF_RESULT_INVALIDSTATE = LoRa mode not configured before 
 *                                                         calling the function
*********************************************************************************************/
uint8_t CSX1276_getRSSI(CSX1276 *this)
{
  uint8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  int rssi_mean = 0;
  int total = 5;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_getRSSI'");
  #endif

  if (this->m_usModemMode == MODEM_MODE_LORA)
  { 
    // Get mean value of RSSI
    for (int i = 0; i < total; i++)
    {
      this->m_nRSSI = -OFFSET_RSSI + CSX1276_readRegister(SPIDeviceHandle, REG_RSSI_VALUE_LORA);
      rssi_mean += this->m_nRSSI;     
    }
    rssi_mean = rssi_mean / total;  
    this->m_nRSSI = rssi_mean;
    
    resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[INFO] RSSI value is ");
      DEBUG_PRINT_DEC(this->m_nRSSI);
      DEBUG_PRINT_CR;
    #endif  
  }
  else
  {
    // LoRa mode must be set before calling this function
    resultCode = LORATRANSCEIVERITF_RESULT_INVALIDSTATE;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] LoRa mode must be configured before calling function");
      DEBUG_PRINT_CR;
    #endif
  }
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_getRSSIpacket(CSX1276 *this)
 * 
 * @brief      Gets the RSSI of last received packet.
 * 
 * @details    The function retrieves in SX1276 register the RSSI of last received packet and
 *             stores the value in 'm_RSSIpacket' member variable of CSX1276 object.\n
 *             The RSSI for last received packet is available only for LoRa radio.\n
 *             The LoRa radio mode must be configured before calling this function.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the last packet 'RSSI' is stored in 
 *                 member variable
 *              - LORATRANSCEIVERITF_RESULT_INVALIDSTATE = LoRa mode not configured before 
 *                 calling the function
*********************************************************************************************/
uint8_t CSX1276_getRSSIpacket(CSX1276 *this)
{ 
  uint8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_getRSSIpacket'");
  #endif

  if (this->m_usModemMode == MODEM_MODE_LORA)
  {
    // RSSIpacket only exists in LoRa 
    resultCode = CSX1276_getSNR(this);
    if (resultCode == LORATRANSCEIVERITF_RESULT_SUCCESS)
    {
      if (this->m_nSNRPacket < 0)
      {
        this->m_nRSSIPacket = -NOISE_ABSOLUTE_ZERO + 10.0 * SignalBwLog[this->m_usBandwidth] + NOISE_FIGURE + (double)this->m_nSNRPacket;
      }
      else
      {
        this->m_nRSSIPacket = CSX1276_readRegister(SPIDeviceHandle, REG_PKT_RSSI_VALUE);
        this->m_nRSSIPacket = -OFFSET_RSSI + (double)this->m_nRSSIPacket;
      }
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT("## RSSI packet value is ");
        DEBUG_PRINT_DEC(this->m_nRSSIPacket);
        DEBUG_PRINT_LN(" ##");
        DEBUG_PRINT_CR;
      #endif
    }
  }
  else
  { 
    // LoRa mode must be set before calling this function
    resultCode = LORATRANSCEIVERITF_RESULT_INVALIDSTATE;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] LoRa mode must be configured before calling function");
      DEBUG_PRINT_CR;
    #endif
  }
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_setRetries(CSX1276 *this, uint8_t RetryNumber)
 * 
 * @brief      Sets the maximum number of send retries.
 * 
 * @details    The function sets the maximum number of send retries (i.e. for acknowledged
 *             packets).\n
 *             This value is stored in 'm_maxRetries' member variable of CSX1276 object.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *
 * @param      RetryNumber
 *             The number of send retries to set for the CSX1276 object.
 *             The maximum allowed value is defined by 'LORATRANSCEIVERITF_MAX_SEND_RETRIES'
 *             in 'LoraTransceiverItf.h'.
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the number of retries is stored in 
 *                 member variable
 *              - LORATRANSCEIVERITF_RESULT_INVALIDPARAMS = the specified 'RetryNumber' value
 *                 is out of range.
*********************************************************************************************/
uint8_t CSX1276_setRetries(CSX1276 *this, uint8_t RetryNumber)
{
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_setRetries'");
  #endif

  if (RetryNumber > LORATRANSCEIVERITF_MAX_SEND_RETRIES)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[ERROR] Retries value cannot be greater than ");
      DEBUG_PRINT_DEC(LORATRANSCEIVERITF_MAX_SEND_RETRIES);
      DEBUG_PRINT_CR;
    #endif
    return LORATRANSCEIVERITF_RESULT_INVALIDPARAMS;
  }

  this->m_usMaxRetries = RetryNumber;
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT("[INFO] Maximum retries value set to ");
    DEBUG_PRINT_DEC(this->m_usMaxRetries);
    DEBUG_PRINT_CR;
  #endif
  return LORATRANSCEIVERITF_RESULT_SUCCESS;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_setMaxCurrent(CSX1276 *this, uint8_t OcpRate)
 * 
 * @brief      Sets the maximum output current in SX1276.
 * 
 * @details    The function limits the current supply of the power amplifier (typically to
 *             protect battery chemistries).
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *
 * @param      OcpRate
 *             The Over Current Protection rate to set in SX1276 registry.
 *             The maximum allowed value is defined by 'LORATRANSCEIVERITF_OCP_MAX'
 *             in 'LoraTransceiverItf.h'.
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the number of retries is stored in 
 *                 member variable
 *              - LORATRANSCEIVERITF_RESULT_INVALIDPARAMS = the specified 'RetryNumber' value
 *                 is out of range.
 *
 * @note       Maximum current is computed as follows:
 *              - if OcpTrim <= 15, Imax = 45+5*OcpRate [mA] (120 mA)
 *              - if 15 < OcpRate <= 27, Imax = -30+10*OcpRate [mA] (130 to 240 mA)
*********************************************************************************************/
uint8_t CSX1276_setMaxCurrent(CSX1276 *this, uint8_t OcpRate)
{
  BYTE st0;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_setMaxCurrent'");
  #endif

  // Check maximum OCP rate value (maximum current supply is 240 mA)
  if (OcpRate > LORATRANSCEIVERITF_OCP_MAX)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[ERROR] Maximum current supply is 240 mA, so maximum parameter value is ");
      DEBUG_PRINT_DEC(LORATRANSCEIVERITF_OCP_MAX);
      DEBUG_PRINT_CR;
    #endif
    return LORATRANSCEIVERITF_RESULT_INVALIDPARAMS;
  }

  if (this->m_usModemMode != MODEM_MODE_LORA)
  {
    // LoRa mode must be set before calling this function
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] LoRa mode must be configured before calling function");
      DEBUG_PRINT_CR;
    #endif
    return LORATRANSCEIVERITF_RESULT_INVALIDSTATE;
  }

  // Save the current OP mode
  st0 = CSX1276_readRegister(SPIDeviceHandle, REG_OP_MODE);  

  // Set LoRa Standby mode to write in registers
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_STANDBY_MODE);  
  }

  // Enable Over Current Protection
  CSX1276_writeRegister(SPIDeviceHandle, REG_OCP, OcpRate | 0b00100000); 
  this->m_usOcpRate = OcpRate;

  // Restore previous OP mode
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, st0);    
  }
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT("[INFO] Maximum current protection set to ");
    DEBUG_PRINT_DEC(OcpRate);
    DEBUG_PRINT_CR;
  #endif
  return LORATRANSCEIVERITF_RESULT_SUCCESS;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_getTemp(CSX1276 *this)
 * 
 * @brief      Gets the temperature of SX1276 device.
 * 
 * @details    The function retrieves current device temperature in SX1276 register and
 *             stores the value in 'm_temp' member variable of CSX1276 object.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the device temperature is stored in 
 *                 member variable
 *              - LORATRANSCEIVERITF_RESULT_INVALIDSTATE = LoRa mode not configured before 
 *                 calling the function
*********************************************************************************************/
uint8_t CSX1276_getTemp(CSX1276 *this)
{
  BYTE st0;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;
  int nTemp;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("Starting 'getTemp'");
  #endif

  if (this->m_usModemMode == MODEM_MODE_LORA)
  { 
    // LoRa mode must be set before calling this function
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] LoRa mode must be configured before calling function");
      DEBUG_PRINT_CR;
    #endif
    return LORATRANSCEIVERITF_RESULT_INVALIDSTATE;
  }

  // Save the current OP mode
  st0 = CSX1276_readRegister(SPIDeviceHandle, REG_OP_MODE);  

  // Allowing access to FSK registers while in LoRa standby mode
  CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_STANDBY_FSK_REGS_MODE);

  // Saving temperature value
  nTemp = CSX1276_readRegister(SPIDeviceHandle, REG_TEMP);

  // Check SNR sign bit
  if (nTemp & 0x80) 
  {
    // The SNR sign bit is 1, invert and divide by 4
    nTemp = ((~nTemp + 1) & 0xFF);
  }
  else
  {
    // Divide by 4
    nTemp = (nTemp & 0xFF);
  }

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT("[INFO] Temperature is: ");
    DEBUG_PRINT_DEC(nTemp);
    DEBUG_PRINT_CR;
  #endif
  this->m_nTemp = nTemp;

  // Restore previous OP mode
  CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, st0);  

  return LORATRANSCEIVERITF_RESULT_SUCCESS;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_getSF(CSX1276 *this)
 * 
 * @brief      Gets the Spreading Factor in SX1276.
 * 
 * @details    The function retrieves, in SX1276 register, the Spreading Factor and stores
 *             the value in 'm_usSpreadingFactor' member variable of CSX1276 object.\n
 *             The LoRa radio mode must be configured before calling this function.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the 'Spreading Factor' is stored in 
 *                 member variable
 *              - LORATRANSCEIVERITF_RESULT_INVALIDSTATE = LoRa mode not configured before 
 *                 calling the function
 *              - LORATRANSCEIVERITF_RESULT_ERROR = the retrieved 'Spreading Factor' is not
 *                 allowed
*********************************************************************************************/
uint8_t CSX1276_getSF(CSX1276 *this)
{
  int8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  BYTE config2;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_getSF'");
  #endif

  if (this->m_usModemMode != MODEM_MODE_LORA)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] LoRa mode must be configured before calling function");
      DEBUG_PRINT_CR;
    #endif
    return LORATRANSCEIVERITF_RESULT_INVALIDSTATE;
  }

  // Take out bits 7-4 from REG_MODEM_CONFIG2 indicates _spreadingFactor
  config2 = (CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG2)) >> 4;

  if (CSX1276_isSF(config2))
  {
    this->m_usSpreadingFactor = config2;
    resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[INFO] Spreading factor is ");
      DEBUG_PRINT_HEX(config2);
      DEBUG_PRINT_CR;
    #endif
  }
  else
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[ERROR] Retrieved Spreading Factor not allowed, value is: ");
      DEBUG_PRINT_HEX(config2);
      DEBUG_PRINT_CR;
    #endif
    resultCode = LORATRANSCEIVERITF_RESULT_ERROR;
  }
  
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_getBW(CSX1276 *this)
 * 
 * @brief      Gets the Bandwidth in SX1276.
 * 
 * @details    The function retrieves the Bandwidth in SX1276 register and stores the value
 *             in 'm_usBandwidth' member variable of CSX1276 object.\n
 *             The LoRa radio mode must be configured before calling this function.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the 'Bandwidth' is stored in member
 *                 variable
 *              - LORATRANSCEIVERITF_RESULT_INVALIDSTATE = LoRa mode not configured before 
 *                 calling the function
 *              - LORATRANSCEIVERITF_RESULT_ERROR = the retrieved 'Bandwidth' is not allowed
*********************************************************************************************/
uint8_t CSX1276_getBW(CSX1276 *this)
{
  uint8_t resultCode = LORATRANSCEIVERITF_RESULT_NOTEXECUTED;
  BYTE config1;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_getBW'");
  #endif

  if (this->m_usModemMode != MODEM_MODE_LORA)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] LoRa mode must be configured before calling function");
      DEBUG_PRINT_CR;
    #endif
    return LORATRANSCEIVERITF_RESULT_INVALIDSTATE;
  }

  // Take out bits 7-4 from REG_MODEM_CONFIG1 indicates _bandwidth
  config1 = (CSX1276_readRegister(SPIDeviceHandle, REG_MODEM_CONFIG1)) >> 4;

  if (CSX1276_isBW(config1))
  {
    this->m_usBandwidth = config1;
    resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[INFO] Bandwidth is ");
      DEBUG_PRINT_HEX(config1);
      DEBUG_PRINT_CR;
    #endif
  }
  else
  {
    resultCode = LORATRANSCEIVERITF_RESULT_ERROR;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[ERROR] Retrieved Bandwidth not allowed, value is: ");
      DEBUG_PRINT_HEX(config1);
      DEBUG_PRINT_CR;
    #endif
  }
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_setPacketLength(CSX1276 *this, uint8_t PacketLength)
 * 
 * @brief      Sets the packet length in SX1276.
 * 
 * @details    The function sets the expected payload length (used in implicit header mode).
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *
 * @param      PacketLength
 *             The payload length to set in SX1276 registry.
 *
 * @return     The function returns one of the following a result codes:
 *              - LORATRANSCEIVERITF_RESULT_SUCCESS = the packet length is set in SX1276/
 *              - LORATRANSCEIVERITF_RESULT_ERROR = error during configuration
 *              - LORATRANSCEIVERITF_RESULT_INVALIDSTATE = function called on invalid state
*********************************************************************************************/
uint8_t CSX1276_setPacketLength(CSX1276 *this, uint8_t PacketLength)
{
  BYTE st0;
  BYTE value = 0x00;
  uint8_t resultCode = LORATRANSCEIVERITF_RESULT_ERROR;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("Starting 'CSX1276_setPacketLength'");
  #endif

  if (this->m_usModemMode != MODEM_MODE_LORA)
  { 
    // LoRa mode must be set before calling this function
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] LoRa mode must be configured before calling function");
      DEBUG_PRINT_CR;
    #endif
    return LORATRANSCEIVERITF_RESULT_INVALIDSTATE;
  }

  // Save the current OP mode
  st0 = CSX1276_readRegister(SPIDeviceHandle, REG_OP_MODE);  
  

  // Set LoRa Standby mode to write in registers
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_STANDBY_MODE);    
  }

  // Set packet length in register
  CSX1276_writeRegister(SPIDeviceHandle, REG_PAYLOAD_LENGTH_LORA, PacketLength);

  // Check length in register
  value = CSX1276_readRegister(SPIDeviceHandle, REG_PAYLOAD_LENGTH_LORA);

  if (PacketLength == value)
  {
//    this->m_pPacketSent->length = PacketLength;
    resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[INFO] Packet length ");
      DEBUG_PRINT_DEC(PacketLength);
      DEBUG_PRINT_LN(" has been successfully set");
      DEBUG_PRINT_CR;
    #endif
  }
  else
  {
    resultCode = LORATRANSCEIVERITF_RESULT_ERROR;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Failed to set packet length in SX1276");
      DEBUG_PRINT_CR;
    #endif
  }

  // Restore to previous OP mode
  if (st0 != LORA_STANDBY_MODE)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, st0);  
  }

  // To Check => removed !!!
//vTaskDelay(pdMS_TO_TICKS(250));

  return resultCode;
}

/*********************************************************************************************
  Private methods (implementation)

  SX1276 packet receive and send operations.

  These functions are invoked by the main automaton (interface commands or notifications
  from ISR).
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_startStandBy(CSX1276 *this)
 * 
 * @brief      Starts the standby mode in SX1276.
 * 
 * @details    The function sets the SX1276 modem in standby mode.\n
 *             The device radio activity is stopped and LoRa packets are not received and 
 *             cannot be sent until the modem mode is explicitly changed (i.e. by invoking
 *             'CSX1276_startReceive' or 'CSX1276_startSent' method.\n
 *             The hardware interrupt request handlers for 'RX_DONE' and 'TX_DONE' interrupts
 *             are disabled.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @return     The function returns 'LORATRANSCEIVERITF_RESULT_SUCCESS' if the device is
 *             in standby mode or 'LORATRANSCEIVERITF_RESULT_ERROR' in case of error.
*********************************************************************************************/
uint8_t CSX1276_startStandBy(CSX1276 *this)
{
  BYTE usRegValue;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;
  
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("Starting 'CSX1276_startStandBy'");
  #endif

  // Disable dectection of 'PACKET_RECEIVED' and 'PACKET_SENT' IRQ
  gpio_intr_disable(PIN_NUM_RX_TX_IRQ); 

  // Change modem mode in SX1276
  CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_STANDBY_MODE);		

  // Delay 100ms -> From libelium/CPham -> removed by FF, TO CHECK
  //vTaskDelay(pdMS_TO_TICKS(100));

  usRegValue = CSX1276_readRegister(SPIDeviceHandle, REG_OP_MODE);		

  if (usRegValue != LORA_STANDBY_MODE)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Failed to set STANDBY mode in SX1276");
    #endif
    return LORATRANSCEIVERITF_RESULT_ERROR;
  }

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] StandBy mode successfully started in SX1276");
  #endif

  return LORATRANSCEIVERITF_RESULT_SUCCESS;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_startReceive(CSX1276 *this)
 * 
 * @brief      Starts the receive mode in SX1276.
 * 
 * @details    The function sets the SX1276 in receive mode.\n
 *             If a LoRa packet is received, it stored in the SX1276 FIFO.\n
 *             The device is configured to accept a paylod of 'LORA_MAX_PAYLOAD_LENGTH' bytes.\n
 *             When the packet is received, a hardware 'RX_DONE' interrupt request is generated.
 *             The function enables the reception of this interrupt on corresponding GPIO of
 *             ESP32 device (i.e. 'PIN_NUM_RX_TX_IRQ').
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @return     The function returns 'LORATRANSCEIVERITF_RESULT_SUCCESS' if the device is
 *             in receiving mode or 'LORATRANSCEIVERITF_RESULT_ERROR' in case of error.
 *
 * @note       The SX1276 'RX_DONE' pin must be connected to 'PIN_NUM_RX_TX_IRQ' pin on ESP32.
*********************************************************************************************/
uint8_t CSX1276_startReceive(CSX1276 *this)
{
  uint8_t resultCode = LORATRANSCEIVERITF_RESULT_ERROR;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;
  
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("Starting 'CSX1276_startReceive'");
  #endif

  // Clear m_pPacketReceived struct
  memset(this->m_pPacketReceived, 0x00, sizeof(CLoraPacket));  

  // Set Testmode
  CSX1276_writeRegister(SPIDeviceHandle, 0x31, 0x43);

  // Set LowPnTxPllOff 
  CSX1276_writeRegister(SPIDeviceHandle, REG_PA_RAMP, 0x09);

  // Set LNA gain: Highest gain. LnaBoost:Improved sensitivity
  CSX1276_writeRegister(SPIDeviceHandle, REG_LNA, 0x23);

  // Setting address pointer in FIFO data buffer    
  CSX1276_writeRegister(SPIDeviceHandle, REG_FIFO_ADDR_PTR, 0x00);   

  // Change RegSymbTimeoutLsb 
  CSX1276_writeRegister(SPIDeviceHandle, REG_SYMB_TIMEOUT_LSB, 0xFF);

  // Set current value of reception buffer pointer
  CSX1276_writeRegister(SPIDeviceHandle, REG_FIFO_RX_BYTE_ADDR, 0x00); 
  
  // Set packet length in order to get all packets with length <= LORA_MAX_PAYLOAD_LENGTH  
  if ((resultCode = CSX1276_setPacketLength(this, LORA_MAX_PAYLOAD_LENGTH)) == LORATRANSCEIVERITF_RESULT_SUCCESS)
  {
    CSX1276_clearFlags(this); 

    // Set SX1276 DIO0 for RX_DONE IRQ (bits 6-7)
    CSX1276_writeRegister(SPIDeviceHandle, REG_DIO_MAPPING1, 0b00000000);     

    // Enable 'PACKET_RECEIVED' IRQ detection (on ESP32)
    gpio_intr_enable(PIN_NUM_RX_TX_IRQ); 

    // Set LORA mode - Rx
    CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_RX_MODE);     

    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] Receiving mode successfully started in SX1276");
    #endif
  }
  else
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Failed to start receiving mode in SX1276");
    #endif
  }
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_getPacket(CSX1276 *this)
 * 
 * @brief      Transfers received packet from SX1276 to CSX1276 object.
 * 
 * @details    The function reads the received packet bytes in SX1276 FIFO and copies them in
 *             'm_pPacketReceived' member variable of CSX1276 object.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @return     The function returns 'LORATRANSCEIVERITF_RESULT_SUCCESS' if bytes are transfered
 *             in CSX1276 object or 'LORATRANSCEIVERITF_RESULT_ERROR' in case of error.
 *
 * @note       On exit, the function clears flag register of SX1276 are sets FIFO base pointer
 *             to 0x00.
*********************************************************************************************/
uint8_t CSX1276_getPacket(CSX1276 *this)
{
  uint8_t resultCode;
  BYTE value;
  BYTE usReceivedBytesNum;
  struct timeval tmNow; 
  bool bPacketReceived = false;
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_getPacket'");
  #endif

  // Debug -> duration
  //previous = xTaskGetTickCount();
  
  // Check if 'RxDone' is true and 'PayloadCrcError' is correct
  value = CSX1276_readRegister(SPIDeviceHandle, REG_IRQ_FLAGS);
  if ((bitRead(value, 6) == 1) && (bitRead(value, 5) == 0))
  { 
    // Packet received and CRC correct
    bPacketReceived = true;
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[INFO] Packet properly received");
    #endif
  }
  else
  {
    // NOTE: Should never reach this point with HW IRQ for 'RxDone'

    if (bitRead(value, 6) != 1)
    { 
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] NOT 'RxDone' flag");
      #endif
    }
    
    if (this->m_usCRC != SX1276_CRC_ON)
    { 
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] NOT 'CRC_ON' enabled");
      #endif
    }
    
    if ((bitRead(value, 5) == 0) && (this->m_usCRC == SX1276_CRC_ON))
    { 
      // CRC is correct
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[INFO] The CRC is correct");
      #endif
    }
    else
    { 
      // CRC incorrect
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] The CRC is incorrect");
      #endif
    }
  }     
  
  // Transfer packet from SX1276 to 'm_pPacketReceived' object if properly received
  if (bPacketReceived == true)
  { 
    CLoraPacket *pPacketReceived = this->m_pPacketReceived;
    
    // Check if 'm_pPacketReceived' is available for receiving data
    // (i.e. owner object must read previous received packet before receiving another packet)
    if (pPacketReceived->m_dwDataSize != 0)
    {
      // Wait a bit (but not too much)
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[WARNING] Previous packet still in buffer");
      #endif

      vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    if (pPacketReceived->m_dwDataSize != 0)
    {
      // Miss this packet
      ++this->m_dwMissedPacketReceivedNumber;
      resultCode = LORATRANSCEIVERITF_RESULT_ERROR;

      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT("[ERROR] Previous packet still in buffer, total missed: ");
        DEBUG_PRINT_DEC(this->m_dwMissedPacketReceivedNumber);
        DEBUG_PRINT_CR;
      #endif
    }
    else
    {
      // Receive buffer available
      pPacketReceived->m_dwTimestamp = xTaskGetTickCount() * portTICK_RATE_MS;
      usReceivedBytesNum = CSX1276_readRegister(SPIDeviceHandle, REG_RX_NB_BYTES);
      pPacketReceived->m_dwDataSize = (DWORD) usReceivedBytesNum;
  
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT("[INFO] Received byte number: ");
        DEBUG_PRINT_HEX(usReceivedBytesNum);
        DEBUG_PRINT_CR;
      #endif
  
      // Store the packet
      // Set address pointer in FIFO data buffer
      CSX1276_writeRegister(SPIDeviceHandle, REG_FIFO_ADDR_PTR, 0x00); 
  
      for (unsigned int i = 0; i < usReceivedBytesNum; i++)
      {
        pPacketReceived->m_usData[i] = CSX1276_readRegister(SPIDeviceHandle, REG_FIFO);
      }
     
      // Retrieve RSSI information for received packet
      // Note: 'm_nRSSIPacket' and 'm_nSNRPacket' member variables are updated
      CSX1276_getRSSIpacket(this);

      // Lora SNR ratio in dB (signed float, 0.1 dB precision)
      sprintf((char *) this->m_ReceivedPacketInfo.m_szSNR, "%.1lf", (double) this->m_nSNRPacket);

      //  RSSI in dBm (signed integer, 1 dB precision)
      sprintf((char *) this->m_ReceivedPacketInfo.m_szRSSI, "%d", (int) this->m_nRSSIPacket);
       
      // UTC timestamp
      gettimeofday(&tmNow, NULL); 
      this->m_ReceivedPacketInfo.m_dwUTCSec = tmNow.tv_sec;
      this->m_ReceivedPacketInfo.m_dwUTCMicroSec = tmNow.tv_usec;

      // Print the packet if debug_mode
      #if (SX1276_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[INFO] Payload data:");
        for (unsigned int i = 0; i < usReceivedBytesNum; i++)
        {
          DEBUG_PRINT_HEX(pPacketReceived->m_usData[i]);   
          DEBUG_PRINT("|");
        }
        DEBUG_PRINT_CR;
        DEBUG_PRINT_LN("## Packet end");
      #endif
  
      resultCode = LORATRANSCEIVERITF_RESULT_SUCCESS;
    }
  }
  else
  {
    // Packet NOT received
    resultCode = LORATRANSCEIVERITF_RESULT_ERROR;

    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Failed to transfer packet from SX1276");
    #endif
  }
  
  // Set address pointer in FIFO data buffer to 0x00 again
  CSX1276_writeRegister(SPIDeviceHandle, REG_FIFO_ADDR_PTR, 0x00);
  
  // Initialize flags 
  CSX1276_clearFlags(this); 
  
  return resultCode;
}

/*****************************************************************************************//**
 * @fn         uint8_t CSX1276_startSend(CSX1276 *this, CLoraTransceiverItf_LoraPacket pLoraPacket)
 * 
 * @brief      Transfers specified LoRa packet to SX1276 and starts sending it.
 * 
 * @details    The function copies LoRa packet payload bytes in SX1276 FIFO and starts to 
 *             transmit them over radio.\n
 *             The SX1276 must be in 'STANDBY' mode before calling this function.
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @param      pLoraPacket
 *             The LoRa packet to send.
 *  
 * @return     The function returns 'LORATRANSCEIVERITF_RESULT_SUCCESS' if SX1276 is sending
 *             packet or 'LORATRANSCEIVERITF_RESULT_INVALIDSTATE' if SX1276 is not in 'STANDBY'
 *             mode.
 *
 * @note       The SX1276 will trigger a 'TX_DONE' IRQ when packet is transmitted.\n
 *             When send operation terminates, the SX1276 automatically returns to 'STANDBY' 
 *             mode.
*********************************************************************************************/
uint8_t CSX1276_startSend(CSX1276 *this, CLoraTransceiverItf_LoraPacket pLoraPacket)
{
  register spi_device_handle_t SPIDeviceHandle = this->m_SpiDeviceHandle;
  
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("Starting 'CSX1276_startSend'");
  #endif

  // The SX1276 must be in 'STANDBY' mode
  if (CSX1276_readRegister(SPIDeviceHandle, REG_OP_MODE) != LORA_STANDBY_MODE)
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] SX1276 not in 'STANDBY' mode");
    #endif

    return LORATRANSCEIVERITF_RESULT_INVALIDSTATE;
  }

  // Keep pointer to packet currently sent
  // Note: The pointed object will never been accessed outside of this function
  //       (i.e. just a reference used for 'PACKET_SENT' event)
  this->m_pPacketToSend = pLoraPacket;

  // Write payload to send in SX1276 FIFO
  // Set address pointer in FIFO data buffer
  CSX1276_writeRegister(SPIDeviceHandle, REG_FIFO_TX_BASE_ADDR, 0x00);
  CSX1276_writeRegister(SPIDeviceHandle, REG_FIFO_ADDR_PTR, 0x00);  

  // Write bytes in FIFO
  BYTE *pData = pLoraPacket->m_usData;
  for (DWORD i = 0; i < pLoraPacket->m_dwDataSize; i++, pData++)
  {
    CSX1276_writeRegister(SPIDeviceHandle, REG_FIFO, *pData);
  }

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Packet bytes copied in FIFO");
    DEBUG_PRINT("Bytes to send: ");
    pData = pLoraPacket->m_usData;
    for (DWORD i = 0; i < pLoraPacket->m_dwDataSize; i++, pData++)
    {
      DEBUG_PRINT_HEX(*pData);
      DEBUG_PRINT("|");
    }
    DEBUG_PRINT_LN("End payload bytes");
  #endif

  // Switch SX1276 to TX mode (i.e. send packet)
  // Notes: 
  //  - The end of send operation will be dectected by 'TX_DONE' IRQ
  //  - When send operation terminates, the SX1276 automatically returns to 'STANDBY' mode
  CSX1276_clearFlags(this); 

  // Set SX1276 DIO0 for TX_DONE IRQ (bits 6-7)
  CSX1276_writeRegister(SPIDeviceHandle, REG_DIO_MAPPING1, 0b01000000);     

  // Enable 'PACKET_SENT' IRQ detection (on ESP32)
  gpio_intr_enable(PIN_NUM_RX_TX_IRQ); 

  // Start to send packet
  CSX1276_writeRegister(SPIDeviceHandle, REG_OP_MODE, LORA_TX_MODE); 
  
  // Timestamp for begining of transmission
  pLoraPacket->m_dwTimestamp = xTaskGetTickCount() * portTICK_RATE_MS;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] The SX1276 in now sending packet");
  #endif

  return LORATRANSCEIVERITF_RESULT_SUCCESS;
}

/*********************************************************************************************
  Private methods (implementation)

  Interrupt Service Routines (ISR).
*********************************************************************************************/

IRAM_ATTR void CSX1276_PacketRxTxIntHandler(CSX1276 *this)
{
  BaseType_t xHigherPriorityTaskWoken = 0;

  // Same IRQ used for both RX_DONE and TX_DONE IRQs (i.e. software configuration of DIO
  // on SX1276 according to OP mode)

  // Notify 'PacketReceived' or 'PacketSent' event to main automaton (RTOS task)
  if (this->m_dwCurrentState == SX1276_AUTOMATON_STATE_RECEIVING)
  {
    xTaskNotifyFromISR(this->m_hAutomatonTask, SX1276_AUTOMATON_NOTIFY_PACKET_RECEIVED, eSetBits,
                       &xHigherPriorityTaskWoken);
  }
  else if (this->m_dwCurrentState == SX1276_AUTOMATON_STATE_SENDING)
  {
    xTaskNotifyFromISR(this->m_hAutomatonTask, SX1276_AUTOMATON_NOTIFY_PACKET_SENT, eSetBits,
                       &xHigherPriorityTaskWoken);
  }

  // Allow quick activation of processing task
  if (xHigherPriorityTaskWoken)
  {
    portYIELD_FROM_ISR();
  }
}


/*********************************************************************************************
  Private methods (implementation)

  Helper functions (class).
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         bool CSX1276_IsDeviceConfigured(CSX1276 *this)
 * 
 * @brief      Checks if SX1276 configuration is complete.
 * 
 * @details    The function checks if all required settings are applied to SX1276 device for
 *             entering the operational state (i.e. ready to receive/send LoRa packets).\n
 *             This method in invoked when automaton is in 'INITIALIZED' state in order to 
 *             trigger the transition to 'STANDBY' state (typiquement when a configuration
 *             command is processed).
 *
 * @param      this
 *             The pointer to CSX1276 object.
 *  
 * @return     The function returns 'true' is all required settings are applied to SX1276 
 *             device or 'false' if some configuration is still requiered.
 *
 * @note       The required settings are defined in 'CLoraTransceiverItf_SetLoraModeParams',
 *             'CLoraTransceiverItf_SetPowerModeParams' and 'CLoraTransceiverItf_SetFreqChannelParams'.\n
 *             Before entering the 'INITIALIZED' state, default values are applied for the 
 *             settings defined in 'CLoraTransceiverItf_SetLoraMACParams' (i.e. no explicit 
 *             configuration required to send/receive packets on public LoRaWAN networks).
*********************************************************************************************/
bool CSX1276_IsDeviceConfigured(CSX1276 *this)
{
  // Check if SX1276 registers contains user settings for 'LoraMode', 'PowerMode' and 'FreqChannel'
  // (i.e. explicit configuration made after device reset)
  if ((this->m_usBandwidth == SX1276_BW_UNDEFINED) || (this->m_usCodingRate == SX1276_CR_UNDEFINED) ||
      (this->m_usSpreadingFactor == SX1276_SF_UNDEFINED) || (this->m_usFreqChannel == SX1276_FREQ_CH_UNDEFINED) ||
      (this->m_usPowerLevel == SX1276_POWER_LEVEL_UNDEFINED) || (this->m_usPowerMode == SX1276_POWER_MODE_UNDEFINED) ||
      (this->m_usOcpRate == SX1276_OCP_UNDEFINED))
  {
    #if (SX1276_DEBUG_LEVEL0)
      DEBUG_PRINT("[INFO] Device not configured. Remaining settings: ");
      if (this->m_usBandwidth == SX1276_BW_UNDEFINED)
        DEBUG_PRINT("Bandwidth, ");
       if (this->m_usCodingRate == SX1276_CR_UNDEFINED)
         DEBUG_PRINT("CR, ");
       if (this->m_usSpreadingFactor == SX1276_SF_UNDEFINED)
         DEBUG_PRINT("SF, ");
       if (this->m_usFreqChannel == SX1276_FREQ_CH_UNDEFINED)
         DEBUG_PRINT("Freq Channel, ");
       if (this->m_usPowerLevel == SX1276_POWER_LEVEL_UNDEFINED)
         DEBUG_PRINT("Power Level, ");
       if (this->m_usPowerMode == SX1276_POWER_MODE_UNDEFINED)
         DEBUG_PRINT("Power Mode, ");
       if (this->m_usOcpRate == SX1276_OCP_UNDEFINED)
         DEBUG_PRINT("OCP Rate");
       DEBUG_PRINT_CR;
    #endif

    return false;
  }

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Device configured (all settings defined)");
  #endif
  return true;
}


/*********************************************************************************************
  Private methods (implementation)

  Helper functions (static).
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         bool CSX1276_isSF(uint8_t SpreadingFactor)
 * 
 * @brief      Checks if the specified Spreading Factor is a valid value.
 * 
 * @details    The function checks the specified value against allowed constants for Spreading
 *             Factor (i.e. one of 'LORATRANSCEIVERITF_SF_xx' definitions in 'LoraTransceiverItf.h').
 *
 * @param      SpreadingFactor
 *             The Spreading Factor value to check.
 *
 * @return     The function returns 'true' if the specified spreading factor is defined or 'false'
 *             otherwise.
*********************************************************************************************/
bool CSX1276_isSF(uint8_t SpreadingFactor)
{
  bool bResult;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_isSF'");
  #endif

  // Checking available values for SpreadingFactor
  switch (SpreadingFactor)
  {
    case LORATRANSCEIVERITF_SF_6:
    case LORATRANSCEIVERITF_SF_7:
    case LORATRANSCEIVERITF_SF_8:
    case LORATRANSCEIVERITF_SF_9:
    case LORATRANSCEIVERITF_SF_10:
    case LORATRANSCEIVERITF_SF_11:
    case LORATRANSCEIVERITF_SF_12: 
      bResult = true;
      break;

    default:
      bResult = false;
  }
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Exiting 'CSX1276_isSF'");
    DEBUG_PRINT_CR;
  #endif
  return bResult;
}

/*****************************************************************************************//**
 * @fn         bool CSX1276_isBW(uint16_t Bandwidth)
 * 
 * @brief      Checks if the specified BW is a valid value.
 * 
 * @details    The function checks the specified value against allowed constants for Bandwidth
 *             (i.e. one of 'LORATRANSCEIVERITF_BANDWIDTH_xxx' definitions in 'LoraTransceiverItf.h').
 *
 * @param      Bandwidth
 *             The bandwidth value to check.
 *
 * @return     The function returns 'true' if the specified bandwidth is defined or 'false'
 *             otherwise.
*********************************************************************************************/
bool CSX1276_isBW(uint16_t Bandwidth)
{
  bool bResult;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_isBW'");
  #endif

  // Checking available values for Bandwidth
  switch (Bandwidth)
  {
    case LORATRANSCEIVERITF_BANDWIDTH_125:
    case LORATRANSCEIVERITF_BANDWIDTH_250:
    case LORATRANSCEIVERITF_BANDWIDTH_500:  
      bResult = true;
      break;

    default: 
      bResult = false;
  }
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Exiting 'CSX1276_isBW'");
    DEBUG_PRINT_CR;
  #endif
  return bResult;
}

/*****************************************************************************************//**
 * @fn         bool CSX1276_isCR(uint8_t CodingRate)
 * 
 * @brief      Checks if the specified CR is a valid value.
 * 
 * @details    The function checks the specified value against allowed constants for Coding Rate
 *             (i.e. one of 'LORATRANSCEIVERITF_CR_x' definitions in 'LoraTransceiverItf.h').
 *
 * @param      CodingRate
 *             The coding rate value to check.
 *
 * @return     The function returns 'true' if the specified coding rate is defined or 'false'
 *             otherwise.
*********************************************************************************************/
bool CSX1276_isCR(uint8_t CodingRate)
{
  bool bResult;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_isCR'");
  #endif

  // Checking available values for CodingRate
  switch (CodingRate)
  {
    case LORATRANSCEIVERITF_CR_5:
    case LORATRANSCEIVERITF_CR_6:
    case LORATRANSCEIVERITF_CR_7:
    case LORATRANSCEIVERITF_CR_8:  
      bResult = true;
      break;

    default:
      bResult = false;
  }
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Exiting 'CSX1276_isCR'");
    DEBUG_PRINT_CR;
  #endif
  return bResult;
}

/*****************************************************************************************//**
 * @fn         bool CSX1276_isChannel(uint8_t FreqChannel)
 * 
 * @brief      Checks if the specified channel is a valid value.
 * 
 * @details    The function checks the specified value against allowed constants for Frequency
 *             Channel (i.e. one of 'LORATRANSCEIVERITF_FREQUENCY_CHANNEL_xx' definitions in 
 *             'LoraTransceiverItf.h').
 *
 * @param      Channel
 *             The frequency channel value to check.
 *
 * @return     The function returns 'true' if the specified coding rate is defined or 'false'
 *             otherwise.
*********************************************************************************************/
bool CSX1276_isChannel(uint8_t FreqChannel)
{
  bool bResult;

  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_CR;
    DEBUG_PRINT_LN("[INFO] Starting 'CSX1276_isChannel'");
  #endif

  // Checking available values for Channel
  switch (FreqChannel)
  {
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_00:
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_01:
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_02:
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_03:
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_04:
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_05:
    case LORATRANSCEIVERITF_FREQUENCY_RX2:

    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_10:
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_11:
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_12:
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_13:
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_14:
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_15:
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_16:
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_17:
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_18:
      bResult = true;
      break;

    default:
      bResult= false;
  }
  #if (SX1276_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] Exiting 'CSX1276_isChannel'");
    DEBUG_PRINT_CR;
  #endif
  return bResult;
}

/*****************************************************************************************//**
 * @fn         uint32_t CSX1276_getFreqRegValue(uint8_t FreqChannel)
 * 
 * @brief      Retrieves the register value for a specified channel frequency.
 * 
 * @details    The function retrieves the value to set in SX1276 register for a specified
 *             Frequency Channel (i.e. one of 'LORATRANSCEIVERITF_FREQUENCY_CHANNEL_xx' definitions
 *             in 'LoraTransceiverItf.h').
 *
 * @param      FreqChannel
 *             The frequency channel to which retrieve the corresponding register value.
 *
 * @return     The function returns the register value if a the 'FreqChannel' is valid or
 *             'SX1276_REG_CH_UNDEFINED' in case of error.
*********************************************************************************************/
uint32_t CSX1276_getFreqRegValue(uint8_t FreqChannel)
{
  // Retrieve register value for 'FreqChannel'
  switch (FreqChannel)
  {
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_00:
      return SX1276_REG_CH_00_868;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_01:
      return SX1276_REG_CH_01_868;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_02:
      return SX1276_REG_CH_02_868;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_03:
      return SX1276_REG_CH_03_868;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_04:
      return SX1276_REG_CH_04_868;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_05:
      return SX1276_REG_CH_05_868;
    case LORATRANSCEIVERITF_FREQUENCY_RX2:
      return SX1276_REG_CH_RX2_868;

    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_10:
      return SX1276_REG_CH_10_868;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_11:
      return  SX1276_REG_CH_11_868;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_12:
      return SX1276_REG_CH_12_868;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_13:
      return SX1276_REG_CH_13_868;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_14:
      return SX1276_REG_CH_14_868;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_15:
      return SX1276_REG_CH_15_868;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_16:
      return SX1276_REG_CH_16_868;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_17:
      return SX1276_REG_CH_17_868;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_18:
      return SX1276_REG_CH_18_868;
  }
  return SX1276_REG_CH_UNDEFINED;
}


/*****************************************************************************************//**
 * @fn         char * CSX1276_getFreqTextValue(uint8_t FreqChannel)
 * 
 * @brief      Retrieves the specified channel frequency in text format.
 * 
 * @details    The function retrieves the value in text format for a specified Frequency Channel
 *             (i.e. one of 'LORATRANSCEIVERITF_FREQUENCY_CHANNEL_xx' definitions in 
 *             'LoraTransceiverItf.h').
 *
 * @param      FreqChannel
 *             The frequency channel to which retrieve value in text format.
 *
 * @return     The function returns the frequency in text format (float in MHz with Hz precision)
 *             or an empty string in case of error.
*********************************************************************************************/
char * CSX1276_getFreqTextValue(uint8_t FreqChannel)
{
  // Retrieve register value for 'FreqChannel'
  switch (FreqChannel)
  {
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_00:
      return SX1276_FREQ_TEXT_CH_00;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_01:
      return SX1276_FREQ_TEXT_CH_01;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_02:
      return SX1276_FREQ_TEXT_CH_02;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_03:
      return SX1276_FREQ_TEXT_CH_03;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_04:
      return SX1276_FREQ_TEXT_CH_04;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_05:
      return SX1276_FREQ_TEXT_CH_05;

    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_10:
      return SX1276_FREQ_TEXT_CH_10;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_11:
      return SX1276_FREQ_TEXT_CH_11;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_12:
      return SX1276_FREQ_TEXT_CH_12;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_13:
      return SX1276_FREQ_TEXT_CH_13;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_14:
      return SX1276_FREQ_TEXT_CH_14;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_15:
      return SX1276_FREQ_TEXT_CH_15;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_16:
      return SX1276_FREQ_TEXT_CH_16;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_17:
      return SX1276_FREQ_TEXT_CH_17;
    case LORATRANSCEIVERITF_FREQUENCY_CHANNEL_18:
      return SX1276_FREQ_TEXT_CH_18;
  }
  return "";
}


char * CSX1276_getCRTextValue(uint8_t CodingRate)
{
  switch (CodingRate)
  {
    case LORATRANSCEIVERITF_CR_5:
      return SX1276_CR_TEXT_CR5;
    case LORATRANSCEIVERITF_CR_6:
      return SX1276_CR_TEXT_CR6;
    case LORATRANSCEIVERITF_CR_7:
      return SX1276_CR_TEXT_CR7;
    case LORATRANSCEIVERITF_CR_8:
      return SX1276_CR_TEXT_CR8;
  }
  return "";
}








/*********************************************************************************************
  Private methods (implementation)
*********************************************************************************************/


