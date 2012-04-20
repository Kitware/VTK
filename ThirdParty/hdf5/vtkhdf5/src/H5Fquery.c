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
 * Created:             H5Fquery.c
 *                      Jan 10 2008
 *                      Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:             File structure query routines.
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
 * Function:	H5F_get_intent
 *
 * Purpose:	Quick and dirty routine to retrieve the file's 'intent' flags
 *          (Mainly added to stop non-file routines from poking about in the
 *          H5F_t data structure)
 *
 * Return:	'intent' on success/abort on failure (shouldn't fail)
 *
 * Programmer:	Quincey Koziol <koziol@ncsa.uiuc.edu>
 *		September 29, 2000
 *
 *-------------------------------------------------------------------------
 */
unsigned
H5F_get_intent(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_get_intent)

    HDassert(f);

    FUNC_LEAVE_NOAPI(f->intent)
} /* end H5F_get_intent() */


/*-------------------------------------------------------------------------
 * Function:	H5F_get_open_name
 *
 * Purpose:	Retrieve the name used to open a file.
 *
 * Return:	Success:	The name of the file.
 * 		Failure:	? (should not happen)
 *
 * Programmer:	Neil Fortner
 *		December 15 2008
 *
 *-------------------------------------------------------------------------
 */
char *
H5F_get_open_name(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_get_open_name)

    HDassert(f);
    HDassert(f->open_name);

    FUNC_LEAVE_NOAPI(f->open_name)
} /* end H5F_get_open_name() */


/*-------------------------------------------------------------------------
 * Function:	H5F_get_actual_name
 *
 * Purpose:	Retrieve the actual name of a file, after resolving symlinks, etc.
 *
 * Return:	Success:	The name of the file.
 * 		Failure:	? (should not happen)
 *
 * Programmer:	Quincey Koziol
 *		November 25 2009
 *
 *-------------------------------------------------------------------------
 */
char *
H5F_get_actual_name(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_get_actual_name)

    HDassert(f);
    HDassert(f->actual_name);

    FUNC_LEAVE_NOAPI(f->actual_name)
} /* end H5F_get_actual_name() */


/*-------------------------------------------------------------------------
 * Function:	H5F_get_extpath
 *
 * Purpose:	Retrieve the file's 'extpath' flags
 *		This is used by H5L_extern_traverse() to retrieve the main file's location
 *		when searching the target file.
 *
 * Return:	'extpath' on success/abort on failure (shouldn't fail)
 *
 * Programmer:	Vailin Choi, April 2, 2008
 *
 *-------------------------------------------------------------------------
 */
char *
H5F_get_extpath(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_get_extpath)

    HDassert(f);
    HDassert(f->extpath);

    FUNC_LEAVE_NOAPI(f->extpath)
} /* end H5F_get_extpath() */


/*-------------------------------------------------------------------------
 * Function:	H5F_get_fcpl
 *
 * Purpose:	Retrieve the value of a file's FCPL.
 *
 * Return:	Success:	The FCPL for the file.
 *
 * 		Failure:	? (should not happen)
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May 25 2005
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5F_get_fcpl(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_get_fcpl)

    HDassert(f);
    HDassert(f->shared);

    FUNC_LEAVE_NOAPI(f->shared->fcpl_id)
} /* end H5F_get_fcpl() */


/*-------------------------------------------------------------------------
 * Function:	H5F_sizeof_addr
 *
 * Purpose:	Quick and dirty routine to retrieve the size of the file's size_t
 *          (Mainly added to stop non-file routines from poking about in the
 *          H5F_t data structure)
 *
 * Return:	'sizeof_addr' on success/abort on failure (shouldn't fail)
 *
 * Programmer:	Quincey Koziol <koziol@ncsa.uiuc.edu>
 *		September 29, 2000
 *
 *-------------------------------------------------------------------------
 */
uint8_t
H5F_sizeof_addr(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_sizeof_addr)

    HDassert(f);
    HDassert(f->shared);

    FUNC_LEAVE_NOAPI(f->shared->sizeof_addr)
} /* end H5F_sizeof_addr() */


/*-------------------------------------------------------------------------
 * Function:	H5F_sizeof_size
 *
 * Purpose:	Quick and dirty routine to retrieve the size of the file's off_t
 *          (Mainly added to stop non-file routines from poking about in the
 *          H5F_t data structure)
 *
 * Return:	'sizeof_size' on success/abort on failure (shouldn't fail)
 *
 * Programmer:	Quincey Koziol <koziol@ncsa.uiuc.edu>
 *		September 29, 2000
 *
 *-------------------------------------------------------------------------
 */
uint8_t
H5F_sizeof_size(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_sizeof_size)

    HDassert(f);
    HDassert(f->shared);

    FUNC_LEAVE_NOAPI(f->shared->sizeof_size)
} /* H5F_sizeof_size() */


/*-------------------------------------------------------------------------
 * Function:	H5F_sym_leaf_k
 *
 * Purpose:	Replaced a macro to retrieve the symbol table leaf size,
 *              now that the generic properties are being used to store
 *              the values.
 *
 * Return:	Success:	Non-negative, and the symbol table leaf size is
 *                              returned.
 *
 * 		Failure:	Negative (should not happen)
 *
 * Programmer:	Raymond Lu
 *		slu@ncsa.uiuc.edu
 *		Oct 14 2001
 *
 *-------------------------------------------------------------------------
 */
unsigned
H5F_sym_leaf_k(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_sym_leaf_k)

    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->sblock);

    FUNC_LEAVE_NOAPI(f->shared->sblock->sym_leaf_k)
} /* end H5F_sym_leaf_k() */


/*-------------------------------------------------------------------------
 * Function:	H5F_Kvalue
 *
 * Purpose:	Replaced a macro to retrieve a B-tree key value for a certain
 *              type, now that the generic properties are being used to store
 *              the B-tree values.
 *
 * Return:	Success:	Non-negative, and the B-tree key value is
 *                              returned.
 *
 * 		Failure:	Negative (should not happen)
 *
 * Programmer:	Raymond Lu
 *		slu@ncsa.uiuc.edu
 *		Oct 14 2001
 *
 *-------------------------------------------------------------------------
 */
unsigned
H5F_Kvalue(const H5F_t *f, const H5B_class_t *type)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_Kvalue)

    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->sblock);
    HDassert(type);

    FUNC_LEAVE_NOAPI(f->shared->sblock->btree_k[type->id])
} /* end H5F_Kvalue() */


/*-------------------------------------------------------------------------
 * Function:	H5F_rdcc_nslots
 *
 * Purpose:	Replaced a macro to retrieve the raw data cache number of slots,
 *              now that the generic properties are being used to store
 *              the values.
 *
 * Return:	Success:	Non-negative, and the raw data cache number of
 *                              of slots is returned.
 *
 * 		Failure:	Negative (should not happen)
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Jun  1 2004
 *
 *-------------------------------------------------------------------------
 */
size_t
H5F_rdcc_nslots(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_rdcc_nslots)

    HDassert(f);
    HDassert(f->shared);

    FUNC_LEAVE_NOAPI(f->shared->rdcc_nslots)
} /* end H5F_rdcc_nelmts() */


/*-------------------------------------------------------------------------
 * Function:	H5F_rdcc_nbytes
 *
 * Purpose:	Replaced a macro to retrieve the raw data cache number of bytes,
 *              now that the generic properties are being used to store
 *              the values.
 *
 * Return:	Success:	Non-negative, and the raw data cache number of
 *                              of bytes is returned.
 *
 * 		Failure:	Negative (should not happen)
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Jun  1 2004
 *
 *-------------------------------------------------------------------------
 */
size_t
H5F_rdcc_nbytes(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_rdcc_nbytes)

    HDassert(f);
    HDassert(f->shared);

    FUNC_LEAVE_NOAPI(f->shared->rdcc_nbytes)
} /* end H5F_rdcc_nbytes() */


/*-------------------------------------------------------------------------
 * Function:	H5F_rdcc_w0
 *
 * Purpose:	Replaced a macro to retrieve the raw data cache 'w0' value
 *              now that the generic properties are being used to store
 *              the values.
 *
 * Return:	Success:	Non-negative, and the raw data cache 'w0' value
 *                              is returned.
 *
 * 		Failure:	Negative (should not happen)
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Jun  2 2004
 *
 *-------------------------------------------------------------------------
 */
double
H5F_rdcc_w0(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_rdcc_w0)

    HDassert(f);
    HDassert(f->shared);

    FUNC_LEAVE_NOAPI(f->shared->rdcc_w0)
} /* end H5F_rdcc_w0() */


/*-------------------------------------------------------------------------
 * Function:	H5F_get_base_addr
 *
 * Purpose:	Quick and dirty routine to retrieve the file's 'base_addr' value
 *          (Mainly added to stop non-file routines from poking about in the
 *          H5F_t data structure)
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu <slu@ncsa.uiuc.edu>
 *		December 20, 2002
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5F_get_base_addr(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_get_base_addr)

    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->sblock);

    FUNC_LEAVE_NOAPI(f->shared->sblock->base_addr)
} /* end H5F_get_base_addr() */


/*-------------------------------------------------------------------------
 * Function:	H5F_grp_btree_shared
 *
 * Purpose:	Replaced a macro to retrieve the shared B-tree node info
 *              now that the generic properties are being used to store
 *              the values.
 *
 * Return:	Success:	Non-void, and the shared B-tree node info
 *                              is returned.
 *
 * 		Failure:	void (should not happen)
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Jul  5 2004
 *
 *-------------------------------------------------------------------------
 */
H5RC_t *
H5F_grp_btree_shared(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_grp_btree_shared)

    HDassert(f);
    HDassert(f->shared);

    FUNC_LEAVE_NOAPI(f->shared->grp_btree_shared)
} /* end H5F_grp_btree_shared() */


/*-------------------------------------------------------------------------
 * Function:	H5F_sieve_buf_size
 *
 * Purpose:	Replaced a macro to retrieve the dataset sieve buffer size
 *              now that the generic properties are being used to store
 *              the values.
 *
 * Return:	Success:	Non-void, and the dataset sieve buffer size
 *                              is returned.
 *
 * 		Failure:	void (should not happen)
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Jul  8 2005
 *
 *-------------------------------------------------------------------------
 */
size_t
H5F_sieve_buf_size(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_sieve_buf_size)

    HDassert(f);
    HDassert(f->shared);

    FUNC_LEAVE_NOAPI(f->shared->sieve_buf_size)
} /* end H5F_sieve_buf_size() */


/*-------------------------------------------------------------------------
 * Function:	H5F_gc_ref
 *
 * Purpose:	Replaced a macro to retrieve the "garbage collect
 *              references flag" now that the generic properties are being used
 *              to store the values.
 *
 * Return:	Success:	The "garbage collect references flag"
 *                              is returned.
 *
 * 		Failure:	(should not happen)
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Jul  8 2005
 *
 *-------------------------------------------------------------------------
 */
unsigned
H5F_gc_ref(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_gc_ref)

    HDassert(f);
    HDassert(f->shared);

    FUNC_LEAVE_NOAPI(f->shared->gc_ref)
} /* end H5F_gc_ref() */


/*-------------------------------------------------------------------------
 * Function:	H5F_use_latest_format
 *
 * Purpose:	Retrieve the 'use the latest version of the format' flag for
 *              the file.
 *
 * Return:	Success:	Non-negative, the 'use the latest format' flag
 *
 * 		Failure:	(can't happen)
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Oct  2 2006
 *
 *-------------------------------------------------------------------------
 */
hbool_t
H5F_use_latest_format(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_use_latest_format)

    HDassert(f);
    HDassert(f->shared);

    FUNC_LEAVE_NOAPI(f->shared->latest_format)
} /* end H5F_use_latest_format() */


/*-------------------------------------------------------------------------
 * Function:	H5F_get_fc_degree
 *
 * Purpose:	Retrieve the 'file close degree' for the file.
 *
 * Return:	Success:	Non-negative, the 'file close degree'
 *
 * 		Failure:	(can't happen)
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Mar  5 2007
 *
 *-------------------------------------------------------------------------
 */
H5F_close_degree_t
H5F_get_fc_degree(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_get_fc_degree)

    HDassert(f);
    HDassert(f->shared);

    FUNC_LEAVE_NOAPI(f->shared->fc_degree)
} /* end H5F_get_fc_degree() */


/*-------------------------------------------------------------------------
 * Function:	H5F_store_msg_crt_idx
 *
 * Purpose:	Retrieve the 'store message creation index' flag for the file.
 *
 * Return:	Success:	Non-negative, the 'store message creation index' flag
 *
 * 		Failure:	(can't happen)
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Mar  6 2007
 *
 *-------------------------------------------------------------------------
 */
hbool_t
H5F_store_msg_crt_idx(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_store_msg_crt_idx)

    HDassert(f);
    HDassert(f->shared);

    FUNC_LEAVE_NOAPI(f->shared->store_msg_crt_idx)
} /* end H5F_store_msg_crt_idx() */


/*-------------------------------------------------------------------------
 * Function:	H5F_has_feature
 *
 * Purpose:	Check if a file has a particular feature enabled
 *
 * Return:	Success:	Non-negative - TRUE or FALSE
 * 		Failure:	Negative (should not happen)
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May 31 2004
 *
 *-------------------------------------------------------------------------
 */
hbool_t
H5F_has_feature(const H5F_t *f, unsigned feature)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_has_feature)

    HDassert(f);
    HDassert(f->shared);

    FUNC_LEAVE_NOAPI((hbool_t)(f->shared->lf->feature_flags&feature))
} /* end H5F_has_feature() */


/*-------------------------------------------------------------------------
 * Function:	H5F_get_driver_id
 *
 * Purpose:	Quick and dirty routine to retrieve the file's 'driver_id' value
 *          (Mainly added to stop non-file routines from poking about in the
 *          H5F_t data structure)
 *
 * Return:	'driver_id' on success/abort on failure (shouldn't fail)
 *
 * Programmer:	Quincey Koziol <koziol@ncsa.uiuc.edu>
 *		October 10, 2000
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5F_get_driver_id(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_get_driver_id)

    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->lf);

    FUNC_LEAVE_NOAPI(f->shared->lf->driver_id)
} /* end H5F_get_driver_id() */


/*-------------------------------------------------------------------------
 * Function:	H5F_get_fileno
 *
 * Purpose:	Quick and dirty routine to retrieve the file's 'fileno' value
 *          (Mainly added to stop non-file routines from poking about in the
 *          H5F_t data structure)
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol <koziol@ncsa.uiuc.edu>
 *		March 27, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_get_fileno(const H5F_t *f, unsigned long *filenum)
{
    herr_t	ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5F_get_fileno, FAIL)

    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->lf);
    HDassert(filenum);

    /* Retrieve the file's serial number */
    if(H5FD_get_fileno(f->shared->lf, filenum) < 0)
	HGOTO_ERROR(H5E_FILE, H5E_BADRANGE, FAIL, "can't retrieve fileno")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_get_fileno() */


/*-------------------------------------------------------------------------
 * Function:	H5F_get_eoa
 *
 * Purpose:	Quick and dirty routine to retrieve the file's 'eoa' value
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol <koziol@ncsa.uiuc.edu>
 *		June 1, 2004
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5F_get_eoa(const H5F_t *f, H5FD_mem_t type)
{
    haddr_t	ret_value;

    FUNC_ENTER_NOAPI(H5F_get_eoa, HADDR_UNDEF)

    HDassert(f);
    HDassert(f->shared);

    /* Dispatch to driver */
    if(HADDR_UNDEF == (ret_value = H5FD_get_eoa(f->shared->lf, type)))
	HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, HADDR_UNDEF, "driver get_eoa request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_get_eoa() */


/*-------------------------------------------------------------------------
 * Function:    H5F_get_vfd_handle
 *
 * Purpose:     Returns a pointer to the file handle of the low-level file
 *              driver.  This is the private function for H5Fget_vfd_handle.
 *
 * Return:      Success:        Non-negative.
 *              Failure:        negative.
 *
 * Programmer:  Raymond Lu
 *              Sep. 16, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_get_vfd_handle(const H5F_t *file, hid_t fapl, void **file_handle)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5F_get_vfd_handle, FAIL)

    /* Sanity check */
    HDassert(file);
    HDassert(file_handle);

    /* Get the VFD handle */
    if(H5FD_get_vfd_handle(file->shared->lf, fapl, file_handle) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't get file handle for file driver")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_get_vfd_handle() */


/*-------------------------------------------------------------------------
 * Function:	H5F_is_tmp_addr
 *
 * Purpose:	Quick and dirty routine to determine if an address is in
 *		the 'temporary' file space.
 *          (Mainly added to stop non-file routines from poking about in the
 *          H5F_t data structure)
 *
 * Return:	TRUE/FALSE on success/abort on failure (shouldn't fail)
 *
 * Programmer:	Quincey Koziol <koziol@hdfgroup.org>
 *		June 11, 2009
 *
 *-------------------------------------------------------------------------
 */
hbool_t
H5F_is_tmp_addr(const H5F_t *f, haddr_t addr)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_is_tmp_addr)

    HDassert(f);
    HDassert(f->shared);

    FUNC_LEAVE_NOAPI(H5F_addr_le(f->shared->tmp_addr, addr))
} /* end H5F_is_tmp_addr() */


/*-------------------------------------------------------------------------
 * Function:	H5F_use_tmp_space
 *
 * Purpose:	Quick and dirty routine to determine if using temporary
 *		file space is allowed for this file.
 *          (Mainly added to stop non-file routines from poking about in the
 *          H5F_t data structure)
 *
 * Return:	TRUE/FALSE on success/abort on failure (shouldn't fail)
 *
 * Programmer:	Quincey Koziol <koziol@hdfgroup.org>
 *		July  1, 2009
 *
 *-------------------------------------------------------------------------
 */
hbool_t
H5F_use_tmp_space(const H5F_t *f)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_use_tmp_space)

    HDassert(f);
    HDassert(f->shared);

    FUNC_LEAVE_NOAPI(f->shared->use_tmp_space)
} /* end H5F_use_tmp_space() */

