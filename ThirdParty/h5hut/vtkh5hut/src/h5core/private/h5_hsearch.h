/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5_HSEARCH_H
#define __PRIVATE_H5_HSEARCH_H

#include "h5core/h5_types.h"

typedef struct hsearch_data {
	struct _ENTRY *table;
	size_t size;
	size_t filled;
	int (*compare)(const void*, const void*);
	unsigned int (*compute_hash)(const void*);
	h5_err_t (*free_entry)(const void*);
} h5_hashtable_t;

/* Action which shall be performed in the call to hsearch.  */
typedef enum {
	H5_FIND,
	H5_ENTER,
	H5_REMOVE
} h5_action_t;

typedef struct h5_entry {
	void* dta;
} h5_entry_t;

/* Reentrant versions which can handle multiple hashing tables at the
   same time.  */
extern h5_err_t
h5priv_hsearch (
        void* item,
        const h5_action_t action,
        void** retval,
        h5_hashtable_t* htab
        );

extern h5_err_t
h5priv_hcreate (
        size_t __nel,
        h5_hashtable_t* __htab,
        int (*compare)(const void*, const void*),
        unsigned int (*compute_hash)(const void*),
        h5_err_t (*free_entry)(const void*)
        );

extern h5_err_t
h5priv_hgrow (
        size_t nel,
        h5_hashtable_t* htab
        );

extern h5_err_t
h5priv_hdestroy (
        h5_hashtable_t* __htab
        );

extern h5_err_t
h5priv_hcreate_string_keyed (
        size_t nel,
        h5_hashtable_t* htab,
        h5_err_t (*free_entry)(const void*)
        );

extern void*
h5priv_htraverse (
        struct hsearch_data* htab,
        unsigned int* idx
        );
#endif
