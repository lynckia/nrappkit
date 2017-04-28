/*
 *
 *    regrpc_client.c
 *
 *    $Source: /cvsroot/nrappkit/nrappkit/src/registry/regrpc_client.c,v $
 *    $Revision: 1.4 $
 *    $Date: 2008/04/07 23:17:39 $
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
#include <string.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <time.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>

#include "registry.h"
#include "registry_int.h"
#include "registry_vtbl.h"
#include "r_assoc.h"
#include "nr_common.h"
#include "r_log.h"
#include "r_errors.h"
#include "r_macros.h"

#include "regrpc.h"

static int nr_reg_init_clnt(nr_registry_module*);
static int nr_reg_reset_client(void);
static int nr_reg_get_char_1_clnt(NR_registry name, char *data);
static int nr_reg_get_uchar_1_clnt(NR_registry name, UCHAR *data);
static int nr_reg_get_int2_1_clnt(NR_registry name, INT2 *data);
static int nr_reg_get_uint2_1_clnt(NR_registry name, UINT2 *data);
static int nr_reg_get_int4_1_clnt(NR_registry name, INT4 *data);
static int nr_reg_get_uint4_1_clnt(NR_registry name, UINT4 *data);
static int nr_reg_get_int8_1_clnt(NR_registry name, INT8 *data);
static int nr_reg_get_uint8_1_clnt(NR_registry name, UINT8 *data);
static int nr_reg_get_double_1_clnt(NR_registry name, double *data);
static int nr_reg_get_registry_1_clnt(NR_registry name, NR_registry data);
static int nr_reg_get_bytes_1_clnt(NR_registry name, UCHAR *data, size_t size, size_t *length);
static int nr_reg_get_string_1_clnt(NR_registry name, char *data, size_t size);
static int nr_reg_get_length_1_clnt(NR_registry name, size_t *len);
static int nr_reg_get_type_1_clnt(NR_registry name, NR_registry_type type);
static int nr_reg_set_char_1_clnt(NR_registry name, char data);
static int nr_reg_set_uchar_1_clnt(NR_registry name, UCHAR data);
static int nr_reg_set_int2_1_clnt(NR_registry name, INT2 data);
static int nr_reg_set_uint2_1_clnt(NR_registry name, UINT2 data);
static int nr_reg_set_int4_1_clnt(NR_registry name, INT4 data);
static int nr_reg_set_uint4_1_clnt(NR_registry name, UINT4 data);
static int nr_reg_set_int8_1_clnt(NR_registry name, INT8 data);
static int nr_reg_set_uint8_1_clnt(NR_registry name, UINT8 data);
static int nr_reg_set_double_1_clnt(NR_registry name, double data);
static int nr_reg_set_registry_1_clnt(NR_registry name);
static int nr_reg_set_bytes_1_clnt(NR_registry name, UCHAR *data, size_t length);
static int nr_reg_set_string_1_clnt(NR_registry name, char *data);
static int nr_reg_del_1_clnt(NR_registry name);
static int nr_reg_get_child_count_1_clnt(NR_registry parent, size_t *count);
static int nr_reg_get_children_1_clnt(NR_registry parent, NR_registry *data, size_t size, size_t *length);
static int nr_reg_fin_1_clnt(NR_registry name);
static int nr_reg_dump_1_clnt(int sorted);


#define CLIENT_ENTER(result_type) \
  { \
    static result_type *result; \
    int r,_status; \
    CLIENT *clnt; \
    int tries = 0; \
  try_again: \
    result = 0; \
    if ((r=nr_reg_get_client(&clnt))) \
        ABORT(r);

#define CLIENT_LEAVE(cleanup_code) \
    _status = 0; \
  abort: \
    { cleanup_code } \
    return(_status); \
  }

#define __ABORT_IF_RPC_FAILED(retval,registry) \
       do { \
           if (! result) { \
               r_log(NR_LOG_REGISTRY, LOG_WARNING, "%s", clnt_sperror(clnt, "No result")); \
               if (++tries > 5) \
                   ABORT(R_IO_ERROR); \
               sleep(tries); \
               nr_reg_reset_client(); \
               nr_reg_init_clnt(NR_REG_MODE_REMOTE); \
               goto try_again; \
           } \
           else if ((retval)) { \
               if ((registry)) \
                 r_log(NR_LOG_REGISTRY, LOG_DEBUG, "Server returned: %s: '%s'", nr_strerror((retval)),(registry)); \
               else \
                 r_log(NR_LOG_REGISTRY, LOG_DEBUG, "Server returned: %s", nr_strerror((retval))); \
               ABORT((retval)); \
           } \
       } while (0)

#define ABORT_IF_RESULT_RETVAL_FAILED(registry)  __ABORT_IF_RPC_FAILED(result->retval, registry)
#define ABORT_IF_RESULT_FAILED()         __ABORT_IF_RPC_FAILED(*result, 0)

/* IMPORTANT: any use of free() here is to free pointers which are 
 * allocated by the RPC library which called into the real malloc(),
 * so free() needs to be the real free()
 */
#undef free

static CLIENT *__clnt = 0;

int
nr_reg_get_client(CLIENT **client)
{
    int _status;
    int sockfd;
    struct hostent *h;
    struct sockaddr_in sin;

    int registryd_port=8113;
    char *pt=0;

    if(pt=getenv("REGISTRYD_PORT")){
      registryd_port=atoi(pt);
    }

    if (! __clnt) {
        if ((h = gethostbyname2("localhost", AF_INET)) == 0) {
            r_log_e(NR_LOG_REGISTRY, LOG_ERR, "Couldn't find localhost");
            ABORT(R_FAILED);
        }

        memset(&sin, 0, sizeof(sin));
#ifdef HAVE_SIN_LEN
        sin.sin_len = sizeof(sin);
#endif
        sin.sin_family = h->h_addrtype;
        sin.sin_port = htons(registryd_port);
        memcpy((char*)&sin.sin_addr, h->h_addr, h->h_length);

        sockfd = -1;
 
        __clnt = clnttcp_create(&sin, REGISTRYDPROG, REGISTRYDVERS, &sockfd, 0, 0);
        if (! __clnt) {
            r_log(NR_LOG_REGISTRY,LOG_WARNING,"%s", clnt_spcreateerror("Unable to allocate RPC"));
            ABORT(R_FAILED);
        }
    }

    *client = __clnt;

    _status = 0;
  abort:
    return(_status);
}

int
nr_reg_reset_client()
{
    clnt_destroy(__clnt);
    __clnt = 0;
    return 0;
}

int
nr_reg_init_clnt(nr_registry_module *me)
{
    int r, _status;
    CLIENT *ignore;

    if ((r=nr_reg_get_client(&ignore)))
        ABORT(r);

    if ((r=nr_reg_cb_init()))
        ABORT(r);
    
    if ((r=nr_reg_client_cb_init()))
        ABORT(r);

    _status = 0;
  abort:
    return(_status);
}

int
nr_reg_get_char_1_clnt(NR_registry name, char *data)
{
CLIENT_ENTER(nr_regd_result_char)

    result = nr_reg_get_char_1(name, clnt);
    ABORT_IF_RESULT_RETVAL_FAILED(name);

    *data = result->out;

CLIENT_LEAVE()
}

int
nr_reg_get_uchar_1_clnt(NR_registry name, UCHAR *data)
{
CLIENT_ENTER(nr_regd_result_UCHAR)

    result = nr_reg_get_uchar_1(name, clnt);
    ABORT_IF_RESULT_RETVAL_FAILED(name);

    *data = result->out;

CLIENT_LEAVE()
}

int
nr_reg_get_int2_1_clnt(NR_registry name, INT2 *data)
{
CLIENT_ENTER(nr_regd_result_INT2)

    result = nr_reg_get_int2_1(name, clnt);
    ABORT_IF_RESULT_RETVAL_FAILED(name);

    *data = result->out;

CLIENT_LEAVE()
}

int
nr_reg_get_uint2_1_clnt(NR_registry name, UINT2 *data)
{
CLIENT_ENTER(nr_regd_result_UINT2)

    result = nr_reg_get_uint2_1(name, clnt);
    ABORT_IF_RESULT_RETVAL_FAILED(name);

    *data = result->out;

CLIENT_LEAVE()
}

int
nr_reg_get_int4_1_clnt(NR_registry name, INT4 *data)
{
CLIENT_ENTER(nr_regd_result_INT4)

    result = nr_reg_get_int4_1(name, clnt);
    ABORT_IF_RESULT_RETVAL_FAILED(name);

    *data = result->out;

CLIENT_LEAVE()
}

int
nr_reg_get_uint4_1_clnt(NR_registry name, UINT4 *data)
{
CLIENT_ENTER(nr_regd_result_UINT4)

    result = nr_reg_get_uint4_1(name, clnt);
    ABORT_IF_RESULT_RETVAL_FAILED(name);

    *data = result->out;

CLIENT_LEAVE()
}

int
nr_reg_get_int8_1_clnt(NR_registry name, INT8 *data)
{
CLIENT_ENTER(nr_regd_result_INT8)

    result = nr_reg_get_int8_1(name, clnt);
    ABORT_IF_RESULT_RETVAL_FAILED(name);

    *data = result->out;

CLIENT_LEAVE()
}

int
nr_reg_get_uint8_1_clnt(NR_registry name, UINT8 *data)
{
CLIENT_ENTER(nr_regd_result_UINT8)

    result = nr_reg_get_uint8_1(name, clnt);
    ABORT_IF_RESULT_RETVAL_FAILED(name);

    *data = result->out;

CLIENT_LEAVE()
}

int
nr_reg_get_double_1_clnt(NR_registry name, double *data)
{
CLIENT_ENTER(nr_regd_result_double)

    result = nr_reg_get_double_1(name, clnt);
    ABORT_IF_RESULT_RETVAL_FAILED(name);

    *data = result->out;

CLIENT_LEAVE()
}

int
nr_reg_get_registry_1_clnt(NR_registry name, NR_registry data)
{
CLIENT_ENTER(nr_regd_result_string)

    result = nr_reg_get_registry_1(name, clnt);
    ABORT_IF_RESULT_RETVAL_FAILED(name);

    if (strlen(result->out) >= sizeof(NR_registry))
        ABORT(R_BAD_ARGS);

    strncpy(data, result->out, sizeof(NR_registry));

CLIENT_LEAVE()
}

int
nr_reg_get_bytes_1_clnt(NR_registry name, UCHAR *data, size_t size, size_t *length)
{
CLIENT_ENTER(nr_regd_result_bytes)

    result = nr_reg_get_bytes_1(name, clnt);
    ABORT_IF_RESULT_RETVAL_FAILED(name);

    if (result->out.out_len > size)
        ABORT(R_BAD_ARGS);

    memcpy(data, result->out.out_val, result->out.out_len);
    *length = result->out.out_len;

CLIENT_LEAVE()
}

int
nr_reg_get_string_1_clnt(NR_registry name, char *data, size_t size)
{
CLIENT_ENTER(nr_regd_result_string)

    result = nr_reg_get_string_1(name, clnt);
    ABORT_IF_RESULT_RETVAL_FAILED(name);

    if (strlen(result->out) >= size)
        ABORT(R_BAD_ARGS);

    strncpy(data, result->out, size);

CLIENT_LEAVE(
    if (result && result->out) free(result->out);
)
}

int
nr_reg_get_length_1_clnt(NR_registry name, size_t *len)
{
CLIENT_ENTER(nr_regd_result_UINT4)

    result = nr_reg_get_length_1(name, clnt);
    ABORT_IF_RESULT_RETVAL_FAILED(name);

    *len = result->out;

CLIENT_LEAVE()
}

int
nr_reg_get_type_1_clnt(NR_registry name, NR_registry_type type)
{
CLIENT_ENTER(nr_regd_result_string)

    result = nr_reg_get_type_1(name, clnt);
    ABORT_IF_RESULT_RETVAL_FAILED(name);

    strncpy(type, result->out, sizeof(NR_registry_type));

CLIENT_LEAVE()
}

int
nr_reg_set_char_1_clnt(NR_registry name, char data)
{
CLIENT_ENTER(int)

    result = nr_reg_set_char_1(name, data, clnt);
    ABORT_IF_RESULT_FAILED();

CLIENT_LEAVE()
}

int
nr_reg_set_uchar_1_clnt(NR_registry name, UCHAR data)
{
CLIENT_ENTER(int)

    result = nr_reg_set_uchar_1(name, data, clnt);
    ABORT_IF_RESULT_FAILED();

CLIENT_LEAVE()
}

int
nr_reg_set_int2_1_clnt(NR_registry name, INT2 data)
{
CLIENT_ENTER(int)

    result = nr_reg_set_int2_1(name, data, clnt);
    ABORT_IF_RESULT_FAILED();

CLIENT_LEAVE()
}

int
nr_reg_set_uint2_1_clnt(NR_registry name, UINT2 data)
{
CLIENT_ENTER(int)

    result = nr_reg_set_uint2_1(name, data, clnt);
    ABORT_IF_RESULT_FAILED();

CLIENT_LEAVE()
}

int
nr_reg_set_int4_1_clnt(NR_registry name, INT4 data)
{
CLIENT_ENTER(int)

    result = nr_reg_set_int4_1(name, data, clnt);
    ABORT_IF_RESULT_FAILED();

CLIENT_LEAVE()
}

int
nr_reg_set_uint4_1_clnt(NR_registry name, UINT4 data)
{
CLIENT_ENTER(int)

    result = nr_reg_set_uint4_1(name, data, clnt);
    ABORT_IF_RESULT_FAILED();

CLIENT_LEAVE()
}

int
nr_reg_set_int8_1_clnt(NR_registry name, INT8 data)
{
CLIENT_ENTER(int)

    result = nr_reg_set_int8_1(name, data, clnt);
    ABORT_IF_RESULT_FAILED();

CLIENT_LEAVE()
}

int
nr_reg_set_uint8_1_clnt(NR_registry name, UINT8 data)
{
CLIENT_ENTER(int)

    result = nr_reg_set_uint8_1(name, data, clnt);
    ABORT_IF_RESULT_FAILED();

CLIENT_LEAVE()
}

int
nr_reg_set_double_1_clnt(NR_registry name, double data)
{
CLIENT_ENTER(int)

    result = nr_reg_set_double_1(name, data, clnt);
    ABORT_IF_RESULT_FAILED();

CLIENT_LEAVE()
}

int
nr_reg_set_registry_1_clnt(NR_registry name)
{
CLIENT_ENTER(int)

    result = nr_reg_set_registry_1(name, clnt);
    ABORT_IF_RESULT_FAILED();

CLIENT_LEAVE()
}

int
nr_reg_set_bytes_1_clnt(NR_registry name, UCHAR *data, size_t length)
{
    nr_regd_data data2;
CLIENT_ENTER(int)

    data2.nr_regd_data_val = (char*)data;
    data2.nr_regd_data_len = length;

    result = nr_reg_set_bytes_1(name, data2, clnt);
    ABORT_IF_RESULT_FAILED();

CLIENT_LEAVE()
}

int
nr_reg_set_string_1_clnt(NR_registry name, char *data)
{
CLIENT_ENTER(int)

    result = nr_reg_set_string_1(name, data, clnt);
    ABORT_IF_RESULT_FAILED();

CLIENT_LEAVE()
}

int
nr_reg_del_1_clnt(NR_registry name)
{
CLIENT_ENTER(int)

    result = nr_reg_del_1(name, clnt);
    ABORT_IF_RESULT_FAILED();

CLIENT_LEAVE()
}

int
nr_reg_get_child_count_1_clnt(NR_registry parent, size_t *count)
{
CLIENT_ENTER(nr_regd_result_UINT4)

    result = nr_reg_get_child_count_1(parent, clnt);
    ABORT_IF_RESULT_RETVAL_FAILED(parent);

    *count = result->out;

CLIENT_LEAVE()
}

int
nr_reg_get_children_1_clnt(NR_registry parent, NR_registry *data, size_t size, size_t *length)
{
CLIENT_ENTER(nr_regd_result_strings)
    int i;

    result = nr_reg_get_children_1(parent, clnt);
    ABORT_IF_RESULT_RETVAL_FAILED(parent);

    if (result->out.out_len > size)
        ABORT(R_BAD_ARGS);

    for (i = 0; i < result->out.out_len; ++i) {
        strncpy(data[i], result->out.out_val[i], sizeof(NR_registry)-1);
    }

    *length = result->out.out_len;

CLIENT_LEAVE(
    if(result){
      if (result->out.out_val){
        for (i = 0; i < result->out.out_len; ++i) {
          if(result->out.out_val[i])
            free(result->out.out_val[i]);
        }
        free(result->out.out_val);
      }
    }
)
}

int
nr_reg_fin_1_clnt(NR_registry name)
{
CLIENT_ENTER(int)

    result = nr_reg_fin_1(name, clnt);
    ABORT_IF_RESULT_FAILED();

CLIENT_LEAVE()
}


int
nr_reg_dump_1_clnt(int sorted)
{
//CLIENT_ENTER(nr_regd_result_strings)
//  int _status;

return(R_INTERNAL); // not yet implemented

//CLIENT_LEAVE()
}


static nr_registry_module_vtbl nr_regrpc_vtbl = {
    nr_reg_init_clnt,
    nr_reg_get_char_1_clnt,
    nr_reg_get_uchar_1_clnt,
    nr_reg_get_int2_1_clnt,
    nr_reg_get_uint2_1_clnt,
    nr_reg_get_int4_1_clnt,
    nr_reg_get_uint4_1_clnt,
    nr_reg_get_int8_1_clnt,
    nr_reg_get_uint8_1_clnt,
    nr_reg_get_double_1_clnt,
    nr_reg_get_registry_1_clnt,
    nr_reg_get_bytes_1_clnt,
    nr_reg_get_string_1_clnt,
    nr_reg_get_length_1_clnt,
    nr_reg_get_type_1_clnt,
    nr_reg_set_char_1_clnt,
    nr_reg_set_uchar_1_clnt,
    nr_reg_set_int2_1_clnt,
    nr_reg_set_uint2_1_clnt,
    nr_reg_set_int4_1_clnt,
    nr_reg_set_uint4_1_clnt,
    nr_reg_set_int8_1_clnt,
    nr_reg_set_uint8_1_clnt,
    nr_reg_set_double_1_clnt,
    nr_reg_set_registry_1_clnt,
    nr_reg_set_bytes_1_clnt,
    nr_reg_set_string_1_clnt,
    nr_reg_del_1_clnt,
    nr_reg_get_child_count_1_clnt,
    nr_reg_get_children_1_clnt,
    nr_reg_fin_1_clnt,
    nr_reg_dump_1_clnt
};

static nr_registry_module nr_regrpc_module = { 0, &nr_regrpc_vtbl };

void *NR_REG_MODE_REMOTE = &nr_regrpc_module;


