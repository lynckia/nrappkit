/**
   ssl_util.c

   
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
   

   ekr@rtfm.com  Wed Mar 13 18:14:54 2002
 */


static char *RCSSTRING __UNUSED__ ="$Id: ssl_util.c,v 1.4 2007/11/21 00:09:13 adamcain Exp $";

#ifdef OPENSSL

#include <sys/types.h>
#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif
#include <fcntl.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "nr_common.h"
#include "r_common.h"
#include "r_log.h"
#include "ssl_util.h"
#include "r_assoc.h"
#include <sys/queue.h>

static int openssl_initialized=0;

typedef struct parsed_cert_ {
     X509 *x509;

     TAILQ_ENTRY(parsed_cert_) entry;
} parsed_cert;

static int _ssl_util_parse_cert(UCHAR *data,int len,parsed_cert **certp);
int nr_ssl_util_destroy_cert(parsed_cert **certp);
int nr_ssl_util_destroy_cert_v(void *v);
static int check_ssl_cert_match(SSL *conn,UCHAR *cert,int cert_l);

#define MAX_PARSED_CERTS 4096

/* Check that the common name matches the
   host name*/
int nr_check_cert(ssl,host)
  SSL *ssl;
  char *host;
  {
    X509 *peer;
    char peer_CN[256];
    
    if(SSL_get_verify_result(ssl)!=X509_V_OK){
      r_log(LOG_GENERIC,LOG_ERR,"Peer not SSL verified");
      ERETURN(R_NOT_FOUND);
    }

    /*Check the cert chain. The chain length
      is automatically checked by OpenSSL when
      we set the verify depth in the ctx */

    /*Check the common name*/
    peer=SSL_get_peer_certificate(ssl);
    X509_NAME_get_text_by_NID
      (X509_get_subject_name(peer),
      NID_commonName, peer_CN, 256);
    if(strcasecmp(peer_CN,host)){
      r_log(LOG_GENERIC,LOG_ERR,"Peer's CN (%s) doesn't match domain name (%s)",
        peer_CN,host);
      ERETURN(R_NOT_FOUND);
    }

    return(0);
  }

int nr_ssl_get_common_name(ssl,name,maxlen)
  SSL *ssl;
  char *name;
  int maxlen;
  {
    X509 *peer;
    
    if(SSL_get_verify_result(ssl)!=X509_V_OK){
      r_log(LOG_GENERIC,LOG_ERR,"Peer not SSL verified");
      ERETURN(R_NOT_FOUND);
    }

    /*Check the cert chain. The chain length
      is automatically checked by OpenSSL when
      we set the verify depth in the ctx */

    /*Check the common name*/
    peer=SSL_get_peer_certificate(ssl);
    X509_NAME_get_text_by_NID
      (X509_get_subject_name(peer),
      NID_commonName, name, maxlen);

    return(0);
  }
  
int nr_tcp_connect(host,port)
  char *host;
  int port;
  {
    struct hostent *hp;
    struct sockaddr_in addr;
    int sock;
    
    if(!(hp=gethostbyname(host))){
      r_log(LOG_GENERIC,LOG_ERR,"Couldn't resolve host %s",host);
      ERETURN(-1);
    }
    memset(&addr,0,sizeof(addr));
    addr.sin_addr=*(struct in_addr*)
      hp->h_addr_list[0];
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);

    if((sock=socket(AF_INET,SOCK_STREAM,
      IPPROTO_TCP))<0) {
      r_log(LOG_GENERIC,LOG_ERR,"Couldn't create socket");
      ERETURN(-1);
    }

    if(connect(sock,(struct sockaddr *)&addr,
      sizeof(addr))<0){
      r_log(LOG_GENERIC,LOG_ERR,"Couldn't connect to master: error=%s",
        strerror(errno));
      ERETURN(-1);
    }
    
    return sock;
  }



int nr_ssl_conn_create_with_cert(UCHAR *cert,int cert_l,char *keyfile,
  char *host,int port,ssl_conn **connp)
  {
    ssl_conn *conn=0;
    int r,_status;

    /* Create the SSL context */
    if(!openssl_initialized){
      openssl_initialized=1;
      SSL_library_init();
      SSL_load_error_strings();
    }
    
    if(!(conn=(ssl_conn *)RCALLOC(sizeof(ssl_conn))))
      ABORT(R_NO_MEMORY);
    conn->sock=-1;
    
    if(!(conn->ssl_ctx=SSL_CTX_new(TLSv1_method())))
      ABORT(R_NO_MEMORY);
    if(keyfile){
      if(!(SSL_CTX_use_certificate_chain_file(conn->ssl_ctx,keyfile))){
        r_log(LOG_GENERIC,LOG_ERR,"Couldn't read SSL cert chain file");
        ABORT(R_NOT_FOUND);
      }
      if(!(SSL_CTX_use_PrivateKey_file(conn->ssl_ctx,keyfile,
        SSL_FILETYPE_PEM))){
        r_log(LOG_GENERIC,LOG_ERR,"Couldn't read SSL key file");
        ABORT(R_NOT_FOUND);
      }
    }

    if(!(conn->sock=nr_tcp_connect(host,port)))
      ABORT(R_NOT_FOUND);
    if(!(conn->ssl=SSL_new(conn->ssl_ctx)))
      ABORT(R_NO_MEMORY);
    if(!(conn->bio=BIO_new_socket(conn->sock,BIO_NOCLOSE)))
      ABORT(R_NO_MEMORY);
    SSL_set_bio(conn->ssl,conn->bio,conn->bio);

    if((r=SSL_connect(conn->ssl))<=0){
      r_log(LOG_GENERIC,LOG_ERR,"Couldn't SSL connect to %s: error=%s",
        host,ERR_reason_error_string(r));
      ABORT(R_NOT_FOUND);
    }

    if(r=check_ssl_cert_match(conn->ssl,cert,cert_l))
      ABORT(r);

    *connp=conn;
    
    _status=0;
  abort:
    if(_status){
      nr_ssl_conn_destroy(&conn);
    }
    return(_status);
  }

static int check_ssl_cert_match(SSL *ssl,UCHAR *cert,int cert_l)
  {
    X509 *x;
    int _status;
    UCHAR *der=0,*tmp;
    int len;
    
    x=SSL_get_peer_certificate(ssl);
    
    if((len=i2d_X509(x,0))<0){
      r_log(LOG_GENERIC,LOG_ERR,"Couldn't encode cert");
      ABORT(R_BAD_DATA);
    }

    if(!(der=(UCHAR *)RMALLOC(len)))
      ABORT(R_NO_MEMORY);

    tmp=der;
    if((len=i2d_X509(x,&tmp))<0){
      r_log(LOG_GENERIC,LOG_ERR,"Couldn't encode cert");
      ABORT(R_BAD_DATA);
    }

    if(len!=cert_l){
      r_log(LOG_GENERIC,LOG_ERR,"certificate does not match");
      ABORT(R_NOT_FOUND);
    }

    if(memcmp(der,cert,cert_l)){
      r_log(LOG_GENERIC,LOG_ERR,"certificate does not match");
      ABORT(R_NOT_FOUND);
    }

    _status=0;
  abort:
    if(x) X509_free(x);
    RFREE(der);
    return(_status);
  }

int nr_ssl_conn_create(rootlist,keyfile,host,port,connp)
  char *rootlist;
  char *keyfile;
  char *host;
  int port;
  ssl_conn **connp;
  {
    ssl_conn *conn=0;
    int r,_status;

    /* Create the SSL context */
    if(!openssl_initialized){
      openssl_initialized=1;
      SSL_library_init();
      SSL_load_error_strings();
    }
    
    if(!(conn=(ssl_conn *)RCALLOC(sizeof(ssl_conn))))
      ABORT(R_NO_MEMORY);
    conn->sock=-1;
    
    if(!(conn->ssl_ctx=SSL_CTX_new(TLSv1_method())))
      ABORT(R_NO_MEMORY);
    if(keyfile){
      if(!(SSL_CTX_use_certificate_chain_file(conn->ssl_ctx,keyfile))){
        r_log(LOG_GENERIC,LOG_ERR,"Couldn't read SSL cert chain file");
        ABORT(R_NOT_FOUND);
      }
      if(!(SSL_CTX_use_PrivateKey_file(conn->ssl_ctx,keyfile,
        SSL_FILETYPE_PEM))){
        r_log(LOG_GENERIC,LOG_ERR,"Couldn't read SSL key file");
        ABORT(R_NOT_FOUND);
      }
    }
    if(!(SSL_CTX_load_verify_locations(conn->ssl_ctx,rootlist,0))){
      r_log(LOG_GENERIC,LOG_ERR,"Couldn't load root list %s",
        rootlist);
      ABORT(R_BAD_ARGS);
    }
    
    if(!(conn->sock=nr_tcp_connect(host,port)))
      ABORT(R_NOT_FOUND);
    if(!(conn->ssl=SSL_new(conn->ssl_ctx)))
      ABORT(R_NO_MEMORY);
    if(!(conn->bio=BIO_new_socket(conn->sock,BIO_NOCLOSE)))
      ABORT(R_NO_MEMORY);
    SSL_set_bio(conn->ssl,conn->bio,conn->bio);

    if((r=SSL_connect(conn->ssl))<=0){
      r_log(LOG_GENERIC,LOG_ERR,"Couldn't SSL connect to %s: error=%s",
        host,ERR_reason_error_string(r));
      ABORT(R_NOT_FOUND);
    }
    if(r=nr_check_cert(conn->ssl,host))
      ABORT(r);

    *connp=conn;
    
    _status=0;
  abort:
    if(_status){
      nr_ssl_conn_destroy(&conn);
    }
    return(_status);
  }
  
int nr_ssl_conn_destroy(connp)
  ssl_conn **connp;
  {
    ssl_conn *conn;
    
    if(!connp || !*connp)
      return(0);

    conn=*connp;
    *connp=0;

    if(conn->ssl_ctx)
      SSL_CTX_free(conn->ssl_ctx);
    if(conn->ssl)
      SSL_free(conn->ssl);
    if(conn->sock==-1)
      NR_SOCKET_CLOSE(conn->sock);

    return(0);
  }

int nr_ssl_write_all(ssl,data,len)
  SSL *ssl;
  UCHAR *data;
  int len;
  {
    int r,_status;
    
    while(len){
      r=SSL_write(ssl,data,len);

      switch(SSL_get_error(ssl,r)){
        case SSL_ERROR_NONE:
          len-=r;
          data+=r;
          break;
        default:
          ABORT(r);
      }
    }

    _status=0;
  abort:
    return(_status);
  }

int nr_ssl_read_data(ssl,data,len,lenp)
  SSL *ssl;
  UCHAR *data;
  int len;
  int *lenp;
  {
    int r,_status;

    r=SSL_read(ssl,data,len);
    
    switch(SSL_get_error(ssl,r)){
      case SSL_ERROR_NONE:
        len=r;
        break;
      case SSL_ERROR_ZERO_RETURN:
        ABORT(R_BAD_DATA);
        break;
      default:
        ABORT(R_BAD_DATA);
        break;
    }

    *lenp=r;
    _status=0;
  abort:
    return(_status);
  }

#ifdef WIN32
  // TODO

#else
int nr_set_nonblock(sock)
  int sock;
  {
    int ofcmode;
    
    /*First we make the socket nonblocking*/
    ofcmode=fcntl(sock,F_GETFL,0);
    ofcmode|=O_NDELAY;
    if(fcntl(sock,F_SETFL,ofcmode))
      ERETURN(R_INTERNAL);

    return(0);
  }
#endif

int nr_ssl_read_x509_pem_data(str,xp)
  char *str;
  X509 **xp;
  {
    BIO *bio=0;
    int _status;
    X509 *x;
    
    if(!(bio=BIO_new_mem_buf(str,strlen(str))))
      ABORT(R_NO_MEMORY);
    if(!(x=PEM_read_bio_X509(bio,0,0,0))){
      r_log(LOG_GENERIC,LOG_ERR,"Couldn't read certificate");
      ABORT(R_BAD_DATA);
    }

    *xp=x;
    
    _status=0;
  abort:
    if(bio) BIO_free(bio);
    return(_status);
  }

     
int nr_ssl_read_private_key_pem_data(str,pp)
  char *str;
  EVP_PKEY **pp;
  {
    BIO *bio=0;
    int _status;
    EVP_PKEY *x;
    
    if(!(bio=BIO_new_mem_buf(str,strlen(str))))
      ABORT(R_NO_MEMORY);
    if(!(x=PEM_read_bio_PrivateKey(bio,0,0,0))){
      r_log(LOG_GENERIC,LOG_ERR,"Couldn't read private key");
      ABORT(R_BAD_DATA);
    }

    *pp=x;
    
    _status=0;
  abort:
    if(bio) BIO_free(bio);    
    return(_status);
  }

int nr_ssl_read_private_key_pem_data_pass(str,pass,pp)
  char *str;
  char *pass;
  EVP_PKEY **pp;
  {
    BIO *bio=0;
    int _status;
    EVP_PKEY *x;

    /* Suppress the callback */
    if(!pass)
      pass="";
    
    if(!(bio=BIO_new_mem_buf(str,strlen(str))))
      ABORT(R_NO_MEMORY);
    if(!(x=PEM_read_bio_PrivateKey(bio,0,0,pass))){
      r_log(LOG_GENERIC,LOG_ERR,"Couldn't read private key");
      ABORT(R_BAD_DATA);
    }

    *pp=x;
    
    _status=0;
  abort:
    if(bio) BIO_free(bio);    
    return(_status);
  }


/* This keeps a cache to cut down on unnecessary parsing */
int nr_ssl_util_parse_cert(data,len,certp)
  UCHAR *data;
  int len;
  X509 **certp;
  {
    static r_assoc *certs;
    static TAILQ_HEAD(parsed_cert_head_,parsed_cert_) cert_list;
    static int cert_count;
    parsed_cert *cert=0;
    int r,_status;
    void *v;
    
    if(!(certs)){
      if(r=r_assoc_create(&certs,r_assoc_crc32_hash_compute,13))
        ABORT(r);
      TAILQ_INIT(&cert_list);
    }

    if(r=r_assoc_fetch(certs,(char*)data,len,&v)){
      if(r!=R_NOT_FOUND)
        ABORT(r);
    }
    else{
      *certp=((parsed_cert *)v)->x509;
      goto done;
    }

    /* Ok, we need to parse this new cert */
    if(cert_count==MAX_PARSED_CERTS){
      cert=TAILQ_FIRST(&cert_list);
      TAILQ_REMOVE(&cert_list,cert,entry);
      nr_ssl_util_destroy_cert(&cert);
      cert_count--;
    }

    if(r=_ssl_util_parse_cert(data,len,&cert))
      ABORT(r);
    
    if(r=r_assoc_insert(certs,(char*)data,len,(void *)cert,0,nr_ssl_util_destroy_cert_v,
      R_ASSOC_NEW))
      ABORT(r);

    cert_count++;

    *certp=cert->x509;

  done:
    _status=0;
  abort:
    if(_status){
      nr_ssl_util_destroy_cert(&cert);
    }
    return(_status);
  }

int _ssl_util_parse_cert(data,len,certp)
  UCHAR *data;
  int len;
  parsed_cert **certp;
  {
    parsed_cert *cert=0;
    UCHAR *d;
    int _status;
    
    if(!(cert=(parsed_cert *)RCALLOC(sizeof(parsed_cert))))
      ABORT(R_NO_MEMORY);

    d=data;

    if(!(cert->x509=d2i_X509(0,&d,len))){
      r_log(LOG_GENERIC,LOG_ERR,"Couldn't parse cert");
      ABORT(R_BAD_DATA);
    }

    *certp=cert;
    
    _status=0;
  abort:
    if(_status){
      nr_ssl_util_destroy_cert(&cert);
    }
    return(_status);
  }
    
int nr_ssl_util_destroy_cert(certp)
  parsed_cert **certp;
  {
    parsed_cert *cert;

    if(!certp || !*certp)
      return(0);

    cert=*certp;

    if(cert->x509)
      X509_free(cert->x509);

    RFREE(cert);

    *certp=0;

    return(0);
  }

int nr_ssl_util_destroy_cert_v(v)
  void *v;
  {
    parsed_cert *cert;

    cert=(parsed_cert *)v;

    nr_ssl_util_destroy_cert(&cert);
    
    return(0);
  }

#endif  /* OPENSSL */

