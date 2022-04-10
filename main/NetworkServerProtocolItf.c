/*****************************************************************************************//**
 * @file     NetworkProtocolItf.c
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     16/04/2018
 *
 * @brief    A NetworkServerProtocol implements the protocol used for the transfer of LoRa 
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

#define NETWORKSERVERPROTOCOLITF_IMPL

#include "NetworkServerProtocolItf.h"
//#include "NetworkServerProtocolItfImpl.h"

/*
typedef struct _CNetworkServerProtocolItfImpl
{
  // Public
  AddRef m_pAddRef;
  ReleaseItf m_pReleaseItf;
  BuildUplinkMessage m_pBuildUplinkMessage;
  ProcessServerMessage m_pProcessServerMessage;
} CNetworkServerProtocolItfImplOb;
*/

//typedef struct _CNetworkServerProtocolItfImpl * CNetworkServerProtocolItfImpl;

/*
typedef struct _INetworkServerProtocol
{
  // Private attributes
  void *m_pOwnerObject;                            // Instance of the object exposing the interface
                                                   // This pointer is provided as argument on interface methods
  CNetworkServerProtocolItfImpl m_pOwnerItfImpl;   // Pointers to implementation functions on owner object
} INetworkServerProtocolOb;

*/

//typedef struct _INetworkServerProtocol * INetworkServerProtocol;


/*********************************************************************************************
  Interface object construction (i.e. 'INetworkServerProtocol' class instance)
 
  Protected methods used by object implementing the interface
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         INetworkServerProtocol INetworkServerProtocol_New(void *pOwnerObject, 
 *                                                   CNetworkServerProtocolItfImpl pOwnerItfImpl)
 *  
 * @brief      Object construction. 
 * 
 * @details    Construction allowed only for objects implementing the 'INetworkServerProtocol' 
 *             interface ('CSemtechProtocolEngine', ...) (protected constructor)
 * 
 * @param      pOwnerObject
 *             Object implementing the interface (i.e. which provides interface to client objects).\n
 * 
 * @param      pOwnerItfImpl
 *             Array of pointers to INetworkServerProtocol implementation functions in parent object.
 *  
 * @return     Pointer to the new 'INetworkServerProtocol' instance.
*********************************************************************************************/
INetworkServerProtocol INetworkServerProtocol_New(void *pOwnerObject, CNetworkServerProtocolItfImpl pOwnerItfImpl)
{
  INetworkServerProtocol this;

  if ((this = (void *) pvPortMalloc(sizeof(CNetworkServerProtocolItfImplOb))) != NULL)
  {
    // Set member variable values
    this->m_pOwnerObject = pOwnerObject;
    this->m_pOwnerItfImpl = pOwnerItfImpl;
  }
  return this;
}

/*****************************************************************************************//**
 * @fn         void INetworkServerProtocol_Delete(INetworkServerProtocol this)
 * 
 * @brief      Object destruction.
 * 
 * @details    Destroys the interface object.\n
 *             This method is allowed only for objects implementing the 'INetworkServerProtocol' 
 *             interface ('CSemtechProtocolEngine', ...) (protected destructor).\n
 *             It is called by owner object when interface reference count reaches 0. 
 * 
 * @param      this
 *             The object pointer.
 *  
 * @return     None.
*********************************************************************************************/
void INetworkServerProtocol_Delete(INetworkServerProtocol this)
{
  vPortFree(this);
}

/*********************************************************************************************
  Public methods of 'INetworkServerProtocol' interface
 
  These methods are available to any object which has a reference to 'INetworkServerProtocol'
  interface
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         uint32_t INetworkServerProtocol_AddRef(INetworkServerProtocol this)
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
uint32_t INetworkServerProtocol_AddRef(INetworkServerProtocol this)
{
  this->m_pOwnerItfImpl->m_pAddRef(this->m_pOwnerObject);
  return true;
}

/*****************************************************************************************//**
 * @fn         uint32_t INetworkServerProtocol_ReleaseItf(INetworkServerProtocol this)
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
uint32_t INetworkServerProtocol_ReleaseItf(INetworkServerProtocol this)
{
  this->m_pOwnerItfImpl->m_pReleaseItf(this->m_pOwnerObject);
  return true;
}

/*****************************************************************************************//**
 * @fn         bool INetworkServerProtocol_BuildUplinkMessge(INetworkServerProtocol this, 
                                   CNetworkServerProtocolItf_BuildUplinkMessageParams pParams)
 * 
 * @brief      Encodes an uplink Lora Packet and build uplink message for Network Server.
 * 
 * @details    This function invokes the implementation of 'BuildUplinkMessage' method on owner
 *             object.\n
 * 
 * @param      this
 *             The object pointer.
 *  
 * @param      pParams
 *             The method parameters. See 'NetworkServerProtocolItf.h' for details.
 *
 * @return     The function returns 'true' if the LoRa Packet is encoded or 'false' if an
 *             error has occured.
*********************************************************************************************/
bool INetworkServerProtocol_BuildUplinkMessage(INetworkServerProtocol this, CNetworkServerProtocolItf_BuildUplinkMessageParams pParams)
{
  return this->m_pOwnerItfImpl->m_pBuildUplinkMessage(this->m_pOwnerObject, pParams);
}

/*****************************************************************************************//**
 * @fn         DWORD INetworkServerProtocol_ProcessServerMessage(INetworkServerProtocol this, 
                                   CNetworkServerProtocolItf_ProcessServerMessageParams pParams)
 * 
 * @brief      Process message received from Network Server (ACK or downlink LoRa packet).
 * 
 * @details    This function invokes the implementation of 'ProcessServerMessage' method on owner
 *             object.\n
 * 
 * @param      this
 *             The object pointer.
 *  
 * @param      pParams
 *             The method parameters. See 'NetworkServerProtocolItf.h' for details.
 *
 * @return     The function returns a code indicating the message state in 'ProtocolEngine'.
 *             The caller must use this state to issue appropriate actions to the associated
 *             data (typically notify the 'TransceiverManager' and/or release data storage).
*********************************************************************************************/
DWORD INetworkServerProtocol_ProcessServerMessage(INetworkServerProtocol this, CNetworkServerProtocolItf_ProcessServerMessageParams pParams)
{
  return this->m_pOwnerItfImpl->m_pProcessServerMessage(this->m_pOwnerObject, pParams);
}

/*****************************************************************************************//**
 * @fn         DWORD INetworkServerProtocol_ProcessSessionEvent(INetworkServerProtocol this, 
                                   CNetworkServerProtocolItf_ProcessSessionEventParams pParams)
 * 
 * @brief      Notifies the 'NetworkServerProtocol' for event occured while processing
 *             a session (uplink message session or downlink message session).
 * 
 * @details    This function invokes the implementation of 'ProcessSessionEvent' method on owner
 *             object.\n
 * 
 * @param      this
 *             The object pointer.
 *  
 * @param      pParams
 *             The method parameters. See 'NetworkServerProtocolItf.h' for details.
 *
 * @return     The function returns a code indicating the session state in 'ProtocolEngine'.
 *             The caller must use this state to issue appropriate actions to the associated
 *             data (typically notify the 'TransceiverManager' and/or release data storage).
*********************************************************************************************/
DWORD INetworkServerProtocol_ProcessSessionEvent(INetworkServerProtocol this, CNetworkServerProtocolItf_ProcessSessionEventParams pParams)
{
  return this->m_pOwnerItfImpl->m_pProcessSessionEvent(this->m_pOwnerObject, pParams);
}



