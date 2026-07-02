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

/****************/
/* Module Setup */
/****************/
#include "H5RTmodule.h" /* This source code file is part of the H5RT module */

#include "H5RTpkg.h"

/***********/
/* Headers */
/***********/
#include "H5private.h"   /* Generic functions */
#include "H5Eprivate.h"  /* Error handling */
#include "H5FLprivate.h" /* Free lists */

H5FL_DEFINE_STATIC(H5RT_t);
H5FL_DEFINE_STATIC(H5RT_node_t);

static herr_t H5RT__bulk_load(H5RT_node_t *node, int rank, H5RT_leaf_t *leaves, size_t count,
                              int prev_sort_dim);
static herr_t H5RT__search_recurse(H5RT_node_t *node, int rank, hsize_t min[], hsize_t max[],
                                   H5RT_result_set_t *result_set);
static herr_t H5RT__node_copy(H5RT_node_t *dest_node, const H5RT_node_t *src_node,
                              const H5RT_leaf_t *old_leaves_base, H5RT_leaf_t *new_leaves_base);
static void   H5RT__free_recurse(H5RT_node_t *node);

/* Result buffer helper functions */
static herr_t H5RT__result_set_init(H5RT_result_set_t *result_set);
static herr_t H5RT__result_set_add(H5RT_result_set_t *result_set, H5RT_leaf_t *leaf);
static herr_t H5RT__result_set_grow(H5RT_result_set_t *result_set);
static int    H5RT__leaf_compare(const void *leaf1, const void *leaf2, void *dim);

/*-------------------------------------------------------------------------
 * Function:    H5RT_leaf_init
 *
 * Purpose:     Initialize an R-tree leaf with dynamic coordinate allocation
 *              based on the specified rank. The leaf structure itself must
 *              already be allocated by the caller.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5RT_leaf_init(H5RT_leaf_t *leaf, int rank, void *record)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    if (!leaf)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid leaf pointer");

    if (rank < 1 || rank > H5S_MAX_RANK)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid rank");

    /* Zero out the leaf structure */
    memset(leaf, 0, sizeof(H5RT_leaf_t));

    /* Allocate coordinate arrays as single block: 3 * rank * sizeof(hsize_t) */
    if (NULL == (leaf->_coords = (hsize_t *)malloc(3 * (size_t)rank * sizeof(hsize_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "failed to allocate leaf coordinates");

    /* Set up pointers to sections of the coordinate block */
    leaf->min = leaf->_coords;
    leaf->max = leaf->_coords + rank;
    leaf->mid = leaf->_coords + (2 * rank);

    leaf->rank   = rank;
    leaf->record = record;

done:
    if (ret_value < 0 && leaf) {
        if (H5RT_leaf_cleanup(leaf) < 0)
            HDONE_ERROR(H5E_RTREE, H5E_CANTRELEASE, FAIL, "failed to clean up leaf on error");
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5RT_leaf_init() */

/*-------------------------------------------------------------------------
 * Function:    H5RT_leaf_cleanup
 *
 * Purpose:     Clean up a leaf's coordinate arrays. The leaf structure
 *              itself is not freed - that's the caller's responsibility.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5RT_leaf_cleanup(H5RT_leaf_t *leaf)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    if (!leaf)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid leaf");

    /* Free coordinate arrays */
    if (leaf->_coords) {
        free(leaf->_coords);
        leaf->_coords = NULL;
        leaf->min     = NULL;
        leaf->max     = NULL;
        leaf->mid     = NULL;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5RT_leaf_cleanup() */

/*-------------------------------------------------------------------------
 * Function:    H5RT__leaf_compare
 *
 * Purpose:     Compare two R-tree leaves for sorting based on their midpoint
 *              coordinates in the specified dimension.
 *
 *              Uses GNU qsort_r signature (context parameter last) for
 *              compatibility with HDqsort_r macro.
 *
 * Return:      -1 if leaf1 < leaf2
 *               0 if leaf1 == leaf2
 *               1 if leaf1 > leaf2
 *
 *-------------------------------------------------------------------------
 */
static int
H5RT__leaf_compare(const void *leaf1, const void *leaf2, void *dim)
{
    const H5RT_leaf_t *l1       = (const H5RT_leaf_t *)leaf1;
    const H5RT_leaf_t *l2       = (const H5RT_leaf_t *)leaf2;
    int                sort_dim = 0;

    assert(leaf1);
    assert(leaf2);
    assert(dim);

    sort_dim = *(int *)dim;

    assert(sort_dim <= l1->rank - 1);

    /* Compare based on the midpoint of the specified dimension */
    if (l1->mid[sort_dim] < l2->mid[sort_dim])
        return -1;
    if (l1->mid[sort_dim] > l2->mid[sort_dim])
        return 1;
    return 0;
} /* end H5RT__leaf_compare() */

/*-------------------------------------------------------------------------
 * Function:    H5RT__compute_slabs
 *
 * Purpose:     Compute the number of slabs and slab size to use when partitioning
 *              leaves into slabs for bulk-loading the r-tree.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5RT__compute_slabs(size_t node_capacity, size_t leaf_count, size_t *slab_count_out, size_t *slab_size_out)
{
    assert(node_capacity > 0);
    assert(leaf_count > 0);
    assert(slab_count_out);
    assert(slab_size_out);
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    double num_slabs_d = -1.0;
    size_t num_slabs   = 0;
    double slab_size_d = -1.0;
    size_t slab_size   = 0;

    if (leaf_count <= node_capacity) {
        /* All leaves will fit into a single node */
        num_slabs = 1;
        slab_size = leaf_count;
    }
    else {
        /* Use intermediate variable to avoid warnings */
        slab_size_d = ceil((double)leaf_count / (double)node_capacity);

        if (slab_size_d > (double)SIZE_MAX)
            HGOTO_ERROR(H5E_RTREE, H5E_OVERFLOW, FAIL, "slab size overflows size_t");
        assert(slab_size_d > 0.0);
        slab_size = (size_t)slab_size_d;
        assert(slab_size > 0);

        num_slabs_d = ceil((double)leaf_count / (double)slab_size);
        if (num_slabs_d > (double)SIZE_MAX)
            HGOTO_ERROR(H5E_RTREE, H5E_OVERFLOW, FAIL, "number of slabs overflows size_t");
        assert(num_slabs_d > 0.0);
        num_slabs = (size_t)num_slabs_d;
    }

    assert(slab_size > 0);
    assert(slab_size <= leaf_count);

    assert(num_slabs > 0);
    assert(num_slabs <= node_capacity);
done:
    if (ret_value == SUCCEED) {
        *slab_count_out = num_slabs;
        *slab_size_out  = slab_size;
    }
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5RT__compute_slabs() */

/*-------------------------------------------------------------------------
 * Function:    H5RT__result_set_init
 *
 * Purpose:     Initialize a result buffer with initial capacity
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5RT__result_set_init(H5RT_result_set_t *result_set)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(result_set);

    result_set->capacity = 32; /* Initial power-of-2 size */
    result_set->count    = 0;

    /* Allocate initial buffer */
    result_set->results = (H5RT_leaf_t **)malloc(result_set->capacity * sizeof(H5RT_leaf_t *));
    if (!result_set->results)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "failed to allocate result buffer");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5RT__result_set_init() */

/*-------------------------------------------------------------------------
 * Function:    H5RT__result_set_grow
 *
 * Purpose:     Double the capacity of the result buffer and fix next pointers
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5RT__result_set_grow(H5RT_result_set_t *result_set)
{
    herr_t        ret_value    = SUCCEED;
    size_t        new_capacity = 0;
    H5RT_leaf_t **new_results  = NULL;

    FUNC_ENTER_PACKAGE

    assert(result_set);
    assert(result_set->results);
    assert(result_set->count == result_set->capacity);

    new_capacity = result_set->capacity * 2;

    /* Overflow check */
    if (new_capacity < result_set->capacity || new_capacity > (SIZE_MAX / sizeof(H5RT_leaf_t *)))
        HGOTO_ERROR(H5E_RTREE, H5E_OVERFLOW, FAIL, "result buffer capacity overflow");

    /* Reallocate the buffer */
    new_results = (H5RT_leaf_t **)realloc(result_set->results, new_capacity * sizeof(H5RT_leaf_t *));
    if (!new_results)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "failed to grow result buffer");

    result_set->results  = new_results;
    result_set->capacity = new_capacity;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5RT__result_set_grow() */

/*-------------------------------------------------------------------------
 * Function:    H5RT__result_set_add
 *
 * Purpose:     Add a leaf to the result buffer, growing if necessary
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5RT__result_set_add(H5RT_result_set_t *result_set, H5RT_leaf_t *leaf)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(result_set);
    assert(leaf);

    /* Grow buffer if full */
    if (result_set->count >= result_set->capacity) {
        if (H5RT__result_set_grow(result_set) < 0)
            HGOTO_ERROR(H5E_RTREE, H5E_CANTALLOC, FAIL, "failed to grow result buffer");
    }

    /* Add the new result */
    result_set->results[result_set->count] = leaf;
    result_set->count++;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5RT__result_set_add() */

/*-------------------------------------------------------------------------
 * Function:    H5RT__bulk_load
 *
 * Purpose:     Load the provided leaves into the r-tree in an efficient manner.
 *              This is an implementation of the sort-tile-recursive (STR) algorithm.
 *              See "STR: A Simple and Efficient Algorithm for R-Tree Packing"
 *              https://archive.org/details/nasa_techdoc_19970016975/page/n9
 *
 * Parameters:  node          - The node to fill
 *              rank          - The rank of the hyper-rectangles
 *              leaves        - A pointer to the first leaf in this block
 *              count         - The number of leaves in this block
 *              prev_sort_dim - The dimension that was last sorted on (or -1 if none)
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5RT__bulk_load(H5RT_node_t *node, int rank, H5RT_leaf_t *leaves, size_t count, int prev_sort_dim)
{
    herr_t       ret_value        = SUCCEED;
    size_t       leaves_left      = 0; /* Leaves left to partition */
    size_t       child_leaf_count = 0;
    H5RT_leaf_t *child_leaf_start = NULL;

    int sort_dim = -1;

    size_t num_slabs = 0;
    size_t slab_size = 0;

    FUNC_ENTER_PACKAGE

    assert(node);
    assert(leaves);
    assert(count > 0);
    assert(prev_sort_dim >= -1);
    assert(rank >= 1 && rank <= H5S_MAX_RANK);

    /* Compute the max/min bounds of the provided node */
    /* Initial values */
    for (int i = 0; i < rank; i++) {
        node->min[i] = leaves[0].min[i];
        node->max[i] = leaves[0].max[i];
    }
    /* Compute max/min from leaves */
    for (size_t i = 0; i < count; i++) {
        for (int d = 0; d < rank; d++) {
            if (leaves[i].min[d] < node->min[d])
                node->min[d] = leaves[i].min[d];
            if (leaves[i].max[d] > node->max[d])
                node->max[d] = leaves[i].max[d];
        }
    }

    if (count <= H5RT_MAX_NODE_SIZE) {
        /* Base Case - All leaves will fit into this node */
        node->nchildren           = (int)count;
        node->children_are_leaves = true;
        node->children.leaves     = leaves;
    }
    else {
        /* Recursive case - there will be child nodes */
        node->children_are_leaves = false;

        /* If we haven't sorted along every dimension yet, sort the hyper-rectangles in this region
         * by the first unsorted coordinate of their midpoints */
        if (prev_sort_dim != rank - 1) {
            assert(prev_sort_dim < rank - 1);
            sort_dim = prev_sort_dim + 1;
            if (H5_UNLIKELY(HDqsort_r((void *)leaves, count, sizeof(H5RT_leaf_t), H5RT__leaf_compare,
                                      (void *)&sort_dim) < 0))
                HGOTO_ERROR(H5E_INTERNAL, H5E_CANTSORT, FAIL, "failed to sort R-tree leaves");
        }
        else {
            sort_dim = prev_sort_dim;
        }

        /* After leaves are sorted in the current dimension, partition the hyper-rectangles into slabs */

        /* Compute number of slabs and slab size for partitioning */
        H5RT__compute_slabs(H5RT_MAX_NODE_SIZE, count, &num_slabs, &slab_size);

        node->nchildren = (int)num_slabs;

        /* Persistent pointer that is moved forward after each assignment
         * of a region leaves to a child node */
        child_leaf_start = leaves;
        leaves_left      = count;

        /* Recurse down to the next dimension to process each slab/region */
        for (int i = 0; i < node->nchildren; i++) {
            /* The final slab should exactly contain the last leaf */
            assert(leaves_left > 0);

            /* Allocate this child node */
            if (NULL == (node->children.nodes[i] = H5FL_MALLOC(H5RT_node_t)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "failed to allocate memory for R-tree node");

            child_leaf_count = (leaves_left < slab_size) ? leaves_left : slab_size;

            /* Recursively fill this child node with leaves from 'child_leaf_start' to 'child_leaf_start' +
             * 'child_leaf_count' */
            if (H5RT__bulk_load(node->children.nodes[i], rank, child_leaf_start, child_leaf_count, sort_dim) <
                0)
                HGOTO_ERROR(H5E_RTREE, H5E_CANTINIT, FAIL, "failed to fill R-tree");

            /* The next 'child_leaf_count' leaves are now assigned */
            child_leaf_start += child_leaf_count;
            leaves_left -= child_leaf_count;
        }
    }

done:
    if (ret_value < 0) {
        /* Free any nodes that were allocated at this level */
        if (node && !node->children_are_leaves) {
            for (int i = 0; i < node->nchildren; i++) {
                if (node->children.nodes[i]) {
                    H5FL_FREE(H5RT_node_t, node->children.nodes[i]);
                    node->children.nodes[i] = NULL;
                }
            }
        }
    }
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5RT__bulk_load() */

/*-------------------------------------------------------------------------
 * Function:    H5RT_create
 *
 * Purpose:     Create a new R-tree from the provided array of 'count'
 *               leaves, each with 'rank' spatial dimensions.
 *
 *              On success, the R-tree takes ownership of the caller-allocated
 *               leaves array.
 *
 *              NOTE: This routine uses a global variable internally, and
 *                is therefore not thread-safe. See the 'qsort_r_threadsafe'
 *                branch of the HDF5 GitHub repository for a beta
 *                implementation that is threadsafe.
 *
 * Return:      A valid pointer to the new R-tree on success/NULL on failure
 *
 *-------------------------------------------------------------------------
 */
H5RT_t *
H5RT_create(int rank, H5RT_leaf_t *leaves, size_t count)
{
    H5RT_t *rtree     = NULL;
    H5RT_t *ret_value = NULL;

    FUNC_ENTER_NOAPI(NULL)

    if (rank < 1 || rank > H5S_MAX_RANK)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid rank");

    if (count == 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "r-tree must have at least one leaf");

    if (NULL == (rtree = H5FL_MALLOC(H5RT_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, NULL, "failed to allocate memory for R-tree");

    rtree->rank    = rank;
    rtree->nleaves = count;

    /* Take ownership of leaves array */
    rtree->leaves = leaves;

    /* Populate the r-tree with nodes containing the provided leaves */
    if (H5RT__bulk_load(&rtree->root, rank, rtree->leaves, count, -1) < 0)
        HGOTO_ERROR(H5E_RTREE, H5E_CANTINIT, NULL, "failed to fill R-tree");

    ret_value = rtree;

done:
    if (!ret_value && rtree)
        H5RT_free(rtree);

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5RT_create() */

/*-------------------------------------------------------------------------
 * Function:    H5RT__search_recurse
 *
 * Purpose:     Recursively search the r-tree for leaves whose bounding boxes
 *              intersect with the provided search region.
 *
 * Parameters:  node (in)   - Node from which to begin the search
 *              rank (in)   - Rank of the hyper-rectangles
 *              min (in)    - Minimum bounds of spatial search, should have 'rank' dims
 *              max (in)    - Maximum bounds of spatial search, should have 'rank' dims
 *              buffer (io) - Dynamic result buffer to add matches to
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5RT__search_recurse(H5RT_node_t *node, int rank, hsize_t min[], hsize_t max[], H5RT_result_set_t *result_set)
{
    hsize_t *curr_min  = NULL;
    hsize_t *curr_max  = NULL;
    herr_t   ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(node);
    assert(result_set);

    /* Check all children for intersection */
    if (node->children_are_leaves)
        for (int i = 0; i < node->nchildren; i++) {
            H5RT_leaf_t *curr_leaf = node->children.leaves + i;
            curr_min               = curr_leaf->min;
            curr_max               = curr_leaf->max;

            if (H5RT__leaves_intersect(rank, min, max, curr_min, curr_max)) {
                /* We found an intersecting leaf, add it to the result set */
                if (H5RT__result_set_add(result_set, curr_leaf) < 0)
                    HGOTO_ERROR(H5E_RTREE, H5E_CANTALLOC, FAIL, "failed to add result to result set");
            }
        }
    else
        for (int i = 0; i < node->nchildren; i++) {
            /* This is an internal node in the r-tree */
            H5RT_node_t *curr_node = node->children.nodes[i];
            curr_min               = curr_node->min;
            curr_max               = curr_node->max;

            /* Only recurse into child node if its bounding box overlaps with the search region */
            if (H5RT__leaves_intersect(rank, min, max, curr_min, curr_max)) {
                /* We found an intersecting internal node, recurse into it */
                if (H5RT__search_recurse(curr_node, rank, min, max, result_set) < 0)
                    HGOTO_ERROR(H5E_RTREE, H5E_CANTGET, FAIL, "recursive search failed");
            }
        }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5RT__search_recurse() */

/*-------------------------------------------------------------------------
 * Function:    H5RT_search
 *
 * Purpose:     Search the r-tree for leaves whose bounding boxes
 *              intersect with the provided min and max bounds.
 *
 *              Returns a linked list of H5RT_leaf_t * structures.
 *              The caller must call H5RT_free_results() to free the
 *              returned result list.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5RT_search(H5RT_t *rtree, hsize_t min[], hsize_t max[], H5RT_result_set_t **results_out)
{
    H5RT_result_set_t *result_set = NULL;
    herr_t             ret_value  = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    assert((hsize_t *)min);
    assert((hsize_t *)max);

    if (!rtree)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid r-tree");

    if (!results_out)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid results output pointer");

    /* Initialize result buffer */
    if ((result_set = (H5RT_result_set_t *)malloc(sizeof(H5RT_result_set_t))) == NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "failed to allocate result set");

    if (H5RT__result_set_init(result_set) < 0)
        HGOTO_ERROR(H5E_RTREE, H5E_CANTINIT, FAIL, "failed to initialize result buffer");

    /* Perform the actual search */
    if (H5RT__search_recurse(&rtree->root, rtree->rank, min, max, result_set) < 0)
        HGOTO_ERROR(H5E_RTREE, H5E_CANTGET, FAIL, "search failed");

    /* Don't cleanup result set on success - caller owns it now */
    *results_out = result_set;

done:
    if (ret_value < 0) {
        /* Clean up buffer on failure */
        if (H5RT_free_results(result_set) < 0)
            HDONE_ERROR(H5E_RTREE, H5E_CANTFREE, FAIL, "unable to free result set on error");
        *results_out = NULL;
    }
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5RT_search() */

/*-------------------------------------------------------------------------
 * Function:    H5RT_free_results
 *
 * Purpose:     Free search results returned by H5RT_search.
 *
 *              Frees both the result set structure and the underlying
 *              results array buffer.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5RT_free_results(H5RT_result_set_t *result_set)
{
    herr_t ret_value = SUCCEED;

    assert(result_set);

    /* Free the results array buffer */
    if (result_set->results)
        free(result_set->results);

    /* Free the result set structure itself */
    free(result_set);

    return ret_value;
} /* end H5RT_free_results() */

/*-------------------------------------------------------------------------
 * Function:    H5RT__node_copy
 *
 * Purpose:     Deep copy a node from source tree to destination tree,
 *              recursively copying all child nodes and remapping leaf
 *              pointers to the new leaves array.
 *
 * Parameters:  dest_node       - Pre-allocated destination node to copy into
 *              src_node        - Source node to copy from
 *              old_leaves_base - Base pointer of original tree's leaves array
 *              new_leaves_base - Base pointer of new tree's leaves array
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5RT__node_copy(H5RT_node_t *dest_node, const H5RT_node_t *src_node, const H5RT_leaf_t *old_leaves_base,
                H5RT_leaf_t *new_leaves_base)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(dest_node);
    assert(src_node);
    assert(old_leaves_base);
    assert(new_leaves_base);

    /* Copy basic node fields */
    memcpy(dest_node->min, src_node->min, H5S_MAX_RANK * sizeof(hsize_t));
    memcpy(dest_node->max, src_node->max, H5S_MAX_RANK * sizeof(hsize_t));
    dest_node->nchildren           = src_node->nchildren;
    dest_node->children_are_leaves = src_node->children_are_leaves;

    if (src_node->children_are_leaves) {
        /* Leaf node: remap the leaves pointer to the corresponding position in new array */
        ptrdiff_t offset           = src_node->children.leaves - old_leaves_base;
        dest_node->children.leaves = new_leaves_base + offset;
    }
    else {
        /* Internal node: recursively copy each child node */
        for (int i = 0; i < src_node->nchildren; i++) {
            /* Allocate new child node */
            if (NULL == (dest_node->children.nodes[i] = H5FL_MALLOC(H5RT_node_t)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "failed to allocate child node");

            /* Recursively copy the child */
            if (H5RT__node_copy(dest_node->children.nodes[i], src_node->children.nodes[i], old_leaves_base,
                                new_leaves_base) < 0)
                HGOTO_ERROR(H5E_RTREE, H5E_CANTCOPY, FAIL, "failed to copy child node");
        }
    }

done:
    if (ret_value < 0 && dest_node && !dest_node->children_are_leaves) {
        /* Clean up any partially allocated child nodes */
        for (int i = 0; i < dest_node->nchildren; i++) {
            if (dest_node->children.nodes[i]) {
                H5FL_FREE(H5RT_node_t, dest_node->children.nodes[i]);
                dest_node->children.nodes[i] = NULL;
            }
        }
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5RT__node_copy() */

/*-------------------------------------------------------------------------
 * Function:    H5RT__free_recurse
 *
 * Purpose:     Recursively free the provided node and its child nodes.
 *              Does not free the leaves.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static void
H5RT__free_recurse(H5RT_node_t *node)
{
    FUNC_ENTER_PACKAGE_NOERR

    assert(node);

    /* Only recurse if the children are more internal nodes */
    if (!node->children_are_leaves)
        for (int i = 0; i < node->nchildren; i++) {
            if (node->children.nodes[i]) {
                H5RT__free_recurse(node->children.nodes[i]);
                H5FL_FREE(H5RT_node_t, node->children.nodes[i]);
            }
        }

    FUNC_LEAVE_NOAPI_VOID
} /* end H5RT__free_recurse() */

/*-------------------------------------------------------------------------
 * Function:    H5RT_free
 *
 * Purpose:     Release the memory associated with an r-tree.
 *              The data pointed to by the leaves is left as-is.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5RT_free(H5RT_t *rtree)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL);

    if (!rtree)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid r-tree");

    H5RT__free_recurse(&rtree->root);

    /* Free each leaf's coordinates (leaf structures are in array, not individually allocated) */
    for (size_t i = 0; i < rtree->nleaves; i++) {
        if (rtree->leaves[i]._coords)
            free(rtree->leaves[i]._coords);
    }

    /* Free the leaves array */
    free(rtree->leaves);
    H5FL_FREE(H5RT_t, rtree);

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5RT_free() */

/*-------------------------------------------------------------------------
 * Function:    H5RT_copy
 *
 * Purpose:     Deep-copy the provided r-tree
 *
 *              NOTE:  The 'record' pointers in the leaves are shallow-copied.
 *
 * Return:      A valid pointer to the new r-tree on success/NULL on failure
 *
 *-------------------------------------------------------------------------
 */
H5RT_t *
H5RT_copy(const H5RT_t *rtree)
{
    H5RT_t *ret_value = NULL;
    H5RT_t *new_tree  = NULL;

    H5RT_leaf_t *new_leaves = NULL;

    FUNC_ENTER_NOAPI(NULL);

    if (!rtree)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid r-tree");

    assert(rtree->leaves);
    assert(rtree->nleaves > 0);

    /* Deep copy the array of leaves */
    if (NULL == (new_leaves = (H5RT_leaf_t *)malloc(rtree->nleaves * sizeof(H5RT_leaf_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, NULL, "failed to allocate memory for R-tree leaves");

    /* Deep copy each leaf manually */
    for (size_t i = 0; i < rtree->nleaves; i++) {
        const H5RT_leaf_t *src_leaf = &rtree->leaves[i];

        /* Allocate coordinate arrays for this leaf */
        new_leaves[i]._coords = (hsize_t *)malloc(3 * (size_t)src_leaf->rank * sizeof(hsize_t));
        if (!new_leaves[i]._coords) {
            /* Clean up already copied leaves */
            for (size_t j = 0; j < i; j++) {
                if (new_leaves[j]._coords)
                    free(new_leaves[j]._coords);
            }
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, NULL, "failed to allocate leaf coordinates");
        }

        /* Set up the leaf structure */
        new_leaves[i].record = src_leaf->record;
        new_leaves[i].rank   = src_leaf->rank;
        new_leaves[i].min    = new_leaves[i]._coords;
        new_leaves[i].max    = new_leaves[i]._coords + src_leaf->rank;
        new_leaves[i].mid    = new_leaves[i]._coords + (2 * src_leaf->rank);

        /* Copy coordinate data */
        memcpy(new_leaves[i]._coords, src_leaf->_coords, 3 * (size_t)src_leaf->rank * sizeof(hsize_t));
    }

    /* Allocate new tree structure */
    if (NULL == (new_tree = H5FL_MALLOC(H5RT_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, NULL, "failed to allocate memory for R-tree copy");

    /* Copy basic tree fields */
    new_tree->rank    = rtree->rank;
    new_tree->nleaves = rtree->nleaves;
    new_tree->leaves  = new_leaves;

    /* Deep copy the root node structure */
    if (H5RT__node_copy(&new_tree->root, &rtree->root, rtree->leaves, new_leaves) < 0)
        HGOTO_ERROR(H5E_RTREE, H5E_CANTCOPY, NULL, "failed to copy r-tree structure");

    ret_value = new_tree;

done:
    if (!ret_value) {
        if (new_tree) {
            if (H5RT_free(new_tree) < 0)
                HDONE_ERROR(H5E_RTREE, H5E_CANTFREE, NULL, "unable to free partially copied r-tree");
        }
        else if (new_leaves) {
            /* Free copied leaves and their coordinates */
            for (size_t i = 0; i < rtree->nleaves; i++) {
                if (new_leaves[i]._coords)
                    free(new_leaves[i]._coords);
            }
            free(new_leaves);
        }
    }

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5RT_copy() */
