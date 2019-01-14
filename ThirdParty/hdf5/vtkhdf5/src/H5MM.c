/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:     H5MM.c
 *              Jul 10 1997
 *              Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:     Memory management functions
 *
 *-------------------------------------------------------------------------
 */


/****************/
/* Module Setup */
/****************/


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5MMprivate.h"	/* Memory management			*/


/****************/
/* Local Macros */
/****************/
#if defined H5_MEMORY_ALLOC_SANITY_CHECK
#define H5MM_SIG_SIZE           4
#define H5MM_HEAD_GUARD_SIZE    8
#define H5MM_TAIL_GUARD_SIZE    8
#define H5MM_BLOCK_FROM_BUF(mem) ((H5MM_block_t *)((unsigned char *)mem - (offsetof(H5MM_block_t, b) + H5MM_HEAD_GUARD_SIZE)))
#endif /* H5_MEMORY_ALLOC_SANITY_CHECK */


/******************/
/* Local Typedefs */
/******************/

#if defined H5_MEMORY_ALLOC_SANITY_CHECK
/* Memory allocation "block", wrapped around each allocation */
struct H5MM_block_t;    /* Forward declaration for typedef */
typedef struct H5MM_block_t {
    unsigned char sig[H5MM_SIG_SIZE];   /* Signature for the block, to indicate it was allocated with H5MM* interface */
    struct H5MM_block_t *next;          /* Pointer to next block in the list of allocated blocks */
    struct H5MM_block_t *prev;          /* Pointer to previous block in the list of allocated blocks */
    union {
        struct {
            size_t size;                /* Size of allocated block */
            hbool_t in_use;             /* Whether the block is in use or is free */
        } info;
        double _align;                  /* Align following buffer (b) to double boundary (unused) */
    } u;
    unsigned char b[];                  /* Buffer for caller (includes header and footer) */
} H5MM_block_t;
#endif /* H5_MEMORY_ALLOC_SANITY_CHECK */


/********************/
/* Local Prototypes */
/********************/
#if defined H5_MEMORY_ALLOC_SANITY_CHECK
static hbool_t H5MM__is_our_block(void *mem);
static void H5MM__sanity_check_block(const H5MM_block_t *block);
static void H5MM__sanity_check(void *mem);
#endif /* H5_MEMORY_ALLOC_SANITY_CHECK */


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/

#if defined H5_MEMORY_ALLOC_SANITY_CHECK
/* Constant strings for block signature, head & tail guards */
static const char H5MM_block_signature_s[H5MM_SIG_SIZE] = {'H', '5', 'M', 'M'};
static const char H5MM_block_head_guard_s[H5MM_HEAD_GUARD_SIZE] = {'D', 'E', 'A', 'D', 'B', 'E', 'E', 'F'};
static const char H5MM_block_tail_guard_s[H5MM_TAIL_GUARD_SIZE] = {'B', 'E', 'E', 'F', 'D', 'E', 'A', 'D'};

/* Flag to indicate the the interface has been initialized */
static hbool_t H5MM_init_s = FALSE;

/* Head of the list of allocated blocks */
static H5MM_block_t H5MM_block_head_s;

/* Statistics about block allocations */
static unsigned long long H5MM_total_alloc_bytes_s = 0;
static unsigned long long H5MM_curr_alloc_bytes_s = 0;
static unsigned long long H5MM_peak_alloc_bytes_s = 0;
static size_t H5MM_max_block_size_s = 0;
static size_t H5MM_total_alloc_blocks_count_s = 0;
static size_t H5MM_curr_alloc_blocks_count_s = 0;
static size_t H5MM_peak_alloc_blocks_count_s = 0;
#endif /* H5_MEMORY_ALLOC_SANITY_CHECK */


#if defined H5_MEMORY_ALLOC_SANITY_CHECK

/*-------------------------------------------------------------------------
 * Function:    H5MM__is_our_block
 *
 * Purpose:     Try to determine if a memory buffer has been allocated through
 *              the H5MM* interface, instead of the system's malloc() routines.
 *
 * Return:      Success:    TRUE/FALSE
 *              Failure:    (Can't fail)
 *
 * Programmer:  Quincey Koziol
 *              Dec 30 2015
 *
 *-------------------------------------------------------------------------
 */
static hbool_t
H5MM__is_our_block(void *mem)
{
    H5MM_block_t *block = H5MM_BLOCK_FROM_BUF(mem);

    return(0 == HDmemcmp(block->sig, H5MM_block_signature_s, H5MM_SIG_SIZE));
}


/*-------------------------------------------------------------------------
 * Function:    H5MM__sanity_check_block
 *
 * Purpose:     Check a block wrapper around a buffer to validate it.
 *
 * Return:      N/A (void)
 *
 * Programmer:  Quincey Koziol
 *              Dec 30 2015
 *
 *-------------------------------------------------------------------------
 */
H5_ATTR_PURE static void
H5MM__sanity_check_block(const H5MM_block_t *block)
{
    HDassert(block->u.info.size > 0);
    HDassert(block->u.info.in_use);
    /* Check for head & tail guards, if not head of linked list */
    if(block->u.info.size != SIZET_MAX) {
        HDassert(0 == HDmemcmp(block->b, H5MM_block_head_guard_s, H5MM_HEAD_GUARD_SIZE));
        HDassert(0 == HDmemcmp(block->b + H5MM_HEAD_GUARD_SIZE + block->u.info.size, H5MM_block_tail_guard_s, H5MM_TAIL_GUARD_SIZE));
    }
}


/*-------------------------------------------------------------------------
 * Function:    H5MM__sanity_check
 *
 * Purpose:     Check a buffer to validate it (just calls
 *              H5MM__sanity_check_block after finding block for buffer)
 *
 * Return:      N/A (void)
 *
 * Programmer:  Quincey Koziol
 *              Dec 30 2015
 *
 *-------------------------------------------------------------------------
 */
H5_ATTR_PURE static void
H5MM__sanity_check(void *mem)
{
    H5MM_block_t *block = H5MM_BLOCK_FROM_BUF(mem);

    H5MM__sanity_check_block(block);
}


/*-------------------------------------------------------------------------
 * Function:    H5MM_sanity_check_all
 *
 * Purpose:     Sanity check all current memory allocations.
 *
 * Return:      N/A (void)
 *
 * Programmer:  Quincey Koziol
 *              Jan  5 2016
 *
 *-------------------------------------------------------------------------
 */
H5_ATTR_PURE void
H5MM_sanity_check_all(void)
{
    H5MM_block_t *curr = NULL;

    curr = H5MM_block_head_s.next;
    while(curr != &H5MM_block_head_s) {
        H5MM__sanity_check_block(curr);
        curr = curr->next;
    } /* end while */
} /* end H5MM_sanity_check_all() */


/*-------------------------------------------------------------------------
 * Function:    H5MM_final_sanity_check
 *
 * Purpose:     Final sanity checks on memory allocation.
 *
 * Return:      N/A (void)
 *
 * Programmer:  Quincey Koziol
 *              Jan  1 2016
 *
 *-------------------------------------------------------------------------
 */
H5_ATTR_PURE void
H5MM_final_sanity_check(void)
{
    HDassert(0 == H5MM_curr_alloc_bytes_s);
    HDassert(0 == H5MM_curr_alloc_blocks_count_s);
    HDassert(H5MM_block_head_s.next == &H5MM_block_head_s);
    HDassert(H5MM_block_head_s.prev == &H5MM_block_head_s);
#ifdef H5MM_PRINT_MEMORY_STATS
    HDfprintf(stderr, "%s: H5MM_total_alloc_bytes_s = %llu\n", __func__, H5MM_total_alloc_bytes_s);
    HDfprintf(stderr, "%s: H5MM_peak_alloc_bytes_s = %llu\n", __func__, H5MM_peak_alloc_bytes_s);
    HDfprintf(stderr, "%s: H5MM_max_block_size_s = %zu\n", __func__, H5MM_max_block_size_s);
    HDfprintf(stderr, "%s: H5MM_total_alloc_blocks_count_s = %zu\n", __func__, H5MM_total_alloc_blocks_count_s);
    HDfprintf(stderr, "%s: H5MM_peak_alloc_blocks_count_s = %zu\n", __func__, H5MM_peak_alloc_blocks_count_s);
#endif /* H5MM_PRINT_MEMORY_STATS */
}
#endif /* H5_MEMORY_ALLOC_SANITY_CHECK */


/*-------------------------------------------------------------------------
 * Function:    H5MM_malloc
 *
 * Purpose:     Similar to the C89 version of malloc().
 *
 *              On size of 0, we return a NULL pointer instead of the
 *              standard-allowed 'special' pointer since that's more
 *              difficult to check as a return value. This is still
 *              considered an error condition since allocations of zero
 *              bytes usually indicate problems.
 *  
 * Return:      Success:    Pointer to new memory
 *              Failure:    NULL
 *
 * Programmer:  Quincey Koziol
 *              Nov  8 2003
 *
 *-------------------------------------------------------------------------
 */
void *
H5MM_malloc(size_t size)
{
    void *ret_value = NULL;

    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

#if defined H5_MEMORY_ALLOC_SANITY_CHECK
    /* Initialize block list head singleton */
    if(!H5MM_init_s) {
        HDmemcpy(H5MM_block_head_s.sig, H5MM_block_signature_s, H5MM_SIG_SIZE);
        H5MM_block_head_s.next = &H5MM_block_head_s;
        H5MM_block_head_s.prev = &H5MM_block_head_s;
        H5MM_block_head_s.u.info.size = SIZET_MAX;
        H5MM_block_head_s.u.info.in_use = TRUE;

        H5MM_init_s = TRUE;
    } /* end if */
#endif /* H5_MEMORY_ALLOC_SANITY_CHECK */

    if(size) {
#if defined H5_MEMORY_ALLOC_SANITY_CHECK
        H5MM_block_t *block;
        size_t alloc_size = sizeof(H5MM_block_t) + size + H5MM_HEAD_GUARD_SIZE + H5MM_TAIL_GUARD_SIZE;

        if(NULL != (block = (H5MM_block_t *)HDmalloc(alloc_size))) {
            /* Set up block */
            HDmemcpy(block->sig, H5MM_block_signature_s, H5MM_SIG_SIZE);
            block->next = H5MM_block_head_s.next;
            H5MM_block_head_s.next = block;
            block->next->prev = block;
            block->prev = &H5MM_block_head_s;
            block->u.info.size = size;
            block->u.info.in_use = TRUE;
            HDmemcpy(block->b, H5MM_block_head_guard_s, H5MM_HEAD_GUARD_SIZE);
            HDmemcpy(block->b + H5MM_HEAD_GUARD_SIZE + size, H5MM_block_tail_guard_s, H5MM_TAIL_GUARD_SIZE);

            /* Update statistics */
            H5MM_total_alloc_bytes_s += size;
            H5MM_curr_alloc_bytes_s += size;
            if(H5MM_curr_alloc_bytes_s > H5MM_peak_alloc_bytes_s)
                H5MM_peak_alloc_bytes_s = H5MM_curr_alloc_bytes_s;
            if(size > H5MM_max_block_size_s)
                H5MM_max_block_size_s = size;
            H5MM_total_alloc_blocks_count_s++;
            H5MM_curr_alloc_blocks_count_s++;
            if(H5MM_curr_alloc_blocks_count_s > H5MM_peak_alloc_blocks_count_s)
                H5MM_peak_alloc_blocks_count_s = H5MM_curr_alloc_blocks_count_s;

            /* Set buffer to return */
            ret_value = block->b + H5MM_HEAD_GUARD_SIZE;
        } /* end if */
        else
            ret_value = NULL;
#else /* H5_MEMORY_ALLOC_SANITY_CHECK */
        ret_value = HDmalloc(size);
#endif /* H5_MEMORY_ALLOC_SANITY_CHECK */
    } /* end if */
    else
        ret_value = NULL;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MM_malloc() */


/*-------------------------------------------------------------------------
 * Function:    H5MM_calloc
 *
 * Purpose:     Similar to the C89 version of calloc(), except this
 *              routine just takes a 'size' parameter.
 *
 *              On size of 0, we return a NULL pointer instead of the
 *              standard-allowed 'special' pointer since that's more
 *              difficult to check as a return value. This is still
 *              considered an error condition since allocations of zero
 *              bytes usually indicate problems.
 *
 *
 * Return:      Success:    Pointer to new memory
 *              Failure:    NULL
 *
 * Programmer:	Quincey Koziol
 *              Nov  8 2003
 *
 *-------------------------------------------------------------------------
 */
void *
H5MM_calloc(size_t size)
{
    void *ret_value = NULL;

    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(size) {
#if defined H5_MEMORY_ALLOC_SANITY_CHECK
        if(NULL != (ret_value = H5MM_malloc(size)))
            HDmemset(ret_value, 0, size);
#else /* H5_MEMORY_ALLOC_SANITY_CHECK */
        ret_value = HDcalloc((size_t)1, size);
#endif /* H5_MEMORY_ALLOC_SANITY_CHECK */
    } /* end if */
    else
        ret_value = NULL;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MM_calloc() */


/*-------------------------------------------------------------------------
 * Function:    H5MM_realloc
 *
 * Purpose:     Similar semantics as C89's realloc(). Specifically, the
 *              following calls are equivalent:
 *
 *              H5MM_realloc(NULL, size)    <==> H5MM_malloc(size)
 *              H5MM_realloc(ptr, 0)        <==> H5MM_xfree(ptr)
 *              H5MM_realloc(NULL, 0)       <==> NULL
 *
 *              Note that the (NULL, 0) combination is undefined behavior
 *              in the C standard.
 *
 * Return:      Success:    Ptr to new memory if size > 0
 *                          NULL if size is zero
 *              Failure:    NULL (input buffer is unchanged on failure)
 *
 * Programmer:  Robb Matzke
 *              Jul 10 1997
 *
 *-------------------------------------------------------------------------
 */
void *
H5MM_realloc(void *mem, size_t size)
{
    void *ret_value = NULL;

    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(NULL == mem && 0 == size)
        /* Not defined in the standard, return NULL */
        ret_value = NULL;
    else {
#if defined H5_MEMORY_ALLOC_SANITY_CHECK
        if(size > 0) {
            if(mem) {
                if(H5MM__is_our_block(mem)) {
                    H5MM_block_t *block = H5MM_BLOCK_FROM_BUF(mem);
                    size_t old_size = block->u.info.size;

                    H5MM__sanity_check(mem);

                    ret_value = H5MM_malloc(size);
                    HDmemcpy(ret_value, mem, MIN(size, old_size));
                    H5MM_xfree(mem);
                } /* end if */
                else
                    ret_value = HDrealloc(mem, size);
            }
            else
                ret_value = H5MM_malloc(size);
        }
        else
            ret_value = H5MM_xfree(mem);
#else /* H5_MEMORY_ALLOC_SANITY_CHECK */
        ret_value = HDrealloc(mem, size);

        /* Some platforms do not return NULL if size is zero. */
        if(0 == size)
            ret_value = NULL;
#endif /* H5_MEMORY_ALLOC_SANITY_CHECK */
    } /* end else */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MM_realloc() */


/*-------------------------------------------------------------------------
 * Function:    H5MM_xstrdup
 *
 * Purpose:     Duplicates a string, including memory allocation.
 *              NULL is an acceptable value for the input string.
 *
 * Return:      Success:    Pointer to a new string (NULL if s is NULL).
 *              Failure:    NULL
 *
 * Programmer:  Robb Matzke
 *              Jul 10 1997
 *-------------------------------------------------------------------------
 */
char *
H5MM_xstrdup(const char *s)
{
    char    *ret_value = NULL;

    FUNC_ENTER_NOAPI(NULL)

    if(s) {
        if(NULL == (ret_value = (char *)H5MM_malloc(HDstrlen(s) + 1)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
        HDstrcpy(ret_value, s);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MM_xstrdup() */


/*-------------------------------------------------------------------------
 * Function:    H5MM_strdup
 *
 * Purpose:     Duplicates a string, including memory allocation.
 *              NULL is NOT an acceptable value for the input string.
 *
 *              If the string to be duplicated is the NULL pointer, then
 *              an error will be raised.
 *
 * Return:      Success:    Pointer to a new string
 *              Failure:    NULL
 *
 * Programmer:  Robb Matzke
 *              Jul 10 1997
 *-------------------------------------------------------------------------
 */
char *
H5MM_strdup(const char *s)
{
    char *ret_value = NULL;

    FUNC_ENTER_NOAPI(NULL)

    if(!s)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "null string")
    if(NULL == (ret_value = (char *)H5MM_malloc(HDstrlen(s) + 1)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
    HDstrcpy(ret_value, s);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MM_strdup() */


/*-------------------------------------------------------------------------
 * Function:    H5MM_xfree
 *
 * Purpose:     Just like free(3) except null pointers are allowed as
 *              arguments, and the return value (always NULL) can be
 *              assigned to the pointer whose memory was just freed:
 *
 *              thing = H5MM_xfree (thing);
 *
 * Return:      Success:    NULL
 *              Failure:    never fails
 *
 * Programmer:  Robb Matzke
 *              Jul 10 1997
 *
 *-------------------------------------------------------------------------
 */
void *
H5MM_xfree(void *mem)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(mem) {
#if defined H5_MEMORY_ALLOC_SANITY_CHECK
        if(H5MM__is_our_block(mem)) {
            H5MM_block_t *block = H5MM_BLOCK_FROM_BUF(mem);

            /* Run sanity checks on this block and its neighbors */
            H5MM__sanity_check(mem);
            H5MM__sanity_check_block(block->next);
            H5MM__sanity_check_block(block->prev);

            /* Update statistics */
            H5MM_curr_alloc_bytes_s -= block->u.info.size;
            H5MM_curr_alloc_blocks_count_s--;

            /* Reset block info */
            HDmemset(block->sig, 0, H5MM_SIG_SIZE);
            block->next->prev = block->prev;
            block->prev->next = block->next;
            block->next = NULL;
            block->prev = NULL;
            block->u.info.in_use = FALSE;

            /* Free the block (finally!) */
            HDfree(block);
        }
        else
            HDfree(mem);
#else /* H5_MEMORY_ALLOC_SANITY_CHECK */
        HDfree(mem);
#endif /* H5_MEMORY_ALLOC_SANITY_CHECK */
    } /* end if */

    FUNC_LEAVE_NOAPI(NULL)
} /* end H5MM_xfree() */

