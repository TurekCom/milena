#ifndef _MY_STDIO_H
#define _MY_STDIO_H 1

extern FILE *my_fopen(char *,char *);
extern int my_fgetc(FILE *);
extern int my_ungetc(int,FILE *);
extern int my_fgets(char *,int,FILE *);
extern int my_fclose(FILE *);
extern int my_fseek(FILE *,int,int);
extern void my_check_file_encoding(FILE *);
extern void my_perror(char *);
extern int get_string_encoding(char *buf,size_t len);
extern unsigned char cep2iso[];
#define fgetc my_fgetc
#define fopen my_fopen
#define fclose my_fclose
#define fgets my_fgets
#define ungetc my_ungetc
#define fseek my_fseek
#define isspace(a) strchr(" \t\r\n",a)

#endif
