/*
 * Copyright (c) 2013 - 2018 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <kwakwa_api/protocol.h>
#include <gglib.h>
#include <libvstring.h>
#include <proto/locale.h>
#include "globaldefines.h"
#include "events.h"

extern struct Library *SysBase, *LocaleBase;

static inline ULONG StatusTranslate(ULONG gg_status)
{
	if(GG_S_AVAIL(gg_status))
		return KWA_STATUS_AVAIL;
	if(GG_S_BLOCKED(gg_status))
		return KWA_STATUS_BLOCKED;
	if(GG_S_BUSY(gg_status))
		return KWA_STATUS_BUSY;
	if(GG_S_DND(gg_status))
		return KWA_STATUS_DND;
	if(GG_S_FFC(gg_status))
		return KWA_STATUS_FFC;
	if(GG_S_INVISIBLE(gg_status))
		return KWA_STATUS_INVISIBLE;
	return KWA_STATUS_NOT_AVAIL;
}

static inline ULONG NewMsgFlagsTranslate(ULONG gg_flags)
{
	ULONG result = 0;

	if(gg_flags == GG_MSG_NORMAL)
		result |= MSG_FLAG_NORMAL;
	if((gg_flags & GG_MSG_OWN) == GG_MSG_OWN)
		result |= MSG_FLAG_MULTILOGON;

	return result;
}


struct KwaEvent *AddEvent(struct MinList *list, ULONG event_type)
{
	struct KwaEvent *event;

	if((event = AllocKwaEvent()))
	{
		event->ke_ModuleID = MODULE_ID;
		event->ke_Type = event_type;

		AddTail((struct List*)list, (struct Node*)event);
	}

	return event;
}

VOID AddListExportEvent(struct MinList *list, BOOL result)
{
	struct KwaEvent *event;

	if((event = AddEvent(list, KE_TYPE_EXPORT_LIST)))
	{
		event->ke_ExportList.ke_Accepted = result;
	}
}

VOID AddNewAvatarEvent(struct MinList *list, ULONG uin, struct Picture *pic)
{
	struct KwaEvent *event;

	if((event = AddEvent(list, KE_TYPE_NEW_AVATAR)))
	{
		if((event->ke_NewAvatar.ke_ContactID = FmtNew("%lu", uin)))
		{
			event->ke_NewAvatar.ke_Picture = pic;
			return;
		}
		Remove((struct Node*)event);
		FreeKwaEvent(event);
	}

}

BOOL AddHttpGetEvent(struct MinList *list, STRPTR url, STRPTR usr_agent, ULONG methodid, APTR usr_data)
{
	struct KwaEvent *event;

	if((event = AddEvent(list, KE_TYPE_SEND_HTTP_GET)))
	{
		if((event->ke_HttpGet.ke_Url = StrNew(url)))
		{
			event->ke_HttpGet.ke_UserAgent = StrNew(usr_agent);
			event->ke_HttpGet.ke_MethodID = methodid;
			event->ke_HttpGet.ke_UserData = usr_data;
			return TRUE;
		}
		Remove((struct Node*)event);
		FreeKwaEvent(event);
	}
	return FALSE;
}

VOID AddErrorEvent(struct MinList *list, ULONG errno, STRPTR txt)
{
	struct KwaEvent *event;

	if((event = AddEvent(list, KE_TYPE_MODULE_MESSAGE)))
	{
		event->ke_ModuleMessage.ke_Errno = errno;
		event->ke_ModuleMessage.ke_MsgTxt = StrNew(txt);
	}
}

VOID AddEventStatusChange(struct MinList *list, ULONG status, STRPTR desc)
{
	struct KwaEvent *event;

	if((event = AddEvent(list, KE_TYPE_STATUS_CHANGE)))
	{
		event->ke_StatusChange.ke_NewStatus = StatusTranslate(status);
		event->ke_StatusChange.ke_Description = StrNew(desc);
	}
}

VOID AddEventListChange(struct MinList *list, ULONG uin, ULONG status, STRPTR desc)
{
	struct KwaEvent *event;

	if((event = AddEvent(list, KE_TYPE_LIST_CHANGE)))
	{
		if((event->ke_ListChange.ke_ContactID = FmtNew("%lu", uin)))
		{
			event->ke_ListChange.ke_Description = StrNew(desc);
			event->ke_ListChange.ke_NewStatus = StatusTranslate(status);
			return;
		}

		Remove((struct Node*)event);
		FreeKwaEvent(event);
	}
}

VOID AddEventTypingNotify(struct MinList *list, ULONG uin, ULONG len)
{
	struct KwaEvent *event;

	if((event = AddEvent(list, KE_TYPE_TYPING_NOTIFY)))
	{
		if((event->ke_ListChange.ke_ContactID = FmtNew("%lu", uin)))
		{
			event->ke_TypingNotify.ke_TxtLen = len;
			return;
		}

		Remove((struct Node*)event);
		FreeKwaEvent(event);
	}
}

VOID AddEventNewMessage(struct MinList *list, ULONG uin, STRPTR msg_txt, ULONG flags, ULONG nix_timestamp)
{
	struct KwaEvent *event;

	if((event = AddEvent(list, KE_TYPE_NEW_MESSAGE)))
	{
		if((event->ke_NewMessage.ke_ContactID = FmtNew("%lu", uin)))
		{
			if((event->ke_NewMessage.ke_Txt = StrNew(msg_txt)))
			{


				event->ke_NewMessage.ke_Flags = NewMsgFlagsTranslate(flags);
				event->ke_NewMessage.ke_TimeStamp = UnixToAmigaTimestamp(nix_timestamp);
				return;
			}
			FmtFree(event->ke_NewMessage.ke_ContactID);
		}
		Remove((struct Node*)event);
		FreeKwaEvent(event);
	}
}

VOID StatusEvent(struct MinList *list, struct GGEvent *gge, ULONG myuin)
{
	if(gge->gge_Type == GGE_TYPE_STATUS_CHANGE)
	{
		struct GGEventStatusChange *sc = &gge->gge_Event.gge_StatusChange;

		if(sc->ggesc_Uin == myuin)
			AddEventStatusChange(list, sc->ggesc_Status, sc->ggesc_Description);

		AddEventListChange(list, sc->ggesc_Uin, sc->ggesc_Status, sc->ggesc_Description);
	}
	else if(gge->gge_Type == GGE_TYPE_LIST_STATUS)
	{
		struct GGEventListStatus *ls = &gge->gge_Event.gge_ListStatus;
		LONG i;

		for(i = 0; i < ls->ggels_ChangesNo; i++)
		{
			if(ls->ggels_StatusChanges[i].ggesc_Uin == myuin)
				AddEventStatusChange(list, ls->ggels_StatusChanges[i].ggesc_Status, ls->ggels_StatusChanges[i].ggesc_Description);

			AddEventListChange(list, ls->ggels_StatusChanges[i].ggesc_Uin, ls->ggels_StatusChanges[i].ggesc_Status,
			 ls->ggels_StatusChanges[i].ggesc_Description);
		}
	}
}

VOID AddEventNewPicture(struct MinList *list, ULONG uin, ULONG flags, ULONG nix_timestamp, APTR data, ULONG size)
{
	struct KwaEvent *event;

	if(data && (event = AddEvent(list, KE_TYPE_NEW_PICTURE)))
	{
		if((event->ke_NewPicture.ke_ContactID = FmtNew("%lu", uin)))
		{
			event->ke_NewPicture.ke_Flags = NewMsgFlagsTranslate(flags);
			event->ke_NewPicture.ke_TimeStamp = UnixToAmigaTimestamp(nix_timestamp);
			event->ke_NewPicture.ke_Data = data;
			event->ke_NewPicture.ke_DataSize = size;
			return;
		}
		Remove((struct Node*)event);
		FreeKwaEvent(event);
	}
}
