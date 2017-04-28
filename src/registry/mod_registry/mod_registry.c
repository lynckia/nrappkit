/**
   mod_registry.c

   
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
   

   ekr@networkresonance.com  Thu Feb  9 16:43:15 2006
 */


static char *RCSSTRING __UNUSED__ ="$Id: mod_registry.c,v 1.2 2006/08/16 19:39:14 adamcain Exp $";

#include "httpd.h"
#include "http_config.h"
#include "http_main.h"
#include "http_protocol.h"
#include "http_log.h"

#include "nr_common.h"
#include "registry.h"
#include "registry_int.h"

#include <stdio.h>

static int initialized=0;

static int transform_name(request_rec *r,char *file,char **transformed);
static int parse_query_string(request_rec *r,char *q, char **qs_value);

#define RETURN_ERROR(error,string) do { ap_log_rerror(__FILE__,__LINE__,APLOG_EMERG,r,string); ABORT(error); } while(0)

static void registry_init(server_rec *s, pool *p)
  {
    if(NR_reg_init(NR_REG_MODE_REMOTE)){
      ap_log_error(__FILE__,__LINE__,APLOG_EMERG,s,"Couldn't open registry");
      return;
    }

    ap_log_error(__FILE__,__LINE__,APLOG_NOTICE,s,"Registry opened");

    initialized=1;
  }




int transform_name(request_rec *r,char *file, char **transformed)
{
    char *c;
    int _status;
    char *f=0;

    if(*file=='/') file ++;
    
    f = r_strdup(file);
    
    if (f == 0)
      RETURN_ERROR(HTTP_INTERNAL_SERVER_ERROR,"No memory");

    for (c = f; *c != '\0'; ++c) {
      if (*c == '/')
        *c = '.';
      if (*c == '?') {
        *c = '\0';
        break;
      }
    }

    *transformed=f;
    _status=0;
abort:
    if(_status)
      RFREE(file);
    
    return(_status);
}

static int parse_query_string(request_rec *r,char *q, char **qs_value)
  {
    int _status;
    char *e;
    
    *qs_value = 0;

    e = strchr(q, '=');
    if (e != 0) {
      *qs_value = (e + 1);
      *e = '\0';
    }

    _status=0;
  //abort:
    return(_status);
  }


static int process_getlength_request(request_rec *r, char *name)
  {
    size_t l;
    int _status;


    if (NR_reg_get_length(name, &l))
      RETURN_ERROR(HTTP_NOT_FOUND, "not found");
    r->content_type = "text/html; charset=us-ascii";
    ap_send_http_header(r);

    if (!r->header_only)
      ap_rprintf(r,"%u", (unsigned int)l);

    _status=0;
  abort:
    return(_status);
    
  }

static int process_getkeys_request(request_rec *r, char *parent)
  {
    int i;
    unsigned int n;
    NR_registry child;
    int cl;
    int _status;
  
    if (NR_reg_get_child_count(parent, &n))
      RETURN_ERROR(HTTP_NOT_FOUND,"not found");

    
    cl = 0;
    for (i = 0; i < n; ++i) {
      if (NR_reg_get_child_registry(parent, i, child))
        RETURN_ERROR(HTTP_NOT_FOUND, "not found");
      cl += strlen(child) + 2;
    }

    r->content_type = "text/html; charset=us-ascii";
    
    ap_send_http_header(r);

    if (!r->header_only){
      for (i = 0; i < n; ++i) {
        (void)NR_reg_get_child_registry(parent, i, child);
        ap_rprintf(r,"%s\r\n", child);
      }
    }

    _status=0;
  abort:
    return(_status);
  }


static int process_get_request(request_rec *r, char *name, int qs_value_type)
{
    char *s=0;
    int rv,_status;
    char          _char;
    UCHAR         _uchar;
    INT2          _int2;
    UINT2         _uint2;
    INT4          _int4;
    UINT4         _uint4;
    INT8          _int8;
    UINT8         _uint8;
    double        _double;
    int alloc = 0;
    int len;

    switch (qs_value_type) {
      default:
        alloc = 100;    /* plenty of room for any of the scalar types */
        break;
      case NR_REG_TYPE_REGISTRY:
        alloc = strlen(name) + 1;
        break;
      case NR_REG_TYPE_STRING:
        alloc = 0;
        break;
    }

    if (alloc > 0) {
      if(!(s = (void*)RMALLOC(alloc)))
        ABORT(R_NO_MEMORY);
    }

    len = alloc;

    switch (qs_value_type){
      case NR_REG_TYPE_CHAR:
        if(rv=NR_reg_get_char(name,&_char))
          ABORT(rv);
        snprintf(s, len, "%d", _char);
        break;
      case NR_REG_TYPE_UCHAR:
        if(rv=NR_reg_get_uchar(name,&_uchar))
          ABORT(rv);
        snprintf(s, len, "%u", _uchar);
        break;
      case NR_REG_TYPE_INT2:
        if(rv=NR_reg_get_int2(name,&_int2))
          ABORT(rv);
        snprintf(s, len, "%d", _int2);
        break;
      case NR_REG_TYPE_UINT2:
        if(rv=NR_reg_get_uint2(name,&_uint2))
          ABORT(rv);
        snprintf(s, len, "%u", _uint2);
        break;
      case NR_REG_TYPE_INT4:
        if(rv=NR_reg_get_int4(name,&_int4))
          ABORT(rv);
        snprintf(s, len, "%d", _int4);
        break;
      case NR_REG_TYPE_UINT4:
        if(rv=NR_reg_get_uint4(name,&_uint4))
          ABORT(rv);
        snprintf(s, len, "%u", _uint4);
        break;
      case NR_REG_TYPE_INT8:
        if(rv=NR_reg_get_int8(name,&_int8))
          ABORT(rv);
        snprintf(s, len, "%lld", _int8);
        break;
      case NR_REG_TYPE_UINT8:
        if(rv=NR_reg_get_uint8(name,&_uint8))
          ABORT(rv);
        snprintf(s, len, "%llu", _uint8);
        break;
      case NR_REG_TYPE_DOUBLE:
        if(rv=NR_reg_get_double(name,&_double))
          ABORT(rv);
        snprintf(s, len, "%#f", _double);
        break;
      case NR_REG_TYPE_REGISTRY:
        snprintf(s, len, "%s", name);
        break;
      case NR_REG_TYPE_BYTES:
        ABORT(R_INTERNAL);
        break; /* Unimplemented */
      case NR_REG_TYPE_STRING:
        if(rv=NR_reg_alloc_string(name,&s))
          ABORT(rv);
        break;
      default:
        ABORT(R_INTERNAL);
        break;
    }
    
    r->content_type = "text/html; charset=us-ascii";

    ap_send_http_header(r);
    
    if (!r->header_only)
      ap_rputs(s,r);

    _status=0;
abort:
    RFREE(s);
    switch(_status){
      case R_NOT_FOUND:
        return(HTTP_NOT_FOUND);
        break;
      case 0:
        return(0);
      default:
        return(HTTP_INTERNAL_SERVER_ERROR);
    }
}

static int process_put_request(request_rec *r, char *name, int qs_value_type)
{
    int rv, _status;
    int len;
    char *content=0;
    int i;
    int req_ok=0;
    char          _char;
    UCHAR         _uchar;
    INT2          _int2;
    UINT2         _uint2;
    INT4          _int4;
    UINT4         _uint4;
    INT8          _int8;
    UINT8         _uint8;
    double        _double;

    if(rv=ap_setup_client_block(r,REQUEST_CHUNKED_ERROR))
      RETURN_ERROR(HTTP_BAD_REQUEST,"content length missing");
    if(!ap_should_client_block(r))
      RETURN_ERROR(HTTP_BAD_REQUEST,"missing content");

    len=r->remaining;
    content = RCALLOC(len+1);
    if (content == 0)
      RETURN_ERROR(HTTP_INTERNAL_SERVER_ERROR, "out of memory");
    if(ap_get_client_block(r,content,len)!=len)
      RETURN_ERROR(HTTP_BAD_REQUEST, "incomplete data");
    
    
    req_ok=1;
/* TODO: 
 *  - prevent duplicates
 *    i.e., if a key is first set as a UCHAR, any later request to set
 *          the same key as a n INT2 should generate an error.
 */

    switch (qs_value_type) {
    case NR_REG_TYPE_CHAR:
      if (sscanf(content, "%hhd", &_char) != 1)
          ABORT(R_FAILED);
      if ((rv=NR_reg_set_char(name, _char)))
          ABORT(rv);
      break;
    case NR_REG_TYPE_UCHAR:
      if (sscanf(content, "%hhu", &_uchar) != 1)
          ABORT(R_FAILED);
      if ((rv=NR_reg_set_uchar(name, _uchar)))
          ABORT(rv);
      break;
    case NR_REG_TYPE_INT2:
      if (sscanf(content, "%hd", &_int2) != 1)
          ABORT(R_FAILED);
      if ((rv=NR_reg_set_int2(name, _int2)))
          ABORT(rv);
      break;
    case NR_REG_TYPE_UINT2:
      if (sscanf(content, "%hu", &_uint2) != 1)
          ABORT(R_FAILED);
      if ((rv=NR_reg_set_uint2(name, _uint2)))
          ABORT(rv);
      break;
    case NR_REG_TYPE_INT4:
      if (sscanf(content, "%d", &_int4) != 1)
          ABORT(R_FAILED);
      if ((rv=NR_reg_set_int4(name, _int4)))
          ABORT(rv);
      break;
    case NR_REG_TYPE_UINT4:
      if (sscanf(content, "%u", &_uint4) != 1)
          ABORT(R_FAILED);
      if ((rv=NR_reg_set_uint4(name, _uint4)))
          ABORT(rv);
      break;
    case NR_REG_TYPE_INT8:
      if (sscanf(content, "%lld", &_int8) != 1)
          ABORT(R_FAILED);
      if ((rv=NR_reg_set_int8(name, _int8)))
          ABORT(rv);
      break;
    case NR_REG_TYPE_UINT8:
      if (sscanf(content, "%llu", &_uint8) != 1)
          ABORT(R_FAILED);
      if ((rv=NR_reg_set_uint8(name, _uint8)))
          ABORT(rv);
      break;
    case NR_REG_TYPE_DOUBLE:
      if (sscanf(content, "%lf", &_double) != 1)
          ABORT(R_FAILED);
      if ((rv=NR_reg_set_double(name, _double)))
          ABORT(rv);
      break;
    case NR_REG_TYPE_REGISTRY:
      if ((rv=NR_reg_set_registry(name)))
          ABORT(rv);
      break;
    case NR_REG_TYPE_BYTES:
//TODO: not yet implemented correctly
      for (i = 0; i < len; ++i) {
        if (scanf(&content[2*i], "%02x", content[i]) != 1)
            ABORT(R_FAILED);
      }
      if ((rv=NR_reg_set_bytes(name, (UCHAR*)content, len)))
          ABORT(rv);
      break;
    case NR_REG_TYPE_STRING:
      if (strlen(content) != len)
          ABORT(R_FAILED);
      if ((rv=NR_reg_set_string(name, content)))
          ABORT(rv);
      break;
    default:
      RETURN_ERROR(HTTP_BAD_REQUEST, "invalid request");
      break;
    }

    r->status=HTTP_CREATED;

    ap_send_http_header(r);
    
    _status = 0;
abort:
    if (_status && _status!=HTTP_CREATED) {
      if(req_ok)
        RETURN_ERROR(HTTP_BAD_REQUEST,"invalid data");
    }
    RFREE(content);
    return(_status);
}

static int process_delete_request(request_rec *r, char *name)
{
  int rv,_status;

    if ((rv=NR_reg_del(name))) {
      if (rv == R_NOT_FOUND)
        RETURN_ERROR(HTTP_BAD_REQUEST,"not found");
      else
        RETURN_ERROR(HTTP_BAD_REQUEST, "internal failure");
    }

    ap_send_http_header(r);
    _status=0;
abort:
    return(_status);
}


static int registry_handler(request_rec *r)
  {
    char *reg_key=0;
    char *qs_param=0;
    char *qs_value=0;
    int qs_value_type;
    
    int rv,_status;
    
    if(!initialized)
      return(HTTP_INTERNAL_SERVER_ERROR);

    ap_soft_timeout("start of registry handler", r);
    
    if(rv=transform_name(r,r->uri,&reg_key))
      ABORT(rv);

    if(!strlen(reg_key))
      RETURN_ERROR(HTTP_BAD_REQUEST,"No registry key specified");

    if(r->args){
      if(!(qs_param=r_strdup(r->args)))
        RETURN_ERROR(HTTP_INTERNAL_SERVER_ERROR,"No memory");
      
      if(rv=parse_query_string(r,qs_param,&qs_value))
        ABORT(rv);
    }

//    ap_log_rerror(__FILE__,__LINE__,APLOG_EMERG,r,"key=%s qp=%s qv=%s",
    //    reg_key,qs_param,qs_value);
         
    if(r->method_number==M_DELETE){
      if (qs_param != 0)
        RETURN_ERROR(HTTP_NOT_FOUND, "found query string");
    }
    else {
      if (qs_param == 0)
        RETURN_ERROR(HTTP_BAD_REQUEST, "missing query string");

      switch (r->method_number) {
        case M_GET:
//        case M_HEAD:   Same in Apache
        case M_PUT:
          if (! strcmp("type", qs_param)) {
            if (nr_reg_compute_type(qs_value, &qs_value_type))
              RETURN_ERROR(HTTP_BAD_REQUEST, "bad query string");
          }
          else if (! strcmp("getkeys", qs_param)) {
            /* no qs_value_type -- do nothing */
          }
          else if (! strcmp("getlength", qs_param)) {
            /* no qs_value_type -- do nothing */
          }
          else if (! strcmp("getnotifications", qs_param)) {
            /* no qs_value_type -- do nothing */
          }
          else {
            RETURN_ERROR(HTTP_BAD_REQUEST,"illegal query string");
          }
          break;
      }
    }

    rv=0;
    switch (r->method_number) {
      case M_GET:
//      case HTTP_HEAD: (same as GET)
        if (! strcmp("getlength", qs_param))
          rv=process_getlength_request(r, reg_key);
        else if (! strcmp("getkeys", qs_param))
          rv=process_getkeys_request(r, reg_key);
//        else if (! strcmp("getnotifications", qs_param))
//          rv=process_notify_request(r, reg_key);
        else
          rv=process_get_request(r, reg_key, qs_value_type);
        break;
      case M_PUT:
        rv=process_put_request(r, reg_key, qs_value_type);
        break;
      case M_DELETE:
        rv=process_delete_request(r, reg_key);
        break;
      default:
        RETURN_ERROR(HTTP_BAD_REQUEST, "illegal method");
        break;
    }

    if(rv)
      ABORT(rv);
    
    _status=OK;
  abort:
    ap_kill_timeout(r);
    RFREE(reg_key);
    RFREE(qs_param);
    return(_status);
  }





/* Module definition */
static const handler_rec registry_handlers[] = {
     {"registry-handler", registry_handler},
     {0}
};

module MODULE_VAR_EXPORT registry_module = {
    STANDARD_MODULE_STUFF,
    registry_init,   /* init */
    0,   /* create_dir_config */
    0,   /* merge_dir_config */
    0,   /* create_server_config */
    0,   /* merge_server_config */
    0,   /* cmds */
    registry_handlers,  /* handlers */
    0,   /* translate_handler */
    0,   /* check_user_id */
    0,   /* auth_checker */
    0,   /* access_checker */
    0,   /* type_checker */
    0,   /* fixer_upper */
    0,   /* logger */
    0,   /* header_parser*/
    0,   /* child_init */
    0,   /* child_exit */
    0    /* post_read_request*/
};
