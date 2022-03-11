/*
 * Copyright (c) 2013 - 2022 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* GGLib/gghandlers.h
 *
 *  NAME
 *    gghandlers.h -- Plik nag��wkowy handler�w.
 *
 *  AUTHOR
 *    Filip Maryja�ski
 *
 *  DESCRIPTION
 *    Definicje sta�ych u�ywanych przez funkcje obs�uguj�ce zdarzenia.
 *
 ********/

#ifndef __GGHANDLERS_H__
#define __GGHANDLERS_H__

/****id* GGLib/GGH_RETURN_#?
 *
 *  NAME
 *    GGH_RETURN_#?
 *
 *  FUNCTION
 *    Definicje kod�w powrot�w z handler�w zdarze�.
 *
 *  ATTRIBUTES
 *    - GGH_RETURN_UNKNOWN -- nie zosta� wywo�any �aden handler;
 *    - GGH_RETURN_NEXT -- po��czenie potrzebuje ponownego wykonania GGWatchEvent;
 *    - GGH_RETURN_ERROR -- wyst�pi� b��d - stan po��czenia zosta� zmieniony oraz zosta� ustawiony kod b��du;
 *    - GGH_RETURN_WAIT -- biblioteka potrzebuje czasu na doko�czenie dzia�ania, mo�na wr�ci� do WaitSelect().
 *
 *  SOURCE
 */

#define GGH_RETURN_UNKNOWN    (-3)
#define GGH_RETURN_NEXT       (-2)
#define GGH_RETURN_ERROR      (-1)
#define GGH_RETURN_WAIT        (0)
/*******GGH_RETURN_#?****/

#include "gglib.h"

LONG GGHandleFresh(struct GGSession *gg_sess, struct GGEvent *gg_event);
LONG GGHandleConnecting(struct GGSession *gg_sess, struct GGEvent *gg_event);
LONG GGHandleConnected(struct GGSession *gg_sess, struct GGEvent *event);
LONG GGHandleDisconnecting(struct GGSession *gg_sess, struct GGEvent *event);

#endif /* __GGHANDLERS_H__ */
