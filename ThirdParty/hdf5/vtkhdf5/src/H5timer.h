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
 * H5timer.h
 *
 * Internal, platform-independent 'timer' support routines
 *-------------------------------------------------------------------------
 */

#ifndef H5timer_H
#define H5timer_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/* KiB, MiB, GiB, TiB, PiB, EiB - Used in profiling and timing code */
#define H5_KB (1024.0F)
#define H5_MB (1024.0F * 1024.0F)
#define H5_GB (1024.0F * 1024.0F * 1024.0F)
#define H5_TB (1024.0F * 1024.0F * 1024.0F * 1024.0F)
#define H5_PB (1024.0F * 1024.0F * 1024.0F * 1024.0F * 1024.0F)
#define H5_EB (1024.0F * 1024.0F * 1024.0F * 1024.0F * 1024.0F * 1024.0F)

/* A set of elapsed/user/system times emitted as a time point by the
 * platform-independent timers.
 */
typedef struct {
    double user;    /* User time in seconds */
    double system;  /* System time in seconds */
    double elapsed; /* Elapsed (wall clock) time in seconds */
} H5_timevals_t;

/* Timer structure for platform-independent timers */
typedef struct {
    H5_timevals_t initial;        /* Current interval start time */
    H5_timevals_t final_interval; /* Last interval elapsed time */
    H5_timevals_t total;          /* Total elapsed time for all intervals */
    bool          is_running;     /* Whether timer is running */
} H5_timer_t;

/* Returns library bandwidth as a pretty string */
H5_DLL void H5_bandwidth(char *buf /*out*/, size_t bufsize, double nbytes, double nseconds);

/* Timer functionality */
H5_DLL time_t   H5_now(void);
H5_DLL uint64_t H5_now_usec(void);
H5_DLL herr_t   H5_timer_init(H5_timer_t *timer /*in,out*/);
H5_DLL herr_t   H5_timer_start(H5_timer_t *timer /*in,out*/);
H5_DLL herr_t   H5_timer_stop(H5_timer_t *timer /*in,out*/);
H5_DLL herr_t   H5_timer_get_times(H5_timer_t timer, H5_timevals_t *times /*in,out*/);
H5_DLL herr_t   H5_timer_get_total_times(H5_timer_t timer, H5_timevals_t *times /*in,out*/);
H5_DLL char    *H5_timer_get_time_string(double seconds);

#endif /* H5timer_H */
