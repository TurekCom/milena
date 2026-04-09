/*
 * milena.c - Milena TTS system
 * Copyright (C) Bohdan R. Rau 2008-2011 <ethanak@polip.com>
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
#include <sys/stat.h>

#ifdef __WIN32
#include <stdint.h>
#define u_int64_t uint64_t
#endif

#ifdef HAVE_MORFOLOGIK
#if HAVE_MORFOLOGIK == 20
#include "int_morfologik.c"
#else
#include "minimorf.c"
#endif
#else
#include "def_morfologik.h"
#endif


#include "milena.h"

#ifdef __WIN32
#include <windows.h>
#include <libgen.h>
#define perror my_perror
#include "my_stdio.h"
#endif

#define IS_LETTER 1
#define IS_REDEF 2
#define IS_CLASS 4

#define CLASS_BIT 3

#define pushbuf(n) do {if (pos<buflen-1) outbuf[pos]=(n);pos++;} while (0)
#define pushstr(s) do {char *d=(s);for (;*d;d++) pushbuf(*d);} while(0);

/* dictionary */

#define MILDIC_V 1
#define MILDIC_U 2
#define MILDIC_P 4
#define MILDIC_R 8
#define MILDIC_N 16
#define MILDIC_K 32

//milena_c unused
//#define MILDIC_C 64

#define MILDIC_NU 128
#define MILDIC_L 0x100
// mildic_never - akcentowany na koñcu po co najmniej dwóch nieakcentowanych
#define MILDIC_NEVER 0x200
#define MILDIC_ATEND 0x400
#define MILDIC_SEP 0x800
#define MILDIC_ATQUE 0x1000
#define MILDIC_FG 0x2000
#define MILDIC_UCA 0x4000
#define MILDIC_ATSTART 0x8000
#define MILDIC_BEFORECAP 0x10000
#define MILDIC_BEFOREUNCAP 0x4000000

// vop operator, HIM zaimek B
#define MILDIC_VOP 0x20000
#define MILDIC_HIM 0x40000

#define MILDIC_SPELL 0x100000
#define MILDIC_NUM 0x200000
#define MILDIC_STRESSED 0x400000
#define MILDIC_ESTRESS 0x800000

//tryb rozkazuj±cy
#define MILDIC_VIMPT 0x1000000

//super separator
#define MILDIC_SUPERSEP 0x2000000

//dopuszczalne znaki za tyld±
#define ESCHAR ",!'+"

struct macronim {
	struct macronim *next;
	char *name;
	char *value;
};

struct ruler {
	struct ruler *next;
	int op;
	void *arg;
	void *arg2;
};

struct letter_rule {
	struct letter_rule *next;
	struct ruler *lrule;
	struct ruler *rrule;
	int eats;
	int langs;
	char *pho;
	char *orig;
};

struct milena_dic {
	char *word;
	int flags;
	int stress;
};

struct user_dic {
	char *word;
	int flags;
	int stress;
	int seq;
	char *pron;
};

struct milena_udic {
	struct milena_udic *next;
	char *word;
	char *pron;
	int flags;
	short stress;
	short pstress;
};

struct milena_pseudo {
	struct milena_pseudo *next;
	char *pattern;
	char *result;
};

struct milena_prefix {
	struct milena_prefix *next;
	char *prefix;
	int pstress;
};

struct milena_udic_macro {
	struct milena_udic_macro *next;
	char *name;
	char *value;
};

struct mika_afx {
	struct mika_afx *next;
	char *afx;
};

struct mika_bfx {
	struct mika_bfx *next;
	char *afx;
	int stress;
};

struct mika_sfx {
	struct mika_sfx *next;
	char *afx;
	short stress;
	short pstress;
	int flags;

};

#define MILENA_NFLEX_FEMALE 1
#define MILENA_NFLEX_HIDDEN 2

struct milena_nflex {
	struct milena_nflex *next;
	char *pattern;
	char *result[4];
	int flags;
};

struct milena_spell {
	struct milena_spell *next;
	int znak;
	char *result;
};

struct milena_emot {
	struct milena_emot *next;
	char *emot;
	char *result;
};

struct milena_stringlist {
	struct milena_stringlist *next;
	char *string;
};

struct milena_tld {
	struct milena_tld *next;
	char *string;
	char *pron;
	int flags;
};

struct milena_key {
	struct milena_key *next;
	char *key;
	char *pron;
};

struct milena_char {
	struct milena_char *next;
	int chr;
	char *pron;
};

/* dwie nastêpne - do rest_recognizera */

#define RESTCMD_STR 0
#define RESTCMD_ALT 1
#define RESTCMD_STAR 2
#define RESTCMD_FUN 3
#define RESTCMD_EXPR 4
#define RESTCMD_CHOICE 5
#define RESTCMD_HASH 6
#define RESTCMD_PHRASE 7

struct restcognizer {
	struct restcognizer *next;
	int command;
	union {
		char *str;
		struct {
			struct restcognizer *left;
			struct restcognizer *rite;
		} alt;
		u_int64_t grama;
	} u;
};

struct restcogfun {
	struct restcogfun *next;
	char *name;
	struct restcognizer *body;
};

struct milena_fin {
    struct milena_fin *next;
    char *string;
    int mode;
};

struct recog_number {
    struct recog_number *next;
    short format[3];
    short pred;
    char *preds[4];
    int num;
    int cas;
    short atend;
    short flags;
};


struct recog_expression {
	struct recog_expression *next;
	char *name;
	char *str;
};


struct recog_morfexpr {
    struct recog_morfexpr *next;
    char *name;
    u_int64_t filter;
    char *str;
    struct milena_stringlist *words;
};

struct recog_sayas {
	struct recog_sayas *next;
	char *name;
	char *format;
};

#define RECNU_DBL 1
#define RECNU_NDBL 2
#define RECNU_SMAL 4
#define RECNU_BIG 8
#define RECNU_SGL 16


struct milena {
	int classes[26];
	int letters[256];
	char *def_pho[256];
	char *redef[256];
	struct macronim *macros;
	struct milena_prefix *prefiksy;
	struct milena_nflex *nflex;
	struct mika_sfx *sfx;
	struct mika_afx *afx;	// ika/yka
	struct mika_bfx *bfx,**lbfx; // ika/yka
	struct mika_bfx *vsfx,**lvsfx; // czasownik akcent
	struct mika_afx *macanty;
	struct milena_fin *ivona_fin; // babole typu "pol." i "za³"
	struct milena_stringlist *nicks; // trzyliterowe, np 'ela', 'ja¶'
	struct milena_tld *tld; // top level domains oprócz dwuliterowych
	struct milena_stringlist *dict_known_words;
	struct recog_number *recog_numbers;
	int ext_verbs_no;	// czasownik z morfologika
	char **ext_verbs;
	struct milena_dic *milena_dic;
	int milena_dic_count;
	int phraser_mode;
	int language_mode;
	int emotmod;
	int digits_limit;
#ifdef HAVE_MORFOLOGIK
	struct
#if HAVE_MORFOLOGIK == 20
	morfologik_data
#else
	minimorf
#endif
	*md;
#endif
	struct milena_pseudo *pseudowords,**last_pseudo;
	struct milena_udic *udic;
	struct milena_udic_macro *udic_macros;
	struct milena_spell *spellers;
	struct milena_emot *emots;
	struct milena_key *keys;
	struct milena_key *chardes;
	struct milena_char *charnms;
	struct recog_format *recs;
	struct recog_expression *recexpr;
	struct recog_morfexpr *morfexpr;
	struct recog_choice *choices;
	struct recog_sayas *sayas;
	struct restcogfun *restcog_funs;
	struct letter_rule *lrules[256];
	struct memblock *mblock;
	int mblock_start,mblock_end;

	int class_bit;
	char *fname;
	int input_line;
	int charset_ok;
	char *rule_char;

	milena_errfun *errfun;
};


#define MBLOCK_SIZE 8176

struct memblock {
	struct memblock *next;
	char memo[1];
};

static void AllocMemBlock(struct milena *cfg)
{
	struct memblock *mbl=malloc(MBLOCK_SIZE);
	mbl->next=cfg->mblock;
	cfg->mblock=mbl;
	cfg->mblock_start=0;
	cfg->mblock_end=MBLOCK_SIZE-sizeof(struct memblock *);
}

static void *qallocMem(struct milena *cfg,size_t len)
{
	void *v;
	len=(len + 15) & 0xfff0;
	if (cfg->mblock_end - cfg->mblock_start <len) AllocMemBlock(cfg);
	v=(void *)(cfg->mblock->memo+cfg->mblock_start);
	cfg->mblock_start+=len;
	return v;
}

static void *sAllocMem(struct milena *cfg,size_t len)
{
	void *v;
	if (cfg->mblock_end - cfg->mblock_start <len) AllocMemBlock(cfg);
	cfg->mblock_end-=len;
	v=(void *)(cfg->mblock->memo+cfg->mblock_end);
	return v;
}

static char *mStrnDup(struct milena *cfg, char *str, int len)
{
	char *c;
	if (cfg->mblock_end - cfg->mblock_start <len+1) AllocMemBlock(cfg);
	cfg->mblock_end-=len+1;
	c=cfg->mblock->memo+cfg->mblock_end;
	memcpy(c,str,len);
	c[len]=0;
	return c;
}

static char *mStrDup(struct milena *cfg,char *str)
{
	int len=strlen(str)+1;
	char *c;
	if (cfg->mblock_end - cfg->mblock_start <len) AllocMemBlock(cfg);
	cfg->mblock_end-=len;
	c=cfg->mblock->memo+cfg->mblock_end;
	strcpy(c,str);
	return c;
}

static char *RevDup(struct milena *cfg,char *str)
{
	int len=strlen(str);
	char *c;int i;
	if (cfg->mblock_end - cfg->mblock_start <len+1) AllocMemBlock(cfg);
	cfg->mblock_end-=len+1;
	c=cfg->mblock->memo+cfg->mblock_end;
	for (i=0;i<len;i++) c[i]=str[len-i-1];
	c[i]=0;
	return c;
}

static char *dupstr(char *c)
{
	return strdup(c);
}
#define qalloc(a) qallocMem(cfg,a)
#define qalloca(a) sAllocMem(cfg,a)
#define strdup(a) mStrDup(cfg,a)
#define revdup(a) RevDup(cfg,a)


static int milena_init_recg(struct milena *cfg);
static int milena_recognize(struct milena *cfg,char **str,char *outbuf,int buflen,int pos);


void milena_registerErrFun(struct milena *cfg,milena_errfun *fun)
{
	cfg->errfun=fun;
}

static void m_message(char *c)
{
#ifdef __WIN32
	MessageBox(NULL,c,NULL,MB_OK | MB_ICONERROR);
#else
	fprintf(stderr,"%s\n",c);
#endif
}

static int m_gerror(struct milena *cfg,char *c)
{
	if (cfg->errfun) {
		cfg->errfun(c,cfg->fname,cfg->input_line);
	}
	else {
#ifdef __WIN32
		char buf[512];
		sprintf(buf,"%s:\n Blad w linii %d: %s",cfg->fname,cfg->input_line,c);
		MessageBox(NULL,buf,NULL,MB_OK | MB_ICONERROR);
#else
		fprintf(stderr,"%s: Blad w linii %d: %s\n",cfg->fname,cfg->input_line,c);
#endif
	}
}

#define gerror(a,b) {m_gerror(a,b);return -1;}

static void gwarning(struct milena *cfg,char *c)
{
	fprintf(stderr,"%s: Ostrzezenie w linii %d: %s\n",cfg->fname,cfg->input_line,c);
}

static void rerror(struct milena *cfg,int left)
{
	fprintf(stderr,"%s: Blad skladni w linii %d w okolicach %s-reguly '%s'\n",cfg->fname,cfg->input_line,(left?"L":"R"),cfg->rule_char);
	exit(1);
}

static void rtrim(char *c)
{
	char *d;
	for (d=c;*c;c++) if (!isspace(*c)) d=c+1;
	*d=0;
}


static void expand_macros(struct milena *cfg,char *whereto,char *rule)
{
	struct macronim *m;
	while (*rule) {
		for (m=cfg->macros;m;m=m->next) if (!strncmp(rule,m->name,strlen(m->name))) break;
		if (!m) {
			*whereto++=*rule++;
			continue;
		}
		rule+=strlen(m->name);
		*whereto++='(';
		expand_macros(cfg,whereto,m->value);
		whereto+=strlen(whereto);
		*whereto++=')';
	}
	*whereto++=0;
}



#define OP_CLASS 0x100
#define OP_CHAR 0x200
#define OP_NEG 0x400
#define OP_OR 0x800
#define OP_SEP 0x1000
#define OP_STAR 0x2000
#define OP_MOD 0x4000

/* rules */

static struct ruler *read_rule(struct milena *cfg,int left,int atstart);

static struct ruler *read_item(struct milena *cfg,int left,int atstart)
{
	struct ruler *r,*rx,**rr;
	int l;
	r=NULL;
	rr=&r;
	if (!*cfg->rule_char) rerror(cfg,left);
	for (;*cfg->rule_char;) {
		if (*cfg->rule_char == ')' || *cfg->rule_char==',') break;
		if (*cfg->rule_char == '!') {
			int clasbit=0;
			cfg->rule_char++;
			if (cfg->letters[(*cfg->rule_char) & 255] & IS_CLASS) {
				clasbit=cfg->classes[*cfg->rule_char - 'A'];
				cfg->rule_char++;
			}
			for (l=0;cfg->rule_char[l];l++) {
				if (cfg->letters[cfg->rule_char[l] & 255] & IS_LETTER) continue;
				if (cfg->rule_char[l]=='#') continue;
				if (left && cfg->rule_char[l]=='^') continue;
				if (!left && cfg->rule_char[l]=='$') continue;
				break;
			}
			if (!l && !clasbit) rerror(cfg,left);
			rx=qalloc(sizeof(*rx));
			rx->next=0;
			rx->op=OP_NEG | clasbit;
			if (l) {
				rx->arg=qalloca(l+1);
				memcpy(rx->arg,cfg->rule_char,l);
				((char *)(rx->arg))[l]=0;
				cfg->rule_char+=l;
			}
			else rx->arg=NULL;
			if (left) {
				rx->next=r;
				r=rx;
			}
			else {
				*rr=rx;
				rr=&rx->next;
			}
			atstart=0;
			continue;
		}
		if ((cfg->letters[(*cfg->rule_char) & 255] & IS_LETTER) || *cfg->rule_char=='#' || (!left && (*cfg->rule_char=='$' || *cfg->rule_char=='@')) || (left && *cfg->rule_char == '^')) {
			rx=qalloc(sizeof(*rx));
			rx->next=0;
			rx->op=OP_CHAR | ((*cfg->rule_char++) & 255);
			if (left) {
				rx->next=r;
				r=rx;
			}
			else {
				*rr=rx;
				rr=&rx->next;
			}
			atstart=0;
			continue;
		}
		if (cfg->letters[(*cfg->rule_char) & 255] & IS_CLASS) {
			int clasbit=cfg->classes[(*cfg->rule_char++) - 'A'];
			rx=qalloc(sizeof(*rx));
			rx->next=0;
			rx->op=OP_CLASS | clasbit;
			if (left) {
				rx->next=r;
				r=rx;
			}
			else {
				*rr=rx;
				rr=&rx->next;
			}
			atstart=0;
			continue;
		}
		if ((*cfg->rule_char == '\'' || *cfg->rule_char == '+') && !left && atstart) {
			rx=qalloc(sizeof(*rx));
			rx->next=0;
			rx->op=(*cfg->rule_char == '\'')?OP_SEP:OP_MOD;
			cfg->rule_char++;
			if (left) {
				rx->next=r;
				r=rx;
			}
			else {
				*rr=rx;
				rr=&rx->next;
			}
			atstart=0;
			continue;
		}
		if (*cfg->rule_char == '*') {
			cfg->rule_char++;
			rx=qalloc(sizeof(*rx));
			rx->next=0;
			rx->op=OP_STAR;
			if (left && !*cfg->rule_char) {
				rx->arg=NULL;
			}
			else {
				rx->arg=read_item(cfg,left,atstart);
			}
			if (left) {
				rx->next=r;
				r=rx;
			}
			else {
				*rr=rx;
				rr=&rx->next;
			}
			atstart=0;
			continue;


		}
		if (*cfg->rule_char != '(') rerror(cfg,left);
		cfg->rule_char++;
		rx=read_rule(cfg,left,atstart);
		atstart=0;
		if (*cfg->rule_char !=')') rerror(cfg,left);
		cfg->rule_char++;
		if (left) {
			struct ruler *rt;
			rt=rx;
			while (rx->next) rx=rx->next;
			rx->next=r;
			r=rt;
		}
		else{
			*rr=rx;
			while (rx->next) rx=rx->next;
			rr=&rx->next;
		}

	}
	if (!r) rerror(cfg,left);
	return r;

}

static struct ruler *read_rule(struct milena *cfg,int left,int atstart)
{
	struct ruler *r;
	r=read_item(cfg,left,atstart);
	while (*cfg->rule_char==',') {
		struct ruler *s=r;
		r=qalloc(sizeof(*r));
		r->next=0;
		r->op=OP_OR;
		r->arg=s;
		cfg->rule_char++;
		r->arg2=read_item(cfg,left,atstart);
	}
	return r;
}

static char *milena_langs[]={"es","en","it","fr","ru","de","ro","hu","pt","se",NULL};

static int add_rule(struct milena *cfg,int litera,char *str)
{
	char *lpart;
	char *rpart;
	char *pho,*c,*d;
	int eat,ladr,radr;
	struct letter_rule *lr,**llr;
	char *orig;
	struct ruler *rleft,*rright;
	char rule_buffer[8192];
	int langs;

	rtrim(str);
	orig=strdup(str);
	c=strchr(str,';');
	if (c) *c=0;
	while (*str && isspace(*str)) str++;
	if (!*str) return 1;
	langs=0;
	if (*str=='[') {
		int fin=0;
		c=str+1;
		while(!fin) {
			int i;
			str=strpbrk(c,"],");
			if (!str) gerror(cfg,"bledny jezyk");
			if (*str==']') fin=1;
			*str++=0;
			for (i=0;milena_langs[i];i++) if (!strcmp(c,milena_langs[i])) break;
			if (!milena_langs[i]) gerror(cfg,"nieznany jezyk");
			langs |= 1<<i;
			c=str;
		}
		while (*str && isspace(*str)) str++;
		if (!*str) gerror(cfg,"brak reguly po jezyku");
	}
	lpart=str;
	rpart=strchr(str,':');
	if (!rpart) gerror(cfg,"bledna regula");
	*rpart++=0;
	for (pho=rpart;*pho && !isspace(*pho);pho++);
	if (*pho) {
		*pho++=0;
		while (*pho && isspace(*pho)) pho++;
	}
	eat=0;
	rtrim(pho);
	for (c=d=pho;*c;c++) {
		if (isdigit(*c)) {
			eat+=strtol(c,&c,10);
			continue;
		}
		if (*c=='$') {
			eat++;
			continue;
		}
		*d++=*c;
	}
	*d=0;
	if (!*pho) {
		gwarning(cfg,"pusty fonem, uzyj '-' do deklaracji");
	}
	if (*pho=='-') pho="";
	ladr=0xFFFF;radr=0xFFFF;
	if (*lpart) {
		expand_macros(cfg,rule_buffer,lpart);
		cfg->rule_char=rule_buffer;
		rleft=read_rule(cfg,1,1);
		if (*cfg->rule_char) rerror(cfg,1);
	}
	else rleft=NULL;
	if (*rpart) {
		expand_macros(cfg,rule_buffer,rpart);
		cfg->rule_char=rule_buffer;
		rright=read_rule(cfg,0,1);
		if (*cfg->rule_char) rerror(cfg,0);
	} else rright=0;

	llr=&cfg->lrules[litera];
	while (*llr) llr=&(*llr)->next;
	lr=qalloc(sizeof(*lr));
	lr->next=0;
	*llr=lr;
	lr->lrule=rleft;
	lr->rrule=rright;
	lr->eats=eat;
	lr->langs=langs;
	lr->pho=strdup(pho);
	lr->orig=orig;
	return 1;
}

static int read_pho_cmd(struct milena *cfg,FILE *f,int m)
{
	char *c,*d,*cmd;
	char input_buf[8192],line_buf[256];
	if (!fgets(input_buf,256,f)) return 0;
	cfg->input_line++;
	if ((c=strchr(input_buf,';'))) *c=0;
	for (;;) {
		int i;
		c=strrchr(input_buf,'\\');
		if (!c) break;
		for (i=1;c[i];i++) if (!isspace(c[i])) break;
		if (c[i]) break;
		*c=0;
		if (!fgets(line_buf,256,f)) gerror(cfg,"Brak kontynuacji linii");
		cfg->input_line++;
		for (d=line_buf;*d && isspace(*d);d++);
		if (!*d) gerror(cfg,"Pusta linia kontynuacyjna");
		strcpy(c,d);
	}
	cmd=input_buf;
	while (*cmd && isspace(*cmd)) cmd++;
	if (!*cmd) return 1;
	for (c=cmd;*c && !isspace(*c);c++);
	if (*c) *c++=0;
	while (*c && isspace(*c)) c++;
	if (!*c) gerror(cfg,"Brak operandu");
	if (!m && !strcmp(cmd,"charset")) {
		if (cfg->charset_ok) gerror(cfg,"redefiniowany charset");
		cfg->charset_ok=1;
		for (;*c;c++) {
			if (isspace(*c)) continue;
			cfg->letters[(*c) & 255]=IS_LETTER;
		}
		return 1;
	}
	if (!cfg->charset_ok) gerror(cfg,"brak definicji charset");
	if (!m && !strcmp(cmd,"declare")) {
		int clasno=(*c++) - 'A';
		if (clasno<0 || clasno > 25 || clasno == ('S'-'A')) gerror(cfg,"bledna nazwa klasy");
		if (cfg->classes[clasno]) gerror(cfg,"klasa zdefiniowana ponownie");
		while (*c && isspace(*c)) c++;
		if (!*c) gerror(cfg,"pusta klasa");
		for (;*c;c++) if (!isspace(*c)) {
			int let=(*c) & 255;
			if (!(cfg->letters[let] & IS_LETTER)) gerror(cfg,"znak nie nalezy do charsetu");
			cfg->letters[let] |= 1<<cfg->class_bit;
		}
		cfg->letters[clasno+'A'] |= IS_CLASS;
		cfg->classes[clasno]=cfg->class_bit;
		cfg->class_bit++;
		return 1;
	}
	if (!m && !strcmp(cmd,"macro")) {
		char *d=c;
		struct macronim *m;
		while (*c && !isspace(*c)) c++;
		if (*c) *c++=0;
		while (*c && isspace(*c)) c++;
		if (!*c) gerror(cfg,"puste makro");
		m=qalloc(sizeof(*m));
		m->next=cfg->macros;
		cfg->macros=m;
		m->name=strdup(d);
		rtrim(c);
		m->value=strdup(c);
		return 1;
	}
	if (!m && !strcmp(cmd,"replace")) {
		int z=(*c++) & 255;
		char *d;
		if (cfg->letters[z] & IS_LETTER) gerror(cfg,"nieprawidlowy replace");
		if (cfg->letters[z] & IS_REDEF) gerror(cfg,"powielony replace");
		while (*c && isspace(*c)) c++;
		if (!*c) gerror(cfg,"pusty replace");
		rtrim(c);
		for (d=c;*d;d++) if (!(cfg->letters[(*c)&255] & IS_LETTER)) gerror(cfg,"znak w replace spoza charsetu");
		cfg->letters[z] |= IS_REDEF;
		cfg->redef[z]=strdup(c);
		return 1;
	}
	if (!m && !strcmp(cmd,"letter")) {
		int litera=(*c++) & 255;
		if (!(cfg->letters[litera] & IS_LETTER)) gerror(cfg,"litera spoza charsetu");
		while (*c && isspace(*c)) c++;
		if (!*c) gerror(cfg,"brak domyslnego fonemu");
		rtrim(c);
		if (cfg->def_pho[litera]) {
			if (!strcmp(cfg->def_pho[litera],c)) gwarning(cfg,"podwojna definicja litery");
			else gerror(cfg,"podwojna definicja litery");
		}
		cfg->def_pho[litera]=strdup(c);
		while (fgets(input_buf,256,f)) {
			cfg->input_line++;
			c=input_buf;
			while (*c && isspace(*c)) c++;
			if (*c=='\\') break;
			for (;;) {
				int i;
				c=strrchr(input_buf,'\\');
				if (!c) break;
				for (i=1;c[i];i++) if (!isspace(c[i])) break;
				if (c[i]) break;
				*c=0;
				if (!fgets(line_buf,256,f)) gerror(cfg,"Brak kontynuacji linii");
				cfg->input_line++;
				for (d=line_buf;*d && isspace(*d);d++);
				if (!*d) gerror(cfg,"Pusta linia kontynuacyjna");
				strcpy(c,d);
			}
			c=input_buf;
			while (*c && isspace(*c)) c++;
			if (!*c) continue;
			if (add_rule(cfg,litera,c)<0) return -1;
		}
		return 1;
	}
	if (m && !strcmp(cmd,"prefix")) {
		char *pf=c;
		struct milena_prefix *px;
		for (;*c && !isspace(*c);c++);
		if (!*c) gerror(cfg,"pusty prefix");
		*c++=0;
		while (*c && isspace(*c)) c++;
		if (!*c) gerror(cfg,"prefix bez akcentu");
		if (!isdigit(*c) || (*c=='0')) gerror(cfg,"bledny akcent");
		px=qalloc(sizeof(*px));
		px->next=cfg->prefiksy;
		cfg->prefiksy=px;
		px->pstress=(*c)-'0';
		px->prefix=strdup(pf);
		return 1;
	}
	if (m && !strcmp(cmd,"sfx")) {
		char *pf=c;
		struct mika_sfx *px;
		int stress=0,pstress=0,flags=0,pfg;
		for (;*c && !isspace(*c);c++);
		if (!*c) gerror(cfg,"pusty sufix");
		*c++=0;
		while (*c && isspace(*c)) c++;
		if (!*c) gerror(cfg,"Bledna linia");
		for (;*c;c++) {
			if (isspace(*c)) continue;
			if (isdigit(*c)) {
				if (stress) gerror(cfg,"Powtorzony akcent");
				stress=*c-'0';
				continue;
			}
			if (*c=='+' && c[1] && isdigit(c[1])) {
				c++;
				if (pstress) gerror(cfg,"Powtorzony akcent");
				pstress=*c-'0';
				continue;
			}

			if (*c=='v') pfg=MILDIC_V;
			else if (*c=='o') pfg=MILDIC_U|MILDIC_V;
			else if (*c=='u') pfg=MILDIC_U;
			else if (*c=='U') pfg=MILDIC_U | MILDIC_L;
			else if (*c=='F') pfg=MILDIC_UCA;
			else if (*c=='p') pfg=MILDIC_P|MILDIC_U;
			else if (*c=='r') pfg=MILDIC_R|MILDIC_U;
			else if (*c=='P') pfg=MILDIC_R|MILDIC_P|MILDIC_U;
			else gerror(cfg,"Nieznana flaga");
			if (flags) gerror(cfg,"Powielona flaga");
			flags=pfg;
		}

		px=qalloc(sizeof(*px));
		px->next=cfg->sfx;
		cfg->sfx=px;
		px->stress=stress;
		px->afx=revdup(pf);
		px->pstress=pstress;
		px->flags=flags;
		return 1;
	}
	if (m && !strcmp(cmd,"verbsfx")) {
		char *pf=c;
		struct mika_bfx *px;
		for (;*c && !isspace(*c);c++);
		if (!*c) gerror(cfg,"pusty sufix");
		*c++=0;
		while (*c && isspace(*c)) c++;
		if (!*c) gerror(cfg,"sufix bez akcentu");
		if (!isdigit(*c) || (*c=='0')) gerror(cfg,"bledny akcent");
		px=qalloc(sizeof(*px));
		px->next=NULL;
		*cfg->lvsfx=px;
		cfg->lvsfx=&px->next;
		px->stress=(*c)-'0';
		px->afx=revdup(pf);
		return 1;
	}
	if (m && !strcmp(cmd,"ikactl")) {
		char *pf=c;
		struct mika_bfx *px;
		for (;*c && !isspace(*c);c++);
		if (!*c) gerror(cfg,"pusty sufix");
		*c++=0;
		while (*c && isspace(*c)) c++;
		if (!*c) gerror(cfg,"sufix bez akcentu");
		if (!isdigit(*c) || (*c=='0')) gerror(cfg,"bledny akcent");
		px=qalloc(sizeof(*px));
		px->next=NULL;
		*cfg->lbfx=px;
		cfg->lbfx=&px->next;
		px->stress=(*c)-'0';
		px->afx=revdup(pf);
		return 1;
	}
	if (m && !strcmp(cmd,"ikasfx")) {
		char *pf=c;
		struct mika_afx *px;
		for (;*c && !isspace(*c);c++);
		if (!*c) gerror(cfg,"pusty sufix");
		*c++=0;
		while (*c && isspace(*c)) c++;
		if (*c) gerror(cfg,"blad skladni");
		px=qalloc(sizeof(*px));
		px->next=cfg->afx;
		cfg->afx=px;
		px->afx=revdup(pf);
		return 1;
	}

	gerror(cfg,"nierozpoznane polecenie");
	return 0;
}


static int read_phoncfg(struct milena *cfg,char *fname,int mode)
{
	FILE *f;int n;

	cfg->fname=fname;
	cfg->input_line=0;
	f=fopen(fname,"rb");
	if (!f) {
		perror(fname);
		return 0;
	}
	for (;;) {
		n=read_pho_cmd(cfg,f,mode);
		if (n<=0) break;
	}
	fclose(f);
	if (n<0) return 0;
	return 1;
}

int milena_ReadStressFile(struct milena *cfg,char *fname)
{
	FILE *f;int n;
	f=fopen(fname,"rb");
	if (!f) {
		perror(fname);
		return 0;
	}
	cfg->fname=fname;
	cfg->input_line=0;
	for (;;) {
		n=read_pho_cmd(cfg,f,1);
		if (n<=0) break;
	}
	fclose(f);
	if (n<0) return 0;
	return 1;
}

static int milena_dic_cmp(const void *v1,const void *v2)
{
	return strcmp(((struct milena_dic *)v1)->word,((struct milena_dic *)v2)->word);
}

static int read_prestr(struct milena *cfg,char *fname)
{
	FILE *f;
	int pos;
	int flags,stress,pf;
	char *c;
	char input_buf[256];
	f=fopen(fname,"rb");
	if (!f) {
		perror(fname);
		return 0;
	}
	cfg->milena_dic_count=0;
	cfg->input_line=0;
	cfg->fname=fname;

	while(fgets(input_buf,256,f)) {
		for (c=input_buf;*c && isspace(*c);c++);
		if (c) cfg->milena_dic_count++;
	}
	fseek(f,0,SEEK_SET);
	cfg->milena_dic=malloc(sizeof(*cfg->milena_dic)*cfg->milena_dic_count);
	for (pos=0;;) {
		flags=0;
		stress=0;
		if (!fgets(input_buf,256,f)) break;
		cfg->input_line++;
		for (c=input_buf;*c && !isspace(*c);c++);
		if (!*c) continue;
		*c++=0;
		if (!*input_buf) gerror(cfg,"Bledna linia");
		while (*c && isspace(*c)) c++;
		if (!*c) gerror(cfg,"Bledna linia");
		for (;*c;c++) {
			if (isspace(*c)) continue;
			if (isdigit(*c)) {
				if (stress) gerror(cfg,"Powtorzony akcent");
				stress=*c-'0';
				continue;
			}
			if (*c=='v') pf=MILDIC_V;
			else if (*c=='V') pf=MILDIC_VIMPT|MILDIC_V;
			else if (*c=='o') pf=MILDIC_U|MILDIC_V;
			else if (*c=='O') pf=MILDIC_VIMPT|MILDIC_U|MILDIC_V;
			else if (*c=='u') pf=MILDIC_U;
			else if (*c=='U') pf=MILDIC_U|MILDIC_L;
			else if (*c=='p') pf=MILDIC_P|MILDIC_U;
			else if (*c=='r') pf=MILDIC_R|MILDIC_U;
			else if (*c=='R') pf=MILDIC_R;
			else if (*c=='P') pf=MILDIC_R|MILDIC_P|MILDIC_U;
			else if (*c=='N') pf=MILDIC_NEVER|MILDIC_U;
			else if (*c=='s') pf=MILDIC_SEP|MILDIC_U;
			else if (*c=='G') pf=MILDIC_FG;
			else if (*c=='w') pf=MILDIC_VOP | MILDIC_V;
			else if (*c=='h') pf=MILDIC_HIM;
			else gerror(cfg,"Nieznana flaga");
			//if (flags) gerror(cfg,"Powielona flaga");
			flags|=pf;
		}
		if (flags == MILDIC_U && !strcmp(input_buf,"nie")) flags |= MILDIC_N;
		cfg->milena_dic[pos].word=strdup(input_buf);
		cfg->milena_dic[pos].flags=flags;
		cfg->milena_dic[pos++].stress=stress;
	}
	fclose(f);
	qsort(cfg->milena_dic,cfg->milena_dic_count,sizeof(*cfg->milena_dic),milena_dic_cmp);
	return 1;
}

char *milena_GetVersion(void)
{
	return MILENA_VERSION;
}

void milena_Close(struct milena *cfg)
{
	struct memblock *mb;
	while (mb=cfg->mblock) {
		cfg->mblock=mb->next;
		free(mb);
	}
	if (cfg->milena_dic) free(cfg->milena_dic);
	if (cfg->ext_verbs) free(cfg->ext_verbs);
	free(cfg);
}

struct milena *milena_Init(char *phonefile,char *dictfile,char *stressfile)
{
	struct milena *cfg;
	cfg=calloc(1,sizeof(*cfg));
	cfg->class_bit=CLASS_BIT;
	cfg->last_pseudo=&cfg->pseudowords;
	cfg->lbfx=&cfg->bfx;
	cfg->lvsfx=&cfg->vsfx;
	cfg->phraser_mode=0;
	cfg->emotmod=1;
	AllocMemBlock(cfg);
	if (!read_phoncfg(cfg,phonefile,0)) {
		milena_Close(cfg);
		return NULL;
	}
	if (!read_phoncfg(cfg,stressfile,1)) {
		milena_Close(cfg);
		return NULL;
	}
	if (!read_prestr(cfg,dictfile)) {
		milena_Close(cfg);
		return NULL;
	}
	milena_init_recg(cfg);
	return cfg;
}

static int phraser_read_unit(struct milena *cfg,char *pat,int flags)
{
	char *c,*res[4];
	int n;
	struct milena_nflex *nf;
	c=pat;
	while (*c && !isspace(*c)) c++;
	if (*c) *c++=0;
	while (*c && isspace(*c)) c++;
	for (n=0;n<4;n++) {
		res[n]=c;
		if (n>=3) break;
		c=strchr(c,'|');
		if (!c) gerror(cfg,"Bledna odmiana");
		*c++=0;
	}
	nf=qalloc(sizeof(*nf));
	nf->next=cfg->nflex;
	cfg->nflex=nf;
	nf->pattern=strdup(pat);
	for (n=0;n<4;n++) nf->result[n]=strdup(res[n]);
	nf->flags=flags;
	return 1;
}

static int phraser_read_mac(struct milena *cfg,char *c)
{
	struct mika_afx *mac;
	while (*c && isspace(*c)) c++;
	if (!*c) gerror(cfg,"Pusty mac");
	rtrim(c);
	mac=qalloc(sizeof(*mac));
	mac->afx=strdup(c);
	mac->next=cfg->macanty;
	cfg->macanty=mac;
	return 1;

}

static int phraser_read_nick(struct milena *cfg,char *c)
{
	struct milena_stringlist *nick;
	int i;
	while (*c && isspace(*c)) c++;
	if (!*c) gerror(cfg,"Pusty nick");
	rtrim(c);
	if (strlen(c) != 3) gerror(cfg,"Bledna dlugosc nick");
	nick=qalloc(sizeof(*nick));
	nick->string=strdup(c);
	nick->next=cfg->nicks;
	cfg->nicks=nick;
	return 1;
}

static int phraser_read_tld(struct milena *cfg,char *c)
{
	struct milena_tld *tld;
	int flags=0;char *s,*pr;
	while (*c && isspace(*c)) c++;
	if (!*c) gerror(cfg,"Pusty tld");
	rtrim(c);
	s=c;pr=NULL;
	while (*c && islower(*c)) c++;
	if (*c) {
	    if (!isspace(*c)) gerror(cfg,"Nieplawidlowy znak w tld");
	    *c++=0;
	    while (*c && isspace(*c)) c++;
	    if (*c=='$') {
		c++;
		if (*c) gerror(cfg,"Nieplawidlowy znak w tld");
		flags=MILDIC_SPELL;
	    }
	    else {
		pr=c;
	    }
	}
	tld=qalloc(sizeof(*tld));
	tld->string=strdup(s);
	tld->pron=pr?strdup(pr):tld->string;
	tld->flags=flags;
	tld->next=cfg->tld;
	cfg->tld=tld;
	return 1;
}


static int phraser_read_spell(struct milena *cfg,char *c)
{
	char *pw=c;
	struct milena_spell *ps;
	for (;*c && !isspace(*c);c++);
	if (!*c) gerror(cfg,"pusty spell");
	*c++=0;
	if (pw[1]) gerror(cfg,"nieprawidlowy spell");
	while (*c && isspace(*c)) c++;
	if (!*c) gerror(cfg,"spell bez rezultatu");
	rtrim(c);
	ps=qalloc(sizeof(*ps));
	ps->next=NULL;
	ps->znak=*pw;
	ps->result=strdup(c);
	ps->next=cfg->spellers;
	cfg->spellers=ps;
	if (ps->znak=='[' || ps->znak=='{') cfg->phraser_mode |= MILENA_PHR_IGNORE_INFO;
	//else if (ps->znak=='~') cfg->phraser_mode |= MILENA_PHR_IGNORE_TILDE;
	return 1;
}

static int phraser_read_pseudo(struct milena *cfg,char *c)
{
	char *pw=c;
	struct milena_pseudo *ps;
	for (;*c && !isspace(*c);c++);
	if (!*c) gerror(cfg,"pusty pseudo");
	*c++=0;
	while (*c && isspace(*c)) c++;
	if (!*c) gerror(cfg,"pseudo bez rezultatu");
	rtrim(c);
	ps=qalloc(sizeof(*ps));
	ps->next=NULL;
	ps->pattern=strdup(pw);
	ps->result=strdup(c);
	*cfg->last_pseudo=ps;
	cfg->last_pseudo=&ps->next;
	return 1;
}

static int phraser_read_emot(struct milena *cfg,char *c)
{
	char *pw=c;
	struct milena_emot *ps;
	for (;*c && !isspace(*c);c++);
	if (!*c) gerror(cfg,"pusty emoticon");
	*c++=0;
	while (*c && isspace(*c)) c++;
	if (!*c) gerror(cfg,"emoticon bez rezultatu");
	rtrim(c);
	ps=qalloc(sizeof(*ps));
	ps->next=cfg->emots;
	cfg->emots=ps;
	ps->emot=strdup(pw);
	ps->result=strdup(c);
	return 1;
}


int milena_ReadVerbs(struct milena *cfg,char *fname)
{
    /* zast±piony przez identyfikacjê czasowników nyleny */
#if 0
	FILE *f;char buf[256];
	int i,add_verb;
	if (cfg->ext_verbs_no) {
		m_message("Plik czasownikow juz wczytany");
		return 0;
	}
	f=fopen(fname,"rb");
	if (!f) {
		m_message("Brak podanego pliku czasownikow");
		return 0;
	}
	fgets(buf,256,f);
	add_verb=strtol(buf,NULL,10);
	fgets(buf,256,f);
	cfg->ext_verbs_no=strtol(buf,NULL,10);
	cfg->milena_dic=realloc(
		cfg->milena_dic,
		sizeof(*cfg->milena_dic)*(add_verb+cfg->milena_dic_count));
	cfg->ext_verbs=malloc(sizeof(*(cfg->ext_verbs))*cfg->ext_verbs_no);
	for (i=0;i<add_verb;i++) {
		fgets(buf,256,f);
		rtrim(buf);
		cfg->milena_dic[cfg->milena_dic_count].word=strdup(buf);
		cfg->milena_dic[cfg->milena_dic_count].flags=MILDIC_V;
		cfg->milena_dic[cfg->milena_dic_count++].stress=0;
	}
	qsort(cfg->milena_dic,cfg->milena_dic_count,sizeof(*cfg->milena_dic),milena_dic_cmp);
	for (i=0;i<cfg->ext_verbs_no;i++) {
		fgets(buf,256,f);
		rtrim(buf);
		cfg->ext_verbs[i]=strdup(buf);
	}
	fclose(f);
#endif
	return 1;
}

static int milena_line_is_recog(char *cmd);
static int milena_recog_line(struct milena *cfg,char *cmd,char *str,int from_dict);
static int read_udic_line(struct milena *,FILE *,char *,int flags);
static int milena_poilu(int n,char *outbuf,int buflen,int pos);
static int milena_koloilu(int n,char *outbuf,int buflen,int pos,int female);
static int milena_speak_digit(int n,int mode,char *outbuf,int buflen,int pos);

static int milena_read_phraser_line(struct milena *cfg,char *line,int from_dict)
{
	char *s,*c;
	s=strstr(line,"//");
	if (s) *s=0;
	s=line;
	while (*s && isspace(*s)) s++;
	if (!*s) return 0;
	c=s;
	while (*c && !isspace(*c)) c++;
	if (*c) *c++=0;
	while (*c && isspace(*c)) c++;
	if (!*c) {
		m_gerror(cfg,"Blad skladni");
		return -1;
	}
	rtrim(c);
	if (!strcmp(s,"unit")) {
		if (phraser_read_unit(cfg,c,0)<0) return -1;
		return 0;
	}
	if (!strcmp(s,"unitf")) {
		if (phraser_read_unit(cfg,c,MILENA_NFLEX_FEMALE)<0) return -1;
		return 0;
	}
	if (!strcmp(s,"unitfh") || !strcmp(s,"unithf")) {
		if (phraser_read_unit(cfg,c,MILENA_NFLEX_FEMALE | MILENA_NFLEX_HIDDEN)<0) return -1;
		return 0;
	}
	if (!strcmp(s,"unith")) {
		if (phraser_read_unit(cfg,c,MILENA_NFLEX_HIDDEN)<0) return -1;
		return 0;
	}
	if (!strcmp(s,"pseudo")) {
		if (phraser_read_pseudo(cfg,c)<0) return -1;
		return 0;
	}
	if (!strcmp(s,"emot")) {
		if (phraser_read_emot(cfg,c)<0) return -1;
		return 0;
	}
	if (!strcmp(s,"word")) {
		if (read_udic_line(cfg,NULL,c,0)<0) return -1;
		return 0;
	}
	
	if (!strcmp(s,"emotmod")) {
		int n=strtol(c,NULL,10);
		if (n<0 || n>9) {
			m_gerror(cfg,"Wartosc poza zakresem");
			return -1;
		}
		cfg->emotmod=n;
		return 0;
	}
	if (!strcmp(s,"spell")) {
		if (phraser_read_spell(cfg,c)<0) return -1;
		return 0;
	}
	if (!strcmp(s,"mac")) {
		if (phraser_read_mac(cfg,c)<0) return -1;
		return 0;
	}
	if (!strcmp(s,"nick")) {
		if (phraser_read_nick(cfg,c)<0) return -1;
		return 0;
	}
	if (!strcmp(s,"tld")) {
		if (phraser_read_tld(cfg,c)<0) return -1;
		return 0;
	}
	if (milena_line_is_recog(s)) {
		if (milena_recog_line(cfg,s,c,from_dict)<0) return -1;
		return 0;
	}
	m_gerror(cfg,"Nierozpoznana komenda");
	return -1;
}

int milena_ReadPhraser(struct milena *cfg,char *fname)
{
	FILE *f;
	char buf[512],*s,*c;
	f=fopen(fname,"rb");
	if (!f) {
		perror(fname);
		return 0;
	}

	cfg->input_line=0;
	cfg->fname=fname;
	while(fgets(buf,512,f)) {
		cfg->input_line++;
		if (milena_read_phraser_line(cfg,buf,0)) {
			fclose(f);
			return 0;
		}
	}
	fclose(f);
	return 1;
}

void milena_SetPhraserMode(struct milena *cfg,int mode)
{
	if (mode>=0 && mode <=3) cfg->phraser_mode=mode;
}

#define MILENA_LANG_ES 1

void milena_SetLangMode(struct milena *cfg,char *lang)
{
	int i;
	for (i=0;milena_langs[i];i++) if (!strcmp(lang,milena_langs[i])) {
		cfg->language_mode |= 1<<i;
		break;
	}
}

#define MIFRAT_ROMAN	1
#define MIFRAT_HEXA	2
#define MIFRAT_DECI	4

#ifdef HAVE_MORFOLOGIK
#include "milena_morfologik.c"
#endif
#include "milena_strtod.c"
#include "milena_udici.c"
#include "milena_phrasi.c"
#include "milena_transi.c"
#include "milena_prestri.c"
#include "milena_recogni.c"
#include "milena_toiso.c"
#include "milena_keychar.c"
#include "milena_ivonizer.c"

/* funkcje dla helperów */

char *milena_FilePath(char *name,char *buffer)
{
#ifdef __WIN32
	static int was=0;
	static char path[MAX_PATH];
	static char path2[MAX_PATH];
	if (!was) {
		GetModuleFileName(NULL,path2,MAX_PATH);
		strcpy(path,dirname(path2));
		strcat(path,"\\data\\");
		was=1;
	}
	strcpy(buffer,path);
	strcat(buffer,name);
#else
/*	struct stat sb;
	if (!stat(name,&sb)) return name;
	sprintf(buffer,"%s/.milena/%s",getenv("HOME"),name);
	if (stat(buffer,&sb)) sprintf(buffer,DATAPATH"/%s",name);
*/	sprintf(buffer,DATAPATH"/%s",name);
#endif
	return buffer;

}


int milena_IsIgnoredWord(struct milena *cfg,char *slowo)
{
	struct milena_stringlist *sl;
	for (sl=cfg->dict_known_words;sl;sl=sl->next) {
		if (!strcmp(sl->string,slowo)) return 1;
	}
	return 0;
}

/* zwraca 0 jesli doszla do konca tekstu, 1 jesli napotkala slowo */
int milena_SkipToUnknownWord(struct milena *cfg,char **str)
{
	char *cs;
	struct milena_udic *ud;
	int bof;
	int at_end(char *s)
	{
		if (*s=='-') {
			s++;
			if (!*s) return 1;
			if (isdigit(*s) || lci[(*s) & 255]) return 0;
			return 1;
		}
		while (*s && isspace(*s)) s++;
		if (!s) return 1;
		if (isdigit(*s) || lci[(*s) & 255]) return 0;
		return 1;
	}

	int numerica(void)
	{
		char *c;
		if (!isdigit(**str)) return 0;
		for (c=*str;*c && isdigit(*c);c++);
		if (*c=='.' && c[1] && isdigit(c[1])) {
			c++;
			for (;*c && isdigit(*c);c++);
		}
		if (!*c || !lci [(*c) & 255]) {
			*str=c;
			return 1;
		}
		return 0;
	}
	for (;;) {
		bof=0;
		while(**str && !lci[(**str) & 255]) {
			if (strchr(".?!\n",**str)) bof=1;
			(*str)++;
		}
		if (!**str) return 0;
		if (milena_SkipRecognizedPart(cfg,str,bof)) continue;
		for (ud=cfg->udic;ud;ud=ud->next) {
			int nphr;
			struct phraser_dic_rc phra[16];
			char *cs;
			cs=*str;
			if (!phraser_CompareDic(cfg,ud->word,str,phra,&nphr)) continue;
			if (ud->flags & MILDIC_ATQUE) {
				if (at_end(*str)!=2) {
					*str=cs;
					continue;
				}
			}
			if (ud->flags & MILDIC_ATEND) {
				if (!at_end(*str)) {
					*str=cs;
					continue;
				}
			}
			break;
		}
		if (ud) continue;
		if (numerica()) {
			while (**str && isspace(**str)) (*str)++;
			milena_get_unit(cfg,str,0,NULL,0);
			continue;
		}
		break; // poczatek slowa
	}
	return 1+bof;
}

#ifndef __WIN32
char const *milena_GetDataPath(void)
{
	return DATAPATH;
}
#endif

int milena_DigitsLimit(struct milena *milena,int limit)
{
    if (limit < 2 || limit > 9) return -1;
    milena->digits_limit=limit;
    return 0;
}

static unsigned char i2ut[128][2]={
    {194,128},{194,129},{194,130},{194,131},{194,132},{194,133},{194,134},
    {194,135},{194,136},{194,137},{194,138},{194,139},{194,140},{194,141},
    {194,142},{194,143},{194,144},{194,145},{194,146},{194,147},{194,148},
    {194,149},{194,150},{194,151},{194,152},{194,153},{194,154},{194,155},
    {194,156},{194,157},{194,158},{194,159},{194,160},{196,132},{203,152},
    {197,129},{194,164},{196,189},{197,154},{194,167},{194,168},{197,160},
    {197,158},{197,164},{197,185},{194,173},{197,189},{197,187},{194,176},
    {196,133},{203,155},{197,130},{194,180},{196,190},{197,155},{203,135},
    {194,184},{197,161},{197,159},{197,165},{197,186},{203,157},{197,190},
    {197,188},{197,148},{195,129},{195,130},{196,130},{195,132},{196,185},
    {196,134},{195,135},{196,140},{195,137},{196,152},{195,139},{196,154},
    {195,141},{195,142},{196,142},{196,144},{197,131},{197,135},{195,147},
    {195,148},{197,144},{195,150},{195,151},{197,152},{197,174},{195,154},
    {197,176},{195,156},{195,157},{197,162},{195,159},{197,149},{195,161},
    {195,162},{196,131},{195,164},{196,186},{196,135},{195,167},{196,141},
    {195,169},{196,153},{195,171},{196,155},{195,173},{195,174},{196,143},
    {196,145},{197,132},{197,136},{195,179},{195,180},{197,145},{195,182},
    {195,183},{197,153},{197,175},{195,186},{197,177},{195,188},{195,189},
    {197,163},{203,153}};

static int i2uf(char *in, char *out, int outlen)
{
    int i;
    unsigned char *c;
    for (i=1, c=(unsigned char *)in;*c;c++,i++) {
        if ((*c) & 0x80) i++;
    }
    if (i > outlen) return i;
    for (c=(unsigned char *)in;*c;c++) {
        if ((*c) & 0x80) {
            *out++=i2ut[(*c) & 0x7f][0];
            *out++=i2ut[(*c) & 0x7f][1];
        }
        else *out++ = *c;
    }
    *out=0;
    return 0;
}

int milena_i2uf(char *in, char *out, int outlen)
{
    return i2uf(in, out, outlen);
}
