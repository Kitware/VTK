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
 * Created:		H5Ocache.c
 *			Sep 28 2005
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		Object header metadata cache virtual functions.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5O_PACKAGE		/*suppress error about including H5Opkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free lists                           */
#include "H5MFprivate.h"	/* File memory management		*/
#include "H5Opkg.h"             /* Object headers			*/
#include "H5WBprivate.h"        /* Wrapped Buffers                      */


/****************/
/* Local Macros */
/****************/

/* Set the object header size to speculatively read in */
/* (needs to be more than the object header prefix size to work at all and
 *      should be larger than the largest object type's default object header
 *      size to save the extra I/O operations) */
#define H5O_SPEC_READ_SIZE 512


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

/* Metadata cache callbacks */
static H5O_t *H5O_load(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *udata);
static herr_t H5O_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t addr, H5O_t *oh, unsigned UNUSED * flags_ptr);
static herr_t H5O_dest(H5F_t *f, H5O_t *oh);
static herr_t H5O_clear(H5F_t *f, H5O_t *oh, hbool_t destroy);
static herr_t H5O_size(const H5F_t *f, const H5O_t *oh, size_t *size_ptr);

static H5O_chunk_proxy_t *H5O_cache_chk_load(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *udata);
static herr_t H5O_cache_chk_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t addr, H5O_chunk_proxy_t *chk_proxy, unsigned UNUSED * flags_ptr);
static herr_t H5O_cache_chk_dest(H5F_t *f, H5O_chunk_proxy_t *chk_proxy);
static herr_t H5O_cache_chk_clear(H5F_t *f, H5O_chunk_proxy_t *chk_proxy, hbool_t destroy);
static herr_t H5O_cache_chk_size(const H5F_t *f, const H5O_chunk_proxy_t *chk_proxy, size_t *size_ptr);

/* Chunk proxy routines */
static herr_t H5O_chunk_proxy_dest(H5O_chunk_proxy_t *chunk_proxy);

/* Chunk routines */
static herr_t H5O_chunk_deserialize(H5O_t *oh, haddr_t addr, size_t len,
    const uint8_t *image, H5O_common_cache_ud_t *udata, hbool_t *dirty);
static herr_t H5O_chunk_serialize(const H5F_t *f, H5O_t *oh, unsigned chunkno);

/* Misc. routines */
static herr_t H5O_add_cont_msg(H5O_cont_msgs_t *cont_msg_info,
    const H5O_cont_t *cont);


/*********************/
/* Package Variables */
/*********************/

/* H5O object header prefix inherits cache-like properties from H5AC */
const H5AC_class_t H5AC_OHDR[1] = {{
    H5AC_OHDR_ID,
    (H5AC_load_func_t)H5O_load,
    (H5AC_flush_func_t)H5O_flush,
    (H5AC_dest_func_t)H5O_dest,
    (H5AC_clear_func_t)H5O_clear,
    (H5AC_size_func_t)H5O_size,
}};

/* H5O object header chunk inherits cache-like properties from H5AC */
const H5AC_class_t H5AC_OHDR_CHK[1] = {{
    H5AC_OHDR_CHK_ID,
    (H5AC_load_func_t)H5O_cache_chk_load,
    (H5AC_flush_func_t)H5O_cache_chk_flush,
    (H5AC_dest_func_t)H5O_cache_chk_dest,
    (H5AC_clear_func_t)H5O_cache_chk_clear,
    (H5AC_size_func_t)H5O_cache_chk_size,
}};

/* Declare external the free list for H5O_unknown_t's */
H5FL_EXTERN(H5O_unknown_t);

/* Declare extern the free list for H5O_chunk_proxy_t's */
H5FL_EXTERN(H5O_chunk_proxy_t);

/* Declare the free list for H5O_cont_t sequences */
H5FL_SEQ_DEFINE(H5O_cont_t);


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5O_load
 *
 * Purpose:	Loads an object header from disk.
 *
 * Return:	Success:	Pointer to the new object header.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug  5 1997
 *
 *-------------------------------------------------------------------------
 */
static H5O_t *
H5O_load(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *_udata)
{
    H5O_t	*oh = NULL;     /* Object header read in */
    H5O_cache_ud_t *udata = (H5O_cache_ud_t *)_udata;       /* User data for callback */
    H5WB_t      *wb = NULL;     /* Wrapped buffer for prefix data */
    uint8_t     read_buf[H5O_SPEC_READ_SIZE];       /* Buffer for speculative read */
    const uint8_t *p;           /* Pointer into buffer to decode */
    uint8_t     *buf;           /* Buffer to decode */
    size_t	spec_read_size; /* Size of buffer to speculatively read in */
    size_t	prefix_size;    /* Size of object header prefix */
    size_t	buf_size;       /* Size of prefix+chunk #0 buffer */
    haddr_t     eoa;		/* Relative end of file address	*/
    H5O_t	*ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_load)

    /* Check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(udata);
    HDassert(udata->common.f);
    HDassert(udata->common.cont_msg_info);

    /* Make certain we don't speculatively read off the end of the file */
    if(HADDR_UNDEF == (eoa = H5F_get_eoa(f, H5FD_MEM_OHDR)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, NULL, "unable to determine file size")

    /* Compute the size of the speculative object header buffer */
    H5_ASSIGN_OVERFLOW(spec_read_size, MIN(eoa - addr, H5O_SPEC_READ_SIZE), /* From: */ hsize_t, /* To: */ size_t);

    /* Attempt to speculatively read both object header prefix and first chunk */
    if(H5F_block_read(f, H5FD_MEM_OHDR, addr, spec_read_size, dxpl_id, read_buf) < 0)
	HGOTO_ERROR(H5E_OHDR, H5E_READERROR, NULL, "unable to read object header")
    p = read_buf;

    /* Allocate space for the object header data structure */
    if(NULL == (oh = H5FL_CALLOC(H5O_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* File-specific, non-stored information */
    oh->sizeof_size = H5F_SIZEOF_SIZE(udata->common.f);
    oh->sizeof_addr = H5F_SIZEOF_ADDR(udata->common.f);

    /* Check for presence of magic number */
    /* (indicates version 2 or later) */
    if(!HDmemcmp(p, H5O_HDR_MAGIC, (size_t)H5_SIZEOF_MAGIC)) {
        /* Magic number */
        p += H5_SIZEOF_MAGIC;

        /* Version */
        oh->version = *p++;
        if(H5O_VERSION_2 != oh->version)
            HGOTO_ERROR(H5E_OHDR, H5E_VERSION, NULL, "bad object header version number")

        /* Flags */
        oh->flags = *p++;
        if(oh->flags & ~H5O_HDR_ALL_FLAGS)
            HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, NULL, "unknown object header status flag(s)")

        /* Number of links to object (unless overridden by refcount message) */
        oh->nlink = 1;

        /* Time fields */
        if(oh->flags & H5O_HDR_STORE_TIMES) {
            uint32_t tmp;       /* Temporary value */

            UINT32DECODE(p, tmp);
            oh->atime = (time_t)tmp;
            UINT32DECODE(p, tmp);
            oh->mtime = (time_t)tmp;
            UINT32DECODE(p, tmp);
            oh->ctime = (time_t)tmp;
            UINT32DECODE(p, tmp);
            oh->btime = (time_t)tmp;
        } /* end if */
        else
            oh->atime = oh->mtime = oh->ctime = oh->btime = 0;

        /* Attribute fields */
        if(oh->flags & H5O_HDR_ATTR_STORE_PHASE_CHANGE) {
            UINT16DECODE(p, oh->max_compact);
            UINT16DECODE(p, oh->min_dense);
            if(oh->max_compact < oh->min_dense)
                HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, NULL, "bad object header attribute phase change values")
        } /* end if */
        else {
            oh->max_compact = H5O_CRT_ATTR_MAX_COMPACT_DEF;
            oh->min_dense = H5O_CRT_ATTR_MIN_DENSE_DEF;
        } /* end else */

        /* First chunk size */
        switch(oh->flags & H5O_HDR_CHUNK0_SIZE) {
            case 0:     /* 1 byte size */
                oh->chunk0_size = *p++;
                break;

            case 1:     /* 2 byte size */
                UINT16DECODE(p, oh->chunk0_size);
                break;

            case 2:     /* 4 byte size */
                UINT32DECODE(p, oh->chunk0_size);
                break;

            case 3:     /* 8 byte size */
                UINT64DECODE(p, oh->chunk0_size);
                break;

            default:
                HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, NULL, "bad size for chunk 0")
        } /* end switch */
        if(oh->chunk0_size > 0 && oh->chunk0_size < H5O_SIZEOF_MSGHDR_OH(oh))
            HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, NULL, "bad object header chunk size")
    } /* end if */
    else {
        /* Version */
        oh->version = *p++;
        if(H5O_VERSION_1 != oh->version)
            HGOTO_ERROR(H5E_OHDR, H5E_VERSION, NULL, "bad object header version number")

        /* Flags */
        oh->flags = H5O_CRT_OHDR_FLAGS_DEF;

        /* Reserved */
        p++;

        /* Number of messages */
        UINT16DECODE(p, udata->v1_pfx_nmesgs);

        /* Link count */
        UINT32DECODE(p, oh->nlink);

        /* Reset unused time fields */
        oh->atime = oh->mtime = oh->ctime = oh->btime = 0;

        /* Reset unused attribute fields */
        oh->max_compact = 0;
        oh->min_dense = 0;

        /* First chunk size */
        UINT32DECODE(p, oh->chunk0_size);
        if((udata->v1_pfx_nmesgs > 0 && oh->chunk0_size < H5O_SIZEOF_MSGHDR_OH(oh)) ||
                (udata->v1_pfx_nmesgs == 0 && oh->chunk0_size > 0))
            HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, NULL, "bad object header chunk size")

        /* Reserved, in version 1 (for 8-byte alignment padding) */
        p += 4;
    } /* end else */

    /* Determine object header prefix length */
    prefix_size = (size_t)(p - (const uint8_t *)read_buf);
    HDassert((size_t)prefix_size == (size_t)(H5O_SIZEOF_HDR(oh) - H5O_SIZEOF_CHKSUM_OH(oh)));

    /* Compute the size of the buffer used */
    buf_size = oh->chunk0_size + (size_t)H5O_SIZEOF_HDR(oh);

    /* Check if the speculative read was large enough to parse the first chunk */
    if(spec_read_size < buf_size) {
        /* Wrap the local buffer for serialized header info */
        if(NULL == (wb = H5WB_wrap(read_buf, sizeof(read_buf))))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "can't wrap buffer")

        /* Get a pointer to a buffer that's large enough for serialized header */
        if(NULL == (buf = (uint8_t *)H5WB_actual(wb, buf_size)))
            HGOTO_ERROR(H5E_OHDR, H5E_NOSPACE, NULL, "can't get actual buffer")

        /* Copy existing raw data into new buffer */
        HDmemcpy(buf, read_buf, spec_read_size);

        /* Read rest of the raw data */
        if(H5F_block_read(f, H5FD_MEM_OHDR, (addr + spec_read_size), (buf_size - spec_read_size), dxpl_id, (buf + spec_read_size)) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_READERROR, NULL, "unable to read object header data")
    } /* end if */
    else
        buf = read_buf;

    /* Parse the first chunk */
    if(H5O_chunk_deserialize(oh, udata->common.addr, oh->chunk0_size, buf, &(udata->common), &oh->cache_info.is_dirty) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "can't deserialize first object header chunk")

    /* Note that we've loaded the object header from the file */
    udata->made_attempt = TRUE;

    /* Set return value */
    ret_value = oh;

done:
    /* Release resources */
    if(wb && H5WB_unwrap(wb) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CLOSEERROR, NULL, "can't close wrapped buffer")

    /* Release the [possibly partially initialized] object header on errors */
    if(!ret_value && oh)
        if(H5O_free(oh) < 0)
	    HDONE_ERROR(H5E_OHDR, H5E_CANTRELEASE, NULL, "unable to destroy object header data")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_load() */


/*-------------------------------------------------------------------------
 * Function:	H5O_flush
 *
 * Purpose:	Flushes (and destroys) an object header.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug  5 1997
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t UNUSED addr, H5O_t *oh, unsigned UNUSED * flags_ptr)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_flush)

    /* Check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(oh);

    /* flush */
    if(oh->cache_info.is_dirty) {
        uint8_t	*p;             /* Pointer to object header prefix buffer */

#ifdef H5O_DEBUG
H5O_assert(oh);
#endif /* H5O_DEBUG */

        /* Point to raw data 'image' for first chunk, which has room for the prefix */
        p = oh->chunk[0].image;

        /* Later versions of object header prefix have different format and
         * also require that chunk 0 always be updated, since the checksum
         * on the entire block of memory needs to be updated if anything is
         * modified */
        if(oh->version > H5O_VERSION_1) {
            uint64_t chunk0_size;       /* Size of chunk 0's data */

            HDassert(oh->chunk[0].size >= (size_t)H5O_SIZEOF_HDR(oh));
            chunk0_size = oh->chunk[0].size - (size_t)H5O_SIZEOF_HDR(oh);

            /* Verify magic number */
            HDassert(!HDmemcmp(p, H5O_HDR_MAGIC, H5_SIZEOF_MAGIC));
            p += H5_SIZEOF_MAGIC;

            /* Version */
            *p++ = oh->version;

            /* Flags */
            *p++ = oh->flags;

            /* Time fields */
            if(oh->flags & H5O_HDR_STORE_TIMES) {
                UINT32ENCODE(p, oh->atime);
                UINT32ENCODE(p, oh->mtime);
                UINT32ENCODE(p, oh->ctime);
                UINT32ENCODE(p, oh->btime);
            } /* end if */

            /* Attribute fields */
            if(oh->flags & H5O_HDR_ATTR_STORE_PHASE_CHANGE) {
                UINT16ENCODE(p, oh->max_compact);
                UINT16ENCODE(p, oh->min_dense);
            } /* end if */

            /* First chunk size */
            switch(oh->flags & H5O_HDR_CHUNK0_SIZE) {
                case 0:     /* 1 byte size */
                    HDassert(chunk0_size < 256);
                    *p++ = (uint8_t)chunk0_size;
                    break;

                case 1:     /* 2 byte size */
                    HDassert(chunk0_size < 65536);
                    UINT16ENCODE(p, chunk0_size);
                    break;

                case 2:     /* 4 byte size */
		    /* use <= 2**32 -1 to stay within 4 bytes integer range */
                    HDassert(chunk0_size <= 4294967295UL);
                    UINT32ENCODE(p, chunk0_size);
                    break;

                case 3:     /* 8 byte size */
                    UINT64ENCODE(p, chunk0_size);
                    break;

                default:
                    HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, FAIL, "bad size for chunk 0")
            } /* end switch */
        } /* end if */
        else {
            /* Version */
            *p++ = oh->version;

            /* Reserved */
            *p++ = 0;

            /* Number of messages */
#ifdef H5O_ENABLE_BAD_MESG_COUNT
            if(oh->store_bad_mesg_count)
                UINT16ENCODE(p, (oh->nmesgs - 1))
            else
#endif /* H5O_ENABLE_BAD_MESG_COUNT */
                UINT16ENCODE(p, oh->nmesgs);

            /* Link count */
            UINT32ENCODE(p, oh->nlink);

            /* First chunk size */
            UINT32ENCODE(p, (oh->chunk[0].size - (size_t)H5O_SIZEOF_HDR(oh)));

            /* Zero to alignment */
            HDmemset(p, 0, (size_t)(H5O_SIZEOF_HDR(oh) - 12));
            p += (size_t)(H5O_SIZEOF_HDR(oh) - 12);
        } /* end else */
        HDassert((size_t)(p - oh->chunk[0].image) == (size_t)(H5O_SIZEOF_HDR(oh) - H5O_SIZEOF_CHKSUM_OH(oh)));

        /* Serialize messages for this chunk */
        if(H5O_chunk_serialize(f, oh, (unsigned)0) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTSERIALIZE, FAIL, "unable to serialize first object header chunk")

        /* Write the chunk out */
        HDassert(H5F_addr_defined(oh->chunk[0].addr));
        if(H5F_block_write(f, H5FD_MEM_OHDR, oh->chunk[0].addr, oh->chunk[0].size, dxpl_id, oh->chunk[0].image) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_WRITEERROR, FAIL, "unable to write object header chunk to disk")

        /* Mark object header as clean now */
	oh->cache_info.is_dirty = FALSE;
    } /* end if */

    /* Destroy the object header, if requested */
    if(destroy)
        if(H5O_dest(f, oh) < 0)
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to destroy object header data")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_flush() */


/*-------------------------------------------------------------------------
 * Function:	H5O_dest
 *
 * Purpose:	Destroys an object header.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Jan 15 2003
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_dest(H5F_t *f, H5O_t *oh)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_dest)

    /* check args */
    HDassert(oh);
    HDassert(oh->rc == 0);

    /* Verify that node is clean */
    HDassert(!oh->cache_info.is_dirty);

    /* If we're going to free the space on disk, the address must be valid */
    HDassert(!oh->cache_info.free_file_space_on_destroy || H5F_addr_defined(oh->cache_info.addr));

    /* Check for releasing file space for object header */
    if(oh->chunk && oh->cache_info.free_file_space_on_destroy) {
        /* Free main (first) object header "chunk" */
        /* (XXX: Nasty usage of internal DXPL value! -QAK) */
        if(H5MF_xfree(f, H5FD_MEM_OHDR, H5AC_dxpl_id, oh->chunk[0].addr, (hsize_t)oh->chunk[0].size) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to free object header")
    } /* end if */

    /* Destroy object header */
    if(H5O_free(oh) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTRELEASE, FAIL, "can't destroy object header")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_dest() */


/*-------------------------------------------------------------------------
 * Function:	H5O_clear
 *
 * Purpose:	Mark a object header in memory as non-dirty.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar 20 2003
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_clear(H5F_t *f, H5O_t *oh, hbool_t destroy)
{
    unsigned	u;      /* Local index variable */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5O_clear)

    /* check args */
    HDassert(oh);

    /* Mark messages as clean */
    for(u = 0; u < oh->nmesgs; u++)
        oh->mesg[u].dirty = FALSE;

#ifndef NDEBUG
    /* Reset the number of messages dirtied by decoding */
    oh->ndecode_dirtied = 0;
#endif /* NDEBUG */

    /* Mark whole header as clean */
    oh->cache_info.is_dirty = FALSE;

    if(destroy)
        if(H5O_dest(f, oh) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to destroy object header data")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_clear() */


/*-------------------------------------------------------------------------
 * Function:	H5O_size
 *
 * Purpose:	Compute the size in bytes of the specified instance of
 *              H5O_t on disk, and return it in *len_ptr.  On failure,
 *              the value of *len_ptr is undefined.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	John Mainzer
 *		5/13/04
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_size(const H5F_t UNUSED *f, const H5O_t *oh, size_t *size_ptr)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_size)

    /* check args */
    HDassert(oh);
    HDassert(size_ptr);

    /* Report the object header's prefix+first chunk length */
    *size_ptr = (size_t)H5O_SIZEOF_HDR(oh) + oh->chunk0_size;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5O_size() */


/*-------------------------------------------------------------------------
 * Function:	H5O_cache_chk_load
 *
 * Purpose:	Loads an object header continuation chunk from disk.
 *
 * Return:	Success:	Pointer to the new object header chunk proxy.
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jul 12 2008
 *
 *-------------------------------------------------------------------------
 */
static H5O_chunk_proxy_t *
H5O_cache_chk_load(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *_udata)
{
    H5O_chunk_proxy_t	*chk_proxy = NULL;     /* Chunk proxy object */
    H5O_chk_cache_ud_t *udata = (H5O_chk_cache_ud_t *)_udata;       /* User data for callback */
    H5WB_t      *wb = NULL;             /* Wrapped buffer for prefix data */
    uint8_t     chunk_buf[H5O_SPEC_READ_SIZE];       /* Buffer for speculative read */
    uint8_t     *buf;                   /* Buffer to decode */
    H5O_chunk_proxy_t	*ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_cache_chk_load)

    /* Check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(udata);
    HDassert(udata->oh);

    /* Allocate space for the object header data structure */
    if(NULL == (chk_proxy = H5FL_CALLOC(H5O_chunk_proxy_t)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTALLOC, NULL, "memory allocation failed")

    /* Wrap the local buffer for serialized header info */
    if(NULL == (wb = H5WB_wrap(chunk_buf, sizeof(chunk_buf))))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "can't wrap buffer")

    /* Get a pointer to a buffer that's large enough for serialized header */
    if(NULL == (buf = (uint8_t *)H5WB_actual(wb, udata->chunk_size)))
        HGOTO_ERROR(H5E_OHDR, H5E_NOSPACE, NULL, "can't get actual buffer")

    /* Read rest of the raw data */
    if(H5F_block_read(f, H5FD_MEM_OHDR, addr, udata->chunk_size, dxpl_id, buf) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_READERROR, NULL, "unable to read object header continuation chunk")

    /* Check if we are still decoding the object header */
    /* (as opposed to bringing a piece of it back from the file) */
    if(udata->decoding) {
        /* Sanity check */
        HDassert(udata->common.f);
        HDassert(udata->common.cont_msg_info);

        /* Parse the chunk */
        if(H5O_chunk_deserialize(udata->oh, udata->common.addr, udata->chunk_size, buf, &(udata->common), &chk_proxy->cache_info.is_dirty) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "can't deserialize object header chunk")

        /* Set the fields for the chunk proxy */
        chk_proxy->oh = udata->oh;
        chk_proxy->chunkno = udata->oh->nchunks - 1;
    } /* end if */
    else {
        /* Sanity check */
        HDassert(udata->chunkno < udata->oh->nchunks);

        /* Set the fields for the chunk proxy */
        chk_proxy->oh = udata->oh;
        chk_proxy->chunkno = udata->chunkno;

        /* Sanity check that the chunk representation we have in memory is the same
         *      as the one being brought in from disk.
         */
        HDassert(0 == HDmemcmp(buf, chk_proxy->oh->chunk[chk_proxy->chunkno].image, chk_proxy->oh->chunk[chk_proxy->chunkno].size));
    } /* end else */

    /* Increment reference count of object header */
    if(H5O_inc_rc(udata->oh) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINC, NULL, "can't increment reference count on object header")

    /* Set return value */
    ret_value = chk_proxy;

done:
    /* Release resources */
    if(wb && H5WB_unwrap(wb) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CLOSEERROR, NULL, "can't close wrapped buffer")

    /* Release the [possibly partially initialized] object header on errors */
    if(!ret_value && chk_proxy)
        if(H5O_chunk_proxy_dest(chk_proxy) < 0)
	    HDONE_ERROR(H5E_OHDR, H5E_CANTRELEASE, NULL, "unable to destroy object header chunk proxy")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_cache_chk_load() */


/*-------------------------------------------------------------------------
 * Function:	H5O_cache_chk_flush
 *
 * Purpose:	Flushes (and destroys) an object header continuation chunk.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jul 12 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_cache_chk_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t addr,
    H5O_chunk_proxy_t *chk_proxy, unsigned UNUSED * flags_ptr)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_cache_chk_flush)

    /* flush */
    if(chk_proxy->cache_info.is_dirty) {
        /* Serialize messages for this chunk */
        if(H5O_chunk_serialize(f, chk_proxy->oh, chk_proxy->chunkno) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTSERIALIZE, FAIL, "unable to serialize object header continuation chunk")

        /* Write the chunk out */
        HDassert(H5F_addr_defined(chk_proxy->oh->chunk[chk_proxy->chunkno].addr));
        HDassert(H5F_addr_eq(addr, chk_proxy->oh->chunk[chk_proxy->chunkno].addr));
        if(H5F_block_write(f, H5FD_MEM_OHDR, addr, chk_proxy->oh->chunk[chk_proxy->chunkno].size, dxpl_id, chk_proxy->oh->chunk[chk_proxy->chunkno].image) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_WRITEERROR, FAIL, "unable to write object header continuation chunk to disk")

        /* Mark object header as clean now */
	chk_proxy->cache_info.is_dirty = FALSE;
    } /* end if */

    /* Destroy the object header, if requested */
    if(destroy)
        if(H5O_cache_chk_dest(f, chk_proxy) < 0)
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to destroy object header continuation chunk data")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_cache_chk_flush() */


/*-------------------------------------------------------------------------
 * Function:	H5O_cache_chk_dest
 *
 * Purpose:	Destroys an object header continuation chunk.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              koziol@hdfgroup.org
 *              July 12, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_cache_chk_dest(H5F_t *f, H5O_chunk_proxy_t *chk_proxy)
{
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_cache_chk_dest)

    /* Check arguments */
    HDassert(chk_proxy);
    HDassert(chk_proxy->chunkno > 0);

    /* Verify that node is clean */
    HDassert(chk_proxy->cache_info.is_dirty == FALSE);

    /* If we're going to free the space on disk, the address must be valid */
    HDassert(!chk_proxy->cache_info.free_file_space_on_destroy || H5F_addr_defined(chk_proxy->cache_info.addr));

    /* Check for releasing file space for object header */
    if(chk_proxy->cache_info.free_file_space_on_destroy) {
        /* Release the space on disk */
        /* (XXX: Nasty usage of internal DXPL value! -QAK) */
        if(H5MF_xfree(f, H5FD_MEM_OHDR, H5AC_dxpl_id, chk_proxy->oh->chunk[chk_proxy->chunkno].addr, (hsize_t)chk_proxy->oh->chunk[chk_proxy->chunkno].size) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to free object header continuation chunk")
    } /* end if */

    /* Destroy object header chunk proxy */
    if(H5O_chunk_proxy_dest(chk_proxy) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTRELEASE, FAIL, "unable to destroy object header chunk proxy")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_cache_chk_dest() */


/*-------------------------------------------------------------------------
 * Function:	H5O_cache_chk_clear
 *
 * Purpose:	Mark a object header continuation chunk in memory as non-dirty.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              koziol@hdfgroup.org
 *              July 12, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_cache_chk_clear(H5F_t *f, H5O_chunk_proxy_t *chk_proxy, hbool_t destroy)
{
    unsigned	u;      /* Local index variable */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5O_cache_chk_clear)

    /* check args */
    HDassert(chk_proxy);

    /* Mark messages in chunk as clean */
    for(u = 0; u < chk_proxy->oh->nmesgs; u++)
        if(chk_proxy->oh->mesg[u].chunkno == chk_proxy->chunkno)
            chk_proxy->oh->mesg[u].dirty = FALSE;

    /* Mark as clean */
    chk_proxy->cache_info.is_dirty = FALSE;

    if(destroy)
        if(H5O_cache_chk_dest(f, chk_proxy) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to destroy object header continuation chunk data")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_cache_chk_clear() */


/*-------------------------------------------------------------------------
 * Function:	H5O_cache_chk_size
 *
 * Purpose:	Compute the size in bytes of the specified instance of
 *              an object header continuation chunk on disk, and return it in
 *              *len_ptr.  On failure, the value of *len_ptr is undefined.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              koziol@hdfgroup.org
 *              July 12, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_cache_chk_size(const H5F_t UNUSED *f, const H5O_chunk_proxy_t *chk_proxy, size_t *size_ptr)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_cache_chk_size)

    /* check args */
    HDassert(chk_proxy);
    HDassert(size_ptr);

    /* Report the object header continuation chunk's length */
    *size_ptr = chk_proxy->oh->chunk[chk_proxy->chunkno].size;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5O_cache_chk_size() */


/*-------------------------------------------------------------------------
 * Function:	H5O_add_cont_msg
 *
 * Purpose:	Add information from a continuation message to the list of
 *              continuation messages in the object header
 *
 * Return:	Success: SUCCEED
 *              Failure: FAIL
 *
 * Programmer:	Quincey Koziol
 *              koziol@hdfgroup.org
 *              July 12, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_add_cont_msg(H5O_cont_msgs_t *cont_msg_info, const H5O_cont_t *cont)
{
    unsigned contno;            /* Continuation message index */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_add_cont_msg)

    /* Check arguments */
    HDassert(cont_msg_info);
    HDassert(cont);

    /* Increase chunk array size, if necessary */
    if(cont_msg_info->nmsgs >= cont_msg_info->alloc_nmsgs) {
        size_t na = MAX(H5O_NCHUNKS, cont_msg_info->alloc_nmsgs * 2);        /* Double # of messages allocated */
        H5O_cont_t *x;

        if(NULL == (x = H5FL_SEQ_REALLOC(H5O_cont_t, cont_msg_info->msgs, na)))
            HGOTO_ERROR(H5E_OHDR, H5E_NOSPACE, FAIL, "memory allocation failed")
        cont_msg_info->alloc_nmsgs = na;
        cont_msg_info->msgs = x;
    } /* end if */

    /* Init the continuation message info */
    contno = cont_msg_info->nmsgs++;
    cont_msg_info->msgs[contno].addr = cont->addr;
    cont_msg_info->msgs[contno].size = cont->size;
    cont_msg_info->msgs[contno].chunkno = cont->chunkno;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_add_cont_msg() */


/*-------------------------------------------------------------------------
 * Function:	H5O_chunk_deserialize
 *
 * Purpose:	Deserialize a chunk for an object header
 *
 * Return:	Success: SUCCEED
 *              Failure: FAIL
 *
 * Programmer:	Quincey Koziol
 *              koziol@hdfgroup.org
 *              July 12, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_chunk_deserialize(H5O_t *oh, haddr_t addr, size_t len, const uint8_t *image,
    H5O_common_cache_ud_t *udata, hbool_t *dirty)
{
    const uint8_t *p;           /* Pointer into buffer to decode */
    uint8_t *eom_ptr;           /* Pointer to end of messages for a chunk */
    unsigned curmesg;           /* Current message being decoded in object header */
    unsigned merged_null_msgs = 0;  /* Number of null messages merged together */
    unsigned chunkno;           /* Current chunk's index */
#ifndef NDEBUG
    unsigned nullcnt;           /* Count of null messages (for sanity checking gaps in chunks) */
#endif /* NDEBUG */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_chunk_deserialize)

    /* Check arguments */
    HDassert(oh);
    HDassert(H5F_addr_defined(addr));
    HDassert(image);
    HDassert(udata->f);
    HDassert(udata->cont_msg_info);

    /* Increase chunk array size, if necessary */
    if(oh->nchunks >= oh->alloc_nchunks) {
        size_t na = MAX(H5O_NCHUNKS, oh->alloc_nchunks * 2);        /* Double # of chunks allocated */
        H5O_chunk_t *x;

        if(NULL == (x = H5FL_SEQ_REALLOC(H5O_chunk_t, oh->chunk, na)))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTALLOC, FAIL, "memory allocation failed")
        oh->alloc_nchunks = na;
        oh->chunk = x;
    } /* end if */

    /* Init the chunk data info */
    chunkno = oh->nchunks++;
    oh->chunk[chunkno].gap = 0;
    if(chunkno == 0) {
        /* First chunk's 'image' includes room for the object header prefix */
        oh->chunk[0].addr = addr;
        oh->chunk[0].size = len + (size_t)H5O_SIZEOF_HDR(oh);
    } /* end if */
    else {
        oh->chunk[chunkno].addr = addr;
        oh->chunk[chunkno].size = len;
    } /* end else */
    if(NULL == (oh->chunk[chunkno].image = H5FL_BLK_MALLOC(chunk_image, oh->chunk[chunkno].size)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTALLOC, FAIL, "memory allocation failed")

    /* Copy disk image into chunk's image */
    HDmemcpy(oh->chunk[chunkno].image, image, oh->chunk[chunkno].size);

    /* Point into chunk image to decode */
    p = oh->chunk[chunkno].image;

    /* Handle chunk 0 as special case */
    if(chunkno == 0)
        /* Skip over [already decoded] prefix */
        p += (size_t)(H5O_SIZEOF_HDR(oh) - H5O_SIZEOF_CHKSUM_OH(oh));
    /* Check for magic # on chunks > 0 in later versions of the format */
    else if(chunkno > 0 && oh->version > H5O_VERSION_1) {
        /* Magic number */
        if(HDmemcmp(p, H5O_CHK_MAGIC, (size_t)H5_SIZEOF_MAGIC))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, FAIL, "wrong object header chunk signature")
        p += H5_SIZEOF_MAGIC;
    } /* end if */

    /* Save # of messages already inspected */
    curmesg = oh->nmesgs;

    /* Decode messages from this chunk */
    eom_ptr = oh->chunk[chunkno].image + (oh->chunk[chunkno].size - H5O_SIZEOF_CHKSUM_OH(oh));
#ifndef NDEBUG
    nullcnt = 0;
#endif /* NDEBUG */
    while(p < eom_ptr) {
        unsigned mesgno;        /* Current message to operate on */
        size_t mesg_size;       /* Size of message read in */
        unsigned id;            /* ID (type) of current message */
        uint8_t	flags;          /* Flags for current message */
        H5O_msg_crt_idx_t crt_idx = 0;  /* Creation index for current message */

        /* Decode message prefix info */

        /* Version # */
        if(oh->version == H5O_VERSION_1)
            UINT16DECODE(p, id)
        else
            id = *p++;

        /* Check for unknown message ID getting encoded in file */
        if(id == H5O_UNKNOWN_ID)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, FAIL, "'unknown' message ID encoded in file?!?")

        /* Message size */
        UINT16DECODE(p, mesg_size);
        HDassert(mesg_size == H5O_ALIGN_OH(oh, mesg_size));

        /* Message flags */
        flags = *p++;
        if(flags & ~H5O_MSG_FLAG_BITS)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, FAIL, "unknown flag for message")
        if((flags & H5O_MSG_FLAG_SHARED) && (flags & H5O_MSG_FLAG_DONTSHARE))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, FAIL, "bad flag combination for message")
        if((flags & H5O_MSG_FLAG_WAS_UNKNOWN) && (flags & H5O_MSG_FLAG_FAIL_IF_UNKNOWN))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, FAIL, "bad flag combination for message")
        if((flags & H5O_MSG_FLAG_WAS_UNKNOWN) && !(flags & H5O_MSG_FLAG_MARK_IF_UNKNOWN))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, FAIL, "bad flag combination for message")

        /* Reserved bytes/creation index */
        if(oh->version == H5O_VERSION_1)
            p += 3; /*reserved*/
        else {
            /* Only decode creation index if they are being tracked */
            if(oh->flags & H5O_HDR_ATTR_CRT_ORDER_TRACKED)
                UINT16DECODE(p, crt_idx);
        } /* end else */

        /* Try to detect invalidly formatted object header message that
         *  extends past end of chunk.
         */
        if(p + mesg_size > eom_ptr)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "corrupt object header")

#ifndef NDEBUG
        /* Increment count of null messages */
        if(H5O_NULL_ID == id)
            nullcnt++;
#endif /* NDEBUG */

        /* Check for combining two adjacent 'null' messages */
        if((udata->file_intent & H5F_ACC_RDWR) &&
                H5O_NULL_ID == id && oh->nmesgs > 0 &&
                H5O_NULL_ID == oh->mesg[oh->nmesgs - 1].type->id &&
                oh->mesg[oh->nmesgs - 1].chunkno == chunkno) {

            /* Combine adjacent null messages */
            mesgno = oh->nmesgs - 1;
            oh->mesg[mesgno].raw_size += H5O_SIZEOF_MSGHDR_OH(oh) + mesg_size;
            oh->mesg[mesgno].dirty = TRUE;
            merged_null_msgs++;
            udata->merged_null_msgs++;
        } /* end if */
        else {
            /* Check if we need to extend message table to hold the new message */
            if(oh->nmesgs >= oh->alloc_nmesgs)
                if(H5O_alloc_msgs(oh, (size_t)1) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTALLOC, FAIL, "can't allocate more space for messages")

            /* Get index for message */
            mesgno = oh->nmesgs++;

            /* Initialize information about message */
            oh->mesg[mesgno].dirty = FALSE;
            oh->mesg[mesgno].flags = flags;
            oh->mesg[mesgno].crt_idx = crt_idx;
            oh->mesg[mesgno].native = NULL;
            oh->mesg[mesgno].raw = (uint8_t *)p;        /* Casting away const OK - QAK */
            oh->mesg[mesgno].raw_size = mesg_size;
            oh->mesg[mesgno].chunkno = chunkno;

            /* Point unknown messages at 'unknown' message class */
            /* (Usually from future versions of the library) */
            if(id >= NELMTS(H5O_msg_class_g) || NULL == H5O_msg_class_g[id]) {
                H5O_unknown_t *unknown;     /* Pointer to "unknown" message info */

                /* Allocate "unknown" message info */
                if(NULL == (unknown = H5FL_MALLOC(H5O_unknown_t)))
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTALLOC, FAIL, "memory allocation failed")

                /* Save the original message type ID */
                *unknown = id;

                /* Save 'native' form of unknown message */
                oh->mesg[mesgno].native = unknown;

                /* Set message to "unknown" class */
                oh->mesg[mesgno].type = H5O_msg_class_g[H5O_UNKNOWN_ID];

                /* Check for "fail if unknown" message flag */
                if(flags & H5O_MSG_FLAG_FAIL_IF_UNKNOWN)
                    HGOTO_ERROR(H5E_OHDR, H5E_BADMESG, FAIL, "unknown message with 'fail if unknown' flag found")
                /* Check for "mark if unknown" message flag, etc. */
                else if((flags & H5O_MSG_FLAG_MARK_IF_UNKNOWN) &&
                        !(flags & H5O_MSG_FLAG_WAS_UNKNOWN) &&
                        (udata->file_intent & H5F_ACC_RDWR)) {

                    /* Mark the message as "unknown" */
                    /* This is a bit aggressive, since the application may
                     * never change anything about the object (metadata or
                     * raw data), but we can sort out the finer details
                     * when/if we start using the flag - QAK
                     */
                    /* Also, it's possible that this functionality may not
                     * get invoked if the object header is brought into
                     * the metadata cache in some other "weird" way, like
                     * using H5Ocopy() - QAK
                     */
                    oh->mesg[mesgno].flags |= H5O_MSG_FLAG_WAS_UNKNOWN;

                    /* Mark the message and chunk as dirty */
                    oh->mesg[mesgno].dirty = TRUE;
                    udata->mesgs_modified = TRUE;
                    *dirty = TRUE;
                } /* end if */
            } /* end if */
            else
                /* Set message class for "known" messages */
                oh->mesg[mesgno].type = H5O_msg_class_g[id];
        } /* end else */

        /* Advance decode pointer past message */
        p += mesg_size;

        /* Check for 'gap' at end of chunk */
        if((eom_ptr - p) > 0 && (eom_ptr - p) < H5O_SIZEOF_MSGHDR_OH(oh)) {
            /* Gaps can only occur in later versions of the format */
            HDassert(oh->version > H5O_VERSION_1);

            /* Gaps should only occur in chunks with no null messages */
            HDassert(nullcnt == 0);

            /* Set gap information for chunk */
            oh->chunk[chunkno].gap = (size_t)(eom_ptr - p);

            /* Increment location in chunk */
            p += oh->chunk[chunkno].gap;
        } /* end if */
    } /* end while */

    /* Check for correct checksum on chunks, in later versions of the format */
    if(oh->version > H5O_VERSION_1) {
        uint32_t stored_chksum;     /* Checksum from file */
        uint32_t computed_chksum;   /* Checksum computed in memory */

        /* Metadata checksum */
        UINT32DECODE(p, stored_chksum);

        /* Compute checksum on chunk */
        computed_chksum = H5_checksum_metadata(oh->chunk[chunkno].image, (oh->chunk[chunkno].size - H5O_SIZEOF_CHKSUM), 0);

        /* Verify checksum */
        if(stored_chksum != computed_chksum)
            HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, FAIL, "incorrect metadata checksum for object header chunk")
    } /* end if */

    /* Sanity check */
    HDassert(p == oh->chunk[chunkno].image + oh->chunk[chunkno].size);

    /* Do some inspection/interpretation of new messages from this chunk */
    /* (detect continuation messages, ref. count messages, etc.) */
    while(curmesg < oh->nmesgs) {
        /* Check if next message to examine is a continuation message */
        if(H5O_CONT_ID == oh->mesg[curmesg].type->id) {
            H5O_cont_t *cont;
            unsigned ioflags = 0;   /* Flags for decode routine */

            /* Decode continuation message */
            cont = (H5O_cont_t *)(H5O_MSG_CONT->decode)(udata->f, udata->dxpl_id, NULL, 0, &ioflags, oh->mesg[curmesg].raw);
            cont->chunkno = udata->cont_msg_info->nmsgs + 1;	/*the next continuation message/chunk */

            /* Save 'native' form of continuation message */
            oh->mesg[curmesg].native = cont;

            /* Add to continuation messages left to interpret */
            if(H5O_add_cont_msg(udata->cont_msg_info, cont) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "can't add continuation message")

            /* Mark the message & chunk as dirty if the message was changed by decoding */
            if((ioflags & H5O_DECODEIO_DIRTY) && (udata->file_intent & H5F_ACC_RDWR)) {
                oh->mesg[curmesg].dirty = TRUE;
                udata->mesgs_modified = TRUE;
                *dirty = TRUE;
            } /* end if */
        } /* end if */
        /* Check if next message to examine is a ref. count message */
        else if(H5O_REFCOUNT_ID == oh->mesg[curmesg].type->id) {
            H5O_refcount_t *refcount;
            unsigned ioflags = 0;   /* Flags for decode routine */

            /* Decode ref. count message */
            HDassert(oh->version > H5O_VERSION_1);
            refcount = (H5O_refcount_t *)(H5O_MSG_REFCOUNT->decode)(udata->f, udata->dxpl_id, NULL, 0, &ioflags, oh->mesg[curmesg].raw);

            /* Save 'native' form of ref. count message */
            oh->mesg[curmesg].native = refcount;

            /* Set object header values */
            oh->has_refcount_msg = TRUE;
            oh->nlink = *refcount;

            /* Mark the message & chunk as dirty if the message was changed by decoding */
            if((ioflags & H5O_DECODEIO_DIRTY) && (udata->file_intent & H5F_ACC_RDWR)) {
                oh->mesg[curmesg].dirty = TRUE;
                udata->mesgs_modified = TRUE;
                *dirty = TRUE;
            } /* end if */
        } /* end if */
        /* Check if next message to examine is a link message */
        else if(H5O_LINK_ID == oh->mesg[curmesg].type->id) {
            /* Increment the count of link messages */
            oh->link_msgs_seen++;
        } /* end if */
        /* Check if next message to examine is an attribute message */
        else if(H5O_ATTR_ID == oh->mesg[curmesg].type->id) {
            /* Increment the count of attribute messages */
            oh->attr_msgs_seen++;
        } /* end if */

        /* Advance to next message */
        curmesg++;
    } /* end while */

    /* Mark the chunk dirty if we've merged null messages */
    if(merged_null_msgs) {
        udata->mesgs_modified = TRUE;
	*dirty = TRUE;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_chunk_deserialize() */


/*-------------------------------------------------------------------------
 * Function:	H5O_chunk_serialize
 *
 * Purpose:	Serialize a chunk for an object header
 *
 * Return:	Success: SUCCEED
 *              Failure: FAIL
 *
 * Programmer:	Quincey Koziol
 *              koziol@hdfgroup.org
 *              July 12, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_chunk_serialize(const H5F_t *f, H5O_t *oh, unsigned chunkno)
{
    H5O_mesg_t *curr_msg;       /* Pointer to current message being operated on */
    unsigned	u;              /* Local index variable */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_chunk_serialize)

    /* Check arguments */
    HDassert(f);
    HDassert(oh);

    /* Encode any dirty messages in this chunk */
    for(u = 0, curr_msg = &oh->mesg[0]; u < oh->nmesgs; u++, curr_msg++)
        if(curr_msg->dirty && curr_msg->chunkno == chunkno)
            /* Casting away const OK -QAK */
            if(H5O_msg_flush((H5F_t *)f, oh, curr_msg) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTENCODE, FAIL, "unable to encode object header message")

    /* Sanity checks */
    if(oh->version > H5O_VERSION_1)
        /* Make certain the magic # is present */
        HDassert(!HDmemcmp(oh->chunk[chunkno].image, (chunkno == 0 ? H5O_HDR_MAGIC : H5O_CHK_MAGIC), H5_SIZEOF_MAGIC));
    else
        /* Gaps should never occur in version 1 of the format */
        HDassert(oh->chunk[chunkno].gap == 0);

    /* Extra work, for later versions of the format */
    if(oh->version > H5O_VERSION_1) {
        uint32_t metadata_chksum;   /* Computed metadata checksum value */
        uint8_t	*p;                 /* Pointer into object header chunk */

        /* Check for gap in chunk & zero it out */
        if(oh->chunk[chunkno].gap)
            HDmemset((oh->chunk[chunkno].image + oh->chunk[chunkno].size) -
                (H5O_SIZEOF_CHKSUM + oh->chunk[chunkno].gap), 0, oh->chunk[chunkno].gap);

        /* Compute metadata checksum */
        metadata_chksum = H5_checksum_metadata(oh->chunk[chunkno].image, (oh->chunk[chunkno].size - H5O_SIZEOF_CHKSUM), 0);

        /* Metadata checksum */
        p = oh->chunk[chunkno].image + (oh->chunk[chunkno].size - H5O_SIZEOF_CHKSUM);
        UINT32ENCODE(p, metadata_chksum);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_chunk_serialize() */


/*-------------------------------------------------------------------------
 * Function:	H5O_chunk_proxy_dest
 *
 * Purpose:	Destroy a chunk proxy object
 *
 * Return:	Success: SUCCEED
 *              Failure: FAIL
 *
 * Programmer:	Quincey Koziol
 *              koziol@hdfgroup.org
 *              July 13, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_chunk_proxy_dest(H5O_chunk_proxy_t *chk_proxy)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_chunk_proxy_dest)

    /* Check arguments */
    HDassert(chk_proxy);

    /* Decrement reference count of object header */
    if(H5O_dec_rc(chk_proxy->oh) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTDEC, FAIL, "can't decrement reference count on object header")

    /* Release the chunk proxy object */
    chk_proxy = H5FL_FREE(H5O_chunk_proxy_t, chk_proxy);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_chunk_proxy_dest() */

