/*
 * Copyright (c) 2013 - 2018 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* GGLib/ggmessage.c
 *
 *  NAME
 *    ggmessage.c -- Plik zawieraj±cy definicje funkcji obs³ugi wiadomo¶ci GG.
 *
 *  AUTHOR
 *    Filip Maryjañski
 *
 *  DESCRIPTION
 *    Definicje funkcji obs³ugi wiadomo¶ci GG (konwersja z i do HTML)
 *
 ********/

#include <libvstring.h>
#include "globaldefines.h"
#include "support.h"
#include "ggmessage.h"

extern struct Library *SysBase;

/****if* ggmessage.c/GGMessageHTMLtoText()
 *
 *  NAME
 *    GGMessageHTMLtoText()
 *
 *  SYNOPSIS
 *    STRPTR GGMessageHTMLtoText(STRPTR html, STRPTR *image)
 *
 *  FUNCTION
 *    Funkcja przeprowadza konwersjê wiadomo¶ci GG zapisanej w formacie HTML
 *    na czysty tekst. Dodatkowo sprawdza czy w wiadomo¶ci pojawiaj± siê jakie¶ obrazki,
 *    je¶li tak to alokuje bufor ich identyfikatory. Adres bufora zostaje zapisany
 *    pod adresem wskazywanym przez image. Identyfikatory kolejnych obrazków s± oddzielone
 *    znakiem '|'. Wszelkie informacje o dodatkowym formatowaniu tekstu wiadomo¶ci zostaj± zignorowane.
 *
 *  INPUTS
 *    - html -- tre¶æ wiadomo¶ci w formacie HTML, zakoñczona 0x00;
 *    - images -- miejsce na zapisanie adresu bufora z identyfikatorami obrazków.
 *
 *  RESULT
 *    Wska¼nik na bufor zawieraj±cy tekst wiadomo¶ci zapisany czystym tekstem
 *    w kodowaniu UTF-8.
 *
 *   NOTES
 *    W przypadku wiadomo¶ci zawieraj±cych tylko obrazki (brak tekstu) funkcja zwróci NULL.
 *
 *****/

STRPTR GGMessageHTMLtoText(STRPTR html, STRPTR *images)
{
	STRPTR result = NULL;
	STRPTR html_act, result_act;
	ENTER();

	*images = NULL;

	if(html && (result = AllocVec(StrLen(html), MEMF_ANY)))
	{
		html_act = html;
		result_act = result;

		while(*html_act != 0x00)
		{
			if(*html_act == '<')
			{
				if(!StrNCmp(html_act, "<b>", 3) || !StrNCmp(html_act, "<i>", 3) || !StrNCmp(html_act, "<u>", 3))
				{
					html_act += 3;
				}
				else if(!StrNCmp(html_act, "</b>", 4) || !StrNCmp(html_act, "</i>", 4) || !StrNCmp(html_act, "</u>", 4))
				{
					html_act += 4;
				}
				else if(!StrNCmp(html_act, "<span", 5))
				{
					while(*html_act != 0x00 && *html_act != '>')
						html_act++;
					html_act++;
				}
				else if(!StrNCmp(html_act, "</span>", 7))
				{
					html_act += 7;
				}
				else if(!StrNCmp(html_act, "<br>", 4))
				{
					html_act += 4;
					*result_act++ = '\n';
				}
				else if(!StrNCmp(html_act, "<br/>", 5))
				{
					html_act += 5;
					*result_act++ = '\n';
				}
				else if(!StrNCmp(html_act, "<img name=\"", 4))
				{
					STRPTR name_end = html_act + 11;

					html_act += 11;

					while(*name_end != 0x00 && *name_end != '"')
						name_end++;

					if(*name_end != 0x00)
					{
						*name_end = 0x00;

						if(*images)
						{
							STRPTR old = *images;

							*images = FmtNew("%ls|%ls", old, html_act);

							StrFree(old);
						}
						else
							*images = StrNewLen(html_act, name_end - html_act);

						*name_end = '"';
					}

					while(*name_end != 0x00 && *name_end != '>')
						name_end++;

					html_act = name_end + 1;
				}
				else
					*result_act++ = *html_act++;
			}
			else if(*html_act == '&')
			{
				if(!StrNCmp(html_act, "&lt;", 4))
				{
					html_act += 4;
					*result_act++ = '<';
				}
				else if(!StrNCmp(html_act, "&gt;", 4))
				{
					html_act += 4;
					*result_act++ = '>';
				}
				else if(!StrNCmp(html_act, "&quot;", 6))
				{
					html_act += 6;
					*result_act++ = '"';
				}
				else if(!StrNCmp(html_act, "&apos;", 6))
				{
					html_act += 6;
					*result_act++ = '\'';
				}
				else if(!StrNCmp(html_act, "&amp;", 5))
				{
					html_act += 5;
					*result_act++ = '&';
				}
				else if(!StrNCmp(html_act, "&nbsp;", 6))
				{
					html_act += 6;
					*result_act++ = ' ';
				}
				else
					*result_act++ = *html_act++;
			}
			else
				*result_act++ = *html_act++;
		}
		*result_act = 0x00;

		if(*result == 0x00) /* wiadomo¶æ nie zawiera³a ¿adnego tekstu (np. przesy³anie tylko obrazka) */
		{
			FreeVec(result);
			result = NULL;
		}
	}

	LEAVE();
	return result;
}

/****if* ggmessage.c/GGMessageTextToHTML()
 *
 *  NAME
 *    GGMessageTextToHTML()
 *
 *  SYNOPSIS
 *    STRPTR GGMessageTextToHTML(STRPTR txt, STRPTR image)
 *
 *  FUNCTION
 *    Funkcja przeprowadza konwersjê czystego tekstu na tekst w HTML-u zgodny
 *    z formatem wiadomo¶ci sieci GG.
 *
 *  INPUTS
 *    - txt -- tre¶æ wiadomo¶ci zakoñczona 0x00;
 *    - images -- identyfikatory obrazków, które nale¿y wstawiæ do wiadomo¶ci oddzielone
 *     znakiem '|' (je¶li NULL to ¿aden obrazek nie bêdzie wstawiony).
 *
 *  RESULT
 *    Wska¼nik na bufor zawieraj±cy tekst wiadomo¶ci zapisany w HTML-u
 *    w kodowaniu UTF-8.
 *
 *****/

STRPTR GGMessageTextToHTML(STRPTR txt, STRPTR images)
{
	STRPTR result = NULL;
	STRPTR span = "<span style=\"color:#000000; font-family:'MS Shell Dlg 2'; font-size:9pt; \">";
	ULONG imgs = 0;
	ULONG html_len = 75 + 7; /* <span...> + </span> */
	STRPTR txt_act = txt;
	ENTER();

	if(images)
	{
		STRPTR t = images;

		imgs++;

		while(*t++ != 0x00)
		{
			if(*t == '|')
				imgs++;
		}

		html_len += imgs * 29; /* <img name="xxxxxxxxxxxxxxxx"> */
	}

	if(txt)
	{
		while(*txt != 0x00)
		{
			if(*txt == '<' || *txt == '>' || *txt == '\n')
				html_len += 4;
			else if(*txt == '"' || *txt == '\'')
				html_len += 6;
			else if(*txt == '&')
				html_len += 5;
			else
				html_len++;

			txt++;
		}
	}

	if((result = AllocVec(html_len + 1, MEMF_ANY)))
	{
		LONG i;
		STRPTR result_act = result + 75;

		StrNCopy(span, result, 75);

		if(txt)
		{
			while(*txt_act != 0x00)
			{
				if(*txt_act == '<')
				{
					*result_act++ = '&';
					*result_act++ = 'l';
					*result_act++ = 't';
					*result_act++ = ';';
				}
				else if(*txt_act == '>')
				{
					*result_act++ = '&';
					*result_act++ = 'g';
					*result_act++ = 't';
					*result_act++ = ';';
				}
				else if(*txt_act == '"')
				{
					*result_act++ = '&';
					*result_act++ = 'q';
					*result_act++ = 'u';
					*result_act++ = 'o';
					*result_act++ = 't';
					*result_act++ = ';';
				}
				else if(*txt_act == '\'')
				{
					*result_act++ = '&';
					*result_act++ = 'a';
					*result_act++ = 'p';
					*result_act++ = 'o';
					*result_act++ = 's';
					*result_act++ = ';';
				}
				else if(*txt_act == '&')
				{
					*result_act++ = '&';
					*result_act++ = 'a';
					*result_act++ = 'm';
					*result_act++ = 'p';
					*result_act++ = ';';
				}
				else if(*txt_act == '\n')
				{
					*result_act++ = '<';
					*result_act++ = 'b';
					*result_act++ = 'r';
					*result_act++ = '>';
				}
				else
				{
					*result_act++ = *txt_act;
				}
				txt_act++;
			}
		}

		*result_act = 0x00;

		for(i = 0; i < imgs; i++)
		{
			result_act = StrCat("<img name=\"", result_act);
			result_act = StrNCopy(images + i * 16 + i, result_act, 16);
			result_act = StrCat("\">", result_act);
		}

		*result_act++ = '<';
		*result_act++ = '/';
		*result_act++ = 's';
		*result_act++ = 'p';
		*result_act++ = 'a';
		*result_act++ = 'n';
		*result_act++ = '>';
		*result_act = 0x00;
	}

	LEAVE();
	return result;
}
