/** @file
  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/MemoryMapLib.h>
#include <Library/HobLib.h>
#include <Library/SortLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

#define MAX_SIZE_FOR_SORT  5
typedef struct {
  UINT64   Attribute;
  UINT64   Capability;
} ATTRIBUTE_CONVERSION_ENTRY;

typedef struct {
  UINTN                            CurrentIndex;
  EFI_PHYSICAL_ADDRESS             CurrentEnd;
  UINT64                           CurrentAttribute;
  EFI_MEMORY_TYPE                  CurrentType;
} CURRENT_INFO;

ATTRIBUTE_CONVERSION_ENTRY mAttributeConversionTable[] = {
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

typedef struct {
  EFI_HOB_RESOURCE_DESCRIPTOR        *ResourceHob;
  BOOLEAN                            ContainingHobList;
  UINTN                              FrontCount;
  EFI_MEMORY_DESCRIPTOR              *FrontBuffer;
  UINTN                              BackCount;
  EFI_MEMORY_DESCRIPTOR              *BackBuffer;
  UINTN                              DestCount;
  EFI_MEMORY_DESCRIPTOR              *DestBuffer;
} RESOURCE_DESCRIPTOR_HOB_BUFFER;

/**
  Function to compare 2 Memory Allocation Hob.

  @param[in] Buffer1            pointer to the firstMemory Allocation Hob to compare
  @param[in] Buffer2            pointer to the second Memory Allocation Hob to compare

  @retval 0                     Buffer1 equal to Buffer2
  @retval <0                    Buffer1 is less than Buffer2
  @retval >0                    Buffer1 is greater than Buffer2
**/
INTN
MemoryAllocationHobCompare (
  IN CONST VOID             *Buffer1,
  IN CONST VOID             *Buffer2
  )
{
  EFI_PHYSICAL_ADDRESS Start1;
  EFI_PHYSICAL_ADDRESS Start2;
  Start1 = (*(EFI_HOB_MEMORY_ALLOCATION**)Buffer1)->AllocDescriptor.MemoryBaseAddress;
  Start2 = (*(EFI_HOB_MEMORY_ALLOCATION**)Buffer2)->AllocDescriptor.MemoryBaseAddress;
  if (Start1 == Start2) {
    return 0;
  } else if (Start1 > Start2) {
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
  for (Capabilities = 0, Conversion = mAttributeConversionTable; Conversion->Attribute != 0; Conversion++) {
    if (Attributes & Conversion->Attribute) {
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
  switch (Type) {
  case EFI_RESOURCE_SYSTEM_MEMORY:
    return EfiConventionalMemory;
  case EFI_RESOURCE_MEMORY_MAPPED_IO:
  case EFI_RESOURCE_FIRMWARE_DEVICE:
    return EfiMemoryMappedIO;
  case EFI_RESOURCE_MEMORY_MAPPED_IO_PORT:
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
  IN  EFI_HOB_MEMORY_ALLOCATION   *FakeMemoryAllocationHob
  )
{
  EFI_PEI_HOB_POINTERS        Hob;
  UINTN                       Index;
  PHYSICAL_ADDRESS            HobSize;
  UINTN                       MemoryAllocationHobCount;
  EFI_HOB_MEMORY_ALLOCATION   *TempMemoryAllocationHob;
  BOOLEAN                     EnumAllHobs;
  EnumAllHobs = FALSE;
  MemoryAllocationHobCount = 0;
  //
  // To simplify the logic, treat Hob list as a memory allocation hob.
  // The memory length will be fixed later to save time.
  //
  FakeMemoryAllocationHob->Header.HobType = EFI_HOB_TYPE_MEMORY_ALLOCATION;
  FakeMemoryAllocationHob->AllocDescriptor.MemoryType = EfiBootServicesData;
  Hob.Raw = GetHobList();
  FakeMemoryAllocationHob->AllocDescriptor.MemoryBaseAddress = (PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  //
  // Enum all hobs, including the fake memory allocation hob which contains the hob list.
  //
  while (!EnumAllHobs) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      if ((Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress >= Base) &&
        (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress <= Limit)) {
        MemoryAllocationHobCount++;
        if (MemoryAllocationHobCount > K) {
          if (MemoryAllocationHobCompare(&MemoryAllocationHobPtr[K - 1], &Hob.MemoryAllocation) == 1) {
            MemoryAllocationHobPtr[K - 1] = Hob.MemoryAllocation;
            QuickSort (MemoryAllocationHobPtr, K, sizeof (*MemoryAllocationHobPtr), MemoryAllocationHobCompare, &TempMemoryAllocationHob);
          }
        } else {
          MemoryAllocationHobPtr[MemoryAllocationHobCount - 1] = Hob.MemoryAllocation;
        }
      }
    }
    if (Hob.MemoryAllocation == FakeMemoryAllocationHob) {
      EnumAllHobs = TRUE;
    } else {
      Hob.Raw = GET_NEXT_HOB (Hob);
      if (END_OF_HOB_LIST (Hob)) {
        //
        // Fix up fake memory allocation hob.
        //
        HobSize = (PHYSICAL_ADDRESS) (UINTN) GET_NEXT_HOB (Hob) - FakeMemoryAllocationHob->AllocDescriptor.MemoryBaseAddress;
        HobSize = (HobSize + EFI_PAGE_SIZE - 1) & (~ ((PHYSICAL_ADDRESS) HobSize - 1));
        FakeMemoryAllocationHob->AllocDescriptor.MemoryLength = HobSize;
        Hob.MemoryAllocation = FakeMemoryAllocationHob;
      }
    }
    if ((MemoryAllocationHobCount == K) || (MemoryAllocationHobCount < K && EnumAllHobs)) {
      QuickSort (MemoryAllocationHobPtr, MemoryAllocationHobCount, sizeof (*MemoryAllocationHobPtr), MemoryAllocationHobCompare, &TempMemoryAllocationHob);
    }
  }
  for (Index = 0; Index < (MemoryAllocationHobCount > K ? K : MemoryAllocationHobCount); Index++) {
    DEBUG ((DEBUG_INFO, "GetSmallestKMemoryAllocationHob   MemoryBaseAddress = 0x%lx\n", MemoryAllocationHobPtr[Index]->AllocDescriptor.MemoryBaseAddress / 0x1000));
  }
  return MemoryAllocationHobCount > K ? K : MemoryAllocationHobCount;
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
  EFI_HOB_RESOURCE_DESCRIPTOR*  TmpResourceHob;
  TmpResourceHob = NULL;
  Hob.Raw = GetHobList();
  while (!END_OF_HOB_LIST (Hob)) {
    if ((Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) &&
        (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY || Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_RESERVED)) {
      if (Hob.ResourceDescriptor->PhysicalStart < Base) {
        Hob.Raw = GET_NEXT_HOB (Hob);
        continue;
      }
      if (TmpResourceHob == NULL) {
        TmpResourceHob = Hob.ResourceDescriptor;
      }
      if (TmpResourceHob->PhysicalStart > Hob.ResourceDescriptor->PhysicalStart) {
        TmpResourceHob = Hob.ResourceDescriptor;
      }
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  return TmpResourceHob;
}

/**
  Append a range to the Memory map table.
  If the MemoryBaseAddress NULL, only change the CurrentInfo

  @param  CurrentInfo             The information about the current status.
  @param  MemoryMapTable          The pointer to MemoryMapTable.
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
  EFI_MEMORY_DESCRIPTOR         *MemoryMapTable,
  EFI_PHYSICAL_ADDRESS          MemoryBaseAddress,
  UINTN                         PageNumber,
  UINT64                        Attribute,
  EFI_MEMORY_TYPE               Type
  )
{
  if (CurrentInfo->CurrentEnd == 0) {
    //
    // Init the first Efi Memory Descriptor
    //
    if (MemoryMapTable != NULL) {
      MemoryMapTable[CurrentInfo->CurrentIndex].Attribute = Attribute;
      MemoryMapTable[CurrentInfo->CurrentIndex].VirtualStart = 0;
      MemoryMapTable[CurrentInfo->CurrentIndex].Type = Type;
      MemoryMapTable[CurrentInfo->CurrentIndex].PhysicalStart = MemoryBaseAddress;
      MemoryMapTable[CurrentInfo->CurrentIndex].NumberOfPages = PageNumber;
    }
    CurrentInfo->CurrentType = Type;
    CurrentInfo->CurrentAttribute = Attribute;
    CurrentInfo->CurrentEnd = PageNumber * EFI_PAGE_SIZE;
  } else if ((CurrentInfo->CurrentEnd == MemoryBaseAddress) &&
             (CurrentInfo->CurrentType == Type) &&
             (CurrentInfo->CurrentAttribute == Attribute)) {
    CurrentInfo->CurrentEnd += PageNumber * EFI_PAGE_SIZE;
    if (MemoryMapTable != NULL) {
        MemoryMapTable[CurrentInfo->CurrentIndex].NumberOfPages += PageNumber;
      }
  } else {
    CurrentInfo->CurrentIndex += 1;
    if (MemoryMapTable != NULL) {
      MemoryMapTable[CurrentInfo->CurrentIndex].Attribute = Attribute;
      MemoryMapTable[CurrentInfo->CurrentIndex].VirtualStart = 0;
      MemoryMapTable[CurrentInfo->CurrentIndex].Type = Type;
      MemoryMapTable[CurrentInfo->CurrentIndex].PhysicalStart = MemoryBaseAddress;
      MemoryMapTable[CurrentInfo->CurrentIndex].NumberOfPages = PageNumber;
    }
    CurrentInfo->CurrentType = Type;
    CurrentInfo->CurrentAttribute = Attribute;
    CurrentInfo->CurrentEnd = MemoryBaseAddress + PageNumber * EFI_PAGE_SIZE;
  }
}

/**
  Create a Guid Hob containing the memory map descriptor table.
  After calling the function, PEI should not do any memory allocation operation.

  @retval EFI_SUCCESS           The Memory Map is created successfully.
**/
EFI_STATUS
EFIAPI
BuildMemMap (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS             Hob;
  UINTN                            ResourceHobCount;
  UINTN                            MemoryAllocationHobCount;
  EFI_PHYSICAL_ADDRESS             ResourceHobStart;
  EFI_PHYSICAL_ADDRESS             ResourceHobEnd;
  EFI_HOB_RESOURCE_DESCRIPTOR      *CurrentResourceHob;
  CURRENT_INFO                     CurrentInfo;
  UINT64                           ResourceHobAttribute;
  UINTN                            Index;
  UNIVERSAL_PAYLOAD_MEMORY_MAP     *MemoryMapHob;
  EFI_MEMORY_TYPE                  ResourceHobType;
  EFI_HOB_MEMORY_ALLOCATION        *MemoryAllocationHobPtr[MAX_SIZE_FOR_SORT];
  EFI_MEMORY_DESCRIPTOR            *MemoryMapTable;
  UINTN                            Round;
  EFI_HOB_MEMORY_ALLOCATION        FakeMemoryAllocationHob;
  ResourceHobCount         = 0;
  MemoryAllocationHobCount = 0;
  Hob.Raw = GetHobList();
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY || Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_RESERVED) {
        ResourceHobCount++;
      }
    }
    if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      MemoryAllocationHobCount++;
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  MemoryMapTable = NULL;
  for (Round = 0; Round < 2; Round++){
    CurrentInfo.CurrentEnd   = 0;
    CurrentInfo.CurrentIndex = 0;
    CurrentResourceHob = GetSmallestResourceHob (CurrentInfo.CurrentEnd);
    while (CurrentResourceHob != NULL) {
        ResourceHobStart = CurrentResourceHob->PhysicalStart;
        ResourceHobEnd = CurrentResourceHob->PhysicalStart + CurrentResourceHob->ResourceLength;
        ResourceHobAttribute = ConvertResourceDescriptorHobAttributesToCapabilities(CurrentResourceHob->ResourceAttribute);
        ResourceHobType = ConvertResourceDescriptorHobResourceTypeToEfiMemoryType(CurrentResourceHob->ResourceType);
        MemoryAllocationHobCount = GetSmallestKMemoryAllocationHob (
                                    MemoryAllocationHobPtr,
                                    1,
                                    ResourceHobStart,
                                    ResourceHobEnd,
                                    &FakeMemoryAllocationHob
                                    );
        if (MemoryAllocationHobCount == 0) {
          AppendEfiMemoryDescriptor (
            &CurrentInfo,
            MemoryMapTable,
            ResourceHobStart,
            (ResourceHobEnd - ResourceHobStart) / EFI_PAGE_SIZE,
            ResourceHobAttribute,
            ResourceHobType
            );
        } else if (MemoryAllocationHobPtr[0]->AllocDescriptor.MemoryBaseAddress == ResourceHobStart) {
          AppendEfiMemoryDescriptor (
            &CurrentInfo,
            MemoryMapTable,
            MemoryAllocationHobPtr[0]->AllocDescriptor.MemoryBaseAddress,
            MemoryAllocationHobPtr[0]->AllocDescriptor.MemoryLength / EFI_PAGE_SIZE,
            ResourceHobAttribute,
            MemoryAllocationHobPtr[0]->AllocDescriptor.MemoryType
            );
        } else {
          AppendEfiMemoryDescriptor (
            &CurrentInfo,
            MemoryMapTable,
            ResourceHobStart,
            (MemoryAllocationHobPtr[0]->AllocDescriptor.MemoryBaseAddress - ResourceHobStart) / EFI_PAGE_SIZE,
            ResourceHobAttribute,
            ResourceHobType
            );
        }
        while (MemoryAllocationHobCount != 0) {
          MemoryAllocationHobCount = GetSmallestKMemoryAllocationHob (
                                      MemoryAllocationHobPtr,
                                      MAX_SIZE_FOR_SORT,
                                      CurrentInfo.CurrentEnd,
                                      ResourceHobEnd,
                                      &FakeMemoryAllocationHob
                                      );
          for (Index = 0; Index < MemoryAllocationHobCount; Index++) {
          }
          for (Index = 0; Index < MemoryAllocationHobCount; Index++) {
            if (CurrentInfo.CurrentEnd > MemoryAllocationHobPtr[Index]->AllocDescriptor.MemoryBaseAddress) {
              //
              // This means Memory Allocation Hobs has overlap.
              // won't handle this cases.
              //
              continue;
            }
            if (CurrentInfo.CurrentEnd != MemoryAllocationHobPtr[Index]->AllocDescriptor.MemoryBaseAddress) {
              AppendEfiMemoryDescriptor (
                &CurrentInfo,
                MemoryMapTable,
                CurrentInfo.CurrentEnd,
                (MemoryAllocationHobPtr[Index]->AllocDescriptor.MemoryBaseAddress - CurrentInfo.CurrentEnd) / EFI_PAGE_SIZE,
                ResourceHobAttribute,
                ResourceHobType
                );
            }
            AppendEfiMemoryDescriptor (
              &CurrentInfo,
              MemoryMapTable,
              MemoryAllocationHobPtr[Index]->AllocDescriptor.MemoryBaseAddress,
              MemoryAllocationHobPtr[Index]->AllocDescriptor.MemoryLength / EFI_PAGE_SIZE,
              ResourceHobAttribute,
              MemoryAllocationHobPtr[Index]->AllocDescriptor.MemoryType
              );
          }
        }
        if (CurrentInfo.CurrentEnd != ResourceHobEnd) {
          AppendEfiMemoryDescriptor (
            &CurrentInfo,
            MemoryMapTable,
            CurrentInfo.CurrentEnd,
            (ResourceHobEnd - CurrentInfo.CurrentEnd) / EFI_PAGE_SIZE,
            ResourceHobAttribute,
            ResourceHobType
            );
        }
        CurrentResourceHob = GetSmallestResourceHob (CurrentInfo.CurrentEnd);
    }
    if (Round == 0) {
      DEBUG ((DEBUG_INFO, "   sizeof (UNIVERSAL_PAYLOAD_MEMORY_MAP  = %d\n",   sizeof (UNIVERSAL_PAYLOAD_MEMORY_MAP)));
      DEBUG ((DEBUG_INFO, "   sizeof (EFI_MEMORY_DESCRIPTOR)  = %d\n",   sizeof (EFI_MEMORY_DESCRIPTOR)));
      DEBUG ((DEBUG_INFO, "   CurrentInfo.CurrentIndex  = %d\n",   CurrentInfo.CurrentIndex));
      MemoryMapHob = BuildGuidHob (&gUniversalPayloadMemoryMapGuid, sizeof (UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof (EFI_MEMORY_DESCRIPTOR) * (CurrentInfo.CurrentIndex + 1));
      MemoryMapHob->Header.Revision = UNIVERSAL_PAYLOAD_MEMORY_MAP_REVISION;
      MemoryMapHob->Header.Length = sizeof (UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof (EFI_MEMORY_DESCRIPTOR) * (CurrentInfo.CurrentIndex + 1);
      MemoryMapTable = MemoryMapHob->MemoryMapTable;
      MemoryMapTable = &MemoryMapHob->MemoryMapTable[0];
    }
  }

  MemoryMapHob->Count = CurrentInfo.CurrentIndex + 1;
  return EFI_SUCCESS;
}
