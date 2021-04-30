/** @file
  Universal Payload general definations.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UNIVERSAL_PAYLOAD_H__
#define __UNIVERSAL_PAYLOAD_H__

#pragma pack(1)

typedef struct {
  UINT8                Revision;
  UINT8                Reserved[3];
} PLD_GENERIC_HEADER;

#pragma pack()

#define PLD_GENERIC_HEADER_REVISION 0

#endif // __UNIVERSAL_PAYLOAD_H__
