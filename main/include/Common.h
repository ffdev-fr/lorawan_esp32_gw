/*********************************************************************************************
PROJECT : LoRaWAN ESP32 Gateway V1.x
 
FILE    : Common.h
 
AUTHOR  : F.Fargon 
 
PURPOSE : This file contains common Espressif framework includes almost used in all C program 
          source files.
 
COMMENT : This file must be included in first position in all C source files. 
*********************************************************************************************/

#ifndef COMMON_H_
#define COMMON_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

# include <time.h> 
# include <sys/time.h> 

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "sdkconfig.h"

#include "Definitions.h"

#endif 
