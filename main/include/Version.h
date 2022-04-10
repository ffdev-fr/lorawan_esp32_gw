/*********************************************************************************************
PROJECT : LoRaWAN ESP32 Gateway V1.x
 
FILE    : Version.h
 
AUTHOR  : F.Fargon 
 
PURPOSE : This file contains the version string.
          This string is sent on the debug UART (stdout) immediatly after user part of the
          application is entered (i.e. 'app_main' function).

COMMENTS: The version string MUST be updated during the build process of a new release. 
          This program is designed for execution on ESP32 Module (Dev.C kit). 
          The LoRa radio is implemented with Semtech SX1276 chip (Modtronix inAir9 module)  
          The implementation uses the Espressif IDF V3.0 framework (with RTOS)
*********************************************************************************************/

#ifndef LORAGW_VERSION_H
#define LORAGW_VERSION_H

/*********************************************************************************************
  Version history
*********************************************************************************************/

/*********************************************************************************************
 Version V1.0.0 RC1

 Feature summary:
  - 
  - 

 Release date:
  - 

 Deployment:
  - 

 Restrictions:
  - 
*********************************************************************************************/


/*********************************************************************************************
 Static Version String
 
 The version string format is 'Major.Minor.SP [RCx][GA] with:
 'Major'     = The major version number
 'Minor'     = The minor version number
 'SP'        = The service pack version number
 '[RCx][GA]' = The Release Candidate number or General Availablity tag
               (mutually exclusive)
*********************************************************************************************/

static char g_szVersionString[] = "1.0.0 Beta1";

#endif

