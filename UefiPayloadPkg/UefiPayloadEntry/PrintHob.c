/** @file
  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"
#include <UniversalPayload/AcpiTable.h>
#include <UniversalPayload/SerialPortInfo.h>
#include <UniversalPayload/PciRootBridges.h>
#include <UniversalPayload/ExtraData.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/AcpiBoardInfoGuid.h>

STATIC CONST CHAR8 * mHobTypeStr[] = {
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
(*HOB_PRINT_HANDLE)(
  IN  VOID          *HobStart,
  IN  UINTN          Hobsize
  );

typedef struct{
  UINT8               Type;
  HOB_PRINT_HANDLE    Function;

}HOB_PRINT_HANDLE_TABLE;

char * mMemoryTypeStr[] = {
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
  
typedef
EFI_STATUS
(*GUID_HOB_PRINT)(
  IN  UINT8          *HobRaw
  );

typedef
EFI_STATUS
(*MEMORY_ALLOCCATION_HOB)(
  IN  UINT8          *HobRaw
  );

typedef struct {
  EFI_GUID       *Guid;
  GUID_HOB_PRINT Function;
  char *         Guidname;
}GUID_HOB_PRINT_HANDLE;

typedef struct{
 EFI_GUID *Guid;
 char *Type;
 MEMORY_ALLOCCATION_HOB Function;
}PRINT_MEMORY_ALLOCCATION_HOB;

EFI_STATUS
PrintHex(
  IN  UINT8         *DataStart,
  IN  UINTN         DataSize
  )
{
  UINTN  Index1;
  UINTN  Index2;
  UINT8  *StartAddr = DataStart;
  for (Index1 = 0; Index1 * 16 < DataSize; Index1++){
    DEBUG ((DEBUG_INFO, "   0x%p:",(DataStart - StartAddr)));
    for (Index2 = 0; (Index2 < 16) && (Index1 * 16 + Index2 < DataSize); Index2++){
      DEBUG ((DEBUG_INFO, " %02x", *DataStart));
      DataStart++ ;
    }
    DEBUG ((DEBUG_INFO, "\n"));
  }
  return EFI_SUCCESS;
}


EFI_STATUS
PrintHandOffHob(
  IN  VOID          *HobStart,
  IN  UINTN          Hobsize
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  if(Hobsize < sizeof(Hob.HandoffInformationTable)){
    DEBUG ((DEBUG_INFO, "   Some Information Missing.. Current HandoffInformationTable Hobsize:0x%x is less than \
    Original HandoffInformationTable size:0x%x \n", Hobsize,sizeof(Hob.HandoffInformationTable)));
  }
  DEBUG ((DEBUG_INFO, "   BootMode            = 0x%x\n", Hob.HandoffInformationTable->BootMode));
  DEBUG ((DEBUG_INFO, "   EfiMemoryTop        = 0x%lx\n", Hob.HandoffInformationTable->EfiMemoryTop));
  DEBUG ((DEBUG_INFO, "   EfiMemoryBottom     = 0x%lx\n", Hob.HandoffInformationTable->EfiMemoryBottom));
  DEBUG ((DEBUG_INFO, "   EfiFreeMemoryTop    = 0x%lx\n", Hob.HandoffInformationTable->EfiFreeMemoryTop));
  DEBUG ((DEBUG_INFO, "   EfiFreeMemoryBottom = 0x%lx\n", Hob.HandoffInformationTable->EfiFreeMemoryBottom));
  DEBUG ((DEBUG_INFO, "   EfiEndOfHobList     = 0x%lx\n", Hob.HandoffInformationTable->EfiEndOfHobList));
  return EFI_SUCCESS;
}

EFI_STATUS
PrintMemAllocCommonHob(
  IN  UINT8          *HobRaw
  )
  {
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobRaw;
  DEBUG ((DEBUG_INFO, "   MemoryBaseAddress = 0x%lx\n", Hob.MemoryAllocationStack->AllocDescriptor.MemoryBaseAddress));
  DEBUG ((DEBUG_INFO, "   MemoryLength      = 0x%lx\n", Hob.MemoryAllocationStack->AllocDescriptor.MemoryLength));
  DEBUG ((DEBUG_INFO, "   MemoryType        = %a \n", mMemoryTypeStr[Hob.MemoryAllocationStack->AllocDescriptor.MemoryType]));
  return EFI_SUCCESS;
  }

EFI_STATUS
PrintMemAllocModuleHob(
  IN  UINT8          *HobRaw
  )
  {
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobRaw;
  PrintMemAllocCommonHob(HobRaw);
  DEBUG ((DEBUG_INFO, "   Module Name = %g\n", Hob.MemoryAllocationModule->ModuleName));
  DEBUG ((DEBUG_INFO, "   Physical Address = 0x%lx\n", Hob.MemoryAllocationModule->EntryPoint));
  return EFI_SUCCESS;
  }

PRINT_MEMORY_ALLOCCATION_HOB PrintMemoryAllocHobTable[] = {
  {&gEfiHobMemoryAllocStackGuid, "EFI_HOB_MEMORY_ALLOCATION_STACK",PrintMemAllocCommonHob},
  {&gEfiHobMemoryAllocBspStoreGuid,"EFI_HOB_MEMORY_ALLOCATION_BSP_STORE",PrintMemAllocCommonHob },
  {&gEfiHobMemoryAllocModuleGuid, "EFI_HOB_MEMORY_ALLOCATION_MODULE",PrintMemAllocModuleHob}
};
EFI_STATUS
PrintMemoryAllocationHob(
  IN  VOID          *HobStart,
  IN  UINTN          Hobsize
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_STATUS Status;
  Hob.Raw = (UINT8 *) HobStart;
  INT8 Index = ARRAY_SIZE(PrintMemoryAllocHobTable) - 1;
  if(Hobsize < sizeof(Hob.MemoryAllocationStack))
  {
     DEBUG ((DEBUG_INFO, "   Some Information Missing.. Current MemoryAllocation Hobsize:0x%x is less than \
    Original MemoryAllocationStack size:0x%x \n", Hobsize,sizeof(Hob.MemoryAllocationStack)));
  }
  while(Index>=0){
    if(CompareGuid (&(Hob.MemoryAllocation->AllocDescriptor.Name), PrintMemoryAllocHobTable[Index].Guid)) 
    {
      DEBUG ((DEBUG_INFO, "   Type = %a\n",PrintMemoryAllocHobTable[Index].Type));
      Status = PrintMemoryAllocHobTable[Index].Function(HobStart);
      break;
    }
    Index-=1;
  }
  if(Index < 0)
  {
    DEBUG ((DEBUG_INFO, "   Type = EFI_HOB_TYPE_MEMORY_ALLOCATION\n"));
    Status = PrintMemAllocCommonHob(HobStart);
  }
  return EFI_SUCCESS;
}


EFI_STATUS
PrintResourceDiscriptorHob(
  IN  VOID          *HobStart,
  IN  UINTN          Hobsize
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  if(Hobsize < sizeof(Hob.ResourceDescriptor)){
        DEBUG ((DEBUG_INFO, "   Some Information Missing.. Current HandoffInformationTable Hobsize:0x%x is less than \
                                Original ResourceDescriptor size:0x%x \n", Hobsize,sizeof(Hob.ResourceDescriptor)));
  }
  DEBUG ((DEBUG_INFO, "   ResourceType      = %a\n", Resource_Type_List[Hob.ResourceDescriptor->ResourceType]));
  if(!IsZeroGuid(&Hob.ResourceDescriptor->Owner))
  DEBUG ((DEBUG_INFO, "   Owner             = %g\n", Hob.ResourceDescriptor->Owner));
  DEBUG ((DEBUG_INFO, "   ResourceAttribute = 0x%x\n", Hob.ResourceDescriptor->ResourceAttribute));
  DEBUG ((DEBUG_INFO, "   PhysicalStart     = 0x%lx\n", Hob.ResourceDescriptor->PhysicalStart));
  DEBUG ((DEBUG_INFO, "   ResourceLength    = 0x%lx\n", Hob.ResourceDescriptor->ResourceLength));
  return EFI_SUCCESS;
}

EFI_STATUS
PrintAcpiGuidHob(
  IN  UINT8          *HobRaw
  )
{
  PLD_ACPI_TABLE         *AcpiTableHob;
  AcpiTableHob = (PLD_ACPI_TABLE *)GET_GUID_HOB_DATA (HobRaw);
  DEBUG ((DEBUG_INFO, "   This is a ACPI table Guid Hob\n"));
  DEBUG ((DEBUG_INFO, "   Rsdp = 0x%p\n", (VOID *) (UINTN) AcpiTableHob->Rsdp));
  return EFI_SUCCESS;
}

EFI_STATUS
PrintSerialGuidHob(
  IN  UINT8          *HobRaw
  )
{
  PLD_SERIAL_PORT_INFO         *SerialPortInfo;
  SerialPortInfo = (PLD_SERIAL_PORT_INFO *)GET_GUID_HOB_DATA (HobRaw);
  DEBUG ((DEBUG_INFO, "   This is a Serial Port Info Guid Hob\n"));
  DEBUG ((DEBUG_INFO, "   UseMmio       = 0x%x\n", SerialPortInfo->UseMmio));
  DEBUG ((DEBUG_INFO, "   RegisterWidth = 0x%x\n", SerialPortInfo->RegisterWidth));
  DEBUG ((DEBUG_INFO, "   BaudRate      = %d\n", SerialPortInfo->BaudRate));
  DEBUG ((DEBUG_INFO, "   RegisterBase  = 0x%lx\n", SerialPortInfo->RegisterBase));
  return EFI_SUCCESS;
}

EFI_STATUS
PrintSmbios3GuidHob(
  IN  UINT8          *HobRaw
  )
{
  PLD_SMBIOS_TABLE_HOB         *SMBiosTable;
  SMBiosTable = (PLD_SMBIOS_TABLE_HOB *)GET_GUID_HOB_DATA (HobRaw);
  DEBUG ((DEBUG_INFO, "   This is a SmBios Guid Hob, with Guid gPldSmbios3TableGuid\n"));
  DEBUG ((DEBUG_INFO, "   SmBiosEntryPoint = 0x%x\n", (VOID *) (UINTN) SMBiosTable->SmBiosEntryPoint));
  return EFI_SUCCESS;
}

EFI_STATUS
PrintSmbiosTablGuidHob(
  IN  UINT8          *HobRaw
  )
{
  PLD_SMBIOS_TABLE_HOB         *SMBiosTable;
  SMBiosTable = (PLD_SMBIOS_TABLE_HOB *)GET_GUID_HOB_DATA (HobRaw);
  DEBUG ((DEBUG_INFO, "   This is a SmBios Guid Hob, with Guid gPldSmbiosTableGuid\n"));
  DEBUG ((DEBUG_INFO, "   SmBiosEntryPoint = 0x%x\n", (VOID *) (UINTN) SMBiosTable->SmBiosEntryPoint)); 
  return EFI_SUCCESS;  
}

EFI_STATUS
PrintAcpiBoardInfoGuidHob(
  IN  UINT8          *HobRaw
  )
{
  ACPI_BOARD_INFO *AcpBoardInfo;
  AcpBoardInfo = (ACPI_BOARD_INFO *)GET_GUID_HOB_DATA (HobRaw);
  DEBUG ((DEBUG_INFO, "   This is a Acpi Guid Hob, with Guid gUefiAcpiBoardInfoGuid\n"));
  DEBUG ((DEBUG_INFO, "   Revision        = 0x%x\n",AcpBoardInfo->Revision));
  DEBUG ((DEBUG_INFO, "   Reserved0       = 0x%x\n",AcpBoardInfo->Reserved0));
  DEBUG ((DEBUG_INFO, "   ResetValue      = 0x%x\n",AcpBoardInfo->ResetValue));
  DEBUG ((DEBUG_INFO, "   PmEvtBase       = 0x%lx\n",AcpBoardInfo->PmEvtBase));
  DEBUG ((DEBUG_INFO, "   PmGpeEnBase     = 0x%lx\n",AcpBoardInfo->PmGpeEnBase));
  DEBUG ((DEBUG_INFO, "   PmCtrlRegBase   = 0x%lx\n",AcpBoardInfo->PmCtrlRegBase));
  DEBUG ((DEBUG_INFO, "   PmTimerRegBase  = 0x%lx\n",AcpBoardInfo->PmTimerRegBase));
  DEBUG ((DEBUG_INFO, "   ResetRegAddress = 0x%lx\n",AcpBoardInfo->ResetRegAddress));
  DEBUG ((DEBUG_INFO, "   PcieBaseAddress = 0x%lx\n",AcpBoardInfo->PcieBaseAddress));
  DEBUG ((DEBUG_INFO, "   PcieBaseSize    = 0x%lx\n",AcpBoardInfo->PcieBaseSize));
  return EFI_SUCCESS;  

}

EFI_STATUS
PrintPciRootBridgeInfoGuidHob(
  IN  UINT8          *HobRaw
  )
{ 
  PLD_PCI_ROOT_BRIDGES *PciRootBridges;
  UINTN Index=0;
  PciRootBridges = (PLD_PCI_ROOT_BRIDGES *)GET_GUID_HOB_DATA (HobRaw);
  DEBUG ((DEBUG_INFO, "   This is a Pci Guid Hob, with Guid gPldPciRootBridgeInfoGuid\n"));
  DEBUG ((DEBUG_INFO, "   Revision = 0x%x\n",PciRootBridges->PldHeader.Revision));
  DEBUG ((DEBUG_INFO, "   Count = 0x%x\n",PciRootBridges->Count));

  while(Index<PciRootBridges->Count)
  {
    DEBUG ((DEBUG_INFO, "   Segment                 = 0x%x\n",PciRootBridges->RootBridge[Index].Segment));
    DEBUG ((DEBUG_INFO, "   Supports                = 0x%lx\n",PciRootBridges->RootBridge[Index].Supports));
    DEBUG ((DEBUG_INFO, "   Attributes              = 0x%lx\n",PciRootBridges->RootBridge[Index].Attributes));
    DEBUG ((DEBUG_INFO, "   DmaAbove4G              = 0x%x\n",PciRootBridges->RootBridge[Index].DmaAbove4G));
    DEBUG ((DEBUG_INFO, "   NoExtendedConfigSpace   = 0x%x\n",PciRootBridges->RootBridge[Index].NoExtendedConfigSpace));
    DEBUG ((DEBUG_INFO, "   ResourceAssigned        = 0x%x\n",PciRootBridges->RootBridge[Index].ResourceAssigned));
    DEBUG ((DEBUG_INFO, "   AllocationAttributes    = 0x%lx\n",PciRootBridges->RootBridge[Index].AllocationAttributes));
    DEBUG ((DEBUG_INFO, "   Bus.Base                = 0x%lx\n",PciRootBridges->RootBridge[Index].Bus.Base));
    DEBUG ((DEBUG_INFO, "   Bus.Limit               = 0x%lx\n",PciRootBridges->RootBridge[Index].Bus.Limit));
    DEBUG ((DEBUG_INFO, "   Bus.Translation         = 0x%lx\n",PciRootBridges->RootBridge[Index].Bus.Translation));
    DEBUG ((DEBUG_INFO, "   Io.Base                 = 0x%lx\n",PciRootBridges->RootBridge[Index].Io.Base));
    DEBUG ((DEBUG_INFO, "   Io.Limit                = 0x%lx\n",PciRootBridges->RootBridge[Index].Io.Limit));
    DEBUG ((DEBUG_INFO, "   Io.Translation          = 0x%lx\n",PciRootBridges->RootBridge[Index].Io.Translation));
    DEBUG ((DEBUG_INFO, "   Mem.Base                = 0x%lx\n",PciRootBridges->RootBridge[Index].Mem.Base));
    DEBUG ((DEBUG_INFO, "   Mem.Limit               = 0x%lx\n",PciRootBridges->RootBridge[Index].Mem.Limit));
    DEBUG ((DEBUG_INFO, "   Mem.Translation         = 0x%lx\n",PciRootBridges->RootBridge[Index].Mem.Translation));
    DEBUG ((DEBUG_INFO, "   MemAbove4G.Base         = 0x%lx\n",PciRootBridges->RootBridge[Index].MemAbove4G.Base));
    DEBUG ((DEBUG_INFO, "   MemAbove4G.Limit        = 0x%lx\n",PciRootBridges->RootBridge[Index].MemAbove4G.Limit));
    DEBUG ((DEBUG_INFO, "   MemAbove4G.Translation  = 0x%lx\n",PciRootBridges->RootBridge[Index].MemAbove4G.Translation));
    DEBUG ((DEBUG_INFO, "   PMem.Base               = 0x%lx\n",PciRootBridges->RootBridge[Index].PMem.Base));
    DEBUG ((DEBUG_INFO, "   PMem.Limit              = 0x%lx\n",PciRootBridges->RootBridge[Index].PMem.Limit));
    DEBUG ((DEBUG_INFO, "   PMem.Translation        = 0x%lx\n",PciRootBridges->RootBridge[Index].PMem.Translation));
    DEBUG ((DEBUG_INFO, "   PMemAbove4G.Base        = 0x%lx\n",PciRootBridges->RootBridge[Index].PMemAbove4G.Base));
    DEBUG ((DEBUG_INFO, "   PMemAbove4G.Limit       = 0x%lx\n",PciRootBridges->RootBridge[Index].PMemAbove4G.Limit));
    DEBUG ((DEBUG_INFO, "   PMemAbove4G.Translation = 0x%lx\n",PciRootBridges->RootBridge[Index].PMemAbove4G.Translation));
    Index+=1;
  }
  return EFI_SUCCESS;  
}

EFI_STATUS
PrintSpiFlashInfoGuidHob(
  IN  UINT8          *HobRaw
  )
{ 
  SPI_FLASH_INFO *SpiFlashInfo;
  SpiFlashInfo = (SPI_FLASH_INFO *)GET_GUID_HOB_DATA (HobRaw);
  DEBUG ((DEBUG_INFO, "   This is a Spi Flash Guid Hob, with Guid gSpiFlashInfoGuid\n"));
  DEBUG ((DEBUG_INFO, "   Revision          = 0x%x\n",SpiFlashInfo->Revision));
  DEBUG ((DEBUG_INFO, "   Reserved          = 0x%x\n",SpiFlashInfo->Reserved));
  DEBUG ((DEBUG_INFO, "   AddressSpaceId    = 0x%x\n",SpiFlashInfo->SpiAddress.AddressSpaceId));
  DEBUG ((DEBUG_INFO, "   RegisterBitWidth  = 0x%x\n",SpiFlashInfo->SpiAddress.RegisterBitWidth));
  DEBUG ((DEBUG_INFO, "   RegisterBitOffset = 0x%x\n",SpiFlashInfo->SpiAddress.RegisterBitOffset));
  DEBUG ((DEBUG_INFO, "   AccessSize        = 0x%x\n",SpiFlashInfo->SpiAddress.AccessSize));
  DEBUG ((DEBUG_INFO, "   Address           = 0x%lx\n",SpiFlashInfo->SpiAddress.Address));
  return EFI_SUCCESS; 
}

EFI_STATUS
PrintNvVariableInfoGuidHob(
  IN  UINT8          *HobRaw
  )
{ 
  NV_VARIABLE_INFO *NvVariable;
  NvVariable = (NV_VARIABLE_INFO *)GET_GUID_HOB_DATA (HobRaw);
  DEBUG ((DEBUG_INFO, "   This is a NV Variable Guid Hob, with Guid gNvVariableInfoGuid\n"));
  DEBUG ((DEBUG_INFO, "   Revision          = 0x%x\n",NvVariable->Revision));
  DEBUG ((DEBUG_INFO, "   VariableStoreBase = 0x%x\n",NvVariable->VariableStoreBase));
  DEBUG ((DEBUG_INFO, "   VariableStoreSize = 0x%x\n",NvVariable->VariableStoreSize));
  return EFI_SUCCESS; 
}

EFI_STATUS
PrintPldExtraDataGuidHob(
  IN  UINT8          *HobRaw
  )
{ 
  PLD_EXTRA_DATA *PldExtraData;
  UINTN Index = 0;
  PldExtraData = (PLD_EXTRA_DATA *)GET_GUID_HOB_DATA (HobRaw);
   DEBUG ((DEBUG_INFO, "   This is a PayLoad Extra Data Guid Hob, with Guid gPldExtraDataGuid\n"));
   DEBUG ((DEBUG_INFO, "   Revision = 0x%x\n",PldExtraData->PldHeader.Revision));
   DEBUG ((DEBUG_INFO, "   Count    = 0x%x\n",PldExtraData->Count));
   while(Index<PldExtraData->Count)
   {
     DEBUG ((DEBUG_INFO, "   Base   = 0x%lx\n",PldExtraData->Entry[Index].Base));
     DEBUG ((DEBUG_INFO, "   Size   = 0x%lx\n",PldExtraData->Entry[Index].Size));
     Index+=1;
   }
   return EFI_SUCCESS; 
}

EFI_STATUS
PrintMemoryTypeInfoGuidHob(
  IN  UINT8          *HobRaw
  )
{ 
  EFI_MEMORY_TYPE_INFORMATION *MemoryTypeInfo;
  MemoryTypeInfo = (EFI_MEMORY_TYPE_INFORMATION *)GET_GUID_HOB_DATA (HobRaw);
  DEBUG ((DEBUG_INFO, "   Type            = 0x%x\n",MemoryTypeInfo->Type));
  DEBUG ((DEBUG_INFO, "   NumberOfPages   = 0x%x\n",MemoryTypeInfo->NumberOfPages));
  return EFI_SUCCESS; 
}

GUID_HOB_PRINT_HANDLE GuidHobPrintHandleTable[] = {
  { &gPldAcpiTableGuid,             PrintAcpiGuidHob,              "gPldAcpiTableGuid"},
  { &gPldSerialPortInfoGuid,        PrintSerialGuidHob,            "gPldSerialPortInfoGuid"},
  { &gPldSmbios3TableGuid,          PrintSmbios3GuidHob,           "gPldSmbios3TableGuid"},
  { &gPldSmbiosTableGuid,           PrintSmbiosTablGuidHob,        "gPldSmbiosTableGuid"},
  { &gUefiAcpiBoardInfoGuid,        PrintAcpiBoardInfoGuidHob,     "gUefiAcpiBoardInfoGuid"},
  { &gPldPciRootBridgeInfoGuid,     PrintPciRootBridgeInfoGuidHob, "gPldPciRootBridgeInfoGuid"},
  { &gSpiFlashInfoGuid,             PrintSpiFlashInfoGuidHob,      "gSpiFlashInfoGuid"},
  { &gNvVariableInfoGuid,           PrintNvVariableInfoGuidHob,    "gNvVariableInfoGuid"},
  { &gEfiMemoryTypeInformationGuid, PrintMemoryTypeInfoGuidHob,    "gEfiMemoryTypeInformationGuid"},
  { &gPldExtraDataGuid,             PrintPldExtraDataGuidHob,      "gPldExtraDataGuid"}
  };

EFI_STATUS
PrintGuidHob(
  IN  VOID          *HobStart,
  IN  UINTN          Hobsize
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  INTN Index = ARRAY_SIZE(GuidHobPrintHandleTable) - 1;
  EFI_STATUS Status;
  if(Hobsize < sizeof(Hob.Guid)){
       DEBUG ((DEBUG_INFO, "   Some Information Missing.. Current HandoffInformationTable Hobsize:0x%x is less than \
                               Original ResourceDescriptor size:0x%x \n", Hobsize,sizeof(Hob.ResourceDescriptor)));
  }
  

while(Index>=0)
{ 
  if(CompareGuid (&(Hob.Guid->Name),GuidHobPrintHandleTable[Index].Guid))
  {
    DEBUG ((DEBUG_INFO, "   Name = %a\n", GuidHobPrintHandleTable[Index].Guidname));
    Status = GuidHobPrintHandleTable[Index].Function(Hob.Raw);
    return EFI_SUCCESS;
  }
  Index-=1;
}
  DEBUG ((DEBUG_INFO, "   Name = %g\n", &Hob.Guid->Name));
  PrintHex(GET_GUID_HOB_DATA (Hob.Raw), GET_GUID_HOB_DATA_SIZE (Hob.Raw));
  return EFI_SUCCESS;
}

EFI_STATUS
PrintFvHob(
  IN  VOID          *HobStart,
  IN  UINTN          Hobsize
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  if(Hobsize < sizeof(Hob.FirmwareVolume)){
    DEBUG ((DEBUG_INFO, "   Some Information Missing.. Current HandoffInformationTable Hobsize:0x%x is less than \
                               Original FirmwareVolume size:0x%x \n", Hobsize,sizeof(Hob.FirmwareVolume)));
  }
  DEBUG ((DEBUG_INFO, "   BaseAddress = 0x%lx\n", Hob.FirmwareVolume->BaseAddress));
  DEBUG ((DEBUG_INFO, "   Length = 0x%lx\n", Hob.FirmwareVolume->Length));
  return EFI_SUCCESS;
}

EFI_STATUS
PrintCpuHob(
  IN  VOID          *HobStart,
  IN  UINTN          Hobsize
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  if(Hobsize < sizeof(Hob.Cpu)){
    DEBUG ((DEBUG_INFO, "   Some Information Missing.. Current HandoffInformationTable Hobsize:0x%x is less than \
                               Original Cpu size:0x%x \n", Hobsize,sizeof(Hob.Cpu)));
  }
  DEBUG ((DEBUG_INFO, "   SizeOfMemorySpace = 0x%lx\n", Hob.Cpu->SizeOfMemorySpace));
  DEBUG ((DEBUG_INFO, "   SizeOfIoSpace     = 0x%lx\n", Hob.Cpu->SizeOfIoSpace));
  return EFI_SUCCESS;
}

EFI_STATUS
PrintMemoryPoolHob(
  IN  VOID          *HobStart,
  IN  UINTN          Hobsize
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
PrintFv2Hob(
  IN  VOID          *HobStart,
  IN  UINTN          Hobsize
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  if(Hobsize < sizeof(Hob.FirmwareVolume2)){
    DEBUG ((DEBUG_INFO, "   Some Information Missing.. Current HandoffInformationTable Hobsize:0x%x is less than \
                               Original FirmwareVolume2 size:0x%x \n", Hobsize,sizeof(Hob.FirmwareVolume2)));
  }
  DEBUG ((DEBUG_INFO, "   BaseAddress = 0x%lx\n", Hob.FirmwareVolume2->BaseAddress));
  DEBUG ((DEBUG_INFO, "   Length = 0x%lx\n", Hob.FirmwareVolume2->Length));
  DEBUG ((DEBUG_INFO, "   FvName = %g\n", &Hob.FirmwareVolume2->FvName));
  DEBUG ((DEBUG_INFO, "   FileName = %g\n", &Hob.FirmwareVolume2->FileName));
  return EFI_SUCCESS;
}

EFI_STATUS
PrintCapsuleHob(
  IN  VOID          *HobStart,
  IN  UINTN          Hobsize
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  if(Hobsize < sizeof(Hob.Capsule)){
    DEBUG ((DEBUG_INFO, "   Some Information Missing.. Current HandoffInformationTable Hobsize:0x%x is less than \
                               Original Capsule size:0x%x \n", Hobsize,sizeof(Hob.Capsule)));
  }
  DEBUG ((DEBUG_INFO, "   BaseAddress = 0x%lx\n", Hob.Capsule->BaseAddress));
  DEBUG ((DEBUG_INFO, "   Length = 0x%lx\n", Hob.Capsule->Length));
  return EFI_SUCCESS;
}

EFI_STATUS
PrintFv3Hob(
  IN  VOID          *HobStart,
  IN  UINTN          Hobsize
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  if(Hobsize < sizeof(Hob.FirmwareVolume3)){
    DEBUG ((DEBUG_INFO, "   Some Information Missing.. Current HandoffInformationTable Hobsize:0x%x is less than \
                               Original FirmwareVolume3 size:0x%x \n", Hobsize,sizeof(Hob.FirmwareVolume3)));
  }
  DEBUG ((DEBUG_INFO, "   BaseAddress = 0x%lx\n", Hob.FirmwareVolume3->BaseAddress));
  DEBUG ((DEBUG_INFO, "   Length = 0x%lx\n", Hob.FirmwareVolume3->Length));
  DEBUG ((DEBUG_INFO, "   AuthenticationStatus = 0x%x\n", Hob.FirmwareVolume3->AuthenticationStatus));
  DEBUG ((DEBUG_INFO, "   ExtractedFv = %g\n", &Hob.FirmwareVolume3->ExtractedFv)); //BOOLEAN
  DEBUG ((DEBUG_INFO, "   FileName = %g\n", &Hob.FirmwareVolume3->FileName)); 
  return EFI_SUCCESS;
}


EFI_STATUS
PrintNotCorrectHob(
  IN  VOID          *HobStart,
  IN  UINTN          Hobsize
  )
{
  ASSERT(FALSE);
  return EFI_SUCCESS;
}

HOB_PRINT_HANDLE_TABLE HobHandles[] = {
 {EFI_HOB_TYPE_INCORRECT,           PrintNotCorrectHob},                  //0x0000
 {EFI_HOB_TYPE_HANDOFF,             PrintHandOffHob},                     //0x0001
 {EFI_HOB_TYPE_MEMORY_ALLOCATION,   PrintMemoryAllocationHob},            //0x0002
 {EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, PrintResourceDiscriptorHob},          //0x0003
 {EFI_HOB_TYPE_GUID_EXTENSION,      PrintGuidHob},                        //0x0004
 {EFI_HOB_TYPE_FV,                  PrintFvHob},                          //0x0005
 {EFI_HOB_TYPE_CPU,                 PrintCpuHob},                         //0x0006
 {EFI_HOB_TYPE_MEMORY_POOL,         PrintMemoryPoolHob},                  //0x0007
 {EFI_HOB_TYPE_INCORRECT,           PrintNotCorrectHob},                  //0x0008
 {EFI_HOB_TYPE_FV2,                 PrintFv2Hob},                         //0x0009
 {EFI_HOB_TYPE_INCORRECT,           PrintNotCorrectHob},                  //0x000A
 {EFI_HOB_TYPE_UEFI_CAPSULE,        PrintCapsuleHob},                     //0x000B
 {EFI_HOB_TYPE_FV3,                 PrintFv3Hob}                          //0x000C
  };


/**
  Print all HOBs info from the HOB list.

  @return The pointer to the HOB list.
**/
VOID
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
  DEBUG ((DEBUG_INFO, "Totally number of HobHandlers = %d\n", ARRAY_SIZE(HobHandles)));
  NumberOfPrintHandles = ARRAY_SIZE(HobHandles);
  
  Count = 0;

  //
  // Parse the HOB list to see which type it is, and print the information.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    Count++;
    DEBUG ((DEBUG_INFO, "HOB[%d]: Type = %a, Offset = 0x%p, Length = 0x%x\n", Count, mHobTypeStr[Hob.Header->HobType], (Hob.Raw - (UINT8 *)HobStart), Hob.Header->HobLength));
    if (Hob.Header->HobType < NumberOfPrintHandles && Hob.Header->HobType == HobHandles[Hob.Header->HobType].Type) {
      HobHandles[Hob.Header->HobType].Function(Hob.Raw,Hob.Header->HobLength);
    }
    else{
      PrintHex(GET_GUID_HOB_DATA (Hob.Raw), GET_GUID_HOB_DATA_SIZE (Hob.Raw));
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  DEBUG ((DEBUG_INFO, "There are totally %d Hobs, the End Hob address is %p\n", Count, Hob.Raw));

}

