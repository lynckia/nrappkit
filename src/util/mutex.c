/**
   mutex.c

   
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
   

   briank@networkresonance.com  Tue Jun 27 11:15:17 PDT 2006
 */


static char *RCSSTRING __UNUSED__ ="$Id: mutex.c,v 1.3 2007/06/26 22:37:56 adamcain Exp $";

#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/file.h>

#include "nr_common.h"
#include "r_errors.h"
#include "r_common.h"
#include "r_macros.h"
#include "r_log.h"
#include "mutex.h"


int
nr_util_create_mutex(nr_util_mutex *m, char *path, mode_t mode)
{
    int r, _status;

    if ((r=open(path, O_RDWR|O_CREAT|O_TRUNC, mode)) < 0) {
        r_log_e(LOG_COMMON, LOG_ERR, "Couldn't allocate lock '%s'", path);
        ABORT(R_FAILED);
    }

    *m = r;

    _status=0;
  abort:
    return(_status);
}

int
nr_util_destroy_mutex(nr_util_mutex *m)
{
    int r,_status;

    if ((r=close(*m)) < 0) {
        r_log_e(LOG_COMMON, LOG_ERR, "Couldn't deallocate lock");
        ABORT(R_FAILED);
    }

    *m = 0;

    _status=0;
  abort:
    return(_status);
}

int
nr_util_acquire_mutex(nr_util_mutex m)
{
    int r,_status;

    if ((r=flock(m, LOCK_EX)) < 0) {
        r_log_e(LOG_COMMON, LOG_ERR, "Couldn't acquire lock");
        ABORT(R_FAILED);
    }

    _status=0;
  abort:
    return(_status);
}

int
nr_util_release_mutex(nr_util_mutex m)
{
    int r,_status;

    if ((r=flock(m, LOCK_UN)) < 0) {
        r_log_e(LOG_COMMON, LOG_ERR, "Couldn't release lock");
        ABORT(R_FAILED);
    }

    _status=0;
  abort:
    return(_status);
}

