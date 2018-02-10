/*
 * Copyright (c) 2013 - 2018 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __GUI_H__
#define __GUI_H__

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include "globaldefines.h"

#ifdef MAKE_ID
#undef MAKE_ID
#endif /* MAKE_ID */
#define MAKE_ID(x) (MODULE_ID | (x))

/* gg basic prefs */
#define USD_PREFS_GG_BASIC_UIN_STRING        0x9EDA1001
#define USD_PREFS_GG_BASIC_PASS_STRING       0x9EDA1002
#define USD_PREFS_GG_CONNECT_AUTO_CHECK      0x9EDA1003
#define USD_PREFS_GG_CONNECT_AUTO_CYCLE      0x9EDA1004
#define USD_PREFS_GG_CONNECT_AUTO_STRING     0x9EDA1005
#define USD_PREFS_GG_OTHER_TYPE_NOTIFY       0x9EDA1006
#define USD_PREFS_GG_OTHER_SUPPORT_AVATARS   0x9EDA1007
#define USD_PREFS_GG_PUBDIR_SEND_BUTTON      0x9EDA1008
#define USD_PREFS_GG_PUBDIR_FIRSTNAME        0x9EDA1009
#define USD_PREFS_GG_PUBDIR_LASTNAME         0x9EDA100A
#define USD_PREFS_GG_PUBDIR_NICKNAME         0x9EDA100B
#define USD_PREFS_GG_PUBDIR_BIRTHYEAR        0x9EDA100C
#define USD_PREFS_GG_PUBDIR_CITY             0x9EDA100D
#define USD_PREFS_GG_PUBDIR_GENDER           0x9EDA100E
#define USD_PREFS_GG_PUBDIR_FAMILYNAME       0x9EDA100F
#define USD_PREFS_GG_PUBDIR_FAMILYCITY       0x9EDA1010
#define USD_PREFS_GG_PUBDIR_FETCH_BUTTON     0x9EDA1011

/* multilogon info window */
#define USD_MULTILOGON_WINDOW                MAKE_ID(0x0000)
#define USD_MULTILOGON_WINDOW_OK             MAKE_ID(0x0001)
#define USD_MULTILOGON_WINDOW_LIST           MAKE_ID(0x0002)


extern struct Library *IntuitionBase;

Object* MUI_NewObjectM(char *classname, ...);
Object *CreatePrefsPage(VOID);
Object *CreateMultilogonWindow(Object *gg);
#define findobj(id, parent) (Object*)DoMethod(parent, MUIM_FindUData, id)

static inline ULONG xget(Object *obj, ULONG att)
{
  ULONG result;

  GetAttr(att, obj, &result);
  return result;
}

#endif /* __GUI_H__ */
