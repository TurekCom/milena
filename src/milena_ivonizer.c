

static void ivolectorize(char *c)
{
    int nword=1;
    char *s;
    int ivowrdcls(char *cs)
    {
        while (*cs) {
            if (*cs == '[') {
                for (;*cs && *cs != ']';cs++) if (*cs == 's') return 1;
                if (*cs) cs++;
                continue;
            }
            if (*cs == '{') {
                for (;*cs && *cs != '}';cs++) if (*cs == 'v') return 2;
                if (*cs) cs++;
                continue;
            }
            return 0;
        }
        return 0;
    }
    char *lspa;
    int wtyp=ivowrdcls(c);
    /* jeśli jest con i nword > 1 i był verb:
     * jeśli poprzedni nie był co, wstaw con
     *
     * jeśli jest verb i nword > 1 i był con:
     * wstaw przecinek na pozycji con
     * nword -= numer con +1
     */
    //fprintf(stderr,"INSTRIMG %d:<%s>\n",wtyp,c);
    int verbpos=-1, conpos=-1;
    char *conptr=NULL;
    if (wtyp == 2) verbpos=0;
    for (s=c;*s;) {
        if (isspace(*s)) {
            lspa=s;
            s++;
            while (*s && isspace(*s)) s++;
            wtyp=ivowrdcls(s);
            if (wtyp == 2) { // verb
                // czy wstawiamy przecinek?
                if (conptr) {
                    //fprintf(stderr,"Comma at <%s>\n",conptr);
                    memcpy(conptr,", [",3);
                    conptr = NULL;
                    nword -= conpos+1;
                }
                verbpos = nword+1;
            }
            if (wtyp == 1 && verbpos >= 0) { // conn
                //if (conpos != nword) conpos = nword+1;
                //if (!conptr) {
              //      fprintf(stderr,"Pre at <%s>\n",s);
                    conpos = nword + 1;
                    conptr=lspa;
                    verbpos=-1;
                //}
            }
            nword++;
            //fprintf(stderr,"WBEG %d:%d:%s\n",wtyp,nword,s);
            
            
        }
        else s++;
    }
    //fprintf(stderr,"NWORD=%d\n",nword);
}

static int _milena_ivonizer(int mode,char *inbuf,char *outbuf,int outlen,struct milena *milena,int widechar, int lector)
{

	static int _px[]={
	    160,260,728,321,164,317,346,167,168,352,350,356,377,173,381,
	    379,176,261,731,322,180,318,347,711,184,353,351,357,378,733,
	    382,380,340,193,194,258,196,313,262,199,268,201,280,203,282,
	    205,206,270,272,323,327,211,212,336,214,215,344,366,218,368,
	    220,221,354,223,341,225,226,259,228,314,263,231,269,233,281,
	    235,283,237,238,271,273,324,328,243,244,337,246,247,345,367,
	    250,369,252,253,355,729};

	struct word {
		char *str;
		int flags;
		int stres;
		int strespos;
		int schwa;
	} *words=NULL;
	
	int in_word=0;
	int word_count=0;
	int flags=0,stres=0;
	int pass;
	char *str,*scopy;
	int pos,inipos,ops;

#define sh_outbuf ((unsigned short *)outbuf)

	int _trans(int znak)
	{
	    znak &= 255;
	    if (znak <= 160) return znak;
	    return _px[znak-160];
	}
	
	int outbuflen(char *c)
	{
	    if (!widechar) {
		return strlen(c);
	    }
	    else {
		int i=0;
		short *z=(short *)c;
		while (*z++) i++;
		return i;
	    }
	}
	
	if (outbuf) {
		pos=inipos=outbuflen(outbuf);
	}
	else {
		pos=outlen=0;
	}
	
	void add_outbuf_char(int znak)
	{
		if (pos<outlen) {
		    if (!widechar) {
			outbuf[pos]=znak;
		    }
		    else {
			((short *)outbuf)[pos]=_trans(znak);
		    }
		}
		pos++;
	}
	
	void set_outbuf_char(int p,int znak)
	{
		if (p<outlen) {
		    if (!widechar) {
			outbuf[p]=znak;
		    }
		    else {
			((short *)outbuf)[p]=_trans(znak);
		    }
		}
	}
	
	void add_outbuf_str(char *str)
	{
		while (*str) add_outbuf_char(*str++);
	}
	int scmp(char *s)
	{
	    int i;
	    static char *smg="eaiouy\xf3\xb1\xea";
	    char *c;
	    for (i=ops;i<pos;i++,s++) {
		if (!*s) return 0;
		if (*s == '@') {
		    if (widechar) {
			for (c=smg;*c;c++) if (sh_outbuf[i] == _trans(*c)) break;
			if (!*c) return 0;
		    }
		    else {
			if (!strchr(smg,outbuf[i])) return 0;
		    }
		    continue;
		}
		if (widechar) {
		    if (sh_outbuf[i] != _trans(*s)) return 0;
		}
		else {
		    if (outbuf[i] != *s) return 0;
		}
	    }
	    if (*s) return 0;
	    return 1;
	}
	
	scopy=dupstr(inbuf);
    if (lector) ivolectorize(scopy);
	for (pass=0;pass<2;pass++) {
		str=scopy;
		word_count=0;
		in_word=0;
		flags=0;
		stres=0;
		for (;;str++) {
			if (!*str) break;
			if (isspace(*str)) {
				if (in_word) {
					if (words) *str=0;
					in_word=0;
					flags=0;
					stres=0;
				}
				continue;
			}
			if (*str=='[') {
				if (in_word) {
					if (words)*str=0;
					in_word=0;
					flags=0;
					stres=0;
				}
				str++;
				for (;*str && *str != ']';str++) {
					if (*str=='-' || *str=='+') {
						if (str[1] && isdigit(str[1])) {
							str++;
						}
						continue;
					}
					if (isdigit(*str)) {
						stres=(*str)-'0';
					}
					else if (*str == 'k') {
						flags |= 1;
					}
				}
				if (!*str) break;
				continue;
			}
			if (*str=='{') {
				if (in_word) {
					if (words) *str=0;
					in_word=0;
					flags=0;
					stres=0;
				}
				char *c=strchr(str+1,'}');
				if (c) str=c;
				else break;
				continue;
			}
			if (!in_word) {
				if (words) {
					words[word_count].str=str;
					words[word_count].flags=flags;
					words[word_count].stres=stres;
					words[word_count].schwa=-1;
				}
				word_count++;
				in_word=1;
			}
		}
		if (!word_count) return -1;
		if (!words) words=calloc(word_count+1,sizeof(struct word));
	}
	int i,keep,started;
	for (i=0,keep=0,started=0;i<word_count;i++) {
		char *c,*d,lastvo=0;
		for (c=d=words[i].str;*c;) {
			if (*c=='~') {
				if (c[1]==',') {
					c+=2;
					continue;
				}
				if (c[1]=='+' || c[1]=='\'') {
					//if (c[2]) {
						*d++='~';
						*d++='\'';
					//}
					c+=2;
					continue;
				}
				if (c[1]=='!') {
					if (c[2]) {
						*d++='~';
						*d++='!';
						words[i].stres=0;
					}
					c+=2;
					continue;
				}
				c+=2;
				continue;
			}
			if (*c=='@') {
				words[i].schwa=d-words[i].str;
				if (lastvo == 'e') *d++='y';
				else *d++='e';
				c++;
			}
			else if (*c=='&') {
				c++;
			}
			else {
				if (strchr("eaiouy\xf3\xb1\xea",*c)) {
				    if ((*c & 255) == 0xb1) lastvo='e';
				    else lastvo=*c;
				}
				*d++=*c++;
			}
		}
		*d=0;
		if (!words[i].stres && words[i].schwa>0) {
		    words[i].stres=2;
		}
		if (words[i].stres) {
			char *c=words[i].str;
			int nl=strlen(c);
			int j,cs;
			cs=0;
			for (j=nl-1;j>=0;j--) {
				if (strchr("eaiouy\xf3\xb1\xea",c[j])) {
					if (c[j]=='i' && strchr("eaou\xf3\xb1\xea",c[j+1])) continue;
					if (cs == words[i].stres-1 && j == words[i].schwa) {
					    words[i].stres++;
					}
					if (cs == words[i].stres) {
					    words[i].strespos=j+1;
					    break;
					}
					cs++;
				}
				if (!j) {
					words[i].strespos=0;
					break;
				}
			}
		}
		if (pos) {
			if (keep) add_outbuf_str("~'");
			else add_outbuf_char(' ');
		}
		started=1;
		keep=words[i].flags & 1;
		int j;
		c=words[i].str;
		for (j=0;c[j];j++) {
			if (words[i].stres && words[i].stres != 2 && words[i].strespos == j) {
				add_outbuf_str("~!");
			}
			if (c[j] == '_') {
				if (c[j+1]) add_outbuf_str("~'");
			}
			else {
				add_outbuf_char(c[j]);
			}
		}
	}
	if (milena && (mode & 7) == 0 && pos < outlen-1 && outbuf) {
	    struct milena_fin *ms;
	    int is_set=0;
	    for (ms=milena->ivona_fin;ms;ms=ms->next) {
		if (ms->mode != 4) continue;
		ops=pos-strlen(ms->string);
		if (ops < inipos) continue;
		if (!scmp(ms->string)) continue;
		mode = ms->mode | (mode & 0xf8);
		is_set=1;
		break;
	    }
	    if (!is_set) {
		ops=pos;
		while (ops > inipos) {
		    if (widechar) {
			if(sh_outbuf[ops-1] <128 && (!lci[sh_outbuf[ops-1]] || isdigit(sh_outbuf[ops-1]))) {
			    break;
			}
		    }
		    else {
			if (!lci[outbuf[ops-1] & 255] || isdigit(outbuf[ops-1])) {
			    break;
			}
		    }
		    ops--;
		}
		if (ops < pos) {
		    for (ms=milena->ivona_fin;ms;ms=ms->next) {
			if (scmp(ms->string)) {
			    mode=ms->mode | (mode & 0xf8);
			    break;
			}
		    }
		}
	    }
	}
	else if (!milena && (mode & 7) == 0) {
	    // default: wszystkie kropki na końcu zdania na wykrzykniki
	    mode=3 | (mode & 0xf8);
	}
	add_outbuf_char(".,?!:,,,"[mode & 7]);
	add_outbuf_char(0);
	free(words);
	free(scopy);
	if (pos > outlen) {
		if (outbuf) outbuf[inipos]=0;
		return pos;
	}
	// od inipos do pos-4 trzeba by sprawdzić podwójne ~'
	/*
	for (i=j=inipos;i<pos;i++) {
	    
	    if (i<pos-4 && !strncmp(outbuf+i,"~'~'",4)) {
		if (outbuf_sh[i] == '~' && outbuf_sh[i+1]=='\'' && outbuf_sh[i+2] == '~' && outbuf_sh[i+3]=='\'')
		i+=1;
		continue;
	    }
	    outbuf[j++]=outbuf[i];
	}
	pos=j;
	* 
	*/
	
	return 0;
	//return (pos>outlen)?pos:0;
}

int milena_ivonizer(int mode,char *inbuf,char *outbuf,int outlen)
{
    return _milena_ivonizer(mode,inbuf,outbuf,outlen,NULL,0,0);
}
int milena_ivonizer_n(int mode,char *inbuf,char *outbuf,int outlen,struct milena *milena)
{
    return _milena_ivonizer(mode,inbuf,outbuf,outlen,milena,0,0);
}

int milena_ivonizer_nb(int mode,char *inbuf,char *outbuf,int outlen,struct milena *milena, int bookmode)
{
    return _milena_ivonizer(mode,inbuf,outbuf,outlen,milena,0,bookmode);
}

static void _milena_ivo2rh(char *buffer)
{
    char *src, *dst;
    src=dst=buffer;
    int csyl(char *txt) {
        int n=0;
        while (*txt && !isspace(*txt)) {
            if (*txt=='i') {
                n++;
                txt++;
                if (strchr("aeou\363\261\352",*txt)) txt++;
                continue;
            }
            if (strchr("aeiouy\261\352\363",*txt++)) n++;
        }
        return n;
    }
    while (*src) {
        if (strchr("zcs",*src) && (!strncmp(src+1,"~'i",3) || !strncmp(src+1,"~'~!i",5))) {
            *dst++=*src++;
            *dst++='j';
            //*dst++='i';
            src += 2;
            continue;
        }
        
        if (strchr("dr",*src) && !strncmp(src+1,"~'",2) && strchr("z\277\274",src[3])) {
            *dst++=*src++;
            *dst++='l';
            src+=2;
            continue;
        }
        if (strchr("cz",*src) && !strncmp(src+1,"~'z",3)) {
            *dst++=*src++;
            *dst++='l';
            src+=2;
            continue;
        }
        if (!strncmp(src,"i~'",3) && strchr("aeiouy\261\352\363",src[3])) {
            *dst++='i';
            *dst++='j';
            *dst++=src[3];
            src+=4;
            continue;
        }
        if (!strncmp(src,"~'",2)) {
            src+=2;
            continue;
        }
        *dst++=*src++;
    }
    *dst=0;
    if (strstr(buffer,"~!")) {
        src=dst=buffer;
        char *eow,*lptr;
        int nsyl;
        while(*src) {
            if (isspace(*src)) {
                while (*src && isspace(*src)) src++;
                if (*src) *dst++=' ';
                continue;
            }
            eow=src;
            lptr=NULL;
            while (*eow && !isspace(*eow)) {
                if (*eow == '~' && eow[1] == '!') {
                    lptr=eow;
                    eow += 2;
                }
                else eow++;
            }
            if (!lptr) {
                while (src < eow) *dst++=*src++;
                continue;
            }
            nsyl=csyl(lptr+2);
            
            if (nsyl == 2 || nsyl > 4) {
                while (src < eow) {
                    if (!strncmp(src,"~!",2)) src += 2;
                    else *dst++=*src++;
                }
                continue;
            }
            int crg = nsyl-1;
            while (crg > 0) {
                if (!strncmp(lptr,"~!",2)) lptr+=2;
                else {
                    if (*lptr == 'i' && strchr("aeou\261\352\363",lptr[1])) {
                        lptr+=2;
                        crg--;
                    }
                    else if (strchr("aeiouy\261\352\363",*lptr++)) crg--;
                }
            }
            // tu korekta
            if (!strncmp(lptr,"~!",2)) lptr += 2;
            crg=strchr("aeou\261\352\363",*lptr) ? 1: 0;
            while (lptr > src) {
                if (lptr >= src+2 && !strncmp(lptr-2,"~!",2)) {
                    lptr -=2;
                    continue;
                }
                if (crg && * (lptr-1) == 'i') {
                    crg=0;
                    lptr--;
                    continue;
                }
                if (!strchr("aeiouy\261\352\363",*(lptr-1))) lptr--;
                else break;
            }
            while (src < lptr) {
                if (*src == '~' && src[1] == '!') src += 2;
                else *dst++=*src++;
            }
            if (dst < src) *dst++=' ';
            else {
                char *cs=strstr(src,"~!");
                if (cs) {
                    int len=cs-src;
                    memmove(dst+1,src,len);
                    *dst=' ';
                    dst+=len+1;
                    src=cs+2;
                }
            }
            while (src < eow) {
                if (*src == '~' && src[1] == '!') src += 2;
                else *dst++=*src++;
            }
        }
        *dst=0;
    }

    // kto kurwa wymyślił, że jak po kropce i spacji jest
    // mała litera to ma traktować kropke jak przecinek...
    
    int dot=1;
    int ulet(char znak)
    {
        static const char *lc="\261\352\266\346\361\363\274\277\263";
        static const char *uc="\241\312\246\306\321\323\254\257\243";
        
        if (isalpha(znak)) return toupper(znak);
        char *c=strchr(lc,znak);
        if (c) return uc[c-lc];
        return 0;
    }
        
    
    for (src=buffer;*src;src++) {
        if (dot) {
            char c=ulet(*src);
            if (c) {
                *src=c;
                dot=0;
            }
        }
        else if (strchr(".?!",*src)) dot=1;
    }
    
}

void milena_ivo2rh(char *buffer)
{
    _milena_ivo2rh(buffer);
}

int milena_ReadIvonaFin(struct milena *cfg,char *fname)
{
    char buf[512],*s,*c;
    FILE *f;
    struct milena_fin *ms;
    int mode;
    cfg->input_line=0;
    f=fopen(fname,"rb");
    if (!f) {
//	    perror(fname);
	    return 0;
    }
    while(fgets(buf,256,f)) {
	cfg->input_line++;
	s=strstr(buf,"//");
	if (s) *s=0;
	s=buf;
	while (*s && isspace(*s)) s++;
	mode=3;
	if (*s==':') {
	    s++;mode=4;
	}
	if (!*s) continue;
	c=s;
	while (*c && !isspace(*c)) c++;
	if (*c) *c=0;
	ms=qalloc(sizeof(*ms));
	ms->next=cfg->ivona_fin;
	ms->mode=mode;
	cfg->ivona_fin=ms;
	ms->string=strdup(s);
    }
    fclose(f);
    return 1;
}
#undef sh_outbuf
