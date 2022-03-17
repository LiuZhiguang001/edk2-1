#include <Uefi.h>
#include <Guid/CborRootmapHobGuid.h>
#include <Library/DebugLib.h>
#include <PiPei.h>
#include <Library/IoLib.h>
#include <Library/GetUplDataLib.h>
#include "CborGetWrapper.h"

UPL_DATA_DECODER  RootValue;
CborParser        Parser;

#define CHECK_CBOR_DECODE_ERROR(Expression)      \
    do {                                         \
      if (Expression != CborNoError) {           \
        return RETURN_UNSUPPORTED;               \
      }                                          \
    } while (FALSE)

RETURN_STATUS
EFIAPI
CborDecoderGetRootMap (
  IN VOID   *Buffer,
  IN UINTN  Size
  )
{
  CHECK_CBOR_DECODE_ERROR (cbor_parser_init (Buffer, Size, 0, &Parser, &RootValue));

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
CborDecoderGetUint64 (
  IN  CHAR8   *String,
  OUT UINT64  *Result,
  IN  VOID    *Map
  )
{
  CborValue  Element;

  if (Map == NULL) {
    Map = &RootValue;
  }

  CHECK_CBOR_DECODE_ERROR (cbor_value_map_find_value (Map, String, &Element));
  if (cbor_value_is_valid (&Element) == FALSE) {
    return RETURN_NOT_FOUND;
  }

  if (cbor_value_is_integer (&Element) == FALSE) {
    return RETURN_INVALID_PARAMETER;
  }

  CHECK_CBOR_DECODE_ERROR (cbor_value_get_uint64 (&Element, Result));

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
CborDecoderGetUint8 (
  IN  CHAR8  *String,
  OUT UINT8  *Result,
  IN  VOID   *Map
  )
{
  CborValue  Element;

  if (Map == NULL) {
    Map = &RootValue;
  }

  CHECK_CBOR_DECODE_ERROR(cbor_value_map_find_value (Map, String, &Element));
  if (cbor_value_is_valid (&Element) == FALSE) {
    return RETURN_NOT_FOUND;
  }

  if (cbor_value_is_simple_type (&Element) == FALSE) {
    return RETURN_INVALID_PARAMETER;
  }

  CHECK_CBOR_DECODE_ERROR (cbor_value_get_simple_type (&Element, Result));

  return RETURN_SUCCESS;

}

RETURN_STATUS
EFIAPI
CborDecoderGetBoolean (
  IN  CHAR8    *String,
  OUT BOOLEAN  *Result,
  IN  VOID     *Map
  )
{
  CborValue  Element;

  if (Map == NULL) {
    Map = &RootValue;
  }

  CHECK_CBOR_DECODE_ERROR(cbor_value_map_find_value (Map, String, &Element));
  if (cbor_value_is_valid (&Element) == FALSE) {
    return RETURN_NOT_FOUND;
  }

  if (cbor_value_is_boolean (&Element) == FALSE) {
    return RETURN_INVALID_PARAMETER;
  }

  CHECK_CBOR_DECODE_ERROR(cbor_value_get_boolean (&Element, (bool *)Result));

  return RETURN_SUCCESS;

}

RETURN_STATUS
EFIAPI
CborDecoderGetBinary (
  IN     VOID   *Value,
  OUT    VOID   *Result,
  IN OUT UINTN  *Size,
  IN     VOID   *Map
  )
{
  CborValue  Element;

  if (Map == NULL) {
    Map = &RootValue;
  }

  CHECK_CBOR_DECODE_ERROR(cbor_value_map_find_value (Map, Value, &Element));
  if (cbor_value_is_valid (&Element) == FALSE) {
    return RETURN_NOT_FOUND;
  }

  if (cbor_value_is_byte_string (&Element) == FALSE) {
    return RETURN_INVALID_PARAMETER;
  }

  CHECK_CBOR_DECODE_ERROR(cbor_value_copy_byte_string (&Element, (uint8_t *)Result, Size, NULL));

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
CborDecoderGetTextString (
  IN     VOID   *Value,
  OUT    UINT8  *Result,
  IN OUT UINTN  *Size,
  IN     VOID   *Map
  )
{
  CborValue  Element;

  if (Map == NULL) {
    Map = &RootValue;
  }

  CHECK_CBOR_DECODE_ERROR(cbor_value_map_find_value (Map, Value, &Element));
  if (cbor_value_is_valid (&Element) == FALSE) {
    return RETURN_NOT_FOUND;
  }

  if (cbor_value_is_text_string (&Element) == FALSE) {
    return RETURN_INVALID_PARAMETER;
  }
  CHECK_CBOR_DECODE_ERROR(cbor_value_copy_text_string (&Element, (char *)Result, Size, NULL));

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
CborDecoderGetArrayNextMap (
  IN OUT UPL_DATA_DECODER  *NextMap
  )
{
  CHECK_CBOR_DECODE_ERROR (cbor_value_advance (NextMap));

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
CborDecoderGetArrayLengthAndFirstElement (
  IN     CHAR8             *String,
  OUT    UINTN             *Size,
  OUT    UPL_DATA_DECODER  *Map
  )
{
  CborValue  Array;

  CHECK_CBOR_DECODE_ERROR (cbor_value_map_find_value (&RootValue, String, &Array));

  if (cbor_value_is_valid (&Array) == FALSE) {
    return RETURN_NOT_FOUND;
  }

  if (cbor_value_is_array (&Array) == FALSE) {
    return RETURN_INVALID_PARAMETER;
  }

  CHECK_CBOR_DECODE_ERROR (cbor_value_get_array_length (&Array, Size));
  CHECK_CBOR_DECODE_ERROR (cbor_value_enter_container (&Array, Map));
  return RETURN_SUCCESS;
}
