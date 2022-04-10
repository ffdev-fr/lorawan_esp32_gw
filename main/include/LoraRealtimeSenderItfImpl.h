/*****************************************************************************************//**
 * @file     LoraRealtimeSenderItfImpl.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     24/12/2018
 *
 * @brief    Structures and definitions for 'ILoraRealtimeSender' interface implementation.
 *
 * @details  Used by objects implementing the interface and for the implementation of the
 *           interface object.
*********************************************************************************************/

#ifndef LORAREALTIMESENDERITFIMPL_H
#define LORAREALTIMESENDERITFIMPL_H


/********************************************************************************************* 
  Forward declarations
*********************************************************************************************/
           

/********************************************************************************************* 
  Prototypes of interface methods                                                                         .
  
  These methods are implemented in the objects exposing the interface.
  Pointers to these methods are registered in the 'ILoraRealtimeSenderItfImpl' object by the
  object implementing the methods.
*********************************************************************************************/

// Public
typedef uint32_t (*AddRef)(void *pOwnerObject);
typedef uint32_t (*ReleaseItf)(void *pOwnerObject);

typedef bool (*Initialize)(void *pOwnerObject, CLoraRealtimeSenderItf_InitializeParams pParams);
typedef bool (*Start)(void *pOwnerObject, CLoraRealtimeSenderItf_StartParams pParams);
typedef bool (*Stop)(void *pOwnerObject, CLoraRealtimeSenderItf_StopParams pParams);

typedef DWORD (*RegisterNodeRxWindows)(void *pOwnerObject, CLoraRealtimeSenderItf_RegisterNodeRxWindowsParams pParams);
typedef DWORD (*ScheduleSendNodePacket)(void *pOwnerObject, CLoraRealtimeSenderItf_ScheduleSendNodePacketParams pParams);

                                                  

/********************************************************************************************* 
 Class for pointers to functions implementing the interface methods on the owner object
 
 Typically, a static object (singleton) is defined in the owner object implementation and
 initialized with pointers to implementation functions.
*********************************************************************************************/

typedef struct _CLoraRealtimeSenderItfImpl
{
  // Public
  AddRef m_pAddRef;
  ReleaseItf m_pReleaseItf;
  Initialize m_pInitialize;
  Start m_pStart;
  Stop m_pStop;
  RegisterNodeRxWindows m_pRegisterNodeRxWindows;
  ScheduleSendNodePacket m_pScheduleSendNodePacket;
} CLoraRealtimeSenderItfImplOb;

typedef struct _CLoraRealtimeSenderItfImpl * CLoraRealtimeSenderItfImpl;


#endif 

