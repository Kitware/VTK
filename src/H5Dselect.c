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

/* Programmer:  Quincey Koziol <koziol@ncsa.uiuc.ued>
 *              Thursday, September 30, 2004
 *
 * Purpose:	Dataspace I/O functions.
 */

/****************/
/* Module Setup */
/****************/

#define H5D_PACKAGE		/*suppress error about including H5Dpkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Dpkg.h"		/* Datasets				*/
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


/*********************/
/* Package Variables */
/*********************/


/*******************/
/* Local Variables */
/*******************/

/* Declare a free list to manage sequences of size_t */
H5FL_SEQ_DEFINE(size_t);

/* Declare a free list to manage sequences of hsize_t */
H5FL_SEQ_DEFINE(hsize_t);



/*-------------------------------------------------------------------------
 * Function:	H5D_select_io
 *
 * Purpose:	Perform I/O directly from application memory and a file
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, November 27, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_select_io(const H5D_io_info_t *io_info, size_t elmt_size,
    size_t nelmts, const H5S_t *file_space, const H5S_t *mem_space)
{
    H5S_sel_iter_t mem_iter;    /* Memory selection iteration info */
    hbool_t mem_iter_init = 0;  /* Memory selection iteration info has been initialized */
    H5S_sel_iter_t file_iter;   /* File selection iteration info */
    hbool_t file_iter_init = 0;	/* File selection iteration info has been initialized */
    hsize_t _mem_off[H5D_IO_VECTOR_SIZE];      /* Array to store sequence offsets in memory */
    hsize_t *mem_off = NULL;    /* Pointer to sequence offsets in memory */
    hsize_t _file_off[H5D_IO_VECTOR_SIZE];     /* Array to store sequence offsets in the file */
    hsize_t *file_off = NULL;   /* Pointer to sequence offsets in the file */
    size_t _mem_len[H5D_IO_VECTOR_SIZE];       /* Array to store sequence lengths in memory */
    size_t *mem_len = NULL;     /* Pointer to sequence lengths in memory */
    size_t _file_len[H5D_IO_VECTOR_SIZE];      /* Array to store sequence lengths in the file */
    size_t *file_len = NULL;    /* Pointer to sequence lengths in the file */
    size_t curr_mem_seq;        /* Current memory sequence to operate on */
    size_t curr_file_seq;       /* Current file sequence to operate on */
    size_t mem_nseq;            /* Number of sequences generated in the file */
    size_t file_nseq;           /* Number of sequences generated in memory */
    ssize_t tmp_file_len;       /* Temporary number of bytes in file sequence */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(H5D_select_io, FAIL)

    /* Check args */
    HDassert(io_info);
    HDassert(io_info->dset);
    HDassert(io_info->store);
    HDassert(TRUE == H5P_isa_class(io_info->dxpl_id, H5P_DATASET_XFER));
    HDassert(io_info->u.rbuf);

    /* Allocate the vector I/O arrays */
    if(io_info->dxpl_cache->vec_size > H5D_IO_VECTOR_SIZE) {
        if(NULL == (mem_len = H5FL_SEQ_MALLOC(size_t,io_info->dxpl_cache->vec_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O length vector array")
        if(NULL == (mem_off = H5FL_SEQ_MALLOC(hsize_t,io_info->dxpl_cache->vec_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O offset vector array")
        if(NULL == (file_len = H5FL_SEQ_MALLOC(size_t,io_info->dxpl_cache->vec_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O length vector array")
        if(NULL == (file_off = H5FL_SEQ_MALLOC(hsize_t,io_info->dxpl_cache->vec_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate I/O offset vector array")
    } /* end if */
    else {
        mem_len = _mem_len;
        mem_off = _mem_off;
        file_len = _file_len;
        file_off = _file_off;
    } /* end else */

    /* Check for only one element in selection */
    if(nelmts == 1) {
        /* Get offset of first element in selections */
        if(H5S_SELECT_OFFSET(file_space, file_off) < 0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_UNSUPPORTED, FAIL, "can't retrieve file selection offset")
        if(H5S_SELECT_OFFSET(mem_space, mem_off) < 0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_UNSUPPORTED, FAIL, "can't retrieve memory selection offset")

        /* Set up necessary information for I/O operation */
        file_nseq = mem_nseq = 1;
        curr_mem_seq = curr_file_seq = 0;
        *file_off *= elmt_size;
        *mem_off *= elmt_size;
        *file_len = *mem_len = elmt_size;

        /* Perform I/O on memory and file sequences */
        if(io_info->op_type == H5D_IO_OP_READ) {
            if((tmp_file_len = (*io_info->layout_ops.readvv)(io_info,
                    file_nseq, &curr_file_seq, file_len, file_off,
                    mem_nseq, &curr_mem_seq, mem_len, mem_off)) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_READERROR, FAIL, "read error")
        } /* end if */
        else {
            HDassert(io_info->op_type == H5D_IO_OP_WRITE);
            if((tmp_file_len = (*io_info->layout_ops.writevv)(io_info,
                    file_nseq, &curr_file_seq, file_len, file_off,
                    mem_nseq, &curr_mem_seq, mem_len, mem_off)) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_WRITEERROR, FAIL, "write error")
        } /* end else */

        /* Decrement number of elements left to process */
        HDassert(((size_t)tmp_file_len % elmt_size) == 0);
    } /* end if */
    else {
        size_t mem_nelem;           /* Number of elements used in memory sequences */
        size_t file_nelem;          /* Number of elements used in file sequences */

        /* Initialize file iterator */
        if(H5S_select_iter_init(&file_iter, file_space, elmt_size) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator")
        file_iter_init = 1;	/* File selection iteration info has been initialized */

        /* Initialize memory iterator */
        if(H5S_select_iter_init(&mem_iter, mem_space, elmt_size) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator")
        mem_iter_init = 1;	/* Memory selection iteration info has been initialized */

        /* Initialize sequence counts */
        curr_mem_seq = curr_file_seq = 0;
        mem_nseq = file_nseq = 0;

        /* Loop, until all bytes are processed */
        while(nelmts > 0) {
            /* Check if more file sequences are needed */
            if(curr_file_seq >= file_nseq) {
                /* Get sequences for file selection */
                if(H5S_SELECT_GET_SEQ_LIST(file_space, H5S_GET_SEQ_LIST_SORTED, &file_iter, io_info->dxpl_cache->vec_size, nelmts, &file_nseq, &file_nelem, file_off, file_len) < 0)
                    HGOTO_ERROR(H5E_INTERNAL, H5E_UNSUPPORTED, FAIL, "sequence length generation failed")

                /* Start at the beginning of the sequences again */
                curr_file_seq = 0;
            } /* end if */

            /* Check if more memory sequences are needed */
            if(curr_mem_seq >= mem_nseq) {
                /* Get sequences for memory selection */
                if(H5S_SELECT_GET_SEQ_LIST(mem_space, 0, &mem_iter, io_info->dxpl_cache->vec_size, nelmts, &mem_nseq, &mem_nelem, mem_off, mem_len) < 0)
                    HGOTO_ERROR(H5E_INTERNAL, H5E_UNSUPPORTED, FAIL, "sequence length generation failed")

                /* Start at the beginning of the sequences again */
                curr_mem_seq = 0;
            } /* end if */

            /* Perform I/O on memory and file sequences */
            if(io_info->op_type == H5D_IO_OP_READ) {
                if((tmp_file_len = (*io_info->layout_ops.readvv)(io_info,
                        file_nseq, &curr_file_seq, file_len, file_off,
                        mem_nseq, &curr_mem_seq, mem_len, mem_off)) < 0)
                    HGOTO_ERROR(H5E_DATASPACE, H5E_READERROR, FAIL, "read error")
            } /* end if */
            else {
                HDassert(io_info->op_type == H5D_IO_OP_WRITE);
                if((tmp_file_len = (*io_info->layout_ops.writevv)(io_info,
                        file_nseq, &curr_file_seq, file_len, file_off,
                        mem_nseq, &curr_mem_seq, mem_len, mem_off)) < 0)
                    HGOTO_ERROR(H5E_DATASPACE, H5E_WRITEERROR, FAIL, "write error")
            } /* end else */

            /* Decrement number of elements left to process */
            HDassert(((size_t)tmp_file_len % elmt_size) == 0);
            nelmts -= ((size_t)tmp_file_len / elmt_size);
        } /* end while */
    } /* end else */

done:
    /* Release file selection iterator */
    if(file_iter_init)
        if(H5S_SELECT_ITER_RELEASE(&file_iter) < 0)
            HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator")

    /* Release memory selection iterator */
    if(mem_iter_init)
        if(H5S_SELECT_ITER_RELEASE(&mem_iter) < 0)
            HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator")

    /* Release vector arrays, if allocated */
    if(file_len && file_len != _file_len)
        file_len = H5FL_SEQ_FREE(size_t, file_len);
    if(file_off && file_off != _file_off)
        file_off = H5FL_SEQ_FREE(hsize_t, file_off);
    if(mem_len && mem_len != _mem_len)
        mem_len = H5FL_SEQ_FREE(size_t, mem_len);
    if(mem_off && mem_off != _mem_off)
        mem_off = H5FL_SEQ_FREE(hsize_t, mem_off);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_select_io() */


/*-------------------------------------------------------------------------
 * Function:	H5D_select_read
 *
 * Purpose:	Reads directly from file into application memory.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, July 23, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_select_read(const H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t nelmts, const H5S_t *file_space, const H5S_t *mem_space)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(H5D_select_read, FAIL)

    /* Call generic selection operation */
    H5_CHECK_OVERFLOW(nelmts, hsize_t, size_t);
    if(H5D_select_io(io_info, type_info->src_type_size, (size_t)nelmts,
            file_space, mem_space) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_READERROR, FAIL, "read error")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_select_read() */


/*-------------------------------------------------------------------------
 * Function:	H5D_select_write
 *
 * Purpose:	Writes directly from application memory into a file
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, July 23, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_select_write(const H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t nelmts, const H5S_t *file_space, const H5S_t *mem_space)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(H5D_select_write, FAIL)

    /* Call generic selection operation */
    H5_CHECK_OVERFLOW(nelmts, hsize_t, size_t);
    if(H5D_select_io(io_info, type_info->dst_type_size, (size_t)nelmts,
            file_space, mem_space) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_WRITEERROR, FAIL, "write error")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_select_write() */

