/*****************************************************************************************//**
 * @file     TransceiverManagerItf.c
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     04/03/2018
 *
 * @brief    A TransceiverManager manages a collection of transceivers (i.e. radio devices
 *           for transmissions between the gateway and endpoints).
 *
 * @details  This class is the interface exposed by objects managing a collection of radio
 *           devices used for packet transmissions between the gateway and endpoints (nodes).\n
 *           Typically, objects implementing 'ITransceiverManager' interface are dedicated to
 *           a given radio protocol (by example, see 'LoraNodeMamager', the 'TransceiverManager'
 *           used for LoRa radio devices).
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

#define TRANSCEIVERMANAGERITF_IMPL

#include "TransceiverManagerItf.h"


/*********************************************************************************************
  Interface object construction (i.e. 'ITransceiverManager' class instance)
 
  Protected methods used by object implementing the interface
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         ITransceiverManager ITransceiverManager_New(void *pOwnerObject, 
 *                                    CTransceiverManagerItfImpl pOwnerItfImpl)
 *  
 * @brief      Object construction. 
 * 
 * @details    Construction allowed only for objects implementing the 'ITransceiverManager' 
 *             interface ('CLoraNodeManager', ...) (protected constructor)
 * 
 * @param      pOwnerObject
 *             Object implementing the interface (i.e. which provides interface to client 
 *             objects).
 * 
 * @param      pOwnerItfImpl
 *             Array of pointers to ITransceiverManager implementation functions in parent 
 *             object.
 *  
 * @return     Pointer to the new 'ITransceiverManager' instance.
*********************************************************************************************/
ITransceiverManager ITransceiverManager_New(void *pOwnerObject, CTransceiverManagerItfImpl pOwnerItfImpl)
{
  ITransceiverManager this;

  if ((this = (void *) pvPortMalloc(sizeof(ITransceiverManagerOb))) != NULL)
  {
    // Set member variable values
    this->m_pOwnerObject = pOwnerObject;
    this->m_pOwnerItfImpl = pOwnerItfImpl;
  }
  return this;
}

/*****************************************************************************************//**
 * @fn         void ITransceiverManager_Delete(ITransceiverManager this)
 * 
 * @brief      Object destruction.
 * 
 * @details    Destroys the interface object.\n
 *             This method is allowed only for objects implementing the 'ITransceiverManager' 
 *             interface ('CLoraNodeManager', ...) (protected destructor).\n
 *             It is called by owner object when interface reference count reaches 0. 
 * 
 * @param      this
 *             The object pointer.
 *  
 * @return     None.
*********************************************************************************************/
void ITransceiverManager_Delete(ITransceiverManager this)
{
  vPortFree(this);
}

/*********************************************************************************************
  Public methods of 'ITransceiverManager' interface
 
  These methods are available to any object which has a reference to 'ITransceiverManager'
  interface
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         uint32_t ITransceiverManager_AddRef(ITransceiverManager this)
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
uint32_t ITransceiverManager_AddRef(ITransceiverManager this)
{
  this->m_pOwnerItfImpl->m_pAddRef(this->m_pOwnerObject);
  return true;
}

/*****************************************************************************************//**
 * @fn         uint32_t ITransceiverManager_ReleaseItf(ITransceiverManager this)
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
uint32_t ITransceiverManager_ReleaseItf(ITransceiverManager this)
{
  this->m_pOwnerItfImpl->m_pReleaseItf(this->m_pOwnerObject);
  return true;
}

/*****************************************************************************************//**
 * @fn         bool ITransceiverManager_Initialize(ITransceiverManager this, 
 *                                        CTransceiverManagerItf_InitializeParams pParams)
 * 
 * @brief      Initializes the Lora Transceiver.
 * 
 * @details    This function invokes the implementation of 'Initialize' method on owner object.\n
 *             The 'Initialize' method is invoked by client object during startup to reset the
 *             transceiver radio devices and to configure them to their default state (typically
 *             standby without radio activity).
 * 
 * @param      this
 *             The object pointer.
 *  
 * @param      pParams
 *             The method parameters. See 'TransceiverManagerItf.h' for details.
 *
 * @return     The function returns 'true' if the 'TransceiverManager' has initialized all
 *             associated transceivers or 'false' if an error has occured.
 * 
 * @note       Typically the 'TransceiverManager' is managed by a single owner objet. Only this
 *             owner calls administrative methods like 'Initialize'.
*********************************************************************************************/
bool ITransceiverManager_Initialize(ITransceiverManager this, CTransceiverManagerItf_InitializeParams pParams)
{
  return this->m_pOwnerItfImpl->m_pInitialize(this->m_pOwnerObject, pParams);
}


bool ITransceiverManager_Attach(ITransceiverManager this, CTransceiverManagerItf_AttachParams pParams)
{
  return this->m_pOwnerItfImpl->m_pAttach(this->m_pOwnerObject, pParams);
}

bool ITransceiverManager_Start(ITransceiverManager this, CTransceiverManagerItf_StartParams pParams)
{
  return this->m_pOwnerItfImpl->m_pStart(this->m_pOwnerObject, pParams);
}

bool ITransceiverManager_Stop(ITransceiverManager this, CTransceiverManagerItf_StopParams pParams)
{
  return this->m_pOwnerItfImpl->m_pStop(this->m_pOwnerObject, pParams);
}

bool ITransceiverManager_SessionEvent(ITransceiverManager this, CTransceiverManagerItf_SessionEvent pEvent)
{
  return this->m_pOwnerItfImpl->m_pSessionEvent(this->m_pOwnerObject, pEvent);
}

