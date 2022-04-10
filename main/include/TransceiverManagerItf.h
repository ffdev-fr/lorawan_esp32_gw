/*****************************************************************************************//**
 * @file     TransceiverManagerItf.h
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

#ifndef TRANSCEIVERMANAGERITF_H
#define TRANSCEIVERMANAGERITF_H

#include "LoraTransceiverItf.h"

// Forward declaration to interface object (used as this on interface methods)
typedef struct _ITransceiverManager * ITransceiverManager;


/********************************************************************************************* 
  Public definitions used by methods of 'ITransceiverManager' interface
*********************************************************************************************/



/********************************************************************************************* 
  Objects used as parameter on methods of 'ITransceiverManager' interface
*********************************************************************************************/

// Forward declarations (objects used to exchange data on interface)
//typedef struct _CTransceiverManager_InitializeParams * CTransceiverManagerItf_InitializeParams;
//typedef struct _CTransceiverManager_AttachParams * CTransceiverManagerItf_AttachParams;
//typedef struct _CTransceiverManager_StartParams * CTransceiverManagerItf_StartParams;
//typedef struct _CTransceiverManager_StopParams * CTransceiverManagerItf_StopParams;
//typedef struct _CTransceiverManager_SessionEvent * CTransceiverManagerItf_SessionEvent;


// Configuration for a 'LoraTransceiver'
// Note: 
//  - This object is used in 'params' object of 'ITransceiverManager_Initialize' method
//  - Typically, the gateway configuration file contains a similar structure for settings definition
//  - This object is constructed using objects defined by 'ILoraTransceiver' interface
typedef struct _CTransceiverManagerItf_LoraTransceiverSettings
{
  // Public
  CLoraTransceiverItf_SetLoraMACParamsOb LoraMAC;
  CLoraTransceiverItf_SetLoraModeParamsOb LoraMode;
  CLoraTransceiverItf_SetPowerModeParamsOb PowerMode;
  CLoraTransceiverItf_SetFreqChannelParamsOb FreqChannel;
} CTransceiverManagerItf_LoraTransceiverSettingsOb;

typedef struct _CTransceiverManagerItf_LoraTransceiverSettings * CTransceiverManagerItf_LoraTransceiverSettings;

typedef struct _CTransceiverManagerItf_InitializeParams
{
  // Public

  // ServerManager to connect to TransceiverManager (i.e. Server side of the gateway)
  // The 'Attach' method on this 'IServerManager' interface is invoked to provide the
  // TransceiverManager's task to notify when new downlink packets are received
  void *m_pServerManagerItf;                

  // Configuration settings
  bool m_bUseBuiltinSettings;    
  CTransceiverManagerItf_LoraTransceiverSettingsOb pLoraTransceiverSettings[];
} CTransceiverManagerItf_InitializeParamsOb;

typedef CTransceiverManagerItf_InitializeParamsOb * CTransceiverManagerItf_InitializeParams;


// Administrative commands
typedef struct _CTransceiverManagerItf_AttachParams
{
  // Public
  TaskFunction_t m_hPacketForwarderTask;
} CTransceiverManagerItf_AttachParamsOb;

typedef CTransceiverManagerItf_AttachParamsOb * CTransceiverManagerItf_AttachParams;


typedef struct _CTransceiverManagerItf_StartParams
{
  // Public
  bool m_bForce;
} CTransceiverManagerItf_StartParamsOb;

typedef CTransceiverManagerItf_StartParamsOb * CTransceiverManagerItf_StartParams;


typedef struct _CTransceiverManagerItf_StoptParams
{
  // Public
  bool m_bForce;
} CTransceiverManagerItf_StopParamsOb;

typedef CTransceiverManagerItf_StopParamsOb * CTransceiverManagerItf_StopParams;


/********************************************************************************************* 
  CTransceiverManagerItf_SessionEvent object

  Used to notify the 'TransceiverManager' of status changes of the 'LoraPacket' in gateway.
  Typically, the 'TransceiverManager' uses these events to apply LoRa protocol rules for the
  packet (i.e. 'LoraPacket' session life)
*********************************************************************************************/

typedef struct _CTransceiverManagerItf_SessionEvent
{
  // Public
  WORD m_wEventType;               // The event ('TRANSCEIVERMANAGER_SESSIONEVENT_xxx')
  void *m_pSession;                
  DWORD m_dwSessionId;             // Unique identifier of session
} CTransceiverManagerItf_SessionEventOb;

typedef CTransceiverManagerItf_SessionEventOb * CTransceiverManagerItf_SessionEvent;

// Note: Base for separation from other commands or internal messages in 'TransceiverManager'
#define TRANSCEIVERMANAGER_SESSIONEVENT_BASE                0x00001000

#define TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_ACCEPTED     (TRANSCEIVERMANAGER_SESSIONEVENT_BASE)
#define TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_REJECTED     (TRANSCEIVERMANAGER_SESSIONEVENT_BASE + 1)
#define TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_PROGRESSING  (TRANSCEIVERMANAGER_SESSIONEVENT_BASE + 2)
#define TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_SENT         (TRANSCEIVERMANAGER_SESSIONEVENT_BASE + 3)
#define TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_FAILED       (TRANSCEIVERMANAGER_SESSIONEVENT_BASE + 4)
#define TRANSCEIVERMANAGER_SESSIONEVENT_DOWNLINK_SCHEDULED  (TRANSCEIVERMANAGER_SESSIONEVENT_BASE + 5)
#define TRANSCEIVERMANAGER_SESSIONEVENT_DOWNLINK_SENDING    (TRANSCEIVERMANAGER_SESSIONEVENT_BASE + 6)
#define TRANSCEIVERMANAGER_SESSIONEVENT_DOWNLINK_SENT       (TRANSCEIVERMANAGER_SESSIONEVENT_BASE + 7)
#define TRANSCEIVERMANAGER_SESSIONEVENT_DOWNLINK_FAILED     (TRANSCEIVERMANAGER_SESSIONEVENT_BASE + 8)


/********************************************************************************************* 
  Public methods of 'ITransceiverManager' interface
 
  These methods are available to any object which has obtained a reference to the
  'ITransceiverManager' interface (i.e. this reference is required with all interface methods)
*********************************************************************************************/

uint32_t ITransceiverManager_AddRef(ITransceiverManager this);
uint32_t ITransceiverManager_ReleaseItf(ITransceiverManager this);

bool ITransceiverManager_Initialize(ITransceiverManager this, CTransceiverManagerItf_InitializeParams pParams);
bool ITransceiverManager_Attach(ITransceiverManager this, CTransceiverManagerItf_AttachParams pParams);
bool ITransceiverManager_Start(ITransceiverManager this, CTransceiverManagerItf_StartParams pParams);
bool ITransceiverManager_Stop(ITransceiverManager this, CTransceiverManagerItf_StopParams pParams);

bool ITransceiverManager_SessionEvent(ITransceiverManager this, CTransceiverManagerItf_SessionEvent pEvent);




#ifdef TRANSCEIVERMANAGERITF_IMPL

/********************************************************************************************* 
  Classes and definition used by objects which expose the 'ITransceiverManager' interface and
  by the interface implementation itself
 
  Note: The implementation file of these objects must define 'TRANSCEIVERMANAGERITF_IMPL'
        (i.e. in order to include the following block of code) 
*********************************************************************************************/

#include "TransceiverManagerItfImpl.h"

/********************************************************************************************* 
  Forward declarations
*********************************************************************************************/
           

/********************************************************************************************* 
  Interface class instanced by objects implementing the interface methods                                                              .
                                                                                                                                        .
  This object is instanced by 'ITransceiverManager_New' method (protected constructor)                                                                           .
*********************************************************************************************/

// Interface class data
typedef struct _ITransceiverManager
{
  // Private attributes
  void *m_pOwnerObject;                          // Instance of the object exposing the interface
                                                 // This pointer is provided as argument on interface methods
  CTransceiverManagerItfImpl m_pOwnerItfImpl;    // Pointers to implementation functions on owner object
} ITransceiverManagerOb;



/********************************************************************************************* 
  Protected methods used by objects implementing the 'ITransceiverManager' interface
*********************************************************************************************/
 
ITransceiverManager ITransceiverManager_New(void *pOwnerObject, CTransceiverManagerItfImpl pOwnerItfImpl);
void ITransceiverManager_Delete(ITransceiverManager this);


#endif
#endif

