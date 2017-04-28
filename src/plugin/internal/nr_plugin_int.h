/**
   nr_plugin_int.h

   
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
   

   ekr@networkresonance.com  Mon Jun 19 18:59:36 2006
 */


#ifndef _nr_plugin_int_h
#define _nr_plugin_int_h

typedef struct nr_plugin_handle_ nr_plugin_handle;

#define NR_PLUGIN_MAX_NAME_LEN 1024

int nr_plugin_load_dso(char *path, nr_plugin_handle **pp);
int nr_plugin_load_dsos(char *prefix);
int nr_plugin_add_plugins(NR_plugin_def **info);
int nr_plugin_get_name(nr_plugin_handle *plugin, char *name);
int nr_plugin_get_version(nr_plugin_handle *plugin, char *version);
int nr_plugin_get_hook(nr_plugin_handle *plugin,char *type,NR_plugin_hook **hookp);
int nr_plugin_execute_hooks(char **types);
int nr_plugin_get_plugin_ct(int *ct);
int nr_plugin_get_plugin(int index,nr_plugin_handle **plugin);

#endif

