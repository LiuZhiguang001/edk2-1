

#include "tinycbor\src\cbor.h"


#include <Base.h>
#include <Library/DebugLib.h>
#include "Proto.h"
#include <Library/MemoryAllocationLib.h>
#define ROW_LIMITER 16


UINT8 buf11[16];

UINTN
PrintHex1 (
  IN  UINT8         *DataStart,
  IN  UINT16         DataSize
  )
{
  UINTN  Index1;
  UINTN  Index2;
  UINT8  *StartAddr;

  StartAddr = DataStart;
  for (Index1 = 0; Index1 * ROW_LIMITER < DataSize; Index1++) {
    DEBUG ((DEBUG_ERROR, "   0x%04p:", (DataStart - StartAddr)));
    for (Index2 = 0; (Index2 < ROW_LIMITER) && (Index1 * ROW_LIMITER + Index2 < DataSize); Index2++){
      DEBUG ((DEBUG_ERROR, " %02x", *DataStart));
      DataStart++;
    }
    DEBUG ((DEBUG_ERROR, "\n"));
  }

  return 0;
}



RETURN_STATUS
EFIAPI
SetCbor (
  OUT VOID                **Buffer,
  OUT UINTN               *Size
  )
{

  
  UINTN   size;
  size  = 300;
  UINT8   *buf = AllocatePool(size);
  CborError err;
  UINT32 buffer1[] = { 0x12345678, 0x0, 0x90ABCDEF};
  CborEncoder encoder, mapEncoder, SubmapEncoder;
  cbor_encoder_init(&encoder, buf, size, 0);

  err = cbor_encode_text_stringz(&encoder, "Before Map");
  err = cbor_encode_text_stringz(&encoder, "Another string");
  cbor_encode_uint(&encoder, 0x31415926);

  cbor_encoder_create_map(&encoder, &mapEncoder, 3);



  cbor_encode_text_stringz(&mapEncoder, "MyGuid");
  cbor_encode_byte_string(&mapEncoder, (const uint8_t *)&gUefiPayloadPkgTokenSpaceGuid, 16);

  cbor_encode_text_stringz(&mapEncoder, "RawByte");
  cbor_encode_byte_string(&mapEncoder, (const uint8_t *)buffer1, 12);

  cbor_encode_text_stringz(&mapEncoder, "SubCbor");
  cbor_encoder_create_map(&mapEncoder, &SubmapEncoder, 4);
  cbor_encode_text_stringz(&SubmapEncoder, "BaudRate");
  cbor_encode_uint(&SubmapEncoder, PcdGet32 (PcdSerialBaudRate));

  cbor_encode_text_stringz(&SubmapEncoder, "RegisterBase");
  cbor_encode_uint(&SubmapEncoder, PcdGet64 (PcdSerialRegisterBase));

  cbor_encode_text_stringz(&SubmapEncoder, "RegisterStride");
  cbor_encode_uint(&SubmapEncoder, PcdGet32 (PcdSerialRegisterStride));

  cbor_encode_text_stringz(&SubmapEncoder, "UseMmio");
  cbor_encode_uint(&SubmapEncoder, PcdGetBool (PcdSerialUseMmio));
  cbor_encoder_close_container(&mapEncoder, &SubmapEncoder);

  cbor_encoder_close_container(&encoder, &mapEncoder);

  CborEncoder arrayEncoder;
  cbor_encoder_create_array	(&encoder, &arrayEncoder, 3);
  cbor_encode_text_stringz(&arrayEncoder, "After Map");
  cbor_encode_text_stringz(&arrayEncoder, "Another string");
  cbor_encode_uint(&arrayEncoder, 0x123456);
  cbor_encoder_close_container(&encoder, &arrayEncoder);
  
  UINTN used_size;
  used_size = cbor_encoder_get_buffer_size (&encoder, (const uint8_t *)buf);
  DEBUG ((EFI_D_ERROR, "used_size: 0x%x \n", used_size ));
  PrintHex1(buf,used_size);

  *Buffer = buf;
  *Size   = used_size;

  
  return 0;
}
