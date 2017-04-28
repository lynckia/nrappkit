/**
   mem_util.c

   
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
   

   ekr@rtfm.com  Wed Mar  3 12:10:39 2004
 */


static char *RCSSTRING __UNUSED__ ="$Id: mem_util.c,v 1.3 2007/06/26 22:37:56 adamcain Exp $";

#include <sys/types.h>
#include "r_common.h"
#include "r_macros.h"
#include "mem_util.h"

#ifdef USE_DLMALLOC
struct mallinfo {
  int arena;    /* total space allocated from system */
  int ordblks;  /* number of non-inuse chunks */
  int smblks;   /* unused -- always zero */
  int hblks;    /* number of mmapped regions */
  int hblkhd;   /* total space in mmapped regions */
  int usmblks;  /* unused -- always zero */
  int fsmblks;  /* unused -- always zero */
  int uordblks; /* total allocated space */
  int fordblks; /* total non-inuse space */
  int keepcost; /* top-most, releasable (via malloc_trim) space */
};

struct mallinfo mallinfo(void);
void malloc_stats(void);
#endif


static UINT4 mem_size_max;
static UINT4 mem_inuse_max;

#if 0
/* Use SBRK */
int nr_memory_status_init()
  {
    if(!initted){
      orig_brk=sbrk(0);
    }
    initted=1;

    return(0);
  }

int nr_memory_status_update(currentp,maxp,inusep,inuse_maxp)
  UINT4 *currentp;
  UINT4 *maxp;
  UINT4 *inusep;
  UINT4 *inuse_maxp;
  {
    int current;

    current=sbrk(0)-orig_brk;

    if(current>mem_size_max){
      mem_size_max=current;
    }

    *currentp=current;
    *maxp=mem_size_max;

    return(0);
  }
       
#else
int nr_memory_status_init()
  {
    return(0);
  }

int nr_memory_status_update(currentp,maxp,inusep,inusemaxp)
  UINT4 *currentp;
  UINT4 *maxp;
  UINT4 *inusep;
  UINT4 *inusemaxp;
  {
    UINT4 current;
    UINT4 inuse;
    
#ifndef USE_DLMALLOC
    r_mem_get_usage(&current);
    inuse=current;
#else
    {
      struct mallinfo info;

      info=mallinfo();
      current=(UINT4)info.arena+info.hblkhd;
      inuse=(UINT4)info.uordblks+info.hblkhd;
    }
#endif
    
    if(current>mem_size_max){
      mem_size_max=current;
    }
    if(inuse>mem_inuse_max){
      mem_inuse_max=inuse;
    }

    *currentp=current;
    *maxp=mem_size_max;
    *inusep=inuse;
    *inusemaxp=mem_inuse_max;

    return(0);
  }


int nr_memory_print_stats()
  {
#ifdef USE_DLMALLOC    
    struct mallinfo info;
    
    info=mallinfo();

    malloc_stats();

    fprintf(stderr,"Unused blocks: %d\n",info.fordblks);
    fprintf(stderr,"Trimmable: %d\n",info.keepcost);
#endif
    return(0);
  }

#endif
