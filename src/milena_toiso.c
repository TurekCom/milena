/*
 * milena_toiso.c - Milena TTS system (charset converter)
 * Copyright (C) Bohdan R. Rau 2010 <ethanak@polip.com>
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

static int uni_isspace(int znak)
{
	int i;
	static int spacje[]={0x1680,0x180e,0x2028,0x2029,0x205f,0x3000,0};
	if (znak<=0x20) return 1;
	if (znak>=0x7f && znak <=0xa0) return 1;
	if (znak>=0x2000 && znak <=0x200b) return 1;
	for (i=0;spacje[i];i++) if (znak==spacje[i]) return 1;
	return 0;
}

//#define pushout(n) do {if (outstr) outstr[pos]=n;pos++;} while (0)
#define pushout(n) do {if (outstr) outstr[pos]=n;pos++;if (instrpos && pos >=maxpos) {*instrpos=instr;return pos;}} while (0)

static int get_unichar(char **str)
{
	int znak,n,m;
	if (!*str) return 0;
	znak=(*(*str)++) & 255;
	if (!(znak & 0x80)) return znak;
	if ((znak & 0xe0)==0xc0) n=1;
	else if ((znak & 0xf0)==0xe0) n=2;
	else {
#ifdef __WIN32
		MessageBox(NULL,"Znaki powyzej \\uFFFF nie sa obslugiwane",NULL,MB_OK | MB_ICONERROR);
#else
		fprintf(stderr,"Znaki powyzej \\uFFFF nie sa obslugiwane\n");
#endif
		return 0;
	}
	znak &= 0x1f;
    while (n--) {
		m=*(*str)++ & 255;
		if ((m & 0xc0)!=0x80) {
#ifdef __WIN32
			MessageBox(NULL,"Bledna sekwencja UTF-8",NULL,MB_OK | MB_ICONERROR);
#else
			fprintf(stderr,"Bledna sekwencja UTF-8\n");
#endif
			return 0;
		}
		znak=(znak<<6) | (m & 0x3f);
	}
	return znak;
}

static char *a_string[]={
/* 0a0 */ " ",
/* 0a1 */ "!",
/* 0a2 */ " cent ",
/* 0a3 */ " funt ",
/* 0a4 */ "¤",
/* 0a5 */ " jen ",
/* 0a6 */ "|",
/* 0a7 */ "§",
/* 0a8 */ " ",
/* 0a9 */ " copyright ",
/* 0aa */ " ",
/* 0ab */ "\"",
/* 0ac */ "~",
/* 0ad */ "\xad",
/* 0ae */ " registered ",
/* 0af */ "-",
/* 0b0 */ "°",
/* 0b1 */ " plus minus ",
/* 0b2 */ "^2",
/* 0b3 */ "^3",
/* 0b4 */ " ",
/* 0b5 */ "mikro",
/* 0b6 */ " ",
/* 0b7 */ ".",
/* 0b8 */ " ",
/* 0b9 */ "^1",
/* 0ba */ " ",
/* 0bb */ "\"",
/* 0bc */ " 1/4 ",
/* 0bd */ " 1/2 ",
/* 0be */ " 3/4 ",
/* 0bf */ "?",
/* 0c0 */ "Á",
/* 0c1 */ "Á",
/* 0c2 */ "Â",
/* 0c3 */ "Â",
/* 0c4 */ "Ä",
/* 0c5 */ "Ã", // "AA",
/* 0c6 */ "Ë", // "AE",
/* 0c7 */ "Ç",
/* 0c8 */ "É",
/* 0c9 */ "É",
/* 0ca */ "E",
/* 0cb */ "Ë",
/* 0cc */ "I",
/* 0cd */ "Í",
/* 0ce */ "Î",
/* 0cf */ "I",
/* 0d0 */ "Ð",
/* 0d1 */ "Ò",
/* 0d2 */ "O",
/* 0d3 */ "Ó",
/* 0d4 */ "Ô",
/* 0d5 */ "Õ",
/* 0d6 */ "Ö",
/* 0d7 */ "×",
/* 0d8 */ "Ö",
/* 0d9 */ "Ú",
/* 0da */ "Ú",
/* 0db */ "U",
/* 0dc */ "Ü",
/* 0dd */ "Ý",
/* 0de */ "T", //"TH",
/* 0df */ "ß",
/* 0e0 */ "á",
/* 0e1 */ "á",
/* 0e2 */ "â",
/* 0e3 */ "â",
/* 0e4 */ "ä",
/* 0e5 */ "ã", //"aa",
/* 0e6 */ "ä", //"ae",
/* 0e7 */ "ç",
/* 0e8 */ "é",
/* 0e9 */ "é",
/* 0ea */ "e",
/* 0eb */ "ë",
/* 0ec */ "í",
/* 0ed */ "í",
/* 0ee */ "î",
/* 0ef */ "i",
/* 0f0 */ "d", //"dh",
/* 0f1 */ "ò",
/* 0f2 */ "o",
/* 0f3 */ "ó",
/* 0f4 */ "ô",
/* 0f5 */ "õ",
/* 0f6 */ "ö",
/* 0f7 */ "÷",
/* 0f8 */ "ö",
/* 0f9 */ "ú",
/* 0fa */ "ú",
/* 0fb */ "u",
/* 0fc */ "ü",
/* 0fd */ "ý",
/* 0fe */ "t", //"th",
/* 0ff */ "y",
/* 100 */ "A",
/* 101 */ "a",
/* 102 */ "Ã",
/* 103 */ "ã",
/* 104 */ "¡",
/* 105 */ "±",
/* 106 */ "Æ",
/* 107 */ "æ",
/* 108 */ "C",
/* 109 */ "c",
/* 10a */ "C",
/* 10b */ "c",
/* 10c */ "È",
/* 10d */ "è",
/* 10e */ "Ï",
/* 10f */ "ï",
/* 110 */ "Ð",
/* 111 */ "ð",
/* 112 */ "E",
/* 113 */ "e",
/* 114 */ "E",
/* 115 */ "e",
/* 116 */ "E",
/* 117 */ "e",
/* 118 */ "Ê",
/* 119 */ "ê",
/* 11a */ "Ì",
/* 11b */ "ì",
/* 11c */ "G",
/* 11d */ "g",
/* 11e */ "G",
/* 11f */ "g",
/* 120 */ "G",
/* 121 */ "g",
/* 122 */ "G",
/* 123 */ "g",
/* 124 */ "H",
/* 125 */ "h",
/* 126 */ "H",
/* 127 */ "H",
/* 128 */ "I",
/* 129 */ "i",
/* 12a */ "I",
/* 12b */ "i",
/* 12c */ "I",
/* 12d */ "i",
/* 12e */ "I",
/* 12f */ "i",
/* 130 */ "I",
/* 131 */ "i",
/* 132 */ "IJ",
/* 133 */ "ij",
/* 134 */ "J",
/* 135 */ "j",
/* 136 */ "K",
/* 137 */ "k",
/* 138 */ "k",
/* 139 */ "Å",
/* 13a */ "å",
/* 13b */ "L",
/* 13c */ "l",
/* 13d */ "¥",
/* 13e */ "µ",
/* 13f */ "L",
/* 140 */ "l",
/* 141 */ "£",
/* 142 */ "³",
/* 143 */ "Ñ",
/* 144 */ "ñ",
/* 145 */ "N",
/* 146 */ "n",
/* 147 */ "Ò",
/* 148 */ "ò",
/* 149 */ "'n",
/* 14a */ "NG",
/* 14b */ "ng",
/* 14c */ "O",
/* 14d */ "o",
/* 14e */ "O",
/* 14f */ "o",
/* 150 */ "Õ",
/* 151 */ "õ",
/* 152 */ "Ö",//"OE",
/* 153 */ "ö",//"oe",
/* 154 */ "À",
/* 155 */ "à",
/* 156 */ "R",
/* 157 */ "r",
/* 158 */ "Ø",
/* 159 */ "ø",
/* 15a */ "¦",
/* 15b */ "¶",
/* 15c */ "S",
/* 15d */ "s",
/* 15e */ "ª",
/* 15f */ "º",
/* 160 */ "©",
/* 161 */ "¹",
/* 162 */ "Þ",
/* 163 */ "þ",
/* 164 */ "«",
/* 165 */ "»",
/* 166 */ "T",
/* 167 */ "t",
/* 168 */ "U",
/* 169 */ "u",
/* 16a */ "U",
/* 16b */ "u",
/* 16c */ "U",
/* 16d */ "u",
/* 16e */ "Ù",
/* 16f */ "ù",
/* 170 */ "Û",
/* 171 */ "û",
/* 172 */ "U",
/* 173 */ "u",
/* 174 */ "W",
/* 175 */ "w",
/* 176 */ "Y",
/* 177 */ "y",
/* 178 */ "Y",
/* 179 */ "¬",
/* 17a */ "¼",
/* 17b */ "¯",
/* 17c */ "¿",
/* 17d */ "®",
/* 17e */ "¾",
/* 17f */ "s"};

static struct {
	int znak;
	char *repr;
} prochar[]={
{0x2116," numer "},
{0x2122," tm "},
{0x2126,"ohm "},
{0x3a9,"ohm "},
{0x3c9," omega "},
{0x221a," pierwiastek "},
{0xe08d, " pierwiastek "},
{0xe0b1, " pi "},
{0x221b," pierwiastek sze¶cienny "},
{0x221e," nieskoñczono¶æ "},
{0x222b, " ca³ka "},
{0,NULL}};


struct cyr_rule {
	int z1;
	int z2;
	char *trans;
};

#include "milena_cyrillic.h"

static int is_cyr(int znak)
{
	if (znak < MIN_CYR) return 0;
	if (znak > MAX_CYR) return 0;
	return is_cyr_char[znak-MIN_CYR];
}

static int translate_cyr(int z1,int z2,char *buf)
{
	int i;
	z1=is_cyr(z1);
	if (!z2 || isspace(z2)) z2='#';
	else z2=is_cyr(z2);
	for (i=0;i<CYR_RULES;i++) {
		if (z1 != cyr_rules[i].z1) continue;
		if (!cyr_rules[i].z2) {
			strcpy(buf,cyr_rules[i].trans);
			return 1;
		}
		if (cyr_rules[i].z2==z2) {
			strcpy(buf,cyr_rules[i].trans);
			return 2;
		}
	}
	return 0;
}


int milena_utf2iso_mp(char *instr,char *outstr,int ignore_oor,int *bad_chars,char **instrpos,int maxpos)
{
	int pos=0;
	int znak,i;
	if (bad_chars) *bad_chars=0;
	while (*instr) {
		char *c=instr;
		znak=get_unichar(&c);
		if (!uni_isspace(znak)) break;
		instr=c;
	}
	if (!*instr) {
		
		return 0;
	}
	while (*instr) {
		znak=get_unichar(&instr);
		if (znak=='\r') {
			if (*instr=='\n') instr++;
			pushout('\n');
			continue;
		}
		if (znak=='\n') {
			pushout('\n');
			continue;
		}
		if (uni_isspace(znak)) {
			pushout(' ');
			continue;
		}
        if (znak==',' && *instr==',') {
			instr++;
			pushout('"');
			continue;
		}
		if (znak <0x80) {
			pushout(znak);
			continue;
		}
		if (znak == 0x2022) {
			pushout('*');
			continue;
		}
		if (znak == 0x2026) {
			pushout('.');
			pushout('.');
			pushout('.');
			continue;
		}
		if (znak==0x218) znak=0x15e;
		else if (znak==0x219) znak=0x15f;
		else if (znak==0x21a) znak=0x162;
		else if (znak==0x21b) znak=0x163;
		if (znak<=0x17f) {
			char *d=a_string[znak-0xa0];
			while (*d) {
				pushout(*d);
				d++;
			}
			continue;
		}
		if (znak >= 0x2018 && znak <=0x201b) {
			pushout('\'');
			continue;
		}
		if ((znak >= 0x201c && znak <=0x201f) || znak==0x2039 || znak==0x203a) {
			pushout('"');
			continue;
		}
		if (znak== 0x2013 || znak == 0x2014 || znak == 0x2212 || znak == 0x2015) {
			pushout('-');
			continue;
		}
		if (is_cyr(znak)) {
			char buf[16];
			char *c=instr;
			int znak2=get_unichar(&c);
			int n=translate_cyr(znak,znak2,buf);
			if (n) {
				if (n==2) instr=c;
				for (c=buf;*c;c++) {
					pushout(*c);
				}
				continue;
			}
		}
		for (i=0;prochar[i].znak;i++) if (prochar[i].znak == znak) break;
		if (prochar[i].znak) {
			char *c=prochar[i].repr;
			for (;*c;c++) {
				pushout(*c);
			}
			continue;
		}
		if (bad_chars) (*bad_chars)++;
		if (!ignore_oor) {
#ifdef __WIN32
			char buf[64];
			sprintf(buf,"Znak spoza zakresu: \\u%04X\n",znak);
			MessageBox(NULL,buf,NULL,MB_OK | MB_ICONERROR);
#else
			fprintf(stderr,"Znak spoza zakresu: \\u%04X\n",znak);
#endif
			break;
		}
		if (ignore_oor == 2) {
			char buf[16],*c;
			sprintf(buf,"&#x%x;",znak);
			for (c=buf;*c;c++) {
				pushout(*c);
			}
		}
		else if (ignore_oor == 3) { /* dla sd */
			char buf[16],*c;
			sprintf(buf,"&\x1bx%x;",znak);
			for (c=buf;*c;c++) {
				pushout(*c);
			}
		}

	}
		
	pushout(0);
	return pos;
}

int milena_utf2iso(char *instr,char *outstr,int ignore_oor,int *bad_chars)
{
	return milena_utf2iso_mp(instr,outstr,ignore_oor,bad_chars,NULL,0);
}

int milena_alnum(int znak)
{
	if (znak < 0x80) {
		return isalnum(znak);
	}
	if (is_cyr(znak) || (znak >=0x218 && znak <= 0x21b)) return 1;
	if (znak>0x17e || znak  <0xc0 || znak == 0xd7 || znak == 0xf7) return 0;
	return 1;
}

#undef pushout
