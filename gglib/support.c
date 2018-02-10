/*
 * Copyright (c) 2013 - 2018 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* GGLib/support.c
 *
 *  NAME
 *    support.c -- Funkcje dodatkowe
 *
 *  AUTHOR
 *    Filip Maryjañski
 *
 *  DESCRIPTION
 *    Funkcje wykorzystywane przez ró¿ne elementy GGLib.
 *
 ********/

#include <proto/exec.h>
#include <proto/socket.h>

#include <proto/ezxml.h>
#include <errno.h>
#include <libvstring.h>

#include "globaldefines.h"
#include "support.h"

extern struct Library *SysBase, *DOSBase, *SocketBase;

#include <proto/z.h>

/****if* support.c/MemSet()
 *
 *  NAME
 *    MemSet()
 *
 *  SYNOPSIS
 *    VOID *MemSet(VOID* ptr, LONG word, LONG size)
 *
 *  FUNCTION
 *    Funkcja ustawia blok pamiêci od adresu ptr do adresu ptr + size na warto¶æ word.
 *    Word jest rzutowane do UBYTE, size powinno byæ podane w bajtach.
 *    (wymuszone przez kompatybilno¶æ z stdlib memset())
 *
 *  INPUTS
 *    - ptr -- wska¼nik na blok pamiêci do ustawienia;
 *    - size -- d³ugo¶æ bloku pamiêci do ustawienia;
 *    - word -- s³owo na które blok ma zostaæ ustawiony.
 *
 *****/

VOID *MemSet(VOID* ptr, LONG word, LONG size)
{
	LONG i;

	for(i = 0; i < size; i++)
		((UBYTE*)ptr)[i] = (UBYTE)word;

	return ptr;
}

/****if* support.c/InetToStr()
 *
 *  NAME
 *    InetToStr()
 *
 *  SYNOPSIS
 *    STRPTR InetToStr(ULONG no)
 *
 *  FUNCTION
 *    Funkcja zmienia numer ip zapisany w postaci pojedyñczej liczby 32 bitowej na tekst w postaci notacji XXX.XXX.XXX.XXX.
 *
 *  INPUTS
 *    - no -- adres IP.
 *
 *  RESULT
 *    Wska¼nik na statyczny bufor.
 *
 *****/

STRPTR InetToStr(ULONG no)
{
	static BYTE result[16];

	FmtNPut((STRPTR)result, "%ld.%ld.%ld.%ld", 16, (no & 0xFF000000) >> 24, (no & 0x00FF0000) >> 16, (no & 0x0000FF00) >> 8, no & 0x000000FF);

	return (STRPTR)result;
}

/****if* support.c/SendAll()
 *
 *  NAME
 *    SendAll()
 *
 *  SYNOPSIS
 *    LONG SendAll(struct Library *SocketBase, LONG sock, BYTE *buf, LONG len)
 *
 *  FUNCTION
 *    Funkcja wysy³a podan± ilo¶æ danych z bufora przez podany socket.
 *    Automatycznie zajmuje siê dzieleniem na porcje oraz b³êdami przerwania wywo³ania systemowego.
 *
 *  INPUTS
 *    - SocketBase -- wska¼nik na bazê socket.library otwart± dla danego procesu;
 *    - sock -- socket, przez który dane maj± wys³ane;
 *    - buf -- wska¼nik na bufor z danymi;
 *    - len -- d³ugo¶æ danych do wys³ania.
 *
 *  RESULT
 *    Ilo¶æ wys³anych danych lub -1 w przypadku b³êdu. W przypadku b³êdu nale¿y sprawdziæ Errno().
 *
 *****/

LONG SendAll(struct Library *SocketBase, LONG sock, BYTE *buf, LONG len)
{
	LONG total = 0, bytesleft = len, n;

	while(total < len)
	{
		n = send(sock, buf + total, bytesleft, 0);

		if(n == -1 && Errno() != EINTR)
			return -1;

		total += n;
		bytesleft -= n;
	}

	return total;
}

/****if* support.c/RecvAll()
 *
 *  NAME
 *    RecvAll()
 *
 *  SYNOPSIS
 *    LONG RecvAll(struct Library *SocketBase, LONG sock, BYTE *buf, LONG len)
 *
 *  FUNCTION
 *    Funkcja odbiera podan± ilo¶æ danych z socketu do podanego bufora.
 *    Automatycznie zajmuje siê dzieleniem na porcje oraz b³êdami przerwania wywo³ania systemowego.
 *
 *  INPUTS
 *    - SocketBase -- wska¼nik na bazê socket.library otwart± dla danego procesu;
 *    - sock -- socket, z którego maj± byæ odebrane dane;
 *    - buf -- wska¼nik na bufor na dane;
 *    - len -- d³ugo¶æ bufora.
 *
 *  RESULT
 *    Ilo¶æ odebranych danych lub -1 w przypadku b³êdu. W przypadku b³êdu nale¿y sprawdziæ Errno().
 *
 *****/

LONG RecvAll(struct Library *SocketBase, LONG sock, BYTE *buf, LONG len)
{
	LONG result;

	while(TRUE)
	{
		result = recv(sock, ((UBYTE*)buf), len, 0);

		if(result == -1 && Errno() == EINTR)
			continue;

		return result;
	}
}

/****if* support.c/StrIEqu()
 *
 *  NAME
 *    StrIEqu()
 *
 *  SYNOPSIS
 *    BOOL StrIEqu(STRPTR s, STRPTR d)
 *
 *  FUNCTION
 *    Funkcja porównuje dwa napisy ignoruj±c wielko¶æ liter.
 *
 *  INPUTS
 *    * s, d -- wska¼niki na napisy do porównania.
 *
 *  RESULT
 *    TRUE je¶li napisy s± identyczne, w przeciwnym wypadku FALSE.
 *
 *****/

BOOL StrIEqu(STRPTR s, STRPTR d) /* case insensitive */
{
	BYTE a, b;

	while(*s != 0x00 && *d != 0x00)
	{
		if(_between('A', *s, 'Z'))
			a = *s | 0x20;
		else
			a = *s;
		if(_between('A', *d, 'Z'))
			b = *d | 0x20;
		else
			b = *d;

		if(a != b)
			return FALSE;

		s++;
		d++;
	}

	if(*s != *d)
		return FALSE;

	return TRUE;
}

/****if* support.c/StrNewLen()
 *
 *  NAME
 *    StrNewLen()
 *
 *  SYNOPSIS
 *    STRPTR StrNewLen(STRPTR s, LONG len)
 *
 *  FUNCTION
 *    Funkcja tworzy nowy napis z podanej ilo¶ci znaków napisu ¼ród³owego.
 *
 *  INPUTS
 *    - s -- wska¼nik na napis ¼ród³owy;
 *    - len -- ilo¶æ znaków do przekopiowania.
 *
 *  RESULT
 *    Wska¼nik na nowy napis.
 *
 *  NOTES
 *    Nowy napis nale¿y zwolniæ poprzez wywo³anie FreeVec().
 *
 *****/

STRPTR StrNewLen(STRPTR s, LONG len)
{
	STRPTR result = NULL;

	if(s != NULL && len > 0)
	{
		if((result = AllocVec(len + 1, MEMF_ANY)))
		{
			LONG i;

			for(i=0; i < len; i++)
				result[i] = s[i];

			result[len] = 0x00;
		}
	}

	return result;
}

/****if* support.c/Inflate()
 *
 *  NAME
 *    Inflate()
 *
 *  SYNOPSIS
 *    UBYTE *Inflate(UBYTE *data, ULONG *len)
 *
 *  FUNCTION
 *    Funkcja przeprowadza rozpakowanie danych spakowanych algorytmem Deflate.
 *
 *  INPUTS
 *    - data -- wska¼nik na bufor zawieraj±cy spakowane dane;
 *    - len -- wska¼nik na zmienn± zawieraj±c± rozmiar bufora ze spakowanymi danymi.
 *      Po rozpakowaniu zostanie tam umieszczona wielko¶æ rozpakowanych danych.
 *
 *  RESULT
 *    Wska¼nik na bufor zawieraj±cy rozpakowane dane. Bufor jest zakoñczony znakiem '\0'.
 *
 *  NOTES
 *    Zwrócony bufor nale¿y zwolniæ poprzez wywo³anie FreeVec(). Funkcja otwiera i zamyka z.library.
 *
 *****/

UBYTE *Inflate(UBYTE *data, ULONG *len)
{
	UBYTE *result = NULL;
	struct Library *ZBase;
	ENTER();

	if((ZBase = OpenLibrary("z.library", 51)))
	{
		struct z_stream_s zstream;
		UBYTE buffer[50];

		zstream.next_in = data;
		zstream.avail_in = *len;
		zstream.zalloc = Z_NULL;
		zstream.zfree = Z_NULL;
		zstream.opaque = Z_NULL;
		zstream.avail_out = sizeof(buffer);
		zstream.total_out = 0;
		zstream.next_out = buffer;

		if(inflateInit(&zstream) == Z_OK)
		{
			LONG res;

			while((res = inflate(&zstream, Z_NO_FLUSH)) == Z_OK)
			{
				zstream.next_out = buffer;
				zstream.avail_out += sizeof(buffer);
			}

			if(inflateEnd(&zstream) == Z_OK)
			{
				if(res == Z_STREAM_END)
				{
					if((result = AllocVec(zstream.total_out + 1, MEMF_ANY)))
					{
						zstream.next_in = data;
						zstream.avail_in = *len;
						zstream.zalloc = Z_NULL;
						zstream.zfree = Z_NULL;
						zstream.opaque = Z_NULL;
						zstream.avail_out = zstream.total_out;
						zstream.total_out = 0;
						zstream.next_out = result;

						if(inflateInit(&zstream) == Z_OK)
						{
							if((res = inflate(&zstream, Z_FINISH)) == Z_STREAM_END)
							{
								if(inflateEnd(&zstream) == Z_OK)
								{
									*len = zstream.total_out;
									result[*len] = 0x00;
									CloseLibrary(ZBase);
									LEAVE();
									return result;
								}
							}
						}
						FreeVec(result);
						result = NULL;
					}
				}
			}
		}
		CloseLibrary(ZBase);
	}

	LEAVE();
	return result;
}

/****if* support.c/Deflate()
 *
 *  NAME
 *    Inflate()
 *
 *  SYNOPSIS
 *    UBYTE *Deflate(UBYTE *data, ULONG *len)
 *
 *  FUNCTION
 *    Funkcja przeprowadza pakowanie danych algorytmem Deflate.
 *
 *  INPUTS
 *    - data -- wska¼nik na bufor zawieraj±cy dane;
 *    - len -- wska¼nik na zmienn± zawieraj±c± rozmiar bufora z danymi.
 *      Po spakowaniu zostanie tam umieszczona wielko¶æ spakowanych danych.
 *
 *  RESULT
 *    Wska¼nik na bufor zawieraj±cy spakowane dane. Bufor jest zakoñczony znakiem '\0'.
 *
 *  NOTES
 *    Zwrócony bufor nale¿y zwolniæ poprzez wywo³anie FreeVec(). Funkcja otwiera i zamyka z.library.
 *
 *****/

UBYTE *Deflate(UBYTE *data, ULONG *len)
{
	struct Library *ZBase;
	UBYTE *result = NULL;
	ENTER();

	if((ZBase = OpenLibrary("z.library", 51)))
	{
		struct z_stream_s zstream;

		zstream.next_in = data;
		zstream.avail_in = *len;
		zstream.zalloc = Z_NULL;
		zstream.zfree = Z_NULL;
		zstream.opaque = Z_NULL;
		zstream.avail_out = 0;
		zstream.total_out = 0;
		zstream.next_out = Z_NULL;

		if(deflateInit(&zstream, Z_BEST_COMPRESSION) == Z_OK)
		{
			*len = deflateBound(&zstream, zstream.avail_in);
			if((result = AllocVec(*len, MEMF_ANY)))
			{
				LONG res;

				zstream.next_out = result;
				zstream.avail_out = *len;

				if((res = deflate(&zstream, Z_FINISH)) == Z_STREAM_END)
				{
					*len = zstream.total_out;
					if(deflateEnd(&zstream) == Z_OK)
					{
						CloseLibrary(ZBase);
						LEAVE();
						return result;
					}
				}
				FreeVec(result);
				result = NULL;
			}
		}
		CloseLibrary(ZBase);
	}

	LEAVE();
	return result;
}

/****if* support.c/StrByteToByte()
 *
 *  NAME
 *    StrByteToByte()
 *
 *  SYNOPSIS
 *    UBYTE StrByteToByte(STRPTR str_byte)
 *
 *  FUNCTION
 *    Funkcja zamienia liczbê szesnastkow± (8 bit) zapisan± w formie tekstu na odpowiednik binarny.
 *
 *  INPUTS
 *    - str_byte -- wska¼nik na o¶mio bitow± liczbê szesnastkow± zapisan± w formie tekstu (dwa znaki).
 *
 *  RESULT
 *    Bajt odopowiadaj±cy podanej liczbie.
 *
 *  NOTES
 *    Funkcja korzysta tylko z dwóch pierwszych znaków podanego tekstu. Tekst nie musi byæ zakoñczony przez '\0'.
 *    Liczba musi byæ zapisana za pomoc± tylko cyfr i ma³ych liter.
 *
 *****/

UBYTE StrByteToByte(STRPTR str_byte)
{
	UBYTE byte;

	if('a' <= *str_byte && *str_byte <= 'f')
		byte = (((*str_byte - 'a') & 0x0F) + 10) * 16;
	else
		byte = ((*str_byte - '0') & 0x0F) * 16;

	if('a' <= *(str_byte + 1) && *(str_byte + 1) <= 'f')
		byte += ((*(str_byte + 1) - 'a') & 0x0F) + 10;
	else
		byte += ((*(str_byte + 1) - '0') & 0x0F);

	return byte;
}

/****if* support.c/FileCrc32()
 *
 *  NAME
 *    FileCrc32()
 *
 *  SYNOPSIS
 *    ULONG FileCrc32(BPTR fh)
 *
 *  FUNCTION
 *    Funkcja wykonuje cykliczn± kontrolê nadmiarow± pliku na 32 bitach.
 *
 *  INPUTS
 *    - fh -- uchwyt do pliku, na którym maj± zostaæ wykonane obliczenia.
 *
 *  RESULT
 *    Wynik funkcji CRC-32 dla podanego pliku. W przypadku b³êdu warto¶æ 0xFFFFFFFF.
 *
 *  NOTES
 *    Funkcja automatycznie przewija wska¼nik pliku na pocz±tek przed wykonaniem siê.
 *    Przed zakoñczeniem funkcja przywraca star± pozycjê w pliku.
 *
 *****/

ULONG FileCrc32(BPTR fh)
{
	static ULONG crc32_tab[256] =
	{
	  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
	  0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	  0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	  0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	  0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
	  0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
	  0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	  0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	  0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	  0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
	  0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
	  0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	  0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	  0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
	  0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
	  0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	  0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	  0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	  0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
	  0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
	  0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	  0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	  0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	  0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
	  0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
	  0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	  0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	  0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	  0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
	  0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
	  0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	  0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	  0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	  0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
	  0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
	  0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	  0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	  0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	  0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
	  0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
	  0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	  0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	  0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	  0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
	  0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
	  0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	  0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	  0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	  0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
	  0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
	  0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	  0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	  0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
	};
	ULONG result = 0xFFFFFFFF, bytes;
	UBYTE buffer[1024];
	ULONG old_pos;

	old_pos = Seek(fh, 0, OFFSET_BEGINING);

	if(old_pos != -1)
	{
		while((bytes = FRead(fh, buffer, 1, sizeof(buffer))))
		{
			LONG i;

			for(i = 0; i < bytes; i++)
				result = (result >> 8) ^ crc32_tab[(result & 0xFF) ^ buffer[i]];
		}
		Seek(fh, old_pos, OFFSET_BEGINING);
	}

	return result ^ 0xFFFFFFFF;
}

/****if* support.c/DumpBinaryData()
 *
 *  NAME
 *    DumpBinaryData()
 *
 *  SYNOPSIS
 *    VOID DumpBinaryData(UBYTE *data, ULONG len)
 *
 *  FUNCTION
 *    Funkcja wykonuje zrzut do debugloga danych wskazywanych przez data o d³ugo¶ci len.
 *    Dane przedstawiane s± jako liczby szesnastkowe (bajtami) oraz ich interpretacja ASCII
 *    (tylko w przypadku, gdy znak jest tzw. widocznym znakiem ASCII, w pozosta³ych przypadkach
 *    wy¶wietlana jest kropka).
 *
 *  INPUTS
 *    - data -- wska¼nik na dane;
 *    - len -- d³ugo¶æ danych wskazywanych przez data.
 *
 *  NOTES
 *    Funkcja dostêpna jest tylko w przypadku kompilacji z zdefiniowanym symbolem __DEBUG__.
 *
 *****/

#ifdef __DEBUG__
#define _between(a,x,b) ((x)>=(a) && (x)<=(b))
#define is_visible_ascii(x) _between(32, x, 127)

VOID DumpBinaryData(UBYTE *data, ULONG len)
{
	ULONG i;
	ULONG pos = 0;

	KPrintF("ptr: %lp len: %lu\n", data, len);
	KPrintF("--------- packet dump start --------------\n\n");

	while(TRUE)
	{
		if(pos >= len)
			break;

		for(i = pos; i < pos + 8; i++)
		{
			if(i < len)
				KPrintF("%02lx ", *(data + i));
			else
				KPrintF("   ");
		}
		KPrintF(" ");

		for(i = pos + 8; i < pos + 16; i++)
		{
			if(i < len)
				KPrintF("%02lx ", *(data + i));
			else
				KPrintF("   ");
		}
		KPrintF(" ");

		for(i = pos; i < pos + 8; i++)
			KPrintF("%c ", i < len ? is_visible_ascii(*(data + i)) ? *(data + i) : '.' : ' ');

		KPrintF(" ");

		for(i = pos + 8; i < pos + 16; i++)
			KPrintF("%c ", i < len ? is_visible_ascii(*(data + i)) ? *(data + i) : '.' : ' ');

		KPrintF("\n");

		pos += 16;
	}

	KPrintF("\n");

	KPrintF("--------- packet dump end --------------\n");
}

#endif /* __DEBUG__ */
