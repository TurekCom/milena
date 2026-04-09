/*
 * mod_mbrola.h - Mbrola output module for Milena TTS system
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


#ifndef MILENA_MOD_MBROLA_H
#define MILENA_MOD_MBROLA_H 1



struct phone_buffer {
	char *str;
	int str_len;
	int buf_len;
};

#ifdef __cplusplus
extern "C" {
#endif

struct milena_mbrola_cfg *milena_InitModMbrola(char *fname);
int milena_ModMbrolaExtras(struct milena_mbrola_cfg *cfg,char *fname);

void milena_ModMbrolaSetVoice(struct milena_mbrola_cfg *cfg,int param,...);

void milena_ModMbrolaGenPhrase(struct milena_mbrola_cfg *cfg,
	char *bufor,
	FILE *f,
	int pmode);
void milena_ModMbrolaGenPhraseP(struct milena_mbrola_cfg *cfg,
	char *bufor,
	struct phone_buffer *phb,
	int pmode);


int milena_ReadInton(struct milena_mbrola_cfg *cfg,char *fname);


void milena_ModMbrolaBreak(struct milena_mbrola_cfg *cfg,FILE *f,int pau);
void milena_ModMbrolaBreakP(struct milena_mbrola_cfg *cfg,struct phone_buffer *phb,int pau);

void milena_ModMbrolaSetFlag(struct milena_mbrola_cfg *cfg,int flag,int mask);
void milena_CloseModMbrola(struct milena_mbrola_cfg *cfg);

#ifdef __cplusplus
}
#endif



#define MILENA_MBFL_EQUALIZE 1
#define MILENA_MBFL_BOOKMODE 2
#define MILENA_MBFL_MOUTH 4

#define MILENA_MBP_PITCH 1
#define MILENA_MBP_RANGE 2
#define MILENA_MBP_STRESLEN 3

#define MILENA_MBRK_NONE 0
#define MILENA_MBRK_NORMAL 1
#define MILENA_MBRK_DIALOG 2
#define MILENA_MBRK_PREDIAL 3
#define MILENA_MBRK_POSTDIAL 4
#define MILENA_MBRK_LONG 5

#endif
