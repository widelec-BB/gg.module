/*
 * Copyright (c) 2013 - 2022 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <proto/openurl.h>
#include <libvstring.h>
#include <gglib.h>

#include "globaldefines.h"
#include "multilogonlist.h"
#include "locale.h"
#include "class.h"

extern struct Library *SysBase, *MUIMasterBase, *UtilityBase, *LocaleBase, *OpenURLBase;

struct MUI_CustomClass *MultilogonListClass;

struct MLP_InsertData {ULONG MethodID; ULONG count; struct GGMultilogonInfo *entries;};

struct MultilogonlistData
{
	Object *menu_strip;
	Object *gg_module;
};

static IPTR MultilogonListDispatcher(VOID);
const struct EmulLibEntry MultilogonListGate = {TRAP_LIB, 0, (VOID(*)(VOID))MultilogonListDispatcher};

struct MUI_CustomClass *CreateMultilogonListClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_List, NULL, sizeof(struct MultilogonlistData), (APTR)&MultilogonListGate);
	MultilogonListClass = cl;
	return cl;
}

VOID DeleteMultilogonListClass(VOID)
{
	if (MultilogonListClass) MUI_DeleteCustomClass(MultilogonListClass);
}

static IPTR MultilogonListNew(Class *cl, Object *obj, struct opSet *msg)
{
	Object *mstrp, *discon;

	obj = (Object*)DoSuperNew((IPTR)cl, (IPTR)obj,
		MUIA_Unicode, TRUE,
		MUIA_Background, MUII_ReadListBack,
		MUIA_Frame, MUIV_Frame_ReadList,
		MUIA_List_Format, (IPTR)"PREPARSE=\33c, PREPARSE=\33c, PREPARSE=\33c, PREPARSE=\33c",
		MUIA_List_Title, TRUE,
		MUIA_ContextMenu, (IPTR)(mstrp = MUI_NewObjectM(MUIC_Menustrip,
			MUIA_Group_Child, MUI_NewObjectM(MUIC_Menu,
				MUIA_Group_Child, (IPTR)(discon = MUI_NewObjectM(MUIC_Menuitem,
					MUIA_Unicode, TRUE,
					MUIA_Menuitem_Title, GetString(MSG_MULTILOGON_WINDOW_LIST_MENU_DISCONNECT),
				TAG_END)),
			TAG_END),
		TAG_END)),
	TAG_MORE, (IPTR)msg->ops_AttrList);

	if(obj)
	{
		struct MultilogonlistData *d = INST_DATA(cl, obj);

		d->menu_strip = mstrp;

		if((d->gg_module = (Object*)GetTagData(MLA_GGModule, (ULONG)NULL, msg->ops_AttrList)))
		{
			DoMethod(discon, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, (IPTR)obj, 1,
			 MLM_Disconnect);

			return (IPTR)obj;
		}
	}

	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR MultilogonListDispose(Class *cl, Object *obj, Msg msg)
{
	struct MultilogonlistData *d = INST_DATA(cl, obj);

	MUI_DisposeObject(d->menu_strip);

	return (IPTR)DoSuperMethodA(cl, obj, msg);
}

static IPTR MultilogonListConstruct(Class *cl, Object *obj, struct MUIP_List_Construct *msg)
{
	struct GGMultilogonInfo *src = (struct GGMultilogonInfo*)msg->entry;
	struct GGMultilogonInfo *dst;

	if((dst = AllocPooled(msg->pool, sizeof(struct GGMultilogonInfo))))
	{
		struct Locale *locale;

		dst->ggmi_Ip = src->ggmi_Ip;
		dst->ggmi_Flags = src->ggmi_Flags;
		dst->ggmi_Features = src->ggmi_Features;
		dst->ggmi_Id = src->ggmi_Id;
		dst->ggmi_Name = StrNew(src->ggmi_Name);

		dst->ggmi_Timestamp = src->ggmi_Timestamp - 2 * 366 * 24 * 3600 - 6 * 365 * 24 * 3600; /* convert unix timestamp to amiga timestamp */

		if((locale = OpenLocale(NULL)))
		{
			dst->ggmi_Timestamp -= 60 * locale->loc_GMTOffset;
			CloseLocale(locale);
		}
	}

	return (IPTR)dst;
}

static IPTR MultilogonListDestruct(Class *cl, Object *obj, struct MUIP_List_Destruct *msg)
{
	struct GGMultilogonInfo *e = msg->entry;

	if(e)
	{
		if(e->ggmi_Name)
			StrFree(e->ggmi_Name);

		FreePooled(msg->pool, e, sizeof(struct GGMultilogonInfo));
	}
	return (IPTR)0;
}

static IPTR MultilogonListDisplay(Class *cl, Object *obj, struct MUIP_List_Display *msg)
{
	struct GGMultilogonInfo *e = msg->entry;
	static UBYTE number[20];
	static UBYTE ip[16];
	static UBYTE time[25];
	static struct ClockData date;

	if(e)
	{
		FmtNPut((STRPTR)number, "%ld", sizeof(number), msg->array[-1] + 1);
		FmtNPut((STRPTR)ip, "%ld.%ld.%ld.%ld", sizeof(ip), (e->ggmi_Ip & 0xFF000000) >> 24, (e->ggmi_Ip & 0x00FF0000) >> 16,
		 (e->ggmi_Ip & 0x0000FF00) >> 8, e->ggmi_Ip & 0x000000FF);
		Amiga2Date(e->ggmi_Timestamp, &date);
		FmtNPut((STRPTR)time, "%lu.%02lu.%02lu %02lu:%02lu:%02lu", sizeof(time), (ULONG)date.year, (ULONG)date.month, (ULONG)date.mday, (ULONG)date.hour, (ULONG)date.min, (ULONG)date.sec);

		msg->array[0] = number;
		msg->array[1] = ip;
		msg->array[2] = e->ggmi_Name;
		msg->array[3] = time;
	}
	else
	{
		msg->array[0] = GetString(MSG_MULTILOGON_WINDOW_LIST_NO);
		msg->array[1] = GetString(MSG_MULTILOGON_WINDOW_LIST_IP);
		msg->array[2] = GetString(MSG_MULTILOGON_WINDOW_LIST_NAME);
		msg->array[3] = GetString(MSG_MULTILOGON_WINDOW_LIST_LOGON_TIME);
	}

	return (IPTR)0;
}

static IPTR MultilogonListInsertData(Class *cl, Object *obj, struct MLP_InsertData *msg)
{
	LONG i;

	DoMethod(obj, MUIM_List_Clear);

	for(i = 0; i < msg->count; i++)
		DoMethod(obj, MUIM_List_InsertSingle, (IPTR)&msg->entries[i], MUIV_List_Insert_Bottom);

	return (IPTR)0;
}

static IPTR MultilogonListDisconnect(Class *cl, Object *obj)
{
	struct MultilogonlistData *d = INST_DATA(cl, obj);
	struct GGMultilogonInfo *act;

	DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&act);

	if(act)
		DoMethod(d->gg_module, GGM_DisconnectMultilogon, (IPTR)&act->ggmi_Id);

	return (IPTR)0;
}

static IPTR MultilogonListOpenWebPubDir(Class *cl, Object *obj)
{
	URL_OpenA("http://ipubdir.gadu-gadu.pl/ngg/", NULL);

	return (IPTR)0;
}


static IPTR MultilogonListDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW: return(MultilogonListNew(cl, obj, (struct opSet*) msg));
		case OM_DISPOSE: return(MultilogonListDispose(cl, obj, msg));
		case MUIM_List_Construct: return(MultilogonListConstruct(cl, obj, (struct MUIP_List_Construct*)msg));
		case MUIM_List_Destruct: return(MultilogonListDestruct(cl, obj, (struct MUIP_List_Destruct*)msg));
		case MUIM_List_Display: return(MultilogonListDisplay(cl, obj, (struct MUIP_List_Display*)msg));
		case MLM_InsertData: return(MultilogonListInsertData(cl, obj, (struct MLP_InsertData*)msg));
		case MLM_Disconnect: return(MultilogonListDisconnect(cl, obj));
		case MLM_OpenWebPubDir: return(MultilogonListOpenWebPubDir(cl, obj));
		default: return (DoSuperMethodA(cl, obj, msg));
	}
}
