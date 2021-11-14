/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"

#define MEMORY_ATTRIBUTE_MASK         (EFI_RESOURCE_ATTRIBUTE_PRESENT             | \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED         | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED              | \
                                       EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED      | \
                                       EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED     | \
                                       EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED | \
                                       EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTED | \
                                       EFI_RESOURCE_ATTRIBUTE_16_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_32_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_64_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_PERSISTENT          )

#define TESTED_MEMORY_ATTRIBUTES      (EFI_RESOURCE_ATTRIBUTE_PRESENT     | \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED      )

extern VOID  *mHobList;

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

/**
  Print all HOBs info from the HOB list.

  @return The pointer to the HOB list.
**/
VOID
PrintHob (
  IN CONST VOID             *HobStart
  );


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

/**
  Some bootloader may pass a pcd database, and UPL also contain a PCD database.
  Dxe PCD driver has the assumption that the two PCD database can be catenated and
  the local token number should be successive.
  This function will fix up the UPL PCD database to meet that assumption.

  @param[in]   DxeFv         The FV where to find the Universal PCD database.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval other              Failed to fix up.
**/
EFI_STATUS
FixUpPcdDatabase (
  IN  EFI_FIRMWARE_VOLUME_HEADER *DxeFv
  )
{
  EFI_STATUS                  Status;
  EFI_FFS_FILE_HEADER         *FileHeader;
  VOID                        *PcdRawData;
  PEI_PCD_DATABASE            *PeiDatabase;
  PEI_PCD_DATABASE            *UplDatabase;
  EFI_HOB_GUID_TYPE           *GuidHob;
  DYNAMICEX_MAPPING           *ExMapTable;
  UINTN                       Index;

  GuidHob = GetFirstGuidHob (&gPcdDataBaseHobGuid);
  if (GuidHob == NULL) {
    //
    // No fix-up is needed.
    //
    return EFI_SUCCESS;
  }
  PeiDatabase = (PEI_PCD_DATABASE *) GET_GUID_HOB_DATA (GuidHob);
  DEBUG ((DEBUG_INFO, "Find the Pei PCD data base, the total local token number is %d\n", PeiDatabase->LocalTokenCount));

  Status = FvFindFileByTypeGuid (DxeFv, EFI_FV_FILETYPE_DRIVER, PcdGetPtr (PcdPcdDriverFile), &FileHeader);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = FileFindSection (FileHeader, EFI_SECTION_RAW, &PcdRawData);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UplDatabase = (PEI_PCD_DATABASE *) PcdRawData;
  ExMapTable  = (DYNAMICEX_MAPPING *) (UINTN) ((UINTN) PcdRawData + UplDatabase->ExMapTableOffset);

  for (Index = 0; Index < UplDatabase->ExTokenCount; Index++) {
    ExMapTable[Index].TokenNumber += PeiDatabase->LocalTokenCount;
  }
  DEBUG ((DEBUG_INFO, "Fix up UPL PCD database successfully\n"));
  return EFI_SUCCESS;
}

/**
  Add HOB into HOB list

  @param[in]  Hob    The HOB to be added into the HOB list.
**/
VOID
AddNewHob (
  IN EFI_PEI_HOB_POINTERS    *Hob
  )
{
  EFI_PEI_HOB_POINTERS    NewHob;

  if (Hob->Raw == NULL) {
    return;
  }
  NewHob.Header = CreateHob (Hob->Header->HobType, Hob->Header->HobLength);

  if (NewHob.Header != NULL) {
    CopyMem (NewHob.Header + 1, Hob->Header + 1, Hob->Header->HobLength - sizeof (EFI_HOB_GENERIC_HEADER));
  }
}

/**
  Found the Resource Descriptor HOB that contains a range (Base, Top)

  @param[in] HobList    Hob start address
  @param[in] Base       Memory start address
  @param[in] Top        Memory end address.

  @retval     The pointer to the Resource Descriptor HOB.
**/
EFI_HOB_RESOURCE_DESCRIPTOR *
FindResourceDescriptorByRange (
  IN VOID                      *HobList,
  IN EFI_PHYSICAL_ADDRESS      Base,
  IN EFI_PHYSICAL_ADDRESS      Top
  )
{
  EFI_PEI_HOB_POINTERS             Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR      *ResourceHob;

  for (Hob.Raw = (UINT8 *) HobList; !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {
    //
    // Skip all HOBs except Resource Descriptor HOBs
    //
    if (GET_HOB_TYPE (Hob) != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that do not describe tested system memory
    //
    ResourceHob = Hob.ResourceDescriptor;
    if (ResourceHob->ResourceType != EFI_RESOURCE_SYSTEM_MEMORY) {
      continue;
    }
    if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) != TESTED_MEMORY_ATTRIBUTES) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that do not contain the PHIT range EfiFreeMemoryBottom..EfiFreeMemoryTop
    //
    if (Base < ResourceHob->PhysicalStart) {
      continue;
    }
    if (Top > (ResourceHob->PhysicalStart + ResourceHob->ResourceLength)) {
      continue;
    }
    return ResourceHob;
  }
  return NULL;
}

/**
  Find the highest below 4G memory resource descriptor, except the input Resource Descriptor.

  @param[in] HobList                 Hob start address
  @param[in] MinimalNeededSize       Minimal needed size.
  @param[in] ExceptResourceHob       Ignore this Resource Descriptor.

  @retval     The pointer to the Resource Descriptor HOB.
**/
EFI_HOB_RESOURCE_DESCRIPTOR *
FindAnotherHighestBelow4GResourceDescriptor (
  IN VOID                        *HobList,
  IN UINTN                       MinimalNeededSize,
  IN EFI_HOB_RESOURCE_DESCRIPTOR *ExceptResourceHob
  )
{
  EFI_PEI_HOB_POINTERS             Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR      *ResourceHob;
  EFI_HOB_RESOURCE_DESCRIPTOR      *ReturnResourceHob;
  ReturnResourceHob = NULL;

  for (Hob.Raw = (UINT8 *) HobList; !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {
    //
    // Skip all HOBs except Resource Descriptor HOBs
    //
    if (GET_HOB_TYPE (Hob) != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that do not describe tested system memory
    //
    ResourceHob = Hob.ResourceDescriptor;
    if (ResourceHob->ResourceType != EFI_RESOURCE_SYSTEM_MEMORY) {
      continue;
    }
    if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) != TESTED_MEMORY_ATTRIBUTES) {
      continue;
    }

    //
    // Skip if the Resource Descriptor HOB equals to ExceptResourceHob
    //
    if (ResourceHob == ExceptResourceHob) {
      continue;
    }
    //
    // Skip Resource Descriptor HOBs that are beyond 4G
    //
    if ((ResourceHob->PhysicalStart + ResourceHob->ResourceLength) > BASE_4GB) {
      continue;
    }
    //
    // Skip Resource Descriptor HOBs that are too small
    //
    if (ResourceHob->ResourceLength < MinimalNeededSize) {
      continue;
    }

    //
    // Return the topest Resource Descriptor
    //
    if (ReturnResourceHob == NULL) {
      ReturnResourceHob = ResourceHob;
    } else {
      if (ReturnResourceHob->PhysicalStart < ResourceHob->PhysicalStart) {
        ReturnResourceHob = ResourceHob;
      }
    }
  }
  return ReturnResourceHob;
}

EFI_STATUS
FindFreeMemoryFromResourceDescriptorHob (
  IN  EFI_PEI_HOB_POINTERS             HobStart,
  IN  UINTN                            MinimalNeededSize,
  OUT EFI_PHYSICAL_ADDRESS             *FreeMemoryBottom,
  OUT EFI_PHYSICAL_ADDRESS             *FreeMemoryTop,
  OUT EFI_PHYSICAL_ADDRESS             *MemoryBottom,
  OUT EFI_PHYSICAL_ADDRESS             *MemoryTop
  )
{
  EFI_HOB_RESOURCE_DESCRIPTOR      *PhitResourceHob;
  EFI_HOB_RESOURCE_DESCRIPTOR      *ResourceHob;

  ASSERT (HobStart.Raw != NULL);
  ASSERT ((UINTN) HobStart.HandoffInformationTable->EfiFreeMemoryTop == HobStart.HandoffInformationTable->EfiFreeMemoryTop);
  ASSERT ((UINTN) HobStart.HandoffInformationTable->EfiMemoryTop == HobStart.HandoffInformationTable->EfiMemoryTop);
  ASSERT ((UINTN) HobStart.HandoffInformationTable->EfiFreeMemoryBottom == HobStart.HandoffInformationTable->EfiFreeMemoryBottom);
  ASSERT ((UINTN) HobStart.HandoffInformationTable->EfiMemoryBottom == HobStart.HandoffInformationTable->EfiMemoryBottom);


  //
  // Try to find Resource Descriptor HOB that contains Hob range EfiMemoryBottom..EfiMemoryTop
  //
  PhitResourceHob = FindResourceDescriptorByRange(HobStart.Raw, HobStart.HandoffInformationTable->EfiMemoryBottom, HobStart.HandoffInformationTable->EfiMemoryTop);
  if (PhitResourceHob == NULL) {
    //
    // Boot loader's Phit Hob is not in an available Resource Descriptor, find another Resource Descriptor for new Phit Hob
    //
    ResourceHob = FindAnotherHighestBelow4GResourceDescriptor(HobStart.Raw, MinimalNeededSize, NULL);
    if (ResourceHob == NULL) {
      return EFI_NOT_FOUND;
    }

    *MemoryBottom     = ResourceHob->PhysicalStart + ResourceHob->ResourceLength - MinimalNeededSize;
    *FreeMemoryBottom = *MemoryBottom;
    *FreeMemoryTop    = ResourceHob->PhysicalStart + ResourceHob->ResourceLength;
    *MemoryTop        = *FreeMemoryTop;
  } else if (PhitResourceHob->PhysicalStart + PhitResourceHob->ResourceLength - HobStart.HandoffInformationTable->EfiMemoryTop >= MinimalNeededSize) {
    //
    // New availiable Memory range in new hob is right above memory top in old hob.
    //
    *MemoryBottom     = HobStart.HandoffInformationTable->EfiFreeMemoryTop;
    *FreeMemoryBottom = HobStart.HandoffInformationTable->EfiMemoryTop;
    *FreeMemoryTop    = *FreeMemoryBottom + MinimalNeededSize;
    *MemoryTop        = *FreeMemoryTop;
  } else if (HobStart.HandoffInformationTable->EfiMemoryBottom - PhitResourceHob->PhysicalStart >= MinimalNeededSize) {
    //
    // New availiable Memory range in new hob is right below memory bottom in old hob.
    //
    *MemoryBottom     = HobStart.HandoffInformationTable->EfiMemoryBottom - MinimalNeededSize;
    *FreeMemoryBottom = *MemoryBottom;
    *FreeMemoryTop    = HobStart.HandoffInformationTable->EfiMemoryBottom;
    *MemoryTop        = HobStart.HandoffInformationTable->EfiMemoryTop;
  } else {
    //
    // In the Resource Descriptor HOB contains boot loader Hob, there is no enough free memory size for payload hob
    // Find another Resource Descriptor Hob
    //
    ResourceHob = FindAnotherHighestBelow4GResourceDescriptor(HobStart.Raw, MinimalNeededSize, PhitResourceHob);
    if (ResourceHob == NULL) {
      return EFI_NOT_FOUND;
    }

    *MemoryBottom     = ResourceHob->PhysicalStart + ResourceHob->ResourceLength - MinimalNeededSize;
    *FreeMemoryBottom = *MemoryBottom;
    *FreeMemoryTop    = ResourceHob->PhysicalStart + ResourceHob->ResourceLength;
    *MemoryTop        = *FreeMemoryTop;
  }
  return EFI_SUCCESS;
}

EFI_STATUS
FindFreeMemoryFromMemoryMapTable (
  IN  EFI_PEI_HOB_POINTERS             HobStart,
  IN  UINTN                            MinimalNeededSize,
  OUT EFI_PHYSICAL_ADDRESS             *FreeMemoryBottom,
  OUT EFI_PHYSICAL_ADDRESS             *FreeMemoryTop,
  OUT EFI_PHYSICAL_ADDRESS             *MemoryBottom,
  OUT EFI_PHYSICAL_ADDRESS             *MemoryTop
  )
{
  VOID                             *Hob;
  UNIVERSAL_PAYLOAD_MEMORY_MAP     *MemoryMapHob;
  EFI_MEMORY_DESCRIPTOR            *MemoryMapTable;
  UINTN                            FindIndex;
  UINTN                            Index;

  Hob = GetNextGuidHob (&gUniversalPayloadMemoryMapGuid, HobStart.Raw);
  if (Hob == NULL) {
    return EFI_NOT_FOUND;
  }
  MemoryMapHob = (UNIVERSAL_PAYLOAD_MEMORY_MAP *) GET_GUID_HOB_DATA (Hob);
  FindIndex = MemoryMapHob->Count;
  for (Index = 0; Index < MemoryMapHob->Count; Index++) {
    MemoryMapTable = ((EFI_MEMORY_DESCRIPTOR *)MemoryMapHob->MemoryMap) + Index;
    if (MemoryMapTable->NumberOfPages * EFI_PAGE_SIZE + MemoryMapTable->PhysicalStart > BASE_4GB) {
      continue;
    }
    if (MemoryMapTable->Type != EfiConventionalMemory || MemoryMapTable->NumberOfPages * EFI_PAGE_SIZE < MinimalNeededSize) {
      continue;
    }
    if ((MemoryMapTable->Attribute & EFI_MEMORY_PRESENT) != EFI_MEMORY_PRESENT) {
      continue;
    }
    if ((MemoryMapTable->Attribute & EFI_MEMORY_INITIALIZED) != EFI_MEMORY_INITIALIZED) {
      continue;
    }
    if ((MemoryMapTable->Attribute & EFI_MEMORY_TESTED) != EFI_MEMORY_TESTED) {
      continue;
    }
    FindIndex = Index;
  }
  if (FindIndex == MemoryMapHob->Count) {
    return EFI_NOT_FOUND;
  }

  MemoryMapTable = ((EFI_MEMORY_DESCRIPTOR *)MemoryMapHob->MemoryMap) + FindIndex;
  *MemoryTop        = MemoryMapTable->NumberOfPages * EFI_PAGE_SIZE + MemoryMapTable->PhysicalStart;
  *FreeMemoryTop    = *MemoryTop;
  *MemoryBottom     = *MemoryTop - MinimalNeededSize;
  *FreeMemoryBottom = *MemoryBottom;
  return EFI_SUCCESS;
}




/**
  It will build HOBs based on information from bootloaders.

  @param[in]  BootloaderParameter   The starting memory address of bootloader parameter block.
  @param[out] DxeFv                 The pointer to the DXE FV in memory.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required HOBs.
**/
EFI_STATUS
BuildHobs (
  IN  UINTN                       BootloaderParameter,
  OUT EFI_FIRMWARE_VOLUME_HEADER  **DxeFv
  )
{
  EFI_STATUS                       Status;
  EFI_PEI_HOB_POINTERS             Hob;
  UINTN                            MinimalNeededSize;
  EFI_PHYSICAL_ADDRESS             FreeMemoryBottom;
  EFI_PHYSICAL_ADDRESS             FreeMemoryTop;
  EFI_PHYSICAL_ADDRESS             MemoryBottom;
  EFI_PHYSICAL_ADDRESS             MemoryTop;
  UNIVERSAL_PAYLOAD_EXTRA_DATA     *ExtraData;
  UINT8                            *GuidHob;
  EFI_HOB_FIRMWARE_VOLUME          *FvHob;
  UNIVERSAL_PAYLOAD_ACPI_TABLE     *AcpiTable;
  ACPI_BOARD_INFO                  *AcpiBoardInfo;
  VOID                             *MemoryMapHob;

  Hob.Raw = (UINT8 *) BootloaderParameter;
  MinimalNeededSize = FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);
  MemoryMapHob = GetNextGuidHob (&gUniversalPayloadMemoryMapGuid, Hob.Raw);
  //MemoryMapHob = NULL;
  if (MemoryMapHob == NULL) {
    DEBUG ((DEBUG_INFO, "FindFreeMemoryFromResourceDescriptorHob \n"));
    Status = FindFreeMemoryFromResourceDescriptorHob (
               Hob,
               MinimalNeededSize,
               &FreeMemoryBottom,
               &FreeMemoryTop,
               &MemoryBottom,
               &MemoryTop
             );

  } else {
    DEBUG ((DEBUG_INFO, "FindFreeMemoryFromMemoryMapTable \n"));
    Status = FindFreeMemoryFromMemoryMapTable (
               Hob,
               MinimalNeededSize,
               &FreeMemoryBottom,
               &FreeMemoryTop,
               &MemoryBottom,
               &MemoryTop
             );
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }
  HobConstructor ((VOID *) (UINTN) MemoryBottom, (VOID *) (UINTN) MemoryTop, (VOID *) (UINTN) FreeMemoryBottom, (VOID *) (UINTN) FreeMemoryTop);
  //
  // From now on, mHobList will point to the new Hob range.
  //

  //
  // Create resource Hob that Uefi can use based on Memory Map Table.
  //
  if (MemoryMapHob != NULL) {
    CreateHobsBasedOnMemoryMap ((UNIVERSAL_PAYLOAD_MEMORY_MAP *) GET_GUID_HOB_DATA (MemoryMapHob));
  }

  //
  // Create an empty FvHob for the DXE FV that contains DXE core.
  //
  BuildFvHob ((EFI_PHYSICAL_ADDRESS) 0, 0);
  //
  // Since payload created new Hob, move all hobs except PHIT from boot loader hob list.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType != EFI_HOB_TYPE_HANDOFF) {
      // Add this hob to payload HOB
      AddNewHob (&Hob);
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  //
  // Get DXE FV location
  //
  GuidHob = GetFirstGuidHob(&gUniversalPayloadExtraDataGuid);
  ASSERT (GuidHob != NULL);
  ExtraData = (UNIVERSAL_PAYLOAD_EXTRA_DATA *) GET_GUID_HOB_DATA (GuidHob);
  ASSERT (ExtraData->Count == 1);
  ASSERT (AsciiStrCmp (ExtraData->Entry[0].Identifier, "uefi_fv") == 0);

  *DxeFv = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) ExtraData->Entry[0].Base;
  ASSERT ((*DxeFv)->FvLength == ExtraData->Entry[0].Size);

  //
  // Create guid hob for acpi board information
  //
  GuidHob = GetFirstGuidHob(&gUniversalPayloadAcpiTableGuid);
  if (GuidHob != NULL) {
    AcpiTable = (UNIVERSAL_PAYLOAD_ACPI_TABLE *) GET_GUID_HOB_DATA (GuidHob);
    AcpiBoardInfo = BuildHobFromAcpi ((UINT64)AcpiTable->Rsdp);
    ASSERT (AcpiBoardInfo != NULL);
  }

  //
  // Update DXE FV information to first fv hob in the hob list, which
  // is the empty FvHob created before.
  //
  FvHob = GetFirstHob (EFI_HOB_TYPE_FV);
  FvHob->BaseAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) *DxeFv;
  FvHob->Length = (*DxeFv)->FvLength;
  return EFI_SUCCESS;
}

/**
  Entry point to the C language phase of UEFI payload.

  @param[in]   BootloaderParameter    The starting address of bootloader parameter block.

  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN UINTN                     BootloaderParameter
  )
{
  EFI_STATUS                    Status;
  PHYSICAL_ADDRESS              DxeCoreEntryPoint;
  EFI_PEI_HOB_POINTERS          Hob;
  EFI_FIRMWARE_VOLUME_HEADER    *DxeFv;

  mHobList = (VOID *) BootloaderParameter;
  DxeFv    = NULL;
  // Call constructor for all libraries
  ProcessLibraryConstructorList ();

  DEBUG ((DEBUG_INFO, "Entering Universal Payload...\n"));
  DEBUG ((DEBUG_INFO, "sizeof(UINTN) = 0x%x\n", sizeof(UINTN)));

  DEBUG_CODE (
    //
    // Dump the Hobs from boot loader
    //
    PrintHob (mHobList);
  );

  // Initialize floating point operating environment to be compliant with UEFI spec.
  InitializeFloatingPointUnits ();

  // Build HOB based on information from Bootloader
  Status = BuildHobs (BootloaderParameter, &DxeFv);
  ASSERT_EFI_ERROR (Status);
  PrintHob (GetHobList());
  FixUpPcdDatabase (DxeFv);
  Status = UniversalLoadDxeCore (DxeFv, &DxeCoreEntryPoint);
  ASSERT_EFI_ERROR (Status);

  //
  // Mask off all legacy 8259 interrupt sources
  //
  IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, 0xFF);
  IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE,  0xFF);

  Hob.HandoffInformationTable = (EFI_HOB_HANDOFF_INFO_TABLE *) GetFirstHob(EFI_HOB_TYPE_HANDOFF);
  HandOffToDxeCore (DxeCoreEntryPoint, Hob);

  // Should not get here
  CpuDeadLoop ();
  return EFI_SUCCESS;
}
