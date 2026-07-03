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

/* Purpose:    This file contains declarations which are visible
 *             only within the H5RT package. Source files outside
 *             the H5RT package should include H5RTprivate.h instead.
 */

#if !(defined H5RT_FRIEND || defined H5RT_MODULE)
#error "Do not include this file outside the H5RT package!"
#endif

#ifndef H5RTpkg_H
#define H5RTpkg_H

/* Get package's private header */
#include "H5RTprivate.h"

/* Other private headers needed by this file */

#define H5RT_MAX_NODE_SIZE 16

/* Forward declaration */
typedef struct H5RT_node_t H5RT_node_t;

/* Internal node of the r-tree */
typedef struct H5RT_node_t {
    hsize_t min[H5S_MAX_RANK]; /* Invalid for root node */
    hsize_t max[H5S_MAX_RANK]; /* Invalid for root node */
    union {
        H5RT_node_t *nodes[H5RT_MAX_NODE_SIZE];
        H5RT_leaf_t *leaves;
    } children;
    int  nchildren;
    bool children_are_leaves;
} H5RT_node_t;

/* Overall r-tree */
struct H5RT_t {
    H5RT_node_t  root;
    H5RT_leaf_t *leaves;
    int          rank;
    size_t       nleaves;
};

/* Inline function to check if two hyper-rectangles intersect */
static inline bool
H5RT__leaves_intersect(int rank, hsize_t min1[], hsize_t max1[], hsize_t min2[], hsize_t max2[])
{
    for (int i = 0; i < rank; i++)
        if (min1[i] > max2[i] || min2[i] > max1[i])
            return false; /* No overlap in i-th dimension */

    return true;
}

#endif /* H5RTpkg_H */
