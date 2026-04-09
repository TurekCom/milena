/***************************************************************************
 *   Based on intonation.cpp (from eSpeak speech synthesizer)              *
 *   Copyright (C) 2005 to 2007 by Jonathan Duddington                     *
 *   email: jonsd@users.sourceforge.net                                    *
 *                                                                         *
 *   Adaptation to Milena TTS System:                                      *
 *   Copyright (C) Bohdan R. Rau 2008 <ethanak@polip.com>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write see:                           *
 *               <http://www.gnu.org/licenses/>.                           *
 ***************************************************************************/
#include "milena_inton.h"


static void count_pitch_vowels(struct pitcher_data *pd)
{
	int i,stres;
	pd->tone_posn = 0;
	for (i=0;i<pd->count;i++) {
		stres=pd->syllables[i].stress & 0x3f;
		if (stres >= STRESS_PRIMARY) break;
	}
	pd->number_pre=i;
	for (i=pd->count-1;i>=0;i--) {
		stres=pd->syllables[i].stress & 0x3f;
		if (stres >= STRESS_PRIMARY) break;
	}
	pd->tone_posn=i;
	pd->syllables[pd->tone_posn].stress = STRESS_FIRST_TONE;
	pd->number_tail = pd->count - pd->tone_posn - 1;
	
		
}
static int count_increments(struct pitcher_data *pd,int ix, int end_ix, int min_stress)
/*************************************************************/
/* Count number of primary stresses up to tonic syllable or body_reset */
{
	int  count = 0;
	int  stress;

	while(ix < end_ix)
	{
		stress = pd->syllables[ix++].stress & 0x3f;
		if(stress >= STRESS_BODY_RESET)
			break;
		else
		if(stress >= min_stress)
			count++;
	}
	return(count);
}  /* end of count_increments */

static void set_pitch(struct pitcher_data *pd,int ix, int base, int drop)
/***********************************************/
// Set the pitch of a vowel in vowel_tab.  Base & drop are Hz * 256
{
	int  pitch1, pitch2;


	if(base < 0)  base = 0;

	pitch2 = ((base * pd->inton->pitch_range2 ) >> 15) + pd->inton->pitch_base2;

	if(drop < 0)
	{
		pd->syllables[ix].env=pd->inton->rise;
		drop = -drop;
	}
	else pd->syllables[ix].env=pd->inton->fall;

	pitch1 = pitch2 + ((drop * pd->inton->pitch_range2) >> 15);

	if(pitch1 > 511) pitch1 = 511;
	if(pitch2 > 511) pitch2 = 511;
	
	if (pitch1>pitch2) {
		int x=pitch1;
		pitch1=pitch2;
		pitch2=x;
	}
	
	pd->syllables[ix].pitch1=pitch1;
	pd->syllables[ix].pitch2=pitch2;
}   /* end of set_pitch */

static int calc_pitch_segment2(struct pitcher_data *pd,int ix, int end_ix, int start_p, int end_p)
/****************************************************************************************/
/* Linear pitch rise/fall, change pitch at min_stress or stronger
	Used for pre-head and tail */
{
	int  stress;
	int  pitch;
	int  increment;
	int  n_increments;
	int  drop;
	static int min_drop[] =  {0x300,0x300,0x300,0x300,0x300,0x500,0xc00,0xc00};

	if(ix >= end_ix)
		return(ix);
		
	n_increments = count_increments(pd,ix,end_ix,0);
	increment = (end_p - start_p) << 8;
	
	if(n_increments > 1)
	{
		increment = increment / n_increments;
	}

	
	pitch = start_p << 8;
	while(ix < end_ix)
	{
		stress = pd->syllables[ix].stress & 0x3f;

		if(increment > 0)
		{
			set_pitch(pd,ix,pitch,-increment);
			pitch += increment;
		}
		else
		{
			drop = -increment;
			if(drop < min_drop[stress])
				drop = min_drop[stress];
				
			pitch += increment;
			set_pitch(pd,ix,pitch,drop);
		}
			
		ix++;
	}
	return(ix);
}   /* end of calc_pitch_segment2 */



static int calc_pitch_segment(struct pitcher_data *pd,
	int ix,
	int end_ix,
	struct tone_table *t,
	int min_stress)
/******************************************************************************/
/* Calculate pitches until next RESET or tonic syllable, or end.
	Increment pitch if stress is >= min_stress.
	Used for tonic segment */
{
	int  stress;
	int  pitch=0;
	int  increment=0;
	int  n_primary=0;
	int  initial;
	int  overflow=0;

	static char overflow_tab[5] = {0, 5, 3, 1, 0};
	static int drops[]={0x400,0x400,0x700,0x700,0x700,0xa00,0x0e00,0x0e00};

	initial = 1;
	while(ix < end_ix)
	{
		stress = pd->syllables[ix].stress & 0x3f;

		if(stress == STRESS_BODY_RESET)
			initial = 1;

		if(initial || (stress >= min_stress))
		{
			if(initial)
			{
				initial = 0;
				overflow = 0;
				n_primary = count_increments(pd,ix,end_ix,min_stress);

				if(n_primary > t->body_max_steps)
					n_primary = t->body_max_steps;

				if(n_primary > 1)
				{
					increment = (t->body_end - t->body_start) << 8;
					increment = increment / (n_primary -1);
				}
				else
					increment = 0;

				pitch = t->body_start << 8;
			}
			else
			{
				if(n_primary > 0)
					pitch += increment;
				else
				{
					pitch = (t->body_end << 8) - (increment * overflow_tab[overflow++])/4;
					if(overflow > 4)  overflow = 0;
				}
			}
			n_primary--;
		}

		if(stress >= STRESS_PRIMARY)
		{
			pd->syllables[ix].stress = STRESS_PRIMARY_MARKED;
			set_pitch(pd,ix,pitch,drops[stress]);
		}
		else
		if(stress >= STRESS_SECONDARY)
		{
			/* use secondary stress for unmarked word stress, if no annotation */
			set_pitch(pd,ix,pitch,drops[stress]);
		}
		else
		{
			/* unstressed, drop pitch if preceded by PRIMARY */
			if((pd->syllables[ix-1].stress & 0x3f) >= STRESS_SECONDARY)
				set_pitch(pd,ix,pitch - (t->body_lower_u << 8), drops[stress]);
			else
				set_pitch(pd,ix,pitch,drops[stress]);
		}

		ix++;
	}
	return(ix);
}   /* end of calc_pitch_segment */



static struct env_data *calc_pitches(struct pitcher_data *pd)
/********************************************************************/
/* Calculate pitch values for the vowels in this tone group */
{
	int  ix;
	struct tone_table *t;
	int  drop;
	struct env_data *tone_pitch_env;

	t = &pd->inton->tone_table[pd->tone_type];
	ix = 0;

	/* vowels before the first primary stress */
	/******************************************/

	if(pd->number_pre > 0)
	{
		ix = calc_pitch_segment2(pd,ix,ix+pd->number_pre,t->pre_start,t->pre_end);
	}

	/* body of tonic segment */
	/*************************/
	ix=calc_pitch_segment(pd,ix,pd->tone_posn, t, STRESS_PRIMARY);

	if(pd->number_tail == 0)
	{
		tone_pitch_env = t->pitch_env0;
		drop = t->tonic_max0 - t->tonic_min0;
		set_pitch(pd,ix++,t->tonic_min0 << 8,drop << 8);
	}
	else
	{
		tone_pitch_env = t->pitch_env1;
		drop = t->tonic_max1 - t->tonic_min1;
		set_pitch(pd,ix++,t->tonic_min1 << 8,drop << 8);
	}


	/* tail, after the tonic syllable */
	/**********************************/
	
	calc_pitch_segment2(pd,ix,pd->count,t->tail_start,t->tail_end);

	return tone_pitch_env;
}   /* end of calc_pitches */


static void compute_pitches(struct intonator *inton,struct syl_env *syllables,int count,int clause_tone)
{
	struct pitcher_data pd;
	int tonic_ix=-1;
	int tonic_iy=-1;
	struct env_data *tonic_env;
	int ix,max_stress=0;
	static unsigned char punctone[8] = {0,1,3,2,5,4,0,0};
	
	pd.syllables=syllables;
	pd.count=count;
	pd.clause_tone=clause_tone;
	pd.tone_type=punctone[clause_tone];
	pd.inton=inton;
	for (ix=0;ix<count;ix++) if (syllables[ix].stress>=max_stress) {
		max_stress=syllables[ix].stress;
		tonic_iy=tonic_ix;
		tonic_ix=ix;
	}
	if (count > 1 && tonic_ix == count-1) {
		if (pd.tone_type ==2) {
			if (count==2) {
				syllables[0].stress=STRESS_PRIMARY;
				syllables[1].stress=0;
				tonic_ix=0;
			}
			else {
				if (tonic_iy<count-3) tonic_iy=count-2;
				syllables[count-1].stress=STRESS_SECONDARY;
				syllables[tonic_iy].stress=STRESS_PRIMARY;
				tonic_ix=tonic_iy;
			}
		}
	}
	else if (count>=4 && tonic_ix==count-4) {
		syllables[tonic_ix].stress=STRESS_SECONDARY;
		tonic_ix+=2;
		syllables[tonic_ix].stress=STRESS_PRIMARY;
	}

	count_pitch_vowels(&pd);
	tonic_env=calc_pitches(&pd);
	/* nie bedzie singles */
	if (count==1) {
		struct env_data *ev=inton->singles[pd.tone_type];
		if (ev) {
			tonic_env=ev;
			tonic_ix=0;
		}
	}
	if (tonic_ix>=0) {
		syllables[tonic_ix].env=tonic_env;
	}
	/* tu juz mamy pitch1 i pitch2 i env */
	for (ix=0;ix<count;ix++) {
		struct env_data *ev=syllables[ix].env;
		int i;
		double base;
		double range;
		syllables[ix].npitch=ev->count<5?ev->count:5;
		base=inton->vp.base_pitch * syllables[ix].pitch1;
		range=inton->vp.base_pitch * inton->vp.range * (syllables[ix].pitch2-syllables[ix].pitch1);
		if ((syllables[ix].marked & 1) && ix != tonic_ix) {
			range*=2;
			base *=1.05;
		}
		for (i=0;i<ev->count;i++) {
			syllables[ix].offsets[i]=ev->offset[i];
			syllables[ix].pitches[i]=base+range*ev->value[i];
		}
	}
}

