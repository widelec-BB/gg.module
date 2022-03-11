/*
 * Copyright (c) 2013 - 2022 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* GGLib/gglib.c
 *
 *  NAME
 *    gglib.c -- G��wny plik �r�d�owy biblioteki
 *
 *  AUTHOR
 *    Filip Maryja�ski
 *
 *  DESCRIPTION
 *    Definicje funkcji przeznaczonych do komunikacji z sieci� GG.
 *
 ********/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/socket.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <libvstring.h>
#include <sys/ioctl.h>
#include <dos/dos.h>
#include <netdb.h>
#include <errno.h>
#include <openssl/ssl.h>
#include "globaldefines.h"
#include "support.h"
#include "endianess.h"
#include "gghandlers.h"
#include "ggpackets.h"
#include "ggmessage.h"
#include "gglib.h"

#define SocketBase gg_sess->SocketBase

extern struct Library *SysBase, *UtilityBase, *DOSBase, *OpenSSL3Base;

__attribute__((section(".text.const"))) const UBYTE GGLibVTag[] = VERSTAG;

/****f* gglib.c/GGCreateSessionTagList()
 *
 *  NAME
 *    GGCreateSessionTagList()
 *
 *  SYNOPSIS
 *    - struct GGSession *GGCreateSessionTagList(ULONG uin, STRPTR password, struct TagItem *taglist)
 *    - struct GGSession *GGCreateSessionTags(ULONG uin, STRPTR password, ULONG tag1, ...)
 *
 *  FUNCTION
 *    Funkcja rozpoczyna asynchroniczne nawi�zywanie po��czenia z sieci� GG.
 *
 *  INPUTS
 *    - uin -- numer GG;
 *    - password -- has�o;
 *    - taglist -- lista tag�w z opcjonalnymi parametrami.
 *
 *  TAGS
 *    - GGA_CreateSession_Status -- ULONG -- status do ustawienia po nawi�zaniu po��czenia;
 *    - GGA_CreateSession_Status_Desc -- STRPTR -- opis statusu do ustawienia po nawi�zaniu po��czenia,
 *       NULL oznacza brak opisu;
 *    - GGA_CreateSession_ImageSize -- UBYTE -- maksymalny rozmiar odbieranych obrazk�w.
 *
 *   RESULT
 *     Funkcja zwraca wska�nik na struktur� GGSession lub NULL w przypadku b��du.
 *
 *****/

struct GGSession *GGCreateSessionTagList(ULONG uin, STRPTR password, struct TagItem *taglist)
{
	struct GGSession *gg_sess = NULL;

	ENTER();

	if(uin && password)
	{
		if((gg_sess = AllocMem(sizeof(struct GGSession), MEMF_ANY | MEMF_CLEAR)))
		{
			if((SocketBase = OpenLibrary("bsdsocket.library", 0)))
			{
				if((gg_sess->ggs_Pass = StrNew(password)))
				{
					STRPTR desc = (STRPTR)GetTagData(GGA_CreateSession_Status_Desc, (ULONG)NULL, taglist);
					ULONG status = GetTagData(GGA_CreateSession_Status, GG_STATUS_AVAIL, taglist);

					if(desc)
					{
						switch(status)
						{
							case GG_STATUS_NOT_AVAIL:
								status = GG_STATUS_NOT_AVAIL_DESCR;
							break;

							case GG_STATUS_FFC:
								status = GG_STATUS_FFC_DESCR;
							break;

							case GG_STATUS_AVAIL:
								status = GG_STATUS_AVAIL_DESCR;
							break;

							case GG_STATUS_BUSY:
								status = GG_STATUS_BUSY_DESCR;
							break;

							case GG_STATUS_DND:
								status = GG_STATUS_DND_DESCR;
							break;

							case GG_STATUS_INVISIBLE:
								status = GG_STATUS_INVISIBLE_DESCR;
							break;
						}
					}
					gg_sess->ggs_Socket = -1;
					gg_sess->ggs_Uin = uin;
					gg_sess->ggs_Status = status;
					gg_sess->ggs_StatusDescription = StrNew(desc);
					gg_sess->ggs_ImageSize = GetTagData(GGA_CreateSession_Image_Size, 0, taglist);
					gg_sess->ggs_SessionState = GGS_STATE_DISCONNECTED;
					gg_sess->ggs_Check |= GGS_CHECK_WRITE; /* biblioteka b�dzie najpierw pisa� (SSL handshake) */
					tprintf("GGCreateSession() succeded\n");
				}
				else
					GG_SESSION_ERROR(gg_sess, GGS_ERRNO_MEM);
			}
			else
				GG_SESSION_ERROR(gg_sess, GGS_ERRNO_MEM);
		}
	}

	LEAVE();
	return gg_sess;
}

/****f* gglib.c/GGFreeSession()
 *
 *  NAME
 *    GGFreeSession()
 *
 *  SYNOPSIS
 *    VOID GGFreeSession(struct GGSession *gg_sess)
 *
 *  FUNCTION
 *    Funkcja zwalnia zasoby zaalokowane przez po��czenie z sieci� GG.
 *
 *  INPUTS
 *    gg_sess -- struktura GGSession odpowiadaj�ce za zwalniane po��czenie.
 *
 *****/

VOID GGFreeSession(struct GGSession *gg_sess)
{
	ENTER();
	if(gg_sess)
	{
		if(gg_sess->ggs_Socket != -1)
		{
			if (gg_sess->ggs_SSL)
				SSL_shutdown(gg_sess->ggs_SSL);
			GGWriteData(gg_sess);
		}

		if (gg_sess->ggs_SSL)
			SSL_free(gg_sess->ggs_SSL);

		if (gg_sess->ggs_SSLCtx)
			SSL_CTX_free(gg_sess->ggs_SSLCtx);

		if(gg_sess->ggs_Socket != -1)
			CloseSocket(gg_sess->ggs_Socket);

		if(SocketBase)
			CloseLibrary(SocketBase);

		if(gg_sess->ggs_StatusDescription)
			StrFree(gg_sess->ggs_StatusDescription);

		if(gg_sess->ggs_WriteBuffer)
			FreeVec(gg_sess->ggs_WriteBuffer);

		if(gg_sess->ggs_RecvBuffer)
			FreeVec(gg_sess->ggs_RecvBuffer);

		if(gg_sess->ggs_Pass)
			StrFree(gg_sess->ggs_Pass);

		FreeMem(gg_sess, sizeof(struct GGSession));
	}
	LEAVE();
}

/****f* gglib.c/GGConnect()
 *
 *  NAME
 *    GGConnect()
 *
 *  SYNOPSIS
 *    BOOL GGConnect(struct GGSession *gg_sess, STRPTR server, USHORT port)
 *
 *  FUNCTION
 *    Funkcja s�u�y do rozpocz�cia nawi�zywania po��czenia z sieci� GG. Nawi�zywanie
 *    jest tylko rozpoczynane, poniewa� po��czenie jest asynchroniczne. Po wykonaniu
 *    tej funkcji nale�y przej�� do oczekiwania na zdarzenia dotycz�ce socketu
 *    po��czenia i je�li takie wyst�pi� wywo�a� GGWatchEvent().
 *
 *  INPUTS
 *    gg_sess -- struktura GGSession odpowiadaj�ce za po��czenie.
 *
 *  RESULT
 *    - TRUE -- sukces
 *    - FALSE -- w.p.p.
 *
 *****/

BOOL GGConnect(struct GGSession *gg_sess, STRPTR server, USHORT port)
{
	ENTER();
	BOOL result = FALSE;

	if(gg_sess && server && port != 0)
	{
		if((gg_sess->ggs_Ip = inet_addr(server)) != INADDR_NONE)
		{
			gg_sess->ggs_Port = port;

			if((gg_sess->ggs_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) != -1)
			{
				struct sockaddr_in my_addr = {0};

				my_addr.sin_family = AF_INET;

				if(bind(gg_sess->ggs_Socket, (struct sockaddr *) &my_addr, sizeof(my_addr)) != -1)
				{
					LONG non_block = 1;

					if(IoctlSocket(gg_sess->ggs_Socket, FIONBIO, (caddr_t)&non_block) != -1)
					{
						struct sockaddr_in addrname = {0};
						struct hostent *he = (struct hostent *)gethostbyname(InetToStr(gg_sess->ggs_Ip));

						if(he)
						{
							addrname.sin_port = htons(port);
							addrname.sin_family = AF_INET;
							addrname.sin_addr = *((struct in_addr *) he->h_addr);

							if(connect(gg_sess->ggs_Socket, (struct sockaddr*)&addrname, sizeof(addrname)) != -1 || Errno() == EINPROGRESS)
							{
								if((gg_sess->ggs_SSLCtx = SSL_CTX_new(TLS_client_method())))
								{
									SSL_CTX_set_verify(gg_sess->ggs_SSLCtx, SSL_VERIFY_NONE, NULL);

									if ((gg_sess->ggs_SSL = SSL_new(gg_sess->ggs_SSLCtx)))
									{
										SSL_set_fd(gg_sess->ggs_SSL, gg_sess->ggs_Socket);
										gg_sess->ggs_SessionState = GGS_STATE_CONNECTING;
										gg_sess->ggs_Check |= GGS_CHECK_WRITE;
										result = TRUE;
									}
									else
										GG_SESSION_ERROR(gg_sess, GGS_ERRNO_SOCKET_LIB);
								}
								else
									GG_SESSION_ERROR(gg_sess, GGS_ERRNO_SOCKET_LIB);
							}
							else
								GG_SESSION_ERROR(gg_sess, GGS_ERRNO_SOCKET_LIB);
						}
						else
							GG_SESSION_ERROR(gg_sess, GGS_ERRNO_SERVER_OFF);
					}
					else
						GG_SESSION_ERROR(gg_sess, GGS_ERRNO_SOCKET_LIB);
				}
				else
					GG_SESSION_ERROR(gg_sess, GGS_ERRNO_SOCKET_LIB);
			}
			else
				GG_SESSION_ERROR(gg_sess, GGS_ERRNO_SOCKET_LIB);
		}
	}
	LEAVE();
	return result;
}

/****f* gglib.c/GGWatchEvent()
 *
 *  NAME
 *    GGWatchEvent()
 *
 *  SYNOPSIS
 *    struct GGEvent *GGWatchEvent(struct GGSession *gg_sess)
 *
 *  FUNCTION
 *    Funkcja s�u�y do obserwacji po��czenia z sieci� GG.
 *
 *  INPUTS
 *    gg_sess -- wska�nik na struktur� GGSession odpowiadaj�c� za obserwowane po��czenie.
 *
 *  RESULT
 *    Wska�nik na struktur� GGEvent opisuj�c� aktualne zdarzenie.
 *
 *****/

struct GGEvent *GGWatchEvent(struct GGSession *gg_sess)
{
	struct GGEvent *event;
	ENTER();

	if((event = AllocMem(sizeof(struct GGEvent), MEMF_ANY | MEMF_CLEAR)))
	{
		event->gge_Type = GGE_TYPE_NOOP;

		while(TRUE)
		{
			LONG handler_result = GGH_RETURN_UNKNOWN;

			switch(gg_sess->ggs_SessionState)
			{
				case GGS_STATE_CONNECTING:
					handler_result = GGHandleConnecting(gg_sess, event);
				break;

				case GGS_STATE_CONNECTED:
					handler_result = GGHandleConnected(gg_sess, event);
				break;

				case GGS_STATE_DISCONNECTING:
					handler_result = GGHandleDisconnecting(gg_sess, event);
				break;

				case GGS_STATE_ERROR:
					handler_result = GGH_RETURN_ERROR;
				break;
			}

			switch(handler_result)
			{
				case GGH_RETURN_NEXT:
				continue;

				default:
					tprintf("Unknown handler's return code!\n");
				case GGH_RETURN_UNKNOWN:
					tprintf("Unknown connection state!\n");
					GGFreeEvent(event);
					return NULL;
				case GGH_RETURN_ERROR:
					event->gge_Type = GGE_TYPE_ERROR;
					event->gge_Event.gge_Error.ggee_Errno = gg_sess->ggs_Errno;
				case GGH_RETURN_WAIT:
					LEAVE();
				return event;
			}
		}
	}

	LEAVE();
	return event;
}

/****f* gglib.c/GGFreeEvent()
 *
 *  NAME
 *    GGFreeEvent()
 *
 *  SYNOPSIS
 *    VOID GGFreeEvent(struct GGEvent *event)
 *
 *  FUNCTION
 *    Funkcja s�u�y do zwalniania pami�ci zaalokowanej na struktur� GGEvent.
 *
 *  INPUTS
 *    event -- wska�nik na struktur� do zwolnienia.
 *
 *****/

VOID GGFreeEvent(struct GGEvent *event)
{
	if(event)
	{
		switch(event->gge_Type)
		{
			/* w razie potrzeby zwalniania czego� szczeg�lnego dla danego typu - doda� tutaj */
			case GGE_TYPE_STATUS_CHANGE:
				if(event->gge_Event.gge_StatusChange.ggesc_Description)
					FreeVec(event->gge_Event.gge_StatusChange.ggesc_Description);
			break;

			case GGE_TYPE_LIST_STATUS:
				if(event->gge_Event.gge_ListStatus.ggels_StatusChanges)
					FreeVec(event->gge_Event.gge_ListStatus.ggels_StatusChanges);
			break;

			case GGE_TYPE_RECV_MSG:
				if(event->gge_Event.gge_RecvMsg.ggerm_Txt)
					FreeVec(event->gge_Event.gge_RecvMsg.ggerm_Txt);
				if(event->gge_Event.gge_RecvMsg.ggerm_ImagesIds)
					FreeVec(event->gge_Event.gge_RecvMsg.ggerm_ImagesIds);
			break;

			case GGE_TYPE_USER_DATA:
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
				}
			break;

			case GGE_TYPE_LIST_IMPORT:
				if(event->gge_Event.gge_ListImport.ggeli_Data)
					FreeVec(event->gge_Event.gge_ListImport.ggeli_Data);
			break;

			case GGE_TYPE_MULTILOGON_INFO:
				if(event->gge_Event.gge_MultilogonInfo.ggemi_Data)
				{
					LONG i;

					for(i = 0; i < event->gge_Event.gge_MultilogonInfo.ggemi_No; i++)
					{
						if(event->gge_Event.gge_MultilogonInfo.ggemi_Data[i].ggmi_Name)
							StrFree(event->gge_Event.gge_MultilogonInfo.ggemi_Data[i].ggmi_Name);
					}
					FreeMem(event->gge_Event.gge_MultilogonInfo.ggemi_Data, sizeof(struct GGMultilogonInfo) * event->gge_Event.gge_MultilogonInfo.ggemi_No);
				}
			break;

			case GGE_TYPE_IMAGE_DATA:
				if(event->gge_Event.gge_ImageData.ggeid_FileName)
					StrFree(event->gge_Event.gge_ImageData.ggeid_FileName);
				if(event->gge_Event.gge_ImageData.ggeid_Data)
					FreeMem(event->gge_Event.gge_ImageData.ggeid_Data, event->gge_Event.gge_ImageData.ggeid_DataSize);
			break;

			case GGE_TYPE_PUBDIR_INFO:
				if(event->gge_Event.gge_PubDirInfo.ggepdi_FirstName)
					StrFree(event->gge_Event.gge_PubDirInfo.ggepdi_FirstName);
				if(event->gge_Event.gge_PubDirInfo.ggepdi_LastName)
					StrFree(event->gge_Event.gge_PubDirInfo.ggepdi_LastName);
				if(event->gge_Event.gge_PubDirInfo.ggepdi_NickName)
					StrFree(event->gge_Event.gge_PubDirInfo.ggepdi_NickName);
				if(event->gge_Event.gge_PubDirInfo.ggepdi_BirthYear)
					StrFree(event->gge_Event.gge_PubDirInfo.ggepdi_BirthYear);
				if(event->gge_Event.gge_PubDirInfo.ggepdi_City)
					StrFree(event->gge_Event.gge_PubDirInfo.ggepdi_City);
			break;
		}
		FreeMem(event, sizeof(struct GGEvent));
	}
}

/****f* gglib.c/GGNotifyList()
 *
 *  NAME
 *    GGNotifyList()
 *
 *  SYNOPSIS
 *    BOOL GGNotifyList(struct GGSession *gg_sess, ULONG *uins, UBYTE *types, LONG no)
 *
 *  FUNCTION
 *    Funkcja s�u�y do wys�ania do serwera listy kontakt�w, aby serwer wiedzia�,
 *    o kt�rych kontaktach nas informowa�. Lista powinna zosta� podana w formie wska�nika
 *    na tablic� ULONG�w oraz wska�nika na tablic� UBYTE�w. W tablicy ULONG�w powinny znale��
 *    si� numery GG kontkatk�w (uin GG), a w odpowiadaj�cych indeksach tablicy UBYTE�w rodzaje.
 *    Rozpoznawane rodzaje to:
 *    - GG_USER_NORMAL -- zwyk�y kontakt;
 *    - GG_USER_OFFLINE -- kontakt, dla kt�rego b�dziemy niewidoczni;
 *    - GG_USER_BLOCKED -- kontakt, od kt�rego nie chcemy otrzymywa� wiadomo�ci.
 *    Je�li uins b�dzie r�wne NULL zostanie wys�any pakiet odpowiadaj�cy za pust� list� kontakt�w.
 *    Je�li types b�dzie r�wne NULL wszystkie kontakty zostan� wys�ane jako GG_USER_NORMAL.
 *
 *  INPUTS
 *    - gg_sess -- wska�nik na struktur� GGSession odpowiadaj�c� za po��czenie;
 *    - uins -- wska�nik na tablic� numer�w GG (uin GG) kontakt�w z listy;
 *    - types -- wska�nik na tablic� rodzaj�w kontkat�w z listy;
 *    - no -- d�ugo�� tablicy uins i types
 *
 *   RESULT
 *    - TRUE -- je�li si� uda�o;
 *    - FALSE -- w.p.p.
 *
 *   NOTES
 *     Tablice, na kt�re wskazuj� uins i types musz� mie� d�ugo�� dok�adnie r�wn� no!
 *
 *****/

BOOL GGNotifyList(struct GGSession *gg_sess, ULONG *uins, UBYTE *types, LONG no)
{
	BOOL result = FALSE;
	ENTER();

	if(gg_sess && GG_SESSION_IS_CONNECTED(gg_sess))
	{
		if(uins == NULL)
		{
			BYTE *pac;
			ULONG len;

			if((pac = GGPacketCreateTags(GGP_TYPE_LIST_EMPTY, &len, TAG_END)))
			{
				if(GGAddToWriteBuffer(gg_sess, pac, len))
				{
					result = TRUE;
				}
			}
		}
		else
		{
			BYTE *buf;
			LONG i;

			while(no > 400)
			{
				ULONG buf_len = sizeof(struct GGPHeader) + 400 * sizeof(ULONG) + 400 * sizeof(UBYTE);

				if((buf = AllocVec(buf_len, MEMF_CLEAR | MEMF_ANY)))
				{
					BYTE *temp = buf + sizeof(struct GGPHeader);
					((struct GGPHeader*)buf)->ggph_Type = EndianFix32(GGP_TYPE_NOTIFY_NORMAL);
					((struct GGPHeader*)buf)->ggph_Length = EndianFix32(buf_len - sizeof(struct GGPHeader));

					for(i = 0; i < 400; i++)
					{
						*((ULONG*)temp) = EndianFix32(uins[i]);
						temp += sizeof(ULONG);
						*temp = types ? types[i] : GG_USER_NORMAL;
						temp++;
					}
					if(!GGAddToWriteBuffer(gg_sess, buf, buf_len))
					{
						FreeVec(buf);
						return FALSE;
					}
				}
				no -= 400;
				uins += 400;
				types += 400;
			}

			ULONG buf_len = sizeof(struct GGPHeader) + no * sizeof(ULONG) + no * sizeof(UBYTE);

			if((buf = AllocVec(buf_len, MEMF_CLEAR | MEMF_ANY)))
			{
				BYTE *temp = buf + sizeof(struct GGPHeader);
				((struct GGPHeader*)buf)->ggph_Type = EndianFix32(GGP_TYPE_NOTIFY_LAST);
				((struct GGPHeader*)buf)->ggph_Length = EndianFix32(buf_len - sizeof(struct GGPHeader));

				for(i = 0; i < no; i++)
				{
					*((ULONG*)temp) = EndianFix32(uins[i]);
					temp += sizeof(ULONG);
					*temp = types ? types[i] : GG_USER_NORMAL;
					temp++;
				}
				if(GGAddToWriteBuffer(gg_sess, buf, buf_len))
					result = TRUE;
				else
					FreeVec(buf);
			}
		}
	}

	LEAVE();
	return result;
}

/****f* gglib.c/GGChangeStatus()
 *
 *  NAME
 *    GGChangeStatus()
 *
 *  SYNOPSIS
 *    BOOL GGChangeStatus(struct GGSession *gg_sess, ULONG status, STRPTR desc)
 *
 *  FUNCTION
 *    Funkcja s�u�y do zmiany statusu aktualnie zalogowanego u�ytkownika.
 *    Obs�ugiwane statusy to:
 *    - GG_STATUS_NOT_AVAIL -- "niedost�pny";
 *    - GG_STATUS_FFC -- "poGGadaj ze mn�";
 *    - GG_STATUS_AVAIL -- "dost�pny";
 *    - GG_STATUS_BUSY -- "zaraz wracam";
 *    - GG_STATUS_DND -- "nie przeszkadza�";
 *    - GG_STATUS_INVISIBLE -- "niewidoczny".
 *    Dok�adanie bitu oznaczaj�cego wyst�pienie opisu realizowane jest automatycznie
 *    w przypadku gdy desc != NULL.
 *    Funkcja automatycznie aktualizuje pola dotycz�ce statusu i opisu
 *    w strukturze opisuj�cej sesj�.
 *
 *  INPUTS
 *    - gg_sess -- wska�nik na struktur� GGSession odpowiadaj�c� za po��czenie;
 *    - status -- maska bitowa opisuj�ca status;
 *    - desc -- opis statusu (mo�e by� NULL -> brak opisu).
 *
 *   RESULT
 *    - TRUE -- je�li si� uda�o;
 *    - FALSE -- w.p.p.
 *
 *   SEE ALSO
 *     GGSession
 *
 *****/

BOOL GGChangeStatus(struct GGSession *gg_sess, ULONG status, STRPTR desc)
{
	BOOL result = FALSE;
	ENTER();

	if(gg_sess && GG_SESSION_IS_CONNECTED(gg_sess))
	{
		BYTE *pac;
		ULONG len;

		if(desc == NULL)
		{
			pac = GGPacketCreateTags(GGP_TYPE_NEW_STATUS, &len,
				GGA_CreatePacket_ULONG, status,
				GGA_CreatePacket_ULONG, GGLIB_STATUS_FLAGS,
				GGA_CreatePacket_ULONG, 0,
			TAG_END);
		}
		else
		{
			switch(status)
			{
				case GG_STATUS_NOT_AVAIL:
					status = GG_STATUS_NOT_AVAIL_DESCR;
				break;

				case GG_STATUS_FFC:
					status = GG_STATUS_FFC_DESCR;
				break;

				case GG_STATUS_AVAIL:
					status = GG_STATUS_AVAIL_DESCR;
				break;

				case GG_STATUS_BUSY:
					status = GG_STATUS_BUSY_DESCR;
				break;

				case GG_STATUS_DND:
					status = GG_STATUS_DND_DESCR;
				break;

				case GG_STATUS_INVISIBLE:
					status = GG_STATUS_INVISIBLE_DESCR;
				break;
			}

			pac = GGPacketCreateTags(GGP_TYPE_NEW_STATUS, &len,
				GGA_CreatePacket_ULONG, status,
				GGA_CreatePacket_ULONG, GGLIB_STATUS_FLAGS,
				GGA_CreatePacket_ULONG, StrLen(desc),
				GGA_CreatePacket_STRPTR, (ULONG)desc,
			TAG_END);
		}

		if(pac)
		{
			if(GGAddToWriteBuffer(gg_sess, pac, len))
			{
				gg_sess->ggs_Status = status;
				if(gg_sess->ggs_StatusDescription)
					StrFree(gg_sess->ggs_StatusDescription);
				gg_sess->ggs_StatusDescription = desc ? StrNew(desc) : NULL;
				result = TRUE;
			}
		}
	}

	LEAVE();
	return result;
}

/****f* gglib.c/GGPing()
 *
 *  NAME
 *    GGPing()
 *
 *  SYNOPSIS
 *    BOOL GGPing(struct GGSession *gg_sess)
 *
 *  FUNCTION
 *    Funkcja s�u�y do wys�ania do serwera pakietu utrzymania po��czenia.
 *    Pakiet ten nale�y wysy�a� co 60 sekund, aby serwer nie zamkn�� po��czenia.
 *
 *  INPUTS
 *    gg_sess -- wska�nik na struktur� GGSession odpowiadaj�c� za po��czenie.
 *
 *   RESULT
 *    - TRUE -- je�li si� uda�o;
 *    - FALSE -- w.p.p.
 *
 *****/

BOOL GGPing(struct GGSession *gg_sess)
{
	BOOL result = FALSE;
	ENTER();

	if(gg_sess && GG_SESSION_IS_CONNECTED(gg_sess))
	{
		BYTE *pac;
		ULONG len;

		if((pac = GGPacketCreateTags(GGP_TYPE_PING, &len, TAG_END)))
		{
			if(GGAddToWriteBuffer(gg_sess, pac, len))
				result = TRUE;
			else
				FreeVec(pac);
		}
	}

	LEAVE();
	return result;
}

/****f* gglib.c/GGTypingNotify()
 *
 *  NAME
 *    GGTypingNotify()
 *
 *  SYNOPSIS
 *    BOOL GGTypingNotify(struct GGSession *gg_sess, ULONG uin, USHORT len)
 *
 *  FUNCTION
 *    Funkcja s�u�y do wys�ania do serwera pakietu informuj�cego o pisaniu.
 *
 *  INPUTS
 *    - gg_sess -- wska�nik na struktur� GGSession odpowiadaj�c� za po��czenie;
 *    - uin -- numer do kt�rego piszemy;
 *    - len -- d�ugo�� tekstu ju� napisanego.
 *
 *   RESULT
 *    - TRUE -- je�li si� uda�o;
 *    - FALSE -- w.p.p.
 *
 *****/

BOOL GGTypingNotify(struct GGSession *gg_sess, ULONG uin, USHORT len)
{
	BOOL result = FALSE;
	ENTER();

	if(gg_sess && GG_SESSION_IS_CONNECTED(gg_sess))
	{
		BYTE *pac;
		ULONG plen;

		if((pac = GGPacketCreateTags(GGP_TYPE_TYPING_NOTIFY, &plen,
			GGA_CreatePacket_USHORT, len,
			GGA_CreatePacket_ULONG, uin,
		TAG_END)))
		{
			if((GGAddToWriteBuffer(gg_sess, pac, plen)))
				result = TRUE;
			else
				FreeVec(pac);
		}
	}

	LEAVE();
	return result;
}

/****f* gglib.c/GGSendMessage()
 *
 *  NAME
 *    GGSendMessage()
 *
 *  SYNOPSIS
 *    BOOL GGSendMessage(struct GGSession *gg_sess, ULONG uin, STRPTR msg, STRPTR image)
 *
 *  FUNCTION
 *    Funkcja s�u�y do wys�ania wiadomo�ci.
 *
 *  INPUTS
 *    - gg_sess -- wska�nik na struktur� GGSession odpowiadaj�c� za po��czenie;
 *    - uin -- numer do kt�rego piszemy;
 *    - msg -- tre�� wiadomo�ci, czysty tekst, kodowanie UTF-8;
 *    - image -- identyfikator obrazka do wys�ania wraz z wiadomo�ci�.
 *
 *   RESULT
 *    - TRUE -- je�li si� uda�o;
 *    - FALSE -- w.p.p.
 *
 *****/

BOOL GGSendMessage(struct GGSession *gg_sess, ULONG uin, STRPTR msg, STRPTR image)
{
	BOOL result = FALSE;
	ENTER();

	if(gg_sess && GG_SESSION_IS_CONNECTED(gg_sess) && (msg || image))
	{
		STRPTR html_msg;
		BYTE *pac;
		struct DateStamp ds;
		ULONG seq;
		ULONG len;

		if((html_msg = GGMessageTextToHTML(msg, image)))
		{
			DateStamp(&ds);

			seq = ds.ds_Days * (24 * 60 * 60);
			seq += (ds.ds_Minute * 60);
			seq += (ds.ds_Tick / TICKS_PER_SECOND);
			seq += 2 * 366 * 24 * 3600 + 6 * 365 * 24 * 3600;

			if((pac = GGPacketCreateTags(GGP_TYPE_SEND_MSG, &len,
				GGA_CreatePacket_ULONG, uin,
				GGA_CreatePacket_ULONG, seq,
				GGA_CreatePacket_ULONG, 0x00000008UL,
				GGA_CreatePacket_ULONG, 0,
				GGA_CreatePacket_ULONG, 0,
				GGA_CreatePacket_STRPTR, (ULONG)html_msg,
			TAG_END)))
			{
				if(GGAddToWriteBuffer(gg_sess, pac, len))
					result = TRUE;
			}
			FreeVec(html_msg);
		}
	}

	LEAVE();
	return result;
}

/****f* gglib.c/GGAddNotify()
 *
 *  NAME
 *    GGAddNotify()
 *
 *  SYNOPSIS
 *    BOOL GGAddNotify(struct GGSession *gg_sess, ULONG uin, UBYTE type)
 *
 *  FUNCTION
 *    Funkcja s�u�y do dodania do listy obserwowanych status�w nowego kontaktu.
 *
 *  INPUTS
 *    - gg_sess -- wska�nik na struktur� GGSession odpowiadaj�c� za po��czenie;
 *    - uin -- numer, kt�ry chcemy doda�;
 *    - type -- typ kontaktu.
 *
 *   RESULT
 *    - TRUE -- je�li si� uda�o;
 *    - FALSE -- w.p.p.
 *
 *   SEE ALSO
 *    GGNotifyList, GGRemoveNotify
 *
 *****/

BOOL GGAddNotify(struct GGSession *gg_sess, ULONG uin, UBYTE type)
{
	BOOL result = FALSE;
	ENTER();

	if(gg_sess && GG_SESSION_IS_CONNECTED(gg_sess))
	{
		BYTE *pac;
		ULONG plen;

		if((pac = GGPacketCreateTags(GGP_TYPE_ADD_NOTIFY, &plen,
			GGA_CreatePacket_ULONG, uin,
			GGA_CreatePacket_UBYTE, type,
		TAG_END)))
		{
			if((GGAddToWriteBuffer(gg_sess, pac, plen)))
				result = TRUE;
			else
				FreeVec(pac);
		}
	}

	LEAVE();
	return result;
}


/****f* gglib.c/GGRemoveNotify()
 *
 *  NAME
 *    GGAddNotify()
 *
 *  SYNOPSIS
 *    BOOL GGAddNotify(struct GGSession *gg_sess, ULONG uin, UBYTE type)
 *
 *  FUNCTION
 *    Funkcja s�u�y do usuni�cia kontaktu z listy obserwowanych status�w.
 *
 *  INPUTS
 *    - gg_sess -- wska�nik na struktur� GGSession odpowiadaj�c� za po��czenie;
 *    - uin -- numer, kt�ry chcemy usun��;
 *    - type -- typ kontaktu.
 *
 *   RESULT
 *    - TRUE -- je�li si� uda�o;
 *    - FALSE -- w.p.p.
 *
 *   SEE ALSO
 *    GGNotifyList, GGAddNotify
 *
 *****/

BOOL GGRemoveNotify(struct GGSession *gg_sess, ULONG uin, UBYTE type)
{
	BOOL result = FALSE;
	ENTER();

	if(gg_sess && GG_SESSION_IS_CONNECTED(gg_sess))
	{
		BYTE *pac;
		ULONG plen;

		if((pac = GGPacketCreateTags(GGP_TYPE_REMOVE_NOTIFY, &plen,
			GGA_CreatePacket_ULONG, uin,
			GGA_CreatePacket_UBYTE, type,
		TAG_END)))
		{
			if((GGAddToWriteBuffer(gg_sess, pac, plen)))
				result = TRUE;
			else
				FreeVec(pac);
		}
	}

	LEAVE();
	return result;
}

/****f* gglib.c/GGRequestContactList()
 *
 *  NAME
 *    GGRequestContactList()
 *
 *  SYNOPSIS
 *    BOOL GGRequestContactList(struct GGSession *gg_sess, UBYTE format)
 *
 *  FUNCTION
 *    Funkcja s�u�y do wys�ania pro�by do serwera o przes�anie wcze�niej wykesportowanej
 *    listy kontakt�w.
 *
 *  INPUTS
 *    - gg_sess -- wska�nik na struktur� GGSession odpowiadaj�c� za po��czenie;
 *    - format -- jedna ze sta�ych GG_LIST_FORMAT_#? okre�laj�ca format listy.
 *
 *   RESULT
 *    - TRUE -- je�li si� uda�o;
 *    - FALSE -- w.p.p.
 *
 *   SEE ALSO
 *    GG_LIST_FORMAT_#?, GGExportContactList()
 *
 *****/

BOOL GGRequestContactList(struct GGSession *gg_sess, UBYTE format)
{
	BOOL result = FALSE;
	ENTER();

	if(gg_sess && GG_SESSION_IS_CONNECTED(gg_sess))
	{
		BYTE *pac;
		ULONG plen;

		if((pac = GGPacketCreateTags(GGP_TYPE_USER_LIST_REQ, &plen,
			GGA_CreatePacket_BYTE,  0x02,    /* typ zapytania, dla importu 0x02 */
			GGA_CreatePacket_ULONG, 0x00,    /* wersja listy (dla importu oryginalny klient wysy�a 0 */
			GGA_CreatePacket_BYTE,  format,  /* typ formatu listy kontakt�w */
			GGA_CreatePacket_BYTE,  0x01,    /* pole o nieznanym przeznaczeniu, zawsze r�wne 0x01 */
		TAG_END)))
		{
			if((GGAddToWriteBuffer(gg_sess, pac, plen)))
				result = TRUE;
			else
				FreeVec(pac);
		}
	}

	LEAVE();
	return result;
}

/****f* gglib.c/GGExportContactList()
 *
 *  NAME
 *    GGExportContactList()
 *
 *  SYNOPSIS
 *    BOOL GGExportContactList(struct GGSession *gg_sess, ULONG ver, UBYTE format, STRPTR list, LONG len)
 *
 *  FUNCTION
 *    Funkcja s�u�y do wys�ania listy kontakt�w na serwer.
 *
 *  INPUTS
 *    - gg_sess -- wska�nik na struktur� GGSession odpowiadaj�c� za po��czenie;
 *    - ver -- najnowsza znana wersja listy kontakt�w;
 *    - format -- jedna ze sta�ych GG_LIST_FORMAT_#? okre�laj�ca format listy;
 *    - list -- lista kontakt�w w wybranym formacie;
 *    - len -- d�ugo�� listy kontakt�w.
 *
 *   RESULT
 *    - TRUE -- je�li si� uda�o;
 *    - FALSE -- w.p.p.
 *
 *   SEE ALSO
 *    GG_LIST_FORMAT_#?, GGRequestContactList()
 *
 *****/

BOOL GGExportContactList(struct GGSession *gg_sess, ULONG ver, UBYTE format, STRPTR list, LONG len)
{
	BOOL result = FALSE;
	ENTER();

	if(gg_sess && GG_SESSION_IS_CONNECTED(gg_sess))
	{
		STRPTR dlist;

		if((dlist = Deflate(list, &len)))
		{
			struct TagItem block = {len, (ULONG)dlist};
			BYTE *pac;
			ULONG plen;

			if((pac = GGPacketCreateTags(GGP_TYPE_USER_LIST_REQ, &plen,
				GGA_CreatePacket_BYTE,   0x00,   /* typ zapytania, dla eksportu 0x00 */
				GGA_CreatePacket_ULONG,  ver,    /* wersja listy */
				GGA_CreatePacket_BYTE,   format, /* typ formatu listy */
				GGA_CreatePacket_BYTE,   0x01,   /* pole o nieznanym przeznaczeniu, zawsze r�wne 0x01 */
				GGA_CreatePacket_BLOCK, (ULONG)&block,
			TAG_END)))
			{
				if((GGAddToWriteBuffer(gg_sess, pac, plen)))
					result = TRUE;
				else
					FreeVec(pac);
			}
			FreeVec(dlist);
		}
	}

	LEAVE();
	return result;
}

/****f* gglib.c/GGDisconnectMultilogon()
 *
 *  NAME
 *    GGDisconnectMultilogon()
 *
 *  SYNOPSIS
 *    BOOL GGDisconnectMultilogon(struct GGSession *gg_sess, QUAD id)
 *
 *  FUNCTION
 *    Funkcja s�u�y do wys�ania listy kontakt�w na serwer.
 *
 *  INPUTS
 *    - gg_sess -- wska�nik na struktur� GGSession odpowiadaj�c� za po��czenie;
 *    - id -- 64-bitowy identyfikator okre�laj�cy po��czenie, kt�re ma zosta� roz��czone.
 *
 *   RESULT
 *    - TRUE -- je�li si� uda�o;
 *    - FALSE -- w.p.p.
 *
 *   SEE ALSO
 *    GGE_TYPE_MULTILOGON_INFO
 *
 *****/

BOOL GGDisconnectMultilogon(struct GGSession *gg_sess, UQUAD id)
{
	BOOL result = FALSE;
	ENTER();

	if(gg_sess && GG_SESSION_IS_CONNECTED(gg_sess))
	{
		BYTE *pac;
		ULONG plen;

		if((pac = GGPacketCreateTags(GGP_TYPE_MULTILOGON_DISCONNECT, &plen,
			GGA_CreatePacket_UQUAD, (IPTR)&id,
		TAG_END)))
		{
			if((GGAddToWriteBuffer(gg_sess, pac, plen)))
				result = TRUE;
			else
				FreeVec(pac);
		}
	}

	LEAVE();
	return result;
}

/****f* gglib.c/GGRequestImage()
 *
 *  NAME
 *    GGRequestImage()
 *
 *  SYNOPSIS
 *    BOOL GGRequestImage(struct GGSession *gg_sess, ULONG uin, STRPTR id)
 *
 *  FUNCTION
 *    Funkcja s�u�y do wys�ania pro�by o przes�anie obrazka.
 *
 *  INPUTS
 *    - gg_sess -- wska�nik na struktur� GGSession odpowiadaj�c� za po��czenie;
 *    - uin -- numer, od kt�rego ��damy obrazka;
 *    - id -- identyfikator obrazka.
 *
 *   RESULT
 *    - TRUE -- je�li si� uda�o;
 *    - FALSE -- w.p.p.
 *
 *   SEE ALSO
 *    GGE_TYPE_IMAGE_DATA, GGEventImageData, GGE_TYPE_IMAGE_REQUEST, GGEventImageRequest, GGSendImageData()
 *
 *****/

BOOL GGRequestImage(struct GGSession *gg_sess, ULONG uin, STRPTR id)
{
	BOOL result = FALSE;
	ENTER();

	if(gg_sess && GG_SESSION_IS_CONNECTED(gg_sess) && id)
	{
		BYTE *pac;
		ULONG plen;
		ULONG crc, size;

		crc = StrByteToByte(id) << 24 | StrByteToByte(id + 2) << 16 | StrByteToByte(id + 4) << 8 |  StrByteToByte(id + 6);
		size = StrByteToByte(id + 8) << 24 | StrByteToByte(id + 10) << 16 | StrByteToByte(id + 12) << 8 |  StrByteToByte(id + 14);

		if((pac = GGPacketCreateTags(GGP_TYPE_SEND_MSG_OLD, &plen,
			GGA_CreatePacket_ULONG, uin, /* numer nadawcy obrazka */
			GGA_CreatePacket_ULONG, 0UL, /* numer sekwencyjny, tutaj mo�e by� 0 */
			GGA_CreatePacket_ULONG, 0x0000004UL, /* klasa wiadomo�ci - obrazek (4) */
			GGA_CreatePacket_UBYTE, 0x00, /* pusta tre�� wiadomo�ci */
			GGA_CreatePacket_UBYTE, 0x04, /* flaga -> zawsze 0x04 */
			GGA_CreatePacket_ULONG, size, /* rozmiar obrazka */
			GGA_CreatePacket_ULONG, crc, /* suma CRC32 obrazka */
		TAG_END)))
		{
			if((GGAddToWriteBuffer(gg_sess, pac, plen)))
				result = TRUE;
			else
				FreeVec(pac);
		}
	}

	LEAVE();
	return result;
}

/****f* gglib.c/GGSendImageData()
 *
 *  NAME
 *    GGSendImageData()
 *
 *  SYNOPSIS
 *    BOOL GGSendImageData(struct GGSession *gg_sess, ULONG uin, BPTR fh)
 *
 *  FUNCTION
 *    Funkcja s�u�y do wys�ania danych obrazka w odpowiedzi na ��danie obrazka.
 *
 *  INPUTS
 *    - gg_sess -- wska�nik na struktur� GGSession odpowiadaj�c� za po��czenie;
 *    - uin -- numer, kt�ry ��da� obrazka;
 *    - fh -- uchwyt do pliku zawieraj�cego obrazek.
 *
 *   RESULT
 *    - TRUE -- je�li si� uda�o;
 *    - FALSE -- w.p.p.
 *
 *   SEE ALSO
 *    GGE_TYPE_IMAGE_DATA, GGEventImageData, GGE_TYPE_IMAGE_REQUEST, GGEventImageRequest, GGSendImageData()
 *
 *****/

BOOL GGSendImageData(struct GGSession *gg_sess, ULONG uin, BPTR fh)
{
	BOOL result = FALSE;
	ENTER();

	if(gg_sess && GG_SESSION_IS_CONNECTED(gg_sess) && fh != (BPTR)0)
	{
		BYTE *pac;
		ULONG plen;
		ULONG crc, size, old_pos;
		UBYTE file_name[17];

		crc = FileCrc32(fh);

		old_pos = Seek(fh, 0, OFFSET_END);
		size = Seek(fh, old_pos, OFFSET_BEGINING);

		FmtNPut(file_name, "%08lx%08lx", sizeof(file_name), crc, size);

		if(size <= 1873)
		{
			UBYTE *pic;

			if((pic = AllocMem(size, MEMF_ANY)))
			{
				if(FRead(fh, pic, size, 1) == 1)
				{
					struct TagItem pic_data = {size, (IPTR)pic};

					if((pac = GGPacketCreateTags(GGP_TYPE_SEND_MSG_OLD, &plen,
						GGA_CreatePacket_ULONG,  uin,             /* numer nadawcy obrazka */
						GGA_CreatePacket_ULONG,  0UL,             /* numer sekwencyjny, tutaj mo�e by� 0 */
						GGA_CreatePacket_ULONG,  0x0000004UL,     /* klasa wiadomo�ci - obrazek (4) */
						GGA_CreatePacket_UBYTE,  0x00,            /* pusta tre�� wiadomo�ci */
						GGA_CreatePacket_UBYTE,  0x05,            /* flaga -> pierwszy pakiet danych: 0x05 */
						GGA_CreatePacket_ULONG,  size,            /* rozmiar obrazka */
						GGA_CreatePacket_ULONG,  crc,             /* suma CRC32 obrazka */
						GGA_CreatePacket_STRPTR, (IPTR)file_name, /* nazwa pliku */
						GGA_CreatePacket_UBYTE,  0x00,            /* bajt ko�cz�cy nazw� pliku */
						GGA_CreatePacket_BLOCK,  (IPTR)&pic_data, /* dane obrazka */
					TAG_END)))
					{
						if((GGAddToWriteBuffer(gg_sess, pac, plen)))
							result = TRUE;
						else
							FreeVec(pac);
					}
				}
				FreeMem(pic, size);
			}
		}
		else
		{
			UBYTE *pic;

			if((pic = AllocMem(1843, MEMF_ANY)))
			{
				struct TagItem pic_data;

				pic_data.ti_Tag = 1843;
				pic_data.ti_Data = (IPTR)pic;

				if(FRead(fh, pic, 1843, 1) == 1)
				{
					if((pac = GGPacketCreateTags(GGP_TYPE_SEND_MSG_OLD, &plen,
						GGA_CreatePacket_ULONG,  uin,             /* numer nadawcy obrazka */
						GGA_CreatePacket_ULONG,  0UL,             /* numer sekwencyjny, teraz zero, p�niej kolejne liczby */
						GGA_CreatePacket_ULONG,  0x0000004UL,     /* klasa wiadomo�ci - obrazek (4) */
						GGA_CreatePacket_UBYTE,  0x00,            /* pusta tre�� wiadomo�ci */
						GGA_CreatePacket_UBYTE,  0x05,            /* flaga -> pierwszy pakiet danych: 0x05 */
						GGA_CreatePacket_ULONG,  size,            /* rozmiar obrazka */
						GGA_CreatePacket_ULONG,  crc,             /* suma CRC32 obrazka */
						GGA_CreatePacket_STRPTR, (IPTR)file_name, /* nazwa pliku */
						GGA_CreatePacket_UBYTE,  0x00,            /* bajt ko�cz�cy nazw� pliku */
						GGA_CreatePacket_BLOCK,  (IPTR)&pic_data, /* dane obrazka */
					TAG_END)))
					{
						if((GGAddToWriteBuffer(gg_sess, pac, plen)))
						{
							ULONG bytes;
							ULONG seq = 1;

							GGWriteData(gg_sess);

							while((bytes = FRead(fh, pic, 1, 1843)))
							{
								pic_data.ti_Tag = bytes;
								pic_data.ti_Data = (IPTR)pic;

								if((pac = GGPacketCreateTags(GGP_TYPE_SEND_MSG_OLD, &plen,
									GGA_CreatePacket_ULONG,  uin,             /* numer nadawcy obrazka */
									GGA_CreatePacket_ULONG,  seq++,           /* numer sekwencyjny, kolejne liczby */
									GGA_CreatePacket_ULONG,  0x0000004UL,     /* klasa wiadomo�ci - obrazek (4) */
									GGA_CreatePacket_UBYTE,  0x00,            /* pusta tre�� wiadomo�ci */
									GGA_CreatePacket_UBYTE,  0x06,            /* flaga -> kolejne pakiety danych: 0x06 */
									GGA_CreatePacket_ULONG,  size,            /* rozmiar obrazka */
									GGA_CreatePacket_ULONG,  crc,             /* suma CRC32 obrazka */
									GGA_CreatePacket_BLOCK,  (IPTR)&pic_data, /* dane obrazka */
								TAG_END)))
								{
									if(!GGAddToWriteBuffer(gg_sess, pac, plen))
										break;

									GGWriteData(gg_sess);
								}
							}

							if(Seek(fh, 0, OFFSET_END) == size)
								result = TRUE;
						}
						else
							FreeVec(pac);
					}
				}
				FreeMem(pic, 1843);
			}
		}
	}

	LEAVE();
	return result;
}

/****f* gglib.c/GGFindInPubDir()
 *
 *  NAME
 *    GGFindInPubDir()
 *
 *  SYNOPSIS
 *    ULONG GGFindInPubDir(struct GGSession *gg_sess, ULONG uin)
 *
 *  FUNCTION
 *    Funkcja s�u�y do wys�ania zapytania do katalogu publicznego, w celu uzyskania
 *    danych dotycz�cych podanego numeru.
 *
 *  INPUTS
 *    - gg_sess -- wska�nik na struktur� GGSession odpowiadaj�c� za po��czenie;
 *    - uin -- numer, kt�rego danych ��damy.
 *
 *   RESULT
 *    Numer sekwencyjny zapytania lub 0 w przypadku b��du.
 *
 *   SEE ALSO
 *    GGE_TYPE_PUBDIR_INFO,
 *
 *****/

ULONG GGFindInPubDir(struct GGSession *gg_sess, ULONG uin)
{
	ULONG result = 0;
	ENTER();

	if(gg_sess && GG_SESSION_IS_CONNECTED(gg_sess))
	{
		TEXT buffer[32];
		BYTE *pac;
		ULONG plen;
		ULONG secs = 2 * 366 * 24 * 3600 - 6 * 365 * 24 * 3600;
		struct DateStamp ds;

		DateStamp(&ds);

		secs += ds.ds_Days * 24 * 60 * 60;
		secs += ds.ds_Minute * 60;
		secs += ds.ds_Tick / TICKS_PER_SECOND;

		FmtNPut(buffer, "%lu", sizeof(buffer), uin);

		if((pac = GGPacketCreateTags(GGP_TYPE_PUBDIR_REQUEST, &plen,
			GGA_CreatePacket_UBYTE, 0x03, /* typ - wyszukiwanie */
			GGA_CreatePacket_ULONG, secs, /* numer sekwencyjny */
			GGA_CreatePacket_STRPTR, (IPTR)"FmNumber", /* zapytanie o podany uin */
			GGA_CreatePacket_BYTE, '\0',
			GGA_CreatePacket_STRPTR, (IPTR)buffer, /* uin */
			GGA_CreatePacket_BYTE, '\0',
		TAG_END)))
		{
			if((GGAddToWriteBuffer(gg_sess, pac, plen)))
				result = secs;
			else
				FreeVec(pac);
		}
	}

	LEAVE();
	return result;
}

/****f* gglib.c/GGCreateImageId()
 *
 *  NAME
 *    GGCreateImageId()
 *
 *  SYNOPSIS
 *    STRPTR GGCreateImageId(BPTR fh)
 *
 *  FUNCTION
 *    Funkcja s�u�y do wygenerowania identyfikatora obrazka.
 *
 *  INPUTS
 *    - fh -- uchwyt do pliku dla kt�rego ma zostata� wygenerowany identyfikator.
 *
 *   RESULT
 *    - wska�nik na statyczny bufor zawieraj�cy wygenerowany identyfikator.
 *
 *   SEE ALSO
 *    GGSendImageData(), GGE_TYPE_IMAGE_REQUEST, GGEventImageRequest
 *
 *****/

STRPTR GGCreateImageId(BPTR fh)
{
	static UBYTE id[17];
	ULONG size, old_pos;

	old_pos = Seek(fh, 0, OFFSET_END);
	size = Seek(fh, old_pos, OFFSET_BEGINING);

	FmtNPut(id, "%08lx%08lx", sizeof(id), FileCrc32(fh), size);

	return (STRPTR)id;
}
