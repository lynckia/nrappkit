/*
 *
 *    nrstats_memory.c
 *
 *    $Source: /cvsroot/nrappkit/nrappkit/src/stats/nrstats_memory.c,v $
 *    $Revision: 1.2 $
 *    $Date: 2006/08/16 19:39:16 $
 *
 *    
 *    Copyright (C) 2005, Network Resonance, Inc.
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
 *
 */

#include "nrstats.h"
#include "nrstats_int.h"

static int
stats_memory_reset(void *stats)
{
    NR_stats_memory *mem = (NR_stats_memory*)stats;

    mem->current_size = 0;
    mem->in_use = 0;

    return 0;
}

static int
stats_memory_print(void *stats, char *namespace, void (*output)(void *handle, const char *fmt, ...), void *handle)
{
    NR_stats_memory *mem = (NR_stats_memory*)stats;

    NR_STAT_PRINT_UINT8(mem, current_size);
    NR_STAT_PRINT_UINT8(mem, max_size);
    NR_STAT_PRINT_UINT8(mem, in_use);
    NR_STAT_PRINT_UINT8(mem, in_use_max);

    return 0;
}

static NR_stats_type  stats_type_memory =
{ "memory", stats_memory_reset, stats_memory_print, 0, sizeof(NR_stats_memory) };
NR_stats_type *NR_stats_type_memory  = &stats_type_memory;

