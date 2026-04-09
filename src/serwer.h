#ifndef ETH_MILENA_SERWER_H
#define ETH_MILENA_SERWER_H 1

extern int client_socket;


extern char *get_from_client(int *lastpar);
extern char *get_from_server(int *lastpar,char *psf);
extern int have_server(void);
extern void init_client_server(int timeout);
extern void start_server(void);
extern void return_to_client(int lastpar, char *output_buffer, size_t output_size);
extern void daemonize_me(void);

#endif
