## @file
#
# Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
[Defines]
  PLATFORM_NAME                       = MemoryMapHostTest
  PLATFORM_GUID                       = 94C83DC1-63D9-4D9C-A0E1-03C0F278A611
  PLATFORM_VERSION                    = 0.01
  DSC_SPECIFICATION                   = 0x00010019
  OUTPUT_DIRECTORY                    = Build/MemoryMapHostTest
  SUPPORTED_ARCHITECTURES             = IA32|X64
  BUILD_TARGETS                       = NOOPT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses.common.HOST_APPLICATION]
  UnitTestLib         |UnitTestFrameworkPkg/Library/UnitTestLib/UnitTestLibCmocka.inf
  DebugLib            |UnitTestFrameworkPkg/Library/Posix/DebugLibPosix/DebugLibPosix.inf
  MemoryAllocationLib |UnitTestFrameworkPkg/Library/Posix/MemoryAllocationLibPosix/MemoryAllocationLibPosix.inf

[Components.common.HOST_APPLICATION]
  UefiPayloadPkg/Library/MemoryMapLib/MemoryMapUnitTest/Host/MemoryMapUnitTest.inf {
    <LibraryClasses>
      MemoryMapLib|UefiPayloadPkg/Library/MemoryMapLib/MemoryMapLib.inf
      HobLib|UefiPayloadPkg/Library/PayloadEntryHobLib/HobLib.inf
  }
