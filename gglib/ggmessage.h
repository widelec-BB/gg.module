/*
 * Copyright (c) 2013 - 2022 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* GGLib/ggmessage.h
 *
 *  NAME
 *    ggmessage.h -- Plik zawierający deklaracje funkcji obsługi wiadomości GG.
 *
 *  AUTHOR
 *    Filip Maryjański
 *
 *  DESCRIPTION
 *    Deklaracje funkcji obsługi wiadomości GG (konwersja z i do HTML)
 *
 ********/

#ifndef __GGMESSAGE_H__
#define __GGMESSAGE_H__

STRPTR GGMessageHTMLtoText(STRPTR html, STRPTR *images);
STRPTR GGMessageTextToHTML(STRPTR txt, STRPTR images);

#endif /* __GGMESSAGE_H__ */
