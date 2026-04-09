/*
 * milena_rintoni.c - Milena TTS system (intonator)
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

#ifndef DYNAMIC_INTONATOR

#define STENV_RISE &env_static[0]
#define STENV_FALL &env_static[1]
#define STENV_FRISE &env_static[2]
#define STENV_FRIS2 &env_static[3]
#define STENV_DROP &env_static[4]
#define STENV_QUESTION &env_static[5]

static struct env_data env_static[]={
	{&env_static[1],"rise",2,{0,100},{0.0,1.0}},
	{&env_static[2],"fall",2,{0,100},{1.0,0}},
	{&env_static[3],"frise",5,{0,17,35,68,100},{1.0,0.24,0.0,0.03,0.09}},
	{&env_static[4],"fris2",5,{0,36,73,87,100},{1.0,0.24,0.0,0.03,0.0125}},
	{&env_static[5],"drop",5,{0,24,50,75,100},{1.0,0.78,0.566,0.19,0.0}},
        {NULL,"question",3,{0,50,100},{0.0,2.0,0.0}}};




static struct intonator intonator_static={
	{1.0,1.0}, // voice param
        STENV_RISE,
        STENV_FALL,
        &env_static[0],
        {NULL,NULL,NULL,
                STENV_QUESTION,
        NULL,NULL}, // singles
        {
                   {STENV_FALL,
    STENV_FALL,
   30, 5, 30, 7,              // statement
   20, 25,   34, 22, 3, 3,   12, 8},

   {STENV_FRISE,
    STENV_FRIS2,
    38,10, 36,10,              // comma
   20, 25,   34, 20,3, 3,   15, 25},

   {STENV_DROP,
    STENV_DROP,
    38, 1, 42,25,              // exclamation
   20, 25,   34, 22, 3, 3,   12, 8},

   {STENV_FRISE,
    STENV_FRIS2,
    38,10,36,10,              // question
   20, 25,   34, 20, 13,  13,   15, 65},

   {STENV_FRISE,
    STENV_FALL,
    38, 1, 42,25,              // colon
   20, 25,   34, 22,3, 3,   12, 10},


   {STENV_FALL,
    STENV_FALL,
   30, 5, 30, 7,              // ellipsis
   20, 25,   22, 18, 3, 3,   13, 22},

        }, // tonetables
        266,130
};
        
#else
static struct env_data env_init[]={
	{NULL,"rise",2,{0,100},{0.0,1.0}},
	{NULL,"fall",2,{0,100},{1.0,0}},
	{NULL,"frise",5,{0,17,35,68,100},{1.0,0.24,0.0,0.03,0.09}},
	{NULL,"fris2",5,{0,36,73,87,100},{1.0,0.24,0.0,0.03,2.125}},
	{NULL,"drop",5,{0,24,50,75,100},{1.0,0.78,0.566,0.19,0.0}}};

static struct tone_table tone_table_init[] = {
   {(struct env_data *)"fall",
    (struct env_data *)"fall",
   30, 5, 30, 7,              // statement
   20, 25,   34, 22, 3, 3,   12, 8},

   {(struct env_data *)"frise",
    (struct env_data *)"fris2",
    38,10, 36,10,              // comma
   20, 25,   34, 20,3, 3,   15, 25},

   {(struct env_data *)"drop",
    (struct env_data *)"drop",
    38, 1, 42,25,              // exclamation
   20, 25,   34, 22, 3, 3,   12, 8},

   {(struct env_data *)"frise",
    (struct env_data *)"fris2",
    38,10,36,10,              // question
   20, 25,   34, 20,3, 3,   15, 25},

   {(struct env_data *)"frise",
    (struct env_data *)"fall",38, 1, 42,25,              // colon
   20, 25,   34, 22,3, 3,   12, 8},

   {(struct env_data *)"fall",
    (struct env_data *)"fall",
   30, 5, 30, 7,              // ellipsis
   20, 25,   34, 22, 3, 3,   12, 8},


/* alternatives
   {PITCHfall, 36, 6,  PITCHfall, 36, 8,
   30, 20,   18, 34,  drops_0, 3, 3,   12, 8, 0},
   
   {PITCHfrise, 38, 8, PITCHfrise2, 36, 8,
   30, 20,   18, 34,  drops_0, 3, 3,   20, 32, 0},

   {PITCHfall, 36, 6,  PITCHfall, 36, 8,
   30, 20,   18, 34,  drops_0, 3, 3,   12, 8, 0},
*/
   
};
#endif

int milena_InitInton(struct milena_mbrola_cfg *cfg)
{
#ifndef DYNAMIC_INTONATOR
        cfg->inton = &intonator_static;
        return 1;
#else

	struct intonator *inton=malloc(sizeof(*inton));
	int i,j;
	struct env_data *ed;
	cfg->inton=inton;
	inton->vp.base_pitch=1.0;
	inton->vp.range=1.0;
	for (i=0;i<5;i++) {
		ed=malloc(sizeof(*ed));
		ed->next=inton->env;
		inton->env=ed;
		ed->count=env_init[i].count;
		ed->name=env_init[i].name;
		for (j=0;j<ed->count;j++) {
			ed->offset[j]=env_init[i].offset[j];
			ed->value[j]=env_init[i].value[j];
		}
		if (!strcmp(ed->name,"rise")) inton->rise=ed;
		else if (!strcmp(ed->name,"fall")) inton->fall=ed;
	}
	for (i=0;i<6;i++) {
		inton->singles[i]=NULL;
		memcpy(&inton->tone_table[i],&tone_table_init[i],sizeof(struct tone_table));
		for (ed=inton->env;ed;ed=ed->next) {
			if (!strcmp((char *)(inton->tone_table[i].pitch_env0),ed->name)) {
				inton->tone_table[i].pitch_env0=ed;
				break;
			}
		}
		for (ed=inton->env;ed;ed=ed->next) {
			if (!strcmp((char *)(inton->tone_table[i].pitch_env1),ed->name)) {
				inton->tone_table[i].pitch_env1=ed;
				break;
			}
		}
	}
	inton->pitch_range2=266;
	inton->pitch_base2=130;
#endif
}

#ifdef DYNAMIC_INTONATOR
static int inton_read_line(struct milena_mbrola_cfg *cfg,FILE *f)
{
	char input_buf[256],*input_text;
	char *cmd,*name;
	static char *tablenames[6]={"statement","comma","exclamation","question","colon","ellipsis"};
	int was_error=0;
	void next_word(void)
	{
		while (*input_text && !isspace(*input_text)) input_text++;
		if (*input_text) {
			*input_text++=0;
			while (*input_text && isspace(*input_text)) input_text++;
		}
	}
	int get_int(void)
	{
		int n;
		if (!*input_text || !isdigit(*input_text)) r_gerror("Oczekiwano liczby");
		n=strtol(input_text,&input_text,10);
		if (*input_text && !isspace(*input_text)) r_gerror("Blad zapisu liczby");
		while (*input_text && isspace(*input_text)) input_text++;
		return n;
	}
	double get_double(void)
	{
		double d;
		if (!*input_text || !isdigit(*input_text)) r_gerror("Oczekiwano liczby");
		d=my_strtod(input_text,&input_text);
		if (*input_text && !isspace(*input_text)) r_gerror("Blad zapisu liczby");
		while (*input_text && isspace(*input_text)) input_text++;
		return d;
	}
	
	if (!(input_text=get_line(cfg,f,input_buf))) return 0;
	if (!*input_text) return 1;
	cmd=input_text;
	next_word();
	if (!*input_text) gerror("Blad skladni");
	if (!strcmp(cmd,"basepitch")) {
		cfg->inton->pitch_base2=get_int();
		if (was_error) return -1;
		if (*input_text) gerror("Blad skladni");
		return 1;
	}
	if (!strcmp(cmd,"baserange")) {
		cfg->inton->pitch_range2=get_int();
		if (*input_text) gerror("Blad skladni");
		return 1;
	}
	if (!strcmp(cmd,"single")) {
		struct env_data *ed;
		int i;
		name=input_text;
		next_word();
		for (i=0;i<6;i++) if (!strcmp(tablenames[i],name)) break;
		if (i>=6) gerror("Nieznana tonetable");
		if (!(ed=cfg->inton->singles[i])) {
			ed=cfg->inton->singles[i]=malloc(sizeof(*ed));
			ed->next=cfg->inton->env;
			cfg->inton->env=ed;
			ed->name=strdup(name);
		}
		for (ed->count=0;ed->count<8;ed->count++) {
			if (!*input_text) break;
			ed->offset[ed->count]=get_int();
			if (was_error) return -1;
			ed->value[ed->count]=get_double();
			if (was_error) return -1;
		}
		if (ed->count < 2 || (*input_text && isdigit(*input_text))) gerror("envelope misi miec od 2 do 8 elementow");
		if (*input_text) gerror("Blad skladni");
		return 1;
	}
	if (!strcmp(cmd,"envelope")) {
		struct env_data *ed;
		name=input_text;
		next_word();
		for (ed=cfg->inton->env;ed;ed=ed->next) if (!strcmp(ed->name,name)) break;
		if (!ed) {
			ed=malloc(sizeof(*ed));
			ed->next=cfg->inton->env;
			cfg->inton->env=ed;
			ed->name=strdup(name);
		}
		for (ed->count=0;ed->count<8;ed->count++) {
			if (!*input_text) break;
			ed->offset[ed->count]=get_int();
			ed->value[ed->count]=get_double();
			if (was_error) return -1;
		}
		if (ed->count < 2 || (*input_text && isdigit(*input_text))) gerror("envelope misi miec od 2 do 8 elementow");
		if (*input_text) gerror("Blad skladni");
		return 1;
	}
	if (!strcmp(cmd,"tonetable")) {
		int i;
		struct tone_table *t;
		struct env_data *ed;
		name=input_text;
		next_word();
		for (i=0;i<6;i++) if (!strcmp(tablenames[i],name)) break;
		if (i>=6) gerror("Nieznana tonetable");
		t=&cfg->inton->tone_table[i];
		name=input_text;next_word();
		for (ed=cfg->inton->env;ed;ed=ed->next) if (!strcmp(ed->name,name)) break;
		if (!ed) gerror("Nieznany envelope");
		t->pitch_env0=ed;
		t->tonic_max0=get_int();
		if (was_error) return -1;
		t->tonic_min0=get_int();
		if (was_error) return -1;
		name=input_text;next_word();
		for (ed=cfg->inton->env;ed;ed=ed->next) if (!strcmp(ed->name,name)) break;
		if (!ed) gerror("Nieznany envelope");
		t->pitch_env1=ed;
		t->tonic_max1=get_int();
		if (was_error) return -1;
		t->tonic_min1=get_int();
		if (was_error) return -1;
		t->pre_start=get_int();
		if (was_error) return -1;
		t->pre_end=get_int();
		if (was_error) return -1;
		t->body_start=get_int();
		if (was_error) return -1;
		t->body_end=get_int();
		if (was_error) return -1;
		t->body_max_steps=get_int();
		if (was_error) return -1;
		t->body_lower_u=get_int();
		if (was_error) return -1;
		t->tail_start=get_int();
		if (was_error) return -1;
		t->tail_end=get_int();
		if (was_error) return -1;
		if (*input_text) gerror("Blad skladni");
		
		return 1;
	}		
	//fprintf(stderr,"%s\n",input_text);
	return 1;
}

int milena_ReadInton(struct milena_mbrola_cfg *cfg,char *fname)
{
	FILE *f;int n;
	f=fopen(fname,"rb");
	if (!f) {
		perror(fname);
		return 0;
	}
	cfg->fname=fname;
	cfg->input_line=0;
	for (;;) {
		n=inton_read_line(cfg,f);
		if (n<=0) break;
	}
	fclose(f);
	if (n<0) return 0;
	return 1;
}
#else

int milena_ReadInton(struct milena_mbrola_cfg *cfg,char *fname)
{
        // funkcja do wywalenia
        return 1;
}
#endif
