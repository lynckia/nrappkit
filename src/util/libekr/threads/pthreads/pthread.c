/**
   pthread.c

   
   Copyright (C) 1999, RTFM, Inc.
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
   

   ekr@rtfm.com  Tue Feb 23 15:08:03 1999
 */
  


static char *RCSSTRING __UNUSED__ ="$Id: pthread.c,v 1.2 2006/08/16 19:39:18 adamcain Exp $";

#ifdef LINUX
#define _GNU_SOURCE 1
#endif 

#include <r_common.h>
#include <r_thread.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>

static int thread_count=0;

#ifdef LINUX_FEDORA
pthread_mutex_t cond_mutex;
#else  /* maybe this is only needed for Darwin? */
pthread_mutex_t cond_mutex={0};
#endif
static int cond_mutex_inited=0;

typedef struct {
     void (*func) (void *);
     void *arg;
} helper;


static void *r_thread_real_create (void *arg);
static int _init_cond_mutex(void);

static void *r_thread_real_create(arg)
  void *arg;
  {
    helper *h;

    h=(helper *)arg;

    thread_count++;
    
    h->func(h->arg);

    thread_count--;
    
    free(h);
    return(0);
  }
                     
int r_thread_fork(func,arg,id)
  void (*func) (void *);
  void *arg;
  r_thread *id;
  {
    pthread_t thread;
    helper *h;
    int r,_status;
    //pthread_attr_t ta;

    h=(helper *)malloc(sizeof(helper));
    
    h->func=func;
    h->arg=arg;
    //if(r= pthread_attr_init(&ta))
    //  ABORT(R_INTERNAL);
   
    //if(r= pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED))
    //  ABORT(R_INTERNAL);

    //if(r=pthread_create(&thread,&ta,r_thread_real_create,(void *)h))
    if(r=pthread_create(&thread,0,r_thread_real_create,(void *)h))
      ABORT(R_INTERNAL);

#ifdef LINUX_FC3
// TODO: find out why this is necessary on FC3
sleep(1);
#endif
    *id = (r_thread)thread ;
    
    _status=0;
  abort:
    return(_status);
  }

int r_thread_yield()
  {
#ifdef DARWIN
    /* pthread_yield is not found on Darwin, and is a non-POSIX pthread function.
     * we'll use sched_yield instead
     */
    sched_yield();
#else
    pthread_yield();
#endif

    return(0);
  }

int r_thread_self()
{
   return (int)pthread_self();
}


int r_thread_exit()
  {
    thread_count--;
    pthread_exit(0);
    return(0);
  }

int r_thread_wait_last()
  {
    do {
#ifdef DARWIN
      /* pthread_yield is not found on Darwin, and is a non-POSIX pthread function.
       * we'll use sched_yield instead
       */
      sched_yield();
#else
      pthread_yield();
#endif
      usleep(10000);
//      DBG((0,"%d threads left",thread_count));
    } while (thread_count);

    return(0);
  }

int r_rwlock_create(lockp)
  r_rwlock **lockp;
  {
    pthread_rwlock_t *lock;
    int r;

    if(!(lock=(pthread_rwlock_t *)malloc(sizeof(pthread_rwlock_t))))
      ERETURN(R_NO_MEMORY);
    
    if(r=pthread_rwlock_init(lock,0))
      ERETURN(R_INTERNAL);

    *lockp=(void *)lock;
    return(0);
  }

int r_rwlock_destroy(lock)
  r_rwlock **lock;
  {
    pthread_rwlock_t *plock;

    if(!lock || !*lock)
      return(0);

    plock=(pthread_rwlock_t *)(*lock);
    
    pthread_rwlock_destroy(plock);

    return(0);
  }

int r_rwlock_lock(lock,action)
  r_rwlock *lock;
  int action;
  {
    pthread_rwlock_t *plock;
    int r,_status;
    
    plock=(pthread_rwlock_t *)lock;

    switch(action){
      case R_RWLOCK_UNLOCK:
	if(r=pthread_rwlock_unlock(plock))
	  ABORT(R_INTERNAL);
	break;
      case R_RWLOCK_RLOCK:
	if(r=pthread_rwlock_rdlock(plock))
	  ABORT(R_INTERNAL);
	break;
      case R_RWLOCK_WLOCK:
	if(r=pthread_rwlock_wrlock(plock))
	  ABORT(R_INTERNAL);
	break;
      default:
	ABORT(R_BAD_ARGS);
    }

    _status=0;
  abort:
    return(_status);
  }

static int _init_cond_mutex(void)
  {
    int r;

    if(cond_mutex_inited)
      return(0);

    if((r=pthread_mutex_init(&cond_mutex,0))){
      ERETURN(R_INTERNAL);
    }

    cond_mutex_inited=1;

    return(0);
  }

int r_cond_init(cond)
  r_cond *cond;
{
   int _status;
   pthread_cond_t *ptcond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
   
   if(pthread_cond_init(ptcond, NULL))
      ABORT(R_INTERNAL);

   *cond = (r_cond)ptcond;
   
   _status=0;
  abort:
   return(_status);
}

int r_cond_wait(cond)
   r_cond cond;
{
   int _status;
   pthread_cond_t *ptcond = (pthread_cond_t *)cond;

   if(!cond_mutex_inited){
     if(_init_cond_mutex())
       ABORT(R_INTERNAL);
   }

   if(pthread_mutex_lock(&cond_mutex))
      ABORT(R_INTERNAL);
   
   if(pthread_cond_wait(ptcond, &cond_mutex))
   {
      pthread_mutex_unlock(&cond_mutex);
      ABORT(R_INTERNAL);
   }

   if(pthread_mutex_unlock(&cond_mutex))
      ABORT(R_INTERNAL);

   
   _status=0;
  abort:
   return(_status);
}

int r_cond_signal(cond)
   r_cond cond;
{
   int r, _status;
   pthread_cond_t *ptcond = (pthread_cond_t *)cond;
   
   if(!cond_mutex_inited){
     if(_init_cond_mutex())
       ABORT(R_INTERNAL);
   }

   if(pthread_mutex_lock(&cond_mutex))
      ABORT(R_INTERNAL);
   
   if(r=pthread_cond_signal(ptcond))
   {
      pthread_mutex_unlock(&cond_mutex);
      ABORT(R_INTERNAL);
   }
   
   if(pthread_mutex_unlock(&cond_mutex))
      ABORT(R_INTERNAL);
   
   _status=0;
  abort:
   return(_status);
}
