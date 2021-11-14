/** @file

  Copyright (c) 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2017 - 2021, Intel Corporation. All rights reserved.<BR>

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

UINT64
ConvertCapabilitiesToResourceDescriptorHobAttributes (
  UINT64               Capabilities
  )
{
  UINT64                      Attributes;
  ATTRIBUTE_CONVERSION_ENTRY  *Conversion;

  for (Attributes = 0, Conversion = mAttributeConversionTable; Conversion->Attribute != 0; Conversion++) {
    if (Capabilities & Conversion->Capability) {
      Attributes |= Conversion->Attribute;
    }
  }

  return Attributes;
}

EFI_RESOURCE_TYPE
ConvertEfiMemoryTypeToResourceDescriptorHobResourceType (
  EFI_MEMORY_TYPE               Type
  )
{
  switch (Type) {
  case EfiConventionalMemory:
    return EFI_RESOURCE_SYSTEM_MEMORY;

  case EfiReservedMemoryType:
    return EFI_RESOURCE_MEMORY_RESERVED;

  default:
    return EFI_RESOURCE_SYSTEM_MEMORY;
  }
}

/**
  Create resource Hob that Uefi can use based on Memory Map Table.
  This function assume the memory map table is sorted, and doesn't have overlap with each other.
  Also this function assume the memory map table only contains physical memory range.

  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
EFI_STATUS
EFIAPI
CreateHobsBasedOnMemoryMap (
  IN UNIVERSAL_PAYLOAD_MEMORY_MAP                    *MemoryMapHob
  )
{

  EFI_PEI_HOB_POINTERS             Hob;
  UINTN                            Index;
  EFI_MEMORY_DESCRIPTOR            *MemMapTable;
  EFI_HOB_RESOURCE_DESCRIPTOR      *ResourceDescriptor;


  for (Index = 0; Index < MemoryMapHob->Count; Index++) {
    MemMapTable = ((EFI_MEMORY_DESCRIPTOR *)MemoryMapHob->MemoryMap) + Index;
    Hob.Raw = GetHobList();
    ResourceDescriptor = NULL;
    while (!END_OF_HOB_LIST (Hob)) {
      if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
        ResourceDescriptor = Hob.ResourceDescriptor;
      }
      Hob.Raw = GET_NEXT_HOB (Hob);
    }
    //
    // Every Memory map range should be contained in one Resource Descriptor Hob.
    //
    if (ResourceDescriptor != NULL &&
        ResourceDescriptor->ResourceAttribute == ConvertCapabilitiesToResourceDescriptorHobAttributes (MemMapTable->Attribute) &&
        ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY &&
        ResourceDescriptor->PhysicalStart + ResourceDescriptor->ResourceLength == MemMapTable->PhysicalStart) {
      ResourceDescriptor->ResourceLength += MemMapTable->NumberOfPages * EFI_PAGE_SIZE;     
    } else {
      BuildResourceDescriptorHob (
        ConvertEfiMemoryTypeToResourceDescriptorHobResourceType (MemMapTable->Type),
        ConvertCapabilitiesToResourceDescriptorHobAttributes (MemMapTable->Attribute),
        MemMapTable->PhysicalStart,
        MemMapTable->NumberOfPages * EFI_PAGE_SIZE
      );
    }
    //
    // Every used Memory map range should be contained in Memory Allocation Hob.
    //
    if (MemMapTable->Type != EfiConventionalMemory && MemMapTable->Type != EfiReservedMemoryType) {
      BuildMemoryAllocationHob (
        MemMapTable->PhysicalStart,
        MemMapTable->NumberOfPages * EFI_PAGE_SIZE,
        MemMapTable->Type
      );
    }
  }
  return EFI_SUCCESS;
}



CHAR8 * mMemoryTypeStr[] = {
  "EfiReservedMemoryType",
  "EfiLoaderCode",
  "EfiLoaderData",
  "EfiBootServicesCode",
  "EfiBootServicesData",
  "EfiRuntimeServicesCode",
  "EfiRuntimeServicesData",
  "EfiConventionalMemory",
  "EfiUnusableMemory",
  "EfiACPIReclaimMemory",
  "EfiACPIMemoryNVS",
  "EfiMemoryMappedIO",
  "EfiMemoryMappedIOPortSpace",
  "EfiPalCode",
  "EfiPersistentMemory",
  "EfiMaxMemoryType"
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




INTN
EFIAPI
MergeMemDescriptorIntoOne (
  UINTN                     FrontCount,
  EFI_MEMORY_DESCRIPTOR     *Front,
  UINTN                     BackCount,
  EFI_MEMORY_DESCRIPTOR     *Back,
  UINTN                     DestCount,
  EFI_MEMORY_DESCRIPTOR     *Dest
  ) 
{
  EFI_PHYSICAL_ADDRESS     Cursor;
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
  Cursor = MIN(Back[0].PhysicalStart, Front[0].PhysicalStart);
  if (Cursor == Front[0].PhysicalStart) {
    CopyMem(&Dest[0], &Front[0], sizeof(EFI_MEMORY_DESCRIPTOR));
  } else {
    CopyMem(&Dest[0], &Back[0], sizeof(EFI_MEMORY_DESCRIPTOR));
    Dest[0].NumberOfPages = 1;
  }

  TempEnd = Dest[TempDestIndex].PhysicalStart + Dest[TempDestIndex].NumberOfPages * EFI_PAGE_SIZE;
  while (TempEnd != End) {
    //DEBUG ((DEBUG_ERROR, "TempDestIndex = %d\n", TempDestIndex));
    //DEBUG ((DEBUG_ERROR, "TempEnd = 0x%lx\n", TempEnd));
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
  //if front[x] is right after dest[temp].end or containing dest[temp].end
  //  if same type and attr
  //    merge
  //  else
  //    append {dest[temp].end front[x].end}
  //    temp ++
  //else
  //  if back[x] is right after dest[temp].end or containing dest[temp].end
  //    if same type and attr
  //      merge to min(front[x].end, back[x])
  //    else
  //      append {dest[temp].end min(front[x].end, back[x])}
  //      temp ++
//
}

INTN
EFIAPI
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

INTN
EFIAPI
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

VOID
Hob2MemDescriptor (
  EFI_MEMORY_DESCRIPTOR        *MemoryDescriptor,
  EFI_HOB_MEMORY_ALLOCATION    *MemoryAllocationHob,
  EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceHobAttribute
 )
{
  ASSERT ((MemoryAllocationHob->AllocDescriptor.MemoryLength & (EFI_PAGE_SIZE - 1)) == 0);

  MemoryDescriptor->PhysicalStart = MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress;
  MemoryDescriptor->NumberOfPages = (MemoryAllocationHob->AllocDescriptor.MemoryLength) / EFI_PAGE_SIZE;
  MemoryDescriptor->Type          = MemoryAllocationHob->AllocDescriptor.MemoryType;
  MemoryDescriptor->VirtualStart  = 0;
  MemoryDescriptor->Attribute     = ConvertResourceDescriptorHobAttributesToCapabilities(ResourceHobAttribute);
}

VOID
PrintMemMap (
  UINTN                     Count,
  EFI_MEMORY_DESCRIPTOR     *MemMap
 )
{
  UINTN Index;
  for(Index = 0; Index < Count; Index++) {
    DEBUG ((DEBUG_ERROR, "  MemMap[%d] [0x%x, 0x%x, as: %a with attribute 0x%lx]\n",
      Index,
      (UINTN) MemMap[Index].PhysicalStart /0x1000, 
      (UINTN) (MemMap[Index].PhysicalStart + MemMap[Index].NumberOfPages * EFI_PAGE_SIZE) /0x1000,
      mMemoryTypeStr[MemMap[Index].Type],
      MemMap[Index].Attribute));
  }
}

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
          (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress <= ResourceHobEnd)){
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
    DEBUG ((DEBUG_ERROR, "ResourceDescriptorHobBuffer[%d].FrontCount = %d\n", Index, ResourceDescriptorHobBuffer[Index].FrontCount));
  }
  MemMapTable =  AllocatePages (EFI_SIZE_TO_PAGES (sizeof (EFI_MEMORY_DESCRIPTOR) * TotalDestCount));
  //
  // From now on, there should be no code to allocate pool or page.
  //

  for (Index = 0; Index < ResourceHobCount; Index++) {
    ResourceHobStart = ResourceDescriptorHobBuffer[Index].ResourceHob->PhysicalStart;
    ResourceHobEnd = ResourceDescriptorHobBuffer[Index].ResourceHob->PhysicalStart + ResourceDescriptorHobBuffer[Index].ResourceHob->ResourceLength;
    ResourceHobAttribute = ResourceDescriptorHobBuffer[Index].ResourceHob->ResourceAttribute;
    //ResourceDescriptorHobBuffer[Index].ResourceHob->Header.HobType = EFI_HOB_TYPE_UNUSED;
    if (ResourceDescriptorHobBuffer[Index].FrontCount != 0) {
      MemoryAllocationHobCount = 0;
      Hob.Raw = GetHobList();
      while (!END_OF_HOB_LIST (Hob)) {
        if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
          if ((Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress >= ResourceHobStart) &&
            (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress <= ResourceHobEnd)){
            Hob2MemDescriptor(&(ResourceDescriptorHobBuffer[Index].FrontBuffer[MemoryAllocationHobCount]), Hob.MemoryAllocation, ResourceHobAttribute);
            //Hob.Header->HobType = EFI_HOB_TYPE_UNUSED;
            MemoryAllocationHobCount++;
            DEBUG ((DEBUG_ERROR, "[%d] = %d  ", Index, MemoryAllocationHobCount));
            ASSERT (MemoryAllocationHobCount <= ResourceDescriptorHobBuffer[Index].FrontCount);
          }
        }
        Hob.Raw = GET_NEXT_HOB (Hob);
      }
      DEBUG ((DEBUG_ERROR, "\n"));
      ResourceDescriptorHobBuffer[Index].FrontCount = MemoryAllocationHobCount;
      PerformQuickSort (ResourceDescriptorHobBuffer[Index].FrontBuffer, MemoryAllocationHobCount, sizeof (EFI_MEMORY_DESCRIPTOR), (SORT_COMPARE) MemoryResourceDescriptorCompare);
    }
    
    Back = ResourceDescriptorHobBuffer[Index].BackBuffer;
    if (ResourceDescriptorHobBuffer[Index].ContainingHobList) {
      Hob.Raw = GetHobList();

      DEBUG ((DEBUG_ERROR, "  ResourceHobStart = %x\n", (UINTN) ResourceHobStart));
      DEBUG ((DEBUG_ERROR, "  Hob.HandoffInformationTable->EfiFreeMemoryBottom = %x\n", (UINTN) Hob.HandoffInformationTable->EfiFreeMemoryBottom));
      DEBUG ((DEBUG_ERROR, "  Hob.HandoffInformationTable->EfiFreeMemoryTop = %x\n", (UINTN) Hob.HandoffInformationTable->EfiFreeMemoryTop));
      DEBUG ((DEBUG_ERROR, "  Hob.HandoffInformationTable->EfiMemoryBottom = %x\n", (UINTN) Hob.HandoffInformationTable->EfiMemoryBottom));
      DEBUG ((DEBUG_ERROR, "  Hob.HandoffInformationTable->EfiMemoryTop = %x\n", (UINTN) Hob.HandoffInformationTable->EfiMemoryTop));

      DEBUG ((DEBUG_ERROR, "  ResourceDescriptorHobBuffer[Index].ResourceHob->ResourceLength = %x\n", (UINTN) ResourceDescriptorHobBuffer[Index].ResourceHob->ResourceLength));
      
      
      DEBUG ((DEBUG_ERROR, "  ResourceHobStart = %x\n", (UINTN) ResourceHobStart));
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
      DEBUG ((DEBUG_ERROR, "Front\n"));
      PrintMemMap (
        ResourceDescriptorHobBuffer[Index].FrontCount,
        ResourceDescriptorHobBuffer[Index].FrontBuffer
      );
      DEBUG ((DEBUG_ERROR, "Back\n"));
      PrintMemMap (
        ResourceDescriptorHobBuffer[Index].BackCount,
        ResourceDescriptorHobBuffer[Index].BackBuffer
      );
      ResourceDescriptorHobBuffer[Index].DestCount = MergeMemDescriptorIntoOne (
        ResourceDescriptorHobBuffer[Index].FrontCount,
        ResourceDescriptorHobBuffer[Index].FrontBuffer,
        ResourceDescriptorHobBuffer[Index].BackCount,
        ResourceDescriptorHobBuffer[Index].BackBuffer,
        ResourceDescriptorHobBuffer[Index].DestCount,
        ResourceDescriptorHobBuffer[Index].DestBuffer
      );
      DEBUG ((DEBUG_ERROR, "Dest\n"));
      PrintMemMap (
        ResourceDescriptorHobBuffer[Index].DestCount,
        ResourceDescriptorHobBuffer[Index].DestBuffer
      );
    } else {
      CopyMem (ResourceDescriptorHobBuffer[Index].DestBuffer, ResourceDescriptorHobBuffer[Index].BackBuffer, sizeof (EFI_MEMORY_DESCRIPTOR) * ResourceDescriptorHobBuffer[Index].BackCount);
      ResourceDescriptorHobBuffer[Index].DestCount = ResourceDescriptorHobBuffer[Index].BackCount;
    }
    TotalDestCountUsed += ResourceDescriptorHobBuffer[Index].DestCount;
  }
  DEBUG ((DEBUG_ERROR, "Final Dest\n"));
  PrintMemMap (
    TotalDestCountUsed,
    ResourceDescriptorHobBuffer[0].DestBuffer
  );

  MemoryMapHob->Count = TotalDestCountUsed;
  MemoryMapHob->MemoryMap = (EFI_PHYSICAL_ADDRESS) (UINTN) ResourceDescriptorHobBuffer[0].DestBuffer;
  //ASSERT (FALSE);
  return EFI_SUCCESS;
}
