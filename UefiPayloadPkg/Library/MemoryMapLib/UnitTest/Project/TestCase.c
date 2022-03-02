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

UINT32 ResourceAttributes = (
	EFI_RESOURCE_ATTRIBUTE_PRESENT |
	EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
	EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
	EFI_RESOURCE_ATTRIBUTE_TESTED |
	EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
	EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
	EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
	);

VOID
TestCase1(
  VOID* HobList1,
  VOID* HobList2
)
{
}

//
// Case that memory allocation hob is partly in a resource hob
//
VOID
TestCase2(
  VOID* HobList1,
  VOID* HobList2
)
{

	BuildResourceDescriptorHob(EFI_RESOURCE_SYSTEM_MEMORY, ResourceAttributes, 0x1000, 0x3000);
	BuildMemoryAllocationHob(0x2000, 0x3000, EfiBootServicesData);

}

//
// Case that memory allocation hob doesn't follow rules
//
VOID
TestCase3(
	VOID* HobList1,
	VOID* HobList2
)
{


	BuildResourceDescriptorHob(EFI_RESOURCE_SYSTEM_MEMORY, ResourceAttributes, 0x1000, 0x3000);
	BuildMemoryAllocationHob(0x2100, 0x1000, EfiBootServicesData);

}

//
// Case that resource hob's base is not 1 page align
//
VOID
TestCase4(
	VOID* HobList1,
	VOID* HobList2
)
{
	BuildResourceDescriptorHob(EFI_RESOURCE_SYSTEM_MEMORY, ResourceAttributes, 0x1200, 0x3000);
	BuildMemoryAllocationHob(0x2000, 0x1000, EfiBootServicesData);

}

//
// Case that resource hob's length is not 1 page align
//
VOID
TestCase5(
	VOID* HobList1,
	VOID* HobList2
)
{
	BuildResourceDescriptorHob(EFI_RESOURCE_SYSTEM_MEMORY, ResourceAttributes, 0x1000, 0x3300);
	BuildMemoryAllocationHob(0x2000, 0x1000, EfiBootServicesData);

}

//
// Case that it works
//
VOID
TestCase6(
	VOID* HobList1,
	VOID* HobList2
)
{
	BuildResourceDescriptorHob(EFI_RESOURCE_SYSTEM_MEMORY, ResourceAttributes, (UINTN)HobList1, SIZE_128MB);
}

//
// Case that resource hob's limit + base will larger than MAX_UINT64
//
VOID
TestCase7(
	VOID* HobList1,
	VOID* HobList2
)
{
	BuildResourceDescriptorHob(EFI_RESOURCE_SYSTEM_MEMORY, ResourceAttributes, 0x2000, 0-0x1000);
}

//
// Case that it works
//
VOID
TestCase8(
	VOID* HobList1,
	VOID* HobList2
)
{
	BuildResourceDescriptorHob(EFI_RESOURCE_SYSTEM_MEMORY, ResourceAttributes, 0x1000, 0x3000);
	BuildMemoryAllocationHob(0x2000, 0 - 0x1000, EfiBootServicesData);
}

TEST_CASE  gTestCase[] = {
  { RETURN_UNSUPPORTED, TestCase1, 503 },
	{ RETURN_UNSUPPORTED, TestCase2, 412 },
		{ RETURN_UNSUPPORTED, TestCase3, 664 }, // in hob.c
		{ RETURN_UNSUPPORTED, TestCase4, 359 },
		{ RETURN_UNSUPPORTED, TestCase5, 364 },
		{ RETURN_SUCCESS, TestCase6, 0 },
				{ RETURN_UNSUPPORTED, TestCase7, 370 },
				{ RETURN_UNSUPPORTED, TestCase8, 407 },
};

UINTN
GetTestCaseCount(
  VOID
) {
  return ARRAY_SIZE(gTestCase);
}
