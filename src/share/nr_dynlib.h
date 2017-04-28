/**
   nr_dynlib.h

   
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
   

   ekr@networkresonance.com  Fri Jan  6 14:07:47 2006
 */


#ifndef _nr_dynlib_h
#define _nr_dynlib_h

#ifdef LINUX_FEDORA
#ifndef __USE_GNU
#define UNDEF__USE_GNU
#define __USE_GNU
#endif
#endif
#include <dlfcn.h>
#ifdef UNDEF__USE_GNU
#undef __USE_GNU
#endif

#include "nr_common.h"


#define DEFINE_NR_GET_DYNAMIC_OBJECT_NAME \
static int nr_get_dynamic_object_name(char **name); \
\
static int nr_get_dynamic_object_name(char **name) \
  { \
    int _status; \
    Dl_info info;\
    \
    if(!dladdr(nr_get_dynamic_object_name, &info)){                    \
      r_log(LOG_GENERIC, LOG_ERR, "Couldn't get dynamic object name %s",dlerror()); \
      ABORT(R_INTERNAL);                                                          \
    } \
\
    *name=(char *)info.dli_fname; \
\
    _status=0;\
  abort: \
    return(_status);\
  }


#endif

