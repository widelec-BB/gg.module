/*
 * Copyright (c) 2013 - 2022 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* GGLib/gglib.h
 *
 *  NAME
 *    gglib.h -- G³ówny plik nag³ówkowy biblioteki.
 *
 *  AUTHOR
 *    Filip Maryjañski
 *
 *  DESCRIPTION
 *    Plik definiuje wszystko co jest niezbêdne, aby móc u¿yæ biblioteki
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

/* Definicje tagów, które rozpoznaje funkcja GGCreateSessionTagList() */

#define GGA_CreateSession_Status          (TAG_USER + 1)
#define GGA_CreateSession_Status_Desc     (TAG_USER + 2)
#define GGA_CreateSession_Image_Size      (TAG_USER + 3)

/****d* gglib.h/GGS_ERRNO_#?
 *
 *  NAME
 *    GGS_ERRNO_#?
 *
 *  FUNCTION
 *    Definicje warto¶ci pola ggs_Errno w strukturze GGSession.
 *    Je¶li b³±d nie zostanie obs³u¿ony wewn±trz biblioteki
 *    zostanie wygenerowane zdarzenie GGE_TYPE_ERROR, w którym
 *    w polu ggee_Errno bêdzie znajdowa³ siê jeden z poni¿szych kodów.
 *
 *  ATTRIBUTES
 *    - GGS_ERRNO_OK -- wszystko w porz±dku (nie wyst±pi³ ¿aden b³±d);
 *    - GGS_ERRNO_MEM -- brak pamiêci;
 *    - GGS_ERRNO_SOCKETLIB -- b³±d bsdsocket.library;
 *    - GGS_ERRNO_HUB_FAILED -- b³±d po³±czenia z HUBem GG;
 *    - GGS_ERRNO_INTERRUPT -- wyst±pi³ sygna³ przerwania (CTRL-C);
 *    - GGS_ERRNO_SERVER_OFF -- po³±czenie zosta³o zerwane;
 *    - GGS_ERRNO_TRY_AGAIN -- chwilowy b³±d, spróbuj ponownie;
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
 *    Definicje warto¶ci pola ggs_SessionState w strukturze GGSession.
 *    Ka¿dy z poni¿szych stanów powinen zostaæ obs³u¿ony przez odpowiedni
 *    handler z pliku gghandlers.c.
 *
 *  ATTRIBUTES
 *    - GGS_STATE_ERROR -- b³±d, nale¿y sprawdziæ warto¶æ pola ggs_Errno w celu ustalenia kodu (GGS_ERRNO_#?);
 *    - GGS_STATE_DISCONNECTED -- biblioteka nie nawi±za³a jeszcze po³±czenia;
 *    - GGS_STATE_CONNECTING -- rozpoczêto nawi±zywanie asynchronicznego po³±czenia z serwerem;
 *    - GGS_STATE_CONNECTED -- po³±czono z serwerem;
 *    - GGS_STATE_DISCONNECTING -- po³±czenie zosta³o zerwane, nale¿y wywo³aæ GGWatchEvent().
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

/* warto¶ci pola ggsCheck w struct GGSession */
#define GGS_CHECK_NONE           (0)      /* biblioteka nic nie chce */
#define GGS_CHECK_READ           (1<<0)   /* biblioteka chce czytaæ */
#define GGS_CHECK_WRITE          (1<<1)   /* biblioteka chce zapisywaæ */

/****f* gglib.h/GG_SESSION_CHECK_READ()
 *
 *  NAME
 *    GG_SESSION_CHECK_READ()
 *
 *  SYNOPSIS
 *    GG_SESSION_CHECK_READ(struct GGSession *gg_sess)
 *
 *  FUNCTION
 *    Makro sprawdza czy biblioteka chce czytaæ z socketu.
 *
 *  INPUTS
 *    gg_sess - wska¼nik na strukturê sesji do sprawdzenia.
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
 *    Makro sprawdza czy biblioteka chce zapisywaæ do socketu.
 *
 *  INPUTS
 *    gg_sess - wska¼nik na strukturê sesji do sprawdzenia.
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
 *    Struktura przechowuj±ca aktualny stan sesji GG.
 *
 *  ATTRIBUTES
 *    - ggs_Uin -- numer GG;
 *    - ggs_Status -- aktualny status GG;
 *    - ggs_StatusDescription -- aktualny opis statusu;
 *    - ggs_Pass - has³o (uwaga: zapisane otwartym tekstem!);
 *    - ggs_ImageSize - maksymalny rozmiar odbieranych obrazków;
 *    - ggs_Ip -- ip serwera, do którego ³±czymy siê;
 *    - ggs_Errno -- kod b³êdu;
 *    - ggs_SessionState -- status po³±czenia;
 *    - ggs_Socket -- socket po³±czenia;
 *    - ggs_RecvBuffer -- bufor na dane wysy³ane;
 *    - ggs_RecvLen -- ilo¶æ ju¿ odebranych danych;
 *    - ggs_WriteBuffer -- bufor na dane wysy³ane;
 *    - ggs_WriteLen -- aktualna d³ugo¶æ bufora wysy³anych danych;
 *    - ggs_WrittenLen -- ilo¶æ danych z bufora wysy³ania, która zosta³a ju¿ wys³ana;
 *    - ggs_Check -- pole bitowe informuj±ce czy biblioteka chce
 *      czytaæ czy pisaæ do socketu;
 *    - SocketBase -- wska¼nik na bazê bsdsocket.library.
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
 *    Makro ustawia sesjê w stan b³êdu oraz ustawia ggs_Errno na podan± warto¶æ.
 *
 *  INPUTS
 *    gg_sess - wska¼nik na strukturê sesji.
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
 *    Makro sprawdza czy sesja jest po³±czona z serwerem.
 *
 *  INPUTS
 *    gg_sess -- wska¼nik na strukturê sesji do sprawdzenia.
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
 *    Struktura przechowuj±ca atrybut opisuj±cy kontakt.
 *
 *  ATTRIBUTES
 *    - gguda_Type -- typ atrybutu;
 *    - gguda_Key -- nazwa atrybutu;
 *    - gguda_Value -- warto¶æ atrybutu.
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
 *    Struktura przechowuj±ca atrybuty opisuj±ce dany kontakt.
 *
 *  ATTRIBUTES
 *    - ggudd_Uin -- numer opisywanego kontaktu;
 *    - ggudd_AttrsNo -- ilo¶æ atrybutów;
 *    - ggudd_Attrs -- tablica struktur opisuj±cych atrybut.
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
 *    Struktura przechowuj±ca atrybuty opisuj±ce inne równoleg³e po³±czenie.
 *
 *  ATTRIBUTES
 *    - ggmi_Ip -- IP po³±czenia;
 *    - ggmi_Flags -- flagi jakie poda³ klient przy po³±czeniu;
 *    - ggmi_Features -- obs³ugiwane przez klienta opcje protoko³u;
 *    - ggmi_Timestamp -- czas zalogowania siê klienta;
 *    - ggmi_Id -- identyfikator po³±czenia;
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
 *    Definicje rodzajów zdarzeñ jakie mo¿e wygenerowaæ biblioteka.
 *
 *  ATTRIBUTES
 *    - GGE_TYPE_ERROR -- wyst±pi³ b³±d;
 *    - GGE_TYPE_NOOP -- nie wydarzy³o siê nic istotnego;
 *    - GGE_TYPE_CONNECTED -- uda³o siê nawi±zaæ po³±czenie z serwerem;
 *    - GGE_TYPE_DISCONNECT -- po³±czenie zosta³o zerwane;
 *    - GGE_TYPE_LOGIN_SUCCESS -- logowanie powiod³o siê;
 *    - GGE_TYPE_LOGIN_FAIL -- logowanie nie powiod³o siê (z³e has³o?);
 *    - GGE_TYPE_STATUS_CHANGE -- zmiana statusu na li¶cie kontaktów;
 *    - GGE_TYPE_LIST_STATUS -- zmiana wiêcej ni¿ jednego statusu na li¶cie kontaktów;
 *    - GGE_TYPE_TYPING_NOTIFY -- otrzymano powiadomienie o pisaniu;
 *    - GGE_TYPE_RECV_MSG -- otrzymano now± wiadomo¶æ;
 *    - GGE_TYPE_USER_DATA -- otrzymano dodatkowe dane dotycz±ce kontaktów;
 *    - GGE_TYPE_LIST_IMPORT -- otrzymano listê kontaktów od serwera;
 *    - GGE_TYPE_LIST_EXPORT -- wys³anie listy kontaktów do serwera;
 *    - GGE_TYPE_MULTILOGON_INFO -- otrzymano informacje o równolegle zalogowanych klientach.
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
 *    podanej wcze¶niej przez GGNotifyList().
 *
 *  ATTRIBUTES
 *    - ggesc_Uin -- numer GG u¿ytkownika zmieniaj±cego status;
 *    - ggesc_Status -- nowy status;
 *    - ggesc_ImageSize -- maksymalny rozmiar odbieranych przez u¿ytkownika obrazków;
 *    - ggesc_Description -- opis statusu (NULL je¶li brak).
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
 *    Struktura opisuje zdarzenie odebrania statusów kontaktów z listy.
 *
 *  ATTRIBUTES
 *    - ggels_ChangesNo -- d³ugo¶æ tablicy ggels_StatusChanges;
 *    - ggels_StatusChanges - tablica struktur opisuj±cych zmiany statusu.
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
 *    Struktura opisuje zdarzenie wyst±pienia b³êdu.
 *
 *  ATTRIBUTES
 *    ggee_Errno -- kod b³êdu.
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
 *    - ggetn_Uin -- uin u¿ytkownika, który pisze do nas wiadomo¶æ;
 *    - ggetn_Length -- d³ugo¶æ tekstu wpisana w pole edycji.
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
 *    Struktura opisuje zdarzenie odebrania nowej wiadomo¶ci.
 *
 *  ATTRIBUTES
 *    - ggerm_Uin -- uin nadawcy;
 *    - ggerm_Time -- czas nadania wiadomo¶ci (UTC, Amiga era);
 *    - ggerm_Flags -- flagi wiadomo¶ci:
 *     - GG_MSG_NORMAL -- zwyk³a wiadomo¶æ;
 *     - GG_MSG_OWN -- wiadomo¶æ wys³ana z innego, równocze¶nie zalogowanego klienta
 *    - ggerm_Txt -- tre¶æ wiadomo¶ci (czysty tekst, kodowanie UTF-8,
 *     mo¿e byæ NULL w przypadku samego obrazka);
 *    - ggerm_ImageId -- identyfikator obrazka (je¶li wyst±pi³ w wiadomo¶ci).
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
 *    - ggeud_UsersNo -- ilo¶æ opisywanych kontaktów;
 *    - ggeud_Data -- wska¼nik na tablicê struktur opisuj±cych dane o kontakcie.
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
 *    Struktura opisuje zdarzenie odebrania listy kontaktów od serwera.
 *
 *  ATTRIBUTES
 *    - ggeil_Version -- wersja listy kontaktów;
 *    - ggeil_Format -- typ formatu listy kontaktów;
 *    - ggeil_Data -- lista kontaktów.
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
 *    Struktura opisuje zdarzenie odebrania potwierdzenia eksportu listy kontaktów.
 *
 *  ATTRIBUTES
 *    - ggeil_Version -- wersja listy kontaktów;
 *    - ggeil_Accept -- TRUE je¶li lista zosta³a zaakceptowana przez serwer, FALSE w.p.p..
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
 *    Struktura opisuje zdarzenie odebrania informacji o innych równolegle zalogowanych klientach.
 *
 *  ATTRIBUTES
 *    - ggemi_No -- ilo¶æ równolegle zalogowanych klientów;
 *    - ggemi_Data -- wska¼nik na tablicê struktur opisuj±cych równolegle zalogowane klienty.
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
 *    Struktura opisuje zdarzenie odebrania danych ¿±danego obrazka.
 *
 *  ATTRIBUTES
 *    - ggeid_Uin -- numer, od którego otrzymali¶my dane obrazka;
 *    - ggeid_Type -- pole opisuj±ce szczegó³y dotycz±ce zdarzenia:
 *     - GG_IMAGE_START_DATA -- pierwsza porcja danych obrazka;
 *     - GG_IMAGE_NEXT_DATA -- kolejne porcje danych obrazka;
 *    - ggeid_ImageSize -- rozmiar obrazka;
 *    - ggeid_Crc32 -- suma kontrolna obrazka (CRC32);
 *    - ggeid_FileName -- wska¼nik na nazwê pliku (tylko przy pierwszej porcji danych obrazka);
 *    - ggeid_DataSize -- rozmiar pola wskazywanego przez ggeid_Data;
 *    - ggeid_Data -- wska¼nik na zawarto¶æ obrazka.
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
 *    Struktura opisuje zdarzenie odebrania ¿±dania obrazka.
 *
 *  ATTRIBUTES
 *    - geeid_Uin -- numer, od którego odebrano ¿±danie obrazka;
 *    - ggeid_ImageSize -- rozmiar ¿±danego obrazka;
 *    - ggeid_Crc32 -- suma kontrolna ¿±danego obrazka (CRC32).
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
 *    - geepdi_Uin -- numer, którego dotycz± dane;
 *    - ggepdi_FirstName -- imiê kontaktu;
 *    - ggepdi_LastName -- nazwisko kontaktu;
 *    - ggepdi_NickName -- nick kontaktu;
 *    - ggepdi_BirthYear -- rok urodzenia kontaktu;
 *    - ggepdi_City -- miejsce zamieszkania kontaktu;
 *    - ggpedi_Gender -- p³eæ kontaktu;
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
 *    Struktura opisuj±ca aktualne zdarzenie.
 *
 *  ATTRIBUTES
 *    - gge_Type -- typ zdarzenia;
 *    - gge_Event -- unia struktur opisuj±cych ró¿ne rodzaje zdarzeñ
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
