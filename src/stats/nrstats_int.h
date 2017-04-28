/*
 *
 *    nrstats_int.h
 *
 *    $Source: /cvsroot/nrappkit/nrappkit/src/stats/nrstats_int.h,v $
 *    $Revision: 1.3 $
 *    $Date: 2007/06/26 22:37:55 $
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

#ifndef __NRSTATS_INT_H__
#define __NRSTATS_INT_H__

#include <sys/types.h>
#ifdef WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <r_types.h>

#define NR_STAT_PRINT_CHAR(object, value) \
    output(handle, "%s.%s: %c\n", namespace, #value, ((object)->value))
#define NR_STAT_PRINT_CHARI(object, value, index) \
    output(handle, "%s.%d.%s: %c\n", namespace, (index), #value, ((object)->value))

#define NR_STAT_PRINT_STRING(object, value) \
    output(handle, "%s.%s: %s\n", namespace, #value, ((object)->value))
#define NR_STAT_PRINT_STRINGI(object, value, index) \
    output(handle, "%s.%d.%s: %s\n", namespace, (index), #value, ((object)->value))

#define NR_STAT_PRINT_TIME(object, value) \
    output(handle, "%s.%s: %s", namespace, #value, ctime(&((object)->value)))
#define NR_STAT_PRINT_TIMEI(object, value, index) \
    output(handle, "%s.%d.%s: %s", namespace, (index), #value, ctime(&((object)->value)))

#define NR_STAT_PRINT_UINT2(object, value) \
    output(handle, "%s.%s: %u\n", namespace, #value, ((object)->value))
#define NR_STAT_PRINT_UINT2I(object, value, index) \
    output(handle, "%s.%d.%s: %u\n", namespace, (index), #value, ((object)->value))

#define NR_STAT_PRINT_UINT8(object, value) \
    output(handle, "%s.%s: %llu\n", namespace, #value, ((object)->value))
#define NR_STAT_PRINT_UINT8I(object, value, index) \
    output(handle, "%s.%d.%s: %llu\n", namespace, (index), #value, ((object)->value))

#endif
