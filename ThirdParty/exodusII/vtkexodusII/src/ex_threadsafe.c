/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#define _GNU_SOURCE
#include "exodusII.h"
#if defined(EXODUS_THREADSAFE)
#include <pthread.h>

#include "exodusII_int.h"

/* NOTE: All code in this file was originally based on the thread-safe
 * code from the hdf5 library.
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

void ex__pthread_first_thread_init(void)
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

int ex__mutex_lock(EX_mutex_t *mutex)
{
  int ret_value = pthread_mutex_lock(&mutex->atomic_lock);
  if (ret_value != 0) {
    ex_err_abort(ret_value, "Lock mutex");
  }
  return ret_value;
}

int ex__mutex_unlock(EX_mutex_t *mutex, const char *func, int line)
{
  int ret_value = pthread_mutex_unlock(&mutex->atomic_lock);
  if (ret_value != 0) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Invalid unlock mutex call from line %d of '%s'", line,
             func);
    ex_err_abort(ret_value, errmsg);
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
