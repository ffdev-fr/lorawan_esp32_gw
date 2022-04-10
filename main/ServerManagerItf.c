/*****************************************************************************************//**
 * @file     ServerManagerItf.c
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     15/04/2018
 *
 * @brief    A ServerManager manages LoRaWan packet exchanges between gateway and the Network
 *           Server.
 *
 * @details  This class is the interface exposed by objects in charge to transfer LoRaWAN 
 *           packets between the gateway and a given Network Server (using a collection of
 *           'ServerConnector').\n
 *           In other words, objects implementing 'IServerManager' interface are dedicated to
 *           a given Network Server and protocol.\n
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

#define SERVERMANAGERITF_IMPL

#include "ServerManagerItf.h"


/*********************************************************************************************
  Interface object construction (i.e. 'IServerManager' class instance)
 
  Protected methods used by object implementing the interface
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         IServerManager IServerManager_New(void *pOwnerObject, 
 *                                               CServerManagerItfImpl pOwnerItfImpl)
 *  
 * @brief      Object construction. 
 * 
 * @details    Construction allowed only for objects implementing the 'IServerManager' 
 *             interface ('CLoraServerManager', ...) (protected constructor)
 * 
 * @param      pOwnerObject
 *             Object implementing the interface (i.e. which provides interface to client 
 *             objects).
 * 
 * @param      pOwnerItfImpl
 *             Array of pointers to IServerManager implementation functions in parent 
 *             object.
 *  
 * @return     Pointer to the new 'IServerManager' instance.
*********************************************************************************************/
IServerManager IServerManager_New(void *pOwnerObject, CServerManagerItfImpl pOwnerItfImpl)
{
  IServerManager this;

  if ((this = (void *) pvPortMalloc(sizeof(IServerManagerOb))) != NULL)
  {
    // Set member variable values
    this->m_pOwnerObject = pOwnerObject;
    this->m_pOwnerItfImpl = pOwnerItfImpl;
  }
  return this;
}

/*****************************************************************************************//**
 * @fn         void IServerManager_Delete(IServerManager this)
 * 
 * @brief      Object destruction.
 * 
 * @details    Destroys the interface object.\n
 *             This method is allowed only for objects implementing the 'IServerManager' 
 *             interface ('CLoraServerManager', ...) (protected destructor).\n
 *             It is called by owner object when interface reference count reaches 0. 
 * 
 * @param      this
 *             The object pointer.
 *  
 * @return     None.
*********************************************************************************************/
void IServerManager_Delete(IServerManager this)
{
  vPortFree(this);
}

/*********************************************************************************************
  Public methods of 'IServerManager' interface
 
  These methods are available to any object which has a reference to 'IServerManager'
  interface
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         uint32_t IServerManager_AddRef(IServerManager this)
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
uint32_t IServerManager_AddRef(IServerManager this)
{
  this->m_pOwnerItfImpl->m_pAddRef(this->m_pOwnerObject);
  return true;
}

/*****************************************************************************************//**
 * @fn         uint32_t IServerManager_ReleaseItf(IServerManager this)
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
uint32_t IServerManager_ReleaseItf(IServerManager this)
{
  this->m_pOwnerItfImpl->m_pReleaseItf(this->m_pOwnerObject);
  return true;
}

/*****************************************************************************************//**
 * @fn         bool IServerManager_Initialize(IServerManager this, 
 *                                            CServerManagerItf_InitializeParams pParams)
 * 
 * @brief      Initializes the Lora Transceiver.
 * 
 * @details    This function invokes the implementation of 'Initialize' method on owner object.\n
 *             The 'Initialize' method is invoked by client object during startup to reset the
 *             network connector devices and to configure them to their default state (typically
 *             standby without network activity).
 * 
 * @param      this
 *             The object pointer.
 *  
 * @param      pParams
 *             The method parameters. See 'ServerManagerItf.h' for details.
 *
 * @return     The function returns 'true' if the 'ServerManager' has initialized all
 *             associated connectors or 'false' if an error has occured.
 * 
 * @note       Typically the 'ServerManager' is managed by a single owner objet. Only this
 *             owner calls administrative methods like 'Initialize'.
*********************************************************************************************/
bool IServerManager_Initialize(IServerManager this, CServerManagerItf_InitializeParams pParams)
{
  return this->m_pOwnerItfImpl->m_pInitialize(this->m_pOwnerObject, pParams);
}


bool IServerManager_Attach(IServerManager this, CServerManagerItf_AttachParams pParams)
{
  return this->m_pOwnerItfImpl->m_pAttach(this->m_pOwnerObject, pParams);
}

bool IServerManager_Start(IServerManager this, CServerManagerItf_StartParams pParams)
{
  return this->m_pOwnerItfImpl->m_pStart(this->m_pOwnerObject, pParams);
}

bool IServerManager_Stop(IServerManager this, CServerManagerItf_StopParams pParams)
{
  return this->m_pOwnerItfImpl->m_pStop(this->m_pOwnerObject, pParams);
}

bool IServerManager_ServerMessageEvent(IServerManager this, CServerManagerItf_ServerMessageEvent pEvent)
{
  return this->m_pOwnerItfImpl->m_pServerMessageEvent(this->m_pOwnerObject, pEvent);
}


