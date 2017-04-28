/**
   nr_plugin_handle.c

   
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
   

   ekr@networkresonance.com  Mon Jun 19 18:59:46 2006
 */


static char *RCSSTRING __UNUSED__ ="$Id: nr_plugin_int.c,v 1.3 2007/06/26 22:37:48 adamcain Exp $";

#include <nr_common.h>
#ifndef WIN32
#include <dlfcn.h>
#endif
#include <sys/queue.h>
#include "registry.h"
#include "nr_plugin.h"
#include "nr_plugin_int.h"

     
struct nr_plugin_handle_ {
     char *path;
     NR_plugin_def *info;
};

#define MAX_PLUGINS 512

static nr_plugin_handle nr_loaded_plugins[MAX_PLUGINS];
static int nr_loaded_plugin_ct;

static int nr_plugin_add(char *path, NR_plugin_def *info, nr_plugin_handle **pp)
  {
    int r,_status;
    char name[NR_PLUGIN_MAX_NAME_LEN];
    char version[NR_PLUGIN_MAX_NAME_LEN];
    nr_plugin_handle *p;
    NR_plugin_hook *hook;
    
    if(nr_loaded_plugin_ct > MAX_PLUGINS){
      r_log(LOG_GENERIC,LOG_EMERG,"Exceeding the maximum number of plugins");
      ABORT(R_INTERNAL);
    }

    p=nr_loaded_plugins+nr_loaded_plugin_ct;
    p->path=r_strdup(path);
    p->info=info;

    if(r=nr_plugin_get_name(p,name))
      ABORT(r);
    if(r=nr_plugin_get_version(p,version))
      ABORT(r);

    /* Execute init */
    if(r=nr_plugin_get_hook(p,"init",&hook)){
      if(r!=R_NOT_FOUND)
        ABORT(r);
    }
    else
      hook();
      
    r_log(LOG_GENERIC,LOG_DEBUG,"Adding plugin '%s' version '%s' path=%s",
      name,version,path);

    if(pp)
      *pp=p;
    
    nr_loaded_plugin_ct++;

    _status=0;
  abort:
    return(_status);
  }

int nr_plugin_add_plugins(NR_plugin_def **info)
  {
    int r,_status;

    while(*info){
      if(r=nr_plugin_add("STATIC",*info,0))
        ABORT(r);
      
      info++;
    }
    
    _status=0;
  abort:
    return(_status);
  }

int nr_plugin_load_dsos(char *prefix)
  {
    unsigned int count;
    unsigned int i;
    int r,_status;
    NR_registry plugins_reg;
    char number[10];
    char *path=0;

    if(r=NR_reg_make_registry(prefix,"plugins",plugins_reg))
      ABORT(r);
    if(r=NR_reg_get_child_count(plugins_reg,&count)){
      if(r!=R_NOT_FOUND){
        r_log_nr(LOG_GENERIC,LOG_ERR,r,"Couldn't get child count for %s.%s",prefix,"plugins");
      }
      ABORT(r);
    }

    for(i=0;i<count;i++){
      sprintf(number,"%d",i);
      if(r=NR_reg_alloc2_string(plugins_reg,number,&path)){
        r_log_nr(LOG_GENERIC,LOG_ERR,r,"Couldn't get %s.%s",plugins_reg,number);
        ABORT(r);
      }
            
      if(r=nr_plugin_load_dso(path, 0)){
        if(r==R_NOT_FOUND){
          RFREE(path); path=0;
          continue;
        }
        else{
          ABORT(r);
        }
      }
      RFREE(path); path=0;
    }

    _status=0;
  abort:
    RFREE(path);
    return(_status);
  }
  
#ifdef WIN32

int nr_plugin_load_dso(char *path, nr_plugin_handle **pp)
  {
    return(R_NOT_FOUND);
  }
#else
int nr_plugin_load_dso(char *path, nr_plugin_handle **pp)
  {
    void *lib=0;
    NR_plugin_def *plugin_def=0;
    int r,_status;
    
    if(!(lib=dlopen(path, RTLD_LAZY | RTLD_LOCAL))){
      r_log(LOG_GENERIC,LOG_ERR,"Couldn't load dso %s:%s",path,dlerror());
      ABORT(R_NOT_FOUND);
    }

    if(!(plugin_def=dlsym(lib,"plugin_def"))){
      r_log(LOG_GENERIC,LOG_ERR,"Couldn't get plugin_def from dso %s:%s",path,dlerror());
      ABORT(R_NOT_FOUND);
    }
    
    if(r=nr_plugin_add(path,plugin_def,pp))
      ABORT(r);
                
    _status=0;
  abort:
    return(_status);
  }
#endif

int nr_plugin_get_name(nr_plugin_handle *p, char *name)
  {
    strncpy(name,p->info->name,NR_PLUGIN_MAX_NAME_LEN);

    return(0);
  }

int nr_plugin_get_version(nr_plugin_handle *p, char *version)
  {
    strncpy(version,p->info->version,NR_PLUGIN_MAX_NAME_LEN);

    return(0);
  }

int nr_plugin_get_hook(nr_plugin_handle *p,char *type,NR_plugin_hook **hookp)
  {
    NR_plugin_hook_def *hook;

    for(hook=p->info->hooks;hook->type;hook++){
      if(!strcmp(type,hook->type)){
        *hookp=hook->func;
        return(0);
      }
    }

    return(R_NOT_FOUND);
  }

int nr_plugin_execute_hooks(char **types)
  {
    int i,j;
    int r,_status;
    NR_plugin_hook *hook;
    char plugin_name[NR_PLUGIN_MAX_NAME_LEN];
    
    for(i=0;i<nr_loaded_plugin_ct;i++){
      if(r=nr_plugin_get_name(&nr_loaded_plugins[i],plugin_name))
        ABORT(r);
      r_log(LOG_GENERIC,LOG_DEBUG,"Executing hooks for plugin '%s'",plugin_name);
      
      for(j=0;types[j];j++){
        if(r=nr_plugin_get_hook(&nr_loaded_plugins[i],types[j],&hook)){
          if(r!=R_NOT_FOUND){
            r_log(LOG_GENERIC,LOG_ERR,"Error trying to get hook '%s' from plugin '%s'",types[j], plugin_name);
            ABORT(r);
          }

          /* Skip hooks that aren't found */
          r_log(LOG_GENERIC,LOG_DEBUG,"No hook '%s'",types[j]);
          continue;
        }

        r_log(LOG_GENERIC,LOG_DEBUG,"Executing hook '%s'",types[j]);
        if(r=hook()){
          r_log_nr(LOG_GENERIC,LOG_ERR,r,"hook '%s' from plugin '%s' failed",types[j], plugin_name);
        }
      }
    }

    _status=0;
  abort:
    return(_status);
  }

int nr_plugin_get_plugin_ct(int *ct)
  {
    *ct=nr_loaded_plugin_ct;

    return(0);
  }

int nr_plugin_get_plugin(int index, nr_plugin_handle **pp)
  {
    if(index>=nr_loaded_plugin_ct)
      ERETURN(R_BAD_ARGS);

    *pp=&nr_loaded_plugins[index];
    
    return(0);
  }
