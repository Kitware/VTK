/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:		H5TSprivate.h
 *			May 2 2000
 *			Chee Wai LEE
 *
 * Purpose:		Private non-prototype header.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifndef H5TSprivate_H_
#define H5TSprivate_H_

/* Public headers needed by this file */
#ifdef LATER
#include "H5TSpublic.h"		/*Public API prototypes */
#endif /* LATER */

/* Library level data structures */

typedef struct H5TS_mutex_struct {
    pthread_t owner_thread;		/* current lock owner */
    pthread_mutex_t atomic_lock;	/* lock for atomicity of new mechanism */
    pthread_cond_t cond_var;		/* condition variable */
    unsigned int lock_count;
} H5TS_mutex_t;

/* Extern global variables */
extern pthread_once_t H5TS_first_init_g;
extern pthread_key_t H5TS_errstk_key_g;
extern pthread_key_t H5TS_funcstk_key_g;

#if defined c_plusplus || defined __cplusplus
extern      "C"
{
#endif	/* c_plusplus || __cplusplus */

H5_DLL void H5TS_first_thread_init(void);
H5_DLL herr_t H5TS_mutex_lock(H5TS_mutex_t *mutex);
H5_DLL herr_t H5TS_mutex_unlock(H5TS_mutex_t *mutex);
H5_DLL herr_t H5TS_cancel_count_inc(void);
H5_DLL herr_t H5TS_cancel_count_dec(void);

#if defined c_plusplus || defined __cplusplus
}
#endif	/* c_plusplus || __cplusplus */

#endif	/* H5TSprivate_H_ */
