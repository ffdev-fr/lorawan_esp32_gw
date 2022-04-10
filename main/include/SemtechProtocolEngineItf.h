/*****************************************************************************************//**
 * @file     SemtechProtocolEngineItf.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     15/04/2018
 *
 * @brief    Class for 'ISemtechProtocolEngine' interface.
 *
 * @details  This class contains the definition of the method used to instantiate a 
 *           'CSemtechProtocolEngine' object.\n
 *           The 'CSemtechProtocolEngine' object implements the generic 'INetworkServerProtocol'
 *           interface. Once instantiated the 'CSemtechProtocolEngine' object is publicly 
 *           accessed using the 'INetworkServerProtocol' interface.
 *
 * @note     This class IS THREAD SAFE.\n
 *           The client object must call the 'ReleaseItf' method on 'INetworkServerProtocol'
 *           interface to destroy the implementation object created by 'CreateInstance'.
*********************************************************************************************/

#ifndef SEMTECHPROTOCOLENGINEITF_H
#define SEMTECHPROTOCOLENGINEITF_H

#include "NetworkServerProtocolItf.h"

// CSemtechProtocolEngine object factory
// This method in invoked by client objet to create a new instance of CSemtechProtocolEngine object
INetworkServerProtocol CSemtechProtocolEngine_CreateInstance();


#endif


