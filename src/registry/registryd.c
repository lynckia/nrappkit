/*
 *
 *    registryd.c
 *
 *    $Source: /cvsroot/nrappkit/nrappkit/src/registry/registryd.c,v $
 *    $Revision: 1.4 $
 *    $Date: 2008/04/07 23:17:39 $
 *
 *    Daemon for the Registry
 *
 *    
 *    Copyright (C) 2005, Network Resonance, Inc.
 *    Copyright (C) 2006, Network Resonance, Inc.
 *    All Rights Reserved
 *    
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions
 *    are met:
 *    
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of Network Resonance, Inc. nor the name of any
 *       contributors to this software may be used to endorse or promote 
 *       products derived from this software without specific prior written
 *       permission.
 *    
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 *    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *    POSSIBILITY OF SUCH DAMAGE.
 *    
 *
 */

#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/un.h>
#include <signal.h>
#include "regrpc.h"
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <sys/ttycom.h>
#ifdef __cplusplus
#include <sysent.h>
#endif /* __cplusplus */
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <syslog.h>

#include "registry.h"
#include "registry_int.h"
#include "registry_vtbl.h"
#include "r_assoc.h"
#include "nr_common.h"
#include "r_log.h"
#include "r_errors.h"
#include "r_macros.h"
#include "async_wait.h"

#include "regrpc_svc.c"

static int registryd_port=8113;

static int nr_registry_server(int argc, char **argv);
static int nr_reg_start_regrpc_svc(void);
static void nr_reg_log_exit(void);
static void nr_reg_pipe_handler(int sig);
static void nr_reg_hup_handler(int sig);
static void nr_reg_dump_handler(int sig);
static void nr_reg_send_event(void *cb_arg, char action, char *name);
static void registrydprog_1_trampoline(struct svc_req *rqstp, SVCXPRT *transp);
static int nr_reg_cleanup_dead_clients(void);
static int nr_reg_send_to_clients(char *buffer, int bufferlen);

typedef struct nr_reg_client_ {
    TAILQ_ENTRY(nr_reg_client_)  entries;
    int fd;
#if 0
    int pid;
#endif
} nr_reg_client;

static TAILQ_HEAD(, nr_reg_client_) nr_reg_client_list = TAILQ_HEAD_INITIALIZER(nr_reg_client_list);


void
nr_reg_log_exit()
{
    r_log(NR_LOG_REGISTRY, LOG_INFO, "Exiting Registry");
}

void
nr_reg_pipe_handler(int sig)
{
    r_log(NR_LOG_REGISTRY, LOG_DEBUG, "Caught SIGPIPE, ignoring");
}

void
nr_reg_hup_handler(int sig)
{
    r_log(NR_LOG_REGISTRY, LOG_INFO, "Reloading Registry");
    exit(0);
}

void
nr_reg_dump_handler(int sig)
{
    r_log(NR_LOG_REGISTRY, LOG_INFO, "Dumping Registry");
    if (NR_reg_dump())
        r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't dump Registry");
}

void
registrydprog_1_trampoline(struct svc_req *rqstp, SVCXPRT *transp)
{
    int r;
    struct timeval tv = {0};
    int ignore;

    /* before handling the RPC, make sure we process any queued events */
    if ((r=NR_async_event_wait2(&ignore, &tv))) {
      if (r != R_EOD)
        r_log_nr(NR_LOG_REGISTRY, LOG_WARNING, r, "Couldn't process events");
    }

    registrydprog_1(rqstp, transp);
}

int
nr_registry_server(int argc, char **argv)
{
    int r;

    if ((r=atexit(nr_reg_log_exit)))
      r_log(NR_LOG_REGISTRY, LOG_WARNING, "Unable to register exit method: %d", r);

    if (signal(SIGPIPE, nr_reg_pipe_handler) == SIG_ERR)
      r_log(NR_LOG_REGISTRY, LOG_WARNING, "Unable to register SIGPIPE handler");

    if (signal(SIGHUP, nr_reg_hup_handler) == SIG_ERR)
      r_log(NR_LOG_REGISTRY, LOG_WARNING, "Unable to register SIGHUP handler");

    if (signal(SIGUSR1, nr_reg_dump_handler) == SIG_ERR)
      r_log(NR_LOG_REGISTRY, LOG_WARNING, "Unable to register SIGUSR1 handler");

    if (NR_reg_init(NR_REG_MODE_LOCAL)) {
      r_log(NR_LOG_REGISTRY, LOG_ERR, "Unable to start registry");
      exit(1);
    }

    printf("Ready registryd pid=%d\n",getpid());
    fflush(stdout);

    /* the server runs in NR_REG_MODE_LOCAL mode, and so it is possible
     * to register a listener which serves as a proxy to relay the
     * events to clients of the server */
    if (NR_reg_register_callback(NR_TOP_LEVEL_REGISTRY, NR_REG_CB_ACTION_ADD, nr_reg_send_event, 0)
     || NR_reg_register_callback(NR_TOP_LEVEL_REGISTRY, NR_REG_CB_ACTION_CHANGE, nr_reg_send_event, 0)
     || NR_reg_register_callback(NR_TOP_LEVEL_REGISTRY, NR_REG_CB_ACTION_DELETE, nr_reg_send_event, 0)
     || NR_reg_register_callback(NR_TOP_LEVEL_REGISTRY, NR_REG_CB_ACTION_FINAL, nr_reg_send_event, 0)) {
      r_log(NR_LOG_REGISTRY, LOG_ERR, "Unable to register callbacks");
      exit(1);
    }

    if ((r=NR_async_wait_init()))
      r_log(NR_LOG_REGISTRY, LOG_ERR, "Couldn't start asynchronous event subsystem");

    return nr_reg_start_regrpc_svc();
}


int
nr_reg_start_regrpc_svc()
{
    int _status;
    int sockfd;
    struct hostent *h;
    struct sockaddr_in sin;
    int optval;
    SVCXPRT *transp = 0;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        r_log_e(NR_LOG_REGISTRY, LOG_ERR, "Couldn't open socket");
        ABORT(R_FAILED);
    }

    if ((h = gethostbyname2("localhost", AF_INET)) == 0) {
        r_log_e(NR_LOG_REGISTRY, LOG_ERR, "Couldn't find localhost");
        ABORT(R_FAILED);
    }

    memset(&sin, 0, sizeof(sin));
#ifdef HAVE_SIN_LEN
    sin.sin_len = sizeof(sin);
#endif
    sin.sin_family = h->h_addrtype;
    sin.sin_port = htons(registryd_port);
    memcpy((char*)&sin.sin_addr, h->h_addr, h->h_length);

    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
 
    if (bind(sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        r_log_e(NR_LOG_REGISTRY, LOG_ERR, "Couldn't bind to socket");
        ABORT(R_FAILED);
    }

    optval = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));

    if ((transp = svctcp_create(sockfd, 0, 0)) == 0) {
        r_log(NR_LOG_REGISTRY, LOG_ERR, "Couldn't create tcp service");
        ABORT(R_FAILED);
    }

    if (!svc_register(transp, REGISTRYDPROG, REGISTRYDVERS, registrydprog_1_trampoline, 0)) {
        r_log(NR_LOG_REGISTRY, LOG_ERR, "unable to register (REGISTRYDPROG, REGISTRYDVERS, tcp).");
        ABORT(R_FAILED);
    }

    svc_run();

    r_log(NR_LOG_REGISTRY, LOG_ERR, "svc_run returned");
    ABORT(R_FAILED);
    /* NOTREACHED */
    _status=0;
  abort:
    endhostent();
    return(_status);
}

int
main(int argc, char **argv)
{
    char *pt=0;

    if(pt=getenv("REGISTRYD_PORT")){
      registryd_port=atoi(pt);
    }
    
    return nr_registry_server(argc, argv);
}

int
nr_reg_register_for_callbacks(int fd, int connect_to_port)
{
    int _status;
    nr_reg_client *client;
    struct sockaddr name;
    socklen_t namelen = sizeof(name);
    int type;
    socklen_t typelen = sizeof(type);
    int opt;
    socklen_t optlen = sizeof(opt);
    int cbfd = -1;
    int flags;

    (void)nr_reg_cleanup_dead_clients();

    if (getpeername(fd, &name, &namelen) < 0) {
        r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't get peer address: %s", strerror(errno));
        ABORT(R_FAILED);
    }

    if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &typelen) < 0) {
        r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't get socket type: %s", strerror(errno));
        ABORT(R_FAILED);
    }

    if ((cbfd = socket(name.sa_family, type, 0)) < 0) {
        r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't get socket: %s", strerror(errno));
        ABORT(R_FAILED);
    }

    if (name.sa_family == AF_INET) {
        ((struct sockaddr_in*)&name)->sin_port = htons(8114);

        opt = 1;
        setsockopt(cbfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, optlen);

        if (bind(cbfd, &name, namelen) < 0)
            r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't bind to address: %s", strerror(errno));
    }

#ifdef SO_NOSIGPIPE
    opt = 1;
    if ((setsockopt(cbfd, SOL_SOCKET, SO_NOSIGPIPE, &opt, optlen)) < 0) {
        r_log(NR_LOG_REGISTRY, LOG_INFO, "Couldn't set option: %s", strerror(errno));
    }
#endif

    if ((flags = fcntl(cbfd, F_GETFL, 0)) == -1) {
        r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't get flags: %s", strerror(errno));
        /* don't abort, just assume no flags are set */
    }

    if (name.sa_family == AF_INET)
         ((struct sockaddr_in*)&name)->sin_port = htons(connect_to_port);

    if (connect(cbfd, &name, namelen) < 0) {
        if (errno != EINPROGRESS) {
            r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't connect to peer: %s", strerror(errno));
            ABORT(R_FAILED);
        }
    }

    if (fcntl(cbfd, F_SETFL, flags & (~ O_NONBLOCK)) == -1) {
        r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't set flags: %s", strerror(errno));
        ABORT(R_FAILED);
    }

    client = (void*)RCALLOC(sizeof(*client));
    if (! client)
        ABORT(R_NO_MEMORY);

    client->fd = cbfd;

    TAILQ_INSERT_TAIL(&nr_reg_client_list, client, entries);
    r_log(NR_LOG_REGISTRY, LOG_DEBUG, "Adding client %d", cbfd);

    _status = 0;
  abort:
    if (_status) {
        r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't add client: %s", nr_strerror(_status));
        if (cbfd != -1) close(cbfd);
    }
    return _status;
}


void
nr_reg_send_event(void *cb_arg, char action, char *name)
{
    int r,_status;
    char *action_str = nr_reg_action_name(action);
    int bufferlen = strlen(name) + strlen(action_str) + 5;
    char buffer[bufferlen];

    r_log(NR_LOG_REGISTRY, LOG_DEBUG, "Sending %s '%s' ", action_str, name);

    snprintf(buffer, bufferlen, "%s %s\n\r", action_str, name);

    if ((r=nr_reg_send_to_clients(buffer, strlen(buffer))))
        ABORT(r);

    _status = 0;
  abort:
    return;
    //return _status;
}

int
nr_reg_send_to_clients(char *buffer, int bufferlen)
{
    nr_reg_client *client;
    nr_reg_client *next_client;

    for (client = TAILQ_FIRST(&nr_reg_client_list); client; client = next_client) {
        next_client = TAILQ_NEXT(client, entries);

        r_log(NR_LOG_REGISTRY, LOG_DEBUG, "Sending to client at %d", client->fd);
        if (NR_SOCKET_WRITE(client->fd, buffer, bufferlen) < 0) {
            r_log_e(NR_LOG_REGISTRY, LOG_DEBUG, "Removing client at %d", client->fd);
            TAILQ_REMOVE(&nr_reg_client_list, client, entries);
            close(client->fd);
            RFREE(client);
        }
    }

    return 0;
}

int
nr_reg_cleanup_dead_clients()
{
    int r,_status;

    /* sending an extra CR isn't a problem, and that will tell us if
     * any of the clients are down */
    if ((r=nr_reg_send_to_clients("\n", 1)))
        ABORT(r);

    _status = 0;
  abort:
    return _status;
}
