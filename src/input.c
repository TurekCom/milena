#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define IBSIZE 1024

struct input_buffer {
	int size;
	char *str;
};

void *alloc_input_buffer(void)
{
	struct input_buffer *ib;
	ib=malloc(sizeof(*ib));
	ib->size=IBSIZE;
	ib->str=malloc(ib->size);
	return (void *)ib;
}

static int getInputFrag(char *buf,int len,FILE *f,int *eol)
{
	int c;
	int i;
	for (i=0;i<len-2;i++) {
		c=fgetc(f);
		if (c==EOF) {
			buf[i]=0;
			*eol=2;
			return i;
		}
		buf[i]=c;
		if (c=='\r') {
			i++;
			c=fgetc(f);
			if (c==EOF) {
				buf[i]=0;
				*eol=2;
				return i;
			}
			if (c=='\n') buf[i++]=c;
			else ungetc(c,f);
			*eol=1;
			buf[i]=0;
			return i;
		}
		if (c=='\n') {
			i++;
			buf[i]=0;
			*eol=1;
			return i;
		}
	}
	*eol=0;
	return i;	
}

char *fgets_ra(void *ibv,FILE *f)
{
	struct input_buffer *ib=(struct input_buffer *)ibv;
	int pos=0;
	int eol=0;
	int len;
	for (;;) {
		if (pos >= ib->size-2) {
			ib->size+=IBSIZE;
			ib->str=realloc(ib->str,ib->size);
		}
		len=getInputFrag(ib->str+pos,ib->size-pos,f,&eol);
		pos+=len;
		if (!len || eol) break;
	}
	if (!pos) return NULL;
	return ib->str;
}
