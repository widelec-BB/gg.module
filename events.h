/*
 * Copyright (c) 2013 - 2018 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __EVENTS_H__
#define __EVENTS_H__

struct KwaEvent *AddEvent(struct MinList *list, ULONG event_type);
VOID AddErrorEvent(struct MinList *list, ULONG errno, STRPTR txt);
VOID AddEventTypingNotify(struct MinList *list, ULONG uin, ULONG len);
VOID AddEventNewMessage(struct MinList *list, ULONG uin, STRPTR msg_txt, ULONG flags, ULONG nix_timestamp);
VOID StatusEvent(struct MinList *list, struct GGEvent *gge, ULONG myuin);
BOOL AddHttpGetEvent(struct MinList *list, STRPTR url, STRPTR usr_agent, ULONG methodid, APTR usr_data);
VOID AddNewAvatarEvent(struct MinList *list, ULONG uin, struct Picture *pic);
VOID AddListExportEvent(struct MinList *list, BOOL result);
VOID AddEventNewPicture(struct MinList *list, ULONG uin, ULONG flags, ULONG nix_timestamp, APTR data, ULONG size);

#endif /* __EVENTS_H__ */
