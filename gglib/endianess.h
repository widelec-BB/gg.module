/*
 * Copyright (c) 2013 - 2022 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* GGLib/endianess.h
 *
 *  NAME
 *    endianess.h -- Definicje makr zamiany kolejno¶ci bajtów.
 *
 *  AUTHOR
 *    Filip Maryjañski
 *
 *  DESCRIPTION
 *    Definicje makr umo¿liwiaj±cych konwersjê warto¶ci 64, 32 i 16 bitowych
 *    z formatu Little Endian na Big Endian i odwrotnie.
 *
 ********/


#ifndef __ENDIANESS_H__
#define __ENDIANESS_H__

#if BYTE_ORDER == BIG_ENDIAN
#define EndianFix64(x) 	\
		((((x) & 0x00000000000000ffULL) << 56) | \
		(((x) & 0x000000000000ff00ULL) << 40)  | \
		(((x) & 0x0000000000ff0000ULL) << 24)  | \
		(((x) & 0x00000000ff000000ULL) << 8)   | \
		(((x) & 0x000000ff00000000ULL) >> 8)   | \
		(((x) & 0x0000ff0000000000ULL) >> 24)  | \
		(((x) & 0x00ff000000000000ULL) >> 40)  | \
		(((x) & 0xff00000000000000ULL) >> 56))
#define EndianFix32(x)  \
		((((x) & 0x000000ffU) << 24) | \
		(((x) & 0x0000ff00U) << 8)   | \
		(((x) & 0x00ff0000U) >> 8)   | \
		(((x) & 0xff000000U) >> 24))
#define EndianFix16(x)  \
		((((x) & 0x00ffU) << 8) | \
		(((x) & 0xff00U) >> 8))
#else /* __BIG_ENDIAN__ */
#if BYTE_ORDER == LITTLE_ENDIAN
#define EndianFix64(x) (x)
#define EndianFix32(x) (x)
#define EndianFix16(x) (x)
#else /* __LITTLE_ENDIAN */
#error Endiannes unknown!
#endif /* __LITTLE_ENDIAN__ */
#endif /* __BIG_ENDIAN__ */

#endif /* __ENDIANESS_H__ */
