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

  MemoryMapCount = 0;

  GetUplMemoryMap (&MemoryMap, &MemoryMapCount, 0);
  FindIndex = MemoryMapCount;
  DEBUG ((DEBUG_INFO, "MemoryMapCount%x\n", MemoryMapCount));
  for (Index = 0; Index < MemoryMapCount; Index++) {
    DEBUG ((DEBUG_INFO, "Index%x\n", Index));
    MemoryMapNumber = 1;
    GetUplMemoryMap (&MemoryMap, &MemoryMapNumber, Index);
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
  GetUplMemoryMap (&MemoryMap, &MemoryMapNumber, FindIndex);

  MemoryTop        = EFI_PAGES_TO_SIZE (MemoryMap.NumberOfPages) + MemoryMap.PhysicalStart;
  FreeMemoryTop    = MemoryTop;
  MemoryBottom     = MemoryTop - FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);
  FreeMemoryBottom = MemoryBottom;
  DEBUG ((DEBUG_INFO, "MemoryTop%lx\n", MemoryTop));
  HobConstructor ((VOID *)(UINTN)MemoryBottom, (VOID *)(UINTN)MemoryTop, (VOID *)(UINTN)FreeMemoryBottom, (VOID *)(UINTN)FreeMemoryTop);

  UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO  *Serial;

  Serial = BuildGuidHob (&gUniversalPayloadSerialPortInfoGuid, sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO));

  UINT64  TempData;

  GetUplUint64 ("SerialPortBaudRate", &TempData);

  Serial->BaudRate = (UINT32)TempData;
  GetUplUint64 ("SerialPortRegisterBase", &TempData);
  Serial->RegisterBase = (UINT64)TempData;

  TempData = 0;
  GetUplUint8 ("SerialPortRegisterStride", (UINT8*)&TempData);
  Serial->RegisterStride  = (UINT8)TempData;
  Serial->Header.Revision = UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO_REVISION;
  Serial->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO);
  GetUplUint64 ("SerialPortUseMmio", &TempData);
  Serial->UseMmio = (BOOLEAN)TempData;

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

RETURN_STATUS
CreateHobsBasedOnMemoryMap1_0 (
  );

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
    DEBUG ((DEBUG_INFO, "BufferSize%lx\n", BufferSize));
  InitializeFloatingPointUnits ();
  CreateHobsBasedOnMemoryMap1_0 ();
  UINT64  TempData;

  Status = GetUplUint64 ("HobList", &TempData);
  DEBUG ((DEBUG_INFO, "Status %lx\n", Status));

  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = (VOID *)(UINTN)TempData;

  PrintHob(Hob.Raw);
  //
  // Create an empty FvHob for the DXE FV that contains DXE core.
  //
  BuildFvHob ((EFI_PHYSICAL_ADDRESS)0, 0);
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


  EFI_HOB_FIRMWARE_VOLUME       *FvHob;

  ACPI_BOARD_INFO               *AcpiBoardInfo;

  //
  // Get DXE FV location
  //
  UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY  *ExtraData;
  UINTN                                    Count;
  UINTN                                    Index;

  Count = 0;
  GetUplExtraData (NULL, &Count, 0);
  ExtraData = AllocatePool (Count * sizeof (UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY));
   
  GetUplExtraData (ExtraData, &Count, 0);
  DEBUG ((DEBUG_INFO, "GetUplExtraData... %d\n", Count));
  for (Index = 0; Index < Count; Index++) {
    DEBUG ((DEBUG_INFO, "GetUplExtraData... %a\n", (UINT8*)ExtraData[Index].Identifier));
    
    if (AsciiStrCmp (ExtraData[Index].Identifier, "uefi_fv") == 0) {
      *DxeFv = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)ExtraData[Index].Base;
      ASSERT ((*DxeFv)->FvLength == ExtraData[Index].Size);
      break;
    }
  }

  if (Index == Count) {
    DEBUG ((DEBUG_INFO, "Index == Count... \n" ));
    return EFI_NOT_FOUND;
  }


  //
  // Create guid hob for acpi board information
  //
  UINT64 Rsdp;
  GetUplUint64("AcpiTableRsdp", &Rsdp);
  AcpiBoardInfo = BuildHobFromAcpi (Rsdp);
  ASSERT (AcpiBoardInfo != NULL);



  //
  // Build CPU Hob
  //
  UINT8  SizeOfMemorySpace;
  UINT8  SizeOfIoSpace;
  GetUplUint8 ("MemorySpace", &SizeOfMemorySpace);
  GetUplUint8 ("IoSpace", &SizeOfIoSpace);
  BuildCpuHob(SizeOfMemorySpace, SizeOfIoSpace);

  //
  // Build gUniversalPayloadPciRootBridgeInfoGuid Hob
  //
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE   * RootBridge;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES  *PciRootBridgeInfo;
  BOOLEAN                             ResourceAssigned;
  Count = 0;
  GetUplPciRootBridges (NULL, &Count, 0);
  RootBridge = AllocatePool (Count * sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE));
   
  GetUplPciRootBridges (RootBridge, &Count, 0);
  DEBUG ((DEBUG_INFO, "GetUplPciRootBridges... %d\n", Count));

  PciRootBridgeInfo = BuildGuidHob (&gUniversalPayloadPciRootBridgeInfoGuid, sizeof (PciRootBridgeInfo) + sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE));
  CopyMem(PciRootBridgeInfo->RootBridge, RootBridge, sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE));
  PciRootBridgeInfo->Count = (UINT8)Count;
  PciRootBridgeInfo->Header.Length = Count * sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE) + sizeof(UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES);
  PciRootBridgeInfo->Header.Revision =  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_REVISION;

  GetUplBoolean ("RootBridgeResourceAssigned", &ResourceAssigned);
  PciRootBridgeInfo->ResourceAssigned = ResourceAssigned;

  //
  // Update DXE FV information to first fv hob in the hob list, which
  // is the empty FvHob created before.
  //
  FvHob              = GetFirstHob (EFI_HOB_TYPE_FV);
  FvHob->BaseAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)*DxeFv;
  FvHob->Length      = (*DxeFv)->FvLength;
  Status = GetUplUint64 ("MemoryMap", &Rsdp);
  DEBUG ((DEBUG_INFO, "GetUplUint64 (MemorySpace, &ResourceAssigned);... %d\n", Status));
  Status = GetUplUint64 ("MemorySpace1", &Rsdp);
  DEBUG ((DEBUG_INFO, "GetUplUint64 (MemorySpace, &ResourceAssigned);... %d\n", Status));
  return EFI_SUCCESS;
}
