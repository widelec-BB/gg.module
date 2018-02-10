/*
 * Copyright (c) 2013 - 2018 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* GGLib/http.c
 *
 *  NAME
 *    http.c -- Funkcje obs³ugi protoko³u http.
 *
 *  AUTHOR
 *    Filip Maryjañski
 *
 *  DESCRIPTION
 *    Funkcje obs³uguj±ce zapytania protoko³u http. Wykorzystuj± klasê http.stream z Regeae.
 *
 ********/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <classes/multimedia/video.h>
#include <classes/multimedia/streams.h>

#include "http.h"
#include "globaldefines.h"

extern struct Library *SysBase;
extern struct IntuitionBase *IntuitionBase;

/****if* http.c/HttpGetRequest()
 *
 *  NAME
 *    HttpGetRequest()
 *
 *  SYNOPSIS
 *    UBYTE *HttpGetRequest(STRPTR url, LONG *data_len, STRPTR user_agent)
 *
 *  FUNCTION
 *    Funkcja wysy³a zapytanie GET protoko³u http.
 *
 *  INPUTS
 *    - url -- adres url (bez "http://");
 *    - data_len -- wska¼nik na zmienn±, w której zostanie umieszczona wielko¶æ przeczytanych danych;
 *    - user_agent -- pozwala na zmianê pola "User-Agent" w danych http, w przypadku NULL warto¶æ domy¶lna http.stream.
 *
 *  RESULT
 *    Funkcja zwraca wska¼nik na bufor z przeczytanymi danymi.
 *
 *  NOTES
 *    Bufor powinien zostaæ zwolniony przez u¿ycie FreeVec()
 *
 *****/

UBYTE *HttpGetRequest(STRPTR url, LONG *data_len, STRPTR user_agent)
{
	UBYTE *buffer = NULL;
	Object *http = NULL;

	ENTER();

	http = NewObject(NULL, "http.stream",
		MMA_StreamName, (ULONG)url,
		MMA_Http_UserAgentOverride, (ULONG)user_agent,
		MMA_Http_RequestType, MMV_Http_RequestType_Get,
	TAG_END);

	if(http)
	{
		QUAD buffer_size = MediaGetPort64(http, 0, MMA_StreamLength);

		if(buffer_size == 0)
			buffer_size = 1024*1024;

		if((buffer_size < 2147483648LL) && (buffer = (UBYTE*)AllocVec((LONG)buffer_size, MEMF_ANY | MEMF_CLEAR)))
		{
			*data_len = DoMethod(http, MMM_Pull, 0, (ULONG)buffer, (LONG)buffer_size);

			if(*data_len <= 0)
			{
				FreeVec(buffer);
				buffer = NULL; /* no longer vaild */
			}
		}
		DisposeObject(http);
	}

	LEAVE();
	return buffer;
}

/****if* http.c/HttpPostRequest()
 *
 *  NAME
 *    HttpPostRequest()
 *
 *  SYNOPSIS
 *    UBYTE *HttpPostRequest(STRPTR url, struct TagItem *postdata, LONG *data_len, STRPTR user_agent)
 *
 *  FUNCTION
 *    Funkcja wysy³a zapytanie POST protoko³u http.
 *
 *  INPUTS
 *    - url -- adres url (bez "http://"!);
 *    - postdata -- wska¼nik na taglistê z danymi do zapytania Post, taglista powinna byæ zgodna z http.stream;
 *    - data_len -- wska¼nik na zmienn±, w której zostanie umieszczona wielko¶æ przeczytanych danych;
 *    - user_agent -- pozwala na zmianê pola "User-Agent" w danych http, w przypadku NULL warto¶æ domy¶lna http.stream.
 *
 *  RESULT
 *    Funkcja zwraca wska¼nik na bufor z przeczytanymi danymi.
 *
 *  NOTES
 *    Bufor powinien zostaæ zwolniony przez u¿ycie FreeVec()
 *
 *****/

UBYTE *HttpPostRequest(STRPTR url, struct TagItem *postdata, LONG *data_len, STRPTR user_agent)
{
	UBYTE *buffer = NULL;
	Object *http = NULL;

	ENTER();

	http = NewObject(NULL, "http.stream",
		MMA_StreamName, (ULONG)url,
		MMA_Http_UserAgent, (ULONG)user_agent,
		MMA_Http_RequestType, MMV_Http_RequestType_PostMulti,
		MMA_Http_PostData, (ULONG)postdata,
	TAG_END);

	if(http)
	{
		QUAD buffer_size = MediaGetPort64(http, 0, MMA_StreamLength);

		if(buffer_size == 0)
			buffer_size = 1024*1024;

		if((buffer_size < 2147483648LL) && (buffer = (UBYTE*)AllocVec((LONG)buffer_size, MEMF_ANY | MEMF_CLEAR)))
		{
			*data_len = DoMethod(http, MMM_Pull, 0, (ULONG)buffer, (LONG)buffer_size);

			if(*data_len <= 0)
			{
				FreeVec(buffer);
				buffer = NULL; /* no longer vaild */
			}
		}
		DisposeObject(http);
	}

	LEAVE();
	return buffer;
}
