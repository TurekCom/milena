/*
 * milena_phrasi.c - Milena TTS system (phraser)
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

static char *aps[]={
	"em","u","ów","om","ach","y",
	"ego","emu","a","owi","ym","ie",
	"owy","owego","owemu",
	"owej","owa","owe","owym","owych",
	"owymi","owie","ami","owi",
	"owski","owskiego","owskiemu",
	"owskim","owska","owsk±","owskiej",
	"owscy","owskie","owskich","owskimi","s",NULL};


static int good_char(struct milena *cfg,char c)
{
	if (isdigit(c)) return 1;
	c=lci[c & 255];
	if (!c) return 0;
	return cfg->letters[c&255] & 3;
}

static int is_samog(struct milena *cfg,char c)
{
	char *s;
	c=lci[c & 255];
	if (strchr("a±eêioóuyüí@áéúýöäãõâë",c)) return 1;
	s=cfg->redef[c & 255];
	if (!s) return 0;
	for (;*s;s++) if (strchr("a±eêioóuyüí@áéúýöäãõâë",*s)) return 1;
	return 0;
}


#define MLWORD_SPEAKABLE 1
#define MLWORD_NUMBER 2
#define MLWORD_UNSPEAK 3

/* MLWORD_NICKY - np. 'dupa23' */
#define MLWORD_NICKY 4


static char *cyfra[]={"zero","jeden","dwa","trzy","cztery",
		"piêæ","sze¶æ","siedem","osiem",
		"dziewiêæ","dziesiêæ","jedena¶cie",
		"dwana¶cie","trzyna¶cie","czterna¶cie",
		"piêtna¶cie","szesna¶cie","siedemna¶cie",
		"osiemna¶cie","dziewiêtna¶cie"};
static char *dzies[]={"","","dwadzie¶cia","trzydzie¶ci",
		"czterdzie¶ci","piêædziesi±t","sze¶ædziesi±t",
		"siedemdziesi±t","osiemdziesi±t","dziewiêædziesi±t"};
static char *setka[]={"zero","sto","dwie¶cie","trzysta","czterysta",
		"piêæset","sze¶æset","siedemset","osiemset","dziewiêæset"};

static char *dits_tys[]={"tysi±c","tysi±ce","tysiêcy"};
static char *dits_mln[]={"milion","miliony","milionów"};
static char *dits_mld[]={"miliard","miliardy","miliardów"};

static char *milena_get_unit(struct milena *cfg,char **str,int last_num,int *female,int hidden);

static int what_dit(int dit)
{
	if (dit==1) return 0;
	dit%=100;
	if (!dit) return 2;
	if (dit>19) dit%=10;
	if (dit>=2 && dit<=4) return 1;
	return 2;
}

static int milena_integer(int nr,char *outbuf,int buflen,int pos)
{

	int mln,tys,blank=0;
	void numeras(int n)
	{
		int setki=n/100,dz;
		n=n%100;
		if (setki) {
			if (blank) pushbuf(' ');
			blank=1;
			pushstr(setka[setki]);
		}
		if (!n) return;
		if (blank) pushbuf(' ');
		blank=1;
		if (n<20) {
			pushstr(cyfra[n]);
			return;
		}
		dz=n/10;
		n%=10;
		pushstr(dzies[dz]);
		if (!n) return;
		pushbuf(' ');
		pushstr(cyfra[n]);
	}

	mln=nr/1000000;
	nr=nr%1000000;


	if (mln) {
		if (mln) numeras(mln);
		if (blank) pushbuf(' ');blank=1;
		pushstr(dits_mln[what_dit(mln)]);
	}
	tys=nr/1000;
	nr=nr%1000;
	if (tys) {
		if(tys > 1 || mln) numeras(tys);
		if (blank) pushbuf(' ');blank=1;
		pushstr(dits_tys[what_dit(tys)]);
	}
	if (nr) numeras(nr);
	return pos;
}

static int milena_digitize(char **str,char *outbuf,int buflen,int pos,int spell,int *outmode,struct milena *cfg,int limit)
{
	int blank=0;
	int ndit;
	long int mld,mln,tys,nr;
	static char *dvs[]={"","","drug","trzeci","czwart","pi±t",
		"szóst","siódm","ósm","dziewi±t","dziesi±t",
		"jedenast","dwunast","trzynast","czternast",
		"piêtnast","szesnast"};

	void numerek(int n,int dits,int fema)
	{
		int setki=n/100,dz;
		if (!n && dits==1) {
			if (blank) pushbuf(' ');
			pushstr("zero");
			return;
		}
		n=n%100;
		if (!setki && dits>2) {
			if (blank) pushbuf(' ');
			pushstr("zero");
			if (n<10) {
				pushstr(" zero ");
				if (fema && n==2) {
					pushstr("dwie");
				}
				else {
					pushstr(cyfra[n]);
				}
				return;
			}
		}
		if (setki) {
			if (blank) pushbuf(' ');
			blank=1;
			pushstr(setka[setki]);
		}
		if (n<10 && !setki && dits==2) {
			if (blank) pushbuf(' ');
			blank=1;
			pushstr("zero ");
			if (fema && n==2) {
				pushstr("dwie");
			}
			else {
				pushstr(cyfra[n]);
			}
			return;
		}
		if (!n) return;
		if (blank) pushbuf(' ');
		blank=1;
		if (n<20) {
			if (fema && n==2) {
				pushstr("dwie");
			}
			else {
				pushstr(cyfra[n]);
			}
			return;
		}
		dz=n/10;
		n%=10;
		pushstr(dzies[dz]);
		if (!n) return;
		pushbuf(' ');
		if (fema && n==2) {
			pushstr("dwie");
		}
		else {
			pushstr(cyfra[n]);
		}
	}


	long int get_num(int ile)
	{
		long int nr=0;
		while (ile>0) {
			ile--;
			nr=nr*10L+ (*(*str)++) -'0';

		}
		return nr;
	}

	int is_frac(char *s)
	{
		int n;
		if (*s!='.' && *s!=',') return 0;
		s++;
		if (!*s || !isdigit(*s)) return 0;
		for (n=0;*s && isdigit(*s); s++,n++);
		if ((*s=='.' || *s==',') && s[1] && isdigit(s[1])) return 0;
		return n;
	}

	void digitize_frac(int nd)
	{

		(*str)++;
		pushstr(" przecinek");
		blank=1;
		if (nd<=3) {
			int n=get_num(nd);
			numerek(n,nd,0);
			return;
		}
		while (**str && isdigit(**str)) {
			int n=*(*str)++;
			pushbuf(' ');
			pushstr(cyfra[n&15]);
		}
	}
	int vulgar(void)
	{
		char *s=*str;
		int n1=strtol(s,&s,10),n2;
		if (*s++!='/') return 0;
		if (*s<'1' || *s>'9') return 0;
		n2=strtol(s,&s,10);
		if (n2<2 || (n2>10 && n2 != 16)) return 0;
		if (n1>=2*n2) return 0;
		if (*s && !isspace(*s)) {
			if (lci[(*s) & 255]) return 0;
			if (strchr("/.-",*s)) {
				if (lci[s[1] & 255]) return 0;
			}
		}
		*str=s;
		if (n1==1) {
			pushstr("jedna");
		}
		else {
			numerek(n1,0,1);
		}
		pushbuf(' ');
		pushstr(dvs[n2]);
		if (n1==1) {
			pushbuf('a');
			return 1;
		}
		if ((n1>=2 && n1<=4) || (n1 >=22 && n1<=24) || n1==32) {
			if (n2==2) {
				pushstr("ie");
			}
			else {
				pushbuf('e');
			}
			return 1;
		}
		if (n2==2) {
			pushstr("ich");
		}
		else {
			pushstr("ych");
		}
		return 1;
	}

	int is_ip_part(char **s)
	{
		int n;
		if (!isdigit(**s)) return -1;
		if (**s=='0') {
			(*s)++;
			if (!**s || !isdigit(**s)) return 0;
			return -1;
		}
		n=strtol(*s,s,10);
		if (n<256) return n;
		return -1;
	}

	void push_dot(int n1,int n2)
	{
		if (!n1 || !n2) return;
		if ((n1%100 == 0 && n2<100) ||
			(n2<100 && n1%10==0 && n1!=10 && n1!=110 && n1!=210)) {
			if (blank) pushbuf(' ');
			blank=1;
			pushstr("kropka");
		}
	}
	int is_ip(void)
	{
		char *s;
		int n1,n2,n3,n4;
		s=*str;
		n1=is_ip_part(&s);
		if (n1<0) return 0;
		if (*s++!='.') return 0;
		n2=is_ip_part(&s);
		if (n2<0) return 0;
		if (*s++!='.') return 0;
		n3=is_ip_part(&s);
		if (n3<0) return 0;
		if (*s++!='.') return 0;
		n4=is_ip_part(&s);
		if (n4<0) return 0;
		if (lci[(*s)&255]) return 0;
		if (*s=='.' && isdigit(s[1])) return 0;
		if (n1) numerek(n1,1,0);else {
			if(blank) pushbuf(' ');
			blank=1;
			pushstr("zero");
		}
		push_dot(n1,n2);
		if (n2) numerek(n2,1,0);
		else {
			if(blank) pushbuf(' ');
			blank=1;
			pushstr("zero");
		}
		push_dot(n2,n3);
		if (n3) numerek(n3,1,0);
		else {
			if(blank) pushbuf(' ');
			blank=1;
			pushstr("zero");
		}
		push_dot(n3,n4);
		if (n4) numerek(n4,1,0);
		else {
			if(blank) pushbuf(' ');
			blank=1;
			pushstr("zero");
		}
		*str=s;
		return 1;
	}
	int is_female_unit()
	{
		char *s=*str;
		int female=0;
		while (*s && isdigit(*s)) s++;
		if (*s=='.' && s[1] && isdigit(s[1])) {
			s+=2;
			while (*s && isdigit(*s)) s++;
		}
		if (!isspace(*s)) return 0;
		while (*s && isspace(*s)) s++;
		s=milena_get_unit(cfg,&s,0,&female,0);
		if (!s) return 0;
		return female;
	}

	int female=is_female_unit();
	if (!spell && is_ip()) {
		if (outmode) *outmode=-1;
		return pos;
	}

	while (**str=='0') {
		if (blank) pushbuf(' ');
		blank=1;
		pushstr("zero");
		(*str)++;
		if (**str!='.' && **str!=',') spell=1;
	}
	if (!spell) {
		if (vulgar()) {
			if (outmode) *outmode=3;
			return pos;
		}
	}

	for (ndit=0;(*str)[ndit] && isdigit((*str)[ndit]);ndit++);
	if (limit && cfg->digits_limit>0 && cfg->digits_limit<10) {
	    if (ndit > cfg->digits_limit) spell=1;
	}
	else {
	    if (ndit>10 || (ndit==10 && **str != '1')) spell=1;
	}
	if (outmode) *outmode=2;
	if (spell) {
		int sout;
		if (ndit<4) {
			numerek(sout=get_num(ndit),ndit,0);
			if (outmode) *outmode=what_dit(sout);
			return pos;
		}
		if (ndit==4) {
			numerek(get_num(2),2,0);
			numerek(get_num(2),2,0);
			return pos;
		}
		while(ndit>3) {
			numerek(get_num(2),2,0);
			ndit-=2;
		}
		numerek(get_num(ndit),ndit,0);
		return pos;
	}
	if (ndit) {
		nr=get_num(ndit);
		if (outmode) *outmode=what_dit(nr);
		if (female && nr == 1) {
			if (blank) pushbuf(' ');blank=1;
			pushstr("jedna");
		}
		else {
			mld=nr/1000000000L;
			nr=nr%1000000000L;
			if (mld) {
				if (mld) numerek(mld,0,0);
				if (blank) pushbuf(' ');blank=1;
				pushstr(dits_mld[what_dit(mld)]);
			}
			mln=nr/1000000;
			nr=nr%1000000;
			if (mln) {
				if (mln) numerek(mln,0,0);
				if (blank) pushbuf(' ');blank=1;
				pushstr(dits_mln[what_dit(mln)]);
			}
			tys=nr/1000;
			nr=nr%1000;
			if (tys) {
				if(tys > 1 || mln) numerek(tys,0,0);
				if (blank) pushbuf(' ');blank=1;
				pushstr(dits_tys[what_dit(tys)]);
			}
			if (nr) numerek(nr,0,female);
		}
	}
	if (!spell) {
		int nd;
		if ((nd=is_frac(*str))) {
			digitize_frac(nd);
			if (outmode) *outmode=3;
		}
	}
	return pos;
}


static char *milena_pseudo(struct milena *cfg,char **str)
{
	struct milena_pseudo *ps;

	char *compare_pseudo(char *s1,char *pat)
	{
		char dn;
		while (*pat) {
			char cn=lci[(*s1) & 255];
			if (cn != *pat) return NULL;
			pat++;
			s1++;
			if (*pat=='*') {
				pat++;
				while (lci[(*s1) & 255] == cn) s1++;
			}
		}
		if (!*s1) return s1;
		dn=lci[(*s1) & 255];
		if (!dn) return s1;
		if (isdigit(dn)) return NULL;
		if (cfg->letters[dn & 255] & 3) return NULL;
		return s1;
	}


	for (ps=cfg->pseudowords;ps;ps=ps->next) {
		char *s=compare_pseudo(*str,ps->pattern);
		if (s) {
			*str=s;
			return ps->result;
		}
	}
	return NULL;
}

static int milena_is_unit(struct milena *cfg,char *str,int mask)
{
    struct milena_nflex *nf;
    int i;
    
    int compare_form(char *str,char *form)
    {
	for (;*form;form++) {
	    if (!*str) return 0;
	    if (isspace(*form)) {
		if (!isspace(*str)) return 0;
		while (*str && isspace(*str)) str++;
		continue;
	    }
	    if (*str++ != *form) return 0;
	}
	if (good_char(cfg,*str)) return 0;
	return 1;
    }
    for (nf=cfg->nflex;nf;nf=nf->next) {
	int n=0;
	for (i=0;i<4;i++) {
	    if ((mask & (1<<i)) && compare_form(str,nf->result[i])) {
		n |= 1<<i;
	    }
	}
	if (n) {
	    if (nf->flags & MILENA_NFLEX_FEMALE) return n | 16;
	    return n;
	}
    }
    return 0;
}

static char *milena_get_unit(struct milena *cfg,char **str,int last_num,int *female,int hidden)
{
	struct milena_nflex *nf;

	char *compare_nflex(char *s1,char *pat)
	{
		char dn;
		while (*pat) {
			if (*s1 != *pat) return NULL;
			pat++;
			s1++;
		}
		if (!*s1) return s1;
		dn=lci[(*s1) & 255];
		if (!dn) return s1;
		if (isdigit(dn)) return NULL;
		if (cfg->letters[dn & 255] & 3) return NULL;
		return s1;
	}
	for (nf=cfg->nflex;nf;nf=nf->next) {
		if (!hidden && (nf->flags & MILENA_NFLEX_HIDDEN)) continue;
		char *s=compare_nflex(*str,nf->pattern);
		if (s) {
			*str=s;
			if (female) *female=nf->flags & MILENA_NFLEX_FEMALE ;
			return nf->result[last_num];
		}
	}
	return NULL;

}


static int milena_spell(struct milena *cfg,char **str,char *outbuf,int buflen,int pos,int last_num)
{
	int blank=0,lws=0,flet=0;
	char *p;
	char *spellsa[]={
		"a","be","c~'e","de","e","ef","gie","ha","i","jot",
		"ka","el","em","en","o","pe","ku","er","es","te",
		"u","fa³","wu","iks","igrek","zet"};
	char *spellsb[]={"³e³","¿¿et","¼zi","¶si","æci","ñni",NULL};
	char *find_spell(char c)
	{
		int i;
		if (c>='a' && c<='z') return spellsa[c-'a'];
		for (i=0;spellsb[i];i++) if(spellsb[i][0]==c) return spellsb[i]+1;
		return NULL;
	}


	if ((p=milena_pseudo(cfg,str))) {
		pushstr(p);
		return pos;
	}
	for (;**str;) {
		char cx,*cy,*c;
		int i,stres;
		if (!(cfg->phraser_mode & MILENA_PHR_IGNORE_TILDE) && **str=='~' && strchr(ESCHAR,(*str)[1])) {
			(*str)+=2;
			continue;
		}
		if (isdigit(**str)) {
			if (blank) pushbuf(' ');
			pos=milena_digitize(str,outbuf,buflen,pos,1,NULL,cfg,0);
			blank=1;lws=flet=0;
			continue;
		}
		cx=lci[(**str) & 255];
		if (!cx) break;
		(*str)++;
		if (find_spell(cx)) {
			stres=1;
			if (cx == 'y') stres=2;
			for (i=0;(*str)[i];i++) {
				char cz=lci[(*str)[i] & 255];
				if (cz=='y') stres=2;
				else if (find_spell(cz)) stres=1;
				else break;
			}
			if (blank) pushbuf(' ');
			if (stres==1) {
				pushstr("[1]");
			}
			else {
				pushstr("[2]");
			}
			lws=0;
			cy=find_spell(cx);
			for(;;) {
				if (lws) {
					if (((lws=='s' || lws=='r' || lws=='t' || lws == 'k') && (*cy=='z' || *cy=='h')) ||
					    (strchr("ns",lws) && *cy=='i')) {
					    	pushstr("~'");
					}
					else if (strchr("aeiour",lws) && strchr("aeiou",*cy)) {
						pushbuf('_');
					}
					else if (lws == 't' && *cy=='t') {
						pushstr("~'");
					}
					else if (lws == 'n' && *cy=='n') {
						pushstr("_~'");
					}
					else if (strchr("aeiou",lws) && !strchr("aeiou",*cy)) {
						pushstr("~'");
					}
				}
				pushstr(cy);
				while (cy[1]) cy++;
				lws=*cy;
				cx=lci[(**str) & 255];
				cy=find_spell(cx);
				if (!cy) break;
				(*str)++;
			}
			blank=1;
			continue;
		}
		if(blank) pushbuf(' ');
		if (cx=='±') c="~!a z ogonkiem";
		else if (cx=='ê') c="~!e z ogonkiem";
		else if (cx=='ó') c="~!o z kresk±";
		else c="litera";
		pushstr(c);
		blank=1;
	}
	return pos;
}

struct phraser_dic_rc {
	char *str;
	int len;
};

static int phraser_CompareDic(struct milena *cfg,char *patr,char **ostr,struct phraser_dic_rc *array,int *onph)
{
	char a_char(char t)
	{
		char tc;
		tc=lci[t & 255];
		if (tc) return tc;
		return t;
	}

	int compare_char(char pchar,char schar)
	{
		if (pchar == schar) return 1;
		if (uci[schar & 255]) return 0;
		pchar=lci[pchar & 255];
		if (pchar == schar) return 1;
		return 0;
	}

	char *str=*ostr;
	int nphr=0;
	while (*patr) {
		char cpat=*patr++;
		if (*str=='~' && !(cfg->phraser_mode & MILENA_PHR_IGNORE_TILDE) && strchr(ESCHAR,str[1])) return 0;
		if (cpat=='\\') {
			if (*patr++ == *str) str++;
			continue;
		}
		if (cpat=='\'') {
			if (*str!='\'' && *str != '`') return 0;
			str++;
			while (*str && isspace(*str)) str++;
			continue;
		}
		if (cpat=='_') { /* opcjonalna spacja */
			while (*str && isspace(*str)) str++;
			continue;
		}
		if (cpat=='+') { /* spacja */
			if (!*str || !isspace(*str)) return 0;
			str++;
			while (*str && isspace(*str)) str++;
			continue;
		}
		if (cpat=='`') {
			if (isspace(*str)) {
				char *ss=str+1;
				while(isspace(*ss)) ss++;
				if (*ss=='\'' || *ss=='`') {
					ss++;
					while (*ss && isspace(*ss)) ss++;
					str=ss;
					continue;
				}
				return 0;
			}
			if (*str=='\'' || *str=='`') {
				str++;
				while (*str && isspace(*str)) str++;
			}
			continue;
		}
		if (cpat=='~') {
			if (*patr == '~') { // dowolny ci±g spacji i my¶lników
				patr++;
				while (*str && (isspace(*str) || *str=='-')) str++;
				continue;
			}
			if (*str=='-') {
				str++;
				continue;
			}
			//while (*str && isspace(*str)) str++;
			//if (*str!='-') continue;
			//str++;
			while (*str && isspace(*str)) str++;
			continue;
		}
		if (cpat=='[') {
			int found=0;
			while (*patr) {
				cpat=*patr++;
				if (cpat==']') break;
				if (!found && compare_char(*str,cpat)) found=1;
			}
			if (!found) return 0;
			str++;
			continue;
		}
		if (cpat=='*') {
			int i;
			if (nphr >= 10) return 0;
			array[nphr].str=str;
			for (i=0;str && lci[(*str) & 255];str++,i++);
			array[nphr++].len=i;
			continue;
		}
		if (cpat=='(') {
			char *bstr;
			int i;
			if (nphr>=10) return 0;
			while (*patr) {
				bstr=patr;
				for (i=0;bstr[i];i++) {
					if (bstr[i]=='|' || bstr[i]==')') break;
					if (!compare_char(str[i],bstr[i])) break;
				}
				if (!bstr[i]) return 0;
				if (bstr[i]==')' || bstr[i]=='|') break;
				for (;*patr;patr++) {
					if (*patr==')') return 0;
					if (*patr=='|') {
						patr++;
						break;
					}
				}
				bstr=patr;
			}
			str+=i;
			array[nphr].len=-1;
			array[nphr++].str=bstr;
			patr=strchr(patr,')');
			if (!patr) return 0;
			patr++;
			continue;
		}
		if (!compare_char(*str,cpat)) return 0;
		str++;
	}
	if (*str) {
		char *c=str-1;
		/* wnêtrze */
		if (*str=='~' && !(cfg->phraser_mode & MILENA_PHR_IGNORE_TILDE) && strchr(ESCHAR,str[1])) return 0;

		/* apos? */
		if (lci[(*c) & 255] && *str=='\'' && lci[str[1]&255]) {
			int i,j;
			for (i=0;aps[i];i++) {
				for (j=0;aps[i][j];j++) if (aps[i][j]!=lci[str[j+1]&255]) break;
				if (aps[i][j]) continue;
				if (!lci[str[j+1] & 255]) return 0;
			}
		}
		if (*c=='.' && *str=='.' && str[1]=='.') return 0;
		if (lci[(*str)&255]) return 0;
	}
	*ostr=str;
	*onph=nphr;
	return 1;

}

static int phraser_OutputDic(struct milena *cfg,
	struct milena_udic *ud,
	int pos,
	char *outbuf,
	int buflen,
	struct phraser_dic_rc *array,
	int narr)
{
	int npr=0;
	char *c;
	if (!ud->pron || !strpbrk(ud->pron,"\t ")) {
		if (ud->flags & MILDIC_V) {
			pushstr("{v}");
		}
		if ((ud->flags & ~ MILDIC_V) || ud->stress || ud->pstress) {
			pushbuf('[');
			if (ud->stress) pushbuf('0'+ud->stress);
			if (ud->pstress) {
				pushbuf('+');
				pushbuf('0'+ud->pstress);
			}
			if (ud->flags & MILDIC_K) pushbuf('k');
			if (ud->flags & MILDIC_U) pushbuf('n');
			pushbuf(']');
		}
	}
	if (!ud->pron) c=ud->word;
	else c=ud->pron;
	for (;*c;c++) {
		char d;
		if (*c!='%') {
			d=lci[(*c) & 255];
			if (d) pushbuf(d);
			else pushbuf(*c);
			continue;
		}
		if (isdigit(c[1])) {
			c++;
			npr=(*c) - '0';
		}
		if (npr<narr) {
			char *s,i;
			i=array[npr].len;
			s=array[npr++].str;
			if (i<0) {
				for (;*s && *s!=')' && *s!='|';s++) {
					d=lci[(*s) & 255];
					if (d) pushbuf(d);
					else pushbuf(*s);
				}
			}
			else {
				for (;*s && i>0;i--,s++) {
					d=lci[(*s) & 255];
					if (d) pushbuf(d);
					else break;
				}
			}
		}
	}
	return pos;
}

static int milena_nicky(struct milena *cfg,char **str,char *outbuf,int buflen,int pos,int last_num)
{
	char nick[64];
	char *si,*so;int n,done;
	struct milena_udic *ud;
	for (si=*str,n=0;good_char(cfg,*si);n++,si++) {
		if (isdigit(*si)) break;
		if (n>=63) return milena_spell(cfg,str,outbuf,buflen,pos,last_num);
		nick[n]=*si;
	}
	nick[n]=0;
	*str=si;
	if (pos) {
		pushbuf(' ');
	}
	for (done=0,ud=cfg->udic;ud;ud=ud->next) {
		int nphr;
		struct phraser_dic_rc phra[16];
		char *cs;
		if (ud->flags & MILDIC_ATSTART) {
			if(pos) continue;
		}
		if (ud->flags & MILDIC_ATQUE) continue;
		if (ud->flags & MILDIC_BEFORECAP) continue;
		if (ud->flags & MILDIC_BEFOREUNCAP) continue;
		if (ud->flags & MILDIC_ATEND) continue;
		cs=nick;
		if (!phraser_CompareDic(cfg,ud->word,&cs,phra,&nphr)) continue;
		if (ud -> flags & MILDIC_SPELL) {
			return milena_spell(cfg,str,outbuf,buflen,pos,last_num);
		}
		pos=phraser_OutputDic(cfg,ud,pos,outbuf,buflen,phra,nphr);
		done=1;
		break;
	}
	if (!done) pushstr(nick);
	pushbuf(' ');
	if (!**str) return pos;
	n=strtol(*str,NULL,10);
	if (n>1900 && n<2100) {
	    int chuj;
	    return milena_digitize(str,outbuf,buflen,pos,0,NULL,cfg,0);
	}
	return milena_spell(cfg,str,outbuf,buflen,pos,last_num);
}


char *mi_spell_all(char c)
{
	static char *spl[]={
		"\x80wielokropek",
		".kropka",
		",przecinek",
		":dwukropek",
		"(nawias",
		")prawy nawias",
		"\"cudzys³ów",
		"'apostrof",
		";¶rednik",
		"-my¶lnik",
		"[nawias kwadratowy",
		"]prawy nawias kwadratowy",
		"{klamra",
		"}prawa klamra",
		"?pytajnik",
		"!wykrzyknik",NULL};
	int i;
	for (i=0;spl[i];i++) if (spl[i][0]==c) return spl[i]+1;
	return NULL;
}

char *mi_spell(struct milena *cfg,char c,int punct,char *some)
{
	struct milena_spell *ms;
	for (ms=cfg->spellers;ms;ms=ms->next) if (ms->znak == c) return ms->result;
	if ((punct == 1 && some && strchr(some,c)) || punct == 2) {
		return mi_spell_all(c);
	}
	return NULL;
}

static char *phraser_get_emot(struct milena *cfg,char *str,char **outstr,int *eof)
{
	char *c,*d;
	struct milena_emot *me;
	static char emotbuf[64];
	int fin=0;
	if (!cfg->emots) return NULL;
	if (eof) *eof=0;
	for (me=cfg->emots;me;me=me->next) {
		for (c=str,d=me->emot;*d;) {
			if (*d=='+') {
				if (*c++!=' ') break;
				d++;
				continue;
			}
			if (*d=='\\') {
				d++;
				if (!*d) break;
			}
			if (*c++ != *d) break;
			d++;
		}
		if (!*d && strlen(me->result)<58) {
			if (outstr) *outstr=c;
			d=emotbuf;
			if (cfg->emotmod) {
				sprintf(d,"{%d}",cfg->emotmod);
				d+=strlen(d);
			}
			strcpy(d,me->result);
			if (strchr("?!",me->result[strlen(me->result-1)])) {
				fin=1;
				if (emotbuf[strlen(emotbuf)-1]=='?') fin=4;
				emotbuf[strlen(emotbuf)-1]=0;
				if (eof) *eof=fin;
			}
			if (cfg->emotmod) strcat(emotbuf,"{0}");
			return emotbuf;
		}
	}
	c=str;
	if (*c=='<') {
		d=emotbuf;
		if (cfg->emotmod) {
			sprintf(d,"{%d}",cfg->emotmod);
			d+=strlen(d);
		}
		c++;
		while (*c && !isdigit(*c) && lci[(*c) & 255]) {
			if (d-emotbuf > 58) break;
			*d++=*c++;
		}
		if (*c=='!' || *c=='?') {
			fin=1;
			if (*c=='?') fin=4;
			*c++;
		}
		if (*c=='>') {
			if (eof) *eof=fin;
			if (cfg->emotmod) strcpy(d,"{0}");
			else *d=0;
			c++;
			if (outstr) *outstr=c;
			return emotbuf;
		}
	}
	return NULL;
}

int milena_GetParType(struct milena *cfg,char **str,int spell_empty)
{
	int dialog=1;
	char *c,*d;
	c=NULL;
	for (;**str;(*str)++) {
		if (phraser_get_emot(cfg,*str,NULL,NULL)) return dialog;
		//struct milena_emot *me;
		//for (me=cfg->emots;me;me=me->next) if (!strncmp(me->emot,*str,strlen(me->emot))) {
			//return dialog;
		//}
		if (!(cfg->phraser_mode & MILENA_PHR_IGNORE_INFO)) {
			if (**str=='{' || **str=='[') {
				if (!c) c=*str;
				d=strchr(*str,((**str)=='{')?'}':']');
				if (d) {
					*str=d+1;
					continue;
				}
			}
		}
		if (**str == '-') {
			dialog=2;
			if (mi_spell(cfg,'-',0,NULL)) break;
		}
		else if (good_char(cfg,**str) || mi_spell(cfg,**str,0,NULL)) break;
	}
	if (!**str) return 0;
	d=*str;
	if (c) *str=c;
	for (;*d;d++) if (good_char(cfg,*d) || (spell_empty && mi_spell(cfg,**str,0,NULL))) return dialog;
	return 0;
}


#define IS_EMOTICON eof=0;if (c=phraser_get_emot(cfg,*str,str,&eof)) {\
if (pos) {\
pushbuf(' ');\
}\
pushstr(c);\
nword++;\
if (!**str) break;\
if (!eof) continue;}

static struct milena_dic *milena_find_dic(struct milena *cfg,char *word);

static int phraser_at_end(struct milena *cfg,char *s,int strict)
{
	char word[256],*c;
	if (*s=='-') {
		s++;
		if (!*s) return 1;
		if (isdigit(*s) || lci[(*s) & 255]) return 0;
		return 1;
	}
	while (*s && *s != '\n' && isspace(*s)) s++;
	if (*s=='?') return 2;
	if (!*s) return 1;
	if (!strict && lci[(*s) & 255]) {
		struct milena_dic *md;
		for (c=word;*s;) {
			int znak;
			if (isdigit(*s)) return 0;
			*c++=znak=lci[(*s++) & 255];
			if (!znak) break;
		}
		*c=0;
		md=milena_find_dic(cfg,word);
		if (md && (md->flags & MILDIC_SEP)) return 1;
		return 0;
	}
	if (isdigit(*s) || lci[(*s) & 255]) return 0;
	return 1;
}

static int ditval(char *start,char *end)
{
    int n=0;
    for (;start<end;start++) {
	if (isdigit(*start)) n=10*n + (*start)-'0';
    }
    return n;
}

static int compute_grama(struct milena *cfg,char *str,u_int64_t gramas[],int *atend,int *count)
{
    char buf[64],*c;
    int n,znak;
    while (*str && isspace(*str)) str++;
    *atend=0;
    *count=0;
    if (!*str || !lci[(*str) & 255]) {
	*atend=1;
	return 1;
    }
    if (isdigit(*str)) return 0;
    n=milena_is_unit(cfg,str,15);
    if (n) {
	int fem=(n&16)?WM_f:0;
	u_int64_t grm;
	//fprintf(stderr,"Unit %s m=%d\n",str,n);
	if (n & (1|8)) {
	    grm = WM_sg | fem;
	    if (n & 1) grm |= WM_nom | WM_acc;
	    if (n & 8) grm |= WM_gen;
	    gramas[(*count)++]=grm;
	}
	if (n & (2|4)) {
	    //grm = WM_sg | fem;
	    //if (n & 2) grm |= WM_pl | WM_nom | WM_acc | fem;
	    //if (n & 4) grm |= WM_pl | WM_gen | fem;
	    grm = WM_pl | fem;
	    if (n & 2) grm |= WM_nom | WM_acc;
	    if (n & 4) grm |= WM_gen;
	    gramas[(*count)++]=grm;
	}
	//fprintf(stderr,"Cnt=%d\n",*count);
	return 1;
    }
    char *s=str;
    for (n=0;;) {
	
	if (!*s) break;
	if (isdigit(*s)) return 0;
	if (*s=='-' && s[1] && !isdigit(s[1]) && lci[s[1] & 255]) {
	    s++;
	    n=0;
	    continue;
	}
	znak=lci[(*s++) & 255];
	if (!znak) break;
	if (n >=63) return 0;
	buf[n++]=znak;
    }
    buf[n]=0;
    if (0 && n>2 && !strcmp(buf+n-2,"ów")) {
	gramas[0]= WM_pl | WM_m | WM_gen;
	*count=1;
	return 1;
    }
    if (0 && n>3 && (!strcmp(buf+n-3,"ich") || !strcmp(buf+n-3,"ych"))) {
	gramas[0]= WM_pl | WM_m | WM_f | WM_gen | WM_loc;
	*count=1;
	return 1;
    }
#ifdef HAVE_MORFOLOGIK
	//n=morf_is_subst_or_adj(cfg,str,gramas);
	n=morf_get_subst_form(cfg,str,gramas);
	if (n) {
	    *count=n;
	    return 1;
	}
#endif
    return 0;
}
	

	
    
    

static int _milena_GetPhraseWithPunct(struct milena *cfg,
	char **str,char *outbuf,int buflen,int *pmode,int punct,char *some)
{
	int pos=0;
	int nword=0;
	char *bword,*c,*ostr=*str;
	int n,typ;
	int last_num;
	int dash,eof,elang;
	char context[4][64];
	int context_no=0;
	struct recog_number *rn;
	int spellme,spellme1;
	
	void clear_context(void)
	{
	    context_no=0;
	}
	void push_context(char *word)
	{
	    char buf[64];
	    char *c;
	    int n,ll;
	    for (n=0,c=buf;*word;n++,word++) {
		if (n>63 || isdigit(*word)) {
		    clear_context();
		    return;
		}
		ll=lci[(*word) & 255];
		if (!ll) break;
		*c++=ll;
	    }
	    *c=0;
	    if (context_no == 4) {
		for (n=0;n<3;n++) strcpy(context[n],context[n+1]);
	    }
	    else context_no++;
	    strcpy(context[context_no-1],buf);
	    //fprintf(stderr,"Context:");
	    //for (n=0;n<context_no;n++) fprintf(stderr," [%s]",context[n]);
	    //fprintf(stderr,"\n");
	}
	
	int context_pasuje(struct recog_number *rn)
	{
	    int i,n;
	    int slowo_pasuje(char *c,char *ctx)
	    {
		char *d;
		for (;*c;) {
		    for (d=ctx;;) {
			if (!*c || *c==')') {
			    return !*d;
			}
			if (*c=='|') {
			    if (!*d) return 1;
			    break;
			}
			if (*d++ != *c++) break;
		    }
		    while (*c && *c!='|') c++;
		    if (*c) c++;
		}
	    }
	    n=context_no-rn->pred;
	    if (n<0) return 0;
	    for (i=0;i<rn->pred;i++) {
		if (!strncmp(rn->preds[i],"e:",2)) {
		    struct recog_expression *expr;
		    for (expr=cfg->recexpr;expr;expr=expr->next) {
			if (strcmp(expr->name,rn->preds[i]+2) || expr->str[0] != '(') continue;
			//fprintf(stderr,"Dopasowanie [%s] do [%s]\n",context[n+i],expr->str+1);
			if (slowo_pasuje(expr->str+1,context[n+i])) break;
		    }
		    if (!expr) return 0;
		    continue;
		}
		if (!slowo_pasuje(rn->preds[i],context[n+i])) {
		    return 0;
		}
	    }
	    return 1;
	}
	    
	struct recog_number *get_recog_number(u_int64_t gramas[],int grama_count,int atend,int val,u_int64_t *outgrama)
	{
	    struct recog_number *rn;
	    int i;
	    int dval(int n)
	    {
		n = n % 100;
		if (n>10 && n < 20) return 0;
		n %= 10;
		if (n >=2 && n <=4) return 1;
		return 0;
	    }
	    for (rn=cfg->recog_numbers;rn;rn=rn->next) {
		if (rn->atend && !atend) continue;
		if (!rn->atend && atend) continue;
		
		if ((rn->flags & RECNU_SMAL) && (val >= 1000)) continue;
		if ((rn->flags & RECNU_BIG) && (val < 1000)) continue;
		if ((rn->flags & RECNU_SGL) && (val != 1)) continue;
		if ((rn->flags & RECNU_DBL) && !dval(val)) continue;
		if ((rn->flags & RECNU_NDBL) && dval(val)) continue;
		
		if (!context_pasuje(rn)) {
		    continue;
		}
		
		if (rn->atend) break;
		for (i=0;i<grama_count;i++) {
		    //fprintf(stderr,"N:%d/%Ld,%Ld\n",rn->num,gramas[i] & WM_NUM_MASK,gramas[i] & rn->num);
		    //fprintf(stderr,"C:%d/%Ld,%Ld\n",rn->cas,gramas[i] & WM_CASU_MASK,gramas[i] & rn->cas);
		    if (rn->num && !(rn->num & gramas[i])) continue;
		    if (rn->cas & gramas[i]) break;
		}
		if (i<grama_count) {
		    *outgrama=gramas[i];
		    return rn;
		}
	    }
	    return NULL;
	}
	    
	    
	    

	int is_cap(char *s)
	{
		while (*s && isspace(*s)) s++;
		if (!*s || !good_char(cfg,*s)) return 0;
		return uci[(*s) & 255];
	}
	int is_finale(char *s,char *fin)
	{
		for (;*fin;s++,fin++) {
			if (!*s) return 0;
			if (lci[(*s) & 255] != *fin) return 0;
		}
		if (good_char(cfg,*s)) return 0;
		return 1;
	}
	int find_mac(char *sq)
	{
		struct mika_afx *mac;
		for (mac=cfg->macanty;mac;mac=mac->next) {
			char *c=mac->afx,*s=sq;
			//fprintf(stderr,"Conp %s %s\n",c,s);
			for (;*c && *s;c++,s++) {
				if (*c=='*') break;
				if (*c!=lci[(*s) & 255]) break;
			}
			if (*c=='*') return 1;
			if (!*c && (!s || !good_char(cfg,*s))) return 1;

		}
		return 0;
	}

	int is_mac(void)
	{
		char *s=NULL,*c,*a;
		int ns;
		a="mek";
		if (!strncasecmp(*str,"o'",2)) {
			s=(*str)+2;
			a="o³";
		}
		else if (!strncasecmp(*str,"mc",2)) {
			s=(*str)+2;
			if (*s=='h') return 0;
		}
		else if (!strncasecmp(*str,"mac",3)) {
			int iq;
			s=(*str)+3;
			for (iq=0;s[iq];iq++) if (!good_char(cfg,s[iq])) break;
			if (iq<3) return 0;
			//if (uci[(*s)&255] && !uci[(*str)[2] & 255]) goto macok;
			/*
			if (*s=='y') {
				if (!good_char(cfg,s[1])) return 0;
				goto macok;
			}
			if (*s=='i') {
				if (is_samog(cfg,s[1])) return 0;
				if (!strncasecmp(s+1,"sz",2)) return 0;
				if (strchr("¶æñ³¿¼ck",s[1])) return 0;
				goto macok;
			}
			*/
			if (find_mac(s)) return 0;
		}
macok:		if (!s) return 0;
		ns=0;
		for (c=s;*c;c++) {
			if(isdigit(*c)) return 0;
			if (!good_char(cfg,*c)) break;
			if(is_samog(cfg,*c)) ns++;
		}
		if (!ns) return 0;
		if (c<s+3) return 0;
		if (pos) pushbuf(' ');
		pushstr("[n]");
		pushstr(a);
		*str=s;
		return 1;
	}
	

	void pushy(int z)
	{
		z=lci[z & 255];
		if (cfg->redef[z & 255]) {
			pushstr(cfg->redef[z & 255]);
		} else {
			pushbuf(z);
		}

	}
	int is_apostr(void)
	{
		char *s=*str,lc,*c;
		int rc=1;
		int i;
		int sag=0;
		for (;*s;s++) {
			if (!good_char(cfg,*s)) break;
			lc=lci[(*s) & 255];
			if (is_samog(cfg,lc)) sag=1;
		}
		if (*s!='\'') return 0;
		if (s == (*str)+1) {
			char *s1;
			int nextap=0;
			//if (lc != 'l' && lc !='d' && lc !='L' && lc !='D') return 0;
			s++;
			for (s1=s;*s1;s1++) {
				if ((*s1)=='\'') {
					nextap=1;
					break;
				}
				if (!good_char(cfg,*s1)) break;
				if (isdigit(*s1)) return 0;
				lc=lci[(*s1) & 255];
				if (is_samog(cfg,lc)) sag=1;
			}
			if (s1==s) return 0;
			if (!sag) return 0;
			if (pos) {
				pushbuf(' ');
			}
			pushy(**str);
			(*str)++;
			(*str)++;
			pushbuf('&');
			if (nextap) {
				for (i=0;aps[i];i++) if (is_finale(s1+1,aps[i])) break;
				if (!aps[i]) nextap=0;
			}

			if (!nextap) {
				for (;**str;(*str)++) {
					if (!good_char(cfg,**str)) break;
					pushy(**str);
				}
				return 1;
			}
			s=s1;
			goto omit_partest;

		}
		if (!sag) return 0;
		//if (lc !='e' && lc!='y') return 0;
		if (lc == 'n' && s[1]=='t' && !good_char(cfg,s[2])) {
			if (pos) {
				pushbuf(' ');
			}
			for (;*str < s;(*str)++) {
				pushy(**str);
				if ((*str) == s-2 && !is_samog(cfg,**str)) {
					pushbuf('Y');
				}
			}
			pushbuf('&');
			goto push_afton;
		}
		for (i=0;aps[i];i++) if (is_finale(s+1,aps[i])) break;
		if (!aps[i]) return 0;
		if (pos) {
			pushbuf(' ');
		}
omit_partest:
#if 0
		for (;*str <s ; (*str)++) {
			pushbuf(**str);
		}
		pushbuf('&');
#else
		for (;*str < s;(*str)++) {
			if (strchr("aesioó",aps[i][0]) && (*str)==s-4 && !strncasecmp((*str),"aine",4)) {
				pushstr("ejn");
				(*str)+=4;
				break;
			}
			if (aps[i][0]=='i' && (*str)==s-2 && !strncasecmp((*str),"te",2)) {
				pushstr("c&");
				(*str)+=2;
				break;
			}
			if (strchr("aeioó",aps[i][0]) && (*str)==s-4 && strchr("eou",**str) && !strncasecmp((*str)+1,"ice",3)) {
				pushbuf(*(*str)++);
				pushstr("js&");
				(*str)+=3;
				break;
			}
			if (strchr("aeioó",aps[i][0]) && (*str)==s-4 && !strncasecmp(*str,"aice",4)) {
				pushstr("ejs&");
				(*str)+=3;
				break;
			}
			if (strchr("aeioó",aps[i][0]) && (*str)==s-3 && !strncasecmp(*str,"ice",3)) {
				pushstr("ajs&");
				(*str)+=3;
				break;
			}
			if (strchr("aeioó",aps[i][0]) && (*str)==s-2 && !strncasecmp(*str,"ce",2)) {
				pushstr("s&");
				(*str)+=2;
				break;
			}
			if (strchr("aeio",aps[i][0]) && (*str)==s-4 && !strncasecmp(*str,"ance",4)) {
				pushstr("ens&");
				(*str)+=4;
				break;
			}
			if (aps[i][0]=='s'  && (*str)==s-2 && !strncasecmp(*str,"ce",2)) {
				pushstr("s@");
				(*str)+=2;
				break;
			}
			if (aps[i][0]=='s'  && (*str)==s-2 && !strncasecmp(*str,"ie",2)) {
				pushstr("i");
				(*str)+=2;
				break;
			}
			if (aps[i][0]=='e' && (*str)==s-3 && !strncasecmp(*str,"thy",3)) {
				pushstr("thy&");
				(*str)+=3;
				break;
			}
			if (aps[i][0]=='s'  && (*str)==s-4 && !strncasecmp(*str,"ance",4)) {
				pushstr("ens@");
				(*str)+=4;
				break;
			}
			if (*str == s-1) {
				if (aps[i][0] == 's') {
					pushbuf(**str);
					if (lc=='s' || lc=='z') {
						pushstr("~'i");
					}
				}
				else if (lc=='y' && !strchr("yo",aps[i][0])) {
					pushbuf('&');
					if (aps[i][0] == 'e') {
						pushbuf('i');
					}
					else {
						pushbuf('j');
					}
				}
				else if (**str!='e') {
					pushy(**str);
					pushbuf('&');
				}
				else pushbuf('&');
			}
			else {
				pushy(**str);
			}
		}
#endif
push_afton:
		(*str)++;
		while((**str) && good_char(cfg,**str)) {
			char z=*(*str)++;
			pushy(z);
		}
		return rc;
	}
	int is_real_number(char *s)
	{
		if (!isdigit(*s)) return 0;
		while (*s && isdigit(*s)) s++;
		if (!*s) return 1;
		if (good_char(cfg,*s)) return 0;
		if (*s !='.' && *s!=',') return 1;
		s++;
		if (!*s) return 1;
		if (!good_char(cfg,*s)) return 1;
		if (!isdigit(*s)) return 0;
		while (*s && isdigit(*s)) s++;
		if (!*s) return 1;
		if (good_char(cfg,*s)) return 0;
		if (*s !='.' && *s!=',') return 1;
		s++;
		if (!*s) return 1;
		return !good_char(cfg,*s);
	}

	int is_int_number(char *s)
	{
		if (!isdigit(*s)) return 0;
		while (*s && isdigit(*s)) s++;
		if (!*s) return 1;
		if (good_char(cfg,*s)) return 0;
		if (*s !='.' && *s!=',') return 1;
		s++;
		if (!*s) return 1;
		if (!good_char(cfg,*s)) return 1;
		return 0;
	}


	int is_nicky_part(char *s,int len)
	{
	    struct milena_stringlist *mc;int i;
	    if (len != 3) return 0;
	    for (mc=cfg->nicks;mc;mc=mc->next) {
		for (i=0;i<3;i++) {
		    if (lci[s[i]&255] != lci[mc->string[i] & 255]) {
			break;
		    }
		}
		if (i==3) return 1;
	    }
	    return 0;
	}
	int is_this_word(char *s,char *word)
	{
	    while (*word) {
		if (lci[(*s++) & 255] != *word++) return 0;
	    }
	    return !good_char(cfg,*s);
	}
	int is_word(char *s)
	{
		int ns=0;
		int nl=0;
		int nd=0;
		int nw=0;
		int nu=0;
		int nas=0;
		char *begs;
		for (;;) {
			if (!(cfg->phraser_mode & MILENA_PHR_IGNORE_TILDE) && *s=='~' && strchr(ESCHAR,s[1])) {
				s+=2;
				continue;
			}
			break;
		}
		if (!*s) return 0;
		if (!good_char(cfg,*s)) return 0;
		for (begs=s;*s;s++) {
			if (!(cfg->phraser_mode & MILENA_PHR_IGNORE_TILDE) && *s=='~') {
				if (strchr(ESCHAR,s[1])) {
					nas=-1;
					s++;
					continue;
				}
				break;
			}
			if (!good_char(cfg,*s)) break;
			if (isdigit(*s)) {
				nd++;
				continue;
			}
			if (nd || nas <0 /*|| ((*s) & 0x80)*/) nas=-1;else nas++;
			nl++;
			if (is_samog(cfg,*s)) {
				ns++;
				if (strchr("Óó¡±Êê",*s)) nu++;
			}
			else if (strchr("WwZz",*s)) nw++;
		}
		if (nd) {
			if (!nl) return MLWORD_NUMBER;
			if (nas>3 && ns>0 && ns < nas) return MLWORD_NICKY;
			if (is_nicky_part(begs,nas)) return MLWORD_NICKY;
			return MLWORD_UNSPEAK;
		}
		if (nl==1 && nu==1) return MLWORD_UNSPEAK;
		if (ns || (nl==1 && nw==1) ) return MLWORD_SPEAKABLE;
		return MLWORD_UNSPEAK;
	}

	int emit_domain_part(char *s,int final,int len,int nodot)
	{
	    int rc=0,i;struct milena_tld *tld=NULL;
	    if (final) {
		    
		    if (len == 2) {
			pos=milena_spell(cfg,&s,outbuf,buflen,pos,-1);
			return 0;
		    }
		    for (tld=cfg->tld;tld;tld=tld->next) {
			if (strlen(tld->string) == len && !strncasecmp(s,tld->string,len)) break;
		    }
		    if (tld) {
			s=tld->pron;
			if (tld->flags & MILDIC_SPELL) {
			    pos=milena_spell(cfg,&s,outbuf,buflen,pos,-1);
			    return 1;
			}
		    }
	    }
	    if (!rc && !nodot) {
		pushstr("kropka ");
	    }
	    for (;*s && (good_char(cfg,*s) || (*s=='-' && s[1] && good_char(cfg,s[1])));) {
		if (*s == '-') {
		    pushstr(" my¶lnik ");
		    s++;
		}
		int typ=is_word(s);
		if (!typ) {
		    pos=milena_spell(cfg,&s,outbuf,buflen,pos,-1);
		    continue;
		}
		struct milena_udic *ud;
		int nphr;
		struct phraser_dic_rc phra[16];
		for (ud=cfg->udic;ud;ud=ud->next) {
			char *cs;
			if (ud->flags & (MILDIC_ATSTART | MILDIC_ATEND | MILDIC_ATQUE | MILDIC_BEFORECAP | MILDIC_BEFOREUNCAP)) {
			    continue;
			}
			cs=s;
			if (!phraser_CompareDic(cfg,ud->word,&s,phra,&nphr)) continue;
			if (ud -> flags & MILDIC_SPELL) {
			    s=cs;
			    typ=MLWORD_UNSPEAK;
			    ud=NULL;
			}
			break;
		}
		if (ud) {
		    pos=phraser_OutputDic(cfg,ud,pos,outbuf,buflen,phra,nphr);
		    continue;
		}
		if (typ == MLWORD_NICKY) {
		    pos=milena_nicky(cfg,&s,outbuf,buflen,pos-1,0);
		    continue;
		}
		if (typ==MLWORD_SPEAKABLE) {
		    char *backup;
		    int mac;
		    backup=*str;
		    *str=s;
		    mac=is_mac();
		    s=*str;
		    *str=backup;
		    if (mac) {
			pushstr("[Len]");
		    }
		    for (;*s;s++) {
			char cx,*sx;
			cx=lci[(*s) & 255];
			if (!cx || cx=='@') break;
			sx=cfg->redef[cx & 255];
			if (sx) {
			    for (;*sx;sx++) {
				pushbuf(*sx);
			    }
			    continue;
			}
			pushbuf(cx);
		    }
		    continue;
		}
		pos=milena_spell(cfg,&s,outbuf,buflen,pos,-1);
	    }
	    return rc;
	}
	
	char *is_domain(int question_only)
	{
	    /* tylko pytanie: zwraca koniec je¶li to domena lub NULL */
	    /* tylko pytanie: zwraca koniec je¶li to domena lub NULL */
	    int partno=0,n,typ,elen[16],last_nodot;
	    char *parts[16];
	    char *c,*s=*str;
	    for (;;) {
		c=s;
		while(*c && (good_char(cfg,*c) || (*c=='-' && c[1] && good_char(cfg,c[1])))) c++;
		if (c==s) break;
		if (partno >= 16) return NULL;
		elen[partno]=c-s;
		parts[partno++]=s;
		s=c;
		if (*s != '.') break;
		if (!s[1] || !good_char(cfg,s[1])) break;
		s++;
	    }
	    if (partno < 2) return NULL;
	    if (elen[partno-1]<2) return NULL;
	    for (c=parts[partno-1];*c && good_char(cfg,*c);c++) if (!isalpha(*c)) return NULL;
	    if (elen[partno-1]>2) {
		struct milena_tld *tld;
		c=parts[partno-1];
		for (tld=cfg->tld;tld;tld=tld->next) {
		    if (elen[partno-1] == strlen(tld->string) && !strncasecmp(c,tld->string,elen[partno-1])) break;
		}
		if (!tld) return NULL;
	    }
	    if (question_only) return s;
	    //printf("Domain [%-*.*s]\n",(s-*str),(s-*str),*str);
	    *str=s;
	    for (n=0,last_nodot=0;n<partno;n++) {
		c=parts[n];
		if (pos) pushbuf(' ');
		last_nodot=emit_domain_part(c,n==partno-1 || (n==partno-2 && elen[partno-1]==2),elen[n],!n && !last_nodot);
	    }
	    return s;
	}

	int is_uri()
	{
	    char *backup=*str;
	    char *dstart,*dend,*c,*port=NULL;
	    static char *protos[]={"http://","https://",NULL};
	    int pn,i;
	    int ipadr[4],domain_name_mode=0;
	    for (pn=0;protos[pn];pn++) if (!strncasecmp(*str,protos[pn],strlen(protos[pn]))) {
		*str += strlen(protos[pn]);
		break;
	    }
	    dstart=*str;
	    dend=is_domain(1);
	    if (!dend && *str && isdigit(**str)) {
		for (i=0;i<4;i++) {
		    if (i) {
			if (*(*str)++ != '.') goto pull;
		    }
		    if (!isdigit(**str)) goto pull;
		    if (**str == '0' && (*str)[1] && isdigit((*str)[1])) goto pull;
		    ipadr[i]=strtol(*str,str,10);
		    if (ipadr[i]>255) goto pull;
		}
		if (**str){
		    if (good_char(cfg,**str)) goto pull;
		    if (**str=='.' && (*str)[1] && good_char(cfg,(*str)[1])) goto pull;
		}
		domain_name_mode=1;
		dend=*str;
	    }
	    if (!dend) {
		if (!strncasecmp(*str,"localhost",9)) {
		    (*str) += 9;
		    if (**str=='.' && (*str)[1] && good_char(cfg,(*str)[1])) goto pull;
		    domain_name_mode=2;
		    dend=*str;
		}
	    }
	    if (!dend) return 0;
	    if (*dend == ':') {
		for(i=0,c=dend+1;*c && isdigit(*c);c++);
		if (!*c || !good_char(cfg,*c)) {
		    port = dend+1;
		}
	    }


	    if (pos) pushbuf(' ');
	    if (protos[pn]) {
		c=protos[pn];
		pos=milena_spell(cfg,&c,outbuf,buflen,pos,-1);
		pushbuf(' ');
	    }
	    *str=dstart;
	    if (domain_name_mode == 1) {
		pos=milena_digitize(str,outbuf,buflen,pos,0,NULL,cfg,0);
	    }
	    else if (domain_name_mode == 2) {
		pushstr("[3]lokalhost");
	    }
	    else {
		is_domain(0);
	    }
	    *str=dend;
	    if (port) {
		*str=port;
		pushstr(" dwukropek ");
		pos=milena_digitize(str,outbuf,buflen,pos,1,NULL,cfg,0);
	    }
		
	    return 1;
	pull:
	    *str=backup;
	    return 0;
	}

	int is_entity()
	{
		char *sc=*str;int base=10;
		int wc;
		char ebuf[256];
		/* tu jaki¶ esc powinien byæ... */
		if (*sc++!='&') return 0;
		//if (*sc++!='#') return 0;
		if (*sc++!='\x1b') return 0;
		if (*sc=='x' || *sc=='X') {
			sc++;
			base=16;
			if (!isxdigit(*sc)) return 0;
		}
		else {
			if (!isdigit(*sc)) return 0;
		}
		wc=strtol(sc,&sc,base);
		if (*sc++!=';') return 0;
		*str=sc;
		pushbuf(' ');
		milena_wchar(cfg,wc,ebuf,256,NULL);
		pushstr(ebuf);
		pushbuf(' ');
		last_num=-1;
		nword++;
		return 1;
	}

	int is_big_number()
	{
	    int n,pno=0;char sbuf[32],*c,*s,*unit;
	    int pmd=0;
	    int nkw=0;
	    int nfirst;
	    int coma=0;
	    int unit_got=0;
	    int female;
	    u_int64_t next_gramas[16],next_grama;
	    int atend;
	    int grama_valid,grama_count;
	    
	    int isit_coma(char *sc)
	    {
		if ((*sc != ',' && *sc != '.') || *sc == pmd) return 0;
		sc++;
		if (!isdigit(*sc++)) return 0;
		if (!isdigit(*sc++)) return 0;
		if (!*sc) return 1;
		if (isspace(*sc)) return 1;
		if (good_char(cfg,*sc)) return 0;
		if (!sc[1]) return 1;
		if (good_char(cfg,sc[1])) return 0;
		return 1;
	    }
	    
	    s=*str;
	    if (!isdigit(*s) || *s=='0') return 0;
	    for (n=0;*s && isdigit(*s);s++,n++);
	    if (n > 3) return 0;
	    if (!*s) return 0;
	    nfirst=n;
	    if (isspace(*s)) {
		pmd=' ';
		for (pno=0;;) {
		    while (*s && isspace(*s)) s++;
		    if (!*s || !isdigit(*s)) break;
		    for (n=0;s[n] && isdigit(s[n]);n++);
		    if (n!=3) return 0;
		    if (pno && !nkw) {
			for (n=0;n<3;n++) if (s[n] != '0') nkw=1;
		    }
		    pno += 1;
		    s+=3;
		    if (*s && !isspace(*s)) {
			if (good_char(cfg,*s)) return 0;
			if ((coma=isit_coma(s))) break;
			if (strchr(".-,",*s) && s[1] && good_char(cfg,s[1])) return 0;
			break;
		    }
		}
		if (cfg->digits_limit && cfg->digits_limit < nfirst + 3 * pno) {
			while (*str < s) {
				if (pos) {
					pushbuf(' ');
				}
				pos=milena_digitize(str,outbuf,buflen,pos,0,NULL,cfg,0);
				while (*str < s && isspace(**str)) (*str)++;
			}
			return 1;
		}
	    }
	    else if (*s == '.' || *s==',') {
		pmd=*s;
		for (pno=0;;) {
		    if (*s!=pmd) break;
		    if (!s[1] || !isdigit(s[1])) break;
		    for (n=0;s[n] && isdigit(s[n+1]);n++);
		    if (n!=3) return 0;
		    if (pno && !nkw) {
			for (n=0;n<3;n++) if (s[n+1] != '0') nkw=1;
		    }
		    pno += 1;
		    s+=4;
		    if (*s && !isspace(*s)) {
			if (good_char(cfg,*s)) return 0;
			if (*s == pmd && s[1] && isdigit(s[1])) continue;
			if ((coma=isit_coma(s))) break;
			if (strchr(".-,",*s) && s[1] && good_char(cfg,s[1])) return 0;
			break;
		    }
		}
	    }
	    if (!pno) return 0;
	    if (pno > 3) goto spelnum;
	    if (coma) goto ohayo;
	    while (*s && isspace(*s)) s++;
	    if (nkw && pmd != ' ' && pno == 1) {
		c=s;
		if (!(unit=milena_get_unit(cfg,&c,0,&female,0))) {
		    female=0;
		    for (n=0;n<3;n++) {
			if (!c[n] || !isupper(c[n])) break;
		    }
		    if (n==3 && (!c[3] || !good_char(cfg,c[3]))) {
			spellme1=unit_got=1;
			goto ohayo;
		    }
		    if (pmd != ' ' && pno == 1) return 0;
		    goto spelnum;
		}
		if (!unit) return 0;
		if (unit && pmd != ' ' && pno == 1 && (!strncmp(unit,"mili",4) || !strncmp(unit,"mikro",4))) {
		    return 0;
		}
		unit_got=1;
	    }
	ohayo:
	    if (!unit_got) {
		c=s;
		while (*c && isspace(*c)) c++;
		if (!(unit=milena_get_unit(cfg,&c,0,&female,0))) {
			female=0;
			for (n=0;n<3;n++) {
			    if (!c[n] || !isupper(c[n])) break;
			}
			if (n==3 && (!c[3] || !good_char(cfg,c[3]))) spellme1=unit_got=1;
			
		}
		else unit_got=1;
	    }
	    if (!unit_got) {
		//fprintf(stderr,"Computing grama\n");
		grama_valid=compute_grama(cfg,s,next_gramas,&atend,&grama_count);
	    }
	    else {
		//fprintf(stderr,"Evaluating grama\n");
		grama_valid=1;
		grama_count=1;
		atend=0;
		next_gramas[0]=WM_pl | WM_gen | WM_nom;
		if (female) next_gramas[0] |=WM_f;
		else next_gramas[0] |= WM_m;
	    }
	    //fprintf(stderr,"Grama valid=%d/%d, g=%LX\n",grama_valid,grama_count,next_gramas[0]);
	    
	    if (grama_valid) {
		rn=get_recog_number(next_gramas,grama_count,atend,1000,&next_grama);
		//fprintf(stderr,"Found grama %LX\n",next_grama);
	    }
	    else rn=NULL;
	    //if (!rn) fprintf(stderr,"No context\n");
	    if (!rn) goto standard_int;
	    int fmode=rn->format[2];
	    if (next_grama & WM_m) fmode=rn->format[0];
	    else if (next_grama & WM_f) fmode=rn->format[1];
	    //fprintf(stderr,"fmode=%d\n",fmode);
	    c=sbuf;
	    for (;*str<s;*(*str)++) if (isdigit(**str)) *c++=**str;
	    *c=0;
	    if (pos) {
		pushbuf(' ');
	    }
	    pos=milena_speak_digit(strtol(sbuf,NULL,10),fmode,outbuf,buflen,pos);
	    last_num=2;
	    return 1;
standard_int:
	    c=sbuf;
	    for (;*str<s;*(*str)++) if (isdigit(**str)) *c++=**str;
	    if (coma) {
		*c++=*(*str)++;
		*c++=*(*str)++;
		*c++=*(*str)++;
	    }
	    *c=0;
	    c=sbuf;
	    if (pos) {
		pushbuf(' ');
	    }
	    pos=milena_digitize(&c,outbuf,buflen,pos,0,&last_num,cfg,0);
	    return 1;
spelnum:
	    if (pno == 1) return 0;
	    while (*str < s) {
		if (**str=='.') {
		    pushstr(" kropka");
		    (*str)++;
		    continue;
		}
		if (**str==',') {
		    pushstr(" przecinek");
		    (*str)++;
		    continue;
		}
		if (isspace(**str)) {
		    while (isspace(**str) && (*str)<s) (*str)++;
		    continue;
		}
		c=sbuf;
		while (*str < s && isdigit(**str)) *c++=*(*str)++;
		*c=0;
		c=sbuf;
		if (pos) {
		    pushbuf(' ');
		}
		pos=milena_digitize(&c,outbuf,buflen,pos,0,&last_num,cfg,0);
	    }
	    while (*s && isspace(*s)) s++;
	    *str=s;
	    last_num=-1;
	    return 1;
	}
	
	void emit_number(int force)
	{
	    if (!force && !is_int_number(*str)) goto standard_number;
	    char *s,*c,*unit;
	    int num;
	    int female,n,unit_got=0,grama_count,atend,grama_valid;
	    u_int64_t next_gramas[16],next_grama;
	    struct recog_number *rn;
	    
	    num=strtol(*str,&s,10);
	    while (*s && isspace(*s)) s++;
	    c=s;
	    if (!(unit=milena_get_unit(cfg,&c,0,&female,0))) {
		female=0;
		for (n=0;n<3;n++) {
		    if (!c[n] || !isupper(c[n])) break;
		}
		if (n==3 && (!c[3] || !good_char(cfg,c[3]))) spellme1=unit_got=1;
	    }
	    else unit_got=1;
	    if (!unit_got) {
		grama_valid=compute_grama(cfg,s,next_gramas,&atend,&grama_count);
		//fprintf(stderr,"Grama valid/count=%d/%d\n",grama_valid,grama_count);
	    }
	    else {
		grama_valid=1;
		grama_count=1;
		atend=0;
		next_gramas[0]=WM_pl | WM_gen | WM_nom;
		if (female) next_gramas[0] |=WM_f;
		else next_gramas[0] |= WM_m;
	    }
	    
	    if (grama_valid) {
		rn=get_recog_number(next_gramas,grama_count,atend,num,&next_grama);
		//fprintf(stderr,"RN=%p\n",rn);
	    }
	    else rn=NULL;
	    if (!rn) goto standard_number;
	    int fmode=rn->format[2];
	    if (next_grama & WM_f) fmode=rn->format[1];
	    else if (next_grama & WM_m) fmode=rn->format[0];
	    if (pos) {
		pushbuf(' ');
	    }
	    *str=s;
	    pos=milena_speak_digit(num,fmode,outbuf,buflen,pos);
	    last_num=(num == 1) ? 3: 2;
	    return;

standard_number:	    
	    pos=milena_digitize(str,outbuf,buflen,pos,0,&last_num,cfg,1);
	}

	    
	    

	elang=0;
	int okolo,kolo;

	for (;**str;) {
		IS_EMOTICON
		if (eof) break;
		if ((!(cfg->phraser_mode & MILENA_PHR_IGNORE_INFO) &&
			(**str=='[' || **str=='{')) ||
			is_word(*str) || mi_spell(cfg,**str,punct,some) ||
			((**str)=='-' && is_word((*str)+1)==MLWORD_NUMBER)) break;
		(*str)++;
	}
	if (eof) goto final_part;
	bword=NULL;
	last_num=-1;
	okolo=0;
	kolo=0;
	spellme=spellme1=0;
	for (;;) {
		okolo=kolo;
		kolo=0;
		spellme=spellme1;
		spellme1=0;
		
		while (**str && isspace(**str)) (*str)++;
		if (!**str) break;
		if (!(cfg->phraser_mode && MILENA_PHR_IGNORE_TILDE)) {
			if (!strncmp(*str,"~'",2) || !strncmp(*str,"~+",2)) {
				(*str) += 2;
				continue;
			}
		}
		//is_entity tylko w trybie screenreadera!
		if (cfg->charnms) {
			if (is_entity()) continue;
		}
		IS_EMOTICON
		if (eof) break;
		if (**str=='.') {
			if (is_int_number((*str)+1)) {
				(*str)++;
				if (pos) {
					pushbuf(' ');
				}
				// kalibry - do konsultacji
				pushstr("kropka ");
				pos=milena_digitize(str,outbuf,buflen,pos,0,&last_num,cfg,0);
				nword++;
				clear_context();
				goto floop;
			}
		}
			    
		if (**str=='-') {
			if (is_real_number((*str)+1)) {
				(*str)++;
				if (pos) {
					pushbuf(' ');
				}
				pushstr("minus ");
				emit_number(0);
				nword++;
				clear_context();
				goto floop;
			}
		}
		if (!(cfg->phraser_mode & MILENA_PHR_IGNORE_INFO)) {
			if (**str=='[') {
				char *cs=strchr((*str),']');
				if (!cs) {
					*str+=strlen(*str);
					break;
				}
				if (!bword) bword=*str;
				*str=cs+1;
				continue;
			}
			if (**str=='{') {
				char *cs=strchr((*str),'}');
				if (!cs) {
					*str+=strlen(*str);
					break;
				}
				if (!bword) bword=*str;
				*str=cs+1;
				continue;
			}
		}
		if (last_num >= 0 && !bword) {
			char *c=milena_get_unit(cfg,str,last_num,NULL,0);
			if (c) {
				if (pos) pushbuf(' ');
				nword++;
				pushstr(c);
				last_num=-1;
				clear_context();
				goto floop;
			}
		}
		dash=0;
		n=milena_recognize(cfg,str,outbuf,buflen,pos);
		if (n>=0) {
			pos=n;
			nword++;
			clear_context();
			goto floop;
		}
		if (is_uri()) {
			last_num=-1;
			bword=NULL;
			nword++;
			clear_context();
			goto aloop;
		}
		if (is_domain(0)) {
			last_num=-1;
			bword=NULL;
			nword++;
			clear_context();
			goto aloop;
		}
		if (is_big_number()) {
		    bword=NULL;
		    nword++;
		    clear_context();
		    goto floop;
		}

		n=is_word(*str);
		

		if (!n) {
			char *c=mi_spell(cfg,**str,punct,some);
			if (c && (!nword || !strchr(",.!?;:",**str))) {
				if (pos) pushbuf(' ');
				pushstr(c);
				nword++;
				(*str)++;
				last_num=-1;
				clear_context();
				continue;
			}
		}
		if (spellme && n==MLWORD_SPEAKABLE) n=MLWORD_UNSPEAK;
		if (n) {
			struct milena_udic *ud;
			nword++;
			if (!bword) {
				for (ud=cfg->udic;ud;ud=ud->next) {
					int nphr;
					struct phraser_dic_rc phra[16];
					char *cs;
					if (ud->flags & MILDIC_ATSTART) {
						if(pos) continue;
					}
					cs=*str;
					if (!phraser_CompareDic(cfg,ud->word,str,phra,&nphr)) continue;
					if (ud->flags & MILDIC_ATQUE) {
						if (phraser_at_end(cfg,*str,0)!=2) {
							*str=cs;
							continue;
						}
					}

					if (ud->flags & MILDIC_BEFORECAP) {
						if (!is_cap(*str)) {
							*str=cs;
							continue;
						}
					}
					if (ud->flags & MILDIC_BEFOREUNCAP) {
						if (is_cap(*str)) {
							*str=cs;
							continue;
						}
					}
					if (ud->flags & MILDIC_ATEND) {
						if (!phraser_at_end(cfg,*str,0)) {
							*str=cs;
							continue;
						}
					}
					if (ud -> flags & MILDIC_SPELL) {
						*str=cs;
						n=MLWORD_UNSPEAK;
						break;
					}
					if (pos) {
						pushbuf(' ');
						if (uci[(*cs) & 255]) {
							pushstr("[F]");
						}
					}
					pos=phraser_OutputDic(cfg,ud,pos,outbuf,buflen,phra,nphr);
					if (ud->pron && !strcmp(ud->pron,"oko³o")) kolo=1;
					if (ud->pron) push_context(ud->pron);
					else clear_context();
					last_num=-1;
					goto floop;
				}

				if (n==MLWORD_SPEAKABLE) {
					int ism=0;
					if (is_mac()) {
						elang=2;
						ism=1;
					}
					if (ism || is_apostr()) {
						last_num=-1;
						clear_context();
						goto floop;
					}
				}
				if (is_apostr()) {
						last_num=-1;
						clear_context();
						goto floop;
				}
			}
			if (pos) {
				pushbuf(' ');
				if (!bword && n==MLWORD_SPEAKABLE && uci[(**str) & 255]) {
					pushstr("[F]");
				}
				if (!bword && n==MLWORD_SPEAKABLE && elang==2) {
					pushstr("[Len]");
				}

			}
			if (bword) {
				if (n==MLWORD_SPEAKABLE)
					for(;bword<*str;bword++) pushbuf(*bword);
				bword=NULL;
			}
			if (n==MLWORD_SPEAKABLE) {
				push_context(*str);
				for (;**str;(*str)++) {
					char cx,*s;
					if (**str=='~' && !(cfg->phraser_mode & MILENA_PHR_IGNORE_TILDE)) {
						if (strchr(ESCHAR,(*str)[1])) {
							(*str)++;
							pushbuf('~');
							pushbuf(**str);
							continue;
						}
						break;
					}
					cx=lci[(**str) & 255];
					if (!cx || cx=='@') break;
					s=cfg->redef[cx & 255];
					if (s) {
						for (;*s;s++) {
							pushbuf(*s);
						}
						continue;
					}
					pushbuf(cx);
				}
				last_num=-1;
			}
			else if (n==MLWORD_NUMBER) {
				emit_number(0);
				clear_context();
			}
			else if (n==MLWORD_NICKY) {
				pos=milena_nicky(cfg,str,outbuf,buflen,pos,0);
				last_num=-1;
				clear_context();
			}
			else {
			    if (isdigit(**str) && (**str) !='0') {
				int n;char *s=*str,*c;
				for (n=0;*s && isdigit(*s);s++) {
				    n=(n*10)+((*s)-'0');
				}
				c=s;
				int fema;
				if (n<100000 && (c=milena_get_unit(cfg,&c,0,&fema,0))) {
				    emit_number(1);
				    nword++;
				    clear_context();
				    goto floop;
				}
			    }
			    pos=milena_spell(cfg,str,outbuf,buflen,pos,last_num);
			    clear_context();
			    last_num=-1;
			}
			elang=0;
floop:			if (**str=='-' && is_word((*str)+1)) dash=1;
		}
aloop:
		if (!dash) {
			IS_EMOTICON
			if (eof) goto final_part;
			if (strchr("()!-,.?:;",**str)) break;
		}
		if (!lci[(**str) & 255] && !mi_spell(cfg,**str,punct,some)) (*str)++;
	}
final_part:
	if (!nword) return -1;
	typ=eof;
	for (;**str;(*str)++) {
		char *c;
		if (isspace(**str)) continue;
		IS_EMOTICON
		if (eof) typ |= eof;
		if (!strchr("()!-,.?:;",**str)) break;
		if (**str=='.') {
			if ((*str)[1]=='.' && (*str)[2]=='.') {
				(*str)+=2;
				if (!typ) typ|=64;
				c=mi_spell(cfg,0x80,punct,some);
				if (c) {
					if (pos) pushbuf(' ');
					pushstr(c);
				}
			}
			else {
				if (c=mi_spell(cfg,'.',punct,some)) {
					if (pos) pushbuf(' ');
					pushstr(c);
				}
				typ |= 1;
			}
			continue;
		}
		c=mi_spell(cfg,**str,punct,some);
		if (c) {
			if (pos) pushbuf(' ');
			pushstr(c);
		}

		if (**str=='!') typ |=8;
		else if (**str=='?') typ |=4;
		else if (**str==':') typ |=16;
		else if (**str==';') typ |=16;
		else if (**str=='-') typ |=32;
		else typ |=2;
	}
	if (!typ) *pmode=0;
	else {
		if (!(typ & 31)) {
			if (typ == 64) {
				if (!**str) *pmode=4;
				else *pmode=7;
			}
			else if (typ==32) *pmode=8+1;
			else *pmode=8+7;
		}
		else if (typ & 4) {*pmode=2;if (typ & 32) (*pmode)+=8;}
		else if (typ & 8) {*pmode=3;if (typ & 32) (*pmode)+=8;}
		else if (typ & 1) *pmode=0;
		else if (typ & 16) *pmode=5;
		else *pmode=1;
	}
	if (!**str) *pmode |= 16;
	if (pos<buflen) {
		outbuf[pos]=0;
		return 0;
	}
	*str=ostr;
	return pos+1;
}

int milena_GetPhraseWithPunct(struct milena *cfg,
	char **str,char *outbuf,int buflen,int *pmode,int punct,char *some)
{
	return _milena_GetPhraseWithPunct(cfg,str,outbuf,buflen,pmode,punct,some);
}
int milena_GetPhrase(struct milena *cfg,
	char **str,char *outbuf,int buflen,int *pmode)
{
	return _milena_GetPhraseWithPunct(cfg,str,outbuf,buflen,pmode,0,NULL);
}

