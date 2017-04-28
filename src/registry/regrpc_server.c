/*
 *
 *    regrpc_server.c
 *
 *    $Source: /cvsroot/nrappkit/nrappkit/src/registry/regrpc_server.c,v $
 *    $Revision: 1.2 $
 *    $Date: 2006/08/16 19:39:14 $
 *
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
 *
 */

#include <assert.h>
#include <string.h>
#include <sys/param.h>
#include <ctype.h>
#include <fcntl.h>

#include "registry.h"
#include "registry_int.h"
#include "r_assoc.h"
#include "nr_common.h"
#include "r_log.h"
#include "r_errors.h"
#include "r_macros.h"

#include "regrpc.h"

static int maybe_adjust_ptr(nr_regd_registry name, char **ptr, size_t *maxlen);

nr_regd_result_char *
nr_reg_get_char_1_svc(nr_regd_registry name,  struct svc_req *rqstp)
{
    static nr_regd_result_char  result;

    result.retval = NR_reg_get_char(name, &result.out);

    return(&result);
}

nr_regd_result_UCHAR *
nr_reg_get_uchar_1_svc(nr_regd_registry name,  struct svc_req *rqstp)
{
    static nr_regd_result_UCHAR  result;

    result.retval = NR_reg_get_uchar(name, &result.out);

    return(&result);
}

nr_regd_result_INT2 *
nr_reg_get_int2_1_svc(nr_regd_registry name,  struct svc_req *rqstp)
{
    static nr_regd_result_INT2  result;

    result.retval = NR_reg_get_int2(name, &result.out);

    return(&result);
}

nr_regd_result_UINT2 *
nr_reg_get_uint2_1_svc(nr_regd_registry name,  struct svc_req *rqstp)
{
    static nr_regd_result_UINT2  result;

    result.retval = NR_reg_get_uint2(name, &result.out);

    return(&result);
}

nr_regd_result_INT4 *
nr_reg_get_int4_1_svc(nr_regd_registry name,  struct svc_req *rqstp)
{
    static nr_regd_result_INT4  result;

    result.retval = NR_reg_get_int4(name, &result.out);

    return(&result);
}

nr_regd_result_UINT4 *
nr_reg_get_uint4_1_svc(nr_regd_registry name,  struct svc_req *rqstp)
{
    static nr_regd_result_UINT4  result;

    result.retval = NR_reg_get_uint4(name, &result.out);

    return(&result);
}

nr_regd_result_INT8 *
nr_reg_get_int8_1_svc(nr_regd_registry name,  struct svc_req *rqstp)
{
    static nr_regd_result_INT8  result;

    result.retval = NR_reg_get_int8(name, &result.out);

    return(&result);
}

nr_regd_result_UINT8 *
nr_reg_get_uint8_1_svc(nr_regd_registry name,  struct svc_req *rqstp)
{
    static nr_regd_result_UINT8  result;

    result.retval = NR_reg_get_uint8(name, &result.out);

    return(&result);
}

nr_regd_result_double *
nr_reg_get_double_1_svc(nr_regd_registry name,  struct svc_req *rqstp)
{
    static nr_regd_result_double  result;

    result.retval = NR_reg_get_double(name, &result.out);

    return(&result);
}

nr_regd_result_string *
nr_reg_get_registry_1_svc(nr_regd_registry name,  struct svc_req *rqstp)
{
    static NR_registry registry;
    static nr_regd_result_string  result = { 0, registry };

    result.retval = NR_reg_get_registry(name, registry);

    return(&result);
}

nr_regd_result_bytes *
nr_reg_get_bytes_1_svc(nr_regd_registry name,  struct svc_req *rqstp)
{
    static nr_regd_result_bytes  result = { 0 };
    static size_t maxlen = 32;
    int r,_status;
    size_t len;
#ifdef SANITY_CHECKS
    static int first_time = 1;

    /* force behavior in development builds for sanity checking */
    if (first_time) maxlen = 2;
    first_time = 0;
#endif

    if (! result.out.out_val) {
        result.out.out_val = RMALLOC(maxlen);
        if (! result.out.out_val)
            ABORT(R_NO_MEMORY);
    }

    if ((r=NR_reg_get_bytes(name, (UCHAR*)result.out.out_val, maxlen, &len))) {
        if (r != R_BAD_ARGS)
            ABORT(r);

        /* probably not enough room, so try again with a larger buffer */ 
        if ((r=maybe_adjust_ptr(name, &result.out.out_val, &maxlen)))
            ABORT(r);

        /* try again with larger buffer */
        if ((r=NR_reg_get_bytes(name, (UCHAR*)result.out.out_val, maxlen, &len)))
            ABORT(r);
    }

    result.out.out_len = len;

    _status = 0;
  abort:
    result.retval = _status;
    return(&result);
}

nr_regd_result_string *
nr_reg_get_string_1_svc(nr_regd_registry name,  struct svc_req *rqstp)
{
    static nr_regd_result_string  result = { 0 };
    static size_t maxlen = 32;
    int r,_status;
#ifdef SANITY_CHECKS
    static int first_time = 1;

    /* force behavior in development builds for sanity checking */
    if (first_time) maxlen = 2;
    first_time = 0;
#endif

    if (! result.out) {
        result.out = RMALLOC(maxlen);
        if (! result.out)
            ABORT(R_NO_MEMORY);
    }

    if ((r=NR_reg_get_string(name, result.out, maxlen))) {
        if (r != R_BAD_ARGS)
            ABORT(r);

        /* probably not enough room, so try again with a larger buffer */ 
        if ((r=maybe_adjust_ptr(name, &result.out, &maxlen)))
            ABORT(r);

        /* try again with larger buffer */
        if ((r=NR_reg_get_string(name, result.out, maxlen)))
            ABORT(r);
    }

    _status = 0;
  abort:
    result.retval = _status;
    return(&result);
}

nr_regd_result_UINT4 *
nr_reg_get_length_1_svc(nr_regd_registry name,  struct svc_req *rqstp)
{
    static nr_regd_result_UINT4  result;
    size_t len;

    result.retval = NR_reg_get_length(name, &len);
    result.out = len;

    return(&result);
}

nr_regd_result_string *
nr_reg_get_type_1_svc(nr_regd_registry name,  struct svc_req *rqstp)
{
    static nr_regd_result_string  result;
    static char str[20];

    result.retval = NR_reg_get_type(name, str);
    result.out = str;

    return(&result);
}

int *
nr_reg_set_char_1_svc(nr_regd_registry name, char data,  struct svc_req *rqstp)
{
    static int  result;

    result = NR_reg_set_char(name, data);

    return(&result);
}

int *
nr_reg_set_uchar_1_svc(nr_regd_registry name, UCHAR data,  struct svc_req *rqstp)
{
    static int  result;

    result = NR_reg_set_uchar(name, data);

    return(&result);
}

int *
nr_reg_set_int2_1_svc(nr_regd_registry name, INT2 data,  struct svc_req *rqstp)
{
    static int  result;

    result = NR_reg_set_int2(name, data);

    return(&result);
}

int *
nr_reg_set_uint2_1_svc(nr_regd_registry name, UINT2 data,  struct svc_req *rqstp)
{
    static int  result;

    result = NR_reg_set_uint2(name, data);

    return(&result);
}

int *
nr_reg_set_int4_1_svc(nr_regd_registry name, INT4 data,  struct svc_req *rqstp)
{
    static int  result;

    result = NR_reg_set_int4(name, data);

    return(&result);
}

int *
nr_reg_set_uint4_1_svc(nr_regd_registry name, UINT4 data,  struct svc_req *rqstp)
{
    static int  result;

    result = NR_reg_set_uint4(name, data);

    return(&result);
}

int *
nr_reg_set_int8_1_svc(nr_regd_registry name, INT8 data,  struct svc_req *rqstp)
{
    static int  result;

    result = NR_reg_set_int8(name, data);

    return(&result);
}

int *
nr_reg_set_uint8_1_svc(nr_regd_registry name, UINT8 data,  struct svc_req *rqstp)
{
    static int  result;

    result = NR_reg_set_uint8(name, data);

    return(&result);
}

int *
nr_reg_set_double_1_svc(nr_regd_registry name, double data,  struct svc_req *rqstp)
{
    static int  result;

    result = NR_reg_set_double(name, data);

    return(&result);
}

int *
nr_reg_set_registry_1_svc(nr_regd_registry name,  struct svc_req *rqstp)
{
    static int  result;

    result = NR_reg_set_registry(name);

    return(&result);
}

int *
nr_reg_set_bytes_1_svc(nr_regd_registry name, nr_regd_data data,  struct svc_req *rqstp)
{
    static int  result;

    result = NR_reg_set_bytes(name, (UCHAR*)data.nr_regd_data_val, data.nr_regd_data_len);

    return(&result);
}

int *
nr_reg_set_string_1_svc(nr_regd_registry name, char *data,  struct svc_req *rqstp)
{
    static int  result;

    result = NR_reg_set_string(name, data);

    return(&result);
}

int *
nr_reg_del_1_svc(nr_regd_registry name,  struct svc_req *rqstp)
{
    static int  result;

    result = NR_reg_del(name);

    return(&result);
}

int *
nr_reg_fin_1_svc(nr_regd_registry name,  struct svc_req *rqstp)
{
    static int  result;

    result = NR_reg_fin(name);

    return(&result);
}

nr_regd_result_UINT4 *
nr_reg_get_child_count_1_svc(nr_regd_registry parent,  struct svc_req *rqstp)
{
    static nr_regd_result_UINT4  result;

    result.retval = NR_reg_get_child_count(parent, &result.out);

    return(&result);
}

nr_regd_result_strings *
nr_reg_get_children_1_svc(nr_regd_registry parent,  struct svc_req *rqstp)
{
    static nr_regd_result_strings  result = { 0 };
    static NR_registry *registries = 0;
    static size_t max_count = 32;
    int r,_status;
    int i;
#ifdef SANITY_CHECKS
    static int first_time = 1;

    /* force recursive behavior in development builds for sanity checking */
    if (first_time) max_count = 2;
    first_time = 0;
#endif

    if (result.out.out_val == 0) {
        if (!(registries = RMALLOC(max_count * sizeof(*registries))))
            ABORT(R_NO_MEMORY);

        if (!(result.out.out_val = RMALLOC(max_count * sizeof(*result.out.out_val))))
            ABORT(R_NO_MEMORY);

        for (i = 0; i < max_count; ++i)
            result.out.out_val[i] = registries[i];
    }

    if ((r=NR_reg_get_children(parent, registries, max_count, (size_t*)&result.out.out_len))) {
        if (r != R_BAD_ARGS || nr_reg_is_valid(parent))
            ABORT(r);

        /* result.out.out_val array not large enough, recurse until it is large enough */

        RFREE(registries);
        RFREE(result.out.out_val);
        registries = 0;
        result.out.out_val = 0;

        max_count *= 2;

        nr_reg_get_children_1_svc(parent, rqstp);
        if (result.retval)
            ABORT(result.retval);
    }

    _status = 0;
  abort:
    result.retval = _status;
    return(&result);
}

int *
nr_reg_dump_1_svc(struct svc_req *rqstp)
{
    static int  result;

    result = NR_reg_dump();

    return(&result);
}

int *
nr_reg_register_for_callbacks_1_svc(int port, struct svc_req *rqstp)
{
    static int result;

#ifdef FREEBSD
    result = nr_reg_register_for_callbacks(rqstp->rq_xprt->xp_fd, port);
#else
    result = nr_reg_register_for_callbacks(rqstp->rq_xprt->xp_sock, port);
#endif

    return(&result);
}

int
maybe_adjust_ptr(nr_regd_registry name, char **ptr, size_t *maxlen)
{
    int r,_status;
    size_t len;

    assert(*ptr);

    if ((r=NR_reg_get_length(name, &len))) {
        if (r == R_NOT_FOUND)
            len = *maxlen + 1;
        else
            ABORT(r);
    }

    if (len >= *maxlen) {
        char *tmp = RREALLOC(*ptr, len+2);
        if (! tmp)
            ABORT(R_NO_MEMORY);

        *ptr  = tmp;
        *maxlen = len+2;
    }

    _status = 0;
  abort:
    return(_status);
}

