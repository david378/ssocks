/*
 *      rssocks.c
 *
 *      Created on: 2011-04-13
 *      Author:     Hugo Caron
 *      Email:      <h.caron@codsec.com>
 *
 * Copyright (C) 2011 by Hugo Caron
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <libsocks/bor-util.h>
#include <libsocks/net-util.h>

#include <libsocks/output-util.h>
#include <libsocks/socks-common.h>
#include <libsocks/socks5-client.h>
#include <libsocks/socks5-server.h>

#include <config.h>

#ifdef _WIN32
#include "win_getopt.h"
#else
#include <getopt.h>
#endif


#define PORT 1080

// global to prevent messing with the stack
// see http://stackoverflow.com/questions/1847789/segmentation-fault-on-large-array-sizes
s_client tc[MAXCLI];

struct globalArgs_t {
    char *host;				// -h option
    unsigned int port;		// -p option
    unsigned int verbosity;	// -v
    unsigned int background;// -b

#ifdef HAVE_LIBSSL
    unsigned int ssl;		// -k option
    char *certfile;			// -c option
#endif

    char *uname;			// -u option
    char *passwd;			// -p option

    char *sockshost;		// -s host:port
    int socksport;

    int ncon;				// -n option
} globalArgs;

int boucle_princ = 1;
void capte_fin (int sig) {
    TRACE(L_VERBOSE, "server: signal %d caught\n", sig);
    boucle_princ = 0;
}

void usage(char *name) {
    printf("rsSocks Reverse Socks5 Server v%s\n", PACKAGE_VERSION);
    printf("It's a reverse socks5 server, you set the socks arg (host:port)\n");
    printf("to your rcsocks instance. It connect back on and wait for request\n");
    printf("Usage:\n");
    printf("\t%s --socks rcsocksserv:1080 -b\n", name);
    printf("\t%s --socks rcsocksserv:1080\n", name);
    //printf("\t%s --socks socksserv.com:1080 --uname admin --passwd abcde\n", name);
    //printf("\t%s -s socksserv.com:1080 -u admin -p abcde -l 1080 -b\n", name);
    printf("Options:\n");
    printf("\t--verbose (increase verbose level)\n\n");
    printf("\t--socks {host:port}\n");
    printf("\t--ncon {nb of reverse connection}\n");
    //printf("\t--uname {uname}\n");
    //printf("\t--passwd {passwd}\n");
#ifdef HAVE_LIBSSL
    printf("\t--cert  {certfile.crt} Certificate of dst server (enable SSL)\n");
#endif
    printf("\t--background\n");
    printf("\n");
    printf("Bug report %s\n", PACKAGE_BUGREPORT);
}

void reverse_server(char *sockshost, int socksport,
                    char *uname, char *passwd, int ssl, int ncon) {
    int soc_ec = -1, maxfd, res, nc, k;
    fd_set set_read;
    fd_set set_write;

    s_socks_conf conf;
    s_socks_client_config config_cli;
    s_socks_server_config config_srv;

#ifdef _WIN32
    WSADATA wsaData;
    int wsaInit = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (wsaInit != 0) {
        ERROR(L_NOTICE, "WSAStartup failed: %d\n", wsaInit);
        exit(1);
    }
#endif

    conf.config.cli = &config_cli;
    conf.config.srv = &config_srv;

    char method[] =  { 0x00, 0x02 };
    char version[] = { SOCKS5_V };
    conf.config.srv->n_allowed_version = 1;
    conf.config.srv->allowed_version = version;
    conf.config.srv->n_allowed_method = 1;
    conf.config.srv->allowed_method = method;


    conf.config.cli->n_allowed_method = 2;
    conf.config.cli->allowed_method = method;

    /* If no username or password  we don't use auth */
    if ( uname == NULL || passwd == NULL )
        --conf.config.cli->n_allowed_method;

    conf.config.cli->loop = 1;
    conf.config.cli->host = NULL;
    conf.config.cli->port = 0;
    conf.config.cli->sockshost = sockshost;
    conf.config.cli->socksport = socksport;
    conf.config.cli->username = uname;
    conf.config.cli->password = passwd;
    conf.config.cli->version = version[0];

    /* Init client tab */
    for (nc = 0; nc < MAXCLI; nc++) init_client (&tc[nc], nc, M_SERVER, &conf);


#ifndef _WIN32
    if ( globalArgs.background == 1 ) {
        TRACE(L_NOTICE, "server: background ...");
        if ( daemon(0, 0) != 0 ) {
            perror("daemon");
            exit(1);
        }
    }

    bor_signal (SIGINT, capte_fin, SA_RESTART);
#endif


    while (boucle_princ) {
        k = init_select_server_reverse(tc, &maxfd, ncon,
                                       &set_read, &set_write, ssl);
        if ( k < 0 ) {
            break;
        }
        res = select (maxfd+1, &set_read, &set_write, NULL, NULL);

        if (res > 0) { /* Search eligible sockets */
            for (nc = 0; nc < MAXCLI; nc++) {
                k = dispatch_server(&tc[nc], &set_read, &set_write);
                if (k == -2 ) {
                    // Not sure why this was set to exit the loop; much more stable without it
                    //boucle_princ = 0;
                    break;
                }
            }
        } else if ( res == 0) {

        } else if (res < 0) {
            if (errno == EINTR) ; /* Received signal, it does nothing */
            else {
                perror ("select");
                break;
            }
        }
    }

#ifdef HAVE_LIBSSL
    if (ssl == 1)
        ssl_cleaning();
#endif
    printf ("Server: closing sockets ...\n");
    if (soc_ec != -1) CLOSE_SOCKET(soc_ec);
    for (nc = 0; nc < MAXCLI; nc++) disconnection(&tc[nc]);

#ifdef _WIN32
    WSACleanup();
#endif
}


void parse_arg(int argc, char *argv[]) {
    memset(&globalArgs, 0, sizeof(globalArgs));
    globalArgs.ncon = 25;
    int c;
    while (1) {
        static struct option long_options[] = {
            {"help",    no_argument,       0, 'h'},
            {"verbose", no_argument,       0, 'v'},
            {"background", no_argument,    0, 'b'},
            {"socks",   required_argument, 0, 's'},
            {"uname",   required_argument, 0, 'u'},
            {"passwd",  required_argument, 0, 'p'},
            {"ncon",    required_argument, 0, 'n'},
#ifdef HAVE_LIBSSL
            {"cert",      required_argument, 0, 'c'},
#endif
            {0, 0, 0, 0}
        };

        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "h?bvc:s:u:p:l:n:",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        char *port;

        switch (c)	{
        case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options[option_index].flag != 0)
                break;
            printf ("option %s", long_options[option_index].name);
            if (optarg)
                printf (" with arg %s", optarg);
            printf ("\n");
            break;

        case 'v':
            //globalArgs.verbosity++;
            verbosity++;
            break;

#ifdef HAVE_LIBSSL
        case 'c':
            globalArgs.ssl = 1;
            globalArgs.certfile = optarg;
            break;
        case 'k':
            globalArgs.ssl = 1;
            break;
#endif

        case 'b':
            globalArgs.background = 1;
            break;

        case 's':

            port = strchr(optarg, ':');
            if ( port == NULL ) {
                usage(argv[0]);
                exit(1);
            }
            *port = 0;
            port++;
            globalArgs.sockshost = optarg;
            globalArgs.socksport = atoi(port);
            /*printf("Connect trought socks %s:%d\n",
            	globalArgs.sockshost, globalArgs.socksport);*/
            break;

        case 'u':
            /* printf("Username: %s\n", optarg); */
            globalArgs.uname = optarg;
            break;

        case 'p':
            /* printf("Passwd: %s\n", optarg); */
            globalArgs.passwd = optarg;
            break;

        case 'n':
            globalArgs.ncon = atoi(optarg);
            break;

        case '?':
            /* getopt_long already printed an error message. */
            usage(argv[0]);
            exit(1);
            break;

        case 'h':
            usage(argv[0]);
            exit(1);
            break;

        default:
            abort ();
        }
    }

    if ( globalArgs.sockshost == NULL || globalArgs.socksport == 0 ) {
        usage(argv[0]);
        exit(1);
    }
#ifdef HAVE_LIBSSL
    /*Initialize ssl with the CA certificate file
     */
    if (globalArgs.certfile != NULL) {
        SSL_load_error_strings();  /* readable error messages */
        SSL_library_init();        /* initialize library */
        TRACE(L_VERBOSE, "client: init ssl ...");
        if (globalArgs.certfile == NULL) {
            ERROR(L_NOTICE, "client: actually need CA certificate file");
            exit(1);
        }
        if ( ssl_init_client(globalArgs.certfile) != 0) {
            ERROR(L_NOTICE, "client: ssl config error");
            exit(1);
        }
        TRACE(L_VERBOSE, "client: ssl ok.");
    }
#endif
}


int main (int argc, char *argv[]) {
    parse_arg(argc, argv);
    reverse_server(globalArgs.sockshost, globalArgs.socksport,
                   globalArgs.uname, globalArgs.passwd,
#ifdef HAVE_LIBSSL
                   globalArgs.ssl,
#else
                   0,
#endif
                   globalArgs.ncon);
    exit(0);
}
