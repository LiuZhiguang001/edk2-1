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

VOID
TestCaseForMemMap1 (
  VOID
  )
{
  UNIVERSAL_PAYLOAD_MEMORY_MAP  *MemoryMapHob;
  EFI_MEMORY_DESCRIPTOR         *MemoryMap;
  UINTN                         Count;
  Count = 1;

  MemoryMapHob = BuildGuidHob (&gUniversalPayloadMemoryMapGuid, sizeof (UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof (EFI_MEMORY_DESCRIPTOR) * Count);
  MemoryMapHob->Header.Revision = UNIVERSAL_PAYLOAD_MEMORY_MAP_REVISION;
  MemoryMapHob->Header.Length   = (UINT16)(sizeof (UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof (EFI_MEMORY_DESCRIPTOR) * Count);
  MemoryMap = MemoryMapHob->MemoryMap;
  MemoryMapHob->Count = Count;
  MemoryMapHob->DescriptorSize = sizeof (EFI_MEMORY_DESCRIPTOR);
  MemoryMap[0].PhysicalStart = 0x1000;
  MemoryMap[0].NumberOfPages = 1;
}

TEST_CASE  gTestCaseForMemMap[] = {
  { RETURN_SUCCESS, TestCaseForMemMap1, 0 },
};

UINTN
GetTestCaseForMemMapCount (
  VOID
  )
{
  return ARRAY_SIZE (gTestCaseForMemMap);
}
