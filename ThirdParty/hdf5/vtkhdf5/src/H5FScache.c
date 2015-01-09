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
 * Created:		H5FScache.c
 *			May  2 2006
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		Implement file free space metadata cache methods.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5FS_PACKAGE		/*suppress error about including H5FSpkg  */

/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5ACprivate.h"        /* Metadata cache                       */
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FSpkg.h"		/* File free space			*/
#include "H5MFprivate.h"	/* File memory management		*/
#include "H5VMprivate.h"		/* Vectors and arrays 			*/
#include "H5WBprivate.h"        /* Wrapped Buffers                      */

/****************/
/* Local Macros */
/****************/

/* File free space format version #'s */
#define H5FS_HDR_VERSION        0               /* Header */
#define H5FS_SINFO_VERSION      0               /* Serialized sections */

/* Size of stack buffer for serialized headers */
#define H5FS_HDR_BUF_SIZE               256


/******************/
/* Local Typedefs */
/******************/

/* User data for skip list iterator callback for iterating over section size nodes when syncing */
typedef struct {
    H5FS_sinfo_t *sinfo;        /* Free space section info */
    uint8_t **p;                /* Pointer to address of buffer pointer to serialize with */
    unsigned sect_cnt_size;     /* # of bytes to encode section size counts in */
} H5FS_iter_ud_t;


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

/* Section info routines */
static herr_t H5FS_sinfo_serialize_sect_cb(void *_item, void UNUSED *key, void *_udata);
static herr_t H5FS_sinfo_serialize_node_cb(void *_item, void UNUSED *key, void *_udata);

/* Metadata cache callbacks */
static H5FS_t *H5FS_cache_hdr_load(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *udata);
static herr_t H5FS_cache_hdr_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t addr, H5FS_t *fspace, unsigned UNUSED * flags_ptr);
static herr_t H5FS_cache_hdr_dest(H5F_t *f, H5FS_t *fspace);
static herr_t H5FS_cache_hdr_clear(H5F_t *f, H5FS_t *fspace, hbool_t destroy);
static herr_t H5FS_cache_hdr_size(const H5F_t *f, const H5FS_t *fspace, size_t *size_ptr);
static H5FS_sinfo_t *H5FS_cache_sinfo_load(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *udata);
static herr_t H5FS_cache_sinfo_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t addr, H5FS_sinfo_t *sinfo, unsigned UNUSED * flags_ptr);
static herr_t H5FS_cache_sinfo_dest(H5F_t *f, H5FS_sinfo_t *sinfo);
static herr_t H5FS_cache_sinfo_clear(H5F_t *f, H5FS_sinfo_t *sinfo, hbool_t destroy);
static herr_t H5FS_cache_sinfo_size(const H5F_t *f, const H5FS_sinfo_t *sinfo, size_t *size_ptr);


/*********************/
/* Package Variables */
/*********************/

/* H5FS header inherits cache-like properties from H5AC */
const H5AC_class_t H5AC_FSPACE_HDR[1] = {{
    H5AC_FSPACE_HDR_ID,
    (H5AC_load_func_t)H5FS_cache_hdr_load,
    (H5AC_flush_func_t)H5FS_cache_hdr_flush,
    (H5AC_dest_func_t)H5FS_cache_hdr_dest,
    (H5AC_clear_func_t)H5FS_cache_hdr_clear,
    (H5AC_size_func_t)H5FS_cache_hdr_size,
}};

/* H5FS serialized sections inherit cache-like properties from H5AC */
const H5AC_class_t H5AC_FSPACE_SINFO[1] = {{
    H5AC_FSPACE_SINFO_ID,
    (H5AC_load_func_t)H5FS_cache_sinfo_load,
    (H5AC_flush_func_t)H5FS_cache_sinfo_flush,
    (H5AC_dest_func_t)H5FS_cache_sinfo_dest,
    (H5AC_clear_func_t)H5FS_cache_sinfo_clear,
    (H5AC_size_func_t)H5FS_cache_sinfo_size,
}};


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/

/* Declare a free list to manage free space section data to/from disk */
H5FL_BLK_DEFINE_STATIC(sect_block);



/*-------------------------------------------------------------------------
 * Function:	H5FS_cache_hdr_load
 *
 * Purpose:	Loads a free space manager header from the disk.
 *
 * Return:      Success:        Pointer to a new free space header
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              koziol@ncsa.uiuc.edu
 *              May  2 2006
 *
 *-------------------------------------------------------------------------
 */
static H5FS_t *
H5FS_cache_hdr_load(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *_udata)
{
    H5FS_t		*fspace = NULL; /* Free space header info */
    H5FS_hdr_cache_ud_t *udata = (H5FS_hdr_cache_ud_t *)_udata; /* user data for callback */
    H5WB_t              *wb = NULL;     /* Wrapped buffer for header data */
    uint8_t             hdr_buf[H5FS_HDR_BUF_SIZE]; /* Buffer for header */
    uint8_t		*hdr;           /* Pointer to header buffer */
    const uint8_t	*p;             /* Pointer into raw data buffer */
    uint32_t            stored_chksum;  /* Stored metadata checksum value */
    uint32_t            computed_chksum; /* Computed metadata checksum value */
    unsigned            nclasses;       /* Number of section classes */
    H5FS_t		*ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check arguments */
    HDassert(f);
    HDassert(udata);

    /* Allocate a new free space manager */
    if(NULL == (fspace = H5FS_new(udata->f, udata->nclasses, udata->classes, udata->cls_init_udata)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Set free space manager's internal information */
    fspace->addr = udata->addr;

    /* Wrap the local buffer for serialized header info */
    if(NULL == (wb = H5WB_wrap(hdr_buf, sizeof(hdr_buf))))
        HGOTO_ERROR(H5E_FSPACE, H5E_CANTINIT, NULL, "can't wrap buffer")

    /* Get a pointer to a buffer that's large enough for header */
    if(NULL == (hdr = (uint8_t *)H5WB_actual(wb, fspace->hdr_size)))
        HGOTO_ERROR(H5E_FSPACE, H5E_NOSPACE, NULL, "can't get actual buffer")

    /* Read header from disk */
    if(H5F_block_read(f, H5FD_MEM_FSPACE_HDR, addr, fspace->hdr_size, dxpl_id, hdr) < 0)
	HGOTO_ERROR(H5E_FSPACE, H5E_READERROR, NULL, "can't read free space header")

    p = hdr;

    /* Magic number */
    if(HDmemcmp(p, H5FS_HDR_MAGIC, (size_t)H5_SIZEOF_MAGIC))
	HGOTO_ERROR(H5E_FSPACE, H5E_CANTLOAD, NULL, "wrong free space header signature")
    p += H5_SIZEOF_MAGIC;

    /* Version */
    if(*p++ != H5FS_HDR_VERSION)
	HGOTO_ERROR(H5E_FSPACE, H5E_CANTLOAD, NULL, "wrong free space header version")

    /* Client ID */
    fspace->client = (H5FS_client_t)*p++;
    if(fspace->client >= H5FS_NUM_CLIENT_ID)
	HGOTO_ERROR(H5E_FSPACE, H5E_CANTLOAD, NULL, "unknown client ID in free space header")

    /* Total space tracked */
    H5F_DECODE_LENGTH(udata->f, p, fspace->tot_space);

    /* Total # of free space sections tracked */
    H5F_DECODE_LENGTH(udata->f, p, fspace->tot_sect_count);

    /* # of serializable free space sections tracked */
    H5F_DECODE_LENGTH(udata->f, p, fspace->serial_sect_count);

    /* # of ghost free space sections tracked */
    H5F_DECODE_LENGTH(udata->f, p, fspace->ghost_sect_count);

    /* # of section classes */
    /* (only check if we actually have some classes) */
    UINT16DECODE(p, nclasses);
    if(fspace->nclasses > 0 && fspace->nclasses != nclasses)
	HGOTO_ERROR(H5E_FSPACE, H5E_CANTLOAD, NULL, "section class count mismatch")

    /* Shrink percent */
    UINT16DECODE(p, fspace->shrink_percent);

    /* Expand percent */
    UINT16DECODE(p, fspace->expand_percent);

    /* Size of address space free space sections are within (log2 of actual value) */
    UINT16DECODE(p, fspace->max_sect_addr);

    /* Max. size of section to track */
    H5F_DECODE_LENGTH(udata->f, p, fspace->max_sect_size);

    /* Address of serialized free space sections */
    H5F_addr_decode(udata->f, &p, &fspace->sect_addr);

    /* Size of serialized free space sections */
    H5F_DECODE_LENGTH(udata->f, p, fspace->sect_size);

    /* Allocated size of serialized free space sections */
    H5F_DECODE_LENGTH(udata->f, p, fspace->alloc_sect_size);

    /* Compute checksum on indirect block */
    computed_chksum = H5_checksum_metadata(hdr, (size_t)(p - (const uint8_t *)hdr), 0);

    /* Metadata checksum */
    UINT32DECODE(p, stored_chksum);

    HDassert((size_t)(p - (const uint8_t *)hdr) == fspace->hdr_size);

    /* Verify checksum */
    if(stored_chksum != computed_chksum)
	HGOTO_ERROR(H5E_FSPACE, H5E_BADVALUE, NULL, "incorrect metadata checksum for fractal heap indirect block")

    /* Set return value */
    ret_value = fspace;

done:
    /* Release resources */
    if(wb && H5WB_unwrap(wb) < 0)
        HDONE_ERROR(H5E_FSPACE, H5E_CLOSEERROR, NULL, "can't close wrapped buffer")
    if(!ret_value && fspace)
        if(H5FS_hdr_dest(fspace) < 0)
            HDONE_ERROR(H5E_FSPACE, H5E_CANTFREE, NULL, "unable to destroy free space header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FS_cache_hdr_load() */ /*lint !e818 Can't make udata a pointer to const */


/*-------------------------------------------------------------------------
 * Function:	H5FS_cache_hdr_flush
 *
 * Purpose:	Flushes a dirty free space header to disk.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              koziol@ncsa.uiuc.edu
 *              May  2 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FS_cache_hdr_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t addr, H5FS_t *fspace, unsigned UNUSED * flags_ptr)
{
    H5WB_t      *wb = NULL;             /* Wrapped buffer for header data */
    uint8_t     hdr_buf[H5FS_HDR_BUF_SIZE]; /* Buffer for header */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(fspace);
    HDassert(H5F_addr_defined(fspace->addr));

    /* Check if the header "owns" the section info */
    if(fspace->sinfo) {
        /* Sanity check - should not be trying to destroy header if it still
         *      "owns" section info
         */
        HDassert(!destroy);

        /* Check if the section info is dirty */
        if(fspace->sinfo->dirty) {
            if(fspace->serial_sect_count > 0) {
                /* Check if we need to allocate space for  section info */
                if(!H5F_addr_defined(fspace->sect_addr)) {
                    /* Sanity check */
                    HDassert(fspace->sect_size > 0);

                    /* Allocate space for the section info in file */
                    if(HADDR_UNDEF == (fspace->sect_addr = H5MF_alloc(f, H5FD_MEM_FSPACE_SINFO, dxpl_id, fspace->sect_size)))
                        HGOTO_ERROR(H5E_FSPACE, H5E_NOSPACE, FAIL, "file allocation failed for free space sections")
                    fspace->alloc_sect_size = (size_t)fspace->sect_size;

                    /* Mark header dirty */
                    /* (don't use cache API, since we're in a callback) */
                    fspace->cache_info.is_dirty = TRUE;
                } /* end if */

                /* Write section info to file */
                if(H5FS_cache_sinfo_flush(f, dxpl_id, FALSE, fspace->sect_addr, fspace->sinfo, NULL) < 0)
                    HGOTO_ERROR(H5E_FSPACE, H5E_CANTFLUSH, FAIL, "unable to save free space section info to disk")
            } /* end if */
            else {
                /* Sanity check that section info doesn't have address */
                HDassert(!H5F_addr_defined(fspace->sect_addr));
            } /* end else */
            /* Mark section info clean */
            fspace->sinfo->dirty = FALSE;
        } /* end if */
    } /* end if */
    else {
        /* Just sanity checks... */
        if(fspace->serial_sect_count > 0)
            /* Sanity check that section info has address */
            HDassert(H5F_addr_defined(fspace->sect_addr));
        else
            /* Sanity check that section info doesn't have address */
            HDassert(!H5F_addr_defined(fspace->sect_addr));
    } /* end else */

    if(fspace->cache_info.is_dirty) {
        uint8_t	*hdr;                   /* Pointer to header buffer */
        uint8_t *p;                     /* Pointer into raw data buffer */
        uint32_t metadata_chksum;       /* Computed metadata checksum value */

        /* Wrap the local buffer for serialized header info */
        if(NULL == (wb = H5WB_wrap(hdr_buf, sizeof(hdr_buf))))
            HGOTO_ERROR(H5E_FSPACE, H5E_CANTINIT, FAIL, "can't wrap buffer")

        /* Get a pointer to a buffer that's large enough for header */
        if(NULL == (hdr = (uint8_t *)H5WB_actual(wb, fspace->hdr_size)))
            HGOTO_ERROR(H5E_FSPACE, H5E_NOSPACE, FAIL, "can't get actual buffer")

        /* Get temporary pointer to header */
        p = hdr;

        /* Magic number */
        HDmemcpy(p, H5FS_HDR_MAGIC, (size_t)H5_SIZEOF_MAGIC);
        p += H5_SIZEOF_MAGIC;

        /* Version # */
        *p++ = H5FS_HDR_VERSION;

        /* Client ID */
        *p++ = fspace->client;

        /* Total space tracked */
        H5F_ENCODE_LENGTH(f, p, fspace->tot_space);

        /* Total # of free space sections tracked */
        H5F_ENCODE_LENGTH(f, p, fspace->tot_sect_count);

        /* # of serializable free space sections tracked */
        H5F_ENCODE_LENGTH(f, p, fspace->serial_sect_count);

        /* # of ghost free space sections tracked */
        H5F_ENCODE_LENGTH(f, p, fspace->ghost_sect_count);

        /* # of section classes */
        UINT16ENCODE(p, fspace->nclasses);

        /* Shrink percent */
        UINT16ENCODE(p, fspace->shrink_percent);

        /* Expand percent */
        UINT16ENCODE(p, fspace->expand_percent);

        /* Size of address space free space sections are within (log2 of actual value) */
        UINT16ENCODE(p, fspace->max_sect_addr);

        /* Max. size of section to track */
        H5F_ENCODE_LENGTH(f, p, fspace->max_sect_size);

        /* Address of serialized free space sections */
        H5F_addr_encode(f, &p, fspace->sect_addr);

        /* Size of serialized free space sections */
        H5F_ENCODE_LENGTH(f, p, fspace->sect_size);

        /* Allocated size of serialized free space sections */
        H5F_ENCODE_LENGTH(f, p, fspace->alloc_sect_size);

        /* Compute checksum */
        metadata_chksum = H5_checksum_metadata(hdr, (size_t)(p - (uint8_t *)hdr), 0);

        /* Metadata checksum */
        UINT32ENCODE(p, metadata_chksum);

	/* Write the free space header. */
        HDassert((size_t)(p - hdr) == fspace->hdr_size);
	if(H5F_block_write(f, H5FD_MEM_FSPACE_HDR, addr, fspace->hdr_size, dxpl_id, hdr) < 0)
	    HGOTO_ERROR(H5E_FSPACE, H5E_CANTFLUSH, FAIL, "unable to save free space header to disk")

	fspace->cache_info.is_dirty = FALSE;
    } /* end if */

    if(destroy)
        if(H5FS_cache_hdr_dest(f, fspace) < 0)
	    HGOTO_ERROR(H5E_FSPACE, H5E_CANTFREE, FAIL, "unable to destroy free space header")

done:
    /* Release resources */
    if(wb && H5WB_unwrap(wb) < 0)
        HDONE_ERROR(H5E_FSPACE, H5E_CLOSEERROR, FAIL, "can't close wrapped buffer")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5FS_cache_hdr_flush() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_cache_hdr_dest
 *
 * Purpose:	Destroys a free space header in memory.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May  2 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FS_cache_hdr_dest(H5F_t *f, H5FS_t *fspace)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check arguments */
    HDassert(fspace);

    /* We should not still be holding on to the free space section info */
    HDassert(!fspace->sinfo);

    /* If we're going to free the space on disk, the address must be valid */
    HDassert(!fspace->cache_info.free_file_space_on_destroy || H5F_addr_defined(fspace->cache_info.addr));

    /* Check for freeing file space for free space header */
    if(fspace->cache_info.free_file_space_on_destroy) {
        /* Sanity check */
        HDassert(H5F_addr_defined(fspace->addr));

        /* Release the space on disk */
        /* (XXX: Nasty usage of internal DXPL value! -QAK) */
        if(H5MF_xfree(f, H5FD_MEM_FSPACE_HDR, H5AC_dxpl_id, fspace->cache_info.addr, (hsize_t)fspace->hdr_size) < 0)
            HGOTO_ERROR(H5E_FSPACE, H5E_CANTFREE, FAIL, "unable to free free space header")
    } /* end if */

    /* Destroy free space header */
    if(H5FS_hdr_dest(fspace) < 0)
        HGOTO_ERROR(H5E_FSPACE, H5E_CANTFREE, FAIL, "unable to destroy free space header")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FS_cache_hdr_dest() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_cache_hdr_clear
 *
 * Purpose:	Mark a free space header in memory as non-dirty.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May  2 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FS_cache_hdr_clear(H5F_t *f, H5FS_t *fspace, hbool_t destroy)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /*
     * Check arguments.
     */
    HDassert(fspace);

    /* Reset the dirty flag.  */
    fspace->cache_info.is_dirty = FALSE;

    if(destroy)
        if(H5FS_cache_hdr_dest(f, fspace) < 0)
	    HGOTO_ERROR(H5E_FSPACE, H5E_CANTFREE, FAIL, "unable to destroy free space header")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FS_cache_hdr_clear() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_cache_hdr_size
 *
 * Purpose:	Compute the size in bytes of a free space header
 *		on disk, and return it in *size_ptr.  On failure,
 *		the value of *size_ptr is undefined.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May  2 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FS_cache_hdr_size(const H5F_t UNUSED *f, const H5FS_t *fspace, size_t *size_ptr)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check arguments */
    HDassert(f);
    HDassert(fspace);
    HDassert(size_ptr);

    /* Set size value */
    *size_ptr = fspace->hdr_size;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5FS_cache_hdr_size() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_cache_sinfo_load
 *
 * Purpose:	Loads free space sections from the disk.
 *
 * Return:      Success:        Pointer to a new free space section info
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              koziol@ncsa.uiuc.edu
 *              July 31 2006
 *
 *-------------------------------------------------------------------------
 */
static H5FS_sinfo_t *
H5FS_cache_sinfo_load(H5F_t *f, hid_t dxpl_id, haddr_t UNUSED addr, void *_udata)
{
    H5FS_sinfo_t	*sinfo = NULL;  /* Free space section info */
    H5FS_sinfo_cache_ud_t *udata = (H5FS_sinfo_cache_ud_t *)_udata; /* user data for callback */
    haddr_t             fs_addr;        /* Free space header address */
    size_t              old_sect_size;  /* Old section size */
    uint8_t		*buf = NULL;    /* Temporary buffer */
    const uint8_t	*p;             /* Pointer into raw data buffer */
    uint32_t            stored_chksum;  /* Stored metadata checksum value */
    uint32_t            computed_chksum; /* Computed metadata checksum value */
    H5FS_sinfo_t	*ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check arguments */
    HDassert(f);
    HDassert(udata);

    /* Allocate a new free space section info */
    if(NULL == (sinfo = H5FS_sinfo_new(udata->f, udata->fspace)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Allocate space for the buffer to serialize the sections into */
    H5_ASSIGN_OVERFLOW(/* To: */ old_sect_size, /* From: */ udata->fspace->sect_size, /* From: */ hsize_t, /* To: */ size_t);
    if(NULL == (buf = H5FL_BLK_MALLOC(sect_block, (size_t)udata->fspace->sect_size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Read buffer from disk */
    if(H5F_block_read(f, H5FD_MEM_FSPACE_SINFO, udata->fspace->sect_addr, (size_t)udata->fspace->sect_size, dxpl_id, buf) < 0)
	HGOTO_ERROR(H5E_FSPACE, H5E_READERROR, NULL, "can't read free space sections")

    /* Deserialize free sections from buffer available */
    p = buf;

    /* Magic number */
    if(HDmemcmp(p, H5FS_SINFO_MAGIC, (size_t)H5_SIZEOF_MAGIC))
	HGOTO_ERROR(H5E_FSPACE, H5E_CANTLOAD, NULL, "wrong free space sections signature")
    p += H5_SIZEOF_MAGIC;

    /* Version */
    if(*p++ != H5FS_SINFO_VERSION)
	HGOTO_ERROR(H5E_FSPACE, H5E_CANTLOAD, NULL, "wrong free space sections version")

    /* Address of free space header for these sections */
    H5F_addr_decode(udata->f, &p, &fs_addr);
    if(H5F_addr_ne(fs_addr, udata->fspace->addr))
	HGOTO_ERROR(H5E_FSPACE, H5E_CANTLOAD, NULL, "incorrect header address for free space sections")

    /* Check for any serialized sections */
    if(udata->fspace->serial_sect_count > 0) {
        hsize_t old_tot_sect_count;     /* Total section count from header */
        hsize_t old_serial_sect_count;  /* Total serializable section count from header */
        hsize_t old_ghost_sect_count;   /* Total ghost section count from header */
        hsize_t old_tot_space;          /* Total space managed from header */
        unsigned sect_cnt_size;         /* The size of the section size counts */

        /* Compute the size of the section counts */
        sect_cnt_size = H5VM_limit_enc_size((uint64_t)udata->fspace->serial_sect_count);

        /* Reset the section count, the "add" routine will update it */
        old_tot_sect_count = udata->fspace->tot_sect_count;
        old_serial_sect_count = udata->fspace->serial_sect_count;
        old_ghost_sect_count = udata->fspace->ghost_sect_count;
        old_tot_space = udata->fspace->tot_space;
        udata->fspace->tot_sect_count = 0;
        udata->fspace->serial_sect_count = 0;
        udata->fspace->ghost_sect_count = 0;
        udata->fspace->tot_space = 0;

        /* Walk through the buffer, deserializing sections */
        do {
            hsize_t sect_size;      /* Current section size */
            size_t node_count;      /* # of sections of this size */
            size_t u;               /* Local index variable */

            /* The number of sections of this node's size */
            UINT64DECODE_VAR(p, node_count, sect_cnt_size);
            HDassert(node_count);

            /* The size of the sections for this node */
            UINT64DECODE_VAR(p, sect_size, sinfo->sect_len_size);
            HDassert(sect_size);

            /* Loop over nodes of this size */
            for(u = 0; u < node_count; u++) {
                H5FS_section_info_t *new_sect;  /* Section that was deserialized */
                haddr_t sect_addr;      /* Address of free space section in the address space */
                unsigned sect_type;     /* Type of free space section */
                unsigned des_flags;     /* Flags from deserialize callback */

                /* The address of the section */
                UINT64DECODE_VAR(p, sect_addr, sinfo->sect_off_size);

                /* The type of this section */
                sect_type = *p++;

                /* Call 'deserialize' callback for this section */
                des_flags = 0;
                HDassert(udata->fspace->sect_cls[sect_type].deserialize);
                if(NULL == (new_sect = (*udata->fspace->sect_cls[sect_type].deserialize)(&udata->fspace->sect_cls[sect_type], udata->dxpl_id, p, sect_addr, sect_size, &des_flags)))
                    HGOTO_ERROR(H5E_FSPACE, H5E_CANTDECODE, NULL, "can't deserialize section")

                /* Update offset in serialization buffer */
                p += udata->fspace->sect_cls[sect_type].serial_size;

                /* Insert section in free space manager, unless requested not to */
                if(!(des_flags & H5FS_DESERIALIZE_NO_ADD))
                    if(H5FS_sect_add(udata->f, udata->dxpl_id, udata->fspace, new_sect, H5FS_ADD_DESERIALIZING, NULL) < 0)
                        HGOTO_ERROR(H5E_FSPACE, H5E_CANTINSERT, NULL, "can't add section to free space manager")
            } /* end for */
        } while(p < ((buf + old_sect_size) - H5FS_SIZEOF_CHKSUM));

        /* Sanity check */
        HDassert((size_t)(p - buf) == (old_sect_size - H5FS_SIZEOF_CHKSUM));
        HDassert(old_sect_size == udata->fspace->sect_size);
        HDassert(old_tot_sect_count == udata->fspace->tot_sect_count);
        HDassert(old_serial_sect_count == udata->fspace->serial_sect_count);
        HDassert(old_ghost_sect_count == udata->fspace->ghost_sect_count);
        HDassert(old_tot_space == udata->fspace->tot_space);
    } /* end if */

    /* Compute checksum on indirect block */
    computed_chksum = H5_checksum_metadata(buf, (size_t)(p - (const uint8_t *)buf), 0);

    /* Metadata checksum */
    UINT32DECODE(p, stored_chksum);

    /* Verify checksum */
    if(stored_chksum != computed_chksum)
	HGOTO_ERROR(H5E_FSPACE, H5E_BADVALUE, NULL, "incorrect metadata checksum for fractal heap indirect block")

    /* Sanity check */
    HDassert((size_t)(p - (const uint8_t *)buf) == old_sect_size);

    /* Set return value */
    ret_value = sinfo;

done:
    if(buf)
        buf = H5FL_BLK_FREE(sect_block, buf);
    if(!ret_value && sinfo)
        if(H5FS_sinfo_dest(sinfo) < 0)
            HDONE_ERROR(H5E_FSPACE, H5E_CANTFREE, NULL, "unable to destroy free space info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FS_cache_sinfo_load() */ /*lint !e818 Can't make udata a pointer to const */


/*-------------------------------------------------------------------------
 * Function:	H5FS_sinfo_serialize_sect_cb
 *
 * Purpose:	Skip list iterator callback to serialize free space sections
 *              of a particular size
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, May  8, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FS_sinfo_serialize_sect_cb(void *_item, void UNUSED *key, void *_udata)
{
    H5FS_section_class_t *sect_cls;     /* Class of section */
    H5FS_section_info_t *sect= (H5FS_section_info_t *)_item;   /* Free space section to work on */
    H5FS_iter_ud_t *udata = (H5FS_iter_ud_t *)_udata; /* Callback info */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check arguments. */
    HDassert(sect);
    HDassert(udata->sinfo);
    HDassert(udata->p);

    /* Get section's class */
    sect_cls = &udata->sinfo->fspace->sect_cls[sect->type];

    /* Check if this section should be serialized (i.e. is not a ghost section) */
    if(!(sect_cls->flags & H5FS_CLS_GHOST_OBJ)) {
        /* The address of the section */
        UINT64ENCODE_VAR(*udata->p, sect->addr, udata->sinfo->sect_off_size);

        /* The type of this section */
        *(*udata->p)++ = (uint8_t)sect->type;

        /* Call 'serialize' callback for this section */
        if(sect_cls->serialize) {
            if((*sect_cls->serialize)(sect_cls, sect, *udata->p) < 0)
                HGOTO_ERROR(H5E_FSPACE, H5E_CANTSERIALIZE, FAIL, "can't syncronize section")

            /* Update offset in serialization buffer */
            (*udata->p) += sect_cls->serial_size;
        } /* end if */
        else
            HDassert(sect_cls->serial_size == 0);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5FS_sinfo_serialize_sect_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_sinfo_serialize_node_cb
 *
 * Purpose:	Skip list iterator callback to serialize free space sections
 *              in a bin
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, May  8, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FS_sinfo_serialize_node_cb(void *_item, void UNUSED *key, void *_udata)
{
    H5FS_node_t *fspace_node = (H5FS_node_t *)_item;   /* Free space size node to work on */
    H5FS_iter_ud_t *udata = (H5FS_iter_ud_t *)_udata; /* Callback info */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check arguments. */
    HDassert(fspace_node);
    HDassert(udata->sinfo);
    HDassert(udata->p);

    /* Check if this node has any serializable sections */
    if(fspace_node->serial_count > 0) {
        /* The number of serializable sections of this node's size */
        UINT64ENCODE_VAR(*udata->p, fspace_node->serial_count, udata->sect_cnt_size);

        /* The size of the sections for this node */
        UINT64ENCODE_VAR(*udata->p, fspace_node->sect_size, udata->sinfo->sect_len_size);

        /* Iterate through all the sections of this size */
        HDassert(fspace_node->sect_list);
        if(H5SL_iterate(fspace_node->sect_list, H5FS_sinfo_serialize_sect_cb, udata) < 0)
            HGOTO_ERROR(H5E_FSPACE, H5E_BADITER, FAIL, "can't iterate over section nodes")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5FS_sinfo_serialize_node_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_cache_sinfo_flush
 *
 * Purpose:	Flushes a dirty free space section info to disk.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              koziol@ncsa.uiuc.edu
 *              July 31 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FS_cache_sinfo_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t addr, H5FS_sinfo_t *sinfo, unsigned UNUSED * flags_ptr)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(sinfo);
    HDassert(sinfo->fspace);
    HDassert(sinfo->fspace->sect_cls);

    if(sinfo->cache_info.is_dirty || sinfo->dirty) {
        H5FS_iter_ud_t udata;       /* User data for callbacks */
        uint8_t	*buf = NULL;        /* Temporary raw data buffer */
        uint8_t *p;                 /* Pointer into raw data buffer */
        uint32_t metadata_chksum;   /* Computed metadata checksum value */
        unsigned bin;               /* Current bin we are on */

        /* Sanity check address */
        if(H5F_addr_ne(addr, sinfo->fspace->sect_addr))
            HGOTO_ERROR(H5E_FSPACE, H5E_CANTLOAD, FAIL, "incorrect address for free space sections")

        /* Allocate temporary buffer */
        if((buf = H5FL_BLK_MALLOC(sect_block, (size_t)sinfo->fspace->sect_size)) == NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

        p = buf;

        /* Magic number */
        HDmemcpy(p, H5FS_SINFO_MAGIC, (size_t)H5_SIZEOF_MAGIC);
        p += H5_SIZEOF_MAGIC;

        /* Version # */
        *p++ = H5FS_SINFO_VERSION;

        /* Address of free space header for these sections */
        H5F_addr_encode(f, &p, sinfo->fspace->addr);

        /* Set up user data for iterator */
        udata.sinfo = sinfo;
        udata.p = &p;
        udata.sect_cnt_size = H5VM_limit_enc_size((uint64_t)sinfo->fspace->serial_sect_count);

        /* Iterate over all the bins */
        for(bin = 0; bin < sinfo->nbins; bin++) {
            /* Check if there are any sections in this bin */
            if(sinfo->bins[bin].bin_list) {
                /* Iterate over list of section size nodes for bin */
                if(H5SL_iterate(sinfo->bins[bin].bin_list, H5FS_sinfo_serialize_node_cb, &udata) < 0)
                    HGOTO_ERROR(H5E_FSPACE, H5E_BADITER, FAIL, "can't iterate over section size nodes")
            } /* end if */
        } /* end for */

        /* Compute checksum */
        metadata_chksum = H5_checksum_metadata(buf, (size_t)(p - buf), 0);

        /* Metadata checksum */
        UINT32ENCODE(p, metadata_chksum);

        /* Sanity check */
        HDassert((size_t)(p - buf) == sinfo->fspace->sect_size);
        HDassert(sinfo->fspace->sect_size <= sinfo->fspace->alloc_sect_size);

        /* Write buffer to disk */
        if(H5F_block_write(f, H5FD_MEM_FSPACE_SINFO, sinfo->fspace->sect_addr, (size_t)sinfo->fspace->sect_size, dxpl_id, buf) < 0)
            HGOTO_ERROR(H5E_FSPACE, H5E_CANTFLUSH, FAIL, "unable to save free space sections to disk")

        buf = H5FL_BLK_FREE(sect_block, buf);

	sinfo->cache_info.is_dirty = FALSE;
        sinfo->dirty = FALSE;
    } /* end if */

    if(destroy)
        if(H5FS_cache_sinfo_dest(f, sinfo) < 0)
	    HGOTO_ERROR(H5E_FSPACE, H5E_CANTFREE, FAIL, "unable to destroy free space section info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5FS_cache_sinfo_flush() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_cache_sinfo_dest
 *
 * Purpose:	Destroys a free space section info in memory.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		July 31 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FS_cache_sinfo_dest(H5F_t *f, H5FS_sinfo_t *sinfo)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check arguments */
    HDassert(sinfo);

    /* If we're going to free the space on disk, the address must be valid */
    HDassert(!sinfo->cache_info.free_file_space_on_destroy || H5F_addr_defined(sinfo->cache_info.addr));

    /* Check for freeing file space for free space section info */
    if(sinfo->cache_info.free_file_space_on_destroy) {
        /* Sanity check */
        HDassert(sinfo->fspace->alloc_sect_size > 0);

        /* Release the space on disk */
        /* (XXX: Nasty usage of internal DXPL value! -QAK) */
        if(H5MF_xfree(f, H5FD_MEM_FSPACE_SINFO, H5AC_dxpl_id, sinfo->cache_info.addr, (hsize_t)sinfo->fspace->alloc_sect_size) < 0)
            HGOTO_ERROR(H5E_FSPACE, H5E_CANTFREE, FAIL, "unable to free free space section info")
    } /* end if */

    /* Destroy free space info */
    if(H5FS_sinfo_dest(sinfo) < 0)
        HGOTO_ERROR(H5E_FSPACE, H5E_CANTFREE, FAIL, "unable to destroy free space info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FS_cache_sinfo_dest() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_cache_sinfo_clear
 *
 * Purpose:	Mark a free space section info in memory as non-dirty.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		July 31 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FS_cache_sinfo_clear(H5F_t *f, H5FS_sinfo_t *sinfo, hbool_t destroy)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /*
     * Check arguments.
     */
    HDassert(sinfo);

    /* Reset the dirty flag.  */
    sinfo->cache_info.is_dirty = FALSE;

    if(destroy)
        if(H5FS_cache_sinfo_dest(f, sinfo) < 0)
	    HGOTO_ERROR(H5E_FSPACE, H5E_CANTFREE, FAIL, "unable to destroy free space section info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FS_cache_sinfo_clear() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_cache_sinfo_size
 *
 * Purpose:	Compute the size in bytes of a free space section info
 *		on disk, and return it in *size_ptr.  On failure,
 *		the value of *size_ptr is undefined.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		July 31 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FS_cache_sinfo_size(const H5F_t UNUSED *f, const H5FS_sinfo_t *sinfo, size_t *size_ptr)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check arguments */
    HDassert(sinfo);
    HDassert(size_ptr);

    /* Set size value */
    H5_ASSIGN_OVERFLOW(/* To: */ *size_ptr, /* From: */ sinfo->fspace->alloc_sect_size, /* From: */ hsize_t, /* To: */ size_t);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5FS_cache_sinfo_size() */

