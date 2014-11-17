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
 * Created:		H5HFbtree2.c
 *			Aug  7 2006
 *			Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:		v2 B-tree callbacks for "huge" object tracker
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5HF_PACKAGE		/*suppress error about including H5HFpkg  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5HFpkg.h"		/* Fractal heaps			*/
#include "H5MFprivate.h"	/* File memory management		*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/

/* v2 B-tree client callback context */
typedef struct H5HF_huge_bt2_ctx_t {
    uint8_t     sizeof_size;    /* Size of file sizes */
    uint8_t     sizeof_addr;    /* Size of file addresses */
} H5HF_huge_bt2_ctx_t;


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/


/* v2 B-tree driver callbacks */

static void *H5HF_huge_bt2_crt_context(void *udata);
static herr_t H5HF_huge_bt2_dst_context(void *ctx);
static void *H5HF_huge_bt2_crt_dbg_context(H5F_t *f, hid_t dxpl_id, haddr_t addr);

static herr_t H5HF_huge_bt2_indir_store(void *native, const void *udata);
static herr_t H5HF_huge_bt2_indir_compare(const void *rec1, const void *rec2);
static herr_t H5HF_huge_bt2_indir_encode(uint8_t *raw, const void *native,
    void *ctx);
static herr_t H5HF_huge_bt2_indir_decode(const uint8_t *raw, void *native,
    void *ctx);
static herr_t H5HF_huge_bt2_indir_debug(FILE *stream, const H5F_t *f, hid_t dxpl_id,
    int indent, int fwidth, const void *record, const void *_udata);

static herr_t H5HF_huge_bt2_filt_indir_store(void *native, const void *udata);
static herr_t H5HF_huge_bt2_filt_indir_compare(const void *rec1, const void *rec2);
static herr_t H5HF_huge_bt2_filt_indir_encode(uint8_t *raw, const void *native,
    void *ctx);
static herr_t H5HF_huge_bt2_filt_indir_decode(const uint8_t *raw, void *native,
    void *ctx);
static herr_t H5HF_huge_bt2_filt_indir_debug(FILE *stream, const H5F_t *f, hid_t dxpl_id,
    int indent, int fwidth, const void *record, const void *_udata);

static herr_t H5HF_huge_bt2_dir_store(void *native, const void *udata);
static herr_t H5HF_huge_bt2_dir_compare(const void *rec1, const void *rec2);
static herr_t H5HF_huge_bt2_dir_encode(uint8_t *raw, const void *native,
    void *ctx);
static herr_t H5HF_huge_bt2_dir_decode(const uint8_t *raw, void *native,
    void *ctx);
static herr_t H5HF_huge_bt2_dir_debug(FILE *stream, const H5F_t *f, hid_t dxpl_id,
    int indent, int fwidth, const void *record, const void *_udata);

static herr_t H5HF_huge_bt2_filt_dir_store(void *native, const void *udata);
static herr_t H5HF_huge_bt2_filt_dir_compare(const void *rec1, const void *rec2);
static herr_t H5HF_huge_bt2_filt_dir_encode(uint8_t *raw, const void *native,
    void *ctx);
static herr_t H5HF_huge_bt2_filt_dir_decode(const uint8_t *raw, void *native,
    void *ctx);
static herr_t H5HF_huge_bt2_filt_dir_debug(FILE *stream, const H5F_t *f, hid_t dxpl_id,
    int indent, int fwidth, const void *record, const void *_udata);

/*********************/
/* Package Variables */
/*********************/
/* v2 B-tree class for indirectly accessed 'huge' objects */
const H5B2_class_t H5HF_HUGE_BT2_INDIR[1]={{ /* B-tree class information */
    H5B2_FHEAP_HUGE_INDIR_ID,               /* Type of B-tree */
    "H5B2_FHEAP_HUGE_INDIR_ID",             /* Name of B-tree class */
    sizeof(H5HF_huge_bt2_indir_rec_t),      /* Size of native record */
    H5HF_huge_bt2_crt_context,              /* Create client callback context */
    H5HF_huge_bt2_dst_context,              /* Destroy client callback context */
    H5HF_huge_bt2_indir_store,              /* Record storage callback */
    H5HF_huge_bt2_indir_compare,            /* Record comparison callback */
    H5HF_huge_bt2_indir_encode,             /* Record encoding callback */
    H5HF_huge_bt2_indir_decode,             /* Record decoding callback */
    H5HF_huge_bt2_indir_debug,              /* Record debugging callback */
    H5HF_huge_bt2_crt_dbg_context,          /* Create debugging context */
    H5HF_huge_bt2_dst_context               /* Destroy debugging context */
}};

/* v2 B-tree class for indirectly accessed, filtered 'huge' objects */
const H5B2_class_t H5HF_HUGE_BT2_FILT_INDIR[1]={{ /* B-tree class information */
    H5B2_FHEAP_HUGE_FILT_INDIR_ID,          /* Type of B-tree */
    "H5B2_FHEAP_HUGE_FILT_INDIR_ID",        /* Name of B-tree class */
    sizeof(H5HF_huge_bt2_filt_indir_rec_t), /* Size of native record */
    H5HF_huge_bt2_crt_context,              /* Create client callback context */
    H5HF_huge_bt2_dst_context,              /* Destroy client callback context */
    H5HF_huge_bt2_filt_indir_store,         /* Record storage callback */
    H5HF_huge_bt2_filt_indir_compare,       /* Record comparison callback */
    H5HF_huge_bt2_filt_indir_encode,        /* Record encoding callback */
    H5HF_huge_bt2_filt_indir_decode,        /* Record decoding callback */
    H5HF_huge_bt2_filt_indir_debug,         /* Record debugging callback */
    H5HF_huge_bt2_crt_dbg_context,          /* Create debugging context */
    H5HF_huge_bt2_dst_context               /* Destroy debugging context */
}};

/* v2 B-tree class for directly accessed 'huge' objects */
const H5B2_class_t H5HF_HUGE_BT2_DIR[1]={{  /* B-tree class information */
    H5B2_FHEAP_HUGE_DIR_ID,                 /* Type of B-tree */
    "H5B2_FHEAP_HUGE_DIR_ID",               /* Name of B-tree class */
    sizeof(H5HF_huge_bt2_dir_rec_t),        /* Size of native record */
    H5HF_huge_bt2_crt_context,              /* Create client callback context */
    H5HF_huge_bt2_dst_context,              /* Destroy client callback context */
    H5HF_huge_bt2_dir_store,                /* Record storage callback */
    H5HF_huge_bt2_dir_compare,              /* Record comparison callback */
    H5HF_huge_bt2_dir_encode,               /* Record encoding callback */
    H5HF_huge_bt2_dir_decode,               /* Record decoding callback */
    H5HF_huge_bt2_dir_debug,                /* Record debugging callback */
    H5HF_huge_bt2_crt_dbg_context,          /* Create debugging context */
    H5HF_huge_bt2_dst_context               /* Destroy debugging context */
}};

/* v2 B-tree class for directly accessed, filtered 'huge' objects */
const H5B2_class_t H5HF_HUGE_BT2_FILT_DIR[1]={{ /* B-tree class information */
    H5B2_FHEAP_HUGE_FILT_DIR_ID,            /* Type of B-tree */
    "H5B2_FHEAP_HUGE_FILT_DIR_ID",          /* Name of B-tree class */
    sizeof(H5HF_huge_bt2_filt_dir_rec_t),   /* Size of native record */
    H5HF_huge_bt2_crt_context,              /* Create client callback context */
    H5HF_huge_bt2_dst_context,              /* Destroy client callback context */
    H5HF_huge_bt2_filt_dir_store,           /* Record storage callback */
    H5HF_huge_bt2_filt_dir_compare,         /* Record comparison callback */
    H5HF_huge_bt2_filt_dir_encode,          /* Record encoding callback */
    H5HF_huge_bt2_filt_dir_decode,          /* Record decoding callback */
    H5HF_huge_bt2_filt_dir_debug,           /* Record debugging callback */
    H5HF_huge_bt2_crt_dbg_context,          /* Create debugging context */
    H5HF_huge_bt2_dst_context               /* Destroy debugging context */
}};

/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/

/* Declare a free list to manage the H5HF_huge_bt2_ctx_t struct */
H5FL_DEFINE_STATIC(H5HF_huge_bt2_ctx_t);



/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_crt_context
 *
 * Purpose:	Create client callback context
 *
 * Note:	Common to all 'huge' v2 B-tree clients
 *
 * Return:	Success:	non-NULL
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Thursday, November 26, 2009
 *
 *-------------------------------------------------------------------------
 */
static void *
H5HF_huge_bt2_crt_context(void *_f)
{
    H5F_t *f = (H5F_t *)_f;     /* User data for building callback context */
    H5HF_huge_bt2_ctx_t *ctx;   /* Callback context structure */
    void *ret_value;            /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(f);

    /* Allocate callback context */
    if(NULL == (ctx = H5FL_MALLOC(H5HF_huge_bt2_ctx_t)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, NULL, "can't allocate callback context")

    /* Determine the size of addresses & lengths in the file */
    ctx->sizeof_addr = H5F_SIZEOF_ADDR(f);
    ctx->sizeof_size = H5F_SIZEOF_SIZE(f);

    /* Set return value */
    ret_value = ctx;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5HF_huge_bt2_crt_context() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_dst_context
 *
 * Purpose:	Destroy client callback context
 *
 * Note:	Common to all 'huge' v2 B-tree clients
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Thursday, November 26, 2009
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_dst_context(void *_ctx)
{
    H5HF_huge_bt2_ctx_t *ctx = (H5HF_huge_bt2_ctx_t *)_ctx;       /* Callback context structure */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(ctx);

    /* Release callback context */
    ctx = H5FL_FREE(H5HF_huge_bt2_ctx_t, ctx);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_dst_context() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_crt_dbg_context
 *
 * Purpose:	Create context for debugging callback
 *
 * Return:	Success:	non-NULL
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, December 1, 2009
 *
 *-------------------------------------------------------------------------
 */
static void *
H5HF_huge_bt2_crt_dbg_context(H5F_t *f, hid_t UNUSED dxpl_id, haddr_t UNUSED addr)
{
    H5HF_huge_bt2_ctx_t *ctx;   /* Callback context structure */
    void *ret_value;            /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(f);

    /* Allocate callback context */
    if(NULL == (ctx = H5FL_MALLOC(H5HF_huge_bt2_ctx_t)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, NULL, "can't allocate callback context")

    /* Determine the size of addresses & lengths in the file */
    ctx->sizeof_addr = H5F_SIZEOF_ADDR(f);
    ctx->sizeof_size = H5F_SIZEOF_SIZE(f);

    /* Set return value */
    ret_value = ctx;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5HF_huge_bt2_crt_dbg_context() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_indir_found
 *
 * Purpose:	Retrieve record for indirectly accessed 'huge' object, when
 *              it's found in the v2 B-tree
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, August  8, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_huge_bt2_indir_found(const void *nrecord, void *op_data)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

#ifdef QAK
HDfprintf(stderr, "%s: nrecord = {%a, %Hu, %Hu}\n", "H5HF_huge_bt2_indir_found",
        ((const H5HF_huge_bt2_indir_rec_t *)nrecord)->addr,
        ((const H5HF_huge_bt2_indir_rec_t *)nrecord)->len,
        ((const H5HF_huge_bt2_indir_rec_t *)nrecord)->id);
#endif /* QAK */
    *(H5HF_huge_bt2_indir_rec_t *)op_data = *(const H5HF_huge_bt2_indir_rec_t *)nrecord;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_indir_found() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_indir_remove
 *
 * Purpose:	Free space for indirectly accessed 'huge' object, as v2 B-tree
 *              is being deleted or v2 B-tree node is removed
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, August  8, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_huge_bt2_indir_remove(const void *nrecord, void *_udata)
{
    H5HF_huge_remove_ud_t *udata = (H5HF_huge_remove_ud_t *)_udata;   /* User callback data */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Free the space in the file for the object being removed */
    if(H5MF_xfree(udata->hdr->f, H5FD_MEM_FHEAP_HUGE_OBJ, udata->dxpl_id, ((const H5HF_huge_bt2_indir_rec_t *)nrecord)->addr, ((const H5HF_huge_bt2_indir_rec_t *)nrecord)->len) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "unable to free space for huge object on disk")

    /* Set the length of the object removed */
    udata->obj_len = ((const H5HF_huge_bt2_indir_rec_t *)nrecord)->len;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5HF_huge_bt2_indir_remove() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_indir_store
 *
 * Purpose:	Store native information into record for v2 B-tree
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, August  7, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_indir_store(void *nrecord, const void *udata)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    *(H5HF_huge_bt2_indir_rec_t *)nrecord = *(const H5HF_huge_bt2_indir_rec_t *)udata;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_indir_store() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_indir_compare
 *
 * Purpose:	Compare two native information records, according to some key
 *
 * Return:	<0 if rec1 < rec2
 *              =0 if rec1 == rec2
 *              >0 if rec1 > rec2
 *
 * Programmer:	Quincey Koziol
 *              Monday, August  7, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_indir_compare(const void *_rec1, const void *_rec2)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

#ifdef QAK
{
const H5HF_huge_bt2_indir_rec_t *rec1 = (const H5HF_huge_bt2_indir_rec_t *)_rec1;
const H5HF_huge_bt2_indir_rec_t *rec2 = (const H5HF_huge_bt2_indir_rec_t *)_rec2;

HDfprintf(stderr, "%s: rec1 = {%a, %Hu, %Hu}\n", "H5HF_huge_bt2_indir_compare", rec1->addr, rec1->len, rec1->id);
HDfprintf(stderr, "%s: rec2 = {%a, %Hu, %Hu}\n", "H5HF_huge_bt2_indir_compare", rec2->addr, rec2->len, rec2->id);
}
#endif /* QAK */
    FUNC_LEAVE_NOAPI((herr_t)(((const H5HF_huge_bt2_indir_rec_t *)_rec1)->id - ((const H5HF_huge_bt2_indir_rec_t *)_rec2)->id))
} /* H5HF_huge_bt2_indir_compare() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_indir_encode
 *
 * Purpose:	Encode native information into raw form for storing on disk
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, August  7, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_indir_encode(uint8_t *raw, const void *_nrecord, void *_ctx)
{
    H5HF_huge_bt2_ctx_t *ctx = (H5HF_huge_bt2_ctx_t *)_ctx;       /* Callback context structure */
    const H5HF_huge_bt2_indir_rec_t *nrecord = (const H5HF_huge_bt2_indir_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(ctx);

    /* Encode the record's fields */
    H5F_addr_encode_len(ctx->sizeof_addr, &raw, nrecord->addr);
    H5F_ENCODE_LENGTH_LEN(raw, nrecord->len, ctx->sizeof_size);
    H5F_ENCODE_LENGTH_LEN(raw, nrecord->id, ctx->sizeof_size);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_indir_encode() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_indir_decode
 *
 * Purpose:	Decode raw disk form of record into native form
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, August  7, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_indir_decode(const uint8_t *raw, void *_nrecord, void *_ctx)
{
    H5HF_huge_bt2_ctx_t *ctx = (H5HF_huge_bt2_ctx_t *)_ctx;       /* Callback context structure */
    H5HF_huge_bt2_indir_rec_t *nrecord = (H5HF_huge_bt2_indir_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(ctx);

    /* Decode the record's fields */
    H5F_addr_decode_len(ctx->sizeof_addr, &raw, &nrecord->addr);
    H5F_DECODE_LENGTH_LEN(raw, nrecord->len, ctx->sizeof_size);
    H5F_DECODE_LENGTH_LEN(raw, nrecord->id, ctx->sizeof_size);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_indir_decode() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_indir_debug
 *
 * Purpose:	Debug native form of record
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, August  7, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_indir_debug(FILE *stream, const H5F_t UNUSED *f, hid_t UNUSED dxpl_id,
    int indent, int fwidth, const void *_nrecord,
    const void UNUSED *_udata)
{
    const H5HF_huge_bt2_indir_rec_t *nrecord = (const H5HF_huge_bt2_indir_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDfprintf(stream, "%*s%-*s {%a, %Hu, %Hu}\n", indent, "", fwidth, "Record:",
        nrecord->addr, nrecord->len, nrecord->id);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_indir_debug() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_filt_indir_found
 *
 * Purpose:	Retrieve record for indirectly accessed, filtered 'huge' object,
 *              when it's found in the v2 B-tree
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, August  8, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_huge_bt2_filt_indir_found(const void *nrecord, void *op_data)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

#ifdef QAK
HDfprintf(stderr, "%s: nrecord = {%a, %Hu, %x, %Hu, %Hu}\n", "H5HF_huge_bt2_filt_indir_found",
        ((const H5HF_huge_bt2_filt_indir_rec_t *)nrecord)->addr,
        ((const H5HF_huge_bt2_filt_indir_rec_t *)nrecord)->len,
        ((const H5HF_huge_bt2_filt_indir_rec_t *)nrecord)->filter_mask,
        ((const H5HF_huge_bt2_filt_indir_rec_t *)nrecord)->obj_size,
        ((const H5HF_huge_bt2_filt_indir_rec_t *)nrecord)->id);
#endif /* QAK */
    *(H5HF_huge_bt2_filt_indir_rec_t *)op_data = *(const H5HF_huge_bt2_filt_indir_rec_t *)nrecord;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_filt_indir_found() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_filt_indir_remove
 *
 * Purpose:	Free space for indirectly accessed, filtered 'huge' object, as
 *              v2 B-tree is being deleted or v2 B-tree node is removed
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, August  8, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_huge_bt2_filt_indir_remove(const void *nrecord, void *_udata)
{
    H5HF_huge_remove_ud_t *udata = (H5HF_huge_remove_ud_t *)_udata;   /* User callback data */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Free the space in the file for the object being removed */
    if(H5MF_xfree(udata->hdr->f, H5FD_MEM_FHEAP_HUGE_OBJ, udata->dxpl_id, ((const H5HF_huge_bt2_filt_indir_rec_t *)nrecord)->addr, ((const H5HF_huge_bt2_filt_indir_rec_t *)nrecord)->len) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "unable to free space for huge object on disk")

    /* Set the length of the object removed */
    udata->obj_len = ((const H5HF_huge_bt2_filt_indir_rec_t *)nrecord)->obj_size;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5HF_huge_bt2_filt_indir_remove() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_filt_indir_store
 *
 * Purpose:	Store native information into record for v2 B-tree
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, August  7, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_filt_indir_store(void *nrecord, const void *udata)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    *(H5HF_huge_bt2_filt_indir_rec_t *)nrecord = *(const H5HF_huge_bt2_filt_indir_rec_t *)udata;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_filt_indir_store() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_filt_indir_compare
 *
 * Purpose:	Compare two native information records, according to some key
 *
 * Return:	<0 if rec1 < rec2
 *              =0 if rec1 == rec2
 *              >0 if rec1 > rec2
 *
 * Programmer:	Quincey Koziol
 *              Monday, August  7, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_filt_indir_compare(const void *_rec1, const void *_rec2)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

#ifdef QAK
{
const H5HF_huge_bt2_filt_indir_rec_t *rec1 = (const H5HF_huge_bt2_filt_indir_rec_t *)_rec1;
const H5HF_huge_bt2_filt_indir_rec_t *rec2 = (const H5HF_huge_bt2_filt_indir_rec_t *)_rec2;

HDfprintf(stderr, "%s: rec1 = {%a, %Hu, %x, %Hu, %Hu}\n", "H5HF_huge_bt2_filt_indir_compare", rec1->addr, rec1->len, rec1->filter_mask, rec1->obj_size, rec1->id);
HDfprintf(stderr, "%s: rec2 = {%a, %Hu, %x, %Hu, %Hu}\n", "H5HF_huge_bt2_filt_indir_compare", rec2->addr, rec2->len, rec2->filter_mask, rec2->obj_size, rec2->id);
}
#endif /* QAK */
    FUNC_LEAVE_NOAPI((herr_t)(((const H5HF_huge_bt2_filt_indir_rec_t *)_rec1)->id - ((const H5HF_huge_bt2_filt_indir_rec_t *)_rec2)->id))
} /* H5HF_huge_bt2_filt_indir_compare() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_filt_indir_encode
 *
 * Purpose:	Encode native information into raw form for storing on disk
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, August  7, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_filt_indir_encode(uint8_t *raw, const void *_nrecord, void *_ctx)
{
    H5HF_huge_bt2_ctx_t *ctx = (H5HF_huge_bt2_ctx_t *)_ctx;       /* Callback context structure */
    const H5HF_huge_bt2_filt_indir_rec_t *nrecord = (const H5HF_huge_bt2_filt_indir_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(ctx);

    /* Encode the record's fields */
    H5F_addr_encode_len(ctx->sizeof_addr, &raw, nrecord->addr);
    H5F_ENCODE_LENGTH_LEN(raw, nrecord->len, ctx->sizeof_size);
    UINT32ENCODE(raw, nrecord->filter_mask);
    H5F_ENCODE_LENGTH_LEN(raw, nrecord->obj_size, ctx->sizeof_size);
    H5F_ENCODE_LENGTH_LEN(raw, nrecord->id, ctx->sizeof_size);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_filt_indir_encode() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_filt_indir_decode
 *
 * Purpose:	Decode raw disk form of record into native form
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, August  7, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_filt_indir_decode(const uint8_t *raw, void *_nrecord, void *_ctx)
{
    H5HF_huge_bt2_ctx_t *ctx = (H5HF_huge_bt2_ctx_t *)_ctx;       /* Callback context structure */
    H5HF_huge_bt2_filt_indir_rec_t *nrecord = (H5HF_huge_bt2_filt_indir_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(ctx);

    /* Decode the record's fields */
    H5F_addr_decode_len(ctx->sizeof_addr, &raw, &nrecord->addr);
    H5F_DECODE_LENGTH_LEN(raw, nrecord->len, ctx->sizeof_size);
    UINT32DECODE(raw, nrecord->filter_mask);
    H5F_DECODE_LENGTH_LEN(raw, nrecord->obj_size, ctx->sizeof_size);
    H5F_DECODE_LENGTH_LEN(raw, nrecord->id, ctx->sizeof_size);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_filt_indir_decode() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_filt_indir_debug
 *
 * Purpose:	Debug native form of record
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, August  7, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_filt_indir_debug(FILE *stream, const H5F_t UNUSED *f, hid_t UNUSED dxpl_id,
    int indent, int fwidth, const void *_nrecord,
    const void UNUSED *_udata)
{
    const H5HF_huge_bt2_filt_indir_rec_t *nrecord = (const H5HF_huge_bt2_filt_indir_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDfprintf(stream, "%*s%-*s {%a, %Hu, %x, %Hu, %Hu}\n", indent, "", fwidth, "Record:",
        nrecord->addr, nrecord->len, nrecord->filter_mask, nrecord->obj_size, nrecord->id);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_filt_indir_debug() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_dir_remove
 *
 * Purpose:	Free space for directly accessed 'huge' object, as v2 B-tree
 *              is being deleted or v2 B-tree node is being removed
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, August  8, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_huge_bt2_dir_remove(const void *nrecord, void *_udata)
{
    H5HF_huge_remove_ud_t *udata = (H5HF_huge_remove_ud_t *)_udata;   /* User callback data */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Free the space in the file for the object being removed */
    if(H5MF_xfree(udata->hdr->f, H5FD_MEM_FHEAP_HUGE_OBJ, udata->dxpl_id, ((const H5HF_huge_bt2_indir_rec_t *)nrecord)->addr, ((const H5HF_huge_bt2_indir_rec_t *)nrecord)->len) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "unable to free space for huge object on disk")

    /* Set the length of the object removed */
    udata->obj_len = ((const H5HF_huge_bt2_indir_rec_t *)nrecord)->len;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5HF_huge_bt2_dir_remove() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_dir_store
 *
 * Purpose:	Store native information into record for v2 B-tree
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, August  7, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_dir_store(void *nrecord, const void *udata)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    *(H5HF_huge_bt2_dir_rec_t *)nrecord = *(const H5HF_huge_bt2_dir_rec_t *)udata;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_dir_store() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_dir_compare
 *
 * Purpose:	Compare two native information records, according to some key
 *
 * Return:	<0 if rec1 < rec2
 *              =0 if rec1 == rec2
 *              >0 if rec1 > rec2
 *
 * Programmer:	Quincey Koziol
 *              Monday, August  7, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_dir_compare(const void *_rec1, const void *_rec2)
{
    const H5HF_huge_bt2_dir_rec_t *rec1 = (const H5HF_huge_bt2_dir_rec_t *)_rec1;
    const H5HF_huge_bt2_dir_rec_t *rec2 = (const H5HF_huge_bt2_dir_rec_t *)_rec2;
    herr_t ret_value;           /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

#ifdef QAK
HDfprintf(stderr, "%s: rec1 = {%a, %Hu}\n", "H5HF_huge_bt2_dir_compare", rec1->addr, rec1->len);
HDfprintf(stderr, "%s: rec2 = {%a, %Hu}\n", "H5HF_huge_bt2_dir_compare", rec2->addr, rec2->len);
#endif /* QAK */
    if(rec1->addr < rec2->addr)
        ret_value = -1;
    else if(rec1->addr > rec2->addr)
        ret_value = 1;
    else if(rec1->len < rec2->len)
        ret_value = -1;
    else if(rec1->len > rec2->len)
        ret_value = 1;
    else
        ret_value = 0;

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5HF_huge_bt2_dir_compare() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_dir_encode
 *
 * Purpose:	Encode native information into raw form for storing on disk
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, August  7, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_dir_encode(uint8_t *raw, const void *_nrecord, void *_ctx)
{
    H5HF_huge_bt2_ctx_t *ctx = (H5HF_huge_bt2_ctx_t *)_ctx;       /* Callback context structure */
    const H5HF_huge_bt2_dir_rec_t *nrecord = (const H5HF_huge_bt2_dir_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(ctx);

    /* Encode the record's fields */
    H5F_addr_encode_len(ctx->sizeof_addr, &raw, nrecord->addr);
    H5F_ENCODE_LENGTH_LEN(raw, nrecord->len, ctx->sizeof_size);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_dir_encode() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_dir_decode
 *
 * Purpose:	Decode raw disk form of record into native form
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, August  7, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_dir_decode(const uint8_t *raw, void *_nrecord, void *_ctx)
{
    H5HF_huge_bt2_ctx_t *ctx = (H5HF_huge_bt2_ctx_t *)_ctx;       /* Callback context structure */
    H5HF_huge_bt2_dir_rec_t *nrecord = (H5HF_huge_bt2_dir_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(ctx);

    /* Decode the record's fields */
    H5F_addr_decode_len(ctx->sizeof_addr, &raw, &nrecord->addr);
    H5F_DECODE_LENGTH_LEN(raw, nrecord->len, ctx->sizeof_size);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_dir_decode() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_dir_debug
 *
 * Purpose:	Debug native form of record
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, August  7, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_dir_debug(FILE *stream, const H5F_t UNUSED *f, hid_t UNUSED dxpl_id,
    int indent, int fwidth, const void *_nrecord,
    const void UNUSED *_udata)
{
    const H5HF_huge_bt2_dir_rec_t *nrecord = (const H5HF_huge_bt2_dir_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDfprintf(stream, "%*s%-*s {%a, %Hu}\n", indent, "", fwidth, "Record:",
        nrecord->addr, nrecord->len);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_dir_debug() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_filt_dir_found
 *
 * Purpose:	Retrieve record for directly accessed, filtered 'huge' object,
 *              when it's found in the v2 B-tree
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, August 15, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_huge_bt2_filt_dir_found(const void *nrecord, void *op_data)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

#ifdef QAK
HDfprintf(stderr, "%s: nrecord = {%a, %Hu, %x, %Hu}\n", "H5HF_huge_bt2_filt_dir_found",
        ((const H5HF_huge_bt2_filt_dir_rec_t *)nrecord)->addr,
        ((const H5HF_huge_bt2_filt_dir_rec_t *)nrecord)->len,
        ((const H5HF_huge_bt2_filt_dir_rec_t *)nrecord)->filter_mask,
        ((const H5HF_huge_bt2_filt_dir_rec_t *)nrecord)->obj_size);
#endif /* QAK */
    *(H5HF_huge_bt2_filt_dir_rec_t *)op_data = *(const H5HF_huge_bt2_filt_dir_rec_t *)nrecord;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_filt_dir_found() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_filt_dir_remove
 *
 * Purpose:	Free space for directly accessed, filtered 'huge' object, as
 *              v2 B-tree is being deleted or v2 B-tree node is removed
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, August 15, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_huge_bt2_filt_dir_remove(const void *nrecord, void *_udata)
{
    H5HF_huge_remove_ud_t *udata = (H5HF_huge_remove_ud_t *)_udata;   /* User callback data */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Free the space in the file for the object being removed */
    if(H5MF_xfree(udata->hdr->f, H5FD_MEM_FHEAP_HUGE_OBJ, udata->dxpl_id, ((const H5HF_huge_bt2_filt_dir_rec_t *)nrecord)->addr, ((const H5HF_huge_bt2_filt_dir_rec_t *)nrecord)->len) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "unable to free space for huge object on disk")

    /* Set the length of the object removed */
    udata->obj_len = ((const H5HF_huge_bt2_filt_dir_rec_t *)nrecord)->obj_size;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5HF_huge_bt2_filt_dir_remove() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_filt_dir_store
 *
 * Purpose:	Store native information into record for v2 B-tree
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, August 15, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_filt_dir_store(void *nrecord, const void *udata)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    *(H5HF_huge_bt2_filt_dir_rec_t *)nrecord = *(const H5HF_huge_bt2_filt_dir_rec_t *)udata;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_filt_dir_store() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_filt_dir_compare
 *
 * Purpose:	Compare two native information records, according to some key
 *
 * Return:	<0 if rec1 < rec2
 *              =0 if rec1 == rec2
 *              >0 if rec1 > rec2
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, August 15, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_filt_dir_compare(const void *_rec1, const void *_rec2)
{
    const H5HF_huge_bt2_filt_dir_rec_t *rec1 = (const H5HF_huge_bt2_filt_dir_rec_t *)_rec1;
    const H5HF_huge_bt2_filt_dir_rec_t *rec2 = (const H5HF_huge_bt2_filt_dir_rec_t *)_rec2;
    herr_t ret_value;           /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

#ifdef QAK
HDfprintf(stderr, "%s: rec1 = {%a, %Hu, %x, %Hu}\n", "H5HF_huge_bt2_filt_dir_compare", rec1->addr, rec1->len, rec1->filter_mask, rec1->obj_size);
HDfprintf(stderr, "%s: rec2 = {%a, %Hu, %x, %Hu}\n", "H5HF_huge_bt2_filt_dir_compare", rec2->addr, rec2->len, rec2->filter_mask, rec2->obj_size);
#endif /* QAK */
    if(rec1->addr < rec2->addr)
        ret_value = -1;
    else if(rec1->addr > rec2->addr)
        ret_value = 1;
    else if(rec1->len < rec2->len)
        ret_value = -1;
    else if(rec1->len > rec2->len)
        ret_value = 1;
    else
        ret_value = 0;

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5HF_huge_bt2_filt_dir_compare() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_filt_dir_encode
 *
 * Purpose:	Encode native information into raw form for storing on disk
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, August 15, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_filt_dir_encode(uint8_t *raw, const void *_nrecord, void *_ctx)
{
    H5HF_huge_bt2_ctx_t *ctx = (H5HF_huge_bt2_ctx_t *)_ctx;       /* Callback context structure */
    const H5HF_huge_bt2_filt_dir_rec_t *nrecord = (const H5HF_huge_bt2_filt_dir_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(ctx);

    /* Encode the record's fields */
    H5F_addr_encode_len(ctx->sizeof_addr, &raw, nrecord->addr);
    H5F_ENCODE_LENGTH_LEN(raw, nrecord->len, ctx->sizeof_size);
    UINT32ENCODE(raw, nrecord->filter_mask);
    H5F_ENCODE_LENGTH_LEN(raw, nrecord->obj_size, ctx->sizeof_size);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_filt_dir_encode() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_filt_dir_decode
 *
 * Purpose:	Decode raw disk form of record into native form
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, August 15, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_filt_dir_decode(const uint8_t *raw, void *_nrecord, void *_ctx)
{
    H5HF_huge_bt2_ctx_t *ctx = (H5HF_huge_bt2_ctx_t *)_ctx;       /* Callback context structure */
    H5HF_huge_bt2_filt_dir_rec_t *nrecord = (H5HF_huge_bt2_filt_dir_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(ctx);

    /* Decode the record's fields */
    H5F_addr_decode_len(ctx->sizeof_addr, &raw, &nrecord->addr);
    H5F_DECODE_LENGTH_LEN(raw, nrecord->len, ctx->sizeof_size);
    UINT32DECODE(raw, nrecord->filter_mask);
    H5F_DECODE_LENGTH_LEN(raw, nrecord->obj_size, ctx->sizeof_size);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_filt_dir_decode() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_huge_bt2_filt_dir_debug
 *
 * Purpose:	Debug native form of record
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, August 15, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_huge_bt2_filt_dir_debug(FILE *stream, const H5F_t UNUSED *f, hid_t UNUSED dxpl_id,
    int indent, int fwidth, const void *_nrecord, const void UNUSED *_udata)
{
    const H5HF_huge_bt2_filt_dir_rec_t *nrecord = (const H5HF_huge_bt2_filt_dir_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDfprintf(stream, "%*s%-*s {%a, %Hu, %x, %Hu}\n", indent, "", fwidth, "Record:",
        nrecord->addr, nrecord->len, nrecord->filter_mask, nrecord->obj_size);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_huge_bt2_filt_dir_debug() */

