/**
   nrappkit_static_plugins.c

   
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
   

   ekr@networkresonance.com  Wed Jun 21 15:50:43 2006
 */


static char *RCSSTRING __UNUSED__ ="$Id: nrappkit_static_plugins.c,v 1.3 2007/11/21 00:09:13 adamcain Exp $";

#ifndef AGNEW

#include <nr_common.h>
#include "nr_plugin.h"

extern NR_plugin_def nr_captured_plugin_def;
extern NR_plugin_def nr_r_log_plugin_def;
#ifdef NRSH_CONFIG
extern NR_plugin_def nr_she_plugin_def;
#endif
extern NR_plugin_def nr_registry_plugin_def;

/* NRAPPKIT only: */
extern NR_plugin_def nr_reassd_plugin_def;

NR_plugin_def *nr_static_plugins[]={
     &nr_captured_plugin_def,
     &nr_r_log_plugin_def,
#ifdef NRSH_CONFIG
     &nr_she_plugin_def,
#endif
     &nr_registry_plugin_def,
     0
};
     
#endif

