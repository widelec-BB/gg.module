/*
 * Copyright (c) 2013 - 2018 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* GGLib/ggmessage.h
 *
 *  NAME
 *    ggmessage.h -- Plik zawieraj±cy deklaracje funkcji obs³ugi wiadomo¶ci GG.
 *
 *  AUTHOR
 *    Filip Maryjañski
 *
 *  DESCRIPTION
 *    Deklaracje funkcji obs³ugi wiadomo¶ci GG (konwersja z i do HTML)
 *
 ********/

#ifndef __GGMESSAGE_H__
#define __GGMESSAGE_H__

STRPTR GGMessageHTMLtoText(STRPTR html, STRPTR *images);
STRPTR GGMessageTextToHTML(STRPTR txt, STRPTR images);

#endif /* __GGMESSAGE_H__ */
