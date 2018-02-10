/*
 * Copyright (c) 2013 - 2018 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* GGLib/gghandlers.c
 *
 *  NAME
 *    gghandlers.c -- Plik zawieraj±cy funkcje obs³ugi zdarzeñ.
 *
 *  AUTHOR
 *    Filip Maryjañski
 *
 *  DESCRIPTION
 *    Definicje funkcji przeznaczonych do obs³ugi zdarzeñ zwi±zanych z sieci± GG.
 *
 ********/

#include <proto/exec.h>
#include <proto/socket.h>
#include <dos/dos.h>
#include <sys/errno.h>
#include <libvstring.h>
#include <string.h>
#include "globaldefines.h"
#include "ggdefs.h"
#include "support.h"
#include "gglib.h"
#include "ggpackets.h"
#include "gghandlers.h"

#define SocketBase gg_sess->SocketBase

extern struct Library *SysBase;


/****if* gghandlers.c/GGHandleConnecting()
 *
 *  NAME
 *    GGHandleConnecting()
 *
 *  SYNOPSIS
 *    LONG GGHandleConnecting(struct GGSession *gg_sess, struct GGEvent *gg_event)
 *
 *  FUNCTION
 *    Funkcja obs³uguje stan GGS_STATE_CONNECTING, sprawdza czy nieblokuj±cy socket nawi±za³
 *    po³±czenie i jest gotowy do u¿ycia. Przenosi po³±czenie w stan GG_STATE_CONNECTED. W przypadku
 *    braku po³±czenia zwraca GGH_RETURN_WAIT.
 *
 *  INPUTS
 *    - gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±ce za po³±czenie;
 *    - gg_event -- wska¼nik na strukturê GGEvent opisuj±c± nastêpne zdarzenie.
 *
 *  RESULT
 *    Kod powrotu z gghandlers.h.
 *    Dodatkowo modyfikowany jest status po³±czenia i ewentualnie kod b³êdu w gg_sess.
 *
 *****/

LONG GGHandleConnecting(struct GGSession *gg_sess, struct GGEvent *gg_event)
{
	LONG result = GGH_RETURN_ERROR;
	ENTER();

	if(send(gg_sess->ggs_Socket, NULL, 0, 0) < 0)
	{
		if(Errno() == EWOULDBLOCK)
			return GGH_RETURN_WAIT;
		gg_sess->ggs_SessionState = GGS_STATE_ERROR;
		return GGH_RETURN_ERROR;
	}
	else
	{
		gg_sess->ggs_SessionState = GGS_STATE_CONNECTED;
		return GGH_RETURN_NEXT;
	}

	LEAVE();
	return result;
}


/****if* gghandlers.c/GGHandleConnected()
 *
 *  NAME
 *    GGHandleConnected()
 *
 *  SYNOPSIS
 *    LONG GGHandleConnected(struct GGSession *gg_sess, struct GGEvent *gg_event)
 *
 *  FUNCTION
 *    Funkcja obs³uguje stan GGS_STATE_CONNECTED.
 *
 *  INPUTS
 *    gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±ce za po³±czenie;
 *    gg_event -- wska¼nik na strukturê GGEvent opisuj±c± nastêpne zdarzenie.
 *
 *  RESULT
 *    Kod powrotu z gghandlers.h.
 *    W przypadku b³êdu modyfikowany jest status po³±czenia i kod b³êdu w gg_sess.
 *
 *****/

LONG GGHandleConnected(struct GGSession *gg_sess, struct GGEvent *event)
{
	LONG result = GGH_RETURN_ERROR;
	struct GGPHeader *pac;
	ENTER();

	if(gg_sess->ggs_WriteBuffer != NULL) /* je¶li mamy co¶ do wys³ania to wysy³amy */
	{
		LONG res = GGWriteData(gg_sess);

		if(res == -1)
		{
			GG_SESSION_ERROR(gg_sess, GGS_ERRNO_SOCKET_LIB);
		}
	}

	if(gg_sess->ggs_SessionState != GGS_STATE_ERROR && (pac = GGReceivePacket(gg_sess)))
	{
		if(GGHandlePacket(gg_sess, event, pac))
		{
			/* tutaj nie ustawiamy nic w event, poniewa¿ zdarzenie zosta³o
			 * wype³nione przez handler pakietu uruchomiony w GGHandlePacket() */
			result = GGH_RETURN_WAIT;
		}
		else
		{
			event->gge_Type = GGE_TYPE_ERROR;
			event->gge_Event.gge_Error.ggee_Errno = GGS_ERRNO_UNKNOWN_PACKET;
			GG_SESSION_ERROR(gg_sess, GGS_ERRNO_UNKNOWN_PACKET);
		}
		FreeVec(pac);
	}
	else if(gg_sess->ggs_SessionState == GGS_STATE_DISCONNECTING)
	{
		event->gge_Type = GGE_TYPE_DISCONNECT;
		result = GGH_RETURN_WAIT;
	}
	else if(gg_sess->ggs_Errno == GGS_ERRNO_TRYAGAIN)
	{
		gg_sess->ggs_Errno = GGS_ERRNO_OK;
		event->gge_Type = GGE_TYPE_NOOP;
		result = GGH_RETURN_WAIT;
	}
	else if(gg_sess->ggs_SessionState == GGS_STATE_ERROR)
	{
		result = GGH_RETURN_ERROR;
	}

	gg_sess->ggs_Check = GGS_CHECK_READ;

	if(gg_sess->ggs_WriteBuffer != NULL)
		gg_sess->ggs_Check |= GGS_CHECK_WRITE;

	LEAVE();
	return result;
}


/****if* gghandlers.c/GGHandleDisconnecting()
 *
 *  NAME
 *    GGHandleDisconnecting()
 *
 *  SYNOPSIS
 *    LONG GGHandleDisconnecting(struct GGSession *gg_sess, struct GGEvent *gg_event)
 *
 *  FUNCTION
 *    Funkcja obs³uguje stan GGS_STATE_DISCONNECTED.
 *
 *  INPUTS
 *    gg_sess -- wska¼nik na strukturê GGSession odpowiadaj±ce za po³±czenie;
 *    gg_event -- wska¼nik na strukturê GGEvent opisuj±c± nastêpne zdarzenie.
 *
 *  RESULT
 *    Kod powrotu z gghandlers.h.
 *    W przypadku b³êdu modyfikowany jest status po³±czenia i kod b³êdu w gg_sess.
 *
 *****/

LONG GGHandleDisconnecting(struct GGSession *gg_sess, struct GGEvent *event)
{
	LONG result = GGH_RETURN_WAIT;

	event->gge_Type = GGE_TYPE_DISCONNECT;

	return result;
}
