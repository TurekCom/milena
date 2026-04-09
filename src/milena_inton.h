/*
 * milena_inton.h - Mbrola output module for Milena TTS system
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


#ifndef MILENA_INTON_H
#define MILENA_INTON_H 1

struct syl_env {
	int stress;
	int npitch;
	int marked;
	struct env_data *env;
	int pitch1,pitch2;
	double pitches[8];
	int syl_len;
	int offsets[8];
};

struct voice_param {
	double base_pitch;
	double range;
};


struct env_data {
	struct env_data *next;
	char *name;
	int count;
	int offset[8];
	double value[8];
};

struct pitcher_data {
	struct syl_env *syllables;
	int count;
	int clause_tone;
	int tone_type;
	int number_pre;
	int number_body;
	int number_tail;
	int tone_posn;
	int annotation;
	int last_primary;
	struct intonator *inton;
};

struct tone_table {
   struct env_data *pitch_env0;     /* pitch envelope, tonic syllable at end */
   struct env_data *pitch_env1;     /*     followed by unstressed */

   unsigned char tonic_max0;
   unsigned char tonic_min0;

   unsigned char tonic_max1;
   unsigned char tonic_min1;

   unsigned char pre_start;
   unsigned char pre_end;

   unsigned char body_start;
   unsigned char body_end;

   unsigned char body_max_steps;
   unsigned char body_lower_u;

   unsigned char tail_start;
   unsigned char tail_end;
};

struct intonator {
	struct voice_param vp;
	struct env_data *rise,*fall,*env,*singles[6];
	struct tone_table tone_table[6];
	int pitch_range2;
	int pitch_base2;
	
};

#define STRESS_SECONDARY  3
#define STRESS_PRIMARY    4
#define STRESS_PRIMARY_MARKED 6
#define STRESS_BODY_RESET 7
#define STRESS_FIRST_TONE 8    /* first of the tone types */



#endif
