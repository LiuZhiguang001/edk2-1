/** @file
  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "UefiPayloadEntry.h"
#include <Library/HobLib.h>
#include <Library/SortLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

#define MAX_SIZE_FOR_SORT  30
typedef struct {
  UINT64   Attribute;
  UINT64   Capability;
} ATTRIBUTE_CONVERSION_ENTRY;

typedef struct {
  UINTN                            Count;
  EFI_PHYSICAL_ADDRESS             CurrentEnd;
  UINT64                           Attribute;
  EFI_MEMORY_TYPE                  MemoryType;
  BOOLEAN                          NewElement;
} CURRENT_INFO;

CONST ATTRIBUTE_CONVERSION_ENTRY mAttributeConversionTable[] = {
  { EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE,             EFI_MEMORY_UC           },
  { EFI_RESOURCE_ATTRIBUTE_UNCACHED_EXPORTED,       EFI_MEMORY_UCE          },
  { EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE,       EFI_MEMORY_WC           },
  { EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE, EFI_MEMORY_WT           },
  { EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE,    EFI_MEMORY_WB           },
  { EFI_RESOURCE_ATTRIBUTE_READ_PROTECTABLE,        EFI_MEMORY_RP           },
  { EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTABLE,       EFI_MEMORY_WP           },
  { EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTABLE,   EFI_MEMORY_XP           },
  { EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTABLE,   EFI_MEMORY_RO           },
  { EFI_RESOURCE_ATTRIBUTE_PRESENT,                 EFI_MEMORY_PRESENT      },
  { EFI_RESOURCE_ATTRIBUTE_INITIALIZED,             EFI_MEMORY_INITIALIZED  },
  { EFI_RESOURCE_ATTRIBUTE_TESTED,                  EFI_MEMORY_TESTED       },
  { EFI_RESOURCE_ATTRIBUTE_PERSISTABLE,             EFI_MEMORY_NV           },
  { EFI_RESOURCE_ATTRIBUTE_MORE_RELIABLE,           EFI_MEMORY_MORE_RELIABLE},
  { 0,                                              0                       }
};

/**
  Function to compare 2 Memory Allocation Hob.

  @param[in] Buffer1            pointer to the firstMemory Allocation Hob to compare
  @param[in] Buffer2            pointer to the second Memory Allocation Hob to compare

  @retval 0                     Buffer1 equal to Buffer2
  @retval <0                    Buffer1 is less than Buffer2
  @retval >0                    Buffer1 is greater than Buffer2
**/
INTN
EFIAPI
MemoryAllocationHobCompare (
  IN CONST VOID             *Buffer1,
  IN CONST VOID             *Buffer2
  )
{
  EFI_HOB_MEMORY_ALLOCATION *Left;
  EFI_HOB_MEMORY_ALLOCATION *Right;

  Left  = *(EFI_HOB_MEMORY_ALLOCATION**)Buffer1;
  Right = *(EFI_HOB_MEMORY_ALLOCATION**)Buffer2;

  if (Left->AllocDescriptor.MemoryBaseAddress == Right->AllocDescriptor.MemoryBaseAddress) {
    return 0;
  } else if (Left->AllocDescriptor.MemoryBaseAddress > Right->AllocDescriptor.MemoryBaseAddress) {
    return 1;
  } else {
    return -1;
  }
}

/**
  Converts a Resource Descriptor HOB attributes to an EFI Memory Descriptor capabilities

  @param  Attributes             The attribute mask in the Resource Descriptor HOB.

  @return The capabilities mask for an EFI Memory Descriptor.
**/
UINT64
ConvertResourceDescriptorHobAttributesToCapabilities (
  UINT64               Attributes
  )
{
  UINT64                      Capabilities;
  ATTRIBUTE_CONVERSION_ENTRY  *Conversion;
  //
  // Convert the Resource HOB Attributes to an EFI Memory Capabilities mask
  //
  for (Capabilities = 0, Conversion = (ATTRIBUTE_CONVERSION_ENTRY  *)mAttributeConversionTable; Conversion->Attribute != 0; Conversion++) {
    if ((Attributes & Conversion->Attribute) != 0) {
      Capabilities |= Conversion->Capability;
    }
  }
  return Capabilities;
}

/**
  Converts a Resource Descriptor HOB type to an EFI Memory Descriptor type

  @param  Hob                The pointer to a Resource Descriptor HOB.

  @return The memory type for an EFI Memory Descriptor.
**/
EFI_MEMORY_TYPE
ConvertResourceDescriptorHobResourceTypeToEfiMemoryType (
  EFI_RESOURCE_TYPE               Type
  )
{
  ASSERT (Type == EFI_RESOURCE_SYSTEM_MEMORY || Type == EFI_RESOURCE_MEMORY_RESERVED);
  switch (Type) {
  case EFI_RESOURCE_SYSTEM_MEMORY:
    return EfiConventionalMemory;
  case EFI_RESOURCE_MEMORY_RESERVED:
    return EfiReservedMemoryType;
  default:
    return EfiMaxMemoryType;
  }
}

/**
  Get the K smalllest memory allocation hobs which is inside [Base, Limit] range.
  We treat Hob list like a fake memory allocation, which should also be sorted together
  with other memory allocation hobs.

  @param  MemoryAllocationHobPtr  The pointer to a memory range that can contain K memory allocation hob pointers
  @param  K                       The number of memory allocation hobs we want to get
  @param  Base                    The base of [Base, Limit] range.
  @param  Limit                   The limit of [Base, Limit] range.
  @param  FakeMemoryAllocationHob A fake memory allocation contains Hob list.

  @return The number of memory allocation hobs we get
**/
UINTN
GetSmallestKMemoryAllocationHob (
  OUT EFI_HOB_MEMORY_ALLOCATION   **MemoryAllocationHobPtr,
  IN  UINTN                       K,
  IN  EFI_PHYSICAL_ADDRESS        Base,
  IN  EFI_PHYSICAL_ADDRESS        Limit,
  IN  EFI_HOB_MEMORY_ALLOCATION   *HobListMemoryAllocation
  )
{
  EFI_PEI_HOB_POINTERS        Hob;
  UINTN                       MemoryAllocationHobCount;
  EFI_HOB_MEMORY_ALLOCATION   *SortBuffer;
  BOOLEAN                     EnumAllHobs;
  EnumAllHobs = FALSE;

  //
  // Enum all hobs, including the fake memory allocation hob which contains the hob list.
  //
  Hob.Raw = (UINT8 *) (UINTN) HobListMemoryAllocation->AllocDescriptor.MemoryBaseAddress;

  MemoryAllocationHobCount = 0;

  if ((HobListMemoryAllocation->AllocDescriptor.MemoryBaseAddress >= Base) &&
    (HobListMemoryAllocation->AllocDescriptor.MemoryBaseAddress + HobListMemoryAllocation->AllocDescriptor.MemoryLength <= Limit)) {
    MemoryAllocationHobPtr[MemoryAllocationHobCount++] = HobListMemoryAllocation;
  }

  while (GET_HOB_TYPE (Hob) != EFI_HOB_TYPE_END_OF_HOB_LIST && (MemoryAllocationHobCount != K)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      if ((Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress >= Base) &&
        (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress + Hob.MemoryAllocation->AllocDescriptor.MemoryLength <= Limit)) {
        MemoryAllocationHobPtr[MemoryAllocationHobCount++] = Hob.MemoryAllocation;
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob.Raw);
  }
  QuickSort (MemoryAllocationHobPtr, MemoryAllocationHobCount, sizeof (*MemoryAllocationHobPtr), MemoryAllocationHobCompare, &SortBuffer);

  while (GET_HOB_TYPE (Hob) != EFI_HOB_TYPE_END_OF_HOB_LIST) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      if ((Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress >= Base) &&
        (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress + Hob.MemoryAllocation->AllocDescriptor.MemoryLength <= Limit)) {
          if (MemoryAllocationHobCompare(&MemoryAllocationHobPtr[MemoryAllocationHobCount - 1], &Hob.MemoryAllocation) == 1) {
            MemoryAllocationHobPtr[MemoryAllocationHobCount - 1] = Hob.MemoryAllocation;
            QuickSort (MemoryAllocationHobPtr, K, sizeof (*MemoryAllocationHobPtr), MemoryAllocationHobCompare, &SortBuffer);
          }
      }
    }
    Hob.Raw = GET_NEXT_HOB (Hob.Raw);
  }

  return MemoryAllocationHobCount;
}

/**
  Get the smalllest resource descriptor hob which is larger than Base.

  @param  Base                    The base address that resource descriptor hob should be larger than.

  @return The smalllest resource descriptor hob which is larger than Base,
          or Null if there is no resource descriptor hob meet the requirement.
**/
EFI_HOB_RESOURCE_DESCRIPTOR*
GetSmallestResourceHob (
  IN  EFI_PHYSICAL_ADDRESS        Base
  )
{
  EFI_PEI_HOB_POINTERS          Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR*  SmallestResourceHob;
  SmallestResourceHob = NULL;

  Hob.Raw = GetHobList();
  while (!END_OF_HOB_LIST (Hob)) {
    if ((Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) &&
        (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY || Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_RESERVED)) {
      if (Hob.ResourceDescriptor->PhysicalStart < Base) {
        Hob.Raw = GET_NEXT_HOB (Hob);
        continue;
      }
      if (SmallestResourceHob == NULL || SmallestResourceHob->PhysicalStart > Hob.ResourceDescriptor->PhysicalStart) {
        SmallestResourceHob = Hob.ResourceDescriptor;
      }
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  return SmallestResourceHob;
}

/**
  Append a range to the Memory map table.
  If the MemoryBaseAddress NULL, only change the CurrentInfo

  @param  CurrentInfo             The information about the current status.
  @param  MemoryMap          The pointer to MemoryMap.
  @param  MemoryBaseAddress       The base address of the memory range.
  @param  PageNumber              The PageNumbe of the memory range..
  @param  Attribute               The Attribute of the memory range..
  @param  Type                    The Type of the memory range..

  @return The smalllest resource descriptor hob which is larger than Base,
          or Null if there is no resource descriptor hob meet the requirement.
**/
VOID
AppendEfiMemoryDescriptor (
  CURRENT_INFO                  *CurrentInfo,
  EFI_MEMORY_DESCRIPTOR         *MemoryMap,
  EFI_PHYSICAL_ADDRESS          MemoryBaseAddress,
  UINT64                        PageNumber,
  UINT64                        Attribute,
  EFI_MEMORY_TYPE               Type
  )
{
  if ((CurrentInfo->CurrentEnd == MemoryBaseAddress) &&
      (CurrentInfo->MemoryType == Type) &&
      (CurrentInfo->Attribute == Attribute) &&
      (CurrentInfo->NewElement == FALSE))
  {
    ASSERT (CurrentInfo->Count != 0);
    CurrentInfo->CurrentEnd += EFI_PAGES_TO_SIZE (PageNumber);
    if (MemoryMap != NULL) {
      MemoryMap[CurrentInfo->Count - 1].NumberOfPages += PageNumber;
    }
  } else  {
    if (MemoryMap != NULL) {
      MemoryMap[CurrentInfo->Count].Attribute = Attribute;
      MemoryMap[CurrentInfo->Count].VirtualStart = 0;
      MemoryMap[CurrentInfo->Count].Type = Type;
      MemoryMap[CurrentInfo->Count].PhysicalStart = MemoryBaseAddress;
      MemoryMap[CurrentInfo->Count].NumberOfPages = PageNumber;
    }
    CurrentInfo->Count++;
    CurrentInfo->MemoryType = Type;
    CurrentInfo->Attribute = Attribute;
    CurrentInfo->CurrentEnd = MemoryBaseAddress + EFI_PAGES_TO_SIZE (PageNumber);
  }
  CurrentInfo->NewElement = FALSE;
}

/**
  Create a Guid Hob containing the memory map descriptor table.
  After calling the function, PEI should not do any memory allocation operation.

  @retval EFI_SUCCESS           The Memory Map is created successfully.
**/
EFI_STATUS
EFIAPI
BuildMemoryMap (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS             Hob;
  UINTN                            MemoryAllocationHobCount;
  EFI_PHYSICAL_ADDRESS             ResourceHobStart;
  EFI_PHYSICAL_ADDRESS             ResourceHobEnd;
  EFI_PHYSICAL_ADDRESS             OverlappedLength;
  EFI_HOB_RESOURCE_DESCRIPTOR      *CurrentResourceHob;
  CURRENT_INFO                     CurrentInfo;
  UINT64                           DefaultMemoryAttribute;
  UINTN                            Index;
  UNIVERSAL_PAYLOAD_MEMORY_MAP     *MemoryMapHob;
  EFI_MEMORY_TYPE                  DefaultMemoryType;
  EFI_HOB_MEMORY_ALLOCATION        *MemoryAllocationHobPtr[MAX_SIZE_FOR_SORT];
  EFI_MEMORY_DESCRIPTOR            *MemoryMap;
  UINTN                            Round;
  EFI_HOB_MEMORY_ALLOCATION        HobListMemoryAllocation;

  Hob.Raw = GetHobList();
  //
  // Create a MemoryAllocation HOB to cover the entire HOB list.
  //
  HobListMemoryAllocation.Header.HobType = EFI_HOB_TYPE_MEMORY_ALLOCATION;
  HobListMemoryAllocation.AllocDescriptor.MemoryType = EfiBootServicesData;
  HobListMemoryAllocation.AllocDescriptor.MemoryBaseAddress = (PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  HobListMemoryAllocation.AllocDescriptor.MemoryLength = (UINTN) Hob.HandoffInformationTable->EfiFreeMemoryBottom
                                                                - HobListMemoryAllocation.AllocDescriptor.MemoryBaseAddress;
  ASSERT ((UINTN)Hob.HandoffInformationTable->EfiFreeMemoryBottom == (UINTN) GET_NEXT_HOB (Hob.HandoffInformationTable->EfiEndOfHobList));

  MemoryMap = NULL;
  //
  // First round is to collect the length of MemoryMap and do memory allocation..
  // Second round is to fill the MemoryMap.
  //
  for (Round = 0; Round < 2; Round++) {
    CurrentInfo.CurrentEnd = 0;
    CurrentInfo.MemoryType = EfiMaxMemoryType;
    CurrentInfo.Count = 0;
    CurrentInfo.NewElement = TRUE;
    CurrentResourceHob = GetSmallestResourceHob (CurrentInfo.CurrentEnd);
    while (CurrentResourceHob != NULL) {
      if (CurrentInfo.CurrentEnd != CurrentResourceHob->PhysicalStart) {
        CurrentInfo.CurrentEnd = CurrentResourceHob->PhysicalStart;
        CurrentInfo.NewElement = TRUE;
      }
      ResourceHobStart = CurrentResourceHob->PhysicalStart;
      ResourceHobEnd = CurrentResourceHob->PhysicalStart + CurrentResourceHob->ResourceLength;
      DefaultMemoryAttribute = ConvertResourceDescriptorHobAttributesToCapabilities(CurrentResourceHob->ResourceAttribute);
      DefaultMemoryType = ConvertResourceDescriptorHobResourceTypeToEfiMemoryType(CurrentResourceHob->ResourceType);
      //
      // {                   Resource                           }
      //     [Memory Allocation]      [Memory Allocation] ...
      //
      //
      do {
        MemoryAllocationHobCount = GetSmallestKMemoryAllocationHob (
                                    MemoryAllocationHobPtr,
                                    MAX_SIZE_FOR_SORT,
                                    CurrentInfo.CurrentEnd,
                                    ResourceHobEnd,
                                    &HobListMemoryAllocation
                                    );
        for (Index = 0; Index < MemoryAllocationHobCount; Index++) {
          //
          // There is overlap case in ovmf.
          //
          if (CurrentInfo.CurrentEnd > MemoryAllocationHobPtr[Index]->AllocDescriptor.MemoryBaseAddress) {
            if (CurrentInfo.CurrentEnd >= MemoryAllocationHobPtr[Index]->AllocDescriptor.MemoryBaseAddress + MemoryAllocationHobPtr[Index]->AllocDescriptor.MemoryLength) {
              //
              // Previous Memory Allocation Hob overlaps the entire current Memory Allocation Hob.
              // Ignore current Memory Allocation Hob.
              //
              continue;
            } else {
              //
              // Previous Memory Allocation Hob overlaps partial of current Memory Allocation Hob.
              // Adjust the CurrentEnd and the last MemoryMap entry.
              //
              
              OverlappedLength = CurrentInfo.CurrentEnd - MemoryAllocationHobPtr[Index]->AllocDescriptor.MemoryBaseAddress;
              if (MemoryMap != NULL) {
                MemoryMap[CurrentInfo.Count - 1].NumberOfPages -= EFI_SIZE_TO_PAGES (OverlappedLength);
              }
              CurrentInfo.CurrentEnd -= OverlappedLength;
              
            }
          }

          //
          // Create Conventional or Reserved memory map entry for the memory before Memory Allocation HOB
          //

          if (CurrentInfo.CurrentEnd != MemoryAllocationHobPtr[Index]->AllocDescriptor.MemoryBaseAddress) {
            AppendEfiMemoryDescriptor (
              &CurrentInfo,
              MemoryMap,
              CurrentInfo.CurrentEnd,
              EFI_SIZE_TO_PAGES (MemoryAllocationHobPtr[Index]->AllocDescriptor.MemoryBaseAddress - CurrentInfo.CurrentEnd),
              DefaultMemoryAttribute,
              DefaultMemoryType
              );
          }
          

          //
          // Create memory map entry for the Memory Allocation HOB
          //
          AppendEfiMemoryDescriptor (
            &CurrentInfo,
            MemoryMap,
            MemoryAllocationHobPtr[Index]->AllocDescriptor.MemoryBaseAddress,
            EFI_SIZE_TO_PAGES (MemoryAllocationHobPtr[Index]->AllocDescriptor.MemoryLength),
            DefaultMemoryAttribute,
            MemoryAllocationHobPtr[Index]->AllocDescriptor.MemoryType
            );
        }
      } while (MemoryAllocationHobCount != 0);

      //
      // Create Conventional or Reserved memory map entry for the memory after the last Memory Allocation HOB
      //
      
      if (CurrentInfo.CurrentEnd != ResourceHobEnd) {
        AppendEfiMemoryDescriptor (
          &CurrentInfo,
          MemoryMap,
          CurrentInfo.CurrentEnd,
          EFI_SIZE_TO_PAGES (ResourceHobEnd - CurrentInfo.CurrentEnd),
          DefaultMemoryAttribute,
          DefaultMemoryType
          );
      
    }
      CurrentResourceHob = GetSmallestResourceHob (CurrentInfo.CurrentEnd);
    }
    if (Round == 0) {
      MemoryMapHob = BuildGuidHob (&gUniversalPayloadMemoryMapGuid, sizeof (UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof (EFI_MEMORY_DESCRIPTOR) * CurrentInfo.Count);
      MemoryMapHob->Header.Revision = UNIVERSAL_PAYLOAD_MEMORY_MAP_REVISION;
      MemoryMapHob->Header.Length = sizeof (UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof (EFI_MEMORY_DESCRIPTOR) * CurrentInfo.Count;
      MemoryMap = MemoryMapHob->MemoryMap;

      //
      // Update the HOB list length
      //
      HobListMemoryAllocation.AllocDescriptor.MemoryLength = (UINTN) Hob.HandoffInformationTable->EfiFreeMemoryBottom
                                                                    - HobListMemoryAllocation.AllocDescriptor.MemoryBaseAddress;
      ASSERT ((UINTN)Hob.HandoffInformationTable->EfiFreeMemoryBottom == (UINTN) GET_NEXT_HOB (Hob.HandoffInformationTable->EfiEndOfHobList));
    }
  }

  MemoryMapHob->Count = CurrentInfo.Count;
  MemoryMapHob->DescriptorSize = sizeof(EFI_MEMORY_DESCRIPTOR);
  return EFI_SUCCESS;
}
