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

typedef
UINTN
(*HOB_TYPE_PRINT_HANDLER) (
    IN  VOID* FirstHob,
    IN  VOID* SecondHob
    );

typedef struct {
    UINT16                    Type;
    HOB_TYPE_PRINT_HANDLER    PrintHandler;
} HOB_TYPES_TABLE;

UINTN
ResDescCompareFunction(
    IN VOID* FirstHob,
    IN VOID* SecondHob
)
{
    EFI_HOB_RESOURCE_DESCRIPTOR* First;
    EFI_HOB_RESOURCE_DESCRIPTOR* Second;

    First = (EFI_HOB_RESOURCE_DESCRIPTOR*)FirstHob;
    Second = (EFI_HOB_RESOURCE_DESCRIPTOR*)SecondHob;

    if ((First->ResourceType == Second->ResourceType)
        && (First->ResourceAttribute == Second->ResourceAttribute)
        && (First->ResourceLength == Second->ResourceLength)
        && (First->PhysicalStart == Second->PhysicalStart)) {
        return 0;
    }
    else {
        return 1;
    }
}

UINTN
MemAllocCompareFunction(
    IN VOID* FirstHob,
    IN VOID* SecondHob
)
{
    EFI_HOB_MEMORY_ALLOCATION* First;
    EFI_HOB_MEMORY_ALLOCATION* Second;

    First = (EFI_HOB_MEMORY_ALLOCATION*)FirstHob;
    Second = (EFI_HOB_MEMORY_ALLOCATION*)SecondHob;

    if (CompareGuid(&First->AllocDescriptor.Name, &Second->AllocDescriptor.Name)
        && (First->AllocDescriptor.MemoryBaseAddress == Second->AllocDescriptor.MemoryBaseAddress)
        && (First->AllocDescriptor.MemoryLength == Second->AllocDescriptor.MemoryLength)
        && (First->AllocDescriptor.MemoryType == Second->AllocDescriptor.MemoryType)) {
        return 0;
    }
    else {
        return 1;
    }
}

HOB_TYPES_TABLE mHobTypes[] = {
  {EFI_HOB_TYPE_MEMORY_ALLOCATION,   MemAllocCompareFunction},
  {EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, ResDescCompareFunction},
};



VOID
VerifyHob(
    IN CONST VOID* oldHobStart,
    IN CONST VOID* newHobStart
)
{
  ;
}
