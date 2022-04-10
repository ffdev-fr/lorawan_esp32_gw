/*********************************************************************************************
PROJECT : LoRaWAN ESP32 Gateway V1.x
 
FILE    : Configuration.h
 
AUTHOR  : F.Fargon 
 
PURPOSE : This file contains global settings for Gateway configuration. 

COMMENTS: For the early version configuration is statically defined in software. 
          The configuration object is instancied in the associated object's implementation :
           . The '#define <OBJECT>CONFIG_IMPL' pragma must be specified before including this
             header file in the implementing object.
*********************************************************************************************/

#ifndef LORAGW_CONFIGURATION_H
#define LORAGW_CONFIGURATION_H

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gateway configuration and radio settings
//
// Note: 
//  - In this early version, the gateway hardware configuration and radio settings are
//    statically defined.
//  - In the final version, a similar object will be created in the main program using a 
//    configuration file and provided to 'CLoraNodeManager' object
//  - The operation mode for the gateway is:
//     .. Each CSX1276 is used for both uplink and downlink on a configured channel frequency
//     .. The CSX1276 is waiting for uplink LoRa packets (i.e. continuous receive mode by default)
//     .. The CSX1276 operation mode is changed to 'Send' mode only when a downlink packet is
//        received from the Network Server (typically LoRa 'ACK' message)
//////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef NODEMANAGERCONFIG_IMPL

CTransceiverManagerItf_InitializeParamsOb g_LoraNodeManagerSettings = 
  { 
    .m_bUseBuiltinSettings = true,

    .pLoraTransceiverSettings = 
    { 
      [0] =
      {
        .LoraMAC = 
        {
          .m_wPreambleLength = LORATRANSCEIVERITF_PREAMBLE_LENGTH_NONE,
          .m_usSyncWord = LORATRANSCEIVERITF_SYNCWORD_NONE,
          .m_usHeader = LORATRANSCEIVERITF_HEADER_NONE,
          .m_usCRC = LORATRANSCEIVERITF_CRC_NONE,
          .m_bForce = false
        },
        .LoraMode =
        { 
          .m_usLoraMode = LORATRANSCEIVERITF_LORAMODE_NONE,
          .m_usCodingRate = LORATRANSCEIVERITF_CR_5,
          .m_usSpreadingFactor = LORATRANSCEIVERITF_SF_7,
          .m_usBandwidth = LORATRANSCEIVERITF_BANDWIDTH_125,
          .m_bForce = false
        },
        .PowerMode = 
        {
          .m_usPowerMode = LORATRANSCEIVERITF_POWER_MODE_LOW,
          .m_usPowerLevel = LORATRANSCEIVERITF_POWER_LEVEL_NONE,
          .m_usOcpRate = LORATRANSCEIVERITF_OCP_NONE,
          .m_bForce = false
        },
        .FreqChannel = 
        {
          .m_usFreqChannel = LORATRANSCEIVERITF_FREQUENCY_CHANNEL_18,
          .m_bForce = false
        },
      },
      [1] =
      {
        .LoraMAC = 
        {
          .m_wPreambleLength = LORATRANSCEIVERITF_PREAMBLE_LENGTH_NONE,
          .m_usSyncWord = LORATRANSCEIVERITF_SYNCWORD_NONE,
          .m_usHeader = LORATRANSCEIVERITF_HEADER_NONE,
          .m_usCRC = LORATRANSCEIVERITF_CRC_NONE,
          .m_bForce = false
        },
        .LoraMode =
        { 
          .m_usLoraMode = LORATRANSCEIVERITF_LORAMODE_NONE,
          .m_usCodingRate = LORATRANSCEIVERITF_CR_5,
          .m_usSpreadingFactor = LORATRANSCEIVERITF_SF_7,
          .m_usBandwidth = LORATRANSCEIVERITF_BANDWIDTH_125,
          .m_bForce = false
        },
        .PowerMode = 
        {
          .m_usPowerMode = LORATRANSCEIVERITF_POWER_MODE_LOW,
          .m_usPowerLevel = LORATRANSCEIVERITF_POWER_LEVEL_NONE,
          .m_usOcpRate = LORATRANSCEIVERITF_OCP_NONE,
          .m_bForce = false
        },
        .FreqChannel = 
        {
          .m_usFreqChannel = LORATRANSCEIVERITF_FREQUENCY_CHANNEL_17,
          .m_bForce = false
        }
      }
    }
  };

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gateway configuration for node management
//
// Note: 
//  - In this early version, the gateway configuration for node management is statically defined.
//  - In the final version, a similar object will be created in the main program using a configuration
//    file and provided to 'CLoraNodeManager' object
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Maximum number of nodes managed by the gateway
#define CONFIG_NODE_MAX_NUMBER     20


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gateway configuration and server connection settings
//
// Note: 
//  - In this early version, the gateway hardware configuration and connection settings are
//    statically defined.
//  - In the final version, a similar object will be created in the main program using a 
//    configuration file and provided to 'CLoraServerManager' object
//  - The operation mode for the gateway is:
//     .. Semtech protocol
//     .. No dynamic failover (first successful connector, in configuration order, is used until the next reboot)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define CONFIG_WIFI_NETWORK_NETGEAR    1
//#define CONFIG_WIFI_NETWORK_ANDROID    1

#define CONFIG_NETWORK_SERVER_TTN      1
//#define CONFIG_NETWORK_SERVER_LORIOT   1


#ifdef SEMTECHPROTOCOLENGINE_IMPL

//#error "included"

// Period for PUSH_DATA (i.e. heartbeat with STAT message if no uplink message sent since period)
//#define CONFIG_SEMTECH_PUSHSTAT_PERIOD  60000
#define CONFIG_SEMTECH_PUSHSTAT_PERIOD  60000

// Period for PULL_DATA uplink message (i.e. check for downlink messages from Network Server)
//#define CONFIG_SEMTECH_PULLDATA_PERIOD  25000
#define CONFIG_SEMTECH_PULLDATA_PERIOD  100000

#endif


#ifdef SERVERMANAGERCONFIG_IMPL

CServerManagerItf_InitializeParamsOb g_LoraServerManagerSettings = 
  { 
    .m_bUseBuiltinSettings = true,

    .LoraServerSettings = 
    {
      .ConnectorSettings =
      {
        [0] =
        {
          #if (CONFIG_WIFI_NETWORK_NETGEAR)
            // Netgear Paris
            .m_szNetworkName = "NETGEAR_11ng\0",           // SSID for Wifi Network
            .m_szNetworkUser = "\0",                       // Not used for Wifi Network
            .m_szNetworkPassword = "spaddeperdussin\0",    // Password for Network (Wifi or GPRS)
          #endif

          #if (CONFIG_WIFI_NETWORK_ANDROID)
            // Android Samsung
            .m_szNetworkName = "AndroidAP\0",         // SSID for Wifi Network
            .m_szNetworkUser = "\0",                  // Not used for Wifi Network
            .m_szNetworkPassword = "gubd3761\0",      // Password for Network (Wifi or GPRS)
          #endif
          .m_dwNetworkJoinTimeout = 60000,
          .m_dwNetworkServerTimeout = 5000
        },
      },
      .m_wNetworkServerProtocol = SERVERMANAGER_PROTOCOL_SEMTECH, 

      #if (CONFIG_NETWORK_SERVER_TTN)
        // TTN
        .m_szNetworkServerUrl = "router.eu.thethings.network\0",
        .m_dwNetworkServerPort = 1700,
        //.m_szGatewayIDToken = "FFFE\0",               // WIFI_MAC[:6] + "FFFE" + WIFI_MAC[6:12]
        .m_szGatewayIDToken = "240AC4FFFE0272B4\0",     // MAC Addr du LoPy NanoGateway (Registered with NS) 
      #endif

      #if (CONFIG_NETWORK_SERVER_LORIOT)
        // Loriot
        .m_szNetworkServerUrl = "eu1.loriot.io\0",
        .m_dwNetworkServerPort = 1780,
        //.m_szGatewayIDToken = "FFFF\0",               // WIFI_MAC[:6] + "FFFF" + WIFI_MAC[6:12]
        .m_szGatewayIDToken = "240AC4FFFF0272B4\0",     // MAC Addr du LoPy NanoGateway (Registered with NS)
      #endif

      .m_szNetworkServerUser = "\0",
      .m_szNetworkServerPassword = "\0",

      .m_dwSNTPServerPeriodSec = 3600,                  // The 0 value indicates SNTP time update not required
      .m_szSNTPServerUrl = "pool.ntp.org\0",

      .m_GatewayMACAddr = {0x24,0x0A,0xC4,0x02,0x72,0xB4}   // The WIFI_MAC (explicitly set on ESP32 during initialization)
    }
  };

#endif
#endif

