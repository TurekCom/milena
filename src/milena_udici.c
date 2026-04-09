/*
 * milena_udici.c - Milena TTS system (phraser)
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

static void sort_wordpar(char *s)
{
	char *s1;
	int vfun(const void *v1,const void *v2)
	{
		return strlen(*(char **)v2)-strlen(*(char **)v1);
	}
	void sort_wordbar(char *s)
	{
		char buf[strlen(s)+1];
		int i,nt;
		for (i=nt=1;s[i];i++) if (s[i]=='|') nt++;
		if (nt>=2) {
			char *bms[nt];
			char *c;
			strcpy(buf,s);
			c=buf;
			bms[0]=c;
			for (i=nt=0;*c;c++) if (*c=='|') {
				*c=0;
				nt++;
				bms[nt]=c+1;
			}
			qsort(bms,nt,sizeof(*bms),vfun);
			for (i=0;i<nt;i++) {
				if (i) *s++='|';
				for (c=bms[i];*c;) *s++=*c++;
			}
		}
	}
	for (;;) {
		s=strchr(s,'(');
		if (!s) return;
		s1=strchr(s,')');
		if (!s1) return;
		s++;
		*s1=0;
		sort_wordbar(s);
		*s1=')';
		s=s1+1;
	}
}
	
static char *expand_udic_macro(struct milena *cfg,char *str)
{
	static char bufor[8192];
	char *c,*d;
	struct milena_udic_macro *um;
	c=bufor;
	for (;;) {
		while (*str && *str !='#') {
			*c++=*str++;
			continue;
		}
		if (!*str) break;
		str++;
		if (*str++!='(') {
			m_gerror(cfg,"Blad skladni");
			return NULL;
		}
		d=str;
		str=strchr(str,')');
		if (!str) {
			m_gerror(cfg,"Blad skladni");
			return NULL;
		}
		*str++=0;
		for (um=cfg->udic_macros;um;um=um->next) if (!strcmp(um->name,d)) break;
		if (!um) {
			m_gerror(cfg,"Nieznane makro");
			return NULL;
		}
		strcpy(c,um->value);
		c+=strlen(c);
	}
	*c=0;
	return bufor;
}


static int good_pron(char *c)
{
	while (*c) {
		if (isspace(*c) || islower(*c) || strchr("_±ê¶æñó¼¿³¹è@'",*c)) {
			c++;
			continue;
		}
		if (*c=='%') {
			c++;
			if (*c && isdigit(*c)) c++;
			continue;
		}
		
		if (*c=='[') {
			c=strchr(c,']');
			if (!c) return 0;
			c++;
			continue;
		}
		if (*c!='~') return 0;
		c++;
		if (!*c || !strchr(ESCHAR,*c)) return 0;
		c++;
	}
	return 1;
}

static int read_udic_line(struct milena *cfg,FILE *f,char *instr,int dict_flags)
{
	char input_buf[8192];
	char *word,*pron,*fgs,*c;
	int flags=0;
	int stress=0;
	int pstress=0;
	struct milena_udic *ud;
	int atend,issingle;
	
	if (f) {
		if (!fgets(input_buf,8192,f)) return 0;
		cfg->input_line++;
	}
	else strcpy(input_buf,instr);
	c=strstr(input_buf,"//");
	if (c) *c++=0;
		
	word=input_buf;
	while (*word && isspace(*word)) word++;
	if (!*word) return 1;
	if (*word == '&') {
	    word++;
	    if (milena_read_phraser_line(cfg,word,1)) {
		return 0;
	    }
	    return 1;
	}
	if (*word == '#') {
		char *macname;
		struct milena_udic_macro *um;
		word++;
		while (word && isspace(*word)) word++;
		if (!*word) gerror(cfg,"Blad skladni");
		macname=word;
		while (*word && !isspace(*word)) word++;
		if (*word) *word++=0;
		if (!*word) gerror(cfg,"Puste makro");
		rtrim(word);
		um=qalloc(sizeof(*um));
		um->next=cfg->udic_macros;
		cfg->udic_macros=um;
		um->name=strdup(macname);
		um->value=strdup(word);
		return 1;
		
	}
	pron=word;
	while (*pron && !isspace(*pron)) pron++;
	if (*pron) {
		*pron++=0;
		while (*pron && isspace(*pron)) pron++;
	}
	if (!*pron) {
		struct milena_stringlist *sl;
		if (!(dict_flags & MILENA_UDIC_DICTMODE)) { 
			if (dict_flags & MILENA_UDIC_IGNORE) return 1;
			gerror(cfg,"Slowo bez znaczenia");
		}
		sl=qalloc(sizeof(*sl));
		sl->string=strdup(word);
		sl->next=cfg->dict_known_words;
		cfg->dict_known_words=sl;
		return 1;
	}
		
	fgs=strchr(pron,'$');
	if (fgs) *fgs++=0;
	rtrim(pron);
	atend=0;
	if (fgs && *fgs=='#') {
		fgs++;
		atend=1;
	}
	for (;fgs && *fgs;fgs++) {
		int pf=0;
		if (isspace(*fgs)) continue;
		if (isdigit(*fgs)) {
			stress=*fgs-'0';
			continue;
		}
		if (*fgs=='+' && fgs[1] && isdigit(fgs[1])) {
			fgs++;
			pstress=*fgs-'0';
			continue;
		}
		if (*fgs=='v') pf=MILDIC_V;
		else if (*fgs=='o') pf=MILDIC_U|MILDIC_V;
		else if (*fgs=='u') pf=MILDIC_U;
		else if (*fgs=='p') pf=MILDIC_P|MILDIC_U;
		else if (*fgs=='P') pf=MILDIC_R|MILDIC_P|MILDIC_U;
		else if (*fgs=='r') pf=MILDIC_R|MILDIC_U;
		else if (*fgs=='S') pf=MILDIC_SPELL;
		else if (*fgs=='e') pf=MILDIC_ATEND;
		else if (*fgs=='s') pf=MILDIC_ATSTART;
		else if (*fgs=='O') pf=MILDIC_ATEND|MILDIC_ATSTART;
		else if (*fgs=='q') pf=MILDIC_ATQUE;
		else if (*fgs=='U') pf=MILDIC_BEFORECAP;
		else if (*fgs=='L') pf=MILDIC_BEFOREUNCAP;
		else gerror(cfg,"Nieznana flaga");
		//if (!flags) flags=pf;
		flags |= pf;
	}
	ud=qalloc(sizeof(*ud));
	if (strstr(word,"#(")) word=expand_udic_macro(cfg,word);
	if (!word) return -1;
	ud->word=strdup(word);
	sort_wordpar(ud->word);
	issingle=1;
	if (strpbrk(word,"-~`+_")) {
		issingle=0;
	}
	if (pron && *pron) {
		if (flags & MILDIC_SPELL) gerror(cfg,"Przy fladze S niepotrzebna wymowa");
		if (!good_pron(pron)) gerror(cfg,"Bledny znak w translacji");
		ud->pron=strdup(pron);
	}
	else if (strpbrk(word,"(*-~`+_")) {
		gerror(cfg,"Wymowa konieczna przy rozwinieciach");
	}
	else ud->pron=NULL;
	if (!atend) {
		struct milena_udic **uud=&cfg->udic;
		if (issingle) {
			for (;*uud;uud=&(*uud)->next) {
				if (!strpbrk((*uud)->word,"-~`+_")) break;
			}
		}
		ud->next=*uud;
		*uud=ud;
	}
	else {
		struct milena_udic **uud;
		for (uud=&cfg->udic;*uud;uud=&(*uud)->next) {
			if (!strcmp((*uud)->word,word)) break;
		}
		ud->next=*uud;
		*uud=ud;
	}
	if (!flags && !stress && !pstress) stress=2;
	ud->flags=flags;
	ud->stress=stress;
	ud->pstress=pstress;
	return 1;
}

int milena_ReadUserDicLineWithFlags(struct milena *cfg,char *instr,int flags,
	char *fname,int lineno)
{
	cfg->fname=fname;
	cfg->input_line=lineno;
	return read_udic_line(cfg,NULL,instr,flags);
}


static int _milena_ReadUserDicWithFlags(struct milena *cfg,char *fname,int flags)
{
	FILE *f;int n;
	f=fopen(fname,"rb");
	if (!f) {
		perror(fname);
		return 0;
	}
#ifdef __WIN32
	my_check_file_encoding(f);
#endif
	cfg->fname=fname;
	cfg->input_line=0;
	for (;;) {
		n=read_udic_line(cfg,f,NULL,flags);
		if (n<=0) break;
	}
	fclose(f);
	if (n<0) return 0;
	return 1;
}

int milena_ReadUserDicWithFlags(struct milena *cfg,char *fname,int flags)
{
	return _milena_ReadUserDicWithFlags(cfg,fname,flags);
}
int milena_ReadUserDic(struct milena *cfg,char *fname)
{
	return _milena_ReadUserDicWithFlags(cfg,fname,0);
}
