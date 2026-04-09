/*
 * mod_mbrola.c - Mbrola output module for Milena TTS system
 * Copyright (C) Bohdan R. Rau 2008 <ethanak@polip.com>
 * 
 * Milena is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * Milena is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with Milena.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "milena_strtod.c"
#include "milena_mbrola.h"
#include "milena_inton.h"

#ifdef __WIN32
#include <windows.h>
#define perror my_perror
#include "my_stdio.h"
#endif

#define MBROFG_SEPARATOR 1
#define MBROFG_ADDLEN 2
#define MBROFG_SUBLEN 4
#define MBROFG_IAFTER 8
#define MBROFG_DUPNEXT 0x10
#define MBROFG_AFTERN 0x20
#define MBROFG_AFTERNG 0x40
#define MBROFG_PALATIZE 0x80
#define MBROFG_JBEFORE 0x100
#define MBROFG_SBEFORE 0x200
#define MBROFG_FBEFORE 0x400
#define MBROFG_SIBEFORE 0x800
#define MBROFG_SHBEFORE 0x1000
#define MBROFG_YBEFORE 0x2000
#define MBROFG_ZBEFORE 0x4000
#define MBROFG_ZHBEFORE 0x8000
#define MBROFG_HBEFORE 0x10000

struct mbrophon {
#ifdef DYNAMIC_INTONATOR
	struct mbrophon *next;
#endif
	char *milena;
	char *sampa;
	int length;
	int vowel;
#ifdef DYNAMIC_INTONATOR
	struct mbrorule *rules;
	int milen;
#else
	int nrules;
	long begrule;
#endif
};

static struct mbrophon PHO_PAUSE={
#ifdef DYNAMIC_INTONATOR
	NULL,"_","_",1,0,NULL,1
#else
        "_","_",1,0,0,0
#endif
        };
	

struct mbrorule {
#ifdef DYNAMIC_INTONATOR
	struct mbrorule *next;
#endif
	char *before;
	char *after;
	long flags;
	long length;
};



struct equalphone {
	struct equalphone *next;
	char *pho;
	char *result;
	int milen;
	struct mbrophon *mb;
};

struct mbrola_modif {
	double rate;
	double pitch;
};

struct milena_mbrola_cfg {
#ifdef DYNAMIC_INTONATOR
	struct mbrophon *mb;
	struct equalphone *eq;
#else
        struct mbrophon *mb1,*mb2;
        int mb1_length, mb2_length;
#endif
	struct intonator *inton;
#ifdef DYNAMIC_INTONATOR
	char *fname;
	int input_line;
#endif
	int aflen[16];
	int breaks[6];
	struct mbrola_modif modf[10];
	int palatizer_len;
	int separator_len;
	int stress_len;
	int geminator_len;
	int minlast_len;
	int iafter_len;
	int jbefore_len;
	int sbefore_len;
	int fbefore_len;
	int hbefore_len;
	int equcomp;
	int equmin;
	float equmpx;
	int flags;
#ifdef DYNAMIC_INTONATOR
	struct memblock *mblock;
	int mblock_start,mblock_end;
#endif
};

#ifndef DYNAMIC_INTONATOR
#include "static_mbrola.c"

#else

#define MBLOCK_SIZE 8176

struct memblock {
	struct memblock *next;
	char memo[1];
};

static void AllocMemBlock(struct milena_mbrola_cfg *cfg)
{
	struct memblock *mbl=malloc(MBLOCK_SIZE);
	mbl->next=cfg->mblock;
	cfg->mblock=mbl;
	cfg->mblock_start=0;
	cfg->mblock_end=MBLOCK_SIZE-sizeof(struct memblock *);
}

static void *mAllocMem(struct milena_mbrola_cfg *cfg,size_t len)
{
	void *v;
	len=(len + 15) & 0xfff0;
	if (cfg->mblock_end - cfg->mblock_start <len) AllocMemBlock(cfg);
	v=(void *)(cfg->mblock->memo+cfg->mblock_start);
	cfg->mblock_start+=len;
	return v;
}

static char *mStrDup(struct milena_mbrola_cfg *cfg,char *str)
{
	int len=strlen(str)+1;
	char *c;
	if (cfg->mblock_end - cfg->mblock_start <len) AllocMemBlock(cfg);
	cfg->mblock_end-=len;
	c=cfg->mblock->memo+cfg->mblock_end;
	strcpy(c,str);
	return c;
}
#endif

void send_outbuf(struct phone_buffer *phb,char *str)
{
	int l=strlen(str);
	if (!phb->buf_len) {
		phb->str=malloc(1024);
		phb->str_len=0;
		phb->buf_len=1024;
	}
	if (!l) return;
	while (phb->str_len+l+1 > phb->buf_len) {
		phb->buf_len*=2;
		phb->str=realloc(phb->str,phb->buf_len);
	}
	strcpy(phb->str+phb->str_len,str);
	phb->str_len+=l;
}

#ifdef DYNAMIC_INTONATOR

#define malloc(a) mAllocMem(cfg,a)
#define strdup(a) mStrDup(cfg,a)

static void rtrim(char *c)
{
	char *d;
	for (d=c;*c;c++) if (!isspace(*c)) d=c+1;
	*d=0;
}

static int m_gerror(struct milena_mbrola_cfg *cfg,char *s)
{
#ifdef __WIN32
	char buf[512];
	sprintf(buf,"%s:\r\n%s w linii %d",cfg->fname,s,cfg->input_line);
	MessageBox(NULL,buf,NULL,MB_OK | MB_ICONERROR);
#else
	fprintf(stderr,"%s: %s w linii %d\n",cfg->fname,s,cfg->input_line);
#endif
	return -1;
}

static char *get_line(struct milena_mbrola_cfg *cfg,FILE *f,char *input_buf)
{
	char *c,*input_text;
	for (;;) {
		if (!fgets(input_buf,256,f)) return 0;
		input_text=input_buf;
		cfg->input_line++;
		while (*input_text && isspace(*input_text)) input_text++;
		c=strchr(input_text,';');
		if (c) *c=0;
		if (*input_text) break;
	}
	rtrim(input_text);
	return input_text;
}

#define gerror(s) {return m_gerror(cfg,s);}
#define r_gerror(s) {m_gerror(cfg,s);was_error=1;return 0;}
#endif


#include "milena_rintoni.c"
#include "milena_intoni.c"

#ifdef DYNAMIC_INTONATOR

static int read_line(struct milena_mbrola_cfg *cfg,FILE *f,int extra)
{
	char *cmd;
	char *input_text,input_buf[256];
	struct mbrophon *this_phone;
	struct mbrorule **last_rule;
	

	
	void insert_mbrophon(void)
	{
		struct mbrophon **ps;
		int l,n;
		l=this_phone->milen=strlen(this_phone->milena);
		for (ps=&cfg->mb;*ps;ps=&(*ps)->next) {
			n=(*ps)->milen-l;
			if (!n) n=strcmp(this_phone->milena,(*ps)->milena);
			if (n<=0) break;
		}
		this_phone->next=*ps;
		*ps=this_phone;
	}

	void insert_equalphon(struct equalphone *ep)
	{
		struct equalphone **epp;
		int l;
		l=ep->milen=strlen(ep->pho);
		for (epp=&cfg->eq;*epp;epp=&(*epp)->next) {
			int n=(*epp)->milen-ep->milen;
			if (!n) n=strcmp(ep->pho,(*epp)->pho);
			if (n<=0) break;
		}
		ep->next=*epp;
		*epp=ep;
	}	

	void next_word(void)
	{
		while (*input_text && !isspace(*input_text)) input_text++;
		if (*input_text) {
			*input_text++=0;
			while (*input_text && isspace(*input_text)) input_text++;
		}
	}
	
	
	if (!(input_text=get_line(cfg,f,input_buf))) return 0;
	if (!*input_text) return 1;
	cmd=input_text;
	next_word();
	if (!extra && !strcmp(cmd,"equal")) {
		char *pho,*res;
		struct equalphone *ep;
		pho=input_text;if (!*pho) gerror("Blad skladni");
		next_word();
		res=input_text;if (!*res) gerror("Blad skladni");
		next_word();
		if (*input_text) gerror("Blad skladni");
		for (ep=cfg->eq;ep;ep=ep->next) if (!strcmp(ep->pho,pho)) gerror("Equal: powielony fonem");
		ep=malloc(sizeof(*ep));
		ep->pho=strdup(pho);
		ep->result=strdup(res);
		insert_equalphon(ep);
		return 1;
	}
	if (!strcmp(cmd,"length")) {
		char *lenname,*c;
		int len;
		lenname=input_text;
		if (!*lenname) gerror("Blad skladni");
		next_word();
		c=input_text;
		if (!*c || !isdigit(*c)) gerror("Blad skladni");
		len=strtol(c,&c,10);
		while (*c && isspace(*c)) c++;
		if (*c) gerror("Blad skladni");
		if (len<1 || len>200) gerror("Wartosc spoza zakresu 1..200");
		if (!strcmp(lenname,"palatizer")) cfg->palatizer_len=len;
		else if (!strcmp(lenname,"separator")) cfg->separator_len=len;
		else if (!strcmp(lenname,"stress")) cfg->stress_len=len;
		else if (!strcmp(lenname,"geminator")) cfg->geminator_len=len;
		else if (!strcmp(lenname,"minlast")) cfg->minlast_len=len;
		else if (!strcmp(lenname,"extrai")) cfg->iafter_len=len;
		else if (!strcmp(lenname,"extraj")) cfg->jbefore_len=len;
		else if (!strcmp(lenname,"extras")) cfg->sbefore_len=len;
		else if (!strcmp(lenname,"extraf")) cfg->fbefore_len=len;
		else if (!strcmp(lenname,"extrah")) cfg->hbefore_len=len;
		else gerror("Bledna nazwa dlugosci");
		return 1;
	}
	if (!strcmp(cmd,"pause")) {
		int dialog=0;
		char *name;
		int i;
		static char *names[8]={"statement","comma","question","exclamation","colon","ellipsis","conj","break"};
		name=input_text;
		if (!*name) gerror("Blad skladni");
		next_word();
		if (!strcmp(name,"dialog")) {
			dialog=8;
			name=input_text;
			if (!*name) gerror("Blad skladni");
			next_word();
		}
		if (!*input_text || !isdigit(*input_text)) gerror("Oczekiwano liczby");
		for (i=0;i<8;i++) if (!strcmp(names[i],name)) break;
		if (i>=8 || (i==6 && dialog)) gerror("Bledna pauza");
		cfg->aflen[i+dialog]=strtol(input_text,&input_text,10);
		return 1;
	}
	if (!strcmp(cmd,"break")) {
		char *name;
		int i;
		static char *brks[]={"normal","dialog","predial","postdial","long"};
		name=input_text;
		if (!*name) gerror("Blad skladni");
		next_word();
		if (!*input_text || !isdigit(*input_text)) gerror("Oczekiwano liczby");
		for (i=0;i<5;i++) if (!strcmp(brks[i],name)) break;
		if (i>=6) gerror("Bledna pauza");
		cfg->breaks[i+1]=strtol(input_text,&input_text,10);
		return 1;
	}
	if (!strcmp(cmd,"modify")) {
		int nm,i;
		double d;
		if (!*input_text || !isdigit(*input_text)) gerror("Oczekiwano liczby");
		nm=strtol(input_text,&input_text,10);
		if (nm<1 || nm>9) gerror("Modyfikator spoza zakresu 1-9");
		for (i=0;i<2;i++) {
			if (!*input_text || !isspace(*input_text)) gerror("Blad skladni");
			while(*input_text && isspace(*input_text)) input_text++;
			if (!*input_text || !isdigit(*input_text)) gerror("Oczekiwano liczby");
			d=my_strtod(input_text,&input_text);
			if (d<0.5 || d>2.0) gerror("Wartosc spoza zakresu 0.5-2.0");
			if (i) cfg->modf[nm].pitch=d;
			else cfg->modf[nm].rate=1/d;
		}
		return 1;
	}
	if (!strcmp(cmd,"equ")) {
		char *c=input_text;
		int n;
		if (!*c) gerror("Pusty equ");
		next_word();
		if (!*input_text) gerror("Oczekiwano liczby");
		if (!strcmp(c,"comp") || !strcmp(c,"min")) {
			n=strtol(input_text,&input_text,10);
			if (n<1) gerror("Bledna liczba");
			if (*c=='c') cfg->equcomp=n;
			else cfg->equmin=n;
			return 1;
		}
		if (!strcmp(c,"mpx")) {
			cfg->equmpx=my_strtod(input_text,&input_text);
			if (cfg->equmpx<0.01 || cfg->equmpx>1.01) gerror("Wartosc spoza zakresu");
			return 1;
		}
		gerror("Nieznany equ");
	}
	if (!extra && !strcmp(cmd,"code")) {
		char *ph1,*ph2,*c;
		int deflen,vowel;
		ph1=input_text;
		if (!*ph1) gerror("Blad skladni");
		next_word();
		ph2=input_text;
		next_word();
		if (!*ph2) gerror("Blad skladni");
		vowel=0;
		if ((c=strstr(ph2,"/v"))) {
			vowel=1;
			*c=0;
		}
		if (!*input_text || !isdigit(*input_text)) gerror("Blad skladni");
		deflen=strtol(input_text,&input_text,10);
		if (*input_text) gerror("Blad skladni");
		this_phone=malloc(sizeof(*this_phone));
		this_phone->milena=strdup(ph1);
		this_phone->sampa=strdup(ph2);
		this_phone->length=deflen;
		this_phone->vowel=vowel;
		this_phone->rules=NULL;
		last_rule=&this_phone->rules;
		insert_mbrophon();
		for (;;) {
			int xlen,flags;
			struct mbrorule *mr;
			char *befo,*aft;
			if (!(input_text=get_line(cfg,f,input_buf))) gerror("Blad skladni na koncu pliku");
			if (*input_text=='\\') break;
			cmd=input_text;
			next_word();
			xlen=0;
			flags=0;
			if (!*input_text) gerror("Pusta regula");
			while (*input_text) {
				if (*input_text == '#') {
					flags=0;
					break;
				}
				if (isspace(*input_text)) {
					input_text++;
					continue;
				}
				if (*input_text == '_') {
					flags |= MBROFG_SEPARATOR;
					input_text++;
					continue;
				}
				if (*input_text == '+') {
					flags |= MBROFG_ADDLEN;
					input_text++;
					continue;
				}
				if (*input_text == '-') {
					flags |= MBROFG_SUBLEN;
					input_text++;
					continue;
				}
				if (*input_text == '=') {
					flags |= MBROFG_IAFTER;
					input_text++;
					continue;
				}
				if (*input_text == 'j') {
					flags |= MBROFG_JBEFORE;
					input_text++;
					continue;
				}
				if (*input_text == 's') {
					flags |= MBROFG_SBEFORE;
					input_text++;
					continue;
				}
				if (*input_text == '<') {
					flags |= MBROFG_YBEFORE;
					input_text++;
					continue;
				}
				if (*input_text == 'S') {
					flags |= MBROFG_SIBEFORE;
					input_text++;
					continue;
				}
				if (*input_text == 'H') {
					flags |= MBROFG_SHBEFORE;
					input_text++;
					continue;
				}
				if (*input_text == 'h') {
					flags |= MBROFG_HBEFORE;
					input_text++;
					continue;
				}
				if (*input_text == 'f') {
					flags |= MBROFG_FBEFORE;
					input_text++;
					continue;
				}
				if (*input_text == 'z') {
					flags |= MBROFG_ZBEFORE;
					input_text++;
					continue;
				}
				if (*input_text == 'Z') {
					flags |= MBROFG_ZHBEFORE;
					input_text++;
					continue;
				}
				
				if (*input_text == '!') {
					flags |= MBROFG_DUPNEXT;
					input_text++;
					continue;
				}
				if (*input_text == 'n') {
					flags |= MBROFG_AFTERN;
					input_text++;
					continue;
				}
				if (*input_text == 'N') {
					flags |= MBROFG_AFTERNG;
					input_text++;
					continue;
				}
				if (*input_text == 'J') {
					flags |= MBROFG_PALATIZE;
					input_text++;
					continue;
				}
				if (isdigit(*input_text)) {
					xlen=strtol(input_text,&input_text,10);
					continue;
				}
				gerror("Blad skladni");
			}
			mr=malloc(sizeof(*mr));
			mr->next=0;
			*last_rule=mr;
			last_rule=&mr->next;
			aft=strchr(cmd,':');
			if (!aft) {
				befo=NULL;
				aft=cmd;
			}
			else {
				befo=cmd;
				*aft++=0;
				if (!*aft) aft=NULL;
			}
			if (!befo && !aft) gerror("Brak poprzednika i nastepnika");
			mr->before=(befo)?strdup(befo):NULL;
			mr->after=(aft)?strdup(aft):NULL;
			mr->flags=flags;
			mr->length=xlen;
		}
		return 1;
	}
	gerror("Nieznane polecenie");
	return 0;
}

int milena_ModMbrolaExtras(struct milena_mbrola_cfg *cfg,char *fname)
{
	FILE *f=fopen(fname,"rb");
	int n;
	if (!f) {
		perror(fname);
		return 0;
	}
	cfg->input_line=0;
	cfg->fname=fname;
	for (;;) {
		n=read_line(cfg,f,1);
		if (n <=0) break;
	}
	fclose(f);
	if (n<0) return 0;
	return 1;
}
#else
int milena_ModMbrolaExtras(struct milena_mbrola_cfg *cfg,char *fname)
{
        return 1;
}

static struct milena_mbrola_cfg static_mbrola_cfg = {
        
        static_mbrophon_1, static_mbrophon_2,//struct mbrophon *mb1,*mb2;
        PHONEMES_1_COUNT, PHONEMES_2_COUNT,//int mb1_length, mb2_length;
	&intonator_static,//struct intonator *inton;
	{100,20,180,100,50,220,15,220,
         150,40,80,80,50,180,15,50},//int aflen[16];
	{0,400,450,600,650,1000},//int breaks[6];
	{{1.0,1.0},{1.0,1.3},{0.8,0.6},{1.0,1.0},{1.0,1.0},
         {1.0,1.0},{1.0,1.0},{1.0,1.0},{1.0,1.0},{1.0,1.0}},       //struct mbrola_modif modf[10];
	5, //int palatizer_len;
	2, //int separator_len;
	5, //int stress_len;
	5, //int geminator_len;
	100, //int minlast_len;
	25, //int iafter_len;
	1, //int jbefore_len;
	4, //int sbefore_len;
	1, //int fbefore_len;
	5, //int hbefore_len;
	20, //int equcomp;
	50, //int equmin;
	0.2, //float equmpx;
	0 //int flags;
};

        
#endif

struct milena_mbrola_cfg *milena_InitModMbrola(char *fname)
{
	struct milena_mbrola_cfg *cfg;
#ifdef DYNAMIC_INTONATOR

	int i;
	FILE *f;
	struct mbrophon *mb;
	struct equalphone *eq;
	static int pre_aflen[16]={
		120,40,120,120,130,40,20,120,
		70,50,50,80,80,40,0,120};
	static int pre_break[]={0,300,600,500,500,900};
	f=fopen(fname,"rb");
	if (!f) {
		perror(fname);
		return NULL;
	}
	
	cfg=calloc(1,sizeof(*cfg));
	for (i=0;i<16;i++) cfg->aflen[i]=pre_aflen[i];
	for (i=0;i<6;i++) cfg->breaks[i]=pre_break[i];
	for (i=0;i<10;i++) cfg->modf[i].rate=cfg->modf[i].pitch=1.0;
	cfg->palatizer_len=5;
	cfg->separator_len=2;
	cfg->stress_len=30;
	cfg->geminator_len=5;
	cfg->iafter_len=15;
	cfg->jbefore_len=5;
	cfg->sbefore_len=5;
	cfg->fbefore_len=5;
	cfg->hbefore_len=5;
	cfg->minlast_len=100;
	AllocMemBlock(cfg);
	cfg->input_line=0;
	cfg->fname=fname;
	for (;;) {
		i=read_line(cfg,f,0);
		if (i<=0) break;
	}
	fclose(f);
	if (i<0) {
		milena_CloseModMbrola(cfg);
		return NULL;
	}
	for (eq=cfg->eq;eq;eq=eq->next) {
		for(mb=cfg->mb;mb;mb=mb->next) {
			if (!strcmp(eq->result,mb->milena)) break;
		}
		if (!mb) {
			char buf[512];
			sprintf(buf,"Equal %s jest nieprawidlowy",eq->pho);
			m_gerror(cfg,buf);
			milena_CloseModMbrola(cfg);
			return NULL;
		}
		eq->mb=mb;
	}
	if (!milena_InitInton(cfg)) {
		milena_CloseModMbrola(cfg);
		return NULL;
	}
#else
        cfg=malloc(sizeof(*cfg));
        memcpy(cfg,&static_mbrola_cfg,sizeof(*cfg));
#endif
	return cfg;
}

void milena_ModMbrolaSetFlag(struct milena_mbrola_cfg *cfg,int flag,int mask)
{
	cfg->flags=(cfg->flags & ~mask) | (flag & mask);
}


void milena_ModMbrolaSetVoice(struct milena_mbrola_cfg *cfg,int param,...)
{
	va_list ap;
	double dpar;
	int ipar;
	
	va_start(ap,param);
	switch(param) {
		case MILENA_MBP_PITCH:
		
		dpar=va_arg(ap,double);
#ifdef DYNAMIC_INTONATOR
		cfg->inton->vp.base_pitch=dpar;
#endif
		break;

		case MILENA_MBP_RANGE:
		
		dpar=va_arg(ap,double);
#ifdef DYNAMIC_INTONATOR
		cfg->inton->vp.range=dpar;
#endif
		break;
		
		case MILENA_MBP_STRESLEN:
		
		ipar=va_arg(ap,int);
		if (ipar<0) ipar=0;
		else if (ipar>50) ipar=50;
		cfg->stress_len=ipar;
		break;
	}
	va_end(ap);
}

#ifndef DYNAMIC_INTONATOR
static struct mbrophon *find_phonem(struct milena_mbrola_cfg *cfg,char **str)
{
        int i;
        for (i=0;i<cfg->mb2_length;i++) {
                if (!strncmp(*str,cfg->mb2[i].milena,2)) {
                        (*str) += 2;
                        return &cfg->mb2[i];
                }
        }
        for (i=0;i<cfg->mb1_length;i++) {
                if (**str == * cfg->mb1[i].milena) {
                        (*str) += 1;
                        return &cfg->mb1[i];
                }
        }
        return NULL;
}
#endif


static void print_mbrophon(struct mbrophon *mb)
{
        fprintf(stderr,"<%s> => <%s>, %s, %d\n",
        mb->milena, mb->sampa, mb->vowel?"vovel":"consonant",mb->length);
}

static void print_mbrule(struct mbrorule *mr)
{
        fprintf(stderr,"rule %s:%s, len %ld, flags %s\n",
                mr->before?mr->before:"",
                mr->after?mr->after:"",
                mr->length,
                (mr->flags & MBROFG_ADDLEN)?"addlen":
                (mr->flags & MBROFG_SUBLEN)?"sublen":"eqlen");
}

static struct mbrophon *get_phonem(struct milena_mbrola_cfg *cfg,
	char *str,char **ostr,int *stres,int *modf,int *sep,int *bsyl)
{
	int srs=0;
	int mdf=0;
	int se=0;
	int begsyl=0;
	for (;*str;str++) {
		if (isspace(*str)) {
			begsyl=1;
			continue;
		}
		if (isspace(*str) || *str=='~') continue;
		if (*str==',') {srs=1;continue;}
		if (*str=='!') {srs=2;continue;}
		if (*str=='?') {srs=3;continue;}
		if (*str=='{') {
			begsyl=1;
			while (*str && *str!='}') {
				if (*str>='0' && *str<='9') mdf=*str;
				str++;
			}
			if (*str) continue;
			break;
		}
		if (*str=='[') {
			begsyl=1;
			/* tu flagi do str */
			while (*str && *str!=']') {
				if (*str=='s') se=1;
				if (*str=='b') se=2;
				str++;
			}
			if (*str) continue;
		}
		break;
	}
	if (*str) {
		struct mbrophon *mb=NULL;
#ifdef DYNAMIC_INTONATOR
		struct equalphone *eq;
		for (eq=cfg->eq;eq;eq=eq->next) {
			char *c=str,*d=eq->pho;
			for (;*d && *c;c++,d++) if (*c != *d) break;
			if (!*d) {
				str=c;
				mb=eq->mb;
				break;
			}
		}
		if (!mb) for (mb=cfg->mb;mb;mb=mb->next) {
			char *c=str,*d=mb->milena;
			for (;*d && *c;c++,d++) if (*c != *d) break;
			if (!*d) {
				str=c;
				break;
			}
		}
#else
                //fprintf(stderr,"STR=<%s>\n",str);
                mb=find_phonem(cfg,&str);
#endif
		if (mb) {
                        //print_mbrophon(mb);
                        //fprintf(stderr,"STR AFTER=<%s>\n",str);
			if (ostr) *ostr=str;
			if (stres) *stres=srs;
			if (modf) *modf=mdf;
			if (sep && se) *sep=se;
			if (bsyl) *bsyl=begsyl;
			return mb;
		}

	}
	if (ostr) *ostr=str;
	return NULL;			
}

static int extra_phonem(struct milena_mbrola_cfg *cfg,char *ph,struct mbrophon *pho1,struct mbrophon *pho2,FILE *f,struct phone_buffer *phb)
{
	int phlen,xflags;
	struct mbrophon *pho;
	struct mbrorule *rule;

#ifdef DYNAMIC_INTONATOR
	for (pho=cfg->mb;pho;pho=pho->next) if (!strcmp(ph,pho->sampa)) break;
#else
        char **c=&ph;
        pho=find_phonem(cfg,c);
#endif
	if (!pho) {
		fprintf(stderr,"Mbrola: niezdefiniowany fonem '%s'\n",ph);
		return 0;
	}
	if (pho->vowel) {
		fprintf(stderr,"Mbrola: ekstra fonem '%s' to samogloska\n",ph);
		return 0;
	}
	phlen=pho->length;
	xflags=0;
#ifdef DYNAMIC_INTONATOR
	for (rule=pho->rules;rule;rule=rule->next) {
#else
        int rule_counter;
        for (rule_counter=0;rule_counter < pho-> nrules;rule_counter++) {
                rule=&static_mbrorules[pho->begrule + rule_counter];
#endif
		if (rule->before) {
			if (!strcmp(rule->before,"^")) continue;
			if (!strcmp(rule->before,"@")) {
				if (!pho2->vowel) continue;
			}
			else if (strcmp(pho2->milena,rule->before)) continue;
		}
		if (!rule->after) break;
		if (!strcmp(rule->after,"*")) break;
		if (!strcmp(rule->after,"$")) {
			if (pho1) break;
			continue;
		}
		if (!pho1) continue;
		if (!strcmp(rule->after,"@")) {
			if (pho1->vowel) break;
			continue;
		}
		if (!strcmp(pho1->milena,rule->after)) break;
	}
#ifdef DYNAMIC_INTONATOR
	if (rule) {
#else
        if (rule_counter < pho-> nrules) {
#endif
		xflags=rule->flags;
		if(rule->length) {
			if (xflags & MBROFG_ADDLEN) phlen+=rule->length;
			else if (xflags & MBROFG_SUBLEN) phlen-=rule->length;
			else phlen=rule->length;
		}
	}
	if (!pho1 && phlen<cfg->minlast_len) phlen=cfg->minlast_len;
	if (phb) {
		char buf[32];
		sprintf(buf,"%s %d\n",pho->sampa,phlen);
		send_outbuf(phb,buf);
	}
	if (f) fprintf(f,"%s %d\n",pho->sampa,phlen);
	return phlen;
}


static void phb42(struct phone_buffer *phb, FILE *f,int n,double d)
{
	char buf[32];
	int t=100*d;
	char *c="";
	if (t<0) {
		t=-t;
		c="-";
	}
	if (phb) {
		sprintf(buf," %d %s%d.%02d",n,c,t/100,t%100);
		send_outbuf(phb,buf);
	}
	else {
		fprintf(f," %d %s%d.%02d",n,c,t/100,t%100);
	}
}
static void _milena_ModMbrolaGenPhrase(struct milena_mbrola_cfg *cfg,
	char *bufor,FILE *f,struct phone_buffer *phb,int pmode)

{
	struct syl_env *sylenv;
	char *str;
	char sxbuf[256];
	static int smodes[]={0,STRESS_SECONDARY,STRESS_PRIMARY,STRESS_PRIMARY_MARKED};
	
	int stres,syls,this_syl,next_dup,dup_vowel,modifier,mdf,sep,pos,i,brk;
	struct mbrophon *pho,*pho1,*pho2;
	int no_phones,in_syl,pass;
	int *phonelist=NULL;
	int same_vowel,last_vowel_len,last_sep_len;
	int vnmode,mkflag;
	
	syls=0;str=bufor;
	dup_vowel=-1;
	no_phones=0;
	while((pho=get_phonem(cfg,str,&str,&stres,NULL,NULL,NULL))) {
		no_phones++;
		if (pho->vowel) syls++;
	}
	if (!syls) return;
	if (cfg->flags & MILENA_MBFL_EQUALIZE) {
		phonelist=calloc(no_phones,sizeof(*phonelist));
		str=bufor;i=0;
		while((pho=get_phonem(cfg,str,&str,&stres,NULL,NULL,&brk))) {
			if (pho->vowel) phonelist[i] |= 1;
			if (brk) phonelist[i] |= 2;
			i++;
		}
		in_syl=0; /* przed samogloska */
		for (i=0;i<no_phones;i++) {
			if (phonelist[i] & 2) {
				in_syl = phonelist[i] & 1;
				continue;
			}
			if (phonelist[i] & 1) {
				if (!in_syl) in_syl=1;
				else phonelist[i]|=2;
				continue;
			}
			if (!in_syl) continue;
			if (i<no_phones-1) {
				if (phonelist[i+1] & 2) continue;
				if (phonelist[i+1] & 1) {
					phonelist[i] |= 2;
					in_syl=0;
				}
			}
		}
	}
	if (syls==1 && ((pmode & 7) != 7)) {
		dup_vowel=0;
		//syls++;
	}
	//fprintf(stderr,"PM %d DV %d\n",pmode & 7,dup_vowel);
	sylenv=calloc((syls+4) , sizeof(*sylenv));
	syls=0;str=bufor;
	sep=0;
	while ((pho=get_phonem(cfg,str,&str,&stres,NULL,&sep,NULL))) if (pho->vowel) {
		if (stres == 3) {
			stres=2;
			sylenv[syls].marked|=1;
		}
		if (sep) {
			sylenv[syls].marked|=(sep==1)?2:4;
			sep=0;
		}
		//fprintf(stderr,"Stress %d\n",stres);
		sylenv[syls++].stress=smodes[stres];
	}
	if ((pmode & 7) == 7) { /* ellipsis - urwanie*/
		dup_vowel=-1;
		sylenv[syls].stress=smodes[2];
		sylenv[syls+1].stress=0;
		syls+=2;
	}
	else if (dup_vowel<0 && sylenv[syls-1].stress) {
		sylenv[syls-2].stress=0;
		dup_vowel=syls-1;
	}
	//fprintf(stderr,"DV %d\n",dup_vowel);
	if (dup_vowel>=0) {
		sylenv[dup_vowel].stress=smodes[2];
		sylenv[syls++].stress=0;
	}
	else {
		int i;
		/* ostatni akcent jest zawsze podstawowy */
		for (i=syls-1;i>=0;i--) if (sylenv[i].stress) {
			sylenv[i].stress=smodes[2];
			break;
		}
	}
	sylenv[syls].stress=0;
	pos=0;
	//fprintf(stderr,"%d\n",pmode & 7);
	// mkflag: dla normalnego tylko supersep, dla lektora równie¿ sep
	// dif: dla lektora 4, normalnie 1
	mkflag=4;
	if (cfg -> flags & MILENA_MBFL_BOOKMODE) mkflag=6;
	while(pos<syls) {
		int i,lst;
		for (i=pos,lst=0;i<syls;i++) {
			if (sylenv[i].stress) lst=i;
			if (sylenv[i].marked & mkflag) {
				int mdif=(sylenv[i].marked & 4)?1:4;
				if (i<pos+mdif || i>syls-mdif) {
					if (!pos || i !=pos) sylenv[i].marked &= ~6;
					continue;
				}
				break;
			}
		}
		if (i<syls) {
			int nmode=1,j;
			for (j=i;j<syls;j++) {
				if (j<i+1 || j>syls-1) continue;
				if (sylenv[j].marked & 4) break;
				if (j<i+4 || j>syls-4) continue;
				if (sylenv[j].marked & 2) break;
			}
			if (j == syls && i>= syls-5 && ((pmode & 7)==1 || (pmode & 7) ==2)) nmode=5;
			//fprintf(stderr,"%d %d %d %d %d\n",pos,i,j,syls,nmode);
			if (lst) sylenv[lst].stress=smodes[2];
			compute_pitches(cfg->inton,sylenv+pos,i-pos,nmode);
			pos=i;
		}
		else break;
	}
	//fprintf(stderr,"%d < %d %d\n",pos,syls,pmode & 7);
	if (!(pmode & 128)) {
		vnmode = pmode & 7;
		if (vnmode == 7) vnmode=0;
	}
	else {
		vnmode = pmode & 7;
		if (vnmode == 7) vnmode=0;
		else if (vnmode == 1) vnmode=5;
	}
	//fprintf(stderr,"%x %x\n",pmode,vnmode);
	if (pos<syls) compute_pitches(cfg->inton,sylenv+pos,syls-pos,vnmode);
	for (pass=(phonelist)?0:1;pass<2;pass++) {
		int this_pho=0,pho_syl=0,tempo_dup=dup_vowel;
		char *nstr;
		same_vowel=0;
		last_sep_len=0;
		str=bufor;
		this_syl=0;
		next_dup=0;
		pho2=NULL;
		mdf=0;
		for (;(pho=get_phonem(cfg,str,&str,&stres,&modifier,&sep,NULL));this_pho++) {
			int phlen;
			int xflags;
			struct mbrorule *rule;
			int isep;
			
			if (phonelist && this_pho && (phonelist[this_pho] & 2)) {
				pho_syl++;
			}
			phlen=pho->length;
			//fprintf(stderr,"%s->%s\n",pho->milena,pho->sampa);
			if (modifier) mdf=modifier-'0';
			xflags=0;
			//fprintf(stderr,"%d %d %d\n",sep,this_syl,sylenv[this_syl].marked);
			if (sep && (sylenv[this_syl].marked & mkflag)) {
				if (pass && this_syl) {
					if (phb) {
						sprintf(sxbuf,"_ %d\n",cfg->aflen[6]?cfg->aflen[6]:cfg->aflen[1]/2);
						send_outbuf(phb,sxbuf);
					}
					else {
						fprintf(f,"_ %d\n",cfg->aflen[6]?cfg->aflen[6]:cfg->aflen[1]/2);
					}
				}
				sep=0;
			}
			isep=0;
			pho1=get_phonem(cfg,str,&nstr,NULL,NULL,&isep,NULL);
			if (pho1 && isep) {
				int n=this_syl;
				if (pho->vowel) n++;
				if (sylenv[n].marked & mkflag) {
					pho1=&PHO_PAUSE;
				}
			}
                        //fprintf(stderr,"This phone: ");
                        //print_mbrophon(pho);
#ifdef DYNAMIC_INTONATOR
			for (rule=pho->rules;rule;rule=rule->next) {
#else
                        int rule_counter;
                        for (rule_counter=0;rule_counter < pho-> nrules;rule_counter++) {
                                //fprintf(stderr,"Rule %d/%d\n",rule_counter,pho->nrules);
                                rule=&static_mbrorules[pho->begrule + rule_counter];
#endif
				//print_mbrule(rule);
                                if (rule->before) {
					if (!strcmp(rule->before,"^")) {
						if (pho2) continue;
					}
					else {
						if (!pho2) continue;
						if (!strcmp(rule->before,"@")) {
							if (!pho2->vowel) continue;
						}
						else if (strcmp(pho2->milena,rule->before)) continue;
					}
				}
				if (!rule->after || !strcmp(rule->after,"*")) break;
				if (!strcmp(rule->after,"$")) {
					if (!pho1) break;
					if (!pho->vowel) continue;
					if (!pho1->vowel) continue;
					if (same_vowel) continue;
					if (dup_vowel==this_syl) continue;
					if (dup_vowel==this_syl+1) continue;
					if (strcmp(pho1->milena,pho->milena)) continue;
					if (pho->sampa[1] || !strchr("aeoIi",pho1->sampa[0])) continue;
					if (get_phonem(cfg,nstr,NULL,NULL,NULL,NULL,NULL)) continue;
					//if (pho1) continue;
					break;
				}
				if (!pho1) continue;
				if (!strcmp(rule->after,"_")) {
					struct mbrophon *phnext;
					phnext=pho1;
					if (pho->vowel &&
						pho1->vowel &&
						!same_vowel &&
						dup_vowel!=this_syl &&
						dup_vowel!=this_syl+1 &&
						!strcmp(pho1->milena,pho->milena) &&
						!pho->sampa[1] &&
						strchr("aeoIi",pho1->sampa[0])) {
							phnext=get_phonem(cfg,nstr,NULL,NULL,NULL,NULL,NULL);
							if (!phnext) continue;
					}
					if (strcmp(phnext->milena,"_")) continue;
					break;
				}
				if (!strcmp(rule->after,"@")) {
					if (pho1->vowel) break;
					continue;
				}
				
				if (!strcmp(pho1->milena,rule->after)) break;
			}
#ifdef DYNAMIC_INTONATOR
			if (rule) {
#else
                        if (rule_counter < pho-> nrules) {
#endif
                                //fprintf(stderr,"Rule pasuje\n");
				xflags=rule->flags;
				if (rule->length) {
					if (xflags & MBROFG_ADDLEN) phlen+=rule->length;
					else if (xflags & MBROFG_SUBLEN) phlen-=rule->length;
					else phlen=rule->length;
				}
			}
			if (mdf && pass) {
				//fprintf(stderr,"R %f\n",cfg->modf[mdf].rate);
				phlen*=cfg->modf[mdf].rate;
				if (phlen<2) phlen=2;
			}
                        if ((cfg->flags & MILENA_MBFL_MOUTH) && pass) {
                                char *c = pho->milena;
                                if (*c == '*') {
                                        sprintf(sxbuf,";M %c\n",pho->milena[1]);
                                }
                                else if (pho->vowel) {
                                        sprintf(sxbuf,";M %c %d\n",
                                                *pho->milena,
                                                (sylenv[this_syl].stress == 0) ? 0 :
                                                (sylenv[this_syl].stress < STRESS_PRIMARY) ? 1 :
                                                 2);
                                }
                                else {
                                        sprintf(sxbuf,";M %c\n",*pho->milena);
                                }
                                if (phb) {
                                        send_outbuf(phb,sxbuf);
                                }
                                else {
                                        fputs(sxbuf, f);
                                }
                        }
			if (pho->vowel) {
				int j,np;
				
				//if (pass) fprintf(stderr,"SYL LEN %d\n",sylenv[this_syl].syl_len);
				next_dup=0;
				if (xflags & MBROFG_JBEFORE) {
					if (pass) {
						if (phb) {
							sprintf(sxbuf,"j	%d\n",cfg->jbefore_len);
							send_outbuf(phb,sxbuf);
						}
						else {
							fprintf(f,"j	%d\n",cfg->jbefore_len);
						}
					}
					else sylenv[pho_syl].syl_len+=cfg->jbefore_len;
				}
				if (xflags & MBROFG_ZBEFORE) {
					if (pass) {
						if (phb) {
							sprintf(sxbuf,"z	%d\n",cfg->sbefore_len);
							send_outbuf(phb,sxbuf);
						} else {
							fprintf(f,"z	%d\n",cfg->sbefore_len);
						}
					}
					else sylenv[pho_syl].syl_len+=cfg->sbefore_len;
				}
				if (xflags & MBROFG_ZHBEFORE) {
					if (pass) {
						if (phb) {
							sprintf(sxbuf,"Z	%d\n",cfg->sbefore_len);
							send_outbuf(phb,sxbuf);
						} else {
							fprintf(f,"Z	%d\n",cfg->sbefore_len);
						}
					}
					else sylenv[pho_syl].syl_len+=cfg->sbefore_len;
				}
				if (xflags & MBROFG_SBEFORE) {
					if (pass) {
						if (phb) {
							sprintf(sxbuf,"s	%d\n",cfg->sbefore_len);
							send_outbuf(phb,sxbuf);
						} else {
							fprintf(f,"s	%d\n",cfg->sbefore_len);
						}
					}
					else sylenv[pho_syl].syl_len+=cfg->sbefore_len;
				}
				if (xflags & MBROFG_SIBEFORE) {
					if (pass) {
						if (phb) {
							sprintf(sxbuf,"s'	%d\n",cfg->sbefore_len);
							send_outbuf(phb,sxbuf);
						} else {
							fprintf(f,"s'	%d\n",cfg->sbefore_len);
						}
					}
					else sylenv[pho_syl].syl_len+=cfg->sbefore_len;
				}
				if (xflags & MBROFG_SHBEFORE) {
					if (pass) {
						if (phb) {
							sprintf(sxbuf,"S	%d\n",cfg->sbefore_len);
							send_outbuf(phb,sxbuf);
						}
						else {
							fprintf(f,"S	%d\n",cfg->sbefore_len);
						}
					}
					else sylenv[pho_syl].syl_len+=cfg->sbefore_len;
				}
				if (xflags & MBROFG_FBEFORE) {
					if (pass) {
						if (phb) {
							sprintf(sxbuf,"f	%d\n",cfg->fbefore_len);
							send_outbuf(phb,sxbuf);
						} else {
							fprintf(f,"f	%d\n",cfg->fbefore_len);
						}
					}
					else sylenv[pho_syl].syl_len+=cfg->fbefore_len;
				}
				if (stres) {
					phlen+=cfg->stress_len*stres;
				}
				if (dup_vowel == this_syl) {
					switch(pmode & 7) {
						case 2:	phlen=phlen*1.7;break;
						case 0: phlen=phlen*1.1;break;
						default: phlen=phlen*1.4;break;
					}
				}
				else {
					if (!same_vowel && dup_vowel != this_syl+1
						&& pho1 && pho1->vowel
						&& !strcmp(pho1->milena,pho->milena) &&
						!pho->sampa[1] && strchr("aeoIi",pho1->sampa[0])) same_vowel=1;
					else if (same_vowel==1) same_vowel=2;
					if (pass && phonelist) {
						phlen+=cfg->equcomp-sylenv[this_syl].syl_len*cfg->equmpx;
						if (phlen<cfg->equmin) phlen=cfg->equmin;
					}
				}
				if (pass) {
					if (!pho1 && phlen<cfg->minlast_len) phlen=cfg->minlast_len;
					if (same_vowel==1) last_vowel_len=phlen;
					if (same_vowel==2) {
						if (phb) {
							sprintf(sxbuf,"%s %d",pho->sampa,phlen+last_vowel_len);
							send_outbuf(phb,sxbuf);
						} else {
							fprintf(f,"%s %d",pho->sampa,phlen+last_vowel_len);
						}
					}
					else if (same_vowel==0) {
						if (phb) {
							sprintf(sxbuf,"%s %d",pho->sampa,phlen);
							send_outbuf(phb,sxbuf);
						} else {
							fprintf(f,"%s %d",pho->sampa,phlen);
						}
					}
				}
				if (dup_vowel !=this_syl) {
					if (pass) {
						if (same_vowel==2) {
							for(j=0;j<sylenv[this_syl-1].npitch;j++) {
								phb42(phb,f,
									(sylenv[this_syl-1].offsets[j]*4)/10,
									sylenv[this_syl-1].pitches[j]*cfg->modf[mdf].pitch
								);
							}
							for (j=0;j<sylenv[this_syl].npitch;j++) {
								phb42(phb,f,
									(sylenv[this_syl].offsets[j]*4/10)+60,
									sylenv[this_syl].pitches[j]*cfg->modf[mdf].pitch
								);
							}
						}
						else if (!same_vowel) {
							for (j=0;j<sylenv[this_syl].npitch;j++) {
								phb42(phb,f,
									sylenv[this_syl].offsets[j],
									sylenv[this_syl].pitches[j]*cfg->modf[mdf].pitch
								);
							}
						}
					}
				}
				else if (pass) {
					int lof,mx,px;
					np=sylenv[this_syl].npitch;
					if (np>2) np--;
					for (j=0;j<np;j++) {
						phb42(phb,f,
							(sylenv[this_syl].offsets[j]*4)/10,
							sylenv[this_syl].pitches[j]*cfg->modf[mdf].pitch
						);
					}
					lof=(sylenv[this_syl].offsets[np-1]*4)/10;
					this_syl++;
					np=sylenv[this_syl].npitch;
					if (np>2) np=1;else np=0;
					mx=6;px=40;
					if ((sylenv[this_syl].offsets[j]*6/10)+40<=lof) {
						mx=4;
						px=60;
					}
					for (j=np;j<sylenv[this_syl].npitch;j++) {
						phb42(phb,f,
							(sylenv[this_syl].offsets[j]*mx/10)+px,
							sylenv[this_syl].pitches[j]*cfg->modf[mdf].pitch
						);
					}
				}
				if (pass && same_vowel != 1) {
					if (phb) send_outbuf(phb,"\n");
					else fprintf(f,"\n");
				}
				if (same_vowel==2) same_vowel=0;
				this_syl++;
				
			}
			else if (phlen) {
				if (xflags & MBROFG_YBEFORE) {
					if (pass) {
						if (phb) {
							sprintf(sxbuf,"I	%d\n",cfg->iafter_len);
							send_outbuf(phb,sxbuf);
						} else {
							fprintf(f,"I	%d\n",cfg->iafter_len);
						}
					}
					else sylenv[pho_syl].syl_len+=cfg->iafter_len;
				}
				if (xflags & MBROFG_HBEFORE) {
					if (pass) {
						if (phb) {
							sprintf(sxbuf,"x	%d\n",cfg->hbefore_len);
							send_outbuf(phb,sxbuf);
						} else {
							fprintf(f,"x	%d\n",cfg->hbefore_len);
						}
					}
					else sylenv[pho_syl].syl_len+=cfg->iafter_len;
				}
		
				if (next_dup) {
					if (pass) {
						if (phb) {
							sprintf(sxbuf,"%s %d\n",pho->sampa,cfg->geminator_len);
							send_outbuf(phb,sxbuf);
						} else {
							fprintf(f,"%s %d\n",pho->sampa,cfg->geminator_len);
						}
					}
					else sylenv[pho_syl].syl_len+=cfg->geminator_len;
				}
				if (pass) {
					if (phb) {
						sprintf(sxbuf,"%s %d\n",pho->sampa,phlen);
						send_outbuf(phb,sxbuf);
					} else {
						fprintf(f,"%s %d\n",pho->sampa,phlen);
					}
				}
				else sylenv[pho_syl].syl_len+=phlen;
			}
			next_dup=(xflags & MBROFG_DUPNEXT);
			if (xflags & MBROFG_AFTERN) {
				if (pass)extra_phonem(cfg,"n",pho1,pho,f,phb);
			
				else sylenv[pho_syl].syl_len+=extra_phonem(cfg,"n",pho1,pho,NULL,NULL);
			}
			else if (xflags & MBROFG_AFTERNG) {
				if (pass)extra_phonem(cfg,"N",pho1,pho,f,phb);
				else sylenv[pho_syl].syl_len+=extra_phonem(cfg,"N",pho1,pho,NULL,NULL);
			}		
			if (xflags & MBROFG_PALATIZE) {
				if (pass) {
					if (phb) {
						sprintf(sxbuf,"i %d\n",cfg->palatizer_len);
						send_outbuf(phb,sxbuf);
					} else {
						fprintf(f,"i %d\n",cfg->palatizer_len);
					}
				}
				else sylenv[pho_syl].syl_len+=cfg->palatizer_len;
			}
			if (xflags & MBROFG_IAFTER) {
				if (pass) {
					if (phb) {
						sprintf(sxbuf,"I	%d\n",cfg->iafter_len);
						send_outbuf(phb,sxbuf);
					}
					else {
						fprintf(f,"I	%d\n",cfg->iafter_len);
					}
				}
				else sylenv[pho_syl].syl_len+=cfg->iafter_len;
			}
			if (xflags & MBROFG_SEPARATOR) {
				if (pho1 && pass) {
					if (phb) {
						sprintf(sxbuf,"_	%d\n",cfg->separator_len);
						send_outbuf(phb,sxbuf);
					} else {
						fprintf(f,"_	%d\n",cfg->separator_len);
					}
				}
			}
			pho2=pho;
	
		}
	}
	free(sylenv);
	if (phonelist) free(phonelist);
	if (!(pmode & 16)) {
		//fprintf(stderr,"PM=%d,%d\n",pmode & 15,cfg->aflen[pmode & 15]);
		if (phb) {
			sprintf(sxbuf,"_	%d\n",cfg->aflen[pmode & 15]);
			send_outbuf(phb,sxbuf);
		} else {
			fprintf(f,"_	%d\n",cfg->aflen[pmode & 15]);
		}
	}
}

void milena_ModMbrolaGenPhraseP(struct milena_mbrola_cfg *cfg,
	char *bufor,struct phone_buffer *phb,int pmode)
{
	return _milena_ModMbrolaGenPhrase(cfg,bufor,NULL,phb,pmode);
}
void milena_ModMbrolaGenPhrase(struct milena_mbrola_cfg *cfg,
	char *bufor,FILE *f,int pmode)
{
	return _milena_ModMbrolaGenPhrase(cfg,bufor,f,NULL,pmode);
}


void milena_ModMbrolaBreakP(struct milena_mbrola_cfg *cfg,struct phone_buffer *outbuf,int pau)
{
	char buf[32];
	if (pau>0 && pau<6) {
		sprintf(buf,"_ %d\n",cfg->breaks[pau]);
		send_outbuf(outbuf,buf);
	}
}
void milena_ModMbrolaBreak(struct milena_mbrola_cfg *cfg,FILE *f,int pau)
{
	if (pau>0 && pau<6) fprintf(f,"_ %d\n",cfg->breaks[pau]);
}

void milena_CloseModMbrola(struct milena_mbrola_cfg *cfg)
{
	struct memblock *mb;
#ifdef DYNAMIC_INTONATOR
	while (mb=cfg->mblock) {
		cfg->mblock=mb->next;
		free(mb);
	}
#endif
	free(cfg);
}
