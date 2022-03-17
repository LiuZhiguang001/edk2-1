/** @file
  This file defines the hob structure for serial port.

  Copyright (c) 2014 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CBOR_ROOTMAP_HOB_GUID_H_
#define __CBOR_ROOTMAP_HOB_GUID_H_

extern EFI_GUID CborRootmapHobGuid;

typedef struct {
  VOID *RootEncoder ;
  VOID *RootMapEncoder;
  VOID *Buffer;
} CBOR_ROOTMAP_INFO;

#endif
