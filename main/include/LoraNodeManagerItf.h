/*********************************************************************************************
MODULE  : LoraNodeManagerItf

AUTHOR  : F.Fargon 
 
PURPOSE : This file contains the definition of the method used to instantiate a 'CLoraNodeManager'
          object.
          The 'CLoraNodeManager' object implements the generic 'ITransceiverManager' interface.
          Once instantiated the 'CLoraNodeManager' object is publicly accessed using the 
          'ITransceiverManager' interface.
 
COMMENTS: The client object must call the 'ReleaseItf' method on 'ITransceiverManager' interface 
          to destroy the 'CLoraNodeManager' object created by 'CreateInstance'.
*********************************************************************************************/

#ifndef LORANODEMANAGERITF_H
#define LORANODEMANAGERITF_H

#include "TransceiverManagerItf.h"

// CLoraNodeManager object factory
// This method in invoked by client objet to create a new instance of CLoraNodeManager object
ITransceiverManager CLoraNodeManager_CreateInstance();



#endif

