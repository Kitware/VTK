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
 * Created:		H5HFdtable.c
 *			Apr 10 2006
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		"Doubling table" routines for fractal heaps.
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
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Vprivate.h"		/* Vectors and arrays 			*/

/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5HF_dtable_init
 *
 * Purpose:	Initialize values for doubling table
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar  6 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_dtable_init(H5HF_dtable_t *dtable)
{
    hsize_t tmp_block_size;             /* Temporary block size */
    hsize_t acc_block_off;              /* Accumulated block offset */
    size_t u;                           /* Local index variable */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_dtable_init)

    /*
     * Check arguments.
     */
    HDassert(dtable);

    /* Compute/cache some values */
    dtable->start_bits = H5V_log2_of2((uint32_t)dtable->cparam.start_block_size);
    dtable->first_row_bits = dtable->start_bits + H5V_log2_of2(dtable->cparam.width);
    dtable->max_root_rows = (dtable->cparam.max_index - dtable->first_row_bits) + 1;
    dtable->max_direct_bits = H5V_log2_of2((uint32_t)dtable->cparam.max_direct_size);
    dtable->max_direct_rows = (dtable->max_direct_bits - dtable->start_bits) + 2;
    dtable->num_id_first_row = dtable->cparam.start_block_size * dtable->cparam.width;
    dtable->max_dir_blk_off_size = H5HF_SIZEOF_OFFSET_LEN(dtable->cparam.max_direct_size);

    /* Build table of block sizes for each row */
    if(NULL == (dtable->row_block_size = (hsize_t *)H5MM_malloc(dtable->max_root_rows * sizeof(hsize_t))))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't create doubling table block size table")
    if(NULL == (dtable->row_block_off = (hsize_t *)H5MM_malloc(dtable->max_root_rows * sizeof(hsize_t))))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't create doubling table block offset table")
    if(NULL == (dtable->row_tot_dblock_free = (hsize_t *)H5MM_malloc(dtable->max_root_rows * sizeof(hsize_t))))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't create doubling table total direct block free space table")
    if(NULL == (dtable->row_max_dblock_free = (size_t *)H5MM_malloc(dtable->max_root_rows * sizeof(size_t))))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't create doubling table max. direct block free space table")
    tmp_block_size = dtable->cparam.start_block_size;
    acc_block_off = dtable->cparam.start_block_size * dtable->cparam.width;
    dtable->row_block_size[0] = dtable->cparam.start_block_size;
    dtable->row_block_off[0] = 0;
    for(u = 1; u < dtable->max_root_rows; u++) {
        dtable->row_block_size[u] = tmp_block_size;
        dtable->row_block_off[u] = acc_block_off;
        tmp_block_size *= 2;
        acc_block_off *= 2;
    } /* end for */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_dtable_init() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_dtable_lookup
 *
 * Purpose:	Compute the row & col of an offset in a doubling-table
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar  6 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_dtable_lookup(const H5HF_dtable_t *dtable, hsize_t off, unsigned *row, unsigned *col)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5HF_dtable_lookup)

    /*
     * Check arguments.
     */
    HDassert(dtable);
    HDassert(row);
    HDassert(col);
#ifdef QAK
HDfprintf(stderr, "%s: off = %Hu\n", "H5HF_dtable_lookup", off);
#endif /* QAK */

    /* Check for offset in first row */
    if(off < dtable->num_id_first_row) {
        *row = 0;
        H5_ASSIGN_OVERFLOW(/* To: */ *col, /* From: */ (off / dtable->cparam.start_block_size), /* From: */ hsize_t, /* To: */ unsigned);
    } /* end if */
    else {
        unsigned high_bit = H5V_log2_gen(off);  /* Determine the high bit in the offset */
        hsize_t off_mask = ((hsize_t)1) << high_bit;       /* Compute mask for determining column */

#ifdef QAK
HDfprintf(stderr, "%s: high_bit = %u, off_mask = %Hu\n", "H5HF_dtable_lookup", high_bit, off_mask);
#endif /* QAK */
        *row = (high_bit - dtable->first_row_bits) + 1;
        H5_ASSIGN_OVERFLOW(/* To: */ *col, /* From: */ ((off - off_mask) / dtable->row_block_size[*row]), /* From: */ hsize_t, /* To: */ unsigned);
    } /* end else */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5HF_dtable_lookup() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_dtable_dest
 *
 * Purpose:	Release information for doubling table
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar 27 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_dtable_dest(H5HF_dtable_t *dtable)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5HF_dtable_dest)

    /*
     * Check arguments.
     */
    HDassert(dtable);

    /* Free the block size lookup table for the doubling table */
    H5MM_xfree(dtable->row_block_size);

    /* Free the block offset lookup table for the doubling table */
    H5MM_xfree(dtable->row_block_off);

    /* Free the total direct block free space lookup table for the doubling table */
    H5MM_xfree(dtable->row_tot_dblock_free);

    /* Free the max. direct block free space lookup table for the doubling table */
    H5MM_xfree(dtable->row_max_dblock_free);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5HF_dtable_dest() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_dtable_size_to_row
 *
 * Purpose:	Compute row that can hold block of a certain size
 *
 * Return:	Non-negative on success (can't fail)
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Apr 25 2006
 *
 *-------------------------------------------------------------------------
 */
unsigned
H5HF_dtable_size_to_row(const H5HF_dtable_t *dtable, size_t block_size)
{
    unsigned row;               /* Row where block will fit */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5HF_dtable_size_to_row)

    /*
     * Check arguments.
     */
    HDassert(dtable);

    if(block_size == dtable->cparam.start_block_size)
        row = 0;
    else
        row = (H5V_log2_of2((uint32_t)block_size) - H5V_log2_of2((uint32_t)dtable->cparam.start_block_size)) + 1;

    FUNC_LEAVE_NOAPI(row)
} /* end H5HF_dtable_size_to_row() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_dtable_size_to_rows
 *
 * Purpose:	Compute # of rows of indirect block of a given size
 *
 * Return:	Non-negative on success (can't fail)
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May 31 2006
 *
 *-------------------------------------------------------------------------
 */
unsigned
H5HF_dtable_size_to_rows(const H5HF_dtable_t *dtable, hsize_t size)
{
    unsigned rows;              /* # of rows required for indirect block */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5HF_dtable_size_to_rows)

    /*
     * Check arguments.
     */
    HDassert(dtable);

    rows = (H5V_log2_gen(size) - dtable->first_row_bits) + 1;

    FUNC_LEAVE_NOAPI(rows)
} /* end H5HF_dtable_size_to_rows() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_dtable_span_size
 *
 * Purpose:	Compute the size covered by a span of entries
 *
 * Return:	Non-zero span size on success/zero on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		July 25 2006
 *
 *-------------------------------------------------------------------------
 */
hsize_t
H5HF_dtable_span_size(const H5HF_dtable_t *dtable, unsigned start_row,
    unsigned start_col, unsigned num_entries)
{
    unsigned start_entry;       /* Entry for first block covered */
    unsigned end_row;           /* Row for last block covered */
    unsigned end_col;           /* Column for last block covered */
    unsigned end_entry;         /* Entry for last block covered */
    hsize_t acc_span_size;      /* Accumulated span size */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5HF_dtable_span_size)

    /*
     * Check arguments.
     */
    HDassert(dtable);
    HDassert(num_entries > 0);

    /* Compute starting entry */
    start_entry = (start_row * dtable->cparam.width) + start_col;

    /* Compute ending entry, column & row */
    end_entry = (start_entry + num_entries) - 1;
    end_row = end_entry / dtable->cparam.width;
    end_col = end_entry % dtable->cparam.width;
#ifdef QAK
HDfprintf(stderr, "%s: start_row = %u, start_col = %u, start_entry = %u\n", "H5HF_sect_indirect_span_size", start_row, start_col, start_entry);
HDfprintf(stderr, "%s: end_row = %u, end_col = %u, end_entry = %u\n", "H5HF_sect_indirect_span_size", end_row, end_col, end_entry);
#endif /* QAK */

    /* Initialize accumulated span size */
    acc_span_size = 0;

    /* Compute span size covered */

    /* Check for multi-row span */
    if(start_row != end_row) {
        /* Accomodate partial starting row */
        if(start_col > 0) {
            acc_span_size = dtable->row_block_size[start_row] *
                    (dtable->cparam.width - start_col);
            start_row++;
        } /* end if */

        /* Accumulate full rows */
        while(start_row < end_row) {
            acc_span_size += dtable->row_block_size[start_row] *
                    dtable->cparam.width;
            start_row++;
        } /* end while */

        /* Accomodate partial ending row */
        acc_span_size += dtable->row_block_size[start_row] *
                (end_col + 1);
    } /* end if */
    else {
        /* Span is in same row */
        acc_span_size = dtable->row_block_size[start_row] *
                ((end_col - start_col) + 1);
    } /* end else */

#ifdef QAK
HDfprintf(stderr, "%s: acc_span_size = %Hu\n", "H5HF_dtable_span_size", acc_span_size);
#endif /* QAK */
    FUNC_LEAVE_NOAPI(acc_span_size)
} /* end H5HF_sect_indirect_span_size() */

