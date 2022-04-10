/*****************************************************************************************//**
 * @file     LoraTransceiverItfImpl.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     16/02/2018
 *
 * @brief    Structures and definitions for 'ILoraTransceiver' interface implementation.
 *
 * @details  Used by objects implementing the interface and for the implementation of the
 *           interface object.
*********************************************************************************************/

#ifndef LORATRANSCEIVERITFIMPL_H
#define LORATRANSCEIVERITFIMPL_H


/********************************************************************************************* 
  Forward declarations
*********************************************************************************************/
           


/********************************************************************************************* 
  Prototypes of interface methods                                                                         .
  
  These methods are implemented in the objects exposing the interface.
  Pointers to these methods are registered in the 'ILoraTransceiverItfImpl' object by the
  object implementing the methods.
*********************************************************************************************/


// Public
typedef uint32_t (*AddRef)(void *pOwnerObject);
typedef uint32_t (*ReleaseItf)(void *pOwnerObject);

typedef bool (*Initialize)(void *pOwnerObject, void *pParams);
typedef bool (*SetLoraMAC)(void *pOwnerObject, void *pParams);
typedef bool (*SetLoraMode)(void *pOwnerObject, void *pParams);
typedef bool (*SetPowerMode)(void *pOwnerObject, void *pParams);
typedef bool (*SetFreqChannel)(void *pOwnerObject, void *pParams);

typedef bool (*StandBy)(void *pOwnerObject, void *pParams);
typedef bool (*Receive)(void *pOwnerObject, void *pParams);
typedef bool (*Send)(void *pOwnerObject, void *pParams);

typedef bool (*GetReceivedPacketInfo)(void *pOwnerObject, void *pParams);
                                                  

/********************************************************************************************* 
 Class for pointers to functions implementing the interface methods on the owner object
 
 Typically, a static object (singleton) is defined in the owner object implementation and
 initialized with pointers to implementation functions.
*********************************************************************************************/

typedef struct _CLoraTransceiverItfImpl
{
  // Public
  AddRef m_pAddRef;
  ReleaseItf m_pReleaseItf;
  Initialize m_pInitialize;
  SetLoraMAC m_pSetLoraMAC;
  SetLoraMode m_pSetLoraMode;
  SetPowerMode m_pSetPowerMode;
  SetFreqChannel m_pSetFreqChannel;
  StandBy m_pStandBy;
  Receive m_pReceive;
  Send m_pSend;
  GetReceivedPacketInfo m_pGetReceivedPacketInfo;
} CLoraTransceiverItfImplOb;

typedef struct _CLoraTransceiverItfImpl * CLoraTransceiverItfImpl;

#endif 
