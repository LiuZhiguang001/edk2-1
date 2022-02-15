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

#define PAGE_HEAD_PRIVATE_SIGNATURE  SIGNATURE_32 ('P', 'H', 'D', 'R')


extern VOID* mHobList;
#define MAX_DEBUG_MESSAGE_LENGTH 0x100
char TempFormat[MAX_DEBUG_MESSAGE_LENGTH];

typedef struct {
	UINT32    Signature;
	VOID* AllocatedBufffer;
	UINTN     TotalPages;
	VOID* AlignedBuffer;
	UINTN     AlignedPages;
} PAGE_HEAD;


BOOLEAN
EFIAPI
DebugPrintEnabled(
  VOID
)
{
  return TRUE;
}


char* strrpc(char* str, char* oldstr, char* newstr) {

	memset(TempFormat, 0, sizeof(TempFormat));

	for (int i = 0; i < strlen(str); i++) {
		if (!strncmp(str + i, oldstr, strlen(oldstr))) {

			strncat_s(TempFormat, MAX_DEBUG_MESSAGE_LENGTH, newstr, sizeof(newstr));

			i += strlen(oldstr) - 1;
		}
		else {
			strncat_s(TempFormat, MAX_DEBUG_MESSAGE_LENGTH, str + i, 1 );
		}
	}

	return TempFormat;
}

VOID
EFIAPI
DebugVPrint(
  IN  UINTN         ErrorLevel,
  IN  CONST CHAR8   *Format,
  IN  VA_LIST       VaListMarker
)
{
  CHAR8  Buffer[MAX_DEBUG_MESSAGE_LENGTH];

	Format = strrpc(Format, "%lx", "%llx");
	char TempFormat1[MAX_DEBUG_MESSAGE_LENGTH];
	memset(TempFormat1, 0, sizeof(TempFormat1));
	strcpy_s(TempFormat1, MAX_DEBUG_MESSAGE_LENGTH, Format);
	Format = strrpc(TempFormat1, "%g", "%lx");

	memset(TempFormat1, 0, sizeof(TempFormat1));
	strcpy_s(TempFormat1, MAX_DEBUG_MESSAGE_LENGTH, Format);
	Format = strrpc(TempFormat1, "%a", "%s");

  vsprintf_s(Buffer, sizeof(Buffer), Format, VaListMarker);
  printf("%s", Buffer);
}

VOID
EFIAPI
DebugPrint(
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  ...
)
{
  VA_LIST  Marker;

  VA_START(Marker, Format);
  DebugVPrint(ErrorLevel, Format, Marker);
  VA_END(Marker);
}

BOOLEAN
EFIAPI
DebugPrintLevelEnabled(
  IN  CONST UINTN        ErrorLevel
)
{
  if (ErrorLevel == DEBUG_VERBOSE) return FALSE;
  return TRUE;
}


EFI_GUID*
CopyGuid(
	EFI_GUID* DestinationGuid,
	CONST EFI_GUID* SourceGuid
)
{
	*DestinationGuid = *SourceGuid;
	*(DestinationGuid + 1) = *(SourceGuid + 1);

	return DestinationGuid;
}




VOID*
EFIAPI
AllocateAlignedPages(
	IN UINTN  Pages,
	IN UINTN  Alignment
)
{
	PAGE_HEAD  PageHead;
	PAGE_HEAD* PageHeadPtr;
	UINTN      AlignmentMask;

	ASSERT((Alignment & (Alignment - 1)) == 0);

	if (Alignment < SIZE_4KB) {
		Alignment = SIZE_4KB;
	}

	AlignmentMask = Alignment - 1;

	//
	// We need reserve Alignment pages for PAGE_HEAD, as meta data.
	//
	PageHead.Signature = PAGE_HEAD_PRIVATE_SIGNATURE;
	PageHead.TotalPages = Pages + EFI_SIZE_TO_PAGES(Alignment) * 2;
	PageHead.AlignedPages = Pages;
	PageHead.AllocatedBufffer = malloc(EFI_PAGES_TO_SIZE(PageHead.TotalPages));
	if (PageHead.AllocatedBufffer == NULL) {
		return NULL;
	}

	PageHead.AlignedBuffer = (VOID*)(((UINTN)PageHead.AllocatedBufffer + AlignmentMask) & ~AlignmentMask);
	if ((UINTN)PageHead.AlignedBuffer - (UINTN)PageHead.AllocatedBufffer < sizeof(PAGE_HEAD)) {
		PageHead.AlignedBuffer = (VOID*)((UINTN)PageHead.AlignedBuffer + Alignment);
	}

	PageHeadPtr = (VOID*)((UINTN)PageHead.AlignedBuffer - sizeof(PAGE_HEAD));
	memcpy(PageHeadPtr, &PageHead, sizeof(PAGE_HEAD));

	return PageHead.AlignedBuffer;
}

VOID*
EFIAPI
AllocatePages(
	IN UINTN  Pages
)
{
	return AllocateAlignedPages(Pages, SIZE_4KB);
}



VOID*
EFIAPI
ZeroMem(
	OUT VOID* Buffer,
	IN UINTN  Length
)
{ 
	UINT8  Value = 0;

	for (; Length != 0; Length--) {
		((UINT8*)Buffer)[Length - 1] = Value;
	}
	return Buffer;
}

INTN
StrCmp(
	CONST CHAR16* FirstString,
	CONST CHAR16* SecondString
)
{
	while ((*FirstString != L'\0') && (*FirstString == *SecondString)) {
		FirstString++;
		SecondString++;
	}
	return *FirstString - *SecondString;
}

UINT64
ReadUnaligned64(
	CONST UINT64* Buffer
)
{
	ASSERT(Buffer != NULL);

	return *Buffer;
}

BOOLEAN
EFIAPI
IsZeroGuid(
	IN CONST GUID* Guid
)
{
	UINT64  LowPartOfGuid;
	UINT64  HighPartOfGuid;

	LowPartOfGuid = ReadUnaligned64((CONST UINT64*)Guid);
	HighPartOfGuid = ReadUnaligned64((CONST UINT64*)Guid + 1);

	//return (BOOLEAN)(LowPartOfGuid == 0 && HighPartOfGuid == 0);
	return TRUE;
}

BOOLEAN
EFIAPI
CompareGuid(
	IN CONST GUID* Guid1,
	IN CONST GUID* Guid2
)
{
	UINT64  LowPartOfGuid1;
	UINT64  LowPartOfGuid2;
	UINT64  HighPartOfGuid1;
	UINT64  HighPartOfGuid2;

	LowPartOfGuid1 = ReadUnaligned64((CONST UINT64*)Guid1);
	LowPartOfGuid2 = ReadUnaligned64((CONST UINT64*)Guid2);
	HighPartOfGuid1 = ReadUnaligned64((CONST UINT64*)Guid1 + 1);
	HighPartOfGuid2 = ReadUnaligned64((CONST UINT64*)Guid2 + 1);

	return (BOOLEAN)(LowPartOfGuid1 == LowPartOfGuid2 && HighPartOfGuid1 == HighPartOfGuid2);
}

