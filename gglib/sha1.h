/*
 * Copyright (c) 2013 - 2018 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* GGLib/sha1.h
 *
 *  NAME
 *    sha1.h -- Definicje funkcji wyznaczania skrótu SHA-1
 *
 *  AUTHOR
 *    Steve Reid
 *
 *  DESCRIPTION
 *    Implementacja funkcji skrótu SHA-1 na licencji public domain.
 *    Wersja specjalnie dopasowana do projektu.
 *
 ********/

#ifndef __SHA1_H__
#define __SHA1_H__

VOID GGSha1Hash(CONST_STRPTR password, ULONG seed, UBYTE *result);

#endif /* __SHA1_H__ */
