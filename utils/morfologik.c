/*
 * morfologik.c - Milena TTS system utilities
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
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#define BLK_SIZE (1024*1024*1024)
#define WRD_CNT 8192

static int blk_end;
static char *blok;
static char **words;
static int wordcnt,wordsiz;

static void morf_add_word(char *c)
{
	int siz=strlen(c)+1;
	if (!blok || blk_end+siz>BLK_SIZE) {
		blok=malloc(BLK_SIZE);
		if (!blok) exit(1);
		blk_end=0;
	}
	if (!words) {
		words=malloc(sizeof(*words) * (wordsiz=WRD_CNT));
	}
	else if (wordcnt>=wordsiz) {
		words=realloc(words,sizeof(*words) * (wordsiz=wordsiz+WRD_CNT));
	}
	words[wordcnt++]=blok+blk_end;
	strcpy(blok+blk_end,c);
	blk_end+=siz;
}


static int read_morf_line(FILE *f)
{
	static char linebuf[512];
	char *c;
	if (!fgets(linebuf,512,f)) return 0;
	c=strchr(linebuf,'\t');
	if (!c) return 1;
	*c++=0;
	c=strchr(c,'\t');
	if (!c) return 1;
	if (strstr(c,"ign")) return 1;
	if (strstr(c,"qub")) return 1;
	for (c=linebuf;*c;c++) {
		if (*c>='a' && *c<='z') continue;
		if (!strchr("±ê¶æñó¼¿³",*c)) break;
	}
	if (*c) {
		return 1;
	}
	morf_add_word(linebuf);
	return 1;
}

static int morf_strcmp(const void *s1,const void *s2)
{
	return strcmp(*(char **)s1,*(char **)s2);
}

void morfologik_read(char *fname)
{
	FILE *f;
	f=fopen(fname,"r");
	if (!f) {
		perror(fname);
		exit(1);
	}
	while(read_morf_line(f));
	fclose(f);
	qsort(words,wordcnt,sizeof(*words),morf_strcmp);
	fprintf(stderr,"Morfologik OK, wczytano %d slow\n",wordcnt);
}

void morfologik_rbase(char *fname)
{
	int fd,i;
	size_t len;
	char *bdy;
	struct stat sb;
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
	
	bdy=malloc(len);
	if (read(fd,bdy,len)!=len) {
		perror("Base read");
		exit(1);
	}
	close(fd);
	wordcnt=wordsiz=strtol(bdy,NULL,10);
	bdy+=10;
	words=malloc(wordcnt*sizeof(*words));
	for (i=0;i<wordcnt;i++) {
		words[i]=bdy;
		bdy+=strlen(bdy)+1;
	}
	fprintf(stderr,"Base OK, wczytano %d slow\n",wordcnt);
}

int morfologik_find(char *c)
{
	int lo,hi,mid,n;
	lo=0;hi=wordcnt-1;
	while (lo<=hi) {
		mid=(lo+hi)/2;
		n=strcmp(words[mid],c);
		if (!n) return 1;
		if (n>0) hi=mid-1;
		else lo=mid+1;
	}
	return 0;
}
