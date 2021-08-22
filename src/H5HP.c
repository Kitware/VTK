/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Purpose:	Provides a heap abstract data type.
 *
 *              (See chapter 11 - "Priority Queues" of _Algorithms_, by
 *              Sedgewick for additional information)
 *
 */

/* Private headers needed */
#include "H5private.h"   /* Generic Functions			*/
#include "H5Eprivate.h"  /* Error handling		  	*/
#include "H5HPprivate.h" /* Heap routines			*/
#include "H5FLprivate.h" /* Memory management functions		*/

/* Local Macros */
#define H5HP_START_SIZE 16 /* Initial number of entries for heaps */

/* Private typedefs & structs */

/* Data structure for entries in the internal heap array */
typedef struct {
    int          val; /* Value to be used for heap condition */
    H5HP_info_t *obj; /* Pointer to object stored in heap */
} H5HP_ent_t;

/* Main heap data structure */
struct H5HP_t {
    H5HP_type_t type;   /* Type of heap (minimum or maximum value at "top") */
    size_t      nobjs;  /* Number of active objects in heap array */
    size_t      nalloc; /* Number of allocated locations in heap array */
    H5HP_ent_t *heap;   /* Pointer to array containing heap entries */
};

/* Static functions */
static herr_t H5HP__swim_max(H5HP_t *heap, size_t loc);
static herr_t H5HP__swim_min(H5HP_t *heap, size_t loc);
static herr_t H5HP__sink_max(H5HP_t *heap, size_t loc);
static herr_t H5HP__sink_min(H5HP_t *heap, size_t loc);

/* Declare a free list to manage the H5HP_t struct */
H5FL_DEFINE_STATIC(H5HP_t);

/* Declare a free list to manage sequences of H5HP_ent_t */
H5FL_SEQ_DEFINE_STATIC(H5HP_ent_t);

/*--------------------------------------------------------------------------
 NAME
    H5HP__swim_max
 PURPOSE
    Restore heap condition by moving an object upward
 USAGE
    herr_t H5HP__swim_max(heap, loc)
        H5HP_t *heap;           IN/OUT: Pointer to heap to modify
        size_t loc;             IN: Location to start from

 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Restore the heap condition for the heap's array by "swimming" the object
    at a location upward.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine is for "maximum" value heaps.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5HP__swim_max(H5HP_t *heap, size_t loc)
{
    int          val;                 /* Temporary copy value of object to move in heap */
    H5HP_info_t *obj;                 /* Temporary pointer to object to move in heap */
    herr_t       ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC_NOERR

    /* Get copies of the information about the object to move in the heap */
    val = heap->heap[loc].val;
    obj = heap->heap[loc].obj;

    /* Move object up in heap until it's reached the maximum location possible */
    while (heap->heap[loc / 2].val < val) {
        /* Move object "above" current location in heap down */
        heap->heap[loc].val = heap->heap[loc / 2].val;
        heap->heap[loc].obj = heap->heap[loc / 2].obj;

        /* Update heap location for object which moved */
        heap->heap[loc].obj->heap_loc = loc;

        /* Move to location "above" current location */
        loc = loc / 2;
    } /* end while */

    /* Put object into heap at correct location */
    heap->heap[loc].val = val;
    heap->heap[loc].obj = obj;

    /* Update heap location for object */
    heap->heap[loc].obj->heap_loc = loc;

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5HP__swim_max() */

/*--------------------------------------------------------------------------
 NAME
    H5HP__swim_min
 PURPOSE
    Restore heap condition by moving an object upward
 USAGE
    herr_t H5HP__swim_min(heap, loc)
        H5HP_t *heap;           IN/OUT: Pointer to heap to modify
        size_t loc;             IN: Location to start from

 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Restore the heap condition for the heap's array by "swimming" the object
    at a location upward.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine is for "minimum" value heaps.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5HP__swim_min(H5HP_t *heap, size_t loc)
{
    int          val;                 /* Temporary copy value of object to move in heap */
    H5HP_info_t *obj;                 /* Temporary pointer to object to move in heap */
    herr_t       ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC_NOERR

    /* Get copies of the information about the object to move in the heap */
    val = heap->heap[loc].val;
    obj = heap->heap[loc].obj;

    /* Move object up in heap until it's reached the minimum location possible */
    while (heap->heap[loc / 2].val > val) {
        /* Move object "above" current location in heap down */
        heap->heap[loc].val = heap->heap[loc / 2].val;
        heap->heap[loc].obj = heap->heap[loc / 2].obj;

        /* Update heap location for object which moved */
        heap->heap[loc].obj->heap_loc = loc;

        /* Move to location "above" current location */
        loc = loc / 2;
    } /* end while */

    /* Put object into heap at correct location */
    heap->heap[loc].val = val;
    heap->heap[loc].obj = obj;

    /* Update heap location for object */
    heap->heap[loc].obj->heap_loc = loc;

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5HP__swim_min() */

/*--------------------------------------------------------------------------
 NAME
    H5HP__sink_max
 PURPOSE
    Restore heap condition by moving an object downward
 USAGE
    herr_t H5HP__sink_max(heap, loc)
        H5HP_t *heap;           IN/OUT: Pointer to heap to modify
        size_t loc;             IN: Location to start from

 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Restore the heap condition for the heap's array by "sinking" the object
    at a location downward.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine is for "maximum" value heaps.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5HP__sink_max(H5HP_t *heap, size_t loc)
{
    int    val;                 /* Temporary copy value of object to move in heap */
    void * obj;                 /* Temporary pointer to object to move in heap */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC_NOERR

    /* Get copies of the information about the object to move in the heap */
    val = heap->heap[loc].val;
    obj = heap->heap[loc].obj;

    /* Move object up in heap until it's reached the maximum location possible */
    while ((2 * loc) <= heap->nobjs) {
        size_t new_loc = loc * 2; /* New object's potential location area */

        /* Get the greater of the two objects below the location in heap */
        if (new_loc < heap->nobjs && (heap->heap[new_loc].val < heap->heap[new_loc + 1].val))
            new_loc++;

        /* Check if the object is smaller than the larger of the objects below it */
        /* If so, its in the correct location now, and we can get out */
        if (val >= heap->heap[new_loc].val)
            break;

        /* Move the greater of the two objects below the current location up */
        heap->heap[loc].val = heap->heap[new_loc].val;
        heap->heap[loc].obj = heap->heap[new_loc].obj;

        /* Update heap location for object which moved */
        heap->heap[loc].obj->heap_loc = loc;

        /* Move to location "below" current location */
        loc = new_loc;
    } /* end while */

    /* Put object into heap at correct location */
    heap->heap[loc].val = val;
    heap->heap[loc].obj = (H5HP_info_t *)obj;

    /* Update heap location for object */
    heap->heap[loc].obj->heap_loc = loc;

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5HP__sink_max() */

/*--------------------------------------------------------------------------
 NAME
    H5HP__sink_min
 PURPOSE
    Restore heap condition by moving an object downward
 USAGE
    herr_t H5HP__sink_min(heap, loc)
        H5HP_t *heap;           IN/OUT: Pointer to heap to modify
        size_t loc;             IN: Location to start from

 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Restore the heap condition for the heap's array by "sinking" the object
    at a location downward.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine is for "minimum" value heaps.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5HP__sink_min(H5HP_t *heap, size_t loc)
{
    int    val;                 /* Temporary copy value of object to move in heap */
    void * obj;                 /* Temporary pointer to object to move in heap */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC_NOERR

    /* Get copies of the information about the object to move in the heap */
    val = heap->heap[loc].val;
    obj = heap->heap[loc].obj;

    /* Move object up in heap until it's reached the maximum location possible */
    while ((2 * loc) <= heap->nobjs) {
        size_t new_loc = loc * 2; /* New object's potential location area */

        /* Get the lesser of the two objects below the location in heap */
        if (new_loc < heap->nobjs && (heap->heap[new_loc].val > heap->heap[new_loc + 1].val))
            new_loc++;

        /* Check if the object is greater than the larger of the objects below it */
        /* If so, its in the correct location now, and we can get out */
        if (val <= heap->heap[new_loc].val)
            break;

        /* Move the greater of the two objects below the current location up */
        heap->heap[loc].val = heap->heap[new_loc].val;
        heap->heap[loc].obj = heap->heap[new_loc].obj;

        /* Update heap location for object which moved */
        heap->heap[loc].obj->heap_loc = loc;

        /* Move to location "below" current location */
        loc = new_loc;
    } /* end while */

    /* Put object into heap at correct location */
    heap->heap[loc].val = val;
    heap->heap[loc].obj = (H5HP_info_t *)obj;

    /* Update heap location for object */
    heap->heap[loc].obj->heap_loc = loc;

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5HP__sink_min() */

/*--------------------------------------------------------------------------
 NAME
    H5HP_create
 PURPOSE
    Create a heap
 USAGE
    H5HP_t *H5HP_create(heap_type)
        H5HP_type_t heap_type;          IN: Type of heap to create

 RETURNS
    Returns a pointer to a heap on success, NULL on failure.
 DESCRIPTION
    Create a priority queue.  The SIZE is used to set the initial number of
    entries allocated.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5HP_t *
H5HP_create(H5HP_type_t heap_type)
{
    H5HP_t *new_heap = NULL; /* Pointer to new heap object created */
    H5HP_t *ret_value;       /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Check args */
    HDassert(heap_type == H5HP_MIN_HEAP || heap_type == H5HP_MAX_HEAP);

    /* Allocate ref-counted string structure */
    if ((new_heap = H5FL_MALLOC(H5HP_t)) == NULL)
        HGOTO_ERROR(H5E_HEAP, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Allocate the array to store the heap entries */
    if ((new_heap->heap = H5FL_SEQ_MALLOC(H5HP_ent_t, (size_t)(H5HP_START_SIZE + 1))) == NULL)
        HGOTO_ERROR(H5E_HEAP, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Set the internal fields */
    new_heap->type   = heap_type;
    new_heap->nobjs  = 0;
    new_heap->nalloc = H5HP_START_SIZE + 1;

    /* Set the information in the 0'th location based on the type of heap */
    if (heap_type == H5HP_MIN_HEAP) {
        /* Set the value in the '0' location to be the minimum value, to
         * simplify the algorithms
         */
        new_heap->heap[0].val = INT_MIN;
        new_heap->heap[0].obj = NULL;
    } /* end if */
    else {
        /* Set the value in the '0' location to be the maximum value, to
         * simplify the algorithms
         */
        new_heap->heap[0].val = INT_MAX;
        new_heap->heap[0].obj = NULL;
    } /* end else */

    /* Set the return value */
    ret_value = new_heap;

done:
    /* Error cleanup */
    if (NULL == ret_value) {
        if (NULL != new_heap) {
            if (NULL != new_heap->heap)
                new_heap->heap = H5FL_SEQ_FREE(H5HP_ent_t, new_heap->heap);
            new_heap = H5FL_FREE(H5HP_t, new_heap);
        } /* end if */
    }     /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5HP_create() */

/*--------------------------------------------------------------------------
 NAME
    H5HP_count
 PURPOSE
    Check the number of elements in a heap
 USAGE
    ssize_t H5HP_count(heap)
        const H5HP_t *heap;     IN: Pointer to heap to query

 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Checks the number of elements in heap
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
ssize_t
H5HP_count(const H5HP_t *heap)
{
    ssize_t ret_value; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(heap);

    /* Check internal consistency */
    /* (Pre-condition) */
    HDassert(heap->nobjs < heap->nalloc);
    HDassert(heap->heap);
    HDassert((heap->type == H5HP_MAX_HEAP && heap->heap[0].val == INT_MAX) ||
             (heap->type == H5HP_MIN_HEAP && heap->heap[0].val == INT_MIN));
    HDassert(heap->heap[0].obj == NULL);

    /* Return the number of objects in the heap */
    H5_CHECK_OVERFLOW(heap->nobjs, size_t, ssize_t);
    ret_value = (ssize_t)heap->nobjs;

    /* No post-condition check necessary, since heap is constant */
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5HP_count() */

/*--------------------------------------------------------------------------
 NAME
    H5HP_insert
 PURPOSE
    Insert an object into a heap, with an initial value
 USAGE
    herr_t H5HP_insert(heap, val, obj)
        H5HP_t *heap;           IN/OUT: Pointer to heap to modify
        int val;                IN: Initial value for object in heap
        void *obj;              IN: Pointer to object to insert into heap

 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Inserts a OBJ into a HEAP, with an initial VALue.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5HP_insert(H5HP_t *heap, int val, void *obj)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(heap);
    HDassert(obj);

    /* Check internal consistency */
    /* (Pre-condition) */
    HDassert(heap->nobjs < heap->nalloc);
    HDassert(heap->heap);
    HDassert((heap->type == H5HP_MAX_HEAP && heap->heap[0].val == INT_MAX) ||
             (heap->type == H5HP_MIN_HEAP && heap->heap[0].val == INT_MIN));
    HDassert(heap->heap[0].obj == NULL);

    /* Increment number of objects in heap */
    heap->nobjs++;

    /* Check if we need to allocate more room for heap array */
    if (heap->nobjs >= heap->nalloc) {
        size_t      n        = MAX(H5HP_START_SIZE, 2 * (heap->nalloc - 1)) + 1;
        H5HP_ent_t *new_heap = H5FL_SEQ_REALLOC(H5HP_ent_t, heap->heap, n);

        if (!new_heap)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to extend heap array");
        heap->heap   = new_heap;
        heap->nalloc = n;
    } /* end if */

    /* Insert new object at end of heap */
    heap->heap[heap->nobjs].val           = val;
    heap->heap[heap->nobjs].obj           = (H5HP_info_t *)obj;
    heap->heap[heap->nobjs].obj->heap_loc = heap->nobjs;

    /* Restore heap condition */
    if (heap->type == H5HP_MAX_HEAP) {
        if (H5HP__swim_max(heap, heap->nobjs) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTINSERT, FAIL, "unable to restore heap condition");
    } /* end if */
    else {
        if (H5HP__swim_min(heap, heap->nobjs) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTINSERT, FAIL, "unable to restore heap condition");
    } /* end else */

done:

    /* Check internal consistency */
    /* (Post-condition) */
    HDassert(heap->nobjs < heap->nalloc);
    HDassert(heap->heap);
    HDassert((heap->type == H5HP_MAX_HEAP && heap->heap[0].val == INT_MAX) ||
             (heap->type == H5HP_MIN_HEAP && heap->heap[0].val == INT_MIN));
    HDassert(heap->heap[0].obj == NULL);

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5HP_insert() */

/*--------------------------------------------------------------------------
 NAME
    H5HP_top
 PURPOSE
    Check the value of the top object in the heap
 USAGE
    herr_t H5HP_top(heap, val)
        const H5HP_t *heap;     IN: Pointer to heap to modify
        int val;                IN/OUT: Initial value for object in heap

 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Checks the value of the top object in a heap
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5HP_top(const H5HP_t *heap, int *val)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(heap);
    HDassert(val);

    /* Check internal consistency */
    /* (Pre-condition) */
    HDassert(heap->nobjs < heap->nalloc);
    HDassert(heap->heap);
    HDassert((heap->type == H5HP_MAX_HEAP && heap->heap[0].val == INT_MAX) ||
             (heap->type == H5HP_MIN_HEAP && heap->heap[0].val == INT_MIN));
    HDassert(heap->heap[0].obj == NULL);

    /* Get value of the top object in the heap */
    *val = heap->heap[1].val;

    /* No post-condition check necessary, since heap is constant */
    FUNC_LEAVE_NOAPI(SUCCEED);
} /* end H5HP_top() */

/*--------------------------------------------------------------------------
 NAME
    H5HP_remove
 PURPOSE
    Remove an object into a heap
 USAGE
    herr_t H5HP_remove(heap, val, obj)
        H5HP_t *heap;           IN/OUT: Pointer to heap to modify
        int *val;               OUT: Pointer to value of object removed from heap
        void **obj;             OUT: Pointer to object removed from heap

 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Removes the top object on a heap, returning its value and object pointer
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5HP_remove(H5HP_t *heap, int *val, void **obj)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(heap);
    HDassert(val);
    HDassert(obj);

    /* Check internal consistency */
    /* (Pre-condition) */
    HDassert(heap->nobjs < heap->nalloc);
    HDassert(heap->heap);
    HDassert((heap->type == H5HP_MAX_HEAP && heap->heap[0].val == INT_MAX) ||
             (heap->type == H5HP_MIN_HEAP && heap->heap[0].val == INT_MIN));
    HDassert(heap->heap[0].obj == NULL);

    /* Check if there are any objects on the heap to remove */
    if (heap->nobjs == 0)
        HGOTO_ERROR(H5E_HEAP, H5E_NOTFOUND, FAIL, "heap is empty");

    /* Get the information for the top object on the heap */
    HDassert(heap->heap[1].obj->heap_loc == 1);
    *val = heap->heap[1].val;
    *obj = heap->heap[1].obj;

    /* Move the last element in the heap to the top */
    heap->heap[1].val           = heap->heap[heap->nobjs].val;
    heap->heap[1].obj           = heap->heap[heap->nobjs].obj;
    heap->heap[1].obj->heap_loc = 1;

    /* Decrement number of objects in heap */
    heap->nobjs--;

    /* Restore heap condition, if there are objects on the heap */
    if (heap->nobjs > 0) {
        if (heap->type == H5HP_MAX_HEAP) {
            if (H5HP__sink_max(heap, (size_t)1) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTDELETE, FAIL, "unable to restore heap condition");
        } /* end if */
        else {
            if (H5HP__sink_min(heap, (size_t)1) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTDELETE, FAIL, "unable to restore heap condition");
        } /* end else */
    }     /* end if */

done:

    /* Check internal consistency */
    /* (Post-condition) */
    HDassert(heap->nobjs < heap->nalloc);
    HDassert(heap->heap);
    HDassert((heap->type == H5HP_MAX_HEAP && heap->heap[0].val == INT_MAX) ||
             (heap->type == H5HP_MIN_HEAP && heap->heap[0].val == INT_MIN));
    HDassert(heap->heap[0].obj == NULL);

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5HP_remove() */

/*--------------------------------------------------------------------------
 NAME
    H5HP_change
 PURPOSE
    Change the priority of an object on a heap
 USAGE
    herr_t H5HP_change(heap, val, obj)
        H5HP_t *heap;           IN/OUT: Pointer to heap to modify
        int val;                IN: New priority value for object
        void *obj;              IN: Pointer to object to modify

 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Changes the priority of an object on a heap.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5HP_change(H5HP_t *heap, int val, void *_obj)
{
    H5HP_info_t *obj = (H5HP_info_t *)_obj; /* Alias for object */
    size_t       obj_loc;                   /* Location of object in heap */
    int          old_val;                   /* Object's old priority value */
    herr_t       ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(heap);
    HDassert(obj);

    /* Check internal consistency */
    /* (Pre-condition) */
    HDassert(heap->nobjs < heap->nalloc);
    HDassert(heap->heap);
    HDassert((heap->type == H5HP_MAX_HEAP && heap->heap[0].val == INT_MAX) ||
             (heap->type == H5HP_MIN_HEAP && heap->heap[0].val == INT_MIN));
    HDassert(heap->heap[0].obj == NULL);

    /* Get the location of the object in the heap */
    obj_loc = obj->heap_loc;
    HDassert(obj_loc > 0 && obj_loc <= heap->nobjs);

    /* Change the heap object's priority */
    old_val                 = heap->heap[obj_loc].val;
    heap->heap[obj_loc].val = val;

    /* Restore heap condition */
    if (val < old_val) {
        if (heap->type == H5HP_MAX_HEAP) {
            if (H5HP__sink_max(heap, obj_loc) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTRESTORE, FAIL, "unable to restore heap condition");
        } /* end if */
        else {
            if (H5HP__swim_min(heap, obj_loc) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTRESTORE, FAIL, "unable to restore heap condition");
        } /* end else */
    }     /* end if */
    else {
        if (heap->type == H5HP_MAX_HEAP) {
            if (H5HP__swim_max(heap, obj_loc) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTRESTORE, FAIL, "unable to restore heap condition");
        } /* end if */
        else {
            if (H5HP__sink_min(heap, obj_loc) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTRESTORE, FAIL, "unable to restore heap condition");
        } /* end else */
    }     /* end else */

done:

    /* Check internal consistency */
    /* (Post-condition) */
    HDassert(heap->nobjs < heap->nalloc);
    HDassert(heap->heap);
    HDassert((heap->type == H5HP_MAX_HEAP && heap->heap[0].val == INT_MAX) ||
             (heap->type == H5HP_MIN_HEAP && heap->heap[0].val == INT_MIN));
    HDassert(heap->heap[0].obj == NULL);

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5HP_change() */

/*--------------------------------------------------------------------------
 NAME
    H5HP_incr
 PURPOSE
    Increment the priority of an object on a heap
 USAGE
    herr_t H5HP_incr(heap, amt, obj)
        H5HP_t *heap;           IN/OUT: Pointer to heap to modify
        unsigned amt;           IN: Amount to increase priority by
        void *obj;              IN: Pointer to object to modify

 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Increments the priority of an object on a heap by one.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5HP_incr(H5HP_t *heap, unsigned amt, void *_obj)
{
    H5HP_info_t *obj = (H5HP_info_t *)_obj; /* Alias for object */
    size_t       obj_loc;                   /* Location of object in heap */
    herr_t       ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(heap);
    HDassert(obj);

    /* Check internal consistency */
    /* (Pre-condition) */
    HDassert(heap->nobjs < heap->nalloc);
    HDassert(heap->heap);
    HDassert((heap->type == H5HP_MAX_HEAP && heap->heap[0].val == INT_MAX) ||
             (heap->type == H5HP_MIN_HEAP && heap->heap[0].val == INT_MIN));
    HDassert(heap->heap[0].obj == NULL);

    /* Get the location of the object in the heap */
    obj_loc = obj->heap_loc;
    HDassert(obj_loc > 0 && obj_loc <= heap->nobjs);

    /* Change the heap object's priority */
    heap->heap[obj_loc].val += (int)amt;

    /* Restore heap condition */
    if (H5HP_MAX_HEAP == heap->type) {
        if (H5HP__swim_max(heap, obj_loc) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTRESTORE, FAIL, "unable to restore heap condition")
    } /* end if */
    else {
        if (H5HP__sink_min(heap, obj_loc) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTRESTORE, FAIL, "unable to restore heap condition")
    } /* end else */

done:

    /* Check internal consistency */
    /* (Post-condition) */
    HDassert(heap->nobjs < heap->nalloc);
    HDassert(heap->heap);
    HDassert((heap->type == H5HP_MAX_HEAP && heap->heap[0].val == INT_MAX) ||
             (heap->type == H5HP_MIN_HEAP && heap->heap[0].val == INT_MIN));
    HDassert(heap->heap[0].obj == NULL);

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5HP_incr() */

/*--------------------------------------------------------------------------
 NAME
    H5HP_decr
 PURPOSE
    Decrement the priority of an object on a heap
 USAGE
    herr_t H5HP_dec(heap, amt, obj)
        H5HP_t *heap;           IN/OUT: Pointer to heap to modify
        unsigned amt;           IN: Amount to decrease priority by
        void *obj;              IN: Pointer to object to modify

 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Decrements the priority of an object on a heap by one.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5HP_decr(H5HP_t *heap, unsigned amt, void *_obj)
{
    H5HP_info_t *obj = (H5HP_info_t *)_obj; /* Alias for object */
    size_t       obj_loc;                   /* Location of object in heap */
    herr_t       ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(heap);
    HDassert(obj);

    /* Check internal consistency */
    /* (Pre-condition) */
    HDassert(heap->nobjs < heap->nalloc);
    HDassert(heap->heap);
    HDassert((heap->type == H5HP_MAX_HEAP && heap->heap[0].val == INT_MAX) ||
             (heap->type == H5HP_MIN_HEAP && heap->heap[0].val == INT_MIN));
    HDassert(heap->heap[0].obj == NULL);

    /* Get the location of the object in the heap */
    obj_loc = obj->heap_loc;
    HDassert(obj_loc > 0 && obj_loc <= heap->nobjs);

    /* Change the heap object's priority */
    H5_CHECK_OVERFLOW(amt, unsigned, int);
    heap->heap[obj_loc].val -= (int)amt;

    /* Restore heap condition */
    if (heap->type == H5HP_MAX_HEAP) {
        if (H5HP__sink_max(heap, obj_loc) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTRESTORE, FAIL, "unable to restore heap condition");
    } /* end if */
    else {
        if (H5HP__swim_min(heap, obj_loc) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTRESTORE, FAIL, "unable to restore heap condition");
    } /* end else */

done:

    /* Check internal consistency */
    /* (Post-condition) */
    HDassert(heap->nobjs < heap->nalloc);
    HDassert(heap->heap);
    HDassert((heap->type == H5HP_MAX_HEAP && heap->heap[0].val == INT_MAX) ||
             (heap->type == H5HP_MIN_HEAP && heap->heap[0].val == INT_MIN));
    HDassert(heap->heap[0].obj == NULL);

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5HP_decr() */

/*--------------------------------------------------------------------------
 NAME
    H5HP_close
 PURPOSE
    Close a heap, deallocating it.
 USAGE
    herr_t H5HP_close(heap)
        H5HP_t *heap;            IN/OUT: Pointer to heap to close

 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Close a heap, freeing all internal information.  Any objects left in
    the heap are not deallocated.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5HP_close(H5HP_t *heap)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(heap);

    /* Check internal consistency */
    /* (Pre-condition) */
    HDassert(heap->nobjs < heap->nalloc);
    HDassert(heap->heap);
    HDassert((heap->type == H5HP_MAX_HEAP && heap->heap[0].val == INT_MAX) ||
             (heap->type == H5HP_MIN_HEAP && heap->heap[0].val == INT_MIN));
    HDassert(NULL == heap->heap[0].obj);

    /* Free internal structures for heap */
    heap->heap = H5FL_SEQ_FREE(H5HP_ent_t, heap->heap);

    /* Free actual heap object */
    heap = H5FL_FREE(H5HP_t, heap);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5HP_close() */
