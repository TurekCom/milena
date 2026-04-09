/*
 * unformat.c - Milena TTS system utilities
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "milenizer.h"

#define F_ALONE 0x01
#define F_EMPTY 0x02
#define F_DIALOG 0x04
#define F_SPLIT 0x08
#define F_HYPHEN 0x10
#define F_SHORT 0x20

#define F_NONE 0x100
#define F_ENTER 0x200
#define F_SPACE 0x400

extern int autopar,splitmode,unformode;

void unformat(char *body)
{
	char *istr,*ostr,*mstr,*lch;
	int i,j,nline,nenter;
	struct {
		int flags;
		char *buf;
	} *linie;
	ostr=mstr=body;
	for (;*body && isspace(*body);body++);
	for (istr=body,nline=1;*istr;istr++) {
		if (*istr=='\r') {
			if (istr[1]!='\n') nline++;
			continue;
		}
		if (*istr=='\n') nline++;
	}
			
	linie=calloc(nline,sizeof(*linie));
	istr=body;nline=0;
	while (*istr) {
		while (*istr && *istr != '\n' && *istr !='\r' && isspace(*istr)) istr++;
		if (!*istr) break;
		linie[nline].flags=0;
		linie[nline++].buf=ostr;
		lch=ostr;
		
		while (*istr && *istr != '\n' && *istr !='\r') {
			if (!isspace(*istr)) lch=ostr+1;
			*ostr++=*istr++;
		}
		if (*istr=='\r') istr++;
		if (*istr=='\n') istr++;
		*lch=0;
		ostr=lch+1;
	}
	if (unformode & UNFORMODE_IGNORE_PAGENO) {
		
		for (i=j=0;i<nline;i++) {
			char *c=linie[i].buf;
			if (!*c) goto loop;
			for (;*c;c++) if (!isdigit(*c)) goto loop;
			for (;j>0 && !*linie[j-1].buf;j--);
			while (i<nline-1 && !*linie[i+1].buf) i++;
			continue; 
		loop:	linie[j].flags = linie[i].flags;
			linie[j++].buf=linie[i].buf;
		}
		nline=j;
	}
	i=0;
	if (splitmode) {
		linie[i].flags=F_ALONE;
		i=1;
	}
	for (;i<nline;i++) {
		int len;
		if (!*linie[i].buf) {
			linie[i].flags = F_EMPTY;
			continue;
		}
		if (!readable(linie[i].buf)) {
			linie[i].flags = F_ALONE;
			continue;
		}
		if (!(unformode & UNFORMODE_NO_DIALOG) && linie[i].buf[0]=='-') linie[i].flags |= F_DIALOG;
		else if (linie[i].buf[0]==splitmode) linie[i].flags |= F_SPLIT;
		len=strlen(linie[i].buf);
		if (autopar && len <= autopar) {
			linie[i].flags |= F_SHORT;
		}
		else if (i<nline-1 && len>10 && (linie[i].buf[len-1] & 255)==0xad) {
			linie[i].buf[len-1]=0;
			linie[i].flags |= F_HYPHEN;
		}
		else if (i<nline-1 && len>10 && linie[i].buf[len-1]=='-') {
			int ns,tr;
			ns=len-2;
			if (my_alpha(linie[i].buf[ns]) && my_alpha(linie[i+1].buf[0])) {
				tr=0;
				for (;ns>=0;ns--) {
					if (!my_alnum(linie[i].buf[ns])) break;
					if (!my_alpha(linie[i].buf[ns])) {
						tr=1;
						break;
					}
				}
				if (!tr) {
					for (ns=0;linie[i+1].buf[ns];ns++) {
						if (!my_alnum(linie[i+1].buf[ns])) break;
						if (!my_alpha(linie[i+1].buf[ns])) {
							tr=1;
							break;
						}
					}
				}
				if (!tr) {
					linie[i].buf[len-1]=0;
					linie[i].flags |= F_HYPHEN;
				}
			}
		}
	}
	for (i=0;i<nline-1;i++) {
		char *c,*d;
		for (c=d=linie[i].buf;*c;c++) if ((*c & 255)!=0xad) *d++=*c;
		*d=0;
		if (linie[i+1].flags & (F_ALONE|F_EMPTY|F_DIALOG|F_SPLIT)) {
			linie[i].flags |= F_ENTER;
			if (!autopar && (linie[i+1].flags & F_EMPTY)) {
				linie[i+1].flags |= F_NONE;
				if (i<nline-2 && linie[i+2].flags & F_EMPTY) {
					linie[i+1].flags |= F_ENTER;
					i++;
				}
			}
			continue;
		}
		if (linie[i].flags & (F_ALONE | F_EMPTY | F_SPLIT | F_SHORT)) {
			linie[i].flags |= F_ENTER;
			continue;
		}
		linie[i].flags |= F_SPACE;
	}
	linie[i].flags |= F_ENTER;
	nenter=0;
	for (i=0;i<nline;i++) {
		char *c=linie[i].buf;
		if (linie[i].flags & F_NONE) continue;
		while (*c) *mstr++=*c++;
		if (linie[i].flags & F_ENTER) {
			if (linie[i].flags & F_EMPTY) {
				if (nenter) continue;
				nenter++;
			}
			else nenter=0;
			*mstr++='\n';
		}
		else {
			nenter=0;
			if ((linie[i].flags & (F_SPACE | F_HYPHEN))==F_SPACE) *mstr++=' ';
		}
	}
	*mstr=0;
	free(linie);
}

