/*********************************************************************************************
PROJECT : LoRaWAN ESP32 Gateway V1.x
 
FILE    : Definitions.h
 
AUTHOR  : F.Fargon 
 
PURPOSE : This file contains program definitions for:
            - Hardware and software configuration
            - Builtin user configuration
            - Hardware resources

COMMENTS: The version string MUST be updated during the build process of a new release. 
          This program is designed for execution on ESP32 Module (Dev.C kit). 
          The LoRa radio is implemented with Semtech SX1276 chip (Modtronix inAir9 module)  
          The implementation uses the Espressif IDF V3.0 framework (with RTOS)
*********************************************************************************************/

#ifndef LORAGW_DEFINITIONS_H
#define LORAGW_DEFINITIONS_H

/********************************************************************************************* 
  Fixed width integers
*********************************************************************************************/

#define BYTE   uint8_t
#define WORD   uint16_t
#define DWORD  uint32_t


/********************************************************************************************* 
  Standard C-99 language type and definitions (not supported in GCC)
*********************************************************************************************/

// BOOLEAN definitions are included from <stdbool.h> by "FreeRTOS.h"
// Note: true = 1, false = 0
#ifndef bool
#define bool    _Bool
#define true    1
#define false   0
#endif



/********************************************************************************************* 
  Utility definitions
*********************************************************************************************/

#define HIBYTE(wValue)   (wValue >> 8)
#define LOBYTE(wValue)   (wValue & 0x00FF)

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)    // read a bit
#define bitSet(value, bit) ((value) |= (1UL << (bit)))     // set bit to '1'
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))  // set bit to '0'


/********************************************************************************************* 
  Helper macros
*********************************************************************************************/

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))



/********************************************************************************************* 
  Definitions for debug traces
*********************************************************************************************/

#define DEBUG_PRINT(msg)        (printf((const char *)(msg)))
#define DEBUG_PRINT_CR          (printf("\n"))
#define DEBUG_PRINT_LN(msg)     (printf("%s\n", (const char *)(msg)))
#define DEBUG_PRINT_HEX(value)  (printf("0x%.8X", (DWORD) value))
#define DEBUG_PRINT_DEC(value)  (printf("%d", (DWORD) value))
#define DEBUG_PRINT_BYTE(value) (printf("0x%.2X", (BYTE) value))
#define DEBUG_PRINT_WORD(value) (printf("0x%.4X", (WORD) value))


/********************************************************************************************* 
  Hardware Configuration
*********************************************************************************************/


// Maximum number of 'LoraTransceiver' supported by the Gateway
#define GATEWAY_MAX_LORATRANSCEIVERS       0x03

// Maximum number of 'ServerConnector' supported by the Gateway
//  - One ESP32 Wifi
//  - One FONA808 GPRS
#define GATEWAY_MAX_SERVERCONNECTORS       2


/********************************************************************************************* 
  Software Configuration
*********************************************************************************************/

// Debug level for software modules
#define DEBUG_LEVEL0           0x01           // NORMAL
#define DEBUG_LEVEL1           0x02           // INFO
#define DEBUG_LEVEL2           0x04           // DEBUG


#define UTILITIES_DEBUG_LEVEL              (DEBUG_LEVEL0)
#define SX1276_DEBUG_LEVEL                 (DEBUG_LEVEL2 | DEBUG_LEVEL1 | DEBUG_LEVEL0)
#define LORANODEMANAGER_DEBUG_LEVEL        (DEBUG_LEVEL2 | DEBUG_LEVEL1 | DEBUG_LEVEL0)
#define LORASERVERMANAGER_DEBUG_LEVEL      (DEBUG_LEVEL2 | DEBUG_LEVEL1 | DEBUG_LEVEL0)
#define ESP32WIFICONNECTOR_DEBUG_LEVEL     (DEBUG_LEVEL2 | DEBUG_LEVEL1 | DEBUG_LEVEL0)

//#define LORASERVERMANAGER_DEBUG_LEVEL      (DEBUG_LEVEL0)
//#define ESP32WIFICONNECTOR_DEBUG_LEVEL     (DEBUG_LEVEL0)

#define SEMTECHPROTOCOLENGINE_DEBUG_LEVEL  (DEBUG_LEVEL2 | DEBUG_LEVEL1 | DEBUG_LEVEL0)
#define LORAREALTIMESENDER_DEBUG_LEVEL  (DEBUG_LEVEL2 | DEBUG_LEVEL1 | DEBUG_LEVEL0)



/********************************************************************************************* 
  Program Constants
*********************************************************************************************/


// Maximum length of LoRa payload
#define LORA_MAX_PAYLOAD_LENGTH    255



#endif
