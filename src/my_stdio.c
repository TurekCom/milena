#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <windows.h>
#include "milena.h"

unsigned char cep2iso[]={
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
	32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
	48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
	64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,
	80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,
	96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,
	112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
	0,0,0,0,0,0,0,0,0,0,169,0,166,171,174,172,
	0,0,0,0,0,0,0,0,0,0,185,0,182,187,190,188,
	160,183,162,163,164,161,0,167,168,0,170,0,0,173,0,175,
	176,0,178,179,180,0,0,0,184,177,186,0,165,189,181,191,
	192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
	208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
	224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
	240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255};

struct my_file {
	int control;
	int eof;
	int fd;
	int buffer_full;
	int ungetc;
	int buflen;
	int bufsize;
	int bufpos;
	char *buffer;
};

int get_string_encoding(char *buf,size_t len)
{
	char *c;
	int i;
	int utf=1;
	static char *isos="??????";
	static char *ceps="??????";
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
	if (utf) return -1;

	/* iso2 czy cp1250? */

	for (iso=cep=0,c=buf;*c;c++) {
		if (strchr(isos,*c)) iso++;
		else if (strchr(ceps,*c)) cep++;
	}
	if (iso < cep) return 1;
	return 0;
}


void my_check_file_encoding(FILE *fi)
{
	int len;char *c;
	int cep,iso,encod,n;
	struct my_file *f=(struct my_file *)fi;	
	len=lseek(f->fd,0,SEEK_END);
	if (f->bufsize<len-1) {
		f->bufsize=len;
		free(f->buffer);
		f->buffer=malloc(len+1);
	}
	f->buffer[len]=0;
	lseek(f->fd,0,SEEK_SET);
	read(f->fd,f->buffer,len);
	f->buflen=len;
	f->bufpos=0;
	f->buffer_full=1;
	
	encod=get_string_encoding(f->buffer,len);
	if (!encod) return; /* ISO-2 */
	if (encod == 1) { /* ISO-1 */
		for (c=f->buffer;*c;c++) {
			*c=cep2iso[(*c)&255];
			if (!*c) *c='?';
			}
		return;
	}
	/* UTF-8 */
	c=f->buffer;
	if (!strncmp(c,"\xEF\xBB\xBF",3)) c+=3;
	n=milena_utf2iso(c,NULL,1,NULL);
	if (n <= 0) return;
	char *bf=malloc(n);
	milena_utf2iso(c,bf,1,NULL);
	free(f->buffer);
	f->buffer=bf;
	f->buflen=strlen(bf);
}
	
FILE *my_fopen(char *name,char *mode)
{
	struct my_file *f;
	int fd;
	if (*mode !='r') return fopen(name,mode);
	f=malloc(sizeof(*f));
	f->fd=open(name,O_RDONLY|O_BINARY);
	if (f->fd<0) {
		free(f);
		return NULL;
	}
	f->bufsize=16000;
	f->buflen=f->bufpos=0;
	f->buffer=malloc(f->bufsize);
	f->eof=f->ungetc=0;
	f->control=0x41424398;
	f->buffer_full=0;
	return (FILE *)f;
}


int my_fgetc(FILE *fi)
{
	int z;struct my_file *f;
	f=(struct my_file *)fi;
	if (f->control != 0x41424398) {
		printf("Bad control\n");
		exit(0);
	}
	if (z=f->ungetc) {
		f->ungetc=0;
		return z;
	}
	if (f->eof) return EOF;
	if (f->buffer_full) {
		if (f->bufpos >= f->buflen) {
			f->eof=1;
			return EOF;
		}
		return f->buffer[f->bufpos++] & 255;
	}
	if (f->bufpos >= f->buflen) {
		f->buflen=read(f->fd,f->buffer,f->bufsize);
		if (f->buflen<=0) {
			f->eof=1;
			return EOF;
		}
		f->bufpos=0;
	}
	return f->buffer[f->bufpos++] & 255;
}

int my_ungetc(int c,FILE *f)
{
	((struct my_file *)f)->ungetc=c;
	return c;
}

int my_fgets(char *buf,int len,FILE *fi)
{
	int z,n;char *c=buf;
	struct my_file *f=(struct my_file *)fi;
	if (f->eof) return 0;
	for (n=0;len>1;) {
		z=my_fgetc(fi);
		if (z==EOF) break;
		if (z=='\n') {
			*buf++=z;
			n++;
			break;
		}
		if (z=='\r') {
			*buf++='\n';
			z=my_fgetc(fi);
			if (z != '\n') my_ungetc(z,fi);
			n++;
			break;
		}
		*buf++=z;
		n++;
		len--;
	}
	*buf=0;
	return n;
}

int my_fclose(FILE *fi)
{
	struct my_file *f=(struct my_file *)fi;
	if (f->control != 0x41424398) return fclose((FILE *)fi);
	close(f->fd);
	free(f->buffer);
	free(f);
	return 0;
}

int my_fseek(FILE *fi,int pos,int whence)
{
	struct my_file *f=(struct my_file *)fi;
	if (f->control != 0x41424398) return fseek((FILE *)fi,pos,whence);
	if (f->buffer_full) {
		int pos;
		switch(whence) {
			case SEEK_END:
				pos+=f->buflen;
				break;
			case SEEK_CUR:
				pos+=f->bufpos;
				break;
		}
		if (pos<0 || pos >f->buflen) {
			return -1;
		}
		f->ungetc=f->eof=0;
		f->bufpos=pos;
		return pos;
	}
	lseek(f->fd,pos,whence);
	f->buflen=f->bufpos=f->ungetc=f->eof=0;
}

void my_perror(char *str)
{
	char buf[1024];
	sprintf(buf,"%s:\n%s",str,strerror(errno));
	MessageBox(NULL,buf,NULL,MB_OK | MB_ICONERROR);
}
