/** @file
  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"
#include <UniversalPayload/AcpiTable.h>
#include <UniversalPayload/SerialPortInfo.h>

STATIC CONST CHAR8 * Hob_Type_List[] = {
  NULL,                                //0x0000
  "EFI_HOB_TYPE_HANDOFF",              //0x0001
  "EFI_HOB_TYPE_MEMORY_ALLOCATION  ",  //0x0002
  "EFI_HOB_TYPE_RESOURCE_DESCRIPTOR",  //0x0003
  "EFI_HOB_TYPE_GUID_EXTENSION",       //0x0004
  "EFI_HOB_TYPE_FV",                   //0x0005
  "EFI_HOB_TYPE_CPU",                  //0x0006
  "EFI_HOB_TYPE_MEMORY_POOL",          //0x0007
  NULL,                                //0x0008
  "EFI_HOB_TYPE_FV2",                  //0x0009
  "EFI_HOB_TYPE_LOAD_PEIM_UNUSED",     //0x000A
  "EFI_HOB_TYPE_UEFI_CAPSULE",         //0x000B
  "EFI_HOB_TYPE_FV3"                   //0x000C
  };

typedef
EFI_STATUS
(EFIAPI *HOB_PRINT_HANDLE)(
  IN  VOID          *HobStart
  );

char * Memorey_Type_List[17] = {
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

char * Resource_Type_List[] = {
  "EFI_RESOURCE_SYSTEM_MEMORY          ", //0x00000000
  "EFI_RESOURCE_MEMORY_MAPPED_IO       ", //0x00000001
  "EFI_RESOURCE_IO                     ", //0x00000002
  "EFI_RESOURCE_FIRMWARE_DEVICE        ", //0x00000003
  "EFI_RESOURCE_MEMORY_MAPPED_IO_PORT  ", //0x00000004
  "EFI_RESOURCE_MEMORY_RESERVED        ", //0x00000005
  "EFI_RESOURCE_IO_RESERVED            ", //0x00000006
  "EFI_RESOURCE_MAX_MEMORY_TYPE        "  //0x00000007
  };

EFI_STATUS
EFIAPI
PrintHex(
  IN  UINT8         *DataStart,
  IN  UINTN         DataSize
  )
{
  UINTN  Index1;
  UINTN  Index2;
  for (Index1 = 0; Index1 * 16 < DataSize; Index1++){
    DEBUG ((DEBUG_INFO, "   0x%p:", DataStart));
    for (Index2 = 0; (Index2 < 16) && (Index1 * 16 + Index2 < DataSize); Index2++){
      DEBUG ((DEBUG_INFO, " %02x", *DataStart));
      DataStart++ ;
    }
    DEBUG ((DEBUG_INFO, "\n"));
  }
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
PrintHandOffHob(
  IN  VOID          *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  DEBUG ((DEBUG_INFO, "   BootMode = 0x%x\n", Hob.HandoffInformationTable->BootMode));
  DEBUG ((DEBUG_INFO, "   EfiMemoryTop = 0x%lx\n", Hob.HandoffInformationTable->EfiMemoryTop));
  DEBUG ((DEBUG_INFO, "   EfiMemoryBottom = 0x%lx\n", Hob.HandoffInformationTable->EfiMemoryBottom));
  DEBUG ((DEBUG_INFO, "   EfiFreeMemoryTop = 0x%lx\n", Hob.HandoffInformationTable->EfiFreeMemoryTop));
  DEBUG ((DEBUG_INFO, "   EfiFreeMemoryBottom = 0x%lx\n", Hob.HandoffInformationTable->EfiFreeMemoryBottom));
  DEBUG ((DEBUG_INFO, "   EfiEndOfHobList = 0x%lx\n", Hob.HandoffInformationTable->EfiEndOfHobList));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PrintMemoryAllocationHob(
  IN  VOID          *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  if(CompareGuid (&(Hob.MemoryAllocation->AllocDescriptor.Name), &gEfiHobMemoryAllocStackGuid)) {
    DEBUG ((DEBUG_INFO, "   Type = EFI_HOB_MEMORY_ALLOCATION_STACK\n"));
    DEBUG ((DEBUG_INFO, "   MemoryBaseAddress = 0x%lx\n", Hob.MemoryAllocationStack->AllocDescriptor.MemoryBaseAddress));
    DEBUG ((DEBUG_INFO, "   MemoryLength = 0x%lx\n", Hob.MemoryAllocationStack->AllocDescriptor.MemoryLength));
    DEBUG ((DEBUG_INFO, "   MemoryType = %a \n", Memorey_Type_List[Hob.MemoryAllocationStack->AllocDescriptor.MemoryType]));
  } else if(CompareGuid (&(Hob.MemoryAllocation->AllocDescriptor.Name), &gEfiHobMemoryAllocBspStoreGuid)) {
    DEBUG ((DEBUG_INFO, "   Type = EFI_HOB_MEMORY_ALLOCATION_BSP_STORE\n"));
    DEBUG ((DEBUG_INFO, "   MemoryBaseAddress = 0x%lx\n", Hob.MemoryAllocationBspStore->AllocDescriptor.MemoryBaseAddress));
    DEBUG ((DEBUG_INFO, "   MemoryLength = 0x%lx\n", Hob.MemoryAllocationBspStore->AllocDescriptor.MemoryLength));
    DEBUG ((DEBUG_INFO, "   MemoryType = %a \n", Memorey_Type_List[Hob.MemoryAllocationBspStore->AllocDescriptor.MemoryType]));
  }else if(CompareGuid (&(Hob.MemoryAllocation->AllocDescriptor.Name), &gEfiHobMemoryAllocModuleGuid)) {
    DEBUG ((DEBUG_INFO, "   Type = EFI_HOB_MEMORY_ALLOCATION_MODULE\n"));
    DEBUG ((DEBUG_INFO, "   MemoryBaseAddress = 0x%lx\n", Hob.MemoryAllocationModule->MemoryAllocationHeader.MemoryBaseAddress));
    DEBUG ((DEBUG_INFO, "   MemoryLength = 0x%lx\n", Hob.MemoryAllocationModule->MemoryAllocationHeader.MemoryLength));
    DEBUG ((DEBUG_INFO, "   MemoryType = %a \n", Memorey_Type_List[Hob.MemoryAllocationModule->MemoryAllocationHeader.MemoryType]));
    DEBUG ((DEBUG_INFO, "   Module Name = %g\n", Hob.MemoryAllocationModule->ModuleName));
    DEBUG ((DEBUG_INFO, "   Physical Address = 0x%x\n", Hob.MemoryAllocationModule->EntryPoint));
  }else {
    DEBUG ((DEBUG_INFO, "   Type = EFI_HOB_TYPE_MEMORY_ALLOCATION\n"));
    DEBUG ((DEBUG_INFO, "   MemoryBaseAddress = 0x%lx\n", Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress));
    DEBUG ((DEBUG_INFO, "   MemoryLength = 0x%lx\n", Hob.MemoryAllocation->AllocDescriptor.MemoryLength));
    DEBUG ((DEBUG_INFO, "   MemoryType = %a \n", Memorey_Type_List[Hob.MemoryAllocation->AllocDescriptor.MemoryType]));
  }
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
PrintResourceDiscriptorHob(
  IN  VOID          *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  DEBUG ((DEBUG_INFO, "   ResourceType = %a\n", Resource_Type_List[Hob.ResourceDescriptor->ResourceType]));
  DEBUG ((DEBUG_INFO, "   Owner = %g\n", Hob.ResourceDescriptor->Owner));
  DEBUG ((DEBUG_INFO, "   ResourceAttribute = 0x%x\n", Hob.ResourceDescriptor->ResourceAttribute));
  DEBUG ((DEBUG_INFO, "   PhysicalStart = 0x%x\n", Hob.ResourceDescriptor->PhysicalStart));
  DEBUG ((DEBUG_INFO, "   ResourceLength = 0x%x\n", Hob.ResourceDescriptor->ResourceLength));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PrintGuidHob(
  IN  VOID          *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;

  DEBUG ((DEBUG_INFO, "   Name = %g\n", &Hob.Guid->Name));
  if(CompareGuid (&(Hob.Guid->Name), &gPldAcpiTableGuid)) {
    PLD_ACPI_TABLE         *AcpiTableHob;
    AcpiTableHob = (PLD_ACPI_TABLE *)GET_GUID_HOB_DATA (Hob.Raw);
    DEBUG ((DEBUG_INFO, "   This is a ACPI table Guid Hob\n"));
    DEBUG ((DEBUG_INFO, "   Rsdp = 0x%p\n", (VOID *) (UINTN) AcpiTableHob->Rsdp));
  } else if(CompareGuid (&(Hob.Guid->Name), &gPldSerialPortInfoGuid)) {
    PLD_SERIAL_PORT_INFO         *SerialPortInfo;
    SerialPortInfo = (PLD_SERIAL_PORT_INFO *)GET_GUID_HOB_DATA (Hob.Raw);
    DEBUG ((DEBUG_INFO, "   This is a Serial Port Info Guid Hob\n"));
    DEBUG ((DEBUG_INFO, "   UseMmio = 0x%x\n", SerialPortInfo->UseMmio));
    DEBUG ((DEBUG_INFO, "   RegisterWidth = 0x%x\n", SerialPortInfo->RegisterWidth));
    DEBUG ((DEBUG_INFO, "   BaudRate = 0x%x\n", SerialPortInfo->BaudRate));
    DEBUG ((DEBUG_INFO, "   RegisterBase = 0x%x\n", SerialPortInfo->RegisterBase));
  } else if(CompareGuid (&(Hob.Guid->Name), &gPldSmbios3TableGuid)) {
    PLD_SMBIOS_TABLE_HOB         *SMBiosTable;
    SMBiosTable = (PLD_SMBIOS_TABLE_HOB *)GET_GUID_HOB_DATA (Hob.Raw);
    DEBUG ((DEBUG_INFO, "   This is a SmBios Guid Hob, with Guid gPldSmbios3TableGuid\n"));
    DEBUG ((DEBUG_INFO, "   SmBiosEntryPoint = 0x%x\n", (VOID *) (UINTN) SMBiosTable->SmBiosEntryPoint));
  } else if(CompareGuid (&(Hob.Guid->Name), &gPldSmbiosTableGuid)) {
    PLD_SMBIOS_TABLE_HOB         *SMBiosTable;
    SMBiosTable = (PLD_SMBIOS_TABLE_HOB *)GET_GUID_HOB_DATA (Hob.Raw);
    DEBUG ((DEBUG_INFO, "   This is a SmBios Guid Hob, with Guid gPldSmbiosTableGuid\n"));
    DEBUG ((DEBUG_INFO, "   SmBiosEntryPoint = 0x%x\n", (VOID *) (UINTN) SMBiosTable->SmBiosEntryPoint));
  } else {
    PrintHex(GET_GUID_HOB_DATA (Hob.Raw), GET_GUID_HOB_DATA_SIZE (Hob.Raw));
  }
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PrintFVHob(
  IN  VOID          *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  DEBUG ((DEBUG_INFO, "   BaseAddress = 0x%lx\n", Hob.FirmwareVolume->BaseAddress));
  DEBUG ((DEBUG_INFO, "   Length = 0x%lx\n", Hob.FirmwareVolume->Length));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PrintCpuHob(
  IN  VOID          *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  DEBUG ((DEBUG_INFO, "   SizeOfMemorySpace = 0x%lx\n", Hob.Cpu->SizeOfMemorySpace));
  DEBUG ((DEBUG_INFO, "   SizeOfIoSpace =0x%lx\n", Hob.Cpu->SizeOfIoSpace));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PrintMemoryPoolHob(
  IN  VOID          *HobStart
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PrintFV2Hob(
  IN  VOID          *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  DEBUG ((DEBUG_INFO, "   BaseAddress = 0x%lx\n", Hob.FirmwareVolume2->BaseAddress));
  DEBUG ((DEBUG_INFO, "   Length = 0x%lx\n", Hob.FirmwareVolume2->Length));
  DEBUG ((DEBUG_INFO, "   FvName = %g\n", &Hob.FirmwareVolume2->FvName));
  DEBUG ((DEBUG_INFO, "   FileName = %g\n", &Hob.FirmwareVolume2->FileName));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PrintCapsuleHob(
  IN  VOID          *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  DEBUG ((DEBUG_INFO, "   BaseAddress = 0x%lx\n", Hob.Capsule->BaseAddress));
  DEBUG ((DEBUG_INFO, "   Length = 0x%lx\n", Hob.Capsule->Length));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PrintFV3Hob(
  IN  VOID          *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  DEBUG ((DEBUG_INFO, "   BaseAddress = 0x%lx\n", Hob.FirmwareVolume3->BaseAddress));
  DEBUG ((DEBUG_INFO, "   Length = 0x%lx\n", Hob.FirmwareVolume3->Length));
  DEBUG ((DEBUG_INFO, "   AuthenticationStatus = 0x%x\n", Hob.FirmwareVolume3->AuthenticationStatus));
  DEBUG ((DEBUG_INFO, "   ExtractedFv = %g\n", &Hob.FirmwareVolume3->ExtractedFv));
  DEBUG ((DEBUG_INFO, "   FileName = %g\n", &Hob.FirmwareVolume3->FileName)); 
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
PrintNotCorrectHob(
  IN  VOID          *HobStart
  )
{
  ASSERT(FALSE);
  return EFI_SUCCESS;
}

HOB_PRINT_HANDLE HobHandles[] = {
  PrintNotCorrectHob,                  //0x0000
  PrintHandOffHob,                     //0x0001
  PrintMemoryAllocationHob,            //0x0002
  PrintResourceDiscriptorHob,          //0x0003
  PrintGuidHob,                        //0x0004
  PrintFVHob,                          //0x0005
  PrintCpuHob,                         //0x0006
  PrintMemoryPoolHob,                  //0x0007
  PrintNotCorrectHob,                  //0x0008
  PrintFV2Hob,                         //0x0009
  PrintNotCorrectHob,                  //0x000A
  PrintCapsuleHob,                     //0x000B
  PrintFV3Hob                          //0x000C
  };

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
  //HOB_PRINT_HANDLE      PrintHobHandle;
  UINTN                 NumberOfPrintHandles;

  ASSERT (HobStart != NULL);
   
  Hob.Raw = (UINT8 *) HobStart;
  DEBUG ((DEBUG_INFO, "Print all Hob information from Hob 0x%p\n", Hob.Raw));
  DEBUG ((DEBUG_INFO, "Totally number of HobHandlers = %d\n", sizeof(HobHandles)));
  NumberOfPrintHandles = sizeof(HobHandles);
  
  Count = 0;

  //
  // Parse the HOB list to see which type it is, and print the information.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    Count++;
    DEBUG ((DEBUG_INFO, "HOB[%d]: Type = %a, Offset = 0x%p, Length = 0x%x\n", Count, Hob_Type_List[Hob.Header->HobType], Hob.Raw, Hob.Header->HobLength));
    if (Hob.Header->HobType < NumberOfPrintHandles) {
      HobHandles[Hob.Header->HobType](Hob.Raw);
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  DEBUG ((DEBUG_INFO, "There are totally %d Hobs, the End Hob address is %p\n", Count, Hob.Raw));

}

