/*
 * Copyright (c) 2013 - 2022 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* GGLib/ggdefs.h
 *
 *  NAME
 *    ggdefs.h -- Definicje sta³ych u¿ywanych przez protokó³ GG.
 *
 *  AUTHOR
 *    Filip Maryjañski
 *
 *  DESCRIPTION
 *    Definicje sta³ych u¿ywanych przez protokó³ GG.
 *    Opracowane na podstawie ¼róde³ libgadu.
 *
 ********/

#ifndef __GGDEFS_H__
#define __GGDEFS_H__

/* po³±czenie */
#define GG_DEFAULT_PORT    (443)
#define GG_HTTP_USERAGENT  "Mozilla/4.7 [en] (Win98; I)"

/* flagi ficzerów */
#define GG_FEATURE_NEW_GG              (0x00000007)          /* w³±cza otrzymywanie pakietów zgodnych z "Nowym GG" */
#define GG_FEATURE_DND_FFC             (0x00000010)          /* status "PoGGadaj ze mn±" i "Nie przeszkadzaæ" */
#define GG_FEATURE_IMAGE_DESCR         (0x00000020)          /* opisy obrazkowe */
#define GG_FEATURE_UNKNOWN_100         (0x00000100)          /* nieznana, oryginalny klient ustawia */
#define GG_FEATURE_USER_DATA           (0x00000200)          /* dodatkowe dane o kontaktach, np. avatary */
#define GG_FEATURE_MSG_ACK             (0x00000400)          /* potwierdzenia wiadomo¶ci */
#define GG_FEATURE_TYPING_NOTIFICATION (0x00002000)          /* powiadomienia o pisaniu */
#define GG_FEATURE_MULTILOGON          (0x00004000)          /* multilogowanie do sieci */

#define GGLIB_FEATURES (GG_FEATURE_NEW_GG | GG_FEATURE_DND_FFC | GG_FEATURE_IMAGE_DESCR | GG_FEATURE_UNKNOWN_100 | GG_FEATURE_USER_DATA | GG_FEATURE_MSG_ACK | GG_FEATURE_TYPING_NOTIFICATION | GG_FEATURE_MULTILOGON)

/* flagi statusów */
#define GG_STATUS_FLAG_UNKNOWN   (0x00000001)                /* nieznana, ale musi wystapiæ */
#define GG_STATUS_FLAG_SPAM      (0x00800000)                /* w³±cza filtr antyspamowy */

#define GGLIB_STATUS_FLAGS (GG_STATUS_FLAG_UNKNOWN | GG_STATUS_FLAG_SPAM)

/* wersje protoko³u */
#define GGLIB_DEFAULT_PROTOCOL_VERSION (0x40)                /* jedna, jedyna obs³ugiwana, domy¶lna */
#define GGLIB_DEFAULT_CLIENT_NAME      "KwaKwa "
#define GGLIB_DEFAULT_CLIENT_VERSION   "1.9"

/* definicje zgodne z oryginalnym klientem GG (Gadu-Gadu 10) */
//#define GGLIB_DEFAULT_CLIENT_NAME      "Gadu-Gadu Client Build "
//#define GGLIB_DEFAULT_CLIENT_VERSION   "10.1.0.11070"

/****d* ggdefs.h/GG_STATUS_#?
 *
 *  NAME
 *    GG_STATUS_#?
 *
 *  FUNCTION
 *    Definicje sta³ych u¿ywanych przez bibliotekê
 *    do oznaczania ró¿nych statusów.
 *
 *  SOURCE
 */

#define GG_STATUS_NOT_AVAIL         (0x0001)
#define GG_STATUS_FFC               (0x0017)
#define GG_STATUS_AVAIL             (0x0002)
#define GG_STATUS_BUSY              (0x0003)
#define GG_STATUS_DND               (0x0021)
#define GG_STATUS_INVISIBLE         (0x0014)
#define GG_STATUS_BLOCKED           (0x0006)

/**********GG_STATUS_#?*/


/****d* ggdefs.h/GG_S_#?
 *
 *  NAME
 *    GG_S_#?
 *
 *  FUNCTION
 *    Makra s³u¿±ce do sprawdzania statusu.
 *
 *  SOURCE
 */

#define GG_S_MASK                   0xFF
#define GG_S(x)                     ((x) & GG_S_MASK)

#define GG_S_NOT_AVAIL(x)           (GG_S(x) == GG_STATUS_NOT_AVAIL || GG_S(x) == GG_STATUS_NOT_AVAIL_DESCR)
#define GG_S_FFC(x)                 (GG_S(x) == GG_STATUS_FFC || GG_S(x) == GG_STATUS_FFC_DESCR)
#define GG_S_AVAIL(x)               (GG_S(x) == GG_STATUS_AVAIL || GG_S(x) == GG_STATUS_AVAIL_DESCR)
#define GG_S_BUSY(x)                (GG_S(x) == GG_STATUS_BUSY || GG_S(x) == GG_STATUS_BUSY_DESCR)
#define GG_S_DND(x)                 (GG_S(x) == GG_STATUS_DND || GG_S(x) == GG_STATUS_DND_DESCR)
#define GG_S_INVISIBLE(x)           (GG_S(x) == GG_STATUS_INVISIBLE || GG_S(x) == GG_STATUS_INVISIBLE_DESCR)
#define GG_S_BLOCKED(x)             (GG_S(x) == GG_STATUS_BLOCKED)

/**********GG_S_#?*/



/* wewnêtrzne dla biblioteki */
#define GG_STATUS_INVISIBLE_DESCR   (0x0016)
#define GG_STATUS_DND_DESCR         (0x0022)
#define GG_STATUS_BUSY_DESCR        (0x0005)
#define GG_STATUS_AVAIL_DESCR       (0x0004)
#define GG_STATUS_FFC_DESCR         (0x0018)
#define GG_STATUS_NOT_AVAIL_DESCR   (0x0015)

/****d* ggdefs.h/GG_USER_#?
 *
 *  NAME
 *    GG_USER_#?
 *
 *  FUNCTION
 *    Definicje sta³ych opisuj±cych u¿ytkownika z listy kontaktów.
 *
 *  ATTRIBUTES
 *    - GG_USER_OFFLINE -- dla danego kontaktu u¿ytkownik chce byæ niewidoczny;
 *    - GG_USER_NORMAL -- normalne zachowanie wobec tego kontaktu;
 *    - GG_USER_BLOCKED -- od danego kontaktu u¿ytkownik nie chce dostawaæ wiadomo¶ci.
 *
 ******/

#define GG_USER_OFFLINE				(0x01)
#define GG_USER_NORMAL				(0x03)
#define GG_USER_BLOCKED				(0x04)

/****d* ggdefs.h/GG_LIST_FORMAT_#?
 *
 *  NAME
 *    GG_LIST_FORMAT_#?
 *
 *  FUNCTION
 *    Definicje sta³ych opisuj±cych format listy kontaktów przy imporcie i eksporcie.
 *
 *  ATTRIBUTES
 *    - GG_LIST_FORMAT_OLD -- stary format listy kontaktów (Gadu-Gadu 7);
 *    - GG_LIST_FORMAT_XML -- lista kontaktów w formacie XML (GG 10 i nowsze).
 *
 ******/

#define GG_LIST_FORMAT_OLD       (0x01)
#define GG_LIST_FORMAT_XML       (0x02)

#endif /* __GGDEFS_H__ */
