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
 * Created:             H5Faccum.c
 *                      Jan 10 2008
 *                      Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:             File metadata "accumulator" routines.  (Used to
 *                      cache small metadata I/Os and group them into a
 *                      single larger I/O)
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"             /* File access				*/
#include "H5FDprivate.h"	/* File drivers				*/
#include "H5Vprivate.h"		/* Vectors and arrays 			*/


/****************/
/* Local Macros */
/****************/

/* Metadata accumulator controls */
#define H5F_ACCUM_THROTTLE      8
#define H5F_ACCUM_THRESHOLD     2048
#define H5F_ACCUM_MAX_SIZE      (1024 *1024) /* Max. accum. buf size (max. I/Os will be 1/2 this size) */


/******************/
/* Local Typedefs */
/******************/

/* Enumerated type to indicate how data will be added to accumulator */
typedef enum {
    H5F_ACCUM_PREPEND,          /* Data will be prepended to accumulator */
    H5F_ACCUM_APPEND            /* Data will be appended to accumulator */
} H5F_accum_adjust_t;


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

/* Declare a PQ free list to manage the metadata accumulator buffer */
H5FL_BLK_DEFINE_STATIC(meta_accum);



/*-------------------------------------------------------------------------
 * Function:	H5F_accum_read
 *
 * Purpose:	Attempts to read some data from the metadata accumulator for
 *              a file into a buffer.
 *
 * Note:	We can't change (or add to) the metadata accumulator, because
 *		this might be a speculative read and could possibly read raw
 *		data into the metadata accumulator.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jan 10 2008
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5F_accum_read(const H5F_t *f, hid_t dxpl_id, H5FD_mem_t type, haddr_t addr,
    size_t size, void *buf/*out*/)
{
    htri_t      ret_value = FALSE;    /* Return value */

    FUNC_ENTER_NOAPI(H5F_accum_read, FAIL)

    HDassert(f);
    HDassert(f->shared);
    HDassert(buf);

    /* Check if this information is in the metadata accumulator */
    if((f->shared->feature_flags & H5FD_FEAT_ACCUMULATE_METADATA) && type != H5FD_MEM_DRAW
            && size < H5F_ACCUM_MAX_SIZE) {
        /* Sanity check */
        HDassert(!f->shared->accum.buf || (f->shared->accum.alloc_size >= f->shared->accum.size));

        /* Current read adjoins or overlaps with metadata accumulator */
        if(H5F_addr_overlap(addr, size, f->shared->accum.loc, f->shared->accum.size)
                || ((addr + size) == f->shared->accum.loc)
                || (f->shared->accum.loc + f->shared->accum.size) == addr) {
            size_t amount_before;       /* Amount to read before current accumulator */
            haddr_t new_addr;           /* New address of the accumulator buffer */
            size_t new_size;            /* New size of the accumulator buffer */

            /* Compute new values for accumulator */
            new_addr = MIN(addr, f->shared->accum.loc);
            new_size = (size_t)(MAX((addr + size), (f->shared->accum.loc + f->shared->accum.size))
                    - new_addr);

            /* Check if we need more buffer space */
            if(new_size > f->shared->accum.size) {
                /* Adjust the buffer size, by doubling it */
                f->shared->accum.alloc_size = MAX(f->shared->accum.alloc_size * 2, new_size);

                /* Reallocate the metadata accumulator buffer */
                if(NULL == (f->shared->accum.buf = H5FL_BLK_REALLOC(meta_accum, f->shared->accum.buf, f->shared->accum.alloc_size)))
                    HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "unable to allocate metadata accumulator buffer")
#ifdef H5_CLEAR_MEMORY
HDmemset(f->shared->accum.buf + f->shared->accum.size, 0, (f->shared->accum.alloc_size - f->shared->accum.size));
#endif /* H5_CLEAR_MEMORY */
            } /* end if */

            /* Read the part before the metadata accumulator */
            if(addr < f->shared->accum.loc) {
                /* Set the amount to read */
                H5_ASSIGN_OVERFLOW(amount_before, (f->shared->accum.loc - addr), hsize_t, size_t);

                /* Make room for the metadata to read in */
                HDmemmove(f->shared->accum.buf + amount_before, f->shared->accum.buf, f->shared->accum.size);

                /* Dispatch to driver */
                if(H5FD_read(f->shared->lf, dxpl_id, type, addr, amount_before, f->shared->accum.buf) < 0)
                    HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "driver read request failed")
            } /* end if */
            else
                amount_before = 0;

            /* Read the part after the metadata accumulator */
            if((addr + size) > (f->shared->accum.loc + f->shared->accum.size)) {
                size_t amount_after;         /* Amount to read at a time */

                /* Set the amount to read */
                H5_ASSIGN_OVERFLOW(amount_after, ((addr + size) - (f->shared->accum.loc + f->shared->accum.size)), hsize_t, size_t);

                /* Dispatch to driver */
                if(H5FD_read(f->shared->lf, dxpl_id, type, (f->shared->accum.loc + f->shared->accum.size), amount_after, (f->shared->accum.buf + f->shared->accum.size + amount_before)) < 0)
                    HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "driver read request failed")
            } /* end if */

            /* Copy the data out of the buffer */
            HDmemcpy(buf, f->shared->accum.buf + (addr - new_addr), size);

            /* Adjust the accumulator address & size */
            f->shared->accum.loc = new_addr;
            f->shared->accum.size = new_size;
        } /* end if */
        /* Current read doesn't overlap with metadata accumulator, read it from file */
        else {
            /* Dispatch to driver */
            if(H5FD_read(f->shared->lf, dxpl_id, type, addr, size, buf) < 0)
                HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "driver read request failed")
        } /* end else */

        /* Indicate success */
        HGOTO_DONE(TRUE);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_accum_read() */


/*-------------------------------------------------------------------------
 * Function:	H5F_accum_adjust
 *
 * Purpose:	Adjust accumulator size, if necessary
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jun 11 2009
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_accum_adjust(H5F_meta_accum_t *accum, H5FD_t *lf, hid_t dxpl_id,
    H5F_accum_adjust_t adjust, size_t size)
{
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5F_accum_adjust)

    HDassert(accum);
    HDassert(lf);
    HDassert(size > 0);
    HDassert(size <= H5F_ACCUM_MAX_SIZE);

    /* Check if we need more buffer space */
    if((size + accum->size) > accum->alloc_size) {
        size_t new_size;        /* New size of accumulator */

        /* Adjust the buffer size to be a power of 2 that is large enough to hold data */
        new_size = (size_t)1 << (1 + H5V_log2_gen((uint64_t)((size + accum->size) - 1)));

        /* Check for accumulator getting too big */
        if(new_size > H5F_ACCUM_MAX_SIZE) {
            size_t shrink_size;     /* Amount to shrink accumulator by */
            size_t remnant_size;    /* Amount left in accumulator */

            /* Cap the accumulator's growth, leaving some room */

            /* Determine the amounts to work with */
            if(size > (H5F_ACCUM_MAX_SIZE / 2)) {
                new_size = H5F_ACCUM_MAX_SIZE;
                shrink_size = accum->size;
                remnant_size = 0;
            } /* end if */
            else {
                new_size = (H5F_ACCUM_MAX_SIZE / 2);
                shrink_size = (H5F_ACCUM_MAX_SIZE / 2);
                remnant_size = accum->size - shrink_size;
            } /* end else */

            /* Check if we need to flush accumulator data to file */
            if(accum->dirty) {
                /* Check whether to accumulator will be prepended or appended */
                if(H5F_ACCUM_PREPEND == adjust) {
                    /* Write out upper part of the existing metadata accumulator, with dispatch to driver */
                    if(H5FD_write(lf, dxpl_id, H5FD_MEM_DEFAULT, (accum->loc + remnant_size), shrink_size, (accum->buf + remnant_size)) < 0)
                        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "file write failed")
                } /* end if */
                else {
                    /* Sanity check */
                    HDassert(H5F_ACCUM_APPEND == adjust);

                    /* Write out lower part of the existing metadata accumulator, with dispatch to driver */
                    if(H5FD_write(lf, dxpl_id, H5FD_MEM_DEFAULT, accum->loc, shrink_size, accum->buf) < 0)
                        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "file write failed")

                    /* Move remnant of accumulator down */
                    HDmemmove(accum->buf, (accum->buf + shrink_size), remnant_size);

                    /* Adjust accumulator's location */
                    accum->loc += shrink_size;
                } /* end else */

                /* Reset accumulator dirty flag (in case of error) */
                accum->dirty = FALSE;
            } /* end if */

            /* Trim the accumulator's use of its buffer */
            accum->size = remnant_size;
        } /* end if */

        /* Check for accumulator needing to be reallocated */
        if(new_size > accum->alloc_size) {
            unsigned char      *new_buf;            /* New buffer to hold the accumulated metadata */

            /* Reallocate the metadata accumulator buffer */
            if(NULL == (new_buf = H5FL_BLK_REALLOC(meta_accum, accum->buf, new_size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer")

            /* Update accumulator info */
            accum->buf = new_buf;
            accum->alloc_size = new_size;
#ifdef H5_CLEAR_MEMORY
HDmemset(accum->buf + accum->size, 0, (accum->alloc_size - (accum->size + size)));
#endif /* H5_CLEAR_MEMORY */
        } /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_accum_adjust() */


/*-------------------------------------------------------------------------
 * Function:	H5F_accum_write
 *
 * Purpose:	Attempts to read some data from the metadata accumulator for
 *              a file into a buffer.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jan 10 2008
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5F_accum_write(const H5F_t *f, hid_t dxpl_id, H5FD_mem_t type, haddr_t addr,
    size_t size, const void *buf)
{
    htri_t      ret_value = FALSE;    /* Return value */

    FUNC_ENTER_NOAPI(H5F_accum_write, FAIL)

    HDassert(f);
    HDassert(f->shared);
    HDassert(f->intent & H5F_ACC_RDWR);
    HDassert(buf);

    /* Check for accumulating metadata */
    if((f->shared->feature_flags & H5FD_FEAT_ACCUMULATE_METADATA) && type != H5FD_MEM_DRAW
            && size < H5F_ACCUM_MAX_SIZE) {
        /* Sanity check */
        HDassert(!f->shared->accum.buf || (f->shared->accum.alloc_size >= f->shared->accum.size));

        /* Check if there is already metadata in the accumulator */
        if(f->shared->accum.size > 0) {
            /* Check if the new metadata adjoins the beginning of the current accumulator */
            if((addr + size) == f->shared->accum.loc) {
                /* Check if we need to adjust accumulator size */
                if(H5F_accum_adjust(&f->shared->accum, f->shared->lf, dxpl_id, H5F_ACCUM_PREPEND, size) < 0)
                    HGOTO_ERROR(H5E_IO, H5E_CANTRESIZE, FAIL, "can't adjust metadata accumulator")

                /* Move the existing metadata to the proper location */
                HDmemmove(f->shared->accum.buf + size, f->shared->accum.buf, f->shared->accum.size);

                /* Copy the new metadata at the front */
                HDmemcpy(f->shared->accum.buf, buf, size);

                /* Set the new size & location of the metadata accumulator */
                f->shared->accum.loc = addr;
                f->shared->accum.size += size;

                /* Mark it as written to */
                f->shared->accum.dirty = TRUE;
            } /* end if */
            /* Check if the new metadata adjoins the end of the current accumulator */
            else if(addr == (f->shared->accum.loc + f->shared->accum.size)) {
                /* Check if we need to adjust accumulator size */
                if(H5F_accum_adjust(&f->shared->accum, f->shared->lf, dxpl_id, H5F_ACCUM_APPEND, size) < 0)
                    HGOTO_ERROR(H5E_IO, H5E_CANTRESIZE, FAIL, "can't adjust metadata accumulator")

                /* Copy the new metadata to the end */
                HDmemcpy(f->shared->accum.buf + f->shared->accum.size, buf, size);

                /* Set the new size of the metadata accumulator */
                f->shared->accum.size += size;

                /* Mark it as written to */
                f->shared->accum.dirty = TRUE;
            } /* end if */
            /* Check if the piece of metadata being written overlaps the metadata accumulator */
            else if(H5F_addr_overlap(addr, size, f->shared->accum.loc, f->shared->accum.size)) {
                size_t add_size;    /* New size of the accumulator buffer */

                /* Check if the new metadata is entirely within the current accumulator */
                if(addr >= f->shared->accum.loc && (addr + size) <= (f->shared->accum.loc + f->shared->accum.size)) {
                    /* Copy the new metadata to the proper location within the accumulator */
                    HDmemcpy(f->shared->accum.buf + (addr - f->shared->accum.loc), buf, size);

                    /* Mark it as written to */
                    f->shared->accum.dirty = TRUE;
                } /* end if */
                /* Check if the new metadata overlaps the beginning of the current accumulator */
                else if(addr < f->shared->accum.loc && (addr + size) <= (f->shared->accum.loc + f->shared->accum.size)) {
                    size_t old_offset;  /* Offset of old data within the accumulator buffer */

                    /* Calculate the amount we will need to add to the accumulator size, based on the amount of overlap */
                    H5_ASSIGN_OVERFLOW(add_size, (f->shared->accum.loc - addr), hsize_t, size_t);

                    /* Check if we need to adjust accumulator size */
                    if(H5F_accum_adjust(&f->shared->accum, f->shared->lf, dxpl_id, H5F_ACCUM_PREPEND, add_size) < 0)
                        HGOTO_ERROR(H5E_IO, H5E_CANTRESIZE, FAIL, "can't adjust metadata accumulator")

                    /* Calculate the proper offset of the existing metadata */
                    H5_ASSIGN_OVERFLOW(old_offset, (addr + size) - f->shared->accum.loc, hsize_t, size_t);

                    /* Move the existing metadata to the proper location */
                    HDmemmove(f->shared->accum.buf + size, f->shared->accum.buf + old_offset, (f->shared->accum.size - old_offset));

                    /* Copy the new metadata at the front */
                    HDmemcpy(f->shared->accum.buf, buf, size);

                    /* Set the new size & location of the metadata accumulator */
                    f->shared->accum.loc = addr;
                    f->shared->accum.size += add_size;

                    /* Mark it as written to */
                    f->shared->accum.dirty = TRUE;
                } /* end if */
                /* Check if the new metadata overlaps the end of the current accumulator */
                else if(addr >= f->shared->accum.loc && (addr + size) > (f->shared->accum.loc + f->shared->accum.size)) {
                    /* Calculate the amount we will need to add to the accumulator size, based on the amount of overlap */
                    H5_ASSIGN_OVERFLOW(add_size, (addr + size) - (f->shared->accum.loc + f->shared->accum.size), hsize_t, size_t);

                    /* Check if we need to adjust accumulator size */
                    if(H5F_accum_adjust(&f->shared->accum, f->shared->lf, dxpl_id, H5F_ACCUM_APPEND, add_size) < 0)
                        HGOTO_ERROR(H5E_IO, H5E_CANTRESIZE, FAIL, "can't adjust metadata accumulator")

                    /* Copy the new metadata to the end */
                    HDmemcpy(f->shared->accum.buf + (addr - f->shared->accum.loc), buf, size);

                    /* Set the new size of the metadata accumulator */
                    f->shared->accum.size += add_size;

                    /* Mark it as written to */
                    f->shared->accum.dirty = TRUE;
                } /* end if */
                /* New metadata overlaps both ends of the current accumulator */
                else {
                    /* Check if we need more buffer space */
                    if(size > f->shared->accum.size) {
                        /* Adjust the buffer size, by doubling it */
                        f->shared->accum.alloc_size = MAX(f->shared->accum.alloc_size * 2, size);

                        /* Reallocate the metadata accumulator buffer */
                        if(NULL == (f->shared->accum.buf = H5FL_BLK_REALLOC(meta_accum, f->shared->accum.buf, f->shared->accum.alloc_size)))
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer")
#ifdef H5_CLEAR_MEMORY
HDmemset(f->shared->accum.buf + size, 0, (f->shared->accum.alloc_size - size));
#endif /* H5_CLEAR_MEMORY */
                    } /* end if */

                    /* Copy the new metadata to the buffer */
                    HDmemcpy(f->shared->accum.buf, buf, size);

                    /* Set the new size & location of the metadata accumulator */
                    f->shared->accum.loc = addr;
                    f->shared->accum.size = size;

                    /* Mark it as written to */
                    f->shared->accum.dirty = TRUE;
                } /* end else */
            } /* end if */
            /* New piece of metadata doesn't adjoin or overlap the existing accumulator */
            else {
                /* Write out the existing metadata accumulator, with dispatch to driver */
                if(f->shared->accum.dirty) {
                    if(H5FD_write(f->shared->lf, dxpl_id, H5FD_MEM_DEFAULT, f->shared->accum.loc, f->shared->accum.size, f->shared->accum.buf) < 0)
                        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "file write failed")

                    /* Reset accumulator dirty flag */
                    f->shared->accum.dirty = FALSE;
                } /* end if */

                /* Cache the new piece of metadata */
                /* Check if we need to resize the buffer */
                if(size > f->shared->accum.alloc_size) {
                    size_t new_size;        /* New size of accumulator */

                    /* Adjust the buffer size to be a power of 2 that is large enough to hold data */
                    new_size = (size_t)1 << (1 + H5V_log2_gen((uint64_t)(size - 1)));

                    /* Grow the metadata accumulator buffer */
                    if(NULL == (f->shared->accum.buf = H5FL_BLK_REALLOC(meta_accum, f->shared->accum.buf, new_size)))
                        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer")

                    /* Note the new buffer size */
                    f->shared->accum.alloc_size = new_size;
#ifdef H5_CLEAR_MEMORY
{
size_t clear_size = MAX(f->shared->accum.size, size);
HDmemset(f->shared->accum.buf + clear_size, 0, (f->shared->accum.alloc_size - clear_size));
}
#endif /* H5_CLEAR_MEMORY */
                } /* end if */
                else {
                    /* Check if we should shrink the accumulator buffer */
                    if(size < (f->shared->accum.alloc_size / H5F_ACCUM_THROTTLE) &&
                            f->shared->accum.alloc_size > H5F_ACCUM_THRESHOLD) {
                        size_t tmp_size = (f->shared->accum.alloc_size / H5F_ACCUM_THROTTLE); /* New size of accumulator buffer */

                        /* Shrink the accumulator buffer */
                        if(NULL == (f->shared->accum.buf = H5FL_BLK_REALLOC(meta_accum, f->shared->accum.buf, tmp_size)))
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer")

                        /* Note the new buffer size */
                        f->shared->accum.alloc_size = tmp_size;
                    } /* end if */
                } /* end else */

                /* Update the metadata accumulator information */
                f->shared->accum.loc = addr;
                f->shared->accum.size = size;
                f->shared->accum.dirty = TRUE;

                /* Store the piece of metadata in the accumulator */
                HDmemcpy(f->shared->accum.buf, buf, size);
            } /* end else */
        } /* end if */
        /* No metadata in the accumulator, grab this piece and keep it */
        else {
            /* Check if we need to reallocate the buffer */
            if(size > f->shared->accum.alloc_size) {
                size_t new_size;        /* New size of accumulator */

                /* Adjust the buffer size to be a power of 2 that is large enough to hold data */
                new_size = (size_t)1 << (1 + H5V_log2_gen((uint64_t)(size - 1)));

                /* Reallocate the metadata accumulator buffer */
                if(NULL == (f->shared->accum.buf = H5FL_BLK_REALLOC(meta_accum, f->shared->accum.buf, new_size)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate metadata accumulator buffer")

                /* Note the new buffer size */
                f->shared->accum.alloc_size = new_size;
#ifdef H5_CLEAR_MEMORY
HDmemset(f->shared->accum.buf + size, 0, (f->shared->accum.alloc_size - size));
#endif /* H5_CLEAR_MEMORY */
            } /* end if */

            /* Update the metadata accumulator information */
            f->shared->accum.loc = addr;
            f->shared->accum.size = size;
            f->shared->accum.dirty = TRUE;

            /* Store the piece of metadata in the accumulator */
            HDmemcpy(f->shared->accum.buf, buf, size);
        } /* end else */

        /* Indicate success */
        HGOTO_DONE(TRUE);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_accum_write() */


/*-------------------------------------------------------------------------
 * Function:    H5F_accum_free
 *
 * Purpose:     Check for free space invalidating [part of] a metadata
 *              accumulator.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              koziol@hdfgroup.org
 *              Jan 10 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_accum_free(H5F_t *f, hid_t dxpl_id, H5FD_mem_t UNUSED type, haddr_t addr,
    hsize_t size)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5F_accum_free, FAIL)

    /* check arguments */
    HDassert(f);

    /* Adjust the metadata accumulator to remove the freed block, if it overlaps */
    if((f->shared->feature_flags & H5FD_FEAT_ACCUMULATE_METADATA)
            && H5F_addr_overlap(addr, size, f->shared->accum.loc, f->shared->accum.size)) {
        size_t overlap_size;        /* Size of overlap with accumulator */

        /* Sanity check */
        /* (The metadata accumulator should not intersect w/raw data */
        HDassert(H5FD_MEM_DRAW != type);

        /* Check for overlapping the beginning of the accumulator */
        if(H5F_addr_le(addr, f->shared->accum.loc)) {
            /* Check for completely overlapping the accumulator */
            if(H5F_addr_ge(addr + size, f->shared->accum.loc + f->shared->accum.size)) {
                /* Reset the accumulator, but don't free buffer */
                f->shared->accum.loc = HADDR_UNDEF;
                f->shared->accum.size = 0;
                f->shared->accum.dirty = FALSE;
            } /* end if */
            /* Block to free must end within the accumulator */
            else {
                size_t new_accum_size;      /* Size of new accumulator buffer */

                /* Calculate the size of the overlap with the accumulator, etc. */
                H5_ASSIGN_OVERFLOW(overlap_size, (addr + size) - f->shared->accum.loc, haddr_t, size_t);
                new_accum_size = f->shared->accum.size - overlap_size;

                /* Move the accumulator buffer information to eliminate the freed block */
                HDmemmove(f->shared->accum.buf, f->shared->accum.buf + overlap_size, new_accum_size);

                /* Adjust the accumulator information */
                f->shared->accum.loc += overlap_size;
                f->shared->accum.size = new_accum_size;
            } /* end else */
        } /* end if */
        /* Block to free must start within the accumulator */
        else {
            /* Calculate the size of the overlap with the accumulator */
            H5_ASSIGN_OVERFLOW(overlap_size, (f->shared->accum.loc + f->shared->accum.size) - addr, haddr_t, size_t);

            /* Block to free is in the middle of the accumulator */
            if(H5F_addr_lt((addr + size), f->shared->accum.loc + f->shared->accum.size)) {
                haddr_t tail_addr;
                size_t tail_size;

                /* Calculate the address & size of the tail to write */
                tail_addr = addr + size;
                H5_ASSIGN_OVERFLOW(tail_size, (f->shared->accum.loc + f->shared->accum.size) - tail_addr, haddr_t, size_t);

                /* Write out the part of the accumulator after the block to free */
                if(H5FD_write(f->shared->lf, dxpl_id, H5FD_MEM_DEFAULT, tail_addr, tail_size, f->shared->accum.buf + (tail_addr - f->shared->accum.loc)) < 0)
                    HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "file write failed")
            } /* end if */

            /* Adjust the accumulator information */
            f->shared->accum.size = f->shared->accum.size - overlap_size;
        } /* end else */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_accum_free() */


/*-------------------------------------------------------------------------
 * Function:	H5F_accum_flush
 *
 * Purpose:	Flush the metadata accumulator to the file
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jan 10 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_accum_flush(H5F_t *f, hid_t dxpl_id)
{
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5F_accum_flush, FAIL)

    HDassert(f);
    HDassert(f->shared);

    /* Check if we need to flush out the metadata accumulator */
    if((f->shared->feature_flags & H5FD_FEAT_ACCUMULATE_METADATA) && f->shared->accum.dirty && f->shared->accum.size > 0) {
        /* Flush the metadata contents */
        if(H5FD_write(f->shared->lf, dxpl_id, H5FD_MEM_DEFAULT, f->shared->accum.loc, f->shared->accum.size, f->shared->accum.buf) < 0)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "file write failed")

        /* Reset the dirty flag */
        f->shared->accum.dirty = FALSE;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_accum_flush() */


/*-------------------------------------------------------------------------
 * Function:	H5F_accum_reset
 *
 * Purpose:	Reset the metadata accumulator for the file
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jan 10 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_accum_reset(H5F_t *f, hid_t dxpl_id)
{
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5F_accum_reset, FAIL)

    HDassert(f);
    HDassert(f->shared);

    /* Flush any dirty data in accumulator */
    if(H5F_accum_flush(f, dxpl_id) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTFLUSH, FAIL, "can't flush metadata accumulator")

    /* Check if we need to reset the metadata accumulator information */
    if(f->shared->feature_flags & H5FD_FEAT_ACCUMULATE_METADATA) {
        /* Sanity check */
        HDassert(!f->closing || FALSE == f->shared->accum.dirty);

        /* Free the buffer */
        if(f->shared->accum.buf)
            f->shared->accum.buf = H5FL_BLK_FREE(meta_accum, f->shared->accum.buf);

        /* Reset the buffer sizes & location */
        f->shared->accum.alloc_size = f->shared->accum.size = 0;
        f->shared->accum.loc = HADDR_UNDEF;
        f->shared->accum.dirty = FALSE;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_accum_reset() */

