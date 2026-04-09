/*
 * readfile.c - Milena TTS system utilities
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
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iconv.h>
#include <ctype.h>
#include <milena.h>
#include "milenizer.h"

#ifdef HAVE_ENCA
#include <math.h>
#include <enca.h>
static const char *get_encoding(char *buf,size_t len)
{
	EncaAnalyser an;
	const double mu = 0.005;
	const double m = 15.0;
	size_t sgnf;
	const char *c;
	EncaEncoding result;
	
	an=enca_analyser_alloc("pl");
	if (!an) {
		fprintf(stderr,"Enca nie dziala\n");
		exit(1);
	}
	enca_set_threshold(an, 1.38);
	enca_set_multibyte(an, 1);
	enca_set_ambiguity(an, 1);
	enca_set_garbage_test(an, 1);
	sgnf = ceil((double)len/(len/m + 1.0/mu));
	enca_set_significant(an, sgnf);
	enca_set_termination_strictness(an, 1);
	enca_set_filtering(an, sgnf > 2);

	result = enca_analyse_const(an, (unsigned char *)buf, len);	
	if (!enca_charset_is_known(result.charset)) {
		fprintf(stderr,"Nierozpoznane kodowanie\n");
		exit(1);
	}
	c=enca_charset_name(result.charset,ENCA_NAME_STYLE_ICONV);
	enca_analyser_free(an);
	return c;
}
#else

static const char *get_encoding(char *buf,size_t len)
{
	char *c;
	int i;
	int utf=1;
	static char *isos="±∂º°¶¨";
	static char *ceps="πúü•åè";
	int iso,cep;
	for (c=buf,i=0;*c && i<len;i++) {
		int p,n=(*c++) & 0xff;
		if (!(n & 0x80)) continue;
		if ((n & 0xe0)==0xc0) p=1;
		else if ((n & 0xf0)==0xe0) p=2;
		else if ((n & 0xf8)==0xf0) p=3;
		else if ((n & 0xfc)==0xf8) p=4;
		else if ((n & 0xfe)==0xfc) p=5;
		else {
			utf=0;
			break;
		}
		for (;p && *c;p--) {
			n=((*c++) & 0xc0);
			if (n != 0x80) {
				utf=0;
				break;
			}
		}
		if (!utf) break;
	}
	if (utf) return "UTF-8";

	/* iso2 czy cp1250? */

	for (iso=cep=0,c=buf;*c;c++) {
		if (strchr(isos,*c)) iso++;
		else if (strchr(ceps,*c)) cep++;
	}
	if (iso < cep) return "CP1250";
	return "ISO-8859-2";
}


#endif




static int my_compare(char *s1,char *s2)
{
	while (*s1) {
		while (*s1 && !isalnum(*s1)) s1++;
		if (!*s1) break;
		if (*s2++ != tolower(*s1++)) return 0;
	}
	if (*s2) return 0;
	return 1;
}


int pdfmode,nodrm;
extern char *read_rtf(char *str,int len);
#define TESTBUF_SIZE 100
char *read_file(char *name,char *encoding)
{
	int fd;
	struct stat sb;
	int flen,n,inb,fmode;
	char *fbuf,*isostr;
	char tempo[64];
	static unsigned char *isword[3]={
		(unsigned char *)"\004\376\067\0\043",
		(unsigned char *)"\x8\320\317\021\340\241\261\032\341",
		(unsigned char *)"\x6\333\245-\0\0\0"};
	char testbuf[TESTBUF_SIZE];
	int i;


	if (stat(name,&sb)) {
		perror(name);
		exit(1);
	}
	if ((fd=open(name,O_RDONLY))<0) {
		perror(name);
		exit(1);
	}
	flen=sb.st_size;
	inb=0;fmode=0;
	if (flen > TESTBUF_SIZE) {
		if (read(fd,testbuf,TESTBUF_SIZE)!=TESTBUF_SIZE) {
			perror(name);
			exit(1);
		}
		inb=TESTBUF_SIZE;
		fmode=0;
		if (!strncmp(testbuf,"{\\rtf",5)) fmode=1;
		else if (!strncmp(testbuf,"%PDF-1",6)) fmode=4;
		else {
			for (i=0;i<3;i++) if (!memcmp(testbuf,isword[i]+1,isword[i][0])) {
				fmode=2;
				break;
			}

		}
		if (!fmode) {
			if (!memcmp(testbuf,"PK\003\004",4)) {
				for (i=0;i<TESTBUF_SIZE-27;i++) {
					if (!memcmp(testbuf+i,"vnd.oasis.opendocument.text",27)) {
						fmode=3;
						break;
					}
				}
			}
		}
	}
	if (fmode==2 || fmode==3 || fmode ==4) {
		char sbuf[1024],*c,*d;
		close(fd);
		if (fmode==2) {
			strcpy(sbuf,"antiword -w 0 -m UTF-8.txt ");
		}
		else if (fmode ==3) {
			strcpy(sbuf,"odt2txt --width=-1 --encoding=UTF-8 ");
		}
		else {
			if (pdfmode) {
			    strcpy(sbuf,"pdftohtml -i -noframes -xml -stdout -enc UTF-8 ");
			    if (nodrm) strcat(sbuf,"-nodrm ");
			}
			else strcpy(sbuf,"pdftotext -raw -enc UTF-8 ");
		}
		c=name;
		d=sbuf+strlen(sbuf);
		for (;*c;c++) {
		if (!isalnum(*c) && !strchr("./",*c)) *d++='\\';
			*d++=*c;
		}
		if (fmode == 4 && !pdfmode) {
			strcpy(d," ");
		}
		else {
			strcpy(d," > ");
		}
		strcpy(tempo,"/tmp/milenizer_XXXXXX");
		fd=mkstemp(tempo);
		if (fd<0) {
			perror("mkstemp");
			exit(1);
		}
		close(fd);
		strcat(sbuf,tempo);
		system(sbuf);
		if (stat(tempo,&sb)) {
			perror(tempo);
			remove(tempo);
			exit(1);
		}
		flen=sb.st_size;
		fd=open(tempo,O_RDONLY);
		if (fd<0) {
			perror(tempo);
			remove(tempo);
			exit(1);
		}
		inb=0;
		remove(tempo);
		encoding="UTF-8";
		//fmode=0;
	}
	fbuf=malloc(flen+1);
	if (inb) memcpy(fbuf,testbuf,inb);
	if (read(fd,fbuf+inb,flen-inb)!=flen-inb) {
		perror(name);
		exit(1);
	}
	close(fd);
	fbuf[flen]=0;
	if (fmode == 2 && rtfdecode) return fbuf;
	if (fmode==1) {
		char *c=read_rtf(fbuf,flen);
		free(fbuf);
		fbuf=c;
		flen=strlen(fbuf);
		if (rtfdecode) return fbuf;
		encoding="UTF-8";
	}
	else if (rtfdecode && fmode !=4) {
		fprintf(stderr,"Plik nie jest w formacie RTF\n");
		exit(1);
	}
	//printf("%d %d\n",fmode,pdfmode);
	if (fmode == 4 && pdfmode) {
		char *c;
		c=new_pdf_parser(fbuf);
		free(fbuf);
		fbuf=c;
		flen=strlen(fbuf);
		
	}
	if (fmode == 4 && rtfdecode) return fbuf;
	if (!encoding) {
		encoding=(char *)get_encoding(fbuf,flen);
		fprintf(stderr,"Prawdopodobne kodowanie: %s\n",encoding);
	}
	if (my_compare(encoding,"iso88592")) return fbuf;
	if (!my_compare(encoding,"utf8")) fbuf=to_utf8(fbuf,flen,encoding,1);
	n=milena_utf2iso(fbuf,NULL,ignore_oor,NULL);
	if (!n) {
		fprintf(stderr,"Plik jest pusty\n");
		exit(1);
	}
	isostr=malloc(n);
	milena_utf2iso(fbuf,isostr,ignore_oor,NULL);
	free(fbuf);
	return isostr;
}

