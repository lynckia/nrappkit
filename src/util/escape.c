/**
   escape.c

   
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
   

   briank@networkresonance.com  Wed Jun  7 14:09:29 PDT 2006
 */

static char *RCSSTRING __UNUSED__ ="$Id: escape.c,v 1.2 2006/08/16 19:39:17 adamcain Exp $";

#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include "nr_common.h"
#include "r_common.h"
#include "escape.h"
#include "r_errors.h"

int
nr_isshok(int c)
{
    if (isalnum(c))
        return 1;

    switch (c) {
       case ',':
       case '_':
       case '.':
       case ':':
       case '-':
           /* the above characters don't require quoting in the shell */
           return 1;
    }

    return 0;
}

/* returns R_REJECTED is string doesn't need to be quoted */
int
nr_shell_quote_string(char *string, char **quoted)
{
    int _status;
    int needs_quoting;
    char *p;
    int len;
    int i;
    char *s = 0;

    needs_quoting = 0;
    for (p = string; *p != '\0'; ++p) {
        if (! nr_isshok(*p))
            ++needs_quoting;
    }

    if (! needs_quoting)
        ABORT(R_REJECTED);

    len = p - string;

    /* 4 because worse case ' (1 char) becomes '"'"' (4 chars longer) */
    s = RMALLOC(len+(4*needs_quoting)+3);
    if (! s)
        ABORT(R_NO_MEMORY);

    p = s;
    *p++ = '\'';
    for (i = 0; i < len; ++i) {
        if (string[i] == '\'') {
            *p++ = '\'';
            *p++ = '"';
            *p++ = '\'';
            *p++ = '"';
        }
        *p++ = string[i];
    }
    *p++ = '\'';
    *p++ = '\0';

    *quoted = s;

    _status = 0;
abort:
    if (_status) RFREE(s);
    return _status;
}

