/**
   ssl_util.h

   
   Copyright (C) 2002-2003, Network Resonance, Inc.
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
   

   ekr@rtfm.com  Wed Mar 13 18:23:17 2002
 */


#ifndef _ssl_util_h
#define _ssl_util_h

#include <openssl/ssl.h>

typedef struct ssl_conn_ {
     SSL_CTX *ssl_ctx;
     SSL *ssl;
     BIO *bio;
     int sock;
} ssl_conn;

int nr_check_cert(SSL *ssl,char *host);
int nr_tcp_connect(char *host,int port);
int nr_ssl_conn_create(char *rootlist,
  char *keyfile,char *host,int port,
  ssl_conn **connp);
int nr_ssl_conn_create_with_cert(UCHAR *cert,int cert_l,char *keyfile,
  char *host,int port,ssl_conn **connp);
int nr_ssl_conn_destroy(ssl_conn **connp);
int nr_ssl_write_all(SSL *ssl,UCHAR *data,int len);
int nr_ssl_read_data(SSL *ssl,UCHAR *data, int len,int *lenp);
int nr_ssl_get_common_name(SSL *ssl,char *name, int maxlen);
int nr_set_nonblock(int fd);

int nr_ssl_read_x509_pem_data(char *str,X509 **xp);
int nr_ssl_read_private_key_pem_data(char *str,EVP_PKEY **pp);
int nr_ssl_read_private_key_pem_data_pass(char *str,char *pass,EVP_PKEY **pp);
int nr_ssl_util_parse_cert(UCHAR *data,int len,X509 **certp);

#endif

