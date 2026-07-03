/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the LICENSE file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:     H5TSprivate.h
 *
 * Purpose:     Thread-safety abstractions used by the library
 *
 *-------------------------------------------------------------------------
 */
#ifndef H5TSprivate_H_
#define H5TSprivate_H_

#ifdef H5_HAVE_THREADS

#ifdef H5_HAVE_THREADSAFE_API
/* Include package's public headers */
#include "H5TSdevelop.h"
#endif /* H5_HAVE_THREADSAFE_API */

/**************************/
/* Library Private Macros */
/**************************/

/* Thread-safety sanity-checking annotations */
#define H5TS_CAPABILITY(x)           H5_ATTR_THREAD_ANNOT(capability(x))
#define H5TS_ACQUIRE(...)            H5_ATTR_THREAD_ANNOT(acquire_capability(__VA_ARGS__))
#define H5TS_ACQUIRE_SHARED(...)     H5_ATTR_THREAD_ANNOT(acquire_shared_capability(__VA_ARGS__))
#define H5TS_RELEASE(...)            H5_ATTR_THREAD_ANNOT(release_capability(__VA_ARGS__))
#define H5TS_RELEASE_SHARED(...)     H5_ATTR_THREAD_ANNOT(release_shared_capability(__VA_ARGS__))
#define H5TS_TRY_ACQUIRE(...)        H5_ATTR_THREAD_ANNOT(try_acquire_capability(__VA_ARGS__))
#define H5TS_TRY_ACQUIRE_SHARED(...) H5_ATTR_THREAD_ANNOT(try_acquire_shared_capability(__VA_ARGS__))

#ifdef H5_HAVE_C11_THREADS
/* Static initialization values */
#define H5TS_ONCE_INITIALIZER ONCE_FLAG_INIT

/* Thread macros */
#define H5TS_thread_self()        thrd_current()
#define H5TS_thread_equal(t1, t2) thrd_equal((t1), (t2))
#define H5TS_THREAD_RETURN_TYPE   H5TS_thread_ret_t

/* Mutex macros */
#define H5TS_MUTEX_TYPE_PLAIN     mtx_plain
#define H5TS_MUTEX_TYPE_RECURSIVE (mtx_plain | mtx_recursive)
#else
#ifdef H5_HAVE_WIN_THREADS
/* Static initialization values */
#define H5TS_ONCE_INITIALIZER     INIT_ONCE_STATIC_INIT

/* Thread macros */
#define H5TS_thread_self()        GetCurrentThread()
#define H5TS_thread_equal(t1, t2) (GetThreadId(t1) == GetThreadId(t2))
#define H5TS_THREAD_RETURN_TYPE   H5TS_thread_ret_t WINAPI

/* Mutex macros */
#define H5TS_MUTEX_TYPE_PLAIN     0
#define H5TS_MUTEX_TYPE_RECURSIVE 1
#else
/* Static initialization values */
#define H5TS_ONCE_INITIALIZER      PTHREAD_ONCE_INIT

/* Thread macros */
#define H5TS_thread_self()         pthread_self()
#define H5TS_thread_equal(t1, t2)  pthread_equal((t1), (t2))
#define H5TS_THREAD_RETURN_TYPE    H5TS_thread_ret_t
#define H5TS_THREAD_CANCEL_DISABLE PTHREAD_CANCEL_DISABLE

/* Mutex macros */
#define H5TS_MUTEX_TYPE_PLAIN      0
#define H5TS_MUTEX_TYPE_RECURSIVE  1
#endif
#endif

/* Atomics macros */
#if defined(H5_HAVE_STDATOMIC_H) && !defined(__cplusplus)
/* atomic_int */
#define H5TS_atomic_init_int(obj, desired)  atomic_init((obj), (desired))
#define H5TS_atomic_load_int(obj)           atomic_load(obj)
#define H5TS_atomic_store_int(obj, desired) atomic_store((obj), (desired))
#define H5TS_atomic_fetch_add_int(obj, arg) atomic_fetch_add((obj), (arg))
#define H5TS_atomic_fetch_sub_int(obj, arg) atomic_fetch_sub((obj), (arg))
#define H5TS_atomic_destroy_int(obj)        /* void */

/* atomic_uint */
#define H5TS_atomic_init_uint(obj, desired)  atomic_init((obj), (desired))
#define H5TS_atomic_load_uint(obj)           atomic_load(obj)
#define H5TS_atomic_store_uint(obj, desired) atomic_store((obj), (desired))
#define H5TS_atomic_fetch_add_uint(obj, arg) atomic_fetch_add((obj), (arg))
#define H5TS_atomic_fetch_sub_uint(obj, arg) atomic_fetch_sub((obj), (arg))
#define H5TS_atomic_destroy_uint(obj)        /* void */

/* atomic_voidp */
#define H5TS_atomic_init_voidp(obj, desired)     atomic_init((obj), (desired))
#define H5TS_atomic_exchange_voidp(obj, desired) atomic_exchange((obj), (desired))
#define H5TS_atomic_compare_exchange_strong_voidp(obj, expected, desired)                                    \
    atomic_compare_exchange_strong((obj), (expected), (desired))
#define H5TS_atomic_destroy_voidp(obj) /* void */
#endif                                 /* H5_HAVE_STDATOMIC_H */

#if defined(H5_HAVE_STDATOMIC_H)
/* Spinlock operations, built from C11 atomics.  Generally follows the example
 * here: http://en.cppreference.com/w/cpp/atomic/atomic_flag with some memory
 * order improvements.
 *
 * Note: Pass a pointer to a H5TS_spinlock_t to all the spinlock macros.
 *
 */

/* Initialize the lock */
#define H5TS_SPINLOCK_INIT(lock)                                                                             \
    do {                                                                                                     \
        *(lock) = ATOMIC_FLAG_INIT;                                                                          \
    } while (0)

/* Acquire the lock */
#define H5TS_SPINLOCK_LOCK(lock)                                                                             \
    do {                                                                                                     \
        while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire))                                \
            ;                                                                                                \
    } while (0)

/* Release the lock */
#define H5TS_SPINLOCK_UNLOCK(lock)                                                                           \
    do {                                                                                                     \
        atomic_flag_clear_explicit(lock, memory_order_release);                                              \
    } while (0)

#endif

/****************************/
/* Library Private Typedefs */
/****************************/

/* Key destructor callback */
typedef void (*H5TS_key_destructor_func_t)(void *);

/* Thread pool */
typedef struct H5TS_pool_t H5TS_pool_t;

/* Portability aliases */
#ifdef H5_HAVE_C11_THREADS

/* Non-recursive readers/writer lock */
typedef struct H5TS_rwlock_t {
    mtx_t    mutex;
    cnd_t    read_cv, write_cv;
    unsigned readers, writers, read_waiters, write_waiters;
} H5TS_rwlock_t;

typedef thrd_t H5TS_thread_t;
typedef int (*H5TS_thread_start_func_t)(void *);
typedef int       H5TS_thread_ret_t;
typedef tss_t     H5TS_key_t;
typedef mtx_t     H5TS_CAPABILITY("mutex") H5TS_mutex_t;
typedef cnd_t     H5TS_cond_t;
typedef once_flag H5TS_once_t;
typedef void (*H5TS_once_init_func_t)(void);
#else
#ifdef H5_HAVE_WIN_THREADS
typedef HANDLE                 H5TS_thread_t;
typedef LPTHREAD_START_ROUTINE H5TS_thread_start_func_t;
typedef DWORD                  H5TS_thread_ret_t;
typedef DWORD                  H5TS_key_t;
typedef CRITICAL_SECTION       H5TS_CAPABILITY("mutex") H5TS_mutex_t;
typedef SRWLOCK                H5TS_rwlock_t;
typedef CONDITION_VARIABLE     H5TS_cond_t;
typedef INIT_ONCE              H5TS_once_t;
typedef PINIT_ONCE_FN          H5TS_once_init_func_t;
#else

/* Non-recursive readers/writer lock */
#if defined(__MACH__)
/*
 * Emulated pthread rwlock for MacOS
 *
 * Can't use pthread rwlock on MacOS due to: "The results [of calling
 *      pthread_rwlock_wrlock] are undefined if the calling thread already
 *      holds the lock at the time the call is made."
 *  but the pthread standard says: "If a deadlock condition occurs or the
 *      calling thread already owns the read-write lock for writing or reading,
 *      the call shall either deadlock or return [EDEADLK]."
 *
 * The net result of this is that the current version of MacOS (v15.x) allows
 * the same thread to recursively acquire a write lock, violating the pthread
 * guarantee of deadlocking or failing.
 *
 */
typedef struct H5TS_rwlock_t {
    pthread_mutex_t mutex;
    pthread_cond_t  read_cv, write_cv;
    unsigned        readers, writers, read_waiters, write_waiters;
} H5TS_rwlock_t;
#else
typedef pthread_rwlock_t H5TS_rwlock_t;
#endif

typedef pthread_t H5TS_thread_t;
typedef void *(*H5TS_thread_start_func_t)(void *);
typedef void           *H5TS_thread_ret_t;
typedef pthread_key_t   H5TS_key_t;
typedef pthread_mutex_t H5TS_CAPABILITY("mutex") H5TS_mutex_t;
typedef pthread_cond_t  H5TS_cond_t;
typedef pthread_once_t  H5TS_once_t;
typedef void (*H5TS_once_init_func_t)(void);
#endif
#endif

/* Atomics */
#if defined(H5_HAVE_STDATOMIC_H) && !defined(__cplusplus)
typedef atomic_int  H5TS_atomic_int_t;
typedef atomic_uint H5TS_atomic_uint_t;
/* Suppress warning about _Atomic being a C11 extension */
H5_WARN_C11_EXTENSIONS_OFF
typedef void *_Atomic H5TS_atomic_voidp_t;
H5_WARN_C11_EXTENSIONS_ON
#else
typedef struct {
    H5TS_mutex_t mutex;
    int          value;
} H5TS_atomic_int_t;
typedef struct {
    H5TS_mutex_t mutex;
    unsigned     value;
} H5TS_atomic_uint_t;
typedef struct {
    H5TS_mutex_t mutex;
    void        *value;
} H5TS_atomic_voidp_t;
#endif

/* Thread Barrier */
#ifdef H5_HAVE_PTHREAD_BARRIER
typedef pthread_barrier_t H5TS_barrier_t;
#else
typedef struct H5TS_barrier_t {
    unsigned           count;
    H5TS_atomic_uint_t openings;
    H5TS_atomic_uint_t generation;
} H5TS_barrier_t;
#endif

/* Semaphores */
#if defined(_WIN32)
/*
 * Windows semaphores
 */

/* System semaphore */
typedef HANDLE H5TS_semaphore_t;

#elif defined(__unix__) && !defined(__MACH__)
/*
 * POSIX semaphores
 */
#include <semaphore.h>

/* System semaphore */
typedef sem_t H5TS_semaphore_t;

#else
/*
 * Emulated semaphores, for MacOS and unknown platforms
 * Can't use POSIX semaphores on MacOS due to:
 *      http://lists.apple.com/archives/darwin-kernel/2009/Apr/msg00010.html
 */

/* Emulate semaphore w/mutex & condition variable */
typedef struct H5TS_semaphore_t {
    H5TS_mutex_t mutex;
    H5TS_cond_t  cond;
    unsigned     waiters;
    int          counter;
} H5TS_semaphore_t;
#endif

#if defined(H5_HAVE_STDATOMIC_H) && !defined(__cplusplus)
/* Spinlock, built from C11 atomic_flag */
typedef atomic_flag H5TS_spinlock_t;
#endif

/*****************************/
/* Library-private Variables */
/*****************************/

/***************************************/
/* Library-private Function Prototypes */
/***************************************/

#ifdef H5_HAVE_THREADSAFE_API
/* Library/thread init/term operations */
H5_DLL void H5TS_term_package(void);
H5_DLL int  H5TS_top_term_package(void);

/* Prepare for / restore after user callback */
#ifdef H5_HAVE_CONCURRENCY
H5_DLL herr_t H5TS_user_cb_prepare(void);
H5_DLL herr_t H5TS_user_cb_restore(void);
#endif /* H5_HAVE_CONCURRENCY */

/* API locking */
#ifdef H5_HAVE_THREADSAFE
H5_DLL herr_t H5TS_api_lock(void);
#else /* H5_HAVE_CONCURRENCY */
H5_DLL herr_t H5TS_api_lock(unsigned *dlftt);
#endif
H5_DLL herr_t H5TS_api_unlock(void);

/* Retrieve per-thread info */
H5_DLL herr_t               H5TS_thread_id(uint64_t *id);
H5_DLL struct H5CX_node_t **H5TS_get_api_ctx_ptr(void);
H5_DLL struct H5E_stack_t  *H5TS_get_err_stack(void);
#endif /* H5_HAVE_THREADSAFE_API */

/* 'Once' operationss */
H5_DLL herr_t H5TS_once(H5TS_once_t *once, H5TS_once_init_func_t func);

/* Mutex operations */
H5_DLL herr_t H5TS_mutex_init(H5TS_mutex_t *mutex, int type);
/* Mutex lock & unlock calls are defined in H5TSmutex.h */
H5_DLL herr_t H5TS_mutex_trylock(H5TS_mutex_t *mutex, bool *acquired) H5TS_TRY_ACQUIRE(SUCCEED, *mutex);
H5_DLL herr_t H5TS_mutex_destroy(H5TS_mutex_t *mutex);

/* R/W locks */
H5_DLL herr_t H5TS_rwlock_init(H5TS_rwlock_t *lock);
/* R/W lock & unlock calls are defined in H5TSrwlock.h */
#if !defined(__cplusplus)
static inline herr_t H5TS_rwlock_rdlock(H5TS_rwlock_t *lock);
static inline herr_t H5TS_rwlock_rdunlock(H5TS_rwlock_t *lock);
static inline herr_t H5TS_rwlock_wrlock(H5TS_rwlock_t *lock);
static inline herr_t H5TS_rwlock_trywrlock(H5TS_rwlock_t *lock, bool *acquired)
    H5TS_TRY_ACQUIRE(SUCCEED, *lock);
static inline herr_t H5TS_rwlock_wrunlock(H5TS_rwlock_t *lock);
#endif
H5_DLL herr_t H5TS_rwlock_destroy(H5TS_rwlock_t *lock);

/* Condition variable operations */
H5_DLL herr_t H5TS_cond_init(H5TS_cond_t *cond);
/* Condition variable wait, signal, broadcast calls are defined in H5TScond.h */
H5_DLL herr_t H5TS_cond_destroy(H5TS_cond_t *cond);

/* Thread-specific keys */
H5_DLL herr_t H5TS_key_create(H5TS_key_t *key, H5TS_key_destructor_func_t dtor);
/* Key set & get calls are defined in H5TSkey.h */
H5_DLL herr_t H5TS_key_delete(H5TS_key_t key);

/* Threads */
H5_DLL herr_t H5TS_thread_create(H5TS_thread_t *thread, H5TS_thread_start_func_t func, void *udata);
H5_DLL herr_t H5TS_thread_join(H5TS_thread_t thread, H5TS_thread_ret_t *ret_val);
H5_DLL herr_t H5TS_thread_detach(H5TS_thread_t thread);
H5_DLL void   H5TS_thread_yield(void);

/* Thread pools */
H5_DLL herr_t H5TS_pool_create(H5TS_pool_t **pool, unsigned num_threads);
/* Thread pool add task call is defined in H5TSpool.h */
#if !defined(__cplusplus)
static inline herr_t H5TS_pool_add_task(H5TS_pool_t *pool, H5TS_thread_start_func_t func, void *ctx);
#endif
H5_DLL herr_t H5TS_pool_destroy(H5TS_pool_t *pool);

/* Emulated C11 atomics */
#if !defined(H5_HAVE_STDATOMIC_H) && !defined(__cplusplus)
/* atomic_int */
H5_DLL void H5TS_atomic_init_int(H5TS_atomic_int_t *obj, int desired);
/* Atomic 'int' load, store, etc. calls are defined in H5TSatomic.h */
static inline int  H5TS_atomic_load_int(H5TS_atomic_int_t *obj);
static inline void H5TS_atomic_store_int(H5TS_atomic_int_t *obj, int desired);
static inline int  H5TS_atomic_fetch_add_int(H5TS_atomic_int_t *obj, int arg);
static inline int  H5TS_atomic_fetch_sub_int(H5TS_atomic_int_t *obj, int arg);
H5_DLL void        H5TS_atomic_destroy_int(H5TS_atomic_int_t *obj);

/* atomic_uint */
H5_DLL void H5TS_atomic_init_uint(H5TS_atomic_uint_t *obj, unsigned desired);
/* Atomic 'unsigned' load, store, etc. calls are defined in H5TSatomic.h */
static inline unsigned H5TS_atomic_load_uint(H5TS_atomic_uint_t *obj);
static inline void     H5TS_atomic_store_uint(H5TS_atomic_uint_t *obj, unsigned desired);
static inline unsigned H5TS_atomic_fetch_add_uint(H5TS_atomic_uint_t *obj, unsigned arg);
static inline unsigned H5TS_atomic_fetch_sub_uint(H5TS_atomic_uint_t *obj, unsigned arg);
H5_DLL void            H5TS_atomic_destroy_uint(H5TS_atomic_uint_t *obj);

/* void * _Atomic (atomic void pointer) */
H5_DLL void H5TS_atomic_init_voidp(H5TS_atomic_voidp_t *obj, void *desired);
/* Atomic 'void *' load, store, etc. calls are defined in H5TSatomic.h */
static inline void *H5TS_atomic_exchange_voidp(H5TS_atomic_voidp_t *obj, void *desired);
static inline bool  H5TS_atomic_compare_exchange_strong_voidp(H5TS_atomic_voidp_t *obj, void **expected,
                                                              void *desired);
H5_DLL void         H5TS_atomic_destroy_voidp(H5TS_atomic_voidp_t *obj);
#endif /* H5_HAVE_STDATOMIC_H */

/* Barrier related function declarations */
H5_DLL herr_t H5TS_barrier_init(H5TS_barrier_t *barrier, unsigned count);
/* Barrier wait call is defined in H5TSbarrier.h */
#if !defined(__cplusplus)
static inline herr_t H5TS_barrier_wait(H5TS_barrier_t *barrier);
#endif /* H5_HAVE_PTHREAD_BARRIER */
H5_DLL herr_t H5TS_barrier_destroy(H5TS_barrier_t *barrier);

H5_DLL herr_t H5TS_semaphore_init(H5TS_semaphore_t *sem, unsigned initial_count);
/* Semaphore signal & wait calls are defined in H5TSsemaphore.h */
#if !defined(__cplusplus)
static inline herr_t H5TS_semaphore_signal(H5TS_semaphore_t *sem);
static inline herr_t H5TS_semaphore_wait(H5TS_semaphore_t *sem);
#endif
H5_DLL herr_t H5TS_semaphore_destroy(H5TS_semaphore_t *sem);

/* Headers with inlined routines */
#ifndef __cplusplus
#include "H5TScond.h"
#include "H5TSmutex.h"
#include "H5TSkey.h"
#ifndef H5_HAVE_STDATOMIC_H
#include "H5TSatomic.h"
#endif /* H5_HAVE_STDATOMIC_H */
#include "H5TSbarrier.h"
#include "H5TSrwlock.h"
#include "H5TSsemaphore.h"
#include "H5TSpool.h"
#endif /* __cplusplus */

#endif /* H5_HAVE_THREADS */

#endif /* H5TSprivate_H_ */
