#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define MINIMORF_PAGE_SIZE (64 * 1024)
#define MINIMORF_MAGIC_HEADER 0xDEBE51AC

#define MINIMORF_CACHE_ONLY 16
#define MINIMORF_NO_CACHE 32
#define MINIMORF_CACHE_FOUND 64

#include "minimorf.h"

struct page_descr {
    int page_id;
    int words;
    int gramct;
    u_int16_t *woffsets;
    u_int64_t *gramas;
    char *strings;
    char *pagememo;
};

struct minimorf_cached_word {
    struct minimorf_cached_word *succ,*pred;
    struct minimorf_cached_word *left,*right,*parent;
    unsigned char color;
    unsigned char gcount;
    short black_quota,min_height;
    int count;
    u_int32_t data;
};


struct minimorf {
    char *minimorf_memo;
    int pageno;
    int corrupted;
    u_int16_t *border_words;
    char *bstrings;
    int mfd;
    int hit_count,miss_count;
    int page_hit_count,page_miss_count;
    int cache_roll_count;
    struct minimorf_cached_word *head,*tailpred,*tail;
    struct minimorf_cached_word *root;
    struct page_descr dpage;
    int list_size;
    u_int32_t data_end;
    u_int32_t struct_start;
    char *memo;
    int memo_size;
    char page[MINIMORF_PAGE_SIZE];
    
};

struct minimorf *minimorf_Init(char *path)
{
    
    if (!path || !*path) path="/usr/share/minimorf/";
    char mpath[strlen(path)+32],*c;
    strcpy(mpath,path);
    c=mpath+strlen(mpath)-1;
    if (*c!='/') strcat(mpath,"/");
    strcat(mpath,"minimorf.idx");
    struct stat s;
    u_int32_t d1;
    u_int16_t d2;
    char *memo;
    int mfd;
    int i;
    struct minimorf *md;
    if (stat(mpath,&s)) {
        return NULL;
    }
    memo=malloc(s.st_size);
    int fd=open(mpath,O_RDONLY);
    if (!fd) {
        free(memo);
        return NULL;
    }
    read(fd,memo,s.st_size);
    close(fd);
    memcpy(&d1,memo,4);
    if (d1 != MINIMORF_MAGIC_HEADER) {
        free(memo);
        return NULL;
    }
    c=mpath+strlen(mpath)-3;
    strcpy(c,"dat");
    mfd=open(mpath,O_RDONLY);
    if (mfd < 0) {
        free(memo);
        return NULL;
    }
    md=malloc(sizeof(*md));
    memcpy(&d2,memo+4,2);
    md->pageno=d2;
    md->border_words=(void *)(memo+6);
    md->bstrings=(void *)(memo+6+4*md->pageno);
    md->minimorf_memo=memo;
    md->mfd=mfd;
    md->dpage.page_id=-1;
    md->dpage.pagememo=md->page;
    md->head=(void *)&(md->tailpred);
    md->tail=(void *)&(md->head);
    md->tailpred=NULL;
    md->list_size=0;
    md->hit_count=0;
    md->miss_count=0;
    md->page_hit_count=0;
    md->page_miss_count=0;
    md->cache_roll_count=0;
    md->memo_size=512*1024;
    md->memo=malloc(md->memo_size);
    md->data_end=0;
    md->struct_start=md->memo_size;
    return md;
    
    
}

int _minimorf_get_word_simple(struct minimorf *md,char *word,u_int64_t *gramas)
{
    int rc,rctl,soffset;
    int lo,hi,mid,pn,n;
    struct page_descr *page;
    

    struct page_descr *load_page(int pn)
    {
        int i,j,k;
        struct page_descr *page;
        u_int16_t data;
        if (md->dpage.page_id == pn) {
            md->page_hit_count++;
            return &md->dpage;
        }
        md->page_miss_count++;
        page=&md->dpage;
        lseek(md->mfd,MINIMORF_PAGE_SIZE * pn,SEEK_SET);
        if (read(md->mfd,page->pagememo,MINIMORF_PAGE_SIZE) != MINIMORF_PAGE_SIZE) {
            md->corrupted=1;
            return NULL;
        }
        page->page_id = pn;
        memcpy(&data,page->pagememo,2);
        page->words=data;
        memcpy(&data,page->pagememo+2,2);
        page->gramct=data;
        page->woffsets=(void *)(page->pagememo+4);
        page->gramas=(void *)(page->pagememo+4+4*page->words);
        page->strings=page->pagememo+4+4*page->words+8*page->gramct;
        return page;
    }

    

    lo=0;hi=md->pageno-1;pn=-1;
    while (lo <= hi) {
        mid=(lo+hi)/2;
        if (strcmp(word,md->bstrings+md->border_words[2*mid]) < 0) {
            hi=mid-1;
            continue;
        }
        if (strcmp(word,md->bstrings+md->border_words[2*mid+1]) > 0) {
            lo=mid+1;
            continue;
        }
        pn=mid;
        break;
    }
    if (pn < 0) {
        return 0;
    }
    page=load_page(pn);
    if (!page) return -1;
    lo=0;
    hi=page->words-1;
    rctl=-1;
    while (lo <= hi) {
        mid=(lo+hi)/2;
        soffset=page->woffsets[2*mid+1];
        n=strcmp(word,page->strings+soffset);
        if (n<0) {
            hi=mid-1;
            continue;
        }
        if (n>0) {
            lo=mid+1;
            continue;
        }
        rctl=2*mid;
        break;
    }
    if (rctl < 0) {
        return 0;
    }
    rc=page->strings[soffset-1];
    soffset=page->woffsets[rctl];
    if (gramas) memcpy(gramas,page->gramas+soffset,8*rc);
    return rc;
}

void minimorf_Stat(struct minimorf *md,FILE *f)
{
    if (!f) f=stderr;
    fprintf(f,"WC free/size  %d/%d\n",md->struct_start-md->data_end,md->memo_size);
    fprintf(f,"Page hit/miss %d/%d\n",md->page_hit_count,md->page_miss_count);
    fprintf(f,"Word hit/miss %d/%d\n",md->hit_count,md->miss_count);
    fprintf(f,"Cache roll %d\n",md->cache_roll_count);
    
}

void minimorf_Free(struct minimorf *md)
{
    if (md) {
        if (md->mfd >= 0) close(md->mfd);
        if (md->minimorf_memo) free(md->minimorf_memo);
        free(md);
    }
}

static char *wt_markers[]={
    "adj",    "adjp",    "adv",    "conj",
    "num",    "pact",    "pant",    "pcon",
    "ppas",    "ppron12",    "ppron3",    "pred",
    "adjc",    "siebie",    "subst",    "verb",
    "brev",    "interj",    "xxx",    "nie",
    "advp",	"prep",	"comp",
    NULL};

static char *wm_casa[]={"nom","gen","dat","acc","inst","loc","voc"};
static char *wm_grad[]={"pos","comp","sup"};
static char *wm_pers[]={"pri","sec","ter"};
static char *wm_genr[]={"m1","m2","m3","n1","n2","p1","p2","p3","m","n","f","p"};
static u_int64_t xm_genr[]={WM_m1,WM_m2,WM_m3,WM_n1,WM_n2,WM_p1,WM_p2,WM_p3,
    WM_m1 | WM_m2 | WM_m3,
    WM_n1 | WM_n2,
    WM_f,
    WM_p1 | WM_p2 | WM_p3};

static char *wm_seq[][2]={
    {"aff",NULL},
    {"neg",NULL},
    {"perf",NULL},
    {"imperf",NULL},
    {"nakc","str2"},
    {"akc","str3"},
    {"praep",NULL},
    {"npraep",NULL},
    {"imps",NULL},
    {"impt",NULL},
    {"inf",NULL},
    {"fin",NULL},
    {"praet",NULL},
    {"pot",NULL},
    {"nstd",NULL},
    {"pun","super"},
    {"npun","cnt"},
    {"rec",NULL},
    {"congr",NULL},
    {"winien",NULL},
    {"bedzie",NULL},
    {"refl",NULL},
    {"nonrefl",NULL},
    {"depr",NULL},
    {"vulgar",NULL},
    {"ill",NULL},
    {"ger",NULL},
    {"wok","str4"},
    {"nwok","sgcplx"},
    {"cplx",NULL}};
    
    
    
u_int64_t minimorf_ParseGrama(char *str)
{
    char buf[256];
    char *cs,*ce;
    u_int64_t grama=0;
    int i,found;
    if (strlen(str)>=256) {
        errno=E2BIG;
        return 0;
    }
    strcpy(buf,str);
    cs=buf;
    ce=strpbrk(cs,".:");
    if (ce) *ce++=0;
    if (*cs) {
        for (i=0;wt_markers[i];i++) {
            if (!strcmp(wt_markers[i],cs)) {
                grama=WT_SET(grama,i+1);
                break;
            }
        }
        if (!wt_markers[i]) {
            errno=EINVAL;
            return 0;
        }
        if (!ce) return grama;
    }
    else if (!ce) {
        errno=EINVAL;
        return 0;
    }
    for(cs=ce;cs && *cs;cs=ce) {
        ce=strpbrk(cs,".:");
        if (ce) *ce++=0;
        found=0;
        if (!strcmp(cs,"sg")) {
            grama |= WM_sg;
            continue;
        }
        if (!strcmp(cs,"pl")) {
            grama |= WM_pl;
            continue;
        }
        if (!strcmp(cs,"pred")) {
            grama |= WM_pred;
            continue;
        }
        for (i=0;i<7;i++) if (!strcmp(cs,wm_casa[i])) {
            grama |= WM_nom << i;
            found=1;
            break;
        }
        if (found) continue;
        for (i=0;i<3;i++) if (!strcmp(cs,wm_grad[i])) {
            grama |= WM_pos << i;
            found=1;
            break;
        }
        if (found) continue;
        for (i=0;i<3;i++) if (!strcmp(cs,wm_pers[i])) {
            grama |= WM_pri << i;
            found=1;
            break;
        }
        for (i=0;i<12;i++) if (!strcmp(cs,wm_genr[i])) {
            grama |= xm_genr[i];
            found=1;
            break;
        }
        if (found) continue;
        for (i=WM_SEQ_FIRST;i<WM_SEQ_END;i++) if (!strcmp(cs,wm_seq[i-WM_SEQ_FIRST][0])) {
            grama |= 1LL << i;
            found=1;
            break;
        }
        if (found) continue;
        errno=EINVAL;
        return 0;
    }
    if (!grama) errno=EINVAL;
    return grama;
}

int minimorf_DecodeGrama(u_int64_t grama,char *buf)
{
    int g=WT_GET(grama);
    int p,i,ext=0;
    buf[0]=0;
    if (g < 1 || g > WT_last) return -1;
    if (g == WT_subst || g == WT_verb || (grama & WM_cmplx)) ext=1;
    strcpy(buf,wt_markers[g-1]);
    if (grama & WM_NUM_MASK) {
        strcat(buf,":");
        p=0;
        if (grama & WM_sg) {
            strcat(buf,"sg");
            p++;
        }
        if (grama & WM_pl) {
            if (p) strcat(buf,".");
            strcat(buf,"pl");
        }
    }
    if (grama & WM_pred) {
        strcat(buf,":pred");
    }
    if (grama & WM_CASU_MASK) {
        for (i=p=0;i<7;i++) {
            if (! (grama & (WM_nom << i))) continue;
            if (p) strcat(buf,".");else strcat(buf,":");
            p++;
            strcat(buf,wm_casa[i]);
        }
    }
    if (grama & WM_GRAD_MASK) {
        for (i=p=0;i<3;i++) {
            if (! (grama & (WM_pos << i))) continue;
            if (p) strcat(buf,".");else strcat(buf,":");
            p++;
            strcat(buf,wm_grad[i]);
        }
    }
    if (grama & WM_PERS_MASK) {
        for (i=p=0;i<3;i++) {
            if (! (grama & (WM_pri << i))) continue;
            if (p) strcat(buf,".");else strcat(buf,":");
            p++;
            strcat(buf,wm_pers[i]);
        }
    }
    if (grama & WM_GENR_MASK) {
        p=0;
        if (grama & (WM_m1 | WM_m2 | WM_m3)) {
            if ((grama & (WM_m1 | WM_m2 | WM_m3)) == (WM_m1 | WM_m2 | WM_m3)) {
                if (p) strcat(buf,".");else strcat(buf,":");p++;
                strcat(buf,"m");
            }
            else {
                for (i=0;i<3;i++) if (grama & (WM_m1 << i)) {
                    if (p) strcat(buf,".");else strcat(buf,":");p++;
                    strcat(buf,wm_genr[i]);
                }
            }
        }
        if (grama & WM_f) {
            if (p) strcat(buf,".");else strcat(buf,":");p++;
            strcat(buf,"f");
        }
        if (grama & (WM_n1 | WM_n2)) {
            
            if ((grama & (WM_n1 | WM_n2)) == (WM_n1 | WM_n2)) {
                if (p) strcat(buf,".");else strcat(buf,":");p++;
                strcat(buf,"n");
            }
            else {
                for (i=0;i<2;i++) if (grama & (WM_n1 << i)) {
                    if (p) strcat(buf,".");else strcat(buf,":");p++;
                    strcat(buf,wm_genr[i+3]);
                }
            }
        }
        if (grama & (WM_p1 | WM_p2 | WM_p3)) {
            if ((grama & (WM_p1 | WM_p2 | WM_p3)) == (WM_p1 | WM_p2 | WM_p3)) {
                if (p) strcat(buf,".");else strcat(buf,":");p++;
                strcat(buf,"p");
            }
            else {
                for (i=0;i<3;i++) if (grama & (WM_p1 << i)) {
                    if (p) strcat(buf,".");else strcat(buf,":");p++;
                    strcat(buf,wm_genr[i+5]);
                }
            }
        }
    }
    for (i=WM_SEQ_FIRST;i<WM_SEQ_END;i++) {
        u_int64_t mask = 1LL << i;
        if (!(grama & mask)) continue;
        if (mask == WM_imperf && (grama & WM_perf)) strcat(buf,".");
        else strcat(buf,":");
	char *c=wm_seq[i-WM_SEQ_FIRST][ext];
	if (!c) c=wm_seq[i-WM_SEQ_FIRST][0];
        strcat(buf,c);
    }
    return 0;
}

#include "worddancer.c"
