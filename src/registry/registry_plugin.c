/**
   registry_plugin.c

   
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
   

   briank@networkresonance.com  Fri Jun 30 14:18:20 PDT 2006
 */


static char *RCSSTRING __UNUSED__ ="$Id: registry_plugin.c,v 1.3 2007/11/21 00:09:13 adamcain Exp $";

#include <nr_common.h>
#include "nr_plugin.h"
#include "nr_reg_keys.h"
#ifdef NRSH_CONFIG
#include "nr_sh_config_logging_util.h"

static int nr_registry_register_config_hook(void)
  {
    int r;

    if (r=nr_config_logging_register("registry", "registry", "Registry facility"))
      ERETURN(r);

    return(0);
  }
#endif

static NR_plugin_hook_def hook_defs[]={
#ifdef NRSH_CONFIG
     {"register_config",nr_registry_register_config_hook},
#endif
     {0}
};

NR_plugin_def nr_registry_plugin_def={
     1,
     "registry_plugin",
     "$Revision: 1.3 $",
     hook_defs
};

