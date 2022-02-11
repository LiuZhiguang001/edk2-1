#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/MemoryMapLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HobLib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <Pi/PiHob.h>
#include <stdlib.h>
#include "declarations.h"

#undef ASSERT
#define ASSERT assert


VOID
CreateRemainingHobs(
	VOID
)
{
	UINT32 ResourceAttributes = (
		EFI_RESOURCE_ATTRIBUTE_PRESENT |
		EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
		EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
		EFI_RESOURCE_ATTRIBUTE_TESTED |
		EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
		EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
		EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
		);

	BuildResourceDescriptorHob(EFI_RESOURCE_SYSTEM_MEMORY, ResourceAttributes, 0x1000, EFI_PAGES_TO_SIZE(4));
	BuildMemoryAllocationHob(0x2000, EFI_PAGES_TO_SIZE(1), EfiBootServicesData);
	BuildMemoryAllocationHob(0x3000, EFI_PAGES_TO_SIZE(1), EfiBootServicesData);
	BuildResourceDescriptorHob(EFI_RESOURCE_SYSTEM_MEMORY, ResourceAttributes, 0x7000, EFI_PAGES_TO_SIZE(7));
	BuildMemoryAllocationHob(0xb000, EFI_PAGES_TO_SIZE(1), EfiBootServicesData);
	BuildMemoryAllocationHob(0xd000, EFI_PAGES_TO_SIZE(1), EfiBootServicesData);

}
