/*
 * dykcjonarz.c - Milena TTS system utilities
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
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <getopt.h>

#include <milena.h>
#include "morfologik.h"

static char lci[256]={
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	'0','1','2','3','4','5','6','7','8','9',0,0,0,0,0,0,
	0,'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
	'p','q','r','s','t','u','v','w','x','y','z',0,0,0,0,0,
	0,'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
	'p','q','r','s','t','u','v','w','x','y','z',0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,'±',0,'³',0,'µ','¶',0,0,'¹','º','»','¼',0,'¾','¿',
	0,'±',0,'³',0,'µ','¶',0,0,'¹','º','»','¼',0,'¾','¿',
	'à','á','â','ã','ä','å','æ','ç','è','é','ê','ë','ì','í','î','ï',
	'ð','ñ','ò','ó','ô','õ','ö',0,'ø','ù','ú','û','ü','ý','þ','ß',
	'à','á','â','ã','ä','å','æ','ç','è','é','ê','ë','ì','í','î','ï',
	'ð','ñ','ò','ó','ô','õ','ö',0,'ø','ù','ú','û','ü','ý','þ',0};

static char uci[256]={
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,1,0,1,0,1,1,0,0,1,1,1,1,0,1,1,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static char l2u[256]={
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	'0','1','2','3','4','5','6','7','8','9',0,0,0,0,0,0,
	0,'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
	'P','Q','R','S','T','U','V','W','X','Y','Z',0,0,0,0,0,
	0,'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
	'P','Q','R','S','T','U','V','W','X','Y','Z',0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,'¡',0,'£',0,'¥','¦',0,0,'©','ª','«','¬',0,'®','¯',
	0,'¡',0,'£',0,'¥','¦',0,0,'©','ª','«','¬',0,'®','¯',
	'À','Á','Â','Ã','Ä','Å','Æ','Ç','È','É','Ê','Ë','Ì','Í','Î','Ï',
	'Ð','Ñ','Ò','Ó','Ô','Õ','Ö',0,'Ø','Ù','Ú','Û','Ü','Ý','Þ','S',
	'À','Á','Â','Ã','Ä','Å','Æ','Ç','È','É','Ê','Ë','Ì','Í','Î','Ï',
	'Ð','Ñ','Ò','Ó','Ô','Õ','Ö',0,'Ø','Ù','Ú','Û','Ü','Ý','Þ',0};

struct slowo {
	struct slowo *l,*r;
	int flags;
	char *str;
	int count;
} *slowa;

static struct milena *milena;
char *use_morfologik,*use_base;

void insert_word(struct slowo **s,char *str,int flags)
{
	while (*s) {
		int n=strcmp((*s)->str,str);
		if (n<0) s=&(*s)->l;
		else if (n>0) s=&(*s)->r;
		else break;
	}
	if (!*s) {
		*s=malloc(sizeof(**s));
		(*s)->flags=flags;
		(*s)->str=strdup(str);
		(*s)->l=(*s)->r=NULL;
		(*s)->count=1;
	}
	else {
	    (*s)->flags |= flags;
	    (*s)->count ++;
	}
}

struct dict {
	struct dict *next;
	char *wrd;
} *dict,*sdict;

void read_dict(char *line,int mode)
{
	char *c;
	struct dict *d;
	c=strstr(line,"//");
	if (c) *c=0;
	while (*line && isspace(*line)) line++;
	if (!*line) return;
	for (c=line;*c && !isspace(*c); c++);
	*c=0;
	d=malloc(sizeof(*d));
	if (!mode) {
		d->next=dict;
		dict=d;
	}
	else {
		d->next=sdict;
		sdict=d;
	}
	d->wrd=strdup(line);
}


void read_dic_file(char *fname,int mode)
{
	FILE *f;
	char buf[1024];
	f=fopen(fname,"r");
	if(!f) {
		perror(fname);
		return;
	}
	while(fgets(buf,1024,f)) read_dict(buf,mode);
	fclose(f);
}


int get_word(char **str)
{
	char *s;
	int flags;
	int bof;
	if (!**str) return 0;
	if (!(bof=milena_SkipToUnknownWord(milena,str))) return 0;
	s=*str;
	if (bof==2) flags=0;
	else if (uci[(**str) & 255]) flags=1;
	else flags = 2;
	for (;**str;(*str)++) {
		if (lci[(**str) & 255]) {
			if (isdigit(**str)) flags |= 4;
			else flags |= 8;
			**str=lci[(**str) & 255];
			continue;
		}
		if (**str !='\'' && **str != '-') break;
		if (!lci[(*str)[1] & 255]) break;
	}
	bof=0;
	if (**str) {
		if (strchr(".?!\n",**str)) bof=1;
		*(*str)++=0;
	}
	if ((flags & 12)==4) return 1;
	if (strlen(s)<3) return 1;
	insert_word(&slowa,s,flags);
	return 1;
	
}


char *trans(char *str)
{
	static char buf[8192],buf2[8192];
	int dummy;
	milena_GetPhrase(milena,&str,buf,8192,&dummy);
	milena_Prestresser(milena,buf,buf2,8192);
	milena_TranslatePhrase(milena,(unsigned char *)buf2,buf,8192,0);
	milena_Poststresser(buf,buf2,8192);
	return buf2;
}

void walk_nomorf(struct slowo *s,int uca)
{
	struct dict *d;
	while (s) {
		walk_nomorf(s->r,uca);
		for (d=sdict;d;d=d->next) if (!strcmp(d->wrd,s->str)) break;
		if (!(s->flags & 16) &&
				((uca && (s->flags & 7) == 1) ||
				(!uca && (s->flags & 7) != 1))) {
		    if (d || milena_IsIgnoredWord(milena,s->str)) {
			s->flags |= 16;
		    }
				    
		    else {
			    if (!morfologik_find(s->str)) {
				//if ((s->flags & 15)==9) printf("%s $S\n",s->str);
				
				if ((s->flags & 7) == 1 && l2u[(*s->str) & 255]) {
				    printf("%c%s",l2u[(*s->str) & 255],s->str+1);
				}
				else {
				    printf("%s",s->str);
				}
				printf(" //%d %s\n",s->count,trans(s->str));
				s->flags |= 16;
			    }
		    }
		}
		s=s->l;
	}
}
void walk(struct slowo *s)
{
	static char *pstr="µ¹º»µ¹º»¾àáâãäåçèéëìíîïðòôõöøùúûüýþßqvx'-";
	static char *estr="aeiouy±êó";
	struct dict *d;
	while (s) {
		walk(s->r);
		if (!(s->flags & 16)) {
			for (d=sdict;d;d=d->next) if (!strcmp(d->wrd,s->str)) break;
			if (!d) {
			    if ((s->flags & 3)==1 || strpbrk(s->str,pstr) || !strpbrk(s->str,estr)) {
				if ((s->flags & 7) == 1 && l2u[(*s->str) & 255]) {
				    printf("%c%s",l2u[(*s->str) & 255],s->str+1);
				}
				else {
				    printf("%s",s->str);
				}
				printf(" //%d %s\n",s->count,trans(s->str));				}
			}
		}
		s=s->l;
	}
}


void help(char *nm)
{
	fprintf(stderr,"Sposob uzycia:\n %s [-L jezyk]... [-u|-f slownik]... [-t temat]... [-m morfologik | [-b baza] [-B]  plik.txt\n",nm);
	exit(1);
}

void read_home_udic(void)
{
	char path[256];
	struct stat sb;
	sprintf(path,"%s/.milena_pl_userdic.dat",getenv("HOME"));
	if (stat(path,&sb)) return;
	if (!milena_ReadUserDicWithFlags(milena,path,MILENA_UDIC_DICTMODE)) exit(1);
	
}

static int file_exists(char *path)
{
	struct stat sb;
	return !stat(path,&sb);
}

int main(int argc,char *argv[])
{
	char *fname;
	char *body;
	struct stat sb;
	size_t len;
	int fd;
	char ibuf[256];
	char obuf[256];
	char ixbuf[256];
	int got_home_udic=0;
	int no_base=0;
	
	read_dic_file(milena_FilePath("pl_dict.dat",ibuf),1);
	milena=milena_Init(
		milena_FilePath("pl_pho.dat",obuf),
		milena_FilePath("pl_dict.dat",ibuf),
		milena_FilePath("pl_stress.dat",ixbuf));
	if (!milena) exit(1);
	if (!milena_ReadPhraser(milena,
		milena_FilePath("pl_phraser.dat",ibuf))) exit(1);
	if (!milena_ReadUserDic(milena,
		milena_FilePath("pl_udict.dat",ibuf))) exit(1);

	for(;;) {
		int c=getopt(argc,argv,"u:L:f:t:m:b:B");
		if (c<0) break;
		if (c=='u') {
			if (!got_home_udic) {
				read_home_udic();
				got_home_udic=1;
			}
			if (!milena_ReadUserDicWithFlags(milena,optarg,MILENA_UDIC_DICTMODE)) exit(1);
			continue;
		}
		if (c=='B') {
			no_base=1;
			continue;
		}
		if (c=='f') {
			if (!got_home_udic) {
				read_home_udic();
				got_home_udic=1;
			}
			if (!milena_ReadPhraser(milena,optarg)) exit(1);
			continue;
		}
		if (c=='m') {
			use_morfologik=optarg;
			continue;
		}
		if (c=='b') {
			use_base=optarg;
			continue;
		}
		if (c=='L') {
			sprintf(obuf,"pl_%s_udic.dat",optarg);
			milena_FilePath(obuf,ibuf);
			if (file_exists(ibuf)) {
				if (!milena_ReadUserDic(milena,ibuf)) exit(1);
			}
			sprintf(obuf,"pl_%s_stress.dat",optarg);
			milena_FilePath(obuf,ibuf);
			if (file_exists(ibuf)) {
				if (!milena_ReadStressFile(milena,ibuf)) exit(1);
			}
			milena_SetLangMode(milena,optarg);
			continue;
		}
		if (c=='t') {
			sprintf(obuf,"pl_%s_theme.dat",optarg);
			if (!milena_ReadPhraser(milena,
				milena_FilePath(obuf,ibuf))) exit(1);
			continue;
		}
		
		help(argv[0]);
	}
	if (!got_home_udic) {
		read_home_udic();
		got_home_udic=1;
	}
	if (optind!=argc-1) help(argv[0]);
	if (use_morfologik && use_base) help(argv[0]);
	if (!use_morfologik && !use_base && !no_base) {
		struct stat sb;
		use_base=DATAPATH"-words/pl_basewords.dat";
		if (stat(use_base,&sb)) use_base=NULL;
	}
	if (use_morfologik) morfologik_read(use_morfologik);
	else if (use_base) morfologik_rbase(use_base);
	fname=argv[optind];
	if (stat(fname,&sb)) {
		perror(fname);
		exit(1);
	}
	len=sb.st_size;
	fd=open(fname,O_RDONLY);
	if (fd<0) {
		perror(fname);
		exit(1);
	}
	body=malloc(len+1);
	if (read(fd,body,len)!=len) {
		perror(fname);
		exit(1);
	}
	close(fd);
	body[len]=0;
	while (get_word(&body));
	if (use_morfologik || use_base) {
		printf("// BRAK W MORFOLOGIKU\n");
		walk_nomorf(slowa,1);
		walk_nomorf(slowa,0);
		printf("\n// WYSTÊPUJ¡ W MORFOLOGIKU\n\n");
	}
	walk(slowa);
}
