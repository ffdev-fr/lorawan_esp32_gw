/*****************************************************************************************//**
 * @file     NetworkServerProtocolItf.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     25/03/2018
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

#ifndef NETWORKSERVERPROTOCOLITF_H
#define NETWORKSERVERPROTOCOLITF_H


#include "TransceiverManagerItf.h"


// Forward declaration to interface object (used as this on interface methods)
typedef struct _INetworkServerProtocol * INetworkServerProtocol;


/********************************************************************************************* 
  Public definitions used by methods of 'INetworkServerProtocol' interface
*********************************************************************************************/



/********************************************************************************************* 
  Objects used as parameter on methods of 'INetworkServerProtocol' interface
*********************************************************************************************/

// Forward declarations (objects used to exchange data on interface)
typedef struct _CNetworkServerProtocol_BuildUplinkMessageParams * CNetworkServerProtocolItf_BuildUplinkMessageParams;
typedef struct _CNetworkServerProtocol_ProcessServerMessageParams * CNetworkServerProtocolItf_ProcessServerMessageParams;
typedef struct _CNetworkServerProtocol_ProcessSessionEventParams * CNetworkServerProtocolItf_ProcessSessionEventParams;


// Types for protocol Uplink messages (generic: protocol independent) 
#define NETWORKSERVERPROTOCOL_UPLINKMSG_HEARTBEAT  0x0001
#define NETWORKSERVERPROTOCOL_UPLINKMSG_LORADATA   0x0002

typedef struct _CNetworkServerProtocol_BuildUplinkMessageParams
{
  // Public
  
  // Type of Uplink message to generate
  //  - NETWORKSERVERPROTOCOL_UPLINKMSG_HEARTBEAT = Asks the 'ProtocolEngine' if it has a 'heartbeat' message to send
  //  - NETWORKSERVERPROTOCOL_UPLINKMSG_LORADATA = Asks the protocolEngine to build the message for sending LoRa data
  WORD m_wMessageType;

  // Message identifier in caller 'ServerManager' (used to build 'm_dwProtocolMessageId')
  WORD m_wServerManagerMessageId;

  // Do not use period configuration for generation of 'Heartbeat' message (i.e. always generate message)
  WORD m_bForceHeartbeat;
  
  // The uplink LoRa packet
  // Not required for 'Heartbeat' message
  CLoraTransceiverItf_LoraPacket m_pLoraPacket;

  // Additional information for uplink LoRa packet
  CLoraTransceiverItf_ReceivedLoraPacketInfo m_pLoraPacketInfo;

  // Buffer where generate the message stream for Network Server
  WORD m_wMaxMessageLength;
  WORD m_wMessageLength;
  BYTE *m_pMessageData;

  // RETURNED INFORMATION
  //
  // Identifier of message in both 'CLoraServerManager' and associated 'ProtocolEngine'
  // The identifier format is:
  //  - LOWORD = Identifier in 'ProtocolEngine'. 
  //             This identifer depends on'ProtocolEngine' implementation and it is used to 
  //             retrieve the protocol session associated to the message.
  //  - HIWORD = Identifier in 'CLoraServerManager'.
  //             This identifier is the provide 'm_wServerManagerMessageId' (see above)
  //
  // NOTE: This identifier MUST be provided in 'INetworkServerProtocol_ProcessSessionEvent' method's parameters  
  DWORD m_dwProtocolMessageId;

} CNetworkServerProtocol_BuildUplinkMessageParamsOb;

// Message received from Network Server
typedef struct _CNetworkServerProtocol_ProcessServerMessageParams
{
  // Public

  // Buffer containing message stream received from Network Server
  WORD m_wMessageLength;
  BYTE *m_pMessageData;

  // Buffer where generate the LoRa packet if received data must be forwarded to node
  //
  // TO CHECK = maybe object defined in CLoraTransceiverItf (i.e. to avoid copy)
  WORD m_wMaxLoraPacketLength;
  WORD m_wLoraPacketLength;
  BYTE *m_pData;

  // RETURNED INFORMATION
  //
  // Identifier of message in both 'CLoraServerManager' and associated 'ProtocolEngine'
  // The identifier format is:
  //  - LOWORD = Identifier in 'ProtocolEngine'. 
  //             This identifer depends on'ProtocolEngine' implementation and it is used to 
  //             retrieve the protocol session associated to the message.
  //  - HIWORD = Identifier in 'CLoraServerManager'.
  //             This identifier is the provide 'm_wServerManagerMessageId' (see above)
  DWORD m_dwProtocolMessageId; 

} CNetworkServerProtocol_ProcessServerMessageParamsOb;


// Types for event occured on session (generic: protocol independent) 
// Event codes used with 'INetworkServerProtocol_ProcessSessionEvent' method.
#define NETWORKSERVERPROTOCOL_SESSIONEVENT_SENT        0x0001
#define NETWORKSERVERPROTOCOL_SESSIONEVENT_SENDFAILED  0x0002
#define NETWORKSERVERPROTOCOL_SESSIONEVENT_CANCELED    0x0003
#define NETWORKSERVERPROTOCOL_SESSIONEVENT_RELEASED    0x0004     // The owner object will do not invoke 'ProtocolEngine'
                                                                  // for the associated session (i.e. 'ProtocolEngine' can 
                                                                  // release its associated resources)

// Result codes for 'INetworkServerProtocol_ProcessSessionEvent' and 'INetworkServerProtocol_ProcessServerMessage' methods 
// 
// Note: 
//  - These methods are invoked according to the 'direction' of message from 'Connector' point of view (i.e. send or receive)
//  - This direction is may be different from 'Protocol' point of view (e.g. 'ACK' message received for uplink send to NS)
//  - The 'xxx_UPLINKSESSIONEVENT_xxx' and '_DOWNLINKSESSIONEVENT_' codes are from 'Protocol' point of view
//  - Each of these methods can return 'xxx_UPLINKSESSIONEVENT_xxx' and '_DOWNLINKSESSIONEVENT_' codes
//    (codes must have different numerical values)
// 
//  - NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_PROGRESSING = The 'ProtocolEngine' session is waiting for next event/message
//  - NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_TERMINATED  = The 'ProtocolEngine' session is successfully terminated
//                                                           (no more event/message expected, the owner object must confirm
//                                                           with NETWORKSERVERPROTOCOL_SESSIONEVENT_RELEASED)
//  - NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_FAILED      = A fatal error occured on 'ProtocolEngine' session. 
//                                                           The session is terminated (no more event/message expected)
//
//  - NETWORKSERVERPROTOCOL_DOWNLINKSESSIONEVENT_PREPARED  = 
//
//  - NETWORKSERVERPROTOCOL_SESSIONERROR_MESSAGE           = The header of received message is invalid (probably UDP corrupted)
//                                                           The associated Transaction cannot be found (will be deleted by
//                                                           expired session check)
//  - NETWORKSERVERPROTOCOL_SESSIONERROR_TRANSACTION       = Unable to retrieve transaction (downlink message too late)
 
#define NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_PROGRESSING   0x0001     // Last asked operation is still progressing (typically send)
#define NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_TERMINATED    0x0003
#define NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_FAILED        0x0004

#define NETWORKSERVERPROTOCOL_DOWNLINKSESSIONEVENT_PREPARED    0x0010     // Data provided with event are processed and ready for use

#define NETWORKSERVERPROTOCOL_SESSIONERROR_OK                  0x1000     // Generic session event succefully processed
#define NETWORKSERVERPROTOCOL_SESSIONERROR_MESSAGE             0x2000     // Invalid downlink message (header)
#define NETWORKSERVERPROTOCOL_SESSIONERROR_TRANSACTION         0x3000     // Unable to retrieve transatcion (downlink message too late)


#define NETWORKSERVERPROTOCOL_IS_UPLINKSESSIONEVENT(ev)   ((ev & 0x000F) != 0 ? (true):(false))
#define NETWORKSERVERPROTOCOL_IS_DOWNLINKSESSIONEVENT(ev) ((ev & 0x00F0) != 0 ? (true):(false))
#define NETWORKSERVERPROTOCOL_IS_SESSIONERROR(ev)         ((ev & 0xF000) != 0 ? (true):(false))


typedef struct _CNetworkServerProtocol_ProcessSessionEventParams
{
  // Public

  // Event
  WORD m_wSessionEvent;

  // Identifier of message in both 'CLoraServerManager' and associated 'ProtocolEngine'
  // Note: This identifier is provided when uplink message is generated by 'BuildUplinkMessage' method
  DWORD m_dwProtocolMessageId; 

} CNetworkServerProtocol_ProcessSessionEventParamsOb;


/********************************************************************************************* 
  Public methods of 'INetworkServerProtocol' interface
 
  These methods are available to any object which has obtained a reference to the
  'INetworkServerProtocol' interface (i.e. this reference is required with all interface methods)
*********************************************************************************************/

uint32_t INetworkServerProtocol_AddRef(INetworkServerProtocol this);
uint32_t INetworkServerProtocol_ReleaseItf(INetworkServerProtocol this);

bool INetworkServerProtocol_BuildUplinkMessage(INetworkServerProtocol this, CNetworkServerProtocolItf_BuildUplinkMessageParams pParams);
DWORD INetworkServerProtocol_ProcessServerMessage(INetworkServerProtocol this, CNetworkServerProtocolItf_ProcessServerMessageParams pParams);
DWORD INetworkServerProtocol_ProcessSessionEvent(INetworkServerProtocol this, CNetworkServerProtocolItf_ProcessSessionEventParams pParams);

#endif

#ifdef NETWORKSERVERPROTOCOLITF_IMPL

/********************************************************************************************* 
  Classes and definition used by objects which expose the 'INetworkServerProtocol' interface and
  by the interface implementation itself
 
  Note: The implementation file of these objects must define 'NETWORKSERVERPROTOCOLITF_IMPL'
        (i.e. in order to include the following block of code) 
*********************************************************************************************/

#include "NetworkServerProtocolItfImpl.h"

/********************************************************************************************* 
  Forward declarations
*********************************************************************************************/
           

/********************************************************************************************* 
  Interface class instanced by objects implementing the interface methods                                                              .
                                                                                                                                        .
  This object is instanced by 'INetworkServerProtocol_New' method (protected constructor)                                                                           .
*********************************************************************************************/

// Interface class data
typedef struct _INetworkServerProtocol
{
  // Private attributes
  void *m_pOwnerObject;                            // Instance of the object exposing the interface
                                                   // This pointer is provided as argument on interface methods
  CNetworkServerProtocolItfImpl m_pOwnerItfImpl;   // Pointers to implementation functions on owner object
} INetworkServerProtocolOb;



/********************************************************************************************* 
  Protected methods used by objects implementing the 'INetworkServerProtocol' interface
*********************************************************************************************/
 
INetworkServerProtocol INetworkServerProtocol_New(void *pOwnerObject, CNetworkServerProtocolItfImpl pOwnerItfImpl);
void INetworkServerProtocol_Delete(INetworkServerProtocol this);


#endif
//#endif




