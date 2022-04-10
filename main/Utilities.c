/*****************************************************************************************//**
 * @file     Utilities.c
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     04/03/2018
 *
 * @brief    Utility classes.
 *
 * @details  This file implements the following utility classes or functions:\n
 *            - CMemoryBlockArray = Fixed size data blocks with quick allocation
 *            - Base64 = Base64 encoding and decoding functions
*********************************************************************************************/


/*********************************************************************************************
  Espressif framework includes
*********************************************************************************************/

#include <Common.h>


/*********************************************************************************************
  Includes for objects implementation
*********************************************************************************************/

#include "Utilities.h"


/********************************************************************************************* 
 MemoryBlockArray Class

 Utility class for fixed size data blocks with quick allocation

 Notes: 
  - The maximum size of collection is 255 memory blocks
  - The object is thread safe

 WARNING: This object cannot be static. It MUST always be allocated by with the construction
          method ('CMemoryBlockArray_New')
*********************************************************************************************/
                   

CMemoryBlockArray CMemoryBlockArray_New(WORD wBlockSize, BYTE usBlockNumber)
{
  CMemoryBlockArray this;

  // Allocate memoty for the object
  // The memory for 'MemoryBlockData' and 'FreeBlockList' is allocated at the end of the object
  if ((this = (void *) pvPortMalloc(sizeof(CMemoryBlockArray) + (wBlockSize * usBlockNumber) +
      usBlockNumber + (((usBlockNumber / 8) + 1) * 2))) != NULL)
  {
    if ((this->m_hMutex = xSemaphoreCreateMutex()) == NULL)
    {
      CMemoryBlockArray_Delete(this);
      return NULL;
    }

    this->m_usArraySize = usBlockNumber;
    this->m_wMemoryBlockSize = wBlockSize;

    this->m_usFreeBlockListHead = 0;

    this->m_pFreeBlockList = ((BYTE *) this) + sizeof(CMemoryBlockArrayOb);
    this->m_pMemoryBlockData = this->m_pFreeBlockList + usBlockNumber;
    this->m_pUsedBlockFlags = this->m_pMemoryBlockData + (wBlockSize * usBlockNumber);
    this->m_pReadyBlockFlags = this->m_pUsedBlockFlags + ((usBlockNumber / 8) + 1);

    for (BYTE i = 0; i < usBlockNumber; i++)
    {
      this->m_pFreeBlockList[i] = i;
    }
    memset(this->m_pUsedBlockFlags, 0, ((usBlockNumber / 8) + 1) * 2);
  }

  #if (UTILITIES_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CMemoryBlockArray_New, block size: ");
    DEBUG_PRINT_DEC((unsigned int) wBlockSize);
    DEBUG_PRINT(", block num: ");
    DEBUG_PRINT_DEC((unsigned int) usBlockNumber);
    DEBUG_PRINT(", data ptr: ");
    DEBUG_PRINT_HEX((unsigned int) this->m_pMemoryBlockData);
    DEBUG_PRINT_CR;
  #endif

  return this;
}

void CMemoryBlockArray_Delete(CMemoryBlockArray this)
{
  if (this->m_hMutex != NULL)
  {
    vSemaphoreDelete(this->m_hMutex);
  }
  vPortFree(this);
}


void * CMemoryBlockArray_GetBlock(CMemoryBlockArray this, CMemoryBlockArrayEntry pEntry)
{
  BYTE *pFlags;

  if (xSemaphoreTake(this->m_hMutex, pdMS_TO_TICKS(500)) == pdFAIL)
  {
    // Should never occur
    #if (UTILITIES_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CMemoryBlockArray_GetBlock - Failed to take mutex");
    #endif

    return false;
  }

  #if (UTILITIES_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CMemoryBlockArray_GetBlock, head: ");
    DEBUG_PRINT_HEX((unsigned int) this->m_usFreeBlockListHead);
    DEBUG_PRINT(", FreeBlockList: ");
    for (WORD i = 0; i < this->m_usArraySize; i++)
    {
      DEBUG_PRINT_HEX((unsigned int) this->m_pFreeBlockList[i]);
      DEBUG_PRINT(", ");
    }
    DEBUG_PRINT_CR;
  #endif


  if (this->m_usFreeBlockListHead != this->m_usArraySize)
  {
    // Provide next block
    pEntry->m_usBlockIndex = this->m_pFreeBlockList[this->m_usFreeBlockListHead];
    pEntry->m_pDataBlock = this->m_pMemoryBlockData + (this->m_wMemoryBlockSize * pEntry->m_usBlockIndex);
    ++this->m_usFreeBlockListHead;

    // Set used block flag
    pFlags = this->m_pUsedBlockFlags + (pEntry->m_usBlockIndex / 8);
    *pFlags |= 0b10000000 >> pEntry->m_usBlockIndex % 8;
  }
  else
  {
    // All entries are used
    pEntry->m_pDataBlock = NULL;
  }

  xSemaphoreGive(this->m_hMutex);

  #if (UTILITIES_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CMemoryBlockArray_GetBlock, index: ");
    DEBUG_PRINT_HEX((unsigned int) pEntry->m_usBlockIndex);
    DEBUG_PRINT(", ptr: ");
    DEBUG_PRINT_HEX((unsigned int) pEntry->m_pDataBlock);
    DEBUG_PRINT_CR;
  #endif

  return pEntry->m_pDataBlock;
}

bool CMemoryBlockArray_ReleaseBlock(CMemoryBlockArray this, BYTE usBlockIndex)
{
  bool bResult;
  BYTE *pFlags;

  if (xSemaphoreTake(this->m_hMutex, pdMS_TO_TICKS(500)) == pdFAIL)
  {
    // Should never occur
    #if (UTILITIES_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CMemoryBlockArray_ReleaseBlock - Failed to take mutex");
    #endif

    return false;
  }

  if (this->m_usFreeBlockListHead != 0)
  {
    // Add released block in free list
    this->m_pFreeBlockList[--this->m_usFreeBlockListHead] = usBlockIndex;

    // Clear used block flag
    pFlags = this->m_pUsedBlockFlags + (usBlockIndex / 8);
    *pFlags &= ~(0b10000000 >> usBlockIndex % 8);

    // Clear ready block flag
    pFlags = this->m_pReadyBlockFlags + (usBlockIndex / 8);
    *pFlags &= ~(0b10000000 >> usBlockIndex % 8);

    bResult = true;
  }
  else
  {
    // Should never occur. 
    // Implementation error on collection  usage (typically blocks released more than once)
    bResult = false;
    #if (UTILITIES_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CMemoryBlockArray_ReleaseBlock - All blocks already released");
    #endif
  }

  xSemaphoreGive(this->m_hMutex);
  return bResult;
}


bool CMemoryBlockArray_IsBlockUsed(CMemoryBlockArray this, BYTE usBlockIndex)
{
  // Note: Read operation on atomic variable (i.e. mutex not required)
  return (this->m_pUsedBlockFlags[usBlockIndex / 8] & (0b10000000 >> usBlockIndex % 8)) != 0 ? true : false;
}


BYTE CMemoryBlockArray_BlockIndexFromPtr(CMemoryBlockArray this, void *pBlockPtr)
{
  return (BYTE)((((BYTE*)pBlockPtr) - this->m_pMemoryBlockData) / this->m_wMemoryBlockSize);
}

void * CMemoryBlockArray_BlockPtrFromIndex(CMemoryBlockArray this, BYTE usBlockIndex)
{
  return this->m_pMemoryBlockData + (usBlockIndex * this->m_wMemoryBlockSize);
}

bool CMemoryBlockArray_IsBlockReady(CMemoryBlockArray this, BYTE usBlockIndex)
{
  BYTE *pFlags;

  // Ready block flag
  // Note: Read operation on atomic variable (i.e. mutex not required)
  pFlags = this->m_pReadyBlockFlags + (usBlockIndex / 8);

  #if (UTILITIES_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CMemoryBlockArray_IsBlockReady BlockIndex: ");
    DEBUG_PRINT_HEX((unsigned int) usBlockIndex);
    DEBUG_PRINT(" , pFlags: ");
    DEBUG_PRINT_HEX((unsigned int) pFlags);
    DEBUG_PRINT(" , Flags value: ");
    DEBUG_PRINT_HEX((unsigned int) *pFlags);
    DEBUG_PRINT_CR;
  #endif

  return (*pFlags & (0b10000000 >> usBlockIndex % 8)) != 0 ? true : false;
}

void CMemoryBlockArray_SetBlockReady(CMemoryBlockArray this, BYTE usBlockIndex)
{
  BYTE *pFlags;

  // Set ready block flag
  pFlags = this->m_pReadyBlockFlags + (usBlockIndex / 8);
  *pFlags |= 0b10000000 >> usBlockIndex % 8;

  #if (UTILITIES_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] CMemoryBlockArray_SetBlockReady BlockIndex: ");
    DEBUG_PRINT_HEX((unsigned int) usBlockIndex);
    DEBUG_PRINT(" , pFlags: ");
    DEBUG_PRINT_HEX((unsigned int) pFlags);
    DEBUG_PRINT(" , Flags value: ");
    DEBUG_PRINT_HEX((unsigned int) *pFlags);
    DEBUG_PRINT_CR;
  #endif

}

//
// Enumerator methods
//
// This implementation is based on optimistic rules:
//  - The contents of array will likely not be modified during enumeration (i.e. no entry added
//    or removed)
//  - The array is locked on each call to guarantee the integrity of returned entry.
//    This will allow other tasks to access the array without delay.
//  - The entry is returned by value.
//  - Typically the caller object will retrieve the reference of returned object later.
//    This assumes that caller object is designed in order to be sure that retrieved object
//    is still valid
//

bool CMemoryBlockArray_EnumStart(CMemoryBlockArray this, CMemoryBlockArrayEnumItem pEnumItem)
{
  BYTE usBlockIndex;

  if (xSemaphoreTake(this->m_hMutex, pdMS_TO_TICKS(500)) == pdFAIL)
  {
    // Should never occur
    #if (UTILITIES_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CMemoryBlockArray_EnumStart - Failed to take mutex");
    #endif
    return false;
  }

  // If array not empty, enumerate entries and provide first used block
  if (this->m_usFreeBlockListHead != 0)
  {
    for (usBlockIndex = 0; usBlockIndex < this->m_usArraySize; usBlockIndex++)
    {
      if ((this->m_pUsedBlockFlags[usBlockIndex / 8] & (0b10000000 >> usBlockIndex % 8)) != 0)
      {
        if ((this->m_pReadyBlockFlags[usBlockIndex / 8] & (0b10000000 >> usBlockIndex % 8)) != 0)
        {
          if (pEnumItem->m_bByValue == true)
          {
            memcpy(pEnumItem->m_pItemData, this->m_pMemoryBlockData + (usBlockIndex * this->m_wMemoryBlockSize),
                   this->m_wMemoryBlockSize);
          }
          else
          {
            pEnumItem->m_pItemData = this->m_pMemoryBlockData + (usBlockIndex * this->m_wMemoryBlockSize);
          }
          xSemaphoreGive(this->m_hMutex);
          pEnumItem->m_usBlockIndex = usBlockIndex;
          pEnumItem->m_usEnumState = usBlockIndex + 1;
          return true;
        }
      }
    }
  }

  xSemaphoreGive(this->m_hMutex);
  return false;
}

bool CMemoryBlockArray_EnumNext(CMemoryBlockArray this, CMemoryBlockArrayEnumItem pEnumItem)
{
  BYTE usBlockIndex;

  // Check for end of enumeration
  if (pEnumItem->m_usEnumState >= this->m_usArraySize)
  {
    // Enumeration terminated
    return false;
  }

  if (xSemaphoreTake(this->m_hMutex, pdMS_TO_TICKS(500)) == pdFAIL)
  {
    // Should never occur
    #if (UTILITIES_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] CMemoryBlockArray_EnumNext - Failed to take mutex");
    #endif
    return false;
  }

  // If array not empty, enumerate entries and provide first used block
  for (usBlockIndex = pEnumItem->m_usEnumState; usBlockIndex < this->m_usArraySize; usBlockIndex++)
  {
    if ((this->m_pUsedBlockFlags[usBlockIndex / 8] & (0b10000000 >> usBlockIndex % 8)) != 0)
    {
      if ((this->m_pReadyBlockFlags[usBlockIndex / 8] & (0b10000000 >> usBlockIndex % 8)) != 0)
      {
        if (pEnumItem->m_bByValue == true)
        {
          memcpy(pEnumItem->m_pItemData, this->m_pMemoryBlockData + (usBlockIndex * this->m_wMemoryBlockSize),
                 this->m_wMemoryBlockSize);
        }
        else
        {
          pEnumItem->m_pItemData = this->m_pMemoryBlockData + (usBlockIndex * this->m_wMemoryBlockSize);
        }
        xSemaphoreGive(this->m_hMutex);
        pEnumItem->m_usBlockIndex = usBlockIndex;
        pEnumItem->m_usEnumState = usBlockIndex + 1;
        return true;
      }
    }
  }

  xSemaphoreGive(this->m_hMutex);
  return false;
}

/********************************************************************************************* 
 Base64 functions

 Utility functions for Base64 encoding and decoding
*********************************************************************************************/

// Private variables for Base64 functions

static char Base64_code_62 = '+';    // RFC 1421 standard character for code 62 
static char Base64_code_63 = '/';    // RFC 1421 standard character for code 63 
static char Base64_code_pad = '=';   // RFC 1421 padding character if padding 

static bool g_bBase64Error;

// Forward declarations
BYTE Base64_CodeToChar(BYTE x);
BYTE Base64_CharToCode(BYTE x);

//
// Private functions
//

BYTE Base64_CodeToChar(BYTE x)
{
  if (x <= 25) 
  {
    return 'A' + x;
  } 
  else if ((x >= 26) && (x <= 51)) 
  {
    return 'a' + (x-26);
  }
  else if ((x >= 52) && (x <= 61))
  {
    return '0' + (x-52);
  }
  else if (x == 62)
  {
    return Base64_code_62;
  }
  else if (x == 63) 
  {
    return Base64_code_63;
  } 

  // Should never occur
  #if (UTILITIES_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[ERROR] Base64_CodeToChar - Byte out of range 0-64 for encoding");
  #endif

  //TODO: improve error management
  g_bBase64Error = true;
  
  return 0x00; 
}

BYTE Base64_CharToCode(BYTE x)
{
  if ((x >= 'A') && (x <= 'Z')) 
  {
    return (BYTE)x - (BYTE)'A';
  } 
  else if ((x >= 'a') && (x <= 'z'))
  {
    return (BYTE)x - (BYTE)'a' + 26;
  } 
  else if ((x >= '0') && (x <= '9')) 
  {
    return (BYTE)x - (BYTE)'0' + 52;
  } 
  else if (x == Base64_code_62) 
  {
    return 62;
  }
  else if (x == Base64_code_63)
  {
    return 63;
  }

  // Should never occur
  #if (UTILITIES_DEBUG_LEVEL0)
    DEBUG_PRINT_LN("[ERROR] Base64_CharToCode - Invalid character for decoding");
  #endif

  //TODO: improve error management
  g_bBase64Error = true;
  
  return 0x00;
}

//
// Public functions
//

WORD Base64_BinToB64Nopad(const BYTE * in, WORD size, BYTE * out, WORD max_len) 
{
  int i;
  WORD result_len;  // size of the result 
  int full_blocks;  // number of 3 unsigned chars / 4 characters blocks 
  int last_bytes;   // number of unsigned chars <3 in the last block
  int last_chars;   // number of characters <4 in the last block 
  uint32_t b;

  // Check input values
  if (size == 0) 
  {
    *out = 0;  // null string 
    return 0;
  }

  // Calculate the number of base64 'blocks' 
  last_chars = 0;
  full_blocks = size / 3;
  last_bytes = size % 3;
  switch (last_bytes) 
  {
    case 1: 
      // 1 byte left to encode -> +2 chars 
      last_chars = 2;
      break;
    case 2:
      // 2 bytes left to encode -> +3 chars 
      last_chars = 3;
      break;
  }

  // Check if output buffer is big enough 
  result_len = (4*full_blocks) + last_chars;

  #if (UTILITIES_DEBUG_LEVEL2)
    DEBUG_PRINT("[DEBUG] Base64_BinToB64Nopad - Data size: ");
    DEBUG_PRINT_DEC(size);
    DEBUG_PRINT(", Encoded size: ");
    DEBUG_PRINT_DEC(result_len);
    DEBUG_PRINT(", Available size: ");
    DEBUG_PRINT_DEC(max_len);
    DEBUG_PRINT_CR;
  #endif

  if (max_len < (result_len + 1)) 
  { 
    // 1 char added for string terminator 
    #if (UTILITIES_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Base64_BinToB64Nopad - Output buffer too small");
    #endif
    return 0xFFFF;
  }

  g_bBase64Error = false;

  // Process all the full blocks
  for (i=0; i < full_blocks; ++i)
  {
    b  = (0xFF & in[3*i]) << 16;
    b |= (0xFF & in[3*i + 1]) << 8;
    b |=  0xFF & in[3*i + 2];
    out[4*i + 0] = Base64_CodeToChar((b >> 18) & 0x3F);
    out[4*i + 1] = Base64_CodeToChar((b >> 12) & 0x3F);
    out[4*i + 2] = Base64_CodeToChar((b >> 6 ) & 0x3F);
    out[4*i + 3] = Base64_CodeToChar( b & 0x3F);
  }

  // Process the last 'partial' block and terminate string 
  i = full_blocks;
  if (last_chars == 0) 
  {
    out[4*i] = 0; // null character to terminate string 
  } 
  else if (last_chars == 2) 
  {
    b = (0xFF & in[3*i]) << 16;
    out[4*i + 0] = Base64_CodeToChar((b >> 18) & 0x3F);
    out[4*i + 1] = Base64_CodeToChar((b >> 12) & 0x3F);
    out[4*i + 2] =  0; // null character to terminate string 
  } 
  else if (last_chars == 3) 
  {
    b = (0xFF & in[3*i]) << 16;
    b |= (0xFF & in[3*i + 1]) << 8;
    out[4*i + 0] = Base64_CodeToChar((b >> 18) & 0x3F);
    out[4*i + 1] = Base64_CodeToChar((b >> 12) & 0x3F);
    out[4*i + 2] = Base64_CodeToChar((b >> 6 ) & 0x3F);
    out[4*i + 3] = 0; // null character to terminate string 
  }
  return g_bBase64Error == true ? 0xFFFF : result_len;
}

WORD Base64_B64ToBinNopad(const BYTE * in, WORD size, BYTE * out, WORD max_len)
{
  int i;
  WORD result_len;  // size of the result 
  int full_blocks;  // number of 3 unsigned chars / 4 characters blocks 
  int last_chars;   // number of characters <4 in the last block 
  int last_bytes;   // number of unsigned chars <3 in the last block 
  uint32_t b;

  // Check input values
  if (size == 0) 
  {
    return 0;
  }

  // Calculate the number of base64 'blocks'
  last_bytes = 0;
  full_blocks = size / 4;
  last_chars = size % 4;
  switch (last_chars) 
  {
    case 1: 
      // only 1 char left is an error
      #if (UTILITIES_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Base64_B64ToBinNopad - Only one char left");
      #endif
      return 0xFFFF;
    case 2: 
      // 2 chars left to decode -> +1 byte 
      last_bytes = 1;
      break;
    case 3: 
      // 3 chars left to decode -> +2 bytes 
      last_bytes = 2;
      break;
  }

  // check if output buffer is big enough 
  result_len = (3*full_blocks) + last_bytes;
  if (max_len < result_len) 
  {
    #if (UTILITIES_DEBUG_LEVEL0)
      DEBUG_PRINT_LN("[ERROR] Base64_B64ToBinNopad - Output buffer too small");
    #endif
    return 0xFFFF;
  }

  // Process all the full blocks 
  for (i=0; i < full_blocks; ++i) 
  {
    b = (0x3F & Base64_CharToCode(in[4*i])) << 18;
    b |= (0x3F & Base64_CharToCode(in[4*i + 1])) << 12;
    b |= (0x3F & Base64_CharToCode(in[4*i + 2])) << 6;
    b |=  0x3F & Base64_CharToCode(in[4*i + 3]);
    out[3*i + 0] = (b >> 16) & 0xFF;
    out[3*i + 1] = (b >> 8 ) & 0xFF;
    out[3*i + 2] =  b & 0xFF;
  }

  // process the last 'partial' block 
  i = full_blocks;
  if (last_bytes == 1) 
  {
    b = (0x3F & Base64_CharToCode(in[4*i])) << 18;
    b |= (0x3F & Base64_CharToCode(in[4*i + 1])) << 12;
    out[3*i + 0] = (b >> 16) & 0xFF;
    if (((b >> 12) & 0x0F) != 0) 
    {
      #if (UTILITIES_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[WARNING] Base64_B64ToBinNopad - Last character contains unusable bits");
      #endif
    }
  } 
  else if (last_bytes == 2) 
  {
    b = (0x3F & Base64_CharToCode(in[4*i])) << 18;
    b |= (0x3F & Base64_CharToCode(in[4*i + 1])) << 12;
    b |= (0x3F & Base64_CharToCode(in[4*i + 2])) << 6;
    out[3*i + 0] = (b >> 16) & 0xFF;
    out[3*i + 1] = (b >> 8) & 0xFF;
    if (((b >> 6) & 0x03) != 0) 
    {
      #if (UTILITIES_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[WARNING] Base64_B64ToBinNopad - Last character contains unusable bits");
      #endif
    }
  }
  return g_bBase64Error == true ? 0xFFFF : result_len;
}

WORD Base64_BinToB64(const BYTE * in, WORD size, BYTE * out, WORD max_len) 
{
  WORD ret;

  ret = Base64_BinToB64Nopad(in, size, out, max_len);

  if (ret == 0xFFFF)
  {
    return 0xFFFF;
  }

  switch (ret%4) 
  {
    case 0:
      // Nothing to do
      return ret;
    case 1:
      #if (UTILITIES_DEBUG_LEVEL0)
        DEBUG_PRINT_LN("[ERROR] Base64_BinToB64 - Invalid unpadded base64 string");
      #endif
      return 0xFFFF;
    case 2: 
      // 2 chars in last block, must add 2 padding char 
      if (max_len >= (ret + 2 + 1)) 
      {
        out[ret] = Base64_code_pad;
        out[ret+1] = Base64_code_pad;
        out[ret+2] = 0;
        return ret+2;
      } 
      else 
      {
        #if (UTILITIES_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[ERROR] Base64_BinToB64 - Not enough room to add padding");
        #endif
        return 0xFFFF;
      }
    case 3: 
      // 3 chars in last block, must add 1 padding char 
      if (max_len >= (ret + 1 + 1)) 
      {
        out[ret] = Base64_code_pad;
        out[ret+1] = 0;
        return ret+1;
      } 
      else 
      {
        #if (UTILITIES_DEBUG_LEVEL0)
          DEBUG_PRINT_LN("[ERROR] Base64_BinToB64 - Not enough room to add padding");
        #endif
        return 0xFFFF;
      }
  }
  return 0xFFFF;
}

WORD Base64_B64ToBin(const BYTE * in, WORD size, BYTE * out, WORD max_len)
{
  if ((size%4 == 0) && (size >= 4))
  { 
    // Potentially padded Base64 
    if (in[size-2] == Base64_code_pad) 
    { 
      // 2 padding char to ignore
      size -= 2;
    } 
    else if (in[size-1] == Base64_code_pad) 
    {
      // 1 padding char to ignore 
      size -= 1;
    } 
  } 
  return Base64_B64ToBinNopad(in, size, out, max_len);
}

