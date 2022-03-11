/*
 * Copyright (c) 2013 - 2022 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __MULTILOGONLIST_H__
#define __MULTILOGONLIST_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include "gui.h"

extern struct MUI_CustomClass *MultilogonListClass;

struct MUI_CustomClass *CreateMultilogonListClass(VOID);
VOID DeleteMultilogonListClass(VOID);

#define MLM_InsertData      MAKE_ID(0x5000)
#define MLM_Disconnect      MAKE_ID(0x5001)
#define MLM_OpenWebPubDir   MAKE_ID(0x5002)

#define MLA_GGModule        MAKE_ID(0x5500)

#endif /* __MULTILOGONLIST_H__ */
