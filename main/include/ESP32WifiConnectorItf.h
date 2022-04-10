/*****************************************************************************************//**
 * @file     ESP32WifiConnectorlItf.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     22/04/2018
 *
 * @brief    A 'CESP32WifiConnector' object use the native WIFI hardware/software of ESP32 
 *           device to implement the 'IServerConnector' interface.
 *
 * @details  This class is the interface exposed by objects implementing the 'IServerConnector'
 *           interface on the native WIFI features of the ESP32 device.\n
 *           Once instantiated the 'CESP32WifiConnector' object is publicly accessed using the
 *           'IServerConnector' interface.
 *
 * @note     The client object must call the 'ReleaseItf' method on 'IServerConnector' interface 
 *           to destroy the 'CESP32WifiConnector' object created by 'CreateInstance'.
 *
 * @note     This class IS THREAD SAFE.
*********************************************************************************************/

#ifndef ESP32WIFICONNECTORITF_H
#define ESP32WIFICONNECTORITF_H

#include "ServerConnectorItf.h"

#include "ServerConnectorItfImpl.h"

/********************************************************************************************* 
  Objects used as parameter for commands on 'CESP32ServerConnector' object
  ('ESP32WIFICONNECTOR_AUTOMATON_CMD_xxx')

  Typically, the 'ESP32WIFICONNECTOR_AUTOMATON_CMD_xxx' are called from 'IServerConnectorItf'
  implementation on 'CESP32WifiConnector' object.
  Some methods need additional parameters specific to the actual type of connector 
  (i.e. additional parameters related to wifi).
*********************************************************************************************/

// Forward declarations (objects used to exchange data on interface)

#define CESP32WifiConnectorItf_InitializeParams CServerConnectorItf_InitializeParams
#define CESP32WifiConnectorItf_StartParams CServerConnectorItf_StartParams
#define CESP32WifiConnectorItf_StopParams CServerConnectorItf_StopParams
#define CESP32WifiConnectorItf_SendParams CServerConnectorItf_SendParams
#define CESP32WifiConnectorItf_SendReceiveParams CServerConnectorItf_SendReceiveParams
#define CESP32WifiConnectorItf_DownlinkReceivedParams CServerConnectorItf_DownlinkReceivedParams

/*
typedef struct _CESP32WifiConnectorItf_InitializeParams * CESP32WifiConnectorItf_InitializeParams;

typedef struct _CESP32WifiConnectorItf_InitializeParams
{
  // Public

  // Connector settings
  CServerConnectorItf_ConnectorSettings m_pConnectorSettings;

  // Initial STAT message used to check that Network Server is reachable
  // A NULL value is allowed if check not required
//  CServerConnectorItf_SendParams pStatMsg;

  IServerManager m_pServerManagerItf;

} CESP32WifiConnectorItf_InitializeeParamsOb;
*/


// CESP32WifiConnector object factory
// This method in invoked by client objet to create a new instance of CESP32WifiConnector object
IServerConnector CESP32WifiConnector_CreateInstance();



#endif

