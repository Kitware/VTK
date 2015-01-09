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
 * Created:             H5Fmpi.c
 *                      Jan 10 2008
 *                      Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:             MPI-related routines.
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
#include "H5Iprivate.h"		/* IDs			  		*/


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


#ifdef H5_HAVE_PARALLEL

/*-------------------------------------------------------------------------
 * Function:	H5F_mpi_get_rank
 *
 * Purpose:	Retrieves the rank of an MPI process.
 *
 * Return:	Success:	The rank (non-negative)
 *
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Friday, January 30, 2004
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5F_mpi_get_rank(const H5F_t *f)
{
    int	ret_value;

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(f && f->shared);

    /* Dispatch to driver */
    if ((ret_value=H5FD_mpi_get_rank(f->shared->lf)) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "driver get_rank request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_mpi_get_rank() */


/*-------------------------------------------------------------------------
 * Function:	H5F_mpi_get_comm
 *
 * Purpose:	Retrieves the file's communicator
 *
 * Return:	Success:	The communicator (non-negative)
 *
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Friday, January 30, 2004
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
MPI_Comm
H5F_mpi_get_comm(const H5F_t *f)
{
    MPI_Comm	ret_value;

    FUNC_ENTER_NOAPI(MPI_COMM_NULL)

    HDassert(f && f->shared);

    /* Dispatch to driver */
    if ((ret_value=H5FD_mpi_get_comm(f->shared->lf))==MPI_COMM_NULL)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, MPI_COMM_NULL, "driver get_comm request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_mpi_get_comm() */


/*-------------------------------------------------------------------------
 * Function:    H5F_mpi_get_size
 *
 * Purpose:     Retrieves the size of an MPI process.
 *
 * Return:      Success:        The size (positive)
 *
 *              Failure:        Negative
 *
 * Programmer:  John Mainzer
 *              Friday, May 6, 2005
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5F_mpi_get_size(const H5F_t *f)
{
    int ret_value;

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(f && f->shared);

    /* Dispatch to driver */
    if ((ret_value=H5FD_mpi_get_size(f->shared->lf)) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "driver get_size request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_mpi_get_size() */


/*-------------------------------------------------------------------------
 * Function:	H5Fset_mpi_atomicity
 *
 * Purpose:	Sets the atomicity mode
 *
 * Return:	Success:	Non-negative
 *
 * 		Failure:	Negative
 *
 * Programmer:	Mohamad Chaarawi
 *		Feb 14, 2012
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fset_mpi_atomicity(hid_t file_id, hbool_t flag)
{
    H5F_t       *file;
    herr_t       ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "ib", file_id, flag);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* Check VFD */
    if(!IS_H5FD_MPIO(file))
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "incorrect VFL driver, must use MPI-I/O driver")

    /* set atomicity value */
    if (H5FD_set_mpio_atomicity (file->shared->lf, flag) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't set atomicity flag")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5Fget_mpi_atomicity
 *
 * Purpose:	Returns the atomicity mode
 *
 * Return:	Success:	Non-negative
 *
 * 		Failure:	Negative
 *
 * Programmer:	Mohamad Chaarawi
 *		Feb 14, 2012
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_mpi_atomicity(hid_t file_id, hbool_t *flag)
{
    H5F_t      *file;
    herr_t     ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*b", file_id, flag);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* Check VFD */
    if(!IS_H5FD_MPIO(file))
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "incorrect VFL driver, must use MPI-I/O driver")

    /* get atomicity value */
    if (H5FD_get_mpio_atomicity (file->shared->lf, flag) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't get atomicity flag")

done:
    FUNC_LEAVE_API(ret_value)
}
#endif /* H5_HAVE_PARALLEL */

