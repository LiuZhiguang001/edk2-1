/** @file
  

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CBOR_SET_WRAPPER_H__
#define __CBOR_SET_WRAPPER_H__



RETURN_STATUS
EFIAPI
CborEncoderInit (
  OUT VOID  **RootEncoderPointer,
  OUT VOID  **RootMapEncoderPointer,
  OUT VOID  **Buffer,
  IN UINTN  size
  );
RETURN_STATUS
EFIAPI
CborEncoderCreateSubMap (
  VOID   *ParentEncoder,
  VOID   *Child
  );

RETURN_STATUS
EFIAPI
CborEncoderCreateArray (
  VOID   *ParentEncoder,
  VOID   *ArrayEncoder,
  UINTN  Length
  );

RETURN_STATUS
EFIAPI
CborEncodeTextString (
  VOID         *MapEncoder,
  const CHAR8  *TextString
  );

RETURN_STATUS
EFIAPI
CborEncodeByteString (
  VOID         *MapEncoder,
  const UINT8  *ByteString,
  UINTN        size
  );

RETURN_STATUS
EFIAPI
CborEncodeUint64 (
  VOID    *MapEncoder,
  UINT64  Value
  );

RETURN_STATUS
EFIAPI
CborEncodeUint8 (
  VOID   *MapEncoder,
  UINT8  Value
  );

RETURN_STATUS
EFIAPI
CborEncodeBoolean (
  VOID     *MapEncoder,
  BOOLEAN  Value
  );

RETURN_STATUS
EFIAPI
CborEncoderCloseContainer (
  VOID  *ParentEncoder,
  VOID  *ContainerEncoder
  );

UINTN
EFIAPI
CborGetBuffer (
  VOID  *RootEncoder,
  VOID         *Buffer
  );

#endif