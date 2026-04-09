/*
 * splitter.c - Milena TTS system utilities
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
#include <string.h>
#include <stdlib.h>
#include "milenizer.h"
#include <ctype.h>

static void failsplit(char *c)
{
	fprintf(stderr,"%s\n",c);
	exit(1);
}

static void print_idx(FILE *idx,char *c,char *ex)
{
	char *d=c+strlen(c)-1;
	int sp=0;
	for (;d>c;d--) if (!strchr(".?!,:;",*d)) break;
	d++;
	for (;c<d;c++) if (!strchr(ex,*c)) break;
	for (;c<d;c++) if (strchr(ex,*c)) sp=1;
	else {
		if (sp) fputs("- ",idx);
		sp=0;
		fputc(*c,idx);
	}
	fputc('\n',idx);
}


void split_body(char *body)
{
	char *title;
	char *c,*d;int lc;
	int this_part;
	int count_part;
	int ndit;
	FILE *f;
	char fpath[256];
	int gline;
	char chpart[256];
	FILE *idx=NULL;
	char *info_part=NULL;
	
	while(*body && my_space(*body)) body++;
	title=body;
	while (*body && *body !='\r' && *body !='\n') body++;
	if (!*body) failsplit("Brak tresci");
	*body++=0;
	while (*body && my_space(*body)) body++;
	if (*body != splitmode) {
	    info_part=body;
	    for (;;) {
		char *eot;
		while (*body && *body !='\r' && *body !='\n') body++;
		eot=body;
		while (*body && my_space(*body)) body++;
		if (!*body) failsplit("Brak tresci");
		if (*body == splitmode) {
		    *eot=0;
		    break;
		}
	    }
	}
	if (!readable(title)) failsplit("Linia tytulowa jest niewymawialna");
	if (make_index) {
		char chindex[256],*c;
		strcpy(chindex,split_path);
		c=strrchr(chindex,'/');
		if (c) c++;
		else c=chindex;
		strcpy(c,"index.nok");
		if (!(idx=fopen(chindex,"w"))) {
			perror(chindex);
			exit(1);
		}
	}
	for (c=d=title;*c;c++) if (my_alnum(*c)) d=c+1;
	lc=0;
	if (strchr(d,'?')) lc='?';
	else if (strchr(d,'!')) lc='!';
	else if (strchr(d,'.')) lc='.';
	if (lc) {
		*d++=lc;
		if (*d) *d=0;
	}
	else {
		if (!*d) {
			c=malloc(strlen(title)+2);
			strcpy(c,title);
			strcat(c,".");
			title=c;
		}
		else {
			*d++='.';
			if (*d) *d=0;
		}
	}
	if (idx) print_idx(idx,title,";");
	for (count_part=1,c=body+1;c && *c;) {
		c=strpbrk(c,"\r\n");
		if (!c) break;
		while (*c && my_space(*c)) c++;
		if (*c==splitmode) count_part++;
	}
	if (count_part>99) ndit=3;
	else ndit=2;
	this_part=0;
	f=NULL;
	for (;;) {
		if (f) {
			if (this_part == 1 && (automode & AUMODE_PROLOG)) {
				fprintf(f,"\n\nkoniec prologu.\n");
			}
			else {
				fprintf(f,"\n\nkoniec rozdzia³u.\n");
			}
			fclose(f);
		}
		body++;
		while (*body && my_space(*body)) body++;
		if (!*body) failsplit("Pusty rozdzial koncowy");
		if (*body==splitmode) failsplit("Pusty rozdzial");
		this_part++;
		sprintf(fpath,"%s_%0*d.txt",split_path,ndit,this_part);
		f=fopen(fpath,"w");
		if (!f) {
			perror(fpath);
			exit(1);
		}
		/* tu piszemy poczatek */
		fprintf(f,"%s ",title);
		if (info_part) {
		    fprintf(f,"\n\n%s\n\n",info_part);
		    info_part=NULL;
		}
		gline=1;
		chpart[0]=0;
		if (automode) {
			int nroz;
			nroz=this_part;
			if (automode & AUMODE_PROLOG) nroz--;
			if (this_part == 1 && (automode & AUMODE_PROLOG)) {
				if (automode & AUMODE_PROLOG_KEEP) {
					strcpy(chpart,"Prolog, ");
				}
				else {
					strcpy(chpart,"Prolog.");
					gline=0;
				}
			}
			else if (this_part == count_part && (automode & (AUMODE_EPILOG | AUMODE_LASTCHAPTER))) {
				if (automode & AUMODE_LASTCHAPTER) {
					sprintf(chpart,"Rozdzia³ %d i ostatni",nroz);
					if (automode & AUMODE_KEEP_LINE) strcat(chpart,", ");
					else {
						gline=0;
						strcat(chpart,".");
					}
				}
				else {
					if (automode & AUMODE_EPILOG_KEEP) {
						sprintf(chpart,"Epilog, ");
					}
					else {
						sprintf(chpart,"Epilog.");
						gline=0;
					}
				}
			}
			else {
				if (automode & AUMODE_KEEP_LINE) sprintf(chpart,"Rozdzia³ %d, ",nroz);
				else {
					sprintf(chpart,"Rozdzia³ %d.",nroz);
					gline=0;
				}
			}
		}
		if (chpart[0]) fprintf(f,"%s",chpart);
		if (gline) {
			c=strpbrk(body,"\r\n");
			if (!c) failsplit("Bledny split");
			*c++=0;
			fprintf(f,"%s",body);
			if (idx) {
				char *sx=strrchr(fpath,'/');
				if (sx) sx++;else sx=fpath;
				fprintf(idx,"%s:",sx);
				if (chpart[0]) strcat(chpart," ");
				strcat(chpart,body);
				print_idx(idx,chpart,";:");
			}
			body=c;
			while(*body && my_space(*body)) body++;
		}
		else if (idx) {
			char *sx=strrchr(fpath,'/');
			if (sx) sx++;else sx=fpath;
			fprintf(idx,"%s:",sx);
			print_idx(idx,chpart,";:");
			
		}
		fprintf(f,"\n\n");
		for (c=body;;) {
			c=strpbrk(c,"\r\n");
			if (!c) break;
			d=c;
			while (*c && my_space(*c)) c++;
			if (*c==splitmode) break;
		}
		if (!c) {
			fputs(body,f);
			break;
		}
		*d++=0;
		fputs(body,f);
		body=c;
	}
	if (f) {
		fprintf(f,"\n\nKoniec ksi±¿ki.\n");
		fclose(f);
	}
}

void split_timed(char *body,int kilobytes)
{
    int bytes=kilobytes * 1024;
    int count,bc;
    int this_part=0;
    char *c,*d;
    FILE *f;
    char fpath[256];
    for (;;) {
	int len=strlen(body);
	this_part++;
	sprintf(fpath,"%s_%0*d.txt",split_path,3,this_part);
	f=fopen(fpath,"w");
	if (!f) {
		perror(fpath);
		exit(1);
	}
	count=(len+bytes/2) / bytes;
	if (count < 2) {
	    fwrite(body,strlen(body),1,f);
	    fclose(f);
	    break;
	}
	bc=len/count;
	c=body+(9*bc)/10;
	if (c-body >= strlen(body)) {
	    fwrite(body,strlen(body),1,f);
	    fclose(f);
	    break;
	}
	    
	d=NULL;
	for (;;) {
	    c=strchr(c,'\n');
	    if (!c) break;
	    if (d && c > body + bc) {
		break;
	    }
	    d=c;
	    c++;
	}
	if (!d) {
	    fwrite(body,strlen(body),1,f);
	    fclose(f);
	    break;
	}
	d++;
	fwrite(body,d-body,1,f);
	fclose(f);
	body=d;
	while (*body && isspace(*body)) body++;
	if (!*body) break;
    }
}
