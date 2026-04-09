/*
 * milena_recogni.c - Milena TTS system (phraser)
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

/*

blank - ci±g spacji
\blank - opcjonalny ci±g spacji

\znak - opcjonalny znak

formaty:
	d - liczba
	r - liczba rzymska
	c - wybór
	e - wyra¿enie tekstowe
	x - liczba szesnastkowa

przyk³ad:

recognize dmy {d:d1-31}.\ {m:r1-12}.\ {y:d}
recognize dmy {d:d1-31} {m:r1-12} {y:d}
recognize dmy {d:d1-31} {m:cmond} {y:d}
recognize dmyr {d:d1-31}.\ {m:r1-12}.\ {y:d} r.
recognize dmyr {d:d1-31} {m:r1-12} {y:d} r.
recognize wieczny {w:r1-30}-{f:ewieczny}
expr wieczny wieczn(ymi|ym|ych|ego|emu|y|a|±|ej|e|i)

recognize ntego {n:d}-go

recognize dayoweek {w:cdow}
choice dow 1 pon
choice dow 1 poniedzia³ek
choiceval dow 1 poniedzia³ek

sayas:
	dXX - liczba we wbudowanym formacie

przyk³ad:

sayas dmyr	{d:dmc} {m:dmt} {y:dmc} roku
sayas dmy	{d:dmc} {m:dmt} {y:dmc}
sayas wieczny	{w:dap}{f}
sayas ntego	{n:dmc}
sayas daoweek	{w}
*/


struct recog_format {
	struct recog_format *next;
	int flags;
	char *name;
	char *str;
};

static char *milena_mons[]={"","stycznia","lutego","marca","kwietnia",
	"maja","czerwca","lipca","sierpnia","wrze¶nia","pa¼dziernika",
	"listopada","grudnia",NULL};


/*
speaker_dit:
	mm - drugi
	fm - druga
	nm - drugie
	mc - drugiemu
	fd - drugiej
	md - drugiego
	fb - drug±
	mn - drugim
	ap - drugo-
	mx - dwa
	fx - dwie
	f0 - zero dwie
	ac - dwu-
	mt - miesi±ca
	pb - drugich
*/

#define MILDIT_MM 0
#define MILDIT_FM 1
#define MILDIT_NM 2
#define MILDIT_MC 3
#define MILDIT_FD 4
#define MILDIT_MD 5
#define MILDIT_FB 6
#define MILDIT_MN 7
#define MILDIT_AP 8
#define MILDIT_PB 9
#define MILDIT_MX 10
#define MILDIT_FX 11
#define MILDIT_F0 12
#define MILDIT_AC 13
#define MILDIT_MT 14
#define MILDIT_XN 15
#define MILDIT_YM 16
#define MILDIT_YF 17
#define MILDIT_SM 18
#define MILDIT_SF 19

#define MILRGT_FLOAT 3
#define MILRGT_EXPR 2
#define MILRGT_CHOICE 1

static char *dec_fin[]={
	"y","a","e","emu","ej","ego","±","ym","o","ych"};

static char *jed_por[]={
	"zerow","pierwsz","drugi","trzeci","czwart",
	"pi±t","szóst","siódm","ósm","dziewi±t",
	"dziesi±t","jedenast","dwunast","trzynast","czternast",
	"piêtnast","szesnast","siedemnast","osiemnast","dziewiêtnast"};
static char *dec_por[]={
	NULL,NULL,"dwudziest","trzydziest","czterdziest",
	"piêædziesi±t","sze¶ædziesi±t","siedemdziesi±t",
	"osiemdziesi±t","dziewiêædziesi±t"};
static char *dec_pac[]={
	NULL,NULL,"dwudziesto","trzydziesto","czterdziesto",
	"piêædziesiêcio","sze¶ædziesiêcio","siedemdziesiêcio",
	"osiemdziesiêcio","dziewiêædziesiêcio"};

static char *hun_por[]={
	NULL,NULL,"dwu","trzech","czterech","piêæ","sze¶æ",
		"siedem","osiem","dziewiêæ"};
static char *mil_por[]={
	NULL,"jedno","dwu","trzy","cztero","piêcio","sze¶cio",
	"siedmio","o¶mio","dziewiêcio","dziesiêcio"};

static char *sgl_prd[]={
	NULL,"jednym","dwoma","trzema","czterema","piêcioma","sze¶cioma",
	"siedmioma","o¶mioma","dziewiêcioma","dziesiêcioma"};


static char *hun_pac[]={NULL,"stu","dwustu","trzystu",
	"czterystu","piêæset","sze¶æset","siedemset","osiemset","dziewiêæset"};
static char *hun_bac[]={NULL,"stu","dwustu","trzystu",
	"czterystu","piêciuset","sze¶ciuset","siedmiuset","o¶miuset","dziewiêciuset"};

static char *dec_ilu[]={
	NULL,NULL,"dwudziestu","trzydziestu","czterdziestu",
	"piêædziesiêciu","sze¶ædziesiêciu","siedemdziesiêciu",
	"osiemdziesiêciu","dziewiêædziesiêciu"};
static char *jed_ilu[]={
	NULL,"jeden","dwóch","trzech","czterech",
	"piêciu","sze¶ciu","siedmiu","o¶miu","dziewiêciu",
	"dziesiêciu","jedenastu","dwunastu","trzynastu","czternastu",
	"piêtnastu","szesnastu","siedemnastu","osiemnastu","dziewiêtnastu"};

static int milena_koloilu(int n,char *outbuf,int buflen,int pos,int female)
{
    int tau;
    if (!n) {
        pushstr("zera");
        return pos;
    }
    if (n==1) {
	if (female) {
	    pushstr("jednej");
	}
	else {
	    pushstr("jednego");
	}
	return pos;
    }
    if (n<1 || n > 999999) return milena_integer(n,outbuf,buflen,pos);
    if (n < 1000) return milena_poilu(n,outbuf,buflen,pos);
    tau=n/1000;
    n %= 1000;
    if (tau > 1) {
	pos=milena_poilu(tau,outbuf,buflen,pos);
	pushstr(" tysiêcy");
    }
    else {
	pushstr("tysi±ca");
    }
    if (n) {
	pushbuf(' ');
	pos=milena_poilu(n,outbuf,buflen,pos);
    }
    return pos;
}
    
    
static int milena_przedilu(int n,char *outbuf,int buflen,int pos,int female)
{
    char buf[64];
    //fprintf(stderr,"Przedilu %d [f=%d]\n",n,female);
    if (n >= 1000 && n <1000000 && !(n % 1000)) {
	n /= 1000;
	if (n == 1) {
	    pushstr("tysi±cem");
	    return pos;
	}
	else {
	    if (n >= 100) {
		if (n % 100 == 0) {
		    strcpy(buf,hun_bac[n/100]);
		    strcpy(buf+strlen(buf)-1,"oma");
		    pushstr(buf);
		}
		else {
		    pushstr(hun_bac[n/100]);
		    pushbuf(' ');
		}
		n %= 100;
	    }
	    if (n) {
		if (n == 1) {
		    pushstr("jeden");
		}
		else if (n <= 10) {
		    pushstr(sgl_prd[n]);
		}
		else if (n <= 19) {
		    pushstr(jed_por[n]);
		    pushstr("oma");
		}
		else {
		    strcpy(buf,dec_ilu[n/10]);
		    strcpy(buf+strlen(buf)-1,"oma");
		    pushstr(buf);
		    if (n % 10) {
			pushbuf(' ');
			pushstr(sgl_prd[n % 10]);
		    }
		}
	    }
	}
	pushstr(" tysi±cami");
	return pos;
    }
    if (n<1 || n>999) return milena_integer(n,outbuf,buflen,pos);
    if (female) {
	if (n == 1) {
	    pushstr("jedn±");
	    return pos;
	}
	if (n == 2) {
	    pushstr("dwiema");
	    return pos;
	}
    }
    else if (n==1) {
	pushstr("jednym");
	return pos;
    }
    if (n >= 100) {
	int st=n/100;
	n=n%100;
	if (!n) {
	    strcpy(buf,hun_bac[st]);
	    strcpy(buf+strlen(buf)-1,"oma");
	    pushstr(buf);
	    return pos;
	}
	else {
	    pushstr(hun_bac[st]);
	    pushbuf(' ');
	}
    }
    if (n == 1) {
	pushstr("jeden");
    }
    else if (n <= 10) {
	pushstr(sgl_prd[n]);
    }
    else if (n <= 19) {
	pushstr(jed_por[n]);
	pushstr("oma");
    }
    else {
	strcpy(buf,dec_ilu[n/10]);
	strcpy(buf+strlen(buf)-1,"oma");
	pushstr(buf);
	if (n % 10) {
	    pushbuf(' ');
	    if ((n % 10) == 1) {
		pushstr("jeden");
	    }
	    else {
		pushstr(sgl_prd[n % 10]);
	    }
	}
    }
    return pos;
}
    
static int milena_poilu(int n,char *outbuf,int buflen,int pos)
{
	if (n<1 || n>999) return milena_integer(n,outbuf,buflen,pos);
	if (n==1) {
		pushstr("jednym");
		return pos;
	}
	if (n>=100) {
		pushstr(hun_bac[n/100]);
		n%=100;
		if (!n) return pos;
		pushbuf(' ');
	}
	if (!n) return pos;
	if (n<20) {
		pushstr(jed_ilu[n]);
		return pos;
	}
	pushstr(dec_ilu[n/10]);
	n%=10;
	if (n) {
		pushbuf(' ');
		pushstr(jed_ilu[n]);
	} return pos;
}

static int milena_speak_digit(int n,int mode,char *outbuf,int buflen,int pos)
{

	int m;
	if (n>999999) return milena_integer(n,outbuf,buflen,pos);
	//fprintf(stderr,"mode=%d\n",mode);
	if (mode == MILDIT_SM) {
		return milena_przedilu(n,outbuf,buflen,pos,0);
	}
	if (mode == MILDIT_SF) {
		return milena_przedilu(n,outbuf,buflen,pos,1);
	}
	if (mode == MILDIT_XN) {
		return milena_poilu(n,outbuf,buflen,pos);
	}
	if (mode == MILDIT_YM) {
		return milena_koloilu(n,outbuf,buflen,pos,0);
	}
	if (mode == MILDIT_YF) {
		return milena_koloilu(n,outbuf,buflen,pos,1);
	}
	if (mode == MILDIT_MT) {
		if (n>=1 && n<=12) pushstr(milena_mons[n]);
		return pos;
	}
	if (mode < MILDIT_MX) {
		if (!n) {
			pushstr(jed_por[0]);
			pushstr(dec_fin[mode]);
			return pos;
		}
		m=n%100;
		if (m) {
			if (n>100) {
				pos=milena_integer(n-m,outbuf,buflen,pos);
				pushbuf(' ');
			}
			if (m>=20) {
				pushstr(dec_por[m/10]);
				pushstr(dec_fin[mode]);
				if (m%10) pushbuf(' ');
				m%=10;
			}
			if (m==2 && mode == MILDIT_FM) {
				pushstr("druga");
			}
			else if (m==2 && mode == MILDIT_FB) {
				pushstr("drug±");
			}
			else if (m==2 && mode == MILDIT_AP) {
				pushstr("drugo");
			}
			else if (m) {
				char *c=dec_fin[mode];
				pushstr(jed_por[m]);
				if ((m==2 || m==3) && *c=='y') c++;
				if (*c) pushstr(c);
			}
			return pos;
		}
		m=n%1000;
		if (m) {
			if (n>1000) {
				pos=milena_integer(n-m,outbuf,buflen,pos);
				pushbuf(' ');
			}
			m/=100;
			if (m>1) pushstr(hun_por[m]);
			pushstr("setn");
			pushstr(dec_fin[mode]);
			return pos;
		}
		if (n==1000) {
			if (mode == MILDIT_AP) {
				pushstr("tysi±c");
			}
			else {
				pushstr("tysiêczn");
				pushstr(dec_fin[mode]);
			}
			return pos;
		}
		m=n/1000;
		pos=milena_speak_digit(m,MILDIT_AC,outbuf,buflen,pos);
		pushstr("tysiêczn");
		pushstr(dec_fin[mode]);
		return pos;
	}

	if (!n && (mode == MILDIT_FX || mode == MILDIT_MX)) {
		pushstr("zero ");
		return pos;
	}
	switch(mode) {
		case MILDIT_F0: if (n<10) pushstr("zero ");
		case MILDIT_FX:
			m=n%100;
			if (m==12 || m%10!=2) return milena_integer(n,outbuf,buflen,pos);
			if (m!=2) {
				pos=milena_integer(n-2,outbuf,buflen,pos);
				pushbuf(' ');
			}
			pushstr("dwie");
			return pos;
		default: break;
	}
	if (mode == MILDIT_AC) {
		m=n%1000;
		if (!n) {
			pushstr("zero");
			return pos;
		}
		if (m) {
			if (n>1000) {
				pos=milena_integer(n-m,outbuf,buflen,pos);
				pushbuf(' ');
			}
			if (!m%100) {
				pushstr(hun_bac[m/100]);
				return pos;
			}
			if (m>=100) pushstr(hun_pac[m/100]);
			m%=100;
			if (!m) return pos;
			if (m<=10) {
				pushstr(mil_por[m]);
				return pos;
			}
			if (m<20) {
				pushstr(jed_por[m]);
				pushbuf('o');
				return pos;
			}
			pushstr(dec_pac[m/10]);
			m%=10;
			if (m) pushstr(mil_por[m]);
			return pos;
		}
		return milena_integer(n,outbuf,buflen,pos);
	}
	return milena_integer(n,outbuf,buflen,pos);
}

struct recog_result {
	short name;
	short mode;
	int value;	/* n dla int/choice, len dla expr */
	char *str;
};

struct recog_choice_item {
	struct recog_choice_item *next;
	int value;
	char *str;
};

struct recog_choice {
	struct recog_choice *next;
	char *name;
	struct recog_choice_item *items;
	struct recog_choice_item *values;
};

#define RECFLAG_EOS 1
#define RECFLAG_ATEND 2
#define RECFLAG_UCASE 4
#define RECFLAG_ATSTART 8
#define RECFLAG_BEFORELOW 16


static int milena_sayhex(int value,int ndits,char *outbuf,int buflen,int pos)
{
	char buf[16];
	char *s;
	int i,nd,nh,blank,n,lsa;
	char *xchar[]={"zero","jeden","dwa","trzy","cztery","piêæ","sze¶æ","siedem","osiem","dziewiêæ","a","be","ce","de","e","ef"};
	if (ndits>0) {
		if (ndits>15) ndits=15;
		sprintf(buf,"%0*x",ndits,value);
	}
	else sprintf(buf,"%x",value);
	for (s=buf;*s && *s=='0';s++) {
		pushstr(xchar[0]);
		if (s[1]) {
			pushbuf(' ');
		}
	}
	for (i=nd=nh=0;s[i];i++) {
		if (isdigit(s[i])) nd++;
		else nh++;
	}
	if (!nh) {
		return milena_integer(strtol(s,NULL,10),outbuf,buflen,pos);
	}
	blank=1;lsa=0;
	for (;*s;) {
		while (*s && isdigit(*s)) {
			if (!blank) {
				pushbuf(' ');
			}
			blank=0;
			pushstr(xchar[*s-'0']);
			s++;
		}
		if (*s) {
			if (!blank) {
				pushbuf(' ');
			}
			blank=1;
			pushstr("[1]");
		}
		else continue;
		while (*s && !isdigit(*s)) {
			n=tolower(*s)-'a';
			if (n<0 || n>5) {s++;continue;}
			if (!blank && lsa && (n==0 || n>=4)) {

				pushstr("_");
			}
			pushstr(xchar[n+10]);
			blank=0;
			lsa = (n<5);
			s++;
		}
		blank=0;
	}
	return pos;
}

static int get_mildit_type(char *format,char **retpos)
{
	int ftyp=MILDIT_MX,j;
	static char *mr_mildit="mmfmnmmcfdmdfbmnappbmxfxf0acmtxnymyfsmsf";
	if (retpos) *retpos=NULL;
	for (j=0;mr_mildit[2*j];j++) if (*format==mr_mildit[2*j] && format[1]==mr_mildit[2*j+1]) {
		ftyp=j;
		if (retpos) *retpos=format+2;
		break;
	}
	return ftyp;
}

static int milena_sayas(struct milena *cfg,char *format,struct recog_result *res,int nres,char *outbuf,int buflen,int pos)
{
	int i;
	while (*format) {
                //printf("FMT <%s>\n",format);
		if (isspace(*format)) {
			pushbuf(' ');
			format++;
			while (*format && isspace(*format)) format++;
			continue;
		}
		if (*format!='{') {
			pushbuf(*format);
			format++;
			continue;
		}
		format++;
		for (i=0;i<nres;i++) if (res[i].name==*format) break;
		if (i>=nres) {
			format=strchr(format,'}');
			if (!format) return pos;
			format++;
			continue;
		}
		format++;
		if (*format==':') format++;
		if (res[i].mode == MILRGT_EXPR) {
			char *c=res[i].str;
			int j;
			for (j=0;j<res[i].value;j++) {
				char d=lci[c[j] & 255];
				if (!d) d=c[j];
				pushbuf(d);
			}
		}
		else if (res[i].mode == MILRGT_CHOICE && *format!='d' && *format!='u') {
			char *c=res[i].str;
			for (;*c;c++) {
				char d=lci[(*c) & 255];
				if (!d) d=*c;
				pushbuf(d);
			}
		}
		else if (*format=='x') {
			int ndit=0;
			if (format[1] && isdigit(format[1])) {
				ndit=strtol(format+1,&format,10);
			}
			pos=milena_sayhex(res[i].value,ndit,outbuf,buflen,pos);
		}
		else if (*format=='u') {
			char *c;
			format++;
                        if (res[i].mode == MILRGT_FLOAT) {
                                int n;
                                c=res[i].str;
                                milena_digitize(&c,NULL,0,0,0,&n,cfg,0);
                                c=milena_get_unit(cfg,&format,n,NULL,1);
                        }
                        else {
                                c=milena_get_unit(cfg,&format,what_dit(res[i].value),NULL,1);
                        }
			if (c) {
				pushstr(c);
			}
			else {
				pushstr(format);
			}


		}
                else if (*format == 'f') {
                        char *c=res[i].str;
                        pos=milena_digitize(&c,outbuf,buflen,pos,0,NULL,cfg,0);
                }
		else {
			int j,ftyp=MILDIT_MX;
			if (*format=='d') {
				format++;
				ftyp=get_mildit_type(format,NULL);
			}
			pos=milena_speak_digit(
					res[i].value,ftyp,outbuf,buflen,pos);

		}
		format=strchr(format,'}');
		if (!format) break;
		format++;
	}
	return pos;
}

static int milena_hexadec(char **str,int ndits)
{
	char *s;
	int val;
	if (!**str || !isxdigit(**str)) return -1;
	val=strtol(*str,&s,16);
	if (ndits && (s - *str)!=ndits) return -1;
	*str=s;
	return val;
}

static int milena_roman(char **str,int allow_lower)
{
	char *s=*str;
	int rc;
	int is_char(int c1,int c2)
	{
		if (c1 == c2) return 1;
		if (allow_lower && c1 == tolower(c2)) return 1;
		return 0;
	}
	int rta(char *sa)
	{
		int n=0;
		if (is_char(*s,*sa)) {
			s++;
			if (is_char(*s,sa[1])) {
				s++;
				return 4;
			}
			if (is_char(*s,sa[2])) {
				s++;
				return 9;
			}
			for (n=1;n<3 && is_char(*s,*sa);n++,s++);
			return n;
		}
		if (!is_char(*s,sa[1])) return 0;
		s++;
		for (n=5;n<8 && is_char(*s,*sa);n++,s++);
		return n;
	}
	for (rc=0;rc<3000 && is_char(*s,'M');s++,rc+=1000);
	rc+=100*rta("CDM");
	rc+=10*rta("XLC");
	rc+=rta("IVX");
	*str=s;
	return rc;
}

/*

wzorce dla recognize_rest:
\znak - znak as is
spacja - spacje
_ - spacje lub pusto
* - dowolna litera, my¶lnik, apostrof, powielone
# - koniec przetwarzania
^ - du¿a litera
[literki] - jedna z literek
@ - samog³oska
$ - spó³g³oska
() - nawiasowanie
| alternatywa
pozosta³e - as is



zapis powinien byæ:
frest nazwa:cia³ofunkcji - deklaracja
{:nazwa} - przy³o¿enie
{e:nazwa} - expr
{c:nazwa} - choice
frest nazwiskomd:_^*([sc]kiego|aka)-


*/


static int milena_rest_cstr(struct milena *cfg,char *patr,char **ss)
{
	char *st=*ss;
	int eqchar(int c1,int c2)
	{
		int t1=lci[c1 & 255],t2;
		if (t1) t2=lci[c2 & 255];
		else {
			t1=c1 & 255;
			t2=c2 & 255;
		}
		return t1 == t2;
	}

	int is_spg(char c)
	{
		char *s;
		c=lci[c & 255];
		if (!c) return 0;
		if (strchr("aeiouy±êóüöäëáéíúý@",c)) return 0;
		s=cfg->redef[c & 255];
		if (!s) return 1;
		for (;*s;s++) if (strchr("aeiouy±êóüöäëáéíúý@",*s)) return 0;
		return 1;
	}

	for (;*patr;patr++) {
		//if (*patr=='-' && !patr[1]) {
		//	if (!*st) break;
		//	if (!good_char(cfg,*st)) break;
		//	return 0;
		//}
		if (*patr == '\\' && patr[1]) {
			patr++;
			if (*patr != *st++) return 0;
			continue;
		}
		if (*patr=='_') {
			while (*st && isspace(*st)) st++;
			continue;
		}
		if (*patr == '+') {
			while (*st && lci[(*st) & 255]) st++;
			continue;
		}
		if (!*st) return 0;
		if (*patr=='^') {
			if (!uci[(*st++) & 255]) return 0;
			continue;
		}
		if (*patr=='0') {
			if (!isdigit((*st++) & 255)) return 0;
			continue;
		}
		if (*patr=='@') {
			if (!is_samog(cfg,*st++)) return 0;
			continue;
		}
		if (*patr=='$') {
			if (!is_spg(*st++)) return 0;
			continue;
		}
		if (*patr=='?') {
			if (uci[(*st++) & 255]) return 0;
			continue;
		}
		if (*patr=='[') {
			int fnd=0;
			for (;;) {
				patr++;
				if (!*patr || *patr==']') return 0;
				if (eqchar(*patr,*st)) {
					fnd=1;
					break;
				}
			}
			if (!fnd) return 0;
			st++;
			patr=strchr(patr,']');
			if (!patr) break;
			continue;
		}
		if (!eqchar(*patr,*st++)) return 0;
	}
	*ss=st;
	return 1;
}




static int milena_phrase_startmode(char *str)
{
	int f=0;
	char *c=str;
	if (milena_roman(&c,0)) return MIFRAT_ROMAN;
	c=str;
	if (milena_hexadec(&c,0)>=0) f |= MIFRAT_HEXA;
	if (isdigit(*str)) f |= MIFRAT_DECI;
	return f;
}

static char *milena_recognize_format(struct milena *cfg,char *str,
	struct recog_format *mft,struct recog_result *res,int *nres,int smode,int pos)
{
	char *s=mft->str;
	char *c,d,fbuf[32];
	int ndit,minval,maxval,mode,value;
	struct recog_expression *expr;
	struct recog_choice *choice;
	struct recog_choice_item *chitem,*chival;

	int compare_char(char pchar,char schar)
	{
		if (pchar == schar) return 1;
		if (uci[pchar & 255]) return 0;
		schar=lci[schar & 255];
		if (pchar == schar) return 1;
		return 0;
	}

	char *compare_expr(char *patr,char *str)
	{
		int i;
		//fprintf(stderr,"Compare expr [%s] with [%s]\n",patr,str);
		while (*patr) {
			if (isspace(*patr)) {
				patr++;
				if (!*str || !isspace(*str)) return NULL;
				while (*str && isspace(*str)) str++;
				continue;
			}
			if (*patr=='_') {
				patr++;
				while (*str && isspace(*str)) str++;
				continue;
			}
			if (*patr!='(') {
				if (!compare_char(*patr++,*str++)) return NULL;
				continue;
			}
			//fprintf(stderr,"Compare %s with %s\n",str,patr);
			patr++;
			for (;;) {
				for (i=0;patr[i] && patr[i]!='|' && patr[i]!=')';i++) {
					if (!compare_char(patr[i],str[i])) break;
				}
				if (!patr[i] || patr[i]=='|' || patr[i]==')') {
					if (!good_char(cfg,str[i])) {
						str+=i;
						break;
					}
				}
				while (*patr && *patr!='|' && *patr!=')') patr++;
				if (*patr++!='|') return NULL;
			}
			patr=strchr(patr,')');
			if (!patr) return str;
			patr++;
			//fprintf(stderr, "Comparison OK [%s] [%s]\n",patr,str);
		}
		return str;
	}

	char *skip_word(char *str)
	{
	    for (;*str;str++) {
		if (isdigit(*str)) return NULL;
		if (lci[(*str) & 255]) continue;
		if (isdigit(*str)) return NULL;
		break;
	    }
	    return str;
	}
	
	char *compare_morfexpr(struct recog_morfexpr *mexpr,char *str)
	{
#ifdef HAVE_MORFOLOGIK
	    struct milena_stringlist *word;
	    char *compare_word(char *word,char *s)
	    {
		while (*word) {
		    if (!*s) return NULL;
		    if (!compare_char(*word++,*s++)) return NULL;
		}
		if (*s) {
		    if (isdigit(*s) || lci[(*s) & 255]) return NULL;
		}
		return s;
	    }
	    if (!mexpr->str && !mexpr->words) return NULL;
	    if (!mexpr->words) {
		mexpr->words=morf_compile_expression(cfg,mexpr->str,mexpr->filter);
		mexpr->str=NULL;
		if (!mexpr->words) return NULL;
	    }
	    for (word=mexpr->words;word;word=word->next) {
		char *s=compare_word(word->string,str);
		if (s) return s;
	    }
	    
#endif	    
	    return NULL;
	}

	auto int rest_function(char **str,char *fname,int level);
	int rest_compare(char **str,struct restcognizer *rc)
	{
		struct recog_expression *expr;
		struct recog_choice *choice;
		char *ss=*str;
		char *c;
		for (;rc;rc=rc->next) {
#ifdef HAVE_MORFOLOGIK
			if (rc->command == RESTCMD_PHRASE) {
				u_int64_t gramas[16];
				int ngram,i;
				ngram=morf_get_subst_form(cfg,ss,gramas);
				//fprintf(stderr,"ESS [%s] %d\n",ss,ngram);
				if (!ngram) return 0;
				for (i=0;i<ngram;i++) {
					u_int64_t sum=gramas[i] & rc->u.grama;
					if (!(sum & WM_NUM_MASK)) continue;
					//fprintf(stderr,"%016Xll",sum);
					if ((rc->u.grama & WM_CASU_MASK) && !(sum & WM_CASU_MASK)) continue;
					if ((rc->u.grama & WM_GENR_MASK) &&!(sum & WM_GENR_MASK)) continue;
					return -1;
				}
				return 0;
			}
#endif
			if (rc->command == RESTCMD_HASH) {
				return -1;
			}
			if (rc->command == RESTCMD_STAR) {
				char *rst=ss;
				if (!rc->next) {
				    while (*rst && good_char(cfg,*rst)) rst++;
				    *str=rst;
				    return 1;
				}
				if (*rst && good_char(cfg,*rst)) {
					while (rst[1] &&  (good_char(cfg,rst[1]) || rst[1]=='\'' || rst[1]=='-')) {
						rst++;
					}
				}
				while (rst >= ss) {
					int nrt=rest_compare(&rst,rc->next);
					if (nrt) {
						*str=rst;
						return nrt;
					}
					rst--;
				}
				return 0;
			}
			if (rc->command == RESTCMD_ALT) {
				if (rest_compare(&ss,rc->u.alt.left)) continue;
				if (!rest_compare(&ss,rc->u.alt.rite)) return 0;
				continue;
			}
			if (rc->command == RESTCMD_STR) {
				c=rc->u.str;
				if (!c) continue;
				if (!milena_rest_cstr(cfg,c,&ss)) return 0;
				continue;
			}
			if (rc->command == RESTCMD_FUN) {
				c=rc->u.str;
				if (!c || !*c) continue;
				if (!rest_function(&ss,c,1)) return 0;
				continue;
			}
			if (rc->command == RESTCMD_CHOICE) {
				if (!rc->u.str || !*rc->u.str) continue;
				for (choice=cfg->choices;choice;choice=choice->next) if (!strcmp(choice->name,rc->u.str)) break;
				if (!choice) return 0;
				for (chitem=choice->items;chitem;chitem=chitem->next) {
					if (c=compare_expr(chitem->str,ss)) break;
				}
				if (!chitem) return 0;
				ss=c;
				continue;
			}
			if (rc->command == RESTCMD_EXPR) {
				if (!rc->u.str || !*rc->u.str) continue;
				for (expr=cfg->recexpr;expr;expr=expr->next) {
					if (strcmp(expr->name,rc->u.str)) continue;
					c=compare_expr(expr->str,ss);
					if (c) break;
				}
				if (!expr) return 0;
				ss=c;
				continue;
			}
			return 0; //nie obslugiwana komenda
		}
		*str=ss;
		return 1;
	}

	int rest_function(char **str,char *fname,int level)
	{
		struct restcogfun *fun;
		struct restcognizer *rca=NULL;
		int rc;
		for (fun=cfg->restcog_funs;fun;fun=fun->next) {
			if (!strcmp(fun->name,fname)) {
				rca=fun->body;
				if (!rca) continue;
				if (!(rc=rest_compare(str,rca))) continue;
				if (rc == -1) return 1;
				if (!level && good_char(cfg,**str)) continue;
				return 1;
			}
		}
		return 0;
	}

	int recognize_right_context(char *pat,char *str)
	{
		char pname[64],*c;
		strcpy(pname,pat+2);
		c=strchr(pname,'}');
		if (c) *c=0;
		return rest_function(&str,pname,0);
	}

//#undef rest_function
//#undef rest_compare

	
	*nres=0;
	if (mft->flags & RECFLAG_ATSTART) {
	    if (pos) return NULL;
	}
	if (*s=='{' && islower(s[1]) && s[2]==':') {
		if (s[3]=='r' && !(smode & MIFRAT_ROMAN)) return NULL;
		if (s[3]=='d' && !(smode & MIFRAT_DECI)) return NULL;
		if (s[3]=='x' && !(smode & MIFRAT_HEXA)) return NULL;
	}
	if (mft->flags & RECFLAG_UCASE) {
		if (!uci[(*str)& 255]) return NULL;
	}
	char *delayed_string=NULL;
	char delayed_name[64];
	while (*s) {
		//fprintf(stderr,"PP [%s] [%s]\n",s,str);
		if (*s=='\\') {
			s++;
			if (!*s) break;
			if (isspace(*s)) {
				while (*str && isspace(*str)) str++;
				while (*s && isspace(*s)) s++;
				continue;
			}
			d=lci[(*str) & 255];
			if (!d) d=*str;
			if (d==*s) str++;

			s++;
			continue;
		}
		if (*s!='{') {
			if (isspace(*s)) {
				if (!*str || !isspace(*str)) return NULL;
				while (*str && isspace(*str)) str++;
				while(*s && isspace(*s)) s++;
				continue;
			}
			if (*str == *s) {
				str++;
				s++;
				continue;
			}
			d=lci[(*str) & 255];
			if (!d) d=*str;
			if (d!=*s) return NULL;
			s++;str++;
			continue;
		}
		if (s[1]==':') {
			if (!recognize_right_context(s,str)) return NULL;
			break;
		}
		s++;
		if (!*s) return NULL;
		res[(*nres)].name=*s++;
		ndit=0;
		if (*s==':') s++;
		if (*s && isdigit(*s)) ndit=(*s++)-'0';
		mode=*s++;
		if (!mode || !strchr("drecxmf",mode)) return NULL;
                if (mode == 'f') {
                        if (*s++ != '}') return NULL;
                        c=str;
                        if (!*c || !isdigit(*c++)) return NULL;
                        while (*c && isdigit(*c)) c++;
                        if (*c == '.' || *c == ',') {
                                c++;
                                if (!*c || !isdigit(*c)) return NULL;
                                while (*c && isdigit(*c)) c++;
                        }
                        //printf("Mode is f for %s\n",str);
                        res[(*nres)].value = c-str;
                        res[(*nres)].mode = MILRGT_FLOAT;
                        res[(*nres)++].str=str;
                        str=c;
                        continue;
                }
		if (mode == 'x') {
			if (*s++!='}') return NULL;
			value=milena_hexadec(&str,ndit);
			if (value < 0) return NULL;
			res[(*nres)].value=value;
			res[(*nres)++].mode=0;
			continue;
		}
		if (mode=='r' || mode=='d') {
			minval=0;
			maxval=999999;
			if (isdigit(*s)) {
				minval=strtol(s,&s,10);
			}
			if (*s=='-' && isdigit(s[1])) {
				maxval=strtol(s+1,&s,10);
			}
			if (*s++!='}') return NULL;
			if (mode=='r') {
				value=milena_roman(&str,0);
				if (!value || value<minval || value>maxval) return NULL;
				res[(*nres)].value=value;
				res[(*nres)++].mode=0;
				continue;
			}
			if (!isdigit(*str)) return NULL;
			value=strtol(str,&c,10);
			if (value<minval || value>maxval) return NULL;
			if (ndit) {
				if (str+ndit != c) return NULL;
			}
			res[(*nres)].value=value;
			res[(*nres)++].mode=0;
			str=c;
			continue;
		}
		for (c=fbuf;*s && *s!='}';) *c++=*s++;
		if (*s++!='}') return NULL;
		*c=0;
		if (!*fbuf) return NULL;
		if (mode == 'm') {
		    struct recog_morfexpr *me;
#ifdef HAVE_MORFOLOGIK
		    if (!cfg->md) return NULL;
		    c=skip_word(str);
		    if (!c) return NULL;
		    if (!delayed_string && strlen(fbuf) < 64) {
			delayed_string=str;
			strcpy(delayed_name,fbuf);
			//fprintf(stderr,"Delay [%s] %s\n",delayed_name,delayed_string);
		    }
		    else {
			for (me=cfg->morfexpr;me;me=me->next) {
			    if (strcmp(me->name,fbuf)) continue;
			    c=compare_morfexpr(me,str);
			    if (c) break;
			}
			if (!me) return NULL;
		    }
		    res[(*nres)].value=c-str;
		    res[(*nres)].str=str;
		    str=c;
		    res[(*nres)++].mode=MILRGT_EXPR;
		    continue;
#else
		    return NULL;
#endif
		}
		if (mode=='e') {
			for (expr=cfg->recexpr;expr;expr=expr->next) {
				if (strcmp(expr->name,fbuf)) continue;
				c=compare_expr(expr->str,str);
				if (c) break;
			}
			if (!expr) return NULL;
			res[(*nres)].value=c-str;
			res[(*nres)].str=str;
			str=c;
			res[(*nres)++].mode=MILRGT_EXPR;
			continue;

		}
		for (choice=cfg->choices;choice;choice=choice->next) if (!strcmp(choice->name,fbuf)) break;
		if (!choice) return NULL;
		for (chitem=choice->items;chitem;chitem=chitem->next) {
			c=compare_expr(chitem->str,str);
			if (c) break;
		}
		if (!chitem) return NULL;
		str=c;
		for (chival=choice->values;chival;chival=chival->next) if (chival->value==chitem->value) break;
		res[(*nres)].value=chitem->value;
		if (chival) {
			res[(*nres)].str=chival->str;
			res[(*nres)++].mode=MILRGT_CHOICE;
		}
		else {
			res[(*nres)++].mode=0;
		}
	}
	if (*str) {
		if (lci[(*str) & 255]) return NULL;
		if (strchr(".'-/,",*str) && str[1] && lci[str[1]&255]) return NULL;
	}
	if (mft->flags & RECFLAG_ATEND) {
		if (!phraser_at_end(cfg,str,1)) return NULL;
	}
	if (mft->flags & RECFLAG_EOS) {
		if (*(str-1)=='.') {
			c=str;
			c++;
			while (*c && isspace(*c)) c++;
			if (*c && uci[(*c) & 255]) str--;
		}
	}
	if (mft->flags & RECFLAG_BEFORELOW) {
	    c=str;
	    while (*c && isspace(*c)) c++;
	    if (!*c || isdigit(*c) || !lci[(*c) & 255] || uci[(*c) & 255]) return NULL;
	}
#ifdef HAVE_MORFOLOGIK
	if (delayed_string && delayed_name) {
	    struct recog_morfexpr *me;
	    for (me=cfg->morfexpr;me;me=me->next) {
		if (strcmp(me->name,delayed_name)) continue;
		//fprintf(stderr,"Delayed [%s] string %s\n",me->name,delayed_string);
		c=compare_morfexpr(me,delayed_string);
		//fprintf(stderr,"Result is [%s]\n",c?c:"chujnia");
		if (c) break;
	    }
	    if (!me) return NULL;
	}
#endif
	return str;
}

static int milena_recognize(struct milena *cfg,char **str,char *outbuf,int buflen,int pos)
{
	char *c;
	struct recog_result res[16];
	int nres;
	struct recog_format *mft;
	struct recog_sayas *sya;
	int smode=milena_phrase_startmode(*str);
	//fprintf(stderr,"RECOG %s",*str);
	for (mft=cfg->recs;mft;mft=mft->next) {
		c=milena_recognize_format(cfg,*str,mft,res,&nres,smode,pos);
		if (c) {
			//fprintf(stderr,"Recognizewd %s\n",mft->name);
			for (sya=cfg->sayas;sya;sya=sya->next) if (!strcmp(sya->name,mft->name)) break;
			if (!sya) continue;
			if (pos) pushbuf(' ');
			pos=milena_sayas(cfg,sya->format,res,nres,outbuf,buflen,pos);
			*str=c;
			return pos;
		}
	}
    //(stderr,"No rec\n");
	return -1;
}

static int milena_SkipRecognizedPart(struct milena *cfg,char **str,int bof)
{
	char *c;
	struct recog_result res[16];
	int nres;
	struct recog_format *mft;
	struct recog_sayas *sya;
	int smode=milena_phrase_startmode(*str);
	for (mft=cfg->recs;mft;mft=mft->next) {
		c=milena_recognize_format(cfg,*str,mft,res,&nres,smode,!bof);
		if (c) {
			for (sya=cfg->sayas;sya;sya=sya->next) if (!strcmp(sya->name,mft->name)) break;
			if (!sya) continue;
			*str=c;
			return 1;
		}
	}
	return 0;
}


static int milena_init_recg(struct milena *cfg)
{
	int i;
	struct recog_choice *rc;
	struct recog_choice_item *rci;
	if (cfg->choices) return 1;
	rc=qalloc(sizeof(*rc));
	rc->next=NULL;
	rc->name="mond";
	cfg->choices=rc;
	rc->items=NULL;
	rc->values=NULL;
	for (i=1;i<=12;i++) {
		rci=qalloc(sizeof(*rci));
		rci->next=rc->items;
		rc->items=rci;
		rci->value=i;
		rci->str=milena_mons[i];
		rci=qalloc(sizeof(*rci));
		rci->next=rc->values;
		rc->values=rci;
		rci->value=i;
		rci->str=milena_mons[i];
	}

}

static struct restcognizer *compile_rest(struct milena *rest,char **str,char **error);
static struct restcognizer *compile_rest_item(struct milena *cfg,char **str,char **error)
{
	struct restcognizer *rc,**rd,*rout;
	char *ustr;
	if (!**str) return NULL;

	if (**str=='(') {
		rout=NULL;
		rd=&rout;
		for (;;) {
			(*str)++;
			rc=compile_rest(cfg,str,error);
			if (*error) return NULL;
			if ((**str) != '|') {
				if (**str) (*str)++;
				*rd=rc;
				return rout;
			}
			*rd=qalloc(sizeof(*rc));
			(*rd)->next=NULL;
			(*rd)->command=RESTCMD_ALT;
			(*rd)->u.alt.left=rc;
			(*rd)->u.alt.rite=NULL;
			rd=&(*rd)->u.alt.rite;
		}
	}
	if (**str=='*') {
		(*str)++;
		rc=qalloc(sizeof(*rc));
		rc->next=NULL;
		rc->command=RESTCMD_STAR;
		return rc;
	}
	if (**str=='#') {
		(*str)++;
		rc=qalloc(sizeof(*rc));
		rc->next=NULL;
		rc->command=RESTCMD_HASH;
		return rc;
	}
	if (**str=='{') {
		int cmd=RESTCMD_FUN;
		(*str)++;
		if (**str == 'c') {
			(*str)++;
			cmd=RESTCMD_CHOICE;
		}
		else if (**str == 'e') {
			(*str)++;
			cmd=RESTCMD_EXPR;
		}
		if (*(*str)++ != ':') {
			*error="Brak dwukropka w dopasowaniu prawostronnym";
			return NULL;
		}
		ustr=strchr(*str,'}');
		if (!ustr || ustr == *str) {
			*error="Brak zakonczenia dopasowania prawostronnego";
			return NULL;
		}
		rc=qalloc(sizeof(*rc));
		rc->next=NULL;
		rc->command=cmd;
		rc->u.str=mStrnDup(cfg,*str,ustr-*str);
		*str=ustr+1;
		return rc;
	}
	ustr=strpbrk(*str,"()|*{#");
	if (!ustr) ustr=*str+strlen(*str);
	rc=qalloc(sizeof(*rc));
	rc->next=NULL;
	rc->command=RESTCMD_STR;
	rc->u.str=mStrnDup(cfg,*str,ustr-(*str));
	*str=ustr;
	return rc;
}

static struct restcognizer *compile_rest_morf(struct milena *cfg,char **str,char **error)
{
#ifdef HAVE_MORFOLOGIK
	static struct {
		char *s;
		u_int64_t v;
	} ns[]={
		{"nom",WM_nom},
		{"gen",WM_gen},
		{"dat",WM_dat},
		{"acc",WM_acc},
		{"inst",WM_inst},
		{"loc",WM_loc},
		{"voc",WM_voc},
		{"m1",WM_m1},
		{"m2",WM_m2},
		{"m3",WM_m3},
		{"m",WM_m1 | WM_m2 | WM_m3},
		{"f",WM_f},
		{"n1",WM_n1},
		{"n2",WM_n2},
		{"n",WM_n2 | WM_n1},
		{NULL,0}};
		
		
	u_int64_t grama = 0;
	int i;
	struct restcognizer *rc;
	
	(*str)++;
	if (!strncmp(*str,"sg",2)) grama = WM_sg;
	else if (!strncmp(*str,"pl",2)) grama = WM_pl;
	else {
		if (error) *error="Nieprawidlowa liczba (nie sg/pl)";
		return NULL;
	}
	(*str) += 2;
	for (;**str;) {
		
		if (**str != '.' && **str != ':') {
			if (error) *error="Nieprawidlowa gramatyka (1)";
			return NULL;
		}
		(*str)++;
		for (i=0;ns[i].s;i++) if (!strncmp(*str,ns[i].s,strlen(ns[i].s))) break;
		if (!ns[i].s) {
			if (error) *error="Nieprawidlowa gramatyka (2)";
			return NULL;
		}
		grama |= ns[i].v;
		(*str) += strlen(ns[i].s);
	}
	rc=qalloc(sizeof(*rc));
	rc->next=NULL;
	rc->command=RESTCMD_PHRASE;
	rc->u.grama = grama;
	return rc;
	
#endif
	return NULL;
}


static struct restcognizer *compile_rest(struct milena *cfg,char **str,char **error)
{
	struct restcognizer *rc,*rout,**rrc;
	rout=NULL;
	rrc=&rout;
	if (**str == '~') {
		return compile_rest_morf(cfg,str,error);
	}
	while (**str) {
		rc=compile_rest_item(cfg,str,error);
		if (*error) return NULL;
		*rrc=rc;
		rrc=&(rc->next);
		if (!**str || **str=='|' || **str==')') return rout;
	}
}

static char *milena_add_recog_fun(struct milena *cfg,char *rest)
{
	char *fname=rest,*err=NULL;
	struct restcogfun *fun;
	struct restcognizer *rc;
	for (;*rest;rest++) if (!islower(*rest)) break;
	if (rest==fname) return "Brak nazwy funkcji";
	if (*rest != ':') return "Brak dwukropka w funkcji";
	*rest++=0;
	rc=compile_rest(cfg,&rest,&err);
	if (err) return err;
	//if (!rc) return "Restcompiler zwraca NULL";
	if (!rc) return NULL;
	if (*rest) return "Wiszacy ogon funkcji";
	fun=qalloc(sizeof(*fun));
	fun->name=strdup(fname);
	fun->body=rc;
	fun->next=cfg->restcog_funs;
	cfg->restcog_funs=fun;
	return NULL;
}

static int milena_line_is_recog(char *cmd)
{
	static char *mrec[]={
		"recognize","sayas","expr","choice","choiceval","frest","number","mexpr","umexpr",NULL};
	int i;
	for (i=0;mrec[i];i++) if (!strcmp(cmd,mrec[i])) return 1;
	return 0;
}

static int milena_recog_line(struct milena *cfg,char *cmd,char *str,int from_dict)
{
	char *rest=str;
	/* from_dict nie mam pomys³u */
	if (!strcmp(cmd,"frest")) {
		char *c=milena_add_recog_fun(cfg,rest);
		if (c) {
			gerror(cfg,c);
		}
		return 1;
	}
	if (!strcmp(cmd,"number")) {
	    struct recog_number *rn;
	    char *c;int i;
	    static char *casa[]={"nom","gen","dat","acc","ins","loc","voc"};
	    while (*rest && isspace(*rest)) rest++;
	    rn=qalloc(sizeof(*rn));
	    rn->pred=0;
	    rn->format[0]=get_mildit_type(rest,&rest);
	    if (!rest) gerror(cfg,"B³êdny format liczby");
	    if (*rest++!=':') gerror(cfg,"B³êdny format liczby");
	    rn->format[1]=get_mildit_type(rest,&rest);
	    if (!rest) gerror(cfg,"B³êdny format liczby");
	    if (*rest++!=':') gerror(cfg,"B³êdny format liczby");
	    rn->format[2]=get_mildit_type(rest,&rest);
	    if (!rest) gerror(cfg,"B³êdny format liczby");
	    if (!isspace(*rest)) gerror(cfg,"B³êdny format liczby");
	    for (;;) {
		while (*rest && isspace(*rest)) rest++;
		if (!*rest) gerror(cfg,"Brak liczby");
		if (*rest == '#') break;
		if (rn->pred == 4) {
		    gerror(cfg,"Za d³ugi kontekst");
		}
		c=rest;
		while (*c && !isspace(*c)) c++;
		if (*c) *c++=0;
		rn->preds[rn->pred++]=strdup(rest);
		rest=c;
	    }
	    rest++;
	    rn->flags =0;
	    while (*rest && !isspace(*rest)) {
		int z=*rest++;
		if (z == '1') rn->flags |= RECNU_SGL;
		else if (z == 'd') rn->flags |= RECNU_DBL;
		else if (z == 'D') rn->flags |= RECNU_NDBL;
		else if (z == 't') rn->flags |= RECNU_SMAL;
		else if (z == 'T') rn->flags |= RECNU_BIG;
		else gerror(cfg,"nieznany modufikator");
	    }
	    while (*rest && isspace(*rest)) rest++;
	    if (*rest=='$' || !*rest) {
		rn->atend=1;
		if (*rest) rest++;
	    }
	    else {
		rn->atend=0;
		rn->num=0;
		rn->cas=0;
		if (!strncmp(rest,"pl:",3)) {
		    rn->num = WM_pl;
		    rest+=3;
		}
		else if (!strncmp(rest,"sg:",3)) {
		    rn->num = WM_sg;
		    rest+=3;
		}
		for (i=0;i<7;i++) if (!strncmp(casa[i],rest,3)) {
		    rn->cas=WM_nom << i;
		}
		if (!rn->cas) gerror(cfg,"Brak przypadku");
		while (*rest && !isspace(*rest)) rest++;
	    }
	    while (*rest && isspace(*rest)) rest++;
	    if (*rest) gerror(cfg,"Smieci na koncu linii");
	    rn->next=cfg->recog_numbers;
	    cfg->recog_numbers=rn;
	    return 1;
	}
	while (*rest && !isspace(*rest)) rest++;
	if (!*rest) gerror(cfg,"Blad skladni");
	*rest++=0;
	while (*rest && isspace(*rest)) rest++;
	if (!strcmp(cmd,"sayas")) {
		struct recog_sayas *sa;
		sa=qalloc(sizeof(*sa));
		sa->next=cfg->sayas;
		cfg->sayas=sa;
		sa->name=strdup(str);
		sa->format=strdup(rest);
		return 1;
	}
	if (!strcmp(cmd,"recognize")) {
		struct recog_format *ra;
		int flags=0;
		int atend=0;
		for (;*str;str++) {
			if (*str=='.') {
				flags |= RECFLAG_EOS;
				continue;
			}
			if (*str=='$') {
				flags |= RECFLAG_ATEND;
				continue;
			}
			if (*str=='@') {
				flags |= RECFLAG_ATSTART;
				continue;
			}
			if (*str=='^') {
				flags |= RECFLAG_UCASE;
				continue;
			}
			if (*str==';') {
			    flags |= RECFLAG_BEFORELOW;
			    continue;
			}
			if (*str=='+') {
				atend=1;
				continue;
			}
			break;
		}
		if (!*str) gerror(cfg,"Blad skladni");
		ra=qalloc(sizeof(*ra));
		if (atend) {
			struct recog_format **rra;
			for (rra=&cfg->recs;*rra;rra=&(*rra)->next);
			*rra=ra;
			ra->next=NULL;
		}
		else {
			ra->next=cfg->recs;
			cfg->recs=ra;
		}
		ra->flags = flags;
		ra->name=strdup(str);
		ra->str=strdup(rest);
		return 1;
	}
	if (!strcmp(cmd,"umexpr")) {
#ifdef HAVE_MORFOLOGIK
#if HAVE_MORFOLOGIK == 10
		struct recog_morfexpr *mexpr;
		struct milena_stringlist *sl=NULL,*nsl;
		char *word;
		if (!*rest) gerror(cfg,"Blad skladni");
		mexpr=malloc(sizeof(*mexpr));
		mexpr->name=strdup(str);
		mexpr->filter=0;
		mexpr->str=NULL;
		rest=strdup(rest);
		while (rest && *rest) {
			word=rest;
			rest=strchr(rest,'|');
			if (rest) *rest++=0;
			nsl=malloc(sizeof(*nsl));
			nsl->string=word;
			nsl->next=sl;
			sl=nsl;
		}
		mexpr->words=sl;
		mexpr->next=cfg->morfexpr;
		cfg->morfexpr=mexpr;
		
#endif
#endif
		return 1;
	}
	if (!strcmp(cmd,"mexpr")) {
#ifdef HAVE_MORFOLOGIK	    
#if HAVE_MORFOLOGIK == 20
	    struct recog_morfexpr *mexpr;
	    char *c;
	    u_int64_t filter=0;
	    c=strchr(str,'/');
	    if (c) {
		*c++=0;
		filter=
		morfologik_ParseGrama(c);
		if (!filter) gerror(cfg,"Blad filtra");
	    }
	    mexpr=malloc(sizeof(*mexpr));
	    mexpr->name=strdup(str);
	    mexpr->filter=filter;
	    mexpr->str=strdup(rest);
	    mexpr->words=NULL;
	    mexpr->next=cfg->morfexpr;
	    cfg->morfexpr=mexpr;
#endif
#endif
	    return 1;
	}
	if (!strcmp(cmd,"expr")) {
		struct recog_expression *expr;
		expr=malloc(sizeof(*expr));
		expr->next=cfg->recexpr;
		cfg->recexpr=expr;
		expr->name=strdup(str);
		sort_wordpar(rest);
		expr->str=strdup(rest);
		return 1;
	}
	if (!strcmp(cmd,"choice") || !strcmp(cmd,"choiceval")) {
		struct recog_choice *rc;
		struct recog_choice_item *rci;
		int val;
		if (!isdigit(*rest)) {
			gerror(cfg,"Oczekiwano liczby");
		}
		val=strtol(rest,&rest,10);
		while (*rest && isspace(*rest)) rest++;
		if (!*rest) {
			gerror(cfg,"Pusty choice");
		}
		for (rc=cfg->choices;rc;rc=rc->next) if (!strcmp(rc->name,str)) break;
		if (!rc) {
			rc=qalloc(sizeof(*rc));
			rc->next=cfg->choices;
			cfg->choices=rc;
			rc->items=NULL;
			rc->values=NULL;
			rc->name=strdup(str);
		}
		rci=qalloc(sizeof(*rci));
		rci->value=val;
		rci->str=strdup(rest);
		if (!strcmp(cmd,"choice")) {
			rci->next=rc->items;
			rc->items=rci;
		}
		else {
			rci->next=rc->values;
			rc->values=rci;
		}
		return 1;
	}
	return 0;

}
