/*
 * milenizer.c - Milena TTS system utilities
 * Copyright (C) Bohdan R. Rau 2008 <ethanak@polip.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write see:
 *               <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>

#include "milenizer.h"

/*

-s split_path
	-S split_char
	-a autonumber
	-A autonumber + fline
		-p,-P prolog
		-e,-E epilog
		-l rozdzial ostatni
-u <len> - unformat
-U <len> - unformat bez pageno
-d - ignoruj dialogi przy unformat
-c <encoding> - kodowanie pliku
-f mode - footnote mode (rtf)
-o path - plik wyjsciowy
-i ignore characters out of range
-r - tylko rtf2txt
*/



char *split_path;
char *output_path;
int splitmode;
int automode;
int unformode;
int autopar;
int rtfdecode;
int make_index;

void help(char *s)
{
	fprintf(stderr,"Sposob uzycia: %s [opcje] <plik>\n",s);
	fprintf(stderr,"\n-- opcje rozformatowania --\n\
  -u <minlen>\n\
  -U <minlen>\n\
     Wlaczenie rozformatowania. Jesli <minlen> wynosi 0, uznaje sie.\n\
     ze akapity sa oddzielone pusta linia. W przeciwnym przypadku\n\
     akapitem konczacym jest akapit o dlugosci nie wiekszej niz <minlen>.\n\
     Wyklucza stosowanie -X.\n\
     Opcja -U powoduje dodatkowo ignorowanie numeracji stron (tzn. linii\n\
     zawierajacych wylacznie cyfry oraz nastepujacych po nich pustych).\n\
  -d\n\
     nie uznaje linii zaczynajacych sie od '-' za dialogi.\n\n\
-- opcje podzialu na rozdzialy --\n\n\
  -s <sciezka>\n\
     wlacza tryb podzialu na pliki rozdzialow. W tym trybie pierwsza linia\n\
     traktowana jest jako linia tytulowa i rozpoczyna kazdy plik. Jesli nie\n\
     podano zadnej z opcji -a/-A, pierwsza linia kazdego rozdzialu jest\n\
     traktowana jako tytulowa rozdzialu i oddzielona kropka od tytulu\n\
     ksiazki. Po linii tytulowej zawsze wystepuje pusty wiersz. Kazdy plik\n\
     konczony jest oddzielonym pustym wierszem zdaniem \"koniec rozdzialu\"\n\
     lub (dla ostatniego) \"koniec ksiazki\". Jesli podano opcje -p/-P,\n\
     prolog konczony jest zdaniem \"koniec prologu\".\n\
     Kolejne pliki maja postac <sciezka>_<numer>.txt, gdzie <numer> jest\n\
     dwu- lub trzycyfrowa liczba oznaczajaca kolejny plik.\n\
  -S <separator>\n\
     Podaje znak separujacy rozdzialy (domyslnie '#'). Znak ten powinien byc\n\
     pierwszym widocznym znakiem linii. Wszelkie biale znaki za separatorem\n\
     sa ignorowane.\n\
  -T <rozmiar> - zmienia dzialanie trybu podzialu. Tekst dzielony jest na\n\
     w miare rowne fragmenty wielkozci podanego rozmiaru (w kilobajtach)\n\
     bez zadnej ingerencji w tresc.\n\
-- opcje autonumeracji rozdzialow --\n\n\
  -a\n\
     wlacza autonumeracje rozdzialow\n\
  -A\n\
     wlacza autonumeracje, dodatkowo pierwsza linia rozdzialu traktowana\n\
     jest jako tytul rozdzialu.\n\
  -l\n\
     wyroznia ostatni rozdzial\n\
  -p, -P\n\
     wyroznia prolog. Jesli podano -P pierwsza linia traktowana jest jako\n\
     tytul prologu.\n\
  -e, -E\n\
     wyroznia epilog. Jesli podano -E pierwsza linia traktowana jest jako\n\
     tytul epilogu.\n\
  Opcje -p/-P i -e/-E dzialaja niezaleznie od tego czy podano -a czy -A.\n\
  Wystapienie ktorejkolwiek z opcji -p/-P/-e/-E/-l bez opcji -a/-A jest\n\
  nielegalne. Wszystkie opcje autonumeracji sa ignorowane, jesli nie podano\n\
  opcji -s lub podano -T.\n\n\
-- opcje konwersji --\n\n\
  -c <kodowanie>\n\
     podaje kodowanie pliku wejsciowego. Jesli nie podano tej opcji,\n\
     program probuje wydedukowac kodowanie na podstawie zawartosci pliku.\n\
     Jesli program skompilowano z biblioteka enca, bedzie ona uzyta do\n\
     okreslenia kodowania. W przeciwnym przypadku dopuszczalne sa tylko\n\
     trzy wejsciowe kodowania (UTF-8, ISO-8859-2, CP_1250).\n\
  -r\n\
     powoduje ze program dokona wylacznie konwersji z formatu RTF/PDF/DOC\n\
     na tekst w kodowaniu UTF-8 (przydatne przy recznej obrobce plikow).\n\
     Podanie tej opcji spowoduje, ze wszystkie opcje oprocz -o, -i, -X, -Y\n\
     oraz -f beda ignorowane.\n\
  -i\n\
     powoduje, ze przy konwersji z UTF-8 znaki spoza dopuszczalnego zakresu\n\
     beda ignorowane. Nie dotyczy to znakow o kodach wyzszych niz \\uFFFF.\n\n\
  -X\n\
     wymusza uzycie pdftohtml zamiast pdftotext. Nie sprawdza sie\n\
     we wszystkich przypadkach. Niemozliwe do stosowania wraz z -u/-U.\n\
  -Y\n\
     jak wyzej, ale ustawia dodatkowo opcje -nodrm. Nie kazda wersja\n\
     pdftohtml obsluguje ten parametr.\n\
-- pozostale opcje --\n\n\
  -h\n\
     wypisuje tresc pomocy\n\
  -x - tworzy plik indeksu dla milena_nokia. Ignorowany bez -s\n\
  -o <nazwa_pliku>\n\
     program zapisze wynik konwersji do podanego pliku (domyslnie program\n\
     wypisuje wynik na standardowe wyjscie).\n\
  -f <tryb>\n\
     okresla tryb traktowania przypisow (footnotes) przy konwersji plikow\n\
     w formacie RTF. Dopuszczalne sa nastepujace tryby:\n\
   d - ignorowanie przypisow (domyslnie);\n\
   n - przypisy w postaci czytelnej na ekranie. Kazdy przypis w tresci\n\
       oznaczany jest [n] (gdzie <n> to numer przypisu). Po akapicie\n\
       wypisywana jest linia \"---\", a po niej wszystkie przypisy wystepujace\n\
       w akapicie.\n\
   e - jak wyzej, ale wszystkie przypisy sa grupowane i wypisane na koncu.\n\
   a - podobnie jak 'n', lecz przypisy wypisane sa w formie czytelnej dla\n\
       syntezatora mowy.\n\
   i - przypisy w postaci czytelnej dla syntezatora mowy beda wypisane\n\
       bezposrednio w miejscu wystapienia przypisu w tresci.\n\
  -j <parametry>\n\
     oddzielone przecinkami parametry parsera PDF:\n\
   p - wstaw numerację stron\n\
   s - pokaż statystykę wysokosci wierszy\n\
   l=<liczba> - maksymalna wysokosc linii w obrebie akapitu\n\
                (domyslnie oszacowana automatycznie)\n\
   a=<liczba> - maksymalna wysokosc linii rozpoczynajacej akapit\n\
		(domyslnie 1.5 wartości l)\n\
   f=<liczba> - minimalna wysokosc ignorowanej czcionki (domyslnie 60)\n\
   b=<liczba> - minimalna wysokosc ignorowanego bloku (domyslnie 100)\n"
   
);
	exit(0);
}

char *body;
int ignore_oor;
int timed_split=0;

int parse_pdf_param(char *c)
{
    int znak,param;
    int getparam(void) {
	if (*c++ == '=') {
	    if (c && isdigit(*c)) {
		param=strtol(c,&c,10);
		return 1;
	    }
	}
	fprintf(stderr,"Bledny parametr parsera PDF: %s\n",c);
	return 0;
    }
    while (c && *c) {
	znak=*c++;
	switch(znak) {
	    case 'p':
	    pdf_autonumber_pages=1;
	    break;
	    
	    case 's':
	    pdf_show_heights=1;
	    break;
	    
	    case 'l':
	    if (!getparam()) return 0;
	    pdf_max_line_height=param;
	    break;
	    
	    case 'a':
	    if (!getparam()) return 0;
	    pdf_max_paragraph_height=param;
	    break;

	    case 'f':
	    if (!getparam()) return 0;
	    pdf_ignore_font_size=param;
	    break;
	    
	    case 'b':
	    if (!getparam()) return 0;
	    pdf_ignore_chunk_height=param;
	    break;
	    
	    default:
	    fprintf(stderr,"Bledny parametr parsera PDF: %s\n",c-1);
	    return 0;
	}
	if (*c==',') c++;
    }
    return 1;
}


int main(int argc,char *argv[])
{
	char *encoding=NULL;
	char *fname;
	for(;;) {
		int c=getopt(argc,argv,"u:U:s:S:aApPeEPdc:f:ho:ilrIxXYT:j:");
		if (c<0) break;
		switch(c) {
			case 'j':
			if (parse_pdf_param(optarg)) continue;
			help(argv[0]);
			case 'T':
			timed_split=strtol(optarg,&optarg,10);
			if (*optarg || timed_split < 5 || timed_split > 20) help(argv[0]);
			continue;
			case 'x': make_index=1;
			continue;
			case 'U':
			unformode |= UNFORMODE_IGNORE_PAGENO;
			case 'u':
			if (pdfmode) help(argv[0]);
			unformode |= UNFORMODE_ON;
			if (!isdigit(*optarg)) help(argv[0]);
			autopar=strtol(optarg,&optarg,10);
			if (*optarg) help(argv[0]);
			continue;
			
			case 'Y':
			nodrm=1;
			case 'X':
			if (unformode) help(argv[0]);
			pdfmode=1;
			continue;
			case 'd':
			unformode |= UNFORMODE_NO_DIALOG;
			continue;
			
			case 'A':
			automode |= AUMODE_KEEP_LINE;
			case 'a':
			automode |= AUMODE_ON;
			continue;
			
			case 's': split_path=optarg;
			continue;
			
			case 'S': if (optarg[1]) help(argv[0]);
			splitmode=*optarg;
			continue;
			
			case 'E': automode |= AUMODE_EPILOG_KEEP;
			case 'e': automode |= AUMODE_EPILOG;continue;
			
			case 'P': automode |= AUMODE_PROLOG_KEEP;
			case 'p': automode |= AUMODE_PROLOG;continue;
			
			case 'l': automode |= AUMODE_LASTCHAPTER;continue;
			
			case 'c': encoding=optarg;continue;
			
			case 'o': output_path=optarg;continue;
			
			case 'i': ignore_oor=1;continue;
			case 'I': ignore_oor=2;continue;
			
			case 'f' : if (optarg[1]) help(argv[0]);
			if (!setFootNote(optarg)) help(argv[0]);
			continue;
			
			case 'r': rtfdecode=1;continue;
			
			default: help(argv[0]);
		}
	}
	if (!split_path) {
	    if (timed_split) {
		help(argv[0]);
	    }
	    make_index=0;
	}
	if (rtfdecode) {
		unformode=0;
		split_path=NULL;
	}
	if (split_path && !splitmode) splitmode='#';
	if (automode && !(automode & 1)) help(argv[0]);
	if (optind != argc-1) help(argv[0]);
	fname=argv[optind];
	body=read_file(fname,encoding);
	if (unformode & UNFORMODE_ON) unformat(body);
	if (!split_path) {
		FILE *f;
		if (output_path) {
			f=fopen(output_path,"w");
			if (!f) {
				perror(output_path);
				exit(1);
			}
		}
		else f=stdout;
		fputs(body,f);
		if (output_path) fclose(f);
		exit(0);
	}
	if (timed_split) {
	    split_timed(body,timed_split);
	    exit(0);
	}
	split_body(body);
	
}


