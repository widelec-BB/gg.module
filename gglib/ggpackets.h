/*
 * Copyright (c) 2013 - 2022 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* GGLib/ggpackets.h
 *
 *  NAME
 *    ggpackets.h -- Plik zawieraj±cy definicje pakietów sieci GG oraz funkcji je obs³uguj±cych.
 *
 *  AUTHOR
 *    Filip Maryjañski
 *
 *  DESCRIPTION
 *    Deklaracje struktur opisuj±cych pakiety sieci GG.
 *    Deklaracje funkcji przeznaczonych do obs³ugi pakietów sieci GG.
 *
 ********/


#ifndef __GGPACKETS_H__
#define __GGPACKETS_H__

#include "gglib.h"

#ifdef __GNUC__
#define GG_PACKED __attribute__ ((packed))
#define GG_DEPRECATED __attribute__ ((deprecated))
#else
#error Compiler not supported
#endif

/* tagi dla GGCreatePacketTagList() */
#define GGA_CreatePacket_LONG           (TAG_USER + 1)
#define GGA_CreatePacket_SHORT          (TAG_USER + 2)
#define GGA_CreatePacket_BYTE           (TAG_USER + 3)
#define GGA_CreatePacket_ULONG          (TAG_USER + 4)
#define GGA_CreatePacket_USHORT         (TAG_USER + 5)
#define GGA_CreatePacket_UBYTE          (TAG_USER + 6)
#define GGA_CreatePacket_STRPTR         (TAG_USER + 7)
#define GGA_CreatePacket_ZEROIZE        (TAG_USER + 8)
#define GGA_CreatePacket_SHA1           (TAG_USER + 9)
#define GGA_CreatePacket_BLOCK          (TAG_USER + 10)
#define GGA_CreatePacket_QUAD           (TAG_USER + 11)
#define GGA_CreatePacket_UQUAD          (TAG_USER + 12)

/****id* ggpackets.h/GGP_TYPE_#?
 *
 *  NAME
 *    GGP_TYPE_#?
 *
 *  FUNCTION
 *    Typy pakietów sieci GG.
 *
 *  SEE ALSO
 *    GGPHeader
 *
 *  SOURCE
 */

#define GGP_TYPE_WELCOME                (0x0001UL)
#define GGP_TYPE_GGLOGIN                (0x0031UL)
#define GGP_TYPE_LOGIN_OK               (0x0035UL)
#define GGP_TYPE_LOGIN_FAIL             (0x0009UL)
#define GGP_TYPE_LOGIN_FAIL2            (0x0043UL)
#define GGP_TYPE_LIST_EMPTY             (0x0012UL)
#define GGP_TYPE_NOTIFY_NORMAL          (0x000FUL)
#define GGP_TYPE_NOTIFY_LAST            (0x0010UL)
#define GGP_TYPE_STATUS_CHANGE          (0x0036UL)
#define GGP_TYPE_LIST_STATUS            (0x0037UL)
#define GGP_TYPE_NEW_STATUS             (0x0038UL)
#define GGP_TYPE_PING                   (0x0008UL)
#define GGP_TYPE_TYPING_NOTIFY          (0x0059UL)
#define GGP_TYPE_MSG_ACK                (0x0046UL)
#define GGP_TYPE_RECV_MSG               (0x002EUL)
#define GGP_TYPE_RECV_OWN_MSG           (0x005AUL)
#define GGP_TYPE_SEND_MSG               (0x002DUL)
#define GGP_TYPE_SEND_MSG_OLD           (0x000BUL) /* ze starej wersji protoko³u, nadal u¿ywane przy przesy³aniu obrazków */
#define GGP_TYPE_ADD_NOTIFY             (0x000DUL)
#define GGP_TYPE_REMOVE_NOTIFY          (0x000EUL)
#define GGP_TYPE_USER_DATA              (0x0044UL)
#define GGP_TYPE_USER_LIST_REQ          (0x0040UL)
#define GGP_TYPE_USER_LIST_REPLY        (0x0041UL)
#define GGP_TYPE_MULTILOGON_INFO        (0x005BUL)
#define GGP_TYPE_MULTILOGON_DISCONNECT  (0x0062UL)
#define GGP_TYPE_PUBDIR_REQUEST         (0x0014UL)
#define GGP_TYPE_PUBDIR_RESPONSE        (0x000eUL)

/******GGP_TYPE_#?******/

/* struktury opisuj±ce obs³ugiwane pakiety GG (na podstawie libgadu) */

/****is* ggpackets.h/GGPHeader
 *
 *  NAME
 *    GGPHeader
 *
 *  FUNCTION
 *    Struktura opisuje nag³ówek pakietu sieci GG.
 *
 *  ATTRIBUTES
 *    - ggph_Type -- typ pakietu;
 *    - ggph_Length -- d³ugo¶æ reszty pakietu (w bajtach).
 *
 *  SEE ALSO
 *    GGP_TYPE_#?
 *
 *  SOURCE
 */

struct GGPHeader
{
	ULONG ggph_Type;
	ULONG ggph_Length;
} GG_PACKED;

/**********GGPHeader********/

/****is* ggpackets.h/GGPWelcome
 *
 *  NAME
 *    GGPWelcome
 *
 *  FUNCTION
 *    Struktura opisuje pakiet powitania sieci GG.
 *    Pakiet ten jest wysy³any przez serwer automatycznie po nawi±zaniu po³±czenia.
 *
 *  ATTRIBUTES
 *    - ggpw_Key -- klucz szyfrowania has³a.
 *
 *  SEE ALSO
 *    GGP_TYPE_#?, GGPacketHandlerWelcome()
 *
 *  SOURCE
 */

struct GGPWelcome
{
	ULONG ggpw_Key;
} GG_PACKED;

/*******GGPWelcome*******/

/****is* ggpackets.h/GGPLoginOK
 *
 *  NAME
 *    GGPLoginOK
 *
 *  FUNCTION
 *    Struktura opisuje pakiet potwierdzaj±cy zalogowanie do sieci GG.
 *    Pakiet ten jest wysy³any przez serwer automatycznie po udanym logowaniu.
 *
 *  ATTRIBUTES
 *    - ggpl_Unknown -- nieznane pole.
 *
 *  SEE ALSO
 *    GGP_TYPE_#?, GGPacketHandlerLogin()
 *
 *  SOURCE
 */

struct GGPLoginOK
{
	ULONG ggpl_Unknown;
} GG_PACKED;

/******GGPLoginOK******/

/****is* ggpackets.h/GGPStatusChange
 *
 *  NAME
 *    GGPStatusChange
 *
 *  FUNCTION
 *    Struktura opisuje pakiet informuj±cy o zmiane statusu przez kontakt.
 *    Pakiet ten jest wysy³any przez serwer automatycznie je¶li kto¶ z listy
 *    kontaktów przes³anej do serwera zmieni swój status.
 *
 *  ATTRIBUTES
 *    - ggpsc_Uin -- uin u¿ytkownika zmieniaj±cego status;
 *    - ggpsc_Status -- nowo ustawiony status;
 *    - ggpsc_Features -- opcje protoko³u obs³ugiwane przez danego u¿ytkownika;
 *    - ggpsc_Unused -- nieu¿ywane pola (pozosta³o¶ci po starych wersjach protoko³u, zawsze zera);
 *    - ggpsc_ImageSize -- maksymalny rozmiar obrazków odbieranych przez danego u¿ytkownika;
 *    - ggpsc_Flags -- flagi statusu (takie jak przy logowaniu do sieci);
 *    - ggpsc_DescLen -- d³ugo¶æ opisu;
 *    - ggpsc_Description -- opis, nie musi wyst±piæ (je¶li brak to ggpsc_DescLen == 0).
 *
 *  SEE ALSO
 *    GGP_TYPE_#?, GGPacketHandlerWelcome(), GG_STATUS_#?
 *
 *  SOURCE
 */

struct GGPStatusChange
{
	ULONG ggpsc_Uin;
	ULONG ggpsc_Status;
	ULONG ggpsc_Features;
	UBYTE ggpsc_Unused[6];
	UBYTE ggpsc_ImageSize;
	UBYTE ggpsc_Unknown;
	ULONG ggpsc_Flags;
	ULONG ggpsc_DescLen;
	UBYTE ggpsc_Description[];
} GG_PACKED;

/******GGPStatusChange******/

/****is* ggpackets.h/GGPTypingNotify
 *
 *  NAME
 *    GGPTypingNotify
 *
 *  FUNCTION
 *    Struktura opisuje pakiet powiadomienia o pisaniu ("pisak" w oryginalnym
 *    kliencie).
 *
 *  ATTRIBUTES
 *    - ggptn_Type -- d³ugo¶æ wpisanego w pole wysy³ki tekstu;
 *    - ggptn_Uin -- uin u¿ytkownika, który do nas pisze.
 *
 *  SEE ALSO
 *    GGP_TYPE_#?, GGPacketHandlerTypingNotify()
 *
 *  SOURCE
 */

struct GGPTypingNotify
{
	USHORT ggptn_Type;
	ULONG ggptn_Uin;
}GG_PACKED;

/******GGPTypingNotify******/

/****is* ggpackets.h/GGPRecvMsg
 *
 *  NAME
 *    GGPRecvMsg
 *
 *  FUNCTION
 *    Struktura opisuje pakiet otrzymania nowej wiadomo¶ci.
 *
 *  ATTRIBUTES
 *    - ggprm_Uin -- numer GG nadawcy;
 *    - ggprm_Seq -- numer sekwencyjny;
 *    - ggprm_Time -- czas nadania (UTC, Unix era);
 *    - ggprm_Class -- klasa wiadomo¶ci;
 *    - ggprm_OffsetPlain -- po³o¿enie tre¶ci czystym tekstem;
 *    - ggprm_OffsetAttrs -- po³o¿enie atrybutów;
 *    - ggprm_HtmlTxt -- tre¶æ w formacie HTML (zakoñczona 0x00).
 *
 *  SEE ALSO
 *    GGP_TYPE_#?, GGPacketHandlerRecvMsg()
 *
 *  SOURCE
 */

struct GGPRecvMsg
{
	ULONG ggprm_Uin;
	ULONG ggprm_Seq;
	ULONG ggprm_Time;
	ULONG ggprm_Class;
	ULONG ggprm_OffsetPlain;
	ULONG ggprm_OffsetAttrs;
	UBYTE ggprm_HtmlTxt[];
}GG_PACKED;

/******GGPRecvMsg******/

/****is* ggpackets.h/GGPUsersData
 *
 *  NAME
 *    GGPUsersData
 *
 *  FUNCTION
 *    Struktura opisuje pakiet otrzymania danych o kontaktach.
 *
 *  ATTRIBUTES
 *    - ggpud_Type -- typ danych;
 *    - ggpud_UsersNo -- ilo¶æ opisywanych u¿ytkowników;
 *    - ggpud_Data[] -- tablica struktur GGUserData.
 *
 *  SEE ALSO
 *    GGP_TYPE_#?, GGPacketHandlerUserData(), GGUserData
 *
 *  SOURCE
 */

struct GGPUsersData
{
	ULONG ggpud_Type;
	ULONG ggpud_UsersNo;
	UBYTE ggpud_Data[];
}GG_PACKED;

/******GGPUsersData******/


/****is* ggpackets.h/GGPUserList
 *
 *  NAME
 *    GGPUserList
 *
 *  FUNCTION
 *    Struktura opisuje pakiet otrzymania danych o li¶cie kontaktów.
 *
 *  ATTRIBUTES
 *    - ggpul_Type -- rodzaj pakietu:
 *       - 0x00 -- w ggpul_Data znajduje siê lista kontaktów;
 *       - 0x10 -- potwierdzenie przyjêcia nowej listy;
 *       - 0x12 -- odmowa przyjêcia nowej listy.
 *    - ggpul_Version -- numer wersji konkaktów aktualnie przechowywanej przez serwer;
 *    - ggpul_Format -- rodzaj formatu listy kontaktów:
 *       - 0x02 -- lista w XML-u skompresowana algorytmem Deflate.
 *    - ggpul_Data[] -- dane w zale¿no¶ci od ggpul_Type.
 *
 *  SEE ALSO
 *    GGP_TYPE_#?, GGPacketHandlerUserListReply()
 *
 *  SOURCE
 */

struct GGPUserList
{
	UBYTE ggpul_Type;
	ULONG ggpul_Version;
	UBYTE ggpul_Format;
	UBYTE ggpul_Dummy;
	UBYTE ggpul_Data[];
}GG_PACKED;

/******GGPUserList******/

/****is* ggpackets.h/GGPMultilogonInfo
 *
 *  NAME
 *    GGPMultilogonInfo
 *
 *  FUNCTION
 *    Struktura opisuje pakiet otrzymania danych o li¶cie kontaktów.
 *
 *  ATTRIBUTES
 *    - ggpmi_ClientsNo -- ilo¶æ równolegle zalogowanych klientów;
 *    - ggpul_Data[] -- tablica struktur opisuj±cych równolegle zalogowane klienty.
 *
 *  SEE ALSO
 *    GGP_TYPE_#?, GGPacketHandlerMultilogonInfo()
 *
 *  SOURCE
 */

struct GGPMultilogonInfo
{
	ULONG ggpmi_ClientsNo;
	UBYTE ggpmi_Data[];
}GG_PACKED;

/******GGPMultilogonInfo******/

/****is* ggpackets.h/GGPPubDirInfo
 *
 *  NAME
 *    GGPPubDirInfo
 *
 *  FUNCTION
 *    Struktura opisuje pakiet odpowiedzi na zapytanie o katalogu publicznym.
 *
 *  ATTRIBUTES
 *    - ggppdi_Type -- typ odpowiedzi:
 *     - GG_PUBDIR_SEARCH_REPLY -- odpowied¼ na wyszukiwanie;
 *    - ggppdi_Seq -- numer sekwencyjny;
 *    - ggppdi_Data -- tre¶æ odpowiedzi.
 *
 *  SEE ALSO
 *    GGP_PUBDIR_#?, GGPacketHandlerPubDirInfo()
 *
 *  SOURCE
 */

struct GGPPubDirInfo
{
	UBYTE ggppdi_Type;
	ULONG ggppdi_Seq;
	UBYTE ggppdi_Data[];
}GG_PACKED;

/******GGPPubDirInfo******/

struct GGPHeader *GGReceivePacket(struct GGSession *gg_sess);
LONG GGWriteData(struct GGSession *gg_sess);
BYTE *GGPacketCreateTagList(ULONG type, ULONG *len, struct TagItem *taglist);
#ifdef USE_INLINE_STDARG
#define GGPacketCreateTags(type, len, ...)	({ULONG _tags[] = {__VA_ARGS__}; GGPacketCreateTagList(type, len, (struct TagItem*)_tags);})
#endif /* USE_INLINE_STDARG */
BOOL GGHandlePacket(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac);
BOOL GGAddToWriteBuffer(struct GGSession *gg_sess, BYTE *add, LONG len);
BOOL GGAcceptMessage(struct GGSession *gg_sess, ULONG seq);

#endif /* __GGPACKETS_H__ */
