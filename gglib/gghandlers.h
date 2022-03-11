/*
 * Copyright (c) 2013 - 2022 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* GGLib/gghandlers.h
 *
 *  NAME
 *    gghandlers.h -- Plik nag³ówkowy handlerów.
 *
 *  AUTHOR
 *    Filip Maryjañski
 *
 *  DESCRIPTION
 *    Definicje sta³ych u¿ywanych przez funkcje obs³uguj±ce zdarzenia.
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
 *    Definicje kodów powrotów z handlerów zdarzeñ.
 *
 *  ATTRIBUTES
 *    - GGH_RETURN_UNKNOWN -- nie zosta³ wywo³any ¿aden handler;
 *    - GGH_RETURN_NEXT -- po³±czenie potrzebuje ponownego wykonania GGWatchEvent;
 *    - GGH_RETURN_ERROR -- wyst±pi³ b³±d - stan po³±czenia zosta³ zmieniony oraz zosta³ ustawiony kod b³êdu;
 *    - GGH_RETURN_WAIT -- biblioteka potrzebuje czasu na dokoñczenie dzia³ania, mo¿na wróciæ do WaitSelect().
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
