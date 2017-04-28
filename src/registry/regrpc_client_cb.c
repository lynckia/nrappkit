/*
 *
 *    regrpc_client_cb.c
 *
 *    $Source: /cvsroot/nrappkit/nrappkit/src/registry/regrpc_client_cb.c,v $
 *    $Revision: 1.2 $
 *    $Date: 2006/08/16 19:39:14 $
 *
 *    Client RPC callback-related functions
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
#include <sys/errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "registry.h"
#include "registry_int.h"
#include "regrpc.h"
#include "r_assoc.h"
#include "r_errors.h"
#include "nr_common.h"
#include "r_log.h"
#include "r_macros.h"
#include "c_buf.h"
#include "async_wait.h"

static void nr_reg_cb_async_wait_cb(int fd, int how, void *arg);
static int nr_reg_cb_process_line(char *line);


int
nr_reg_client_cb_init()
{
    static int cbfd = -1;
    int r, _status;
    int fd;
    int srvfd = -1;
    struct sockaddr name;
    socklen_t namelen = sizeof(name);
    int type;
    socklen_t typelen = sizeof(type);
    int connect_to_port;
    c_buf *buf = 0;
    CLIENT *clnt;
    int *result;

    if (cbfd != -1)
        close(cbfd);

    if ((r=nr_reg_get_client(&clnt)))
        ABORT(r);

    if ((r=NR_async_wait_init()))
        ABORT(r);

#ifdef CLGET_FD
    if (!clnt_control(clnt, CLGET_FD, (void*)&fd)) {
        r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't obtain callback pipe");
        ABORT(R_FAILED);
    }
#else
    fd = *(int*)clnt->cl_private;
#endif

    if (getsockname(fd, &name, &namelen) < 0) {
        r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't get address: %s", strerror(errno));
        ABORT(R_FAILED);
    }

    if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &typelen) < 0) {
        r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't get socket type: %s", strerror(errno));
        ABORT(R_FAILED);
    }

    if ((srvfd = socket(name.sa_family, type, 0)) < 0) {
        r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't get socket: %s", strerror(errno));
        ABORT(R_FAILED);
    }

    if (name.sa_family == AF_INET)
         ((struct sockaddr_in*)&name)->sin_port = 0;

    if (bind(srvfd, &name, namelen) < 0) {
        r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't bind to socket: %s", strerror(errno));
        ABORT(R_FAILED);
    }

    if (listen(srvfd, CALLBACK_SERVER_BACKLOG) < 0) {
        r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't listen: %s", strerror(errno));
        ABORT(R_FAILED);
    }

    if (getsockname(srvfd, &name, &namelen) < 0) {
        r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't get server address: %s", strerror(errno));
        ABORT(R_FAILED);
    }

    connect_to_port = (name.sa_family == AF_INET) ? ntohs(((struct sockaddr_in*)&name)->sin_port) : 0;

    result = nr_reg_register_for_callbacks_1(connect_to_port, clnt);
    if ((!result) || *result) {
        r_log(NR_LOG_REGISTRY,LOG_WARNING, "Couldn't register for callbacks");
        ABORT(R_FAILED);
    }

    if ((cbfd = accept(srvfd, &name, &namelen)) < 0) {
        r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't accept: %s", strerror(errno));
        ABORT(R_FAILED);
    }

    /* non-blocking I/O required by nr_c_buf_nb_fill */
    if (fcntl(cbfd, F_SETFL, O_NONBLOCK) == -1)
        ABORT(R_FAILED);

    if ((r=nr_c_buf_create(&buf, 8192)))
        ABORT(r);

    if ((r=NR_ASYNC_WAIT(cbfd, NR_ASYNC_WAIT_READ, nr_reg_cb_async_wait_cb, buf)))
        ABORT(r);

    _status=0;
  abort:
    if (_status) {
       r_log(NR_LOG_REGISTRY, LOG_DEBUG, "Couldn't init notifications: %s", nr_strerror(_status));
       if (buf) nr_c_buf_destroy(&buf);
       if (cbfd != -1) close(cbfd);
       cbfd = -1;
    }
    if (srvfd != -1) close(srvfd);
    return(_status);
}

void
nr_reg_cb_async_wait_cb(int fd, int how, void *arg)
{
    c_buf *buf = (c_buf*)arg;
    int r;
    int i;
    char *line;

    r_log(NR_LOG_REGISTRY, LOG_DEBUG, "Entering registry client callback");

    if ((r=nr_c_buf_nb_fill(fd, buf, 8192))) {
      if (r == R_EOD) {
        r_log(NR_LOG_REGISTRY, LOG_WARNING, "Lost callback connection");

        if ((r=nr_reg_client_cb_init())) {
          r_log(NR_LOG_REGISTRY, LOG_EMERG, "Couldn't get callback connection");
//TODO: what's the right thing to do here????
          exit(1);
        }
      }
      else {
        r_log(NR_LOG_REGISTRY, LOG_EMERG, "Failed during notification");
//TODO: what's the right thing to do here????
        exit(1);
      }
    }
    else {
parse_next_line:
        for (i = 0; i < C_RLEFT(buf); ++i) {
          if (C_RPTR(buf)[i] == '\n' || C_RPTR(buf)[i] == '\r') {
            C_READ(buf, (UCHAR**)&line, i+1);

            line[i] = '\0';

            if (i > 0)
                (void)nr_reg_cb_process_line(line);

            goto parse_next_line;
          }
        }

        /* re-register callbacks connection with async_wait */
        NR_ASYNC_WAIT(fd, NR_ASYNC_WAIT_READ, nr_reg_cb_async_wait_cb, buf);
    }

    if (C_RLEFT(buf) == 0)
        C_ZERO(buf);
}

int
nr_reg_cb_process_line(char *line)
{
    int r,_status;
    int skip;
    char action;

    if (!strncasecmp("change ", line, 7)) {
        skip = 7;
        action = NR_REG_CB_ACTION_CHANGE;
    }
    else if (!strncasecmp("add ", line, 4)) {
        skip = 4;
        action = NR_REG_CB_ACTION_ADD;
    }
    else if (!strncasecmp("delete ", line, 7)) {
        skip = 7;
        action = NR_REG_CB_ACTION_DELETE;
    }
    else if (!strncasecmp("final ", line, 6)) {
        skip = 6;
        action = NR_REG_CB_ACTION_FINAL;
    }
    else {
        r_log(NR_LOG_REGISTRY, LOG_WARNING, "Couldn't raise event, illegal data '%s'", line);
        ABORT(R_BAD_DATA);
    }

    line += skip;

    if ((r=nr_reg_raise_event(line, action))) {
        r_log(NR_LOG_REGISTRY, LOG_DEBUG, "Couldn't raise event on '%s': ", line, nr_strerror(r));
        ABORT(r);
    }

    _status = 0;
  abort:
    return(_status);
}

