/*****************************************************************************************//**
 * @file     TransceiverManagerItfImpl.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     04/03/2018
 *
 * @brief    Structures and definitions for 'ITransceiverManager' interface implementation.
 *
 * @details  Used by objects implementing the interface and for the implementation of the
 *           interface object.
*********************************************************************************************/

#ifndef TRANSCEIVERMANAGERITFIMPL_H
#define TRANSCEIVERMANAGERITFIMPL_H


/********************************************************************************************* 
  Forward declarations
*********************************************************************************************/
           


/********************************************************************************************* 
  Prototypes of interface methods                                                                         .
  
  These methods are implemented in the objects exposing the interface.
  Pointers to these methods are registered in the 'ITransceiverManagerItfImpl' object by the
  object implementing the methods.
*********************************************************************************************/


// Public
typedef uint32_t (*AddRef)(void *pOwnerObject);
typedef uint32_t (*ReleaseItf)(void *pOwnerObject);

typedef bool (*Initialize)(void *pOwnerObject, void *pParams);
typedef bool (*Attach)(void *pOwnerObject, void *pParams);
typedef bool (*Start)(void *pOwnerObject, void *pParams);
typedef bool (*Stop)(void *pOwnerObject, void *pParams);
typedef bool (*SessionEvent)(void *pOwnerObject, void *pEvent);



/********************************************************************************************* 
 Class for pointers to functions implementing the interface methods on the owner object
 
 Typically, a static object (singleton) is defined in the owner object implementation and
 initialized with pointers to implementation functions.
*********************************************************************************************/

typedef struct _CTransceiverManagerItfImpl
{
  // Public
  AddRef m_pAddRef;
  ReleaseItf m_pReleaseItf;
  Initialize m_pInitialize;
  Attach m_pAttach;
  Start m_pStart;
  Stop m_pStop;
  SessionEvent m_pSessionEvent;

} CTransceiverManagerItfImplOb;

typedef struct _CTransceiverManagerItfImpl * CTransceiverManagerItfImpl;

#endif 

