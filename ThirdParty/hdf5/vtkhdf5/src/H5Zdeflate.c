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

/*
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Friday, August 27, 1999
 */

#define H5Z_PACKAGE		/*suppress error about including H5Zpkg	  */


#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Zpkg.h"		/* Data filters				*/

#ifdef H5_HAVE_FILTER_DEFLATE

#ifdef H5_HAVE_ZLIB_H
#   include "hdf5_zlib.h"
#endif

/* Local function prototypes */
static size_t H5Z_filter_deflate (unsigned flags, size_t cd_nelmts,
    const unsigned cd_values[], size_t nbytes, size_t *buf_size, void **buf);

/* This message derives from H5Z */
const H5Z_class2_t H5Z_DEFLATE[1] = {{
    H5Z_CLASS_T_VERS,       /* H5Z_class_t version */
    H5Z_FILTER_DEFLATE,		/* Filter id number		*/
    1,              /* encoder_present flag (set to true) */
    1,              /* decoder_present flag (set to true) */
    "deflate",			/* Filter name for debugging	*/
    NULL,                       /* The "can apply" callback     */
    NULL,                       /* The "set local" callback     */
    H5Z_filter_deflate,         /* The actual filter function	*/
}};

#define H5Z_DEFLATE_SIZE_ADJUST(s) (HDceil(((double)(s))*1.001)+12)


/*-------------------------------------------------------------------------
 * Function:	H5Z_filter_deflate
 *
 * Purpose:	Implement an I/O filter around the 'deflate' algorithm in
 *              libz
 *
 * Return:	Success: Size of buffer filtered
 *		Failure: 0
 *
 * Programmer:	Robb Matzke
 *              Thursday, April 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5Z_filter_deflate (unsigned flags, size_t cd_nelmts,
		    const unsigned cd_values[], size_t nbytes,
		    size_t *buf_size, void **buf)
{
    void	*outbuf = NULL;         /* Pointer to new buffer */
    int		status;                 /* Status from zlib operation */
    size_t	ret_value;              /* Return value */

    FUNC_ENTER_NOAPI(H5Z_filter_deflate, 0)

    /* Sanity check */
    HDassert(*buf_size > 0);
    HDassert(buf);
    HDassert(*buf);

    /* Check arguments */
    if (cd_nelmts!=1 || cd_values[0]>9)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, 0, "invalid deflate aggression level")

    if (flags & H5Z_FLAG_REVERSE) {
	/* Input; uncompress */
	z_stream	z_strm;                 /* zlib parameters */
	size_t		nalloc = *buf_size;     /* Number of bytes for output (compressed) buffer */

        /* Allocate space for the compressed buffer */
	if (NULL==(outbuf = H5MM_malloc(nalloc)))
	    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "memory allocation failed for deflate uncompression")

        /* Set the uncompression parameters */
	HDmemset(&z_strm, 0, sizeof(z_strm));
	z_strm.next_in = (Bytef *)*buf;
        H5_ASSIGN_OVERFLOW(z_strm.avail_in,nbytes,size_t,unsigned);
	z_strm.next_out = (Bytef *)outbuf;
        H5_ASSIGN_OVERFLOW(z_strm.avail_out,nalloc,size_t,unsigned);

        /* Initialize the uncompression routines */
	if (Z_OK!=inflateInit(&z_strm))
	    HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, 0, "inflateInit() failed")

        /* Loop to uncompress the buffer */
	do {
            /* Uncompress some data */
	    status = inflate(&z_strm, Z_SYNC_FLUSH);

            /* Check if we are done uncompressing data */
	    if (Z_STREAM_END==status)
                break;	/*done*/

            /* Check for error */
	    if (Z_OK!=status) {
		(void)inflateEnd(&z_strm);
		HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, 0, "inflate() failed")
	    }
            else {
                /* If we're not done and just ran out of buffer space, get more */
                if(0 == z_strm.avail_out) {
                    void	*new_outbuf;         /* Pointer to new output buffer */

                    /* Allocate a buffer twice as big */
                    nalloc *= 2;
                    if(NULL == (new_outbuf = H5MM_realloc(outbuf, nalloc))) {
                        (void)inflateEnd(&z_strm);
                        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "memory allocation failed for deflate uncompression")
                    } /* end if */
                    outbuf = new_outbuf;

                    /* Update pointers to buffer for next set of uncompressed data */
                    z_strm.next_out = (unsigned char*)outbuf + z_strm.total_out;
                    z_strm.avail_out = (uInt)(nalloc - z_strm.total_out);
                } /* end if */
            } /* end else */
	} while(status==Z_OK);

        /* Free the input buffer */
	H5MM_xfree(*buf);

        /* Set return values */
	*buf = outbuf;
	outbuf = NULL;
	*buf_size = nalloc;
	ret_value = z_strm.total_out;

        /* Finish uncompressing the stream */
	(void)inflateEnd(&z_strm);
    } /* end if */
    else {
	/*
	 * Output; compress but fail if the result would be larger than the
	 * input.  The library doesn't provide in-place compression, so we
	 * must allocate a separate buffer for the result.
	 */
	const Bytef *z_src = (const Bytef*)(*buf);
	Bytef	    *z_dst;		/*destination buffer		*/
	uLongf	     z_dst_nbytes = (uLongf)H5Z_DEFLATE_SIZE_ADJUST(nbytes);
	uLong	     z_src_nbytes = (uLong)nbytes;
        int          aggression;     /* Compression aggression setting */

        /* Set the compression aggression level */
        H5_ASSIGN_OVERFLOW(aggression,cd_values[0],unsigned,int);

        /* Allocate output (compressed) buffer */
	if(NULL == (outbuf = H5MM_malloc(z_dst_nbytes)))
	    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "unable to allocate deflate destination buffer")
        z_dst = (Bytef *)outbuf;

        /* Perform compression from the source to the destination buffer */
	status = compress2(z_dst, &z_dst_nbytes, z_src, z_src_nbytes, aggression);

        /* Check for various zlib errors */
	if(Z_BUF_ERROR == status)
	    HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, 0, "overflow")
	else if(Z_MEM_ERROR == status)
	    HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, 0, "deflate memory error")
	else if(Z_OK != status)
	    HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, 0, "other deflate error")
        /* Successfully uncompressed the buffer */
        else {
            /* Free the input buffer */
	    H5MM_xfree(*buf);

            /* Set return values */
	    *buf = outbuf;
	    outbuf = NULL;
	    *buf_size = nbytes;
	    ret_value = z_dst_nbytes;
	} /* end else */
    } /* end else */

done:
    if(outbuf)
        H5MM_xfree(outbuf);
    FUNC_LEAVE_NOAPI(ret_value)
}
#endif /* H5_HAVE_FILTER_DEFLATE */

