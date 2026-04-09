/*
 * main.c - main file for Milena TTS system
 * Copyright (C) Bohdan R. Rau 2008-2011 <ethanak@polip.com>
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include "milena.h"
#include "milena_mbrola.h"
#include "input.h"
#ifndef __WIN32
#include "serwer.h"
#endif

struct milena *milena;
struct milena_mbrola_cfg *mbrola;
int daisy,new_ellipsis,input_utf;
int disable_morfologik=0;

int debug=0,phronly=0,is_ivo=0,is_rhv=0,ivr_nopause=0;
char obuf[8192];
char ibuf[8192];
char ixbuf[8192];
char *ivo_voice=NULL;
char *ivo_buf=NULL;
int ivo_length=0;
char *rh_buf=NULL;
int rh_length=0;
int bookmode=0;

void translate_phrase(char *phrase,char **otbuf,int *blen)
{
	static char *buf1,*buf2;
	static int len1,len2;
	int n;
	if (!len1) {
		buf1=malloc(len1=8192);
	}
	if (!len2) {
		buf2=malloc(len2=8192);
	}
	n=milena_Prestresser(milena,phrase,buf1,len1);
	if (n) {
		len1=n+1024;
		buf1=realloc(buf1,len1);
		milena_Prestresser(milena,phrase,buf1,len1);
	}
	n=milena_TranslatePhrase(milena,(unsigned char *)buf1,buf2,len2,0);
	if (n) {
		len2=n+1024;
		buf2=realloc(buf2,len2);
		milena_TranslatePhrase(milena,(unsigned char *)buf1,buf2,len2,0);
	}
	n=milena_Poststresser(buf2,*otbuf,*blen);
	if (n) {
		*blen = n+1024;
		*otbuf=realloc(*otbuf,*blen);
		
	}
	milena_Poststresser(buf2,*otbuf,*blen);
}

int get_stressed_syllable(char *str)
{
	int syl=1;
	for (;*str;str++) {
		if (*str=='~') {
			str++;
			if (!*str) return 0;
			if (*str=='!') return syl;
			continue;
		}
		if (strchr("aeiouyYMOE",*str)) syl++;
	}
	return 0;
}

int count_syllables(char *str)
{
	int syl=0;
	for (;*str;str++) {
		if (strchr("aeiouyYMOE",*str)) syl++;
	}
	return syl;
}


void translate_par(char *par,FILE *f)
{
	static char *phrase_buf,*sentence_buf,*second_buf;
	static int phrase_len,sentence_len,second_len;
	int this_ptyp,next_ptyp;
	int have_phrase=0;
	int get_phrase()
	{
		int nlen;
		char *opar=par;
		if (have_phrase) {
			char *c=sentence_buf;
			int n=sentence_len;
			sentence_buf=second_buf;
			second_buf=c;
			sentence_len=second_len;
			second_len=n;
			have_phrase=0;
			this_ptyp=next_ptyp;
			return 1;
		}
		
		nlen=milena_GetPhrase(milena,&par,phrase_buf,phrase_len,&this_ptyp);
		if (nlen<0) return 0;
		if (nlen) {
			phrase_len=nlen+1024;
			phrase_buf=realloc(phrase_buf,phrase_len);
			milena_GetPhrase(milena,&par,phrase_buf,phrase_len,&this_ptyp);
		}
		translate_phrase(phrase_buf,&sentence_buf,&sentence_len);
		if (daisy) {
			while (opar < par && isspace(*opar)) opar++;
			fprintf(f,";PHR %d %-*.*s\n",this_ptyp,(int)(par-opar),(int)(par-opar),opar);
		}
		return 1;
	}
	
	int get_next_phrase()
	{
		int nlen;
		nlen=milena_GetPhrase(milena,&par,phrase_buf,phrase_len,&next_ptyp);
		if (nlen<0) return 0;
		if (nlen) {
			phrase_len=nlen+1024;
			phrase_buf=realloc(phrase_buf,phrase_len);
			milena_GetPhrase(milena,&par,phrase_buf,phrase_len,&next_ptyp);
		}
		translate_phrase(phrase_buf,&second_buf,&second_len);
		have_phrase=1;
		return 1;
		
	}
	if (!phrase_len) {
		phrase_buf=malloc(phrase_len=8192);
		sentence_buf=malloc(sentence_len=8192);
		second_buf=malloc(second_len=8192);
	}
	for (;;) {
		if (!get_phrase()) return;
		if (!daisy && (this_ptyp & 7) == 1) {
			if (!get_next_phrase()) this_ptyp=(this_ptyp & 0xf8) | 5;
			else {
				//int csy=get_stressed_syllable(second_buf);
				int b=0;
				if ((next_ptyp & 7) == 2) {
					if (count_syllables(second_buf)<6) b=1;
				}
				//if (!b && csy>3) b=1;
				//if (b) this_ptyp=(this_ptyp & 0xf8) | 4;
				if (b) this_ptyp |= 128;
			}
			
		}
		if (!new_ellipsis && (this_ptyp & 7)==7) {
			this_ptyp=(this_ptyp & 0xf8) | 4;
		}
		milena_ModMbrolaGenPhrase(mbrola,sentence_buf,f,this_ptyp);
	}
}

void translate_simple_par(char *par, FILE *fout)
{
	int ptype;
	for (;;) {
		int n;
		n=milena_GetPhrase(milena,&par,ibuf,8192,&ptype);
		if (n) break;
		if (!bookmode && (ptype & 7)==7) ptype=(ptype & 0xf8) | 4;
		if (debug || phronly) fprintf(stderr,"%d: %s\n",ptype,ibuf);
		if (phronly) continue;
		milena_Prestresser(milena,ibuf,obuf,8192);
		if (debug) fprintf(stderr,"%s\n",obuf);
		milena_TranslatePhrase(milena,(unsigned char *)obuf,ibuf,8192,debug);
		if (debug) fprintf(stderr,"%s\n",ibuf);
		if (is_ivo) {
            n=milena_ivonizer_nb(ptype,ibuf,ivo_buf,ivo_length,milena, bookmode);
			if (n) {
				ivo_length=2*n+256;
				if (ivo_buf) ivo_buf=realloc(ivo_buf,ivo_length);
				else {
					ivo_buf=malloc(ivo_length);
					*ivo_buf=0;
				}
				milena_ivonizer_nb(ptype,ibuf,ivo_buf,ivo_length,milena,bookmode);
			}
            continue;
		}
		milena_Poststresser(ibuf,obuf,8192);
		if (debug) fprintf(stderr,"%s\n",obuf);
		if (!(debug & 2)) milena_ModMbrolaGenPhrase(mbrola,obuf,fout,ptype);
	}
}



static char *DESCR_STRING="Milena - translator tekstu polskiego na fonemy Mbroli\n\
Wersja: "MILENA_VERSION"\n\
(C) 2008-2023 Bohdan R. Rau <ethanak@polip.com>\n\n";

void help(char *name)
{
	fprintf(stderr,"%s",DESCR_STRING);
	fprintf(stderr,"Sposob uzycia:\n\t%s [opcje]\n\n",name);
	fprintf(stderr," -h	help\n");
#ifdef __WIN32
	fprintf(stderr," -x	Ivona debug (musi byc jako pierwszy)\n");
    fprintf(stderr," -G\tRHVoice debug (musi byc jako pierwszy)\n");
#else
	fprintf(stderr," -C	Klient/serwer (musi byc jako pierwszy)\n");
	fprintf(stderr," -T n   Limit czasu oczekiwania na serwer (w sekundach)\n");
	fprintf(stderr," -x	Ivona debug (musi byc jako pierwszy za 'C')\n");
    fprintf(stderr," -G\tRHVoice debug (musi byc jako pierwszy za 'C')\n");
	
#endif
	fprintf(stderr," -g xx\tglos Ivony (wymaga -x)\n");
    fprintf(stderr," -Y\twylacza pokazywanie pauzy dla Ivony/RHVoice\n");
    fprintf(stderr," -v	verbose, pokazuje etapy posrednie\n	i zastosowane reguly translacji\n");
	fprintf(stderr," -V	jak -v, wylacza generowanie fonemow dla mbroli\n");
	fprintf(stderr," -p	phraser, wylacznie etap frazera\n	do testow regul rozpoznawania i slownika\n");
	fprintf(stderr," -l	lektor, uwzglednia dialogi i puste akapity\n");
	fprintf(stderr," -d	Uwzglednia dodatkowe informacje dla daisy. Implikuje -l\n");
	fprintf(stderr," -D n   Limit cyfr w liczbie czytanej jako int (2..9)\n");
	fprintf(stderr," -i	frazer ignoruje informacje dodatkowe w {} i []\n");
	fprintf(stderr," -I	frazer ignoruje akcenty i separatory\n");
	fprintf(stderr," -E	odczytuje akapity nie zawierajace liter ani cyfr\n");
	fprintf(stderr," -e	odczytuje emoticony\n");
	fprintf(stderr," -P	frazer odczytuje wiekszosc znakow nieliterowych\n\t(implikuje -i -I -E)\n");
	fprintf(stderr," -H	frazer odczytuje godziny\n");
	fprintf(stderr," -L xx	frazer wczytuje plik jezykowy\n");
	fprintf(stderr," -u xx	wczytuje dodatkowy plik slownika\n");
	fprintf(stderr," -f xx	wczytuje dodatkowy plik frazera\n");
	fprintf(stderr," -t xx	wczytuje temat\n");
	fprintf(stderr," -m xx	wczytuje dodatkowy plik modulu Mbroli\n");
	fprintf(stderr," -M	wczytuje plik pro_mbrola modulu Mbroli\n");
	fprintf(stderr," -Q	korekta dlugosci samoglosek\n");
	fprintf(stderr," -s n	uwydatnienie akcentu (0..50)\n");
	fprintf(stderr," -z	alternatywna interpretacja wielokropka\n");
	fprintf(stderr," -R	tryb screenreadera\n");
	fprintf(stderr," -U	tekst wejsciowy jest w UTF-8\n");
	fprintf(stderr," -Z	uwzglednia dodatkowe informacje dla ust\n");
#ifdef HAVE_MORFOLOGIK
#if HAVE_MORFOLOGIK == 10
	fprintf(stderr," -S	Uzyj Morfologika poza trybem lektora\n");
	fprintf(stderr," -X     Nie uzywaj Morfologika (ignoruje -S)\n");
#else

	fprintf(stderr," -S	Uzyj Morfologika SHM poza trybem lektora\n");
#ifdef __WIN32
	fprintf(stderr," -X     Nie uzywaj Morfologika (ignoruje -S)\n");
#else
	fprintf(stderr," -b	Uruchom serwer Morfologika SHM jesli nie istnieje\n\t(implikuje -S)\n");
	fprintf(stderr," -B xx	Uruchom serwer Morfologika SHM z podanej sciezki jesli nie istnieje\n\t(implikuje -S)\n");
	fprintf(stderr," -k     Zamknij serwer Morfologika SHM i wyjdz\n");
	fprintf(stderr," -X     Nie uzywaj Morfologika (ignoruje -S, -b i -B)\n");
#endif
#endif
#endif

	fprintf(stderr,"Opcje -u, -m, -L, -t oraz -f moga wystepowac wielokrotnie.\n\n");
#ifdef HAVE_MORFOLOGIK
#ifndef __WIN32
	fprintf(stderr,"Uzycie Morfologika w trybie Shared Memory moze wymagac\nzmiany w /etc/sysctl.conf:\n\
kernel.shmmax = 167772160\n");
#endif
#endif
	exit(1);
	
}

static int file_exists(char *path)
{
	struct stat sb;
	return !stat(path,&sb);
}
void read_home_udic(void)
{
#ifndef __WIN32
	char path[256];
	struct stat sb;
	sprintf(path,"%s/.milena_pl_userdic.dat",getenv("HOME"));
	if (stat(path,&sb)) return;
	if (!milena_ReadUserDicWithFlags(milena,path,MILENA_UDIC_IGNORE)) exit(0);
#endif
}

int main(int argc,char *argv[])
{
	char *psf;
	int phmode=0,stresext=-1;
	int partyp,lastpar=-1,pau,spell_empty=0;
	void *inputctl;
	int got_home_udic=0;
	int morfol=0;
	int morfol_loaded=0;
	char *morfol_path=NULL;
	int run_morfol=0;
	void *morfol_data=NULL;
	debug=0;

	char *iso_buf=NULL;
	int iso_size=0;
	int iam_server=0;
	int iam_client=0;
	int had_opt=0;
	int server_timeout=0;
	
	FILE *fout;
	char *output_buffer;
	size_t output_size;
	
	void init_mil(void)
	{
		if (milena) return;
		milena=milena_Init(
			milena_FilePath((is_ivo?"ive_pho.dat":"pl_pho.dat"),obuf),
			milena_FilePath("pl_dict.dat",ibuf),
			milena_FilePath("pl_stress.dat",ixbuf));
		if (!milena) exit(1);
		if (!milena_ReadPhraser(milena,
			milena_FilePath("pl_phraser.dat",ibuf))) exit(1);
		if (!is_ivo) {
			mbrola=milena_InitModMbrola(
				milena_FilePath("pl_mbrola.dat",ibuf));
			if (!mbrola) exit(1);
		}
		if (!milena_ReadUserDic(milena,
			milena_FilePath("pl_udict.dat",ibuf))) exit(1);
		if (is_ivo) {
		    milena_ReadUserDic(milena,
			milena_FilePath("ive_udict.dat",ibuf));
		    milena_ReadIvonaFin(milena,
			milena_FilePath("ive_fin.dat",ibuf));
		}
		if (!is_ivo && !milena_ReadInton(mbrola,
			milena_FilePath("pl_intona.dat",ibuf))) exit(1);
	}
	
	for(;;) {
		int c=getopt(argc,argv,"vVu:f:ldphiIm:PHL:EeQZs:t:zRxUMD:g:GY"
#ifndef __WIN32
		"CT:"
#endif
#ifdef HAVE_MORFOLOGIK
#if HAVE_MORFOLOGIK == 10
		"SX"
#else
#ifdef __WIN32
			"SX"
#else
			"SbB:kX"
#endif
#endif
#endif
		);
#ifndef __WIN32
		if (c=='C') {
			if (had_opt) {
				help(argv[0]);
			}
			if (!have_server()) {
				int pid=fork();
				if (pid < 0) {
					perror("fork");
					exit(1);
				}
				if (pid) iam_client=1;
				else {
                                        daemonize_me();
                                        iam_server=1;
                                }
			}
			else {
				iam_client=1;
			}
			had_opt=1;
			continue;
		}
		had_opt=1;
		if (c=='T') {
			server_timeout=10*strtol(optarg,NULL,10);
			continue;
		}
#endif				
					
		if (c=='x') {
			if (milena) help(argv[0]);
			is_ivo=1;
		}
        if (c=='G') {
            if (milena) help(argv[0]);
            is_ivo=1;
            is_rhv=1;
        }
            
		if (!iam_client) init_mil();
		if (c=='x' || c == 'G') {
			continue;
		}
		if (c<0) break;
		if (c=='v') {
			if (!iam_client && !iam_server) {
				debug|=1;
			}
			continue;
		}
		if (c=='V') {
			if (!iam_client && !iam_server) {
				debug|=2;
			}
			continue;
		}
		if (c=='E') {
			spell_empty=1;
			continue;
		}
		if (c=='z') {
			new_ellipsis=1;
			continue;
		}
		if (c=='Q') {
			if (!iam_client && !is_ivo) milena_ModMbrolaSetFlag(
				mbrola,-1,MILENA_MBFL_EQUALIZE);
			continue;
		}
		if (c=='Z') {
			if (!iam_client && !is_ivo) milena_ModMbrolaSetFlag(
				mbrola,-1,MILENA_MBFL_MOUTH);
			continue;
		}
		if (c=='i') {
			phmode |= MILENA_PHR_IGNORE_INFO;
			continue;
		}
		if (c=='I') {
			phmode |= MILENA_PHR_IGNORE_TILDE;
			continue;
		}
		
		if (c=='u') {
			if (iam_client) continue;
			if (!got_home_udic) {
				read_home_udic();
				got_home_udic=1;
			}
			if (!milena_ReadUserDicWithFlags(milena,optarg,MILENA_UDIC_IGNORE)) exit(0);
			continue;
		}
		if (c=='f') {
			if (iam_client) continue;
			if (!got_home_udic) {
				read_home_udic();
				got_home_udic=1;
			}
			if (!milena_ReadPhraser(milena,optarg)) exit(0);
			continue;
		}
		if (c=='g') {
		    if (!is_ivo || is_rhv) {
			help(argv[0]);
		    }
		    ivo_voice=strdup(optarg);
		    continue;
		}
		if (c=='D') {
			if (iam_client) continue;
			
		    if (milena_DigitsLimit(milena,strtol(optarg,NULL,10))) help(argv[0]);
		    continue;
		}
		if (c=='M') {
			if (iam_client) continue;
			if (!is_ivo && !milena_ModMbrolaExtras(mbrola,
			    milena_FilePath("pl_pro_mbrola.dat",ibuf))) exit(0);
			continue;
		}
		    
		if (c=='m') {
			if (iam_client) continue;
			
			if (!is_ivo && !milena_ModMbrolaExtras(mbrola,optarg)) exit(1);
			continue;
		}
		if (c=='l') {
			if (iam_client) continue;
			bookmode=1;
			continue;
		}
#ifdef HAVE_MORFOLOGIK
		if (c=='X') {
			if (iam_client) continue;
			disable_morfologik=1;
			continue;
		}
		if (c=='S') {
			if (iam_client) continue;
			morfol=1;
			continue;
		}
#ifndef __WIN32
#if HAVE_MORFOLOGIK == 20
		if (c=='b') {
			if (iam_client) continue;
			
			morfol=1;
			run_morfol=1;
			morfol_path=NULL;
			continue;
		}
		if (c=='B') {
			if (iam_client) continue;
			
			morfol=1;
			run_morfol=1;
			morfol_path=strdup(optarg);
			continue;
		}

		if (c=='k') {
			if (iam_client || iam_server) continue;
			milena_DestroyMorfologik();
			exit(0);
		}
#endif
#endif
#endif
		if (c=='d') {
			if (iam_client) continue;
			
			bookmode=daisy=1;
			continue;
		}
		if (c=='p') {
			if (iam_client || iam_server) continue;
			
			phronly=1;
			continue;
		}
		if (c=='P') {
			if (iam_client) continue;
			
			phmode|=4;
			spell_empty=1;
			continue;
		}
		if (c=='H') {
			if (iam_client) continue;
			
			if (!milena_ReadPhraser(milena,
				milena_FilePath("pl_hours.dat",ibuf))) exit(1);
			continue;
		}
		if (c=='e') {
			if (iam_client) continue;
			
			if (!milena_ReadPhraser(milena,
				milena_FilePath("pl_emots.dat",ibuf))) exit(1);
			continue;
		}
		if (c=='s') {
			if (iam_client) continue;
			
			stresext=strtol(optarg,NULL,10);
			continue;
		}
		if (c=='L') {
			if (iam_client) continue;
			
			sprintf(obuf,"pl_%s_udic.dat",optarg);
			milena_FilePath(obuf,ibuf);
			if (file_exists(ibuf)) {
				if (!milena_ReadUserDic(milena,ibuf)) exit(1);
			}
			sprintf(obuf,"pl_%s_stress.dat",optarg);
			milena_FilePath(obuf,ibuf);
			if (file_exists(ibuf)) {
				if (!milena_ReadStressFile(milena,ibuf)) exit(1);
			}
			milena_SetLangMode(milena,optarg);
			continue;
		}
		if (c=='t') {
			
			if (iam_client) continue;
			sprintf(obuf,"pl_%s_theme.dat",optarg);
			if (!milena_ReadPhraser(milena,
				milena_FilePath(obuf,ibuf))) exit(1);
			continue;
		}
		if (c=='R') {
			if (iam_client) continue;
			
			if (!milena_ReadKeyChar(milena,milena_FilePath("pl_keys.dat",ibuf))) exit(1);
			continue;
		}
		
		if (c == 'U') {
			if (iam_server) continue;
			input_utf=1;
			continue;
		}
        if (c == 'Y') {
            ivr_nopause =1;
            continue;
        }
		help(argv[0]);
	}
	if (!iam_client) {
		if (is_ivo && ivo_voice) {
		    char *c;
		    for (c=ivo_voice;*c;c++) *c=tolower(*c);
		    sprintf(obuf,"iv_%s_udic.dat",ivo_voice);
		    milena_FilePath(obuf,ibuf);
		    if (file_exists(ibuf)) milena_ReadUserDic(milena,ibuf);
		    sprintf(obuf,"iv_%s_fin.dat",ivo_voice);
		    milena_FilePath(obuf,ibuf);
		    if (file_exists(ibuf)) milena_ReadIvonaFin(milena,ibuf);
		}
	
	
		if (!got_home_udic) {
			read_home_udic();
			got_home_udic=1;
		}
		if (phmode & 4) {
			if (!milena_ReadPhraser(milena,milena_FilePath("pl_charspell.dat",ibuf))) exit(0);
		}
		else if (phmode & 3) milena_SetPhraserMode(milena,phmode & 3);
		
		if (stresext>=0) {
			if (!is_ivo) milena_ModMbrolaSetVoice(mbrola,
				MILENA_MBP_STRESLEN,stresext);
		}
#ifdef HAVE_MORFOLOGIK
#ifndef __WIN32
#if HAVE_MORFOLOGIK == 20
		if (run_morfol && !disable_morfologik) {
			morfol_data=milena_StartMorfologik(morfol_path);
			if (!morfol_data) {
				perror("Morfologik");
				exit(1);
			}
		}
#endif
#endif
#endif
		if (bookmode) {
			if (!is_ivo) milena_ModMbrolaSetFlag(
					mbrola,-1,MILENA_MBFL_BOOKMODE);
#ifdef HAVE_MORFOLOGIK
			if (disable_morfologik || !(morfol_loaded=milena_SetMorfologik(milena,morfol_data)))
#endif
			milena_ReadVerbs(milena,
					milena_FilePath("pl_verbs.dat",ibuf));
		}
#ifdef HAVE_MORFOLOGIK
		else if (morfol && !disable_morfologik) {
			if (!morfol_data) {
				morfol_data=milena_GetMorfologik(NULL,1);
			}
			if (morfol_data) {
				morfol_loaded=1;
				milena_SetMorfologik(milena,morfol_data);
				if (!is_ivo) milena_ModMbrolaSetFlag(
					mbrola,-1,MILENA_MBFL_BOOKMODE);
			}
		}
#endif
	}
#ifdef __WIN32
	inputctl=alloc_input_buffer();
#else
	if (!iam_server) {
		inputctl=alloc_input_buffer();
		if (iam_client) {
			init_client_server(server_timeout);
		}
	}
	else {
		start_server();
	}
#endif
	if (!iam_client && is_ivo) {
		ivo_buf=malloc(ivo_length=1024);
		*ivo_buf=0;
        if (is_rhv) {
            rh_buf=malloc(rh_length=1024);
        }
	}
	for (;;) {
#ifndef __WIN32
		if (iam_server) {
			psf=get_from_client(&lastpar);
			if (!psf) continue;
			fout=open_memstream(&output_buffer,&output_size);
		}
		else {
#endif
			fout=stdout;
			psf=fgets_ra(inputctl,stdin);
			if (!psf) break;
			if (input_utf && *psf) {
				int n=milena_utf2iso(psf,NULL,1,NULL);
				if (n) {
					if (!iso_buf) {
						iso_size=n  + 8192;
						iso_buf=malloc(iso_size);
					}
					else if (iso_size <= n) {
						iso_size=n  + 8192;
						iso_buf=realloc(iso_buf,iso_size);
					}
					milena_utf2iso(psf,iso_buf,1,NULL);
					psf=iso_buf;
				}
			}
#ifndef __WIN32

			if (iam_client) {
				char *buf=get_from_server(&lastpar,psf);
				fwrite(buf,strlen(buf),1,stdout);
				fflush(stdout);
				free(buf);
				continue;
			}
		}
#endif		
		partyp=milena_GetParType(milena,&psf,spell_empty);
		if (!partyp) {
			if (lastpar>0) lastpar=0;
			goto end_loop;
		}
		if (lastpar<0) {
			pau=0;
		}
		else {
			if (!bookmode) pau=MILENA_MBRK_NORMAL;
			else {
				if (lastpar==0) pau=MILENA_MBRK_LONG;
				else if (lastpar == partyp) pau=MILENA_MBRK_NORMAL;
				else if (partyp == MILENA_PARTYPE_DIALOG) pau=MILENA_MBRK_PREDIAL;
				else pau=MILENA_MBRK_POSTDIAL;
			}
		}
		lastpar=partyp;
		if (pau && !phronly) {
			if (!is_ivo) milena_ModMbrolaBreak(mbrola,fout,pau);
			if (daisy) fprintf(fout,";PAR %d\n",partyp);
		}
		if (debug || !bookmode || is_ivo) {
			translate_simple_par(psf,fout);
		}
		else	{
			translate_par(psf,fout);
		}
		if (is_ivo && ivo_buf) {
            if (is_rhv) {
                milena_ivo2rh(ivo_buf);
                int nl=milena_i2uf(ivo_buf, rh_buf, rh_length);
                if (nl) {
                    rh_buf=realloc(rh_buf, rh_length = 2*nl+256);
                    milena_i2uf(ivo_buf, rh_buf, rh_length);
                }
                if (ivr_nopause) 
                    fprintf(fout,"%s\n",rh_buf);
                else
                    fprintf(fout,"%d %s\n",pau,rh_buf);
            }
            else {
                if (ivr_nopause) 
                    fprintf(fout,"%s\n",ivo_buf);
                else
                    fprintf(fout,"%d %s\n",pau,ivo_buf);
            }
			*ivo_buf=0;
		}
end_loop:
#ifndef __WIN32
		if (iam_server) {
			fclose(fout);
			return_to_client(lastpar,output_buffer,output_size);
			free(output_buffer);
		}
#else
        ;
#endif
	}
	if (!iam_server && !phronly && !debug && !is_ivo) fprintf(stdout,"_ 2\n");
	if (!iam_client) {
		milena_Close(milena);
		if (!is_ivo) milena_CloseModMbrola(mbrola);
	}
	return 0;
}
