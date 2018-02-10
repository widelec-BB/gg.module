/*
 * Copyright (c) 2013 - 2018 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __GLOBALDEFINES_H__
#define __GLOBALDEFINES_H__

#define CLASSNAME "gg.module"
#define VERSION 1
#define REVISION 3
#define DATE __AMIGADATE__
#define VERS "gg.module 1.3"
#define AUTHOR "Filip \"widelec\" Maryjañski"
#define DESC "GG Module for KwaKwa"
#define COPYRIGHT "© " "2013 - 2018 " AUTHOR
#define VSTRING VERS DATE COPYRIGHT
#define VERSTAG "\0$VER: " VSTRING
#define RELEASE_TAG "F1"
#define PROTOCOL_NAME "GG"
#define MODULE_ID 0x23000000

#ifdef __DEBUG__
#include <clib/debug_protos.h>
#define tprintf(template, ...) KPrintF((CONST_STRPTR)CLASSNAME " " __FILE__ " %d: " template, __LINE__ , ##__VA_ARGS__)
#define ENTER(...) KPrintF((CONST_STRPTR)CLASSNAME " enters: %s\n", __PRETTY_FUNCTION__)
#define LEAVE(...) KPrintF((CONST_STRPTR)CLASSNAME " leaves: %s\n", __PRETTY_FUNCTION__)
#define strd(x)(x ? x : (STRPTR)"NULL")
#else
#define tprintf(...)
#define ENTER(...)
#define LEAVE(...)
#define strd(x)
#endif /* __DEBUG__ */

#define UNUSED __attribute__((unused))

#define CACHE_DIR "PROGDIR:cache/gg.module/"
#define CACHE_AVATARS_DIR CACHE_DIR "avatars/"
#define CACHE_LIST_VERSION CACHE_DIR "listver.cfg"
#define CACHE_PICTURES_DIR CACHE_DIR "pictures/"
#define GG_AVATAR_BIG_URL "avatars.gg.pl/%lu/s,small"


#endif /* __GLOBALDEFINES_H__ */
