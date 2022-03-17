/** @file
  Get Universal Payload data library

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __GET_UPL_DATA_LIB_H__
#define __GET_UPL_DATA_LIB_H__

#include  <Base.h>
#include "UplDataDefinition.h"

UINT64
EFIAPI
InitUplFromBuffer (
  IN VOID   *Buffer
  );

RETURN_STATUS
EFIAPI
GetUplUint64 (
  IN  CHAR8   *String,
  OUT UINT64  *Result
  );

RETURN_STATUS
EFIAPI
GetUplUint8 (
  IN  CHAR8   *String,
  OUT UINT8   *Result
  );

RETURN_STATUS
EFIAPI
GetUplBoolean (
  IN  CHAR8     *String,
  OUT BOOLEAN   *Result
  );

RETURN_STATUS
EFIAPI
GetUplAsciiString (
  IN     CHAR8  *String,
  IN OUT UINT8  *Buffer,
  IN OUT UINTN  *Size
  );

RETURN_STATUS
EFIAPI
CborDecoderGetUint8 (
  IN  CHAR8   *String,
  OUT UINT8  *Result,
  IN  VOID    *Map
  );

RETURN_STATUS
EFIAPI
CborDecoderGetBoolean (
  IN  CHAR8    *String,
  OUT BOOLEAN  *Result,
  IN  VOID     *Map
  );

RETURN_STATUS
EFIAPI
CborDecoderGetBinary (
  IN     VOID   *Value,
  OUT    VOID   *Result,
  IN OUT UINTN  *Size,
  IN     VOID   *Map
  );

RETURN_STATUS
EFIAPI
GetUplExtraData (
  IN OUT UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY  *Data,
  IN OUT UINTN                                    *Count,
  IN     UINTN                                    Index
  );

RETURN_STATUS
EFIAPI
GetUplMemoryMap (
  IN OUT EFI_MEMORY_DESCRIPTOR  *Data,
  IN OUT UINTN                  *Count,
  IN     UINTN                  Index
  );

RETURN_STATUS
EFIAPI
GetUplPciRootBridges (
  IN OUT UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE  *Data,
  IN OUT UINTN                                    *Count,
  IN     UINTN                                    Index
  );

RETURN_STATUS
EFIAPI
GetUplResourceData (
  IN OUT UNIVERSAL_PAYLOAD_RESOURCE_DESCRIPTOR         *Data,
  IN OUT UINTN                                    *Count,
  IN     UINTN                                    Index
  );

RETURN_STATUS
EFIAPI
GetUplMemoryAllocationData (
  IN OUT UNIVERSAL_PAYLOAD_MEMORY_ALLOCATION           *Data,
  IN OUT UINTN                                    *Count,
  IN     UINTN                                    Index
  );
#endif
