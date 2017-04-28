/**
   util_db.c

   
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
   

   ekr@rtfm.com  Wed Dec 26 17:49:58 2001
 */


static char *RCSSTRING __UNUSED__ ="$Id: util_db.c,v 1.2 2006/08/16 19:39:17 adamcain Exp $";

#ifdef AGNEW

#include <sys/types.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include "r_common.h"
#include "util.h"
#include "util_db.h"
#include "r_log.h"

#ifndef INCLUDE_DB_185
#include <db.h>
#else
#include <db_185.h>
#endif


int nra_db_create(base,name,dbp)
  char *base;
  char *name;
  util_db **dbp;
  {
    DB *db=0;
    char *fname=0;
    int r,_status;

    if(r=nr_get_filename(base,name,&fname)){
      ABORT(r);
    }

    if(!(db=dbopen(fname,O_RDWR | O_CREAT, 0600, DB_HASH, 0))){
      r_log(LOG_GENERIC,LOG_ERR,"Couldn't open file %s",fname);
      ABORT(R_NOT_FOUND);
    }

    /* Some OSes (Linux) require a sync here on first
       dbopen because they don't like empty DBs. This
       seems harmless*/
    db->sync(db,0);
    
    *dbp=(util_db *)db;

    _status=0;
  abort:
    RFREE(fname);
    return(_status);
  }

int nra_db_destroy(dbp)
  util_db **dbp;
  {
    DB *db;
    int _status;
    
    if(!dbp || !*dbp)
      return(0);

    db=(DB *)*dbp;

    if(db->close(db))
      ABORT(R_INTERNAL);

    _status=0;
  abort:
    return(_status);
  }

int nra_db_store_uint8(d,key,val)
  util_db *d;
  char *key;
  UINT8 val;
  {
    char buf[17];
    DB *db=(DB *)d;
    DBT k,v;
    int _status;
    int l;
    
    l=sprintf(buf,"%qx",val);
    
    k.data=(void *)key;
    k.size=strlen(key)+1;

    v.data=(void *)buf;
    v.size=l+1;

    if(db->put(db,&k,&v,0))
      ABORT(R_INTERNAL);

    _status=0;
  abort:
    return(_status);
  }

int nra_db_fetch_uint8(db,key,val)
  util_db *db;
  char *key;
  UINT8 *val;
  {
    int r,_status;
    DBT k,v;
    
    k.data=(void *)key;
    k.size=strlen(key)+1;

    r=db->get(db,&k,&v,0);

    if(r<0)
      ABORT(R_INTERNAL);
    if(r>0)
      ABORT(R_NOT_FOUND);

    *val=strtoq((char *)v.data,0,16);

    _status=0;
  abort:
    return(_status);
  }

int nra_db_sync(db)
  util_db *db;
  {
    int _status;
    
    if(db->sync(db,0))
      ABORT(R_INTERNAL);

    _status=0;
  abort:
    return(_status);
  }
#endif
