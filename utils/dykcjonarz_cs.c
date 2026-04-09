/*
 * dykcjonarz_cs.c - Milena TTS system utilities
 * Copyright (C) Bohdan R. Rau 2009 <ethanak@polip.com>
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
#include <time.h>
#include <ctype.h>

FILE *inf,*outf,*tmpf;
struct rule {
	struct rule *next;
	char *rin;
	char *rout;
} *rules;

int main(int argc,char *argv[])
{
	char buf[1024];
	char xname[256];
	char xtemp[256];
	FILE *f;
	int fd;
	int was=0;
	char *c;
	
	if (argc != 3) {
		fprintf(stderr,"%s <infile> <outfile>\n",argv[0]);
		exit(1);
	}
	inf=fopen(argv[1],"r");
	if (!inf) {
		perror(argv[1]);
		exit(1);
	}
	outf=fopen(argv[2],"w");
	if (!outf) {
		perror(argv[2]);
		exit(1);
	}
	sprintf(xtemp,"%s/tmp/milena_XXXXXX",getenv("HOME"));
	fd=mkstemp(xtemp);
	if (fd<0) {
		perror(xtemp);
		exit(1);
	}
	tmpf=fdopen(fd,"w");
	sprintf(xname,"%s/.milena_pl_userdic.dat",getenv("HOME"));
	f=fopen(xname,"r");
	if (f) {
		while(fgets(buf,1024,f)) {
			struct rule *ru;
			fputs(buf,tmpf);
			c=strstr(buf,"//");
			if (c) *c=0;
			else c=buf+strlen(buf);
			while (c>buf) {
				c--;
				if (!isspace(*c)) break;
				*c=0;
			}
			if (!*buf) continue;
			c=buf;
			while (*c && !isspace(*c)) c++;
			if (*c) {
				*c++=0;
				while (*c && isspace(*c)) c++;
			}
			ru=malloc(sizeof(*ru));
			ru->rin=strdup(buf);
			if (*c) ru->rout=strdup(c);
			else ru->rout="";
			ru->next=rules;
			rules=ru;
		}
		fclose(f);
	}
	was=0;
	while(fgets(buf,1024,inf)) {
		int to_main=0;
		char *wrd;
		struct rule *ru;
		c=strstr(buf,"//");
		if (c) *c=0;
		else c=buf+strlen(buf);
		while (c>buf) {
			c--;
			if (!isspace(*c)) break;
			*c=0;
		}
		if (!*buf) continue;
		wrd=buf;
		if (*wrd=='#') {
			wrd++;
			to_main=1;
			if (!*wrd) continue;
			printf("To main %s\n",wrd);
		}
		c=wrd;
		while (*c && !isspace(*c)) c++;
		if (*c) {
			*c++=0;
			while (*c && isspace(*c)) c++;
		}
		if (!to_main) {
			if (*c) fprintf(outf,"%s %s",wrd,c);
			continue;
		}
		for (ru=rules;ru;ru=ru->next) {
			if (strcmp(ru->rin,wrd)) continue;
			if (!strcmp(ru->rout,c)) break;
		}
		if (ru) continue;
		if (!was) {
			time_t now=time(0);
			struct tm *tm=localtime(&now);
			char tmbuf[256];
			strftime(tmbuf,256,"// %d.%m.%Y %H:%M",tm);
			fprintf(tmpf,"\n%s\n",tmbuf);
			was=1;
		}
		fprintf(tmpf,"%s",wrd);
		if (*c) {
			fprintf(tmpf," %s",c);
		}
		fprintf(tmpf,"\n");
	}
	fclose(tmpf);
	printf("Was=%d\n",was);
	if (was) {
		if (rename(xtemp,xname)) {
			perror("rename");
			exit(1);
		}
	}
	else remove(xtemp);
	
}
