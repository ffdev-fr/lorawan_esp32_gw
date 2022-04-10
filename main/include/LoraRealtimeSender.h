/*****************************************************************************************//**
 * @file     LoraRealtimeSender.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     24/12/2018
 *
 * @brief    Sends downlink LoRa packets to nodes when their RX windows are open.
 *
 * @details  The downlink packets can be sent to nodes only when they are listening (i.e. the
 *           radio device may activate RX mode only for a short period of time).\n
 *           The owner object transmits to 'LoraRealtimeSender' the downlink LoRa packets to send.
 *           When a LoRa packet is enqueued for send, the 'LoraRealtimeSender' computes the time
 *           when Lora packet can be send to destination node.
 *           The 'LoraRealtimeSender' automatically sends waiting packets at the right time.\n
 *
 * @note     In current version only LoRaWAN class A is implemented.
 *
 * @note     This object is only used by 'LoraNodeManager' object (i.e. child object).
 *           The 'LoraNodeManager' delegates the downlink send operation to 'LoraRealTimeSender'.
 *
 * @note     The "LoRaWAN ESP32 Gateway V1.x" project is designed for execution on ESP32 
 *           Module (Dev.C kit).\n
 *           The implementation uses the Espressif IDF V3.0 framework (with RTOS)              
*********************************************************************************************/

#ifndef LORAREALTIMESENDER_H_
#define LORAREALTIMESENDER_H_

/*********************************************************************************************
  Includes
*********************************************************************************************/

#include "Utilities.h"


/********************************************************************************************* 
  Definitions for debug traces
  The debug level is specified with 'SEMTECHPROTOCOLENGINE_DEBUG_LEVEL' in Definitions.h file
*********************************************************************************************/

#define LORAREALTIMESENDER_DEBUG_LEVEL0 ((LORAREALTIMESENDER_DEBUG_LEVEL & 0x01) > 0)
#define LORAREALTIMESENDER_DEBUG_LEVEL1 ((LORAREALTIMESENDER_DEBUG_LEVEL & 0x02) > 0)
#define LORAREALTIMESENDER_DEBUG_LEVEL2 ((LORAREALTIMESENDER_DEBUG_LEVEL & 0x04) > 0)


/********************************************************************************************* 
  Definitions (implementation)
*********************************************************************************************/


// Time constants for LoRaWAN protocol for CLASS A devices (LoRaWAN specification for EU863-870)
// Values are in milliseconds

// Constants for standard RX Windows ('RECEIVE_DELAYx') 
// Note: 
//   Current version does not support device configuration by LoRa MAC commands.
//   It is assumed that devices use the standard timings for RX windows.
//   Even with MAC command configuration, the RX2 windows always starts 1000 ms after RX1 window.
#define LORAREALTIMESENDER_CLASSA_RECEIVE_DELAY1   1000
#define LORAREALTIMESENDER_CLASSA_RECEIVE_DELAY2   (LORAREALTIMESENDER_CLASSA_RECEIVE_DELAY1 + 1000)

// Percentage of receive delay duration allowed to detect downlink packet preamble on device
#define LORAREALTIMESENDER_CLASSA_RX_PREAMBLE_RATIO   90

// Estimated duration of device's receive mode during RX window.
// Note: The LoRaWAN specification requires a minimum duration for reception of the LoRa packet 
//       preamble.
#define LORAREALTIMESENDER_LORAWAN_RX_WINDOW_LENGTH  (((LORAREALTIMESENDER_CLASSA_RECEIVE_DELAY2 - LORAREALTIMESENDER_CLASSA_RECEIVE_DELAY1) *  LORAREALTIMESENDER_CLASSA_RX_PREAMBLE_RATIO) / 100)

// Delay required by gateway ('SenderTask' and transceiver') to start data transmission
#define LORAREALTIMESENDER_GATEWAY_TX_DELAY    100


/********************************************************************************************* 
  Structures 
*********************************************************************************************/



/********************************************************************************************* 
 NodeReceiveWindow Class

 This class maintains the start time of active receive windows for a node:
  - This object is exclusively used by 'LoraRealtimeSender' (private).
    The 'LoraRealtimeSender' maintains these objects in the 'm_pNodeReceiveWindowArray'
    'MemoryBlockArray'.
  - For Class A nodes:
     .. This object is instancied when an 'uplink' LoRa packet is received.
     .. The start times of receive windows are computed and the 'NodeReceiveWindow' is inserted
        in the 'm_pNodeReceiveWindowArray' array.
     .. The object is removed from 'm_pNodeReceiveWindowArray' if a downlink packet is processed
        or when RX2 window is closed (i.e. periodically checked by 'SenderTask').
*********************************************************************************************/

typedef struct _CNodeReceiveWindow
{
  // Node description and access
  BYTE m_usDeviceClass;
  DWORD m_dwDeviceAddr;
  ILoraTransceiver m_pLoraTransceiverItf;

  // Start timestamps for RX windows
  DWORD m_dwRX1WindowTimestamp;
  DWORD m_dwRX2WindowTimestamp;

} CNodeReceiveWindowOb;

typedef struct _CNodeReceiveWindow * CNodeReceiveWindow;

// Class constants and definitions

// LoRa Class for node (i.e. allowed values for 'm_usDeviceClass' variable) 
// Note: Use values defined on 'LoraRealtimeSenderItf' interface
#define NODERECEIVEWINDOW_DEVICECLASS_A     LORAREALTIMESENDER_DEVICECLASS_A
#define NODERECEIVEWINDOW_DEVICECLASS_C     LORAREALTIMESENDER_DEVICECLASS_C


/********************************************************************************************* 
 CRealtimeLoraPacket Class

 This class maintains the schedule parameters for downlink LoRa packet to send:
  - This object is exclusively used by 'LoraRealtimeSender' (private).
  - The 'CRealtimeLoraPacket' object is inserted in the 'm_pRealtimeLoraPacketArray' array of
    'LoraRealtimeSender' object by the 'ScheduleSendNodePacket' method of 'LoraRealtimeSenderItf'
    interface.
  - The 'SenderTask' of 'LoraRealtimeSender' object retrieves the 'CRealtimeLoraPacket' object
    just in time and asks the 'LoraTransceiver' to send it. During this process, the 'SenderTask'
    also removes the object from 'm_pRealtimeLoraPacketArray' array.
*********************************************************************************************/

typedef struct _CRealtimeLoraPacket
{
  // Transceiver to use to send LoRa packet to node
  // Note: In current version, the transceiver settings are statically defined by configuration
  //       (i.e. the gateway does not use radio parameters specified by Network Server with
  //       downlink message)
  ILoraTransceiver m_pLoraTransceiverItf;

  // Identifiers of session defined in parent object for management of the downlink LoRa packet
  // Note: These identifiers are used by 'LoraRealtimeSender' to send notifications to parent object
  DWORD m_dwDownlinkSessionId;
  void *m_pDownlinkSession;

  // Timestamp indicating when LoRa packet must be sent:
  //  - If 'm_bASAP' is true, the 'm_dwSendTimestamp' value is the time limit to send the packet.
  //    Note: The packet with ASAP are sent according to ascending value of 'm_dwSendTimestamp'
  //  - If 'm_bASAP', the 'm_dwSendTimestamp' value is the absolute time value when packet
  //    must be transmitted to transceiver for send.
  bool m_bASAP;
  DWORD m_dwSendTimestamp;

  // Downlink Lora packet to send
  CLoraTransceiverItf_LoraPacket m_pPacketToSend;

} CRealtimeLoraPacketOb;

typedef struct _CRealtimeLoraPacket * CRealtimeLoraPacket;



/********************************************************************************************* 
 LoraRealtimeSender Class
*********************************************************************************************/

// Class data
typedef struct _CLoraRealtimeSender
{
  // Interface
  ILoraRealtimeSender m_pLoraRealtimeSenderItf;

  uint32_t m_nRefCount;      // The 'CLoraRealtimeSender' object is reference counted (number of client 
                             // objects owning one of the public interfaces exposed by 'CLoraRealtimeSender')

  // Object main state
  DWORD m_dwCurrentState;

  // Collection of 'NodeReceiveWindow' currently active
  // Memory block array for 'CNodeReceiveWindowOb' objects
  CMemoryBlockArray m_pNodeReceiveWindowArray;

  // Collection of 'RealtimeLoraPacket' currently waiting for send
  // Memory block array for 'CRealtimeLoraPacketOb' objects
  CMemoryBlockArray m_pRealtimeLoraPacketArray;

  // Next downlink LoRa packet to send
  // Note: A NULL value indicates that no LoRa packet is waiting in queue
  CRealtimeLoraPacket m_pNextRealtimeLoraPacket;

  // Mutex for access to 'm_pRealtimeLoraPacketArray'
  SemaphoreHandle_t m_hPacketArrayMutex;

  // Semaphore for a LoRa packet waiting for send
  SemaphoreHandle_t m_hPacketWaiting;

  //
  // Tasks and associated queues
  //

  // 'PacketSender' task (automaton for sending LoRa packets just in time)
  TaskFunction_t m_hPacketSenderTask;



  // Interface to 'LoraNodeManager' 
  // The 'LoraRealtimeSender' invokes the 'SessionEvent' method to notify the 'LoraNodeManager'
  // during the processing of downlink packet
  ITransceiverManager m_pTransceiverManagerItf;


} CLoraRealtimeSender;

// Class constants and definitions

// Main Automaton (PacketSender task)
// Note: Numbering order MUST match object's life cycle
#define LORAREALTIMESENDER_AUTOMATON_STATE_CREATING      0
#define LORAREALTIMESENDER_AUTOMATON_STATE_CREATED       1
#define LORAREALTIMESENDER_AUTOMATON_STATE_INITIALIZED   2
#define LORAREALTIMESENDER_AUTOMATON_STATE_IDLE          3
#define LORAREALTIMESENDER_AUTOMATON_STATE_RUNNING       4
#define LORAREALTIMESENDER_AUTOMATON_STATE_STOPPING      5
#define LORAREALTIMESENDER_AUTOMATON_STATE_TERMINATED    6
#define LORAREALTIMESENDER_AUTOMATON_STATE_ERROR         7


// Methods for 'ILoraRealtimeSender' interface implementation on 'LoraRealtimeSender' object
// The 'CNerworkServerProtocolItfImpl' structure provided by 'LoraRealtimeSender' object contains pointers to these methods 
uint32_t CLoraRealtimeSender_AddRef(void *this);
uint32_t CLoraRealtimeSender_ReleaseItf(void *this);

bool CLoraRealtimeSender_Initialize(void *this, CLoraRealtimeSenderItf_InitializeParams pParams);
bool CLoraRealtimeSender_Start(void *this, CLoraRealtimeSenderItf_StartParams pParams);
bool CLoraRealtimeSender_Stop(void *this, CLoraRealtimeSenderItf_StopParams pParams);

bool CLoraRealtimeSender_RegisterNodeRxWindows(void *this, CLoraRealtimeSenderItf_RegisterNodeRxWindowsParams pParams);
DWORD CLoraRealtimeSender_ScheduleSendNodePacket(void *this, CLoraRealtimeSenderItf_ScheduleSendNodePacketParams pParams);


// Construction
CLoraRealtimeSender * CLoraRealtimeSender_New();
void CLoraRealtimeSender_Delete(CLoraRealtimeSender *this);

// Task functions
void CLoraRealtimeSender_PacketSenderAutomaton(CLoraRealtimeSender *this);


// Class public methods


// Class private methods (implementation helpers)
CNodeReceiveWindow CLoraRealtimeSender_FindNodeReceiveWindow(CLoraRealtimeSender *this, DWORD dwDeviceAddr, bool bCheckExpired);
CRealtimeLoraPacket CLoraRealtimeSender_GetNextRealtimePacket(CLoraRealtimeSender *this);
void CLoraRealtimeSender_RemoveExpiredNodeReceiveWindows(CLoraRealtimeSender *this);


#endif






