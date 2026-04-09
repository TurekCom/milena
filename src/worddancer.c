
#define MFLG_NIE 64


static u_int64_t _filter_gf(u_int64_t grama,int flags)
{
    int cm = 1 << WT_GET(grama);
    while (flags) {
        if (flags & MFLG_NIE) {
            //printf("%x %x\n",cm,((1<<WT_adj) | (1<<WT_ppas) | (1<<WT_pact) | (1<<WT_adv)));
            if (!(cm & ((1<<WT_adj) | (1<<WT_ppas) | (1<<WT_pact) | (1<<WT_adv)))) {
                return 0;
            }
            if (grama & WM_neg) return 0;
            grama &= ~WM_aff;
            grama |= WM_neg;
            flags &= ~MFLG_NIE;
            if (!flags) return grama;
            continue;
        }
        return grama;
    }
}

static int _filter_loc(u_int64_t *loc,u_int64_t *gramas,int ngram,int flags)
{
    int i,n;
    u_int64_t grama;
    for (i=n=0;i<ngram;i++) {
        grama=_filter_gf(loc[i],flags);
        if (grama) {
            gramas[n++]=grama;
        }
    }
    return n;
}

static int _minimorf_ident_word(struct minimorf *md,char *word,u_int64_t *gramas,int flags)
{
    
    #include "minimorf_cache.c"

    int startswith(register char *w,register char *p)
    {
        while (*p && (*w++ == *p++));
        return !*p;
    }
    
    u_int64_t loc_gramas[64];
    int loc_gcount;
    int rc;
    
    //fprintf(stderr,"Get [%s]\n",word);
    rc=cache_GetWord(word,gramas);
    //fprintf(stderr,"Cache return %d\n",rc);
    if (rc >= 0) return rc;
    rc=_minimorf_get_word_simple(md,word,gramas);
    //fprintf(stderr,"Simple return %d\n",rc);
    if (rc < 0) return -1;
    if (rc != 0) goto finale;
    if (!(flags & MFLG_NIE) && startswith(word,"nie")) {
        loc_gcount=_minimorf_ident_word(md,word+3,loc_gramas,flags | MFLG_NIE);
        if (loc_gcount < 0) return -1;
        if (loc_gcount > 0) {
            rc=_filter_loc(loc_gramas,gramas,loc_gcount,flags | MFLG_NIE);
            if (rc) goto finale;
        }
    }
    
    if (!flags) cache_SetWord(word,0,NULL);
    return 0;
finale:
    //printf("Finale\n");
    if (!flags) cache_SetWord(word,rc,gramas);
    return rc;
}

int minimorf_GetWord(struct minimorf *md,char *word,u_int64_t *gramas)
{
    return _minimorf_ident_word(md,word,gramas,0);
}
