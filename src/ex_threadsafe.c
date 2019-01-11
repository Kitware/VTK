/*
 * Copyright(c) 2005-2017 National Technology &Engineering Solutions
 * of Sandia, LLC(NTESS).Under the terms of Contract DE - NA0003525 with
 * NTESS, the U.S.Government retains certain rights in this software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and / or other materials provided
 * with the                                                 distribution.
 *
 * * Neither the name of NTESS nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _GNU_SOURCE
#include "exodusII.h"
#if defined(EXODUS_THREADSAFE)
#include <pthread.h>

#include "exodusII_int.h"

#include <stdio.h>
#include <string.h>

/* NOTE: All code in this file is based on the thread-safe code from the
 * hdf5 library.
 */

/* Global variable definitions */
pthread_once_t EX_first_init_g = PTHREAD_ONCE_INIT;
pthread_key_t  EX_errval_key_g;

EX_mutex_t EX_g;

static void ex_key_destructor(void *key_val)
{
  if (key_val != NULL)
    free(key_val);
}

#define ex_err_abort(status, message)                                                              \
  do {                                                                                             \
    fprintf(stderr, "%s in file %s at line %d: %s\n", message, __FILE__, __LINE__,                 \
            strerror(status));                                                                     \
    abort();                                                                                       \
  } while (0)

void ex_pthread_first_thread_init(void)
{
  int err = pthread_mutexattr_init(&EX_g.attribute);
  if (err != 0) {
    ex_err_abort(err, "Mutex Attr Init");
  }

  err = pthread_mutexattr_settype(&EX_g.attribute, PTHREAD_MUTEX_RECURSIVE);
  if (err != 0) {
    ex_err_abort(err, "Mutex Attr Set Type");
  }

  err = pthread_mutex_init(&EX_g.atomic_lock, &EX_g.attribute);
  if (err != 0) {
    ex_err_abort(err, "Mutex Init");
  }

  /* initialize key for thread-specific error stacks */
  err = pthread_key_create(&EX_errval_key_g, ex_key_destructor);
  if (err != 0) {
    ex_err_abort(err, "Create errval key");
  }
}

int ex_mutex_lock(EX_mutex_t *mutex)
{
  int ret_value = pthread_mutex_lock(&mutex->atomic_lock);
  if (ret_value != 0) {
    ex_err_abort(ret_value, "Lock mutex");
  }
  return ret_value;
}

int ex_mutex_unlock(EX_mutex_t *mutex)
{
  int ret_value = pthread_mutex_unlock(&mutex->atomic_lock);
  if (ret_value != 0) {
    ex_err_abort(ret_value, "Unlock mutex");
  }
  return ret_value;
}

EX_errval_t *exerrval_get(void)
{
  EX_errval_t *ex_errval_local = (EX_errval_t *)pthread_getspecific(EX_errval_key_g);
  if (!ex_errval_local) {
    /*
     * First time thread calls library - create new value and associate
     * with key
     */
    ex_errval_local = (EX_errval_t *)calloc(1, sizeof(EX_errval_t));
    pthread_setspecific(EX_errval_key_g, (void *)ex_errval_local);
  }

  return ex_errval_local;
}
#else
void ex_dummy() {}
#endif
