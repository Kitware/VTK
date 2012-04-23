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
 * Created:		H5Pdcpl.c
 *			February 26 1998
 *			Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:		Dataset creation property list class routines
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/
#define H5P_PACKAGE		/*suppress error about including H5Ppkg	  */
#define H5Z_PACKAGE		/*suppress error about including H5Zpkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5ACprivate.h"	/* Metadata cache			*/
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Ppkg.h"		/* Property lists		  	*/
#include "H5Zpkg.h"		/* Data filters				*/


/****************/
/* Local Macros */
/****************/

/* Define default layout information */
#define H5D_DEF_STORAGE_COMPACT_INIT  {(hbool_t)FALSE, (size_t)0, NULL}
#define H5D_DEF_STORAGE_CONTIG_INIT   {HADDR_UNDEF, (hsize_t)0}
#define H5D_DEF_STORAGE_CHUNK_INIT    {H5D_CHUNK_BTREE, HADDR_UNDEF,  NULL, {{NULL}}}
#define H5D_DEF_LAYOUT_CHUNK_INIT    {(unsigned)1, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, (uint32_t)0, (hsize_t)0, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
#ifdef H5_HAVE_C99_DESIGNATED_INITIALIZER
#define H5D_DEF_STORAGE_COMPACT  {H5D_COMPACT, { .compact = H5D_DEF_STORAGE_COMPACT_INIT }}
#define H5D_DEF_STORAGE_CONTIG   {H5D_CONTIGUOUS, { .contig = H5D_DEF_STORAGE_CONTIG_INIT }}
#define H5D_DEF_STORAGE_CHUNK    {H5D_CHUNKED, { .chunk = H5D_DEF_STORAGE_CHUNK_INIT }}
#define H5D_DEF_LAYOUT_COMPACT  {H5D_COMPACT, H5O_LAYOUT_VERSION_3, NULL, {H5D_DEF_LAYOUT_CHUNK_INIT}, H5D_DEF_STORAGE_COMPACT}
#define H5D_DEF_LAYOUT_CONTIG   {H5D_CONTIGUOUS, H5O_LAYOUT_VERSION_3, NULL, {H5D_DEF_LAYOUT_CHUNK_INIT}, H5D_DEF_STORAGE_CONTIG}
#define H5D_DEF_LAYOUT_CHUNK    {H5D_CHUNKED, H5O_LAYOUT_VERSION_3, NULL, {H5D_DEF_LAYOUT_CHUNK_INIT}, H5D_DEF_STORAGE_CHUNK}
#else /* H5_HAVE_C99_DESIGNATED_INITIALIZER */
/* Note that the compact & chunked layout initialization values are using the
 *      contiguous layout initialization in the union, because the contiguous
 *      layout is first in the union.  These values are overridden in the
 *      H5P_init_def_layout() routine. -QAK
 */
#define H5D_DEF_LAYOUT_COMPACT  {H5D_COMPACT, H5O_LAYOUT_VERSION_3, NULL, {H5D_DEF_LAYOUT_CHUNK_INIT}, {H5D_CONTIGUOUS, H5D_DEF_STORAGE_CONTIG_INIT}}
#define H5D_DEF_LAYOUT_CONTIG   {H5D_CONTIGUOUS, H5O_LAYOUT_VERSION_3, NULL, {H5D_DEF_LAYOUT_CHUNK_INIT}, {H5D_CONTIGUOUS, H5D_DEF_STORAGE_CONTIG_INIT}}
#define H5D_DEF_LAYOUT_CHUNK    {H5D_CHUNKED, H5O_LAYOUT_VERSION_3, NULL, {H5D_DEF_LAYOUT_CHUNK_INIT}, {H5D_CONTIGUOUS, H5D_DEF_STORAGE_CONTIG_INIT}}
#endif /* H5_HAVE_C99_DESIGNATED_INITIALIZER */

/* ========  Dataset creation properties ======== */
/* Definitions for storage layout property */
#define H5D_CRT_LAYOUT_SIZE        sizeof(H5O_layout_t)
#define H5D_CRT_LAYOUT_DEF         H5D_DEF_LAYOUT_CONTIG
#define H5D_CRT_LAYOUT_CMP     H5P_dcrt_layout_cmp
/* Definitions for fill value.  size=0 means fill value will be 0 as
 * library default; size=-1 means fill value is undefined. */
#define H5D_CRT_FILL_VALUE_SIZE    sizeof(H5O_fill_t)
#define H5D_CRT_FILL_VALUE_DEF     {{0, NULL, H5O_NULL_ID, {{0, HADDR_UNDEF}}}, H5O_FILL_VERSION_2, NULL, 0, NULL, H5D_ALLOC_TIME_LATE, H5D_FILL_TIME_IFSET, FALSE}
#define H5D_CRT_FILL_VALUE_CMP     H5P_fill_value_cmp
/* Definitions for space allocation time state */
#define H5D_CRT_ALLOC_TIME_STATE_SIZE   sizeof(unsigned)
#define H5D_CRT_ALLOC_TIME_STATE_DEF    1
/* Definitions for external file list */
#define H5D_CRT_EXT_FILE_LIST_SIZE sizeof(H5O_efl_t)
#define H5D_CRT_EXT_FILE_LIST_DEF  {HADDR_UNDEF, 0, 0, NULL}
#define H5D_CRT_EXT_FILE_LIST_CMP  H5P_dcrt_ext_file_list_cmp


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

/* General routines */
static herr_t H5P_set_layout(H5P_genplist_t *plist, const H5O_layout_t *layout);
#ifndef H5_HAVE_C99_DESIGNATED_INITIALIZER
static herr_t H5P_init_def_layout(void);
#endif /* H5_HAVE_C99_DESIGNATED_INITIALIZER */

/* Property class callbacks */
static herr_t H5P_dcrt_reg_prop(H5P_genclass_t *pclass);
static herr_t H5P_dcrt_copy(hid_t new_plist_t, hid_t old_plist_t, void *copy_data);
static herr_t H5P_dcrt_close(hid_t dxpl_id, void *close_data);

/* Property callbacks */
static int H5P_dcrt_layout_cmp(const void *value1, const void *value2, size_t size);
static int H5P_dcrt_ext_file_list_cmp(const void *value1, const void *value2, size_t size);


/*********************/
/* Package Variables */
/*********************/

/* Dataset creation property list class library initialization object */
const H5P_libclass_t H5P_CLS_DCRT[1] = {{
    "dataset create",		/* Class name for debugging     */
    &H5P_CLS_OBJECT_CREATE_g,	/* Parent class ID              */
    &H5P_CLS_DATASET_CREATE_g,	/* Pointer to class ID          */
    &H5P_LST_DATASET_CREATE_g,	/* Pointer to default property list ID */
    H5P_dcrt_reg_prop,		/* Default property registration routine */
    NULL,		        /* Class creation callback      */
    NULL,		        /* Class creation callback info */
    H5P_dcrt_copy,		/* Class copy callback          */
    NULL,		        /* Class copy callback info     */
    H5P_dcrt_close,		/* Class close callback         */
    NULL 		        /* Class close callback info    */
}};


/*****************************/
/* Library Private Variables */
/*****************************/

/* Declare extern the free list to manage blocks of type conversion data */
H5FL_BLK_EXTERN(type_conv);

/* Defaults for each type of layout */
#ifdef H5_HAVE_C99_DESIGNATED_INITIALIZER
static const H5O_layout_t H5D_def_layout_compact_g = H5D_DEF_LAYOUT_COMPACT;
static const H5O_layout_t H5D_def_layout_contig_g = H5D_DEF_LAYOUT_CONTIG;
static const H5O_layout_t H5D_def_layout_chunk_g = H5D_DEF_LAYOUT_CHUNK;
#else /* H5_HAVE_C99_DESIGNATED_INITIALIZER */
static H5O_layout_t H5D_def_layout_compact_g = H5D_DEF_LAYOUT_COMPACT;
static H5O_layout_t H5D_def_layout_contig_g = H5D_DEF_LAYOUT_CONTIG;
static H5O_layout_t H5D_def_layout_chunk_g = H5D_DEF_LAYOUT_CHUNK;
static hbool_t H5P_dcrt_def_layout_init_g = FALSE;
#endif /* H5_HAVE_C99_DESIGNATED_INITIALIZER */



/*-------------------------------------------------------------------------
 * Function:    H5P_dcrt_reg_prop
 *
 * Purpose:     Register the dataset creation property list class's properties
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              October 31, 2006
 *-------------------------------------------------------------------------
 */
static herr_t
H5P_dcrt_reg_prop(H5P_genclass_t *pclass)
{
    H5O_layout_t layout = H5D_CRT_LAYOUT_DEF;          /* Default storage layout */
    H5O_fill_t fill = H5D_CRT_FILL_VALUE_DEF;           /* Default fill value */
    unsigned alloc_time_state = H5D_CRT_ALLOC_TIME_STATE_DEF;   /* Default allocation time state */
    H5O_efl_t efl = H5D_CRT_EXT_FILE_LIST_DEF;          /* Default external file list */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_dcrt_reg_prop)

    /* Register the storage layout property */
    if(H5P_register_real(pclass, H5D_CRT_LAYOUT_NAME, H5D_CRT_LAYOUT_SIZE, &layout, NULL, NULL, NULL, NULL, NULL, H5D_CRT_LAYOUT_CMP, NULL) < 0)
       HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the fill value property */
    if(H5P_register_real(pclass, H5D_CRT_FILL_VALUE_NAME, H5D_CRT_FILL_VALUE_SIZE, &fill, NULL, NULL, NULL, NULL, NULL, H5D_CRT_FILL_VALUE_CMP, NULL) < 0)
       HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the space allocation time state property */
    if(H5P_register_real(pclass, H5D_CRT_ALLOC_TIME_STATE_NAME, H5D_CRT_ALLOC_TIME_STATE_SIZE, &alloc_time_state, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the external file list property */
    if(H5P_register_real(pclass, H5D_CRT_EXT_FILE_LIST_NAME, H5D_CRT_EXT_FILE_LIST_SIZE, &efl, NULL, NULL, NULL, NULL, NULL, H5D_CRT_EXT_FILE_LIST_CMP, NULL) < 0)
       HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_dcrt_reg_prop() */


/*-------------------------------------------------------------------------
 * Function:       H5P_dcrt_copy
 *
 * Purpose:        Callback routine which is called whenever any dataset
 *                 creation property list is copied.  This routine copies
 *                 the properties from the old list to the new list.
 *
 * Return:         Success:        Non-negative
 *                 Failure:        Negative
 *
 * Programmer:     Raymond Lu
 *                 Tuesday, October 2, 2001
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5P_dcrt_copy(hid_t dst_plist_id, hid_t src_plist_id, void UNUSED *copy_data)
{
    H5O_fill_t     src_fill, dst_fill;          /* Source & destination fill values */
    H5O_efl_t      src_efl, dst_efl;            /* Source & destination external file lists */
    H5O_layout_t   src_layout, dst_layout;      /* Source & destination layout */
    H5P_genplist_t *src_plist;                  /* Pointer to source property list */
    H5P_genplist_t *dst_plist;                  /* Pointer to destination property list */
    herr_t         ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_dcrt_copy)

    /* Verify property list IDs */
    if(NULL == (dst_plist = (H5P_genplist_t *)H5I_object(dst_plist_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset creation property list")
    if(NULL == (src_plist = (H5P_genplist_t *)H5I_object(src_plist_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset creation property list")

    /* Get the layout, fill value, external file list, and data pipeline
     *  properties from the old property list
     */
    if(H5P_get(src_plist, H5D_CRT_LAYOUT_NAME, &src_layout) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get layout")
    if(H5P_get(src_plist, H5D_CRT_FILL_VALUE_NAME, &src_fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value")
    if(H5P_get(src_plist, H5D_CRT_EXT_FILE_LIST_NAME, &src_efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get external file list")

    /* Make copy of layout */
    if(NULL == H5O_msg_copy(H5O_LAYOUT_ID, &src_layout, &dst_layout))
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't copy layout")

    /* Reset layout values set when dataset is created */
    dst_layout.ops = NULL;
    switch(dst_layout.type) {
        case H5D_COMPACT:
            dst_layout.storage.u.compact.buf = H5MM_xfree(dst_layout.storage.u.compact.buf);
            HDmemset(&dst_layout.storage.u.compact, 0, sizeof(dst_layout.storage.u.compact));
            break;

        case H5D_CONTIGUOUS:
            dst_layout.storage.u.contig.addr = HADDR_UNDEF;
            dst_layout.storage.u.contig.size = 0;
            break;

        case H5D_CHUNKED:
            /* Reset chunk size */
            dst_layout.u.chunk.size = 0;

            /* Reset index info, if the chunk ops are set */
            if(dst_layout.storage.u.chunk.ops)
		/* Reset address and pointer of the array struct for the chunked storage index */
                if(H5D_chunk_idx_reset(&dst_layout.storage.u.chunk, TRUE) < 0)
                    HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "unable to reset chunked storage index in dest")

            /* Reset chunk index ops */
            dst_layout.storage.u.chunk.ops = NULL;
            break;

        default:
            HDassert(0 && "Unknown layout type!");
    } /* end switch */

    /* Make copy of fill value */
    if(NULL == H5O_msg_copy(H5O_FILL_ID, &src_fill, &dst_fill))
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't copy fill value")

    /* Make copy of external file list */
    HDmemset(&dst_efl, 0, sizeof(H5O_efl_t));
    if(NULL == H5O_msg_copy(H5O_EFL_ID, &src_efl, &dst_efl))
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't copy external file list")

    /* Reset efl name_offset and heap_addr, these are the values when the dataset is created */
    if(dst_efl.slot) {
        unsigned int i;

        dst_efl.heap_addr = HADDR_UNDEF;
        for(i = 0; i < dst_efl.nused; i++)
            dst_efl.slot[i].name_offset = 0;
    } /* end if */

    /* Set the layout, fill value, external file list, and data pipeline
     *  properties for the destination property list
     */
    if(H5P_set(dst_plist, H5D_CRT_LAYOUT_NAME, &dst_layout) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set layout")
    if(H5P_set(dst_plist, H5D_CRT_FILL_VALUE_NAME, &dst_fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set fill value")
    if(H5P_set(dst_plist, H5D_CRT_EXT_FILE_LIST_NAME, &dst_efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set external file list")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_dcrt_copy() */


/*-------------------------------------------------------------------------
 * Function:	H5P_dcrt_close
 *
 * Purpose:	Callback routine which is called whenever any dataset create
 *              property list is closed.  This routine performs any generic
 *              cleanup needed on the properties the library put into the list.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, July 11, 2001
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5P_dcrt_close(hid_t dcpl_id, void UNUSED *close_data)
{
    H5O_fill_t      fill;               /* Fill value */
    H5O_efl_t       efl;                /* External file list */
    H5P_genplist_t *plist;              /* Property list */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_dcrt_close)

    /* Check arguments */
    if(NULL == (plist = (H5P_genplist_t *)H5I_object(dcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset creation property list")

    /* Get the fill value, external file list, and data pipeline properties
     * from the old property list */
    if(H5P_get(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value")
    if(H5P_get(plist, H5D_CRT_EXT_FILE_LIST_NAME, &efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get external file list")

    /* Clean up any values set for the fill-value and external file-list */
    if(H5O_msg_reset(H5O_FILL_ID, &fill) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "can't release fill info")
    if(H5O_msg_reset(H5O_EFL_ID, &efl) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "can't release external file list info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_dcrt_close() */


/*-------------------------------------------------------------------------
 * Function:       H5P_dcrt_layout_cmp
 *
 * Purpose:        Callback routine which is called whenever the layout
 *                 property in the dataset creation property list is
 *                 compared.
 *
 * Return:         positive if VALUE1 is greater than VALUE2, negative if
 *                      VALUE2 is greater than VALUE1 and zero if VALUE1 and
 *                      VALUE2 are equal.
 *
 * Programmer:     Quincey Koziol
 *                 Tuesday, December 23, 2008
 *
 *-------------------------------------------------------------------------
 */
static int
H5P_dcrt_layout_cmp(const void *_layout1, const void *_layout2, size_t UNUSED size)
{
    const H5O_layout_t *layout1 = (const H5O_layout_t *)_layout1,     /* Create local aliases for values */
        *layout2 = (const H5O_layout_t *)_layout2;
    herr_t ret_value = 0;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_dcrt_layout_cmp)

    /* Sanity check */
    HDassert(layout1);
    HDassert(layout1);
    HDassert(size == sizeof(H5O_layout_t));

    /* Check for different layout type */
    if(layout1->type < layout2->type) HGOTO_DONE(-1)
    if(layout1->type > layout2->type) HGOTO_DONE(1)

    /* Check for different layout version */
    if(layout1->version < layout2->version) HGOTO_DONE(-1)
    if(layout1->version > layout2->version) HGOTO_DONE(1)

    /* Compare non-dataset-specific fields in layout info */
    switch(layout1->type) {
        case H5D_COMPACT:
        case H5D_CONTIGUOUS:
            break;

        case H5D_CHUNKED:
            {
                unsigned u;     /* Local index variable */

                /* Check the number of dimensions */
                if(layout1->u.chunk.ndims < layout2->u.chunk.ndims) HGOTO_DONE(-1)
                if(layout1->u.chunk.ndims > layout2->u.chunk.ndims) HGOTO_DONE(1)

                /* Compare the chunk dims */
                for(u = 0; u < layout1->u.chunk.ndims - 1; u++) {
                    if(layout1->u.chunk.dim[u] < layout2->u.chunk.dim[u]) HGOTO_DONE(-1)
                    if(layout1->u.chunk.dim[u] > layout2->u.chunk.dim[u]) HGOTO_DONE(1)
                } /* end for */
            } /* end case */
            break;

        default:
            HDassert(0 && "Unknown layout type!");
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_dcrt_layout_cmp() */


/*-------------------------------------------------------------------------
 * Function:       H5P_fill_value_cmp
 *
 * Purpose:        Callback routine which is called whenever the fill value
 *                 property in the dataset creation property list is compared.
 *
 * Return:         positive if VALUE1 is greater than VALUE2, negative if
 *                      VALUE2 is greater than VALUE1 and zero if VALUE1 and
 *                      VALUE2 are equal.
 *
 * Programmer:     Quincey Koziol
 *                 Wednesday, January 7, 2004
 *
 *-------------------------------------------------------------------------
 */
int
H5P_fill_value_cmp(const void *_fill1, const void *_fill2, size_t UNUSED size)
{
    const H5O_fill_t *fill1 = (const H5O_fill_t *)_fill1,     /* Create local aliases for values */
        *fill2 = (const H5O_fill_t *)_fill2;
    int cmp_value;              /* Value from comparison */
    herr_t ret_value = 0;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_fill_value_cmp)

    /* Sanity check */
    HDassert(fill1);
    HDassert(fill2);
    HDassert(size == sizeof(H5O_fill_t));

    /* Check the size of fill values */
    if(fill1->size < fill2->size) HGOTO_DONE(-1);
    if(fill1->size > fill2->size) HGOTO_DONE(1);

    /* Check the types of the fill values */
    if(fill1->type == NULL && fill2->type != NULL) HGOTO_DONE(-1);
    if(fill1->type != NULL && fill2->type == NULL) HGOTO_DONE(1);
    if(fill1->type != NULL)
        if((cmp_value = H5T_cmp(fill1->type, fill2->type, FALSE)) != 0)
            HGOTO_DONE(cmp_value);

    /* Check the fill values in the buffers */
    if(fill1->buf == NULL && fill2->buf != NULL) HGOTO_DONE(-1);
    if(fill1->buf != NULL && fill2->buf == NULL) HGOTO_DONE(1);
    if(fill1->buf != NULL)
        if((cmp_value = HDmemcmp(fill1->buf, fill2->buf, (size_t)fill1->size)) != 0)
            HGOTO_DONE(cmp_value);

    /* Check the allocation time for the fill values */
    if(fill1->alloc_time < fill2->alloc_time) HGOTO_DONE(-1);
    if(fill1->alloc_time > fill2->alloc_time) HGOTO_DONE(1);

    /* Check the fill time for the fill values */
    if(fill1->fill_time < fill2->fill_time) HGOTO_DONE(-1);
    if(fill1->fill_time > fill2->fill_time) HGOTO_DONE(1);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_fill_value_cmp() */


/*-------------------------------------------------------------------------
 * Function:       H5P_dcrt_ext_file_list_cmp
 *
 * Purpose:        Callback routine which is called whenever the external file
 *                 list property in the dataset creation property list is
 *                 compared.
 *
 * Return:         positive if VALUE1 is greater than VALUE2, negative if
 *                      VALUE2 is greater than VALUE1 and zero if VALUE1 and
 *                      VALUE2 are equal.
 *
 * Programmer:     Quincey Koziol
 *                 Wednesday, January 7, 2004
 *
 *-------------------------------------------------------------------------
 */
static int
H5P_dcrt_ext_file_list_cmp(const void *_efl1, const void *_efl2, size_t UNUSED size)
{
    const H5O_efl_t *efl1 = (const H5O_efl_t *)_efl1,     /* Create local aliases for values */
        *efl2 = (const H5O_efl_t *)_efl2;
    int cmp_value;              /* Value from comparison */
    herr_t ret_value = 0;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_dcrt_ext_file_list_cmp)

    /* Sanity check */
    HDassert(efl1);
    HDassert(efl2);
    HDassert(size == sizeof(H5O_efl_t));

    /* Check the heap address of external file lists */
    if(H5F_addr_defined(efl1->heap_addr) || H5F_addr_defined(efl2->heap_addr)) {
        if(!H5F_addr_defined(efl1->heap_addr) && H5F_addr_defined(efl2->heap_addr)) HGOTO_DONE(-1);
        if(H5F_addr_defined(efl1->heap_addr) && !H5F_addr_defined(efl2->heap_addr)) HGOTO_DONE(1);
        if((cmp_value = H5F_addr_cmp(efl1->heap_addr, efl2->heap_addr)) != 0)
            HGOTO_DONE(cmp_value);
    } /* end if */

    /* Check the number of allocated efl entries */
    if(efl1->nalloc < efl2->nalloc) HGOTO_DONE(-1);
    if(efl1->nalloc > efl2->nalloc) HGOTO_DONE(1);

    /* Check the number of used efl entries */
    if(efl1->nused < efl2->nused) HGOTO_DONE(-1);
    if(efl1->nused > efl2->nused) HGOTO_DONE(1);

    /* Check the efl entry information */
    if(efl1->slot == NULL && efl2->slot != NULL) HGOTO_DONE(-1);
    if(efl1->slot != NULL && efl2->slot == NULL) HGOTO_DONE(1);
    if(efl1->slot != NULL && efl1->nused > 0) {
        size_t u;       /* Local index variable */

        /* Loop through all entries, comparing them */
        for(u = 0; u < efl1->nused; u++) {
            /* Check the name offset of the efl entry */
            if(efl1->slot[u].name_offset < efl2->slot[u].name_offset) HGOTO_DONE(-1);
            if(efl1->slot[u].name_offset > efl2->slot[u].name_offset) HGOTO_DONE(1);

            /* Check the name of the efl entry */
            if(efl1->slot[u].name == NULL && efl2->slot[u].name != NULL) HGOTO_DONE(-1);
            if(efl1->slot[u].name != NULL && efl2->slot[u].name == NULL) HGOTO_DONE(1);
            if(efl1->slot[u].name != NULL)
                if((cmp_value = HDstrcmp(efl1->slot[u].name, efl2->slot[u].name)) != 0)
                    HGOTO_DONE(cmp_value);

            /* Check the file offset of the efl entry */
            if(efl1->slot[u].offset < efl2->slot[u].offset) HGOTO_DONE(-1);
            if(efl1->slot[u].offset > efl2->slot[u].offset) HGOTO_DONE(1);

            /* Check the file size of the efl entry */
            if(efl1->slot[u].size < efl2->slot[u].size) HGOTO_DONE(-1);
            if(efl1->slot[u].size > efl2->slot[u].size) HGOTO_DONE(1);
        } /* end for */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_dcrt_ext_file_list_cmp() */


/*-------------------------------------------------------------------------
 * Function:  H5P_set_layout
 *
 * Purpose:   Sets the layout of raw data in the file.
 *
 * Return:    Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, November 23, 2004
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5P_set_layout(H5P_genplist_t *plist, const H5O_layout_t *layout)
{
    unsigned alloc_time_state;          /* State of allocation time property */
    herr_t ret_value = SUCCEED;         /* return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_set_layout)

    /* Get the allocation time state */
    if(H5P_get(plist, H5D_CRT_ALLOC_TIME_STATE_NAME, &alloc_time_state) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get space allocation time state")

    /* If we still have the "default" allocation time, change it according to the new layout */
    if(alloc_time_state) {
        H5O_fill_t fill;            /* Fill value */

        /* Get current fill value info */
        if(H5P_get(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value")

        /* Set the default based on layout */
        switch(layout->type) {
            case H5D_COMPACT:
                fill.alloc_time = H5D_ALLOC_TIME_EARLY;
                break;

            case H5D_CONTIGUOUS:
                fill.alloc_time = H5D_ALLOC_TIME_LATE;
                break;

            case H5D_CHUNKED:
                fill.alloc_time = H5D_ALLOC_TIME_INCR;
                break;

            default:
                HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "unknown layout type")
        } /* end switch */

        /* Set updated fill value info */
        if(H5P_set(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set space allocation time")
    } /* end if */

    /* Set layout value */
    if(H5P_set(plist, H5D_CRT_LAYOUT_NAME, layout) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't set layout")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_set_layout() */

#ifndef H5_HAVE_C99_DESIGNATED_INITIALIZER

/*-------------------------------------------------------------------------
 * Function:  H5P_init_def_layout
 *
 * Purpose:   Set the default layout information for the various types of
 *              dataset layouts
 *
 * Return:    Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, January 13, 2009
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5P_init_def_layout(void)
{
    const H5O_layout_chunk_t def_layout_chunk = H5D_DEF_LAYOUT_CHUNK_INIT;
    const H5O_storage_compact_t def_store_compact = H5D_DEF_STORAGE_COMPACT_INIT;
    const H5O_storage_chunk_t def_store_chunk = H5D_DEF_STORAGE_CHUNK_INIT;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_init_def_layout)

    /* Initialize the default layout info for non-contigous layouts */
    H5D_def_layout_compact_g.storage.u.compact = def_store_compact;
    H5D_def_layout_chunk_g.u.chunk = def_layout_chunk;
    H5D_def_layout_chunk_g.storage.u.chunk = def_store_chunk;

    /* Note that we've initialized the default values */
    H5P_dcrt_def_layout_init_g = TRUE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5P_init_def_layout() */
#endif /* H5_HAVE_C99_DESIGNATED_INITIALIZER */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_layout
 *
 * Purpose:	Sets the layout of raw data in the file.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Tuesday, January  6, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and set property for
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_layout(hid_t plist_id, H5D_layout_t layout_type)
{
    H5P_genplist_t *plist;              /* Property list pointer */
    const H5O_layout_t *layout;         /* Pointer to default layout information for type specified */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_API(H5Pset_layout, FAIL)
    H5TRACE2("e", "iDl", plist_id, layout_type);

    /* Check arguments */
    if(layout_type < 0 || layout_type >= H5D_NLAYOUTS)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "raw data layout method is not valid")

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

#ifndef H5_HAVE_C99_DESIGNATED_INITIALIZER
    /* If the compiler doesn't support C99 designated initializers, check if
     *  the default layout structs have been initialized yet or not.  *ick* -QAK
     */
    if(!H5P_dcrt_def_layout_init_g)
        if(H5P_init_def_layout() < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't initialize default layout info")
#endif /* H5_HAVE_C99_DESIGNATED_INITIALIZER */

    /* Get pointer to correct default layout */
    switch(layout_type) {
        case H5D_COMPACT:
            layout = &H5D_def_layout_compact_g;
            break;

        case H5D_CONTIGUOUS:
            layout = &H5D_def_layout_contig_g;
            break;

        case H5D_CHUNKED:
            layout = &H5D_def_layout_chunk_g;
            break;

        default:
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "unknown layout type")
    } /* end switch */

    /* Set value */
    if(H5P_set_layout(plist, layout) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't set layout")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_layout() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_layout
 *
 * Purpose:	Retrieves layout type of a dataset creation property list.
 *
 * Return:	Success:	The layout type
 *
 *		Failure:	H5D_LAYOUT_ERROR (negative)
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and get property for
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
H5D_layout_t
H5Pget_layout(hid_t plist_id)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    H5O_layout_t layout;        /* Layout property */
    H5D_layout_t ret_value;     /* Return value */

    FUNC_ENTER_API(H5Pget_layout, H5D_LAYOUT_ERROR)
    H5TRACE1("Dl", "i", plist_id);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, H5D_LAYOUT_ERROR, "can't find object for ID")

    /* Get layout property */
    if(H5P_get(plist, H5D_CRT_LAYOUT_NAME, &layout) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, H5D_LAYOUT_ERROR, "can't get layout")

    /* Set return value */
    ret_value = layout.type;

done:
    FUNC_LEAVE_API(ret_value)
} /* ed H5Pget_layout() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_chunk
 *
 * Purpose:	Sets the number of dimensions and the size of each chunk to
 *		the values specified.  The dimensionality of the chunk should
 *		match the dimensionality of the dataspace.
 *
 *		As a side effect, the layout method is changed to
 *		H5D_CHUNKED.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Tuesday, January  6, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and set property for
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_chunk(hid_t plist_id, int ndims, const hsize_t dim[/*ndims*/])
{
    H5P_genplist_t *plist;      /* Property list pointer */
    H5O_layout_t chunk_layout;  /* Layout information for setting chunk info */
    uint64_t chunk_nelmts;      /* Number of elements in chunk */
    unsigned u;                 /* Local index variable */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(H5Pset_chunk, FAIL)
    H5TRACE3("e", "iIs*[a1]h", plist_id, ndims, dim);

    /* Check arguments */
    if(ndims <= 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "chunk dimensionality must be positive")
    if(ndims > H5S_MAX_RANK)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "chunk dimensionality is too large")
    if(!dim)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no chunk dimensions specified")

#ifndef H5_HAVE_C99_DESIGNATED_INITIALIZER
    /* If the compiler doesn't support C99 designated initializers, check if
     *  the default layout structs have been initialized yet or not.  *ick* -QAK
     */
    if(!H5P_dcrt_def_layout_init_g)
        if(H5P_init_def_layout() < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't initialize default layout info")
#endif /* H5_HAVE_C99_DESIGNATED_INITIALIZER */

    /* Verify & initialize property's chunk dims */
    HDmemcpy(&chunk_layout, &H5D_def_layout_chunk_g, sizeof(H5D_def_layout_chunk_g));
    HDmemset(&chunk_layout.u.chunk.dim, 0, sizeof(chunk_layout.u.chunk.dim));
    chunk_nelmts = 1;
    for(u = 0; u < (unsigned)ndims; u++) {
        if(dim[u] == 0)
            HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "all chunk dimensions must be positive")
        if(dim[u] != (dim[u] & 0xffffffff))
            HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "all chunk dimensions must be less than 2^32")
        chunk_nelmts *= dim[u];
        if(chunk_nelmts > (uint64_t)0xffffffff)
            HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "number of elements in chunk must be < 4GB")
        chunk_layout.u.chunk.dim[u] = (uint32_t)dim[u]; /* Store user's chunk dimensions */
    } /* end for */

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Set chunk information in property list */
    chunk_layout.u.chunk.ndims = (unsigned)ndims;
    if(H5P_set_layout(plist, &chunk_layout) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set layout")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_chunk() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_chunk
 *
 * Purpose:	Retrieves the chunk size of chunked layout.  The chunk
 *		dimensionality is returned and the chunk size in each
 *		dimension is returned through the DIM argument.	 At most
 *		MAX_NDIMS elements of DIM will be initialized.
 *
 * Return:	Success:	Positive Chunk dimensionality.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and set property for
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
int
H5Pget_chunk(hid_t plist_id, int max_ndims, hsize_t dim[]/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    H5O_layout_t layout;        /* Layout information */
    int ret_value;              /* Return value */

    FUNC_ENTER_API(H5Pget_chunk, FAIL)
    H5TRACE3("Is", "iIsx", plist_id, max_ndims, dim);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Retrieve the layout property */
    if(H5P_get(plist, H5D_CRT_LAYOUT_NAME, &layout) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "can't get layout")
    if(H5D_CHUNKED != layout.type)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a chunked storage layout")

    if(dim) {
        unsigned	u;      /* Local index variable */

        /* Get the dimension sizes */
        for(u = 0; u < layout.u.chunk.ndims && u < (unsigned)max_ndims; u++)
            dim[u] = layout.u.chunk.dim[u];
    } /* end if */

    /* Set the return value */
    ret_value = (int)layout.u.chunk.ndims;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_chunk() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_external
 *
 * Purpose:	Adds an external file to the list of external files. PLIST_ID
 *		should be an object ID for a dataset creation property list.
 *		NAME is the name of an external file, OFFSET is the location
 *		where the data starts in that file, and SIZE is the number of
 *		bytes reserved in the file for the data.
 *
 *		If a dataset is split across multiple files then the files
 *		should be defined in order. The total size of the dataset is
 *		the sum of the SIZE arguments for all the external files.  If
 *		the total size is larger than the size of a dataset then the
 *		dataset can be extended (provided the dataspace also allows
 *		the extending).
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Tuesday, March	3, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and set property for
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_external(hid_t plist_id, const char *name, off_t offset, hsize_t size)
{
    size_t		idx;
    hsize_t		total, tmp;
    H5O_efl_t           efl;
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Pset_external, FAIL)
    H5TRACE4("e", "i*soh", plist_id, name, offset, size);

    /* Check arguments */
    if (!name || !*name)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "no name given")
    if (offset<0)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "negative external file offset")
    if (size<=0)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "zero size")

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    if(H5P_get(plist, H5D_CRT_EXT_FILE_LIST_NAME, &efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get external file list")
    if(efl.nused > 0 && H5O_EFL_UNLIMITED==efl.slot[efl.nused-1].size)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "previous file size is unlimited")

    if (H5O_EFL_UNLIMITED!=size) {
        for (idx=0, total=size; idx<efl.nused; idx++, total=tmp) {
            tmp = total + efl.slot[idx].size;
            if (tmp <= total)
                HGOTO_ERROR (H5E_EFL, H5E_OVERFLOW, FAIL, "total external data size overflowed")
        } /* end for */
    } /* end if */


    /* Add to the list */
    if(efl.nused >= efl.nalloc) {
        size_t na = efl.nalloc + H5O_EFL_ALLOC;
        H5O_efl_entry_t *x = (H5O_efl_entry_t *)H5MM_realloc(efl.slot, na * sizeof(H5O_efl_entry_t));

        if(!x)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
        efl.nalloc = na;
        efl.slot = x;
    } /* end if */
    idx = efl.nused;
    efl.slot[idx].name_offset = 0; /*not entered into heap yet*/
    efl.slot[idx].name = H5MM_xstrdup (name);
    efl.slot[idx].offset = offset;
    efl.slot[idx].size = size;
    efl.nused++;

    if(H5P_set(plist, H5D_CRT_EXT_FILE_LIST_NAME, &efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't set external file list")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_external_count
 *
 * Purpose:	Returns the number of external files for this dataset.
 *
 * Return:	Success:	Number of external files
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Tuesday, March  3, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and set property for
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
int
H5Pget_external_count(hid_t plist_id)
{
    H5O_efl_t           efl;
    H5P_genplist_t *plist;      /* Property list pointer */
    int ret_value;              /* return value */

    FUNC_ENTER_API(H5Pget_external_count, FAIL)
    H5TRACE1("Is", "i", plist_id);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Get value */
    if(H5P_get(plist, H5D_CRT_EXT_FILE_LIST_NAME, &efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get external file list")

    /* Set return value */
    ret_value = (int)efl.nused;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_external_count() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_external
 *
 * Purpose:	Returns information about an external file.  External files
 *		are numbered from zero to N-1 where N is the value returned
 *		by H5Pget_external_count().  At most NAME_SIZE characters are
 *		copied into the NAME array.  If the external file name is
 *		longer than NAME_SIZE with the null terminator, then the
 *		return value is not null terminated (similar to strncpy()).
 *
 *		If NAME_SIZE is zero or NAME is the null pointer then the
 *		external file name is not returned.  If OFFSET or SIZE are
 *		null pointers then the corresponding information is not
 *		returned.
 *
 * See Also:	H5Pset_external()
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Tuesday, March  3, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and get property for
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_external(hid_t plist_id, unsigned idx, size_t name_size, char *name/*out*/,
		 off_t *offset/*out*/, hsize_t *size/*out*/)
{
    H5O_efl_t           efl;
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pget_external, FAIL)
    H5TRACE6("e", "iIuzxxx", plist_id, idx, name_size, name, offset, size);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Get value */
    if(H5P_get(plist, H5D_CRT_EXT_FILE_LIST_NAME, &efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get external file list")

    if(idx >= efl.nused)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "external file index is out of range")

    /* Return values */
    if(name_size>0 && name)
        HDstrncpy(name, efl.slot[idx].name, name_size);
    if(offset)
        *offset = efl.slot[idx].offset;
    if(size)
        *size = efl.slot[idx].size;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_external() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_szip
 *
 * Purpose:	Sets the compression method for a permanent or transient
 *		filter pipeline (depending on whether PLIST_ID is a dataset
 *		creation or transfer property list) to H5Z_FILTER_SZIP
 *		Szip is a special compression package that is said to be good
 *              for scientific data.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Kent Yang
 *              Tuesday, April 1, 2003
 *
 * Modifications:
 *
 *          Nat Furrer and James Laird
 *          June 30, 2004
 *          Now ensures that SZIP encoding is enabled
 *          SZIP defaults to k13 compression
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_szip(hid_t plist_id, unsigned options_mask, unsigned pixels_per_block)
{
    H5O_pline_t         pline;
    H5P_genplist_t *plist;      /* Property list pointer */
    unsigned cd_values[2];      /* Filter parameters */
    unsigned int config_flags;
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_szip, FAIL)
    H5TRACE3("e", "iIuIu", plist_id, options_mask, pixels_per_block);

    if(H5Zget_filter_info(H5Z_FILTER_SZIP, &config_flags) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "can't get filter info")

    if(! (config_flags & H5Z_FILTER_CONFIG_ENCODE_ENABLED))
        HGOTO_ERROR(H5E_PLINE, H5E_NOENCODER, FAIL, "Filter present but encoding is disabled.")

    /* Check arguments */
    if ((pixels_per_block%2)==1)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "pixels_per_block is not even")
    if (pixels_per_block>H5_SZIP_MAX_PIXELS_PER_BLOCK)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "pixels_per_block is too large")

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Always set K13 compression (and un-set CHIP compression) */
    options_mask &= (~H5_SZIP_CHIP_OPTION_MASK);
    options_mask |= H5_SZIP_ALLOW_K13_OPTION_MASK;

    /* Always set "raw" (no szip header) flag for data */
    options_mask |= H5_SZIP_RAW_OPTION_MASK;

    /* Mask off the LSB and MSB options, if they were given */
    /* (The HDF5 library sets them internally, as needed) */
    options_mask &= ~(H5_SZIP_LSB_OPTION_MASK|H5_SZIP_MSB_OPTION_MASK);

    /* Set the parameters for the filter */
    cd_values[0]=options_mask;
    cd_values[1]=pixels_per_block;

    /* Add the filter */
    if(H5P_get(plist, H5O_CRT_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get pipeline")
    if(H5Z_append(&pline, H5Z_FILTER_SZIP, H5Z_FLAG_OPTIONAL, (size_t)2, cd_values) < 0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to add szip filter to pipeline")
    if(H5P_set(plist, H5O_CRT_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to set pipeline")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_szip() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_shuffle
 *
 * Purpose:	Sets the shuffling method for a permanent
 *		filter to H5Z_FILTER_SHUFFLE
 *		and bytes of the datatype of the array to be shuffled
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Kent Yang
 *              Wednesday, November 13, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_shuffle(hid_t plist_id)
{
    H5O_pline_t         pline;
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_shuffle, FAIL)
    H5TRACE1("e", "i", plist_id);

    /* Check arguments */
    if(TRUE != H5P_isa_class(plist_id, H5P_DATASET_CREATE))
        HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset creation property list")

    /* Get the plist structure */
    if(NULL == (plist = (H5P_genplist_t *)H5I_object(plist_id)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Add the filter */
    if(H5P_get(plist, H5O_CRT_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get pipeline")
    if(H5Z_append(&pline, H5Z_FILTER_SHUFFLE, H5Z_FLAG_OPTIONAL, (size_t)0, NULL) < 0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to shuffle the data")
    if(H5P_set(plist, H5O_CRT_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to set pipeline")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_shuffle() */


/*-------------------------------------------------------------------------
 * Function:    H5Pset_nbit
 *
 * Purpose:     Sets nbit filter for a dataset creation property list
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Xiaowen Wu
 *              Wednesday, December 22, 2004
 *
 * Modifications:
 *
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_nbit(hid_t plist_id)
{
    H5O_pline_t         pline;
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_nbit, FAIL)
    H5TRACE1("e", "i", plist_id);

    /* Check arguments */
    if(TRUE != H5P_isa_class(plist_id, H5P_DATASET_CREATE))
        HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset creation property list")

    /* Get the plist structure */
    if(NULL == (plist = (H5P_genplist_t *)H5I_object(plist_id)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Add the nbit filter */
    if(H5P_get(plist, H5O_CRT_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get pipeline")
    if(H5Z_append(&pline, H5Z_FILTER_NBIT, H5Z_FLAG_OPTIONAL, (size_t)0, NULL) < 0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to add nbit filter to pipeline")
    if(H5P_set(plist, H5O_CRT_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to set pipeline")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_nbit() */


/*-------------------------------------------------------------------------
 * Function:    H5Pset_scaleoffset
 *
 * Purpose:     Sets scaleoffset filter for a dataset creation property list
 *              and user-supplied parameters
 *
 * Parameters:  scale_factor:
                              for integer datatype,
                              this parameter will be
                              minimum-bits, if this value is set to 0,
                              scaleoffset filter will calculate the minimum-bits.

                              For floating-point datatype,
                              For variable-minimum-bits method, this will be
                              the decimal precision of the filter,
                              For fixed-minimum-bits method, this will be
                              the minimum-bit of the filter.
                scale_type:   0 for floating-point variable-minimum-bits,
                              1 for floating-point fixed-minimum-bits,
                              other values, for integer datatype

 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Xiaowen Wu
 *              Thursday, April 14, 2005
 *
 * Modifications:
 *
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_scaleoffset(hid_t plist_id, H5Z_SO_scale_type_t scale_type, int scale_factor)
{
    H5O_pline_t         pline;
    H5P_genplist_t *plist;      /* Property list pointer */
    unsigned cd_values[2];      /* Filter parameters */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_scaleoffset, FAIL)
    H5TRACE3("e", "iZaIs", plist_id, scale_type, scale_factor);

    /* Check arguments */
    if(TRUE != H5P_isa_class(plist_id, H5P_DATASET_CREATE))
        HGOTO_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset creation property list")

    if(scale_factor < 0)
       HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "scale factor must be > 0")
    if(scale_type!=H5Z_SO_FLOAT_DSCALE && scale_type!=H5Z_SO_FLOAT_ESCALE && scale_type!=H5Z_SO_INT)
       HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid scale type")

    /* Get the plist structure */
    if(NULL == (plist = (H5P_genplist_t *)H5I_object(plist_id)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Set parameters for the filter
     * scale_type = 0:     floating-point type, filter uses variable-minimum-bits method,
     *                     scale_factor is decimal scale factor
     * scale_type = 1:     floating-point type, filter uses fixed-minimum-bits method,
     *                     scale_factor is the fixed minimum number of bits
     * scale type = other: integer type, scale_factor is minimum number of bits
     *                     if scale_factor = 0, then filter calculates minimum number of bits
     */
    cd_values[0] = scale_type;
    cd_values[1] = (unsigned)scale_factor;

    /* Add the scaleoffset filter */
    if(H5P_get(plist, H5O_CRT_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get pipeline")
    if(H5Z_append(&pline, H5Z_FILTER_SCALEOFFSET, H5Z_FLAG_OPTIONAL, (size_t)2, cd_values) < 0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to add scaleoffset filter to pipeline")
    if(H5P_set(plist, H5O_CRT_PIPELINE_NAME, &pline) < 0)
        HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to set pipeline")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_scaleoffset() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_fill_value
 *
 * Purpose:	Set the fill value for a dataset creation property list. The
 *		VALUE is interpretted as being of type TYPE, which need not
 *		be the same type as the dataset but the library must be able
 *		to convert VALUE to the dataset type when the dataset is
 *		created.  If VALUE is NULL, it will be interpreted as
 *		undefining fill value.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, October  1, 1998
 *
 * Modifications:
 *
 *              Raymond Lu
 *              Tuesday, October 2, 2001
 *              Changed the way to check parameter and set property for
 *              generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fill_value(hid_t plist_id, hid_t type_id, const void *value)
{
    H5P_genplist_t *plist;              /* Property list pointer */
    H5O_fill_t fill;                    /* Fill value to modify */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_API(H5Pset_fill_value, FAIL)
    H5TRACE3("e", "ii*x", plist_id, type_id, value);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Get the current fill value */
    if(H5P_get(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value")

    /* Release the dynamic fill value components */
    H5O_fill_reset_dyn(&fill);

    if(value) {
        H5T_t *type;            /* Datatype for fill value */
        H5T_path_t *tpath;      /* Conversion information */

        /* Retrieve pointer to datatype */
	if(NULL == (type = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")

	/* Set the fill value */
        if(NULL == (fill.type = H5T_copy(type, H5T_COPY_TRANSIENT)))
            HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, FAIL, "can't copy datatype")
        fill.size = (ssize_t)H5T_get_size(type);
        if(NULL == (fill.buf = H5MM_malloc((size_t)fill.size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "memory allocation failed for fill value")
        HDmemcpy(fill.buf, value, (size_t)fill.size);

        /* Set up type conversion function */
        if(NULL == (tpath = H5T_path_find(type, type, NULL, NULL, H5AC_ind_dxpl_id, FALSE)))
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "unable to convert between src and dest data types")

        /* If necessary, convert fill value datatypes (which copies VL components, etc.) */
        if(!H5T_path_noop(tpath)) {
            uint8_t *bkg_buf = NULL;    /* Background conversion buffer */

            /* Allocate a background buffer */
            if(H5T_path_bkg(tpath) && NULL == (bkg_buf = H5FL_BLK_CALLOC(type_conv, (size_t)fill.size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

            /* Convert the fill value */
            if(H5T_convert(tpath, type_id, type_id, (size_t)1, (size_t)0, (size_t)0, fill.buf, bkg_buf, H5AC_ind_dxpl_id) < 0) {
                if(bkg_buf)
                    bkg_buf = H5FL_BLK_FREE(type_conv, bkg_buf);
                HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "datatype conversion failed")
            } /* end if */

            /* Release the background buffer */
            if(bkg_buf)
                bkg_buf = H5FL_BLK_FREE(type_conv, bkg_buf);
        } /* end if */
    }  /* end if */
    else
        fill.size = (-1);

    /* Update fill value in property list */
    if(H5P_set(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't set fill value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_fill_value() */


/*-------------------------------------------------------------------------
 * Function:	H5P_get_fill_value
 *
 * Purpose:	Queries the fill value property of a dataset creation
 *		property list.  The fill value is returned through the VALUE
 *		pointer and the memory is allocated by the caller.  The fill
 *		value will be converted from its current datatype to the
 *		specified TYPE.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, October 17, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5P_get_fill_value(H5P_genplist_t *plist, const H5T_t *type, void *value/*out*/,
    hid_t dxpl_id)
{
    H5O_fill_t          fill;                   /* Fill value to retrieve */
    H5T_path_t		*tpath;		        /*type conversion info	*/
    void		*buf = NULL;		/*conversion buffer	*/
    void		*bkg = NULL;		/*conversion buffer	*/
    hid_t		src_id = -1;		/*source datatype id	*/
    hid_t		dst_id = -1;		/*destination datatype id	*/
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5P_get_fill_value, FAIL)

    /*
     * If no fill value is defined then return an error.  We can't even
     * return zero because we don't know the datatype of the dataset and
     * datatype conversion might not have resulted in zero.  If fill value
     * is undefined, also return error.
     */
    if(H5P_get(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value")
    if(fill.size == -1)
	HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "fill value is undefined")

    /* Check for "default" fill value */
    if(fill.size == 0) {
	HDmemset(value, 0, H5T_get_size(type));
   	HGOTO_DONE(SUCCEED);
    } /* end if */

     /*
     * Can we convert between the source and destination datatypes?
     */
    if(NULL == (tpath = H5T_path_find(fill.type, type, NULL, NULL, dxpl_id, FALSE)))
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to convert between src and dst datatypes")
    if((src_id = H5I_register(H5I_DATATYPE, H5T_copy(fill.type, H5T_COPY_TRANSIENT), FALSE)) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to copy/register datatype")

    /*
     * Data type conversions are always done in place, so we need a buffer
     * other than the fill value buffer that is large enough for both source
     * and destination.  The app-supplied buffer might do okay.
     */
    if(H5T_get_size(type) >= H5T_get_size(fill.type)) {
        buf = value;
        if(H5T_path_bkg(tpath) && NULL == (bkg = H5MM_malloc(H5T_get_size(type))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for type conversion")
    } /* end if */
    else {
        if(NULL == (buf = H5MM_malloc(H5T_get_size(fill.type))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for type conversion")
        if(H5T_path_bkg(tpath))
            bkg = value;
    } /* end else */
    HDmemcpy(buf, fill.buf, H5T_get_size(fill.type));

    /* Do the conversion */
    if((dst_id = H5I_register(H5I_DATATYPE, H5T_copy(type, H5T_COPY_TRANSIENT), FALSE)) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to copy/register datatype")
    if(H5T_convert(tpath, src_id, dst_id, (size_t)1, (size_t)0, (size_t)0, buf, bkg, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "datatype conversion failed")
    if(buf != value)
        HDmemcpy(value, buf, H5T_get_size(type));

done:
    if(buf != value)
        H5MM_xfree(buf);
    if(bkg != value)
        H5MM_xfree(bkg);
    if(src_id >= 0)
        H5I_dec_ref(src_id, FALSE);
    if(dst_id >= 0)
        H5I_dec_ref(dst_id, FALSE);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_get_fill_value() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_fill_value
 *
 * Purpose:	Queries the fill value property of a dataset creation
 *		property list.  The fill value is returned through the VALUE
 *		pointer and the memory is allocated by the caller.  The fill
 *		value will be converted from its current datatype to the
 *		specified TYPE.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, October  1, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_fill_value(hid_t plist_id, hid_t type_id, void *value/*out*/)
{
    H5P_genplist_t      *plist;                 /* Property list pointer */
    H5T_t		*type;		        /* Datatype		*/
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(H5Pget_fill_value, FAIL)
    H5TRACE3("e", "iix", plist_id, type_id, value);

    /* Check arguments */
    if(NULL == (type = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
    if(!value)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,"no fill value output buffer")

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Get the fill value */
    if(H5P_get_fill_value(plist, type, value, H5AC_ind_dxpl_id) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_fill_value() */


/*-------------------------------------------------------------------------
 * Function:    H5P_is_fill_value_defined
 *
 * Purpose:	Check if fill value is defined.  Internal version of function
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Raymond Lu
 *              Wednesday, January 16, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5P_is_fill_value_defined(const H5O_fill_t *fill, H5D_fill_value_t *status)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5P_is_fill_value_defined, FAIL)

    HDassert(fill);
    HDassert(status);

    /* Check if the fill value was "unset" */
    if(fill->size == -1 && !fill->buf)
	*status = H5D_FILL_VALUE_UNDEFINED;
    /* Check if the fill value was set to the default fill value by the library */
    else if(fill->size == 0 && !fill->buf)
	*status = H5D_FILL_VALUE_DEFAULT;
    /* Check if the fill value was set by the application */
    else if(fill->size > 0 && fill->buf)
	*status = H5D_FILL_VALUE_USER_DEFINED;
    else {
	*status = H5D_FILL_VALUE_ERROR;
        HGOTO_ERROR(H5E_PLIST, H5E_BADRANGE, FAIL, "invalid combination of fill-value info")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_is_fill_value_defined() */


/*-------------------------------------------------------------------------
 * Function:    H5P_fill_value_defined
 *
 * Purpose:	Check if fill value is defined.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, October 17, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5P_fill_value_defined(H5P_genplist_t *plist, H5D_fill_value_t *status)
{
    H5O_fill_t	        fill;                   /* Fill value to query */
    herr_t		ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5P_fill_value_defined, FAIL)

    HDassert(status);

    /* Get the fill value struct */
    if(H5P_get(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value")

    /* Get the fill-value status */
    if(H5P_is_fill_value_defined(&fill, status) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "can't check fill value status")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_fill_value_defined() */


/*-------------------------------------------------------------------------
 * Function:    H5Pfill_value_defined
 *
 * Purpose:	Check if fill value is defined.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Raymond Lu
 *              Wednesday, January 16, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pfill_value_defined(hid_t plist_id, H5D_fill_value_t *status)
{
    H5P_genplist_t 	*plist;                 /* Property list to query */
    herr_t		ret_value = SUCCEED;

    FUNC_ENTER_API(H5Pfill_value_defined, FAIL)
    H5TRACE2("e", "i*DF", plist_id, status);

    HDassert(status);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Get the fill-value status */
    if(H5P_fill_value_defined(plist, status) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "can't check fill value status")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pfill_value_defined() */


/*-------------------------------------------------------------------------
 * Function:    H5Pset_alloc_time
 *
 * Purpose:     Set space allocation time for dataset during creation.
 *		Valid values are H5D_ALLOC_TIME_DEFAULT, H5D_ALLOC_TIME_EARLY,
 *			H5D_ALLOC_TIME_LATE, H5D_ALLOC_TIME_INCR
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 * 		Wednesday, January 16, 2002
 *
 * Modifications:
 *
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_alloc_time(hid_t plist_id, H5D_alloc_time_t alloc_time)
{
    H5P_genplist_t *plist; 	/* Property list pointer */
    H5O_fill_t fill;            /* Fill value property to modify */
    unsigned alloc_time_state;  /* State of allocation time property */
    herr_t ret_value = SUCCEED; /* return value 	 */

    FUNC_ENTER_API(H5Pset_alloc_time, FAIL)
    H5TRACE2("e", "iDa", plist_id, alloc_time);

    /* Check arguments */
    if(alloc_time < H5D_ALLOC_TIME_DEFAULT || alloc_time > H5D_ALLOC_TIME_INCR)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid allocation time setting")

    /* Get the property list structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_CREATE)))
	HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Check for resetting to default for layout type */
    if(alloc_time == H5D_ALLOC_TIME_DEFAULT) {
        H5O_layout_t layout;            /* Type of storage layout */

        /* Retrieve the storage layout */
        if(H5P_get(plist, H5D_CRT_LAYOUT_NAME, &layout) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get layout")

        /* Set the default based on layout */
        switch(layout.type) {
            case H5D_COMPACT:
                alloc_time = H5D_ALLOC_TIME_EARLY;
                break;

            case H5D_CONTIGUOUS:
                alloc_time = H5D_ALLOC_TIME_LATE;
                break;

            case H5D_CHUNKED:
                alloc_time = H5D_ALLOC_TIME_INCR;
                break;

            default:
                HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "unknown layout type")
        } /* end switch */

        /* Reset the "state" of the allocation time property back to the "default" */
        alloc_time_state = 1;
    } /* end if */
    else
        /* Set the "state" of the allocation time property to indicate the user modified it */
        alloc_time_state = 0;

    /* Retrieve previous fill value settings */
    if(H5P_get(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value")

    /* Update property value */
    fill.alloc_time = alloc_time;

    /* Set values */
    if(H5P_set(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set fill value")
    if(H5P_set(plist, H5D_CRT_ALLOC_TIME_STATE_NAME, &alloc_time_state) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set space allocation time")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Pset_alloc_time() */


/*-------------------------------------------------------------------------
 * Function:    H5Pget_alloc_time
 *
 * Purpose:     Get space allocation time for dataset creation.
 *		Valid values are H5D_ALLOC_TIME_DEFAULT, H5D_ALLOC_TIME_EARLY,
 *			H5D_ALLOC_TIME_LATE, H5D_ALLOC_TIME_INCR
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Raymond Lu
 *              Wednesday, January 16, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_alloc_time(hid_t plist_id, H5D_alloc_time_t *alloc_time/*out*/)
{
    herr_t ret_value = SUCCEED; /* return value          */

    FUNC_ENTER_API(H5Pget_alloc_time, FAIL)
    H5TRACE2("e", "ix", plist_id, alloc_time);

    /* Get values */
    if(alloc_time) {
        H5P_genplist_t *plist;  /* Property list pointer */
        H5O_fill_t fill;        /* Fill value property to query */

        /* Get the property list structure */
        if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_CREATE)))
            HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

        /* Retrieve fill value settings */
        if(H5P_get(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value")

        /* Set user's value */
        *alloc_time = fill.alloc_time;
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_alloc_time() */


/*-------------------------------------------------------------------------
 * Function:    H5Pset_fill_time
 *
 * Purpose:	Set fill value writing time for dataset.  Valid values are
 *		H5D_FILL_TIME_ALLOC and H5D_FILL_TIME_NEVER.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Raymond Lu
 *              Wednesday, January 16, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fill_time(hid_t plist_id, H5D_fill_time_t fill_time)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    H5O_fill_t fill;            /* Fill value property to modify */
    herr_t ret_value = SUCCEED; /* return value          */

    FUNC_ENTER_API(H5Pset_fill_time, FAIL)
    H5TRACE2("e", "iDf", plist_id, fill_time);

    /* Check arguments */
    if(fill_time < H5D_FILL_TIME_ALLOC || fill_time > H5D_FILL_TIME_IFSET)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid fill time setting")

    /* Get the property list structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Retrieve previous fill value settings */
    if(H5P_get(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value")

    /* Update property value */
    fill.fill_time = fill_time;

    /* Set values */
    if(H5P_set(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set fill value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_fill_time() */


/*-------------------------------------------------------------------------
 * Function:    H5Pget_fill_time
 *
 * Purpose:	Get fill value writing time.  Valid values are H5D_NEVER
 *		and H5D_ALLOC.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Raymond Lu
 *              Wednesday, January 16, 2002
 *
 * Modifications:
 *
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_fill_time(hid_t plist_id, H5D_fill_time_t *fill_time/*out*/)
{
    herr_t ret_value = SUCCEED; /* return value          */

    FUNC_ENTER_API(H5Pget_fill_time, FAIL)
    H5TRACE2("e", "ix", plist_id, fill_time);

    /* Set values */
    if(fill_time) {
        H5P_genplist_t *plist;  /* Property list pointer */
        H5O_fill_t fill;        /* Fill value property to query */

        /* Get the property list structure */
        if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_CREATE)))
            HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

        /* Retrieve fill value settings */
        if(H5P_get(plist, H5D_CRT_FILL_VALUE_NAME, &fill) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value")

        /* Set user's value */
        *fill_time = fill.fill_time;
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_fill_time() */

