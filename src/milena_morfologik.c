#ifndef __WIN32
#if HAVE_MORFOLOGIK == 20
#include <sys/ipc.h>
#include <sys/shm.h>

static char *morfologik_fnames[]={
	"/usr/local/share/morfologik/morfologik.bin",
	"/usr/share/morfologik/morfologik.bin",
	"/opt/share/morfologik/morfologik.bin",
	NULL};

void milena_DestroyMorfologik(void)
{
    int i,kid;
    for (i=0;i<=5;i++) {
        kid=shmget(MF_SHMAT_MAGIC+i,0,0);
        if (kid >= 0) shmctl(kid,IPC_RMID,NULL);
    }
}

void *milena_StartMorfologik(char *path)
{
    struct morfologik_data *md;
    md=morfologik_InitSHM();
    if (md) {
	return (void *) md;
    }
    if (path) {
	md=morfologik_CreateShm(path);
    }
    else {
	int i;
	struct stat sb;
	for (i=0;morfologik_fnames[i];i++) {
	    if (!stat(morfologik_fnames[i],&sb)) {
		md=morfologik_CreateShm(morfologik_fnames[i]);
		break;
	    }
	}
    }
    return (void *)md;
}
#else
static char *morfologik_fnames[]={
    "/usr/local/share/minimorf/",
    "/usr/share/minimorf/",
    NULL };
#endif
#endif

#define WM_m (WM_m1 | WM_m2 | WM_m3)
#define WM_n (WM_n1 | WM_n2)

static char lci[];
static unsigned char i2ut[128][2];
static void i2u(char *iso,char *utf)
{
    int n;
#if HAVE_MORFOLOGIK == 10
    strcpy(utf,iso);
#else
    while (n=*iso++) {
	n &= 255;
	if (n < 0x80) *utf++=n;
	else {
	    *utf++=i2ut[n-128][0];
	    *utf++=i2ut[n-128][1];
	}
    }
    *utf=0;
#endif
}

#if HAVE_MORFOLOGIK == 20
static struct morfologik_data *_milena_get_morfologik(char *fname,int shmonly)
{
    struct morfologik_data *md;
    struct stat sb;
    
    if (fname) {
	return morfologik_Init(fname);
    }
#ifndef __WIN32
    md=morfologik_InitSHM();
    if (!md) {
	int i;
	if (shmonly) return NULL;
	for (i=0;morfologik_fnames[i];i++) {
	    if (!stat(morfologik_fnames[i],&sb)) {
		md=morfologik_Init(morfologik_fnames[i]);
		if (md) break;
		}
	    }
	}
#else
    char path2[MAX_PATH];
    char path[MAX_PATH];
    GetModuleFileName(NULL,path2,MAX_PATH);
    strcpy(path,dirname(path2));
    strcat(path,"\\data\\morfologik.bin");
    if (!stat(path,&sb)) {
	md=morfologik_Init(path);
    }
#endif    
    return md;
}
#else
static struct minimorf *_milena_get_morfologik(char *fname,int shmonly)
{
    struct minimorf *md=NULL;
    struct stat sb;int i;
    if (fname) {
	return minimorf_Init(fname);
    }
    for (i=0;morfologik_fnames[i];i++) {
	if (!stat(morfologik_fnames[i],&sb)) {
	    md=minimorf_Init(morfologik_fnames[i]);
	    if (md) break;
	}
    }
    return md;
}
#endif

void *milena_GetMorfologik(char *fname,int shmonly)
{
    return (void *)_milena_get_morfologik(fname,shmonly);
}


int milena_SetMorfologik(struct milena *cfg,void *mdv)
{
#if HAVE_MORFOLOGIK == 20
    struct morfologik_data *md=(struct morfologik_data *)mdv;
#else
    struct minimorf *md=(struct minimorf *)mdv;
#endif
    if (!md) {
	md=_milena_get_morfologik(NULL,0);
    }
    if (!md) return -1;
    cfg->md=md;
    return 0;
}


static char *morf_get_word(struct milena *cfg,char **str,char **buf)
{
    int i,l;
    char *c=*buf;
    char *d=c;
    char *s=*str;
    if (*s=='-') s++;
    while (*s && isspace(*s)) s++;
    if (!*s) return NULL;
    if (!cfg->md) return NULL;
    for (i=0;i<64;i++) {
	l=lci[(*s) & 255];
	if (!l) break;
	s++;
	*c++=l;
    }
    if (!i || i>=64) return NULL;
    if (*s && isdigit(*s)) return NULL;
    *c++=0;
    *str=s;
    *buf=c;
    return d;
}	

static int morf_is_subst_or_adj(struct milena *cfg,char *str,u_int64_t gramas[])
{
    char bufor[128];
    char ubuf[6*126];
    char *slowo,*pisownia,*baza;
    char *bf=bufor;
    int id,n,baseid;
    u_int64_t grama;
    int imask=(1<<WT_subst) | (1<<WT_adj) | (1<<WT_pact) | (1<<WT_pant) | (1<<WT_pcon) | (1<<WT_ppas);
    
    if (!cfg->md) return 0;
    slowo=morf_get_word(cfg,&str,&bf);
    if (!slowo) return 0;
    i2u(slowo,ubuf);
#if HAVE_MORFOLOGIK == 20
    //fprintf(stderr,"Searching for %s\n",ubuf);
    n=0;
    id=morfologik_IdentifyWord(cfg->md,ubuf,&pisownia,&baza,&baseid,&grama);
    if (id < 0) return 0;
    //fprintf(stderr,"Found [%s/%s]\n",pisownia,baza);
    if ((1 <<WT_GET(grama)) & imask) {
	gramas[n++]=grama;
    }
    for (;n<16;) {
	id=morfologik_IdentifyNext(cfg->md,id,&pisownia,&baza,&baseid,&grama);
	if (id < 0) break;
	//fprintf(stderr,"Found next [%s/%s]\n",pisownia,baza);
	if ((1 <<WT_GET(grama)) & imask) {
	    gramas[n++]=grama;
	}
    }
#else
    int i,gc;
    gc=minimorf_GetWord(cfg->md,ubuf,gramas);
    for (i=n=0;i<gc;i++) {
	if ((1 <<WT_GET(gramas[i])) & imask) {
	    gramas[n++]=gramas[i];
	}
    }
#endif
    return n;
}

static int morf_is_verb_w(struct milena *cfg,char *word)
{
    char buf[strlen(word)*2+2];
    i2u(word,buf);
#if HAVE_MORFOLOGIK == 20
    u_int64_t grama;
    char *pisownia,*baza;
    int id,baseid;

    for (
	id=morfologik_IdentifyWord(cfg->md,buf,&pisownia,&baza,&baseid,&grama);
	id >= 0;
	id=morfologik_IdentifyNext(cfg->md,id,&pisownia,&baza,&baseid,&grama)) {
	    if ((WT_GET(grama) == WT_verb) || (grama & WM_cmplx)) {
		if ((grama & WM_str3)) return 3;
		if ((grama & WM_str4)) return 4;
		return 1;
	    }
    }
#else
    u_int64_t gramas[32];
    int i,n;
    n=minimorf_GetWord(cfg->md,buf,gramas);
    for (i=0;i<n;i++) {
	if ((WT_GET(gramas[i]) == WT_verb) || (gramas[i] & WM_cmplx)) {
	    if ((gramas[i] & WM_str3)) return 3;
	    if ((gramas[i] & WM_str4)) return 4;
	    return 1;
	}
    }
#endif
    return 0;
}

static int morf_get_gramas(struct milena *cfg,char *word,u_int64_t gramas[],int imask)
{
    int n=0;
    //fprintf(stderr,"MGG %s\n",word);
#if HAVE_MORFOLOGIK == 20    
    char *pisownia,*baza;int id,baseid;
    u_int64_t grama;
    
    id=morfologik_IdentifyWord(cfg->md,word,&pisownia,&baza,&baseid,&grama);
    if (id < 0) return 0;
    if (!imask || ((1 << WT_GET(grama)) & imask)) {
	gramas[n++]=grama;
    }
    for (;n<16;) {
	id=morfologik_IdentifyNext(cfg->md,id,&pisownia,&baza,&baseid,&grama);
	if (id < 0) break;
	if (!imask || ((1 << WT_GET(grama)) & imask)) {
	    gramas[n++]=grama;
	}
    }
#else
    int i,gc;
    gc=minimorf_GetWord(cfg->md,word,gramas);
    for (i=n=0;i<gc;i++) {
	if ((1 <<WT_GET(gramas[i])) & imask) {
	    gramas[n++]=gramas[i];
	}
    }
#endif
    return n;
}

static u_int64_t morf_adj_subst_pass(u_int64_t grama1, u_int64_t grama2)
{
    u_int64_t mask= grama1 & grama2 & ~WT_MASK;
    if (!(mask & WM_NUM_MASK)) return 0;
    if (!(mask & WM_CASU_MASK)) return 0;
    if (!(mask & WM_GENR_MASK)) return 0;
    return mask;
}

static int morf_get_subst_form(struct milena *cfg,char *str,u_int64_t outgramas[])
{
    char buf[1024];
    char ubuf[2100];
    char *bf=buf;
    char *slowo;
    u_int64_t gramas0[16],gramas1[16],gramas2[16];
    int ngram0=0,ngram1=0,ngram2=0,dl;
    int WT_adjall=(1<<WT_adj) | (1<<WT_pact) | (1<<WT_pant) | (1<<WT_pcon) | (1<<WT_ppas);
    int imask=(1<<WT_subst) | WT_adjall | (1<<WT_adv);
    int s_subst,s_adj,s_adv,i,t,ngram=0;
    
    if (!cfg->md) return 0;
    
    bf=buf;
    slowo=morf_get_word(cfg,&str,&bf);
    if (!slowo) return 0;
    i2u(slowo,ubuf);
    ngram0=morf_get_gramas(cfg,ubuf,gramas0,imask);
    //fprintf(stderr,"NGram = %d\n",ngram);
    if (!ngram0) return 0;
    //fprintf(stderr,"NGRAM0 [%s] found %d\n",ubuf,ngram0);
    for (s_subst=s_adj=s_adv=i=0;i<ngram0;i++) {
	t=WT_GET(gramas0[i]);
	if (t==WT_subst) s_subst ++;
	else if (t==WT_adv) s_adv ++;
	else s_adj++;
    }
    //fprintf(stderr,"Su/Aj/Av=%d/%d/%d\n",s_subst,s_adj,s_adv);
    if (s_subst) {
	for (i=0;i<ngram0 && ngram < 16;i++) {
	    if (WT_GET(gramas0[i]) == WT_subst) {
		outgramas[ngram++]=gramas0[i];
	    }
	}
    }
    if (!s_adv && !s_adj) {
	//fprintf(stderr,"Returning %d\n",ngram);
	return ngram;
    }
    if (s_adv) { // niemozliwe zeby bylo razem z adj, szukamy przymiotnika
	bf=buf;
	slowo=morf_get_word(cfg,&str,&bf);
	if (!slowo) return ngram;
	i2u(slowo,ubuf);
	ngram0=morf_get_gramas(cfg,ubuf,gramas0,WT_adjall);
	//fprintf(stderr,"NGRAM0(2) [%s] found %d\n",ubuf,ngram0);
	if (!ngram0) return ngram;
    } // teraz tu mamy przymiotnik
    
    bf=buf;
    slowo=morf_get_word(cfg,&str,&bf);
    if (!slowo) goto rtadj;
    i2u(slowo,ubuf);
    if (!strcmp(ubuf,"i")) {
	bf=buf;
	slowo=morf_get_word(cfg,&str,&bf);
	if (!slowo) goto rtadj;
	i2u(slowo,ubuf);
	ngram1=morf_get_gramas(cfg,ubuf,gramas1,WT_adjall | (1<<WT_adv));
	//fprintf(stderr,"NGRAM1 [%s] found %d\n",ubuf,ngram1);
	if (!ngram1) goto rtadj;
	for (s_subst=s_adj=s_adv=i=0;i<ngram1;i++) {
	    t=WT_GET(gramas1[i]);
	    if (t==WT_subst) s_subst ++;
	    else if (t==WT_adv) s_adv ++;
	    else s_adj++;
	}
	//fprintf(stderr,"Su/Aj/Av=%d/%d/%d\n",s_subst,s_adj,s_adv);
	if (s_adv) {
	    bf=buf;
	    slowo=morf_get_word(cfg,&str,&bf);
	    if (!slowo) goto rtadj;
	    i2u(slowo,ubuf);
	    ngram1=morf_get_gramas(cfg,ubuf,gramas1,WT_adjall);
	    if (!ngram1) goto rtadj;
	}
	bf=buf;
	slowo=morf_get_word(cfg,&str,&bf);
	if (!slowo) goto rtadj;
	i2u(slowo,ubuf);
    }
    // tu powinien byc rzeczownik
    ngram2=morf_get_gramas(cfg,ubuf,gramas2,1<<WT_subst);
    //fprintf(stderr,"NGRAM2 [%s] found %d\n",ubuf,ngram2);
    if (!ngram2) {
	//zwracamy przymiotniki z gram0
rtadj:	dl=0;
	for (i=0;i<ngram0 && ngram < 16;i++) {
	    if ((1<<WT_GET(gramas0[i])) & WT_adjall) {
		if (!dl) {
		    ngram=0;
		    dl=1;
		}
		outgramas[ngram++]=gramas0[i];
	    }
	}
	//fprintf(stderr,"Awaryjnie zwracam %d\n",ngram);
	return ngram;
    }
    int ndd=0;
    dl=0;
    for (i=0;i<ngram2;i++) {
	int j;
	u_int64_t mask;
	for (j=0;j<ngram0;j++) {
	    if (!((1<<WT_GET(gramas0[j])) & WT_adjall)) continue;
	    mask=morf_adj_subst_pass(gramas0[j],gramas2[i]);
	    if (!mask) continue;
	    //fprintf(stderr,"Pasuje maska %LX\n",mask);
	    if (ngram1) {
		int k;
		u_int64_t mask2;
		for (k=0;k<ngram1;k++) {
		    //fprintf(stderr,"ADA %LX %Ld \n",gramas1[k],WT_GET(gramas1[k]));
		    if (!((1<<WT_GET(gramas1[k])) & WT_adjall)) continue;
		    mask2=morf_adj_subst_pass(gramas1[k],mask);
		    if (mask2) {
			//fprintf(stderr,"Pasuje sub maska %LX\n",mask2);
			ndd++;
			if (!dl) {
			    dl=1;
			    ngram=0;
			}
			if (ngram<16) outgramas[ngram++]=mask2;
		    }
		    else {
			//fprintf(stderr,"Nie %LX %LX\n",gramas1[k],mask);
		    }
		}
	    }
	    else {
		ndd++;
		if (!dl) {
		    dl=1;
		    ngram=0;
		}
		if (ngram<16) outgramas[ngram++]=mask;
	    }
	}
    }
    //fprintf(stderr,"Dopasowano %d, return %d\n",ndd,ngram);
    if (ndd) return ngram;
    goto rtadj;
}

/* archaizmy typu 'kurwyscie przewszeteczne' */
static struct mika_sfx *morf_scie_sfx(struct milena *cfg,char *wrd,int is_scie, struct mika_sfx *esfx)
{
    char ubuf[1000];
    u_int64_t gramas[16];
    int ngramas,i,pstress=0;
    int WT_adjall=(1<<WT_adj) | (1<<WT_pact) | (1<<WT_pant) | (1<<WT_pcon) | (1<<WT_ppas);
    int imask=(1<<WT_subst) | WT_adjall | (1<<WT_adv);
    int is_verb;

    int count_syl(char *c)
    {
	int cs;
	for (cs=0;*c;c++) {
	    if (!strchr("aeiouy\xea\xb1",*c)) continue;
	    cs++;
	    if (*c=='i' && strchr("aoeuó\xea\xb1",c[1])) {
		c++;
		continue;
	    }
	}
	return cs;
    }

    
    int ctl;
    for (ctl=0;ctl<2;ctl++) {
	int found=0;
	is_verb=0;
	i2u(wrd,ubuf);
	ngramas=morf_get_gramas(cfg,ubuf,gramas,imask);
	if (!ngramas) {
	    int n=strlen(ubuf);
	    if (n > 3 && !strcmp(ubuf+n-4,"iemi")) {
		strcpy(ubuf+n-4,"imi");
		ngramas=morf_get_gramas(cfg,ubuf,gramas,WT_adjall);
	    }
	    else if (n > 3 && !strcmp(ubuf+n-3,"emi")) {
		ubuf[n-3]='y';
		ngramas=morf_get_gramas(cfg,ubuf,gramas,WT_adjall);
	    }
	    else if (n > 3 && !strcmp(ubuf+n-3,"iem")) {
		strcpy(ubuf+n-3,"im");
		ngramas=morf_get_gramas(cfg,ubuf,gramas,WT_adjall);
	    }
	    else if (n > 3 && !strcmp(ubuf+n-2,"em")) {
		ubuf[n-2]='y';
		ngramas=morf_get_gramas(cfg,ubuf,gramas,WT_adjall);
	    }
	}
	for (i=0;i<ngramas;i++) {
	    int cm=WT_GET(gramas[0]);
	    if (cm == WT_adv) goto rt3;
	    if (cm == WT_subst) {
		if (gramas[i] & WM_pl) {if (gramas[i] & WM_nom) is_verb=1;goto rt3;}
		if (is_scie) {if (gramas[i] & WM_nom) is_verb=1;goto rt3;}
		if (gramas[i] & (WM_gen | WM_dat | WM_acc | WM_inst | WM_loc)) goto rt3;
		continue;
	    }
	    // przymiotnik
	    if ((gramas[i] & WM_pl) && (gramas[i] & (WM_nom | WM_inst))) {is_verb=MILDIC_V;goto rt3;}
	    if ((gramas[i] & WM_sg) && is_scie && (gramas[i] & (WM_nom | WM_inst))) {is_verb=MILDIC_V;goto rt3;}
	}
	if (ctl) return NULL;
	if (!strncmp(wrd,"arcy",4)) {
	    pstress=found=1;
	    wrd += 4;
	}
	else if (!strncmp(wrd,"prze",4)) {
	    pstress=found=1;
	    wrd += 4;
	}
	if (!found) return NULL;
	if (count_syl(wrd) < 2) return NULL;
    }
    return NULL;
rt3:
    esfx->pstress=pstress;
    esfx->stress=3;
    esfx->flags=is_verb;
    return esfx;
}

/* mexpr i okolice */

struct milena_stringlist *morf_compile_expression(struct milena *cfg,char *str,u_int64_t filter)
{
#if HAVE_MORFOLOGIK == 20
    struct milena_stringlist *sl=NULL,*nsl;
    char *c,word[64],iword[64],*uword;
    int nr,baseid,fnr;
    void add_word(char *word)
    {
	if (milena_utf2iso(word,NULL,1,NULL) > 63) return;
	milena_utf2iso(word,iword,1,NULL);
	for (nsl=sl;nsl;nsl=nsl-> next) if (!strcmp(nsl->string,iword)) return;
	nsl=malloc(sizeof(*nsl));
	nsl->string=strdup(iword);
	nsl->next=sl;
	sl=nsl;
    }
	
    
    
    if (!cfg->md) return NULL;
    for (;;) {
	//fprintf(stderr,"S=[%s]\n",str);
	while (*str && isspace(*str)) str++;
	if (!*str) break;
	c=str;
	while (*str && !isspace(*str)) str++;
	if (*str) *str++=0;
	//fprintf(stderr,"C=[%s]\n",str);
	if (str-c >= 32) continue;
	i2u(c,word);
	fnr=morfologik_GenWord(cfg->md,word,(char **)&uword,NULL,filter);
	if (fnr >= 0) {
	    //fprintf(stderr, "Found base of '%s'\n",word);
	    while (fnr >= 0) {
		add_word(uword);
		fnr=morfologik_GenNext(cfg->md,fnr,(char **)&uword,NULL,filter);
	    }
	}
	else {
	    //fprintf(stderr, "No base of '%s'\n",word);
	    nr=morfologik_IdentifyWordStd(cfg->md,word,NULL,NULL,&baseid,0);
	    while (nr >= 0) {
		fnr=morfologik_GenWordById(cfg->md,baseid,(char **)&uword,NULL,filter);
		while (fnr >= 0) {
		    //fprintf(stderr,"Adding %s\n",uword);
		    add_word(uword);
		    fnr=morfologik_GenNext(cfg->md,fnr,(char **)&uword,NULL,filter);
		}
		nr=morfologik_IdentifyNext(cfg->md,nr,NULL,NULL,&baseid,0);
	    }
	}
    }
    return sl;
#else
    return NULL;
#endif
}
