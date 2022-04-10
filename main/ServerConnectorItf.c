/*****************************************************************************************//**
 * @file     ServerConnectorItf.c
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     23/04/2018
 *
 * @brief    A ServerConnector allows access to Lora Network Server through generic methods
 *           independent of underlying transport layer (Wifi, GPRS ...).
 *
 * @details  This class is the interface exposed by objects implementing the network connection
 *           and data exchange on the physical transport layer.\n
 *           Typically, objects implementing 'IServerConnector' interface are dedicated to
 *           a given network device (by example, see 'ESP32WifiConnector', the network access
 *           implemented using Wifi transport on ESP32 Wifi hardware).
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

#define SERVERCONNECTORITF_IMPL

#include "ServerConnectorItf.h"


/*********************************************************************************************
  Interface object construction (i.e. 'IServerConnector' class instance)
 
  Protected methods used by object implementing the interface
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         IServerConnector IServerConnector_New(void *pOwnerObject, 
 *                                                   CServerConnectorItfImpl pOwnerItfImpl)
 *  
 * @brief      Object construction. 
 * 
 * @details    Construction allowed only for objects implementing the 'IServerConnector' 
 *             interface ('CESP32WifiConnector', ...) (protected constructor)
 * 
 * @param      pOwnerObject
 *             Object implementing the interface (i.e. which provides interface to client 
 *             objects).
 * 
 * @param      pOwnerItfImpl
 *             Array of pointers to IServerConnector implementation functions in parent 
 *             object.
 *  
 * @return     Pointer to the new 'IServerConnector' instance.
*********************************************************************************************/
IServerConnector IServerConnector_New(void *pOwnerObject, CServerConnectorItfImpl pOwnerItfImpl)
{
  IServerConnector this;

  if ((this = (void *) pvPortMalloc(sizeof(IServerConnectorOb))) != NULL)
  {
    // Set member variable values
    this->m_pOwnerObject = pOwnerObject;
    this->m_pOwnerItfImpl = pOwnerItfImpl;
  }
  return this;
}

/*****************************************************************************************//**
 * @fn         void IServerConnector_Delete(IServerConnector this)
 * 
 * @brief      Object destruction.
 * 
 * @details    Destroys the interface object.\n
 *             This method is allowed only for objects implementing the 'IServerConnector' 
 *             interface ('CESP32WifiConnector', ...) (protected destructor).\n
 *             It is called by owner object when interface reference count reaches 0. 
 * 
 * @param      this
 *             The object pointer.
 *  
 * @return     None.
*********************************************************************************************/
void IServerConnector_Delete(IServerConnector this)
{
  vPortFree(this);
}

/*********************************************************************************************
  Public methods of 'IServerConnector' interface
 
  These methods are available to any object which has a reference to 'IServerConnector'
  interface
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         uint32_t IServerConnector_AddRef(IServerConnector this)
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
uint32_t IServerConnector_AddRef(IServerConnector this)
{
  this->m_pOwnerItfImpl->m_pAddRef(this->m_pOwnerObject);
  return true;
}

/*****************************************************************************************//**
 * @fn         uint32_t IServerConnector_ReleaseItf(IServerConnector this)
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
uint32_t IServerConnector_ReleaseItf(IServerConnector this)
{
  this->m_pOwnerItfImpl->m_pReleaseItf(this->m_pOwnerObject);
  return true;
}

/*****************************************************************************************//**
 * @fn         bool IServerConnector_Initialize(IServerConnector this, 
 *                                              CServerConnectorItf_InitializeParams pParams)
 * 
 * @brief      Initializes the Lora Transceiver.
 * 
 * @details    This function invokes the implementation of 'Initialize' method on owner object.\n
 *             The 'Initialize' method is invoked by client object during startup to reset the
 *             network devices and to configure them to their default state (typically standby
 *             without activity).
 * 
 * @param      this
 *             The object pointer.
 *  
 * @param      pParams
 *             The method parameters. See 'ServerConnectorItf.h' for details.
 *
 * @return     The function returns 'true' if the 'ServerConnector' has initialized the
 *             associated network device or 'false' if an error has occured.
 * 
 * @note       Typically the 'ServerConnector' is managed by a single owner objet. Only this
 *             owner calls administrative methods like 'Initialize'.
*********************************************************************************************/
bool IServerConnector_Initialize(IServerConnector this, CServerConnectorItf_InitializeParams pParams)
{
  return this->m_pOwnerItfImpl->m_pInitialize(this->m_pOwnerObject, pParams);
}

bool IServerConnector_Start(IServerConnector this, CServerConnectorItf_StartParams pParams)
{
  return this->m_pOwnerItfImpl->m_pStart(this->m_pOwnerObject, pParams);
}

bool IServerConnector_Stop(IServerConnector this, CServerConnectorItf_StopParams pParams)
{
  return this->m_pOwnerItfImpl->m_pStop(this->m_pOwnerObject, pParams);
}

bool IServerConnector_Send(IServerConnector this, CServerConnectorItf_SendParams pParams)
{
  return this->m_pOwnerItfImpl->m_pSend(this->m_pOwnerObject, pParams);
}

bool IServerConnector_SendReceive(IServerConnector this, CServerConnectorItf_SendReceiveParams pParams)
{
  return this->m_pOwnerItfImpl->m_pSendReceive(this->m_pOwnerObject, pParams);
}

bool IServerConnector_DownlinkReceived(IServerConnector this, CServerConnectorItf_DownlinkReceivedParams pParams)
{
  return this->m_pOwnerItfImpl->m_pDownlinkReceived(this->m_pOwnerObject, pParams);
}





