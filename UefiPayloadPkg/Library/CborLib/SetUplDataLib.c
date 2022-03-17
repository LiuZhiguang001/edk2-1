#include <PiPei.h>
#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Guid/CborRootmapHobGuid.h>
#include "CborSetWrapper.h"
#include <Library/SetUplDataLib.h>

#define CHECK_ERROR(Expression)          \
    do {                                 \
      Status = Expression;               \
      if (Status != RETURN_SUCCESS) {    \
        return Status;                   \
      }                                  \
    } while (FALSE)

VOID  *RootEncoderPointer, *RootMapEncoderPointer;
VOID  *Buffer;

RETURN_STATUS
EFIAPI
SetUplDataLibConstructor (
  VOID
  )
{
  VOID               *Hob;
  UINTN              Size;
  CBOR_ROOTMAP_INFO  *Data;
  RETURN_STATUS      Status;

  Size = PcdGet32(PcdUniversalPayloadInterfaceSize);
  DEBUG ((DEBUG_INFO, "KBT In TinyCborEncoderLibConstructor.\n"));
  Hob = (VOID *)GetFirstGuidHob (&CborRootmapHobGuid);
  if (Hob == NULL) {
    CHECK_ERROR (CborEncoderInit (&RootEncoderPointer, &RootMapEncoderPointer, &Buffer, Size));
    Data                 = (CBOR_ROOTMAP_INFO *)BuildGuidHob (&CborRootmapHobGuid, sizeof (CBOR_ROOTMAP_INFO));
    Data->RootEncoder    = RootEncoderPointer;
    Data->RootMapEncoder = RootMapEncoderPointer;
    Data->Buffer         = Buffer;
    SetUplUint64("BufferSize", Size);
  } else {
    Data                  = GET_GUID_HOB_DATA (Hob);
    RootEncoderPointer    = Data->RootEncoder;
    RootMapEncoderPointer = Data->RootMapEncoder;
    Buffer                = Data->Buffer;
  }

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
SetUplUint64 (
  IN CHAR8   *Key,
  IN UINT64  Value
  )
{
  RETURN_STATUS  Status;

  CHECK_ERROR (CborEncodeTextString (RootMapEncoderPointer, Key));
  CHECK_ERROR (CborEncodeUint64 (RootMapEncoderPointer, Value));
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
SetUplUint8 (
  IN CHAR8  *Key,
  IN UINT8  Value
  )
{
  RETURN_STATUS  Status;

  CHECK_ERROR (CborEncodeTextString (RootMapEncoderPointer, Key));
  CHECK_ERROR (CborEncodeUint8 (RootMapEncoderPointer, Value));
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
SetUplBinary (
  IN CHAR8  *Key,
  IN VOID   *Value,
  IN UINTN  Size
  )
{
  RETURN_STATUS  Status;

  CHECK_ERROR (CborEncodeTextString (RootMapEncoderPointer, Key));
  CHECK_ERROR (CborEncodeByteString (RootMapEncoderPointer, Value, Size));
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
SetUplAsciiString (
  IN CHAR8  *Key,
  IN UINT8  *Value,
  IN UINTN  Size
  )
{
  // todo: Size need be used.
  // todo :if value don't have a terminal character, Size will be the maxium number
  RETURN_STATUS  Status;

  CHECK_ERROR (CborEncodeTextString (RootMapEncoderPointer, Key));
  CHECK_ERROR (CborEncodeTextString (RootMapEncoderPointer, (const char *)Value));
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
SetUplBoolean (
  IN CHAR8    *Key,
  IN BOOLEAN  Value
  )
{
  RETURN_STATUS  Status;

  CHECK_ERROR (CborEncodeTextString (RootMapEncoderPointer, Key));
  CHECK_ERROR (CborEncodeBoolean (RootMapEncoderPointer, Value));
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
LockUplAndGetBuffer (
  OUT VOID   **Buff,
  OUT UINTN  *Size
  )
{
  RETURN_STATUS  Status;

  CHECK_ERROR (CborEncoderCloseContainer (RootEncoderPointer, RootMapEncoderPointer));
  *Size  = CborGetBuffer (RootEncoderPointer, Buffer);
  *Buff  = Buffer;
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
SetUplExtraData (
  IN UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY  *Data,
  IN UINTN                               Count
  )
{
  VOID           *ArrayEncoder;
  VOID           *SubMapEncoder;
  UINTN          Index;
  RETURN_STATUS  Status;

  CHECK_ERROR (CborEncodeTextString (RootMapEncoderPointer, "UplExtradata"));
  CHECK_ERROR (CborEncoderCreateArray (RootMapEncoderPointer, &ArrayEncoder, Count));
  for (Index = 0; Index < Count; Index++) {
    CHECK_ERROR (CborEncoderCreateSubMap (ArrayEncoder, &SubMapEncoder));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Identifier"));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, Data[Index].Identifier));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Base"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Base));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Size"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Size));
    CHECK_ERROR (CborEncoderCloseContainer (ArrayEncoder, SubMapEncoder));
  }

  CHECK_ERROR (CborEncoderCloseContainer (RootMapEncoderPointer, ArrayEncoder));
  return EFI_SUCCESS;
}

RETURN_STATUS
EFIAPI
SetUplMemoryMap (
  IN EFI_MEMORY_DESCRIPTOR  *Data,
  IN UINTN                  Count
  )
{
  VOID           *ArrayEncoder;
  VOID           *SubMapEncoder;
  UINTN          Index;
  RETURN_STATUS  Status;

  CHECK_ERROR (CborEncodeTextString (RootMapEncoderPointer, "MemoryMap"));
  CHECK_ERROR (CborEncoderCreateArray (RootMapEncoderPointer, &ArrayEncoder, Count));
  for (Index = 0; Index < Count; Index++) {
    CHECK_ERROR (CborEncoderCreateSubMap (ArrayEncoder, &SubMapEncoder));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Base"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].PhysicalStart));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "NumberOfPages"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].NumberOfPages));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Type"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Type));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Attribute"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Attribute));
    CHECK_ERROR (CborEncoderCloseContainer (ArrayEncoder, SubMapEncoder));
  }

  CHECK_ERROR (CborEncoderCloseContainer (RootMapEncoderPointer, ArrayEncoder));
  return EFI_SUCCESS;
}

RETURN_STATUS
EFIAPI
SetUplPciRootBridges (
  IN UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE  *Data,
  IN UINTN                              Count
  )
{
  VOID           *ArrayEncoder;
  VOID           *SubMapEncoder;
  UINTN          Index;
  RETURN_STATUS  Status;

  CHECK_ERROR (CborEncodeTextString (RootMapEncoderPointer, "RootBridgeInfo"));
  CHECK_ERROR (CborEncoderCreateArray (RootMapEncoderPointer, &ArrayEncoder, Count));

  for (Index = 0; Index < Count; Index++) {
    CHECK_ERROR (CborEncoderCreateSubMap (ArrayEncoder, &SubMapEncoder));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Segment"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Segment));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Supports"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Supports));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Attributes"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Attributes));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "DmaAbove4G"));
    CHECK_ERROR (CborEncodeBoolean (SubMapEncoder, Data[Index].DmaAbove4G));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "NoExtendedConfigSpace"));
    CHECK_ERROR (CborEncodeBoolean (SubMapEncoder, Data[Index].NoExtendedConfigSpace));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "AllocationAttributes"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].AllocationAttributes));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "BusBase"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Bus.Base));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "BusLimit"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Bus.Limit));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "BusTranslation"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Bus.Translation));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "IoBase"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Io.Base));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "IoLimit"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Io.Limit));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "IoTranslation"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Io.Translation));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "MemBase"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Mem.Base));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "MemLimit"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Mem.Limit));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "MemTranslation"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Mem.Translation));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "MemAbove4GBase"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].MemAbove4G.Base));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "MemAbove4GLimit"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].MemAbove4G.Limit));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "MemAbove4GTranslation"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].MemAbove4G.Translation));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "PMemBase"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].PMem.Base));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "PMemLimit"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].PMem.Limit));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "PMemTranslation"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].PMem.Translation));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "PMemAbove4GBase"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].PMemAbove4G.Base));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "PMemAbove4GLimit"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].PMemAbove4G.Limit));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "PMemAbove4GTranslation"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].PMemAbove4G.Translation));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "HID"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].HID));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "UID"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].UID));
    CHECK_ERROR (CborEncoderCloseContainer (ArrayEncoder, SubMapEncoder));
  }

  CHECK_ERROR (CborEncoderCloseContainer (RootMapEncoderPointer, ArrayEncoder));
  return EFI_SUCCESS;
}

RETURN_STATUS
EFIAPI
SetUplResourceData (
  IN UNIVERSAL_PAYLOAD_RESOURCE_DESCRIPTOR  *Data,
  IN UINTN                                  Count
  )
{
  VOID           *ArrayEncoder;
  VOID           *SubMapEncoder;
  UINTN          Index;
  RETURN_STATUS  Status;

  DEBUG ((EFI_D_ERROR, "KBT Data->Owner: %g \n", &Data->Owner));
  DEBUG ((EFI_D_ERROR, "KBT sizeof(Data[Index].Owner: %x \n", sizeof (Data[0].Owner)));
  CHECK_ERROR (CborEncodeTextString (RootMapEncoderPointer, "Resource"));
  CHECK_ERROR (CborEncoderCreateArray (RootMapEncoderPointer, &ArrayEncoder, Count));
  for (Index = 0; Index < Count; Index++) {
    CHECK_ERROR (CborEncoderCreateSubMap (ArrayEncoder, &SubMapEncoder));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Owner"));
    CHECK_ERROR (CborEncodeByteString (SubMapEncoder, (UINT8 *)&Data[Index].Owner, sizeof (Data[Index].Owner)));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Type"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].ResourceType));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Attribute"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].ResourceAttribute));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Base"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].PhysicalStart));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Length"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].ResourceLength));
    CHECK_ERROR (CborEncoderCloseContainer (ArrayEncoder, SubMapEncoder));
  }

  CHECK_ERROR (CborEncoderCloseContainer (RootMapEncoderPointer, ArrayEncoder));
  return EFI_SUCCESS;
}

RETURN_STATUS
EFIAPI
SetUplMemoryAllocationData (
  IN UNIVERSAL_PAYLOAD_MEMORY_ALLOCATION  *Data,
  IN UINTN                                Count
  )
{
  VOID           *ArrayEncoder;
  VOID           *SubMapEncoder;
  UINTN          Index;
  RETURN_STATUS  Status;

  CHECK_ERROR (CborEncodeTextString (RootMapEncoderPointer, "ResourceAllocation"));
  CHECK_ERROR (CborEncoderCreateArray (RootMapEncoderPointer, &ArrayEncoder, Count));
  for (Index = 0; Index < Count; Index++) {
    CHECK_ERROR (CborEncoderCreateSubMap (ArrayEncoder, &SubMapEncoder));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Name"));
    CHECK_ERROR (CborEncodeByteString (SubMapEncoder, (UINT8 *)&Data[Index].Name, sizeof (Data[Index].Name)));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Base"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].MemoryBaseAddress));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Length"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].MemoryLength));
    CHECK_ERROR (CborEncodeTextString (SubMapEncoder, "Type"));
    CHECK_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].MemoryType));
    CHECK_ERROR (CborEncoderCloseContainer (ArrayEncoder, SubMapEncoder));
  }

  CHECK_ERROR (CborEncoderCloseContainer (RootMapEncoderPointer, ArrayEncoder));
  return EFI_SUCCESS;
}
