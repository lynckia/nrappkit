/**
   r_log_plugin.c

   
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
   

   ekr@networkresonance.com  Wed Jun 21 14:37:21 2006
 */


static char *RCSSTRING __UNUSED__ ="$Id: r_log_plugin.c,v 1.4 2008/01/29 01:00:50 rescorla1 Exp $";

#include <nr_common.h>
#include "nr_plugin.h"
#include "r_log.h"
#include "nr_reg_keys.h"

#ifdef NRSH_CONFIG
#include "nr_sh_config_logging_util.h"

static int nr_r_log_register_config_hook(void)
  {
    int r;
    extern int nr_sh_config_logging();
    extern int nr_sh_config_logging_syslog();
    extern int nr_sh_config_logging_syslog_enable();
    extern int nr_sh_config_logging_syslog_default();
    extern int nr_sh_config_logging_syslog_level();
    extern int nr_sh_config_logging_syslog_servers();

    if (r=nr_config_logging_register("logging","logger","Logging facility"))
      ERETURN(r);

    if (r=nr_config_logging_register("generic", "generic", "Miscellaneous"))
      ERETURN(r);

    if (r=nr_sh_config_logging())
      ERETURN(r);

    if (r=nr_sh_config_logging_syslog())
      ERETURN(r);

    if (r=nr_sh_config_logging_syslog_enable())
      ERETURN(r);

    if (r=nr_sh_config_logging_syslog_default())
      ERETURN(r);

    if (r=nr_sh_config_logging_syslog_level())
      ERETURN(r);

    if (r=nr_sh_config_logging_syslog_servers())
      ERETURN(r);

    return(0);
  }
#endif

static NR_plugin_hook_def hook_defs[]={
#ifdef NRSH_CONFIG
     {"register_config",nr_r_log_register_config_hook},
#endif
     {0}
};

NR_plugin_def nr_r_log_plugin_def={
     1,
     "r_log_plugin",
     "$Revision: 1.4 $",
     hook_defs
};
