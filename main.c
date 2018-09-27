#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>

#include "tcp_relay.h"

char i_host[128] = "192.168.2.254";/* site name */
char i_port[128] = "8080";/* port number */

#ifdef WIN32
char *optarg;

char getopt(int c, char *v[], char *opts)
{
    static int now = 1;
    char *p;

    if (now >= c) return EOF;

    if (v[now][0] == '-' && (p = strchr(opts, v[now][1]))) {
        optarg = v[now+1];
        now +=2;
        return *p;
    }

    return EOF;
}

#else
char *optarg;
#endif

#define required(a) if (!a) { return -1; }

int init(int argc, char *argv[])
{
    char c;

    while ((c = getopt(argc, argv, "i:p:?")) != EOF) {
    //printf("%c\n",c);
        if (c == '?')
            return -1;
        switch (c) { 
        case 'i':
            required(optarg);
            strcpy(i_host, optarg);
            break;
        case 'p':
            required(optarg);
            strcpy(i_port, optarg);
            break;
        default:
            return -1;
        }
    }

    /* 
     * there is no default value for hostname, port number, 
     * password or uri
     */
    if (i_host[0] == '\0' || i_port[0] == '\0')
        return -1;

    return 1;
}

void print_usage()
{
    char *usage[] =
    {
        "Usage:",
        "    -i    host name",
        "    -p    port",
        "example:",
        "    -i 192.168.16.254 -p 8080",
    };   
    int i;

    for (i = 0; i < sizeof(usage) / sizeof(char*); i++)
        printf("%s\n", usage[i]);
    
    return;
}

int main(int argc, char *argv[])
{
    int fd;
    int n;

    /* parse command line etc ... */
    if (init(argc, argv) < 0) {
        print_usage();
        exit(1);
    }

    tcp_relay_index(i_host, (unsigned short)atoi(i_port), 0, 500);/* 500ms relay time */
    usleep(1000000);
    tcp_relay_index(i_host, (unsigned short)atoi(i_port), 2, 500);

    return 0;
}
