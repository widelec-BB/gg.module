/*
 * Copyright (c) 2013 - 2022 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* GGLib/ggpackets.c
 *
 *  NAME
 *    ggpackets.c -- Plik zawieraj±cy definicje pakietów sieci GG oraz funkcji je obs³uguj±cych.
 *
 *  AUTHOR
 *    Filip Maryjañski
 *
 *  DESCRIPTION
 *    Definicje funkcji przeznaczonych do obs³ugi pakietów sieci GG.
 *
 ********/

#include <proto/exec.h>
#include <exec/types.h>
#include <proto/socket.h>
#include <sys/errno.h>
#include <proto/utility.h>
#include <libvstring.h>
#include "globaldefines.h"
#include "endianess.h"
#include "support.h"
#include "gglib.h"
#include "ggpackets.h"
#include "sha1.h"
#include "ggmessage.h"
#include "ggdefs.h"

#define SocketBase gg_sess->SocketBase

extern struct Library *SysBase, *DOSBase, *UtilityBase, *OpenSSL3Base;

/****if* ggpackets.c/GGReceivePacket()
 *
 *  NAME
 *    GGReceivePacket()
 *
 *  SYNOPSIS
 *    GGPHeader *GGReceivePacket(struct GGSession *gg_sess)
 *
 *  FUNCTION
 *    Funkcja najpierw odbiera nag³ówek pakietu GG, pó¼niej resztê pakietu.
 *
 *  INPUTS
 *    gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±cej za po³±czenie.
 *
 *  RESULT
 *    Wska¼nik na zaalokowan± strukturê GGPHeader po której bezpo¶rednio w pamiêci
 *    jest dalsza czê¶æ pakietu lub NULL w przypadku b³êdu. Zwrócony bufor nale¿y zwolniæ
 *    u¿ywaj±c FreeVec().
 *
 *****/

struct GGPHeader *GGReceivePacket(struct GGSession *gg_sess)
{
	struct GGPHeader *header = NULL;
	BYTE *packet;
	LONG len = -1, res;
	ENTER();

	while(TRUE)
	{
		if((gg_sess->ggs_RecvBuffer == NULL && gg_sess->ggs_RecvLen == 0))
		{
			if((gg_sess->ggs_RecvBuffer = AllocVec(sizeof(struct GGPHeader) + 1, MEMF_ANY | MEMF_CLEAR)) == NULL)
			{
				/* brak pamiêci */
				goto fail;
			}
		}

		header = (struct GGPHeader*)gg_sess->ggs_RecvBuffer;

		if(gg_sess->ggs_RecvLen < sizeof(struct GGPHeader))
		{
			/* nie odebrali¶my jeszcze ca³ego nag³ówka */
			len = sizeof(struct GGPHeader) - gg_sess->ggs_RecvLen;
		}
		else if(gg_sess->ggs_RecvLen >= sizeof(struct GGPHeader) + EndianFix32(header->ggph_Length))
		{
			/* odebrali¶my ca³y pakiet */
			break;
		}
		else
		{
			/* nie odebrali¶my jeszcze ca³ego pakietu */
			len = sizeof(struct GGPHeader) + EndianFix32(header->ggph_Length) - gg_sess->ggs_RecvLen;
		}

		res = RecvAllSSL(SocketBase, gg_sess->ggs_SSL, (BYTE*)header + gg_sess->ggs_RecvLen, len);

		if(res == 0)
		{
			/* stracili¶my po³±czenie... */
			GG_SESSION_ERROR(gg_sess, GGS_ERRNO_SERVER_OFF);
			gg_sess->ggs_SessionState = GGS_STATE_DISCONNECTING;
			goto fail;
		}

		if(res < 0 && Errno() == EAGAIN)
		{
			/* nie uda³o siê pobraæ ca³ego pakietu, trzeba spróbowaæ ponownie */
			gg_sess->ggs_Errno = GGS_ERRNO_TRYAGAIN;
			goto eagain;
		}

		if(res < 0)
		{
			GG_SESSION_ERROR(gg_sess, GGS_ERRNO_SOCKET_LIB);
			goto fail;
		}

		if(gg_sess->ggs_RecvLen + res == sizeof(struct GGPHeader))
		{
			BYTE *buf;
			/* odebrali¶my ca³y nag³ówek */

			if(EndianFix32(header->ggph_Length) == 0)
				break; /* nie ma dalszej czê¶ci pakietu, zwracamy co mamy, niech siê martwi dispacher pakietów */

			if(EndianFix32(header->ggph_Length) > 65535)
				goto fail;

			if((buf = AllocVec(sizeof(struct GGPHeader) + EndianFix32(header->ggph_Length) + 1, MEMF_ANY | MEMF_CLEAR)) == NULL)
				goto fail;

			CopyMem(gg_sess->ggs_RecvBuffer, buf, sizeof(struct GGPHeader));
			FreeVec(gg_sess->ggs_RecvBuffer);
			gg_sess->ggs_RecvBuffer = buf;
		}
		gg_sess->ggs_RecvLen += res;
	}

	/* przepinamy bufor jako nasz nowy pakiet */
	packet = gg_sess->ggs_RecvBuffer;
	/* przygotowujemy bufor do kolejnego odbierania pakietu */
	gg_sess->ggs_RecvBuffer = NULL;
	gg_sess->ggs_RecvLen = 0;

	/* zakañczamy pakiet zerem (stringi s± zwykle na koñcu pakietu) */
	*(packet + sizeof(struct GGPHeader) + EndianFix32(header->ggph_Length)) = 0;

	header->ggph_Length = EndianFix32(header->ggph_Length);
	header->ggph_Type = EndianFix32(header->ggph_Type);

	LEAVE();
	return (struct GGPHeader*)packet;

fail: /* b³±d, tego pakietu ju¿ nie uratujemy... */
	if(gg_sess->ggs_RecvBuffer)
		FreeVec(gg_sess->ggs_RecvBuffer);
	gg_sess->ggs_RecvBuffer = NULL;

eagain: /* zwracamy nulla i czekamy na ponowne wywo³anie */
	LEAVE();
	return NULL;
}

/****if* ggpackets.c/GGWriteData()
 *
 *  NAME
 *    GGWriteData()
 *
 *  SYNOPSIS
 *    LONG GGWriteData(struct GGSession *gg_sess)
 *
 *  FUNCTION
 *    Funkcja wysy³a dane z bufora wysy³anych danych sesji.
 *
 *  INPUTS
 *    gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±cej za po³±czenie.
 *
 *  RESULT
 *    Ilo¶æ wys³anych danych lub w przypadku b³êdu:
 *    - -1 -- b³±d krytyczny;
 *    - -2 -- b³±d tymczasowy, bufor pozosta³ zachowany, nale¿y spróbowaæ wys³aæ ponownie.
 *
 *****/

LONG GGWriteData(struct GGSession *gg_sess)
{
	LONG result;
	ENTER();

	if(gg_sess->ggs_WriteBuffer == NULL || gg_sess->ggs_WriteLen == 0)
	{
		return 0;
	}

	result = SendAllSSL(gg_sess->ggs_SSL, gg_sess->ggs_WriteBuffer + gg_sess->ggs_WrittenLen, gg_sess->ggs_WriteLen - gg_sess->ggs_WrittenLen);

	if(result == -1)
	{
		/* b³±d wysy³ania */

		if(Errno() == EAGAIN)
		{
			/* nic krytycznego, mo¿na spróbowaæ ponownie */
			result = -2;
		}
		else
		{
			/* krytyczny, umieramy */
			GG_SESSION_ERROR(gg_sess, GGS_ERRNO_SOCKET_LIB);
		}
	}
	else if(gg_sess->ggs_WrittenLen + result >= gg_sess->ggs_WriteLen)
	{
		/* wys³ano ca³y bufor */
		FreeVec(gg_sess->ggs_WriteBuffer);
		gg_sess->ggs_WriteBuffer = NULL;
		gg_sess->ggs_WrittenLen = gg_sess->ggs_WriteLen = 0;
	}
	else if(result > 0)
	{
		/* trochê posz³o, ale jeszcze nie ca³e, aktualizujemy ile wys³ali¶my */
		gg_sess->ggs_WrittenLen += result;
	}

	LEAVE();
	return result;
}

/****if* ggpackets.c/GGPacketCreateTagList()
 *
 *  NAME
 *    GGPacketCreateTagList()
 *
 *  SYNOPSIS
 *    - BYTE *GGPacketCreateTagList(ULONG type, ULONG *len, struct TagItem *taglist)
 *    - BYTE *GGPacketCreateTags(ULONG type, ULONG *len, ULONG tag1, ...)
 *
 *  FUNCTION
 *    Funkcja s³u¿y do utworzenia pakietu do wys³ania. Przeprowadza automatyczn± konwersjê
 *    miêdzy big a little endianem. Zwrócony bufor nale¿y zwolniæ przez FreeVec().
 *
 *  INPUTS
 *    type -- typ pakietu;
 *    len -- wska¼nik na zmienn± do której zostanie zapisana d³ugo¶æ przygotowanego bufora;
 *    taglist -- lista tagów opisuj±ca kolejne pola pakietu, aktualnie rozpoznawane tagi to:
 *    - GGA_CreatePacket_LONG;
 *    - GGA_CreatePacket_SHORT;
 *    - GGA_CreatePacket_BYTE;
 *    - GGA_CreatePacket_ULONG;
 *    - GGA_CreatePacket_USHORT;
 *    - GGA_CreatePacket_UBYTE;
 *    - GGA_CreatePacket_QUAD -- ti_Data powinno zawieraæ wska¼nik na QUAD;
 *    - GGA_CreatePacket_UQUAD -- ti_Data powinno zawieraæ wska¼nik na UQUAD;
 *    - GGA_CreatePacket_STRPTR;
 *    - GGA_CreatePacket_ZEROIZE -- tag oznacza d³ugo¶æ bloku wype³nionego zerami (w przypadku nieu¿ywanych pól w pakietach).
 *     Ka¿dy z tagów opisuje kolejn± warto¶æ która ma zostaæ w³±czona do pakietu. Rozmiar
 *     jest obliczany na podstawie nazwy tagu, jako dane tagu powinny zostaæ podane dane,
 *     które maj± zostaæ zawarte w pakiecie. Dane zostan± w³±czone do pakietu zgodnie z podan±
 *     kolejno¶ci±;
 *    - GGA_CreatePacket_SHA1 -- warto¶æ taga to wska¼nik na tablicê z hashem sha1 (64 bajty).
 *    - GGA_CreatePacket_BLOCK -- warto¶æ taga to wska¼nik na strukturê TagItem, gdzie w polu ti_Tag zawarta jest d³ugo¶æ
 *     bloku (w bajtach) jaki ma zostaæ wpisany do pakietu, a ti_Data to wska¼nik na blok danych jaki ma byæ wpisany do pakietu.
 *
 *  RESULT
 *    Wska¼nik na zaalokowany bufor pakietu lub NULL w przypadku b³êdu.
 *
 *****/

BYTE *GGPacketCreateTagList(ULONG type, ULONG *len, struct TagItem *taglist)
{
	struct TagItem *tag, *tagptr;
	LONG pac_len = 0;
	BYTE *result = NULL;
	ENTER();

	if(taglist == NULL || len == NULL)
	{
		LEAVE();
		return NULL;
	}

	/* obliczanie wielko¶ci bufora dla danego pakietu */

	tagptr = taglist;
	while((tag = NextTagItem(&tagptr)))
	{
		switch(tag->ti_Tag)
		{
			case GGA_CreatePacket_LONG:
				pac_len += sizeof(LONG);
			break;

			case GGA_CreatePacket_SHORT:
				pac_len += sizeof(SHORT);
			break;

			case GGA_CreatePacket_BYTE:
				pac_len += sizeof(BYTE);
			break;

			case GGA_CreatePacket_QUAD:
				pac_len += sizeof(UQUAD);
			break;

			case GGA_CreatePacket_ULONG:
				pac_len += sizeof(ULONG);
			break;

			case GGA_CreatePacket_USHORT:
				pac_len += sizeof(USHORT);
			break;

			case GGA_CreatePacket_UBYTE:
				pac_len += sizeof(UBYTE);
			break;

			case GGA_CreatePacket_UQUAD:
				pac_len += sizeof(UQUAD);
			break;

			case GGA_CreatePacket_STRPTR:
				if((STRPTR)tag->ti_Data != NULL)
					pac_len += StrLen((STRPTR)tag->ti_Data);
			break;

			case GGA_CreatePacket_ZEROIZE:
				pac_len += tag->ti_Data;
			break;

			case GGA_CreatePacket_SHA1:
				pac_len += 64;
			break;

			case GGA_CreatePacket_BLOCK:
				pac_len += ((struct TagItem *)tag->ti_Data)->ti_Tag;
			break;
		}
	}

	if((result = AllocVec(sizeof(struct GGPHeader) + pac_len, MEMF_ANY | MEMF_CLEAR)))
	{
		BYTE *temp = result;

		/* wpisanie nag³ówka pakietu do bufora */
		*((ULONG*)temp) = EndianFix32(type);
		temp += sizeof(ULONG);
		*((ULONG*)temp) = EndianFix32(pac_len);
		temp += sizeof(ULONG);

		/* wpisanie reszty pakietu do bufora */
		tagptr = taglist;
		while((tag = NextTagItem(&tagptr)))
		{
			switch(tag->ti_Tag)
			{
				case GGA_CreatePacket_LONG:
					*((LONG*)temp) = (LONG)EndianFix32((LONG)tag->ti_Data);
					temp += sizeof(LONG);
				break;

				case GGA_CreatePacket_SHORT:
					*((SHORT*)temp) = (SHORT)EndianFix16((SHORT)tag->ti_Data);
					temp += sizeof(SHORT);
				break;

				case GGA_CreatePacket_BYTE:
					*temp = (BYTE)tag->ti_Data;
					temp++;
				break;

				case GGA_CreatePacket_QUAD:
					*((QUAD*)temp) = EndianFix64(*((QUAD*)tag->ti_Data));
					temp += sizeof(QUAD);
				break;

				case GGA_CreatePacket_ULONG:
					*((ULONG*)temp) = (ULONG)EndianFix32((ULONG)tag->ti_Data);
					temp += sizeof(ULONG);
				break;

				case GGA_CreatePacket_USHORT:
					*((USHORT*)temp) = (USHORT)EndianFix16((USHORT)tag->ti_Data);
					temp += sizeof(USHORT);
				break;

				case GGA_CreatePacket_UBYTE:
					*((UBYTE*)temp) = (UBYTE)tag->ti_Data;
					temp++;
				break;

				case GGA_CreatePacket_UQUAD:
					*((UQUAD*)temp) = EndianFix64(*((UQUAD*)tag->ti_Data));
					temp += sizeof(UQUAD);
				break;

				case GGA_CreatePacket_STRPTR:
					if((STRPTR)tag->ti_Data != NULL)
					{
						LONG str_len = StrLen((STRPTR)tag->ti_Data);
						CopyMem((APTR)tag->ti_Data, (APTR)temp, str_len);
						temp += str_len;
					}
				break;

				case GGA_CreatePacket_ZEROIZE:
					MemSet(temp, 0x00, tag->ti_Data);
					temp += tag->ti_Data;
				break;

				case GGA_CreatePacket_SHA1:
					CopyMem((APTR)tag->ti_Data, (APTR)temp, 64);
					temp += 64;
				break;

				case GGA_CreatePacket_BLOCK:
					CopyMem((APTR)((struct TagItem*)tag->ti_Data)->ti_Data, (APTR)temp, ((struct TagItem*)tag->ti_Data)->ti_Tag);
					temp += (ULONG)((struct TagItem*)tag->ti_Data)->ti_Tag;
				break;
			}
		}
	}

	if(len)
		*len = sizeof(struct GGPHeader) + pac_len;

	LEAVE();
	return result;
}

/****if* ggpackets.c/GGAddToWriteBuffer()
 *
 *  NAME
 *    GGAddToWriteBuffer()
 *
 *  SYNOPSIS
 *    BOOL GGAddToWriteBuffer(struct GGSession *gg_sess, BYTE *add, LONG len)
 *
 *  FUNCTION
 *    Funkcja dodaje dane wskazywane przez add o d³ugo¶ci len na koniec bufora wysy³ania
 *    sesji opisanej przez gg_sess. Automatycznie ustawiane jest pole ggs_Check
 *    struktury sesji tak, aby biblioteka informowa³a o chêci zapisania bufora do socketu.
 *
 *  INPUTS
 *    - gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±cej za po³±czenie.
 *    - add -- wska¼nik na bufor danych który powinien zostaæ do³±czony;
 *    - len -- d³ugo¶æ danych wskazywanych przez add.
 *
 *  RESULT
 *    - TRUE -- je¶li uda³o siê do³±czyæ dane;
 *    - FALSE -- je¶li nie uda³o siê do³±czyæ danych (zabrak³o pamiêci).
 *
 *  NOTES
 *    Je¶li funkcja zakoñczy³a siê sukcesem nie wolno zmieniaæ zawarto¶ci
 *    danych wskazywanych przez add, nie wolno te¿ próbowaæ zwalniaæ tej pamiêci!
 *
 *****/

BOOL GGAddToWriteBuffer(struct GGSession *gg_sess, BYTE *add, LONG len)
{
	BOOL result = FALSE;
	ENTER();

	if(add && len > 0)
	{
		if(gg_sess->ggs_WriteBuffer == NULL && gg_sess->ggs_WriteLen == 0)
		{
			gg_sess->ggs_WriteBuffer = add;
			gg_sess->ggs_WriteLen = len;
			gg_sess->ggs_WrittenLen = 0;
			gg_sess->ggs_Check |= GGS_CHECK_WRITE;
			result = TRUE;
		}
		else
		{
			BYTE *old_buf = gg_sess->ggs_WriteBuffer + gg_sess->ggs_WrittenLen;
			LONG old_len = gg_sess->ggs_WriteLen - gg_sess->ggs_WrittenLen;
			BYTE *new_buf;

			if((new_buf = AllocVec(old_len + len, MEMF_ANY | MEMF_CLEAR)))
			{
				CopyMem(old_buf, new_buf, old_len);
				CopyMem(add, new_buf + old_len, len);

				FreeVec(gg_sess->ggs_WriteBuffer);
				FreeVec(add);

				gg_sess->ggs_WriteBuffer = new_buf;
				gg_sess->ggs_WriteLen = old_len + len;
				gg_sess->ggs_WrittenLen = 0;
				gg_sess->ggs_Check |= GGS_CHECK_WRITE;

				result = TRUE;
			}
		}
	}

	LEAVE();
	return result;
}

/****if* ggpackets.c/GGAcceptMessage()
 *
 *  NAME
 *    GGAcceptMessage()
 *
 *  SYNOPSIS
 *    BOOL GGAcceptMessage(struct GGSession *gg_sess, ULONG seq)
 *
 *  FUNCTION
 *    Funkcja dodaje do bufora wysy³ania pakiet potwierdzenia otrzymania wiadomo¶ci.
 *
 *  INPUTS
 *    - gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±ce za po³±czenie;
 *    - seq -- numer sekwencyjny otrzymanej wiadomo¶ci
 *
 *  RESULT
 *    - TRUE -- je¶li wszystko zakoñczy³o siê sukcesem;
 *    - FALSE -- w.p.p.
 *
 *****/

BOOL GGAcceptMessage(struct GGSession *gg_sess, ULONG seq)
{
	BOOL result = FALSE;
	BYTE *pac;
	ULONG len;
	ENTER();

	if((pac = GGPacketCreateTags(GGP_TYPE_MSG_ACK, &len,
		GGA_CreatePacket_ULONG, seq,
	TAG_END)))
	{
		if(GGAddToWriteBuffer(gg_sess, pac, len))
			result = TRUE;
		else
			FreeVec(pac);
	}
	LEAVE();
	return result;
}

/****if* ggpackets.c/GGPacketHandlerWelcome()
 *
 *  NAME
 *    GGPacketHandlerWelcome()
 *
 *  SYNOPSIS
 *    static VOID GGPacketHandlerWelcome(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
 *
 *  FUNCTION
 *    Funkcja obs³uguje pakiet typu GGP_TYPE_WELCOME, odsy³a pakiet logowania i odpowiednio wype³nia strukturê zdarzenia.
 *
 *  INPUTS
 *    - gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±ce za po³±czenie;
 *    - event -- wska¼nik na strukturê zdarzenia, któr± handler ma wype³niæ;
 *    - pac -- wska¼nik na strukturê GGPHeader, po której bezpo¶rednio w pamiêci znajduje siê struct GGPWelcome.
 *
 *****/

static VOID GGPacketHandlerWelcome(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
{
	ENTER();
	if(pac->ggph_Length == sizeof(struct GGPWelcome))
	{
		struct GGPWelcome *gw = (struct GGPWelcome*)(pac + 1);
		UBYTE hash[64];
		LONG buffer_len; /* wielko¶æ bufora potrzebnego na przechowanie pakietu */
		BYTE *login_packet = NULL;

		MemSet(hash, 0, sizeof(hash));

		GGSha1Hash(gg_sess->ggs_Pass, gw->ggpw_Key, hash);	/* obliczamy hash has³a */

		login_packet = GGPacketCreateTags(GGP_TYPE_GGLOGIN, &buffer_len,
			GGA_CreatePacket_ULONG, gg_sess->ggs_Uin,
			GGA_CreatePacket_STRPTR, (ULONG)"pl",
			GGA_CreatePacket_BYTE, 0x02, /* rodzaj hashu - sha1 */
			GGA_CreatePacket_SHA1, (ULONG)hash,
			GGA_CreatePacket_ULONG, gg_sess->ggs_Status,
			GGA_CreatePacket_ULONG, GGLIB_STATUS_FLAGS,
			GGA_CreatePacket_ULONG, GGLIB_FEATURES,
			GGA_CreatePacket_ZEROIZE, 12,
			GGA_CreatePacket_UBYTE, gg_sess->ggs_ImageSize,
			GGA_CreatePacket_BYTE, 0x64,
			GGA_CreatePacket_ULONG, StrLen(GGLIB_DEFAULT_CLIENT_NAME GGLIB_DEFAULT_CLIENT_VERSION),
			GGA_CreatePacket_STRPTR, (ULONG)(GGLIB_DEFAULT_CLIENT_NAME GGLIB_DEFAULT_CLIENT_VERSION),
			GGA_CreatePacket_ULONG, gg_sess->ggs_StatusDescription ? StrLen(gg_sess->ggs_StatusDescription) : 0,
			GGA_CreatePacket_STRPTR, (ULONG)gg_sess->ggs_StatusDescription,
		TAG_END);

		if(login_packet)
		{
			if((GGAddToWriteBuffer(gg_sess, login_packet, buffer_len)))
			{
				event->gge_Type = GGE_TYPE_NOOP;
			}
			else
			{
				GG_SESSION_ERROR(gg_sess, GGS_ERRNO_MEM);
			}
		}
	}
	LEAVE();
}

/****if* ggpackets.c/GGPacketHandlerLogin()
 *
 *  NAME
 *    GGPacketHandlerLogin()
 *
 *  SYNOPSIS
 *    static VOID GGPacketHandlerLogin(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
 *
 *  FUNCTION
 *    Funkcja obs³uguje pakiety: GGP_TYPE_LOGINOK, GGP_TYPE_LOGINFAIL i GGP_TYPE_LOGINFAIL2 generuj±c odpowiednie zdarzenie
 *    (GGE_TYPE_LOGIN_SUCCESS lub GGE_TYPE_LOGIN_FAIL).
 *
 *  INPUTS
 *    - gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±ce za po³±czenie;
 *    - event -- wska¼nik na strukturê zdarzenia, któr± handler ma wype³niæ;
 *    - pac -- wska¼nik na strukturê GGPHeader, po której bezpo¶rednio w pamiêci znajduje siê GGPWelcome.
 *
 *  SEE ALSO
 *    GGE_TYPE_#?
 *
 *****/

static VOID GGPacketHandlerLogin(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
{
	ENTER();

	if(pac->ggph_Type == GGP_TYPE_LOGIN_OK)
	{
		event->gge_Type = GGE_TYPE_LOGIN_SUCCESS;
	}
	else if(pac->ggph_Type == GGP_TYPE_LOGIN_FAIL || pac->ggph_Type == GGP_TYPE_LOGIN_FAIL2)
	{
		event->gge_Type = GGE_TYPE_LOGIN_FAIL;
	}

	LEAVE();
}

/****if* ggpackets.c/GGPacketHandlerStatusChange()
 *
 *  NAME
 *    GGPacketHandlerStatusChange()
 *
 *  SYNOPSIS
 *    static VOID GGPacketHandlerStatusChange(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
 *
 *  FUNCTION
 *    Funkcja obs³uguje pakiet GGP_TYPE_STATUS_CHANGE generuj±c odpowiednie zdarzenie (GGE_TYPE_STATUS_CHANGE).
 *
 *  INPUTS
 *    - gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±ce za po³±czenie;
 *    - event -- wska¼nik na strukturê zdarzenia, któr± handler ma wype³niæ;
 *    - pac -- wska¼nik na strukturê GGPHeader, po której bezpo¶rednio w pamiêci znajduje siê GGPStatusChange.
 *
 *  SEE ALSO
 *    GGE_TYPE_#?, GGEventStatusChange
 *
 *****/

static VOID GGPacketHandlerStatusChange(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
{
	struct GGPStatusChange *sch = (struct GGPStatusChange*)(pac + 1);
	ULONG desc_len = EndianFix32(sch->ggpsc_DescLen);
	ENTER();

	event->gge_Type = GGE_TYPE_STATUS_CHANGE;
	event->gge_Event.gge_StatusChange.ggesc_Uin = EndianFix32(sch->ggpsc_Uin);
	event->gge_Event.gge_StatusChange.ggesc_Status = EndianFix32(sch->ggpsc_Status);
	event->gge_Event.gge_StatusChange.ggesc_ImageSize = sch->ggpsc_ImageSize;

	if(desc_len > 0)
		event->gge_Event.gge_StatusChange.ggesc_Description = StrNewLen(&sch->ggpsc_Description[0], desc_len);
	else
		event->gge_Event.gge_StatusChange.ggesc_Description = NULL;

	LEAVE();
}

/****if* ggpackets.c/GGPacketHandlerListStatus()
 *
 *  NAME
 *    GGPacketHandlerListStatus()
 *
 *  SYNOPSIS
 *    static VOID GGPacketHandlerListStatus(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
 *
 *  FUNCTION
 *    Funkcja obs³uguje pakiet GGP_TYPE_LIST_STATUS generuj±c odpowiednie zdarzenie (GGE_TYPE_LIST_STATUS).
 *
 *  INPUTS
 *    - gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±ce za po³±czenie;
 *    - event -- wska¼nik na strukturê zdarzenia, któr± handler ma wype³niæ;
 *    - pac -- wska¼nik na strukturê GGPHeader, po której bezpo¶rednio w pamiêci znajduje siê tablica GGPStatusChange.
 *
 *  SEE ALSO
 *    GGE_TYPE_#?, GGEventListStatus, GGPacketHandlerStatusChange(), GGPStatusChange
 *
 *****/

static VOID GGPacketHandlerListStatus(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
{
	ENTER();
	struct GGPStatusChange *sc = (struct GGPStatusChange*)(pac+1);
	struct GGPStatusChange *temp = sc;
	LONG act_len = pac->ggph_Length;
	LONG entries_no = 0;

	while(act_len >= sizeof(struct GGPStatusChange))
	{
		ULONG desc_len = EndianFix32(temp->ggpsc_DescLen);

		entries_no++;

		temp++;
		act_len -= sizeof(struct GGPStatusChange);
		temp = (struct GGPStatusChange*)(((UBYTE*)temp) + desc_len);
		act_len -= desc_len;
	}

	if(entries_no > 0)
	{
		if((event->gge_Event.gge_ListStatus.ggels_StatusChanges = AllocVec(sizeof(struct GGEventStatusChange) * entries_no, MEMF_ANY | MEMF_CLEAR)))
		{
			event->gge_Type = GGE_TYPE_LIST_STATUS;
			event->gge_Event.gge_ListStatus.ggels_ChangesNo = entries_no;

			entries_no = 0;
			act_len = pac->ggph_Length;
			temp = sc;

			while(act_len >= sizeof(struct GGPStatusChange))
			{
				ULONG desc_len = EndianFix32(temp->ggpsc_DescLen);

				event->gge_Event.gge_ListStatus.ggels_StatusChanges[entries_no].ggesc_Uin = EndianFix32(temp->ggpsc_Uin);
				event->gge_Event.gge_ListStatus.ggels_StatusChanges[entries_no].ggesc_Status = EndianFix32(temp->ggpsc_Status);
				event->gge_Event.gge_ListStatus.ggels_StatusChanges[entries_no].ggesc_ImageSize = temp->ggpsc_ImageSize;

				if(desc_len > 0)
				{
					event->gge_Event.gge_ListStatus.ggels_StatusChanges[entries_no].ggesc_Description = StrNewLen(temp->ggpsc_Description, desc_len);
				}
				else
				{
					event->gge_Event.gge_ListStatus.ggels_StatusChanges[entries_no].ggesc_Description = NULL;
				}

				entries_no++;
				temp++;
				act_len -= sizeof(struct GGPStatusChange);
				temp = (struct GGPStatusChange*) (((UBYTE*)temp) + desc_len);
				act_len -= desc_len;
			}
		}
		else
		{
			GG_SESSION_ERROR(gg_sess, GGS_ERRNO_MEM);
		}
	}
	else
	{
		event->gge_Type = GGE_TYPE_NOOP;
	}

	LEAVE();
}

/****if* ggpackets.c/GGPacketHandlerTypingNotify()
 *
 *  NAME
 *    GGPacketHandlerTypingNotify()
 *
 *  SYNOPSIS
 *    static VOID GGPacketHandlerTypingNotify(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
 *
 *  FUNCTION
 *    Funkcja obs³uguje pakiet GGP_TYPE_TYPING_NOTIFY generuj±c odpowiednie zdarzenie (GGE_TYPE_TYPING_NOTIFY).
 *
 *  INPUTS
 *    - gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±ce za po³±czenie;
 *    - event -- wska¼nik na strukturê zdarzenia, któr± handler ma wype³niæ;
 *    - pac -- wska¼nik na strukturê GGPHeader, po której bezpo¶rednio w pamiêci znajduje siê tablica GGPStatusChange.
 *
 *  SEE ALSO
 *    GGE_TYPE_#?, GGEventTypingNotify
 *
 *****/

static VOID GGPacketHandlerTypingNotify(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
{
	struct GGPTypingNotify *n = (struct GGPTypingNotify*)(pac+1);
	ENTER();

	event->gge_Type = GGE_TYPE_TYPING_NOTIFY;
	event->gge_Event.gge_TypingNotify.ggetn_Uin = EndianFix32(n->ggptn_Uin);
	event->gge_Event.gge_TypingNotify.ggetn_Length = EndianFix16(n->ggptn_Type);
	LEAVE();
}

/****if* ggpackets.c/GGPacketHandlerRecvMsg()
 *
 *  NAME
 *    GGPacketHandlerRecvMsg()
 *
 *  SYNOPSIS
 *    static VOID GGPacketHandlerRecvMsg(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
 *
 *  FUNCTION
 *    Funkcja obs³uguje pakiet GGP_TYPE_RECV_MSG generuj±c odpowiednie zdarzenie (GGE_TYPE_RECV_MSG).
 *
 *  INPUTS
 *    - gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±ce za po³±czenie;
 *    - event -- wska¼nik na strukturê zdarzenia, któr± handler ma wype³niæ;
 *    - pac -- wska¼nik na strukturê GGPHeader, po której bezpo¶rednio w pamiêci znajduje siê tablica GGPStatusChange.
 *
 *  SEE ALSO
 *    GGE_TYPE_#?, GGEventRecvMsg, GGEventImageData, GGPRecvMsg
 *
 *****/

static VOID GGPacketHandlerRecvMsg(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
{
	struct GGPRecvMsg *ms = (struct GGPRecvMsg*)(pac + 1);
	ENTER();

	if(GGAcceptMessage(gg_sess, EndianFix32(ms->ggprm_Seq)))
	{
		STRPTR image;

		if((event->gge_Event.gge_RecvMsg.ggerm_Txt = GGMessageHTMLtoText(ms->ggprm_HtmlTxt, &image)) || image)
		{
			event->gge_Type = GGE_TYPE_RECV_MSG;
			event->gge_Event.gge_RecvMsg.ggerm_Uin = EndianFix32(ms->ggprm_Uin);
			event->gge_Event.gge_RecvMsg.ggerm_Time = EndianFix32(ms->ggprm_Time);
			event->gge_Event.gge_RecvMsg.ggerm_ImagesIds = image;
			event->gge_Event.gge_RecvMsg.ggerm_Flags = pac->ggph_Type == GGP_TYPE_RECV_OWN_MSG ? GG_MSG_OWN : GG_MSG_NORMAL;
		}
		else if(pac->ggph_Length > sizeof(struct GGPRecvMsg) + 1) /* mamy do czynienia z wiadomo¶ci± zawieraj±c± dane obrazka */
		{
			BYTE *image_data = ((BYTE*)(ms + 1)) + 1; /* omijamy ca³± strukturê GGPRecvMsg oraz bajt 0x00 oznaczaj±cy pust± wiadomo¶æ */
			BYTE flag = *image_data;

			image_data++;

			if(flag == 0x04) /* kto¶ ¿±da od nas obrazka */
			{
				event->gge_Type = GGE_TYPE_IMAGE_REQUEST;

				event->gge_Event.gge_ImageRequest.ggeir_Uin = EndianFix32(ms->ggprm_Uin);

				event->gge_Event.gge_ImageRequest.ggeir_ImageSize = EndianFix32(*((ULONG*)image_data));
				image_data += sizeof(ULONG);

				event->gge_Event.gge_ImageRequest.ggeir_Crc32 = EndianFix32(*((ULONG*)image_data));
			}
			else /* otrzymali¶my ¿±dany obrazek */
			{
				event->gge_Type = GGE_TYPE_IMAGE_DATA;

				event->gge_Event.gge_ImageData.ggeid_Uin = EndianFix32(ms->ggprm_Uin);

				event->gge_Event.gge_ImageData.ggeid_ImageSize = EndianFix32(*((ULONG*)image_data));
				image_data += sizeof(ULONG);

				event->gge_Event.gge_ImageData.ggeid_Crc32 = EndianFix32(*((ULONG*)image_data));
				image_data += sizeof(ULONG);

				if(flag == 0x05) /* pierwsza porcja danych obrazka */
				{
					ULONG name_len = StrLen(image_data);

					event->gge_Event.gge_ImageData.ggeid_Type = GG_IMAGE_START_DATA;

					event->gge_Event.gge_ImageData.ggeid_FileName = StrNewLen(image_data, name_len);

					image_data += name_len + 1;
				}
				else if(flag == 0x06) /* dalsza czê¶æ obrazka, brak nazwy pliku */
				{
					event->gge_Event.gge_ImageData.ggeid_Type = GG_IMAGE_NEXT_DATA;
				}

				event->gge_Event.gge_ImageData.ggeid_DataSize = ((BYTE*)pac) + pac->ggph_Length - image_data + sizeof(struct GGPHeader);

				if((event->gge_Event.gge_ImageData.ggeid_Data = AllocMem(event->gge_Event.gge_ImageData.ggeid_DataSize, MEMF_ANY)))
				{
					CopyMem(image_data, event->gge_Event.gge_ImageData.ggeid_Data, event->gge_Event.gge_ImageData.ggeid_DataSize);
				}
				else
				{
					if(event->gge_Event.gge_ImageData.ggeid_FileName)
						StrFree(event->gge_Event.gge_ImageData.ggeid_FileName);

					event->gge_Type = GGE_TYPE_ERROR;

					GG_SESSION_ERROR(gg_sess, GGS_ERRNO_MEM);
				}
			}
		}
		else
			GG_SESSION_ERROR(gg_sess, GGS_ERRNO_MEM);
	}
	else
		GG_SESSION_ERROR(gg_sess, GGS_ERRNO_MEM);

	LEAVE();
}

/****if* ggpackets.c/GGPacketHandlerUserData()
 *
 *  NAME
 *    GGPacketHandlerUserData()
 *
 *  SYNOPSIS
 *    static VOID GGPacketHandlerUserData(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
 *
 *  FUNCTION
 *    Funkcja obs³uguje pakiet GGP_TYPE_USER_DATA generuj±c odpowiednie zdarzenie (GGE_TYPE_USER_DATA).
 *
 *  INPUTS
 *    - gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±ce za po³±czenie;
 *    - event -- wska¼nik na strukturê zdarzenia, któr± handler ma wype³niæ;
 *    - pac -- wska¼nik na strukturê GGPHeader, po której bezpo¶rednio w pamiêci znajduje siê struktura GGPUsersData.
 *
 *  SEE ALSO
 *    GGE_TYPE_#?
 *
 *****/

static VOID GGPacketHandlerUserData(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
{
	struct GGPUsersData *ud = (struct GGPUsersData*)(pac + 1);
	UBYTE *data = (UBYTE*)ud->ggpud_Data;
	ENTER();

	ud->ggpud_Type = EndianFix32(ud->ggpud_Type);
	ud->ggpud_UsersNo = EndianFix32(ud->ggpud_UsersNo);

	if((event->gge_Event.gge_UsersData.ggeud_Data = AllocMem(sizeof(struct GGUserData) * ud->ggpud_UsersNo, MEMF_ANY)))
	{
		struct GGUserData *edata = event->gge_Event.gge_UsersData.ggeud_Data;
		LONG i;

		event->gge_Type = GGE_TYPE_USER_DATA;
		event->gge_Event.gge_UsersData.ggeud_Type = ud->ggpud_Type;
		event->gge_Event.gge_UsersData.ggeud_UsersNo = ud->ggpud_UsersNo;

		for(i = 0; i < ud->ggpud_UsersNo; i++)
		{
			edata[i].ggud_Uin = EndianFix32(*((ULONG*)data));
			data += sizeof(ULONG);

			if(data > ud->ggpud_Data + pac->ggph_Length)
				goto error;

			edata[i].ggud_AttrsNo = EndianFix32(*((ULONG*)data));
			data += sizeof(ULONG);

			if(data > ud->ggpud_Data + pac->ggph_Length)
				goto error;

			if((edata[i].ggud_Attrs = AllocMem(sizeof(struct GGUserDataAttr) * edata[i].ggud_AttrsNo, MEMF_ANY)))
			{
				LONG j;
				struct GGUserDataAttr *attrs = edata[i].ggud_Attrs;

				for(j = 0; j < edata[i].ggud_AttrsNo; j++)
				{
					ULONG key_size, value_size;

					key_size = EndianFix32(*((ULONG*)data));
					data += sizeof(ULONG);

					if(data > ud->ggpud_Data + pac->ggph_Length)
						goto error;

					attrs[j].gguda_Key = StrNewLen(data, key_size);
					data += key_size;

					if(data > ud->ggpud_Data + pac->ggph_Length)
						goto error;

					attrs[j].gguda_Type = EndianFix32(*((ULONG*)data));
					data += sizeof(ULONG);

					if(data > ud->ggpud_Data + pac->ggph_Length)
						goto error;

					value_size = EndianFix32(*((ULONG*)data));
					data += sizeof(ULONG);

					if(data > ud->ggpud_Data + pac->ggph_Length)
						goto error;

					attrs[j].gguda_Value = StrNewLen(data, value_size);
					data += value_size;

					if(data > ud->ggpud_Data + pac->ggph_Length)
						goto error;
				}
			}
		}
	}

	LEAVE();
	return;

error:
	if(event->gge_Event.gge_UsersData.ggeud_Data)
	{
		LONG i;

		for(i = 0; i < event->gge_Event.gge_UsersData.ggeud_UsersNo; i++)
		{
			if(event->gge_Event.gge_UsersData.ggeud_Data[i].ggud_Attrs)
			{
				LONG j;

				for(j = 0; j < event->gge_Event.gge_UsersData.ggeud_Data[i].ggud_AttrsNo; j++)
				{
					if(event->gge_Event.gge_UsersData.ggeud_Data[i].ggud_Attrs[j].gguda_Key)
						StrFree(event->gge_Event.gge_UsersData.ggeud_Data[i].ggud_Attrs[j].gguda_Key);

					if(event->gge_Event.gge_UsersData.ggeud_Data[i].ggud_Attrs[j].gguda_Value)
						StrFree(event->gge_Event.gge_UsersData.ggeud_Data[i].ggud_Attrs[j].gguda_Value);
				}
				FreeMem(event->gge_Event.gge_UsersData.ggeud_Data[i].ggud_Attrs, sizeof(struct GGUserDataAttr) * event->gge_Event.gge_UsersData.ggeud_Data[i].ggud_AttrsNo);
			}
		}
		FreeMem(event->gge_Event.gge_UsersData.ggeud_Data, sizeof(struct GGUserData) * event->gge_Event.gge_UsersData.ggeud_UsersNo);
		event->gge_Event.gge_UsersData.ggeud_Data = NULL;
		event->gge_Type = GGE_TYPE_NOOP;
	}
}


/****if* ggpackets.c/GGPacketHandlerUserListReply()
 *
 *  NAME
 *    GGPacketHandlerUserListReply()
 *
 *  SYNOPSIS
 *    static VOID GGPacketHandlerUserListReply(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
 *
 *  FUNCTION
 *    Funkcja obs³uguje pakiet GGP_TYPE_USER_LIST_REPLY generuj±c odpowiednie zdarzenie GGE_TYPE_USER_LIST.
 *
 *  INPUTS
 *    - gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±ce za po³±czenie;
 *    - event -- wska¼nik na strukturê zdarzenia, któr± handler ma wype³niæ;
 *    - pac -- wska¼nik na strukturê GGPHeader pakietu do wy¶wietlenia.
 *
 *  SEE ALSO
 *    GGE_TYPE_#?, Inflate()
 *
 *  NOTES
 *    Funkcja wywo³uje funkcjê Inflate(), która otwiera z.library.
 *
 *****/

static VOID GGPacketHandlerUserListReply(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
{
	struct GGPUserList *ul = (struct GGPUserList*)(pac + 1);
	ULONG data_len = pac->ggph_Length - sizeof(struct GGPUserList);
	ENTER();

	ul->ggpul_Version = EndianFix32(ul->ggpul_Version);

	if(ul->ggpul_Type == 0x00) /* odebrali¶my listê od serwera */
	{
		STRPTR data;

		if((data = Inflate(ul->ggpul_Data, &data_len)))
		{
			event->gge_Type = GGE_TYPE_LIST_IMPORT;
			event->gge_Event.gge_ListImport.ggeli_Version = ul->ggpul_Version;
			event->gge_Event.gge_ListImport.ggeli_Data = data;
			event->gge_Event.gge_ListImport.ggeli_Format = ul->ggpul_Format;
		}
		else
		{
			event->gge_Type = GGE_TYPE_ERROR;
			event->gge_Event.gge_Error.ggee_Errno = GGS_ERRNO_MEM;
		}
	}
	else if(ul->ggpul_Type == 0x10) /* serwer zakceptowa³ listê */
	{
		event->gge_Type = GGE_TYPE_LIST_EXPORT;
		event->gge_Event.gge_ListExport.ggele_Version = ul->ggpul_Version;
		event->gge_Event.gge_ListExport.ggele_Accept = TRUE;
	}
	else if(ul->ggpul_Type == 0x12) /* serwer odrzuci³ listê */
	{
		event->gge_Type = GGE_TYPE_LIST_EXPORT;
		event->gge_Event.gge_ListExport.ggele_Version = ul->ggpul_Version;
		event->gge_Event.gge_ListExport.ggele_Accept = FALSE;
	}
	else
	{
		event->gge_Type = GGE_TYPE_NOOP;
	}

	LEAVE();
}


/****if* ggpackets.c/GGPacketHandlerMultilogonInfo()
 *
 *  NAME
 *    GGPacketHandlerMultilogonInfo()
 *
 *  SYNOPSIS
 *    static VOID GGPacketHandlerMultilogonInfo(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
 *
 *  FUNCTION
 *    Funkcja obs³uguje pakiet GGP_TYPE_MULTILOGON_INFO generuj±c odpowiednie zdarzenie GGE_TYPE_MULTILOGON_INFO.
 *
 *  INPUTS
 *    - gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±ce za po³±czenie;
 *    - event -- wska¼nik na strukturê zdarzenia, któr± handler ma wype³niæ;
 *    - pac -- wska¼nik na strukturê GGPHeader pakietu do wy¶wietlenia.
 *
 *  SEE ALSO
 *    GGE_TYPE_#?
 *
 *****/

static VOID GGPacketHandlerMultilogonInfo(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
{
	struct GGPMultilogonInfo *ul = (struct GGPMultilogonInfo*)(pac + 1);
	UBYTE *data = (UBYTE*)&ul->ggpmi_Data;
	ENTER();

	ul->ggpmi_ClientsNo = EndianFix32(ul->ggpmi_ClientsNo);

	event->gge_Type = GGE_TYPE_MULTILOGON_INFO;
	event->gge_Event.gge_MultilogonInfo.ggemi_No = ul->ggpmi_ClientsNo;

	if((event->gge_Event.gge_MultilogonInfo.ggemi_Data = AllocMem(sizeof(struct GGMultilogonInfo) * ul->ggpmi_ClientsNo, MEMF_ANY)))
	{
		LONG i;

		for(i = 0; i < ul->ggpmi_ClientsNo; i++)
		{
			LONG name_len;

			event->gge_Event.gge_MultilogonInfo.ggemi_Data[i].ggmi_Ip = *((ULONG*)data);
			data += sizeof(ULONG);

			event->gge_Event.gge_MultilogonInfo.ggemi_Data[i].ggmi_Flags = EndianFix32(*((ULONG*)data));
			data += sizeof(ULONG);

			event->gge_Event.gge_MultilogonInfo.ggemi_Data[i].ggmi_Features = EndianFix32(*((ULONG*)data));
			data += sizeof(ULONG);

			event->gge_Event.gge_MultilogonInfo.ggemi_Data[i].ggmi_Timestamp = EndianFix32(*((ULONG*)data));
			data += sizeof(ULONG);

			event->gge_Event.gge_MultilogonInfo.ggemi_Data[i].ggmi_Id = EndianFix64(*((UQUAD*)data));
			data += sizeof(UQUAD);

			/* ominiêcie pola o nieznanym przeznaczeniu (zawsze równego 0) */
			data += sizeof(ULONG);

			name_len = EndianFix32(*((ULONG*)data));
			data += sizeof(ULONG);

			event->gge_Event.gge_MultilogonInfo.ggemi_Data[i].ggmi_Name = StrNewLen((STRPTR)data, name_len);
			data += name_len;
		}
	}

	LEAVE();
}

/****if* ggpackets.c/GGPacketHandlerPubDirInfo()
 *
 *  NAME
 *    GGPacketHandlerPubDirInfo()
 *
 *  SYNOPSIS
 *    static VOID GGPacketHandlerPubDirInfo(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
 *
 *  FUNCTION
 *    Funkcja obs³uguje pakiet GGP_TYPE_PUBDIR_RESPONSE generuj±c odpowiednie zdarzenie GGE_TYPE_PUBDIR_INFO.
 *
 *  INPUTS
 *    - gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±ce za po³±czenie;
 *    - event -- wska¼nik na strukturê zdarzenia, któr± handler ma wype³niæ;
 *    - pac -- wska¼nik na strukturê GGPHeader pakietu do wy¶wietlenia.
 *
 *  SEE ALSO
 *    GGE_TYPE_#?
 *
 *****/

static VOID GGPacketHandlerPubDirInfo(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
{
	struct GGPPubDirInfo *i = (struct GGPPubDirInfo*)(pac + 1);
	ENTER();

	if(i->ggppdi_Type == 0x05) /* obs³ugujemy tylko odpowiedzi wyszukiwania */
	{
		struct GGEventPubDirInfo *p = &event->gge_Event.gge_PubDirInfo;
		STRPTR t = (STRPTR)&i->ggppdi_Data;
		STRPTR end = t + pac->ggph_Length - sizeof(i->ggppdi_Type) - sizeof(i->ggppdi_Seq); /* koniec pola ggppdi_Data */

		event->gge_Type = GGE_TYPE_PUBDIR_INFO;
		p->ggepdi_Seq = EndianFix32(i->ggppdi_Seq);

		while(t < end)
		{
			if(StrEqu(t, "FmNumber"))
			{
				t += 9;
				t += StrToLong(t, &p->ggepdi_Uin);
				t++;
			}
			else if(StrEqu(t, "firstname"))
			{
				t += 10;
				p->ggepdi_FirstName = StrNew(t);
				t += StrLen(t) + 1;
			}
			else if(StrEqu(t, "lastname"))
			{
				t += 9;
				p->ggepdi_LastName = StrNew(t);
				t += StrLen(t) + 1;
			}
			else if(StrEqu(t, "nickname"))
			{
				t += 9;
				p->ggepdi_NickName = StrNew(t);
				t += StrLen(t) + 1;
			}
			else if(StrEqu(t, "birthyear"))
			{
				t += 10;
				p->ggepdi_BirthYear = StrNew(t);
				t += StrLen(t) + 1;
			}
			else if(StrEqu(t, "city"))
			{
				t += 5;
				p->ggepdi_City = StrNew(t);
				t += StrLen(t) + 1;
			}
			else if(StrEqu(t, "gender"))
			{
				t += 7;

				if(*t == '2')
					p->ggepdi_Gender = GG_INFO_GENDER_MALE;
				else if(*t == '1')
					p->ggepdi_Gender = GG_INFO_GENDER_FEMALE;

				t += 2;
			}
			else if(StrEqu(t, "status"))
			{
				t += 7;

				if(*t == '2')
					p->ggepdi_Status = GG_STATUS_AVAIL;
				else if(*t == '3')
					p->ggepdi_Status = GG_STATUS_BUSY;
				else if(*t == '1')
					p->ggepdi_Status = GG_STATUS_NOT_AVAIL;
				else if(*t == '8')
					p->ggepdi_Status = GG_STATUS_DND;

				t += 2;
			}
			else
				t++;
		}
	}
	else
		event->gge_Type = GGE_TYPE_NOOP;

	LEAVE();
}

/****if* ggpackets.c/GGPacketHandlerUnknownPacket()
 *
 *  NAME
 *    GGPacketHandlerUnknownPacket()
 *
 *  SYNOPSIS
 *    static VOID GGPacketHandlerUnknownPacket(struct GGSession *gg_sess, struct GGPHeader *pac)
 *
 *  FUNCTION
 *    Funkcja s³u¿y do wys³ania na debug log zawarto¶ci nieznanego pakietu w formie tekstowej.
 *
 *  INPUTS
 *    - gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±ce za po³±czenie;
 *    - pac -- wska¼nik na strukturê GGPHeader pakietu do wy¶wietlenia.
 *
 *  SEE ALSO
 *    GGE_TYPE_#?
 *
 *  NOTES
 *    Funkcja dostêpna tylko przy kompilacji ze zdefiniowanym symbolem __DEBUG__.
 *
 *****/

#ifdef __DEBUG__
static VOID GGPacketHandlerUnknownPacket(struct GGSession *gg_sess, struct GGPHeader *pac)
{
	DumpBinaryData((UBYTE*)pac, pac->ggph_Length);
}
#endif /* __DEBUG__ */

/****if* ggpackets.c/GGHandlePacket()
 *
 *  NAME
 *    GGHandlePacket()
 *
 *  SYNOPSIS
 *    BOOL GGHandlePacket(struct GGSession *gg_sess, struct GGPHeader *pac)
 *
 *  FUNCTION
 *    Funkcja uruchamia odpowiedni handler dla danego typu pakietu.
 *
 *  INPUTS
 *    - gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±ce za po³±czenie;
 *    - event -- wska¼nik na zaalokowan± strukturê zdarzenia do wype³nienia;
 *    - pac -- wska¼nik na strukturê GGPHeader danego pakietu.
 *
 *  RESULT
 *    - TRUE -- je¶li odpowiedni handler zosta³ znaleziony
 *    - FALSE -- handler dla danego typu pakietu nie istnieje
 *
 *****/

BOOL GGHandlePacket(struct GGSession *gg_sess, struct GGEvent *event, struct GGPHeader *pac)
{
	BOOL result = FALSE;
	ENTER();

	switch(pac->ggph_Type)
	{
		case GGP_TYPE_WELCOME:
			GGPacketHandlerWelcome(gg_sess, event, pac);
			result = TRUE;
		break;

		case GGP_TYPE_LOGIN_OK:
		case GGP_TYPE_LOGIN_FAIL:
		case GGP_TYPE_LOGIN_FAIL2:
			GGPacketHandlerLogin(gg_sess, event, pac);
			result = TRUE;
		break;

		case GGP_TYPE_STATUS_CHANGE:
			GGPacketHandlerStatusChange(gg_sess, event, pac);
			result = TRUE;
		break;

		case GGP_TYPE_LIST_STATUS:
			GGPacketHandlerListStatus(gg_sess, event, pac);
			result = TRUE;
		break;

		case GGP_TYPE_TYPING_NOTIFY:
			GGPacketHandlerTypingNotify(gg_sess, event, pac);
			result = TRUE;
		break;

		case GGP_TYPE_RECV_OWN_MSG:
		case GGP_TYPE_RECV_MSG:
			GGPacketHandlerRecvMsg(gg_sess, event, pac);
			result = TRUE;
		break;

		case GGP_TYPE_USER_DATA:
			GGPacketHandlerUserData(gg_sess, event, pac);
			result = TRUE;
		break;

		case GGP_TYPE_USER_LIST_REPLY:
			GGPacketHandlerUserListReply(gg_sess, event, pac);
			result = TRUE;
		break;

		case GGP_TYPE_MULTILOGON_INFO:
			GGPacketHandlerMultilogonInfo(gg_sess, event, pac);
			result = TRUE;
		break;

		case GGP_TYPE_PUBDIR_RESPONSE:
			GGPacketHandlerPubDirInfo(gg_sess, event, pac);
			result = TRUE;
		break;

		default:
#ifdef __DEBUG__
			GGPacketHandlerUnknownPacket(gg_sess, pac);
#endif /* __DEBUG__ */
			event->gge_Type = GGE_TYPE_NOOP;
			result = TRUE; /* zakomentowanie spowoduje zamykanie po³±czenia po odebraniu nieznanego pakietu */
	}

	LEAVE();
	return result;
}


