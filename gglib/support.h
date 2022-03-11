/*
 * Copyright (c) 2013 - 2022 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __SUPPORT_H__
#define __SUPPORT_H__

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <openssl/ssl.h>

#define _between(a,x,b) ((x)>=(a) && (x)<=(b))

VOID *MemSet(VOID *ptr, LONG word, LONG size);
STRPTR InetToStr(ULONG no);
LONG SendAllSSL(SSL *ssl, BYTE *buf, LONG len);
LONG RecvAllSSL(struct Library *SocketBase, SSL *ssl, BYTE *buf, LONG len);
BOOL StrIEqu(STRPTR s, STRPTR d); /* case insensitive */
STRPTR StrNewLen(STRPTR s, LONG len);
UBYTE *Inflate(UBYTE *data, ULONG *len);
UBYTE *Deflate(UBYTE *data, ULONG *len);
UBYTE StrByteToByte(STRPTR str_byte);
ULONG FileCrc32(BPTR fh);

#ifdef __DEBUG__
VOID DumpBinaryData(UBYTE *data, ULONG len);
#endif /* __DEBUG__ */

#endif /* __SUPPORT_H__ */
