/** @file
  Set Universal Payload data library

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SET_UPL_DATA_LIB_H__
#define __SET_UPL_DATA_LIB_H__

#include  <Base.h>
#include "UplDataDefinition.h"

RETURN_STATUS
EFIAPI
SetUplUint64 (
  IN CHAR8   *Key,
  IN UINT64  Value
  );

RETURN_STATUS
EFIAPI
SetUplUint8 (
  IN CHAR8  *Key,
  IN UINT8  Value
  );

RETURN_STATUS
EFIAPI
SetUplBinary (
  IN CHAR8  *Key,
  IN VOID   *Value,
  IN UINTN  Size
  );

RETURN_STATUS
EFIAPI
SetUplAsciiString (
  IN CHAR8  *Key,
  IN UINT8  *Value,
  IN UINTN  Size
  );

RETURN_STATUS
EFIAPI
SetUplBoolean (
  IN CHAR8    *Key,
  IN BOOLEAN  Value
  );

RETURN_STATUS
EFIAPI
LockUplAndGetBuffer (
  OUT VOID   **Buff,
  OUT UINTN  *Size
  );

RETURN_STATUS
EFIAPI
SetUplExtraData (
  IN UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY  *Data,
  IN UINTN                                    Count
  );

RETURN_STATUS
EFIAPI
SetUplMemoryMap (
  IN EFI_MEMORY_DESCRIPTOR  *Data,
  IN UINTN                  Count
  );

RETURN_STATUS
EFIAPI
SetUplPciRootBridges (
  IN UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE  *Data,
  IN UINTN                                    Count
  );

RETURN_STATUS
EFIAPI
SetUplResourceData (
  IN UNIVERSAL_PAYLOAD_RESOURCE_DESCRIPTOR         *Data,
  IN UINTN                                    Count
  );

RETURN_STATUS
EFIAPI
SetUplMemoryAllocationData (
  IN UNIVERSAL_PAYLOAD_MEMORY_ALLOCATION           *Data,
  IN UINTN                                    Count
  );
#endif
