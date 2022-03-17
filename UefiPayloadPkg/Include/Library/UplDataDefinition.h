/** @file
  Universal Payload data definition used by SetUplDataLib.h and GetUplDataLib.h

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UPL_DATA_DEFINITION_H__
#define __UPL_DATA_DEFINITION_H__

#include <Uefi/UefiSpec.h>
#include <UniversalPayload/PciRootBridges.h>
#include <UniversalPayload/ExtraData.h>

typedef struct {
  EFI_GUID                    Owner;
  EFI_RESOURCE_TYPE           ResourceType;
  EFI_RESOURCE_ATTRIBUTE_TYPE ResourceAttribute;
  EFI_PHYSICAL_ADDRESS        PhysicalStart;
  UINT64                      ResourceLength;
} UNIVERSAL_PAYLOAD_RESOURCE_DESCRIPTOR;

typedef struct {
  EFI_GUID                Name;
  EFI_PHYSICAL_ADDRESS    MemoryBaseAddress;
  UINT64                  MemoryLength;
  EFI_MEMORY_TYPE         MemoryType;
} UNIVERSAL_PAYLOAD_MEMORY_ALLOCATION;

#endif
