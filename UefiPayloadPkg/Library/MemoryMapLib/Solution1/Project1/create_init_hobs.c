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
#include <stdlib.h>
#include <time.h>

#define  SPECIAL_ARRAY_MAX 50
#define  N_ARRAY_MAX 50
#define  WHOLE_BASE 0
#define  WHOLE_LIMIT 0x0000100000000000ULL   //SIZE_16TB


extern VOID* mHobList;

#define  BIT16  0x00010000
#define  BIT32  0x0000000100000000ULL
#define  MAX_UINT64  ((UINT64)0xFFFFFFFFFFFFFFFFULL)

UINT64 TempArray[N_ARRAY_MAX]; // for sort

UINT16
Get16BitRandomNumber(
	VOID
)
{
	UINT16 retval;
	retval = rand() * 2 + rand() % 2;


	return retval;
}

UINT32
Get32BitRandomNumber(
	VOID
)
{
	UINT32 retval;
	retval = ((UINT32)Get16BitRandomNumber()) * BIT16 + (UINT32)Get16BitRandomNumber();

	return retval;
}


UINT64
Get64BitRandomNumber(
	VOID
)
{
	UINT64 retval;
	retval = ((UINT64)Get32BitRandomNumber()) * BIT32 + (UINT64)Get32BitRandomNumber();

	return retval;

}

UINT64
GetRandomNumber(
	UINT64 Base,
	UINT64 Limit,
	UINT64 AlignSize,
	UINT64 *PreferPointArray,
	UINTN  PreferPointCount
)
{
	Base  = (Base + AlignSize - 1) & (~(AlignSize - 1));
	Limit = Limit & (~(AlignSize - 1));

	if (Base == Limit) {
		return Base;
	}

	if (Base > Limit) {
		return MAX_UINT64;
	}

	UINT64 SpecialArray[SPECIAL_ARRAY_MAX];
	UINTN SpecialCount;
	UINTN  Index;


	Index = 0;
	SpecialCount = 0;

	SpecialArray[Index] = Base;
	Index++;
	SpecialCount++;

	SpecialArray[Index] = Limit;
	Index++;
	SpecialCount++;

	
	SpecialArray[Index] = Base + AlignSize;
	Index++;
	SpecialCount++;

	SpecialArray[Index] = Limit - AlignSize;
	SpecialCount++;

	for (Index = 0; Index < PreferPointCount; Index++) {
		if ((PreferPointArray[Index] >= Base) &&
				(PreferPointArray[Index] <= Limit) &&
				(PreferPointArray[Index] == ((PreferPointArray[Index] + 1) & (~(AlignSize - 1))))) {
			SpecialArray[SpecialCount] = PreferPointArray[Index];
			SpecialCount++;
		}
	}
	
	if (Get16BitRandomNumber() % 100 < 20) {
		// special case
		Index = Get32BitRandomNumber() % SpecialCount;
		return SpecialArray[Index];
	}
	else {
		return Base + (Get64BitRandomNumber() % (Limit / AlignSize - Base / AlignSize)) * AlignSize;
	}
}

UINTN
GetNRandomNumber(
	UINT64 Base,
	UINT64 Limit,
	UINT64 AlignSize,
	UINT64* PreferPointArray,
	UINTN  PreferPointCount,
	UINTN  N,
	UINT64** RandomList
) {
	UINTN AvailibleRandom1 = 0;
	UINTN AvailibleRandom2 = 0;
	UINT64* InitList = *RandomList;
	
	if (N == 0) {
		return 0;
	}
	UINT64 x = GetRandomNumber(Base, Limit, AlignSize, PreferPointArray, PreferPointCount);
	if (x == MAX_UINT64) {
		return 0;
	}
	**RandomList = x;
	(*RandomList)++;


	AvailibleRandom1 = GetNRandomNumber(
		(x > (Base + Limit) / 2) ? x + 1 : Base,
		(x > (Base + Limit) / 2) ? Limit : x - 1,
		AlignSize,
		PreferPointArray,
		PreferPointCount,
		(N - 1) / 2,
		RandomList
	);

	AvailibleRandom2 = GetNRandomNumber(
		(x > (Base + Limit) / 2) ? Base : x + 1,
		(x > (Base + Limit) / 2) ? x - 1 : Limit,
		AlignSize,
		PreferPointArray,
		PreferPointCount,
		N - 1 - AvailibleRandom1,
		RandomList
	);
	CopyMem(TempArray, InitList, (AvailibleRandom1 + AvailibleRandom2 + 1) * sizeof(UINT64));
	if (x > (Base + Limit) / 2) {
		// x, larger than x, smaller than x
		CopyMem(InitList, TempArray + AvailibleRandom1 + 1, AvailibleRandom2 * sizeof(UINT64));
		CopyMem((InitList) + AvailibleRandom2, TempArray , 1 * sizeof(UINT64));
		CopyMem((InitList) + AvailibleRandom2 + 1, TempArray + 1, AvailibleRandom1 * sizeof(UINT64));
	}
	else {
		// x, smaller than x, larger than x
		CopyMem(InitList, TempArray + 1, AvailibleRandom1 * sizeof(UINT64));
		CopyMem((InitList) + AvailibleRandom1, TempArray, 1 * sizeof(UINT64));
		CopyMem((InitList) + AvailibleRandom1 + 1, TempArray + 1 + AvailibleRandom1, AvailibleRandom2 * sizeof(UINT64));
	}

	return AvailibleRandom1 + AvailibleRandom2 + 1;

}


VOID
EFIAPI
CreateTwoHandoffTableHob(
	VOID** HobList1,
	VOID** HobList2,
	VOID*  Range
)
{
	VOID* MemBottom;
	VOID* MemTop;
	VOID* FreeMemBottom;
	VOID* FreeMemTop;
	UINT64 Length = SIZE_32MB;
	MemBottom = Range;
	MemTop = (VOID*)((UINTN)MemBottom + Length);
	FreeMemBottom = (VOID*)((UINTN)MemBottom);
	FreeMemTop = (VOID*)((UINTN)MemTop);

	*HobList1 = HobConstructor(MemBottom, MemTop, FreeMemBottom, FreeMemTop);

	MemBottom = (VOID*)((UINTN)MemBottom + SIZE_64MB);
	MemTop = (VOID*)((UINTN)MemBottom + Length);
	FreeMemBottom = (VOID*)((UINTN)MemBottom);
	FreeMemTop = (VOID*)((UINTN)MemTop);

	*HobList2 = HobConstructor(MemBottom, MemTop, FreeMemBottom, FreeMemTop);

}


UINT64 AllResourceAttribute[] = {
		EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE,
		EFI_RESOURCE_ATTRIBUTE_UNCACHED_EXPORTED,
		EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE,
		EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE,
		EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE,
		EFI_RESOURCE_ATTRIBUTE_READ_PROTECTABLE,
		EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTABLE,
		EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTABLE,
		EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTABLE,
		EFI_RESOURCE_ATTRIBUTE_PRESENT,
		EFI_RESOURCE_ATTRIBUTE_INITIALIZED,
		EFI_RESOURCE_ATTRIBUTE_TESTED,
		EFI_RESOURCE_ATTRIBUTE_PERSISTABLE,
		EFI_RESOURCE_ATTRIBUTE_MORE_RELIABLE
};

UINT64 OffenResourceAttribute[5];

UINT64
RandomResourceAttribute() {
	UINT64 ResourceAttribute = 0;
	UINT64 Random = Get64BitRandomNumber();
	UINTN Index;
	for (Index = 0; Index < ARRAY_SIZE(AllResourceAttribute); Index++) {
		ResourceAttribute += (Random & 1 ? AllResourceAttribute[Index] : 0);
		Random = Random >> 1;
	}
	return ResourceAttribute;
}

UINT64
RandomResourceAttribute_EX() {
	if (rand() % 2 == 0) {
		return RandomResourceAttribute();
	}
	else {
		return OffenResourceAttribute[rand() % ARRAY_SIZE(OffenResourceAttribute)];
	}
}


UINT64
CreateRandomMemoryHob(
	UINT64 Base,
	UINT64 Limit,
	UINT64 AlignSize,
	VOID* HobList1,
	VOID* HobList2
)
{

	BOOLEAN ContainHobList1 = TRUE;
	BOOLEAN ContainHobList2 = TRUE;
	UINTN   Random;
	EFI_MEMORY_TYPE type;
	if ((Limit <= ((EFI_HOB_HANDOFF_INFO_TABLE*)HobList1)->EfiMemoryBottom) || (Base >= ((EFI_HOB_HANDOFF_INFO_TABLE*)HobList1)->EfiMemoryTop)) ContainHobList1 = FALSE;
	if ((Limit <= ((EFI_HOB_HANDOFF_INFO_TABLE*)HobList2)->EfiMemoryBottom) || (Base >= ((EFI_HOB_HANDOFF_INFO_TABLE*)HobList2)->EfiMemoryTop)) ContainHobList2 = FALSE;

	if (ContainHobList1 && !ContainHobList2) {
		return 0;
	}
	Random = rand() % 5;
	if (ContainHobList1 && ContainHobList2) {
		Base = GetRandomNumber(((EFI_HOB_HANDOFF_INFO_TABLE*)HobList1)->EfiMemoryTop, ((EFI_HOB_HANDOFF_INFO_TABLE*)HobList2)->EfiMemoryBottom, AlignSize, NULL, 0);
		Random++;
	}

	if (ContainHobList2) {
		type = EfiBootServicesData;
	}	else {
		Random = rand() % 5;
		switch (Random)
		{
		case 0:
			return;
		case 1:
			type = EfiBootServicesData;
			break;
		case 2:
			type = EfiReservedMemoryType;
			break;
		default:
			type = rand() % 14;
			break;
		}
	}
	BuildMemoryAllocationHob(Base, (Limit - Base), type);
}

UINT64
CreateRandomResourceHob(
	UINT64 Base,
	UINT64 Limit,
	UINT64 AlignSize,
	VOID* HobList1,
	VOID* HobList2,
	UINTN   MemoryPointArrayMaxCount
) 
{
	
	BOOLEAN ContainHobList1 = TRUE;
	BOOLEAN ContainHobList2 = TRUE;
	EFI_RESOURCE_TYPE ResourceType;
	UINT64 MemoryPointArray[N_ARRAY_MAX];
	//UINTN   MemoryPointArrayMaxCount = 2; // must no larger than ResourcePointArray, and is an even number
	UINT64* MemoryPointArrayPointer;
	UINTN   MemoryPointArrayCount;
	UINTN   Index = 0;
	MemoryPointArrayPointer = MemoryPointArray;

	if ((Limit <= ((EFI_HOB_HANDOFF_INFO_TABLE*)HobList1)->EfiMemoryBottom) || (Base >= ((EFI_HOB_HANDOFF_INFO_TABLE*)HobList1)->EfiMemoryTop)) ContainHobList1 = FALSE;
	if ((Limit <= ((EFI_HOB_HANDOFF_INFO_TABLE*)HobList2)->EfiMemoryBottom) || (Base >= ((EFI_HOB_HANDOFF_INFO_TABLE*)HobList2)->EfiMemoryTop)) ContainHobList2 = FALSE;

	if (ContainHobList1 || ContainHobList2) {
		ResourceType = EFI_RESOURCE_SYSTEM_MEMORY;
	}
	else {
		if (rand() % 2 == 0) {
			ResourceType = EFI_RESOURCE_SYSTEM_MEMORY;
		}
		else {
			ResourceType = EFI_RESOURCE_MEMORY_RESERVED;
		}
	}
	BuildResourceDescriptorHob(ResourceType, RandomResourceAttribute_EX(), Base, (Limit - Base));
	if (ContainHobList2) {
		// start[ ... 2n+1 ponit... HobList2 ... 2k+1 point...]end
		MemoryPointArrayCount = GetNRandomNumber(Base, ((EFI_HOB_HANDOFF_INFO_TABLE*)HobList2)->EfiMemoryBottom, AlignSize, NULL, 0, (rand()% (MemoryPointArrayMaxCount - 1)) / 2 * 2 + 1, &MemoryPointArrayPointer);
		MemoryPointArrayCount += GetNRandomNumber(((EFI_HOB_HANDOFF_INFO_TABLE*)HobList2)->EfiMemoryTop, Limit, AlignSize, NULL, 0, (rand() % (MemoryPointArrayMaxCount - MemoryPointArrayCount)) / 2 * 2 + 1, &MemoryPointArrayPointer);
	}
	else {
		MemoryPointArrayCount = GetNRandomNumber(Base, Limit, AlignSize, NULL, 0, (rand() % (MemoryPointArrayMaxCount)) / 2 * 2, &MemoryPointArrayPointer);
	}

	while (MemoryPointArrayCount != 0) {
		CreateRandomMemoryHob(
			MemoryPointArray[Index],
			MemoryPointArray[Index + 1],
			AlignSize,
			HobList1,
			HobList2
		);
		Index += 2;
		MemoryPointArrayCount -= 2;
	}
	

}


VOID
CreateRemainingHobs(
	VOID* HobList1,
	VOID* HobList2
)
{
	/*
	int i, n;
	n = 50;
	time_t t;
	srand((unsigned)time(&t));

	UINT64 SpecialArray[SPECIAL_ARRAY_MAX];
	UINT64 NRandom[SPECIAL_ARRAY_MAX];
	SpecialArray[0] = 0x3e0;
	SpecialArray[1] = 0x2e0;

	for (i = 0; i < n; i++) {
		NRandom[i] = 0;
		;// printf("%#llx\n", GetRandomNumber(0x100, 0x500, 0x10, SpecialArray, 2));
	}
	UINT64* NRandomList;
	NRandomList = NRandom;

	n = GetNRandomNumber(0x100, 0x10000, 0x10, SpecialArray, 2, 50, &NRandomList);

	for (i = 0; i < n; i++) {
		
		 printf("%d:  %#llx\n", i, NRandom[i]);
	}


	//exit(0);
	

	for (i = 0; i < ARRAY_SIZE(OffenResourceAttribute); i++) {
		OffenResourceAttribute[i] = RandomResourceAttribute();
		printf("%d:  %#llx\n", i, OffenResourceAttribute[i]);
	}
	//exit(0);
	*/

	// start[ ... 2n+1 ponit...HobList1..2n point... HobList2 ... 2k+1 point...]end
	UINT64 SpecialArray[SPECIAL_ARRAY_MAX];
	SpecialArray[0] = 0x0000000100000000ULL;
	UINTN   SpecialArrayCount = 1;

	UINTN   ResourcePointArrayMaxCount = 2; // must no larger than ResourcePointArray, and is an even number
	UINT64  ResourcePointArray[N_ARRAY_MAX];
	UINT64* ResourcePointArrayPointer;
	UINTN   ResourcePointArrayCount;
	UINTN   Index = 0;
	UINT64  AlignSize = 0x1000;
	UINTN   MemoryPointArrayMaxCount;
	MemoryPointArrayMaxCount = 2;
	ResourcePointArrayPointer = ResourcePointArray;

	ResourcePointArrayCount = GetNRandomNumber(WHOLE_BASE, ((EFI_HOB_HANDOFF_INFO_TABLE*)HobList1)->EfiMemoryBottom, AlignSize, SpecialArray, SpecialArrayCount, (rand() % (ResourcePointArrayMaxCount - 1)) / 2 * 2 + 1, &ResourcePointArrayPointer);
	ResourcePointArrayCount += GetNRandomNumber(((EFI_HOB_HANDOFF_INFO_TABLE*)HobList1)->EfiMemoryTop, ((EFI_HOB_HANDOFF_INFO_TABLE*)HobList2)->EfiMemoryBottom, AlignSize, SpecialArray, SpecialArrayCount, (rand() % (ResourcePointArrayMaxCount - ResourcePointArrayCount)) / 2 * 2, &ResourcePointArrayPointer);
	ResourcePointArrayCount += GetNRandomNumber(((EFI_HOB_HANDOFF_INFO_TABLE*)HobList2)->EfiMemoryTop, WHOLE_LIMIT, AlignSize, SpecialArray, SpecialArrayCount, (rand() % (ResourcePointArrayMaxCount - ResourcePointArrayCount)) / 2 * 2 + 1, &ResourcePointArrayPointer);


	while (ResourcePointArrayCount != 0) {
		CreateRandomResourceHob(
			ResourcePointArray[Index],
			ResourcePointArray[Index + 1],
			AlignSize,
			HobList1,
			HobList2,
			MemoryPointArrayMaxCount
		);
		Index += 2;
		ResourcePointArrayCount -= 2;
	}
	/*


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
		*/
}
