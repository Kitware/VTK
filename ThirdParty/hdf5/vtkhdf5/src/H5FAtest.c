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

/*
 * Purpose:     Fixed array testing functions.
 *
 */

/**********************/
/* Module Declaration */
/**********************/

#include "H5FAmodule.h"         /* This source code file is part of the H5FA module */
#define H5FA_TESTING


/***********************/
/* Other Packages Used */
/***********************/


/***********/
/* Headers */
/***********/
#include "H5private.h"        /* Generic Functions            */
#include "H5Eprivate.h"        /* Error handling              */
#include "H5FApkg.h"        /* Fixed Arrays                */
#include "H5FLprivate.h"    /* Free Lists                           */
#include "H5VMprivate.h"         /* Vector functions            */


/****************/
/* Local Macros */
/****************/

/* Sanity checking value for callback contexts */
#define H5FA__TEST_BOGUS_VAL    42


/******************/
/* Local Typedefs */
/******************/

/* Callback context */
typedef struct H5FA__test_ctx_t {
    uint32_t    bogus;          /* Placeholder field to verify that context is working */
} H5FA__test_ctx_t;


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

/* Fixed array class callbacks */
static void *H5FA__test_crt_context(void *udata);
static herr_t H5FA__test_dst_context(void *ctx);
static herr_t H5FA__test_fill(void *nat_blk, size_t nelmts);
static herr_t H5FA__test_encode(void *raw, const void *elmt, size_t nelmts,
    void *ctx);
static herr_t H5FA__test_decode(const void *raw, void *elmt, size_t nelmts,
    void *ctx);
static herr_t H5FA__test_debug(FILE *stream, int indent, int fwidth,
    hsize_t idx, const void *elmt);
static void *H5FA__test_crt_dbg_context(H5F_t *f, haddr_t obj_addr);


/*********************/
/* Package Variables */
/*********************/

/* Fixed array testing class information */
const H5FA_class_t H5FA_CLS_TEST[1]={{
    H5FA_CLS_TEST_ID,           /* Type of Fixed array */
    "Testing",                  /* Name of fixed array class */
    sizeof(uint64_t),           /* Size of native element */
    H5FA__test_crt_context,     /* Create context */
    H5FA__test_dst_context,     /* Destroy context */
    H5FA__test_fill,            /* Fill block of missing elements callback */
    H5FA__test_encode,          /* Element encoding callback */
    H5FA__test_decode,          /* Element decoding callback */
    H5FA__test_debug,           /* Element debugging callback */
    H5FA__test_crt_dbg_context, /* Create debugging context */
    H5FA__test_dst_context      /* Destroy debugging context */
}};


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/

/* Declare a free list to manage the H5FA__test_ctx_t struct */
H5FL_DEFINE_STATIC(H5FA__test_ctx_t);



/*-------------------------------------------------------------------------
 * Function:    H5FA__test_crt_context
 *
 * Purpose:     Create context for callbacks
 *
 * Return:      Success:    non-NULL
 *              Failure:    NULL
 *
 * Programmer:  Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(STATIC, ERR,
void *, NULL, NULL,
H5FA__test_crt_context(void H5_ATTR_UNUSED *udata))

    /* Local variables */
    H5FA__test_ctx_t *ctx;              /* Context for callbacks */

    /* Allocate new context structure */
    if(NULL == (ctx = H5FL_MALLOC(H5FA__test_ctx_t)))
        H5E_THROW(H5E_CANTALLOC, "can't allocate fixed array client callback context")

    /* Initialize the context */
    ctx->bogus = H5FA__TEST_BOGUS_VAL;

    /* Set return value */
    ret_value = ctx;

CATCH

END_FUNC(STATIC)  /* end H5FA__test_crt_context() */


/*-------------------------------------------------------------------------
 * Function:    H5FA__test_dst_context
 *
 * Purpose:     Destroy context for callbacks
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(STATIC, NOERR,
herr_t, SUCCEED, -,
H5FA__test_dst_context(void *_ctx))

    /* Local variables */
    H5FA__test_ctx_t *ctx = (H5FA__test_ctx_t *)_ctx;   /* Callback context to destroy */

    /* Sanity checks */
    HDassert(H5FA__TEST_BOGUS_VAL == ctx->bogus);

    /* Release context structure */
    ctx = H5FL_FREE(H5FA__test_ctx_t, ctx);

END_FUNC(STATIC)  /* end H5FA__test_dst_context() */


/*-------------------------------------------------------------------------
 * Function:    H5FA__test_fill
 *
 * Purpose:     Fill "missing elements" in block of elements
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(STATIC, NOERR,
herr_t, SUCCEED, -,
H5FA__test_fill(void *nat_blk, size_t nelmts))

    /* Local variables */
    uint64_t fill_val = H5FA_TEST_FILL;          /* Value to fill elements with */

    /* Sanity checks */
    HDassert(nat_blk);
    HDassert(nelmts);

    H5VM_array_fill(nat_blk, &fill_val, sizeof(uint64_t), nelmts);

END_FUNC(STATIC)  /* end H5FA__test_fill() */


/*-------------------------------------------------------------------------
 * Function:    H5FA__test_encode
 *
 * Purpose:     Encode an element from "native" to "raw" form
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(STATIC, NOERR,
herr_t, SUCCEED, -,
H5FA__test_encode(void *raw, const void *_elmt, size_t nelmts, void H5_ATTR_UNUSED *_ctx))

    /* Local variables */
#ifndef NDEBUG
    H5FA__test_ctx_t *ctx = (H5FA__test_ctx_t *)_ctx;   /* Callback context to destroy */
#endif /* NDEBUG */
    const uint64_t *elmt = (const uint64_t *)_elmt;     /* Convenience pointer to native elements */

    /* Sanity checks */
    HDassert(raw);
    HDassert(elmt);
    HDassert(nelmts);
    HDassert(H5FA__TEST_BOGUS_VAL == ctx->bogus);

    /* Encode native elements into raw elements */
    while(nelmts) {
        /* Encode element */
        /* (advances 'raw' pointer) */
        UINT64ENCODE(raw, *elmt);

        /* Advance native element pointer */
        elmt++;

        /* Decrement # of elements to encode */
        nelmts--;
    } /* end while */

END_FUNC(STATIC)  /* end H5FA__test_encode() */


/*-------------------------------------------------------------------------
 * Function:    H5FA__test_decode
 *
 * Purpose:     Decode an element from "raw" to "native" form
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(STATIC, NOERR,
herr_t, SUCCEED, -,
H5FA__test_decode(const void *_raw, void *_elmt, size_t nelmts, void H5_ATTR_UNUSED *_ctx))

    /* Local variables */
#ifndef NDEBUG
    H5FA__test_ctx_t *ctx = (H5FA__test_ctx_t *)_ctx;   /* Callback context to destroy */
#endif /* NDEBUG */
    uint64_t *elmt = (uint64_t *)_elmt;     /* Convenience pointer to native elements */
    const uint8_t *raw = (const uint8_t *)_raw; /* Convenience pointer to raw elements */

    /* Sanity checks */
    HDassert(raw);
    HDassert(elmt);
    HDassert(nelmts);
    HDassert(H5FA__TEST_BOGUS_VAL == ctx->bogus);

    /* Decode raw elements into native elements */
    while(nelmts) {
        /* Decode element */
        /* (advances 'raw' pointer) */
        UINT64DECODE(raw, *elmt);

        /* Advance native element pointer */
        elmt++;

        /* Decrement # of elements to decode */
        nelmts--;
    } /* end while */

END_FUNC(STATIC)  /* end H5FA__test_decode() */


/*-------------------------------------------------------------------------
 * Function:    H5FA__test_debug
 *
 * Purpose:     Display an element for debugging
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(STATIC, NOERR,
herr_t, SUCCEED, -,
H5FA__test_debug(FILE *stream, int indent, int fwidth, hsize_t idx,
    const void *elmt))

    /* Local variables */
    char temp_str[128];     /* Temporary string, for formatting */

    /* Sanity checks */
    HDassert(stream);
    HDassert(elmt);

    /* Print element */
    HDsprintf(temp_str, "Element #%llu:", (unsigned long long)idx);
    HDfprintf(stream, "%*s%-*s %llu\n", indent, "", fwidth, temp_str,
        (unsigned long long)*(const uint64_t *)elmt);

END_FUNC(STATIC)  /* end H5FA__test_debug() */


/*-------------------------------------------------------------------------
 * Function:    H5FA__test_crt_dbg_context
 *
 * Purpose:     Create context for debugging callback
 *
 * Return:      Success:    non-NULL
 *              Failure:    NULL
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, December 1, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(STATIC, ERR,
void *, NULL, NULL,
H5FA__test_crt_dbg_context(H5F_t H5_ATTR_UNUSED *f, haddr_t H5_ATTR_UNUSED obj_addr))

    /* Local variables */
    H5FA__test_ctx_t *ctx;              /* Context for callbacks */

    /* Allocate new context structure */
    if(NULL == (ctx = H5FL_MALLOC(H5FA__test_ctx_t)))
        H5E_THROW(H5E_CANTALLOC, "can't allocate fixed array client callback context")

    /* Initialize the context */
    ctx->bogus = H5FA__TEST_BOGUS_VAL;

    /* Set return value */
    ret_value = ctx;

CATCH

END_FUNC(STATIC)  /* end H5FA__test_crt_dbg_context() */


/*-------------------------------------------------------------------------
 * Function:    H5FA_get_cparam_test
 *
 * Purpose:     Retrieve the parameters used to create the fixed array
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PRIV, NOERR,
herr_t, SUCCEED, -,
H5FA_get_cparam_test(const H5FA_t *fa, H5FA_create_t *cparam))

    /* Check arguments. */
    HDassert(fa);
    HDassert(cparam);

    /* Get fixed array creation parameters */
    cparam->raw_elmt_size = fa->hdr->cparam.raw_elmt_size;
    cparam->nelmts = fa->hdr->cparam.nelmts;

END_FUNC(PRIV)  /* end H5FA_get_cparam_test() */


/*-------------------------------------------------------------------------
 * Function:    H5FA_cmp_cparam_test
 *
 * Purpose:     Compare the parameters used to create the fixed array
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PRIV, ERRCATCH,
int, 0, -,
H5FA_cmp_cparam_test(const H5FA_create_t *cparam1, const H5FA_create_t *cparam2))

    /* Check arguments. */
    HDassert(cparam1);
    HDassert(cparam2);

    /* Compare creation parameters for array */
    if(cparam1->raw_elmt_size < cparam2->raw_elmt_size)
        H5_LEAVE(-1)
    else if(cparam1->raw_elmt_size > cparam2->raw_elmt_size)
        H5_LEAVE(1)

CATCH

END_FUNC(PRIV)  /* end H5FA_cmp_cparam_test() */

