/*
 * Copyright (c) 2013 - 2018 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __SUPPORT_H__
#define __SUPPORT_H__

#include <kwakwa_api/pictures.h>
#include <proto/charsets.h>

APTR LoadFile(STRPTR path, ULONG *size);
struct Picture *LoadPictureFile(STRPTR path);
struct Picture *LoadPictureMemory(APTR data, QUAD *length);
struct Picture *CopyPicture(struct Picture *src);
VOID FreePicture(struct Picture *pic);
STRPTR StrNewUTF8(STRPTR src, ULONG src_mib);

#endif /* __SUPPORT_H__ */
