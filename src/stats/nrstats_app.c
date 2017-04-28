/*
 *
 *    nrstats_app.c
 *
 *    $Source: /cvsroot/nrappkit/nrappkit/src/stats/nrstats_app.c,v $
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

#include <time.h>
#include <string.h>

#include "nrstats.h"
#include "nrstats_int.h"
#include "nr_common.h"

static int
stats_app_reset(void *stats)
{
    NR_stats_app *app = (NR_stats_app*)stats;

    app->last_counter_reset = time(0);
    strcpy(app->version,nr_revision_number());

    return 0;
}


static int
stats_app_print(void *stats, char *namespace, void (*output)(void *handle, const char *fmt, ...), void *handle)
{
    NR_stats_app *app = (NR_stats_app*)stats;

    NR_STAT_PRINT_STRING(app, version);
    NR_STAT_PRINT_TIME(app, last_counter_reset);
    NR_STAT_PRINT_TIME(app, last_restart);
    NR_STAT_PRINT_UINT8(app, total_restarts);

    return 0;
}

static NR_stats_type stats_type_app =
{ "app", stats_app_reset, stats_app_print, 0, sizeof(NR_stats_app) };
NR_stats_type *NR_stats_type_app  = &stats_type_app;

