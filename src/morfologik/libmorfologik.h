/*
 * libmorfologik.h - fast and simple interface to morfologik
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

#ifndef LIBMORFOLOGIK_H
#define LIBMORFOLOGIK_H 1

struct morfologik_data;

/* serwer SHM */
extern void morfologik_CloseShm(struct morfologik_data *data);
extern struct morfologik_data *morfologik_CreateShm(char *fname);


extern void morfologik_Free(struct morfologik_data *data);
extern struct morfologik_data *morfologik_Init(char *fname);
extern struct morfologik_data *morfologik_InitSHM(void);


extern int morfologik_WordCount(struct morfologik_data *md);
extern const char *morfologik_WordAt(struct morfologik_data *md,
                int numword,
                char **pisownia,
                char ** baza,
                int *baseid,
                u_int64_t *grama);


extern int morfologik_FindWord(struct morfologik_data *md,
                char *slowko,
                int fast,
                u_int32_t *wdata,
                int *wcount,
                int *alena);

extern int morfologik_Distance(char *wrd1, char *wrd2);
extern int morfologik_Fonex(char *wrd, char *fonex);

/*
 *
 * zwraca ID (wewnętrzna wartość)
 */

extern int morfologik_GenWord(struct morfologik_data *md,
                       char *baseword,
                       char **pisownia,
                       u_int64_t *grama,
                       u_int64_t filter);
extern int morfologik_GenWordById(struct morfologik_data *md,
                       int baseid,
                       char **pisownia,
                       u_int64_t *grama,
                       u_int64_t filter);
extern int morfologik_GenNext(struct morfologik_data *md,
                       int vid,
                       char **pisownia,
                       u_int64_t *grama,
                       u_int64_t filter);

/*
 * word - słowo (utf-8, małymi literami) do identyfikacji
 * pisownia - znaleziona pisownia słowa
 * baza - znalezione słowo bazowe
 * baseid - znaleziony ID słowa bazowego (dla getWord)
 * grama - znaleziona forma gramatyczna
 * Zwraca ID sekwencji (dla identifyNext) lub ujemną wartość jeśli brak
 */

extern int morfologik_IdentifyWord(struct morfologik_data *md,char *word,
                            char **pisownia,
                            char ** baza,
                            int *baseid,
                            u_int64_t *grama);

/*
 * jak IdentifyWord, ale nie szuka złożeń poza przeczeniem
 */
 
extern int morfologik_IdentifyWordStd(struct morfologik_data *md,char *word,
                            char **pisownia,
                            char ** baza,
                            int *baseid,
                            u_int64_t *grama);

/*
 * jak IdentifyWord, dopasowania sterowane flagami
 */

extern int morfologik_IdentifyWordWithFlags(struct morfologik_data *md,char *word,
                            char **pisownia,
                            char ** baza,
                            int *baseid,
                            u_int64_t *grama,
			    int flags);

/*
 * ID - id sekwencji z poprzedniego wywołania IdentifyWord/IdentifyNext
 * pisownia - znaleziona pisownia słowa
 * baza - znalezione słowo bazowe
 * baseid - znaleziony ID słowa bazowego (dla getWord)
 * grama - znaleziona forma gramatyczna
 * Zwraca ID sekwencji (dla identifyNext) lub ujemną wartość jeśli brak
 */


extern int morfologik_IdentifyNext(struct morfologik_data *md,int id,
                            char **pisownia,
                            char ** baza,
                            int *baseid,
                            u_int64_t *grama);


extern u_int64_t morfologik_ParseGrama(char *str);
extern int morfologik_DecodeGrama(u_int64_t grama,char *buf);
extern int morfologik_FilterGrama(u_int64_t grama,u_int64_t filter);

// znaczenie bitów w grama:
// terminologia - patrz morfologik/readme_pl.txt

#define WT_SHIFT 58
#define WT_MASK (31LL << WT_SHIFT)

#define WT_GET(a) (((a) >> WT_SHIFT) & 31)
#define WT_SET(a,t) (((a) & ~(31LL << WT_SHIFT)) | (((u_int64_t)(t)) << WT_SHIFT))

#define WT_adj  1
#define WT_adjp  2
#define WT_adv  3
#define WT_conj 4
#define WT_num  5
#define WT_pact 6
#define WT_pant 7
#define WT_pcon 8
#define WT_ppas 9
#define WT_ppron12 10
#define WT_ppron3 11
#define WT_pred 12
#define WT_adjc 13
#define WT_siebie 14
#define WT_subst 15
#define WT_verb 16
#define WT_brev 17
#define WT_interj 18
#define WT_xxx 19
#define WT_nie 20
#define WT_advp 21
#define WT_prep 22
#define WT_comp 23

#define WT_last 23

#define WM_sg   (1LL<<0)
#define WM_pl   (1LL<<1)

#define WM_NUM_MASK (3LL)

#define WM_pred (1LL<<2)

#define WM_nom (1LL<<3)
#define WM_gen (1LL<<4)
#define WM_dat (1LL<<5)
#define WM_acc (1LL<<6)
#define WM_inst (1LL<<7)
#define WM_loc (1LL<<8)
#define WM_voc (1LL<<9)

#define WM_CASU_MASK (127LL<<3)

#define WM_pos (1LL<<10)
#define WM_comp (1LL<<11)
#define WM_sup (1LL<<12)

#define WM_GRAD_MASK (7LL << 10)

#define WM_m1 (1LL<<13)
#define WM_m2 (1LL<<14)
#define WM_m3 (1LL<<15)
//#define WM_m (1LL<<16)
#define WM_f (1LL<<16)
#define WM_n1 (1LL<<17)
#define WM_n2 (1LL<<18)
//#define WM_n (1LL<<20)
#define WM_p1 (1LL<<19)
#define WM_p2 (1LL<<20)
#define WM_p3 (1LL<<21)
//#define WM_p (1LL<<24)

#define WM_GENR_MASK (511LL << 13)

#define WM_pri (1LL<<22)
#define WM_sec (1LL<<23)
#define WM_tri (1LL<<24)

#define WM_PERS_MASK (7LL << 22)

#define WM_aff (1LL<<25)
#define WM_neg (1LL<<26)
#define WM_perf (1LL<<27)
#define WM_imperf (1LL<<28)
#define WM_nakc (1LL<<29)
#define WM_akc (1LL<<30)
#define WM_praep (1LL<<31)
#define WM_npraep (1LL<<32)
#define WM_imps (1LL<<33)
#define WM_impt (1LL<<34)
#define WM_inf (1LL<<35)
#define WM_fin (1LL<<36)
#define WM_praet (1LL<<37)
#define WM_pot (1LL<<38)
#define WM_nstd (1LL<<39)
#define WM_pun (1LL<<40)
#define WM_npun (1LL<<41)
#define WM_rec (1LL<<42)
#define WM_congr (1LL<<43)

#define WM_winien (1LL<<44)
#define WM_bedzie (1LL<<45)
#define WM_refl (1LL<<46)
#define WM_nonrefl (1LL<<47)
#define WM_depr (1LL<<48)
#define WM_vulgar (1LL<<49)
#define WM_illegal (1LL<<50)
#define WM_ger  (1LL<<51)
#define WM_wok (1LL<<52)
#define WM_nwok (1LL<<53)
#define WM_cmplx (1LL<<54)


// dla czasowników i rzeczowników

#define WM_str2 WM_nakc
#define WM_str3 WM_akc
#define WM_str4 WM_wok

// następne ważne tylko dla cmplx
// liczba pojedyncza czasownika

#define WM_sgcplx WM_nwok

// rzeczowniki, przymiotniki i przysłówki
#define WM_super WM_pun
#define WM_cnt WM_npun


#define WM_SEQ_FIRST 25
#define WM_SEQ_END 55

#define MFLAG_STANDARD 1
#define MFLAG_NEGATED 2
#define MFLAG_SUPER 4
#define MFLAG_CNT 8
#define MFLAG_NOANCIENT 16


#define MF_FILE_MAGIC 0x31EF0194
#define MF_SHMAT_MAGIC 0x30FA0100
#define MF_NAMES_KEY (MF_SHMAT_MAGIC+1)
#define MF_BINWORDS_KEY (MF_SHMAT_MAGIC+2)
#define MF_BASEWORDS_KEY (MF_SHMAT_MAGIC+3)
#define MF_VECTORS_KEY (MF_SHMAT_MAGIC+4)
#define MF_FONEX_KEY (MF_SHMAT_MAGIC+5)
#define MF_MFLIST_KEY (MF_SHMAT_MAGIC+6)
#define MF_STRINGS_KEY (MF_SHMAT_MAGIC+7)

#define MF_MAXFINDRES 64

#endif
