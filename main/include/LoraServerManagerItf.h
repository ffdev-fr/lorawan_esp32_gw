/*********************************************************************************************
MODULE  : LoraServerManagerItf

AUTHOR  : F.Fargon 
 
PURPOSE : This file contains the definition of the method used to instantiate a 'CLoraServerManager'
          object.
          The 'CLoraServerManager' object implements the generic 'IServerManager' interface.
          Once instantiated the 'CLoraServerManager' object is publicly accessed using the 
          'ITransceiverManager' interface.
 
COMMENTS: The client object must call the 'ReleaseItf' method on 'IServerManager' interface 
          to destroy the 'CLoraServerManager' object created by 'CreateInstance'.
*********************************************************************************************/

#ifndef LORASERVERMANAGERITF_H
#define LORASERVERMANAGERITF_H

#include "ServerManagerItf.h"

// CLoraServerManager object factory
// This method in invoked by client objet to create a new instance of CLoraServerManager object
IServerManager CLoraServerManager_CreateInstance(BYTE usWifiConnectorNumber,BYTE usGPRSConnectorNumber,
                                                 BYTE usNetworkServerProtocol);



#endif

