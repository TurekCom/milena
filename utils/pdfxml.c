/*
 * pdfxml.c - from Milena Audiobook Creator
 * Copyright (C) Bohdan R. Rau 2013 <ethanak@polip.com>
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
#include <ctype.h>

#ifdef PDF_PARSER_STANDALONE
#define gint int
#define gunichar int
#define g_free free
#define g_realloc realloc
#define g_malloc malloc
#define g_strdup strdup
#define g_malloc0(a) calloc(1,a)

char *
g_utf8_prev_char (const char *p)
{
  while (1)
    {
      p--;
      if ((*p & 0xc0) != 0x80)
	return (char *)p;
    }
    return NULL;
}

gunichar g_utf8_get_char(char *str)
{
	int znak,n,m;
	if (!str) return 0;
	znak=(*str++) & 255;
	if (!(znak & 0x80)) return znak;
	if ((znak & 0xe0)==0xc0) n=1;
	else if ((znak & 0xf0)==0xe0) n=2;
	else {
	    return (gunichar)'?';
	}
	znak &= 0x1f;
	while (n--) {
		m=*str++ & 255;
		if ((m & 0xc0)!=0x80) {
			return (gunichar)'?';
		}
		znak=(znak<<6) | (m & 0x3f);
	}
	return znak;
}


int g_unichar_isalnum(int z)
{
    if (z < 0x80) return isalnum(z);
    if (z==0xd7 || z== 0xf7) return 0;
    return (z >= 0xc0 && z < 0x180);
}

#else
#include "config.h"
#include <glib.h>
#endif

#define MY_MEMBLOCK_SIZE (1024*1000)

struct MyMemo {
    struct MyMemo *next;
    int data_end;
    int string_start;
    char memo[MY_MEMBLOCK_SIZE];
};

static struct MyMemo *MyMemblock;

static void *MyAlloc(int size,int forstring)
{
    struct MyMemo *m,**mm;
    if (size >= MY_MEMBLOCK_SIZE/2) {
	size_t sajz=sizeof(struct MyMemo)+size-MY_MEMBLOCK_SIZE;
	m=g_malloc0(sajz);
	m->next=NULL;
	m->data_end=0;
	m->string_start=0;
	for (mm=&MyMemblock;*mm;mm=&(*mm)->next);
	*mm=m;
	return (void *)m->memo;
    }
    if (!forstring) {
	size=(size+15) & 0xfffff0;
    }
    if (!MyMemblock || MyMemblock->string_start-MyMemblock->data_end < size) {
	m=g_malloc0(sizeof(*MyMemblock));
	m->next=MyMemblock;
	MyMemblock=m;
	m->data_end=0;
	m->string_start=MY_MEMBLOCK_SIZE;
    }
    if (forstring) {
	MyMemblock->string_start -= size;
	return MyMemblock->memo+MyMemblock->string_start;
    }
    int memo_pos;
    memo_pos=MyMemblock->data_end;
    MyMemblock->data_end += size;
    return MyMemblock->memo+memo_pos;
}

static void MyFreeMem(void)
{
    struct MyMemo *m;
    while ((m=MyMemblock)) {
	MyMemblock=m->next;
	g_free(m);
    }
}

static char *MyStrdup(char *c)
{
    char *d=MyAlloc(strlen(c)+1,1);
    strcpy(d,c);
    return d;
}
    
struct pdf_chunk {
    struct pdf_chunk *next;
    int left;
    int top;
    int width;
    int height;
    int fontsize;
    char *str;
};

static struct pdf_font {
    struct pdf_font *next;
    int spec;
    int size;
} *pdf_fonts;

struct pdf_line {
    struct pdf_line *next;
    int left;
    int top;
    int right;
    int bottom;
    int flags;
    struct pdf_chunk *chunks;
    char *content;
};

static struct pdf_page {
    struct pdf_page *next;
    char *content;
    struct pdf_chunk *chunks;
    struct pdf_line *lines;
} *pdf_pages,**pdf_new_page;

static struct pdf_diff {
    struct pdf_diff *next;
    int size;
    int count;
} *pdf_diffs;

gint pdf_ignore_font_size=60;
gint pdf_ignore_chunk_height=100;
gint pdf_autonumber_pages=0;
gint pdf_max_line_height=0;
gint pdf_max_paragraph_height=0;
gint pdf_show_heights=0;

static void preparse_pdf_page(struct pdf_page *page)
{
    struct pdf_font *pf;
    struct pdf_chunk *pc,**ppc;
    struct pdf_line *lastline=NULL,*pl,**ppl;
    char *buf=page->content;
    char *c,*d;
    int last_top=-1;
    int last_bottom;
    static char *chunk_buffer=NULL;
    static int chunk_buffer_len=1024;
    int last_blank=0;
    
    if (!chunk_buffer) {
	chunk_buffer=g_malloc(chunk_buffer_len);
    }
    
    void add_diff(int size)
    {
	struct pdf_diff *d,**dd;
	if (size < 0) return;
	for (dd=&pdf_diffs;*dd;dd=&(*dd)->next) {
	    if ((*dd)->size < size) continue;
	    if ((*dd)->size > size) break;
	    (*dd)->count++;
	    return;
	}
	d=MyAlloc(sizeof(*d),0);
	d->next=*dd;
	*dd=d;
	d->size=size;
	d->count=1;
    }
    
    void padd(char **ddst,char *src)
    {
	int znak;
	static char *liga[]={"ff","fi","fl","ffi","ffl"};
	char *dst=*ddst;
	while (*src) {
	    znak=*src++;
	    if (isspace(znak)) {
		last_blank=1;
		continue;
	    }
	    if (znak == '<') {
		src=strchr(src,'>');
		if (src) {
		    src++;
		    continue;
		}
	    }
	    if (last_blank) {
		*dst++=' ';
		last_blank=0;
	    }
	    if (znak != '&') {
		if (!(znak & 0x80)) {
		    *dst++=znak;
		    continue;
		}
		int n=0;
		if ((znak & 0xe0) == 0xc0) n=1;
		else if ((znak & 0xf0) == 0xe0) n=2;
		else if ((znak & 0xf8) == 0xf0) n=3;
		else if ((znak & 0xfc) == 0xf8) n=4;
		else if ((znak & 0xfe) == 0xfc) n=5;
		else n=0;
		*dst++=znak;
		while (*src && n>0) {
		    *dst++=*src++;
		    n--;
		}
		continue;
	    }
	    if (*src=='#') {
		char *dq;
		if (src[1]=='x') {
		    znak=strtol(src+2,&dq,16);
		}
		else {
		    znak=strtol(src+1,&dq,10);
		}
		if (!znak || *dq != ';') {
		    *dst++='&';
		    continue;
		}
		src=dq+1;
		if (znak <= 32) {
		    last_blank=1;
		    continue;
		}
		if (znak >= 0xFB00 && znak <= 0xFB04) {
		    strcpy(dst,liga[znak-0xFB00]);
		    dst+=strlen(dst);
		    continue;
		}
		if (znak < 128) {
		    *dst++=znak;
		}
		else if (znak < 0x800) {
		    *dst++=0xc0 | (znak >> 6);
		    *dst++=0x80 | (znak & 0x3f);
		}
		else if (znak < 0x10000) {
		    *dst++=0xe0 | (znak >> 12);
		    *dst++=0x80 | ((znak >> 6) & 0x3f);
		    *dst++=0x80 | (znak & 0x3f);
		}
		else {
		    *dst++='?';
		}
		continue;
	    }
            if (!strncmp(src,"amp;",4)) {
                *dst++='&';
                src+=4;continue;
            }
            if (!strncmp(src,"lt;",3)) {
                *dst++='<';
                src+=3;
                continue;
            }
            if (!strncmp(src,"gt;",3)) {
                *dst++='>';
                src+=3;
                continue;
            }
            if (!strncmp(src,"quot;",5)) {
                *dst++='"';
                src+=5;
                continue;
            }
	    *dst++='&';
        }
	*ddst=dst;
    }

    void connect_chunks(struct pdf_line *pl)
    {
	int tl;struct pdf_chunk *pc;
	char *c;
	int leftpos;
	for (tl=0,pc=pl->chunks;pc;pc=pc->next) {
	    tl += strlen(pc->str) +1;
	}
	if (tl >= chunk_buffer_len) {
	    while (tl >= chunk_buffer_len) chunk_buffer_len *= 2;
	    chunk_buffer=g_realloc(chunk_buffer,chunk_buffer_len);
	}
	leftpos=pl->chunks->left;
	last_blank=0;
	for (c=chunk_buffer,pc=pl->chunks;pc;pc=pc->next) {
	    if (pc->left > leftpos+2) {
		last_blank=1;
	    }
	    padd(&c,pc->str);
	    leftpos=pc->left+pc->width;
	}
	*c=0;
	for (c=chunk_buffer;*c;c++) if (!isspace(*c)) break;
	pl->content=MyStrdup(chunk_buffer);
    }
    ppc=&page->chunks;
    ppl=&page->lines;
    for (;buf;) {
	int w=0,h=0,l=0,t=0,f=0;
	buf=strchr(buf,'<');
	if (!buf) break;
	buf++;
	if (strncmp(buf,"text",4)) continue;
	c=buf+4;
        buf=strstr(buf,"</text");
        if (!buf) break;
        *buf++=0;
	d=strchr(c,'>');
	if (!d) continue;
	*d++=0;
	for (;;) {
	    while (*c && !isspace(*c)) c++;
	    if (!*c) break;
	    while (*c && isspace(*c)) c++;
	    if (!*c) break;
	    if (!strncmp(c,"left=\"",6)) l=strtol(c+6,&c,10);
	    else if (!strncmp(c,"top=\"",5)) t=strtol(c+5,&c,10);
	    else if (!strncmp(c,"width=\"",7)) w=strtol(c+7,&c,10);
	    else if (!strncmp(c,"font=\"",6)) f=strtol(c+6,&c,10);
	    else if (!strncmp(c,"height=\"",8)) h=strtol(c+8,&c,10);
	}
	if (!l || !t || !w || !h) continue;
	if (pdf_ignore_chunk_height>0 && h>pdf_ignore_chunk_height) continue;
	for (pf=pdf_fonts;pf;pf=pf->next) if (pf->spec == f) break;
	if (pf && pdf_ignore_font_size> 0 && pf->size > pdf_ignore_font_size) continue;
	pc=MyAlloc(sizeof(*pc),0);
	*ppc=pc;
	ppc=&pc->next;
	pc->left=l;
	pc->top=t;
	pc->width=w;
	pc->height=h;
	pc->fontsize=pf?pf->size:0;
	pc->str=d;
    }
    /* zakładamy że chunki tworzą linie */
    //printf("Page start\n");
    while ((pc=page->chunks)) {
	//printf("Chunk at %d.%d/%d.%d: %s\n",pc->top,pc->top+pc->height,last_top,last_bottom,pc->str);
	page->chunks=pc->next;
	pc->next=0;
	if (pc->top+pc->height < last_top) {
	    last_top=-1;
	}
	if (last_top < 0 || pc->top >= last_bottom-1) {
	    //printf("-- new line add\n");
	    pl=MyAlloc(sizeof(*pl),0);
	    last_top=pc->top;
	    last_bottom=pc->top+pc->height;
	    pl->chunks=pc;
	    for (ppl=&page->lines;*ppl;ppl=&(*ppl)->next) {
		if ((*ppl)->top >= last_bottom) break;
	    }
	    pl->next=*ppl;
	    *ppl=lastline=pl;
	    lastline->top=pc->top;
	    lastline->bottom=pc->top+pc->height;
	    //ppl=&pl->next;
	    continue;
	}
	if (pc->top+pc->height < last_top) {
	    //printf("Chunk ignored %d < %d %s\n",pc->top+pc->height, last_top,pc->str);
	    /* ignore chunk */
	    continue;
	}
	//printf("--Connected to line ad %d\n",lastline->top);
	for (ppc=&lastline->chunks;*ppc;ppc=&(*ppc)->next) {
	    if (pc->left < (*ppc)->left) break;
	}
	if (pc->top < lastline->top) lastline->top=pc->top;
	if (pc->top+pc->height > lastline->bottom) lastline->bottom=pc->top+pc->height;
	if (pc->top < last_top) last_top=pc->top;
	if (pc->top+pc->height > last_bottom) last_bottom=pc->top+pc->height;
	pc->next=*ppc;
	*ppc=pc;
    }
    for (pl=page->lines;pl;pl=pl->next) {
	pl->left=99999;
	pl->top=99999;
	pl->right=0;
	pl->bottom=0;
	for (pc=pl->chunks;pc;pc=pc->next) {
	    if (pc->left < pl->left) pl->left=pc->left;
	    if (pc->top < pl->top) pl->top=pc->top;
	    if (pc->left+pc->width > pl->right) pl->right=pc->left+pc->width;
	    if (pc->top+pc->height > pl->bottom) pl->bottom=pc->top+pc->height;
	}
	connect_chunks(pl);
	//printf("%d: %s\n",pl->top,pl->content);
    }
    for (ppl=&page->lines;*ppl;) {
	if ((*ppl)->content) ppl=&(*ppl)->next;
	else *ppl=(*ppl)->next;
    }
    for (pl=page->lines;pl && pl->next;pl=pl->next) {
	add_diff(pl->next->top-pl->top);
    }
}

static void postparse_pdf_page(struct pdf_page *pp,int minpd,int maxpd,int np)
{
    struct pdf_line *pl,*lastl;
    static char *page_buffer=NULL;
    static int page_buffer_len=1024;
    int lmarg,rmarg,tl;
    char *dst;
    if (!page_buffer) {
	page_buffer=g_malloc(page_buffer_len);
    }
    lmarg=999999;
    rmarg=0;
    tl=100;
    for (pl=pp->lines;pl;pl=pl->next) {
	if (pl->left < lmarg) lmarg=pl->left;
	if (pl->right > rmarg) rmarg=pl->right;
	tl+=strlen(pl->content)+2;
    }
    if (page_buffer_len <= tl) {
	while (page_buffer_len <= tl) page_buffer_len *=2;
	page_buffer=g_realloc(page_buffer,page_buffer_len);
    }
    lastl=NULL;
    dst=page_buffer;
    if (pdf_autonumber_pages) {
	sprintf(dst,"--<%03d>--\n", np);
	dst+=strlen(dst);
    }
    for (pl=pp->lines;pl;lastl=pl,pl=pl->next) {
	if (lastl) {
	    int dif=pl->top-lastl->top;
	    if (dif > maxpd) {
		*dst++='\n';
	    }
	    if (dif > minpd || pl->left > lmarg+2 || lastl->right < rmarg-32) {
		//printf("%d>%d, %d>%d, %d<%d, %s\n",dif,minpd,pl->left,lmarg+2,lastl->right,rmarg-10,pl->content);
		*dst++='\n';
	    }
	    else {
		int vb=0;
		if (dst > page_buffer+20) {
		    char *d=dst-1;
		    gunichar z;
		    if (*d == '-') {
			d=g_utf8_prev_char(d);
			z=g_utf8_get_char(d);
			if (g_unichar_isalnum(z)) {
			    vb=1;
			}
		    }
		}
		if (vb) dst--;
		else *dst++=' ';
	    }
	}
	strcpy(dst,pl->content);
	dst+=strlen(dst);
    }
    *dst++='\n';
    *dst=0;
    pp->content=MyStrdup(page_buffer);
}

static void postparse_pdf_pages(int minpd,int maxpd)
{
    struct pdf_page *pp;int n;
    for (pp=pdf_pages,n=1;pp;pp=pp->next,n++) {
	postparse_pdf_page(pp,minpd,maxpd,n);
    }
}

char *new_pdf_parser(char *buf)
{
    char *c,*d;
    struct pdf_page *pp;
    pdf_fonts=NULL;
    pdf_pages=NULL;
    pdf_diffs=NULL;
    pdf_new_page=&pdf_pages;
    
    while (buf) {
	c=strstr(buf,"<page");
	if (!c) break;
	buf=strstr(c,"</page");
	if (buf) *buf++=0;
	c=strchr(c,'>');
	if (!c) continue;
	*c++=0;
	for (;;) {
	    while (*c && isspace(*c)) c++;
	    if (!*c) break;
	    if (strncmp(c,"<fontspec",9)) break;
	    if (!c[9] || !isspace(c[9])) break;
	    c+=9;
	    d=c;
	    c=strchr(c,'>');
	    if (c) *c++=0;
	    int spec=-1;
	    int size=-1;
	    while (spec < 0 || size < 0) {
		while (*d && isspace(*d)) d++;
		if (!*d) break;
		if (!strncmp(d,"id=\"",4)) spec=strtol(d+4,NULL,10);
		else if (!strncmp(d,"size=\"",6)) size=strtol(d+6,NULL,10);
		while (*d && !isspace(*d)) d++;
	    }
	    if (spec >= 0 && size >= 0) {
		struct pdf_font *pf=MyAlloc(sizeof(struct pdf_font),0);
		pf->spec=spec;
		pf->size=size;
		pf->next=pdf_fonts;
		pdf_fonts=pf;
	    }
	}
	if (!c) continue;
	pp=MyAlloc(sizeof(*pp),0);
	pp->next=NULL;
	*pdf_new_page=pp;
	pdf_new_page=&(pp->next);
	pp->chunks=NULL;
	pp->content=c;
    }
    if (!pdf_pages) {
	MyFreeMem();
	return 0;
    }
    for (pp=pdf_pages;pp;pp=pp->next) {
	preparse_pdf_page(pp);
    }
    
    struct pdf_diff *da,*db;
    int mc=0,ms=0,mps=0;
    if (pdf_show_heights) {
	fprintf(stderr,"Statystyka wysokości linii\n");
	for (da=pdf_diffs;da;da=da->next) fprintf(stderr,"%-3d:%d\n",da->size,da->count);
    }
    for (da=pdf_diffs;da;da=da->next) {
	int n=da->count;
	for (db=da->next;db && db->size < da->size+5;db=db->next) {
	    n += db->count;
	}
	if (n>mc) {
	    ms=da->size+5;
	    mc=n;
	}
    }
    if (pdf_show_heights) fprintf(stderr,"Linia=%d, Akapit=%d, Ilość=%d\n",ms,mps=(ms * 3) /2,mc);
    if (pdf_max_line_height) ms=pdf_max_line_height;
    mps=(ms * 3) /2;
    if (pdf_max_paragraph_height > ms) {
	mps=pdf_max_paragraph_height; 
    }
    postparse_pdf_pages(ms,mps);
    char *content;
    int clen;
    for (pp=pdf_pages,clen=0;pp;pp=pp->next) {
	clen+=strlen(pp->content);
    }
    if (!clen) {
	content=g_strdup("Pusty plik PDF");
    }
    else {
	content=g_malloc(clen+1);
	c=content;
	for (pp=pdf_pages;pp;pp=pp->next) {
	    strcpy(c,pp->content);
	    c+=strlen(c);
	}
    }
    MyFreeMem();
    return content;
}
