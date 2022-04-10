/*****************************************************************************************//**
 * @file     LoraRealtimeSenderItf.c
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     24/12/2018
 *
 * @brief    A LoraRealtimeSender implements the protocol used for the transfer of LoRa 
 *           packets between the gateway and a given LoRa Network Server.
 *
 * @details  This class is the interface exposed by objects implementing the protocol for a
 *           given LoRa Network Server.\n
 *           The main functions are for encoding and decoding LoRaWAN frames ('CLoraPacket')
 *           to/from proprietary messages used by the Network Server.
 *
 * @note     This class IS THREAD SAFE.
*********************************************************************************************/

/*********************************************************************************************
  Espressif framework includes
*********************************************************************************************/

#include <Common.h>

/*********************************************************************************************
  Includes for object implementation
*********************************************************************************************/

#define LORAREALTIMESENDERITF_IMPL

#include "LoraTransceiverItf.h"
#include "TransceiverManagerItf.h"
#include "LoraRealtimeSenderItf.h"


/*********************************************************************************************
  Interface object construction (i.e. 'ILoraRealtimeSender' class instance)
 
  Protected methods used by object implementing the interface
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         ILoraRealtimeSender ILoraRealtimeSender_New(void *pOwnerObject, 
 *                                                   CLoraRealtimeSenderItfImpl pOwnerItfImpl)
 *  
 * @brief      Object construction. 
 * 
 * @details    Construction allowed only for objects implementing the 'ILoraRealtimeSender' 
 *             interface ('CLoraRealtimeSender', ...) (protected constructor)
 * 
 * @param      pOwnerObject
 *             Object implementing the interface (i.e. which provides interface to client objects).\n
 * 
 * @param      pOwnerItfImpl
 *             Array of pointers to ILoraRealtimeSender implementation functions in parent object.
 *  
 * @return     Pointer to the new 'ILoraRealtimeSender' instance.
*********************************************************************************************/
ILoraRealtimeSender ILoraRealtimeSender_New(void *pOwnerObject, CLoraRealtimeSenderItfImpl pOwnerItfImpl)
{
  ILoraRealtimeSender this;

  if ((this = (void *) pvPortMalloc(sizeof(CLoraRealtimeSenderItfImplOb))) != NULL)
  {
    // Set member variable values
    this->m_pOwnerObject = pOwnerObject;
    this->m_pOwnerItfImpl = pOwnerItfImpl;
  }
  return this;
}

/*****************************************************************************************//**
 * @fn         void ILoraRealtimeSender_Delete(ILoraRealtimeSender this)
 * 
 * @brief      Object destruction.
 * 
 * @details    Destroys the interface object.\n
 *             This method is allowed only for objects implementing the 'ILoraRealtimeSender' 
 *             interface ('CLoraRealtimeSender', ...) (protected destructor).\n
 *             It is called by owner object when interface reference count reaches 0. 
 * 
 * @param      this
 *             The object pointer.
 *  
 * @return     None.
*********************************************************************************************/
void ILoraRealtimeSender_Delete(ILoraRealtimeSender this)
{
  vPortFree(this);
}

/*********************************************************************************************
  Public methods of 'ILoraRealtimeSender' interface
 
  These methods are available to any object which has a reference to 'ILoraRealtimeSender'
  interface
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         uint32_t ILoraRealtimeSender_AddRef(ILoraRealtimeSender this)
 * 
 * @brief      Increments the interface's reference count.
 * 
 * @details    This function invokes the implementation of 'AddRef' method on owner object.\n
 *             The 'AddRef' method is invoked by client objects to make sure that interface will 
 *             be valid until they need it (i.e. client object invokes 'ReleaseItf' when it do
 *             not need the interface anymore).
 * 
 * @param      this
 *             The object pointer.
 *  
 * @return     The value of reference count once incremented.
*********************************************************************************************/
uint32_t ILoraRealtimeSender_AddRef(ILoraRealtimeSender this)
{
  this->m_pOwnerItfImpl->m_pAddRef(this->m_pOwnerObject);
  return true;
}

/*****************************************************************************************//**
 * @fn         uint32_t ILoraRealtimeSender_ReleaseItf(ILoraRealtimeSender this)
 * 
 * @brief      Increments the interface's reference count.
 * 
 * @details    This function invokes the implementation of 'ReleasItf' method on owner object.\n
 *             The 'ReleaseItf' method is invoked by client objects when it do not need the 
 *             interface anymore.\n
 *             Typically, the owner object destroys the interface and itself when no more object
 *             is requiring it.
 * 
 * @param      this
 *             The object pointer.
 *  
 * @return     The value of reference count once decremented.
*********************************************************************************************/
uint32_t ILoraRealtimeSender_ReleaseItf(ILoraRealtimeSender this)
{
  this->m_pOwnerItfImpl->m_pReleaseItf(this->m_pOwnerObject);
  return true;
}

/*****************************************************************************************//**
 * @fn         bool ILoraRealtimeSender_Initialize(ILoraRealtimeSender this, 
 *                                  CLoraRealtimeSenderItf_InitializeParams pParams)
 * 
 * @brief      Attachs the interface of parent object for notifications during send process.
 * 
 * @details    This function invokes the implementation of 'Initialize' method on owner
 *             object.\n
 * 
 * @param      this
 *             The object pointer.
 *  
 * @param      pParams
 *             The method parameters. See 'LoraRealtimeSenderItf.h' for details.
 *
 * @return     The function returns 'true' if the 'LoraRealtimeSender' is initialize or 'false'
 *             if an error has occured (method invoked in an invalid state).
*********************************************************************************************/
bool ILoraRealtimeSender_Initialize(ILoraRealtimeSender this, CLoraRealtimeSenderItf_InitializeParams pParams)
{
  return this->m_pOwnerItfImpl->m_pInitialize(this->m_pOwnerObject, pParams);
}

/*****************************************************************************************//**
 * @fn         bool ILoraRealtimeSender_Start(ILoraRealtimeSender this, 
 *                                  CLoraRealtimeSenderItf_StartParams pParams)
 * 
 * @brief      Starts to send to nodes the LoRa packets waiting in the object's just in time queue.
 * 
 * @details    This function invokes the implementation of 'Start' method on owner object.\n
 * 
 * @param      this
 *             The object pointer.
 *  
 * @param      pParams
 *             The method parameters. See 'LoraRealtimeSenderItf.h' for details.
 *
 * @return     The function returns 'true' if the 'LoraRealtimeSender' is started or 'false'
 *             if an error has occured (method invoked in an invalid state).
*********************************************************************************************/
bool ILoraRealtimeSender_Start(ILoraRealtimeSender this, CLoraRealtimeSenderItf_StartParams pParams)
{
  return this->m_pOwnerItfImpl->m_pStart(this->m_pOwnerObject, pParams);
}

/*****************************************************************************************//**
 * @fn         bool ILoraRealtimeSender_Stop(ILoraRealtimeSender this, 
 *                                  CLoraRealtimeSenderItf_StopParams pParams)
 * 
 * @brief      Stops to send to nodes the LoRa packets waiting in the object's just in time queue.
 * 
 * @details    This function invokes the implementation of 'Stop' method on owner object.\n
 * 
 * @param      this
 *             The object pointer.
 *  
 * @param      pParams
 *             The method parameters. See 'LoraRealtimeSenderItf.h' for details.
 *
 * @return     The function returns 'true' if the 'LoraRealtimeSender' is stopped or 'false'
 *             if an error has occured (method invoked in an invalid state).
*********************************************************************************************/
bool ILoraRealtimeSender_Stop(ILoraRealtimeSender this, CLoraRealtimeSenderItf_StopParams pParams)
{
  return this->m_pOwnerItfImpl->m_pStop(this->m_pOwnerObject, pParams);
}


/*****************************************************************************************//**
 * @fn         bool ILoraRealtimeSender_RegisterNodeRxWindows(ILoraRealtimeSender this, 
 *                               CLoraRealtimeSenderItf_RegisterNodeRxWindowsParams pParams)
 * 
 * @brief      Stops to send to nodes the LoRa packets waiting in the object's just in time queue.
 * 
 * @details    This function invokes the implementation of 'RegisterNodeRxWindows' method on
 *             owner object.\n
 * 
 * @param      this
 *             The object pointer.
 *  
 * @param      pParams
 *             The method parameters. See 'LoraRealtimeSenderItf.h' for details.
 *
 * @return     The 'true' value is returned if node's RX windows are registered or false if
 *             an error has occurred (typically current time past the end of last RX2 window).
*********************************************************************************************/
bool ILoraRealtimeSender_RegisterNodeRxWindows(ILoraRealtimeSender this, CLoraRealtimeSenderItf_RegisterNodeRxWindowsParams pParams)
{
  return this->m_pOwnerItfImpl->m_pRegisterNodeRxWindows(this->m_pOwnerObject, pParams);
}

/*****************************************************************************************//**
 * @fn         bool ILoraRealtimeSender_ScheduleSendNodePacket(ILoraRealtimeSender this, 
 *                                CLoraRealtimeSenderItf_ScheduleSendNodePacketParams pParams)
 * 
 * @brief      Stops to send to nodes the LoRa packets waiting in the object's just in time queue.
 * 
 * @details    This function invokes the implementation of 'ScheduleSendNodePacket' method on 
 *             owner object.\n
 * 
 * @param      this
 *             The object pointer.
 *  
 * @param      pParams
 *             The method parameters. See 'LoraRealtimeSenderItf.h' for details.
 *
 * @return     The function returns a code to indicate if the LoRa packet is scheduled for
 *             send (= 'LORAREALTIMESENDER_SCHEDULESEND_xxx'. The semantic of returned codes
 *             is specified from codes used by Semtech protocol for TX_ACK message.
*********************************************************************************************/
DWORD ILoraRealtimeSender_ScheduleSendNodePacket(ILoraRealtimeSender this, CLoraRealtimeSenderItf_ScheduleSendNodePacketParams pParams)
{
  return this->m_pOwnerItfImpl->m_pScheduleSendNodePacket(this->m_pOwnerObject, pParams);
}



