/*****************************************************************************************//**
 * @file     NetworkServerProtocolItfImpl.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     15/04/2018
 *
 * @brief    Structures and definitions for 'INetworkServerProtocol' interface implementation.
 *
 * @details  Used by objects implementing the interface and for the implementation of the
 *           interface object.
*********************************************************************************************/

#ifndef NETWORKSERVERPROTOCOLITFIMPL_H
#define NETWORKSERVERPROTOCOLITFIMPL_H


/********************************************************************************************* 
  Forward declarations
*********************************************************************************************/
           

/********************************************************************************************* 
  Prototypes of interface methods                                                                         .
  
  These methods are implemented in the objects exposing the interface.
  Pointers to these methods are registered in the 'INetworkServerProtocolItfImpl' object by the
  object implementing the methods.
*********************************************************************************************/

// Public
typedef uint32_t (*AddRef)(void *pOwnerObject);
typedef uint32_t (*ReleaseItf)(void *pOwnerObject);

typedef bool (*BuildUplinkMessage)(void *pOwnerObject, CNetworkServerProtocolItf_BuildUplinkMessageParams pParams);
typedef DWORD (*ProcessServerMessage)(void *pOwnerObject, CNetworkServerProtocolItf_ProcessServerMessageParams pParams);
typedef DWORD (*ProcessSessionEvent)(void *pOwnerObject, CNetworkServerProtocolItf_ProcessSessionEventParams pParams);
                                                  

/********************************************************************************************* 
 Class for pointers to functions implementing the interface methods on the owner object
 
 Typically, a static object (singleton) is defined in the owner object implementation and
 initialized with pointers to implementation functions.
*********************************************************************************************/

typedef struct _CNetworkServerProtocolItfImpl
{
  // Public
  AddRef m_pAddRef;
  ReleaseItf m_pReleaseItf;
  BuildUplinkMessage m_pBuildUplinkMessage;
  ProcessServerMessage m_pProcessServerMessage;
  ProcessSessionEvent m_pProcessSessionEvent;
} CNetworkServerProtocolItfImplOb;

typedef struct _CNetworkServerProtocolItfImpl * CNetworkServerProtocolItfImpl;


#endif 

