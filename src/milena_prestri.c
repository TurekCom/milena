/*
 * milena_prestri.c - Milena TTS system (prestresser)
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
static struct milena_dic *milena_find_dic(struct milena *cfg,char *word)
{
	int lo,hi,mid,n,len;
	for (lo=0,hi=cfg->milena_dic_count-1;lo<=hi;) {
		mid=(lo+hi)/2;
		n=strcmp(word,cfg->milena_dic[mid].word);
		if (!n) return cfg->milena_dic+mid;
		if (n<0) hi=mid-1;
		else lo=mid+1;
	}
	len=strlen(word);
	if (len>=5 && !strcmp(word+len-2,"¿e")) {
		char wrd2[len];
		memcpy(wrd2,word,len-2);
		wrd2[len-2]=0;
		for (lo=0,hi=cfg->milena_dic_count-1;lo<=hi;) {
			mid=(lo+hi)/2;
			n=strcmp(wrd2,cfg->milena_dic[mid].word);
			if (!n) break;
			if (n<0) hi=mid-1;
			else lo=mid+1;
		}
		if (lo > hi) return NULL;
		if (cfg->milena_dic[mid].flags & MILDIC_VIMPT) {
			static struct milena_dic mdx;
			mdx.flags=MILDIC_V;
			mdx.stress=3;
			return &mdx;
		}
	}

	return NULL;
}



/* local verbs */
#include "milena_verbdic.h"

int local_chverb(const char *word, struct milena *cfg)
{
    const char *stri(int n)
    {
        return (const char *)(verbmemo+n);
    }

    const char *startswith(const char *word, const char *needle)
    {
        while (*needle) {
            if (*word++ != *needle++) return NULL;
        }
        return word;
    }

    int reverse2(char *dst, const char *src)
    {
        int d=strlen(src);
        int i;
        for (i=0;i<d;i++) dst[d-i-1]=src[i];
        dst[d]=0;
        return d;
    }

    int ikamatch(const char *str, const char *patr)
    {
        while (*patr) {
            if (*patr == '#') {
                return !*str;
            }
            if (!*str) return 0;
            if (*patr == 'A') {
                return is_samog(cfg, *str);
            }
            if (*str++ != *patr++) return 0;
        }
        return 1;
    }


    if (strlen(word) > 31) return -1;
    char revbuf[32];
    reverse2(revbuf, word);
    char *xs=revbuf;
    int csa=0;
        for (;*xs && csa < 2;xs++) {
        if (*xs == 'i') {
            csa++;
            if (is_samog(cfg, *xs)) xs++;
        }
        else if (is_samog(cfg,*xs)) csa++;
    }
    if (csa < 2) return -2;
    int i, typ;
    for (i=0; i< verbsuf_len; i++) {
        const char *rs=stri(verbsufdata[i].offset);
        const char *c=startswith(revbuf,rs);
        if (!c) continue;
        typ = verbsufdata[i].verb;
        int j;
        if (verbsufdata[i].len) {
            int fnd=0;
            for (j=0;j<verbsufdata[i].len;j++) {
                rs += strlen(rs)+1;
                //printf("Compare %s %s\n", rs, c);
                if (!strcmp(c,rs)) {
                        //printf("found\n");
                    fnd=1;
                    break;
                }
            }
            if (!fnd) typ = 1-typ;
        }
        break;
    }
    //return typ;
    if (!typ) return 0;
    int stress=0;
    for (i=0;i<verbsfx_count; i++) {
        if (ikamatch(revbuf,stri(verbsfxdata[i].offset))) {
            stress = verbsfxdata[i].stress;
            break;
        }
    }
    return 1+stress;


}


/*
 * <jedna sylaba> ¿e¶cie | ¿e¶my - akcent na trzeciej (done)
 * przymiotnik po³±czony z ¶cie/¶my na trzeciej od koñca
 *   pl:m1.f:nom ¶my|¶cie
 *   sg:m1.f:nom.inst ¶cie
 * rzeczownik [acc] zakoñczony samog³osk± - np "wódkê¶cie pili" ¶cie/¶my
 */

static int milena_word_info(struct milena *cfg,char *word,int *flags,int *stress,int *pstress)
{
	struct milena_dic *md=milena_find_dic(cfg,word);
	int len,ucase;
	struct mika_bfx *bfx;
	struct mika_afx *afx;
	struct mika_sfx *sfx,*rem_sfx,esfx;

	static struct {
		char *s;
		int len;
		int str;
		int syl;
	} pre_sto[]={
		{"stu",3,1,1},
		{"dwustu",6,1,2},
		{"trzystu",7,1,2},
		{"czterystu",9,1,3},
		{"piêciuset",9,1,3},
		{"piêæset",7,1,2},
		{"sze¶ciuset",10,1,3},
		{"sze¶æset",8,1,2},
		{"siedmiuset",10,1,3},
		{"siedemset",9,1,3},
		{"o¶miuset",8,1,3},
		{"osiemset",8,1,3},
		{"dziewiêciuset",11,2,4},
		{"dziewiêæset",9,1,3},
		{NULL,0,0,0}},
	pre_dec[]={
		{"dwudziesto",10,2,3},
		{"dwudziestu",10,2,3},
		{"trzydziesto",11,2,3},
		{"trzydziestu",11,2,3},
		{"czterdziesto",12,2,3},
		{"czterdziestu",12,2,3},
		{"piêædziesiêcio",14,3,4},
		{"piêædziesiêciu",14,3,4},
		{"sze¶ædziesiêcio",15,3,4},
		{"sze¶ædziesiêciu",15,3,4},
		{"siedemdziesiêciu",16,4,5},
		{"siedemdziesiêcio",16,4,5},
		{"osiemdziesiêcio",15,4,5},
		{"osiemdziesiêciu",15,4,5},
		{"dziewiêædziesiêcio",18,4,5},
		{"dziewiêædziesiêciu",18,4,5},
		{NULL,0,0,0}},
	pre_sin1[]={
		{"jedno",5,1,2},
		{"dwu",3,1,1},
		{"trzy",4,1,1},
		{"trój",4,1,1},
		{"cztero",6,1,2},
		{"piêcio",6,1,2},
		{"sze¶cio",7,1,2},
		{"siedmio",7,1,2},
		{"o¶mio",5,1,2},
		{"dziewiêcio",10,2,3},
		{"pierwszo",8,1,2},
		{"drugo",5,1,2},
		{"trzecio",7,1,2},
		{"czwarto",7,1,2},
		{"pi±to",5,1,2},
		{"szósto",6,1,2},
		{"siódmo",6,1,2},
		{"ósmo",4,1,2},
		{"dziewi±to",9,2,3},
		{"kilku",5,1,2},
		{"paro",4,1,2},
		{"paru",4,1,2},
		{NULL,0,0,0}},
	pre_sin2[]={
		{"dziesiêcio",10,2,3},
		{"dziesi±to",9,2,3},
		{"jedenasto",9,3,4},
		{"jedenastu",9,3,4},
		{"dwunasto",8,2,3},
		{"dwunastu",8,2,3},
		{"trzynasto",9,2,3},
		{"trzynastu",9,2,3},
		{"czternasto",10,2,3},
		{"czternastu",10,2,3},
		{"piêtnasto",9,2,3},
		{"piêtnastu",9,2,3},
		{"szesnasto",9,2,3},
		{"szesnastu",9,2,3},
		{"siedemnasto",11,3,4},
		{"siedemnastu",11,3,4},
		{"osiemnasto",10,3,4},
		{"osiemnastu",10,3,4},
		{"dziewiêtnasto",13,3,4},
		{"dziewiêtnastu",13,3,4},
		{"kilkunasto",10,3,4},
		{"kilkunastu",10,3,4},
		{"parunasto",9,3,4},
		{"parunasto",9,3,4},
		{"kilkudziesiêcio",15,4,5},
		{"kilkudziesiêciu",15,4,5},
		{"parudziesiêciu",14,4,5},
		{"parudziesiêcio",14,4,5},
		{NULL,0,0,0}};


	int count_syl(char *c)
	{
		int cs;
		for (cs=0;*c;c++) {
			if (!strchr("aeiouy±êóüöáéíúý",*c)) continue;
			cs++;
			if (*c=='i' && strchr("aoeuó±ê",c[1])) {
				c++;
				continue;
			}
			/*
			if (c[1]=='u' && strchr("ae",*c)) {
				c++;
				continue;
			}
			*/
		}
		return cs;
	}

	int rmatch(char *rpat,int n)
	{
		int i;
		for (;;) {
			if (!*rpat) return n;
			if (*rpat=='#') {
				if (n>0) return -1;
				return n;
			}
			if (n<=0) return -1;
			if (*rpat=='@') { // samogloska
			    if (is_samog(cfg,word[n-1])) return n;
			    return -1;
			}
			if (*rpat=='A') {
				for (i=0;i<n;i++) if (strchr("aeiouy±êó",word[i])) return n;
				return -1;
			}
			if (*rpat=='B') {
				for (i=0;i<n;i++) if (word[i] & 0x80) return n;
				return -1;
			}
			if (*rpat == '1') {
				int cs=0;
				for (i=0;i<n;i++) {
					if (!strchr("aeiouy±êó",word[i])) continue;
					if (cs) return -1;
					if (word[i]=='i' && strchr("aeou±êó",word[i+1])) i++;
					cs=1;
				}
				if (cs != 1) return -1;
				return n;
			}
			if (word[n-1]!=*rpat) return -1;
			rpat++;
			n--;
		}
	}

	void prestress_licz(char *word)
	{
		int syl,str;
		int i;
		syl=0;
		str=0;
		if (!strncmp(word,"tysi±c",6)) {
			str=1;
			syl=2;
			word+=6;
		}
		for (i=0;pre_sto[i].s;i++) if (!strncmp(word,pre_sto[i].s,pre_sto[i].len)) {
			str=syl+pre_sto[i].str;
			syl+=pre_sto[i].syl;
			word+=pre_sto[i].len;
			break;
		}
		for (i=0;pre_sin2[i].s;i++) if (!strncmp(word,pre_sin2[i].s,pre_sin2[i].len)) {
			*pstress=syl+pre_sin2[i].str;
			return;
		}
		for (i=0;pre_dec[i].s;i++) if (!strncmp(word,pre_dec[i].s,pre_dec[i].len)) {
			str=syl+pre_dec[i].str;
			syl+=pre_dec[i].syl;
			word+=pre_dec[i].len;
			break;
		}
		for (i=0;pre_sin1[i].s;i++) if (!strncmp(word,pre_sin1[i].s,pre_sin1[i].len)) {
			*pstress=syl+pre_sin1[i].str;
			return;
		}
		if (str) *pstress=str;
	}



	int syls;

	if (md) {
		*flags=md->flags;
		*stress=md->stress;
		return 1;
	}

	len=strlen(word);

#ifdef HAVE_MORFOLOGIK
	
	if (cfg->md) {
		int rc;
		if ((rc=morf_is_verb_w(cfg,word))) {
			//fprintf(stderr,"VB [%s] %d\n",word,rc);
			*flags=MILDIC_V;
			if (rc > 1) * stress=rc;
			return 2;
		}
	}
	else
#endif
    {

        int rc=local_chverb(word, cfg);
        //fprintf(stderr,"W='%s', rc=%d\n", word,rc);
        if (rc>0) {
            *flags = MILDIC_V;
            if (rc > 1) *stress = rc-1;
            return 2;
        }
    }
#if 0
	for (bfx=cfg->vsfx;bfx;bfx=bfx->next) {
		int n=rmatch(bfx->afx,len);
		if (n>=0) {
			if (count_syl(word)>bfx->stress+1) {
				struct milena_prefix *px;
				for (px=cfg->prefiksy;px;px=px->next) {
					if (!strncmp(px->prefix,word,strlen(px->prefix))) {
						*pstress=px->pstress;
						break;
					}
				}
			}
			*flags=MILDIC_V;
			*stress=bfx->stress;
			return 2;

		}
	}
#endif

	syls=count_syl(word);
	ucase=(*flags) & MILDIC_UCA;
	
	
	
	/* sfx musz± byæ obrobione przed extwords */
	sfx=NULL;

	if (syls>1) {
		for (sfx=cfg->sfx;sfx;sfx=sfx->next) {
			if ((sfx->flags & MILDIC_UCA) && !ucase) continue;
			if (rmatch(sfx->afx,len)>=0) break;
		}
	}
	rem_sfx=sfx;
	if (!rem_sfx && syls>1 && cfg->ext_verbs_no) {
		char bf[len+1];
		int i,lo,hi,mid;
		for (i=0;i<len;i++) bf[len-i-1]=word[i];
		bf[len]=0;
		lo=0;
		hi=cfg->ext_verbs_no-1;
		while (lo<=hi) {
			mid=(lo+hi)/2;
			i=strncmp(bf,cfg->ext_verbs[mid],strlen(cfg->ext_verbs[mid]));
			if (!i) {
				*flags=MILDIC_V;
				if (!strncmp("e¿",bf,2) && strchr("¼ñjbæ",bf[2])) *stress=3;
				return 2;
			}
			if (i<0) hi=mid-1;
			else lo=mid+1;
		}

	}
	if (syls<2) return 0;
	if (!strncmp(word,"arcy",4)) {
		int n=count_syl(word+4);
		if (n==1) {
			*stress=1;
			return 2;
		}
		if (n>1) {
			*pstress=1;
		}
	}
	else {
		struct milena_prefix *px;
		for (px=cfg->prefiksy;px;px=px->next) {
			if (!strncmp(px->prefix,word,strlen(px->prefix))) {
				*pstress=px->pstress;
				break;
			}
		}
		if (!px) prestress_licz(word);
	}
	if (rem_sfx) {
		if (!*pstress) *pstress=rem_sfx->pstress;
		if (!*stress) *stress=rem_sfx->stress;
		if (!*flags) *flags=rem_sfx->flags;
		return 1;
	}
	for (sfx=cfg->sfx;sfx;sfx=sfx->next) {
		if ((sfx->flags & MILDIC_UCA) && !ucase) continue;
		if (rmatch(sfx->afx,len)>=0) {
			if (!*pstress) *pstress=sfx->pstress;
			if (!*stress) *stress=sfx->stress;
			if (!*flags) *flags=sfx->flags;
			return 1;
		}
	}
	int n=-1,m=-1;

	for (afx=cfg->afx;afx;afx=afx->next) {
		n=rmatch(afx->afx,len);
		if (n>0) break;
	}
	if (n>0) {
		for (bfx=cfg->bfx;bfx;bfx=bfx->next) {
			m=rmatch(bfx->afx,n);
			if (m>=0) {
				*stress=bfx->stress;
				break;
			}
		}
	}

	//return 0;

	if (len>3 && strchr("lLdD",word[0]) && word[1]=='&') {
		if (strchr("aeiou±ê",word[len-1])) {
			*stress=0;
			*flags=0;
			return 0;
		}
		if (rmatch("me",len)>0 ||
			rmatch("yts",len)>0 ||
			rmatch("my",len)>0 ||
			rmatch("miks",len)>0 ||
			rmatch("ywo",len)>0) {
			*stress=0;
			*flags=0;
			return 0;
		}
		*stress=1;
		*flags=0;
		return 1;
	}

	return 0;

}

int milena_Prestresser(struct milena *cfg,char *str,char *outbuf,int buflen)
{
	int flags,stress,pstress,modf,lang;
	struct {
		char word[256];
		int flags;
		int stress;
		int pstress;
		int modifier;
		int spanish;
		int lang;
	} *slowa;
	int nslow,mslow,i,j,n,count_stress,last_op;
	char *c;
	int pos=0;
	int lpos;
	int spanish_stress;

	int good_char(char *s)
	{
		int n=lci[(*s) & 255];
		if (!n) n=*s;
		if (cfg->letters[n&255] & 3) return n;
		if (*s != '~') return 0;
		if (s[1]=='!' || s[1]=='\'') return 1;
		return 0;
	}
	int is_spanish(int znak,char *str)
	{
		if (!strchr("áéíúýó",znak)) return 0;
		if (znak == 'í' && good_char(str) && is_samog(cfg,*str)) {
			int cs=0;
			for (;*str && good_char(str);str++) {
				if (is_samog(cfg,*str)) cs += 1;
			}
			if (cs > 1) return 0;
			return 1;
		}
			
		if (znak == 'ó') {
			if (!strchr("nlrm",lci[(*str++) & 255])) return 0;
			if (lci[(*str++) & 255] != 'e') return 0;
			if (!strchr("sz",lci[(*str++) &255])) return 0;
			if (good_char(str)) return -1;
			return 2;
		}
		if (!strchr("nlrm",lci[(*str++) & 255])) return 1;
		if (lci[(*str++) & 255] != 'e') return 1;
		if (!strchr("sz",lci[(*str++) &255])) return 1;
		//fprintf(stderr,"UJ[%s]\n",str);
		if (good_char(str)) return 0;
		return 1;
	}
	
	slowa=malloc(sizeof(*slowa) * (mslow=64));
	count_stress=0;last_op=-1;
	for (nslow=0;;) {
		flags=0;
		stress=0;
		pstress=0;
		modf=0;
		lang=0;
		spanish_stress=-1;
		for (;*str;str++) {
			if (*str=='[') {
				str++;
				for (;*str && *str!=']';str++) {
					if (*str=='n') flags |= MILDIC_U;
					else if (*str=='L') {
						int k;
						for (k=0;milena_langs[k];k++) {
							if (!strncmp(milena_langs[k],str+1,2)) {
								lang |= 1 <<k;
								str+=2;
								break;
							}
						}
					}
					else if (*str=='F') flags |= MILDIC_UCA;
					else if (*str=='f') flags |= MILDIC_FG;
					else if (*str=='k') flags |= MILDIC_K;
					else if (*str=='s') flags |= MILDIC_SEP;
					else if (*str=='b') flags |= MILDIC_SUPERSEP;
					else if (isdigit(*str)) stress=*str-'0';
					else if (*str=='+' && str[1] && str[1]!=']') {
						str++;
						pstress=*str-'0';
					}
				}
				if (!*str) break;
				continue;
			}
			if (*str=='{') {
				str++;
				for (;*str && *str!='}';str++) {
					if (*str=='v') flags |= MILDIC_V;
					else if (*str>='0' && *str<='9') modf=*str;
				}
				if (!*str) break;
				continue;
			}
			if (!isspace(*str)) break;
		}
		if (!*str) break;
		if (!good_char(str)) break;
		i=0;
		for (c=str;*c;c++) {
			if (*c!='~') {
				n=good_char(c);
				if (!n) break;
				if (i>254) continue;
				if ((lang | cfg->language_mode) & MILENA_LANG_ES) {
					int t=is_spanish(n,c+1);
					if (t>0) {
					//if (strchr("áéíúý",n)) {
						if (t>1) n='o';
						if (spanish_stress==-1) spanish_stress=i;
						else spanish_stress=-2;
					}
					else if (t<0) n='o';
				}
				slowa[nslow].word[i++]=n;
				continue;
			}
			if (!c[1] || !strchr(ESCHAR,c[1])) break;
			if (i >= 253) continue;
			slowa[nslow].word[i++]=*c++;
			if (*c=='!' || *c==',') flags |= MILDIC_STRESSED;
			slowa[nslow].word[i++]=*c;
		}
		//fprintf(stderr,"WORD NUMBER %d\n",nslow);
		if (nslow >= mslow-1) {
			mslow=2*mslow;
			//fprintf(stderr,"REALLOCATION %d/%d\n",nslow,mslow);
			slowa=realloc(slowa,mslow*sizeof(*slowa));
		}
		slowa[nslow].word[i]=0;
		slowa[nslow].flags=flags;
		slowa[nslow].stress=stress;
		slowa[nslow].modifier=modf;
		slowa[nslow].lang=lang;
/* koñcówka 'ón' - do  es_stress
		if ((cfg->language_mode & MILENA_LANG_ES) &&
			spanish_stress == -1 &&
			i>2 &&
			slowa[nslow].word[i-2]=='ó' &&
			slowa[nslow].word[i-1]=='n') {
				slowa[nslow].word[i-2]='o';
				spanish_stress=i-2;
		}
*/
		slowa[nslow].spanish=spanish_stress;
		slowa[nslow++].pstress=pstress;
		str=c;
	}
	//fprintf(stderr,"Total words %d\n",nslow);
	for (i=0;i<nslow;i++) {
		if ((slowa[i].flags & ~MILDIC_UCA) || slowa[i].stress || slowa[i].pstress) continue;
		milena_word_info(cfg,slowa[i].word,&slowa[i].flags,&slowa[i].stress,&slowa[i].pstress);
	}
	//for (i=0;i<nslow;i++) fprintf(stderr,"%d %04X %d\n",i,slowa[i].flags ,slowa[i].stress);
	// korekta op-r-$
	if (nslow >=2 &&
		(slowa[nslow-2].flags & (MILDIC_V | MILDIC_U))==(MILDIC_V | MILDIC_U) &&
		(slowa[nslow-1].flags & MILDIC_U)) {
			slowa[nslow-1].flags &= ~(MILDIC_R | MILDIC_U);

	}
	// korekta form grzeczno¶ciowych
	for (i=0;i<nslow-1;i++) {
		if ((slowa[i].flags & MILDIC_FG) && (slowa[i+1].flags & (MILDIC_V | MILDIC_UCA))) {
			slowa[i].flags |= MILDIC_U;
			if (i) slowa[i-1].flags &= ~MILDIC_U;
		}
	}

	for (i=0;i<nslow-1;i++) {
		if (slowa[i].flags & MILDIC_N) {
			if (slowa[i+1].flags & MILDIC_V) {
				slowa[i].flags |= MILDIC_K;
				slowa[i].flags &= ~MILDIC_U;
				//slowa[i].flags |= MILDIC_U;
				//if (i) slowa[i-1].flags &= ~MILDIC_U;
			}
			continue;
		}
		if (slowa[i].flags & MILDIC_P) {
			if (slowa[i+1].flags & MILDIC_R) slowa[i].flags |= MILDIC_K;
			continue;
		}
		if (i==nslow-2 && (slowa[i+1].flags & MILDIC_NEVER)) {
			/*
			MILDIC_NEVER jest tylko do jednosylabowych!
			*/
			if(slowa[i].flags & MILDIC_U) {
				slowa[i].flags &= ~MILDIC_U;
			}
			/*
			else {
			slowa[i].flags |= MILDIC_ESTRESS;
			}

			else if (slowa[i].stress >= 3) {
				slowa[i+1].flags &= ~(MILDIC_U|MILDIC_NEVER);
				slowa[i+1].stress=2;

			}
			else if (slowa[i].stress>=3) {
				//slowa[i].stress=1;
				slowa[i].flags |= MILDIC_ESTRESS;
			}
			*/
		}
	}

	/* korekta dla pojedynczego operatora */
	/* w trybie lektorskim sprawdzamy tylko czasowniki */
	/* co jest bez sensu
	for (i=0;i<nslow;i++) {
		if (!(slowa[i].flags & MILDIC_U)) {
			if (!cfg->ext_verbs || (slowa[i].flags & MILDIC_V)) count_stress++;
		}
		else if (slowa[i].flags & MILDIC_V) last_op=i;
	}
	if (!count_stress && last_op>=0 && (last_op == nslow-1 || (slowa[last_op+1].flags & MILDIC_U))) slowa[last_op].flags &=~MILDIC_U;
	*/

	/* inna wersja */
	for (i=0;i<nslow;i++) {
		if (!(slowa[i].flags & MILDIC_U)) continue;
		if (!(slowa[i].flags & MILDIC_V)) continue;
		if (i==nslow-1 || (slowa[i+1].flags & MILDIC_U)) {
			slowa[i].flags &= ~MILDIC_U;
		}
	}
	/* korekta dla SEP */
	lpos=0;
	if (cfg->ext_verbs_no
#ifdef HAVE_MORFOLOGIK
	 || cfg->md
#endif
	 ) while (lpos<nslow) {
		int found_verb=0;
		int last_sep=-1;
		for (i=lpos;i<nslow;i++) {
			if (slowa[i].flags & MILDIC_V) {
				if (last_sep>0) break;
				found_verb=1;
			}
			else if (slowa[i].flags & MILDIC_SEP) {
				if (found_verb && i>lpos) {
					if (last_sep != i-1) last_sep=i;
				}
				slowa[i].flags &= ~MILDIC_SEP;
			}
		}
		if (last_sep<0) {
			for (j=lpos;j<nslow;j++) slowa[j].flags &= ~MILDIC_SEP;
			lpos=nslow;
			break;
		}
		for (j=last_sep+1;j<nslow;j++) if (slowa[j].flags & MILDIC_V) break;
		if (j>=nslow) {
			for (j=i+1;j<nslow;j++) slowa[j].flags &= ~MILDIC_SEP;
			lpos=nslow;
			break;
		}
		slowa[last_sep].flags |= MILDIC_SEP;
		lpos=last_sep+1;

	}

	/* korekta akcentu operatora */
	for (i=0;i<nslow-1;i++) {
		if (!(slowa[i].flags & MILDIC_VOP)) continue;
		if (!(slowa[i+1].flags & MILDIC_V)) continue;
		if (slowa[i+1].word[strlen(slowa[i+1].word)-1] != 'æ') continue;
		if ((i < nslow-2) && (slowa[i+2].flags & MILDIC_HIM)) {
			slowa [i+1].flags &= ~MILDIC_U;
			slowa [i].flags |= MILDIC_U;
			i++;
			continue;
		}

		/* uproszczenie 1 */
		slowa [i+1].flags |= MILDIC_U;
		slowa [i].flags &= ~MILDIC_U;
		i++;
	}

	for (i=0;i<nslow;i++) {
		spanish_stress=slowa[i].spanish;
		if (slowa[i].stress || slowa[i].flags & MILDIC_STRESSED) spanish_stress=-1;
		if (i) pushbuf(' ');
		if (slowa[i].modifier || (slowa[i].flags & MILDIC_V)) {
			pushbuf('{');
			if (slowa[i].modifier) {
				pushbuf(slowa[i].modifier);
			}
			if (slowa[i].flags & MILDIC_V) {
				pushbuf('v');
			}
			pushbuf('}');
		}
		if (slowa[i].lang || slowa[i].stress || slowa[i].pstress || (slowa[i].flags & (MILDIC_NEVER | MILDIC_NU | MILDIC_K | MILDIC_U | MILDIC_SEP | MILDIC_ESTRESS | MILDIC_VOP | MILDIC_HIM | MILDIC_SUPERSEP))) {
			pushbuf('[');
			if (slowa[i].lang) {
				int k;
				for (k=0;milena_langs[k];k++) if (slowa[i].lang & (1<<k)) {
					pushbuf('L');
					pushstr(milena_langs[k]);
				}
			}
			if (slowa[i].stress) pushbuf(slowa[i].stress+'0');
			if (slowa[i].pstress) {
				pushbuf('+');
				pushbuf(slowa[i].pstress+'0');
			}
			if (slowa[i].flags & MILDIC_K) pushbuf('k');
			if (slowa[i].flags & MILDIC_ESTRESS) pushbuf('e');
			if (slowa[i].flags & MILDIC_SEP) pushbuf('s');
			if (slowa[i].flags & MILDIC_SUPERSEP) pushbuf('b');
			if (slowa[i].flags & MILDIC_VOP) pushbuf('w');
			if (slowa[i].flags & MILDIC_HIM) pushbuf('h');
			//fprintf(stderr,"%d %s %x\n",i,slowa[i].word,slowa[i].flags);
			if (!i || i<nslow-1 || (!(slowa[i].flags & MILDIC_L) && !(slowa[i-1].flags & MILDIC_K))) {
				if (i==nslow-1 && (slowa[i].flags & MILDIC_NEVER)) pushbuf('q');
				else {
					if (slowa[i].flags & MILDIC_U) pushbuf('n');
					if (slowa[i].flags & MILDIC_NU) pushbuf('u');
				}
			}
			pushbuf(']');
		}
		if (slowa[i].flags & MILDIC_UCA && !slowa[i].word[1]) {
			if (slowa[i].word[0]=='w') {
				pushstr("wu");
				continue;
			}
			if (slowa[i].word[0]=='z') {
				pushstr("zet");
				continue;
			}
		}
		if (spanish_stress > 0) {
			pushbuf('~');
			pushbuf(',');
		}
		for (c=slowa[i].word;*c;c++) {
			if (! spanish_stress--) {
				pushbuf('~');
				pushbuf('!');
			}
			pushbuf(*c);
	}	}
	free(slowa);
	if (pos<buflen) {
		outbuf[pos]=0;
		return 0;
	}
	return pos+1;

}
