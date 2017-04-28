/**
   async_timer.c

   
   Copyright (C) 2004, Network Resonance, Inc.
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
   

   ekr@rtfm.com  Sun Feb 22 19:17:45 2004

   This code implements the timing wheel algorithms described
   in G. Varghese and A. Lauck, "Hashed and Hierarchical Timing
   Wheels", Proc. 11th ACM SOSP, 171-180

   and

   A.M. Costello and G. Varghese "Redesigning the BSD Callout
   and Timer Facilities"
*/


static char *RCSSTRING __UNUSED__ ="$Id: async_timer.c,v 1.7 2008/11/25 22:24:44 adamcain Exp $";

#include <sys/queue.h>
#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include "nr_common.h"
#include "r_common.h"
#include "r_time.h"
#include "async_wait.h"
#include "async_wait_int.h"
#include "async_timer.h"


#define INITIALIZE if(!initialized) NR_async_timer_init()

#define WHEEL_SIZE                  4096
#define WHEEL_MASK                  WHEEL_SIZE-1
#define MS_PER_ENTRY                1

typedef struct wheel_entry_ {
     TAILQ_HEAD(cb_q_head,callback_) cbs;
} wheel_entry;

static wheel_entry callwheel[WHEEL_SIZE];
static struct timeval first_time;
static UINT8 current_offset;
static wheel_entry *working_entry;
static int pending_deactivates;
static int timers_set;
static int initialized=0;

int NR_async_timer_init()
  {
    int i;

    NR_async_wait_init();
    
    if(initialized)
      return(0);

    for(i=0;i<WHEEL_SIZE;i++){
      TAILQ_INIT(&callwheel[i].cbs);
    }

    initialized=1;
    return(0);
  }

/* Note that you can call this after NR_async_timer_init()
   but before NR_async_timer_update_time(). If you do that,
   the behavior is as if you had called update_time() and
   then timer_set()
*/
int NR_async_timer_set(delay_ms,cb,cb_arg,function,line,handle)
  int delay_ms;
  NR_async_cb cb;
  void *cb_arg;
  char *function;
  int line;
  void **handle;
  {
    callback *cbb=0;
    int r,_status;
    int off;
    struct cb_q_head *queue;
    

    off=delay_ms/MS_PER_ENTRY;
    
    if(off==0) off++;

    if(r=nr_async_create_cb(cb,cb_arg,-1,current_offset+off,function,line,&cbb))
      ABORT(R_NO_MEMORY);
    
    queue=&callwheel[(current_offset+off)&WHEEL_MASK].cbs;
    TAILQ_INSERT_TAIL(queue,cbb,entry);
    cbb->backptr=queue;

    timers_set++;

    if(handle)
      *handle=(void *)cbb;
    
    _status=0;
  abort:
    return(_status);
  }

int NR_async_timer_cancel(cbv)
  void *cbv;
  {
    callback *cb=(callback *)cbv;

    assert(initialized);
    
    if(!cb)
      return(0);
    if(!cb->cb)
      return(0); /* Self-cancellation */
    
    if(cb->backptr==&working_entry->cbs){
      /* We're attempting to delete off the queue we're currently
         working on. Just mark the entry as deleted. 
      */
      cb->cb=0;
      pending_deactivates=1;
    }
    else{
      TAILQ_REMOVE((struct cb_q_head *)cb->backptr,cb,entry);
      
      timers_set--;
      
      nr_async_destroy_cb(&cb);
    }
    
    return(0);
  }

int NR_async_timer_update_time(tv)
  struct timeval *tv;
  {
    struct timeval diff;
    int r;
    UINT8 new_offset,offset;
    
    if(!initialized) {
      NR_async_timer_init();
    }

    if(!first_time.tv_sec){
      memcpy(&first_time,tv,sizeof(struct timeval));
      return(0);
    }

    
    if(r=r_timeval_diff(tv,&first_time,&diff))
      return(r);
    
    new_offset=r_timeval2int(&diff)/(1000*MS_PER_ENTRY);
    if(new_offset<current_offset) /* time must go forward */
      return(0);

    offset=current_offset+1;
    current_offset=new_offset;
      
    for(;offset<=current_offset;offset++){
      callback *cb,*cb2;

      working_entry=&callwheel[offset & WHEEL_MASK];
      pending_deactivates=0;
      
      cb=TAILQ_FIRST(&callwheel[offset & WHEEL_MASK].cbs);
      
      while(cb){
        cb2=TAILQ_NEXT(cb,entry);

        /* e.g. 1025 and 2049 both appear in slot 1
           This way we only execute the timeouts that are
           actually current. Still not a good idea to use
           timeouts way in the future */
        if(cb->resource <= current_offset){
          /* If the cb val is 0 the callback was cancelled */
          if(cb->cb){
            NR_async_cb cbf=cb->cb;

            cb->cb=0;
            
            cbf(0,-1,cb->arg);
          }
          timers_set--;
          TAILQ_REMOVE(&callwheel[offset & WHEEL_MASK].cbs,cb,entry);
          nr_async_destroy_cb(&cb);
        }
        
        cb=cb2;
      }

      /* If this is set then one of the callbacks deactivated an
         entry on this queue. We need to rewalk through the queue
         and purge the empties */
      if(pending_deactivates){
        cb=TAILQ_FIRST(&callwheel[offset & WHEEL_MASK].cbs);

        while(cb){
          cb2=TAILQ_NEXT(cb,entry);

          /* If the cb pointer is zero, it's been deactivated*/
          if(!cb->cb){
            timers_set--;
            TAILQ_REMOVE(&callwheel[offset & WHEEL_MASK].cbs,cb,entry);
            nr_async_destroy_cb(&cb);
          }

          cb=cb2;
        }
      }
    }

    working_entry=0;
    
    return(0);
  }

/* returns R_NOT_FOUND if there are no timeouts scheduled */
int NR_async_timer_next_timeout(int *delay_ms)
  {
    int i;
    callback *cb;
    int this_delay_ms;
    int min_delay_ms;

    min_delay_ms=INT_MAX;
    for(i=0;i<WHEEL_SIZE;i++){
      cb=TAILQ_FIRST(&callwheel[i].cbs);
      while(cb){
        if(cb->cb){
          this_delay_ms=cb->resource-(current_offset/MS_PER_ENTRY);
          if(this_delay_ms<min_delay_ms)
            min_delay_ms=this_delay_ms;
        }
        cb=TAILQ_NEXT(cb,entry);
      }
    }
    if(min_delay_ms==INT_MAX)
      return(R_NOT_FOUND);

    *delay_ms=min_delay_ms;

    return(0);
  }

int NR_async_timer_sanity_check_for_cb_deleted(cb,cb_arg)
  NR_async_cb cb;
  void *cb_arg;
  {
    int i;
    callback *cbp;
    
    for(i=0;i<WHEEL_SIZE;i++){
      cbp=TAILQ_FIRST(&callwheel[i].cbs);

      while(cbp){
        if(((!cb) || (cbp->cb==cb)) && cbp->arg==cb_arg){
          /* printf("Hanging cb %x %x\n",(int)cbp,(int)cb_arg); */
          abort();
        }
        cbp=TAILQ_NEXT(cbp,entry);
      }
    }

    return(0);
  }
