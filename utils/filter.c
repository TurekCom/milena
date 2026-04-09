#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/milena.h"
#include "milenizer.h"
#include <getopt.h>
#include <ctype.h>

char inbuf[8192];
char *outbuf;

void help(char *name)
{
	fprintf(stderr,"Sposob uzycia:\n%s [-h] [-R] [-H] [kodowanie]\n",name);
	exit(0);
}

static struct {
	char *entity;
	int znak,len;
} znaczki[]={
	{"lt",'<',2},
	{"gt",'>',2},
	{"amp",'&',3},
	{"quot",'"',4},
	{"apos",'\'',4},
	{NULL,0}};

void emit_unichar(int n)
{
	char inbuf[8],outbuf[64];
	int m;
	if (n<0x80) {
		fputc(n,stdout);
		return;
	}
	if (n<=0x7ff) {
		inbuf[0]=0xc0 | (n>>6);
		inbuf[1]=0x80 | (n & 0x3f);
		inbuf[2]=0;
	}
	else if (n<0x10000) {
		inbuf[0]=0xe0 | (n>>12);
		inbuf[1]=0x80 | ((n>>6) & 0x3f);
		inbuf[2]=0x80 | (n & 0x3f);
		inbuf[3]=0;
	}
	else {
		fputc('?',stdout);
		return;
	}
	m=milena_utf2iso(inbuf,outbuf,1,NULL);
	if (m) fputs(outbuf,stdout);
}


char *mentity(char *str)
{
	int n,base,i;
	char *c;
	str++;
	if (*str=='#') {
		str++;
		base=10;
		if (*str=='x' || *str=='X') {
			str++;
			base=16;
		}
		n=strtol(str,&c,base);
		if (*c!=';') return NULL;
		c++;
		emit_unichar(n);
		return c;
	}
	for (i=0;znaczki[i].znak;i++) if (!strncmp(str,znaczki[i].entity,znaczki[i].len)) break;
	if (znaczki[i].znak) {
		c=str+znaczki[i].len;
		if (*c==';') {
			emit_unichar(znaczki[i].znak);
			return c+1;
		}
	}
	return NULL;
}


void mhtml(char *str)
{
	char *c;
	while (*str) {
		c=strpbrk(str,"<&");
		if (!c) {
			fputs(str,stdout);
			return;
		}
		if (c!=str) {
			fwrite(str,c-str,1,stdout);
			str=c;
		}
		if (*str=='&') {
			c=mentity(str);
			if (c) {
				str=c;
				continue;
			}
			fputc(*str++,stdout);
			continue;
		}
		if (!strncmp(str,"</",2)) {
			for (c=str+2;*c && *c!='>';c++) if (!isalpha(*c)) break;
			if (*c=='>') {
				c++;
				str=c;
				continue;
			}
			fputc(*str++,stdout);
			continue;
		}
		if (!str[1] || !isalpha(str[1])) {
			fputc(*str++,stdout);
			continue;
		}
		c=strchr(str,'>');
		if (c) str=c+1;
		else fputc(*str++,stdout);
	}
}

int main(int argc,char *argv[])
{
	int n,html=0,srdr=0;
	char *charset;
	for(;;) {
		int c=getopt(argc,argv,"hHR");
		if (c<0) break;
		if (c=='H') {
			html=1;
			continue;
		}
		if (c=='R') {
			srdr=1;
			continue;
		}
		help(argv[0]);
	}
	if (argc>optind) charset=argv[optind];
	else charset="UTF-8";
	while(fgets(inbuf,8192,stdin)) {
		int frees=0;
		char *s;
		if (strcasecmp(charset,"UTF-8")) {
			s=to_utf8(inbuf,strlen(inbuf),charset,0);
			frees=1;
		}
		else s=inbuf;
		n=milena_utf2iso(s,NULL,1,NULL);
		if (!n) {
			if (frees) free(s);
			continue;
		}
		outbuf=malloc(n);
		milena_utf2iso(s,outbuf,srdr?3:1,NULL);
		if (frees) free(s);
		if (!html) {
			fputs(outbuf,stdout);
		}
		else {
			mhtml(outbuf);
		}
		free(outbuf);
	}
	exit(0);
}
