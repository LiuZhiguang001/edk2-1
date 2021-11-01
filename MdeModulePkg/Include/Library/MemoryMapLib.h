/** @file
  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Uefi/UefiSpec.h>
#include <UniversalPayload/MemoryMap.h>

/**
  Create a Guid Hob containing the memory map descriptor table.
  After calling the function, PEI should not do any memory allocation operation.

  @retval EFI_SUCCESS           The Memory Map is created successfully.
**/
EFI_STATUS
EFIAPI
BuildMemMap (
  VOID
  );
