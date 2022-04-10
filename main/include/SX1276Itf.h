/*********************************************************************************************
MODULE  : SX1276Itf

AUTHOR  : F.Fargon 
 
PURPOSE : This file contains the definition of the method used to instantiate a 'CSX1276' object.
          The 'CSX1276' object implements the generic 'ILoraTransceiver' interface.
          Once instantiated the 'CSX1276' object is publicly accessed using the 'ILoraTransceiver'
          interface.
 
COMMENTS: The client object must call the 'ReleaseItf' method on 'ILoraTransceiver' interface 
          to destroy the 'CSX1276' object created by 'CreateInstance'.
*********************************************************************************************/

#ifndef SX1276ITF_H
#define SX1276ITF_H

#include "LoraTransceiverItf.h"

// CSX1276 object factory
// This method in invoked by client objet to create a new instance of CSX1276 object
ILoraTransceiver CSX1276_CreateInstance();



#endif

