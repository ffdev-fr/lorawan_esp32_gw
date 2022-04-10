/*****************************************************************************************//**
 * @file     LoraRealtimeSenderItf.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     24/12/2018
 *
 * @brief    Class for 'ILoraRealtimeSender' interface.
 *
 * @details  This class is instanced by objects implementing the interface.\n
 *           Typically, the interface is provided by owner object in return of its static
 *           'CreateInstance' function.
 *           In current implementation, this interface is provided by the CLoraRealtimeSender
 *           object.
 *
 * @note     This class IS THREAD SAFE.\n
 *           The client object must call the 'ReleaseItf' method on 'ILoraRealtimeSender'
 *           interface to destroy the implementation object created by 'CreateInstance'.
 *
 * @note     This interface is specified for software design consistency (i.e. only the
 *           'CLoraRealtimeSender' object implements it).
*********************************************************************************************/

#ifndef LORAREALTIMESENDERITF_H
#define LORAREALTIMESENDERITF_H


// Forward declaration to interface object (used as this on interface methods)
typedef struct _ILoraRealtimeSender * ILoraRealtimeSender;

// Forward declarations (objects used to exchange data on interface)
typedef struct _CLoraRealtimeSenderItf_InitializeParams * CLoraRealtimeSenderItf_InitializeParams;
typedef struct _CLoraRealtimeSenderItf_StartParams * CLoraRealtimeSenderItf_StartParams;
typedef struct _CLoraRealtimeSenderItf_StopParams * CLoraRealtimeSenderItf_StopParams;
typedef struct _CLoraRealtimeSenderItf_RegisterNodeRxWindowsParams * CLoraRealtimeSenderItf_RegisterNodeRxWindowsParams;
typedef struct _CLoraRealtimeSenderItf_ScheduleSendNodePacketParams * CLoraRealtimeSenderItf_ScheduleSendNodePacketParams;


/********************************************************************************************* 
  Public definitions used by methods of 'ILoraRealtimeSender' interface
*********************************************************************************************/



/********************************************************************************************* 
  Objects used as parameter on methods of 'ILoraRealtimeSender' interface
*********************************************************************************************/

typedef struct _CLoraRealtimeSenderItf_InitializeParams
{
  // Public
  
  // Interface of parent object ('CLoraNodeManager')
  // This interface is used to notify events occurring during send operation (i.e. in order to
  // manage the downlink session)
  void * m_pTransceiverManagerItf; 

} CLoraRealtimeSenderItf_InitializeParamsOb;


typedef struct _CLoraRealtimeSenderItf_StartParams
{
  // Public
  bool m_bForce;
} CLoraRealtimeSenderItf_StartParamsOb;


typedef struct _CLoraRealtimeSenderItf_StoptParams
{
  // Public
  bool m_bForce;
} CLoraRealtimeSenderItf_StopParamsOb;

//
// Parameters and definitions for 'RegisterNodeRxWindows' method
//

typedef struct _CLoraRealtimeSenderItf_RegisterNodeRxWindowsParams
{
  // Public

  // LoRa class (value: LORAREALTIMESENDER_DEVICECLASS_x)
  BYTE m_usDeviceClass;

  // Node unique identifier
  DWORD m_dwDeviceAddr;

  // The interface to 'LoraTransceiver' receiving the uplink packet
  // The downlink packet will be sent using this transceiver (i.e. same radio settings) 
  ILoraTransceiver m_pLoraTransceiverItf;

  // Timestamp for end of transmission of uplink packet
  // Value in milliseconds
  DWORD m_dwRXTimestamp;

} CLoraRealtimeSenderItf_RegisterNodeRxWindowsParamsOb;


// LoRa Class for node (i.e. allowed values for 'm_usDeviceClass' variable) 
// Note: Use values defined on 'LoraRealtimeSenderItf' interface
#define LORAREALTIMESENDER_DEVICECLASS_A         1
#define LORAREALTIMESENDER_DEVICECLASS_C         2

//
// Parameters and definitions for 'ScheduleSendNodePacket' method
//

typedef struct _CLoraRealtimeSenderItf_ScheduleSendNodePacketParams
{
  // Public

  // Node unique identifier
  DWORD m_dwDeviceAddr;

  // Identifiers of session defined in parent object for management of the downlink LoRa packet
  // Note: Thess identifiers are used by 'LoraRealtimeSender' to send notifications to parent object
  DWORD m_dwDownlinkSessionId;
  void *m_pDownlinkSession;


  // Downlink Lora packet to send
  CLoraTransceiverItf_LoraPacket m_pPacketToSend;

} CLoraRealtimeSenderItf_ScheduleSendNodePacketParamsOb;



// Result codes for 'ILoraRealtimeSender_ScheduleSendNodePacket' method
//
// The result codes are defined using the specification of Semtech protocol:
//  - NONE             = Packet has been programmed for downlink
//  - TOO_LATE         = Rejected because it was already too late to program this packet for downlink
//  - TOO_EARLY        = Rejected because downlink packet timestamp is too much in advance
//  - COLLISION_PACKET = Rejected because there was already a packet programmed in requested timeframe
//  - COLLISION_BEACON = Rejected because there was already a beacon planned in requested timeframe
//  - TX_FREQ          = Rejected because requested frequency is not supported by TX RF chain
//  - TX_POWER         = Rejected because requested power is not supported by gateway
//  - GPS_UNLOCKED     = Rejected because GPS is unlocked, so GPS timestamp cannot be used

#define LORAREALTIMESENDER_SCHEDULESEND_NONE                0
#define LORAREALTIMESENDER_SCHEDULESEND_TOO_LATE            1
#define LORAREALTIMESENDER_SCHEDULESEND_TOO_EARLY           2
#define LORAREALTIMESENDER_SCHEDULESEND_COLLISION_PACKET    3
#define LORAREALTIMESENDER_SCHEDULESEND_COLLISION_BEACON    4
#define LORAREALTIMESENDER_SCHEDULESEND_TX_FREQ             5
#define LORAREALTIMESENDER_SCHEDULESEND_TX_POWER            6
#define LORAREALTIMESENDER_SCHEDULESEND_GPS_UNLOCKED        7



// CLoraRealtimeSender object factory
// This method in invoked by client objet to create a new instance of CLoraRealtimeSender object
ILoraRealtimeSender CLoraRealtimeSender_CreateInstance();


/********************************************************************************************* 
  Public methods of 'ILoraRealtimeSender' interface
 
  These methods are available to any object which has obtained a reference to the
  'ILoraRealtimeSender' interface (i.e. this reference is required with all interface methods)
*********************************************************************************************/

uint32_t ILoraRealtimeSender_AddRef(ILoraRealtimeSender this);
uint32_t ILoraRealtimeSender_ReleaseItf(ILoraRealtimeSender this);

bool ILoraRealtimeSender_Initialize(ILoraRealtimeSender this, CLoraRealtimeSenderItf_InitializeParams pParams);
bool ILoraRealtimeSender_Start(ILoraRealtimeSender this, CLoraRealtimeSenderItf_StartParams pParams);
bool ILoraRealtimeSender_Stop(ILoraRealtimeSender this, CLoraRealtimeSenderItf_StopParams pParams);

bool ILoraRealtimeSender_RegisterNodeRxWindows(ILoraRealtimeSender this, CLoraRealtimeSenderItf_RegisterNodeRxWindowsParams pParams);
DWORD ILoraRealtimeSender_ScheduleSendNodePacket(ILoraRealtimeSender this, CLoraRealtimeSenderItf_ScheduleSendNodePacketParams pParams);

#endif

#ifdef LORAREALTIMESENDERITF_IMPL

/********************************************************************************************* 
  Classes and definition used by objects which expose the 'ILoraRealtimeSender' interface and
  by the interface implementation itself
 
  Note: The implementation file of these objects must define 'NETWORKSERVERPROTOCOLITF_IMPL'
        (i.e. in order to include the following block of code) 
*********************************************************************************************/

#include "LoraRealtimeSenderItfImpl.h"

/********************************************************************************************* 
  Forward declarations
*********************************************************************************************/
           

/********************************************************************************************* 
  Interface class instanced by objects implementing the interface methods                                                              .
                                                                                                                                        .
  This object is instanced by 'ILoraRealtimeSender_New' method (protected constructor)                                                                           .
*********************************************************************************************/

// Interface class data
typedef struct _ILoraRealtimeSender
{
  // Private attributes
  void *m_pOwnerObject;                            // Instance of the object exposing the interface
                                                   // This pointer is provided as argument on interface methods
  CLoraRealtimeSenderItfImpl m_pOwnerItfImpl;      // Pointers to implementation functions on owner object
} ILoraRealtimeSenderOb;



/********************************************************************************************* 
  Protected methods used by objects implementing the 'ILoraRealtimeSender' interface
*********************************************************************************************/
 
ILoraRealtimeSender ILoraRealtimeSender_New(void *pOwnerObject, CLoraRealtimeSenderItfImpl pOwnerItfImpl);
void ILoraRealtimeSender_Delete(ILoraRealtimeSender this);


#endif




