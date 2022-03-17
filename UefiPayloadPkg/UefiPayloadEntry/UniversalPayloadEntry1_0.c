/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"
#include <Library/GetUplDataLib.h>

extern VOID  *mHobList;

/**
  Print all HOBs info from the HOB list.

  @return The pointer to the HOB list.
**/
VOID
PrintHob (
  IN CONST VOID  *HobStart
  );

EFI_STATUS
InitHobList (
  )
{
  UINTN                  MemoryMapCount;
  UINTN                  MemoryMapNumber;
  EFI_MEMORY_DESCRIPTOR  MemoryMap;
  UINTN                  FindIndex;
  UINTN                  Index;
  EFI_PHYSICAL_ADDRESS   FreeMemoryBottom;
  EFI_PHYSICAL_ADDRESS   FreeMemoryTop;
  EFI_PHYSICAL_ADDRESS   MemoryBottom;
  EFI_PHYSICAL_ADDRESS   MemoryTop;
  EFI_STATUS             Status;

  MemoryMapCount = 0;

  Status = GetUplMemoryMap (&MemoryMap, &MemoryMapCount, 0);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return EFI_NOT_FOUND;
  }

  FindIndex = MemoryMapCount;
  DEBUG ((DEBUG_INFO, "MemoryMapCount%x\n", MemoryMapCount));
  for (Index = 0; Index < MemoryMapCount; Index++) {
    DEBUG ((DEBUG_INFO, "Index%x\n", Index));
    MemoryMapNumber = 1;
    Status          =  GetUplMemoryMap (&MemoryMap, &MemoryMapNumber, Index);
    if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
    }

    DEBUG ((DEBUG_INFO, "MemoryMap.PhysicalStart %lx\n", MemoryMap.PhysicalStart));
    DEBUG ((DEBUG_INFO, "MemoryMap.NumberOfPages %lx %lx\n", MemoryMap.NumberOfPages, FixedPcdGet32 (PcdSystemMemoryUefiRegionSize)));
    DEBUG ((DEBUG_INFO, "MemoryMap.Type %lx\n", MemoryMap.Type));
    //
    // Skip above 4G memory
    //
    if (MemoryMap.PhysicalStart + EFI_PAGES_TO_SIZE (MemoryMap.NumberOfPages) > BASE_4GB) {
      continue;
    }

    if ((MemoryMap.Type != EfiConventionalMemory) || (EFI_PAGES_TO_SIZE (MemoryMap.NumberOfPages) < FixedPcdGet32 (PcdSystemMemoryUefiRegionSize))) {
      continue;
    }

    FindIndex = Index;
  }

  DEBUG ((DEBUG_INFO, "FindIndex%x\n", FindIndex));
  if (FindIndex == MemoryMapCount) {
    return EFI_NOT_FOUND;
  }

  MemoryMapNumber = 1;
  Status          = GetUplMemoryMap (&MemoryMap, &MemoryMapNumber, FindIndex);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  MemoryTop        = EFI_PAGES_TO_SIZE (MemoryMap.NumberOfPages) + MemoryMap.PhysicalStart;
  FreeMemoryTop    = MemoryTop;
  MemoryBottom     = MemoryTop - FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);
  FreeMemoryBottom = MemoryBottom;
  DEBUG ((DEBUG_INFO, "MemoryTop%lx\n", MemoryTop));
  HobConstructor ((VOID *)(UINTN)MemoryBottom, (VOID *)(UINTN)MemoryTop, (VOID *)(UINTN)FreeMemoryBottom, (VOID *)(UINTN)FreeMemoryTop);

  UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO  *Serial;
  UINT64                              BaudRate;
  UINT64                              RegisterBase;
  UINT8                               RegisterStride;
  BOOLEAN                             UseMmio;

  Serial                  = BuildGuidHob (&gUniversalPayloadSerialPortInfoGuid, sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO));
  Serial->Header.Revision = UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO_REVISION;
  Serial->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO);
  Status                  =  GetUplUint64 ("SerialPortBaudRate", &BaudRate);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Serial->BaudRate = (UINT32)BaudRate;
  Status           =  GetUplUint64 ("SerialPortRegisterBase", &RegisterBase);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Serial->RegisterBase = RegisterBase;

  Status =  GetUplUint8 ("SerialPortRegisterStride", &RegisterStride);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Serial->RegisterStride = RegisterStride;

  Status =  GetUplBoolean ("SerialPortUseMmio", &UseMmio);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Serial->UseMmio = UseMmio;

  return EFI_SUCCESS;
}

BOOLEAN
IsHobNeed (
  EFI_PEI_HOB_POINTERS  Hob
  );

VOID
AddNewHob (
  IN EFI_PEI_HOB_POINTERS  *Hob
  );

EFI_STATUS
CreateHobsBasedOnMemoryMap1_0 (
  VOID
  );

EFI_STATUS
CreateFvHobsBasedOnUplData (
  OUT EFI_FIRMWARE_VOLUME_HEADER  **DxeFv
  )
{
  UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY  *ExtraData;
  UINTN                               Count;
  UINTN                               Index;

  //
  // Get DXE FV location
  //
  Count = 0;
  GetUplExtraData (NULL, &Count, 0);
  ExtraData = AllocatePool (Count * sizeof (UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY));

  GetUplExtraData (ExtraData, &Count, 0);
  for (Index = 0; Index < Count; Index++) {
    if (AsciiStrCmp (ExtraData[Index].Identifier, "uefi_fv") == 0) {
      *DxeFv = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)ExtraData[Index].Base;
      ASSERT ((*DxeFv)->FvLength == ExtraData[Index].Size);
      BuildFvHob ((EFI_PHYSICAL_ADDRESS)(UINTN)*DxeFv, (*DxeFv)->FvLength);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
CreateHobsBasedOnUplData (
  VOID
  )
{
  EFI_STATUS            Status;
  UINT64                HobList;
  EFI_PEI_HOB_POINTERS  Hob;

  Status = GetUplUint64 ("HobList", &HobList);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "UPL data doesn't contain HobList\n"));
  } else {
    Hob.Raw = (VOID *)(UINTN)HobList;
    PrintHob (Hob.Raw);
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
  }

  //
  // Create guid hob for acpi board information
  //
  UINT64                        Rsdp;
  ACPI_BOARD_INFO               *AcpiBoardInfo;
  UNIVERSAL_PAYLOAD_ACPI_TABLE  *AcpiTableHob;

  Status = GetUplUint64 ("AcpiTableRsdp", &Rsdp);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "UPL data doesn't contain HobList\n"));
  } else {
    AcpiTableHob                  = (UNIVERSAL_PAYLOAD_ACPI_TABLE *)BuildGuidHob (&gUniversalPayloadAcpiTableGuid, sizeof (UNIVERSAL_PAYLOAD_ACPI_TABLE));
    AcpiTableHob->Rsdp            = (EFI_PHYSICAL_ADDRESS)Rsdp;
    AcpiTableHob->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_ACPI_TABLE);
    AcpiTableHob->Header.Revision = UNIVERSAL_PAYLOAD_ACPI_TABLE_REVISION;
    AcpiBoardInfo                 = BuildHobFromAcpi (Rsdp);
    ASSERT (AcpiBoardInfo != NULL);
  }

  //
  // Build CPU Hob
  //
  UINT8  SizeOfMemorySpace;
  UINT8  SizeOfIoSpace;

  Status = GetUplUint8 ("MemorySpace", &SizeOfMemorySpace);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "UPL data doesn't contain MemorySpace\n"));
    return EFI_NOT_FOUND;
  }

  Status = GetUplUint8 ("IoSpace", &SizeOfIoSpace);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "UPL data doesn't contain IoSpace\n"));
    return EFI_NOT_FOUND;
  }

  BuildCpuHob (SizeOfMemorySpace, SizeOfIoSpace);

  //
  // Build gUniversalPayloadPciRootBridgeInfoGuid Hob
  //
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE   *RootBridge;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES  *PciRootBridgeInfo;
  BOOLEAN                             ResourceAssigned;
  UINTN                               Count;

  Count  = 0;
  Status = GetUplPciRootBridges (NULL, &Count, 0);
  if (Status != RETURN_BUFFER_TOO_SMALL) {
    DEBUG ((DEBUG_INFO, "UPL data doesn't contain PciRootBridges info\n"));
  } else {
    RootBridge = AllocatePool (Count * sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE));

    Status = GetUplPciRootBridges (RootBridge, &Count, 0);
    if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
    }

    DEBUG ((DEBUG_INFO, "GetUplPciRootBridges... %d\n", Count));

    PciRootBridgeInfo = BuildGuidHob (&gUniversalPayloadPciRootBridgeInfoGuid, sizeof (PciRootBridgeInfo) + sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE));
    CopyMem (PciRootBridgeInfo->RootBridge, RootBridge, sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE));
    PciRootBridgeInfo->Count           = (UINT8)Count;
    PciRootBridgeInfo->Header.Length   = Count * sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE) + sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES);
    PciRootBridgeInfo->Header.Revision = UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_REVISION;

    Status = GetUplBoolean ("RootBridgeResourceAssigned", &ResourceAssigned);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "UPL data doesn't contain ResourceAssigned\n"));
      ResourceAssigned = FALSE;
    }

    PciRootBridgeInfo->ResourceAssigned = ResourceAssigned;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Spec1_0Entry (
  IN UINTN                        BootloaderParameter,
  OUT EFI_FIRMWARE_VOLUME_HEADER  **DxeFv
  )
{
  EFI_STATUS  Status;
  UINT64      BufferSize;

  BufferSize = InitUplFromBuffer ((UINTN *)BootloaderParameter);
  if (BufferSize == 0) {
    return EFI_NOT_FOUND;
  }

  InitHobList ();
  // Call constructor for all libraries
  ProcessLibraryConstructorList ();
  DEBUG ((DEBUG_INFO, "Entering Universal Payload...\n"));
  DEBUG ((DEBUG_INFO, "sizeof(UINTN) = 0x%x\n", sizeof (UINTN)));
  DEBUG ((DEBUG_INFO, "UPL data BufferSize is 0x%lx\n", BufferSize));
  InitializeFloatingPointUnits ();

  Status = CreateHobsBasedOnMemoryMap1_0 ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = CreateFvHobsBasedOnUplData (DxeFv);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = CreateHobsBasedOnUplData ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
