/** @file
  BROTLI UEFI header file for definitions

  Allows BROTLI code to build under UEFI (edk2) build environment

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PROTO11_H__
#define __PROTO11_H__

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#define memcpy                      CopyMem
#define memset(dest,ch,count)       SetMem(dest,(UINTN)(count),(UINT8)(ch))
#define strlen(str)                 (size_t)(AsciiStrnLenS(str,MAX_STRING_SIZE))

#define memcmp(buf1,buf2,count)           (int)(CompareMem(buf1,buf2,(UINTN)(count)))
#define memmove(dest,source,count)        CopyMem(dest,source,(UINTN)(count))
#define strcpy(strDest,strSource)         AsciiStrCpyS(strDest,MAX_STRING_SIZE,strSource)
#define strncpy(strDest,strSource,count)  AsciiStrnCpyS(strDest,MAX_STRING_SIZE,strSource,(UINTN)count)
#define atoi(str)                         AsciiStrDecimalToUintn(str)
#define strcmp(str1, str2)                AsciiStrCmp(str1,str2)
#define assert(expression)

#define CHAR_BIT    8       /* Number of bits in a char */

typedef INT8     int8_t;
typedef INT16    int16_t;
typedef INT32    int32_t;
typedef INT64    int64_t;
typedef UINT8    uint8_t;
typedef UINT16   uint16_t;
typedef UINT32   uint32_t;
typedef UINT64   uint64_t;
typedef UINTN    size_t;
typedef BOOLEAN  bool;

#define false FALSE
#define true TRUE



typedef INT8     int_least8_t;
typedef INT16    int_least16_t;
typedef INT32    int_least32_t;
typedef INT64    int_least64_t;
typedef UINT8    uint_least8_t;
typedef UINT16   uint_least16_t;
typedef UINT32   uint_least32_t;
typedef UINT64   uint_least64_t;
typedef UINT8    uint_fast8_t;

#define offsetof(type,member)             OFFSET_OF(type,member)



typedef struct q_useful_buf_c {
    const void *ptr;
    size_t      len;
} UsefulBufC;



#endif
