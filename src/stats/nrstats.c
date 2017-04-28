/*
 *
 *    nrstats.c
 *
 *    $Source: /cvsroot/nrappkit/nrappkit/src/stats/nrstats.c,v $
 *    $Revision: 1.3 $
 *    $Date: 2007/06/26 22:37:55 $
 *
 *    API for keeping and sharing statistics
 *
 *    
 *    Copyright (C) 2003, Network Resonance, Inc.
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

#ifdef LINUX
#undef _BSD_SOURCE
#define SVID_SOURCE
#endif

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <limits.h>
#include <time.h>
#include <sys/shm.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>

#include "nr_common.h"
#include "nrstats.h"
#include "r_macros.h"
#include "r_errors.h"
#include "mutex.h"

/* a large buffer is allocated so that changes to the
 * stats structures don't cause SEGVs when those structures
 * grow, virtual memory will swap out the unused portion */
#define MAX_NR_STATS_SIZE    0x4ffff

/* the maximum size of the complete name of a stat */
#define NR_MAX_STATS_NAME    (NR_MAX_STATS_TYPE_NAME*3)

/* the maximum number of stats buffers to allocate */
#define NR_MAX_STATS_SHMS    64

/* shared memory and semaphore keys */
#define SHMKEY   ftok("/", 0x53484D )

/* wrapper structure for all stats obtained from NR_stats_get() */
typedef struct wrapper_ {
    char               name[NR_MAX_STATS_NAME];
    char               libname[PATH_MAX];    
    int                size;     /* size of buffer only */
    double             buffer;   /* used for obtaining stats address offset */
} wrapper;

#define STATS2WRAPPER(stats)  \
        ((wrapper*)(((char*)(stats))-offsetof(struct wrapper_, buffer)))
#define FIRSTWRAPPER ((wrapper*)&(((shm_info*)shm)[1]))
/* NEXTWRAPPER is not the optimal packing in memory, but close enough */
#define NEXTWRAPPER(header) \
        ((header) + (2 + ((header)->size / sizeof(*(header)))))

/* info structure at start of shared memory segment */
typedef struct shm_info_ {
    unsigned int    nshms;
} shm_info;

static void null_printf(void *handle, const char *fmt, ...) { }

static char           *app                = 0;
static char           *user               = 0;
static void          (*errprintf)(void *errhandle, const char *fmt, ...) = null_printf;
static void           *errhandle          = 0;
static int             shmid              = -1;
static void           *shm                = 0;
static shm_info       *shminfo            = 0;
static int             mutex              = -1;
static char           *names[NR_MAX_STATS_SHMS] = { 0 };
static NR_stats_type  *types[NR_MAX_STATS_TYPES] = { 0 };
static unsigned int    ntypes             = 0;

static void
init_values()
{
    app        = 0;
    user       = 0;
    errprintf  = null_printf;
    errhandle  = 0;
    shmid      = -1;
    shm        = 0;
    shminfo    = 0;
    mutex      = -1;
}

static int
compute_name(char *module_name, char *type_name, char out[NR_MAX_STATS_NAME])
{
    if (module_name) {
        if (snprintf(out, NR_MAX_STATS_NAME-1, "%s.%s.%s", app, module_name, type_name) >= NR_MAX_STATS_NAME) {
            return R_BAD_ARGS;
        }
    }
    else {
        if (snprintf(out, NR_MAX_STATS_NAME-1, "%s.%s", app, type_name) >= NR_MAX_STATS_NAME) {
         return R_BAD_ARGS;
        }
    }

    return 0;
}

static int
get_type_name_from_name(char *name, char **type_name)
{
    char *t = 0;

    t = strrchr(name, '.');
    if (t == 0)
        return R_BAD_ARGS;

    *type_name = t+1;

    return 0;  
}

static int
find_type_index(char *type_name, int *index)
{
    int i;

    for (i = 0; i < ntypes; ++i) {
        if (! strcmp(type_name, types[i]->name)) {
            *index = i;
            return 0;
        }
    }

    return R_BAD_ARGS;
}

static int
get_type_from_name(char *name, NR_stats_type **type)
{
    char *type_name = 0;
    int i;
    int r, _status;

    if (r=get_type_name_from_name(name, &type_name))
        ABORT(r);

    if (r=find_type_index(type_name, &i))
        ABORT(r);

    *type = types[i];

    _status=0;
  abort:
    return(_status);
}

static int
find_stats(char *name, wrapper **out)
{
    wrapper *header = 0;
    int i;

    *out = 0;

    header = FIRSTWRAPPER;
    for (i = 0; i < shminfo->nshms; ++i) {
        if (! strcmp(name, header->name)) {
            *out = header;
            break;
        }
        header = NEXTWRAPPER(header);
    }

    return 0;
}

static int
find_free_wrapper(wrapper **out)
{
    wrapper *header = 0;
    int i;

    *out = 0;

    header = FIRSTWRAPPER;
    for (i = 0; i < shminfo->nshms; ++i) {
        header = NEXTWRAPPER(header);
    }

    *out = header;

    return 0;
}

static int
alloc_stats(char *name, char *libname, unsigned int size, wrapper **out)
{
    wrapper *header = 0;
    int r, _status;
    
    if (shminfo->nshms >= NR_MAX_STATS_SHMS)
        ABORT(R_NO_MEMORY);

    if (r=find_free_wrapper(&header))
        ABORT(R_NO_MEMORY);

    if (((char*)header) > (((char*)shm) + MAX_NR_STATS_SIZE))
        ABORT(R_NO_MEMORY);

    strncpy(header->name, name, sizeof(header->name));
    if (libname)
      strncpy(header->libname, libname, sizeof(header->libname));
    else
      header->libname[0] = '\0';
    
    header->size = size;

    ++(shminfo->nshms);

    *out = header;

    _status=0;
  abort:
    return(_status);
}

static int
sanity_check(void *stats) {
    wrapper *header = STATS2WRAPPER(stats);
    wrapper *sanity = 0;

    /* sanity check that this is a reasonable wrapper by doing a
     * lookup on the name and seeing if the same wrapper comes back */
    if (find_stats(header->name, &sanity) != 0 || header != sanity)
        return R_BAD_ARGS;

    return 0;
}

static int
register_type(NR_stats_type *type)
{
    int i;

    if (find_type_index(type->name, &i) != 0) {
        /* insert at end of list */

        if (ntypes >= NR_MAX_STATS_TYPES)
            return R_NO_MEMORY;

        types[ntypes++] = type;
    }
    else {
        /* replace an existing type */
        types[i] = type;
    }

    return 0;    
}

int
NR_stats_startup(char *app_name, char *user_name, void (*err)(void *errhandle, const char *fmt, ...), void *errhandle)
{
    int r, _status;
    int shmcreated = 0;

    app = app_name;
    user = user_name;
    errprintf = (err == 0) ? null_printf : err;
    errhandle = (err == 0) ? 0 : errhandle;

    /* shared memory */
    if (shmid < 0 || shm == 0) {
        shmid = shmget(SHMKEY, 0, 0);
        if (shmid < 0) {
            if (errno != ENOENT)
                ABORT(R_FAILED);

            shmid = shmget(SHMKEY, MAX_NR_STATS_SIZE, IPC_CREAT | SHM_R | SHM_W);
            if (shmid < 0)
                ABORT(R_FAILED);

            shmcreated = 1;
        }

        shm = shmat(shmid, 0, SHM_RND);
        if (shm == (void*)(-1))
            ABORT(R_FAILED);

        shminfo = (shm_info*)shm;

        if (shmcreated) {
            shminfo->nshms = 0;
        }
    }

    /* semaphore */
    if (mutex < 0) {
        if ((r=nr_util_create_mutex(&mutex, NR_TEMP_DIR "stats.lock", S_IRUSR|S_IWUSR)))
            ABORT(r);
    }

    register_type(NR_stats_type_app);
    register_type(NR_stats_type_memory);

    _status=0;
  abort:
    if (_status!=0) {
        errprintf(errhandle, "Unable to allocate IPC: %s\n", strerror(errno));
        close(mutex);
        init_values();
    }
    return(_status);
}

int
NR_stats_shutdown()
{
    if (app == 0)
        return R_ALREADY;

    if (shm != 0) {
        if (shmdt(shm) < 0) {
            errprintf(errhandle, "Unable to detach shared memory: %s\n", strerror(errno));
            return R_FAILED;
        }
    }

    if (mutex >= 0)
        close(mutex);

    /* just to make doubly sure that things are really shut down */
    init_values();

    return 0;
}

/* stats is copied by reference */
int
NR_stats_get(char *module_name, NR_stats_type *type, int flag, void **stats)
{
    wrapper *header = 0;
    char name[NR_MAX_STATS_NAME];
    int r, _status;

    if (app == 0)
        ABORT(R_FAILED);

    if (r=register_type(type))
        ABORT(r);

    if (r=compute_name(module_name, type->name, name))
        ABORT(r);

    if (r=find_stats(name, &header))
        ABORT(r);

    if (header == 0) {
      if (flag & NR_STATS_CREATE) {
        char *libname="";
        if(type->get_lib_name){
          if(r=type->get_lib_name(&libname))
            ABORT(r);
        }
        if (r=alloc_stats(name, libname, type->size, &header))
            ABORT(r);
      }
      else {
          ABORT(R_NOT_FOUND);
      }
    }

    *stats = (void*)&(header->buffer);

    assert(header == STATS2WRAPPER(*stats));
    assert(sanity_check(*stats) == 0);

    _status=0;
  abort:
    return(_status);
}

int
NR_stats_clear(void *stats)
{
    wrapper *header = 0;
    int r, _status;

    if (r=sanity_check(stats))
        ABORT(r);

    header = STATS2WRAPPER(stats);

    memset(&header->buffer, 0, header->size);
    NR_stats_reset(stats);

    _status=0;
  abort:
    return(_status);
}

int
NR_stats_reset(void *stats)
{
    wrapper *header = 0;
    NR_stats_type *type = 0;
    int r, _status;

    if (r=sanity_check(stats))
        ABORT(r);

    header = STATS2WRAPPER(stats);

    if (r=get_type_from_name(header->name, &type))
        ABORT(r);

    type->reset(stats);

    _status=0;
  abort:
    return(_status);
}

int
NR_stats_register(NR_stats_type *type)
{
    int r, _status;

    if (app == 0)
        ABORT(R_FAILED);

    if (r=register_type(type))
        ABORT(r);

    _status=0;
  abort:
    return(_status);
}


int
NR_stats_acquire_mutex(void *stats)
{
    int r, _status;

    assert(sanity_check(stats) >= 0);

    if (mutex >= 0) {
        if ((r=nr_util_acquire_mutex(mutex)))
            ABORT(r);
    }

    _status=0;
  abort:
    return(_status);
}

int
NR_stats_release_mutex(void *stats)
{
    int r, _status;

    if (mutex >= 0) {
        if ((r=nr_util_release_mutex(mutex)))
            ABORT(r);
    }

    assert(sanity_check(stats) >= 0);

    _status=0;
  abort:
    return(_status);
}

int
NR_stats_get_names(unsigned int *nn, char ***n)
{
    wrapper *header = 0;
    int i;

    if (app == 0)
        return R_FAILED;

    header = FIRSTWRAPPER;
    for (i = 0; i < shminfo->nshms; ++i) {
        names[i] = header->name;
        header = NEXTWRAPPER(header);
    }

    *nn = shminfo->nshms;
    *n = names;

    return 0;
}

int
NR_stats_get_by_name(char *name, NR_stats_type **type, void **stats)
{
    wrapper *header = 0;
    NR_stats_type *tmp = 0;
    int r, _status;

    if (app == 0)
        ABORT(R_FAILED);

    if (r=find_stats(name, &header))
        ABORT(r);

    if(!header)
        ABORT(R_NOT_FOUND);
    
    if (type != 0) {
        *type = (get_type_from_name(name, &tmp) == 0) ? tmp : 0;
    }

    if (stats != 0)
        *stats = (void*)(&header->buffer);

    _status=0;
  abort:
    return(_status);
}

int
NR_stats_rmids(void)
{
    int id;
    int _status;

    if (shmid >= 0)
        ABORT(R_FAILED);

    id = shmget(SHMKEY, 0, 0);
    if (id < 0) {
        if (errno != ENOENT)
            ABORT(R_FAILED);
    }
    else {
        if (shmctl(id, IPC_RMID, 0) < 0)
            ABORT(R_FAILED);
    }

    _status=0;
  abort:
    if (_status != 0)
        errprintf(errhandle, "Unable to remove IPC: %s\n", strerror(errno));
    return(_status);
}


  
int NR_stats_get_lib_name(void *stats, char **lib_name)
{
    wrapper *header = 0;
    int r, _status;

    if (r=sanity_check(stats))
        ABORT(r);

    header = STATS2WRAPPER(stats);

    *lib_name = header->libname;

    _status=0;
  abort:
    return(_status);
}


/*
 * Remove program name in prefix of the form program_name.module_name 
 */
char *
NR_prefix_to_stats_module(char *prefix)
{
    char *p;

    p = strchr(prefix, '.');
    if (!p || (strlen(p) < 2))
        return prefix;
    p++;
    return p;
}

