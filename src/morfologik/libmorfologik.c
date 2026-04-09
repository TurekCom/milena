/*
 * libmorfologik.c - fast and simple interface to morfologik
 * Copyright (C) Bohdan R. Rau 2012-2014 <ethanak@polip.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#ifndef __WIN32
#include <sys/ipc.h>
#include <sys/shm.h>
#else
#include <stdint.h>
#define u_int32_t uint32_t
#endif


#include "libmorfologik.h"

typedef unsigned short unichar;


struct header {
    u_int32_t magic;
    u_int32_t string_size;
    u_int32_t word_count;
    u_int32_t uniword_count;
    u_int32_t base_count;
    u_int32_t basevector_count;
    u_int32_t fonex_count;
    u_int32_t mflist_size;
};

struct base_word {
    u_int32_t name;
    u_int32_t vector;
};

struct bin_word {
    u_int32_t writename;
    u_int32_t basename;
    u_int32_t baseword;
    u_int32_t next_word;
    u_int64_t grama;
};

struct morfologik_data {
    struct header header;
    u_int32_t *names;
    struct bin_word *bin_words;
    struct base_word *base_words;
    u_int32_t *vector;
    u_int32_t *fonex;
    u_int32_t *mflist;
    
    char *strings;
#ifndef __WIN32
    /* dla właściciela bloku wspólnego */
    int self_shmid;
    int names_shmid;
    int bin_words_shmid;
    int base_words_shmid;
    int vectors_shmid;
    int fonex_shmid;
    int mflist_shmid;
    int strings_shmid;
#endif    
};

static void mf_free(struct morfologik_data *data)
{
    if (!data) return;
    if (data->header.magic == MF_FILE_MAGIC) {
        if (data->strings) free(data->strings);
        if (data->vector) free(data->vector);
        if (data->base_words) free(data->base_words);
        if (data->bin_words) free(data->bin_words);
        if (data->names) free(data->names);
        if (data->fonex) free(data->fonex);
        if (data->mflist) free(data->mflist);
    }
#ifndef __WIN32
    else {
        if (data->strings) shmdt(data->strings);
        if (data->vector) shmdt(data->vector);
        if (data->base_words) shmdt(data->base_words);
        if (data->bin_words) shmdt(data->bin_words);
        if (data->names) shmdt(data->names);
        if (data->fonex) shmdt(data->fonex);
        if (data->mflist) shmdt(data->mflist);
    }
#endif
    free(data);
}

void morfologik_Free(struct morfologik_data *data)
{
    mf_free(data);
}

static int __read_it(int fd,void **mem,size_t size)
{
    *mem=malloc(size);
    return read(fd,*mem,size) == size;
}

#ifndef __WIN32

static int __shm_read(int fd,u_int32_t key,int *id,void **memout,size_t size)
{
    void *mem;
    int keyid=shmget(key,size,IPC_CREAT | IPC_EXCL | 0644);
    //fprintf(stderr,"Keyid %d for %08X/%ld\n",keyid,key,size);
    if (keyid < 0) return 0;
    mem=shmat(keyid,NULL,0);
    if (!mem) {
	//fprintf(stderr,"Bad mem\n");
        shmctl(keyid,IPC_RMID,NULL);
        return 0;
    }
    if (read(fd,mem,size) != size) {
        //fprintf(stderr,"Bad read\n");
        shmdt(mem);
        shmctl(keyid,IPC_RMID,NULL);
        return 0;
    }
    *id=keyid;
    *memout=mem;
    return 1;
}


static void mf_shmdelete(struct morfologik_data *data)
{
    int kid=data->self_shmid;
    if (data->strings) shmdt(data->strings);
    if (data->vector) shmdt(data->vector);
    if (data->base_words) shmdt(data->base_words);
    if (data->bin_words) shmdt(data->bin_words);
    if (data->names) shmdt(data->names);
    if (data->fonex) shmdt(data->fonex);
    if (data->mflist) shmdt(data->mflist);
    if (data->names_shmid >= 0) shmctl(data->names_shmid,IPC_RMID,NULL);
    if (data->bin_words_shmid >= 0) shmctl(data->bin_words_shmid,IPC_RMID,NULL);
    if (data->base_words_shmid >= 0) shmctl(data->base_words_shmid,IPC_RMID,NULL);
    if (data->vectors_shmid >= 0) shmctl(data->vectors_shmid,IPC_RMID,NULL);
    if (data->strings_shmid >= 0) shmctl(data->strings_shmid,IPC_RMID,NULL);
    if (data->fonex_shmid >= 0) shmctl(data->fonex_shmid,IPC_RMID,NULL);
    if (data->mflist_shmid >= 0) shmctl(data->mflist_shmid,IPC_RMID,NULL);
    shmdt(data);
    shmctl(kid,IPC_RMID,0);
}

void morfologik_CloseShm(struct morfologik_data *data)
{
    mf_shmdelete(data);
}

struct morfologik_data *morfologik_CreateShm(char *fname)
{
    struct morfologik_data *shm_md;
    struct header hdr;
    int md_shmid;
#ifdef MORFDIR
    if (!fname) fname=MORFDIR;
#endif

    int fd=open(fname,O_RDONLY);
    if (fd<0) {
        return NULL;
    }
    if (read(fd,&hdr,sizeof(struct header)) != sizeof(struct header)) {
        close(fd);
        return NULL;
    }
    //fprintf(stderr,"%X %X\n",hdr.magic,MF_FILE_MAGIC);
    if (hdr.magic != MF_FILE_MAGIC) {
        close(fd);
        return NULL;
    }
    //fprintf(stderr,"Morfol starting\n");
    md_shmid=shmget(MF_SHMAT_MAGIC,sizeof(struct morfologik_data),IPC_CREAT | IPC_EXCL | 0644);
    if (md_shmid < 0) return NULL;
    //fprintf(stderr,"SHMID OK\n");
    shm_md=shmat(md_shmid,NULL,0);
    if (!shm_md) {
        close(fd);
        shmctl(md_shmid,IPC_RMID,NULL);
        return NULL;
    }
    //fprintf(stderr,"SHMAT OK\n");
    memset(shm_md,0,sizeof(struct morfologik_data));
    shm_md->header=hdr;
    shm_md->header.magic = MF_SHMAT_MAGIC;
    shm_md->self_shmid=md_shmid;
    shm_md->names_shmid=-1;
    shm_md->bin_words_shmid=-1;
    shm_md->base_words_shmid=-1;
    shm_md->vectors_shmid=-1;
    shm_md->strings_shmid=-1;
    shm_md->fonex_shmid=-1;
    shm_md->mflist_shmid=-1;
    if (!__shm_read(fd,MF_NAMES_KEY,&shm_md->names_shmid,(void **)&shm_md->names,4 * hdr.uniword_count)) goto bad;
//    fprintf(stderr,"M1 OK\n");
    if (!__shm_read(fd,MF_BINWORDS_KEY,&shm_md->bin_words_shmid,(void **)&shm_md->bin_words,sizeof(struct bin_word) * hdr.word_count)) goto bad;
//    fprintf(stderr,"M2 OK\n");
    if (!__shm_read(fd,MF_BASEWORDS_KEY,&shm_md->base_words_shmid,(void **)&shm_md->base_words,sizeof(struct base_word) * hdr.base_count)) goto bad;
//    fprintf(stderr,"M3 OK\n");
    if (!__shm_read(fd,MF_VECTORS_KEY,&shm_md->vectors_shmid,(void **)&shm_md->vector,4 * hdr.basevector_count)) goto bad;
//    fprintf(stderr,"M4 OK\n");
    if (!__shm_read(fd,MF_FONEX_KEY,&shm_md->fonex_shmid,(void **)&shm_md->fonex,8 * hdr.fonex_count)) goto bad;
    if (!__shm_read(fd,MF_MFLIST_KEY,&shm_md->mflist_shmid,(void **)&shm_md->mflist,4 * hdr.mflist_size)) goto bad;
    if (!__shm_read(fd,MF_STRINGS_KEY,&shm_md->strings_shmid,(void **)&shm_md->strings,hdr.string_size)) goto bad;
//    fprintf(stderr,"M5 OK\n");
    close(fd);
    return shm_md;
bad:
    close(fd);
    mf_shmdelete(shm_md);
    return NULL;
}

static int __shm_connect(int key,void **mem,size_t count)
{
    int kid=shmget(key,count,0);
    if (kid < 0) return 0;
    *mem=shmat(kid,NULL,SHM_RDONLY);
    return (*mem != NULL);
}

struct morfologik_data *morfologik_InitSHM(void)
{
    struct morfologik_data *md,*shm_md;
    int shm_mid;
    
    shm_mid=shmget(MF_SHMAT_MAGIC,sizeof(*shm_md),0);
    if (shm_mid < 0) return NULL;
    shm_md=shmat(shm_mid,NULL,SHM_RDONLY);
    md=malloc(sizeof(*md));
    memset(md,0,sizeof(*md));
    md->header=shm_md->header;
    md->header.magic = MF_SHMAT_MAGIC;
    shmdt(shm_md);
    if (!__shm_connect(MF_NAMES_KEY,(void **)&md->names,4 * md->header.uniword_count)) goto bad;
    if (!__shm_connect(MF_BINWORDS_KEY,(void **)&md->bin_words,sizeof(struct bin_word) * md->header.word_count)) goto bad;
    if (!__shm_connect(MF_BASEWORDS_KEY,(void **)&md->base_words,sizeof(struct base_word) * md->header.base_count)) goto bad;
    if (!__shm_connect(MF_VECTORS_KEY,(void **)&md->vector,4 * md->header.basevector_count)) goto bad;
    if (!__shm_connect(MF_FONEX_KEY,(void **)&md->fonex,md->header.fonex_count * 8)) goto bad;
    if (!__shm_connect(MF_MFLIST_KEY,(void **)&md->mflist,md->header.mflist_size * 4)) goto bad;
    if (!__shm_connect(MF_STRINGS_KEY,(void **)&md->strings,md->header.string_size)) goto bad;
    return md;
bad:
    mf_free(md);
    return NULL;
}

#endif

struct morfologik_data *morfologik_Init(char *fname)
{
    
#ifdef MORFDIR
    if (!fname) fname=MORFDIR;
#endif
    int fd=open(fname,O_RDONLY
#ifdef __WIN32
    | O_BINARY
#endif
    );
    struct morfologik_data *md;
    if (!fd) {
        return NULL;
    }
    md=malloc(sizeof(*md));
    memset(md,0,sizeof(*md));
    if (read(fd,&md->header,sizeof(struct header)) != sizeof(struct header)) {
bad:    close(fd);
        mf_free(md);
        return NULL;
    }
    if (md->header.magic != MF_FILE_MAGIC) {
        close(fd);
        mf_free(md);
        errno=EINVAL;
        return NULL;
    }
    if (!__read_it(fd,(void **)&md->names,4 * md->header.uniword_count)) goto bad;
    if (!__read_it(fd,(void **)&md->bin_words,sizeof(struct bin_word) * md->header.word_count)) goto bad;
    if (!__read_it(fd,(void **)&md->base_words,sizeof(struct base_word) * md->header.base_count)) goto bad;
    if (!__read_it(fd,(void **)&md->vector,4 * md->header.basevector_count)) goto bad;
    if (!__read_it(fd,(void **)&md->fonex,4 * md->header.fonex_count)) goto bad;
    if (!__read_it(fd,(void **)&md->mflist,4 * md->header.mflist_size)) goto bad;
    if (!__read_it(fd,(void **)&md->strings,md->header.string_size)) goto bad;
    close(fd);
    return md;
}

#define NMID_SHIFT 24
#define NMID_MASK (0x7f << NMID_SHIFT)
#define NMID_GET(a) (((a) & NMID_MASK) >> NMID_SHIFT)
#define NMID_ID(a) ((a) & ~NMID_MASK)
#define NMID_SET(a,b) (((a) & ~NMID_MASK) | ((b) << NMID_SHIFT))

/*
#define NMID_advemi 1
#define NMID_advem 2
#define NMID_impspot 3
#define NMID_pindysmy 4
#define NMID_pindyscie 5
#define NMID_pindybym 6
#define NMID_pindybys 7
#define NMID_pindyby 8
*/

enum {
    NMID_advemi=1,
    NMID_advem,
    NMID_impspot,
    // połączenia z czasownikiem
    NMID_pindysmy,
    NMID_pindyscie,
    NMID_pindybym,
    NMID_pindybys,
    NMID_pindyby,
    NMID_pindybysmy,
    NMID_pindybyscie,
    // archaiczne formy czasowników
    
    NMID_alim,
    NMID_ista
    };
    
#define NMID_negate 64
#define NMID_super 32
#define NMID_cnt 16

static u_int64_t __filter_nmid(u_int64_t grama,int smy)
{
    int cm=WT_GET(grama);

    if (smy & NMID_negate) {
	if (cm != WT_adj && cm != WT_ppas && cm != WT_pact) return 0;
	if (grama & WM_neg) return 0;
	grama &= ~WM_aff;
	grama |= WM_neg;
	smy &= ~NMID_negate;
	if (!smy) return grama;
	return __filter_nmid(grama,smy);
    }
    
    if (smy & NMID_super) {
	if (cm != WT_adj && cm != WT_subst && cm != WT_adv) return 0;
	smy &= ~NMID_super;
	grama |= WM_super | WM_cmplx;
	if (!smy) return grama;
	return __filter_nmid(grama,smy);
    }

    if (smy & NMID_cnt) {
	if (cm != WT_adj && cm != WT_subst && cm != WT_adv) return 0;
	smy &= ~NMID_cnt;
	grama |= WM_cnt | WM_cmplx;
	if (!smy) return grama;
	return __filter_nmid(grama,smy);
    }
    
    if (smy == NMID_ista) {
	if (cm != WT_verb) return 0;
	if (!(grama & (WM_pl))) return 0;
	if (!(grama & (WM_pri | WM_sec))) return 0;
	return grama | WM_depr | WM_nstd;
    }
    if (smy == NMID_alim) {
	if (cm != WT_verb) return 0;
	if ((grama & (WM_pl | WM_praet)) != (WM_pl | WM_praet)) return 0;
	return (grama | WM_depr | WM_nstd) & ~WM_str3;
    }
    if (smy >= NMID_pindysmy && smy <= NMID_pindybyscie) {
	if (cm != WT_subst && cm != WT_adj && cm != WT_adv && cm != WT_advp) {
	    return 0;
	}
	if (cm == WT_subst || cm == WT_adj) {
	    grama &= ~WM_voc;
	    if (smy != NMID_pindyscie) {
		//if (grama & WM_sg) {
		//    grama &= ~(WM_nom | WM_pl);
		//}
		grama &= ~WM_nom;
	    }
	    if (!(grama & WM_CASU_MASK)) return 0;
	}
	if (smy == NMID_pindysmy) grama |= WM_pri | WM_str3;
	else if (smy == NMID_pindyscie) grama |= WM_sec | WM_str3;
	else if (smy == NMID_pindybym) grama |= WM_pri | WM_sgcplx | WM_pot | WM_illegal | WM_str3;
	else if (smy == NMID_pindybys) grama |= WM_sec | WM_sgcplx | WM_pot | WM_illegal | WM_str3;
	else if (smy == NMID_pindyby) grama |= WM_tri | WM_pot | WM_illegal | WM_str3;
	else if (smy == NMID_pindybysmy) grama |= WM_pri | WM_pot | WM_illegal;
	else if (smy == NMID_pindybyscie) grama |= WM_sec | WM_pot | WM_illegal;
	return grama | WM_cmplx;
    }
    if (smy==NMID_impspot) {
	if (cm != WT_verb) return 0;
	if (!(grama & WM_imps)) return 0;
	return grama | WM_pot | WM_depr | WM_illegal;
    }
    if (smy==NMID_advem) {
	// niebieskiem aniełem
	if (cm != WT_adj && cm != WT_ppas) return 0;
	if ((grama & (WM_sg | WM_inst)) == (WM_sg | WM_inst)) {
	    return (grama & ~((WM_CASU_MASK | WM_NUM_MASK) ^ (WM_inst | WM_sg)))  | WM_depr;
	}
	// niebieskiem aniełom
	if ((grama & (WM_pl | WM_dat)) == (WM_pl | WM_dat)) {
	    return (grama & ~((WM_CASU_MASK | WM_NUM_MASK) ^ (WM_dat | WM_pl)))  | WM_depr;
	}
	return 0;
    }
    if (smy=NMID_advemi) {
	// niebieskiemi aniołami
	if (cm != WT_adj) return 0;
	if ((grama & (WM_pl | WM_inst)) != (WM_pl | WM_inst)) return 0;
	return  (grama & ~((WM_NUM_MASK | WM_CASU_MASK) ^ (WM_inst | WM_pl))) | WM_depr;
    }
}




static int __morfologik_IdentifyNext(struct morfologik_data *md,int id,
                            char **pisownia,
                            char ** baza,
                            int *baseid,
                            u_int64_t *grama)
{
    id=md->bin_words[id].next_word;
    if (id < 0) return id;
    if (pisownia) *pisownia=md->strings+md->bin_words[id].writename;
    if (baza) *baza=md->strings+md->bin_words[id].basename;
    if (baseid) *baseid=md->bin_words[id].baseword;
    if (grama) *grama=md->bin_words[id].grama;
    return id;    
}

int morfologik_IdentifyNext(struct morfologik_data *md,int id,
                            char **pisownia,
                            char ** baza,
                            int *baseid,
                            u_int64_t *grama)
{
    u_int64_t loc_grama;
    char *loc_baza;
    char *loc_pisownia;
    int loc_baseid;
    int nid=NMID_GET(id);
//    printf("Next NID=%d/id\n",nid,id);
    if (!nid) {
	return __morfologik_IdentifyNext(md,id,
                            pisownia,
                            baza,
                            baseid,
                            grama);
	}
    id=NMID_ID(id);
    for (;;) {
	id=__morfologik_IdentifyNext(md,id,
                            &loc_pisownia,
                            &loc_baza,
                            &loc_baseid,
                            &loc_grama);
	if (id < 0) return -1;
	loc_grama=__filter_nmid(loc_grama,nid);
	if (loc_grama) break;
    }
    if (grama)*grama=loc_grama;
    if (baseid) *baseid=loc_baseid;
    if (pisownia) *pisownia=loc_pisownia;
    if (baza) *baza=loc_baza;
    return NMID_SET(id,nid);
	
}

static int __endswith(char *word,int len,char *fin)
{
    int n;
    if ((n=strlen(fin))>=len-1) return 0;
    return !strcmp(word+(len-n),fin);
}

    
	
    
static int __morfologik_IdentifyWordSimple(struct morfologik_data *md,char *word,
                            char **pisownia,char ** baza,int *baseid,u_int64_t *grama)
{
    int lo,hi,mid,n;
    char *c;
    lo=0;
    hi=md->header.uniword_count-1;
    while (lo <= hi) {
        mid=(hi+lo)/2;
        c=md->strings + md->names[mid];
        n=strcmp(word,c);
        if (n < 0) hi=mid-1;
        else if (n>0) lo=mid+1;
        else {
            if (pisownia) *pisownia=md->strings+md->bin_words[mid].writename;
            if (baza) *baza=md->strings+md->bin_words[mid].basename;
            if (baseid) *baseid=md->bin_words[mid].baseword;
            if (grama) *grama=md->bin_words[mid].grama;
            return mid;
        }
    }
    return -1;
}



static int startswith(char *word,char *with)
{
    int n;
    for (n=0;*with;n++) {
	if (*with++ != *word++) return 0;
    }
    return n;
}


static char *counters[]={
    "wielo",
    "multi",
    "półtora",
    "pół",
    "ćwierć",
    "tysiąco",
    "kilku",
    "kilkunasto",
    "kilkudziesięcio",
    NULL};

static int is_count(char *word)
{
    int n,i;
    for (i=0;counters[i];i++) {
	if ((n=startswith(word,counters[i]))) {
	    return n;
	}
    }
    return 0;
}

static char *setki[]={
    "stu","dwustu","trzystu","czterystu","pięćset","sześćset",
    "siedmiuset","ośmiuset","dziewięciuset",
    "siedenset","osiemset","dziewięćset",NULL};

static char *jednostki[]={
    "jedno","dwu","trój","trzy","cztero","czworo",
    "pięcio","sześcio","siedmio","ośmio","dziewięcio",NULL};
static char *nastki[]={
    "dziesięcio","jedenasto","dwunasto","trzynasto",
    "czternasto","piętnasto","szesnasto","siedemnasto",
    "osiemnasto","dziewiętnasto",NULL};
static char *dziestki[]={
    "dwudziesto","trzydziesto","czterdziesto",
    "pięćdziesięcio","sześćdziesięcio","siedemdziesięcio",
    "osiemdziesięcio","dziewięćdziesięcio",NULL};

static int __morfologik_IdentifyWord(struct morfologik_data *md,char *word,char **pisownia,char ** baza,int *baseid,u_int64_t *grama,int flags);
  
static int __morfologik_IdentifyCount(struct morfologik_data *md,char *word,char **pisownia,char ** baza,int *baseid,u_int64_t *grama,int flags)
{
    int n,i,rc;
    u_int64_t loc_grama;
    char *loc_baza;
    char *loc_pisownia;
    int loc_baseid;
    char buf[strlen(word)+32];
    if (!flags && (n=startswith(word,"tysiąc"))) {
	word+=n;
	return __morfologik_IdentifyCount(md,word,pisownia,baza,baseid,grama,1);
    }
    if (flags <= 1) { //setki
	for (i=0;setki[i];i++) if ((n=startswith(word,setki[i]))) {
	    word += n;
	    return __morfologik_IdentifyCount(md,word,pisownia,baza,baseid,grama,2);
	}
    }
    if (flags <= 2) { //dziesiątki
	for (i=0;dziestki[i];i++) if ((n=startswith(word,dziestki[i]))) {
	    word += n;
	    return __morfologik_IdentifyCount(md,word,pisownia,baza,baseid,grama,3);
	}
	for (i=0;nastki[i];i++) if ((n=startswith(word,nastki[i]))) {
	    word += n;
	    goto idword;
	    return __morfologik_IdentifyWord(md,word,pisownia,baza,baseid,grama,MFLAG_CNT);
	}
	for (i=0;jednostki[i];i++) if ((n=startswith(word,jednostki[i]))) {
	    word += n;
	    goto idword;
	    return __morfologik_IdentifyWord(md,word,pisownia,baza,baseid,grama,MFLAG_CNT);
	}
    }
    if (flags <= 3) {
	for (i=0;jednostki[i];i++) if ((n=startswith(word,jednostki[i]))) {
	    word += n;
	    goto idword;
	    return __morfologik_IdentifyWord(md,word,pisownia,baza,baseid,grama,MFLAG_CNT);
	}
    }
    if (!flags) return -1;
idword:
    strcpy(buf,"wielo");
    strcat(buf,word);
    //fprintf(stderr,"Checking buf [%s]\n",buf);
    for (
	rc= __morfologik_IdentifyWord(md,buf,&loc_pisownia,&loc_baza,&loc_baseid,&loc_grama,MFLAG_CNT);
	rc >= 0;
	rc= __morfologik_IdentifyNext(md,rc,&loc_pisownia,&loc_baza,&loc_baseid,&loc_grama)) {
	    //fprintf(stderr,"ERD=%d [%s]\n",rc,(rc>=0)?loc_baza:"chuj");
	    loc_grama=__filter_nmid(loc_grama,NMID_cnt);
	    if (loc_grama) break;
    }
    if (rc < 0) for (
	rc= __morfologik_IdentifyWord(md,word,&loc_pisownia,&loc_baza,&loc_baseid,&loc_grama,MFLAG_CNT);
	rc >= 0;
	rc= __morfologik_IdentifyNext(md,rc,&loc_pisownia,&loc_baza,&loc_baseid,&loc_grama)) {
	    //fprintf(stderr,"ERD=%d [%s]\n",rc,(rc>=0)?loc_baza:"chuj");
	    loc_grama=__filter_nmid(loc_grama,NMID_cnt);
	    if (loc_grama) break;
    }
    if (rc < 0) return -1;
    if (grama)*grama=loc_grama;
    if (baseid) *baseid=loc_baseid;
    if (pisownia) *pisownia=loc_pisownia;
    if (baza) *baza=loc_baza;
    return NMID_SET(rc, NMID_cnt);
    
}

	    
	    
static int __morfologik_IdentifyWord(struct morfologik_data *md,char *word,char **pisownia,char ** baza,int *baseid,u_int64_t *grama,int flags)
{
    u_int64_t loc_grama;
    char *loc_baza;
    char *loc_pisownia;
    int loc_baseid;
    int len=strlen(word);
    int rc;
    int pgsg;
    char buf[2*len];

    //fprintf(stderr,"%s/%d\n",word,flags);
    rc=__morfologik_IdentifyWordSimple(md,word,pisownia,baza,baseid,grama);
    if (rc >= 0) {
	return rc;
    }
    
    if (!(flags & MFLAG_NEGATED) && !strncmp(word,"nie",3)) {
	flags |= MFLAG_NEGATED;
	word += 3;
	pgsg=NMID_negate;
	goto prefixer;
    }
    
    if (flags & MFLAG_STANDARD) return -1;
    if (!(flags & MFLAG_SUPER) && !strncmp(word,"arcy",4)) {
	flags |= MFLAG_SUPER;
	word += 4;
	pgsg=NMID_super;
	goto prefixer;
    }
    if (!(flags & MFLAG_SUPER) && (!strncmp(word,"super",5) || !strncmp(word,"ultra",5))) {
	flags |= MFLAG_SUPER;
	word += 5;
	pgsg=NMID_super;
	goto prefixer;
    }
    if (!(flags & (MFLAG_CNT))) {
	int n=is_count(word);
	if (n) {
	    word += n;
	    pgsg=NMID_cnt;
	    flags |= MFLAG_CNT;
	    goto prefixer;
	}
	rc=__morfologik_IdentifyCount(md,word,pisownia,baza,baseid,grama,0);
	if (rc >= 0) return rc;
    }

    if (__endswith(word,len,"noby") || __endswith(word,len,"toby")) {
	strcpy(buf,word);
	buf[len-2]=0;
	pgsg=NMID_impspot;
	goto get_emi;
    }
    
    if (flags & MFLAG_NOANCIENT) return -1;
    
    if (__endswith(word,len,"śwa")) {
	strcpy(buf,word);
	strcpy(buf+len-2,"my");
	pgsg=NMID_ista;
	goto get_emi;
    }

    if (__endswith(word,len,"ta")) {
	strcpy(buf,word);
	strcpy(buf+len-2,"cie");
	pgsg=NMID_ista;
	goto get_emi;
    }


    if (__endswith(word,len,"lim") || __endswith(word,len,"łym")) {
	strcpy(buf,word);
	strcpy(buf+len-1,"śmy");
	pgsg=NMID_alim;
	goto get_emi;
    }
    if (__endswith(word,len,"emi")) {
	strcpy(buf,word);
	if (buf[len-4] == 'i') {
	    strcpy(buf+len-3,"mi");
	}
	else {
	    strcpy(buf+len-3,"ymi");
	}
	pgsg=NMID_advemi;
	goto get_emi;
    }
    if (__endswith(word,len,"émi")) {
	strcpy(buf,word);
	if (buf[len-5] == 'i') {
	    strcpy(buf+len-4,"mi");
	}
	else {
	    strcpy(buf+len-4,"ymi");
	}
	pgsg=NMID_advemi;
	goto get_emi;
    }
    if (__endswith(word,len,"em")) {
	strcpy(buf,word);
	if (buf[len-3] == 'i') {
	    strcpy(buf+len-2,"im");
	}
	else {
	    strcpy(buf+len-2,"ym");
	}
	pgsg=NMID_advem;
	goto get_emi;
    }
    if (__endswith(word,len,"ém")) {
	strcpy(buf,word);
	if (buf[len-4] == 'i') {
	    strcpy(buf+len-3,"im");
	}
	else {
	    strcpy(buf+len-3,"ym");
	}
	pgsg=NMID_advem;
	goto get_emi;
    }
    if (__endswith(word,len,"śmy")) {
	strcpy(buf,word);
	buf[len-4]=0;
	pgsg=NMID_pindysmy;
	goto get_emi;
    }
    if (__endswith(word,len,"ście")) {
	strcpy(buf,word);
	buf[len-5]=0;
	pgsg=NMID_pindyscie;
	goto get_emi;
    }
    if (__endswith(word,len,"bym")) {
	strcpy(buf,word);
	buf[len-3]=0;
	pgsg=NMID_pindybym;
	goto get_emi;
    }
    if (__endswith(word,len,"byś")) {
	strcpy(buf,word);
	buf[len-4]=0;
	pgsg=NMID_pindybys;
	goto get_emi;
    }
    if (__endswith(word,len,"by")) {
	strcpy(buf,word);
	buf[len-2]=0;
	pgsg=NMID_pindyby;
	goto get_emi;
    }
    return -1;

prefixer:
	
	for (
	    rc=__morfologik_IdentifyWord(md,word,&loc_pisownia,&loc_baza,&loc_baseid,&loc_grama,flags);
	    rc >=0;
	    rc=__morfologik_IdentifyNext(md,rc,
                            &loc_pisownia,
                            &loc_baza,
                            &loc_baseid,
                            &loc_grama)) {
		loc_grama=__filter_nmid(loc_grama,pgsg);
		if (loc_grama) break;
	}
	if (rc < 0) return -1;
	if (grama)*grama=loc_grama;
	if (baseid) *baseid=loc_baseid;
	if (pisownia) *pisownia=loc_pisownia;
	if (baza) *baza=loc_baza;
	return NMID_SET(rc, pgsg);
    

get_emi:
	for (
	    rc=__morfologik_IdentifyWordSimple(md,buf,&loc_pisownia,&loc_baza,&loc_baseid,&loc_grama);
	    rc >= 0;
	    rc=__morfologik_IdentifyNext(md,rc,
                            &loc_pisownia,
                            &loc_baza,
                            &loc_baseid,
                            &loc_grama)) {
	    
	    loc_grama=__filter_nmid(loc_grama,pgsg);
	    if (loc_grama) break;
	}
	if (rc < 0) return -1;
	if (grama)*grama=loc_grama;
	if (baseid) *baseid=loc_baseid;
	if (pisownia) *pisownia=loc_pisownia;
	if (baza) *baza=loc_baza;
	return NMID_SET(rc, pgsg);
    return -1;
}
    
int morfologik_IdentifyWord(struct morfologik_data *md,char *word,char **pisownia,char ** baza,int *baseid,u_int64_t *grama)
{
    return __morfologik_IdentifyWord(md,word,pisownia,baza,baseid,grama,0);
}

int morfologik_IdentifyWordStd(struct morfologik_data *md,char *word,char **pisownia,char ** baza,int *baseid,u_int64_t *grama)
{
    return __morfologik_IdentifyWord(md,word,pisownia,baza,baseid,grama,MFLAG_STANDARD);
}

int morfologik_IdentifyWordWithFlags(struct morfologik_data *md,char *word,char **pisownia,char ** baza,int *baseid,u_int64_t *grama,int flags)
{
    return __morfologik_IdentifyWord(md,word,pisownia,baza,baseid,grama,flags);
}

const char *morfologik_WordAt(struct morfologik_data *md, int numword,
                        char **pisownia,
                char ** baza,
                int *baseid,
                u_int64_t *grama)
{
    if (numword < 0 || numword >= md->header.uniword_count) return NULL;
    if (pisownia) *pisownia=md->strings+md->bin_words[numword].writename;
    if (baza) *baza=md->strings+md->bin_words[numword].basename;
    if (baseid) *baseid=md->bin_words[numword].baseword;
    if (grama) *grama=md->bin_words[numword].grama;
    return md->strings+md->bin_words[numword].writename;
}

int morfologik_WordCount(struct morfologik_data *md)
{
    return md->header.uniword_count;
}

static int __good_filter(u_int64_t filter,u_int64_t grama)
{
    int wt=WT_GET(filter);
    if (wt && wt != WT_GET(grama)) return 0;
    filter &= (1LL << WM_SEQ_END) -1;
    if (!filter) return 1;
    if (filter & WM_CASU_MASK) {
        if (!(filter & WM_CASU_MASK & grama)) return 0;
        filter |= WM_CASU_MASK;
        grama |= WM_CASU_MASK;
    }
    if (filter & WM_GENR_MASK) {
        if (!(filter & WM_GENR_MASK & grama)) return 0;
        filter |= WM_GENR_MASK;
        grama |= WM_GENR_MASK;
    }
    if ((grama & filter) != filter) return 0;
    return 1;
}

int morfologik_FilterGrama(u_int64_t grama,u_int64_t filter)
{
    return __good_filter(filter,grama);
}

static int __genWord(struct morfologik_data *md,
                       int vid,
                       char **pisownia,
                       u_int64_t *grama,
                       u_int64_t filter)
{
    for (;;) {
        int word=md->vector[vid];
        if (word < 0) return -1;
        vid ++;
        if (filter && !__good_filter(filter,md->bin_words[word].grama)) continue;
        if (pisownia) *pisownia=md->strings+md->bin_words[word].writename;
        if (grama) *grama=md->bin_words[word].grama;
        return vid;
    }
}

int morfologik_GenNext(struct morfologik_data *md,
                       int vid,
                       char **pisownia,
                       u_int64_t *grama,
                       u_int64_t filter)
{
    return __genWord(md,vid,pisownia,grama,filter);
}

int morfologik_GenWordById(struct morfologik_data *md,
                       int baseid,
                       char **pisownia,
                       u_int64_t *grama,
                       u_int64_t filter
                       )
{
    return __genWord(md,md->base_words[baseid].vector,pisownia,grama,filter);
}

int morfologik_GenWord(struct morfologik_data *md,
                       char *baseword,
                       char **pisownia,
                       u_int64_t *grama,
                       u_int64_t filter
                       )
{
    int lo,hi,mid,n;
    char *c;
    lo=0;
    hi=md->header.base_count-1;
    while (lo <= hi) {
        mid=(lo+hi)/2;
        c=md->strings+md->base_words[mid].name;
        n=strcmp(baseword,c);
        if (n < 0) hi=mid-1;
        else if (n>0) lo=mid+1;
        else return __genWord(md,md->base_words[mid].vector,pisownia,grama,filter);
    }
    return -1;
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
    
    
    
u_int64_t morfologik_ParseGrama(char *str)
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

int morfologik_DecodeGrama(u_int64_t grama,char *buf)
{
    int g=WT_GET(grama);
    int p,i,ext=0;
    buf[0]=0;
    if (g < 1 || g > WT_last) return -1;
    if (g == WT_subst || g == WT_verb || (grama & WM_cmplx)) ext=1;
    strcpy(buf,wt_markers[g-1]);
//    printf("BUF=[%s]\n",buf);
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
//	printf("BUF=[%s]\n",buf);
    }
    if (grama & WM_pred) {
        strcat(buf,":pred");
//	printf("BUF=[%s]\n",buf);
    }
    if (grama & WM_CASU_MASK) {
        for (i=p=0;i<7;i++) {
            if (! (grama & (WM_nom << i))) continue;
            if (p) strcat(buf,".");else strcat(buf,":");
            p++;
            strcat(buf,wm_casa[i]);
//	    printf("BUF=[%s]\n",buf);
        }
    }
    if (grama & WM_GRAD_MASK) {
        for (i=p=0;i<3;i++) {
            if (! (grama & (WM_pos << i))) continue;
            if (p) strcat(buf,".");else strcat(buf,":");
            p++;
            strcat(buf,wm_grad[i]);
//	    printf("BUF=[%s]\n",buf);
        }
    }
    if (grama & WM_PERS_MASK) {
        for (i=p=0;i<3;i++) {
            if (! (grama & (WM_pri << i))) continue;
            if (p) strcat(buf,".");else strcat(buf,":");
            p++;
            strcat(buf,wm_pers[i]);
//	    printf("BUF=[%s]\n",buf);
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
//        printf("BUF=[%s]\n",buf);
    }
    for (i=WM_SEQ_FIRST;i<WM_SEQ_END;i++) {
        u_int64_t mask = 1LL << i;
        if (!(grama & mask)) continue;
        if (mask == WM_imperf && (grama & WM_perf)) strcat(buf,".");
        else strcat(buf,":");
	char *c=wm_seq[i-WM_SEQ_FIRST][ext];
	if (!c) c=wm_seq[i-WM_SEQ_FIRST][0];
//	printf("I=%d\n",i);
        strcat(buf,c);
//	printf("BUF=[%s]\n",buf);
    }
}

static unichar utol[]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
	32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
	48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
	64,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,
	112,113,114,115,116,117,118,119,120,121,122,91,92,93,94,95,
	96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,
	112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
	128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
	144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
	160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
	176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
	224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
	240,241,242,243,244,245,246,215,248,249,250,251,252,253,254,223,
	224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
	240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,
	257,257,259,259,261,261,263,263,265,265,267,267,269,269,271,271,
	273,273,275,275,277,277,279,279,281,281,283,283,285,285,287,287,
	289,289,291,291,293,293,295,295,297,297,299,299,301,301,303,303,
	105,305,307,307,309,309,311,311,312,314,314,316,316,318,318,320,
	320,322,322,324,324,326,326,328,328,329,331,331,333,333,335,335,
	337,337,339,339,341,341,343,343,345,345,347,347,349,349,351,351,
	353,353,355,355,357,357,359,359,361,361,363,363,365,365,367,367,
	369,369,371,371,373,373,375,375,255,378,378,380,380,382,382,383 };

static int utolower(unichar znak)
{
    if (znak < 0x180) return utol[znak];
    return znak;
}

static int get_uniword(char *str, unichar *out)
{
    unichar znak;
    int fs, n, len;
    len = 0;
    while (*str) {
        fs = *str++;
        if (!(fs & 0x80)) {
            znak = fs;
        }
        else {
            if ((fs & 0xe0) == 0xc0) {
                znak = fs & 0x1f;
                n = 1;
            }
            else if (fs & 0xf0 == 0xe0) {
                znak = fs & 0x0f;
                n = 2;
            }
            else return 0;
            while (n--) {
                fs = *str++;
                if ((fs & 0xc0) != 0x80) return 0;
                znak = (znak <<6) | (fs & 0x3f);
            }
        }
        *out++ = utolower(znak);
        len++;
        if (len == 31) break;
    }
    *out = 0;
    return len;
}

static int gainb(unichar znak, wchar_t *word)
{
    while (*word) {
        if (znak == *word++) return 1;
    }
    return 0;
}

static void dedup(char *c)
{
    char *d=c, last=0;
    int n = 0;
    while (*d) {
        char s=*d++;
        if (strchr("aeo",s)) s='a';
        if (s != last) {
            *c++ = last = s;
            n++;
            if (n >= 8) break;
            
        }
    }
    *c=0;
}

char *mkoword(unichar *word, char *oword)
{
    char *c=oword;
    for (;*word;word++) {
        if (*word == '-') return NULL;
        if (*word == '\'') return NULL;
        if (*word == 'c' && word[1] == 'h') {
            *c++='h';
            word++;
            continue;
        }
        if ((*word == 'c' || *word == 's' || *word == 'r') && word[1] == 'z') {
            *c++=*word;
            word++;
            continue;
        }
        if (*word == 'b') {*c++='p';continue;}
        if (*word == 'g') {*c++='k';continue;}
        if (*word == 'z') {*c++='s';continue;}
        if (*word == L'ą') {*c++='o'; continue;}
        if (*word == L'ę') {*c++='e'; continue;}
        if (*word == L'ś') {*c++='s'; continue;}
        if (*word == L'ć') {*c++='c'; continue;}
        if (*word == L'ń') {*c++='n'; continue;}
        if (*word == L'ó') {*c++='o'; continue;}
        if (*word == L'ź') {*c++='s'; continue;}
        if (*word == L'ż') {*c++='s'; continue;}
        if (*word == L'ł') {*c++='l'; continue;}
        if (*word == 'u') {*c++='o'; continue;}
        if (*word == 'a') {*c++='o'; continue;}
        if (*word == 'l') {*c++='r'; continue;}
        if (*word == 'd' && gainb(word[1],L"zźż")) {
            *c++='c'; word++; continue;
        }
        if (*word == 't' && word[1] =='c') continue;
        if (*word == 'd' && word[1] =='c') continue;
        if (*word == 'd') {*c++='t';continue;}
        if (gainb(*word,L"ij") && gainb(word[1],L"aeiouyąę")) continue;
        if (*word == 'i') {*c++='y';continue;}
        if (*word == 'w') {*c++='f';continue;}
        if (*word == 'v') {*c++='f';continue;}
        if (*word == 'x') {*c++='k';*c++='s';continue;}
        if (*word >= 0x80) {
            //printf("Bad char %x\n", *word);
            return NULL;
        }
        *c++=*word;
    }
    *c=0;
    dedup(oword);
    return oword;
}
static int __spl_word(unichar *word, unichar *oword)
{
    unichar *ow = oword;
    for (;*word;word++) {
        if (*word == 's' && word[1] == 'z') {
            *ow++ = 'S'; word++; continue;
        }
        if (*word == 'c' && word[1] == 'z') {
            *ow++ = 'C'; word++; continue;
        }
        if (*word == 'c' && word[1] == 'h') {
            *ow++ = 'H'; word++; continue;
        }
        if (*word == 'r' && word[1] == 'z') {
            *ow++ = 'R'; word++; continue;
        }
        if (*word == 'k' && word[1] == 's') {
            *ow++ = 'X'; word++; continue;
        }
        if (*word == 'l' && word[1] == 'l') {
            *ow++ = 'L'; word++; continue;
        }
        *ow++ = *word;
    }
    *ow = 0;
    return ow - oword;
}

static int __gstrlen(unichar *w)
{
    int i;
    for (i=0;*w;w++, i++);
    return i;
}
static int __cost(unichar a, unichar b)
{
    static wchar_t s1[]=L"aescnozzluCCSSRHbgwvvqxl";
    static wchar_t s2[]=L"ąęśćńóźżłócćsśżhpkffwkXL";
    static wchar_t s3[]=L"CSRHX";
    int i, dua;
    
    if (a == b) return 0;
    for (i=0; s1[i]; i++) {
        if ((s1[i] == a && s2[i] == b) || (s2[i] == a && s1[i] == b)) return 1;
    }
    if ((a == 'r' && b == 'l') || (a == 'l' && b == 'r')) return 2; // leworwer
    dua=0;
    for (i=0;s3[i];i++) {
        if (a == s3[i]) dua |= 1;
        if (b == s3[i]) dua |= 2;
    }
    if (dua == 1 || dua == 2) return 20;
    return 10;
}

static int __odist(char *word1, int len1, char *word2, int len2)
{
    short d[10][10];
    int i, j, d1,d2,d3,d4, cost;
    for (i=0; i<=len1; i++) d[i][0] = i;
    for (j=0; j<=len2; j++) d[0][j] = j;
    for (i=0; i<len1; i++) {
        for (j=0; j<len2; j++) {
            cost = (word1[i] == word2[j]) ? 0 : 1;
            d1 = d[i][j+1] + 1;
            d2 = d[i+1][j] + 1;
            d3 = d[i][j] + cost;
            if (d1 > d2) d1=d2;
            if (d1 > d3) d1 = d3;
             if(i>0 && j>0 && word1[i]==word2[j-1] && word1[i-1]==word2[j] ) {
                d4 = d[i-1][j-1] + cost;
                if (d1 > d4) d1 = d4;
             }
             d[i+1][j+1] = d1;
        }
    }
    return d[len1][len2];
}

static int __edist(unichar *word1, int len1, unichar *word2, int len2)
{
    short d[33][33];
    int i, j, d1,d2,d3,d4, cost;
    if (len1 <= 0) len1 = __gstrlen(word1);
    if (len2 <= 0) len2 = __gstrlen(word2);
    for (i=0; i<=len1; i++) d[i][0] = i * 10;
    for (j=0; j<=len2; j++) d[0][j] = j * 10;
    for (i=0; i<len1; i++) {
        for (j=0; j<len2; j++) {
            cost = __cost(word1[i], word2[j]);
            d1 = d[i][j+1] + 10;
            d2 = d[i+1][j] + 10;
            d3 = d[i][j] + cost;
            if (d1 > d2) d1=d2;
            if (d1 > d3) d1 = d3;
            if(i>0 && j>0 && word1[i]==word2[j-1] && word1[i-1]==word2[j] ) {
                d4 = d[i-1][j-1] + cost;
                if (d1 > d4) d1 = d4;
             }
             d[i+1][j+1] = d1;
        }
    }
    return d[len1][len2];
    
}


static int __get_words(struct morfologik_data *md, unichar *word, int len, int adr, u_int32_t *wdata, int *wcount, int *alena)
{
    unichar fword[64], worda[64], wordb[64], znak;
    int len1 = __spl_word(word, worda);
    for (; md->mflist[adr]; adr++) {
        u_int32_t nt =  md->mflist[adr];
        int loclen = nt >> 24;
        if (loclen < len-2 || loclen > len + 2) continue;
        nt = nt & 0xffffff;
        char *c = md->strings+md->bin_words[nt].writename;
        get_uniword(c, fword);
        int len2 = __spl_word(fword, wordb);
        int dis = __edist(worda, len1, wordb, len2);
        if (dis > *alena) continue;
        if (dis < *alena) {
            *wcount = 0;
            *alena = dis;
        }
        if (*wcount < MF_MAXFINDRES) wdata[(*wcount)++] = nt;
    }
    return *wcount;
}

int morfologik_Distance(char *wrd1, char *wrd2)
{
    unichar worda[64], wordb[64], oword[64];
    int len1, len2;
    get_uniword(wrd1, oword);
    len1 = __spl_word(oword, worda);
    get_uniword(wrd2, oword);
    len1 = __spl_word(oword, wordb);
    return __edist(worda, len1, wordb, len2);
}

int morfologik_Fonex(char *wrd, char *fonex)
{
    unichar word[64];
    get_uniword(wrd, word);
    if (!mkoword(word, fonex)) return -1;
    return 0;
}

int morfologik_FindWord(struct morfologik_data *md, char *slowko, int fast, u_int32_t *wdata, int *wcount, int *alena)
{
    unichar word[32], znak;
    char oword[64];
    int i,j,s;

    *alena = 30;
    *wcount = 0;
    j = get_uniword(slowko, word);
    if (j >= 31 || j <= 2) return -1;
    if (!mkoword(word, oword)) return -1;
    int lo, hi, mid, found=0;
    if (fast) {
        lo = 0;
        hi = md->header.fonex_count -1;
        while (!found && lo <= hi) {
            mid = (lo + hi) / 2;
            char *c = md->strings + md->fonex[2 * mid];
            int n = strcmp(c, oword);
            if (n > 0) hi = mid - 1;
            else if (n < 0) lo = mid + 1;
            else found = 1;
        }
        if (found) {
            __get_words(md, word, j, md->fonex[2*mid + 1], wdata, wcount, alena);
            if (*wcount && *alena < 6) return *wcount;
        }
    }
    s=strlen(oword);
    for (i=0; i< md->header.fonex_count; i++) {
        char *sword = md->strings + md->fonex[2 * i];
        int t = strlen(sword);
        if (t < s-1 || t > s+1) continue;
        int n = __odist(oword, s, sword, t);
        if (n > 2) continue;
        __get_words(md, word, j, md->fonex[2*i + 1], wdata, wcount, alena);
    }
    return *wcount;
}


#if 0
main(int argc,char *argv[])
{
    char *pi,*ba;u_int64_t grama,filter;char buf[256];
    int id,bid,vid;
    struct morfologik_data *md;
    
    
    //md=morfologik_Init("morfologik.bin");
    md=morfologik_InitSHM();
    if (!md) md=morfologik_Init("morfologik.bin");
    if (!md) {
        perror("Chujy");
        exit(1);
    }
    
    if (argc >2) {
        filter=morfologik_ParseGrama(argv[2]);
        if (!filter) {
            perror("Filter");
            exit(1);
        }
        
        morfologik_DecodeGrama(filter,buf);
        printf("%s\n",buf);
    }
    else filter=0;
    
    printf("OK, starting\n");
    id=morfologik_IdentifyWord(md,argv[1],&pi,&ba,&bid,&grama);
    while (id >= 0) {
        morfologik_DecodeGrama(grama,buf);
        printf("%s/%s %s\n",pi,ba,buf);
        printf("Odmianka\n");
        vid=morfologik_GenWordById(md,bid,&pi,&grama,filter);
        while (vid >= 0) {
            morfologik_DecodeGrama(grama,buf);
            printf("    %s %s\n",pi,buf);
            vid=morfologik_GenNext(md,vid,&pi,&grama,filter);
        }
        printf("\n");
        id=morfologik_IdentifyNext(md,id,&pi,&ba,&bid,&grama);
    }
    
}
#endif
