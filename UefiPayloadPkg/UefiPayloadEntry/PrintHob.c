/** @file
  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"

UINT8* Memorey_Type_List[17] = {
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

typedef
VOID
(EFIAPI *PRINT_ONE_HOB)(
  IN VOID             *HobPointer
  );

typedef struct {
  EFI_GUID           *GUID;
  PRINT_ONE_HOB      PrintHob;
} GUID_HANDLER_PAIR;

VOID
PrintHandOffHob(
  IN VOID             *HobPointer
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = HobPointer;
  DEBUG ((DEBUG_INFO, "   BootMode = 0x%x\n", Hob.HandoffInformationTable->BootMode));
  DEBUG ((DEBUG_INFO, "   EfiMemoryTop = 0x%lx\n", Hob.HandoffInformationTable->EfiMemoryTop));
  DEBUG ((DEBUG_INFO, "   EfiMemoryBottom = 0x%lx\n", Hob.HandoffInformationTable->EfiMemoryBottom));
  DEBUG ((DEBUG_INFO, "   EfiFreeMemoryTop = 0x%lx\n", Hob.HandoffInformationTable->EfiFreeMemoryTop));
  DEBUG ((DEBUG_INFO, "   EfiFreeMemoryBottom = 0x%lx\n", Hob.HandoffInformationTable->EfiFreeMemoryBottom));
  DEBUG ((DEBUG_INFO, "   EfiEndOfHobList = 0x%lx\n", Hob.HandoffInformationTable->EfiEndOfHobList));
}

VOID
PrintGuidHob(
  IN VOID             *HobPointer
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = HobPointer;
  DEBUG ((DEBUG_INFO, "   BootMode = 0x%x\n", Hob.HandoffInformationTable->BootMode));
  DEBUG ((DEBUG_INFO, "   EfiMemoryTop = 0x%lx\n", Hob.HandoffInformationTable->EfiMemoryTop));
  DEBUG ((DEBUG_INFO, "   EfiMemoryBottom = 0x%lx\n", Hob.HandoffInformationTable->EfiMemoryBottom));
  DEBUG ((DEBUG_INFO, "   EfiFreeMemoryTop = 0x%lx\n", Hob.HandoffInformationTable->EfiFreeMemoryTop));
  DEBUG ((DEBUG_INFO, "   EfiFreeMemoryBottom = 0x%lx\n", Hob.HandoffInformationTable->EfiFreeMemoryBottom));
  DEBUG ((DEBUG_INFO, "   EfiEndOfHobList = 0x%lx\n", Hob.HandoffInformationTable->EfiEndOfHobList));
}


UINT8*  HobTypeStringList[13] = {
  "Undefined                       ",   //Undefined                         0x0000
  "EFI_HOB_TYPE_HANDOFF            ",   //EFI_HOB_TYPE_HANDOFF              0x0001
  "EFI_HOB_TYPE_MEMORY_ALLOCATION  ",   //EFI_HOB_TYPE_MEMORY_ALLOCATION    0x0002
  "EFI_HOB_TYPE_RESOURCE_DESCRIPTOR",   //EFI_HOB_TYPE_RESOURCE_DESCRIPTOR  0x0003
  "EFI_HOB_TYPE_GUID_EXTENSION     ",   //EFI_HOB_TYPE_GUID_EXTENSION       0x0004
  "EFI_HOB_TYPE_FV                 ",   //EFI_HOB_TYPE_FV                   0x0005
  "EFI_HOB_TYPE_CPU                ",   //EFI_HOB_TYPE_CPU                  0x0006
  "EFI_HOB_TYPE_MEMORY_POOL        ",   //EFI_HOB_TYPE_MEMORY_POOL          0x0007
  "Undefined                       ",   //Undefined                         0x0008
  "EFI_HOB_TYPE_FV2                ",   //EFI_HOB_TYPE_FV2                  0x0009
  "EFI_HOB_TYPE_LOAD_PEIM_UNUSED   ",   //EFI_HOB_TYPE_LOAD_PEIM_UNUSED     0x000A
  "EFI_HOB_TYPE_UEFI_CAPSULE       ",   //EFI_HOB_TYPE_UEFI_CAPSULE         0x000B
  "EFI_HOB_TYPE_FV3                "    //EFI_HOB_TYPE_FV3                  0x000C
  };  

PRINT_ONE_HOB PrintHobHandlerList[13] = {
  NULL,             //Undefined                         0x0000
  PrintHandOffHob,  //EFI_HOB_TYPE_HANDOFF              0x0001
  NULL,             //EFI_HOB_TYPE_MEMORY_ALLOCATION    0x0002
  NULL,             //EFI_HOB_TYPE_RESOURCE_DESCRIPTOR  0x0003
  PrintGuidHob,     //EFI_HOB_TYPE_GUID_EXTENSION       0x0004
  NULL,             //EFI_HOB_TYPE_FV                   0x0005
  NULL,             //EFI_HOB_TYPE_CPU                  0x0006
  NULL,             //EFI_HOB_TYPE_MEMORY_POOL          0x0007
  NULL,             //Undefined                         0x0008
  NULL,             //EFI_HOB_TYPE_FV2                  0x0009
  NULL,             //EFI_HOB_TYPE_LOAD_PEIM_UNUSED     0x000A
  NULL,             //EFI_HOB_TYPE_UEFI_CAPSULE         0x000B
  NULL              //EFI_HOB_TYPE_FV3                  0x000C
  };     

//GUID_HANDLER_PAIR GuidHandlerPairList[] = {
//  {},
//}
/**
  Print all HOBs info from the HOB list.

  @return The pointer to the HOB list.
**/
VOID
EFIAPI
PrintHob (
  IN CONST VOID             *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINTN                 Count;
  PRINT_ONE_HOB         PrintOneHobHandler;

  ASSERT (HobStart != NULL);
   
  Hob.Raw = (UINT8 *) HobStart;
  DEBUG ((DEBUG_INFO, "Print all Hob information from Hob 0x%p\n", Hob.Raw));

  Count = 0;

  //
  // Parse the HOB list to see which type it is, and print the information.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    DEBUG ((DEBUG_INFO, "HOB[%d]: Type = %s  Offset = 0x%x  Length = %d, ",
      Count, HobTypeStringList[Hob.Header->HobType], (UINTN)Hob.Raw, Hob.Header->HobLength));
    Count++;
    PrintOneHobHandler = PrintHobHandlerList[Hob.Header->HobType];
    if (PrintOneHobHandler == NULL) {
      DEBUG ((DEBUG_ERROR, "Error: there is no print function for this kind of type %d\n", Hob.Header->HobType));
    }
/*
    switch (Hob.Header->HobType) {
      case EFI_HOB_TYPE_HANDOFF:
        DEBUG ((DEBUG_INFO, "whoes type is EFI_HOB_TYPE_HANDOFF\n"));
        DEBUG ((DEBUG_INFO, "   HOB Offset = 0x%x\n", Hob));
        DEBUG ((DEBUG_INFO, "   HobLength = 0x%x\n", Hob.HandoffInformationTable->Header.HobLength));

        break;
      case EFI_HOB_TYPE_MEMORY_ALLOCATION:
        if(CompareGuid (&(Hob.MemoryAllocation->AllocDescriptor.Name), &gEfiHobMemoryAllocStackGuid)) {
          DEBUG ((DEBUG_INFO, "whoes type is EFI_HOB_MEMORY_ALLOCATION_STACK\n"));
          DEBUG ((DEBUG_INFO, "   HOB Offset = 0x%x\n", Hob));
          DEBUG ((DEBUG_INFO, "   HobLength = 0x%x\n", Hob.MemoryAllocationStack->Header.HobLength));
          DEBUG ((DEBUG_INFO, "   MemoryBaseAddress = 0x%lx\n", Hob.MemoryAllocationStack->AllocDescriptor.MemoryBaseAddress));
          DEBUG ((DEBUG_INFO, "   MemoryLength = 0x%lx\n", Hob.MemoryAllocationStack->AllocDescriptor.MemoryLength));
          DEBUG ((DEBUG_INFO, "   MemoryType = %a \n", Memorey_Type_List[Hob.MemoryAllocationStack->AllocDescriptor.MemoryType]));
          break;
        } else if(CompareGuid (&(Hob.MemoryAllocation->AllocDescriptor.Name), &gEfiHobMemoryAllocBspStoreGuid)) {
          DEBUG ((DEBUG_INFO, "whoes type is EFI_HOB_MEMORY_ALLOCATION_BSP_STORE\n"));
          DEBUG ((DEBUG_INFO, "   HOB Offset = 0x%x\n", Hob));
          DEBUG ((DEBUG_INFO, "   HobLength = 0x%x\n", Hob.MemoryAllocationBspStore->Header.HobLength));
          DEBUG ((DEBUG_INFO, "   MemoryBaseAddress = 0x%lx\n", Hob.MemoryAllocationBspStore->AllocDescriptor.MemoryBaseAddress));
          DEBUG ((DEBUG_INFO, "   MemoryLength = 0x%lx\n", Hob.MemoryAllocationBspStore->AllocDescriptor.MemoryLength));
          DEBUG ((DEBUG_INFO, "   MemoryType = %a \n", Memorey_Type_List[Hob.MemoryAllocationBspStore->AllocDescriptor.MemoryType]));
          break;
        }else if(CompareGuid (&(Hob.MemoryAllocation->AllocDescriptor.Name), &gEfiHobMemoryAllocModuleGuid)) {
          DEBUG ((DEBUG_INFO, "whoes type is EFI_HOB_MEMORY_ALLOCATION_MODULE\n"));
          DEBUG ((DEBUG_INFO, "   HOB Offset = 0x%x\n", Hob));
          DEBUG ((DEBUG_INFO, "   HobLength = 0x%x\n", Hob.MemoryAllocationModule->Header.HobLength));
          DEBUG ((DEBUG_INFO, "   MemoryBaseAddress = 0x%lx\n", Hob.MemoryAllocationModule->MemoryAllocationHeader.MemoryBaseAddress));
          DEBUG ((DEBUG_INFO, "   MemoryLength = 0x%lx\n", Hob.MemoryAllocationModule->MemoryAllocationHeader.MemoryLength));
          DEBUG ((DEBUG_INFO, "   MemoryType = %a \n", Memorey_Type_List[Hob.MemoryAllocationModule->MemoryAllocationHeader.MemoryType]));
          DEBUG ((DEBUG_INFO, "   Module Name = %g\n", Hob.MemoryAllocationModule->ModuleName));
          DEBUG ((DEBUG_INFO, "   Physical Address = 0x%x\n", Hob.MemoryAllocationModule->EntryPoint));
          break;
        }else {
          DEBUG ((DEBUG_INFO, "whoes type is EFI_HOB_TYPE_MEMORY_ALLOCATION\n"));
          DEBUG ((DEBUG_INFO, "   HOB Offset = 0x%x\n", Hob));
          DEBUG ((DEBUG_INFO, "   HobLength = 0x%x\n", Hob.MemoryAllocation->Header.HobLength));
          DEBUG ((DEBUG_INFO, "   MemoryBaseAddress = 0x%lx\n", Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress));
          DEBUG ((DEBUG_INFO, "   MemoryLength = 0x%lx\n", Hob.MemoryAllocation->AllocDescriptor.MemoryLength));
          DEBUG ((DEBUG_INFO, "   MemoryType = %a \n", Memorey_Type_List[Hob.MemoryAllocation->AllocDescriptor.MemoryType]));
          break;
        }
      case EFI_HOB_TYPE_RESOURCE_DESCRIPTOR:
        DEBUG ((DEBUG_INFO, "whoes type is EFI_HOB_TYPE_RESOURCE_DESCRIPTOR\n"));
        DEBUG ((DEBUG_INFO, "   HOB Offset = 0x%x\n", Hob));
        DEBUG ((DEBUG_INFO, "   HobLength = 0x%x\n", Hob.ResourceDescriptor->Header.HobLength));
        DEBUG ((DEBUG_INFO, "   ResourceType = 0x%x\n", Hob.ResourceDescriptor->ResourceType));
        DEBUG ((DEBUG_INFO, "   ResourceAttribute = 0x%x\n", Hob.ResourceDescriptor->ResourceAttribute));
        DEBUG ((DEBUG_INFO, "   PhysicalStart = 0x%x\n", Hob.ResourceDescriptor->PhysicalStart));
        DEBUG ((DEBUG_INFO, "   ResourceLength = 0x%x\n", Hob.ResourceDescriptor->ResourceLength));
        break;
      case EFI_HOB_TYPE_GUID_EXTENSION:
        DEBUG ((DEBUG_INFO, "whoes type is EFI_HOB_TYPE_GUID_EXTENSION\n"));
        DEBUG ((DEBUG_INFO, "   HOB Offset = %lx\n", Hob));
        DEBUG ((DEBUG_INFO, "   Name = %g\n", &Hob.Guid->Name));
        DEBUG ((DEBUG_INFO, "   HobLength = %d\n", Hob.Guid->Header.HobLength));
        break;
      case EFI_HOB_TYPE_FV:
        DEBUG ((DEBUG_INFO, "whoes type is EFI_HOB_TYPE_FV\n"));
        DEBUG ((DEBUG_INFO, "   HOB Offset = 0x%x\n", Hob));
        DEBUG ((DEBUG_INFO, "   HobLength = 0x%x\n", Hob.FirmwareVolume->Header.HobLength));
        DEBUG ((DEBUG_INFO, "   BaseAddress = 0x%lx\n", Hob.FirmwareVolume->BaseAddress));
        DEBUG ((DEBUG_INFO, "   Length = 0x%lx\n", Hob.FirmwareVolume->Length));
        break;
      case EFI_HOB_TYPE_CPU:
        DEBUG ((DEBUG_INFO, "whoes type is EFI_HOB_TYPE_CPU\n"));
        DEBUG ((DEBUG_INFO, "   HOB Offset = 0x%x\n", Hob));
        DEBUG ((DEBUG_INFO, "   HobLength = 0x%x\n", Hob.Cpu->Header.HobLength));
        DEBUG ((DEBUG_INFO, "   SizeOfMemorySpace = 0x%lx\n", Hob.Cpu->SizeOfMemorySpace));
        DEBUG ((DEBUG_INFO, "   SizeOfIoSpace =0x%lx\n", Hob.Cpu->SizeOfIoSpace));
        break;
      case EFI_HOB_TYPE_MEMORY_POOL:
        DEBUG ((DEBUG_INFO, "whoes type is EFI_HOB_TYPE_MEMORY_POOL\n"));
        DEBUG ((DEBUG_INFO, "   HOB Offset = 0x%x\n", Hob));
        DEBUG ((DEBUG_INFO, "   HobLength = 0x%x\n", Hob.Pool->Header.HobLength));
        break;
      case EFI_HOB_TYPE_FV2:
        DEBUG ((DEBUG_INFO, "whoes type is EFI_HOB_TYPE_FV2\n"));
        DEBUG ((DEBUG_INFO, "   HOB Offset = 0x%x\n", Hob));
        DEBUG ((DEBUG_INFO, "   HobLength = 0x%x\n", Hob.FirmwareVolume2->Header.HobLength));
        DEBUG ((DEBUG_INFO, "   BaseAddress = 0x%lx\n", Hob.FirmwareVolume2->BaseAddress));
        DEBUG ((DEBUG_INFO, "   Length = 0x%lx\n", Hob.FirmwareVolume2->Length));
        DEBUG ((DEBUG_INFO, "   FvName = %g\n", &Hob.FirmwareVolume2->FvName));
        DEBUG ((DEBUG_INFO, "   FileName = %g\n", &Hob.FirmwareVolume2->FileName));
        break;
      case EFI_HOB_TYPE_UEFI_CAPSULE:
        DEBUG ((DEBUG_INFO, "whoes type is EFI_HOB_TYPE_UEFI_CAPSULE\n"));
        DEBUG ((DEBUG_INFO, "   HOB Offset = 0x%x\n", Hob));
        DEBUG ((DEBUG_INFO, "   HobLength = 0x%x\n", Hob.Capsule->Header.HobLength));
        DEBUG ((DEBUG_INFO, "   BaseAddress = 0x%lx\n", Hob.Capsule->BaseAddress));
        DEBUG ((DEBUG_INFO, "   Length = 0x%lx\n", Hob.Capsule->Length));
        break;
      case EFI_HOB_TYPE_FV3:
        DEBUG ((DEBUG_INFO, "whoes type is EFI_HOB_TYPE_FV3\n"));
        DEBUG ((DEBUG_INFO, "   HOB Offset = 0x%x\n", Hob));
        DEBUG ((DEBUG_INFO, "   HobLength = 0x%x\n", Hob.FirmwareVolume3->Header.HobLength));
        DEBUG ((DEBUG_INFO, "   BaseAddress = 0x%lx\n", Hob.FirmwareVolume3->BaseAddress));
        DEBUG ((DEBUG_INFO, "   Length = 0x%lx\n", Hob.FirmwareVolume3->Length));
        DEBUG ((DEBUG_INFO, "   AuthenticationStatus = 0x%x\n", Hob.FirmwareVolume3->AuthenticationStatus));
        DEBUG ((DEBUG_INFO, "   ExtractedFv = %g\n", &Hob.FirmwareVolume3->ExtractedFv));
        DEBUG ((DEBUG_INFO, "   FileName = %g\n", &Hob.FirmwareVolume3->FileName));  
        break;
      default:
        DEBUG ((DEBUG_INFO, "\n"));
        break;
    }

*/
    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  DEBUG ((DEBUG_INFO, "There are totally %d Hobs, the End Hob address is %p\n", Count, Hob.Raw));

}

