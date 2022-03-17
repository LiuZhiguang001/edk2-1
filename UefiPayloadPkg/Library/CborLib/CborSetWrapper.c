#include "tinycbor\src\cbor.h"
#include <Base.h>
#include <Library/DebugLib.h>
#include <PiPei.h>
#include <Library/MemoryAllocationLib.h>

#define CHECK_CBOR_ENCODE_ERROR(Expression)    \
    do {                                       \
      switch (Expression)                      \
      {                                        \
      case CborNoError:                        \
        break;                                 \
      case CborErrorOutOfMemory:               \
        return RETURN_OUT_OF_RESOURCES;        \
      default:                                 \
        return EFI_UNSUPPORTED;                \
      }                                        \
    } while (FALSE)

CborEncoder  mSubMapEncoder;
CborEncoder  mArrayEncoder;

CborEncoder  *gRootMapEncoderPointer;

RETURN_STATUS
EFIAPI
CborEncoderInit (
  OUT VOID  **RootEncoderPointer,
  OUT VOID  **RootMapEncoderPointer,
  OUT VOID  **Buffer,
  IN UINTN  BufferSize
  )
{
  *RootEncoderPointer    = AllocatePages (EFI_SIZE_TO_PAGES (BufferSize+ 2*sizeof (CborEncoder)));
  *RootMapEncoderPointer = (UINT8 *)*RootEncoderPointer + sizeof (CborEncoder);
  *Buffer                = (UINT8 *)*RootEncoderPointer + 2*sizeof (CborEncoder);

  cbor_encoder_init (*RootEncoderPointer, *Buffer, BufferSize, 0);
  CHECK_CBOR_ENCODE_ERROR (cbor_encoder_create_map (*RootEncoderPointer, *RootMapEncoderPointer, CborIndefiniteLength));
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
CborEncoderCreateSubMap (
  VOID  *ParentEncoder,
  VOID  **Child
  )
{
  CHECK_CBOR_ENCODE_ERROR (cbor_encoder_create_map (ParentEncoder, &mSubMapEncoder, CborIndefiniteLength));
  *Child = &mSubMapEncoder;
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
CborEncoderCreateArray (
  VOID   *ParentEncoder,
  VOID   **ArrayEncoder,
  UINTN  Length
  )
{
  CHECK_CBOR_ENCODE_ERROR (cbor_encoder_create_array (ParentEncoder, &mArrayEncoder, Length));
  *ArrayEncoder = &mArrayEncoder;
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
CborEncodeTextString (
  VOID         *MapEncoder,
  const CHAR8  *TextString
  )
{
  CHECK_CBOR_ENCODE_ERROR (cbor_encode_text_stringz (MapEncoder, TextString));
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
CborEncodeByteString (
  VOID         *MapEncoder,
  const UINT8  *ByteString,
  UINTN        Size
  )
{
  CHECK_CBOR_ENCODE_ERROR (cbor_encode_byte_string (MapEncoder, ByteString, Size));

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
CborEncodeUint64 (
  VOID    *MapEncoder,
  UINT64  Value
  )
{
  CHECK_CBOR_ENCODE_ERROR (cbor_encode_uint (MapEncoder, Value));

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
CborEncodeUint8 (
  VOID   *MapEncoder,
  UINT8  Value
  )
{
  CHECK_CBOR_ENCODE_ERROR (cbor_encode_uint (MapEncoder, Value));

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
CborEncodeBoolean (
  VOID     *MapEncoder,
  BOOLEAN  Value
  )
{
  CHECK_CBOR_ENCODE_ERROR (cbor_encode_boolean (MapEncoder, Value));

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
CborEncoderCloseContainer (
  VOID  *ParentEncoder,
  VOID  *ContainerEncoder
  )
{
  CHECK_CBOR_ENCODE_ERROR (cbor_encoder_close_container (ParentEncoder, ContainerEncoder));

  return RETURN_SUCCESS;
}

UINTN
EFIAPI
CborGetBuffer (
  VOID  *ParentEncoder,
  VOID  *Buffer
  )
{
  UINTN  UsedSize;

  UsedSize = cbor_encoder_get_buffer_size (ParentEncoder, (const uint8_t *)Buffer);
  DEBUG ((EFI_D_ERROR, "used_size: 0x%x \n", UsedSize));

  return UsedSize;
}
