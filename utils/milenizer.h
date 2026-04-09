/*
 * milenizer.h - Milena TTS system utilities
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

#ifndef MILENIZER_H
#define MILENIZER_H

#define AUMODE_ON 1
#define AUMODE_KEEP_LINE 2
#define AUMODE_PROLOG 4
#define AUMODE_PROLOG_KEEP 8
#define AUMODE_EPILOG 16
#define AUMODE_EPILOG_KEEP 32
#define AUMODE_LASTCHAPTER 64

#define UNFORMODE_ON 1
#define UNFORMODE_IGNORE_PAGENO 2
#define UNFORMODE_NO_DIALOG 4

extern char *split_path;
extern int splitmode;
extern int automode;
extern int unformode;
extern int pdfmode;
extern int autopar;
extern int ignore_oor;
extern int rtfdecode;
extern int make_index;
extern int nodrm;

int my_alpha(int);
int my_upper(int);
int my_alnum(int);
int my_space(int);
int readable(char *);
int to_iso2(char *instr,char *outstr);
char *to_utf8(char *fbuf,int flen,char *cset,int freeme);

void unformat(char *);
char *read_file(char *name,char *encoding);

int setFootNote(char *);

void split_body(char *body);
void split_timed(char *body,int kilobytes);

char *pdxml2txt(char *buf);
char *new_pdf_parser(char *buf);
extern int pdf_ignore_font_size;
extern int pdf_ignore_chunk_height;
extern int pdf_autonumber_pages;
extern int pdf_max_line_height;
extern int pdf_max_paragraph_height;
extern int pdf_show_heights;

#endif
