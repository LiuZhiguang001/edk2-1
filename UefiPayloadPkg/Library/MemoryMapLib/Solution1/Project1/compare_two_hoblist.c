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


#define MAX_SIZE 30000

typedef struct {
  BOOLEAN IsPhysicalMem;
  UINT32  ResType;
  UINT32  MemType;
  UINT64  Attribute;
  UINTN                   ResourceHobIndex;
  UINTN                   MemHobIndex;
  UINTN                   HobListIndex;
}INFO_STRUCT;


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
GetInfoOfPointFromHob(
  IN CONST UINT64 Point,
  IN CONST VOID* HobList,
  OUT      INFO_STRUCT  *Info,
  IN  UINTN  HoblistIndex
)
{
  EFI_PEI_HOB_POINTERS    Hob;
  UINTN                   Index;
  BOOLEAN                 ContainInResourceHob;
  BOOLEAN                 ContainInMemHob;
  BOOLEAN                 ContainInHobList;

  UINTN                   ResourceHobIndex;
  UINTN                   MemHobIndex;
  UINTN                   HobListIndex;

  ResourceHobIndex = MAX_UINT32;
  MemHobIndex = MAX_UINT32;
  HobListIndex = MAX_UINT32;
  Info->Attribute = MAX_UINT64;
  Info->IsPhysicalMem = FALSE;
  Info->MemType = MAX_UINT32;
  Info->ResType = MAX_UINT32;

  Hob.Raw = (UINT8*)HobList;
  Index = 0;
  ContainInResourceHob = FALSE;
  ContainInMemHob = FALSE;
  ContainInHobList = FALSE;

  while (!END_OF_HOB_LIST(Hob)) {

    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if (Point >= Hob.ResourceDescriptor->PhysicalStart
        && Point < (Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength)) {

        if (ContainInResourceHob) {
          printf("Error: Point(%llx) already exist in more than one resource hob, hob[%d] and hob[%d] in hoblist%d\n", Point, ResourceHobIndex, Index, HoblistIndex);
        }

        Info->ResType = Hob.ResourceDescriptor->ResourceType;
        Info->Attribute = Hob.ResourceDescriptor->ResourceAttribute;
        ResourceHobIndex = Index;
        Info->IsPhysicalMem = TRUE;
        ContainInResourceHob = TRUE;
      }
    }
    if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      if (Point >= Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress
        && Point < (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress + Hob.MemoryAllocation->AllocDescriptor.MemoryLength)) {

        if (ContainInMemHob) {
          printf("Error: Point(%llx) already exist in more than one memory hob, hob[%d] and hob[%d] in hoblist%d\n", Point, MemHobIndex, Index, HoblistIndex);
        }

        Info->MemType = Hob.MemoryAllocation->AllocDescriptor.MemoryType;
        MemHobIndex = Index;
      }
    }

    if (Hob.Header->HobType == EFI_HOB_TYPE_HANDOFF) {
      if (Point >= (UINT64) (UINTN) Hob.Raw
        && Point < (((Hob.HandoffInformationTable->EfiFreeMemoryBottom) + SIZE_4KB - 1) & (~(SIZE_4KB - 1)))) {

        if (ContainInHobList) {
          printf("Error: Point(%llx) already exist in more than one resource hob, hob[%d] and hob[%d] in ", Point, ResourceHobIndex, Index);
        }

        ResourceHobIndex = Index;
        ContainInHobList = TRUE;
      }
    }
    Hob.Raw = GET_NEXT_HOB(Hob);
    Index++;
  }

  if (ContainInHobList) {
    if (ContainInMemHob) {
      ASSERT(Info->MemType == EfiBootServicesData);
    }
    else {
      Info->MemType = EfiBootServicesData;
    }
  }
  if (ContainInHobList || ContainInMemHob) {
    ASSERT(ContainInResourceHob);
  }

  Info->ResourceHobIndex = ResourceHobIndex;
  Info->MemHobIndex = MemHobIndex;
  Info->HobListIndex = HobListIndex;
}

UINTN
GetKeyPointFromHob(
  IN CONST VOID* Start,
  IN UINT64* Array
)
{
  EFI_PEI_HOB_POINTERS    Hob;
  Hob.Raw = (UINT8*)Start;
  UINTN i = 0;
  UINTN Count = 0;
  while (!END_OF_HOB_LIST(Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR || Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      Count++;
    }
    Hob.Raw = GET_NEXT_HOB(Hob);
  }

  Hob.Raw = (UINT8*)Start;
  while (!END_OF_HOB_LIST(Hob) && i <= Count * 2) {
    if (Hob.Header->HobType != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR && Hob.Header->HobType != EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      Hob.Raw = GET_NEXT_HOB(Hob);
      continue;
    }
    else if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      Array[i++] = Hob.ResourceDescriptor->ResourceLength + 1;
      Array[i++] = (Hob.ResourceDescriptor->PhysicalStart + (Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength)) / 2;
    }
    else if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      Array[i++] = Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress + 1;
      Array[i++] = (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress + (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress + Hob.MemoryAllocation->AllocDescriptor.MemoryLength)) / 2;
    }
    Hob.Raw = GET_NEXT_HOB(Hob);
  }
  return i;
}

VOID
VerifyHob(
  IN CONST VOID* oldHobStart,
  IN CONST VOID* newHobStart
)
{
  UINT64 OldArray[MAX_SIZE];
  UINT64 NewArray[MAX_SIZE];
  UINT64 AllArray[MAX_SIZE] = { 0 };
  UINTN oldcount, newcount, allcount;
  INFO_STRUCT Info1, Info2;

  oldcount = GetKeyPointFromHob(oldHobStart, OldArray);
  newcount = GetKeyPointFromHob(newHobStart, NewArray);
  allcount = oldcount + newcount;

  for (int k = 0; k < oldcount; k++) {
    AllArray[k] = OldArray[k];
  }
  for (int l = oldcount, m = 0; m < newcount; l++, m++) {
    AllArray[l] = NewArray[m];
  }

  for (int j = 0; j < allcount; j++) {
    GetInfoOfPointFromHob(AllArray[j], oldHobStart, &Info1,1);
     GetInfoOfPointFromHob(AllArray[j], newHobStart, &Info2, 2);

     if (Info1.Attribute == Info2.Attribute && Info1.MemType == Info2.MemType && Info1.ResType == Info2.ResType && Info1.IsPhysicalMem == Info2.IsPhysicalMem) {
       ;
     }
     else {
       if (Info1.Attribute == Info2.Attribute && Info1.MemType == Info2.MemType && Info1.IsPhysicalMem == Info2.IsPhysicalMem && Info2.MemType == EfiReservedMemoryType && Info2.ResType == EFI_RESOURCE_MEMORY_RESERVED) {
         // case that in hoblist1, it is in non-reserved resource hob but reserved memory hob
         ;
       } else if(Info1.Attribute == Info2.Attribute && Info1.ResType == Info2.ResType && Info1.IsPhysicalMem == Info2.IsPhysicalMem && Info2.MemType == EfiReservedMemoryType && Info2.ResType == EFI_RESOURCE_MEMORY_RESERVED)
       {
         // case that in hoblist1, it is in reserved resource hob but no in reserved memory hob
         ;
       } else if (Info1.Attribute == Info2.Attribute && Info1.ResType == Info2.ResType && Info1.IsPhysicalMem == Info2.IsPhysicalMem && Info1.MemType == EfiConventionalMemory && Info2.ResType == EFI_RESOURCE_SYSTEM_MEMORY)
       {
         // case that in hoblist1, there is EfiConventionalMemory memory alloction hob
         ;
       }
       else {
         printf("The point which has issue is %llx\n", AllArray[j]);
         printf("Index in hob 1\n");

         printf("Index of ResourceHobIndex is %d\n", Info1.ResourceHobIndex);
         printf("Index of MemHobIndex is %d\n",      Info1.MemHobIndex);
         printf("Index of HobListIndex is %d\n",     Info1.HobListIndex);

                  printf("Index in hob 2\n");

         printf("Index of ResourceHobIndex is %d\n", Info2.ResourceHobIndex);
         printf("Index of MemHobIndex is %d\n",      Info2.MemHobIndex);
         printf("Index of HobListIndex is %d\n",     Info2.HobListIndex);
         ASSERT(FALSE);
       }

     }
  }

}