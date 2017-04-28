/**
   api_force.c

   
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
   

   ekr@rtfm.com  Sun Jan  1 11:07:07 2006
 */


static char *RCSSTRING __UNUSED__ ="$Id: api_force.c,v 1.3 2007/11/21 00:09:12 adamcain Exp $";

#include <sys/queue.h>
#include "api_force.h"
#include "r_types.h"

void nr_plugin_api_force_link(void)
  {
    ;
  }


#include "r_data.h"
static void *r_data_force __UNUSED__ =r_data_create;

#include "r_list.h"
static void *r_list_force __UNUSED__ =r_list_create;

#include "c_buf.h"
static void *c_buf_force __UNUSED__ =nr_c_buf_create;
     
#include "ed_ssl.h"
static void *ed_ssl_force __UNUSED__ =nr_dec_ssl_uintX;

#include "registry.h"
static void *registry_force __UNUSED__ =NR_reg_init;

#include "async_wait.h"
static void *async_wait_force __UNUSED__=NR_async_event_wait2;

#include "async_timer.h"
static void *async_timer_force __UNUSED__=NR_async_timer_set;

#include "r_log.h"
static void *log_force __UNUSED__ =r_log_register;

#ifdef NRSH_CONFIG
#include "nr_sh_config_logging_util.h"
static void *conf_log_force __UNUSED__=nr_config_logging_register;
#endif

#ifdef OPENSSL
#include <openssl/ssl.h>
static void *openssl_force1 __UNUSED__=OPENSSL_add_all_algorithms_noconf;
static void *openssl_force2 __UNUSED__=SSLv23_server_method;
#endif
