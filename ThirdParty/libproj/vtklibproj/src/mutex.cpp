/******************************************************************************
 * Project:  PROJ.4
 * Purpose:  Mutex (thread lock) functions.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2009, Frank Warmerdam
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

/* For PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "proj.h"
#ifndef _WIN32
#include "proj_config.h"
#endif
#include "proj_internal.h"

/* on win32 we always use win32 mutexes, even if pthreads are available */
#if defined(_WIN32) && !defined(MUTEX_stub)
#ifndef MUTEX_win32
#  define MUTEX_win32
#endif
#  undef  MUTEX_pthread
#endif

#if !defined(MUTEX_stub) && !defined(MUTEX_pthread) && !defined(MUTEX_win32)
#  define MUTEX_stub
#endif

/************************************************************************/
/* ==================================================================== */
/*                      stub mutex implementation                       */
/* ==================================================================== */
/************************************************************************/

#ifdef MUTEX_stub

/************************************************************************/
/*                            pj_acquire_lock()                         */
/*                                                                      */
/*      Acquire the PROJ.4 lock.                                        */
/************************************************************************/

void pj_acquire_lock()
{
}

/************************************************************************/
/*                            pj_release_lock()                         */
/*                                                                      */
/*      Release the PROJ.4 lock.                                        */
/************************************************************************/

void pj_release_lock()
{
}

/************************************************************************/
/*                          pj_cleanup_lock()                           */
/************************************************************************/
void pj_cleanup_lock()
{
}

#endif /* def MUTEX_stub */

/************************************************************************/
/* ==================================================================== */
/*                    pthread mutex implementation                      */
/* ==================================================================== */
/************************************************************************/

#ifdef MUTEX_pthread

#include "pthread.h"

#ifdef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
static pthread_mutex_t core_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#else
static pthread_mutex_t core_lock;

/************************************************************************/
/*                          pj_create_lock()                            */
/************************************************************************/

static void pj_create_lock()
{
    /*
    ** We need to ensure the core mutex is created in recursive mode
    */
    pthread_mutexattr_t mutex_attr;

    pthread_mutexattr_init(&mutex_attr);
#ifdef HAVE_PTHREAD_MUTEX_RECURSIVE
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
#else
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
    pthread_mutex_init(&core_lock, &mutex_attr);
}

#endif /* PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP */

/************************************************************************/
/*                          pj_acquire_lock()                           */
/*                                                                      */
/*      Acquire the PROJ.4 lock.                                        */
/************************************************************************/

void pj_acquire_lock()
{

#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
    static pthread_once_t sOnceKey = PTHREAD_ONCE_INIT;
    if( pthread_once(&sOnceKey, pj_create_lock) != 0 )
    {
        fprintf(stderr, "pthread_once() failed in pj_acquire_lock().\n");
    }
#endif

    pthread_mutex_lock( &core_lock);
}

/************************************************************************/
/*                          pj_release_lock()                           */
/*                                                                      */
/*      Release the PROJ.4 lock.                                        */
/************************************************************************/

void pj_release_lock()
{
    pthread_mutex_unlock( &core_lock );
}

/************************************************************************/
/*                          pj_cleanup_lock()                           */
/************************************************************************/
void pj_cleanup_lock()
{
}

#endif /* def MUTEX_pthread */

/************************************************************************/
/* ==================================================================== */
/*                      win32 mutex implementation                      */
/* ==================================================================== */
/************************************************************************/

#ifdef MUTEX_win32

#include <windows.h>

static HANDLE mutex_lock = nullptr;

#if _WIN32_WINNT >= 0x0600

/************************************************************************/
/*                          pj_create_lock()                            */
/************************************************************************/

static BOOL CALLBACK pj_create_lock(PINIT_ONCE InitOnce,
                                    PVOID Parameter,
                                    PVOID *Context)
{
    (void)InitOnce;
    (void)Parameter;
    (void)Context;
    mutex_lock = CreateMutex( nullptr, FALSE, nullptr );
    return TRUE;
}
#endif

/************************************************************************/
/*                            pj_init_lock()                            */
/************************************************************************/

static void pj_init_lock()

{
#if _WIN32_WINNT >= 0x0600
    static INIT_ONCE sInitOnce = INIT_ONCE_STATIC_INIT;
    InitOnceExecuteOnce( &sInitOnce, pj_create_lock, nullptr, nullptr );
#else
    if( mutex_lock == nullptr )
        mutex_lock = CreateMutex( nullptr, FALSE, nullptr );
#endif
}

/************************************************************************/
/*                          pj_acquire_lock()                           */
/*                                                                      */
/*      Acquire the PROJ.4 lock.                                        */
/************************************************************************/

void pj_acquire_lock()
{
    if( mutex_lock == nullptr )
        pj_init_lock();

    WaitForSingleObject( mutex_lock, INFINITE );
}

/************************************************************************/
/*                          pj_release_lock()                           */
/*                                                                      */
/*      Release the PROJ.4 lock.                                        */
/************************************************************************/

void pj_release_lock()
{
    if( mutex_lock == nullptr )
        pj_init_lock();
    else
        ReleaseMutex( mutex_lock );
}

/************************************************************************/
/*                          pj_cleanup_lock()                           */
/************************************************************************/
void pj_cleanup_lock()
{
    if( mutex_lock != nullptr )
    {
        CloseHandle( mutex_lock );
        mutex_lock = nullptr;
    }
}

#endif /* def MUTEX_win32 */
