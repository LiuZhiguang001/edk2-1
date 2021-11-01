/** @file
 Define the structure for the Universal Payload Memory map.

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - Universal Payload Specification 0.75 (https://universalpayload.github.io/documentation/)
**/

#ifndef UNIVERSAL_PAYLOAD_MEMORY_MAP_H_
#define UNIVERSAL_PAYLOAD_MEMORY_MAP_H_

#include <Uefi.h>
#include <UniversalPayload/UniversalPayload.h>

#define EFI_MEMORY_PRESENT      0x0100000000000000ULL
#define EFI_MEMORY_INITIALIZED  0x0200000000000000ULL
#define EFI_MEMORY_TESTED       0x0400000000000000ULL

#pragma pack (1)

typedef struct {
  UNIVERSAL_PAYLOAD_GENERIC_HEADER   Header;
  UINT32                             Count;
  EFI_MEMORY_DESCRIPTOR              MemoryMapTable[0];
} UNIVERSAL_PAYLOAD_MEMORY_MAP;

#pragma pack()

#define UNIVERSAL_PAYLOAD_MEMORY_MAP_REVISION 1

extern GUID gUniversalPayloadMemoryMapGuid;
#endif
