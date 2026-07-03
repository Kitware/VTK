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

/*
 * This file contains private information about the r-tree module
 */
#ifndef H5RTprivate_H
#define H5RTprivate_H

#include "H5private.h"

/* The overall r-tree structure */
typedef struct H5RT_t H5RT_t;

/* Struct representing a leaf in the r-tree */
typedef struct H5RT_leaf_t {
    void    *record;
    int      rank;
    hsize_t *min;     /* Points to _coords[0] */
    hsize_t *max;     /* Points to _coords[rank] */
    hsize_t *mid;     /* Points to _coords[2*rank] */
    hsize_t *_coords; /* Private: single allocation for all coordinate arrays */
} H5RT_leaf_t;

/* Dynamic result buffer for efficient search result allocation */
typedef struct H5RT_result_set_t {
    H5RT_leaf_t **results;  /* Array of pointers to result leaves */
    size_t        capacity; /* Current buffer size (power of 2) */
    size_t        count;    /* Number of results used */
} H5RT_result_set_t;

/* Leaf helper functions */
H5_DLL herr_t H5RT_leaf_init(H5RT_leaf_t *leaf, int rank, void *record);
H5_DLL herr_t H5RT_leaf_cleanup(H5RT_leaf_t *leaf);

/* Main R-tree functions */
H5_DLL H5RT_t *H5RT_create(int rank, H5RT_leaf_t *leaves, size_t count);
H5_DLL herr_t  H5RT_search(H5RT_t *rtree, hsize_t min[], hsize_t max[], H5RT_result_set_t **results_out);
H5_DLL herr_t  H5RT_free_results(H5RT_result_set_t *results);
H5_DLL herr_t  H5RT_free(H5RT_t *rtree);
H5_DLL H5RT_t *H5RT_copy(const H5RT_t *rtree);
#endif /* H5RTprivate_H */
