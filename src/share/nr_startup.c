/*
 *    
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
 */
#include "nr_common.h"
#include <registry.h>
#include <r_log.h>
#include <util.h>
#include "nr_startup.h"
#include "nr_plugin.h"
#include "nr_plugin_int.h"
#ifndef NR_NO_API_FORCE
/* Force the plugin API to load */
#include <api_force.h>
#endif

extern NR_plugin_def *nr_static_plugins[];

int nr_app_startup(char *progname,UINT4 flags,int *log_facility,void **app_stats,void **memory_stats)
  {
    int r,_status;
    int facility=0;
    int init_reg=0;
    void *reg_mode=0;
    
    if(flags & NR_APP_STARTUP_REGISTRY_REMOTE){
      if(flags & NR_APP_STARTUP_REGISTRY_LOCAL){
        r_log(facility,LOG_EMERG,"Can't open registry both remote and local");
        ABORT(R_INTERNAL);
      }

      init_reg=1;
      reg_mode=NR_REG_MODE_REMOTE;
    }

    if(flags & NR_APP_STARTUP_REGISTRY_LOCAL){
      init_reg=1;
      reg_mode=NR_REG_MODE_LOCAL;
    }

    if(init_reg){
      if(r=NR_reg_init(reg_mode)){
        r_log(facility,LOG_EMERG,"Couldn't initialize registry");
        ABORT(r);
      }
    }

    if(flags & NR_APP_STARTUP_INIT_LOGGING){
      r_log_init();
      
      if(r=r_log_register(progname,&facility))
        ABORT(r);

      *log_facility=facility;
    }

    if(flags & NR_APP_STARTUP_STATS){
      if(r=NR_stats_startup(progname, CAPTURE_USER, nr_errprintf_log2, 0))
        ABORT(r);
    }

    if(flags & NR_APP_STARTUP_STATS_APP){
      NR_stats_app *app_stats_a;
      
      if(r=NR_stats_get(NULL, NR_stats_type_app, NR_STATS_CREATE, app_stats))
        ABORT(R_INTERNAL);
      if(flags & NR_APP_STARTUP_STATS_RESET)
        NR_stats_type_app->reset(*app_stats);
      app_stats_a=*app_stats;
      strcpy(app_stats_a->version,nr_revision_number());
    }
    
    if(flags & NR_APP_STARTUP_STATS_MEMORY){
      if(r=NR_stats_get(NULL, NR_stats_type_memory, NR_STATS_CREATE, memory_stats))
        ABORT(R_INTERNAL);
      if(flags & NR_APP_STARTUP_STATS_RESET)
        NR_stats_type_memory->reset(*memory_stats);
    }

    if(flags & NR_APP_STARTUP_PLUGIN) {
      char *types[2];
      
      types[1]=0;
      
      if(r=nr_plugin_add_plugins(nr_static_plugins))
        ABORT(r);
      if(r=nr_plugin_load_dsos("nr_plugins")){
        if(r!=R_NOT_FOUND)
          ABORT(r);
      }

      if(flags & NR_APP_STARTUP_PLUGIN_REGISTER_LOGGING){
        types[0]="register_logging";
        
        if(r=nr_plugin_execute_hooks(types))
          ABORT(r);
      }
#ifdef NRSH_CONFIG
      if(flags & NR_APP_STARTUP_PLUGIN_REGISTER_CONFIG){
        types[0]="register_config";
        
        if(r=nr_plugin_execute_hooks(types))
          ABORT(r);
      }
#endif
      if(flags & NR_APP_STARTUP_PLUGIN_REGISTER_STATS){
        types[0]="register_stats";
        
        if(r=nr_plugin_execute_hooks(types))
          ABORT(r);
      }
    }

    _status=0;
  abort:
    return(_status);
  }
