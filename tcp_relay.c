/*
 * on Unix:
 *    cc -c tcp_relay.c
 *    cc -o tcp_relay tcp_relay.o
 *
 * on Windows NT:
 *    open connector.c in Visual Studio
 *    press 'F7' to link -- a project to be created
 *    add wsock32.lib to the link section under project setting
 *    press 'F7' again
 *
 * running:
 *    type 'tcp_relay' for usage
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

//#define WIN32 1
#ifdef WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "tcp_relay.h"

#ifndef INADDR_NONE
#define INADDR_NONE     0xffffffff
#endif
#define MAX_STRING_LEN  1024
#define BUFSIZE  2048

char buf[BUFSIZE];

void err_doit(int errnoflag, const char *fmt, va_list ap);
void err_quit(const char *fmt, ...);

//xnet_select x defines
#define READ_STATUS  0
#define WRITE_STATUS 1
#define EXCPT_STATUS 2

/*
s    - SOCKET
sec  - timeout seconds
usec - timeout microseconds
x    - select status
*/
int xnet_select(int s, int sec, int usec, short x)
{
    int st = errno;
    struct timeval to;
    fd_set fs;
    to.tv_sec = sec;
    to.tv_usec = usec;
    FD_ZERO(&fs);
    FD_SET(s, &fs);
    switch(x){
    case READ_STATUS:
        st = select(s+1, &fs, 0, 0, &to);
        break;
    case WRITE_STATUS:
        st = select(s+1, 0, &fs, 0, &to);
        break;
    case EXCPT_STATUS:
        st = select(s+1, 0, 0, &fs, &to);
        break;
    }
    return (st);
}

int tcp_connect(const char *host, const unsigned short port)
{
    char * transport = "tcp";
    struct hostent      *phe;   /* pointer to host information entry    */
    struct protoent *ppe;       /* pointer to protocol information entry*/
    struct sockaddr_in sin;     /* an Internet endpoint address  */
    int s;                    /* socket descriptor and socket type    */

#ifdef WIN32
    {
        WORD wVersionRequested;
        WSADATA wsaData;
        int err;
 
        wVersionRequested = MAKEWORD( 2, 0 );
 
        err = WSAStartup( wVersionRequested, &wsaData );
        if ( err != 0 ) {
            /* Tell the user that we couldn't find a usable */
            /* WinSock DLL.                               */
            printf("can't initialize socket library\n");
            exit(0);
        }
    }
#endif    
    
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    
    if ((sin.sin_port = htons(port)) == 0)
        err_quit("invalid port \"%d\"\n", port);
    
    /* Map host name to IP address, allowing for dotted decimal */
    if ( phe = gethostbyname(host) )
        memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
    else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
        err_quit("can't get \"%s\" host entry\n", host);
    
    /* Map transport protocol name to protocol number */
    if ( (ppe = getprotobyname(transport)) == 0)
        err_quit("can't get \"%s\" protocol entry\n", transport);
    
    /* Allocate a socket */
    s = socket(PF_INET, SOCK_STREAM, ppe->p_proto);
    if (s < 0)
        err_quit("can't create socket: %s\n", strerror(errno));
    
    /* Connect the socket */
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        err_quit("can't connect to %s:%d\n", host, port);

    return s;
}

void err_doit(int errnoflag, const char *fmt, va_list ap)
{
    int errno_save;
    char buf[MAX_STRING_LEN];

    errno_save = errno; 
    vsprintf(buf, fmt, ap);
    if (errnoflag)
        sprintf(buf + strlen(buf), ": %s", strerror(errno_save));
    strcat(buf, "\n");
    fflush(stdout);
    fputs(buf, stderr);
    fflush(NULL);
    return;
}

/* Print a message and terminate. */
void err_quit(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(0, fmt, ap);
    va_end(ap);
    exit(1);
}

#ifdef WIN32
#define write(a, b, c) send(a, b, c, 0)
#define read(a, b, c) recv(a, b, c, 0)
#endif

int tcp_relay_index(const char *host, const unsigned short port, char index, int delay)
{
    int fd;
    int n;
   					        //   0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15   16   17   18   19
    char send_data[20] = {(char)0xAA,0x0F,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,(char)0xBB};

    //buf[0] = '\0';

    /* pack the info into the buffer */     
    //strcpy(buf, "HelloWorld");
 
    /* make connection to the server */
    fd = tcp_connect(host, port);

	if(xnet_select(fd, 0, 500, WRITE_STATUS)>0){
        /* send off the message, OPEN */
        send_data[2] = index;
        send_data[3] = 0x01;
        write(fd, send_data, sizeof(send_data));
#ifdef MSGDBG
        printf("Client send data:\n");
        for (int i = 0; i < sizeof(send_data); i++)
            printf(" 0x%02x", (unsigned char)send_data[i]);
        printf("\n");
#endif
    }

    if(xnet_select(fd, 3, 0, READ_STATUS)>0){
        /* display the server response */
        n = read(fd, buf, BUFSIZE);
#ifdef MSGDBG
        printf("Server response:\n");
        for (int i = 0; i < strlen(buf); i++)
            printf(" 0x%02x", (unsigned char)buf[i]);
        printf("\n");
#endif
	} else{
        err_quit("Socket I/O Read Timeout %s:%s\n", host, port);
    }

    if (delay > 100)
        usleep(delay * 1000);
    else
        usleep(100000);

    if(xnet_select(fd, 0, 500, WRITE_STATUS)>0){
        /* send off the message, CLOSE */
        send_data[2] = index;
        send_data[3] = 0x02;
        write(fd, send_data, sizeof(send_data));
#ifdef MSGDBG
        printf("Client send data:\n");
        for (int i = 0; i < sizeof(send_data); i++)
            printf(" 0x%02x", (unsigned char)send_data[i]);
        printf("\n");
#endif
    }

    if(xnet_select(fd, 3, 0, READ_STATUS)>0){
		/* display the server response */
		n = read(fd, buf, BUFSIZE);
#ifdef MSGDBG
        printf("Server response:\n");
		for (int i = 0; i < strlen(buf); i++)
		    printf(" 0x%02x", (unsigned char)buf[i]);
        printf("\n");
#endif
	} else{
        err_quit("Socket I/O Read Timeout %s:%s\n", host, port);
    }

#ifdef WIN32
    WSACleanup();
#endif

    return 0;
}
