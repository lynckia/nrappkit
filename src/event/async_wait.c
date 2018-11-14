/**
   NR_async_wait.c


   Copyright (C) 2001-2003, Network Resonance, Inc.
   Copyright (C) 2006, Network Resonance, Inc.
   All Rights Reserved

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. Neither the name of Network Resonance, Inc. nor the name of any
      contributors to this software may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.


   ekr@rtfm.com  Thu Dec 20 20:14:44 2001
 */


static char *RCSSTRING __UNUSED__ ="$Id: async_wait.c,v 1.7 2008/01/23 23:06:50 adamcain Exp $";

#include <sys/queue.h>
#include <sys/types.h>
#ifdef USE_EPOLL
#include <sys/epoll.h>
#endif
#ifndef WIN32
#include <unistd.h>
#endif
#include <assert.h>
#include <signal.h>
#include <string.h>
#include "nr_common.h"
#include "r_common.h"
#include "async_wait.h"
#include "async_wait_int.h"

static TAILQ_HEAD(cb_q_head,callback_) q_head,free_q,w_q_head;
static callback *w_q_n1;
static int initialized=0;

#define SOCKET_VEC_SIZE 20000
#define ASYNC_WAIT_TYPES 2

#ifdef USE_EPOLL
#define MAX_EVENTS 500
struct epoll_event ev, events[MAX_EVENTS];
static int epollfd;
#endif

#ifdef WIN32
typedef struct sock_cbs_ {
    SOCKET     sock;
    callback  *cbs[ASYNC_WAIT_TYPES];
    TAILQ_ENTRY(sock_cbs_)  entry;
} sock_cbs;

static TAILQ_HEAD(sock_cbs_head,sock_cbs_) sock_cbs_q;

#else
static callback *socket_vec[SOCKET_VEC_SIZE][ASYNC_WAIT_TYPES];
#endif

#define INITIALIZE if(!initialized) NR_async_wait_init()


int NR_async_wait_init()
  {
#ifndef WIN32
    int i,j;
#endif

    if(initialized)
      return(0);

    TAILQ_INIT(&q_head);
    TAILQ_INIT(&free_q);

#ifdef WIN32
    TAILQ_INIT(&sock_cbs_q);
#else
    for(i=0;i<SOCKET_VEC_SIZE;i++){
      for(j=0;j<ASYNC_WAIT_TYPES;j++){
        socket_vec[i][j]=0;
      }
    }
#endif

#ifdef USE_EPOLL
  epollfd = epoll_create1(0);
  if (epollfd == -1) {
    return (-1);
  }
#endif

    initialized=1;

    return(0);
  }

int NR_async_wait(sock,how,cb,cb_arg,func,line)
  NR_SOCKET sock;
  int how;
  NR_async_cb cb;
  void *cb_arg;
  char *func;
  int line;
  {
    callback *cbb=0;
    int r,_status;

#ifdef WIN32
    sock_cbs *sc;

    INITIALIZE;

    if(sock<0)
      ABORT(R_BAD_ARGS);
    if(how>=ASYNC_WAIT_TYPES)
      ABORT(R_BAD_ARGS);

  //for(sc=TAILQ_FIRST(&sock_cbs_q); sc; sc=TAILQ_NEXT(sc)){
    TAILQ_FOREACH(sc, &sock_cbs_q, entry) {
      if(sock == sc->sock)
        break;
    }
    if(sc){
      if(sc->cbs[how])
        nr_async_destroy_cb(&sc->cbs[how]);
    }
    else{
      if(!(sc=(sock_cbs *)RCALLOC(sizeof(sock_cbs))))
        ABORT(R_NO_MEMORY);
      sc->sock=sock;
      TAILQ_INSERT_TAIL(&sock_cbs_q,sc,entry);
    }
    if(r=nr_async_create_cb(cb,cb_arg,how,(int)sock,func,line,&cbb))
      ABORT(R_NO_MEMORY);
    sc->cbs[how]=cbb;

#else
    INITIALIZE;

    if(sock<0)
      ABORT(R_BAD_ARGS);
    if(sock>=SOCKET_VEC_SIZE)
      ABORT(R_BAD_ARGS);
    if(how>=ASYNC_WAIT_TYPES)
      ABORT(R_BAD_ARGS);

    if(socket_vec[sock][how])
      nr_async_destroy_cb(&socket_vec[sock][how]);
    if(r=nr_async_create_cb(cb,cb_arg,how,sock,func,line,&cbb))
      ABORT(R_NO_MEMORY);

#ifdef USE_EPOLL
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev) == -1) {
      if(errno != EEXIST) {
        ABORT(R_INTERNAL);
      }
    }
#endif
    socket_vec[sock][how]=cbb;
#endif

    _status=0;
  abort:
    return(_status);
  }

int NR_async_cancel(sock,how)
  NR_SOCKET sock;
  int how;
  {
    int _status;
    callback *c;
#ifdef WIN32
    sock_cbs *sc;

    INITIALIZE;

    if(sock<0)
      ABORT(R_BAD_ARGS);
    if(how>=ASYNC_WAIT_TYPES)
      ABORT(R_BAD_ARGS);

 // for(sc=TAILQ_FIRST(&sock_cbs_q); sc; sc=TALIQ_NEXT(sc)){
    TAILQ_FOREACH(sc, &sock_cbs_q, entry) {
      if(sock == sc->sock)
        break;
    }
    if(sc && sc->cbs[how])
      nr_async_destroy_cb(&sc->cbs[how]);
#else
    INITIALIZE;

    if(sock<0)
      ABORT(R_BAD_ARGS);
    if(sock>=SOCKET_VEC_SIZE)
      ABORT(R_BAD_ARGS);
    if(how>=ASYNC_WAIT_TYPES)
      ABORT(R_BAD_ARGS);

    if(socket_vec[sock][how])
      nr_async_destroy_cb(&socket_vec[sock][how]);
#endif

#ifdef USE_EPOLL
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, sock, &ev) == -1) {
      if(errno != ENOENT) {
        ABORT(R_INTERNAL);
      }
    }
#endif

    /* Also cancel anything queued */
    if(w_q_n1) {
      c=TAILQ_NEXT(w_q_n1,entry);
      while(c){
        if(c->resource==sock && c->how==how)
          break;

        c=TAILQ_NEXT(c,entry);
      }
      if(c)
        c->cb=0;
    }

    _status=0;
  abort:
    return(_status);
  }

int NR_async_schedule(cb,cb_arg,func,line)
  NR_async_cb cb;
  char *func;
  int line;
  void *cb_arg;
  {
    callback *cbb=0;
    int r,_status;

    INITIALIZE;
    if(r=nr_async_create_cb(cb,cb_arg,-1,-1,func,line,&cbb))
      ABORT(R_NO_MEMORY);
    TAILQ_INSERT_TAIL(&q_head,cbb,entry);

    _status=0;
  abort:
    return(_status);
  }


int NR_async_event_wait(eventsp)
  int *eventsp;
  {
    return(NR_async_event_wait2(eventsp,0));
  }

#ifdef USE_EPOLL
int NR_async_event_wait2(eventsp,tv)
  int *eventsp;
  struct timeval *tv;
  {
    int r,_status;
    callback *n2;
    int millis = tv ? (tv->tv_sec * (uint64_t)1000) + (tv->tv_usec / 1000) : -1;
    int n;
    int evts;

    INITIALIZE;

    r = epoll_wait(epollfd, events, MAX_EVENTS, TAILQ_EMPTY(&q_head) ? millis : 10);
    if (r == -1) {
      ABORT(R_INTERNAL);
    }
    int reads = 0;
    if(r>0){
      for(n=0; n <= r; n++){
        int i = events[n].data.fd;
        if (socket_vec[i][0]) {
          if(events[n].events & EPOLLIN) {
            TAILQ_INSERT_TAIL(&q_head,socket_vec[i][0],entry);
            socket_vec[i][0]=0;
            reads++;
          }
        }
        if (socket_vec[i][1]) {
          if(events[n].events & EPOLLOUT) {
            TAILQ_INSERT_TAIL(&q_head,socket_vec[i][1],entry);
            socket_vec[i][1]=0;
          }
        }
      }
    } else {
      ABORT(R_EOD);
    }


    /* Now make a temporary list and then zero the working list */
    //memcpy(&w_q_head,&q_head,sizeof(q_head));
    w_q_head.tqh_first=q_head.tqh_first;
    w_q_head.tqh_last=q_head.tqh_last;

    TAILQ_INIT(&q_head);

      /* Finally, walk through the list and fire the callbacks */
    evts=0;
    w_q_n1=TAILQ_FIRST(&w_q_head);
    while(w_q_n1){
      n2=TAILQ_NEXT(w_q_n1,entry);
      evts++;
#ifdef NR_DEBUG_ASYNC
      fprintf(stderr,"Firing %s(%d,%d,%d)\n","UNKNOWN",w_q_n1->resource,w_q_n1->how,n1->arg);
#endif
      if(w_q_n1->cb){
        w_q_n1->cb(w_q_n1->resource,w_q_n1->how,w_q_n1->arg);
      }
      nr_async_destroy_cb(&w_q_n1);
      w_q_n1=n2;
    }
    if (reads == 0) {
      ABORT(R_EOD);
    }
    *eventsp=evts;
    _status=0;
  abort:
    return(_status);
  }

#else
int NR_async_event_wait2(eventsp,tv)
  int *eventsp;
  struct timeval *tv;
  {
    int events=0;
    fd_set fd_read,fd_write;
    int maxfd=-1;
    int r,_status;
    callback *n2;
    struct timeval no_time={0,0}; /* No wait */
#ifdef WIN32
    sock_cbs *sc;
#else
    int i;
#endif

    INITIALIZE;

    FD_ZERO(&fd_read);
    FD_ZERO(&fd_write);

#ifdef WIN32
    TAILQ_FOREACH(sc, &sock_cbs_q, entry) {
      if(sc->cbs[0]) { FD_SET(sc->sock,&fd_read); maxfd++;}
      if(sc->cbs[1]) { FD_SET(sc->sock,&fd_write); maxfd++;}
    }
#else
    for(i=0;i<SOCKET_VEC_SIZE;i++){
      if(socket_vec[i][0]) {FD_SET(i,&fd_read); maxfd=i;}
      if(socket_vec[i][1]) {FD_SET(i,&fd_write); maxfd=i;}
    }
#endif

    if(maxfd==-1){
      /* Hibernating with no callbacks scheduled is nuts! */
      if(TAILQ_EMPTY(&q_head))
        ABORT(R_EOD);
    }
    else {
      r=select(maxfd+1,&fd_read,&fd_write,0,
        TAILQ_EMPTY(&q_head)?tv:&no_time);

      /* We timed out */
      if(r==0){
        if(tv && TAILQ_EMPTY(&q_head) && (tv->tv_sec || tv->tv_usec))
          ABORT(R_WOULDBLOCK);
      }

#ifdef WIN32
      if(r==SOCKET_ERROR){
        if(WSAGetLastError()!=WSAEINTR)
          ABORT(R_INTERNAL);

        r=0; /* Set this so that we don't try to fire IO callbacks */
      }
      /* Build the list of callbacks to fire */
      if(r>0){
        TAILQ_FOREACH(sc, &sock_cbs_q, entry) {
          if(sc->cbs[0] && FD_ISSET(sc->sock,&fd_read)){
            TAILQ_INSERT_TAIL(&q_head,sc->cbs[0],entry);
            sc->cbs[0]=0;
          }
          if(sc->cbs[1] && FD_ISSET(sc->sock,&fd_write)){
            TAILQ_INSERT_TAIL(&q_head,sc->cbs[1],entry);
            sc->cbs[1]=0;
          }
        }
      }
#else
      if(r<0){
        if(errno!=EINTR)
          ABORT(R_INTERNAL);

        r=0; /* Set this so that we don't try to fire IO callbacks */
      }

      /* Build the list of callbacks to fire */
      if(r>0){
        for(i=0;i<=maxfd;i++){
          if(socket_vec[i][0]){
            if(FD_ISSET(i,&fd_read)){
              TAILQ_INSERT_TAIL(&q_head,socket_vec[i][0],entry);
              socket_vec[i][0]=0;
            }
          }
          if(socket_vec[i][1]){
            if(FD_ISSET(i,&fd_write)){
              assert(socket_vec[i][1]!=0);
              TAILQ_INSERT_TAIL(&q_head,socket_vec[i][1],entry);
              socket_vec[i][1]=0;
            }
          }
        }
      }
#endif
    }

    /* Now make a temporary list and then zero the working list */
    //memcpy(&w_q_head,&q_head,sizeof(q_head));
    w_q_head.tqh_first=q_head.tqh_first;
    w_q_head.tqh_last=q_head.tqh_last;

    TAILQ_INIT(&q_head);

      /* Finally, walk through the list and fire the callbacks */
    events=0;
    w_q_n1=TAILQ_FIRST(&w_q_head);
    while(w_q_n1){
      n2=TAILQ_NEXT(w_q_n1,entry);
      events++;
#ifdef NR_DEBUG_ASYNC
      fprintf(stderr,"Firing %s(%d,%d,%d)\n","UNKNOWN",w_q_n1->resource,w_q_n1->how,n1->arg);
#endif
      if(w_q_n1->cb){
        w_q_n1->cb(w_q_n1->resource,w_q_n1->how,w_q_n1->arg);
      }
      nr_async_destroy_cb(&w_q_n1);
      w_q_n1=n2;
    }

    *eventsp=events;
    _status=0;
  abort:
    return(_status);
  }
#endif

int nr_async_create_cb(cb,arg,how,resource,func,line,cbp)
  NR_async_cb cb;
  void *arg;
  int how;
  int resource;
  char *func;
  int line;
  callback **cbp;
  {
    callback *cbb=0;
    int _status;

    if(TAILQ_EMPTY(&free_q)){
      if(!(cbb=(callback *)RMALLOC(sizeof(callback))))
        ABORT(R_NO_MEMORY);
    }
    else{
      cbb=TAILQ_FIRST(&free_q);
      TAILQ_REMOVE(&free_q,cbb,entry);
    }
#ifdef SANITY_CHECKS
    memset(cbb,0,sizeof(callback));
#endif

    cbb->cb=cb;
    cbb->arg=arg;
    cbb->how=how;
    cbb->resource=resource;
    cbb->func=func;
    cbb->line=line;

    *cbp=cbb;
    _status=0;
  abort:
    if(_status){
      nr_async_destroy_cb(&cbb);
    }
    return(_status);
  }

int nr_async_destroy_cb(cbp)
  callback **cbp;
  {
    callback *cb;

    if(!cbp || !*cbp)
      return(0);

    cb=*cbp;
    *cbp=0;

#ifdef SANITY_CHECKS
    memset(cb,0xff,sizeof(callback));
#endif

    /* Now put it on the free queue */
    TAILQ_INSERT_TAIL(&free_q,cb,entry);

    return(0);
  }
