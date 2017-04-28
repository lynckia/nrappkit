/**
   nrregistryctl.c

   
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
   

   briank@networkresonance.com  Fri Feb 10 11:30:31 PST 2006
 */


static char *RCSSTRING __UNUSED__ ="$Id: nrregctl.c,v 1.3 2007/06/26 22:37:50 adamcain Exp $";

#include <stdio.h>
#include <time.h>
#include <ctype.h>
#ifdef WIN32
#include <fcntl.h>
#else
#include <unistd.h>
#include <sys/fcntl.h>
#endif
#include <async_wait.h>
#include "registry.h"
#include "registry_int.h"
#include "r_assoc.h"
#include "nr_common.h"
#include "r_log.h"
#include "r_errors.h"
#include "r_macros.h"
#include "escape.h"

static int usage(void);
static int fail(char *what, char *name, int r);
static int nrregctl_dump(char *from, int shell_mode);
static int nrregctl_read(char *name, int shell_mode);
static int nrregctl_write(char *name, char *str, char *value);
static int nrregctl_delete(char *name);
static int nrregctl_list(char *name);
static int nrregctl_final(char *name);
static void nrregctl_watch_cb(void *cb_arg, char action, NR_registry name);
static int nrregctl_watch(char *name);
int main(int argc, char **argv);

static char *PROGNAME = "nrregctl";

int
usage()
{
    fprintf(stderr, "usage: %s write [type] name value\n", PROGNAME);
    fprintf(stderr, "       %s write [type] name @filename\n", PROGNAME);
    fprintf(stderr, "       %s write registry name\n", PROGNAME);
    fprintf(stderr, "       %s final name\n", PROGNAME);
    fprintf(stderr, "       %s read name\n", PROGNAME);
    fprintf(stderr, "       %s delete name\n", PROGNAME);
    fprintf(stderr, "       %s list name\n", PROGNAME);
    fprintf(stderr, "       %s dump [name]\n", PROGNAME);
    fprintf(stderr, "       %s export [name]\n", PROGNAME);
    fprintf(stderr, "       %s watch [name]\n", PROGNAME);
    exit(1);
}

int
fail(char *what, char *name, int r)
{
    if (name)
        fprintf(stderr, "%s: %s\n", name, nr_strerror(r));
    else
        fprintf(stderr, "%s: %s\n", PROGNAME, nr_strerror(r));
    exit(1);
}

int
nrregctl_dump(char *from, int shell_mode)
{
    int r;
    int i;
    unsigned int size;
    size_t count;
    NR_registry_type str;
    int type;

    if (! from)
        from = "";

    size = 0;
    if ((r=NR_reg_get_child_count(from, &size)))
        if (r != R_NOT_FOUND)
            fail("dump", from, r);

    if (size > 0) {
        /* +100 fudge factor, just to be safe */
        NR_registry children[size+100];

        if ((r=NR_reg_get_children(from, children, size, &count)))
            fail("dump", from, r);

        for (i = 0; i < count; ++i) {
            if ((r=NR_reg_get_type(children[i], str)))
                fail("dump", children[i], r);

            if ((r=nr_reg_compute_type(str, &type)))
                fail("dump", children[i], r);
       
            if (type == NR_REG_TYPE_REGISTRY) {
                if ((r=nrregctl_dump(children[i], shell_mode)))
                    fail("dump", children[i], r);
            }
            else {
                if (shell_mode) {
                    char *child = children[i];

                    nr_shell_quote_string(child, &child);
                    printf("%s write %s %s ", PROGNAME, str, child);
                }
                else {
                    printf("%s: ", children[i]);
                }

                if ((r=nrregctl_read(children[i], shell_mode)))
                    fail("dump", children[i], r);
            }
        }
    }

    return 0;
}

int
nrregctl_read(char *name, int shell_mode)
{
    int r;
    NR_registry_type str;
    int type;
    int i;
    int escape;
    char          _char;
    UCHAR         _uchar;
    INT2          _int2;
    UINT2         _uint2;
    INT4          _int4;
    UINT4         _uint4;
    INT8          _int8;
    UINT8         _uint8;
    double        _double;
    NR_registry   _registry;
    char         *_string;
    Data          _bytes;

    if ((r=NR_reg_get_type(name, str)))
        fail("read", name, r);

    if ((r=nr_reg_compute_type(str, &type)))
        fail("read", name, r);

    switch (type) {
    case NR_REG_TYPE_CHAR:
        if ((r=NR_reg_get_char(name, &_char)))
            fail("read", name, r);
        if (shell_mode)
            escape = ! nr_isshok(_char);
        else
            escape = (isspace(_char) || !isprint(_char));
        if (escape)
            printf("%s\\%03o\n", (shell_mode?"\\":""), _char);
        else
            printf("%c\n", _char);
        break;
    case NR_REG_TYPE_UCHAR:
        if ((r=NR_reg_get_uchar(name, &_uchar)))
            fail("read", name, r);
        printf("0x%02x\n", _uchar);
        break;
    case NR_REG_TYPE_INT2:
        if ((r=NR_reg_get_int2(name, &_int2)))
            fail("read", name, r);
        printf("%d\n", _int2);
        break;
    case NR_REG_TYPE_UINT2:
        if ((r=NR_reg_get_uint2(name, &_uint2)))
            fail("read", name, r);
        printf("%u\n", _uint2);
        break;
    case NR_REG_TYPE_INT4:
        if ((r=NR_reg_get_int4(name, &_int4)))
            fail("read", name, r);
        printf("%d\n", _int4);
        break;
    case NR_REG_TYPE_UINT4:
        if ((r=NR_reg_get_uint4(name, &_uint4)))
            fail("read", name, r);
        printf("%u\n", _uint4);
        break;
    case NR_REG_TYPE_INT8:
        if ((r=NR_reg_get_int8(name, &_int8)))
            fail("read", name, r);
        printf("%lld\n", _int8);
        break;
    case NR_REG_TYPE_UINT8:
        if ((r=NR_reg_get_uint8(name, &_uint8)))
            fail("read", name, r);
        printf("%llu\n", _uint8);
        break;
    case NR_REG_TYPE_DOUBLE:
        if ((r=NR_reg_get_double(name, &_double)))
            fail("read", name, r);
        printf("%f\n", _double);
        break;
    case NR_REG_TYPE_BYTES:
        if ((r=NR_reg_alloc_data(name, &_bytes)))
            fail("read", name, r);

        for (i = 0; i < _bytes.len; ++i) {
            printf("%02x", ((UCHAR*)_bytes.data)[i]);
            if ((i % 35) == 34)
                printf("\n");
        }
        if ((--i % 35) != 34)
            printf("\n");
        break;
    case NR_REG_TYPE_STRING:
        if ((r=NR_reg_alloc_string(name, &_string)))
            fail("read", name, r);
        if (shell_mode)
            nr_shell_quote_string(_string, &_string);
        printf("%s\n", _string);
        break;
    case NR_REG_TYPE_REGISTRY:
        if ((r=NR_reg_get_registry(name, _registry)))
            fail("read", name, r);
        _string = _registry;
        if (shell_mode)
            nr_shell_quote_string(_string, &_string);
        printf("%s\n", _string);
        break;
    default:
        fail("write", name, R_INTERNAL);
    }

    return 0;
}

int
nrregctl_write(char *str, char *name, char *value)
{
    int r;
    int type;
    int i;
    int fd;
    char          _char;
    int           _int;
    INT4          _int4;
    UINT4         _uint4;
    INT8          _int8;
    UINT8         _uint8;
    double        _double;
    Data          _bytes;
    char          _hex[3];
    NR_registry_type str2;

    if (str == 0) {
        if ((r=NR_reg_get_type(name, str2)))
            usage();

        str = str2;
    }

    if ((r=nr_reg_compute_type(str, &type)))
        fail("write", name, r);

    switch (type) {
    default:
        if (value == 0)
            usage();
        if (strlen(value) < 1)
            usage();
        break;
    case NR_REG_TYPE_REGISTRY:
        if (value != 0)
            usage();
        break;
    case NR_REG_TYPE_STRING:
    case NR_REG_TYPE_BYTES:
        if (value == 0)
            usage();
        break;
    }

    if (value != 0 && value[0] == '@') {
        fd = open(&value[1], O_RDONLY, 0);
        if (fd < 0) {
            perror(&value[1]);
            exit(1);
        }
        
        value = malloc(10000000);
        if (! value) {
            perror(&value[1]);
            exit(1);
        }

        if (NR_SOCKET_READ(fd, value, 10000000) < 0) {
            perror(&value[1]);
            exit(1);
        }
    }

    switch (type) {
    case NR_REG_TYPE_CHAR:
        if (value[0] == '\\' && isdigit(value[1])) {
            sscanf(&value[1], "%o", &_int);
            _char = (char)_int;
        }
        else if (value[0] == '0' && value[1] == 'x' && isxdigit(value[2])) {
            sscanf(&value[2], "%x", &_int);
            _char = (char)_int;
        }
        else {
            sscanf(value, "%c", &_char);
        }
        if ((r=NR_reg_set_char(name, _char)))
            fail("write", name, r);
        break;
    case NR_REG_TYPE_UCHAR:
        if (value[0] == '\\' && isdigit(value[1])) {
            sscanf(&value[1], "%o", &_int);
        }
        else if (value[0] == '0' && value[1] == 'x' && isxdigit(value[2])) {
            sscanf(&value[2], "%x", &_int);
        }
        else {
            usage();
        }

        if ((r=NR_reg_set_uchar(name, (UCHAR)_int)))
            fail("write", name, r);
        break;
    case NR_REG_TYPE_INT2:
        sscanf(value, "%d", &_int);
        if ((r=NR_reg_set_int2(name, (INT2)_int)))
            fail("write", name, r);
        break;
    case NR_REG_TYPE_UINT2:
        sscanf(value, "%u", &_int);
        if ((r=NR_reg_set_uint2(name, (UINT2)_int)))
            fail("write", name, r);
        break;
    case NR_REG_TYPE_INT4:
        sscanf(value, "%d", &_int4);
        if ((r=NR_reg_set_int4(name, _int4)))
            fail("write", name, r);
        break;
    case NR_REG_TYPE_UINT4:
        sscanf(value, "%u", &_uint4);
        if ((r=NR_reg_set_uint4(name, _uint4)))
            fail("write", name, r);
        break;
    case NR_REG_TYPE_INT8:
        sscanf(value, "%lld", &_int8);
        if ((r=NR_reg_set_int8(name, _int8)))
            fail("write", name, r);
        break;
    case NR_REG_TYPE_UINT8:
        sscanf(value, "%llu", &_uint8);
        if ((r=NR_reg_set_uint8(name, _uint8)))
            fail("write", name, r);
        break;
    case NR_REG_TYPE_DOUBLE:
        sscanf(value, "%lf", &_double);
        if ((r=NR_reg_set_double(name, _double)))
            fail("write", name, r);
        break;
    case NR_REG_TYPE_BYTES:
        _bytes.len = strlen(value);
        _bytes.data = malloc(_bytes.len);
        if (_bytes.len > 2 && value[0] == '0' && value[1] == 'x') {
            value += 2;
            _bytes.len -= 2;
        }
        if (_bytes.len % 2) 
            usage();
        _bytes.len /= 2;

        _hex[2] = '\0';
        for (i = 0; i < _bytes.len; ++i) {
            _hex[0] = value[i*2];
            if (isspace(_hex[0])) {
                _bytes.len -= 1;
                continue;
            }
            _hex[1] = value[(i*2)+1];
            sscanf(_hex, "%x", &_int);
            ((UCHAR*)_bytes.data)[i] = (UCHAR)_int;
        }
        break;
    case NR_REG_TYPE_STRING:
        if ((r=NR_reg_set_string(name, value)))
            fail("write", name, r);
        break;
    case NR_REG_TYPE_REGISTRY:
        if ((r=NR_reg_set_registry(name)))
            fail("write", name, r);
        break;
    default:
        fail("write", name, R_INTERNAL);
    }

    return 0;
}

int
nrregctl_delete(char *name)
{
    int r;

    if ((r=NR_reg_del(name)))
        fail("delete", name, r);

    return 0;
}

int
nrregctl_list(char *name)
{
    int r;
    int i;
    unsigned int size;
    size_t count;

    size = 0;
    if ((r=NR_reg_get_child_count(name, &size)))
        if (r != R_NOT_FOUND)
            fail("list", name, r);

    if (size > 0) {
        NR_registry children[size+1];

        if ((r=NR_reg_get_children(name, children, size, &count)))
            fail("list", name, r);

        qsort(children, count, sizeof(*children), (void*)strcmp);

        for (i = 0; i < count; ++i) {
            printf("%s\n", children[i]);
        }
    }

    return 0;
}

int
nrregctl_final(char *name)
{
    int r;

    if ((r=NR_reg_fin(name)))
        fail("final", name, r);

    return 0;
}

void
nrregctl_watch_cb(void *cb_arg, char action, NR_registry name)
{
    time_t clock = time(0);
    char *t = ctime(&clock);
    char *s = nr_reg_action_name(action);

    t[strlen(t)-1] = '\0';
    printf("%s: %s %s\n", t, s, name);
}

int
nrregctl_watch(char *name)
{
    int r;

    if ((r=NR_reg_register_callback(name, NR_REG_CB_ACTION_ADD|NR_REG_CB_ACTION_CHANGE|NR_REG_CB_ACTION_DELETE|NR_REG_CB_ACTION_FINAL, nrregctl_watch_cb, 0)))
        fail("register_callback", name, r);

    while (!NR_async_event_wait(&r));

    return 0;
}

int
main(int argc, char **argv)
{
    int r;

    PROGNAME = strrchr(argv[0], '/');
    if (PROGNAME)
        ++PROGNAME;
    else
        PROGNAME = argv[0];

    if ((r=NR_reg_init(NR_REG_MODE_REMOTE))) {
        fprintf(stderr, "%s: %s\n", PROGNAME, nr_strerror(r));
        exit(1);
    }

    switch (argc) {
    case 2:
        if (!strcmp(argv[1], "dump")) nrregctl_dump(0, 0);
        else if (!strcmp(argv[1], "export")) nrregctl_dump(0, 1);
        else if (!strcmp(argv[1], "watch")) nrregctl_watch("");
        else usage();
        break;
    case 3:
        if (!strcmp(argv[1], "read")) nrregctl_read(argv[2], 0);
        else if (!strcmp(argv[1], "delete")) nrregctl_delete(argv[2]);
        else if (!strcmp(argv[1], "dump")) nrregctl_dump(argv[2], 0);
        else if (!strcmp(argv[1], "export")) nrregctl_dump(argv[2], 1);
        else if (!strcmp(argv[1], "list")) nrregctl_list(argv[2]);
        else if (!strcmp(argv[1], "final")) nrregctl_final(argv[2]);
        else if (!strcmp(argv[1], "watch")) nrregctl_watch(argv[2]);
        else usage();
        break;
    case 4:
        if (!strcmp(argv[1], "write")) {
            if (!strcmp(argv[2], "registry"))
                nrregctl_write(argv[2], argv[3], 0);
            else
                nrregctl_write(0, argv[2], argv[3]);
        }
        else usage();
        break;
    case 5:
        if (!strcmp(argv[1], "write")) nrregctl_write(argv[2], argv[3], argv[4]);
        else usage();
        break;
    default:
        usage();
        break;
    }

    return 0;
}

