#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include "serwer.h"

#define SOCKET_PATH "/tmp/.milena_sock"

int client_socket=-1;
int server_socket=-1;
static int server_timeout;
static int connect_server(void)
{
	int ret,sock;
	struct sockaddr_un address_unix;
	//if (client_socket >= 0) return 1;
	address_unix.sun_family = AF_UNIX;
	strcpy(address_unix.sun_path, SOCKET_PATH);
	address_unix.sun_path[sizeof(address_unix.sun_path) - 1] = '\0';
	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Client socket");
		exit(1);
	}
	ret =
	    connect(sock, (struct sockaddr *)&address_unix,
		    sizeof(address_unix));
	if (ret != -1) {
	    client_socket=sock;
	    return 1;
	}
	close(sock);
	return 0;
}

int have_server(void)
{
    int sock, ret;
    struct sockaddr_un address_unix;
    unsigned long ctrl=0x50505050;
    if (connect_server()) {
	write(client_socket,&ctrl,sizeof(ctrl));
	close(client_socket);
	client_socket=-1;
	return 1;
    }
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
	address_unix.sun_family = AF_UNIX;
	strcpy(address_unix.sun_path, SOCKET_PATH);
	address_unix.sun_path[sizeof(address_unix.sun_path) - 1] = '\0';
    ret = bind(sock,(struct sockaddr *)&address_unix,
		    sizeof(address_unix));
    if (ret) {
	unlink(SOCKET_PATH);
	ret = bind(sock,(struct sockaddr *)&address_unix,
		    sizeof(address_unix));
    }
    if (ret) {
	perror("bind");
	exit(1);
    }
    server_socket = sock;
    return 0;
}

void init_client_server(int timeout)
{
    if (!timeout) timeout=60;
    else if (timeout < 10) timeout=10;
    server_timeout=timeout;
    /*
    if (!connect_server()) {
	int i;
	if (!timeout) timeout=60;
	else if (timeout < 10) timeout=10;
	for (i=0;i<timeout;i++) {
	    if (connect_server()) break;
	    usleep(100000);
	}
	if (i >= timeout) {
	    perror("Cannot connect to server");
	    exit(1);
	}
    }
    */
    close(server_socket);
    signal(SIGPIPE,SIG_IGN);
}


void daemonize_me(void)
{
    setsid();
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    if (fork()) exit(0);
    
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>0; x--)
    {
        if (x != server_socket) {
            close (x);
        }
    }
}


void start_server(void)
{
    
    if (listen(server_socket,5)) {
	perror("listen");
	exit(1);
    }
}

static char *server_buffer;
static int server_buffer_len;
static int communication_fd;

char *get_from_client(int *lastpar)
{
    unsigned long psize;
    signed char lpar;
    int m;
    struct sockaddr_in addr;
    int clilen=sizeof(addr);
    int fd=accept(server_socket,(void *)&addr,&clilen);
    
    if (fd < 0) {
	perror("accept");
	exit(1);
    }
    if (read(fd,&psize,sizeof(psize)) != sizeof(psize)) {
	perror("read");
	exit(1);
    }
    if (psize == 0x51515151) {
	// quit
	exit(0);
    }
    if (psize == 0x50505050) {
	//fprintf(stderr,"Control\n");
	return NULL;
    }
    if (read(fd,&lpar,1) != 1) {
	perror("read");
	exit(1);
    }
    if (!server_buffer) {
	int n=1024;
	while (n < psize+1) {
	    n *= 2;
	}
	server_buffer=malloc(server_buffer_len=n);
    }
    else if (server_buffer_len < psize + 1) {
	while (server_buffer_len < psize + 1) {
	    server_buffer_len *= 2;
	}
	server_buffer=realloc(server_buffer,server_buffer_len);
    }
    if ((m=read(fd,server_buffer,psize)) != psize) {
	perror("read");
	exit(1);
    }
    server_buffer[psize]=0;
    communication_fd=fd;
    *lastpar=lpar;
    //fprintf(stderr,"Server got lastpar %d\n",*lastpar);
    return server_buffer;
}




char *get_from_server(int *lastpar,char *psf)
{
    void bad_server(void)
    {
	perror("Server error");
	exit(1);
    }
    
    unsigned long psize;
    signed char lpar=*lastpar;
    char *buf;
    
    
    //fprintf(stderr,"GFS START\n");

    if (!connect_server()) {
	int i;
	for (i=0;i<server_timeout;i++) {
	    if (connect_server()) break;
	    usleep(100000);
	}
	if (i >= server_timeout) {
	    perror("Cannot connect to server");
	    exit(1);
	}
    }


    
    //printf("PSF='%s'\n",psf);
    if (!*psf) return NULL;
    psize=strlen(psf);
    if (write(client_socket,&psize,sizeof(psize))!=sizeof(psize)) bad_server();
    //fprintf(stderr,"Write 1\n");
    if (write(client_socket,&lpar,1)!=1) bad_server();
    //fprintf(stderr,"Write 2\n");
    if (write(client_socket,psf,psize) != psize) bad_server();
    //fprintf(stderr,"Write 3\n");
    if (read(client_socket,&psize,sizeof(psize))!=sizeof(psize)) bad_server();
    //fprintf(stderr,"R1\n");
    if (read(client_socket,&lpar,1)!=1) bad_server();
    //fprintf(stderr,"R2\n");
    buf=malloc(psize+1);
    if (read(client_socket,buf,psize)!=psize) bad_server();
    //fprintf(stderr,"R3\n");
    buf[psize]=0;
    *lastpar=lpar;
    close(client_socket);
    client_socket=-1;
    return buf;
}

void return_to_client(int lastpar, char *output_buffer, size_t output_size)
{
    void bad_server(void)
    {
	perror("Write error");
	exit(1);
    }
    unsigned long psize=output_size;
    signed char lpar=lastpar;
    if (write(communication_fd,&psize,sizeof(psize))!=sizeof(psize)) bad_server();
    if (write(communication_fd,&lpar,1)!=1) bad_server();
    if (psize && write(communication_fd,output_buffer,psize) != psize) bad_server();
    close(communication_fd);
}
