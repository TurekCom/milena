/*
 * convert.c - Milena TTS system utilities
 * Copyright (C) Bohdan R. Rau 2008 <ethanak@polip.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write see:
 *               <http://www.gnu.org/licenses/>.
 */
#ifdef __WIN32
#define LIBICONV_STATIC 1
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iconv.h>
#include <ctype.h>

char *to_utf8(char *fbuf,int flen,char *cset,int freeme)
{
	iconv_t ic;
	char pbuf[256];
	size_t inlen,outlen,desclen;
	char *ubody,*c,*d;
	ic=iconv_open("UTF-8",cset);
	if (ic == (iconv_t)-1) {
		perror(cset);
		exit(1);
	}
	desclen=0;
	inlen=flen;
	c=fbuf;
	while(inlen) {
		outlen=256;
		d=pbuf;
		iconv(ic,&c,&inlen,&d,&outlen);
		if (outlen == 256) {
			perror("iconv");
			exit(1);
		}
		desclen+=256-outlen;
	}
	ubody=malloc(desclen+1);
	iconv(ic,NULL,NULL,NULL,NULL);
	c=fbuf;
	d=ubody;
	inlen=flen;
	outlen=desclen;
	iconv(ic,&c,&inlen,&d,&outlen);
	if (inlen) {
		perror("iconv");
		exit(1);
	}
	iconv_close(ic);
	ubody[desclen]=0;
	if (freeme) free(fbuf);
	return ubody;
}

