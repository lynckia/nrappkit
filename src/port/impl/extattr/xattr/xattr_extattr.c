/**
   xattr_extattr.c

   
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
   

   ekr@networkresonance.com  Tue May  2 08:31:00 2006
 */


static __UNUSED__ char *RCSSTRING="$Id: xattr_extattr.c,v 1.2 2006/08/16 19:39:12 adamcain Exp $";

#include <sys/types.h>
#ifdef DARWIN
#include <sys/xattr.h>
#else
#include <attr/xattr.h>
#endif
#include <errno.h>
#include <nr_common.h>
#include <r_log.h>
#include <nr_extattr.h>

int nr_get_extattr_file(const char *path, int attrnamespace,
  const char *attrname, void *data, size_t *length, size_t nbytes)
  {
    int r,_status;
    char *namespace;
    char name[1024];
    
    switch(attrnamespace){
      case NR_EXTATTR_USER:
        namespace="user";
        break;
      case NR_EXTATTR_SYSTEM:
        namespace="system";
        break;
      default:
        ABORT(R_BAD_ARGS);
    }

    if((strlen(namespace) + strlen(attrname) + 2)>sizeof(name))
      ABORT(R_BAD_ARGS);

    sprintf(name,"%s.%s",namespace,attrname);

#ifdef DARWIN
    if((r=getxattr(path,name,data,nbytes,0,0))<0)
#else
    if((r=getxattr(path,name,data,nbytes))<0)
#endif
    {
      r_log(LOG_GENERIC,LOG_WARNING,
        "Couldn't get attr %s for file %s: %s",name,path,strerror(errno));

      switch(errno){
        case ENOATTR:
        case ENOENT:
          ABORT(R_NOT_FOUND);
          break;
        case ERANGE:
          ABORT(R_BAD_ARGS);
          break;
        default:
          ABORT(R_INTERNAL);
      }
    }

    *length=r;
    
    _status=0;
  abort:
    return(_status);
  }
