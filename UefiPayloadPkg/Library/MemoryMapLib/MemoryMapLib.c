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

typedef struct {
  UINT64   Attribute;
  UINT64   Capability;
} ATTRIBUTE_CONVERSION_ENTRY;

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

  Merge two Memory Descriptor ranges into one.
  The Back is the base range, and it will cover the whole range.
  The Front has high priority than Back, and it can only cover part of the range.

  @param FrontCount      Count of Front range.
  @param Front           The Front Range
  @param BackCount       Count of Back range.
  @param Back            The Back Range
  @param DestCount       Count of Dest range.
  @param Dest            The Dest Range

  @retval      Count of Dest range
**/
UINTN
MergeMemDescriptorIntoOne (
  UINTN                     FrontCount,
  EFI_MEMORY_DESCRIPTOR     *Front,
  UINTN                     BackCount,
  EFI_MEMORY_DESCRIPTOR     *Back,
  UINTN                     DestCount,
  EFI_MEMORY_DESCRIPTOR     *Dest
  ) 
{
  EFI_PHYSICAL_ADDRESS     End;
  EFI_PHYSICAL_ADDRESS     TempEnd;
  UINTN                    TempDestIndex;
  UINTN                    Index;
  UINTN                    TempFrontIndex;
  UINTN                    TempBackIndex;
  UINTN                    TempMin;

  TempDestIndex = 0;
  End = MAX(Back[BackCount-1].PhysicalStart + Back[BackCount-1].NumberOfPages * EFI_PAGE_SIZE,
          Front[FrontCount-1].PhysicalStart + Front[FrontCount-1].NumberOfPages * EFI_PAGE_SIZE);
  if (MIN(Back[0].PhysicalStart, Front[0].PhysicalStart) == Front[0].PhysicalStart) {
    CopyMem(&Dest[0], &Front[0], sizeof(EFI_MEMORY_DESCRIPTOR));
  } else {
    CopyMem(&Dest[0], &Back[0], sizeof(EFI_MEMORY_DESCRIPTOR));
    Dest[0].NumberOfPages = 1;
  }

  TempEnd = Dest[TempDestIndex].PhysicalStart + Dest[TempDestIndex].NumberOfPages * EFI_PAGE_SIZE;
  while (TempEnd != End) {
    for (Index = 0; Index < FrontCount; Index ++) {
      if (TempEnd < Front[Index].PhysicalStart + Front[Index].NumberOfPages * EFI_PAGE_SIZE) {
        break;
      }
    }
    TempFrontIndex = Index;
    if (TempFrontIndex != FrontCount) {
      if (TempEnd >= Front[TempFrontIndex].PhysicalStart) {
        if ((Dest[TempDestIndex].Type == Front[TempFrontIndex].Type) && (Dest[TempDestIndex].Attribute == Front[TempFrontIndex].Attribute)) {
          Dest[TempDestIndex].NumberOfPages = (Front[TempFrontIndex].PhysicalStart + Front[TempFrontIndex].NumberOfPages * EFI_PAGE_SIZE - Dest[TempDestIndex].PhysicalStart) / EFI_PAGE_SIZE;
        } else {
          TempDestIndex++;
          CopyMem(&Dest[TempDestIndex], &Front[TempFrontIndex], sizeof(EFI_MEMORY_DESCRIPTOR));
          Dest[TempDestIndex].PhysicalStart = TempEnd;
        }
        TempEnd = Dest[TempDestIndex].PhysicalStart + Dest[TempDestIndex].NumberOfPages * EFI_PAGE_SIZE;
        continue;
      }
    }
    for (Index = 0; Index < BackCount; Index ++) {
      if (TempEnd < Back[Index].PhysicalStart + Back[Index].NumberOfPages * EFI_PAGE_SIZE) {
        break;
      }
    }
    ASSERT (Index < BackCount);
    TempBackIndex = Index;
    if (TempFrontIndex != FrontCount) {
      TempMin = MIN(Back[TempBackIndex].PhysicalStart + Back[TempBackIndex].NumberOfPages * EFI_PAGE_SIZE, 
        Front[TempFrontIndex].PhysicalStart);
    } else {
      TempMin = Back[TempBackIndex].PhysicalStart + Back[TempBackIndex].NumberOfPages * EFI_PAGE_SIZE;
    }
    if ((Dest[TempDestIndex].Type == Back[TempBackIndex].Type) && (Dest[TempDestIndex].Attribute == Back[TempBackIndex].Attribute)) {
      Dest[TempDestIndex].NumberOfPages = (TempMin - Dest[TempDestIndex].PhysicalStart) / EFI_PAGE_SIZE;
    } else {
      TempDestIndex++;
      CopyMem(&Dest[TempDestIndex], &Back[TempBackIndex], sizeof(EFI_MEMORY_DESCRIPTOR));
      Dest[TempDestIndex].PhysicalStart = TempEnd;
      Dest[TempDestIndex].NumberOfPages = (TempMin - Dest[TempDestIndex].PhysicalStart) / EFI_PAGE_SIZE;
    }
    TempEnd = Dest[TempDestIndex].PhysicalStart + Dest[TempDestIndex].NumberOfPages * EFI_PAGE_SIZE;
  }

  DestCount = TempDestIndex + 1;
  return DestCount;
}

/**
  Function to compare 2  Resource Descriptor Hobs for use in QuickSort.

  @param[in] Buffer1            pointer to the first Resource Descriptor Hob to compare
  @param[in] Buffer2            pointer to the second Resource Descriptor Hob to compare

  @retval 0                     Buffer1 equal to Buffer2
  @retval <0                    Buffer1 is less than Buffer2
  @retval >0                    Buffer1 is greater than Buffer2
**/
UINTN
ResourceDescriptorCompare (
  IN CONST VOID             *Buffer1,
  IN CONST VOID             *Buffer2
  )
{
  EFI_PHYSICAL_ADDRESS Start1;
  EFI_PHYSICAL_ADDRESS Start2;
  Start1 = ((RESOURCE_DESCRIPTOR_HOB_BUFFER*)Buffer1)->ResourceHob->PhysicalStart;
  Start2 = ((RESOURCE_DESCRIPTOR_HOB_BUFFER*)Buffer2)->ResourceHob->PhysicalStart;
  if (Start1 == Start2) {
    return 0;
  } else if (Start1 > Start2) {
    return 1;
  } else {
    return -1;
  }
}

/**
  Function to compare 2 Memory Resource Descriptors for use in QuickSort.

  @param[in] Buffer1            pointer to the first Memory Resource Descriptor to compare
  @param[in] Buffer2            pointer to the second Memory Resource Descriptor to compare

  @retval 0                     Buffer1 equal to Buffer2
  @retval <0                    Buffer1 is less than Buffer2
  @retval >0                    Buffer1 is greater than Buffer2
**/
UINTN
MemoryResourceDescriptorCompare (
  IN CONST VOID             *Buffer1,
  IN CONST VOID             *Buffer2
  )
{
  EFI_PHYSICAL_ADDRESS Start1;
  EFI_PHYSICAL_ADDRESS Start2;
  Start1 = ((EFI_MEMORY_DESCRIPTOR*)Buffer1)->PhysicalStart;
  Start2 = ((EFI_MEMORY_DESCRIPTOR*)Buffer2)->PhysicalStart;
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
  EFI_HOB_RESOURCE_DESCRIPTOR               *Hob
  )
{
  switch (Hob->ResourceType) {
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
  RESOURCE_DESCRIPTOR_HOB_BUFFER   *ResourceDescriptorHobBuffer;
  EFI_PHYSICAL_ADDRESS             PhitHobStart;
  EFI_PHYSICAL_ADDRESS             ResourceHobStart;
  EFI_PHYSICAL_ADDRESS             ResourceHobEnd;
  EFI_RESOURCE_ATTRIBUTE_TYPE      ResourceHobAttribute;
  UINTN                            Index;
  UINTN                            BackIndex;
  EFI_PHYSICAL_ADDRESS             BackNextStart;
  UINTN                            TotalDestCount;
  UINTN                            TotalDestCountUsed;
  EFI_MEMORY_DESCRIPTOR            *Back;
  EFI_MEMORY_DESCRIPTOR            *MemMapTable;
  UNIVERSAL_PAYLOAD_MEMORY_MAP     *MemoryMapHob;

  MemoryMapHob = BuildGuidHob (&gUniversalPayloadMemoryMapGuid, sizeof (UNIVERSAL_PAYLOAD_MEMORY_MAP));
  MemoryMapHob->Header.Revision = UNIVERSAL_PAYLOAD_MEMORY_MAP_REVISION;
  MemoryMapHob->Header.Length = sizeof (UNIVERSAL_PAYLOAD_MEMORY_MAP);
  
  Hob.HandoffInformationTable = (EFI_HOB_HANDOFF_INFO_TABLE *) GetFirstHob(EFI_HOB_TYPE_HANDOFF);
  PhitHobStart = Hob.HandoffInformationTable->EfiMemoryBottom;

  ResourceHobCount = 0;
  Hob.Raw = GetHobList();
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY || Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_RESERVED) {
        ResourceHobCount++;
      }
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  ResourceDescriptorHobBuffer = AllocatePages (sizeof (RESOURCE_DESCRIPTOR_HOB_BUFFER) * ResourceHobCount);

  Index = 0;
  Hob.Raw = GetHobList();
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY || Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_RESERVED) {
        ResourceDescriptorHobBuffer[Index].ResourceHob = Hob.ResourceDescriptor;
        if ((PhitHobStart >= Hob.ResourceDescriptor->PhysicalStart) &&
          (PhitHobStart <= (Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength))) {
          ResourceDescriptorHobBuffer[Index].ContainingHobList = TRUE;
        } else {
          ResourceDescriptorHobBuffer[Index].ContainingHobList = FALSE;
        }
        Index++;
      }
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  PerformQuickSort (ResourceDescriptorHobBuffer, ResourceHobCount, sizeof (RESOURCE_DESCRIPTOR_HOB_BUFFER), (SORT_COMPARE) ResourceDescriptorCompare);

  //
  // Allocate memory needed
  //
  TotalDestCount = 0;
  TotalDestCountUsed = 0;
  for (Index = 0; Index < ResourceHobCount; Index++) {
    ResourceHobStart = ResourceDescriptorHobBuffer[Index].ResourceHob->PhysicalStart;
    ResourceHobEnd = ResourceDescriptorHobBuffer[Index].ResourceHob->PhysicalStart + ResourceDescriptorHobBuffer[Index].ResourceHob->ResourceLength;
    MemoryAllocationHobCount = 0;
    Hob.Raw = GetHobList();
    while (!END_OF_HOB_LIST (Hob)) {
      if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
        if ((Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress >= ResourceHobStart) &&
          (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress <= ResourceHobEnd)) {
          MemoryAllocationHobCount++;
        }
      }
      Hob.Raw = GET_NEXT_HOB (Hob);
    }
    ResourceDescriptorHobBuffer[Index].FrontCount = MemoryAllocationHobCount;
    if (ResourceDescriptorHobBuffer[Index].ContainingHobList) {
      ResourceDescriptorHobBuffer[Index].FrontCount += 1;
      ResourceDescriptorHobBuffer[Index].BackCount = 5;
    } else{
      ResourceDescriptorHobBuffer[Index].BackCount = 1;
    }
    ResourceDescriptorHobBuffer[Index].DestCount = 2 * (ResourceDescriptorHobBuffer[Index].FrontCount + ResourceDescriptorHobBuffer[Index].BackCount);  
    TotalDestCount += ResourceDescriptorHobBuffer[Index].DestCount;
    if (ResourceDescriptorHobBuffer[Index].FrontCount != 0) {
      ResourceDescriptorHobBuffer[Index].FrontBuffer = AllocatePool (ResourceDescriptorHobBuffer[Index].FrontCount * sizeof (EFI_MEMORY_DESCRIPTOR));
    } else {
      ResourceDescriptorHobBuffer[Index].FrontBuffer = NULL;
    }
    ResourceDescriptorHobBuffer[Index].BackBuffer = AllocatePool (ResourceDescriptorHobBuffer[Index].BackCount * sizeof (EFI_MEMORY_DESCRIPTOR));
  }
  MemMapTable =  AllocatePages (EFI_SIZE_TO_PAGES (sizeof (EFI_MEMORY_DESCRIPTOR) * TotalDestCount));
  //
  // From now on, there should be no code to allocate memory pool or page.
  //

  for (Index = 0; Index < ResourceHobCount; Index++) {
    ResourceHobStart = ResourceDescriptorHobBuffer[Index].ResourceHob->PhysicalStart;
    ResourceHobEnd = ResourceDescriptorHobBuffer[Index].ResourceHob->PhysicalStart + ResourceDescriptorHobBuffer[Index].ResourceHob->ResourceLength;
    ResourceHobAttribute = ResourceDescriptorHobBuffer[Index].ResourceHob->ResourceAttribute;
    ResourceDescriptorHobBuffer[Index].ResourceHob->Header.HobType = EFI_HOB_TYPE_UNUSED;
    if (ResourceDescriptorHobBuffer[Index].FrontCount != 0) {
      MemoryAllocationHobCount = 0;
      Hob.Raw = GetHobList();
      while (!END_OF_HOB_LIST (Hob)) {
        if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
          if ((Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress >= ResourceHobStart) &&
            (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress <= ResourceHobEnd)) {
            ASSERT ((Hob.MemoryAllocation->AllocDescriptor.MemoryLength & (EFI_PAGE_SIZE - 1)) == 0);
            ResourceDescriptorHobBuffer[Index].FrontBuffer[MemoryAllocationHobCount].PhysicalStart = Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress;
            ResourceDescriptorHobBuffer[Index].FrontBuffer[MemoryAllocationHobCount].NumberOfPages = (Hob.MemoryAllocation->AllocDescriptor.MemoryLength) / EFI_PAGE_SIZE;
            ResourceDescriptorHobBuffer[Index].FrontBuffer[MemoryAllocationHobCount].Type          = Hob.MemoryAllocation->AllocDescriptor.MemoryType;
            ResourceDescriptorHobBuffer[Index].FrontBuffer[MemoryAllocationHobCount].VirtualStart  = 0;
            ResourceDescriptorHobBuffer[Index].FrontBuffer[MemoryAllocationHobCount].Attribute     = ConvertResourceDescriptorHobAttributesToCapabilities(ResourceHobAttribute);
            Hob.Header->HobType = EFI_HOB_TYPE_UNUSED;
            MemoryAllocationHobCount++;
            ASSERT (MemoryAllocationHobCount <= ResourceDescriptorHobBuffer[Index].FrontCount);
          }
        }
        Hob.Raw = GET_NEXT_HOB (Hob);
      }
      ResourceDescriptorHobBuffer[Index].FrontCount = MemoryAllocationHobCount;
      PerformQuickSort (ResourceDescriptorHobBuffer[Index].FrontBuffer, MemoryAllocationHobCount, sizeof (EFI_MEMORY_DESCRIPTOR), (SORT_COMPARE) MemoryResourceDescriptorCompare);
    }
    
    Back = ResourceDescriptorHobBuffer[Index].BackBuffer;
    if (ResourceDescriptorHobBuffer[Index].ContainingHobList) {
      Hob.Raw = GetHobList();
      //
      // This resource hob contains Hob list, so the back range can have at most 5 range.
      //
      BackIndex = 0;
      BackNextStart = 0;

      Back[BackIndex].PhysicalStart = ResourceHobStart;
      Back[BackIndex].NumberOfPages = (Hob.HandoffInformationTable->EfiMemoryBottom - ResourceHobStart) / EFI_PAGE_SIZE;
      Back[BackIndex].Type = EfiConventionalMemory;
      Back[BackIndex].VirtualStart  = 0;
      Back[BackIndex].Attribute     = ConvertResourceDescriptorHobAttributesToCapabilities(ResourceHobAttribute);
      BackNextStart = Back[BackIndex].PhysicalStart + Back[BackIndex].NumberOfPages * EFI_PAGE_SIZE;
      if (Back[BackIndex].NumberOfPages != 0) {
        BackIndex += 1;
      }

      Back[BackIndex].PhysicalStart = BackNextStart;
      Back[BackIndex].NumberOfPages = (Hob.HandoffInformationTable->EfiFreeMemoryBottom - BackNextStart - 1) / EFI_PAGE_SIZE + 1;
      Back[BackIndex].Type = EfiBootServicesData;
      Back[BackIndex].VirtualStart  = 0;
      Back[BackIndex].Attribute     = ConvertResourceDescriptorHobAttributesToCapabilities(ResourceHobAttribute);
      BackNextStart = Back[BackIndex].PhysicalStart + Back[BackIndex].NumberOfPages * EFI_PAGE_SIZE;
      if (Back[BackIndex].NumberOfPages != 0) {
        BackIndex += 1;
      }

      Back[BackIndex].PhysicalStart = BackNextStart;
      Back[BackIndex].NumberOfPages = (Hob.HandoffInformationTable->EfiFreeMemoryTop - Back[BackIndex].PhysicalStart) / EFI_PAGE_SIZE;
      Back[BackIndex].Type = EfiConventionalMemory;
      Back[BackIndex].VirtualStart  = 0;
      Back[BackIndex].Attribute     = ConvertResourceDescriptorHobAttributesToCapabilities(ResourceHobAttribute);
      BackNextStart = Back[BackIndex].PhysicalStart + Back[BackIndex].NumberOfPages * EFI_PAGE_SIZE;
      if (Back[BackIndex].NumberOfPages != 0) {
        BackIndex += 1;
      }

      Back[BackIndex].PhysicalStart = BackNextStart;
      Back[BackIndex].NumberOfPages = (Hob.HandoffInformationTable->EfiMemoryTop - BackNextStart) / EFI_PAGE_SIZE;
      Back[BackIndex].Type = EfiBootServicesData;
      Back[BackIndex].VirtualStart  = 0;
      Back[BackIndex].Attribute     = ConvertResourceDescriptorHobAttributesToCapabilities(ResourceHobAttribute);
      BackNextStart = Back[BackIndex].PhysicalStart + Back[BackIndex].NumberOfPages * EFI_PAGE_SIZE;
      if (Back[BackIndex].NumberOfPages != 0) {
        BackIndex += 1;
      }

      Back[BackIndex].PhysicalStart = BackNextStart;
      Back[BackIndex].NumberOfPages = (ResourceHobEnd - BackNextStart) / EFI_PAGE_SIZE;
      Back[BackIndex].Type = EfiConventionalMemory;
      Back[BackIndex].VirtualStart  = 0;
      Back[BackIndex].Attribute     = ConvertResourceDescriptorHobAttributesToCapabilities(ResourceHobAttribute);
      if (Back[BackIndex].NumberOfPages != 0) {
        BackIndex += 1;
      }
      ResourceDescriptorHobBuffer[Index].BackCount = BackIndex;

    } else {
      Back[0].PhysicalStart = ResourceHobStart;
      Back[0].NumberOfPages = ResourceDescriptorHobBuffer[Index].ResourceHob->ResourceLength / EFI_PAGE_SIZE;
      Back[0].Type = ConvertResourceDescriptorHobResourceTypeToEfiMemoryType (ResourceDescriptorHobBuffer[Index].ResourceHob);
      Back[0].VirtualStart  = 0;
      Back[0].Attribute     = ConvertResourceDescriptorHobAttributesToCapabilities(ResourceHobAttribute);
    }

    if (Index == 0) {
      ResourceDescriptorHobBuffer[Index].DestBuffer = MemMapTable;
      ResourceDescriptorHobBuffer[Index].DestCount = TotalDestCount - TotalDestCountUsed;
    } else {
      ResourceDescriptorHobBuffer[Index].DestBuffer = ResourceDescriptorHobBuffer[Index-1].DestBuffer + ResourceDescriptorHobBuffer[Index-1].DestCount;
      ResourceDescriptorHobBuffer[Index].DestCount = TotalDestCount - TotalDestCountUsed;
    }
    if (Back[0].Type != EfiConventionalMemory && Back[0].Type != EfiReservedMemoryType) {
      ResourceDescriptorHobBuffer[Index].DestCount = 0;
      continue;
    }
    if (ResourceDescriptorHobBuffer[Index].FrontCount != 0) {
      ResourceDescriptorHobBuffer[Index].DestCount = MergeMemDescriptorIntoOne (
        ResourceDescriptorHobBuffer[Index].FrontCount,
        ResourceDescriptorHobBuffer[Index].FrontBuffer,
        ResourceDescriptorHobBuffer[Index].BackCount,
        ResourceDescriptorHobBuffer[Index].BackBuffer,
        ResourceDescriptorHobBuffer[Index].DestCount,
        ResourceDescriptorHobBuffer[Index].DestBuffer
      );
    } else {
      CopyMem (ResourceDescriptorHobBuffer[Index].DestBuffer, ResourceDescriptorHobBuffer[Index].BackBuffer, sizeof (EFI_MEMORY_DESCRIPTOR) * ResourceDescriptorHobBuffer[Index].BackCount);
      ResourceDescriptorHobBuffer[Index].DestCount = ResourceDescriptorHobBuffer[Index].BackCount;
    }
    TotalDestCountUsed += ResourceDescriptorHobBuffer[Index].DestCount;
  }
  MemoryMapHob->Count = TotalDestCountUsed;
  MemoryMapHob->MemoryMap = (EFI_PHYSICAL_ADDRESS) (UINTN) ResourceDescriptorHobBuffer[0].DestBuffer;
  return EFI_SUCCESS;
}
