/** @file
  @copyright
  INTEL CONFIDENTIAL
  Copyright 2020 Intel Corporation. <BR>

  The source code contained or described herein and all documents related to the
  source code ("Material") are owned by Intel Corporation or its suppliers or
  licensors. Title to the Material remains with Intel Corporation or its suppliers
  and licensors. The Material may contain trade secrets and proprietary    and
  confidential information of Intel Corporation and its suppliers and licensors,
  and is protected by worldwide copyright and trade secret laws and treaty
  provisions. No part of the Material may be used, copied, reproduced, modified,
  published, uploaded, posted, transmitted, distributed, or disclosed in any way
  without Intel's prior express written permission.

  No license under any patent, copyright, trade secret or other intellectual
  property right is granted to or conferred upon you by disclosure or delivery
  of the Materials, either expressly, by implication, inducement, estoppel or
  otherwise. Any license under such intellectual property rights must be
  express and approved by Intel in writing.

  Unless otherwise agreed by Intel in writing, you may not remove or alter
  this notice or any other notice embedded in Materials by Intel or
  Intel's suppliers or licensors in any way.
**/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#undef NULL // avoid warning due to multiple definition of NULL. One in VC header file, the other in Base.h.
#include <Base.h>
#include <Uefi.h>
#include <Library/MemoryMapLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include "Declarations.h"

extern EFI_GUID  gEfiHobMemoryAllocModuleGuid = {
  0xF8E21975, 0x0899, 0x4F58, { 0xA4, 0xBE, 0x55, 0x25, 0xA9, 0xC6, 0xD7, 0x7A }
};
extern EFI_GUID  gEfiHobMemoryAllocStackGuid = {
  0x4ED4BF27, 0x4092, 0x42E9, { 0x80, 0x7D, 0x52, 0x7B, 0x1D, 0x00, 0xC9, 0xBD }
};
extern EFI_GUID  gUniversalPayloadMemoryMapGuid = {
  0x60ae3012, 0xea8d, 0x4010, { 0xa9, 0x9f, 0xb7, 0xe2, 0xf2, 0x90, 0x82, 0x55 }
};
extern EFI_GUID  gUefiAcpiBoardInfoGuid = {
  0xad3d31b, 0xb3d8, 0x4506, { 0xae, 0x71, 0x2e, 0xf1, 0x10, 0x6, 0xd9, 0xf }
};
extern EFI_GUID  gUniversalPayloadAcpiTableGuid = {
  0x9f9a9506, 0x5597, 0x4515, { 0xba, 0xb6, 0x8b, 0xcd, 0xe7, 0x84, 0xba, 0x87 }
};
extern EFI_GUID  gUniversalPayloadPciRootBridgeInfoGuid = {
  0xec4ebacb, 0x2638, 0x416e, { 0xbe, 0x80, 0xe5, 0xfa, 0x4b, 0x51, 0x19, 0x01 }
};
extern EFI_GUID  gUniversalPayloadSmbios3TableGuid = {
  0x92b7896c, 0x3362, 0x46ce, { 0x99, 0xb3, 0x4f, 0x5e, 0x3c, 0x34, 0xeb, 0x42 }
};
extern EFI_GUID  gUniversalPayloadSmbiosTableGuid = {
  0x590a0d26, 0x06e5, 0x4d20, { 0x8a, 0x82, 0x59, 0xea, 0x1b, 0x34, 0x98, 0x2d }
};
extern EFI_GUID  gUniversalPayloadExtraDataGuid = {
  0x15a5baf6, 0x1c91, 0x467d, { 0x9d, 0xfb, 0x31, 0x9d, 0x17, 0x8d, 0x4b, 0xb4 }
};
extern EFI_GUID  gUniversalPayloadSerialPortInfoGuid = {
  0xaa7e190d, 0xbe21, 0x4409, { 0x8e, 0x67, 0xa2, 0xcd, 0xf, 0x61, 0xe1, 0x70 }
};
extern EFI_GUID  gEfiMemoryTypeInformationGuid = {
  0x4C19049F, 0x4137, 0x4DD3, { 0x9C, 0x10, 0x8B, 0x97, 0xA8, 0x3F, 0xFD, 0xFA }
};
extern EFI_GUID  gEdkiiBootManagerMenuFileGuid = {
  0xdf939333, 0x42fc, 0x4b2a, { 0xa5, 0x9e, 0xbb, 0xae, 0x82, 0x81, 0xfe, 0xef }
};
extern EFI_GUID  gEfiHobMemoryAllocBspStoreGuid = {
  0x564B33CD, 0xC92A, 0x4593, { 0x90, 0xBF, 0x24, 0x73, 0xE4, 0x3C, 0x63, 0x22 }
};

extern TEST_CASE  gTestCase[];

UINTN  ErrorLineNumber;
extern BOOLEAN IgnoreOtherAssert;
UINTN
GetTestCaseCount (
  VOID
  );

VOID  *mHobList;

int
main (
  )
{
  VOID  *MemoryMapHob;

  VOID                  *HobList1;
  VOID                  *HobList2;
  VOID                  *HobList1Backup;
  VOID                  *Range;
  VOID                  *AlignedRange;
  UINTN                 AlignmentMask;
  UINTN                 Alignment;
  UINTN                 Index;
  UINTN                 TestCaseCount;
  EFI_PEI_HOB_POINTERS  Hob1;
  EFI_PEI_HOB_POINTERS  HobBackup;
  RETURN_STATUS         Status;

  Alignment     = SIZE_4KB;
  AlignmentMask = Alignment - 1;
  TestCaseCount = GetTestCaseCount ();
  IgnoreOtherAssert = FALSE;
  printf ("TestCaseCount %d \n", TestCaseCount);
  for (Index = 0; Index < 0 + TestCaseCount; Index++) {
    printf ("Run for %d times\n", Index + 1);
    Range = malloc (SIZE_128MB + SIZE_128MB);
    if (Range == NULL) {
      printf ("Memory is not allocated\n");
      ASSERT (FALSE);
    }

    AlignedRange = (VOID *)(((UINTN)Range + AlignmentMask) & ~AlignmentMask);
    CreateTwoHandoffTableHob (&HobList1, &HobList2, &HobList1Backup, AlignedRange);

    mHobList = HobList1;
    if (Index < TestCaseCount) {
      ErrorLineNumber = gTestCase[Index].LineNumber;
      gTestCase[Index].TestCaseFunction (HobList1, HobList2);
    } else {
      CreateRemainingHobs (HobList1, HobList2, 0, Get64BitRandomNumber ()); // Create end which is above hoblist2.limit
    }

    CopyMem (HobList1Backup, HobList1, SIZE_64MB);
    Status = BuildMemoryMap ();
    if (Index < TestCaseCount) {
      IgnoreOtherAssert = FALSE;
      
      assert (Status == gTestCase[Index].ExpectedStatus);
      if (Status != RETURN_SUCCESS) {
        assert(ErrorLineNumber == 0);
      }
    } else {
      PrintHob (HobList1);
      MemoryMapHob = GetNextGuidHob (&gUniversalPayloadMemoryMapGuid, mHobList);
      mHobList     = HobList2;

      CreateHobsBasedOnMemoryMap ((UNIVERSAL_PAYLOAD_MEMORY_MAP *)GET_GUID_HOB_DATA (MemoryMapHob));

      Hob1.Raw = (UINT8 *)HobList1;
      HobBackup.Raw = (UINT8 *)HobList1Backup;

      while (TRUE) {
        if (HobBackup.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
          Hob1.Header->HobType = EFI_HOB_TYPE_RESOURCE_DESCRIPTOR;
        } else if (HobBackup.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
          Hob1.Header->HobType = EFI_HOB_TYPE_MEMORY_ALLOCATION;
        } else if (HobBackup.Header->HobType == EFI_HOB_TYPE_END_OF_HOB_LIST) {
          break;
        }

        Hob1.Raw = GET_NEXT_HOB (Hob1);
        HobBackup.Raw = GET_NEXT_HOB (HobBackup);
      }

      // PrintHob (HobList1);
      // PrintHob (HobList2);
      VerifyHob (HobList1, HobList2);
    }

    if (Range != NULL) {
      free (Range);
    }
  }

  return 0;
}
