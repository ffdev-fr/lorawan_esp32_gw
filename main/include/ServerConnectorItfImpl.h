/*****************************************************************************************//**
 * @file     ServerConnectorItfImpl.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     22/04/2018
 *
 * @brief    Structures and definitions for 'IServerConnector' interface implementation.
 *
 * @details  Used by objects implementing the interface and for the implementation of the
 *           interface object.
*********************************************************************************************/

#ifndef SERVERCONNECTORITFIMPL_H
#define SERVERCONNECTORITFIMPL_H


/********************************************************************************************* 
  Forward declarations
*********************************************************************************************/
           


/********************************************************************************************* 
  Prototypes of interface methods                                                                         .
  
  These methods are implemented in the objects exposing the interface.
  Pointers to these methods are registered in the 'IServerConnectorItfImpl' object by the
  object implementing the methods.
*********************************************************************************************/

// Public
typedef uint32_t (*AddRef)(void *pOwnerObject);
typedef uint32_t (*ReleaseItf)(void *pOwnerObject);

//typedef int (*AddRef)(void *pOwnerObject);
//typedef int (*ReleaseItf)(void *pOwnerObject);

typedef bool (*Initialize)(void *pOwnerObject, void *pParams);
typedef bool (*Start)(void *pOwnerObject, void *pParams);
typedef bool (*Stop)(void *pOwnerObject, void *pParams);
typedef bool (*Send)(void *pOwnerObject, void *pParams);
typedef bool (*SendReceive)(void *pOwnerObject, void *pParams);
typedef bool (*DownlinkReceived)(void *pOwnerObject, void *pParams);
                                                  

/********************************************************************************************* 
 Class for pointers to functions implementing the interface methods on the owner object
 
 Typically, a static object (singleton) is defined in the owner object implementation and
 initialized with pointers to implementation functions.
*********************************************************************************************/

typedef struct _CServerConnectorItfImpl
{
  // Public
  AddRef m_pAddRef;
  ReleaseItf m_pReleaseItf;
  Initialize m_pInitialize;
  Start m_pStart;
  Stop m_pStop;
  Send m_pSend;
  SendReceive m_pSendReceive;
  DownlinkReceived m_pDownlinkReceived;
} CServerConnectorItfImplOb;

typedef struct _CServerConnectorItfImpl * CServerConnectorItfImpl;

#endif 


