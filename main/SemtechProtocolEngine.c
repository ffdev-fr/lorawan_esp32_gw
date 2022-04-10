/*****************************************************************************************//**
 * @file     SemtechProtocolEngine.c
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     30/03/2018
 *
 * @brief    Encodes and decodes messages exchanged with a Network Server using the Semtech
 *           protocol.
 *
 * @details  The Semtech protocol is the legacy protocol used on many LoRaWAN network.
 *           The main functions are for encoding and decoding LoRaWAN frames ('CLoraPacket')
 *           to/from proprietary messages used by the Semtech Network Server protocol.
 *           The "Semtech Network Server Protocol" uses JSON messages. In JSON message, the
 *           the variable for LoRaWan packet contains packet's bytes encoded in Base64.
 *
 * @note     The "LoRaWAN ESP32 Gateway V1.x" project is designed for execution on ESP32 
 *           Module (Dev.C kit).\n
 *           The implementation uses the Espressif IDF V3.0 framework (with RTOS)              
*********************************************************************************************/


/*********************************************************************************************
  Espressif framework includes
*********************************************************************************************/

#include <Common.h>

/*********************************************************************************************
  Includes for object implementation
*********************************************************************************************/

// The CSemtechProtocolEngine object implements the 'INetworkServerProtocol' interface
#define NETWORKSERVERPROTOCOLITF_IMPL

#include "NetworkServerProtocolItf.h"

// Object's definitions and methods
#include "SemtechProtocolEngine.h"
         
// The settings for Semtech protocol behavior defined the global configuration file
#define SEMTECHPROTOCOLENGINE_IMPL
#include "Configuration.h"

    
/*********************************************************************************************
  Instantiate global static objects used by module implementation
*********************************************************************************************/

// 'INetworkServerProtocol' interface function pointers
struct _CNetworkServerProtocolItfImpl g_NetworkServerProtocolItfImplOb = { .m_pAddRef = CSemtechProtocolEngine_AddRef,
                                                                           .m_pReleaseItf = CSemtechProtocolEngine_ReleaseItf,
                                                                           .m_pBuildUplinkMessage = CSemtechProtocolEngine_BuildUplinkMessage,
                                                                           .m_pProcessServerMessage = CSemtechProtocolEngine_ProcessServerMessage,
                                                                           .m_pProcessSessionEvent = CSemtechProtocolEngine_ProcessSessionEvent
                                                                         };



/********************************************************************************************* 

 CSemtechProtocolEngine Class

*********************************************************************************************/



/********************************************************************************************* 
  Public methods of CSemtechProtocolEngine object
 
  These methods are exposed on object's public interfaces
*********************************************************************************************/

/*********************************************************************************************
  Object instance factory
 
  The factory contains one method used to create a new object instance.
  This method provides the 'INetworkServerProtocol' interface object for object's use and destruction.
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         INetworkServerProtocol CSemtechProtocolEngine_CreateInstance()
 * 
 * @brief      Creates a new instance of CSemtechProtocolEngine object.
 * 
 * @details    A new instance of CSemtechProtocolEngine object is created and its 
 *             'INetworkServerProtocol' interface is returned. The owner object invokes methods
 *             of this interface encode and decode messages exchanged with a Network Server 
 *             using the Semtech protocol.
 *
 * @return     A 'INetworkServerProtocol' interface object.\n
 *             The reference count for returned 'INetworkServerProtocol' interface is set to 1.
 *
 * @note       The CSemtechProtocolEngine object is destroyed when the last reference to 
 *             'INetworkServerProtocol' is released (i.e. call to 'INetworkServerProtocol_ReleaseItf' 
 *             method).
*********************************************************************************************/
INetworkServerProtocol CSemtechProtocolEngine_CreateInstance()
{
  CSemtechProtocolEngine * pSemtechProtocolEngine;
     
  // Create the object
  if ((pSemtechProtocolEngine = CSemtechProtocolEngine_New()) != NULL)
  {
    // Create the 'INetworkServerProtocol' interface object
    if ((pSemtechProtocolEngine->m_pNetworkServerProtocolItf =
        INetworkServerProtocol_New(pSemtechProtocolEngine, &g_NetworkServerProtocolItfImplOb)) != NULL)
    {
      ++(pSemtechProtocolEngine->m_nRefCount);
    }
    return pSemtechProtocolEngine->m_pNetworkServerProtocolItf;
  }

  return NULL;
}

/*********************************************************************************************
  Public methods exposed on 'INetworkServerProtocol' interface
 
  The static 'CSemtechProtocolEngineItfImplOb' object is initialized with pointers to these functions.
  The static 'CSemtechProtocolEngineItfImplOb' object is referenced in the 'INetworkServerProtocol'
  interface provided by 'CreateInstance' method (object factory).
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         uint32_t CSemtechProtocolEngine_AddRef(void *this)
 * 
 * @brief      Increments the object's reference count.
 * 
 * @details    This function increments object's global reference count.\n
 *             The reference count is used to track the number of existing external references
 *             to 'INetworkServerProtocol' interface implemented by CSemtechProtocolEngine object.
 * 
 * @param      this
 *             The pointer to CSemtechProtocolEngine object.
 *  
 * @return     The value of reference count once incremented.
*********************************************************************************************/
uint32_t CSemtechProtocolEngine_AddRef(void *this)
{
  return ++((CSemtechProtocolEngine *)this)->m_nRefCount;
}

/*****************************************************************************************//**
 * @fn         uint32_t CSemtechProtocolEngine_ReleaseItf(void *this)
 * 
 * @brief      Decrements the object's reference count.
 * 
 * @details    This function decrements object's global reference count and destroy the object
 *             when count reaches 0.\n
 *             The reference count is used to track the number of existing external references
 *             to 'INetworkServerProtocol' interface implemented by CSemtechProtocolEngine object.
 * 
 * @param      this
 *             The pointer to CSemtechProtocolEngine object.
 *  
 * @return     The value of reference count once decremented.
*********************************************************************************************/
uint32_t CSemtechProtocolEngine_ReleaseItf(void *this)
{
  // Delete the object if its interface reference count reaches zero
  if (((CSemtechProtocolEngine *)this)->m_nRefCount == 1)
  {
    CSemtechProtocolEngine_Delete((CSemtechProtocolEngine *)this);
    return 0;
  }
  return --((CSemtechProtocolEngine *)this)->m_nRefCount;
}

/*****************************************************************************************//**
 * @fn         bool CSemtechProtocolEngine_BuildUplinkMessage(void *this, 
 *                                  CNetworkServerProtocolItf_BuildUplinkMessageParams pParams)
 * 
 * @brief      Builds an uplink message for LoRa data or heartbeat.
 * 
 * @details    This function is called like follows:\n
 *              - The 'ServerManager' invokes the function when it receives a Lora packet from
 *                Node. The function generate a PUSH_DATA message containing the LoRa data.\n
 *              - The 'ServerManager' invokes periodically this function to check if a protocol
 *                uplink message must be sent. The function may generate a PUSH_DATA STAT message
 *                or a PULL_DATA message according to configurated periods (i.e. the period
 *                management if implemented in the 'ProtocolEngine' = owner object simply calls
 *                periodically using a comptible frequency).
 * 
 * @param      this
 *             The pointer to CSemtechProtocolEngine object.
 *  
 * @return     The 'true' value is returned if a message is prepared and must be sent by the
 *             caller object.
 *
 * @note       The message is generated in a buffer provided (and owned) by the caller object.
 *
 * @note       If the message must be sent by the caller object, the caller object must notify
 *             the CSemtechProtocolEngine object for the result of the send operation (see
 *             'INetworkServerProtocol_ProcessSessionEvent' for details).
*********************************************************************************************/
bool CSemtechProtocolEngine_BuildUplinkMessage(void *this, 
                                               CNetworkServerProtocolItf_BuildUplinkMessageParams pParams)
{
  CMemoryBlockArrayEntryOb MemBlockArrayEntry;
  CSemtechMessageTransaction pMessageTransaction;
  BYTE *pStreamHead;
  BYTE pTempBuffer[40];
  WORD wLength;
  struct tm *tmTime;
  CLoraTransceiverItf_ReceivedLoraPacketInfo pPacketInfo;
  TickType_t dwCurrentTicks;
  DWORD dwElapsedTicks;
  WORD wSemtechMsgType;           // SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_PUSH_DATA
                                  // or SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_PULL_DATA

  // Step 1: Obtain a memory block for the 'CSemtechMessageTransactionOb' object
  if ((pMessageTransaction = (CSemtechMessageTransaction) CMemoryBlockArray_GetBlock
      (((CSemtechProtocolEngine *)this)->m_pTransactionArray, &MemBlockArrayEntry)) == NULL)
  {
    // Should never occur. Buffer for 'CSemtechMessageTransaction' exhausted
    // Note: No recovery mechanism = for stress test in current version
    #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CSemtechProtocolEngine_BuildUplinkMessage- buffer exhausted. Packet discarded");
    #endif
    return false;
  }

  dwCurrentTicks = xTaskGetTickCount();
  
  // For 'heartbeat' message, check if period is elapsed
  if (pParams->m_wMessageType == NETWORKSERVERPROTOCOL_UPLINKMSG_HEARTBEAT)
  {
    if (pParams->m_bForceHeartbeat == false)
    {
      dwElapsedTicks = CSemtechProtocolEngine_GetElapsedTicks(dwCurrentTicks, ((CSemtechProtocolEngine *)this)->m_dwLastPushDataTicks);

      // Check if the 'STAT' PUSH_DATA message is required
      if (dwElapsedTicks < pdMS_TO_TICKS(CONFIG_SEMTECH_PUSHSTAT_PERIOD))
      {
        // The PUSH_DATA message not required this time, check for PULL_DATA message
        dwElapsedTicks = CSemtechProtocolEngine_GetElapsedTicks(dwCurrentTicks, ((CSemtechProtocolEngine *)this)->m_dwLastPullDataTicks);
        if (dwElapsedTicks < pdMS_TO_TICKS(CONFIG_SEMTECH_PULLDATA_PERIOD))
        {
          #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
            DEBUG_PRINT_LN("[DEBUG] CSemtechProtocolEngine_BuildUplinkMessage - Heartbeat not required");
            DEBUG_PRINT("[DEBUG] CSemtechProtocolEngine_BuildUplinkMessage - Number of pending uplink transactions for LoRa messages: ");
            DEBUG_PRINT_DEC(((CSemtechProtocolEngine *)this)->m_wPendingUpTransactionCount);
            DEBUG_PRINT_CR;
          #endif
          CMemoryBlockArray_ReleaseBlock(((CSemtechProtocolEngine *)this)->m_pTransactionArray, pMessageTransaction->m_usTransactionId);
          return false;
        }
        // Generate a PULL_DATA message
        wSemtechMsgType = SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_PULL_DATA;
        ((CSemtechProtocolEngine *)this)->m_dwLastPullDataTicks = dwCurrentTicks; 
  
        #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
          DEBUG_PRINT_LN("[DEBUG] CSemtechProtocolEngine_BuildUplinkMessage - Building PULL_DATA Heartbeat message");
        #endif
      }
      else
      {
        // Generate a PUSH_DATA message (STAT message)
        wSemtechMsgType = SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_PUSH_DATA;
        ((CSemtechProtocolEngine *)this)->m_dwLastPushDataTicks = dwCurrentTicks; 
  
        #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
          DEBUG_PRINT_LN("[DEBUG] CSemtechProtocolEngine_BuildUplinkMessage - Building STAT Heartbeat message (period)");
        #endif
      }
    }
    else
    {
      // Generate a 'forced' PUSH_DATA message (STAT message)
      wSemtechMsgType = SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_PUSH_DATA;

      #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
        DEBUG_PRINT_LN("[DEBUG] CSemtechProtocolEngine_BuildUplinkMessage - Building STAT Heartbeat message (forced)");
      #endif
    }
  }
  else
  {
    // Generate a PUSH_DATA message (LoRa packet message)
    wSemtechMsgType = SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_PUSH_DATA;

    // Update counters for LoRa packets received from nodes
    ++((CSemtechProtocolEngine *)this)->m_dwRxnbCount;
    ++((CSemtechProtocolEngine *)this)->m_dwRxokCount;

    #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] CSemtechProtocolEngine_BuildUplinkMessage - Building PUSH_DATA message for LoRa packet");
    #endif
  }

  #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CSemtechProtocolEngine_BuildUplinkMessage - Starting to build message, ticks: ");
    DEBUG_PRINT_DEC(dwCurrentTicks);
    DEBUG_PRINT_CR;
  #endif

  // The identifier of transaction is the entry index in MemoryBlockArray
  pMessageTransaction->m_usTransactionId = MemBlockArrayEntry.m_usBlockIndex;

  // Step 2: Obtain a random identifier for the Semtech message 
  //
  // A 16 bit value used by Network Server to identify the message in ACK reply (see Semtech protocol) 
  pMessageTransaction->m_wMessageId = CSemtechProtocolEngine_GetNewMessageId(((CSemtechProtocolEngine *)this),
                                                                             pMessageTransaction->m_usTransactionId);
  pMessageTransaction->m_wMessageType = wSemtechMsgType; 
  pMessageTransaction->m_usTransactionType = 
    wSemtechMsgType == SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_PULL_DATA ? SEMTECHMESSAGETRANSACTION_TYPE_PULLDATA : 
                                                                         SEMTECHMESSAGETRANSACTION_TYPE_PUSHDATA;

  pMessageTransaction->m_bHeartbeat = pParams->m_wMessageType == NETWORKSERVERPROTOCOL_UPLINKMSG_HEARTBEAT ? true : false;
  pMessageTransaction->m_dwLastEventTicks = pMessageTransaction->m_dwTransactionStartTicks = dwCurrentTicks;
  pMessageTransaction->m_wTransactionState = SEMTECHPROTOCOLENGINE_TRANSACTION_STATE_SENDING;

  // The message identifer is returned for later use when calling 'INetworkServerProtocol_ProcessSessionEvent'
  // event notification method
  // Note: Identifier contains ids for both 'ServerManager' and 'ProtovolEngine'
  pParams->m_dwProtocolMessageId = pMessageTransaction->m_dwProtocolMessageId = 
    (((DWORD) pParams->m_wServerManagerMessageId) << 16) | pMessageTransaction->m_wMessageId;
   
  #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CSemtechProtocolEngine_BuildUplinkMessage - Transaction created: ");
    DEBUG_PRINT_HEX(pMessageTransaction);
    DEBUG_PRINT(", m_dwProtocolMessageId: ");
    DEBUG_PRINT_HEX(pMessageTransaction->m_dwProtocolMessageId);
    DEBUG_PRINT(", m_wMessageType: ");
    DEBUG_PRINT_HEX(pMessageTransaction->m_wMessageType);
    DEBUG_PRINT(", m_wTransactionState: ");
    DEBUG_PRINT_HEX(pMessageTransaction->m_wTransactionState);
    DEBUG_PRINT(", m_usTransactionType: ");
    DEBUG_PRINT_HEX(pMessageTransaction->m_usTransactionType);
    DEBUG_PRINT(", (Param) m_wServerManagerMessageId: ");
    DEBUG_PRINT_HEX(pParams->m_wServerManagerMessageId);
    DEBUG_PRINT_CR;
  #endif


  // Step 3: Build the message header (i.e. PUSH_DATA or PULL_DATA message)
  // 
  // The format is:
  //   - Byte  0      = protocol version (= 2)
  //   - Bytes 1-2    = random token (the message Id)
  //   - Byte  3      = PUSH_DATA identifier (0x00) or PULL_DATA identifier (0x02)
  //   - Bytes 4-11   = gateway unique identifier (MAC address with Gateway token)
  //   - Bytes 12-end = JSON object, starting with {, ending with } 
  //                    The object can be 'rxpk' (= LoRa data) or 'stat' (keepalive heartbeat = gateway status)

  // Step 3.1 - Message header (bytes 0-11)

  // Note: The message is generated in a memory block owned by the client object (block pointer
  //       specified in 'CNetworkServerProtocolItf_BuildUplinkMessageParams')
  pStreamHead = pParams->m_pMessageData;

  *(pStreamHead++) = SEMTECHPROTOCOLENGINE_SEMTECH_PROTOCOL_VERSION;
  *((WORD*) pStreamHead) = pMessageTransaction->m_wMessageId;
  pStreamHead += 2;
  *(pStreamHead++) = wSemtechMsgType;
  memcpy(pStreamHead, ((CSemtechProtocolEngine *) this)->m_GatewayMACAddr, 8);
  pStreamHead += 8;

  // Step 3.2 - Message JSON stream for object to send (bytes 12-)

  if (pParams->m_wMessageType == NETWORKSERVERPROTOCOL_UPLINKMSG_LORADATA)
  {
    // Generate the 'rxpk' object (i.e. using LoRa packet specified in parameters)
    pPacketInfo = pParams->m_pLoraPacketInfo;
  
    // The 'rxpk' object is a JSON array (i.e. one entry per LoRa packet to transmit)
    // Note: In this implementation, the Semtech message only contains one LoRa packet 
    memcpy(pStreamHead, (void *)"{\"rxpk\":[{", 10);
    pStreamHead += 10;
  
    #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[WARNING] CSemtechProtocolEngine_BuildUplinkMessage- TO DO: improve timestamp management");
    #endif
  
    // RAW timestamp, 8-17 useful chars
    // Internal timestamp of "RX finished" event (32bit unsigned)
    // REM: Milliseconds calculated using System Tick count (i.e. rotates in about 23 days)
  
    // TO DO = check if snprint available (= best efficiency)
  
    sprintf((char*) pTempBuffer, "\"tmst\":%u", pParams->m_pLoraPacket->m_dwTimestamp);
    memcpy(pStreamHead, pTempBuffer, wLength = strlen((char*) pTempBuffer));
    pStreamHead += wLength;
  
    // Packet RX time 
    // UTC time of pkt RX, microsecond precision, ISO 8601 'compact' format (37 useful chars)
  
    // Split the UNIX timestamp to its calendar components
    time_t timePacket = pPacketInfo->m_dwUTCSec;
    tmTime = gmtime(&timePacket);
    sprintf((char*) pTempBuffer, ",\"time\":\"%04i-%02i-%02iT%02i:%02i:%02i.%06liZ\"", (tmTime->tm_year) + 1900,
            (tmTime->tm_mon) + 1, tmTime->tm_mday, tmTime->tm_hour, tmTime->tm_min, tmTime->tm_sec, 
            (long int) pPacketInfo->m_dwUTCMicroSec);
    memcpy(pStreamHead, pTempBuffer, wLength = strlen((char*) pTempBuffer));
    pStreamHead += wLength;
  
    // RX central frequency in MHz (unsigned float, Hz precision)
    memcpy(pStreamHead, ",\"freq\":", 8);
    pStreamHead += 8;
    memcpy(pStreamHead, pPacketInfo->m_szFrequency, wLength = strlen((char*) pPacketInfo->m_szFrequency));
    pStreamHead += wLength;
  
    // Packet modulation, 13-14 useful chars 
    memcpy(pStreamHead, (void *) ",\"modu\":\"LORA\"", 14);
    pStreamHead += 14;
  
    // Lora datarate and bandwidth, 16-19 useful chars
    // LoRa datarate identifier (eg. SF12BW500) 
    memcpy(pStreamHead, ",\"datr\":\"", 9);
    pStreamHead += 9;
    memcpy(pStreamHead, pPacketInfo->m_szDataRate, wLength = strlen((char*) pPacketInfo->m_szDataRate));
    pStreamHead += wLength;
  
    // Lora coding rate, 13 useful chars
    // LoRa coding rate identifier (eg. 4/5) 
    memcpy(pStreamHead, "\",\"codr\":\"", 10);
    pStreamHead += 10;
    memcpy(pStreamHead, pPacketInfo->m_szCodingRate, wLength = strlen((char*) pPacketInfo->m_szCodingRate));
    pStreamHead += wLength;

    // Lora SNR, 11-13 useful chars 
    // Lora SNR ratio in dB (signed float, 0.1 dB precision)
    memcpy(pStreamHead, "\",\"lsnr\":", 9);
    pStreamHead += 9;
    memcpy(pStreamHead, pPacketInfo->m_szSNR, wLength = strlen((char*) pPacketInfo->m_szSNR));
    pStreamHead += wLength;
  
    // Packet RSSI and payload size, 18-23 useful chars
    //  - RSSI in dBm (signed integer, 1 dB precision)
    //  - RF packet payload size in bytes (unsigned integer)
    memcpy(pStreamHead, ",\"rssi\":", 8);
    pStreamHead += 8;
    memcpy(pStreamHead, pPacketInfo->m_szRSSI, wLength = strlen((char*) pPacketInfo->m_szRSSI));
    pStreamHead += wLength;
  
    sprintf((char*) pTempBuffer, ",\"size\":%u", pParams->m_pLoraPacket->m_dwDataSize);
    memcpy(pStreamHead, pTempBuffer, wLength = strlen((char*) pTempBuffer));
    pStreamHead += wLength;
  
    // NOTE: The following fields are required by the specification.
    //       In current version the associated concepts are not implemented dans hardcoded values are provided
    memcpy(pStreamHead, ",\"chan\":0,\"rfch\":0,\"stat\":1", 27);
    pStreamHead += 27;
                                                         
    // Packet Base64 encoded RF payload padded, 14-350 useful chars
    memcpy(pStreamHead, (void *)",\"data\":\"", 9);
    pStreamHead += 9;
  
    #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
      *((BYTE *) pStreamHead) = 0;
      DEBUG_PRINT_LN("[DEBUG] CSemtechProtocolEngine_BuildUplinkMessage- JSON Message stream (before base64): ");
      DEBUG_PRINT_LN(pParams->m_pMessageData + 12);
    #endif
  
    // Check available space
    //  - Base64 encoding -> 255 bytes = 340 chars in Base64 (+ null char)
    //  - End of serialization -> 2 bytes
    //  - Null char if JSON stream displayed for debug -> 1 byte
  
    #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
      *((BYTE *) pStreamHead) = 0;
      DEBUG_PRINT("[DEBUG] CSemtechProtocolEngine_BuildUplinkMessage- MaxMessageLength: ");
      DEBUG_PRINT_DEC(pParams->m_wMaxMessageLength);
      DEBUG_PRINT(", current size: ");
      DEBUG_PRINT_DEC((pStreamHead - pParams->m_pMessageData));
      DEBUG_PRINT(", diff: ");
      DEBUG_PRINT_DEC(pParams->m_wMaxMessageLength - ((WORD) (pStreamHead - pParams->m_pMessageData)));
      DEBUG_PRINT_CR;
    #endif
  
    if (pParams->m_wMaxMessageLength - ((WORD) (pStreamHead - pParams->m_pMessageData)) < 343)
    {
      #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] CSemtechProtocolEngine_BuildUplinkMessage- buffer to small to encode payload data");
      #endif
      CMemoryBlockArray_ReleaseBlock(((CSemtechProtocolEngine *)this)->m_pTransactionArray, pMessageTransaction->m_usTransactionId);
      return false;
    }
  
    #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
      DEBUG_PRINT_LN("[DEBUG] Payload data before encoding:");
      for (unsigned int i = 0; i < pParams->m_pLoraPacket->m_dwDataSize; i++)
      {
        DEBUG_PRINT_BYTE(pParams->m_pLoraPacket->m_usData[i]);   
        DEBUG_PRINT("|");
      }
      DEBUG_PRINT_CR;
      DEBUG_PRINT_LN("## Packet end");
    #endif
  
    wLength = Base64_BinToB64(pParams->m_pLoraPacket->m_usData, pParams->m_pLoraPacket->m_dwDataSize, pStreamHead, 341);
  
    #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
      if (wLength != 0xFFFF)
      {  
        DEBUG_PRINT("[DEBUG] Encoded payload length: ");
        DEBUG_PRINT_DEC(wLength);
        DEBUG_PRINT_CR;
        *(pStreamHead + wLength) = 0;
        DEBUG_PRINT("[DEBUG] Encoded Payload data: ");
        DEBUG_PRINT_LN(pStreamHead);
    
        BYTE *pDecodedData = (BYTE *) pvPortMalloc(pParams->m_pLoraPacket->m_dwDataSize + 10);
      
        WORD wDecodedLength = Base64_B64ToBin(pStreamHead, wLength, pDecodedData, pParams->m_pLoraPacket->m_dwDataSize + 10);
      
        if (wDecodedLength != 0xFFFF)
        {
          if (wDecodedLength != pParams->m_pLoraPacket->m_dwDataSize)
          {
            DEBUG_PRINT("[DEBUG] Payload data after encoding and decoding: Wrong length, encoded: ");
            DEBUG_PRINT_DEC(wLength);
            DEBUG_PRINT(", decoded: ");
            DEBUG_PRINT_DEC(wDecodedLength);
            DEBUG_PRINT_CR;
          }
          DEBUG_PRINT_LN("[DEBUG] Payload data after encoding and decoding:");
          for (unsigned int i = 0; i < (unsigned int) wDecodedLength; i++)
          {
            DEBUG_PRINT_BYTE(pDecodedData[i]);   
            DEBUG_PRINT("|");
          }
          DEBUG_PRINT_CR;
          DEBUG_PRINT_LN("## Packet end");
        }
        else
        {
          DEBUG_PRINT_LN("[DEBUG] Payload data after encoding and decoding: Decoding error");
        }
        vPortFree(pDecodedData);
      }
    #endif
  
    if (wLength == 0xFFFF)
    {
      #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] CSemtechProtocolEngine_BuildUplinkMessage- failed to encode payload in Base64");
      #endif
      CMemoryBlockArray_ReleaseBlock(((CSemtechProtocolEngine *)this)->m_pTransactionArray, pMessageTransaction->m_usTransactionId);
      return false;
    }
    pStreamHead += wLength;

    // End of packet serialization
    *(pStreamHead++) = '"'; 
    *(pStreamHead++) = '}'; 
    *(pStreamHead++) = ']'; 
    *(pStreamHead++) = '}'; 
  }
  else if (pParams->m_wMessageType == NETWORKSERVERPROTOCOL_UPLINKMSG_HEARTBEAT)
  {
    // Note: Additional stream not required for 'PULL_DATA' message
    if (wSemtechMsgType == SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_PUSH_DATA)
    {
      // Generate the 'stat' object (i.e. built using current state recorded in ProtocolEngine)
      BYTE * pResult = CSemtechProtocolEngine_GetStatStream(this, pStreamHead);
      if (pResult == NULL)
      {
        #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[ERROR] CSemtechProtocolEngine_BuildUplinkMessage- failed to build 'stat' stream");
        #endif
        CMemoryBlockArray_ReleaseBlock(((CSemtechProtocolEngine *)this)->m_pTransactionArray, pMessageTransaction->m_usTransactionId);
        return false;
      }
      pStreamHead = pResult;
    }
  }

  // Transaction is created
  ++((CSemtechProtocolEngine *)this)->m_wPendingUpTransactionCount;
  pParams->m_wMessageLength = (WORD) (pStreamHead - pParams->m_pMessageData);

  #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
    *((BYTE *) pStreamHead) = 0;
    DEBUG_PRINT_LN("[DEBUG] CSemtechProtocolEngine_BuildUplinkMessage- Message stream (with header): ");
    for (int i = 0; i < 12; i++)
    {
      DEBUG_PRINT_BYTE(pParams->m_pMessageData[i]);
      DEBUG_PRINT(", ");
    }
    DEBUG_PRINT_LN(pParams->m_pMessageData + 12);
    DEBUG_PRINT("[DEBUG] CSemtechProtocolEngine_BuildUplinkMessage- Message stream size: ");
    DEBUG_PRINT_DEC((DWORD)pParams->m_wMessageLength);
    DEBUG_PRINT_LN(" bytes");
  #endif

  #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CSemtechProtocolEngine_BuildUplinkMessage- Number of pending uplink transactions: ");
    DEBUG_PRINT_DEC(((CSemtechProtocolEngine *)this)->m_wPendingUpTransactionCount);
    DEBUG_PRINT_CR;
  #endif

  return true;
}

// Downlink message received from Network Server
// Note: This message may be associated to an uplink or downlink protocol session
// Note: This function MUST not be called anymore for the Transaction associated to this message if it
//       returns 'NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_TERMINATED'
DWORD CSemtechProtocolEngine_ProcessServerMessage(void *this, 
                                                  CNetworkServerProtocolItf_ProcessServerMessageParams pParams)
{
  WORD wToken;
  BYTE usMessageType;
  BYTE usTransactionId;
  CSemtechMessageTransaction pMessageTransaction;
  DWORD dwCurrentTicks;

  #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[INFO] CSemtechProtocolEngine_ProcessServerMessage - Entered");
  #endif

  // Semtech messages contain at least 4 bytes
  //  - Protocol version (1 byte)
  //  - Token (2 bytes)
  //  - Message type (1 byte)
  if (pParams->m_wMessageLength < 4)
  {
    #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CSemtechProtocolEngine_ProcessServerMessage - Invalid message (size less than 4 bytes)");
    #endif
    return NETWORKSERVERPROTOCOL_SESSIONERROR_MESSAGE;
  }

  // Step 1: Retrieve the message type
  if (*((BYTE *) pParams->m_pMessageData) != SEMTECHPROTOCOLENGINE_SEMTECH_PROTOCOL_VERSION)
  {
    #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CSemtechProtocolEngine_ProcessServerMessage - Invalid protocol version (or corrupted data)");
    #endif
    return NETWORKSERVERPROTOCOL_SESSIONERROR_MESSAGE;
  }

  wToken = *((WORD *) (pParams->m_pMessageData + 1));
  usMessageType = *(pParams->m_pMessageData + 3);

  #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CSemtechProtocolEngine_ProcessServerMessage - Semtech token (in received msg): ");
    DEBUG_PRINT_HEX((DWORD) wToken);
    DEBUG_PRINT(", Semtech msg type: ");
    DEBUG_PRINT_DEC((DWORD) usMessageType);
    DEBUG_PRINT_CR;
  #endif


  // Step 2: Process message according to its type

  dwCurrentTicks = xTaskGetTickCount();

  // Check for ACK received for an uplink transaction (i.e. reply for PUSH_DATA or PULL_DATA message sent by Gateway)
  if ((usMessageType == SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_PUSH_ACK) ||
      (usMessageType == SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_PULL_ACK))
  {
    // The ACK message terminates the protocol transaction
    // Retrieve the transaction id from provided token
    // The identifier of transaction is the entry index in MemoryBlockArray
    usTransactionId = wToken & SEMTECHPROTOCOLENGINE_TRANSACTION_ID_MASK;

    pMessageTransaction = 
      (CSemtechMessageTransaction) CMemoryBlockArray_BlockPtrFromIndex(((CSemtechProtocolEngine *)this)->m_pTransactionArray, usTransactionId);

    #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
      DEBUG_PRINT("[DEBUG] CSemtechProtocolEngine_ProcessServerMessage - Processing ACK, Transaction retrieved: ");
      DEBUG_PRINT_HEX(pMessageTransaction);
      DEBUG_PRINT_CR;
    #endif

    // Consistency check
    if ((CMemoryBlockArray_IsBlockUsed(((CSemtechProtocolEngine *)this)->m_pTransactionArray, usTransactionId) == false) ||
        (pMessageTransaction->m_wMessageId != wToken))
    {
      // Unable to retrieve the 'Transaction' associated with the message
      #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
        DEBUG_PRINT("[WARNING] CSemtechProtocolEngine_ProcessServerMessage - Unable to retrieve transaction, maybe message too late (");
        if (pMessageTransaction->m_wMessageId != wToken)
        {
          DEBUG_PRINT_LN("wrong m_wMessageId)");
        }
        else
        {
          DEBUG_PRINT_LN("block not used)");
        }
      #endif
      return NETWORKSERVERPROTOCOL_SESSIONERROR_TRANSACTION;
    }

    // Transaction found, provide Protocol Message identifier to caller (typically used by caller to retrieve its own session descriptor)
    pParams->m_dwProtocolMessageId = pMessageTransaction->m_dwProtocolMessageId;

    #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL1)
      // For debug only: 100 ticks per second (measured with pdMS_TO_TICKS())
      DEBUG_PRINT("[INFO] CSemtechProtocolEngine_ProcessServerMessage - ACK received after (ms): ");
      DEBUG_PRINT_DEC((dwCurrentTicks - pMessageTransaction->m_dwTransactionStartTicks) * 10);
      DEBUG_PRINT_CR; 
    #endif

    // Update ACK received counter (heartbeat and LoRa packets)
    ++((CSemtechProtocolEngine *)this)->m_dwAckrCount;

    // NOTE: 
    //  Nothing more (i.e. transaction terminated for 'ProtocolEngine')
    //  The owner object will process ACK according to uplink message type:
    //    .. Heartbeat (PUSH_DATA 'STAT' or PULL_DATA) = nothing
    //    .. PUSH_DATA for LoRa packet = generate the 'confirmation' packet using uplink RX parameters
    //                                   (i.e. the Semtech protocol does not provide the payload for LoRa 
    //                                   packet to send to node = must be generated)         

    return NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_TERMINATED;
  }

  // TO DO -> Other messages types
  if (usMessageType == SEMTECHPROTOCOLENGINE_SEMTECH_MESSAGE_PULL_RESP)
  {
  
    // In case of PULL_RESP a LoRa packet must be sent to Node
    //  - This message instanciate a downlink transaction in 'ProtocolEngine'
    //  - The data in PULL_RESP message are process to describe the LoRa packet to transmit
   
    // The caller must provide the memory block for LoRa packet
    if (pParams->m_pData == NULL || pParams->m_wMaxLoraPacketLength == 0)
    {
      // Should never occur (i.e. adjust memory block array size)
      // Unable to process the dwonlink message
      #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] CSemtechProtocolEngine_ProcessServerMessage - No memory to encode ACK (no confirmation available for Node)");
      #endif
    }

    // Prepare the LoRa packet
    
  }



  // Unknown type, probably corrupted data
  #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[ERROR] CSemtechProtocolEngine_ProcessServerMessage - Invalid message type (possibly corrupted data)");
  #endif
  return NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_FAILED;
}

// Event occured while processing the session (initiated by uplink or downlink message) 
// Note: This function MUST not be called anymore for the Transaction associated to this session if it returns
//       'NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_FAILED'
// Note: The owner obejct must call this function with 'NETWORKSERVERPROTOCOL_SESSIONEVENT_RELEASED' when the 
//       'NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_TERMINATED' code is returned (i.e. to confirm that it has
//       finisehd with the session).
DWORD CSemtechProtocolEngine_ProcessSessionEvent(void *this, 
                                                 CNetworkServerProtocolItf_ProcessSessionEventParams pParams)
{                                     
  CSemtechMessageTransaction pMessageTransaction;
  BYTE usBlockIndex;

  DWORD dwResult = NETWORKSERVERPROTOCOL_SESSIONERROR_OK;

  #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
    DEBUG_PRINT("[INFO] CSemtechProtocolEngine_ProcessSessionEvent - Entered, event: ");
    DEBUG_PRINT_HEX((DWORD) pParams->m_wSessionEvent);
    DEBUG_PRINT_CR;
  #endif

  // Step 1: Retrieve the transaction associated with the event 
  //
  // The 'TransactionId' is encoded in the LOWORD of specified 'm_dwProtocolMessageId.
  // The 'TransactionId' is the index in the 'MemoryBlockArray' used for 'CSemtechMessageTransaction'
  usBlockIndex = (BYTE)(((WORD) pParams->m_dwProtocolMessageId) & SEMTECHPROTOCOLENGINE_TRANSACTION_ID_MASK);
  pMessageTransaction = CMemoryBlockArray_BlockPtrFromIndex(((CSemtechProtocolEngine *)this)->m_pTransactionArray, usBlockIndex);

  // Consistency check (i.e. 'MemoryBlockArray' entries are reused (timeout applies when waiting for protocol
  // message/events)
  if ((CMemoryBlockArray_IsBlockUsed(((CSemtechProtocolEngine *)this)->m_pTransactionArray, usBlockIndex) == false) ||
      (pMessageTransaction->m_wMessageId != (WORD) pParams->m_dwProtocolMessageId))
  {
    // Unable to retrieve the 'Transaction' associated with the message
    #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
      DEBUG_PRINT("[WARNING] CSemtechProtocolEngine_ProcessSessionEvent- Unable to retrieve transaction (");
      if (pMessageTransaction->m_wMessageId != (WORD) pParams->m_dwProtocolMessageId)
      {
        DEBUG_PRINT("wrong m_wMessageId), MessageTransaction.m_dwProtocolMessageId: ");
        DEBUG_PRINT_HEX(pMessageTransaction->m_dwProtocolMessageId);
        DEBUG_PRINT(", Params.m_dwProtocolMessageId: ");
        DEBUG_PRINT_HEX(pParams->m_dwProtocolMessageId);
        DEBUG_PRINT_CR;
      }
      else
      {
        DEBUG_PRINT_LN("block not used)");
      }
    #endif
    return NETWORKSERVERPROTOCOL_SESSIONERROR_TRANSACTION;
  }

  #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CSemtechProtocolEngine_ProcessSessionEvent - Transaction found: ");
    DEBUG_PRINT_HEX(pMessageTransaction);
    DEBUG_PRINT(", m_dwProtocolMessageId: ");
    DEBUG_PRINT_HEX(pMessageTransaction->m_dwProtocolMessageId);
    DEBUG_PRINT(", m_wMessageType: ");
    DEBUG_PRINT_HEX(pMessageTransaction->m_wMessageType);
    DEBUG_PRINT(", m_wTransactionState: ");
    DEBUG_PRINT_HEX(pMessageTransaction->m_wTransactionState);
    DEBUG_PRINT(", m_usTransactionType: ");
    DEBUG_PRINT_HEX(pMessageTransaction->m_usTransactionType);
    DEBUG_PRINT_CR;
  #endif

  // Step 2: Process according to received event and transaction type
  switch (pParams->m_wSessionEvent)
  {
    case NETWORKSERVERPROTOCOL_SESSIONEVENT_SENT:
      if ((pMessageTransaction->m_usTransactionType == SEMTECHMESSAGETRANSACTION_TYPE_PUSHDATA) ||
          (pMessageTransaction->m_usTransactionType == SEMTECHMESSAGETRANSACTION_TYPE_PULLDATA))
      {
        if (pMessageTransaction->m_wTransactionState == SEMTECHPROTOCOLENGINE_TRANSACTION_STATE_SENDING)
        {
          // Update transaction state
          // Automaton will wait for 'ACK' meessage (or timeout)
          pMessageTransaction->m_dwLastEventTicks = xTaskGetTickCount();
          pMessageTransaction->m_wTransactionState = SEMTECHPROTOCOLENGINE_TRANSACTION_STATE_SENT;
          dwResult = NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_PROGRESSING;

          // Update counters uplink messages sent (Heartbeat and LoRa packets)
          ++((CSemtechProtocolEngine *)this)->m_dwUpnbCount;

          // If sending a LoRa packet, update forwarded packet counter
          if (pMessageTransaction->m_bHeartbeat == false)
          {
            ++((CSemtechProtocolEngine *)this)->m_dwRxfwCount;
          }
        }
        else
        {
          // Should never occur. Message received in invalid state (ignore it)
          #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
            DEBUG_PRINT_LN("[ERROR] CSemtechProtocolEngine_ProcessSessionEvent - Message received in invalid state(1), ignored");
          #endif
        }
      }
      else
      {
        #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[WARNING] CSemtechProtocolEngine_ProcessSessionEvent - TO DO process SEND event for other transaction type");
        #endif
      }
      break;

    case NETWORKSERVERPROTOCOL_SESSIONEVENT_SENDFAILED:
      if ((pMessageTransaction->m_usTransactionType == SEMTECHMESSAGETRANSACTION_TYPE_PUSHDATA) ||
          (pMessageTransaction->m_usTransactionType == SEMTECHMESSAGETRANSACTION_TYPE_PULLDATA))
      {
        if (pMessageTransaction->m_wTransactionState == SEMTECHPROTOCOLENGINE_TRANSACTION_STATE_SENDING)
        {
          // Unable to send the message, terminate the transaction
          CMemoryBlockArray_ReleaseBlock(((CSemtechProtocolEngine *)this)->m_pTransactionArray, usBlockIndex);
          --((CSemtechProtocolEngine *)this)->m_wPendingUpTransactionCount;
          dwResult = NETWORKSERVERPROTOCOL_UPLINKSESSIONEVENT_FAILED;

          #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
            DEBUG_PRINT("[DEBUG] CSemtechProtocolEngine_ProcessSessionEvent - SendFailed - Number of pending uplink transactions: ");
            DEBUG_PRINT_DEC(((CSemtechProtocolEngine *)this)->m_wPendingUpTransactionCount);
            DEBUG_PRINT_CR;
          #endif
        }
        else
        {
          // Should never occur. Message received in invalid state (ignore it)
          #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
            DEBUG_PRINT_LN("[ERROR] CSemtechProtocolEngine_ProcessSessionEvent - Message received in invalid state(1), ignored");
          #endif
        }
      }
      else
      {
        #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[WARNING] CSemtechProtocolEngine_ProcessSessionEvent - TO DO process SENDFAILED event for other transaction type");
        #endif
      }
      break;

    case NETWORKSERVERPROTOCOL_SESSIONEVENT_RELEASED:
      // Owner object confirms it has finished with the session (release memory block used for the transaction)
      #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
        DEBUG_PRINT_LN("[DEBUG] CSemtechProtocolEngine_ProcessSessionEvent - Releasing Transaction memory block");
      #endif

      CMemoryBlockArray_ReleaseBlock(((CSemtechProtocolEngine *)this)->m_pTransactionArray, usBlockIndex);
      --((CSemtechProtocolEngine *)this)->m_wPendingUpTransactionCount;

      #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
        DEBUG_PRINT_LN("[DEBUG] CSemtechProtocolEngine_ProcessSessionEvent - Transaction memory block released");
      #endif

      #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
        DEBUG_PRINT("[DEBUG] CSemtechProtocolEngine_ProcessSessionEvent - Session released, ticks: ");
        DEBUG_PRINT_DEC((DWORD) xTaskGetTickCount());
        DEBUG_PRINT_CR;
      #endif

      #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
        DEBUG_PRINT("[DEBUG] CSemtechProtocolEngine_ProcessSessionEvent - Released - Number of pending uplink transactions: ");
        DEBUG_PRINT_DEC(((CSemtechProtocolEngine *)this)->m_wPendingUpTransactionCount);
        DEBUG_PRINT_CR;
      #endif
      break;

    case NETWORKSERVERPROTOCOL_SESSIONEVENT_CANCELED:
      // Owner object asks to cancel the transaction (typically no more event expected from Network Server)
      CMemoryBlockArray_ReleaseBlock(((CSemtechProtocolEngine *)this)->m_pTransactionArray, usBlockIndex);
      --((CSemtechProtocolEngine *)this)->m_wPendingUpTransactionCount;

      #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2)
        DEBUG_PRINT("[DEBUG] CSemtechProtocolEngine_ProcessSessionEvent - Canceled - Number of pending uplink transactions: ");
        DEBUG_PRINT_DEC(((CSemtechProtocolEngine *)this)->m_wPendingUpTransactionCount);
        DEBUG_PRINT_CR;
      #endif
      break;

    default:
      // By design the function must process all types of event
      #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] CSemtechProtocolEngine_ProcessSessionEvent - Unknown session event");
      #endif
      break;
  }

  return dwResult;
}


/********************************************************************************************* 
  Private methods of CSemtechProtocolEngine object
 
  The following methods CANNOT be called by another object
*********************************************************************************************/



/*********************************************************************************************
  Construction

  Protected methods : must be called only object factory and 'INetworkServerProtocol' interface
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         CSemtechProtocolEngine * CSemtechProtocolEngine_New()
 * 
 * @brief      Object construction.
 * 
 * @details    
 * 
 * @return     The function returns the pointer to the CSemtechProtocolEngine instance.
 *
 * @note       This function only creates the object and its dependencies (RTOS objects).\n
*********************************************************************************************/
CSemtechProtocolEngine * CSemtechProtocolEngine_New()
{
  CSemtechProtocolEngine *this;

#if SEMTECHPROTOCOLENGINE_DEBUG_LEVEL2
  printf("CSemtechProtocolEngine_New -> Debug level 2 (DEBUG)\n");
#elif SEMTECHPROTOCOLENGINE_DEBUG_LEVEL1
  printf("CSemtechProtocolEngine_New -> Debug level 1 (INFO)\n");
#elif SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0
  printf("CSemtechProtocolEngine_New -> Debug level 0 (NORMAL)\n");
#endif 

  if ((this = (void *) pvPortMalloc(sizeof(CSemtechProtocolEngine))) != NULL)
  {
    // Embedded objects are not defined (i.e. created below)
    this->m_pTransactionArray = NULL;

    // Allocate memory blocks for internal collections
    if ((this->m_pTransactionArray = CMemoryBlockArray_New(sizeof(CSemtechMessageTransactionOb),
        SEMTECHPROTOCOLENGINE_MAX_TRANSACTIONS)) == NULL)
    {
      CSemtechProtocolEngine_Delete(this);
      return NULL;
    }

    // Initialize object's properties
    this->m_nRefCount = 0;

    this->m_dwLastPushDataTicks = 0;    
    this->m_dwLastPullDataTicks = 0;

    this->m_wMessageIdCounter = 0;
    this->m_wPendingUpTransactionCount = 0;

    this->m_dwRxnbCount = 0; 
    this->m_dwRxokCount = 0; 
    this->m_dwRxfwCount = 0; 
    this->m_dwAckrCount = 0; 
    this->m_dwDwnbCount = 0; 
    this->m_dwTxnbCount = 0; 
    this->m_dwUpnbCount = 0;

    // Hardcoded
    // TO DO -> Provided during initialization (from configuration or GPS)
    strcpy((char*) this->m_strGatewayLatitude, "45.835549");
    strcpy((char*) this->m_strGatewayLongitude, "2.281144");
    strcpy((char*) this->m_strGatewayAltitude, "110");
    this->m_wGatewayLatitudeLength = 9;
    this->m_wGatewayLongitudeLength = 8;
    this->m_wGatewayAltitudeLength = 3;


    #ifdef CONFIG_NETWORK_SERVER_LORIOT
      BYTE MACAddr[8] = {0x24,0x0A,0xC4,0xFF,0xFF,0x02,0x72,0xB4};
    #endif

    #ifdef CONFIG_NETWORK_SERVER_TTN
      BYTE MACAddr[8] = {0x24,0x0A,0xC4,0xFF,0xFE,0x02,0x72,0xB4};
    #endif

    memcpy(this->m_GatewayMACAddr, MACAddr, 8);

  }
  return this;
}

/*****************************************************************************************//**
 * @fn         void CSemtechProtocolEngine_Delete(CSemtechProtocolEngine *this)
 * 
 * @brief      Object destruction.
 * 
 * @details    Destroys the CSemtechProtocolEngine object.\n
 *             The associated RTOS objects are destroyed and the memory used by CSemtechProtocolEngine
 *             object are released.

 * @param      this
 *             The pointer to CSemtechProtocolEngine object.
 *  
 * @return     None.
*********************************************************************************************/
void CSemtechProtocolEngine_Delete(CSemtechProtocolEngine *this)
{
  // Free memory
  if (this->m_pTransactionArray != NULL)
  {
    CMemoryBlockArray_Delete(this->m_pTransactionArray);
  }

  vPortFree(this);
}




/*********************************************************************************************
  Private methods (implementation)

  Utility functions
*********************************************************************************************/


WORD CSemtechProtocolEngine_GetNewMessageId(CSemtechProtocolEngine *this, BYTE usTransactionId)
{
  // The identifier is built using 2 values:
  //  - <hi_part><low_part>'
  //  - An incremental counter (this->m_wMessageIdCounter) for '<hi_part>'
  //  - The 'usTransactionId' for '<low_part>'
  //  - The range for these parts depends on the maximum number of active transactions 
  //    configured for the 'SemtechProtocolEngine' (= SEMTECHPROTOCOLENGINE_MAX_TRANSACTIONS)
  //    (i.e. the range for 'usTransactionId')
  if (this->m_wMessageIdCounter == 0xFFFF >> SEMTECHPROTOCOLENGINE_MAX_TRANSACTION_BITS)
  {
    // By design, do not generate id with 0 value
    this->m_wMessageIdCounter = 1;
  }
  else
  {
    this->m_wMessageIdCounter++;
  }
  return (this->m_wMessageIdCounter << SEMTECHPROTOCOLENGINE_MAX_TRANSACTION_BITS) | ((WORD) usTransactionId);
}

// Builds the 'stat' JSON string using current counter values
// The format of this 'stat' JSON string is conform for use in the 'PUSH_DATA' message
// The function return the pointer to 'end of stream + 1' for updated stream or NULL in case of error 
// The calling function MUST ensure that 'pStreamData' buffer length is at least 170 bytes (for 16-bit counters)
// or 190 bytes (for 32-bit counters)

BYTE * CSemtechProtocolEngine_GetStatStream(CSemtechProtocolEngine *this, BYTE *pStreamData)
{
  struct tm *tmTime;
  time_t timeNow;
  BYTE pTempBuffer[64];
  WORD wLength;
  BYTE *pStreamHead;
  double dTempValue;

  pStreamHead = pStreamData;

  memcpy(pStreamHead, (void *)"{\"stat\":{", 9);
  pStreamHead += 9;

  #if (SEMTECHPROTOCOLENGINE_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[WARNING] CSemtechProtocolEngine_GetStatStream- TO DO: improve timestamp management");
  #endif

  // Gateway system time
  // UTC 'system' time of the gateway, ISO 8601 'expanded' format (23 useful chars)

  // Split the UNIX timestamp to its calendar components 
  time(&timeNow);
  tmTime = gmtime(&timeNow);
  sprintf((char*) pTempBuffer, "\"time\":\"%04i-%02i-%02i %02i:%02i:%02i GMT\"", (tmTime->tm_year) + 1900,
          (tmTime->tm_mon) + 1, tmTime->tm_mday, tmTime->tm_hour, tmTime->tm_min, tmTime->tm_sec);
  memcpy(pStreamHead, pTempBuffer, wLength = strlen((char*) pTempBuffer));
  pStreamHead += wLength;

  // GPS latitude of the gateway in degree (float, precision 5 decimals, North is +)
  memcpy(pStreamHead, ",\"lati\":", 8);
  pStreamHead += 8;
  memcpy(pStreamHead, this->m_strGatewayLatitude, this->m_wGatewayLatitudeLength);
  pStreamHead += this->m_wGatewayLatitudeLength;

  // GPS latitude of the gateway in degree (float, precision 5 decimals, East is +)
  memcpy(pStreamHead, ",\"long\":", 8);
  pStreamHead += 8;
  memcpy(pStreamHead, this->m_strGatewayLongitude, this->m_wGatewayLongitudeLength);
  pStreamHead += this->m_wGatewayLongitudeLength;
  
  // GPS altitude of the gateway in meter RX (integer)
  memcpy(pStreamHead, ",\"alti\":", 8);
  pStreamHead += 8;
  memcpy(pStreamHead, this->m_strGatewayAltitude, this->m_wGatewayAltitudeLength);
  pStreamHead += this->m_wGatewayAltitudeLength;

  // Number of radio packets received from nodes (unsigned integer)
  sprintf((char*) pTempBuffer, ",\"rxnb\":%u", this->m_dwRxnbCount);
  memcpy(pStreamHead, pTempBuffer, wLength = strlen((char*) pTempBuffer));
  pStreamHead += wLength;

  // Number of radio packets received from nodes with a valid PHY CRC (unsigned integer)
  sprintf((char*) pTempBuffer, ",\"rxok\":%u", this->m_dwRxokCount);
  memcpy(pStreamHead, pTempBuffer, wLength = strlen((char*) pTempBuffer));
  pStreamHead += wLength;

  // Number of radio packets forwarded to Network Server (unsigned integer)
  sprintf((char*) pTempBuffer, ",\"rxfw\":%u", this->m_dwRxfwCount);
  memcpy(pStreamHead, pTempBuffer, wLength = strlen((char*) pTempBuffer));
  pStreamHead += wLength;

  // Percentage of upstream datagrams that were acknowledged (float, precision 1 decimal)
  if (this->m_dwUpnbCount == 0)
  {
    dTempValue = 100;
  }
  else
  {
    dTempValue = ((double) this->m_dwAckrCount * 100) / this->m_dwUpnbCount;
  }
  sprintf((char*) pTempBuffer, ",\"ackr\":%.1f", dTempValue); 

  memcpy(pStreamHead, pTempBuffer, wLength = strlen((char*) pTempBuffer));
  pStreamHead += wLength;

  // Number of downlink datagrams received (unsigned integer)
  sprintf((char*) pTempBuffer, ",\"dwnb\":%u", this->m_dwDwnbCount);
  memcpy(pStreamHead, pTempBuffer, wLength = strlen((char*) pTempBuffer));
  pStreamHead += wLength;

  // Number of radio packets emitted to nodes (unsigned integer)
  sprintf((char*) pTempBuffer, ",\"txnb\":%u}}", this->m_dwTxnbCount);
  memcpy(pStreamHead, pTempBuffer, wLength = strlen((char*) pTempBuffer));
  pStreamHead += wLength;

  return pStreamHead;
}

DWORD CSemtechProtocolEngine_GetElapsedTicks(DWORD dwCurrentTicks, DWORD dwPreviousTicks)
{
  if (dwCurrentTicks < dwPreviousTicks)
  {
    #if configUSE_16_BIT_TICKS == 1
      return dwCurrentTicks + (0xFFFF - dwPreviousTicks);
    #else
      return dwCurrentTicks + (0xFFFFFFFF - dwPreviousTicks);
    #endif
  }
  return dwCurrentTicks - dwPreviousTicks;
}
