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
 * Created:		H5Gbtree2.c
 *			Sep  9 2006
 *			Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:		v2 B-tree callbacks for indexing fields on links
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5G_PACKAGE		/*suppress error about including H5Gpkg  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Gpkg.h"		/* Groups		  		*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/

/*
 * Data exchange structure for dense link storage.  This structure is
 * passed through the fractal heap layer to compare links.
 */
typedef struct H5G_fh_ud_cmp_t {
    /* downward */
    H5F_t       *f;                     /* Pointer to file that fractal heap is in */
    hid_t       dxpl_id;                /* DXPL for operation                */
    const char  *name;                  /* Name of link to compare           */
    H5B2_found_t found_op;              /* Callback when correct link is found */
    void        *found_op_data;         /* Callback data when correct link is found */

    /* upward */
    int         cmp;                    /* Comparison of two link names      */
} H5G_fh_ud_cmp_t;


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

/* v2 B-tree function callbacks */

/* v2 B-tree driver callbacks for 'creation order' index */
static herr_t H5G_dense_btree2_corder_store(void *native, const void *udata);
static herr_t H5G_dense_btree2_corder_compare(const void *rec1, const void *rec2);
static herr_t H5G_dense_btree2_corder_encode(uint8_t *raw, const void *native,
    void *ctx);
static herr_t H5G_dense_btree2_corder_decode(const uint8_t *raw, void *native,
    void *ctx);
static herr_t H5G_dense_btree2_corder_debug(FILE *stream, const H5F_t *f, hid_t dxpl_id,
    int indent, int fwidth, const void *record, const void *_udata);

/* v2 B-tree driver callbacks for 'name' index */
static herr_t H5G_dense_btree2_name_store(void *native, const void *udata);
static herr_t H5G_dense_btree2_name_compare(const void *rec1, const void *rec2);
static herr_t H5G_dense_btree2_name_encode(uint8_t *raw, const void *native,
    void *ctx);
static herr_t H5G_dense_btree2_name_decode(const uint8_t *raw, void *native,
    void *ctx);
static herr_t H5G_dense_btree2_name_debug(FILE *stream, const H5F_t *f, hid_t dxpl_id,
    int indent, int fwidth, const void *record, const void *_udata);

/* Fractal heap function callbacks */
static herr_t H5G_dense_fh_name_cmp(const void *obj, size_t obj_len, void *op_data);


/*********************/
/* Package Variables */
/*********************/
/* v2 B-tree class for indexing 'name' field of links */
const H5B2_class_t H5G_BT2_NAME[1]={{   /* B-tree class information */
    H5B2_GRP_DENSE_NAME_ID,             /* Type of B-tree */
    "H5B2_GRP_DENSE_NAME_ID",           /* Name of B-tree class */
    sizeof(H5G_dense_bt2_name_rec_t),   /* Size of native record */
    NULL,                               /* Create client callback context */
    NULL,                               /* Destroy client callback context */
    H5G_dense_btree2_name_store,        /* Record storage callback */
    H5G_dense_btree2_name_compare,      /* Record comparison callback */
    H5G_dense_btree2_name_encode,       /* Record encoding callback */
    H5G_dense_btree2_name_decode,       /* Record decoding callback */
    H5G_dense_btree2_name_debug,        /* Record debugging callback */
    NULL,                               /* Create debugging context */
    NULL                                /* Destroy debugging context */
}};

/* v2 B-tree class for indexing 'creation order' field of links */
const H5B2_class_t H5G_BT2_CORDER[1]={{ /* B-tree class information */
    H5B2_GRP_DENSE_CORDER_ID,           /* Type of B-tree */
    "H5B2_GRP_DENSE_CORDER_ID",         /* Name of B-tree class */
    sizeof(H5G_dense_bt2_corder_rec_t), /* Size of native record */
    NULL,                               /* Create client callback context */
    NULL,                               /* Destroy client callback context */
    H5G_dense_btree2_corder_store,      /* Record storage callback */
    H5G_dense_btree2_corder_compare,    /* Record comparison callback */
    H5G_dense_btree2_corder_encode,     /* Record encoding callback */
    H5G_dense_btree2_corder_decode,     /* Record decoding callback */
    H5G_dense_btree2_corder_debug,      /* Record debugging callback */
    NULL,                               /* Create debugging context */
    NULL                                /* Destroy debugging context */
}};

/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5G_dense_fh_name_cmp
 *
 * Purpose:	Compares the name of a link in a fractal heap to another
 *              name
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Sep 11 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_dense_fh_name_cmp(const void *obj, size_t UNUSED obj_len, void *_udata)
{
    H5G_fh_ud_cmp_t *udata = (H5G_fh_ud_cmp_t *)_udata;         /* User data for 'op' callback */
    H5O_link_t *lnk;    /* Pointer to link created from heap object */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Decode link information */
    if(NULL == (lnk = (H5O_link_t *)H5O_msg_decode(udata->f, udata->dxpl_id, NULL, H5O_LINK_ID, (const unsigned char *)obj)))
        HGOTO_ERROR(H5E_SYM, H5E_CANTDECODE, FAIL, "can't decode link")

    /* Compare the string values */
    udata->cmp = HDstrcmp(udata->name, lnk->name);

    /* Check for correct link & callback to make */
    if(udata->cmp == 0 && udata->found_op) {
        if((udata->found_op)(lnk, udata->found_op_data) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTOPERATE, FAIL, "link found callback failed")
    } /* end if */

    /* Release the space allocated for the link */
    H5O_msg_free(H5O_LINK_ID, lnk);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_dense_fh_name_cmp() */


/*-------------------------------------------------------------------------
 * Function:	H5G_dense_btree2_name_store
 *
 * Purpose:	Store user information into native record for v2 B-tree
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Saturday, September  9, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_dense_btree2_name_store(void *_nrecord, const void *_udata)
{
    const H5G_bt2_ud_ins_t *udata = (const H5G_bt2_ud_ins_t *)_udata;
    H5G_dense_bt2_name_rec_t *nrecord = (H5G_dense_bt2_name_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Copy user information info native record */
    nrecord->hash = udata->common.name_hash;
    HDmemcpy(nrecord->id, udata->id, (size_t)H5G_DENSE_FHEAP_ID_LEN);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5G_dense_btree2_name_store() */


/*-------------------------------------------------------------------------
 * Function:	H5G_dense_btree2_name_compare
 *
 * Purpose:	Compare two native information records, according to some key
 *
 * Return:	<0 if rec1 < rec2
 *              =0 if rec1 == rec2
 *              >0 if rec1 > rec2
 *
 * Programmer:	Quincey Koziol
 *              Monday, September 11, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_dense_btree2_name_compare(const void *_bt2_udata, const void *_bt2_rec)
{
    const H5G_bt2_ud_common_t *bt2_udata = (const H5G_bt2_ud_common_t *)_bt2_udata;
    const H5G_dense_bt2_name_rec_t *bt2_rec = (const H5G_dense_bt2_name_rec_t *)_bt2_rec;
    herr_t ret_value;           /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(bt2_udata);
    HDassert(bt2_rec);

#ifdef QAK
{
unsigned u;

HDfprintf(stderr, "%s: bt2_udata = {'%s', %x}\n", "H5G_dense_btree2_name_compare", bt2_udata->name, (unsigned)bt2_udata->name_hash);
HDfprintf(stderr, "%s: bt2_rec = {%x, ", "H5G_dense_btree2_name_compare", (unsigned)bt2_rec->hash);
for(u = 0; u < H5G_DENSE_FHEAP_ID_LEN; u++)
    HDfprintf(stderr, "%02x%s", bt2_rec->id[u], (u < (H5G_DENSE_FHEAP_ID_LEN - 1) ? " " : "}\n"));
}
#endif /* QAK */
    /* Check hash value */
    if(bt2_udata->name_hash < bt2_rec->hash)
        ret_value = (-1);
    else if(bt2_udata->name_hash > bt2_rec->hash)
        ret_value = 1;
    else {
        H5G_fh_ud_cmp_t fh_udata;       /* User data for fractal heap 'op' callback */
        herr_t status;                  /* Status from fractal heap 'op' routine */

        /* Sanity check */
        HDassert(bt2_udata->name_hash == bt2_rec->hash);

        /* Prepare user data for callback */
        /* down */
        fh_udata.f = bt2_udata->f;
        fh_udata.dxpl_id = bt2_udata->dxpl_id;
        fh_udata.name = bt2_udata->name;
        fh_udata.found_op = bt2_udata->found_op;
        fh_udata.found_op_data = bt2_udata->found_op_data;

        /* up */
        fh_udata.cmp = 0;

        /* Check if the user's link and the B-tree's link have the same name */
        status = H5HF_op(bt2_udata->fheap, bt2_udata->dxpl_id, bt2_rec->id,
                H5G_dense_fh_name_cmp, &fh_udata);
        HDassert(status >= 0);

        /* Callback will set comparison value */
        ret_value = fh_udata.cmp;
    } /* end else */

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5G_dense_btree2_name_compare() */


/*-------------------------------------------------------------------------
 * Function:	H5G_dense_btree2_name_encode
 *
 * Purpose:	Encode native information into raw form for storing on disk
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, September 11, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_dense_btree2_name_encode(uint8_t *raw, const void *_nrecord, void UNUSED *ctx)
{
    const H5G_dense_bt2_name_rec_t *nrecord = (const H5G_dense_bt2_name_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Encode the record's fields */
    UINT32ENCODE(raw, nrecord->hash)
    HDmemcpy(raw, nrecord->id, (size_t)H5G_DENSE_FHEAP_ID_LEN);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5G_dense_btree2_name_encode() */


/*-------------------------------------------------------------------------
 * Function:	H5G_dense_btree2_name_decode
 *
 * Purpose:	Decode raw disk form of record into native form
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, September 11, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_dense_btree2_name_decode(const uint8_t *raw, void *_nrecord, void UNUSED *ctx)
{
    H5G_dense_bt2_name_rec_t *nrecord = (H5G_dense_bt2_name_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Decode the record's fields */
    UINT32DECODE(raw, nrecord->hash)
    HDmemcpy(nrecord->id, raw, (size_t)H5G_DENSE_FHEAP_ID_LEN);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5G_dense_btree2_name_decode() */


/*-------------------------------------------------------------------------
 * Function:	H5G_dense_btree2_name_debug
 *
 * Purpose:	Debug native form of record
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, September 11, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_dense_btree2_name_debug(FILE *stream, const H5F_t UNUSED *f, hid_t UNUSED dxpl_id,
    int indent, int fwidth, const void *_nrecord,
    const void UNUSED *_udata)
{
    const H5G_dense_bt2_name_rec_t *nrecord = (const H5G_dense_bt2_name_rec_t *)_nrecord;
    unsigned u;                 /* Local index variable */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDfprintf(stream, "%*s%-*s {%lx, ", indent, "", fwidth, "Record:",
        nrecord->hash);
    for(u = 0; u < H5G_DENSE_FHEAP_ID_LEN; u++)
        HDfprintf(stderr, "%02x%s", nrecord->id[u], (u < (H5G_DENSE_FHEAP_ID_LEN - 1) ? " " : "}\n"));

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5G_dense_btree2_name_debug() */


/*-------------------------------------------------------------------------
 * Function:	H5G_dense_btree2_corder_store
 *
 * Purpose:	Store user information into native record for v2 B-tree
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, October 30, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_dense_btree2_corder_store(void *_nrecord, const void *_udata)
{
    const H5G_bt2_ud_ins_t *udata = (const H5G_bt2_ud_ins_t *)_udata;
    H5G_dense_bt2_corder_rec_t *nrecord = (H5G_dense_bt2_corder_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Copy user information info native record */
    nrecord->corder = udata->common.corder;
    HDmemcpy(nrecord->id, udata->id, (size_t)H5G_DENSE_FHEAP_ID_LEN);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5G_dense_btree2_corder_store() */


/*-------------------------------------------------------------------------
 * Function:	H5G_dense_btree2_corder_compare
 *
 * Purpose:	Compare two native information records, according to some key
 *
 * Return:	<0 if rec1 < rec2
 *              =0 if rec1 == rec2
 *              >0 if rec1 > rec2
 *
 * Programmer:	Quincey Koziol
 *              Monday, October 30, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_dense_btree2_corder_compare(const void *_bt2_udata, const void *_bt2_rec)
{
    const H5G_bt2_ud_common_t *bt2_udata = (const H5G_bt2_ud_common_t *)_bt2_udata;
    const H5G_dense_bt2_corder_rec_t *bt2_rec = (const H5G_dense_bt2_corder_rec_t *)_bt2_rec;
    herr_t ret_value;           /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(bt2_udata);
    HDassert(bt2_rec);

#ifdef QAK
{
unsigned u;

HDfprintf(stderr, "%s: bt2_udata->corder = %Hd\n", "H5G_dense_btree2_corder_compare", (hsize_t)bt2_udata->corder);
HDfprintf(stderr, "%s: bt2_rec = {%Hu, ", "H5G_dense_btree2_corder_compare", (hsize_t)bt2_rec->corder);
for(u = 0; u < H5G_DENSE_FHEAP_ID_LEN; u++)
    HDfprintf(stderr, "%02x%s", bt2_rec->id[u], (u < (H5G_DENSE_FHEAP_ID_LEN - 1) ? " " : "}\n"));
}
#endif /* QAK */
    /* Check creation order value */
    if(bt2_udata->corder < bt2_rec->corder)
        ret_value = -1;
    else if(bt2_udata->corder > bt2_rec->corder)
        ret_value = 1;
    else
        ret_value = 0;

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5G_dense_btree2_corder_compare() */


/*-------------------------------------------------------------------------
 * Function:	H5G_dense_btree2_corder_encode
 *
 * Purpose:	Encode native information into raw form for storing on disk
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, October 30, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_dense_btree2_corder_encode(uint8_t *raw, const void *_nrecord, void UNUSED *ctx)
{
    const H5G_dense_bt2_corder_rec_t *nrecord = (const H5G_dense_bt2_corder_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Encode the record's fields */
    INT64ENCODE(raw, nrecord->corder)
    HDmemcpy(raw, nrecord->id, (size_t)H5G_DENSE_FHEAP_ID_LEN);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5G_dense_btree2_corder_encode() */


/*-------------------------------------------------------------------------
 * Function:	H5G_dense_btree2_corder_decode
 *
 * Purpose:	Decode raw disk form of record into native form
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, October 30, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_dense_btree2_corder_decode(const uint8_t *raw, void *_nrecord, void UNUSED *ctx)
{
    H5G_dense_bt2_corder_rec_t *nrecord = (H5G_dense_bt2_corder_rec_t *)_nrecord;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Decode the record's fields */
    INT64DECODE(raw, nrecord->corder)
    HDmemcpy(nrecord->id, raw, (size_t)H5G_DENSE_FHEAP_ID_LEN);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5G_dense_btree2_corder_decode() */


/*-------------------------------------------------------------------------
 * Function:	H5G_dense_btree2_corder_debug
 *
 * Purpose:	Debug native form of record
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, October 30, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_dense_btree2_corder_debug(FILE *stream, const H5F_t UNUSED *f, hid_t UNUSED dxpl_id,
    int indent, int fwidth, const void *_nrecord,
    const void UNUSED *_udata)
{
    const H5G_dense_bt2_corder_rec_t *nrecord = (const H5G_dense_bt2_corder_rec_t *)_nrecord;
    unsigned u;                 /* Local index variable */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDfprintf(stream, "%*s%-*s {%Hu, ", indent, "", fwidth, "Record:",
        nrecord->corder);
    for(u = 0; u < H5G_DENSE_FHEAP_ID_LEN; u++)
        HDfprintf(stderr, "%02x%s", nrecord->id[u], (u < (H5G_DENSE_FHEAP_ID_LEN - 1) ? " " : "}\n"));

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5G_dense_btree2_corder_debug() */

