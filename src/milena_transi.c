/*
 * milena_transi.c - Milena TTS system (translator + poststresser)
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


#define EPN_VOWEL "aeiouyYMOEUI"

static unsigned char *next_char(unsigned char *c,int *sep)
{
	if (!*c) return c;
	c++;
	if (sep) *sep=0;
	for (;;) {
		if (*c=='~') { /* stress etc. control sequences */
			c++;
			if (*c=='\'' && sep) *sep=1;
			else if (*c=='+' && sep) *sep=2;
			if (*c) c++;
			continue;
		}
		if (*c=='[') { /* extra informations about word */
			c=(unsigned char *)strchr((char *)c,']');
			if (!c) return (unsigned char *)"";
			c++;
			continue;
		}
		if (*c=='{') { /* extra informations about word */
			c=(unsigned char *)strchr((char *)c,'}');
			if (!c) return (unsigned char *)"";
			c++;
			continue;
		}
		return c;
	}
}

static unsigned char *prev_char(unsigned char *c,unsigned char *start)
{
	unsigned char *d;
	if (!c) return 0;
	if (c<=start) return 0;
	c--;
	while (c>=start) {
		if (c>start) {
			d=c-1;
			if (*d=='~') {
				c=d-1;
				if (c<start) return 0;
				continue;
			}
		}
		if (*c=='}') {
			c--;
			while (c>=start && c[1] !='{') c--;
			if (c<start) return 0;
			continue;
		}
		if (*c==']') {
			c--;
			while (c>=start && c[1] !='[') c--;
			if (c<start) return 0;
			continue;
		}
		return c;
	}
	if (c<start) return 0;
	return c;
}


static int dopasuj_l(struct milena *cfg,unsigned char *str,struct ruler *ruler,unsigned char *start,unsigned char **cout)
{
	int nr,op;
	for (;ruler;ruler=ruler->next) {
		op=ruler->op;
		if (op & OP_STAR) {
			int n;unsigned char *c;
			if (ruler->arg) {
				n=dopasuj_l(cfg,str,(struct ruler *)(ruler->arg),start,&c);
				if (!n) return 0;
				str=c;
			}
			for (;;) {
				if (!str || !*str || isspace(*str)) return 0;
				n=dopasuj_l(cfg,str,ruler->next,start,&c);
				if (n) {
					str=c;
					return 1;
				}
				str=prev_char(str,start);
			}
			return 0;
		}
		if (op & OP_CHAR) {
			nr=op & 255;
			if (nr=='^') {
				if (str) return 0;
				continue;
			}
			if (nr=='#') {
				if (str && *str && !isspace(*str)) return 0;
				str=prev_char(str,start);
				continue;
			}
			if (!str || *str != nr) return 0;
			str=prev_char(str,start);
			continue;
		}
		if (op & OP_OR) {
			unsigned char *c;
			int n;
			n=dopasuj_l(cfg,str,(struct ruler *)(ruler->arg),start,&c);
			if (!n) {
				n=dopasuj_l(cfg,str,(struct ruler *)(ruler->arg2),start,&c);
			}
			if (!n) {
				return 0;
			}
			str=c;
			continue;
		}
		if (op & OP_CLASS) {
			nr=op & 255;
			if (!str) return 0;
			if (cfg->letters[(*str) & 255] & (1<<nr)) {
				str=prev_char(str,start);
				continue;
			}
			return 0;
		}
		if (op & OP_NEG) {
			char *s;
			nr=op & 255;
			if (nr) { /* klasa */
				if (!str) return 0;
				if (!(cfg->letters[(*str) & 255] & (1<<nr))) return 0;
			}
			if (ruler->arg) for (s=(char *)ruler->arg;*s;s++){
				if (*s=='#') {
					if (!str || isspace(*str)) return 0;
					continue;
				}
				if (*s=='^') {
					if (!str) return 0;
					continue;
				}
				if (str && (*str & 255)==(*s & 255)) return 0;
			}
			str=prev_char(str,start);
			continue;
		}
		return 0; /* error? */
	}
	if (cout) *cout=str;
	return 1;
}

static unsigned char *dopasuj_r(struct milena *cfg,unsigned char *str,struct ruler *ruler,int sep)
{
	int nr,op;
	for (;ruler;ruler=ruler->next) {
		op=ruler->op;
		if (op & OP_STAR) {
			char *c;
			for (;;) {
				if (!*str || isspace(*str)) return NULL;
				c=dopasuj_r(cfg,str,ruler->arg,0);
				if (c) break;
				str=next_char(str,NULL);
			}
			continue;
		}
		if (op & OP_SEP) {
			if (sep != 1) return NULL;
			continue;
		}
		if (op & OP_MOD) {
			if (sep != 2) return NULL;
			continue;
		}
		if (op & OP_CHAR) {
			nr=op & 255;
			//fprintf(stderr,"CMP %c %c\n",*str,nr);
			if (nr=='$') {
				if (*str) {
					return NULL;
				}
				str=next_char(str,NULL);
				continue;
			}
			if (nr=='#') {
				if (*str && !isspace(*str)) {
					return NULL;
				}
				str=next_char(str,NULL);
				continue;
			}
			if (*str != nr) {
				return NULL;
			}
			str=next_char(str,NULL);
			continue;
		}
		if (op & OP_OR) {
			unsigned char *c;
			c=dopasuj_r(cfg,str,(struct ruler *)ruler->arg,sep);
			if (!c) c=dopasuj_r(cfg,str,(struct ruler *)ruler->arg2,sep);
			if (!c) return NULL;
			str=c;
			continue;
		}
		if (op & OP_CLASS) {
			nr=op & 255;
			if (cfg->letters[(*str) & 255] & (1<<nr)) {
				str=next_char(str,NULL);
				continue;
			}
			return NULL;
		}
		if (op & OP_NEG) {
			char *s;
			nr=op & 255;
			if (nr) { /* klasa */
				if (!str) return NULL;
				if (!ruler->arg) { /* nie klasa */
					if ((cfg->letters[(*str) & 255] & (1<<nr))) return NULL;
				}
				else {
					if (!(cfg->letters[(*str) & 255] & (1<<nr))) return NULL;
				}
			}
			if (ruler->arg) for (s=(char *)ruler->arg;*s;s++){
				if (*s=='#') {
					if (!str || isspace(*str)) return NULL;
					continue;
				}
				if (*s=='$') {
					if (!str || !*str) return NULL;
					continue;
				}
				//fprintf(stderr,"<%c %d %c %d>\n",*str,*str,*s,*s);
				if (str && (*str & 255)==(*s & 255)) return NULL;
			}
			str=next_char(str,NULL);
			continue;
		}
		return NULL; /* error? */
	}
	return str;
}

static void dump_lrule(struct ruler *r)
{
	int op;
	for (;r;r=r->next) {
		op=r->op;
		if (op & OP_CHAR) {
			fprintf(stderr,"%c",op & 255);
			continue;
		}
		if (op & OP_STAR) {
			fprintf(stderr,"*(");
			dump_lrule((struct ruler *)(r->arg));
			fprintf(stderr,")");
			continue;
		}
		if (op & OP_CLASS) {
			fprintf(stderr,"[%d]",op & 255);
			continue;
		}
		if (op & OP_NEG) {
			fprintf(stderr,"(!");
			if (op & 255) fprintf(stderr,"[%d]",op & 255);
			if (r->arg) fprintf(stderr,"%s",(char *)(r->arg));
			fprintf(stderr,")");
			return;
		}
		if (op & OP_OR) {
			fprintf(stderr,"(");
			dump_lrule((struct ruler *)(r->arg));
			fprintf(stderr,",");
			dump_lrule((struct ruler *)(r->arg));
			fprintf(stderr,")");
			continue;
		}
		fprintf(stderr,"?");
	}
}

static void dump_rule(struct letter_rule *rule)
{
	if (!rule) {
		fprintf(stderr,"Default rule\n");
		return;
	}
	if (rule->lrule) {
		fprintf(stderr,"Left rule: ");
		dump_lrule(rule->lrule);
		fprintf(stderr,"\n");
	}
	if (rule->rrule) {
		fprintf(stderr,"Rite rule: ");
		dump_lrule(rule->rrule);
		fprintf(stderr,"\n");
	}
}

static void reschwa(char *c)
{
	int e=0;
	for (;*c;c++) {
		if (*c=='e' || *c=='E') {
			e=1;
			continue;
		}
		if (strchr("aiouyOE",*c)) {
			e=0;
			continue;
		}
		if (*c=='Y' && e) *c='I';
	}
}
int milena_TranslatePhrase(struct milena* cfg,unsigned char *str,char *outbuf,int buflen,int debug)
{
	int pos,eats,litera,sep;
	unsigned char *lstr,*cstr,*start;
	char *src,*pho;
	struct letter_rule *rule;
	int elang;

	while (*str && isspace(*str)) str++;
	pos=0;
	eats=0;
	start=str;
	elang=cfg->language_mode;
	while (*str) {
		if (isspace(*str)) {
			elang=cfg->language_mode;
			while (*str && isspace(*str)) str++;
			if (!*str) break;
			pushbuf(' ');
			continue;
		}
		if (*str=='~') {
			if (str[1]=='\'' || str[1]=='+') {
				str+=2;
				continue;
			}
			pushbuf('~');
			str++;
			if (*str) {
				pushbuf(*str);
				str++;
			}
			continue;
		}
		if (*str=='[') {
			pushbuf('[');
			str++;
			while (*str) {
				int z=*str++;
				if (z == 'L') {
					int k;
					for (k=0;milena_langs[k];k++) if (!strncmp(str,milena_langs[k],2)) {
						elang |= 1 <<k;
						str+=2;
						break;
					}
				}
				else {
					pushbuf(z);
				}
				if (z==']') break;
			}
			continue;
		}
		if (*str=='{') {
			pushbuf('{');
			str++;
			while (*str) {
				int z=*str++;
				pushbuf(z);
				if (z=='}') break;
			}
			continue;
		}
		if (*str && eats>0) {
			eats--;
			str++;
			continue;
		}
		if (!*str) break;
		litera=(*str) & 255;
		cstr=next_char(str,&sep);
		lstr=prev_char(str,start);
		pho=NULL;
		eats=0;
		src="default";
		for (rule = cfg->lrules[litera];rule;rule=rule->next) {
			if (rule->langs) {
				if (!(rule->langs & elang /*cfg->language_mode*/)) continue;
			}
			if (rule->rrule) {
				if (!dopasuj_r(cfg,cstr,rule->rrule,sep)) continue;
			}

			if (rule->lrule) {
				if (!dopasuj_l(cfg,lstr,rule->lrule,start,NULL)) continue;
			}
			pho=rule->pho;
			eats=rule->eats;
			src=rule->orig;
			break;
		}
		if (!pho) pho=cfg->def_pho[litera];
		if (debug) {
			fprintf(stderr,"Litera %c regula %s\n",litera,src);
			//dump_rule(rule);
		}
		if (pho) {
			while (*pho) {
				pushbuf(*pho);
				pho++;
			}
		}
		str++;
	}
	if (pos<buflen) {
		outbuf[pos]=0;
		reschwa(outbuf);
		return 0;
	}
	return pos+1;
}

#define WINFO_UNSTRES 1
#define WINFO_KEEP 2
#define WINFO_NEVER 4
#define WINFO_PRIMARY 8
#define WINFO_ATEND 16
#define WINFO_EXTSTRES 32
#define WINFO_COND2SYL 64

static int get_next_stress(char *c,int *wordinfo,int *nexts)
{
	int nextsyl,nextstress,nsx;
	nsx=0;
	for (;*c;) {
		if (*c=='~' || isalpha(*c)) break;
		if (*c=='{') {
			while(*c && *c!='}') c++;
			if (*c) c++;
			continue;
		}
		if (*c!='[') {
			c++;
			continue;
		}
		c++;
		while (*c!=']') {
			if (*c=='n' && wordinfo) (*wordinfo) |= WINFO_UNSTRES;
			if (*c=='+') {
				c++;
				if (isdigit(*c)) c++;
				continue;
			}
			if (isdigit(*c)) nsx=(*c)-'0';
			c++;
		}
		if (*c) c++;
	}
	nextsyl=0;
	nextstress=-1;
	for (;*c;) {
		if (isspace(*c) || *c=='{' || *c=='[') break;
		if (*c!='~') {
			if (strchr(EPN_VOWEL,*c)) nextsyl++;
			c++;
			continue;
		}
		c++;
		if (*c=='!') nextstress=nextsyl;
		if (*c && !isspace(*c) && !isalpha(*c)) c++;
	}
	//if (!nsx) nsx=nextsyl-2;
	if (!nsx) nsx=2;
	if (nextstress>=0) nsx=nextsyl-nextstress;
	if (nexts) *nexts=nextsyl;
	return nsx;
}

int milena_Poststresser(char *str,char *outbuf,int buflen)
{
	int pos=0;
	int word_info=0;
	int stress_syllable;
	int second_syllable;
	int has_stress;
	int syllabs,prev_syllabs,this_syllabs;
	int spa;
	char *c;
	int dont_stress=0;
	int dont_second=0;
	int last_stressed=0;

	int stress_distance=0;
	int prim_stress_distance=0;

	int on_schwa(int stres,int syls,char *s)
	{
	    int n;
	    for (n=-1;*s;s++) {
		if (isspace(*s) || *s=='[') return 0;
		if (strchr(EPN_VOWEL,*s)) {
		    n++;
		    if (n == syls - stres) {
			return *s == 'Y' || *s == 'I';
		    }
		}
	    }
	    return 0;
	}

	int is_last_word(char *s)
	{
		while (*s && !isspace(*s)) s++;
		while (*s && isspace(*s)) s++;
		return (*s)?0:1;

	}
	prev_syllabs=this_syllabs=0;
	for (;*str;) {
		stress_syllable=2;
		second_syllable=0;
		word_info=0;
		spa=0;
		for (;*str;) {
			if (isspace(*str)) {
				spa=1;
				str++;
				continue;
			}
			if (*str=='{') {
				if (pos && spa) pushbuf(' ');
				spa=0;
				for (;*str && *str!='}';str++) {
					//if (*str == 'm') word_info |=WINFO_PRIMARY;
					pushbuf(*str);
				}
				pushbuf('}');
				str++;
				continue;
			}
			if (*str=='[') {
				if (pos && spa) {
					pushbuf(' ');
				}
				spa=0;
				str++;
				pushbuf('[');
				while (*str && *str!=']') {
					if (isdigit(*str)) stress_syllable=*str-'0';
					else if (*str=='+' && str[1] && isdigit(str[1])) {
						str++;
						second_syllable=*str-'0';
					}
					else if (*str=='n') word_info |= WINFO_UNSTRES;
					else if (*str=='k') word_info |= WINFO_KEEP;
					else if (*str=='u') word_info |= WINFO_NEVER;
					else if (*str=='U') word_info |= WINFO_NEVER|WINFO_ATEND;
					//else if (*str=='c') word_info |= WINFO_PRIMARY;
					else if (*str=='e') word_info |= WINFO_EXTSTRES;
					else if (*str=='q') word_info |= WINFO_COND2SYL;
					else pushbuf(*str);
					str++;
				}
				pushbuf(']');
				if (*str) str++;
				continue;
			}
			break;
		}
		if (!*str) break;
		if (pos && spa) {
			pushbuf(' ');
		}
		spa=0;
		if (dont_stress) {
			dont_stress=0;
			while (*str) {
				if (isspace(*str) || *str=='[') break;
				if (strchr(EPN_VOWEL,*str)) {
					stress_distance++;
					prim_stress_distance++;
				}
				else if (*str=='~' && (str[1]==',' || str[1]=='!')) {
					str+=2;
					continue;
				}
				pushbuf(*str);
				str++;
			}
			continue;
		}
		has_stress=0;
		syllabs=0;
		prev_syllabs=this_syllabs;
		for (c=str;*c;) {
			if (isspace(*c)) break;
			if (*c=='[') break;
			if (*c != '~') {
				if (strchr(EPN_VOWEL,*c)) syllabs++;
				c++;continue;
			}
			c++;
			if (*c=='!' || *c==',') has_stress=1;
			if (*c && !isspace(*c) && !isalpha(*c)) c++;
		}
		this_syllabs=syllabs;
		/* je¶li second_syllable by³a podana, liczoa jest od pocz±tku */
		if (second_syllable) second_syllable=syllabs+1-second_syllable;
		/*

		dla flagi 'k' obliczamy ilo¶æ sylab w nastêpnym s³owie.
		Je¶li równa 1 akcentujemy na ostatniej.
		Je¶li wiêksza, traktujemy jak nieakcentowany
		Je¶li nie ma nastêpnego s³owa akcentujemy normalnie

		*/

		if (word_info & WINFO_KEEP) {
			int nextsyl;
			int nsx=get_next_stress(c,&word_info,&nextsyl);
			//fprintf(stderr,"Nx %d %d\n",nsx,nextsyl);
			if (nextsyl) {
				if (nsx > nextsyl) {
					dont_stress=1;
					stress_syllable=nsx-nextsyl;
					/* intonator Duddingtona niespecjalnie dobrze intonuje secondary */
					word_info &= ~WINFO_UNSTRES;
				}
				else if (nextsyl >= nsx+1) {
					second_syllable=1;
					dont_second=2;
					word_info |= WINFO_UNSTRES;
				}
				else if (nextsyl==1) {
					stress_syllable=1;
					dont_stress=1;

				}
				else word_info |= WINFO_UNSTRES;
			}
		}
		//fprintf(stderr,"LS=%d '%s'\n",stress_distance,str);
		//fprintf(stderr,"%d\n",second_syllable);
		if (word_info & WINFO_COND2SYL) {
			//fprintf(stderr,"OSD %d\n",prim_stress_distance);
			if (prim_stress_distance <2 || prim_stress_distance == 3) word_info |= WINFO_UNSTRES|WINFO_NEVER;
		}
		if (is_last_word(str) && !(word_info & WINFO_NEVER)) {
			if (syllabs>1 || !last_stressed || stress_distance) word_info &= ~WINFO_UNSTRES;
		}
		last_stressed=!(word_info & WINFO_UNSTRES);
		//fprintf(stderr,"LSA SS %d %d\n",has_stress,stress_syllable);
		if (has_stress || !stress_syllable) {
			while (*str) {
				if (isspace(*str) || *str=='[') break;
				pushbuf(*str);
				str++;
			}
			continue;
		}
		if (stress_syllable>syllabs) {
			if ((word_info & WINFO_UNSTRES) && syllabs==1) stress_syllable=0;
			else stress_syllable=syllabs;
		}
		else if (stress_syllable < syllabs && on_schwa(stress_syllable,syllabs,str)) {
		    stress_syllable++;
		}
		//fprintf(stderr,"%d %d:%d\n",second_syllable,stress_syllable,dont_second);
		if (stress_syllable==4) second_syllable=2;
		else if(dont_second==1) second_syllable=0;
		//else if (!second_syllable && stress_syllable<=syllabs-2) second_syllable=stress_syllable+2;
		else if (!second_syllable && stress_syllable<=syllabs-2) {
			if (stress_syllable < syllabs-2) second_syllable=syllabs;
		}
		else if (stress_syllable && second_syllable <=stress_syllable+1) {
			if (stress_syllable<=syllabs-3) second_syllable=syllabs;
			else second_syllable=0;
		}
		//fprintf(stderr,"%d %d:%d\n",second_syllable,stress_syllable,dont_second);
		if (dont_second) dont_second--;

		/* UWAGA!
		  Czemu ja to kurwa zerowa³em w tym miejscu?
		  stress_distance=prim_stress_distance=0;
		*/
		//fprintf(stderr,"SS %d\n",stress_syllable,syllabs);
		while (*str) {
			if (isspace(*str) || *str=='[') break;
			//fprintf(stderr,"%c\n",*str);
			if (strchr(EPN_VOWEL,*str)) {
				stress_distance++;
				prim_stress_distance++;
				if (stress_syllable == syllabs) prim_stress_distance=0;
				if (stress_syllable == syllabs || ((word_info & WINFO_EXTSTRES) && syllabs == stress_syllable-2)) {
					//fprintf(stderr,"SD zero at %s\n",str);
					stress_distance=0;
					pushbuf('~');
					pushbuf((word_info & WINFO_PRIMARY)?'?':(word_info & WINFO_UNSTRES)?',':'!');
				}
				else if (second_syllable == syllabs) {
					pushbuf('~');
					pushbuf(',');
				}
				syllabs--;
			}
			pushbuf(*str);
			str++;
		}
	}
	if (pos<buflen) {
		outbuf[pos]=0;
		return 0;
	}
	return pos+1;
}

