/*
 * Copyright (c) 2013 - 2018 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <clib/alib_protos.h>
#include <kwakwa_api/protocol.h>
#include <libvstring.h>
#include <proto/ezxml.h>
#include <proto/locale.h>
#include "locale.h"
#include "class.h"
#include "events.h"
#include "gui.h"
#include "multilogonlist.h"
#include "support.h"
#include "globaldefines.h"

#define GG_PING_TIMEOUT 60

struct GetAvatarUsrData
{
	STRPTR url;
	STRPTR cache_key;
	ULONG uin;
};

struct PictureQueueEntry
{
	struct MinNode node;
	ULONG uin;
	ULONG flags;
	ULONG timestamp;
	UBYTE filename[17];
	ULONG size;
	ULONG act_pos;
	APTR data;
};

struct PubDirQueueEntry
{
	struct MinNode node;
	ULONG seq;
	Object *obj;
	ULONG method;
};

struct GGP_ParseUserData {ULONG MethodID; struct GGEventUsersData *udata;};
struct GGP_GetAvatar {ULONG MethodID; ULONG uin; STRPTR key;};
struct GGP_ParseXMLList {ULONG MethodID; struct GGEventListImport *list;};
struct GGP_DisconnectMultilogon {ULONG MethodID; QUAD *id;};
struct GGP_SendImageData {ULONG MethodID; struct GGEventImageRequest *ir;};
struct GGP_RecvMsg {ULONG MethodID; struct GGEventRecvMsg *rm;};
struct GGP_ReceiveImageData {ULONG MethodID; struct GGEventImageData *id;};
struct GGP_ParsePubDirInfo {ULONG MethodID; struct GGEventPubDirInfo *ipdi;};

extern struct Library *SysBase, *DOSBase, *IntuitionBase, *UtilityBase, *EzxmlBase, *LocaleBase;

BOOL GGWriteData(struct GGSession*); /* get rid of "implict declaration" warning */

ULONG strlen(STRPTR a) /* for MakeDirAll()... */
{
	return StrLen(a);
}

static inline ULONG TranslateStatus(ULONG status)
{
	if(KWA_S_AVAIL(status))
		status = GG_STATUS_AVAIL;
	else if(KWA_S_BUSY(status))
		status = GG_STATUS_BUSY;
	else if(KWA_S_DND(status))
		status = GG_STATUS_DND;
	else if(KWA_S_FFC(status))
		status = GG_STATUS_FFC;
	else if(KWA_S_INVISIBLE(status))
		status = GG_STATUS_INVISIBLE;
	else
		status = GG_STATUS_NOT_AVAIL;

	return status;
}

static IPTR mNew(Class *cl, Object *obj, struct opSet *msg)
{
	if((obj = (Object*)DoSuperMethodA(cl, obj, (Msg)msg)))
	{
		BPTR avatars_dir, pictures_dir;
		LONG amake_res = 0, pmake_res = 0;
		struct ObjData *d = INST_DATA(cl, obj);

		NewList((struct List*)&d->EventsList);
		NewList((struct List*)&d->PicturesQueue);
		NewList((struct List*)&d->PubDirQueue);

		if((d->AppObj = (Object*)GetTagData(KWAA_AppObject, (IPTR)NULL, msg->ops_AttrList)))
		{
			if(!(avatars_dir = Lock(CACHE_AVATARS_DIR, ACCESS_READ)))
				amake_res = MakeDirAll(CACHE_AVATARS_DIR);

			if(!(pictures_dir = Lock(CACHE_PICTURES_DIR, ACCESS_READ)))
				pmake_res = MakeDirAll(CACHE_PICTURES_DIR);

			if((avatars_dir || amake_res) && (pictures_dir || pmake_res))
			{
				BPTR fh;

				d->Timeout = GG_PING_TIMEOUT;

				if(avatars_dir)
					UnLock(avatars_dir);

				if(pictures_dir)
					UnLock(pictures_dir);

				if((fh = Open(CACHE_LIST_VERSION, MODE_OLDFILE)))
				{
					FRead(fh, &d->ListVersion, sizeof(ULONG), 1);
					Close(fh);
				}

				if((d->PrefsPanel = CreatePrefsPage()))
				{
					d->GuiTagList[0].ti_Tag = KWAG_PrefsEntry;
					d->GuiTagList[0].ti_Data = (ULONG)PROTOCOL_NAME;

					d->GuiTagList[1].ti_Tag = KWAG_PrefsPage;
					d->GuiTagList[1].ti_Data = (ULONG)d->PrefsPanel;

					d->GuiTagList[2].ti_Tag = KWAG_Window;
					d->GuiTagList[2].ti_Data = (ULONG)CreateMultilogonWindow(obj);

					d->GuiTagList[3].ti_Tag = KWAG_ToolsEntry;
					d->GuiTagList[3].ti_Data = (ULONG)MUI_NewObjectM(MUIC_Menuitem,
						MUIA_Menuitem_Title, GetString(MSG_MULTILOGON_MENU_ENTRY_TITLE),
					TAG_END);

					DoMethod(d->GuiTagList[3].ti_Data, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, d->GuiTagList[2].ti_Data, 3,
					 MUIM_Set, MUIA_Window_Open, TRUE);

					d->GuiTagList[4].ti_Tag = KWAG_ToolsEntry;
					d->GuiTagList[4].ti_Data = (ULONG)MUI_NewObjectM(MUIC_Menuitem,
						MUIA_Menuitem_Title, GetString(MSG_PUBDIR_MENU_ENTRY_TITLE),
					TAG_END);

					DoMethod(d->GuiTagList[4].ti_Data, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, (IPTR)findobj(USD_MULTILOGON_WINDOW_LIST, d->GuiTagList[2].ti_Data), 1,
					 MLM_OpenWebPubDir);

					return (IPTR)obj;
				}
			}
		}
	}

	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}


static IPTR mDispose(Class *cl, Object *obj, Msg msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	BPTR fh;
	struct Node *n;

	if((fh = Open(CACHE_LIST_VERSION, MODE_NEWFILE)))
	{
		FWrite(fh, &d->ListVersion, sizeof(ULONG), 1);
		Close(fh);
	}

	DoMethod(obj, KWAM_FreeEvents, (IPTR)&d->EventsList);

	if(d->GGSession)
		GGFreeSession(d->GGSession);

	d->GGSession = NULL;

	while((n = RemHead((struct List*)&d->PubDirQueue)))
		FreeMem(n, sizeof(struct PubDirQueueEntry));

	while((n = RemHead((struct List*)&d->PicturesQueue)))
		FreeMem(n, sizeof(struct PictureQueueEntry));

	return DoSuperMethodA(cl, obj, msg);
}


static IPTR mGet(Class *cl, Object *obj, struct opGet *msg)
{
	struct ObjData *d = (struct ObjData*)INST_DATA(cl, obj);

	switch (msg->opg_AttrID)
	{
		case KWAA_Socket:
			if(d->GGSession)
				*msg->opg_Storage = (ULONG)d->GGSession->ggs_Socket;
			else
				*msg->opg_Storage = (ULONG)-1;
		return TRUE;

		case KWAA_GuiTagList:
			*msg->opg_Storage = (ULONG)d->GuiTagList;
		return TRUE;

		case KWAA_ProtocolName:
			*msg->opg_Storage = (ULONG)PROTOCOL_NAME;
		return TRUE;

		case KWAA_ModuleID:
			*msg->opg_Storage = (ULONG)MODULE_ID;
		return TRUE;

		case KWAA_WantRead:
			if(d->GGSession && GG_SESSION_CHECK_READ(d->GGSession))
				*msg->opg_Storage = (ULONG)TRUE;
			else
				*msg->opg_Storage = (ULONG)FALSE;
		return TRUE;

		case KWAA_WantWrite:
			if(d->GGSession && GG_SESSION_CHECK_WRITE(d->GGSession))
				*msg->opg_Storage = (ULONG)TRUE;
			else
				*msg->opg_Storage = (ULONG)FALSE;
		return TRUE;

		case KWAA_UserID:
			*msg->opg_Storage = xget(findobj(USD_PREFS_GG_BASIC_UIN_STRING, d->PrefsPanel), MUIA_String_Contents);
		return TRUE;

		default:
			return DoSuperMethodA(cl, obj, (Msg)msg);
	}
}

static IPTR mConnect(Class *cl, Object *obj, struct KWAP_Connect *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	BOOL result = FALSE;
	STRPTR uin_str = (STRPTR)xget(findobj(USD_PREFS_GG_BASIC_UIN_STRING, d->PrefsPanel), MUIA_String_Contents);
	STRPTR password = (STRPTR)xget(findobj(USD_PREFS_GG_BASIC_PASS_STRING, d->PrefsPanel), MUIA_String_Contents);
	ULONG uin;
	ENTER();

	if(StrToLong(uin_str, &uin) != -1)
	{
		if(!d->GGSession)
		{
			tprintf("creating new session...\n");
			d->GGSession = GGCreateSessionTags(uin, password,
				GGA_CreateSession_Image_Size, 255,
				GGA_CreateSession_Status, TranslateStatus(msg->Status),
				GGA_CreateSession_Status_Desc, (ULONG)msg->Description,
			TAG_END);
		}

		if(d->GGSession)
		{
			tprintf("session already exists\n");
			if(d->GGSession->ggs_Uin == uin)
			{
				tprintf("uin correct!\n");
				if(d->ServerIP[0] == 0x00)
				{
					STRPTR url;

					tprintf("no server addr!\n");

					if((url = FmtNew("appmsg.gadu-gadu.pl/appsvc/appmsg_ver8.asp?fmnumber=%ld&fmt=2&lastmsg=0&version=" GGLIB_DEFAULT_CLIENT_VERSION, uin)))
					{
						tprintf("url: %ls\n", url);
						tprintf("first Event: %lp\n", d->EventsList.mlh_Head);
						AddHttpGetEvent(&d->EventsList, url, GG_HTTP_USERAGENT, GGM_HubDone, NULL);
						tprintf("first Event2: %lp\n", d->EventsList.mlh_Head);
						result = TRUE;
					}
					else
						AddErrorEvent(&d->EventsList, ERRNO_OUT_OF_MEMORY, NULL);
				}
				else
				{
					tprintf("try to connect\n");
					if(GGConnect(d->GGSession, d->ServerIP, GG_DEFAULT_PORT))
					{
						result = TRUE;
						AddErrorEvent(&d->EventsList, ERRNO_ONLY_MESSAGE, GetString(MSG_MODULE_MSG_START_CONNECTING));
					}
					else
						AddErrorEvent(&d->EventsList, ERRNO_CONNECTION_FAILED, NULL);
				}
			}
			else
			{
				tprintf("free session...\n");
				GGFreeSession(d->GGSession);

				result = mConnect(cl, obj, msg);
			}
		}
		else
		{
			AddErrorEvent(&d->EventsList, ERRNO_OUT_OF_MEMORY, NULL);
		}
	}
	else
	{
		tprintf("StrToLong failed!\n");
		AddErrorEvent(&d->EventsList, ERRNO_LOGIN_FAILED, NULL);
	}

	LEAVE();
	return (IPTR)result;
}

static IPTR mDisconnect(Class *cl, Object *obj, struct KWAP_Disconnect *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	ENTER();

	if(!GGChangeStatus(d->GGSession, GG_STATUS_NOT_AVAIL, msg->Description))
		AddErrorEvent(&d->EventsList, ERRNO_OUT_OF_MEMORY, "GGChangeStatus()");

	if(d->GGSession)
		GGWriteData(d->GGSession);

	LEAVE();
	return (IPTR)TRUE;
}

static IPTR mWatchEvents(Class *cl, Object *obj, struct KWAP_WatchEvents *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	struct GGEvent *gg_event;

	if(msg->CanRead || msg->CanWrite)
	{
		if((gg_event = GGWatchEvent(d->GGSession)))
		{
			switch(gg_event->gge_Type)
			{
				case GGE_TYPE_LOGIN_FAIL:
					AddErrorEvent(&d->EventsList, ERRNO_LOGIN_FAILED, GetString(MSG_MODULE_MSG_LOGIN_FAILED));
				break;

				case GGE_TYPE_DISCONNECT:
					GGFreeSession(d->GGSession);
					d->GGSession = NULL;
					AddEvent(&d->EventsList, KE_TYPE_DISCONNECT);
					AddErrorEvent(&d->EventsList, ERRNO_ONLY_MESSAGE, GetString(MSG_MODULE_MSG_DISCONNECTED));
				break;

				case GGE_TYPE_LOGIN_SUCCESS:
					AddEvent(&d->EventsList, KE_TYPE_CONNECT);
				break;

				case GGE_TYPE_STATUS_CHANGE:
				case GGE_TYPE_LIST_STATUS:
					StatusEvent(&d->EventsList, gg_event, d->GGSession->ggs_Uin);
				break;

				case GGE_TYPE_TYPING_NOTIFY:
					AddEventTypingNotify(&d->EventsList, gg_event->gge_Event.gge_TypingNotify.ggetn_Uin, gg_event->gge_Event.gge_TypingNotify.ggetn_Length);
				break;

				case GGE_TYPE_RECV_MSG:
					DoMethod(obj, GGM_RecvMsg, (IPTR)&gg_event->gge_Event.gge_RecvMsg);
				break;

				case GGE_TYPE_ERROR:
					switch(gg_event->gge_Event.gge_Error.ggee_Errno)
					{
						case GGS_ERRNO_MEM:
							AddErrorEvent(&d->EventsList, ERRNO_OUT_OF_MEMORY, NULL);
						break;

						case GGS_ERRNO_HUB_FAILED:
						case GGS_ERRNO_INTERRUPT:
						case GGS_ERRNO_SERVER_OFF:
						case GGS_ERRNO_SOCKET_LIB:
							AddErrorEvent(&d->EventsList, ERRNO_CONNECTION_FAILED, NULL);
						break;

						case GGS_ERRNO_UNKNOWN_PACKET:
							AddErrorEvent(&d->EventsList, ERRNO_ONLY_MESSAGE, GetString(MSG_MOUDLE_MSG_UNKNOWN_PACKET));
						break;
					}
				break;


				case GGE_TYPE_USER_DATA:
					DoMethod(obj, GGM_ParseUserData, (IPTR)&gg_event->gge_Event.gge_UsersData);
				break;

				case GGE_TYPE_LIST_IMPORT:
					DoMethod(obj, GGM_ParseXMLList, (IPTR)&gg_event->gge_Event.gge_ListImport);
				break;

				case GGE_TYPE_LIST_EXPORT:
					AddListExportEvent(&d->EventsList, gg_event->gge_Event.gge_ListExport.ggele_Accept);
					if(gg_event->gge_Event.gge_ListExport.ggele_Accept)
					{
						d->ListVersion = gg_event->gge_Event.gge_ListExport.ggele_Version;
						AddErrorEvent(&d->EventsList, ERRNO_ONLY_MESSAGE, GetString(MSG_MODULE_MSG_LIST_EXPORT_OK));
					}
					else
						AddErrorEvent(&d->EventsList, ERRNO_ONLY_MESSAGE, GetString(MSG_MODULE_MSG_LIST_EXPORT_FAIL));
				break;

				case GGE_TYPE_MULTILOGON_INFO:
					DoMethod(findobj(USD_MULTILOGON_WINDOW_LIST, (Object*)d->GuiTagList[2].ti_Data), MLM_InsertData,
					 gg_event->gge_Event.gge_MultilogonInfo.ggemi_No, (IPTR)gg_event->gge_Event.gge_MultilogonInfo.ggemi_Data);
				break;

				case GGE_TYPE_IMAGE_REQUEST:
					DoMethod(obj, GGM_SendImageData, (IPTR)&gg_event->gge_Event.gge_ImageRequest);
				break;

				case GGE_TYPE_IMAGE_DATA:
					DoMethod(obj, GGM_ReceiveImageData, (IPTR)&gg_event->gge_Event.gge_ImageData);
				break;

				case GGE_TYPE_PUBDIR_INFO:
					DoMethod(obj, GGM_ParsePubDirInfo, (IPTR)&gg_event->gge_Event.gge_PubDirInfo);
				break;

				case GGE_TYPE_NOOP:
				case GGE_TYPE_CONNECTED:
				break;

				default:
					tprintf("unknown event: %ld\n", gg_event->gge_Type);
			}
			GGFreeEvent(gg_event);
		}
	}

	return (IPTR)&d->EventsList;
}

static IPTR mNotifyList(Class *cl, Object *obj, struct KWAP_NotifyList *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);

	if(msg->EntriesNo == 0)
	{
		if(!GGNotifyList(d->GGSession, NULL, NULL, 0))
			AddErrorEvent(&d->EventsList, ERRNO_OUT_OF_MEMORY, "GGNotifyList()");
	}
	else if(msg->Entries != NULL)
	{
		ULONG *uins;
		UBYTE *types;

		if((uins = AllocMem(msg->EntriesNo * sizeof(ULONG), MEMF_ANY)))
		{
			if((types = AllocMem(msg->EntriesNo * sizeof(BYTE), MEMF_ANY)))
			{
				ULONG i;
				ULONG act = 0;

				for(i = 0; i < msg->EntriesNo; i++)
				{
					if(StrToLong(msg->Entries[i].nle_EntryID, uins + act) != -1)
					{
						if(KWA_S_BLOCKED(msg->Entries[i].nle_Status))
							types[act] = GG_USER_BLOCKED;
						else if(KWA_S_HIDE(msg->Entries[i].nle_Status))
							types[act] = GG_USER_OFFLINE;
						else
							types[act] = GG_USER_NORMAL;

						act++;
					}
				}

				if(!GGNotifyList(d->GGSession, uins, types, msg->EntriesNo))
					AddErrorEvent(&d->EventsList, ERRNO_OUT_OF_MEMORY, "GGNotifyList()");

				FreeMem(types, msg->EntriesNo * sizeof(BYTE));
			}
			else
				AddErrorEvent(&d->EventsList, ERRNO_OUT_OF_MEMORY, "AllocMem() types");
			FreeMem(uins, msg->EntriesNo * sizeof(ULONG));
		}
		else
			AddErrorEvent(&d->EventsList, ERRNO_OUT_OF_MEMORY, "AllocMem() uins");
	}

	return (IPTR)0;
}

static IPTR mFreeEvent(Class *cl, Object *obj, struct KWAP_FreeEvents *msg)
{
	struct KwaEvent *event;


	while((event = (struct KwaEvent*)RemHead((struct List*)msg->Events)))
	{
		tprintf("free event type: %ld\n", event->ke_Type);
		switch(event->ke_Type)
		{
			case KE_TYPE_STATUS_CHANGE:
				if(event->ke_StatusChange.ke_Description)
					StrFree(event->ke_StatusChange.ke_Description);
			break;

			case KE_TYPE_LIST_CHANGE:
				if(event->ke_ListChange.ke_ContactID)
					FmtFree(event->ke_ListChange.ke_ContactID);
				if(event->ke_ListChange.ke_Description)
					StrFree(event->ke_ListChange.ke_Description);
			break;

			case KE_TYPE_TYPING_NOTIFY:
				if(event->ke_TypingNotify.ke_ContactID)
					FmtFree(event->ke_TypingNotify.ke_ContactID);
			break;

			case KE_TYPE_NEW_MESSAGE:
				if(event->ke_NewMessage.ke_ContactID)
					FmtFree(event->ke_NewMessage.ke_ContactID);
				if(event->ke_NewMessage.ke_Txt)
					StrFree(event->ke_NewMessage.ke_Txt);
			break;

			case KE_TYPE_MODULE_MESSAGE:
				if(event->ke_ModuleMessage.ke_MsgTxt)
					StrFree(event->ke_ModuleMessage.ke_MsgTxt);
			break;

			case KE_TYPE_NEW_AVATAR:
				if(event->ke_NewAvatar.ke_ContactID)
					StrFree(event->ke_NewAvatar.ke_ContactID);
				if(event->ke_NewAvatar.ke_Picture)
					FreePicture(event->ke_NewAvatar.ke_Picture);
			break;

			case KE_TYPE_IMPORT_LIST:
			{
				LONG i;

				for(i = 0; i < event->ke_ImportList.ke_ContactsNo; i++)
				{
					struct ContactEntry *a = &event->ke_ImportList.ke_Contacts[i];

					if(a->entryid)
						StrFree(a->entryid);
					if(a->name)
						StrFree(a->name);
					if(a->firstname)
						StrFree(a->firstname);
					if(a->lastname)
						StrFree(a->lastname);
					if(a->groupname)
						StrFree(a->groupname);
					if(a->birthyear)
						StrFree(a->birthyear);
					if(a->city)
						StrFree(a->city);
				}
				FreeMem(event->ke_ImportList.ke_Contacts, event->ke_ImportList.ke_ContactsNo * sizeof(struct ContactEntry));
			}
			break;

			case KE_TYPE_NEW_PICTURE:
				if(event->ke_NewPicture.ke_ContactID)
					StrFree(event->ke_NewPicture.ke_ContactID);
				if(event->ke_NewPicture.ke_Data)
					FreeMem(event->ke_NewPicture.ke_Data, event->ke_NewPicture.ke_DataSize);
			break;
		}

		FreeKwaEvent(event);
		tprintf("end free event\n");
	}

	return (IPTR)0;
}


static IPTR mChangeStatus(Class *cl, Object *obj, struct KWAP_ChangeStatus *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);

	return (IPTR)GGChangeStatus(d->GGSession, TranslateStatus(msg->Status), msg->Description);
}


static IPTR mSendMessage(Class *cl, Object *obj, struct KWAP_SendMessage *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	ULONG uin;
	BOOL result = FALSE;

	if(StrToLong(msg->ContactID, &uin) != -1)
		result = GGSendMessage(d->GGSession, uin, msg->Txt, NULL);

	if(!result)
		AddErrorEvent(&d->EventsList, ERRNO_OUT_OF_MEMORY, NULL);

	return (IPTR)result;
}

static IPTR mTypingNotify(Class *cl, Object *obj, struct KWAP_TypingNotify *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	ULONG uin;
	BOOL result = FALSE;

	if((BOOL)xget(findobj(USD_PREFS_GG_OTHER_TYPE_NOTIFY, d->PrefsPanel), MUIA_Selected))
	{
		if(StrToLong(msg->ContactID, &uin) != -1)
			result = GGTypingNotify(d->GGSession, uin, msg->TxtLength);
	}
	else
		result = TRUE;

	return (IPTR)result;
}

static IPTR mAddNotify(Class *cl, Object *obj, struct KWAP_AddNotify *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	ULONG uin;
	BOOL result = FALSE;

	if(StrToLong(msg->Entry->nle_EntryID, &uin) != -1)
	{
		UBYTE type = 0;

		if(KWA_S_HIDE(msg->Entry->nle_Status))
			type = GG_USER_OFFLINE;
		else if(KWA_S_BLOCKED(msg->Entry->nle_Status))
			type = GG_USER_BLOCKED;
		else if(KWA_S_NORMAL(msg->Entry->nle_Status))
			type = GG_USER_NORMAL;

		result = GGAddNotify(d->GGSession, uin, type);
	}
	return (IPTR)result;
}

static IPTR mRemoveNotify(Class *cl, Object *obj, struct KWAP_RemoveNotify *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	ULONG uin;
	BOOL result = FALSE;

	if(StrToLong(msg->Entry->nle_EntryID, &uin) != -1)
	{
		UBYTE type = 0;

		if(KWA_S_HIDE(msg->Entry->nle_Status))
			type = GG_USER_OFFLINE;
		else if(KWA_S_BLOCKED(msg->Entry->nle_Status))
			type = GG_USER_BLOCKED;
		else if(KWA_S_NORMAL(msg->Entry->nle_Status))
			type = GG_USER_NORMAL;

		result = GGRemoveNotify(d->GGSession, uin, type);
	}

	return (IPTR)result;
}

static IPTR mPing(Class *cl, Object *obj)
{
	struct ObjData *d = INST_DATA(cl, obj);

	if(!--d->Timeout)
	{
		if(d->GGSession)
		{
			if(!GGPing(d->GGSession))
				AddErrorEvent(&d->EventsList, ERRNO_ONLY_MESSAGE, "GGPing() fail\n");
		}
		d->Timeout = GG_PING_TIMEOUT;
	}

	return(IPTR)0;
}

static IPTR mImportList(Class *cl, Object *obj)
{
	struct ObjData *d = INST_DATA(cl, obj);

	if(!GGRequestContactList(d->GGSession, GG_LIST_FORMAT_XML))
		AddErrorEvent(&d->EventsList, ERRNO_OUT_OF_MEMORY, NULL);

	return (IPTR)0;
}

static IPTR mExportList(Class *cl, Object *obj, struct KWAP_ExportList *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	STRPTR buffer, *groups;
	ULONG buf_len = 20; /* GG70ExportString,;\r\n */
	LONG i, groups_max = 20;
	ENTER();

	if((groups = AllocMem(groups_max * sizeof(STRPTR), MEMF_ANY | MEMF_CLEAR)))
	{
		STRPTR *tmp_gr = groups;

		for(i = 0; i < msg->ContactsNo; i++)
		{
			if(msg->Contacts[i].firstname)
				buf_len += StrLen(msg->Contacts[i].firstname);

			if(msg->Contacts[i].lastname)
				buf_len += StrLen(msg->Contacts[i].lastname);

			if(msg->Contacts[i].nickname)
				buf_len += StrLen(msg->Contacts[i].nickname);

			if(ContactNameLoc(msg->Contacts[i]))
				buf_len += StrLen(ContactNameLoc(msg->Contacts[i]));

			if(msg->Contacts[i].groupname)
			{
				buf_len += StrLen(msg->Contacts[i].groupname);

				tmp_gr = groups;

				while(*tmp_gr != NULL && tmp_gr < groups + groups_max)
				{
					if(StrEqu(*tmp_gr, msg->Contacts[i].groupname))
						break;
					tmp_gr++;
				}

				if(tmp_gr == groups + groups_max)
				{
					tmp_gr = groups;
					if(!(groups = AllocMem((groups_max << 1) * sizeof(STRPTR), MEMF_ANY | MEMF_CLEAR)))
					{
						AddErrorEvent(&d->EventsList, ERRNO_OUT_OF_MEMORY, NULL);
						return (IPTR)0;
					}
					CopyMem(tmp_gr, groups, groups_max * sizeof(STRPTR));
					groups[groups_max] = msg->Contacts[i].groupname;
					FreeMem(tmp_gr, groups_max * sizeof(STRPTR));

					groups_max <<= 1;
				}
				else if(*tmp_gr == NULL)
				{
					*tmp_gr = msg->Contacts[i].groupname;
				}
			}
			if(msg->Contacts[i].entryid)
				buf_len += StrLen(msg->Contacts[i].entryid);

			buf_len += 18; /* fields endings ';', zeros, carriage return and new line */
		}

		tmp_gr = groups;

		while(*tmp_gr != NULL && tmp_gr < groups + groups_max)
		{
			buf_len += StrLen(*tmp_gr) + 2; /* 2 -> ",;" */
			tmp_gr++;
		}

		if((buffer = AllocMem(buf_len + 1, MEMF_ANY)))
		{
			STRPTR buf = buffer + 18;

			FmtPut(buffer, "GG70ExportString,;");

			tmp_gr = groups;

			while(*tmp_gr != NULL && tmp_gr < groups + groups_max)
			{
				buf = StrCopy(*tmp_gr, buf);
				*buf++ = ',';
				*buf++ = ';';
				tmp_gr++;
			}

			*buf++ = '\r';
			*buf++ = '\n';

			for(i = 0; i < msg->ContactsNo; i++)
			{
				if(msg->Contacts[i].firstname)
					buf = StrCopy(msg->Contacts[i].firstname, buf);

				*buf++ = ';';

				if(msg->Contacts[i].lastname)
					buf = StrCopy(msg->Contacts[i].lastname, buf);

				*buf++ = ';';

				if(msg->Contacts[i].nickname)
					buf = StrCopy(msg->Contacts[i].nickname, buf);

				*buf++ = ';';

				if(ContactNameLoc(msg->Contacts[i]))
					buf = StrCopy(ContactNameLoc(msg->Contacts[i]), buf);

				*buf++ = ';';
				/* cell phone number */
				*buf++ = ';';

				if(msg->Contacts[i].groupname)
					buf = StrCopy(msg->Contacts[i].groupname, buf);

				*buf++ = ';';

				if(msg->Contacts[i].entryid)
					buf = StrCopy(msg->Contacts[i].entryid, buf);

				*buf++ = ';';

				/* e-mail */
				*buf++ = ';';

				/* new message sound */
				*buf++ = '0';
				*buf++ = ';';

				/* new message sound file path */
				*buf++ = ';';

				/* available sound */
				*buf++ = '0';
				*buf++ = ';';

				/* available sound path */
				*buf++ = ';';

				/* home phone number */
				*buf++ = '0';
				*buf++ = ';';
				*buf++ = '\r';
				*buf++ = '\n';
			}

			buffer[buf_len] = 0x00;

			if(!GGExportContactList(d->GGSession, d->ListVersion, GG_LIST_FORMAT_OLD, buffer, buf_len))
				AddErrorEvent(&d->EventsList, ERRNO_OUT_OF_MEMORY, NULL);

			FreeMem(buffer, buf_len);
		}
		FreeMem(groups, groups_max * sizeof(STRPTR));
	}

	LEAVE();
	return (IPTR)0;
}


static IPTR mSendPicture(Class *cl, Object *obj, struct KWAP_SendPicture *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	BOOL result = FALSE;
	UBYTE buffer[500];
	STRPTR id = NULL;
	ULONG uin;
	BPTR org_fh, cache_fh;

	if(StrToLong(msg->ContactID, &uin) != -1)
	{
		tprintf("wysy³am na UIN: %ld\n", uin);
		if((org_fh = Open(msg->Path, MODE_OLDFILE)))
		{
			id = GGCreateImageId(org_fh);

			tprintf("oryginalny otwarty, id: %ls\n", id);

			FmtNPut(buffer, CACHE_PICTURES_DIR"%ls", sizeof(buffer), id);

			tprintf("sciezka w cachu: %ls\n", buffer);

			if((cache_fh = Open(buffer, MODE_OLDFILE)))
			{
				tprintf("wysy³am z cachu\n");
				if(GGSendMessage(d->GGSession, uin, NULL, id))
					result = TRUE;
				Close(cache_fh);
			}
			else if((cache_fh = Open(buffer, MODE_NEWFILE)))
			{
				ULONG bytes;

				result = TRUE;

				while((bytes = FRead(org_fh, buffer, 1, sizeof(buffer))))
				{
					if(FWrite(cache_fh, buffer, bytes, 1) != 1)
					{
						result = FALSE;
						break;
					}
				}

				if(result)
				{
					tprintf("wysylam wczytany\n");
					if(!GGSendMessage(d->GGSession, uin, NULL, id))
						result = FALSE;
				}

				Close(cache_fh);
			}

			Close(org_fh);
		}
	}

	if(!result)
	{
		FmtNPut(buffer, GetString(MSG_MODULE_MSG_PIC_SEND_FAILED), sizeof(buffer), id ? id : (STRPTR)"NULL");
		AddErrorEvent(&d->EventsList, ERRNO_ONLY_MESSAGE, buffer);
	}
	else
		tprintf("poszlo\n");

	return (IPTR)result;
}

static IPTR mFetchContactInfo(Class *cl, Object *obj, struct KWAP_FetchContactInfo *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	BOOL result = FALSE;
	ENTER();

	if(d->GGSession)
	{
		if(msg->Req && msg->ReqMethod)
		{
			ULONG uin;

			if(StrToLong(msg->ContactID, &uin) != -1)
			{
				ULONG seq;

				if((seq = GGFindInPubDir(d->GGSession, uin)))
				{
					struct PubDirQueueEntry *e;

					if((e = AllocMem(sizeof(struct PubDirQueueEntry), MEMF_ANY)))
					{
						e->seq = seq;
						e->obj = msg->Req;
						e->method = msg->ReqMethod;

						AddTail((struct List*)&d->PubDirQueue, (struct Node*)e);

						result = TRUE;
					}
				}
			}
		}
	}

	LEAVE();
	return result;
}

static IPTR mHubDone(Class *cl, Object *obj, struct KWAP_HttpCallBack *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);

	if(msg->DataLength > 0)
	{
		ULONG spaces_no = 0, ip_start = 0, ip_end;

		while(msg->Data[ip_start] != 0x00)
		{
			if(msg->Data[ip_start++] == ' ')
				if(++spaces_no == 2)
					break;
		}

		if(msg->Data[ip_start] != 0x00)
		{
			ip_end = ip_start + 1;

			while(msg->Data[ip_end] != 0x00 && msg->Data[ip_end] != ':' && msg->Data[ip_end++] != ' ');

			msg->Data[ip_end] = 0x00;

			StrNCopy(msg->Data + ip_start, d->ServerIP, 16);

			if(GGConnect(d->GGSession, d->ServerIP, GG_DEFAULT_PORT))
			{
				AddErrorEvent(&d->EventsList, ERRNO_ONLY_MESSAGE, GetString(MSG_MODULE_MSG_START_CONNECTING));
			}
			else
				AddErrorEvent(&d->EventsList, ERRNO_CONNECTION_FAILED, NULL);
		}
		else
			AddErrorEvent(&d->EventsList, ERRNO_CONNECTION_FAILED, GetString(MSG_MODULE_MSG_HUB_FAIL));
	}
	else
		AddErrorEvent(&d->EventsList, ERRNO_CONNECTION_FAILED, GetString(MSG_MODULE_MSG_HUB_FAIL));


	return (IPTR)0;
}

static IPTR mParseUserData(Class *cl, Object *obj, struct GGP_ParseUserData *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	ULONG users_no = msg->udata->ggeud_UsersNo;
	ULONG i;
	BOOL avatars_on = (BOOL)xget(findobj(USD_PREFS_GG_OTHER_SUPPORT_AVATARS, d->PrefsPanel), MUIA_Selected);

	if(!avatars_on)
		return (IPTR)0;

	for(i = 0; i < users_no; i++)
	{
		struct GGUserData *udata = &msg->udata->ggeud_Data[i];
		LONG j;

		for(j = 0; j < udata->ggud_AttrsNo; j++)
		{
			if(StrEqu("avatar", udata->ggud_Attrs[j].gguda_Key))
			{
				DoMethod(obj, GGM_GetAvatar, udata->ggud_Uin, (IPTR)udata->ggud_Attrs[j].gguda_Value);
			}
		}
	}

	return (IPTR)0;
}

static IPTR mGetAvatar(Class *cl, Object *obj, struct GGP_GetAvatar *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	UBYTE buffer[40];
	struct GetAvatarUsrData *usr_data;
	struct Picture *pic;

	FmtNPut(buffer, CACHE_AVATARS_DIR"%ls", sizeof(buffer), msg->key);

	if((pic = LoadPictureFile(buffer)))
		AddNewAvatarEvent(&d->EventsList, msg->uin, pic);
	else if((usr_data = AllocMem(sizeof(struct GetAvatarUsrData), MEMF_ANY)))
	{
		usr_data->uin = msg->uin;
		if((usr_data->url = FmtNew(GG_AVATAR_BIG_URL, msg->uin)))
		{
			if((usr_data->cache_key = StrNew(msg->key)))
			{
				if(AddHttpGetEvent(&d->EventsList, usr_data->url, GG_HTTP_USERAGENT, GGM_NewAvatar, usr_data))
					return (IPTR)0;

				StrFree(usr_data->cache_key);
			}
			FmtFree(usr_data->url);
		}
		FreeMem(usr_data, sizeof(struct GetAvatarUsrData));
	}

	return (IPTR)0;
}

static IPTR mNewAvatar(Class *cl, Object *obj, struct KWAP_HttpCallBack *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	struct GetAvatarUsrData *usr_data = (struct GetAvatarUsrData*)msg->UserData;
	QUAD length = msg->DataLength;

	if(usr_data)
	{
		if(msg->DataLength > 0 && msg->Data)
		{
			struct Picture *pic;
			BPTR fh;
			UBYTE buffer[40];

			FmtNPut(buffer, CACHE_AVATARS_DIR"%ls", sizeof(buffer), usr_data->cache_key);

			if((fh = Open(buffer, MODE_NEWFILE)))
			{
				FWrite(fh, msg->Data, msg->DataLength, 1);
				Close(fh);
			}

			if((pic = LoadPictureMemory(msg->Data, &length)))
				AddNewAvatarEvent(&d->EventsList, usr_data->uin, pic);
		}

		if(usr_data->url)
			FmtFree(usr_data->url);

		if(usr_data->cache_key)
			StrFree(usr_data->cache_key);

		FreeMem(usr_data, sizeof(struct GetAvatarUsrData));
	}

	return (IPTR)0;
}

static IPTR mParseXMLList(Class *cl, Object *obj, struct GGP_ParseXMLList *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	ENTER();

	if(msg->list->ggeli_Format == GG_LIST_FORMAT_XML)
	{
		ezxml_t list;

		if((list = ezxml_parse_str(msg->list->ggeli_Data, StrLen(msg->list->ggeli_Data))))
		{
			ezxml_t fcontact;
			struct KwaEvent *e;

			if((fcontact = ezxml_get(list, "Contacts", 0, "Contact", -1)))
			{
				ezxml_t con = fcontact;
				ULONG cons_no = 1;

				while((con = ezxml_next(con)))
					cons_no++;

				if((e = AddEvent(&d->EventsList, KE_TYPE_IMPORT_LIST)))
				{
					if((e->ke_ImportList.ke_Contacts = AllocMem(cons_no * sizeof(struct ContactEntry), MEMF_ANY | MEMF_CLEAR)))
					{
						LONG i;
						ezxml_t tag, groups;

						if((groups = ezxml_child(list, "Groups")))
						{
							d->ListVersion = msg->list->ggeli_Version;
							e->ke_ImportList.ke_ContactsNo = cons_no;
							con = fcontact;

							for(i = 0; i < cons_no; i++)
							{
								STRPTR contact_group_id = ezxml_txt(ezxml_get(con, "Groups", 0, "GroupId", -1));
								ezxml_t act_group = ezxml_child(groups, "Group");

								e->ke_ImportList.ke_Contacts[i].pluginid = MODULE_ID;

								if((tag = ezxml_child(con, "GGNumber")))
									e->ke_ImportList.ke_Contacts[i].entryid = StrNew(ezxml_txt(tag));

								if((tag = ezxml_child(con, "ShowName")))
									e->ke_ImportList.ke_Contacts[i].name = StrNew(ezxml_txt(tag));

								if((tag = ezxml_child(con, "FirstName")))
									e->ke_ImportList.ke_Contacts[i].firstname = StrNew(ezxml_txt(tag));

								if((tag = ezxml_child(con, "NickName")))
									e->ke_ImportList.ke_Contacts[i].firstname = StrNew(ezxml_txt(tag));

								if((tag = ezxml_child(con, "LastName")))
									e->ke_ImportList.ke_Contacts[i].lastname = StrNew(ezxml_txt(tag));

								e->ke_ImportList.ke_Contacts[i].groupname = NULL;

								if(contact_group_id)
								{
									while(act_group && StrEqu(ezxml_txt(ezxml_child(act_group, "Id")), contact_group_id) != TRUE)
										act_group = ezxml_next(act_group);

									if(act_group != NULL) /* we found proper group! */
										e->ke_ImportList.ke_Contacts[i].groupname = StrNew(ezxml_txt(ezxml_child(act_group, "Name")));
								}

								con = ezxml_next(con);
							}
						}
					}
					else
						Remove((struct Node*)e);
				}
			}
			ezxml_free(list);
		}
	}

	LEAVE();
	return (IPTR)0;
}

static IPTR mDisconnectMultilogon(Class *cl, Object *obj, struct GGP_DisconnectMultilogon *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);

	GGDisconnectMultilogon(d->GGSession, *msg->id);

	return (IPTR)0;
}

static IPTR mSendImageData(Class *cl, Object *obj, struct GGP_SendImageData *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	UBYTE buffer[50];
	BPTR fh;

	FmtNPut(buffer, CACHE_PICTURES_DIR"%08lx%08lx", sizeof(buffer), msg->ir->ggeir_Crc32, msg->ir->ggeir_ImageSize);

	if((fh = Open(buffer, MODE_OLDFILE)))
	{
		GGSendImageData(d->GGSession, msg->ir->ggeir_Uin, fh);
		Close(fh);
	}

	return (IPTR)0;
}

static IPTR mRecvMsg(Class *cl, Object *obj, struct GGP_RecvMsg *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);

	if(msg->rm->ggerm_Txt)
		AddEventNewMessage(&d->EventsList, msg->rm->ggerm_Uin, msg->rm->ggerm_Txt, msg->rm->ggerm_Flags, msg->rm->ggerm_Time);

	if(msg->rm->ggerm_ImagesIds)
	{
		UBYTE buffer[50];
		ULONG imgs = 1, i;
		STRPTR t = msg->rm->ggerm_ImagesIds, end;

		end = StrCopy(CACHE_PICTURES_DIR, buffer);

		while(*t++ != 0x00)
		{
			if(*t == '|')
				imgs++;
		}

		for(i = 0; i < imgs; i++)
		{
			APTR pic;
			ULONG size;

			StrNCopy(msg->rm->ggerm_ImagesIds + i * 16 + i, end, 16);


			if((pic = LoadFile(buffer, &size)))
			{
				/* we have image in cache, load and go with it */
				AddEventNewPicture(&d->EventsList, msg->rm->ggerm_Uin, msg->rm->ggerm_Flags, msg->rm->ggerm_Time, pic, size);
			}
			else
			{
				/* image not found in cache, request from sender and add to queue */
				struct PictureQueueEntry *en;

				if((en = AllocMem(sizeof(struct PictureQueueEntry), MEMF_ANY)))
				{
					en->uin = msg->rm->ggerm_Uin;
					en->flags = msg->rm->ggerm_Flags;
					en->timestamp = msg->rm->ggerm_Time;
					en->data = NULL;
					StrNCopy(end, en->filename, 17);
					GGRequestImage(d->GGSession, en->uin, en->filename);

					AddTail((struct List*)&d->PicturesQueue, (struct Node*)en);
				}
			}
		}
	}

	return (IPTR)0;
}

static IPTR mReceiveImageData(Class *cl, Object *obj, struct GGP_ReceiveImageData *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	struct PictureQueueEntry *en;
	static UBYTE buf[17];

	FmtNPut(buf, "%08lx%08lx", sizeof(buf), msg->id->ggeid_Crc32, msg->id->ggeid_ImageSize);

	ForeachNode(&d->PicturesQueue, en)
	{
		if(StrEqu(en->filename, buf))
		{
			if(!en->data)
			{
				en->act_pos = 0;
				en->size = msg->id->ggeid_ImageSize;

				if(!(en->data = AllocMem(en->size, MEMF_ANY)))
					goto remove_picture_from_queue;
			}

			CopyMem(msg->id->ggeid_Data, en->data + en->act_pos, msg->id->ggeid_DataSize);
			en->act_pos += msg->id->ggeid_DataSize;

			if(en->act_pos == en->size)
			{
				BPTR fh;
				UBYTE cache_path[50];

				FmtNPut(cache_path, CACHE_PICTURES_DIR"%ls", sizeof(cache_path), en->filename);

				if((fh = Open(cache_path, MODE_NEWFILE)))
				{
					FWrite(fh, en->data, en->size, 1);
					Close(fh);
				}

				AddEventNewPicture(&d->EventsList, en->uin, en->flags, en->timestamp, en->data, en->size);

				goto remove_picture_from_queue;
			}
			break;
		}
	}

	return (IPTR)0;

remove_picture_from_queue:
	if(en)
	{
		Remove((struct Node*)en);
		FreeMem(en, sizeof(struct PictureQueueEntry));
	}
	return (IPTR)0;
}

static IPTR mParsePubDirInfo(Class *cl, Object *obj, struct GGP_ParsePubDirInfo *msg)
{
	struct ObjData *d = INST_DATA(cl, obj);
	struct PubDirQueueEntry *e = NULL;
	ENTER();

	ForeachNode(&d->PubDirQueue, e)
	{
		if(msg->ipdi->ggepdi_Seq == e->seq)
		{
			struct ContactEntry *con;

			Remove((struct Node*)e);

			if((con = AllocVec(sizeof(struct ContactEntry), MEMF_ANY)))
			{
				con->entryid = FmtNew("%lu", msg->ipdi->ggepdi_Uin);
				con->pluginid = MODULE_ID;
				con->name = NULL;
				con->nickname = StrNewUTF8(msg->ipdi->ggepdi_NickName, MIBENUM_WINDOWS_1250);
				con->firstname = StrNewUTF8(msg->ipdi->ggepdi_FirstName, MIBENUM_WINDOWS_1250);
				con->lastname = StrNewUTF8(msg->ipdi->ggepdi_LastName, MIBENUM_WINDOWS_1250);
				con->groupname = NULL;
				con->birthyear = StrNewUTF8(msg->ipdi->ggepdi_BirthYear, MIBENUM_WINDOWS_1250);
				con->city = StrNewUTF8(msg->ipdi->ggepdi_City, MIBENUM_WINDOWS_1250);
				con->status = 0;
				con->statusdesc = NULL;
				con->gender = msg->ipdi->ggepdi_Gender;
				con->unread = FALSE;
				con->avatar = NULL;

				DoMethod(d->AppObj, MUIM_Application_PushMethod, (IPTR)e->obj, 2, e->method, (IPTR)con);
			}

			FreeMem(e, sizeof(struct PubDirQueueEntry));
			break;
		}
	}

	LEAVE();
	return (IPTR)0;
}

IPTR ClassDispatcher(void)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch(msg->MethodID)
	{
		case OM_NEW:                     return mNew(cl, obj, (struct opSet*)msg);
		case OM_DISPOSE:                 return mDispose(cl, obj, msg);
		case OM_GET:                     return mGet(cl, obj, (struct opGet*)msg);
		case KWAM_Connect:               return mConnect(cl, obj, (struct KWAP_Connect*)msg);
		case KWAM_Disconnect:            return mDisconnect(cl, obj, (struct KWAP_Disconnect*)msg);
		case KWAM_WatchEvents:           return mWatchEvents(cl, obj, (struct KWAP_WatchEvents*)msg);
		case KWAM_FreeEvents:            return mFreeEvent(cl, obj, (struct KWAP_FreeEvents*)msg);
		case KWAM_NotifyList:            return mNotifyList(cl, obj, (struct KWAP_NotifyList*)msg);
		case KWAM_ChangeStatus:          return mChangeStatus(cl, obj, (struct KWAP_ChangeStatus*)msg);
		case KWAM_SendMessage:           return mSendMessage(cl, obj, (struct KWAP_SendMessage*)msg);
		case KWAM_TypingNotify:          return mTypingNotify(cl, obj, (struct KWAP_TypingNotify*)msg);
		case KWAM_AddNotify:             return mAddNotify(cl, obj, (struct KWAP_AddNotify*)msg);
		case KWAM_RemoveNotify:          return mRemoveNotify(cl, obj, (struct KWAP_RemoveNotify*)msg);
		case KWAM_TimedMethod:           return mPing(cl, obj);
		case KWAM_ImportList:            return mImportList(cl, obj);
		case KWAM_ExportList:            return mExportList(cl, obj, (struct KWAP_ExportList*)msg);
		case KWAM_SendPicture:           return mSendPicture(cl, obj, (struct KWAP_SendPicture*)msg);
		case KWAM_FetchContactInfo:      return mFetchContactInfo(cl, obj, (struct KWAP_FetchContactInfo*)msg);
		case GGM_HubDone:                return mHubDone(cl, obj, (struct KWAP_HttpCallBack*)msg);
		case GGM_ParseUserData:          return mParseUserData(cl, obj, (struct GGP_ParseUserData*)msg);
		case GGM_GetAvatar:              return mGetAvatar(cl, obj, (struct GGP_GetAvatar*)msg);
		case GGM_NewAvatar:              return mNewAvatar(cl, obj, (struct KWAP_HttpCallBack*)msg);
		case GGM_ParseXMLList:           return mParseXMLList(cl, obj, (struct GGP_ParseXMLList*)msg);
		case GGM_DisconnectMultilogon:   return mDisconnectMultilogon(cl, obj, (struct GGP_DisconnectMultilogon*)msg);
		case GGM_SendImageData:          return mSendImageData(cl, obj, (struct GGP_SendImageData*)msg);
		case GGM_RecvMsg:                return mRecvMsg(cl, obj, (struct GGP_RecvMsg*)msg);
		case GGM_ReceiveImageData:       return mReceiveImageData(cl, obj, (struct GGP_ReceiveImageData*)msg);
		case GGM_ParsePubDirInfo:        return mParsePubDirInfo(cl, obj, (struct GGP_ParsePubDirInfo*)msg);
		default:                         return DoSuperMethodA(cl, obj, msg);
	}
}
