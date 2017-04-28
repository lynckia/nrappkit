/**
   nr_startup.h

   
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
   

   ekr@networkresonance.com  Tue Feb  7 17:15:54 2006
 */


#ifndef _nr_startup_h
#define _nr_startup_h

int nr_app_startup(char *progname,UINT4 flags,
  int *log_facility,void **app_stats,void **memory_stats);

#define NR_APP_STARTUP_INIT_LOGGING                       (1)
#define NR_APP_STARTUP_REGISTRY_REMOTE                    (1<<1)
#define NR_APP_STARTUP_REGISTRY_LOCAL                     (1<<2)
#define NR_APP_STARTUP_STATS                              (1<<3)
#define NR_APP_STARTUP_STATS_APP                          (1<<4)
#define NR_APP_STARTUP_STATS_MEMORY                       (1<<5)
#define NR_APP_STARTUP_STATS_RESET                        (1<<6)
#define NR_APP_STARTUP_DECODER                            (1<<7)

#define NR_APP_STARTUP_PLUGIN                             (1<<24)
#define NR_APP_STARTUP_PLUGIN_REGISTER_DECODER            (1<<25)
#define NR_APP_STARTUP_PLUGIN_REGISTER_CONFIG             (1<<26)
#define NR_APP_STARTUP_PLUGIN_REGISTER_LOGGING            (1<<27)
#define NR_APP_STARTUP_PLUGIN_REGISTER_STATS              (1<<28)

/* Not really all */
#define NR_APP_STARTUP_ALL                                ((1<<7)-1) & ~NR_APP_STARTUP_REGISTRY_LOCAL

#endif

