/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5_MAPS_H
#define __PRIVATE_H5_MAPS_H

#include "private/h5_types.h"
#include "h5core/h5_log.h"
#include "h5core/h5_err.h"
#include "h5core/h5_syscall.h"
#include "private/h5_log.h"

#include <string.h>

// Allocate new list
#define h5priv_alloc_xlist( type )                                      \
        static inline h5_err_t                                          \
        h5priv_alloc_ ## type ## list (                                 \
                h5_ ## type ## list_t**list,                            \
                const int32_t size                                      \
                ) {                                                     \
		H5_PRIV_API_ENTER (h5_err_t, "list=%p, size=%d", list, size); \
		TRY (*list = h5_calloc (                                \
		             1, sizeof (**list)+size*sizeof ((*list)->items[0]))); \
                (*list)->size = size;                                   \
                H5_RETURN (H5_SUCCESS);                        \
	}

// Free list
#define h5priv_free_xlist( type )                                       \
        static inline h5_err_t                                          \
        h5priv_free_ ## type ## list (                                  \
                h5_ ## type ## list_t**list                             \
                ) {                                                     \
		H5_PRIV_API_ENTER (h5_err_t, "list=%p", list);          \
		if (*list == NULL) H5_LEAVE (H5_SUCCESS);      \
		TRY (h5_free (*list));                                  \
		*list = NULL;                                           \
		H5_RETURN (H5_SUCCESS);                        \
	}

// Insert item
#define h5priv_insert_into_xlist( type )                                \
        static inline h5_loc_idx_t                                      \
        h5priv_insert_into_ ## type ## list (                           \
                h5_ ## type ## list_t**list,                            \
                h5_ ## type ## _t id,                                   \
                h5_loc_idx_t idx                                        \
                ) {                                                     \
		H5_PRIV_API_ENTER (h5_err_t,                            \
		                   "list=%p, id=%llu, idx=%llu",        \
		                   list,                                \
		                   (long long unsigned)id,              \
		                   (long long unsigned)idx);            \
		if (*list == NULL) {                                    \
			TRY (h5priv_alloc_ ## type ## list (list, 2));  \
		} else if ((*list)->num_items == (*list)->size) {       \
			h5_size_t size = (*list)->size;                 \
			if (size == 0) {                                \
				size = 2;                               \
			} else {                                        \
				size *= 2;                              \
			}                                               \
			size_t num_bytes = sizeof (**list) + (size-1)*sizeof((*list)->items[0]); \
			TRY (*list = h5_alloc (*list, num_bytes));      \
                        (*list)->size = size;                           \
                }                                                       \
                h5_ ## type ## list_t* l = *list;                       \
                if (idx == -1) {                                        \
                        idx = l->num_items;                             \
                } else {                                                \
                        memmove (                                       \
                                &l->items[idx+1],                       \
                                &l->items[idx],                         \
                                (l->num_items - idx) * sizeof (l->items[0])); \
               }                                                        \
               l->items[idx] = id;                                      \
               l->num_items++;                                          \
               H5_RETURN (idx);                                \
        }

/*
  Find ID in sorted list.

  Note:
  if the number of items is zero, the while condition is false. So
  we don't have to handle this case special.
*/
#define h5priv_find_in_xlist(type)                                      \
        static inline h5_loc_idx_t                                      \
        h5priv_find_in_ ## type ## list (                               \
                h5_ ## type ## list_t* list,                            \
                const h5_ ## type ## _t item                            \
                ) {                                                     \
                H5_PRIV_API_ENTER (h5_err_t,                            \
                                   "list=%p, item=%llu",                \
                                   list, (long long unsigned)item);     \
                if (!list) {						\
                        H5_LEAVE (-1);                         \
                }                                                       \
                register ssize_t low = 0;                               \
                register ssize_t mid;                                   \
                register ssize_t high = list->num_items - 1;            \
                while (low <= high) {                                   \
                        mid = (low + high) / 2;                         \
                        if (list->items[mid] > item)                    \
                                high = mid - 1;                         \
                        else if (list->items[mid] < item)               \
                                low = mid + 1;                          \
                        else                                            \
                                H5_LEAVE (mid);                \
                }                                                       \
                H5_RETURN (-(low+1));                          \
	}


// Search in sorted list. If item is not in list, add it.
#define h5priv_search_in_xlist( type )                                  \
        static inline h5_loc_idx_t                                      \
        h5priv_search_in_ ## type ## list (                             \
                h5_ ## type ## list_t**list,                            \
                h5_ ## type ## _t item                                  \
                ) {                                                     \
		H5_PRIV_API_ENTER (                                     \
                        h5_err_t,                                       \
                        "list=%p, item=%llu",                           \
                        list, (long long unsigned)item);                \
		h5_loc_idx_t idx = h5priv_find_in_ ## type ## list (*list, item); \
		if (idx < 0) {						\
			idx = -(idx+1);					\
			TRY (idx = h5priv_insert_into_ ## type ## list (list, item, idx)); \
		}							\
		H5_RETURN (idx);				\
        }


h5priv_alloc_xlist (loc_idx)
h5priv_free_xlist (loc_idx)
h5priv_insert_into_xlist (loc_idx)
h5priv_find_in_xlist (loc_idx)
h5priv_search_in_xlist (loc_idx)

h5priv_alloc_xlist (glb_idx)
h5priv_free_xlist (glb_idx)
h5priv_insert_into_xlist (glb_idx)
h5priv_find_in_xlist (glb_idx)
h5priv_search_in_xlist (glb_idx)


h5_err_t
h5priv_alloc_strlist (h5_strlist_t**, const h5_size_t);

h5_err_t
h5priv_free_strlist (h5_strlist_t**);

ssize_t
h5priv_insert_strlist (h5_strlist_t**, const char *const, size_t);

ssize_t
h5priv_find_strlist (h5_strlist_t*, const char* const item);

ssize_t
h5priv_search_strlist (h5_strlist_t**, const char* const);

h5_err_t
h5priv_remove_strlist (h5_strlist_t*, const char* const);

h5_err_t
h5priv_new_idxmap (
        h5_idxmap_t *map,
        const h5_size_t size
        );

static inline h5_err_t
h5priv_grow_idxmap (
        h5_idxmap_t* map,
        const size_t size // grow to this size
        ) {
	H5_PRIV_API_ENTER (h5_err_t,
	                   "map=%p, size=%llu",
	                   map, (long long unsigned)size);
	if (map->num_items >= size)
		H5_LEAVE (H5_SUCCESS);
	int new = (map->items == NULL);
	size_t size_in_bytes = size * sizeof (map->items[0]);
	TRY (map->items = h5_alloc (map->items, size_in_bytes));
	map->size = size;
	if (new) map->num_items = 0;
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5priv_insert_idxmap (
        h5_idxmap_t *map,
        h5_glb_idx_t glb_idx,
        h5_loc_idx_t loc_idx
        );

h5_loc_idx_t
h5priv_search_idxmap (
        h5_idxmap_t *map,
        h5_glb_idx_t value
        );

h5_err_t
h5priv_sort_idxmap (
        h5_idxmap_t *map
        );

#endif
