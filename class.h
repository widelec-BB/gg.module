/*
 * Copyright (c) 2013 - 2018 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __CLASS_VERSION_H__
#define __CLASS_VERSION_H__

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <kwakwa_api/protocol.h>
#include <gglib.h>
#include "globaldefines.h"

#define GGM_HubDone                 MAKE_ID(0x00000001)
#define GGM_ParseUserData           MAKE_ID(0x00000002)
#define GGM_GetAvatar               MAKE_ID(0x00000003)
#define GGM_NewAvatar               MAKE_ID(0x00000004)
#define GGM_ParseXMLList            MAKE_ID(0x00000005)
#define GGM_DisconnectMultilogon    MAKE_ID(0x00000006)
#define GGM_SendImageData           MAKE_ID(0x00000007)
#define GGM_RecvMsg                 MAKE_ID(0x00000008)
#define GGM_ReceiveImageData        MAKE_ID(0x00000009)
#define GGM_ParsePubDirInfo         MAKE_ID(0x0000000A)

struct ObjData
{
	struct GGSession   *GGSession;
	struct MinList     EventsList;
	struct MinList     PicturesQueue;
	struct MinList     PubDirQueue;
	UBYTE              ServerIP[16];
	ULONG              Timeout;
	ULONG              ListVersion;
	Object             *PrefsPanel;
	struct TagItem     GuiTagList[6];
	Object             *AppObj;
};

/* don't touch enything below this comment, unless you really know what you are doing... */

struct ClassBase
{
	struct Library          LibNode;
	Class                  *BClass;
	APTR                    Seglist;
	struct SignalSemaphore  BaseLock;
	BOOL                    InitFlag;
};

#endif
