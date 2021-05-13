/** @file
  This library will parse the Slim Bootloader to get required information.

  Copyright (c) 2014 - 2019, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <IndustryStandard/Acpi.h>
#include <Library/PlatformSupportLib.h>
#include <Library/BlParseLib.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>

#include "SystemTableInfoGuid.h"
#include "LoaderPlatformInfoGuid.h"
#include "FlashMapInfoGuid.h"
#include "DeviceTableHobGuid.h"
#include "SmmInfoGuid.h"



/**
  This function retrieves a GUIDed HOB data from Slim Bootloader.

  This function will search SBL HOB list to find the first GUIDed HOB that
  its GUID matches Guid.

  @param[in]  Guid        A pointer to HOB GUID to search.

  @retval NULL            Failed to find the GUID HOB.
  @retval others          GUIDed HOB data pointer.

**/
VOID *
GetGuidHobData (
  IN       EFI_GUID      *Guid
  )
{
  UINT8                  *GuidHob;
  CONST VOID             *HobList;

  HobList = GetParameterBase ();
  ASSERT (HobList != NULL);
  GuidHob = GetNextGuidHob (Guid, HobList);
  if (GuidHob != NULL) {
    return GET_GUID_HOB_DATA (GuidHob);
  }

  return NULL;
}

/**
  Get the SMRAM info

  @param[out]     SmramHob      Pointer to SMRAM information
  @param[in, out] Size          The buffer size in input, and the actual data size in output.

  @retval RETURN_SUCCESS       Successfully find out the SMRAM info.
  @retval EFI_BUFFER_TOO_SMALL The buffer is too small for the data.
  @retval RETURN_NOT_FOUND     Failed to find the SMRAM info.

**/
RETURN_STATUS
GetSmramInfo (
  OUT EFI_SMRAM_HOB_DESCRIPTOR_BLOCK   *SmramHob,
  IN OUT UINT32                        *Size
  )
{
  LOADER_SMM_INFO                      *SmmInfo;
  UINT32                               NeedSize;

  SmmInfo = (LOADER_SMM_INFO *) GetGuidHobData (&gSmmInformationGuid);
  if (SmmInfo == NULL) {
    ASSERT (FALSE);
    return RETURN_NOT_FOUND;
  }

  NeedSize = sizeof (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK);
  if ((SmmInfo->Flags & SMM_FLAGS_4KB_COMMUNICATION) != 0) {
    NeedSize += sizeof (EFI_SMRAM_DESCRIPTOR);
  }

  if (NeedSize > *Size) {
    *Size = NeedSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  if ((SmmInfo->Flags & SMM_FLAGS_4KB_COMMUNICATION) == 0) {
    SmramHob->NumberOfSmmReservedRegions  = 1;
    SmramHob->Descriptor[0].CpuStart      = SmmInfo->SmmBase;
    SmramHob->Descriptor[0].PhysicalStart = SmmInfo->SmmBase;
    SmramHob->Descriptor[0].PhysicalSize  = SmmInfo->SmmSize;
    SmramHob->Descriptor[0].RegionState   = 0;
  } else {
    SmramHob->NumberOfSmmReservedRegions  = 2;
    SmramHob->Descriptor[0].CpuStart      = SmmInfo->SmmBase;
    SmramHob->Descriptor[0].PhysicalStart = SmmInfo->SmmBase;
    SmramHob->Descriptor[0].PhysicalSize  = SIZE_4KB;
    SmramHob->Descriptor[0].RegionState   = EFI_ALLOCATED;

    SmramHob->Descriptor[1].CpuStart      = SmmInfo->SmmBase + SIZE_4KB;
    SmramHob->Descriptor[1].PhysicalStart = SmmInfo->SmmBase + SIZE_4KB;
    SmramHob->Descriptor[1].PhysicalSize  = SmmInfo->SmmSize - SIZE_4KB;
    SmramHob->Descriptor[1].RegionState   = 0;
  }
  *Size = NeedSize;

  return RETURN_SUCCESS;
}


/**
  Get the SMM register info

  @param[out]     SmmRegHob      Pointer to SMM register information
  @param[in, out] Size          The buffer size in input, and the actual data size in output.

  @retval RETURN_SUCCESS       Successfully find out the SMM register info.
  @retval EFI_BUFFER_TOO_SMALL The buffer is too small for the data.
  @retval RETURN_NOT_FOUND     Failed to find the SMM register info.

**/
RETURN_STATUS
GetSmmRegisterInfo (
  OUT PLD_SMM_REGISTERS                *SmmRegHob,
  IN OUT UINT32                        *Size
  )
{
  LOADER_SMM_INFO                      *SmmInfo;
  UINT32                               NeedSize;
  PLD_GENERIC_REGISTER                 *Reg;

  SmmInfo = (LOADER_SMM_INFO *) GetGuidHobData (&gSmmInformationGuid);
  if (SmmInfo == NULL) {
    ASSERT (FALSE);
    return RETURN_NOT_FOUND;
  }

  NeedSize = sizeof (PLD_SMM_REGISTERS) + sizeof (PLD_GENERIC_REGISTER) * 5;
  if (NeedSize > *Size) {
    *Size = NeedSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  SmmRegHob->Revision = 0;
  SmmRegHob->Count    = 5;

  Reg = &SmmRegHob->Registers[0];
  Reg->Id                        = REGISTER_ID_SMI_GBL_EN;
  Reg->Value                     = 1;
  Reg->Address.AddressSpaceId    = EFI_ACPI_3_0_SYSTEM_IO;
  Reg->Address.RegisterBitWidth  = 1;
  Reg->Address.RegisterBitOffset = SmmInfo->SmiCtrlReg.SmiGblPos;
  Reg->Address.AccessSize        = EFI_ACPI_3_0_DWORD;
  Reg->Address.Address           = SmmInfo->SmiCtrlReg.Address;

  Reg++;
  Reg->Id                        = REGISTER_ID_SMI_GBL_EN_LOCK;
  Reg->Value                     = 1;
  Reg->Address.AddressSpaceId    = EFI_ACPI_3_0_SYSTEM_MEMORY;
  Reg->Address.RegisterBitWidth  = 1;
  Reg->Address.RegisterBitOffset = SmmInfo->SmiLockReg.SmiLockPos;
  Reg->Address.AccessSize        = EFI_ACPI_3_0_DWORD;
  Reg->Address.Address           = SmmInfo->SmiLockReg.Address;

  Reg++;
  Reg->Id                        = REGISTER_ID_SMI_EOS;
  Reg->Value                     = 1;
  Reg->Address.AddressSpaceId    = EFI_ACPI_3_0_SYSTEM_IO;
  Reg->Address.RegisterBitWidth  = 1;
  Reg->Address.RegisterBitOffset = SmmInfo->SmiCtrlReg.SmiEosPos;
  Reg->Address.AccessSize        = EFI_ACPI_3_0_DWORD;
  Reg->Address.Address           = SmmInfo->SmiCtrlReg.Address;

  Reg++;
  Reg->Id                        = REGISTER_ID_SMI_APM_EN;
  Reg->Value                     = 1;
  Reg->Address.AddressSpaceId    = EFI_ACPI_3_0_SYSTEM_IO;
  Reg->Address.RegisterBitWidth  = 1;
  Reg->Address.RegisterBitOffset = SmmInfo->SmiCtrlReg.SmiApmPos;
  Reg->Address.AccessSize        = EFI_ACPI_3_0_DWORD;
  Reg->Address.Address           = SmmInfo->SmiCtrlReg.Address;

  Reg++;
  Reg->Id                        = REGISTER_ID_SMI_APM_STS;
  Reg->Value                     = 1;
  Reg->Address.AddressSpaceId    = EFI_ACPI_3_0_SYSTEM_IO;
  Reg->Address.RegisterBitWidth  = 1;
  Reg->Address.RegisterBitOffset = SmmInfo->SmiStsReg.SmiApmPos;
  Reg->Address.AccessSize        = EFI_ACPI_3_0_DWORD;
  Reg->Address.Address           = SmmInfo->SmiStsReg.Address;

  *Size = NeedSize;

  return RETURN_SUCCESS;
}


/**
  Get the payload S3 communication HOB

  @param[out]     PldS3Hob      Pointer to payload S3 communication buffer
  @param[in, out] Size          The buffer size in input, and the actual data size in output.

  @retval RETURN_SUCCESS       Successfully get the payload S3 communication HOB.
  @retval EFI_BUFFER_TOO_SMALL The buffer is too small for the data.
  @retval RETURN_NOT_FOUND     Failed to find the SMM communication info.

**/
RETURN_STATUS
GetPldS3CommunicationInfo (
  OUT PLD_S3_COMMUNICATION             *PldS3Hob,
  IN OUT UINT32                        *Size
  )
{
  LOADER_SMM_INFO                      *SmmInfo;

  SmmInfo = (LOADER_SMM_INFO *) GetGuidHobData (&gSmmInformationGuid);
  if (SmmInfo == NULL) {
    ASSERT (FALSE);
    return RETURN_NOT_FOUND;
  }

  if ((SmmInfo->Flags & SMM_FLAGS_4KB_COMMUNICATION) == 0) {
    return RETURN_UNSUPPORTED;
  }

  if (sizeof (PLD_S3_COMMUNICATION) > *Size) {
    *Size = sizeof (PLD_S3_COMMUNICATION);
    return EFI_BUFFER_TOO_SMALL;
  }

  PldS3Hob->CommBuffer.CpuStart      = SmmInfo->SmmBase;
  PldS3Hob->CommBuffer.PhysicalStart = SmmInfo->SmmBase;
  PldS3Hob->CommBuffer.PhysicalSize  = SIZE_4KB;
  PldS3Hob->CommBuffer.RegionState   = 0;
  PldS3Hob->PldAcpiS3Enable          =FALSE;
  *Size = sizeof (PLD_S3_COMMUNICATION);

  return RETURN_SUCCESS;
}


/**
  Gets component information from the flash map by partition.

  This function will look for the component matching the input signature
  in the flash map, if found, it will look for the component with back up
  flag based on the backup partition parmeter and will return the
  base address and size of the component.

  @param[in]  Signature         Signature of the component information required
  @param[in]  IsBackupPartition TRUE for Back up copy, FALSE for primary copy
  @param[out] Base              Base address of the component
  @param[out] Size              Size of the component

  @retval    EFI_SUCCESS    Found the component with the matching signature.
  @retval    EFI_NOT_FOUND  Component with the matching signature not found.

**/
EFI_STATUS
GetComponentInfo (
  IN  UINT32     Signature,
  IN  BOOLEAN    IsBackupPartition,
  OUT UINT32     *Base,
  OUT UINT32     *Size
  )
{
  UINTN                 Index;
  UINT32                MaxEntries;
  UINT32                PcdBase;
  UINT32                PcdSize;
  UINT32                RomBase;
  FLASH_MAP             *FlashMapPtr;
  EFI_STATUS            Status;
  FLASH_MAP_ENTRY_DESC  EntryDesc;

  PcdBase = 0;
  PcdSize = 0;
  Status = EFI_NOT_FOUND;

  FlashMapPtr = (FLASH_MAP *) GetGuidHobData (&gFlashMapInfoGuid);
  if (FlashMapPtr == NULL) {
    return RETURN_NOT_FOUND;
  }

  RomBase = (UINT32) (0x100000000ULL - FlashMapPtr->RomSize);
  MaxEntries = ((FlashMapPtr->Length - FLASH_MAP_HEADER_SIZE) / sizeof (FLASH_MAP_ENTRY_DESC));

  for (Index = 0; Index < MaxEntries; Index++) {
    EntryDesc = FlashMapPtr->EntryDesc[Index];
    //
    // Look for the component with desired signature
    //
    if (EntryDesc.Signature == 0xFFFFFFFF) {
      Status = EFI_NOT_FOUND;
      break;
    }
    if (EntryDesc.Signature == Signature) {
      //
      // Check if need to get back up copy
      // Back up copies can be identified with back up flag
      //
      if ( ((EntryDesc.Flags & FLASH_MAP_FLAGS_NON_REDUNDANT_REGION) != 0) ||
           (((IsBackupPartition ? FLASH_MAP_FLAGS_BACKUP : 0) ^ (EntryDesc.Flags & FLASH_MAP_FLAGS_BACKUP)) == 0) ) {
        PcdBase = (UINT32) (RomBase + EntryDesc.Offset);
        PcdSize = EntryDesc.Size;
        Status = EFI_SUCCESS;
        break;
      }
    }
  }

  //
  // If base and pcdbase are not 0, fill and return the value
  //
  if ((Base != NULL) && (PcdBase != 0)) {
    *Base = PcdBase;
  }
  if ((Size != NULL) && (PcdSize != 0)) {
    *Size = PcdSize;
  }

  return Status;
}


/**
  Get device address

  If PCI device in device table, the device address format is 0x00BBDDFF, where
  BB, DD and FF are PCI bus, device and function number.
  If the device is MMIO device, the device address format is 0xMMxxxxxx, where
  MM should be non-zero value, xxxxxx could be any value.

  @param[in]  DeviceType         The device type, refer OS_BOOT_MEDIUM_TYPE.
  @param[in]  DeviceInstance     The device instance number starting from 0.

  @retval     Device address for a given device instance, return 0 if the device
              could not be found from device table.
**/
UINT32
GetDeviceAddr (
  IN  UINT8          DeviceType,
  IN  UINT8          DeviceInstance
  )
{
  PLT_DEVICE_TABLE   *DeviceTable;
  PLT_DEVICE         *PltDevice;
  UINT32             DeviceBase;
  UINT32             Index;
  UINT8              Bus;
  UINT8              Device;
  UINT8              Function;

  DeviceTable = (PLT_DEVICE_TABLE *) GetGuidHobData (&gDeviceTableHobGuid);
  DEBUG((EFI_D_INFO, "   DeviceTable = 0x%x\n",DeviceTable));
  if (DeviceTable == NULL) {
    return 0;
  }

  DeviceBase = 0;
  PltDevice  = NULL;
  for (Index = 0; Index < DeviceTable->DeviceNumber; Index++) {
    PltDevice = &DeviceTable->Device[Index];
    if ((PltDevice->Type == DeviceType) && (PltDevice->Instance == DeviceInstance)){
      DEBUG((EFI_D_INFO, "   found it  = 0x%x\n",PltDevice->Dev.DevAddr));
      break;
    }
  }

  if (DeviceTable->DeviceNumber != Index) {
    DeviceBase = PltDevice->Dev.DevAddr;
  }

  Bus        = (UINT8)((DeviceBase >> 16) & 0xFF);
  Device     = (UINT8)((DeviceBase >> 8)  & 0xFF);
  Function   = (UINT8)(DeviceBase & 0xFF);
  DeviceBase = (UINT32)(Bus << 20) + (UINT32)(Device << 15) + (UINT32)(Function << 12);

  return DeviceBase;
}


/**
  get PCIe base address from ACPI table

  @param[in]  AcpiTableBase        ACPI table start address in memory

  @retval PCIE BASE                Successfully get it from ACPI table.
  @retval 0                        Failed to get PCIE from ACPI table

**/
UINT64
GetPcieBaseFromAcpi (
  IN   UINT64                                   AcpiTableBase
  )
{
  EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;
  EFI_ACPI_DESCRIPTION_HEADER                   *Rsdt;
  UINT32                                        *Entry32;
  UINTN                                         Entry32Num;
  UINTN                                         Idx;
  UINT32                                        *Signature;
  EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *MmCfgHdr;
  EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE *MmCfgBase;
  UINT64                                        PcieBaseAddress;

  Rsdp = (EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)(UINTN)AcpiTableBase;
  DEBUG ((DEBUG_INFO, "Rsdp at 0x%p\n", Rsdp));
  DEBUG ((DEBUG_INFO, "Rsdt at 0x%x\n", Rsdp->RsdtAddress));

  //
  // Search Rsdt
  //
  MmCfgHdr = NULL;
  PcieBaseAddress = 0;
  Rsdt     = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)(Rsdp->RsdtAddress);
  if (Rsdt != NULL) {
    Entry32  = (UINT32 *)(Rsdt + 1);
    Entry32Num = (Rsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 2;
    for (Idx = 0; Idx < Entry32Num; Idx++) {
      Signature = (UINT32 *)(UINTN)Entry32[Idx];
      if (*Signature == EFI_ACPI_5_0_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE) {
        MmCfgHdr = (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *)Signature;
        DEBUG ((DEBUG_INFO, "Found MM config address in Rsdt\n"));
        break;
      }
    }
  }

  if (MmCfgHdr == NULL) {
    return 0;
  }

  MmCfgBase = (EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE *)((UINT8*) MmCfgHdr + sizeof (*MmCfgHdr));
  PcieBaseAddress = MmCfgBase->BaseAddress;
  DEBUG ((DEBUG_INFO, "PcieBaseAddr 0x%lx\n", PcieBaseAddress));

  return PcieBaseAddress;
}


/**
  Find the Spi flash related info.

  @param  SpiFlashInfo        Pointer to Spi flash info structure.

  @retval RETURN_SUCCESS     Successfully find the info.
  @retval RETURN_NOT_FOUND   Failed to find the info .

**/
RETURN_STATUS
ParseSpiFlashInfo (
  OUT SPI_FLASH_INFO           *SpiFlashInfo
  )
{
  LOADER_PLATFORM_INFO         *PlatformInfo;
  UINT64                       SpiPciBase;
  SYSTEM_TABLE_INFO            *TableInfo;

  TableInfo = (SYSTEM_TABLE_INFO *)GetGuidHobData (&gUefiSystemTableInfoGuid);
  if (TableInfo == NULL) {
    ASSERT (FALSE);
    return RETURN_NOT_FOUND;
  }

  // SPI PCI config base = PCIE base + first SPI device address
  SpiPciBase = GetPcieBaseFromAcpi (TableInfo->AcpiTableBase);
  ASSERT (SpiPciBase != 0);
  SpiPciBase += GetDeviceAddr (4, 0);
  DEBUG((EFI_D_INFO, " SpiPciBase = 0x%x\n", SpiPciBase));

  SpiFlashInfo->SpiAddress.AddressSpaceId    = SPACE_ID_PCI_CONFIGURATION;
  SpiFlashInfo->SpiAddress.RegisterBitWidth  = 32;
  SpiFlashInfo->SpiAddress.RegisterBitOffset = 0;
  SpiFlashInfo->SpiAddress.AccessSize        = REGISTER_BIT_WIDTH_DWORD;
  SpiFlashInfo->SpiAddress.Address           = SpiPciBase;

  PlatformInfo = (LOADER_PLATFORM_INFO *) GetGuidHobData (&gLoaderPlatformInfoGuid);
  if (PlatformInfo != NULL) {
    SpiFlashInfo->Flags = PlatformInfo->Flags;
    DEBUG((EFI_D_INFO, " SpiFlashInfo->Flags = 0x%x\n", SpiFlashInfo->Flags));
  }

  return EFI_SUCCESS;
}



/**
  GEt the NV variable info.

  @param  NvVariableInfo     Pointer to NV variable info.

  @retval RETURN_SUCCESS     Successfully get the info.
  @retval RETURN_NOT_FOUND   Failed to get the info 

**/
RETURN_STATUS
ParseNvVariableInfo (
  OUT NV_VARIABLE_INFO         *NvVariableInfo
  )
{
  EFI_STATUS                   Status;
  UINT32                       Base;
  UINT32                       Size;

  Status = GetComponentInfo (FLASH_MAP_SIG_EPVARIABLE, FALSE, &Base, &Size);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NvVariableInfo->VariableStoreBase = Base;
  NvVariableInfo->VariableStoreSize = Size;

  return EFI_SUCCESS;
}

/**
  Parse platform specific information from bootloader

  @retval RETURN_SUCCESS       The platform specific coreboot support succeeded.
  @retval RETURN_DEVICE_ERROR  The platform specific coreboot support could not be completed.

**/
EFI_STATUS
EFIAPI
ParsePlatformInfo (
  VOID
  )
{
  EFI_STATUS                       Status;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK   *SmramHob;
  UINT32                           Size;
  UINT8                            Buffer[200];
  PLD_SMM_REGISTERS                *SmmRegisterHob;
  VOID                             *NewHob;
  UINT32                           Index;
  PLD_S3_COMMUNICATION             *PldS3Info;
  SPI_FLASH_INFO                   SpiFlashInfo;
  SPI_FLASH_INFO                   *NewSpiFlashInfo;
  NV_VARIABLE_INFO                 NvVariableInfo;
  NV_VARIABLE_INFO                 *NewNvVariableInfo;

  //
  // Create SMRAM information HOB
  //
  SmramHob = (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK *)Buffer;
  Size     = sizeof (Buffer);
  Status = GetSmramInfo (SmramHob, &Size);
  DEBUG((DEBUG_INFO, "GetSmramInfo = %r, data Size = 0x%x\n", Status, Size));
  if (!EFI_ERROR (Status)) {
    NewHob = BuildGuidHob (&gEfiSmmSmramMemoryGuid, Size);
    ASSERT (NewHob != NULL);
    CopyMem (NewHob, SmramHob, Size);
    DEBUG((DEBUG_INFO, "Region count = 0x%x\n", SmramHob->NumberOfSmmReservedRegions));
    for (Index = 0; Index < SmramHob->NumberOfSmmReservedRegions; Index++ ) {
      DEBUG((DEBUG_INFO, "CpuStart[%d] = 0x%lx\n", Index, SmramHob->Descriptor[Index].CpuStart));
      DEBUG((DEBUG_INFO, "base[%d]     = 0x%lx\n", Index, SmramHob->Descriptor[Index].PhysicalStart));
      DEBUG((DEBUG_INFO, "size[%d]     = 0x%lx\n", Index, SmramHob->Descriptor[Index].PhysicalSize));
      DEBUG((DEBUG_INFO, "State[%d]    = 0x%lx\n", Index, SmramHob->Descriptor[Index].RegionState));
    }
  }

  //
  // Create SMM register information HOB
  //
  SmmRegisterHob = (PLD_SMM_REGISTERS *)Buffer;
  Size           = sizeof (Buffer);
  Status = GetSmmRegisterInfo (SmmRegisterHob, &Size);
  DEBUG((DEBUG_INFO, "GetSmmRegisterInfo = %r, data Size = 0x%x\n", Status, Size));
  if (!EFI_ERROR (Status)) {
    NewHob = BuildGuidHob (&gPldSmmRegisterInfoGuid, Size);
    ASSERT (NewHob != NULL);
    CopyMem (NewHob, SmramHob, Size);
    DEBUG((DEBUG_INFO, "SMM register count = 0x%x\n", SmmRegisterHob->Count));
    for (Index = 0; Index < SmmRegisterHob->Count; Index++ ) {
      DEBUG((DEBUG_INFO, "ID[%d]            = 0x%lx\n", Index, SmmRegisterHob->Registers[Index].Id));
      DEBUG((DEBUG_INFO, "Value[%d]         = 0x%lx\n", Index, SmmRegisterHob->Registers[Index].Value));
      DEBUG((DEBUG_INFO, "AddressSpaceId    = 0x%x\n",  SmmRegisterHob->Registers[Index].Address.AddressSpaceId));
      DEBUG((DEBUG_INFO, "RegisterBitWidth  = 0x%x\n",  SmmRegisterHob->Registers[Index].Address.RegisterBitWidth));
      DEBUG((DEBUG_INFO, "RegisterBitOffset = 0x%x\n",  SmmRegisterHob->Registers[Index].Address.RegisterBitOffset));
      DEBUG((DEBUG_INFO, "AccessSize        = 0x%x\n",  SmmRegisterHob->Registers[Index].Address.AccessSize));
      DEBUG((DEBUG_INFO, "Address           = 0x%lx\n", SmmRegisterHob->Registers[Index].Address.Address));
    }
  }

  //
  // Create SMM S3 information HOB
  //
  PldS3Info = (PLD_S3_COMMUNICATION *)Buffer;
  Size      = sizeof (Buffer);
  Status = GetPldS3CommunicationInfo (PldS3Info, &Size);
  DEBUG((DEBUG_INFO, "GetPldS3Info = %r, data Size = 0x%x\n", Status, Size));
  if (!EFI_ERROR (Status)) {
    NewHob = BuildGuidHob (&gPldS3CommunicationGuid, Size);
    ASSERT (NewHob != NULL);
    CopyMem (NewHob, SmramHob, Size);
    PldS3Info = (PLD_S3_COMMUNICATION *)NewHob;
  }

  //
  // Create a HOB for SPI flash info
  //
  Status = ParseSpiFlashInfo(&SpiFlashInfo);
  if (!EFI_ERROR (Status)) {
    NewSpiFlashInfo = BuildGuidHob (&gSpiFlashInfoGuid, sizeof (SPI_FLASH_INFO));
    ASSERT (NewSpiFlashInfo != NULL);
    CopyMem (NewSpiFlashInfo, &SpiFlashInfo, sizeof (SPI_FLASH_INFO));
    DEBUG ((DEBUG_INFO, "Flags=0x%x, SPI PCI Base=0x%lx\n", SpiFlashInfo.Flags, SpiFlashInfo.SpiAddress.Address));
  }

  //
  // Create a hob for NV variable
  //
  Status = ParseNvVariableInfo(&NvVariableInfo);
  if (!EFI_ERROR (Status)) {
    NewNvVariableInfo = BuildGuidHob (&gNvVariableInfoGuid, sizeof (NV_VARIABLE_INFO));
    ASSERT (NewNvVariableInfo != NULL);
    CopyMem (NewNvVariableInfo, &NvVariableInfo, sizeof (NV_VARIABLE_INFO));
    DEBUG ((DEBUG_INFO, "VarStoreBase=0x%x, length=0x%x\n", NvVariableInfo.VariableStoreBase, NvVariableInfo.VariableStoreSize));
  }

  return EFI_SUCCESS;
}