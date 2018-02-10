/*
 * Copyright (c) 2013 - 2018 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __GLOBALDEFINES_H__
#define __GLOBALDEFINES_H__

#define APP_NAME "gglib"

#ifdef __DEBUG__
#include <clib/debug_protos.h>
#define tprintf(template, ...) KPrintF((CONST_STRPTR)APP_NAME " " __FILE__ " %d: " template, __LINE__ , ##__VA_ARGS__)
#define ENTER(...) KPrintF((CONST_STRPTR)APP_NAME " enters: %s\n", __PRETTY_FUNCTION__)
#define LEAVE(...) KPrintF((CONST_STRPTR)APP_NAME " leaves: %s\n", __PRETTY_FUNCTION__)
#define strd(x) (x) ? (x) : (STRPTR)"NULL"
#else
#define tprintf(...)
#define ENTER(...)
#define LEAVE(...)
#define strd(...)
#endif

#define VERSION "0"
#define REVISION "1"
#define VERS "gglib " VERSION "." REVISION
#define AUTHOR "Filip \"widelec\" Maryjañski"
#define COPYRIGHT "© 2013 - 2018 " AUTHOR
#define VSTRING VERS __AMIGADATE__ " " COPYRIGHT
#define VERSTAG "\0$VER: " VSTRING


#endif /* __GLOBALDEFINES_H__ */
