/* Copyright (C) 1993,1995-1997,2002,2005,2007,2008
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1993.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>

#include "private/h5_log.h"
#include "private/h5_hsearch.h"

#include "h5core/h5_types.h"
#include "h5core/h5_syscall.h"


/* [Aho,Sethi,Ullman] Compilers: Principles, Techniques and Tools, 1986
   [Knuth]            The Art of Computer Programming, part 3 (6.4)  */


/* The reentrant version has no static variables to maintain the state.
   Instead the interface of all functions is extended to take an argument
   which describes the current status.  */
typedef struct _ENTRY {
	unsigned int used;
	void* entry;
} _ENTRY;


/* For the used double hash method the table size has to be a prime. To
   correct the user given table size we need a prime test.  This trivial
   algorithm is adequate because
   a)  the code is (most probably) called a few times per program run and
   b)  the number is small because the table must fit in the core  */
static int
isprime (const unsigned int number) {
	/* no even number will be passed */
	unsigned int div = 3;

	while (div * div < number && number % div != 0)
		div += 2;

	return number % div != 0;
}

/* Before using the hash table we must allocate memory for it.
   Test for an existing table are done. We allocate one element
   more as the found prime number says. This is done for more effective
   indexing as explained in the comment for the hsearch function.
   The contents of the table is zeroed, especially the field used
   becomes zero.  */
h5_err_t
h5priv_hcreate (
        size_t nel,
        h5_hashtable_t* htab,
        int (*compare)(const void*, const void*),
        unsigned int (*compute_hash)(const void*),
        h5_err_t (*free_entry)(const void*)
        ) {
	H5_PRIV_API_ENTER (h5_err_t,
	                   "nel=%llu, htab=%p, compare=%p, "
	                   "compute_hash=%p, free_entry=%p",
	                   (long long unsigned)nel,
	                   htab, compare, compute_hash, free_entry);

	/* Test for correct arguments.  */
	if (htab == NULL) {
		H5_LEAVE (h5_error_internal ());
	}
	/* Change nel to the first prime number not smaller as nel. */
	nel |= 1;      /* make odd */
	while (!isprime (nel))
		nel += 2;

	htab->size = nel;
	htab->filled = 0;
	htab->compare = compare;
	htab->compute_hash = compute_hash;
	htab->free_entry = free_entry;

	/* allocate memory and zero out */
	TRY (htab->table = (_ENTRY *) h5_calloc (
	             htab->size + 1, sizeof (_ENTRY)));

	/* everything went alright */
	H5_RETURN (H5_SUCCESS);
}

/*
   Resize hash table. Actually we create a new hash table with the specified
   size and copy the old data.
 */
h5_err_t
h5priv_hgrow (
        size_t nel,             // number of entries to grow
        h5_hashtable_t* htab    // hash table to resize
        ) {
	H5_PRIV_API_ENTER (h5_err_t,
	                   "nel=%llu, htab=%p",
	                   (long long unsigned)nel, htab);
	if (htab == NULL) {
		h5_error_internal ();
	}
	nel += htab->size; // new size

	// create new hash table
	h5_hashtable_t __htab;
	memset (&__htab, 0, sizeof (__htab));
	h5_debug ("Resize hash table from %zu to %zu elements.", htab->size, nel);
	TRY (h5priv_hcreate (nel, &__htab, htab->compare,
	                     htab->compute_hash, htab->free_entry));
	if (htab->table != NULL) {
		h5_debug ("New hash table created, filling ...");

		// add all entries to new hash table
		unsigned int idx;
		for (idx = 1; idx <= htab->size; idx++) {
			if (htab->table[idx].used) {
				void* ventry;
				TRY (h5priv_hsearch (
				             htab->table[idx].entry,
				             H5_ENTER,
				             &ventry,
				             &__htab));
				if (idx % 1000000 == 0) {
					h5_debug (".");
				}
			}
		}
		/* Free used memory.  */
		TRY (h5_free (htab->table));
		h5_debug ("Old hash table removed");
	}
	*htab = __htab;
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
hwalk (
        struct hsearch_data* htab,
        h5_err_t (*visit)(const void *item)
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "htab=%p, visit=%p", htab, visit);
	unsigned int idx = 1;
	for (idx = 1; idx < htab->size; idx++) {
		if (htab->table[idx].used) {
			TRY ((*visit)(&htab->table[idx].entry));
		}
	}
	H5_RETURN (H5_SUCCESS);
}

void*
h5priv_htraverse (
        struct hsearch_data* htab,
        unsigned int* idx
        ) {
	assert (*idx > 0);
	for (; *idx < htab->size; (*idx)++) {
		if (htab->table[*idx].used) {
			void* result = htab->table[*idx].entry;
			(*idx)++;
			return result;
		}
	}
	return NULL;
}


/* After using the hash table it has to be destroyed. The used memory can
   be freed and the local static variable can be marked as not used.  */
h5_err_t
h5priv_hdestroy (
        struct hsearch_data* htab
        ) {
	H5_PRIV_API_ENTER (h5_err_t, "htab=%p", htab);
	/* Test for correct arguments.  */
	if (htab == NULL) {
		H5_LEAVE (h5_error_internal ());
	}
	/* Free used memory.  */
	if (htab->free_entry) {
		TRY (hwalk (htab, htab->free_entry));
	}
	TRY (h5_free (htab->table));

	/* the sign for an existing table is an value != NULL in htable */
	htab->table = NULL;
	H5_RETURN (H5_SUCCESS);
}



/* This is the search function. It uses double hashing with open addressing.
   The argument item.key has to be a pointer to an zero terminated, most
   probably strings of chars. The function for generating a number of the
   strings is simple but fast. It can be replaced by a more complex function
   like ajw (see [Aho,Sethi,Ullman]) if the needs are shown.

   We use an trick to speed up the lookup. The table is created by hcreate
   with one more element available. This enables us to use the index zero
   special. This index will never be used because we store the first hash
   index in the field used where zero means not used. Every other value
   means used. The used field can be used as a first fast comparison for
   equality of the stored and the parameter value. This helps to prevent
   unnecessary expensive calls of strcmp.  */
h5_err_t
h5priv_hsearch (
        void* item,
        const h5_action_t action,
        void** retval,
        struct hsearch_data* htab
        ) {
	H5_PRIV_API_ENTER (h5_err_t,
	                   "item=%p, action=%d, retval=%p, htab=%p",
	                   item, (int)action, retval, htab);
	unsigned int hval;
	unsigned int idx;

	/* Compute an value for the given key. Perhaps use a better method. */
	hval = htab->compute_hash(item);

	/* First hash function: simply take the modul but prevent zero. */
	idx = hval % htab->size + 1;

	if (htab->table[idx].used) {
		/* Further action might be required according to the action
		   value. */
		if (htab->table[idx].used == hval
		    && (htab->compare (item, htab->table[idx].entry) == 0) ) {
			if (retval) {
				*retval = htab->table[idx].entry;
			}
			H5_LEAVE (H5_SUCCESS);
		}

		/* Second hash function, as suggested in [Knuth] */
		unsigned int hval2 = 1 + hval % (htab->size - 2);
		unsigned int first_idx = idx;

		do {
			/* Because SIZE is prime this guarantees to step
			   through all available indices.  */
			if (idx <= hval2)
				idx = htab->size + idx - hval2;
			else
				idx -= hval2;

			/* If we visited all entries leave the loop
			   unsuccessfully.  */
			if (idx == first_idx)
				break;

			/* If entry is found use it. */
			if (htab->table[idx].used == hval
			    && (htab->compare (
			                item, htab->table[idx].entry) == 0) ) {
				if (retval) {
					*retval = htab->table[idx].entry;
				}
				H5_LEAVE (H5_SUCCESS);
			}
		} while (htab->table[idx].used);
	}

	/* An empty bucket has been found. */
	if (action == H5_ENTER) {
		/* If table is full and another entry should be entered return
		   with error.  */
		if (htab->filled == htab->size) {
			if (retval) {
				*retval = NULL;
			}
			H5_LEAVE (h5_error_internal ());
		}

		htab->table[idx].used  = hval;
		htab->table[idx].entry = item;

		++htab->filled;

		if (retval) {
			*retval = htab->table[idx].entry;
		}
		H5_LEAVE (H5_SUCCESS);
	} else if (action == H5_REMOVE) {
		htab->table[idx].used = 0;              /* mark as unused, but */
		*retval = htab->table[idx].entry;       /* return ptr to entry */
		H5_LEAVE (H5_SUCCESS);
	}
	if (retval) *retval = NULL;
	h5_debug ("Key not found in hash table.");
	H5_RETURN (H5_NOK);
}

typedef struct {
	char* key;
} h5_hitem_string_keyed_t;

static int
cmp_string_keyed (
        const void* __a,
        const void* __b
        ) {
	h5_hitem_string_keyed_t *a = (h5_hitem_string_keyed_t*) __a;
	h5_hitem_string_keyed_t *b = (h5_hitem_string_keyed_t*) __b;
	return strcmp (a->key, b->key);
}

static unsigned int
compute_string_keyed (
        const void* __entry
        ) {
	h5_hitem_string_keyed_t* entry = (h5_hitem_string_keyed_t*) __entry;
	unsigned int len = strlen (entry->key);
	unsigned int hval = len;
	unsigned int count = len;
	while (count-- > 0)  {
		hval <<= 4;
		hval += entry->key[count];
	}
	return hval;
}

static h5_err_t
free_string_keyed (
        const void* __entry
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "__entry=%p", __entry);
	h5_hitem_string_keyed_t* entry = (h5_hitem_string_keyed_t*) __entry;
	TRY (h5_free (entry->key));
	TRY (h5_free (entry));
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5priv_hcreate_string_keyed (
        size_t nel,
        h5_hashtable_t* htab,
        h5_err_t (*free_entry)(const void*)
        ) {
	H5_PRIV_API_ENTER (h5_err_t, "htab=%p, free_entry=%p", htab, free_entry);
	if (free_entry == NULL) {
		TRY (h5priv_hcreate (nel, htab,
		                     cmp_string_keyed,
		                     compute_string_keyed,
		                     free_string_keyed));
	} else {
		TRY (h5priv_hcreate (nel, htab,
		                     cmp_string_keyed,
		                     compute_string_keyed,
		                     free_entry));
	}
	H5_RETURN (H5_SUCCESS);
}
