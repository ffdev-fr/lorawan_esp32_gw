/*****************************************************************************************//**
 * @file     LoraTransceiverItf.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     16/02/2018
 *
 * @brief    Class for 'ILoraTransceiver' interface.
 *
 * @details  This class is instanced by objects implementing the interface.\n
 *           Typically, the interface is provided by owner object in return of its static
 *           'CreateInstance' function.
 *           In current implementation, this interface is provided by the CSX1276 object.
 *
 * @note     This class IS THREAD SAFE.\n
 *           The client object must call the 'ReleaseItf' method on 'ILoraTransceiver' interface 
 *           to destroy the implementation object created by 'CreateInstance'.
*********************************************************************************************/

#ifndef LORATRANSCEIVERITF_H
#define LORATRANSCEIVERITF_H


// Forward declaration to interface object (used as this on interface methods)
typedef struct _ILoraTransceiver * ILoraTransceiver;


/********************************************************************************************* 
  Public definitions used by methods of 'ILoraTransceiver' interface
*********************************************************************************************/

/********************************************************************************************* 
  Definitions for LoRa radio configuration
*********************************************************************************************/

// LoRa frequency channels
#define LORATRANSCEIVERITF_FREQUENCY_CHANNEL_10   10     // Channel 10, central frequency = 865.200MHz
#define LORATRANSCEIVERITF_FREQUENCY_CHANNEL_11   11     // Channel 11, central frequency = 865.500MHz
#define LORATRANSCEIVERITF_FREQUENCY_CHANNEL_12   12     // Channel 12, central frequency = 865.800MHz
#define LORATRANSCEIVERITF_FREQUENCY_CHANNEL_13   13     // Channel 13, central frequency = 866.100MHz
#define LORATRANSCEIVERITF_FREQUENCY_CHANNEL_14   14     // Channel 15, central frequency = 866.400MHz
#define LORATRANSCEIVERITF_FREQUENCY_CHANNEL_15   15     // Channel 16, central frequency = 866.700MHz
#define LORATRANSCEIVERITF_FREQUENCY_CHANNEL_16   16     // Channel 17, central frequency = 867.000MHz
#define LORATRANSCEIVERITF_FREQUENCY_CHANNEL_17   17     // Channel 18, central frequency = 868.000MHz
#define LORATRANSCEIVERITF_FREQUENCY_CHANNEL_18   18     // Channel 19, central frequency = 868.100MHz

// EU868 Standard 6 channel frequency plan
#define LORATRANSCEIVERITF_FREQUENCY_CHANNEL_00   0   // channel 0, central freq = 868.100MHz (DR0-5) (SF7) (125 kHz) (default LoRaWAN EU channel)
#define LORATRANSCEIVERITF_FREQUENCY_CHANNEL_01   1   // channel 1, central freq = 868.300MHz (DR0-5) (SF7) (125 kHz) (default LoRaWAN EU channel)
#define LORATRANSCEIVERITF_FREQUENCY_CHANNEL_02   2   // channel 2, central freq = 868.500MHz (DR0-5) (SF7) (125 kHz) (default LoRaWAN EU channel)
#define LORATRANSCEIVERITF_FREQUENCY_CHANNEL_03   3   // channel 3, central freq = 868.850MHz (DR0-5) (SF7) (125 kHz)
#define LORATRANSCEIVERITF_FREQUENCY_CHANNEL_04   4   // channel 4, central freq = 869.050MHz (DR0-5) (SF7) (125 kHz)
#define LORATRANSCEIVERITF_FREQUENCY_CHANNEL_05   5   // channel 5, central freq = 869.525MHz (DR0-5) (SF7) (125 kHz)
#define LORATRANSCEIVERITF_FREQUENCY_RX2          6   // Downlink RX2, central freq = 869.525MHz (DR0) (SF12) (125 kHz) (default LoRaWAN EU RX2)

#define LORATRANSCEIVERITF_FREQUENCY_CHANNEL_NONE 0      // Ignore parameter

// LoRa bandwidth
#define LORATRANSCEIVERITF_BANDWIDTH_7_8     0x00    
#define LORATRANSCEIVERITF_BANDWIDTH_10_4    0x01 
#define LORATRANSCEIVERITF_BANDWIDTH_15_6    0x02 
#define LORATRANSCEIVERITF_BANDWIDTH_20_8    0x03 
#define LORATRANSCEIVERITF_BANDWIDTH_31_25   0x04 
#define LORATRANSCEIVERITF_BANDWIDTH_41_7    0x05 
#define LORATRANSCEIVERITF_BANDWIDTH_62_5    0x06 
#define LORATRANSCEIVERITF_BANDWIDTH_125     0x07 
#define LORATRANSCEIVERITF_BANDWIDTH_250     0x08 
#define LORATRANSCEIVERITF_BANDWIDTH_500     0x09 

#define LORATRANSCEIVERITF_BANDWIDTH_NONE    0x00      // Ignore parameter

// LoRa Coding Rate
#define LORATRANSCEIVERITF_CR_5     0x01    // CR = 4/5
#define LORATRANSCEIVERITF_CR_6     0x02    // CR = 4/6
#define LORATRANSCEIVERITF_CR_7     0x03    // CR = 4/7
#define LORATRANSCEIVERITF_CR_8     0x04    // CR = 4/8

#define LORATRANSCEIVERITF_CR_NONE  0x00    // Ignore parameter

// LoRa Spreading Factor
#define LORATRANSCEIVERITF_SF_6     0x06
#define LORATRANSCEIVERITF_SF_7     0x07
#define LORATRANSCEIVERITF_SF_8     0x08
#define LORATRANSCEIVERITF_SF_9     0x09
#define LORATRANSCEIVERITF_SF_10    0x0A
#define LORATRANSCEIVERITF_SF_11    0x0B
#define LORATRANSCEIVERITF_SF_12    0x0C

#define LORATRANSCEIVERITF_SF_NONE  0x00    // Ignore parameter

// LoRa mode
#define LORATRANSCEIVERITF_LORAMODE_1     0x01      // CR = 4/5, SF = 12, BW = 125 KHz
#define LORATRANSCEIVERITF_LORAMODE_2     0x02      // CR = 4/5, SF = 12, BW = 250 KHz
#define LORATRANSCEIVERITF_LORAMODE_3     0x03      // CR = 4/5, SF = 10, BW = 11250 KHz
#define LORATRANSCEIVERITF_LORAMODE_4     0x04      // CR = 4/5, SF = 12, BW = 500 KHz 
#define LORATRANSCEIVERITF_LORAMODE_5     0x05      // CR = 4/5, SF = 10, BW = 250 KHz 
#define LORATRANSCEIVERITF_LORAMODE_6     0x06      // CR = 4/5, SF = 11, BW = 500 KHz 
#define LORATRANSCEIVERITF_LORAMODE_7     0x07      // CR = 4/5, SF = 9,  BW = 250 KHz 
#define LORATRANSCEIVERITF_LORAMODE_8     0x08      // CR = 4/5, SF = 9,  BW = 500 KHz 
#define LORATRANSCEIVERITF_LORAMODE_9     0x09      // CR = 4/5, SF = 8,  BW = 500 KHz 
#define LORATRANSCEIVERITF_LORAMODE_10    0x10      // CR = 4/5, SF = 7,  BW = 500 KHz 
#define LORATRANSCEIVERITF_LORAMODE_11    0x11      // CR = 4/5, SF = 12, BW = 125 KHz 

#define LORATRANSCEIVERITF_LORAMODE_NONE  0x00      // Ignore parameter

// LoRa MAC Synchronization Word
#define LORATRANSCEIVERITF_SYNCWORD_PUBLIC       0x34    // Standard value for public networks
#define LORATRANSCEIVERITF_SYNCWORD_PRIVATE      0x12

#define LORATRANSCEIVERITF_SYNCWORD_NONE         0x00    // Ignore parameter

// LoRa MAC Preamble length
#define LORATRANSCEIVERITF_PREAMBLE_LENGTH_LORA  0x08    // Standard value for LoRa networks

#define LORATRANSCEIVERITF_PREAMBLE_LENGTH_NONE  0xFF    // Ignore parameter

// LoRa MAC Header
#define LORATRANSCEIVERITF_HEADER_OFF            0x00
#define LORATRANSCEIVERITF_HEADER_ON             0x01

#define LORATRANSCEIVERITF_HEADER_NONE           0xFF    // Ignore parameter

// LoRa MAC CRC
#define LORATRANSCEIVERITF_CRC_OFF               0x00
#define LORATRANSCEIVERITF_CRC_ON                0x01

#define LORATRANSCEIVERITF_CRC_NONE              0xFF    // Ignore parameter

// LoRa radio Power Mode
#define LORATRANSCEIVERITF_POWER_MODE_LOW        0x01    // 2dBm (max. current 100mA)
#define LORATRANSCEIVERITF_POWER_MODE_HIGH       0x02    // 6dBm (max. current 100mA)
#define LORATRANSCEIVERITF_POWER_MODE_MAX        0x03    // 14dBm (max. current 100mA)
#define LORATRANSCEIVERITF_POWER_MODE_BOOST      0x04    // 14dBm + PA_BOOST (max. current 130mA)
#define LORATRANSCEIVERITF_POWER_MODE_BOOST2     0x05    // 20dBm + PA_BOOST (max. current 150mA)

#define LORATRANSCEIVERITF_POWER_MODE_NONE       0x00    // Ignore parameter


// LoRa radio Power Level (values 0 to 14dBm)
#define LORATRANSCEIVERITF_POWER_LEVEL_NONE      0xFF    // Ignore parameter

// Output Current Protection
#define LORATRANSCEIVERITF_OCP_MAX               27      // Maximum value for Over Current Protection (27 = 240 mA)
#define LORATRANSCEIVERITF_OCP_NONE              255     // Ignore parameter


// Maximum values
#define LORATRANSCEIVERITF_MAX_SEND_RETRIES      0x03    // Maximum number of 'Send' retries


/********************************************************************************************* 
  Objects and definitions used for event notifications
*********************************************************************************************/

// The event message (i.e. posted to event queue of owner object)
typedef struct _CLoraTransceiverItf_Event
{
  // Public
  WORD m_wEventType;                              // The event
  ILoraTransceiver m_pLoraTransceiverItf;         // Interface to object which generate the event
  void * m_pEventData;                            // Depends on event type
} CLoraTransceiverItf_EventOb;

typedef struct _CLoraTransceiverItf_Event * CLoraTransceiverItf_Event;


// Event sent to indicate that a new received packet is ready in object implementing the interface
// The 'm_pEventData' member of 'CLoraTransceiverItf_EventOb' is a 'CLoraTransceiverItf_LoraPacket'
// (i.e. pointer to received packet)
#define LORATRANSCEIVERITF_EVENT_PACKETRECEIVED      0x0001

// Event sent to indicate that object implementing the interface have terminated a send operation
// The 'm_pEventData' member of 'CLoraTransceiverItf_EventOb' is a 'CLoraTransceiverItf_LoraPacket'
// (i.e. pointer to sent packet)
#define LORATRANSCEIVERITF_EVENT_PACKETSENT          0x0002


/********************************************************************************************* 
  Result codes returned by methods of 'ILoraTransceiver' interface
*********************************************************************************************/

#define LORATRANSCEIVERITF_RESULT_SUCCESS        0x01              // Successfully executed
#define LORATRANSCEIVERITF_RESULT_ERROR          0x02              // An error has occurred
#define LORATRANSCEIVERITF_RESULT_INVALIDSTATE   0x03              // Function call not allowed on current state
#define LORATRANSCEIVERITF_RESULT_TIMEOUT        0x04              // Timeout when waiting for event
#define LORATRANSCEIVERITF_RESULT_NOTEXECUTED    0x05              // Failure without error information
#define LORATRANSCEIVERITF_RESULT_INVALIDPARAMS  0x06              // Invamid parameters specified
                                                 


/********************************************************************************************* 
  Objects used as parameter on methods of 'ILoraTransceiver' interface
*********************************************************************************************/

// Forward declarations (objects used to exchange data on interface)
typedef struct _CLoraTransceiverItf_InitializeParams * CLoraTransceiverItf_InitializeParams;
typedef struct _CLoraTransceiverItf_SetLoraMACParams * CLoraTransceiverItf_SetLoraMACParams;
typedef struct _CLoraTransceiverItf_SetLoraModeParams * CLoraTransceiverItf_SetLoraModeParams;
typedef struct _CLoraTransceiverItf_SetPowerModeParams * CLoraTransceiverItf_SetPowerModeParams;
typedef struct _CLoraTransceiverItf_SetFreqChannelParams * CLoraTransceiverItf_SetFreqChannelParams;
typedef struct _CLoraTransceiverItf_StandByParams * CLoraTransceiverItf_StandByParams;
typedef struct _CLoraTransceiverItf_ReceiveParams * CLoraTransceiverItf_ReceiveParams;
typedef struct _CLoraTransceiverItf_SendParams * CLoraTransceiverItf_SendParams;
typedef struct _CLoraTransceiverItf_GetReceivedPacketInfoParams * CLoraTransceiverItf_GetReceivedPacketInfoParams;

typedef struct _CLoraTransceiverItf_LoraPacket * CLoraTransceiverItf_LoraPacket;
typedef struct _CLoraTransceiverItf_ReceivedLoraPacketInfo * CLoraTransceiverItf_ReceivedLoraPacketInfo;

typedef struct _CLoraTransceiverItf_InitializeParams
{
  // Public
  QueueHandle_t m_hEventNotifyQueue;
  CLoraTransceiverItf_SetLoraMACParams pLoraMAC;
  CLoraTransceiverItf_SetLoraModeParams pLoraMode;
  CLoraTransceiverItf_SetPowerModeParams pPowerMode;
  CLoraTransceiverItf_SetFreqChannelParams pFreqChannel;
} CLoraTransceiverItf_InitializeParamsOb;


typedef struct _CLoraTransceiverItf_SetLoraMACParams
{
  // Public
  WORD m_wPreambleLength;
  BYTE m_usSyncWord;
  BYTE m_usHeader;
  BYTE m_usCRC;
  bool m_bForce;
} CLoraTransceiverItf_SetLoraMACParamsOb;


typedef struct _CLoraTransceiverItf_SetLoraModeParams
{
  // Public
  BYTE m_usLoraMode;
  BYTE m_usCodingRate;
  BYTE m_usSpreadingFactor;
  BYTE m_usBandwidth;
  bool m_bForce;
} CLoraTransceiverItf_SetLoraModeParamsOb;


typedef struct _CLoraTransceiverItf_SetPowerModeParams
{
  // Public
  BYTE m_usPowerMode;
  BYTE m_usPowerLevel;
  BYTE m_usOcpRate;
  bool m_bForce;
} CLoraTransceiverItf_SetPowerModeParamsOb;


typedef struct _CLoraTransceiverItf_SetFreqChannelParams
{
  // Public
  BYTE m_usFreqChannel;
  bool m_bForce;
} CLoraTransceiverItf_SetFreqChannelParamsOb;


typedef struct _CLoraTransceiverItf_StandByParams
{
  // Public
  bool m_bForce;
} CLoraTransceiverItf_StandByParamsOb;


typedef struct _CLoraTransceiverItf_ReceiveParams
{
  // Public
  bool m_bForce;
} CLoraTransceiverItf_ReceiveParamsOb;


typedef struct _CLoraTransceiverItf_SendParams
{
  // Public
  CLoraTransceiverItf_LoraPacket m_pPacketToSend;
} CLoraTransceiverItf_SendParamsOb;


typedef struct _CLoraTransceiverItf_GetReceivedPacketInfoParams
{
  // Public
  CLoraTransceiverItf_ReceivedLoraPacketInfo m_pPacketInfo;
} CLoraTransceiverItf_GetReceivedPacketInfoParamsOb;


// The implementation object MUST instanciate object with identical structure in order
// to exchange packets.
// In other words, the interface uses a shared 'CLoraTransceiverItf_LoraPacketOb' to
// implement a 'writer/reader' pattern (synchronized access).
typedef struct _CLoraTransceiverItf_LoraPacket
{
  // System tick count for LoRaWAN protocol time rules (i.e. uplink RX windows)
  // Received packet = when packet is received
  // Packet to send  = when sending packet bytes begins
  DWORD m_dwTimestamp;

  // Packet payload size (= size of 'm_usData' array)
  // Note: This member variable is used as synchronization flag for packet transmission between
  //       writer and reader objects.
  //       Due to this concurrent access to 'm_dwDataSize', the type of this variable MUST be
  //       DWORD (i.e. size for atomic access by ESP32 = no MUTEX required)
  //       - For received packets : the reader object must set this variable with 0 to allow the
  //         writer object to store a new received packet (i.e. read confirmation)
  //  
  DWORD m_dwDataSize;

  // Packet payload
  // Note: This member variable must be at the end of structure
  BYTE m_usData[];
} CLoraTransceiverItf_LoraPacketOb;


// Meta data associated to last received LoraPacket in text format
// 
// IMPORTANT: This structure MUST be identical to 'CReceivedLoraPacketInfo'
//            (i.e. direct memcpy to client buffer for optimization)
// 
// Note: The memory block is owned by the calling object
typedef struct _CLoraTransceiverItf_ReceivedLoraPacketInfo
{
  // UTC timestamp seconds and micro seconds (UNIX epoch)
  DWORD m_dwUTCSec;
  DWORD m_dwUTCMicroSec;

  // RX central frequency in MHz (unsigned float, Hz precision)
  BYTE m_szFrequency[8];

  // LoRa datarate identifier (eg. SF12BW500) 
  BYTE m_szDataRate[10];

  // LoRa coding rate identifier (eg. 4/5) 
  BYTE m_szCodingRate[4];

  // Lora SNR ratio in dB (signed float, 0.1 dB precision)
  BYTE m_szSNR[7];

  // RSSI in dBm (signed integer, 1 dB precision)
  BYTE m_szRSSI[5];
} CLoraTransceiverItf_ReceivedLoraPacketInfoOb;



/********************************************************************************************* 
  Public methods of 'ILoraTransceiver' interface
 
  These methods are available to any object which has obtained a reference to the
  'ILoraTransceiver' interface (i.e. this reference is required with all interface methods)
*********************************************************************************************/

uint32_t ILoraTransceiver_AddRef(ILoraTransceiver this);
uint32_t ILoraTransceiver_ReleaseItf(ILoraTransceiver this);

bool ILoraTransceiver_Initialize(ILoraTransceiver this, CLoraTransceiverItf_InitializeParams pParams);
bool ILoraTransceiver_SetLoraMAC(ILoraTransceiver this, CLoraTransceiverItf_SetLoraMACParams pParams);
bool ILoraTransceiver_SetLoraMode(ILoraTransceiver this, CLoraTransceiverItf_SetLoraModeParams pParams);
bool ILoraTransceiver_SetPowerMode(ILoraTransceiver this, CLoraTransceiverItf_SetPowerModeParams pParams);
bool ILoraTransceiver_SetFreqChannel(ILoraTransceiver this, CLoraTransceiverItf_SetFreqChannelParams pParams);

bool ILoraTransceiver_StandBy(ILoraTransceiver this, CLoraTransceiverItf_StandByParams pParams);
bool ILoraTransceiver_Receive(ILoraTransceiver this, CLoraTransceiverItf_ReceiveParams pParams);
bool ILoraTransceiver_Send(ILoraTransceiver this, CLoraTransceiverItf_SendParams pParams);

bool ILoraTransceiver_GetReceivedPacketInfo(ILoraTransceiver this, CLoraTransceiverItf_GetReceivedPacketInfoParams pParams);

#ifdef LORATRANSCEIVERITF_IMPL

/********************************************************************************************* 
  Classes and definition used by objects which expose the 'ILoraTransceiver' interface and
  by the interface implementation itself
 
  Note: The implementation file of these objects must define 'LORATRANSCEIVERITF_IMPL'
        (i.e. in order to include the following block of code) 
*********************************************************************************************/

#include "LoraTransceiverItfImpl.h"

/********************************************************************************************* 
  Forward declarations
*********************************************************************************************/
           

/********************************************************************************************* 
  Interface class instanced by objects implementing the interface methods                                                              .
                                                                                                                                        .
  This object is instanced by 'ILoraTransceiver_New' method (protected constructor)                                                                           .
*********************************************************************************************/

// Interface class data
typedef struct _ILoraTransceiver
{
  // Private attributes
  void *m_pOwnerObject;                        // Instance of the object exposing the interface
                                               // This pointer is provided as argument on interface methods
  CLoraTransceiverItfImpl m_pOwnerItfImpl;     // Pointers to implementation functions on owner object
} ILoraTransceiverOb;



/********************************************************************************************* 
  Protected methods used by objects implementing the 'ILoraTransceiver' interface
*********************************************************************************************/
 
ILoraTransceiver ILoraTransceiver_New(void *pOwnerObject, CLoraTransceiverItfImpl pOwnerItfImpl);
void ILoraTransceiver_Delete(ILoraTransceiver this);


#endif
#endif

