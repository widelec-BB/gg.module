/*
 * Copyright (c) 2013 - 2022 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef LOCALE_H
#define LOCALE_H 1

/*
** locale.h
**
** (c) 2006 by Guido Mersmann
**
** Object source created by SimpleCat
*/

/*************************************************************************/
#define CATCOMP_BLOCK 1     /* enable CATCOMP_BLOCK */
#define CATCOMP_ARRAY
#include "translations.h" /* change name to correct locale header if needed */

/*
** Prototypes
*/

BOOL   Locale_Open( STRPTR catname, ULONG version, ULONG revision);
void   Locale_Close(void);
STRPTR GetString(long ID);

/*************************************************************************/

#endif /* LOCALE_H */
