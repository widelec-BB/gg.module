/*
 * Copyright (c) 2013 - 2022 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* GGLib/gglib.h
 *
 *  NAME
 *    gglib.h -- G��wny plik nag��wkowy biblioteki.
 *
 *  AUTHOR
 *    Filip Maryja�ski
 *
 *  DESCRIPTION
 *    Plik definiuje wszystko co jest niezb�dne, aby m�c u�y� biblioteki
 *    w programie.
 *
 ********/

#ifndef __GGLIB_H__
#define __GGLIB_H__

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#include "ggdefs.h"

/* Definicje tag�w, kt�re rozpoznaje funkcja GGCreateSessionTagList() */

#define GGA_CreateSession_Status          (TAG_USER + 1)
#define GGA_CreateSession_Status_Desc     (TAG_USER + 2)
#define GGA_CreateSession_Image_Size      (TAG_USER + 3)

/****d* gglib.h/GGS_ERRNO_#?
 *
 *  NAME
 *    GGS_ERRNO_#?
 *
 *  FUNCTION
 *    Definicje warto�ci pola ggs_Errno w strukturze GGSession.
 *    Je�li b��d nie zostanie obs�u�ony wewn�trz biblioteki
 *    zostanie wygenerowane zdarzenie GGE_TYPE_ERROR, w kt�rym
 *    w polu ggee_Errno b�dzie znajdowa� si� jeden z poni�szych kod�w.
 *
 *  ATTRIBUTES
 *    - GGS_ERRNO_OK -- wszystko w porz�dku (nie wyst�pi� �aden b��d);
 *    - GGS_ERRNO_MEM -- brak pami�ci;
 *    - GGS_ERRNO_SOCKETLIB -- b��d bsdsocket.library;
 *    - GGS_ERRNO_HUB_FAILED -- b��d po��czenia z HUBem GG;
 *    - GGS_ERRNO_INTERRUPT -- wyst�pi� sygna� przerwania (CTRL-C);
 *    - GGS_ERRNO_SERVER_OFF -- po��czenie zosta�o zerwane;
 *    - GGS_ERRNO_TRY_AGAIN -- chwilowy b��d, spr�buj ponownie;
 *    - GGS_ERRNO_UNKNOWN_PACKET -- nieznany pakiet.
 *
 *  SEE ALSO
 *    GGSession, GGEventError
 *
 *  SOURCE
 */

#define GGS_ERRNO_OK             (0)
#define GGS_ERRNO_MEM            (1)
#define GGS_ERRNO_SOCKET_LIB     (2)
#define GGS_ERRNO_HUB_FAILED     (3)
#define GGS_ERRNO_INTERRUPT      (4)
#define GGS_ERRNO_SERVER_OFF     (5)
#define GGS_ERRNO_TRYAGAIN       (6)
#define GGS_ERRNO_UNKNOWN_PACKET (7)

/*********GGS_ERRNO_#?*******/

/****id* gglib.h/GGS_STATE_#?
 *
 *  NAME
 *    GGS_STATE_#?
 *
 *  FUNCTION
 *    Definicje warto�ci pola ggs_SessionState w strukturze GGSession.
 *    Ka�dy z poni�szych stan�w powinen zosta� obs�u�ony przez odpowiedni
 *    handler z pliku gghandlers.c.
 *
 *  ATTRIBUTES
 *    - GGS_STATE_ERROR -- b��d, nale�y sprawdzi� warto�� pola ggs_Errno w celu ustalenia kodu (GGS_ERRNO_#?);
 *    - GGS_STATE_DISCONNECTED -- biblioteka nie nawi�za�a jeszcze po��czenia;
 *    - GGS_STATE_CONNECTING -- rozpocz�to nawi�zywanie asynchronicznego po��czenia z serwerem;
 *    - GGS_STATE_CONNECTED -- po��czono z serwerem;
 *    - GGS_STATE_DISCONNECTING -- po��czenie zosta�o zerwane, nale�y wywo�a� GGWatchEvent().
 *
 *  SEE ALSO
 *    GGSession
 *
 *  SOURCE
 */

#define GGS_STATE_ERROR          (-1)
#define GGS_STATE_DISCONNECTED   (0)
#define GGS_STATE_CONNECTING     (1)
#define GGS_STATE_CONNECTED      (2)
#define GGS_STATE_DISCONNECTING  (3)

/************GGS_STATE_#?***********/

/* warto�ci pola ggsCheck w struct GGSession */
#define GGS_CHECK_NONE           (0)      /* biblioteka nic nie chce */
#define GGS_CHECK_READ           (1<<0)   /* biblioteka chce czyta� */
#define GGS_CHECK_WRITE          (1<<1)   /* biblioteka chce zapisywa� */

/****f* gglib.h/GG_SESSION_CHECK_READ()
 *
 *  NAME
 *    GG_SESSION_CHECK_READ()
 *
 *  SYNOPSIS
 *    GG_SESSION_CHECK_READ(struct GGSession *gg_sess)
 *
 *  FUNCTION
 *    Makro sprawdza czy biblioteka chce czyta� z socketu.
 *
 *  INPUTS
 *    gg_sess - wska�nik na struktur� sesji do sprawdzenia.
 *
 *  SOURCE
 */

#define GG_SESSION_CHECK_READ(gg_sess) ((gg_sess)->ggs_Check & GGS_CHECK_READ)

/*******GG_SESSION_CHECK_READ()*******/

/****f* gglib.h/GG_SESSION_CHECK_WRITE()
 *
 *  NAME
 *    GG_SESSION_CHECK_WRITE()
 *
 *  SYNOPSIS
 *    GG_SESSION_CHECK_WRITE(struct GGSession *gg_sess)
 *
 *  FUNCTION
 *    Makro sprawdza czy biblioteka chce zapisywa� do socketu.
 *
 *  INPUTS
 *    gg_sess - wska�nik na struktur� sesji do sprawdzenia.
 *
 *  SOURCE
 */

#define GG_SESSION_CHECK_WRITE(gg_sess) ((gg_sess)->ggs_Check & GGS_CHECK_WRITE)

/*******GG_SESSION_CHECK_WRITE()*******/

/****s* gglib.h/GGSession
 *
 *  NAME
 *    GGSession
 *
 *  FUNCTION
 *    Struktura przechowuj�ca aktualny stan sesji GG.
 *
 *  ATTRIBUTES
 *    - ggs_Uin -- numer GG;
 *    - ggs_Status -- aktualny status GG;
 *    - ggs_StatusDescription -- aktualny opis statusu;
 *    - ggs_Pass - has�o (uwaga: zapisane otwartym tekstem!);
 *    - ggs_ImageSize - maksymalny rozmiar odbieranych obrazk�w;
 *    - ggs_Ip -- ip serwera, do kt�rego ��czymy si�;
 *    - ggs_Errno -- kod b��du;
 *    - ggs_SessionState -- status po��czenia;
 *    - ggs_Socket -- socket po��czenia;
 *    - ggs_RecvBuffer -- bufor na dane wysy�ane;
 *    - ggs_RecvLen -- ilo�� ju� odebranych danych;
 *    - ggs_WriteBuffer -- bufor na dane wysy�ane;
 *    - ggs_WriteLen -- aktualna d�ugo�� bufora wysy�anych danych;
 *    - ggs_WrittenLen -- ilo�� danych z bufora wysy�ania, kt�ra zosta�a ju� wys�ana;
 *    - ggs_Check -- pole bitowe informuj�ce czy biblioteka chce
 *      czyta� czy pisa� do socketu;
 *    - SocketBase -- wska�nik na baz� bsdsocket.library.
 *
 *  SEE ALSO
 *    GGS_ERRNO_#?, GGS_STATE_#?, GG_SESSION_CHECK_WRITE(), GG_SESSION_CHECK_READ()
 *
 *  SOURCE
 */

struct GGSession
{
	ULONG ggs_Uin;
	ULONG ggs_Status;
	STRPTR ggs_StatusDescription;
	STRPTR ggs_Pass;
	BYTE ggs_ImageSize;

	ULONG ggs_Ip;
	USHORT ggs_Port;
	ULONG ggs_Errno;
	LONG ggs_SessionState;
	LONG ggs_Socket;
	BYTE *ggs_RecvBuffer;
	LONG ggs_RecvLen;
	BYTE *ggs_WriteBuffer;
	LONG ggs_WriteLen;
	LONG ggs_WrittenLen;
	LONG ggs_Check;
	struct Library* SocketBase;
};

/********GGSession****/

/****if* gglib.h/GG_SESSION_ERROR()
 *
 *  NAME
 *    GG_SESSION_ERROR()
 *
 *  SYNOPSIS
 *    GG_SESSION_ERROR(struct GGSession *gg_sess, ULONG errno)
 *
 *  FUNCTION
 *    Makro ustawia sesj� w stan b��du oraz ustawia ggs_Errno na podan� warto��.
 *
 *  INPUTS
 *    gg_sess - wska�nik na struktur� sesji.
 *
 *  SEE ALSO
 *    GGS_STATE_#?, GGS_ERRNO_#?
 *
 *  SOURCE
 */

#define GG_SESSION_ERROR(gg_sess, error) do{tprintf("session error: %ld\n", error); (gg_sess)->ggs_SessionState = GGS_STATE_ERROR; (gg_sess)->ggs_Errno = error;}while(0)

/*******GG_SESSION_ERROR********/

/****if* gglib.h/GG_SESSION_IS_CONNECTED()
 *
 *  NAME
 *    GG_SESSION_IS_CONNECTED()
 *
 *  SYNOPSIS
 *    GG_SESSION_IS_CONNECTED(struct GGSession *gg_sess)
 *
 *  FUNCTION
 *    Makro sprawdza czy sesja jest po��czona z serwerem.
 *
 *  INPUTS
 *    gg_sess -- wska�nik na struktur� sesji do sprawdzenia.
 *
 *  SEE ALSO
 *    GGS_STATE_#?
 *
 *  SOURCE
 */

#define GG_SESSION_IS_CONNECTED(gg_sess) ((gg_sess)->ggs_SessionState == GGS_STATE_CONNECTED)

/*******GG_SESSION_IS_CONNECTED********/


/****s* gglib.h/GGUserDataAttr
 *
 *  NAME
 *    GGUserDataAttr
 *
 *  FUNCTION
 *    Struktura przechowuj�ca atrybut opisuj�cy kontakt.
 *
 *  ATTRIBUTES
 *    - gguda_Type -- typ atrybutu;
 *    - gguda_Key -- nazwa atrybutu;
 *    - gguda_Value -- warto�� atrybutu.
 *
 *  SEE ALSO
 *    GGE_TYPE_#?, GGUserData, GGEventUsersData
 *
 *  SOURCE
 */

struct GGUserDataAttr
{
	LONG gguda_Type;
	STRPTR gguda_Key;
	STRPTR gguda_Value;
};

/********GGUserDataAttr****/

/****s* gglib.h/GGUserData
 *
 *  NAME
 *    GGUserData
 *
 *  FUNCTION
 *    Struktura przechowuj�ca atrybuty opisuj�ce dany kontakt.
 *
 *  ATTRIBUTES
 *    - ggudd_Uin -- numer opisywanego kontaktu;
 *    - ggudd_AttrsNo -- ilo�� atrybut�w;
 *    - ggudd_Attrs -- tablica struktur opisuj�cych atrybut.
 *
 *  SEE ALSO
 *    GGE_TYPE_#?, GGUserDataAttr, GGEventUsersData
 *
 *  SOURCE
 */

struct GGUserData
{
	ULONG ggud_Uin;
	ULONG ggud_AttrsNo;
	struct GGUserDataAttr *ggud_Attrs;
};

/********GGUserData****/

/****s* gglib.h/GGMultilogonInfo
 *
 *  NAME
 *    GGMultilogonInfo
 *
 *  FUNCTION
 *    Struktura przechowuj�ca atrybuty opisuj�ce inne r�wnoleg�e po��czenie.
 *
 *  ATTRIBUTES
 *    - ggmi_Ip -- IP po��czenia;
 *    - ggmi_Flags -- flagi jakie poda� klient przy po��czeniu;
 *    - ggmi_Features -- obs�ugiwane przez klienta opcje protoko�u;
 *    - ggmi_Timestamp -- czas zalogowania si� klienta;
 *    - ggmi_Id -- identyfikator po��czenia;
 *    - ggmi_Name -- nazwa klienta.
 *
 *  SEE ALSO
 *    GGE_TYPE_#?, GGEventMultilogonInfo
 *
 *  SOURCE
 */

struct GGMultilogonInfo
{
	ULONG  ggmi_Ip;
	ULONG  ggmi_Flags;
	ULONG  ggmi_Features;
	ULONG  ggmi_Timestamp;
	UQUAD  ggmi_Id;
	STRPTR ggmi_Name;
};

/********GGMultilogonInfo****/

/****d* gglib.h/GGE_TYPE_#?
 *
 *  NAME
 *    GGE_TYPE_#?
 *
 *  FUNCTION
 *    Definicje rodzaj�w zdarze� jakie mo�e wygenerowa� biblioteka.
 *
 *  ATTRIBUTES
 *    - GGE_TYPE_ERROR -- wyst�pi� b��d;
 *    - GGE_TYPE_NOOP -- nie wydarzy�o si� nic istotnego;
 *    - GGE_TYPE_CONNECTED -- uda�o si� nawi�za� po��czenie z serwerem;
 *    - GGE_TYPE_DISCONNECT -- po��czenie zosta�o zerwane;
 *    - GGE_TYPE_LOGIN_SUCCESS -- logowanie powiod�o si�;
 *    - GGE_TYPE_LOGIN_FAIL -- logowanie nie powiod�o si� (z�e has�o?);
 *    - GGE_TYPE_STATUS_CHANGE -- zmiana statusu na li�cie kontakt�w;
 *    - GGE_TYPE_LIST_STATUS -- zmiana wi�cej ni� jednego statusu na li�cie kontakt�w;
 *    - GGE_TYPE_TYPING_NOTIFY -- otrzymano powiadomienie o pisaniu;
 *    - GGE_TYPE_RECV_MSG -- otrzymano now� wiadomo��;
 *    - GGE_TYPE_USER_DATA -- otrzymano dodatkowe dane dotycz�ce kontakt�w;
 *    - GGE_TYPE_LIST_IMPORT -- otrzymano list� kontakt�w od serwera;
 *    - GGE_TYPE_LIST_EXPORT -- wys�anie listy kontakt�w do serwera;
 *    - GGE_TYPE_MULTILOGON_INFO -- otrzymano informacje o r�wnolegle zalogowanych klientach.
 *
 *  SOURCE
 */

#define GGE_TYPE_ERROR           (-1)
#define GGE_TYPE_NOOP            (0)
#define GGE_TYPE_CONNECTED       (1)
#define GGE_TYPE_DISCONNECT      (2)
#define GGE_TYPE_LOGIN_SUCCESS   (3)
#define GGE_TYPE_LOGIN_FAIL      (4)
#define GGE_TYPE_STATUS_CHANGE   (5)
#define GGE_TYPE_LIST_STATUS     (6)
#define GGE_TYPE_TYPING_NOTIFY   (7)
#define GGE_TYPE_RECV_MSG        (8)
#define GGE_TYPE_USER_DATA       (9)
#define GGE_TYPE_LIST_IMPORT     (10)
#define GGE_TYPE_LIST_EXPORT     (11)
#define GGE_TYPE_MULTILOGON_INFO (12)
#define GGE_TYPE_IMAGE_DATA      (13)
#define GGE_TYPE_IMAGE_REQUEST   (14)
#define GGE_TYPE_PUBDIR_INFO     (15)

/*********GGE_TYPE_#?*****************/

/****s* gglib.h/GGEventStatusChange
 *
 *  NAME
 *    GGEventStatusChange
 *
 *  FUNCTION
 *    Struktura opisuje zdarzenie zmiany statusu przez kontakt z listy
 *    podanej wcze�niej przez GGNotifyList().
 *
 *  ATTRIBUTES
 *    - ggesc_Uin -- numer GG u�ytkownika zmieniaj�cego status;
 *    - ggesc_Status -- nowy status;
 *    - ggesc_ImageSize -- maksymalny rozmiar odbieranych przez u�ytkownika obrazk�w;
 *    - ggesc_Description -- opis statusu (NULL je�li brak).
 *
 *  SOURCE
 */

struct GGEventStatusChange
{
	ULONG ggesc_Uin;
	ULONG ggesc_Status;
	UBYTE ggesc_ImageSize;
	STRPTR ggesc_Description;
};

/********GGEventStatusChange****/

/****s* gglib.h/GGEventListStatus
 *
 *  NAME
 *    GGEventListStatus
 *
 *  FUNCTION
 *    Struktura opisuje zdarzenie odebrania status�w kontakt�w z listy.
 *
 *  ATTRIBUTES
 *    - ggels_ChangesNo -- d�ugo�� tablicy ggels_StatusChanges;
 *    - ggels_StatusChanges - tablica struktur opisuj�cych zmiany statusu.
 *
 *  SEE ALSO
 *    GGEventStatusChange, GGPacketHandlerListStatus()
 *
 *  SOURCE
 */

struct GGEventListStatus
{
	LONG ggels_ChangesNo;
	struct GGEventStatusChange *ggels_StatusChanges;
};

/********GGEventListStatus****/

/****s* gglib.h/GGEventError
 *
 *  NAME
 *    GGEventError
 *
 *  FUNCTION
 *    Struktura opisuje zdarzenie wyst�pienia b��du.
 *
 *  ATTRIBUTES
 *    ggee_Errno -- kod b��du.
 *
 *  SEE ALSO
 *    GGS_ERRNO_#?
 *
 *  SOURCE
 */

struct GGEventError
{
	LONG ggee_Errno;    /* kopia ggs_Errno */
};

/********GGEventError****/

/****s* gglib.h/GGEventTypingNotify
 *
 *  NAME
 *    GGEventTypingNotify
 *
 *  FUNCTION
 *    Struktura opisuje zdarzenie odebrania powiadomienia o pisaniu.
 *
 *  ATTRIBUTES
 *    - ggetn_Uin -- uin u�ytkownika, kt�ry pisze do nas wiadomo��;
 *    - ggetn_Length -- d�ugo�� tekstu wpisana w pole edycji.
 *
 *  SEE ALSO
 *    GGPacketHandlerTypingNotify()
 *
 *  SOURCE
 */

struct GGEventTypingNotify
{
	ULONG ggetn_Uin;
	USHORT ggetn_Length;
};

/********GGEventTypingNotify****/

/****s* gglib.h/GGEventRecvMsg
 *
 *  NAME
 *    GGEventRecvMsg
 *
 *  FUNCTION
 *    Struktura opisuje zdarzenie odebrania nowej wiadomo�ci.
 *
 *  ATTRIBUTES
 *    - ggerm_Uin -- uin nadawcy;
 *    - ggerm_Time -- czas nadania wiadomo�ci (UTC, Amiga era);
 *    - ggerm_Flags -- flagi wiadomo�ci:
 *     - GG_MSG_NORMAL -- zwyk�a wiadomo��;
 *     - GG_MSG_OWN -- wiadomo�� wys�ana z innego, r�wnocze�nie zalogowanego klienta
 *    - ggerm_Txt -- tre�� wiadomo�ci (czysty tekst, kodowanie UTF-8,
 *     mo�e by� NULL w przypadku samego obrazka);
 *    - ggerm_ImageId -- identyfikator obrazka (je�li wyst�pi� w wiadomo�ci).
 *
 *  SEE ALSO
 *    GGPacketHandlerRecvMsg()
 *
 *  SOURCE
 */

#define GG_MSG_NORMAL (0UL)
#define GG_MSG_OWN    (1UL)

struct GGEventRecvMsg
{
	ULONG ggerm_Uin;
	ULONG ggerm_Time;
	ULONG ggerm_Flags;
	STRPTR ggerm_Txt;
	STRPTR ggerm_ImagesIds;
};

/********GGEventRecvMsg****/

/****s* gglib.h/GGEventUsersData
 *
 *  NAME
 *    GGEventUsersData
 *
 *  FUNCTION
 *    Struktura opisuje zdarzenie odebrania dodatkowych danych o kontaktach.
 *
 *  ATTRIBUTES
 *    - ggeud_Type -- typ danych;
 *    - ggeud_UsersNo -- ilo�� opisywanych kontakt�w;
 *    - ggeud_Data -- wska�nik na tablic� struktur opisuj�cych dane o kontakcie.
 *
 *  SEE ALSO
 *    GGPacketHandlerUserData(), GGUserData
 *
 *  SOURCE
 */

struct GGEventUsersData
{
	ULONG ggeud_Type;
	ULONG ggeud_UsersNo;
	struct GGUserData *ggeud_Data;
};

/********GGEventUsersData****/

/****s* gglib.h/GGEventListImport
 *
 *  NAME
 *    GGEventListImport
 *
 *  FUNCTION
 *    Struktura opisuje zdarzenie odebrania listy kontakt�w od serwera.
 *
 *  ATTRIBUTES
 *    - ggeil_Version -- wersja listy kontakt�w;
 *    - ggeil_Format -- typ formatu listy kontakt�w;
 *    - ggeil_Data -- lista kontakt�w.
 *
 *  SEE ALSO
 *    GGPacketHandlerUserListReply(), GG_LIST_FORMAT_#?
 *
 *  SOURCE
 */

struct GGEventListImport
{
	ULONG ggeli_Version;
	UBYTE ggeli_Format;
	STRPTR ggeli_Data;
};

/********GGEventListImport****/

/****s* gglib.h/GGEventListExport
 *
 *  NAME
 *    GGEventListExport
 *
 *  FUNCTION
 *    Struktura opisuje zdarzenie odebrania potwierdzenia eksportu listy kontakt�w.
 *
 *  ATTRIBUTES
 *    - ggeil_Version -- wersja listy kontakt�w;
 *    - ggeil_Accept -- TRUE je�li lista zosta�a zaakceptowana przez serwer, FALSE w.p.p..
 *
 *  SEE ALSO
 *    GGPacketHandlerUserListReply()
 *
 *  SOURCE
 */

struct GGEventListExport
{
	ULONG ggele_Version;
	BOOL ggele_Accept;
};

/********GGEventListExport****/

/****s* gglib.h/GGEventMultilogonInfo
 *
 *  NAME
 *    GGEventMultilogonInfo
 *
 *  FUNCTION
 *    Struktura opisuje zdarzenie odebrania informacji o innych r�wnolegle zalogowanych klientach.
 *
 *  ATTRIBUTES
 *    - ggemi_No -- ilo�� r�wnolegle zalogowanych klient�w;
 *    - ggemi_Data -- wska�nik na tablic� struktur opisuj�cych r�wnolegle zalogowane klienty.
 *
 *  SEE ALSO
 *    GGPacketHandlerMultilogonInfo()
 *
 *  SOURCE
 */

struct GGEventMultilogonInfo
{
	ULONG ggemi_No;
	struct GGMultilogonInfo *ggemi_Data;
};

/********GGEventMultilogonInfo****/

/****s* gglib.h/GGEventImageData
 *
 *  NAME
 *    GGEventImageData
 *
 *  FUNCTION
 *    Struktura opisuje zdarzenie odebrania danych ��danego obrazka.
 *
 *  ATTRIBUTES
 *    - ggeid_Uin -- numer, od kt�rego otrzymali�my dane obrazka;
 *    - ggeid_Type -- pole opisuj�ce szczeg�y dotycz�ce zdarzenia:
 *     - GG_IMAGE_START_DATA -- pierwsza porcja danych obrazka;
 *     - GG_IMAGE_NEXT_DATA -- kolejne porcje danych obrazka;
 *    - ggeid_ImageSize -- rozmiar obrazka;
 *    - ggeid_Crc32 -- suma kontrolna obrazka (CRC32);
 *    - ggeid_FileName -- wska�nik na nazw� pliku (tylko przy pierwszej porcji danych obrazka);
 *    - ggeid_DataSize -- rozmiar pola wskazywanego przez ggeid_Data;
 *    - ggeid_Data -- wska�nik na zawarto�� obrazka.
 *
 *  SEE ALSO
 *    GGPacketHandlerRecvMsg(), GGRequestImage()
 *
 *  SOURCE
 */

#define GG_IMAGE_START_DATA (1UL)
#define GG_IMAGE_NEXT_DATA (2UL)

struct GGEventImageData
{
	ULONG  ggeid_Uin;
	ULONG  ggeid_Type;
	ULONG  ggeid_ImageSize;
	ULONG  ggeid_Crc32;
	STRPTR ggeid_FileName;
	ULONG  ggeid_DataSize;
	UBYTE  *ggeid_Data;
};

/********GGEventImageData****/

/****s* gglib.h/GGEventImageRequest
 *
 *  NAME
 *    GGEventImageRequest
 *
 *  FUNCTION
 *    Struktura opisuje zdarzenie odebrania ��dania obrazka.
 *
 *  ATTRIBUTES
 *    - geeid_Uin -- numer, od kt�rego odebrano ��danie obrazka;
 *    - ggeid_ImageSize -- rozmiar ��danego obrazka;
 *    - ggeid_Crc32 -- suma kontrolna ��danego obrazka (CRC32).
 *
 *  SEE ALSO
 *    GGPacketHandlerRecvMsg(), GGRequestImage()
 *
 *  SOURCE
 */

struct GGEventImageRequest
{
	ULONG ggeir_Uin;
	ULONG ggeir_ImageSize;
	ULONG ggeir_Crc32;
};

/********GGEventImageRequest****/

/****s* gglib.h/GGEventPubDirInfo
 *
 *  NAME
 *    GGEventPubDirInfo
 *
 *  FUNCTION
 *    Struktura opisuje zdarzenie odebrania danych z katalogu publicznego.
 *
 *  ATTRIBUTES
 *    - geepdi_Uin -- numer, kt�rego dotycz� dane;
 *    - ggepdi_FirstName -- imi� kontaktu;
 *    - ggepdi_LastName -- nazwisko kontaktu;
 *    - ggepdi_NickName -- nick kontaktu;
 *    - ggepdi_BirthYear -- rok urodzenia kontaktu;
 *    - ggepdi_City -- miejsce zamieszkania kontaktu;
 *    - ggpedi_Gender -- p�e� kontaktu;
 *    - ggpedi_Status -- aktualny status kontaktu.
 *
 *  SEE ALSO
 *    GGPacketHandlerPubDirInfo(), GGFindInPubDir()
 *
 *  SOURCE
 */

#define GG_INFO_GENDER_UNKNOWN 0
#define GG_INFO_GENDER_MALE 1
#define GG_INFO_GENDER_FEMALE 2

struct GGEventPubDirInfo
{
	ULONG  ggepdi_Seq;
	ULONG  ggepdi_Uin;
	STRPTR ggepdi_FirstName;
	STRPTR ggepdi_LastName;
	STRPTR ggepdi_NickName;
	STRPTR ggepdi_BirthYear;
	STRPTR ggepdi_City;
	ULONG  ggepdi_Gender;
	ULONG  ggepdi_Status;
};

/********GGEventPubDirInfo****/

/****s* gglib.h/GGEvent
 *
 *  NAME
 *    GGEvent
 *
 *  FUNCTION
 *    Struktura opisuj�ca aktualne zdarzenie.
 *
 *  ATTRIBUTES
 *    - gge_Type -- typ zdarzenia;
 *    - gge_Event -- unia struktur opisuj�cych r�ne rodzaje zdarze�
 *
 *  SEE ALSO
 *    GGE_TYPE_#?, GGEventError, GGEventStatusChange
 *
 *  SOURCE
 */

struct GGEvent
{
	LONG gge_Type;
	union GGEvents
	{
		struct GGEventError            gge_Error;
		struct GGEventStatusChange     gge_StatusChange;
		struct GGEventListStatus       gge_ListStatus;
		struct GGEventTypingNotify     gge_TypingNotify;
		struct GGEventRecvMsg          gge_RecvMsg;
		struct GGEventUsersData        gge_UsersData;
		struct GGEventListImport       gge_ListImport;
		struct GGEventListExport       gge_ListExport;
		struct GGEventMultilogonInfo   gge_MultilogonInfo;
		struct GGEventImageData        gge_ImageData;
		struct GGEventImageRequest     gge_ImageRequest;
		struct GGEventPubDirInfo       gge_PubDirInfo;
	} gge_Event;
};

/********GGEvent****/

struct GGSession *GGCreateSessionTagList(ULONG uin, STRPTR password, struct TagItem *taglist);
#ifdef USE_INLINE_STDARG
#define GGCreateSessionTags(uin, password, ...)	({ULONG _tags[] = {__VA_ARGS__}; GGCreateSessionTagList(uin, password, (struct TagItem*)_tags);})
#endif /* USE_INLINE_STDARG */
BOOL GGConnect(struct GGSession *gg_sess, STRPTR server, USHORT port);
struct GGEvent *GGWatchEvent(struct GGSession *gg_sess);
BOOL GGNotifyList(struct GGSession *gg_sess, ULONG *uins, UBYTE *types, LONG no);
BOOL GGChangeStatus(struct GGSession *gg_sess, ULONG status, STRPTR desc);
BOOL GGPing(struct GGSession *gg_sess);
BOOL GGTypingNotify(struct GGSession *gg_sess, ULONG uin, USHORT len);
BOOL GGSendMessage(struct GGSession *gg_sess, ULONG uin, STRPTR msg, STRPTR image);
BOOL GGAddNotify(struct GGSession *gg_sess, ULONG uin, UBYTE type);
BOOL GGRemoveNotify(struct GGSession *gg_sess, ULONG uin, UBYTE type);
BOOL GGRequestContactList(struct GGSession *gg_sess, UBYTE format);
BOOL GGExportContactList(struct GGSession *gg_sess, ULONG ver, UBYTE format, STRPTR list, LONG len);
BOOL GGDisconnectMultilogon(struct GGSession *gg_sess, UQUAD id);
BOOL GGRequestImage(struct GGSession *gg_sess, ULONG uin, STRPTR id);
BOOL GGSendImageData(struct GGSession *gg_sess, ULONG uin, BPTR fh);
ULONG GGFindInPubDir(struct GGSession *gg_sess, ULONG uin);
VOID GGFreeEvent(struct GGEvent *event);
VOID GGFreeSession(struct GGSession *gg_sess);

STRPTR GGCreateImageId(BPTR fh);


#endif /* __GGLIB_H__ */
