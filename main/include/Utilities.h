/*****************************************************************************************//**
 * @file     Utilities.h
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     04/03/2018
 *
 * @brief    Utility classes.
 *
 * @details  This file implements the following utility classes:\n
 *            - CMemoryBlockArray = Fixed size data blocks with quick allocation
*********************************************************************************************/

#ifndef UTILITIES_H_
#define UTILITIES_H_

/********************************************************************************************* 
  Definitions for debug traces
  The debug level is specified with 'UTILITIES_DEBUG_LEVEL' in Definitions.h file
*********************************************************************************************/

#define UTILITIES_DEBUG_LEVEL0 ((UTILITIES_DEBUG_LEVEL & 0x01) > 0)
#define UTILITIES_DEBUG_LEVEL1 ((UTILITIES_DEBUG_LEVEL & 0x02) > 0)
#define UTILITIES_DEBUG_LEVEL2 ((UTILITIES_DEBUG_LEVEL & 0x04) > 0)


/********************************************************************************************* 
  Definitions (implementation)
*********************************************************************************************/



/********************************************************************************************* 
 MemoryBlockArray Class

 Utility class for fixed size data blocks with quick allocation

 Notes: 
  - The maximum size of collection is 255 memory blocks
  - The object is thread safe

 WARNING: This object cannot be static. It MUST always be allocated by with the construction
          method ('CMemoryBlockArray_New')
*********************************************************************************************/

// Class data
typedef struct _CMemoryBlockArray
{
  // This collection is thread safe
  SemaphoreHandle_t m_hMutex;

  // Size of a single memory block 
  WORD m_wMemoryBlockSize;

  // Maximumn number of fixed size memory blocks
  BYTE m_usArraySize;

  // Head of free block list (LIFO, start index 0)
  // This is the index of the next free block
  // When head is equal to array size, all blocks are used
  // When head is 0, the array is empty (i.e. no memory block stored)
  BYTE m_usFreeBlockListHead;

  // Note: Keep the following member variables at the end of structure

  // Free memory block list (LIFO)
  // The list entry contains the index of free memory block in 'm_pMemoryBlockData'
  BYTE *m_pFreeBlockList;

  // Memory for data blocks
  BYTE *m_pMemoryBlockData;

  // Memory used block flags
  BYTE *m_pUsedBlockFlags;

  // Memory ready block flags
  BYTE *m_pReadyBlockFlags;

  // Note: Here is the beginning of storage space for data, list, used flags and ready flags
  //       (i.e. allocated within the 'CMemoryBlockArray' object)

} CMemoryBlockArrayOb;

typedef struct _CMemoryBlockArray * CMemoryBlockArray;


// Utility structure describing one entry in 'CMemoryBlockArray'
typedef struct _CMemoryBlockArrayEntry
{
  // Pointer to data block (i.e. allocated storage for entry)
  BYTE *m_pDataBlock;

  // Index of block in the array
  BYTE m_usBlockIndex;

} CMemoryBlockArrayEntryOb;

typedef struct _CMemoryBlockArrayEntry * CMemoryBlockArrayEntry;


// Utility structure for enumerator (parameter for 'EnumStart' and 'EnumNext' methods)
typedef struct _CMemoryBlockArrayEnumItem
{
  // Mode for returning the MemoryBlock data
  bool m_bByValue;

  // Index of retrieved block in the array
  BYTE m_usBlockIndex;

  // Pointer for associated data
  // The behavior depends on 'm_bByValue' parameter:
  //  - If 'm_bByValue' is true, the pointer is a memory buffer where enumerated item bytes are
  //    copied. The caller object must provide the buffer.
  //    Note: This buffer must have a minimal size of 'wBlockSize' value specified when array
  //          is created (see 'New' method)
  //  - If 'm_bByValue' is false, the 'm_pItemData' is set by the function to reference the
  //    storage buffer in the array
  BYTE *m_pItemData;

  // Private member
  // Note: This variable is used by the enumerator methods and caller must not modify it
  BYTE m_usEnumState;

} CMemoryBlockArrayEnumItemOb;

typedef struct _CMemoryBlockArrayEnumItem * CMemoryBlockArrayEnumItem;

// Class constants and definitions


// Class public methods

CMemoryBlockArray CMemoryBlockArray_New(WORD wBlockSize, BYTE usBlockNumber);
void CMemoryBlockArray_Delete(CMemoryBlockArray this);

void * CMemoryBlockArray_GetBlock(CMemoryBlockArray this, CMemoryBlockArrayEntry pEntry);
bool CMemoryBlockArray_ReleaseBlock(CMemoryBlockArray this, BYTE usBlockIndex);
bool CMemoryBlockArray_IsBlockUsed(CMemoryBlockArray this, BYTE usBlockIndex);
BYTE CMemoryBlockArray_BlockIndexFromPtr(CMemoryBlockArray this, void *pBlockPtr);
void * CMemoryBlockArray_BlockPtrFromIndex(CMemoryBlockArray this, BYTE usBlockIndex);
bool CMemoryBlockArray_IsBlockReady(CMemoryBlockArray this, BYTE usBlockIndex);
void CMemoryBlockArray_SetBlockReady(CMemoryBlockArray this, BYTE usBlockIndex);
bool CMemoryBlockArray_EnumStart(CMemoryBlockArray this, CMemoryBlockArrayEnumItem pEnumItem);
bool CMemoryBlockArray_EnumNext(CMemoryBlockArray this, CMemoryBlockArrayEnumItem pEnumItem);


// Class private methods



/********************************************************************************************* 
 Base64 functions

 Utility functions for Base64 encoding and decoding
*********************************************************************************************/

WORD Base64_BinToB64Nopad(const BYTE * in, WORD size, BYTE * out, WORD max_len); 
WORD Base64_B64ToBinNopad(const BYTE * in, WORD size, BYTE * out, WORD max_len);
WORD Base64_BinToB64(const BYTE * in, WORD size, BYTE * out, WORD max_len); 
WORD Base64_B64ToBin(const BYTE * in, WORD size, BYTE * out, WORD max_len);



#endif

