/*
 * unrtf.c - Milena TTS system utilities
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
#include <ctype.h>
#include <iconv.h>

#ifdef __WIN32
#define perror my_perror
extern void my_perror(char *);
#endif

#define FOOT_NORMAL 1
#define FOOT_AUDIO 2
#define FOOT_ATEND 4
#define FOOT_IGNORE 0
#define FOOT_INAUDIO 8

static struct fcharset {
	int fcs;
	int cp;
} fcharsets[]={
{0,1252},
{1,0},
{161,1253},
{162,1254},
{163,1258},
{177,1255},
{186,1257},
{204,1251},
{238,1250},
{-1,-1}};

static struct fontdef {
	struct fontdef *next;
	int f;
	int codepage;
	iconv_t ic;
} *fds;

static void free_fontdefs(void)
{
	struct fontdef *fd;
	while ((fd=fds)) {
		fds=fds->next;
		if (fd->ic) iconv_close(fd->ic);
		free(fd);
	}
}


static void add_fontdef(int f,int fch)
{
	int i,cp;
	struct fontdef *fd;
	for (i=0;;i++) {
		if (fcharsets[i].fcs<0) return;
		if (fcharsets[i].fcs==fch) {
			cp=fcharsets[i].cp;
			break;
		}
	}
	fd=malloc(sizeof(*fd));
	fd->next=fds;
	fds=fd;
	fd->f=f;
	fd->codepage=cp;
	fd->ic=NULL;
}

static int this_font;


static int footNoteMode = FOOT_IGNORE;
static char *footString="przypis";

static char *global_str;
static int global_len;

static int get_chara(void)
{
	if (global_len<=0) return EOF;
	global_len--;
	return (*global_str++) & 255;
	
}

static void un_getchar(int dummy)
{
	global_str--;
	global_len++;
}

int setFootNote(char *c)
{
	switch(*c) {
		case 'd': footNoteMode=FOOT_IGNORE;return 1;
		case 'a': footNoteMode=FOOT_AUDIO;return 1;
		case 'e': footNoteMode=FOOT_ATEND;return 1;
		case 'n': footNoteMode=FOOT_NORMAL;return 1;
		case 'i': footNoteMode=FOOT_INAUDIO;return 1;
	}
	return 0;
}

static void fail(char *c)
{
#ifdef __WIN32
	MessageBox(NULL,c,NULL,MB_OK | MB_ICONERROR);
#else
	fprintf(stderr,"Failed: %s\n",c);
#endif
	exit(1);
}

static void seof()
{
	fail("unexpected EOF");
}

#define STACK_SIZE 1024

static struct stack_item {
	short mode;
	short cp;
	struct fontdef *fd;
} stack[STACK_SIZE];
static int stack_pos;

static void get_fontdef(int param)
{
	struct fontdef *fd;
	for (fd=fds;fd;fd=fd->next) if (fd->f == param) break;
	if (!fd) return;
	stack[stack_pos].fd=fd;
}


#define KMODE_IGNORE 1
#define KMODE_CHAR 2
#define KMODE_BIN 3
#define KMODE_ACP 4
#define KMODE_FOOTNOTE 5
#define KMODE_SKIPDEST 6
#define KMODE_SETFONT 7
#define KMODE_DEFFONT 8
#define KMODE_CHARSET 9
#define KMODE_NEWPAGE 10

#define RD_SKIP 1
#define RD_FOOT 2
#define RD_FONT 4

static struct keyword {
	char *keyword;
	int mode;
	int equiv;
} keywords[]={
	{"ansi",KMODE_IGNORE,0},
	{"ansicpg",KMODE_ACP,0},
	{"author",KMODE_SKIPDEST,0},
	{"b",KMODE_IGNORE,0},
	{"bin",KMODE_BIN,0},
	{"buptim",KMODE_SKIPDEST,0},
	{"colortbl",KMODE_SKIPDEST,0},
	{"cols",KMODE_IGNORE,0},
	{"comment",KMODE_SKIPDEST,0},
	{"creatim",KMODE_SKIPDEST,0},
	{"doccomm",KMODE_SKIPDEST,0},
	{"emdash",KMODE_CHAR,'-'},
	{"endash",KMODE_CHAR,'-'},
	{"f",KMODE_DEFFONT,0},
	{"facingp",KMODE_IGNORE,0},
	{"fcharset",KMODE_CHARSET,0},
	{"fi",KMODE_IGNORE,0},
	{"fonttbl",KMODE_SETFONT,0},
	{"footer",KMODE_SKIPDEST,0},
	{"footerf",KMODE_SKIPDEST,0},
	{"footerl",KMODE_SKIPDEST,0},
	{"footerr",KMODE_SKIPDEST,0},
	{"footnote",KMODE_FOOTNOTE,0},
	{"ftncn",KMODE_SKIPDEST,0},
	{"ftnsep",KMODE_SKIPDEST,0},
	{"ftnsepc",KMODE_SKIPDEST,0},
	{"header",KMODE_SKIPDEST,0},
	{"headerf",KMODE_SKIPDEST,0},
	{"headerl",KMODE_SKIPDEST,0},
	{"headerr",KMODE_SKIPDEST,0},
	{"i",KMODE_IGNORE,0},
	{"info",KMODE_SKIPDEST,0},
	{"keywords",KMODE_SKIPDEST,0},
	{"landscape",KMODE_IGNORE,0},
	{"ldblquote",KMODE_CHAR,'"'},
	{"li",KMODE_IGNORE,0},
	{"line",KMODE_CHAR,0x0a},
	{"lquote",KMODE_CHAR,'\''},
	{"margb",KMODE_IGNORE,0},
	{"margl",KMODE_IGNORE,0},
	{"margr",KMODE_IGNORE,0},
	{"margt",KMODE_IGNORE,0},
	{"operator",KMODE_SKIPDEST,0},
	{"page",KMODE_NEWPAGE,0},
	{"paperh",KMODE_IGNORE,0},
	{"paperw",KMODE_IGNORE,0},
	{"par",KMODE_CHAR,0x0a},
	{"pgndec",KMODE_IGNORE,0},
	{"pgnlcltr",KMODE_IGNORE,0},
	{"pgnlcrm",KMODE_IGNORE,0},
	{"pgnstart",KMODE_IGNORE,0},
	{"pgnucltr",KMODE_IGNORE,0},
	{"pgnucrm",KMODE_IGNORE,0},
	{"pgnx",KMODE_IGNORE,0},
	{"pgny",KMODE_IGNORE,0},
	{"pict",KMODE_SKIPDEST,0},
	{"printim",KMODE_SKIPDEST,0},
	{"private",KMODE_SKIPDEST,0},
	{"qc",KMODE_IGNORE,0},
	{"qj",KMODE_IGNORE,0},
	{"ql",KMODE_IGNORE,0},
	{"qr",KMODE_IGNORE,0},
	{"rdblquote",KMODE_CHAR,'"'},
	{"revtim",KMODE_SKIPDEST,0},
	{"ri",KMODE_IGNORE,0},
	{"rquote",KMODE_CHAR,'\''},
	{"rtf",KMODE_IGNORE,0},
	{"rxe",KMODE_SKIPDEST,0},
	{"sbkcol",KMODE_IGNORE,0},
	{"sbkeven",KMODE_IGNORE,0},
	{"sbknone",KMODE_IGNORE,0},
	{"sbkodd",KMODE_IGNORE,0},
	{"sbkpage",KMODE_IGNORE,0},
	{"sect",KMODE_NEWPAGE,0},
	{"sectd",KMODE_NEWPAGE,0},
	{"stylesheet",KMODE_SKIPDEST,0},
	{"subject",KMODE_SKIPDEST,0},
	{"tab",KMODE_CHAR,' '},
	{"tc",KMODE_SKIPDEST,0},
	{"title",KMODE_SKIPDEST,0},
	{"txe",KMODE_SKIPDEST,0},
	{"u",KMODE_IGNORE,0},
	{"xe",KMODE_SKIPDEST,0}
		
};

#define KEYNUM (sizeof(keywords)/sizeof(keywords[0]))

int umode;

static int whatMode(char *keyword,int *outch)
{
	int low,hig,mid;
	for (low=0,hig=KEYNUM-1;low<=hig;) {
		int n;
		mid=(low+hig)/2;
		n=strcmp(keywords[mid].keyword,keyword);
		if (!n) {
			*outch=keywords[mid].equiv;
			return keywords[mid].mode;
		}
		if (n>0) hig=mid-1;else low=mid+1;
	}
	if (umode) {
		umode=0;
		stack[stack_pos].mode=RD_SKIP;
		return KMODE_SKIPDEST;
	}
	return KMODE_IGNORE;
}

static int codePage=1250;
static int nline=0,wspace=0;
static iconv_t ic;

static char *txt_memo;
static int txt_size;
static int txt_pos;

static int fnCount;

static struct footNote {
	struct footNote *next;
	int number;
	int txtlen;
	int txtsize;
	char *txt;
} *footNotes;


static void add_char(int c)
{
	if (txt_pos>=txt_size-1) {
		txt_size+=65536;
		txt_memo=realloc(txt_memo,txt_size);
	}
	txt_memo[txt_pos++]=c;
}

static void add_str(char *str,int len)
{
	while (len-- > 0) add_char(*str++);
}

static void newFootnote()
{
	struct footNote *ft;
	char buf[64];
	ft=malloc(sizeof(*ft));
	ft->next=footNotes;
	footNotes=ft;
	ft->number=++fnCount;
	ft->txtlen=0;
	ft->txtsize=1024;
	ft->txt=malloc(ft->txtsize);
	if (footNoteMode & FOOT_AUDIO) {
		sprintf(buf," %s %d",footString,fnCount);
	}
	else sprintf(buf,"[%d]",fnCount);
	add_str(buf,strlen(buf));
}

static void footChar(int z)
{
	if (footNoteMode == FOOT_INAUDIO) {
		add_char(z);
		return;
	}
	if (!footNotes) return;
	if (footNotes->txtlen>=footNotes->txtsize) {
		footNotes->txtsize*=2;
		footNotes->txt=realloc(footNotes->txt,footNotes->txtsize);
	}
	footNotes->txt[footNotes->txtlen++]=z;
}

static void emitUniChar(int c,int is_unicode);
static void emitStr(char *c,int len);
#define emitChar(c) emitUniChar(c,0)

static void flushFootNote(struct footNote *ft)
{
	char ftbuf[64];
	if (ft->next) flushFootNote(ft->next);
	if (footNoteMode & FOOT_AUDIO) {
		sprintf(ftbuf,"%s %d: ",footString,ft->number);
	}
	else sprintf(ftbuf,"[%d] ",ft->number);
	emitStr(ftbuf,-1);
	emitStr(ft->txt,ft->txtlen);
	emitChar('\n');
	free(ft->txt);
	free(ft);
}

static int flushingFoots;

static void flushFootnotes(void)
{
	flushingFoots=1;
	if (footNoteMode != FOOT_AUDIO)	emitStr("---\n",4);
	flushFootNote(footNotes);
	footNotes=NULL;
	flushingFoots=0;
}


static int last_nl=0;
static int in_text=0;
static void emitUniChar(int c,int is_unicode)
{
	size_t l1,l2;
	iconv_t *iic;
	char *c1,*c2;
	char inbuf[4],outbuf[32];
	static char incode[16];
	
	if (c==13) {
		nline++;
		last_nl=1;
		return;
	}
	if (c==10) {
		if (!last_nl) nline++;
		last_nl=0;
		return;
	}
	if (isspace(c) && nline) return;
	if (nline) {
		if (in_text) {
			add_char('\n');
			nline--;
			if (!flushingFoots && footNotes && (footNoteMode & (FOOT_NORMAL | FOOT_AUDIO))) flushFootnotes();
			if (nline) add_char('\n');
		}
		nline=0;
		wspace=0;
	}
	in_text=1;
	if (isspace(c) && !is_unicode) {
		wspace=1;
		return;
	}
	if (wspace) add_char(' ');
	wspace=0;
	if (is_unicode) {
		if (c<=0x7f) {
			add_char(c);
		}
		else if (c <=0x7ff) {
			add_char((c>>6) | 0xc0);
			add_char((c & 0x3f) | 0x80);
		}
		else if (c <= 0xffff) {
			add_char((c>>12) | 0xe0);
			add_char(((c>>6) & 0x3f) | 0x80);
			add_char((c & 0x3f) | 0x80);
		}
		else {
			add_char('?');
		}
		return;
	}
	if (!stack[stack_pos].fd) iic=&ic;
	else iic=&stack[stack_pos].fd->ic;
	if (!*iic) {
		int cp=0;
		if (stack[stack_pos].fd) cp=stack[stack_pos].fd->codepage;
		if (!cp) cp=codePage;
		if (!codePage) fail("no codepage");
		if (codePage != 10000) sprintf(incode,"CP%d",cp);
		else strcpy(incode,"MACINTOSH");
		*iic=iconv_open("UTF-8",incode);
		if (*iic == (iconv_t)-1) {
			perror("iconv open");
			exit(1);
		}
	}
	
	inbuf[0]=c;
	l1=1;
	c1=inbuf;
	l2=32;
	c2=outbuf;
	
	if (iconv(*iic,&c1,&l1,&c2,&l2)==(size_t)-1) {
#ifndef __WIN32
		fprintf(stderr,"%s => %s\n",incode,"UTF-8");
		fprintf(stderr,"%02x\n",inbuf[0] & 255);
#endif
		perror("iconv");
		exit(1);
	}
	l2=32-l2;
	if (l2==2 && outbuf[0]==',' && outbuf[1]==',') {
		outbuf[0]='"';
		c2=outbuf+1;
	}
	for (c1=outbuf;c1<c2;c1++) add_char(*c1);
}


static void emitStr(char *c,int len)
{
	if (len<0) len=strlen(c);
	while (len-- > 0) emitChar(*c++);
}


static void ifnDestSkip()
{
	umode=1;
}

static void parseChar(int z)
{
	if (!stack[stack_pos].mode) emitChar(z);
	else if (stack[stack_pos].mode==RD_FOOT) footChar(z);
}
static void parseUniChar(int z)
{
	if (!stack[stack_pos].mode) emitUniChar(z,1);
	else if (stack[stack_pos].mode==RD_FOOT) footChar(z);
}

static int gethex(void)
{
	char cs[3];
	int z;char *c;
	if ((z=get_chara())==EOF) seof();
	cs[0]=z;
	if ((z=get_chara())==EOF) seof();
	cs[1]=z;
	cs[2]=0;
	z=strtol(cs,&c,16);
	if (*c) fail("bad hex character");
	return z;
}

static int readKeyword(char *kword,int *par)
{
	int z,n,p,h;
	z=get_chara();
	if (z==EOF) seof();
	if (!isalpha(z)) {
		*kword++=z;
		*kword=0;
		return 0;
	}
	*kword++=z;
	for (;;) {
		if ((z=get_chara())==EOF) seof();
		if (!isalpha(z)) break;
		*kword++=z;
	}
	*kword=0;
	n=h=0;
	if (z=='-') {
		n=1;
		if ((z=get_chara())==EOF) seof();
	}
	if (isdigit(z)) {
		p=z-'0';
		for (;;) {
			if ((z=get_chara())==EOF) seof();
			if (!isdigit(z)) break;
			p=10*p+(z-'0');
		}
		if (n) p=-n;
		h=1;
		*par=p;
	}
	if (z!=' ') un_getchar(z);
	return h;
}

static void parseKeyword(void)
{
	char kword[32];int param,hpar,z;
	hpar=readKeyword(kword,&param);
	if (!isalpha(*kword)) {
		switch(*kword) {
			case '\'': parseChar(gethex());return;
			case '*': ifnDestSkip();return;
			case '~': parseChar(' ');return;
		}
		parseChar(*kword);
		return;
	}
	if (*kword=='u' && !kword[1]) {
		parseUniChar(param);
		if (global_len >=4 && !strncmp(global_str,"\\'",2)) {
			global_len -=4;
			global_str+=4;
			if (global_len >=4 && !strncmp(global_str,"\\'",2)) {
			    global_len -=4;
			    global_str+=4;
			}
		}
		else if (global_len >1) {
		    global_len -= 1;
		    global_str += 1;
		}
		return;
	}
	switch(whatMode(kword,&z)) {
		case KMODE_BIN:
			if (!hpar || param<0) fail("illegal binary length");
			while (--param>=0) get_chara();
			return;
		case KMODE_CHAR:
			parseChar(z);
			return;
		case KMODE_ACP:
			if (!hpar) fail("no param with codepage");
			codePage=param;
			return;
		case KMODE_SETFONT:
			stack[stack_pos].mode=RD_FONT;
			return;
		case KMODE_DEFFONT:
			if (stack[stack_pos].mode == RD_FONT) {
				this_font=param;
				return;
			}
			else {
				get_fontdef(param);
			}
			return;
		case KMODE_CHARSET:
			if (stack[stack_pos].mode == RD_FONT) add_fontdef(this_font,param);
			return;
		case KMODE_SKIPDEST:
			stack[stack_pos].mode=RD_SKIP;
			return;
		case KMODE_FOOTNOTE:
			if (stack[stack_pos].mode) {
				stack[stack_pos].mode=RD_SKIP;
			}
			else {
				if (footNoteMode == FOOT_IGNORE) {
					stack[stack_pos].mode=RD_SKIP;
				}
				else if (footNoteMode == FOOT_INAUDIO) {
					char buf[64];
					sprintf(buf," %s: ",footString);
					emitStr(buf,-1);
				}
				else {
					newFootnote();
					stack[stack_pos].mode=RD_FOOT;
				}
			}
			return;
		case KMODE_NEWPAGE:
			parseChar('\n');
			parseChar('\n');
			return;
		default:
			return;
	}
	
}

static void push_stack()
{
	if (stack_pos >=STACK_SIZE-1) fail("stack overflow");
	stack[stack_pos+1]=stack[stack_pos];
	stack_pos++;
}

static void pull_stack()
{
	if (!stack_pos) fail("stack underflow");
	stack_pos--;
}

static void parseFileString(char *str,int len)
{
	int z;
	global_str=str;
	global_len=len;
	while((z=get_chara())!=EOF) {
		switch(z) {
			case '{': push_stack();continue;
			case '}': pull_stack();continue;
			case '\\': parseKeyword();continue;
			case 0x0a:
			case 0x0d: continue;
		}
		parseChar(z);
	}
}

char *read_rtf(char *str,int len)
{
	txt_size=65536;
	txt_memo=malloc(txt_size);
	parseFileString(str,len);
	if (nline) add_char('\n');
	if (footNotes) flushFootnotes();
	free_fontdefs();
	txt_memo[txt_pos]=0;
	return txt_memo;
	
}
