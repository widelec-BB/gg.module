/*
 * Copyright (c) 2013 - 2018 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/muimaster.h>
#include <proto/alib.h>
#include "globaldefines.h"
#include "multilogonlist.h"
#include "gui.h"
#include "locale.h"

#define EmptyRectangle(weight) MUI_NewObjectM(MUIC_Rectangle, MUIA_Weight, weight, TAG_END)


extern struct Library *MUIMasterBase, *IntuitionBase;

Object* MUI_NewObjectM(char *classname, ...)
{
	va_list args, args2;
	LONG argc = 0;
	ULONG tag;
	Object *result = NULL;

	__va_copy(args2, args);

	va_start(args, classname);

	while((tag = va_arg(args, ULONG)) != TAG_END)
	{
		va_arg(args, IPTR);
		argc++;
	}

	va_end(args);

	{
		struct TagItem tags[argc + 1];  // one for {TAG_END, 0}
		LONG i;

		va_start(args2, classname);

		for (i = 0; i < argc; i++)
		{
			tags[i].ti_Tag = va_arg(args2, ULONG);
			tags[i].ti_Data = va_arg(args2, IPTR);
		}

		tags[argc].ti_Tag = TAG_END;
		tags[argc].ti_Data = 0;

		va_end(args2);

		result = (Object*)MUI_NewObjectA(classname, tags);
	}
	return result;
}


static inline Object* StringLabel(STRPTR label, STRPTR preparse)
{
	Object *obj = MUI_NewObjectM(MUIC_Text,
		MUIA_FramePhantomHoriz, TRUE,
		MUIA_Frame, MUIV_Frame_String,
		MUIA_Text_PreParse, (ULONG)preparse,
		MUIA_Text_Contents, (ULONG)label,
		MUIA_HorizWeight, 0,
	TAG_END);

	return obj;
}

static inline Object* StringGadget(ULONG id)
{
	Object *obj = MUI_NewObjectM(MUIC_String,
		MUIA_UserData, id,
		MUIA_ObjectID, id,
		MUIA_Frame, MUIV_Frame_String,
		MUIA_Background, MUII_StringBack,
		MUIA_CycleChain, TRUE,
		MUIA_String_AdvanceOnCR, TRUE,
	TAG_END);

	return obj;
}

static inline Object* GfxButton(ULONG id, STRPTR pic, UBYTE control)
{
	Object *obj = MUI_NewObjectM(MUIC_Group,
		MUIA_InputMode, MUIV_InputMode_RelVerify,
		MUIA_UserData, id,
		MUIA_ObjectID, id,
		MUIA_ControlChar, (ULONG)control,
		MUIA_Group_Child, (ULONG)MUI_NewObjectM(MUIC_Dtpic,
			MUIA_Dtpic_Name, (ULONG)pic,
		TAG_END),
	TAG_END);

	return obj;
}

static inline Object* NormalButton(STRPTR label, UBYTE control, LONG objid, ULONG weight)
{
	Object *obj = MUI_NewObjectM(MUIC_Text,
		MUIA_Text_Contents, (ULONG)label,
		MUIA_Text_PreParse, (ULONG)"\33c",
		MUIA_Frame, MUIV_Frame_Button,
		MUIA_Background, MUII_ButtonBack,
		MUIA_Font, MUIV_Font_Button,
		MUIA_InputMode, MUIV_InputMode_RelVerify,
		MUIA_Text_HiChar, (ULONG)control,
		MUIA_ControlChar, (ULONG)control,
		MUIA_CycleChain, TRUE,
		MUIA_HorizWeight, weight,
		MUIA_UserData, objid,
	TAG_END);

	return obj;
}


Object *CreatePrefsPage(VOID)
{
	Object *result;/*, *pubdir_send, *pubdir_fetch;*/
	Object *login_group, *pass_string;
	ENTER();

	pass_string = 	MUI_NewObjectM(MUIC_String,
		MUIA_ObjectID, USD_PREFS_GG_BASIC_PASS_STRING,
		MUIA_UserData, USD_PREFS_GG_BASIC_PASS_STRING,
		MUIA_Frame, MUIV_Frame_String,
		MUIA_Background, MUII_StringBack,
		MUIA_CycleChain, TRUE,
		MUIA_String_Secret, TRUE,
	TAG_END);

	login_group = MUI_NewObjectM(MUIC_Group,
		MUIA_Frame, MUIV_Frame_Group,
		MUIA_FrameTitle, (ULONG)GetString(MSG_PREFS_GG_BASIC),
		MUIA_Background, MUII_GroupBack,
		MUIA_Group_Columns, 2,
		MUIA_Group_Child, (ULONG)StringLabel(GetString(MSG_PREFS_GG_BASIC_UIN), "\33r"),
		MUIA_Group_Child, (ULONG)MUI_NewObjectM(MUIC_String,
			MUIA_ObjectID, USD_PREFS_GG_BASIC_UIN_STRING,
			MUIA_UserData, USD_PREFS_GG_BASIC_UIN_STRING,
			MUIA_Frame, MUIV_Frame_String,
			MUIA_Background, MUII_StringBack,
			MUIA_CycleChain, TRUE,
			MUIA_String_Accept, (ULONG)"0123456789",
			MUIA_String_AdvanceOnCR, TRUE,
			MUIA_ShortHelp, (ULONG)GetString(MSG_PREFS_GG_BASIC_UIN_HELP),
		TAG_END),
		MUIA_Group_Child, (ULONG)StringLabel(GetString(MSG_PREFS_GG_BASIC_PASS), "\33r"),
		MUIA_Group_Child, (ULONG)pass_string,
	TAG_END);

	result = MUI_NewObjectM(MUIC_Group,
		MUIA_Group_Child, (ULONG)EmptyRectangle(100),
		MUIA_Group_Child, (ULONG)login_group,
		MUIA_Group_Child, (ULONG)MUI_NewObjectM(MUIC_Group,
			MUIA_Background, MUII_GroupBack,
			MUIA_Frame, MUIV_Frame_Group,
			MUIA_FrameTitle, GetString(MSG_PREFS_GG_OTHER),
			MUIA_Group_Horiz, TRUE,
			MUIA_Group_Child, MUI_NewObjectM(MUIC_Group,

				MUIA_Group_Child, (ULONG)MUI_NewObjectM(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, MUI_NewObjectM(MUIC_Image,
						MUIA_ObjectID, USD_PREFS_GG_OTHER_TYPE_NOTIFY,
						MUIA_UserData, USD_PREFS_GG_OTHER_TYPE_NOTIFY,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, TRUE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, (ULONG)GetString(MSG_PREFS_GG_OTHER_SEND_TYPING_NOTIFY_HELP),
					TAG_END),
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_GG_OTHER_SEND_TYPING_NOTIFY), "\33l"),
					MUIA_Group_Child, (ULONG)EmptyRectangle(100),
				TAG_END),

				MUIA_Group_Child, (ULONG)MUI_NewObjectM(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, (ULONG)MUI_NewObjectM(MUIC_Image,
						MUIA_ObjectID, USD_PREFS_GG_OTHER_SUPPORT_AVATARS,
						MUIA_UserData, USD_PREFS_GG_OTHER_SUPPORT_AVATARS,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, TRUE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, (ULONG)GetString(MSG_PREFS_GG_OTHER_SUPPORT_AVATARS_HELP),
					TAG_END),
					MUIA_Group_Child, (ULONG)StringLabel(GetString(MSG_PREFS_GG_OTHER_SUPPORT_AVATARS), "\33l"),
					MUIA_Group_Child, (ULONG)EmptyRectangle(100),
				TAG_END),

			TAG_END),
		TAG_END),
		MUIA_Group_Child, (ULONG)EmptyRectangle(100),
	TAG_END);

	LEAVE();
	return result;
}

Object *CreateMultilogonWindow(Object *gg)
{
	Object *ok_but;
	Object *result = MUI_NewObjectM(MUIC_Window,
		MUIA_Background, MUII_GroupBack,
		MUIA_Window_ID, USD_MULTILOGON_WINDOW,
		MUIA_UserData, USD_MULTILOGON_WINDOW,
		MUIA_Window_Title, GetString(MSG_MULTILOGON_WINDOW_TITLE),
		MUIA_Window_RootObject, MUI_NewObjectM(MUIC_Group,
			MUIA_Background, MUII_GroupBack,
			MUIA_Frame, MUIV_Frame_Group,
			MUIA_FrameTitle, GetString(MSG_MULTILOGON_WINDOW_LIST_TITLE),
			MUIA_Group_Child, (IPTR)NewObject(MultilogonListClass->mcc_Class, NULL,
				MUIA_UserData, USD_MULTILOGON_WINDOW_LIST,
				MLA_GGModule, (IPTR)gg,
			TAG_END),
			MUIA_Group_Child, MUI_NewObjectM(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, EmptyRectangle(100),
				MUIA_Group_Child, (ok_but = NormalButton(GetString(MSG_MULTILOGON_WINDOW_OK), *GetString(MSG_MULTILOGON_WINDOW_OK_HOTKEY), USD_MULTILOGON_WINDOW_OK, 50)),
				MUIA_Group_Child, EmptyRectangle(100),
			TAG_END),
		TAG_END),
	TAG_END);

	DoMethod(result, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)result, 3,
	 MUIM_Set, MUIA_Window_Open, FALSE);
	DoMethod(ok_but, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)result, 3,
	 MUIM_Set, MUIA_Window_Open, FALSE);

	return result;
}
