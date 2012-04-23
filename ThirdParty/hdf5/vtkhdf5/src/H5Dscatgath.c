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

/****************/
/* Module Setup */
/****************/

#define H5D_PACKAGE		/*suppress error about including H5Dpkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Dpkg.h"		/* Dataset functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free Lists                           */


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Local Prototypes */
/********************/
static size_t H5D_gather_file(const H5D_io_info_t *io_info,
    const H5S_t *file_space, H5S_sel_iter_t *file_iter, size_t nelmts,
    void *buf);
static herr_t H5D_scatter_file(const H5D_io_info_t *io_info,
    const H5S_t *file_space, H5S_sel_iter_t *file_iter, size_t nelmts,
    const void *buf);
static size_t H5D_gather_mem(const void *_buf,
    const H5S_t *space, H5S_sel_iter_t *iter, size_t nelmts,
    const H5D_dxpl_cache_t *dxpl_cache, void *_tgath_buf/*out*/);
static herr_t H5D_compound_opt_read(size_t nelmts, const H5S_t *mem_space,
    H5S_sel_iter_t *iter, const H5D_dxpl_cache_t *dxpl_cache,
    const H5D_type_info_t *type_info, void *user_buf/*out*/);
static herr_t H5D_compound_opt_write(size_t nelmts, const H5D_type_info_t *type_info);


/*********************/
/* Package Variables */
/*********************/


/*******************/
/* Local Variables */
/*******************/

/* Declare a free list to manage sequences of size_t */
H5FL_SEQ_EXTERN(size_t);

/* Declare a free list to manage sequences of hsize_t */
H5FL_SEQ_EXTERN(hsize_t);



/*-------------------------------------------------------------------------
 * Function:	H5D_scatter_file
 *
 * Purpose:	Scatters dataset elements from the type conversion buffer BUF
 *		to the file F where the data points are arranged according to
 *		the file dataspace FILE_SPACE and stored according to
 *		LAYOUT and EFL. Each element is ELMT_SIZE bytes.
 *		The caller is requesting that NELMTS elements are copied.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, June 20, 2002
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_scatter_file(const H5D_io_info_t *_io_info,
    const H5S_t *space, H5S_sel_iter_t *iter, size_t nelmts,
    const void *_buf)
{
    H5D_io_info_t tmp_io_info;     /* Temporary I/O info object */
    hsize_t _off[H5D_IO_VECTOR_SIZE];             /* Array to store sequence offsets */
    hsize_t *off = NULL;           /* Pointer to sequence offsets */
    hsize_t mem_off;               /* Offset in memory */
    size_t mem_curr_seq;           /* "Current sequence" in memory */
    size_t dset_curr_seq;          /* "Current sequence" in dataset */
    size_t _len[H5D_IO_VECTOR_SIZE];              /* Array to store sequence lengths */
    size_t *len = NULL;            /* Array to store sequence lengths */
    size_t orig_mem_len, mem_len;  /* Length of sequence in memory */
    size_t  nseq;                  /* Number of sequences generated */
    size_t  nelem;                 /* Number of elements used in sequences */
    herr_t  ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_scatter_file)

    /* Check args */
    HDassert(_io_info);
    HDassert(space);
    HDassert(iter);
    HDassert(nelmts > 0);
    HDassert(_buf);

    /* Set up temporary I/O info object */
    HDmemcpy(&tmp_io_info, _io_info, sizeof(*_io_info));
    tmp_io_info.op_type = H5D_IO_OP_WRITE;
    tmp_io_info.u.wbuf = _buf;

    /* Allocate the vector I/O arrays */
    if(tmp_io_info.dxpl_cache->vec_size > H5D_IO_VECTOR_SIZE) {
        if(NULL == (len = H5FL_SEQ_MALLOC(size_t, tmp_io_info.dxpl_cache->vec_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O length vector array")
        if(NULL == (off = H5FL_SEQ_MALLOC(hsize_t, tmp_io_info.dxpl_cache->vec_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O offset vector array")
    } /* end if */
    else {
        len = _len;
        off = _off;
    } /* end else */

    /* Loop until all elements are written */
    while(nelmts > 0) {
        /* Get list of sequences for selection to write */
        if(H5S_SELECT_GET_SEQ_LIST(space, H5S_GET_SEQ_LIST_SORTED, iter, tmp_io_info.dxpl_cache->vec_size, nelmts, &nseq, &nelem, off, len) < 0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_UNSUPPORTED, FAIL, "sequence length generation failed")

        /* Reset the current sequence information */
        mem_curr_seq = dset_curr_seq = 0;
        orig_mem_len = mem_len = nelem * iter->elmt_size;
        mem_off = 0;

        /* Write sequence list out */
        if((*tmp_io_info.layout_ops.writevv)(&tmp_io_info, nseq, &dset_curr_seq,
                len, off, (size_t)1, &mem_curr_seq, &mem_len, &mem_off) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_WRITEERROR, FAIL, "write error")

        /* Update buffer */
        tmp_io_info.u.wbuf = (const uint8_t *)tmp_io_info.u.wbuf + orig_mem_len;

        /* Decrement number of elements left to process */
        nelmts -= nelem;
    } /* end while */

done:
    /* Release resources, if allocated */
    if(len && len != _len)
        len = H5FL_SEQ_FREE(size_t, len);
    if(off && off != _off)
        off = H5FL_SEQ_FREE(hsize_t, off);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D_scatter_file() */


/*-------------------------------------------------------------------------
 * Function:	H5D_gather_file
 *
 * Purpose:	Gathers data points from file F and accumulates them in the
 *		type conversion buffer BUF.  The LAYOUT argument describes
 *		how the data is stored on disk and EFL describes how the data
 *		is organized in external files.  ELMT_SIZE is the size in
 *		bytes of a datum which this function treats as opaque.
 *		FILE_SPACE describes the dataspace of the dataset on disk
 *		and the elements that have been selected for reading (via
 *		hyperslab, etc).  This function will copy at most NELMTS
 *		elements.
 *
 * Return:	Success:	Number of elements copied.
 *		Failure:	0
 *
 * Programmer:	Quincey Koziol
 *              Monday, June 24, 2002
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5D_gather_file(const H5D_io_info_t *_io_info,
    const H5S_t *space, H5S_sel_iter_t *iter, size_t nelmts,
    void *_buf/*out*/)
{
    H5D_io_info_t tmp_io_info;  /* Temporary I/O info object */
    hsize_t _off[H5D_IO_VECTOR_SIZE];   /* Array to store sequence offsets */
    hsize_t *off = NULL;        /* Pointer to sequence offsets */
    hsize_t mem_off;            /* Offset in memory */
    size_t mem_curr_seq;        /* "Current sequence" in memory */
    size_t dset_curr_seq;       /* "Current sequence" in dataset */
    size_t _len[H5D_IO_VECTOR_SIZE];    /* Array to store sequence lengths */
    size_t *len = NULL;         /* Pointer to sequence lengths */
    size_t orig_mem_len, mem_len;       /* Length of sequence in memory */
    size_t nseq;                /* Number of sequences generated */
    size_t nelem;               /* Number of elements used in sequences */
    size_t ret_value = nelmts;  /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_gather_file)

    /* Check args */
    HDassert(_io_info);
    HDassert(_io_info->dset);
    HDassert(_io_info->store);
    HDassert(space);
    HDassert(iter);
    HDassert(nelmts > 0);
    HDassert(_buf);

    /* Set up temporary I/O info object */
    HDmemcpy(&tmp_io_info, _io_info, sizeof(*_io_info));
    tmp_io_info.op_type = H5D_IO_OP_READ;
    tmp_io_info.u.rbuf = _buf;

    /* Allocate the vector I/O arrays */
    if(tmp_io_info.dxpl_cache->vec_size > H5D_IO_VECTOR_SIZE) {
        if(NULL == (len = H5FL_SEQ_MALLOC(size_t, tmp_io_info.dxpl_cache->vec_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "can't allocate I/O length vector array")
        if(NULL == (off = H5FL_SEQ_MALLOC(hsize_t, tmp_io_info.dxpl_cache->vec_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "can't allocate I/O offset vector array")
    } /* end if */
    else {
        len = _len;
        off = _off;
    } /* end else */

    /* Loop until all elements are read */
    while(nelmts > 0) {
        /* Get list of sequences for selection to read */
        if(H5S_SELECT_GET_SEQ_LIST(space, H5S_GET_SEQ_LIST_SORTED, iter, tmp_io_info.dxpl_cache->vec_size, nelmts, &nseq, &nelem, off, len) < 0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_UNSUPPORTED, 0, "sequence length generation failed")

        /* Reset the current sequence information */
        mem_curr_seq = dset_curr_seq = 0;
        orig_mem_len = mem_len = nelem * iter->elmt_size;
        mem_off = 0;

        /* Read sequence list in */
        if((*tmp_io_info.layout_ops.readvv)(&tmp_io_info, nseq, &dset_curr_seq,
                len, off, (size_t)1, &mem_curr_seq, &mem_len, &mem_off) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_READERROR, 0, "read error")

        /* Update buffer */
        tmp_io_info.u.rbuf = (uint8_t *)tmp_io_info.u.rbuf + orig_mem_len;

        /* Decrement number of elements left to process */
        nelmts -= nelem;
    } /* end while */

done:
    /* Release resources, if allocated */
    if(len && len != _len)
        len = H5FL_SEQ_FREE(size_t, len);
    if(off && off != _off)
        off = H5FL_SEQ_FREE(hsize_t, off);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D_gather_file() */


/*-------------------------------------------------------------------------
 * Function:	H5D_scatter_mem
 *
 * Purpose:	Scatters NELMTS data points from the scatter buffer
 *		TSCAT_BUF to the application buffer BUF.  Each element is
 *		ELMT_SIZE bytes and they are organized in application memory
 *		according to SPACE.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, July 8, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_scatter_mem (const void *_tscat_buf, const H5S_t *space,
    H5S_sel_iter_t *iter, size_t nelmts, const H5D_dxpl_cache_t *dxpl_cache,
    void *_buf/*out*/)
{
    uint8_t *buf = (uint8_t *)_buf;   /* Get local copies for address arithmetic */
    const uint8_t *tscat_buf = (const uint8_t *)_tscat_buf;
    hsize_t _off[H5D_IO_VECTOR_SIZE];          /* Array to store sequence offsets */
    hsize_t *off = NULL;          /* Pointer to sequence offsets */
    size_t _len[H5D_IO_VECTOR_SIZE];           /* Array to store sequence lengths */
    size_t *len = NULL;           /* Pointer to sequence lengths */
    size_t curr_len;            /* Length of bytes left to process in sequence */
    size_t nseq;                /* Number of sequences generated */
    size_t curr_seq;            /* Current sequence being processed */
    size_t nelem;               /* Number of elements used in sequences */
    herr_t ret_value = SUCCEED;   /* Number of elements scattered */

    FUNC_ENTER_NOAPI(H5D_scatter_mem, FAIL)

    /* Check args */
    HDassert(tscat_buf);
    HDassert(space);
    HDassert(iter);
    HDassert(nelmts > 0);
    HDassert(buf);

    /* Allocate the vector I/O arrays */
    if(dxpl_cache->vec_size > H5D_IO_VECTOR_SIZE) {
        if(NULL == (len = H5FL_SEQ_MALLOC(size_t, dxpl_cache->vec_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O length vector array")
        if(NULL == (off = H5FL_SEQ_MALLOC(hsize_t, dxpl_cache->vec_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O offset vector array")
    } /* end if */
    else {
        len = _len;
        off = _off;
    } /* end else */

    /* Loop until all elements are written */
    while(nelmts > 0) {
        /* Get list of sequences for selection to write */
        if(H5S_SELECT_GET_SEQ_LIST(space, 0, iter, dxpl_cache->vec_size, nelmts, &nseq, &nelem, off, len) < 0)
            HGOTO_ERROR (H5E_INTERNAL, H5E_UNSUPPORTED, 0, "sequence length generation failed")

        /* Loop, while sequences left to process */
        for(curr_seq = 0; curr_seq < nseq; curr_seq++) {
            /* Get the number of bytes in sequence */
            curr_len = len[curr_seq];

            HDmemcpy(buf + off[curr_seq], tscat_buf, curr_len);

            /* Advance offset in destination buffer */
            tscat_buf += curr_len;
        } /* end for */

        /* Decrement number of elements left to process */
        nelmts -= nelem;
    } /* end while */

done:
    /* Release resources, if allocated */
    if(len && len != _len)
        len = H5FL_SEQ_FREE(size_t, len);
    if(off && off != _off)
        off = H5FL_SEQ_FREE(hsize_t, off);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5D_scatter_mem() */


/*-------------------------------------------------------------------------
 * Function:	H5D_gather_mem
 *
 * Purpose:	Gathers dataset elements from application memory BUF and
 *		copies them into the gather buffer TGATH_BUF.
 *		Each element is ELMT_SIZE bytes and arranged in application
 *		memory according to SPACE.
 *		The caller is requesting that at most NELMTS be gathered.
 *
 * Return:	Success:	Number of elements copied.
 *		Failure:	0
 *
 * Programmer:	Quincey Koziol
 *              Monday, June 24, 2002
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5D_gather_mem(const void *_buf, const H5S_t *space,
    H5S_sel_iter_t *iter, size_t nelmts, const H5D_dxpl_cache_t *dxpl_cache,
    void *_tgath_buf/*out*/)
{
    const uint8_t *buf = (const uint8_t *)_buf;   /* Get local copies for address arithmetic */
    uint8_t *tgath_buf = (uint8_t *)_tgath_buf;
    hsize_t _off[H5D_IO_VECTOR_SIZE];          /* Array to store sequence offsets */
    hsize_t *off = NULL;          /* Pointer to sequence offsets */
    size_t _len[H5D_IO_VECTOR_SIZE];           /* Array to store sequence lengths */
    size_t *len = NULL;           /* Pointer to sequence lengths */
    size_t curr_len;            /* Length of bytes left to process in sequence */
    size_t nseq;                /* Number of sequences generated */
    size_t curr_seq;            /* Current sequence being processed */
    size_t nelem;               /* Number of elements used in sequences */
    size_t ret_value = nelmts;    /* Number of elements gathered */

    FUNC_ENTER_NOAPI_NOINIT(H5D_gather_mem)

    /* Check args */
    HDassert(buf);
    HDassert(space);
    HDassert(iter);
    HDassert(nelmts > 0);
    HDassert(tgath_buf);

    /* Allocate the vector I/O arrays */
    if(dxpl_cache->vec_size > H5D_IO_VECTOR_SIZE) {
        if(NULL == (len = H5FL_SEQ_MALLOC(size_t, dxpl_cache->vec_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "can't allocate I/O length vector array")
        if(NULL == (off = H5FL_SEQ_MALLOC(hsize_t, dxpl_cache->vec_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "can't allocate I/O offset vector array")
    } /* end if */
    else {
        len = _len;
        off = _off;
    } /* end else */

    /* Loop until all elements are written */
    while(nelmts > 0) {
        /* Get list of sequences for selection to write */
        if(H5S_SELECT_GET_SEQ_LIST(space, 0, iter, dxpl_cache->vec_size, nelmts, &nseq, &nelem, off, len) < 0)
            HGOTO_ERROR (H5E_INTERNAL, H5E_UNSUPPORTED, 0, "sequence length generation failed")

        /* Loop, while sequences left to process */
        for(curr_seq = 0; curr_seq < nseq; curr_seq++) {
            /* Get the number of bytes in sequence */
            curr_len = len[curr_seq];

            HDmemcpy(tgath_buf, buf + off[curr_seq], curr_len);

            /* Advance offset in gather buffer */
            tgath_buf += curr_len;
        } /* end for */

        /* Decrement number of elements left to process */
        nelmts -= nelem;
    } /* end while */

done:
    /* Release resources, if allocated */
    if(len && len != _len)
        len = H5FL_SEQ_FREE(size_t, len);
    if(off && off != _off)
        off = H5FL_SEQ_FREE(hsize_t, off);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5D_gather_mem() */


/*-------------------------------------------------------------------------
 * Function:	H5D_scatgath_read
 *
 * Purpose:	Perform scatter/gather ead from a contiguous [piece of a] dataset.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Thursday, March  6, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_scatgath_read(const H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t nelmts, const H5S_t *file_space, const H5S_t *mem_space)
{
    const H5D_dxpl_cache_t *dxpl_cache = io_info->dxpl_cache;     /* Local pointer to dataset transfer info */
    void        *buf = io_info->u.rbuf; /* Local pointer to application buffer */
    H5S_sel_iter_t mem_iter;            /*memory selection iteration info*/
    hbool_t	mem_iter_init = FALSE;	/*memory selection iteration info has been initialized */
    H5S_sel_iter_t bkg_iter;            /*background iteration info*/
    hbool_t	bkg_iter_init = FALSE;	/*background iteration info has been initialized */
    H5S_sel_iter_t file_iter;           /*file selection iteration info*/
    hbool_t	file_iter_init = FALSE;	/*file selection iteration info has been initialized */
    hsize_t	smine_start;		/*strip mine start loc	*/
    size_t	smine_nelmts;		/*elements per strip	*/
    herr_t	ret_value = SUCCEED;	/*return value		*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_scatgath_read)

    /* Sanity check */
    HDassert(io_info);
    HDassert(type_info);
    HDassert(mem_space);
    HDassert(file_space);
    HDassert(buf);

    /* Check for NOOP read */
    if(nelmts == 0)
        HGOTO_DONE(SUCCEED)

    /* Figure out the strip mine size. */
    if(H5S_select_iter_init(&file_iter, file_space, type_info->src_type_size) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize file selection information")
    file_iter_init = TRUE;	/*file selection iteration info has been initialized */
    if(H5S_select_iter_init(&mem_iter, mem_space, type_info->dst_type_size) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize memory selection information")
    mem_iter_init = TRUE;	/*file selection iteration info has been initialized */
    if(H5S_select_iter_init(&bkg_iter, mem_space, type_info->dst_type_size) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize background selection information")
    bkg_iter_init = TRUE;	/*file selection iteration info has been initialized */

    /* Start strip mining... */
    for(smine_start = 0; smine_start < nelmts; smine_start += smine_nelmts) {
        size_t n;               /* Elements operated on */

        /* Go figure out how many elements to read from the file */
        HDassert(H5S_SELECT_ITER_NELMTS(&file_iter) == (nelmts - smine_start));
        smine_nelmts = (size_t)MIN(type_info->request_nelmts, (nelmts - smine_start));

        /*
         * Gather the data from disk into the datatype conversion
         * buffer. Also gather data from application to background buffer
         * if necessary.
         */

	/*
         * Gather data
         */
        n = H5D_gather_file(io_info, file_space, &file_iter, smine_nelmts,
                type_info->tconv_buf/*out*/);
	if(n != smine_nelmts)
            HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "file gather failed")

        /* If the source and destination are compound types and subset of each other
         * and no conversion is needed, copy the data directly into user's buffer and
         * bypass the rest of steps.
         */
        if(type_info->cmpd_subset && H5T_SUBSET_FALSE != type_info->cmpd_subset->subset) {
            if(H5D_compound_opt_read(smine_nelmts, mem_space, &mem_iter, dxpl_cache,
                    type_info, buf /*out*/) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "datatype conversion failed")
        } /* end if */
        else {
            if(H5T_BKG_YES == type_info->need_bkg) {
                n = H5D_gather_mem(buf, mem_space, &bkg_iter, smine_nelmts,
                        dxpl_cache, type_info->bkg_buf/*out*/);
                if(n != smine_nelmts)
                    HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "mem gather failed")
            } /* end if */

            /*
             * Perform datatype conversion.
             */
            if(H5T_convert(type_info->tpath, type_info->src_type_id, type_info->dst_type_id,
                    smine_nelmts, (size_t)0, (size_t)0, type_info->tconv_buf,
                    type_info->bkg_buf, io_info->dxpl_id) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "datatype conversion failed")

            /* Do the data transform after the conversion (since we're using type mem_type) */
            if(!type_info->is_xform_noop)
                if(H5Z_xform_eval(dxpl_cache->data_xform_prop, type_info->tconv_buf, smine_nelmts, type_info->mem_type) < 0)
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "Error performing data transform")

            /*
             * Scatter the data into memory.
             */
            if(H5D_scatter_mem(type_info->tconv_buf, mem_space, &mem_iter,
                    smine_nelmts, dxpl_cache, buf/*out*/) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "scatter failed")
        } /* end else */
    } /* end for */

done:
    /* Release selection iterators */
    if(file_iter_init) {
        if(H5S_SELECT_ITER_RELEASE(&file_iter) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release selection iterator")
    } /* end if */
    if(mem_iter_init) {
        if(H5S_SELECT_ITER_RELEASE(&mem_iter) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release selection iterator")
    } /* end if */
    if(bkg_iter_init) {
        if(H5S_SELECT_ITER_RELEASE(&bkg_iter) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release selection iterator")
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_scatgath_read() */


/*-------------------------------------------------------------------------
 * Function:	H5D_scatgath_write
 *
 * Purpose:	Perform scatter/gather write to a contiguous [piece of a] dataset.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Thursday, March  6, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_scatgath_write(const H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t nelmts, const H5S_t *file_space, const H5S_t *mem_space)
{
    const H5D_dxpl_cache_t *dxpl_cache = io_info->dxpl_cache;     /* Local pointer to dataset transfer info */
    const void  *buf = io_info->u.wbuf; /* Local pointer to application buffer */
    H5S_sel_iter_t mem_iter;            /*memory selection iteration info*/
    hbool_t	mem_iter_init = FALSE;	/*memory selection iteration info has been initialized */
    H5S_sel_iter_t bkg_iter;            /*background iteration info*/
    hbool_t	bkg_iter_init = FALSE;	/*background iteration info has been initialized */
    H5S_sel_iter_t file_iter;           /*file selection iteration info*/
    hbool_t	file_iter_init = FALSE;	/*file selection iteration info has been initialized */
    hsize_t	smine_start;		/*strip mine start loc	*/
    size_t	smine_nelmts;		/*elements per strip	*/
    herr_t	ret_value = SUCCEED;	/*return value		*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_scatgath_write)

    /* Sanity check */
    HDassert(io_info);
    HDassert(type_info);
    HDassert(mem_space);
    HDassert(file_space);
    HDassert(buf);

    /* Check for NOOP write */
    if(nelmts == 0)
        HGOTO_DONE(SUCCEED)

    /* Figure out the strip mine size. */
    if(H5S_select_iter_init(&file_iter, file_space, type_info->dst_type_size) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize file selection information")
    file_iter_init = TRUE;	/*file selection iteration info has been initialized */
    if(H5S_select_iter_init(&mem_iter, mem_space, type_info->src_type_size) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize memory selection information")
    mem_iter_init = TRUE;	/*file selection iteration info has been initialized */
    if(H5S_select_iter_init(&bkg_iter, file_space, type_info->dst_type_size) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize background selection information")
    bkg_iter_init = TRUE;	/*file selection iteration info has been initialized */

    /* Start strip mining... */
    for(smine_start = 0; smine_start < nelmts; smine_start += smine_nelmts) {
        size_t n;               /* Elements operated on */

        /* Go figure out how many elements to read from the file */
        HDassert(H5S_SELECT_ITER_NELMTS(&file_iter) == (nelmts - smine_start));
        smine_nelmts = (size_t)MIN(type_info->request_nelmts, (nelmts - smine_start));

        /*
         * Gather data from application buffer into the datatype conversion
         * buffer. Also gather data from the file into the background buffer
         * if necessary.
         */
        n = H5D_gather_mem(buf, mem_space, &mem_iter, smine_nelmts,
                dxpl_cache, type_info->tconv_buf/*out*/);
        if(n != smine_nelmts)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "mem gather failed")

        /* If the source and destination are compound types and the destination is
         * is a subset of the source and no conversion is needed, copy the data
         * directly into user's buffer and bypass the rest of steps.  If the source
         * is a subset of the destination, the optimization is done in conversion
         * function H5T_conv_struct_opt to protect the background data.
         */
        if(type_info->cmpd_subset && H5T_SUBSET_DST == type_info->cmpd_subset->subset
            && type_info->dst_type_size == type_info->cmpd_subset->copy_size) {
            if(H5D_compound_opt_write(smine_nelmts, type_info) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "datatype conversion failed")
        } /* end if */
        else {
            if(H5T_BKG_YES == type_info->need_bkg) {
                n = H5D_gather_file(io_info, file_space, &bkg_iter, smine_nelmts,
                        type_info->bkg_buf/*out*/);
                if(n != smine_nelmts)
                    HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "file gather failed")
            } /* end if */

            /* Do the data transform before the type conversion (since
             * transforms must be done in the memory type). */
            if(!type_info->is_xform_noop)
	        if(H5Z_xform_eval(dxpl_cache->data_xform_prop, type_info->tconv_buf, smine_nelmts, type_info->mem_type) < 0)
		    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "Error performing data transform")

            /*
             * Perform datatype conversion.
             */
            if(H5T_convert(type_info->tpath, type_info->src_type_id, type_info->dst_type_id,
                    smine_nelmts, (size_t)0, (size_t)0, type_info->tconv_buf,
                    type_info->bkg_buf, io_info->dxpl_id) < 0)
                 HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "datatype conversion failed")
        } /* end else */

        /*
         * Scatter the data out to the file.
         */
        if(H5D_scatter_file(io_info, file_space, &file_iter, smine_nelmts,
                type_info->tconv_buf) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "scatter failed")
    } /* end for */

done:
    /* Release selection iterators */
    if(file_iter_init) {
        if(H5S_SELECT_ITER_RELEASE(&file_iter) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release selection iterator")
    } /* end if */
    if(mem_iter_init) {
        if(H5S_SELECT_ITER_RELEASE(&mem_iter) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release selection iterator")
    } /* end if */
    if(bkg_iter_init) {
        if(H5S_SELECT_ITER_RELEASE(&bkg_iter) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release selection iterator")
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_scatgath_write() */


/*-------------------------------------------------------------------------
 * Function:	H5D_compound_opt_read
 *
 * Purpose:	A special optimization case when the source and
 *              destination members are a subset of each other, and
 *              the order is the same, and no conversion is needed.
 *              For example:
 *                  struct source {            struct destination {
 *                      TYPE1 A;      -->          TYPE1 A;
 *                      TYPE2 B;      -->          TYPE2 B;
 *                      TYPE3 C;      -->          TYPE3 C;
 *                  };                             TYPE4 D;
 *                                                 TYPE5 E;
 *                                             };
 *              or
 *                  struct destination {       struct source {
 *                      TYPE1 A;      <--          TYPE1 A;
 *                      TYPE2 B;      <--          TYPE2 B;
 *                      TYPE3 C;      <--          TYPE3 C;
 *                  };                             TYPE4 D;
 *                                                 TYPE5 E;
 *                                             };
 *              The optimization is simply moving data to the appropriate
 *              places in the buffer.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *		11 June 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_compound_opt_read(size_t nelmts, const H5S_t *space,
    H5S_sel_iter_t *iter, const H5D_dxpl_cache_t *dxpl_cache,
    const H5D_type_info_t *type_info, void *user_buf/*out*/)
{
    uint8_t    *ubuf = (uint8_t *)user_buf;     /* Cast for pointer arithmetic	*/
    uint8_t    *xdbuf;                          /* Pointer into dataset buffer */
    hsize_t    _off[H5D_IO_VECTOR_SIZE];        /* Array to store sequence offsets */
    hsize_t    *off = NULL;                     /* Pointer to sequence offsets */
    size_t     _len[H5D_IO_VECTOR_SIZE];        /* Array to store sequence lengths */
    size_t     *len = NULL;                     /* Pointer to sequence lengths */
    size_t     src_stride, dst_stride, copy_size;
    herr_t     ret_value = SUCCEED;	       /*return value		*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_compound_opt_read)

    /* Check args */
    HDassert(nelmts > 0);
    HDassert(space);
    HDassert(iter);
    HDassert(dxpl_cache);
    HDassert(type_info);
    HDassert(type_info->cmpd_subset);
    HDassert(H5T_SUBSET_SRC == type_info->cmpd_subset->subset ||
        H5T_SUBSET_DST == type_info->cmpd_subset->subset);
    HDassert(user_buf);

    /* Allocate the vector I/O arrays */
    if(dxpl_cache->vec_size > H5D_IO_VECTOR_SIZE) {
        if(NULL == (len = H5FL_SEQ_MALLOC(size_t, dxpl_cache->vec_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O length vector array")
        if(NULL == (off = H5FL_SEQ_MALLOC(hsize_t, dxpl_cache->vec_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O offset vector array")
    } /* end if */
    else {
        len = _len;
        off = _off;
    } /* end else */

    /* Get source & destination strides */
    src_stride = type_info->src_type_size;
    dst_stride = type_info->dst_type_size;

    /* Get the size, in bytes, to copy for each element */
    copy_size = type_info->cmpd_subset->copy_size;

    /* Loop until all elements are written */
    xdbuf = type_info->tconv_buf;
    while(nelmts > 0) {
        size_t     nseq;                /* Number of sequences generated */
        size_t     curr_seq;            /* Current sequence being processed */
        size_t     elmtno;		/* Element counter */

        /* Get list of sequences for selection to write */
        if(H5S_SELECT_GET_SEQ_LIST(space, 0, iter, dxpl_cache->vec_size, nelmts, &nseq, &elmtno, off, len) < 0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_UNSUPPORTED, 0, "sequence length generation failed")

        /* Loop, while sequences left to process */
        for(curr_seq = 0; curr_seq < nseq; curr_seq++) {
            size_t     curr_off;        /* Offset of bytes left to process in sequence */
            size_t     curr_len;        /* Length of bytes left to process in sequence */
            size_t     curr_nelmts;	/* Number of elements to process in sequence   */
            uint8_t    *xubuf;
            size_t     i;               /* Local index variable */

            /* Get the number of bytes and offset in sequence */
            curr_len = len[curr_seq];
            H5_CHECK_OVERFLOW(off[curr_seq], hsize_t, size_t);
            curr_off = (size_t)off[curr_seq];

            /* Decide the number of elements and position in the buffer. */
            curr_nelmts = curr_len / dst_stride;
            xubuf = ubuf + curr_off;

            /* Copy the data into the right place. */
            for(i = 0; i < curr_nelmts; i++) {
                HDmemmove(xubuf, xdbuf, copy_size);

                /* Update pointers */
                xdbuf += src_stride;
                xubuf += dst_stride;
            } /* end for */
        } /* end for */

        /* Decrement number of elements left to process */
        nelmts -= elmtno;
    } /* end while */

done:
    /* Release resources, if allocated */
    if(len && len != _len)
        len = H5FL_SEQ_FREE(size_t, len);
    if(off && off != _off)
        off = H5FL_SEQ_FREE(hsize_t, off);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_compound_opt_read() */


/*-------------------------------------------------------------------------
 * Function:	H5D_compound_opt_write
 *
 * Purpose:	A special optimization case when the source and
 *              destination members are a subset of each other, and
 *              the order is the same, and no conversion is needed.
 *              For example:
 *                  struct source {            struct destination {
 *                      TYPE1 A;      -->          TYPE1 A;
 *                      TYPE2 B;      -->          TYPE2 B;
 *                      TYPE3 C;      -->          TYPE3 C;
 *                  };                             TYPE4 D;
 *                                                 TYPE5 E;
 *                                             };
 *              or
 *                  struct destination {       struct source {
 *                      TYPE1 A;      <--          TYPE1 A;
 *                      TYPE2 B;      <--          TYPE2 B;
 *                      TYPE3 C;      <--          TYPE3 C;
 *                  };                             TYPE4 D;
 *                                                 TYPE5 E;
 *                                             };
 *              The optimization is simply moving data to the appropriate
 *              places in the buffer.
 *
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *		11 June 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_compound_opt_write(size_t nelmts, const H5D_type_info_t *type_info)
{
    uint8_t    *xsbuf, *xdbuf;                  /* Source & destination pointers into dataset buffer */
    size_t     src_stride, dst_stride;          /* Strides through source & destination datatypes */
    size_t     i;                               /* Local index variable */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_compound_opt_write)

    /* Check args */
    HDassert(nelmts > 0);
    HDassert(type_info);

    /* Initialize values for loop */
    src_stride = type_info->src_type_size;
    dst_stride = type_info->dst_type_size;

    /* Loop until all elements are written */
    xsbuf = (uint8_t *)type_info->tconv_buf;
    xdbuf = (uint8_t *)type_info->tconv_buf;
    for(i = 0; i < nelmts; i++) {
        HDmemmove(xdbuf, xsbuf, dst_stride);

        /* Update pointers */
        xsbuf += src_stride;
        xdbuf += dst_stride;
    } /* end for */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5D_compound_opt_write() */

