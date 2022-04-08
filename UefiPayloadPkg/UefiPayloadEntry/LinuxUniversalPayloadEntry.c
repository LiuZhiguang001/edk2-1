/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"
#include "linux.h"

#define MEMORY_ATTRIBUTE_MASK  (EFI_RESOURCE_ATTRIBUTE_PRESENT             |        \
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

#define TESTED_MEMORY_ATTRIBUTES  (EFI_RESOURCE_ATTRIBUTE_PRESENT     |     \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED      )

#define STACK_SIZE  0x20000
extern VOID  *mHobList;

/**
  Print all HOBs info from the HOB list.

  @return The pointer to the HOB list.
**/
VOID
PrintHob (
  IN CONST VOID  *HobStart
  );


/**
  Add HOB into HOB list

  @param[in]  Hob    The HOB to be added into the HOB list.
**/
VOID
AddNewHob (
  IN EFI_PEI_HOB_POINTERS  *Hob
  )
{
  EFI_PEI_HOB_POINTERS  NewHob;

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
  IN VOID                  *HobList,
  IN EFI_PHYSICAL_ADDRESS  Base,
  IN EFI_PHYSICAL_ADDRESS  Top
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;

  for (Hob.Raw = (UINT8 *)HobList; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
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
  IN VOID                         *HobList,
  IN UINTN                        MinimalNeededSize,
  IN EFI_HOB_RESOURCE_DESCRIPTOR  *ExceptResourceHob
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ReturnResourceHob;

  ReturnResourceHob = NULL;

  for (Hob.Raw = (UINT8 *)HobList; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
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

/**
  Check the HOB and decide if it is need inside Payload

  Payload maintainer may make decision which HOB is need or needn't
  Then add the check logic in the function.

  @param[in] Hob The HOB to check

  @retval TRUE  If HOB is need inside Payload
  @retval FALSE If HOB is needn't inside Payload
**/
BOOLEAN
IsHobNeed (
  EFI_PEI_HOB_POINTERS  Hob
  )
{
  if (Hob.Header->HobType == EFI_HOB_TYPE_HANDOFF) {
    return FALSE;
  }

  if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
    if (CompareGuid (&Hob.MemoryAllocationModule->MemoryAllocationHeader.Name, &gEfiHobMemoryAllocModuleGuid)) {
      return FALSE;
    }
  }

  // Arrive here mean the HOB is need
  return TRUE;
}

EFI_STATUS
EFIAPI
BuildMemoryMap (
  VOID
  );

VOID
EFIAPI
LoadLinux (
  IN  UINT32  BootParam,
  IN  UINT32  KernelBase
  );

/**
  It will build HOBs based on information from bootloaders.

  @param[in]  BootloaderParameter   The starting memory address of bootloader parameter block.
  @param[out] DxeFv                 The pointer to the DXE FV in memory.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required HOBs.
**/
EFI_STATUS
BuildBootParamsAndBootToLinux (
  IN  UINTN  BootloaderParameter
  )
{
  EFI_PEI_HOB_POINTERS          Hob;
  UNIVERSAL_PAYLOAD_EXTRA_DATA  *ExtraData;
  UINT8                         *GuidHob;
  UNIVERSAL_PAYLOAD_MEMORY_MAP  *MemoryMapHob;
  VOID                          *NewKernelBase;
  struct linux_params           *params;
  UINTN                         IndexOfLinuxKernel;
  UINTN                         IndexOfInitramfs;
  UINTN                         IndexOfBootParams;
  UINTN                         Index;
  VOID                          *Cmdline;
  CHAR8  *Defaultcmdline = "loglevel=7 earlyprintk=serial,ttyS0,115200 console=ttyS0,115200 disable_mtrr_cleanup";

  Hob.Raw = (UINT8 *)BootloaderParameter;
  //
  // Since payload created new Hob, move all hobs except PHIT from boot loader hob list.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (IsHobNeed (Hob)) {
      // Add this hob to payload HOB
      AddNewHob (&Hob);
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  
  //
  // Get linux kernel and intiramfs
  //
  GuidHob = GetFirstGuidHob (&gUniversalPayloadExtraDataGuid);
  ASSERT (GuidHob != NULL);
  ExtraData = (UNIVERSAL_PAYLOAD_EXTRA_DATA *)GET_GUID_HOB_DATA (GuidHob);
  IndexOfLinuxKernel = (UINTN)-1;
  IndexOfInitramfs   = (UINTN)-1;
  IndexOfBootParams  = (UINTN)-1;

  for (Index = 0; Index < ExtraData->Count; Index++) {
    if (AsciiStrCmp (ExtraData->Entry[Index].Identifier, "linux") == 0) {
      ASSERT (IndexOfLinuxKernel == (UINTN)-1);
      IndexOfLinuxKernel = Index;
    }

    if (AsciiStrCmp (ExtraData->Entry[Index].Identifier, "initramfs") == 0) {
      ASSERT (IndexOfInitramfs == (UINTN)-1);
      IndexOfInitramfs = Index;
    }

    if (AsciiStrCmp (ExtraData->Entry[Index].Identifier, "bootparams") == 0) {
      ASSERT (IndexOfBootParams == (UINTN)-1);
      IndexOfBootParams = Index;
    }
  }
  ASSERT (IndexOfLinuxKernel != (UINTN)-1);
  ASSERT (IndexOfBootParams != (UINTN)-1);
  
  Cmdline = AllocateAlignedPages (EFI_SIZE_TO_PAGES (sizeof (struct linux_params)), 1*1024*1024);
  CopyMem (Cmdline, Defaultcmdline, (UINTN)AsciiStrLen (Defaultcmdline));

  // struct linux_params params;
  params = AllocateAlignedPages (EFI_SIZE_TO_PAGES (sizeof (struct linux_params)), 16*1024*1024);
  CopyMem (params, (VOID*)(UINTN) ExtraData->Entry[IndexOfBootParams].Base, (UINTN) ExtraData->Entry[IndexOfBootParams].Size);
  params->orig_video_mode   = 3;
  params->orig_video_cols   = 80;
  params->orig_video_lines  = 25;
  params->orig_video_isVGA  = 1;
  params->orig_video_points = 16;
  params->loader_type = 0xff;
  params->cmd_line_ptr = (UINTN)Cmdline;

  NewKernelBase =AllocateAlignedPages (EFI_SIZE_TO_PAGES (MAX(ExtraData->Entry[IndexOfLinuxKernel].Size, params->init_size)), params->kernel_alignment);
  DEBUG ((DEBUG_INFO, "NewKernelBase = 0x%x\n", (UINTN)NewKernelBase));
  DEBUG ((DEBUG_INFO, "ExtraData->Entry[IndexOfLinuxKernel].Size = 0x%x\n", (UINTN)ExtraData->Entry[IndexOfLinuxKernel].Size));
  DEBUG ((DEBUG_INFO, "ExtraData->Entry[IndexOfLinuxKernel].Base = 0x%x\n", (UINTN)ExtraData->Entry[IndexOfLinuxKernel].Base));
  DEBUG ((DEBUG_INFO, "init_size = 0x%x\n", (UINTN)params->init_size));
  DEBUG ((DEBUG_INFO, "relocatable_kernel = 0x%x\n", (UINTN)params->relocatable_kernel));
  DEBUG ((DEBUG_INFO, "kernel_alignment  = 0x%x\n", (UINTN)params->kernel_alignment));

  CopyMem (NewKernelBase, (UINT8 *)(UINTN)ExtraData->Entry[IndexOfLinuxKernel].Base , ExtraData->Entry[IndexOfLinuxKernel].Size);
  params->kernel_start = (UINT32)(UINTN)NewKernelBase;

  if (IndexOfInitramfs != (UINTN)-1) {
    params->initrd_start = ExtraData->Entry[IndexOfInitramfs].Base;
    params->initrd_size  = ExtraData->Entry[IndexOfInitramfs].Size;
  }

  //
  // All memory allocation is done. Create memory map.
  //
  BuildMemoryMap ();

  //
  // Create E820 memory table
  //
  MemoryMapHob = GetFirstGuidHob (&gUniversalPayloadMemoryMapGuid);
  MemoryMapHob = GET_GUID_HOB_DATA (MemoryMapHob);
  for (Index = 0; Index < MemoryMapHob->Count; Index++) {
    params->e820_map[Index].addr = MemoryMapHob->MemoryMap[Index].PhysicalStart;
    params->e820_map[Index].size = EFI_PAGES_TO_SIZE (MemoryMapHob->MemoryMap[Index].NumberOfPages);
    params->e820_map[Index].type = MemoryMapHob->MemoryMap[Index].Type;
    if (params->e820_map[Index].type == EfiConventionalMemory) {
      params->e820_map[Index].type = E820_RAM;
    } else {
      params->e820_map[Index].type = E820_RESERVED;
    }
  }
  params->e820_map_nr = MemoryMapHob->Count;

  DEBUG ((DEBUG_INFO, "LoadLinux = 0x%x\n", LoadLinux));
  LoadLinux ((UINT32)(VOID *)params, (UINT32)(params->kernel_start));

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
FindMemoryAndSwitchStack (
  IN  UINTN  BootloaderParameter
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  UINTN                        MinimalNeededSize;
  EFI_PHYSICAL_ADDRESS         FreeMemoryBottom;
  EFI_PHYSICAL_ADDRESS         FreeMemoryTop;
  EFI_PHYSICAL_ADDRESS         MemoryBottom;
  EFI_PHYSICAL_ADDRESS         MemoryTop;
  EFI_HOB_RESOURCE_DESCRIPTOR  *PhitResourceHob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;

  Hob.Raw           = (UINT8 *)BootloaderParameter;
  MinimalNeededSize = FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);

  ASSERT (Hob.Raw != NULL);
  ASSERT ((UINTN)Hob.HandoffInformationTable->EfiFreeMemoryTop == Hob.HandoffInformationTable->EfiFreeMemoryTop);
  ASSERT ((UINTN)Hob.HandoffInformationTable->EfiMemoryTop == Hob.HandoffInformationTable->EfiMemoryTop);
  ASSERT ((UINTN)Hob.HandoffInformationTable->EfiFreeMemoryBottom == Hob.HandoffInformationTable->EfiFreeMemoryBottom);
  ASSERT ((UINTN)Hob.HandoffInformationTable->EfiMemoryBottom == Hob.HandoffInformationTable->EfiMemoryBottom);
  DEBUG ((DEBUG_ERROR, "kuang %a: %d\n", __FILE__, __LINE__));
  //
  // Try to find Resource Descriptor HOB that contains Hob range EfiMemoryBottom..EfiMemoryTop
  //
  PhitResourceHob = FindResourceDescriptorByRange (Hob.Raw, Hob.HandoffInformationTable->EfiMemoryBottom, Hob.HandoffInformationTable->EfiMemoryTop);
  if (PhitResourceHob == NULL) {
    //
    // Boot loader's Phit Hob is not in an available Resource Descriptor, find another Resource Descriptor for new Phit Hob
    //
    ResourceHob = FindAnotherHighestBelow4GResourceDescriptor (Hob.Raw, MinimalNeededSize, NULL);
    if (ResourceHob == NULL) {
      return EFI_NOT_FOUND;
    }

    MemoryBottom     = ResourceHob->PhysicalStart + ResourceHob->ResourceLength - MinimalNeededSize;
    FreeMemoryBottom = MemoryBottom;
    FreeMemoryTop    = ResourceHob->PhysicalStart + ResourceHob->ResourceLength;
    MemoryTop        = FreeMemoryTop;
  } else if (PhitResourceHob->PhysicalStart + PhitResourceHob->ResourceLength - Hob.HandoffInformationTable->EfiMemoryTop >= MinimalNeededSize) {
    //
    // New availiable Memory range in new hob is right above memory top in old hob.
    //
    MemoryBottom     = Hob.HandoffInformationTable->EfiFreeMemoryTop;
    FreeMemoryBottom = Hob.HandoffInformationTable->EfiMemoryTop;
    FreeMemoryTop    = FreeMemoryBottom + MinimalNeededSize;
    MemoryTop        = FreeMemoryTop;
  } else if (Hob.HandoffInformationTable->EfiMemoryBottom - PhitResourceHob->PhysicalStart >= MinimalNeededSize) {
    //
    // New availiable Memory range in new hob is right below memory bottom in old hob.
    //
    MemoryBottom     = Hob.HandoffInformationTable->EfiMemoryBottom - MinimalNeededSize;
    FreeMemoryBottom = MemoryBottom;
    FreeMemoryTop    = Hob.HandoffInformationTable->EfiMemoryBottom;
    MemoryTop        = Hob.HandoffInformationTable->EfiMemoryTop;
  } else {
    //
    // In the Resource Descriptor HOB contains boot loader Hob, there is no enough free memory size for payload hob
    // Find another Resource Descriptor Hob
    //
    ResourceHob = FindAnotherHighestBelow4GResourceDescriptor (Hob.Raw, MinimalNeededSize, PhitResourceHob);
    if (ResourceHob == NULL) {
      DEBUG ((DEBUG_ERROR, "kuang %a: %d\n", __FILE__, __LINE__));
      return EFI_NOT_FOUND;
    }

    MemoryBottom     = ResourceHob->PhysicalStart + ResourceHob->ResourceLength - MinimalNeededSize;
    FreeMemoryBottom = MemoryBottom;
    FreeMemoryTop    = ResourceHob->PhysicalStart + ResourceHob->ResourceLength;
    MemoryTop        = FreeMemoryTop;
  }

  DEBUG ((DEBUG_ERROR, "kuang %a: %d\n", __FILE__, __LINE__));
  HobConstructor ((VOID *)(UINTN)MemoryBottom, (VOID *)(UINTN)MemoryTop, (VOID *)(UINTN)FreeMemoryBottom, (VOID *)(UINTN)FreeMemoryTop);
  //
  // From now on, mHobList will point to the new Hob range.
  // Since we can allocate memory, allocate a new stack.
  //
  EFI_PHYSICAL_ADDRESS  StackBase;
  EFI_PHYSICAL_ADDRESS  StackTop;

  StackBase = (EFI_PHYSICAL_ADDRESS)AllocateAlignedPages (EFI_SIZE_TO_PAGES (STACK_SIZE), CPU_STACK_ALIGNMENT);
  StackTop  = StackBase + EFI_SIZE_TO_PAGES (STACK_SIZE) * EFI_PAGE_SIZE - CPU_STACK_ALIGNMENT;
  StackTop  = (EFI_PHYSICAL_ADDRESS)(UINTN)ALIGN_POINTER (StackTop, CPU_STACK_ALIGNMENT);
  DEBUG ((DEBUG_ERROR, "kuang going to switch stack 1 %a: %d\n", __FILE__, __LINE__));
  SwitchStack (
    (SWITCH_STACK_ENTRY_POINT)(UINTN)BuildBootParamsAndBootToLinux,
    (VOID *)(UINTN)BootloaderParameter,
    NULL,
    (VOID *)(UINTN)StackTop
    );
  return EFI_ABORTED;
}

/**
  Entry point to the C language phase of UEFI payload.

  @param[in]   BootloaderParameter    The starting address of bootloader parameter block.

  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN UINTN  BootloaderParameter
  )
{
  mHobList = (VOID *)BootloaderParameter;

  // Call constructor for all libraries
  ProcessLibraryConstructorList ();

  DEBUG ((DEBUG_INFO, "Entering Linux Universal Payload...\n"));
  DEBUG ((DEBUG_INFO, "sizeof(UINTN) = 0x%x\n", sizeof (UINTN)));

  DEBUG_CODE (
    //
    // Dump the Hobs from boot loader
    //
    PrintHob (mHobList);
    );

  // Initialize floating point operating environment to be compliant with UEFI spec.
  InitializeFloatingPointUnits ();

  // Build HOB based on information from Bootloader
  FindMemoryAndSwitchStack (BootloaderParameter);

  // Should not get here
  CpuDeadLoop ();
  return EFI_SUCCESS;
}
