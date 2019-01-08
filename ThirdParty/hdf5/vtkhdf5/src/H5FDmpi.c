/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              Friday, January 30, 2004
 *
 * Purpose:	Common routines for all MPI-based VFL drivers.
 *
 */


#include "H5private.h"		/* Generic Functions			*/
#include "H5CXprivate.h"        /* API Contexts                         */
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"		/* File access				*/
#include "H5FDprivate.h"	/* File drivers				*/
#include "H5FDmpi.h"            /* Common MPI file driver		*/
#include "H5Pprivate.h"		/* Property lists			*/

#ifdef H5_HAVE_PARALLEL


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpi_get_rank
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
H5FD_mpi_get_rank(const H5FD_t *file)
{
    const H5FD_class_mpi_t *cls;

    int	ret_value;

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(file);
    cls = (const H5FD_class_mpi_t *)(file->cls);
    HDassert(cls);
    HDassert(cls->get_rank);        /* All MPI drivers must implement this */

    /* Dispatch to driver */
    if ((ret_value=(cls->get_rank)(file))<0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "driver get_rank request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_mpi_get_rank() */


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpi_get_size
 *
 * Purpose:	Retrieves the size of the communicator used for the file
 *
 * Return:	Success:	The communicator size (non-negative)
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
H5FD_mpi_get_size(const H5FD_t *file)
{
    const H5FD_class_mpi_t *cls;
    int	ret_value;

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(file);
    cls = (const H5FD_class_mpi_t *)(file->cls);
    HDassert(cls);
    HDassert(cls->get_size);        /* All MPI drivers must implement this */

    /* Dispatch to driver */
    if ((ret_value=(cls->get_size)(file))<0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "driver get_size request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_mpi_get_size() */


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpi_get_comm
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
H5FD_mpi_get_comm(const H5FD_t *file)
{
    const H5FD_class_mpi_t *cls;
    MPI_Comm	ret_value;

    FUNC_ENTER_NOAPI(MPI_COMM_NULL)

    HDassert(file);
    cls = (const H5FD_class_mpi_t *)(file->cls);
    HDassert(cls);
    HDassert(cls->get_comm);        /* All MPI drivers must implement this */

    /* Dispatch to driver */
    if ((ret_value=(cls->get_comm)(file))==MPI_COMM_NULL)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, MPI_COMM_NULL, "driver get_comm request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_mpi_get_comm() */


/*-------------------------------------------------------------------------
 * Function:	H5FD_get_mpi_info
 *
 * Purpose:	Retrieves the file's mpi info
 *
 * Return:	Success:	SUCCEED
 *
 *		Failure:	Negative
 *
 * Programmer:	John Mainzer
 *              4/4/17
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_get_mpi_info(H5FD_t *file, void** mpi_info)
{
    const H5FD_class_mpi_t *cls;
    herr_t  ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(file);
    cls = (const H5FD_class_mpi_t *)(file->cls);
    HDassert(cls);
    HDassert(cls->get_mpi_info);    /* All MPI drivers must implement this */

    /* Dispatch to driver */
    if((ret_value = (cls->get_mpi_info)(file, mpi_info)) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "driver get_mpi_info request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_get_mpi_info() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_mpi_MPIOff_to_haddr
 *
 * Purpose:     Convert an MPI_Offset value to haddr_t.
 *
 * Return:      Success:	The haddr_t equivalent of the MPI_OFF
 *				argument.
 *
 *              Failure:	HADDR_UNDEF
 *
 * Programmer:  Unknown
 *              January 30, 1998
 *
 * Modifications:
 * 		Robb Matzke, 1999-04-23
 *		An error is reported for address overflows. The ADDR output
 *		argument is optional.
 *
 * 		Robb Matzke, 1999-08-06
 *		Modified to work with the virtual file layer.
 *-------------------------------------------------------------------------
 */
haddr_t
H5FD_mpi_MPIOff_to_haddr(MPI_Offset mpi_off)
{
    haddr_t ret_value=HADDR_UNDEF;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if (mpi_off != (MPI_Offset)(haddr_t)mpi_off)
        ret_value=HADDR_UNDEF;
    else
        ret_value=(haddr_t)mpi_off;

    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_mpi_haddr_to_MPIOff
 *
 * Purpose:     Convert an haddr_t value to MPI_Offset.
 *
 * Return:      Success:	Non-negative, the MPI_OFF argument contains
 *				the converted value.
 *
 * 		Failure:	Negative, MPI_OFF is undefined.
 *
 * Programmer:  Unknown
 *              January 30, 1998
 *
 * Modifications:
 * 		Robb Matzke, 1999-04-23
 *		An error is reported for address overflows. The ADDR output
 *		argument is optional.
 *
 * 		Robb Matzke, 1999-07-28
 *		The ADDR argument is passed by value.
 *
 * 		Robb Matzke, 1999-08-06
 *		Modified to work with the virtual file layer.
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_mpi_haddr_to_MPIOff(haddr_t addr, MPI_Offset *mpi_off/*out*/)
{
    herr_t ret_value=FAIL;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(mpi_off);

    /* Convert the HDF5 address into an MPI offset */
    *mpi_off = (MPI_Offset)addr;

    if (addr != (haddr_t)((MPI_Offset)addr))
        ret_value=FAIL;
    else
        ret_value=SUCCEED;

    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_mpi_comm_info_dup
 *
 * Purpose:     Make duplicates of communicator and Info object.
 * 		If the Info object is in fact MPI_INFO_NULL, no duplicate
 * 		is made but the same value assigned to the new Info object
 * 		handle.
 *
 * Return:      Success:	Non-negative.  The new communicator and Info
 * 				object handles are returned via comm_new and
 * 				info_new pointers.
 *
 * 		Failure:	Negative.
 *
 * Programmer:  Albert Cheng
 *              Jan  8, 2003
 *
 * Modifications:
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_mpi_comm_info_dup(MPI_Comm comm, MPI_Info info, MPI_Comm *comm_new, MPI_Info *info_new)
{
    herr_t	ret_value=SUCCEED;
    MPI_Comm	comm_dup=MPI_COMM_NULL;
    MPI_Info	info_dup=MPI_INFO_NULL;
    int		mpi_code;

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    if (MPI_COMM_NULL == comm)
	HGOTO_ERROR(H5E_INTERNAL, H5E_BADVALUE, FAIL, "not a valid argument")
    if (!comm_new || !info_new)
	HGOTO_ERROR(H5E_INTERNAL, H5E_BADVALUE, FAIL, "bad pointers")

    /* Dup them.  Using temporary variables for error recovery cleanup. */
    if (MPI_SUCCESS != (mpi_code=MPI_Comm_dup(comm, &comm_dup)))
	HMPI_GOTO_ERROR(FAIL, "MPI_Comm_dup failed", mpi_code)
    if (MPI_INFO_NULL != info){
	if (MPI_SUCCESS != (mpi_code=MPI_Info_dup(info, &info_dup)))
	    HMPI_GOTO_ERROR(FAIL, "MPI_Info_dup failed", mpi_code)
    }else{
	/* No dup, just copy it. */
	info_dup = info;
    }

    /* Set MPI_ERRORS_RETURN on comm_dup so that MPI failures are not fatal, 
       and return codes can be checked and handled. May 23, 2017 FTW */
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_set_errhandler(comm_dup, MPI_ERRORS_RETURN)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Errhandler_set failed", mpi_code)
 
    /* copy them to the return arguments */
    *comm_new = comm_dup;
    *info_new = info_dup;

done:
    if (FAIL == ret_value){
	/* need to free anything created here */
	if (MPI_COMM_NULL != comm_dup)
	    MPI_Comm_free(&comm_dup);
	if (MPI_INFO_NULL != info_dup)
	    MPI_Info_free(&info_dup);
    }

    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5FD_mpi_comm_info_free
 *
 * Purpose:     Free the communicator and Info object.
 * 		If comm or info is in fact MPI_COMM_NULL or MPI_INFO_NULL
 * 		respectively, no action occurs to it.
 *
 * Return:      Success:	Non-negative.  The values the pointers refer
 * 				to will be set to the corresponding NULL
 * 				handles.
 *
 * 		Failure:	Negative.
 *
 * Programmer:  Albert Cheng
 *              Jan  8, 2003
 *
 * Modifications:
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_mpi_comm_info_free(MPI_Comm *comm, MPI_Info *info)
{
    herr_t      ret_value=SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    if (!comm || !info)
	HGOTO_ERROR(H5E_INTERNAL, H5E_BADVALUE, FAIL, "not a valid argument")

    if (MPI_COMM_NULL != *comm)
	MPI_Comm_free(comm);
    if (MPI_INFO_NULL != *info)
	MPI_Info_free(info);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

#ifdef NOT_YET

/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_wait_for_left_neighbor
 *
 * Purpose:	Blocks until (empty) msg is received from immediately
 *		lower-rank neighbor. In conjunction with
 *		H5FD_mpio_signal_right_neighbor, useful for enforcing
 *		1-process-at-at-time access to critical regions to avoid race
 *		conditions (though it is overkill to require that the
 *		processes be allowed to proceed strictly in order of their
 *		rank).
 *
 * Note:	This routine doesn't read or write any file, just performs
 *		interprocess coordination. It really should reside in a
 *		separate package of such routines.
 *
 * Return:	Success:	0
 *		Failure:	-1
 *
 * Programmer:	rky
 *              19981207
 *
 * Modifications:
 *		Robb Matzke, 1999-08-09
 *		Modified to work with the virtual file layer.
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_mpio_wait_for_left_neighbor(H5FD_t *_file)
{
    H5FD_mpio_t	*file = (H5FD_mpio_t*)_file;
    char msgbuf[1];
    MPI_Status rcvstat;
    int		mpi_code;		/* mpi return code */
    herr_t      ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(file);
    HDassert(H5FD_MPIO==file->pub.driver_id);

    /* Portably initialize MPI status variable */
    HDmemset(&rcvstat,0,sizeof(MPI_Status));

    /* p0 has no left neighbor; all other procs wait for msg */
    if (file->mpi_rank != 0) {
        if (MPI_SUCCESS != (mpi_code=MPI_Recv( &msgbuf, 1, MPI_CHAR,
			file->mpi_rank-1, MPI_ANY_TAG, file->comm, &rcvstat )))
            HMPI_GOTO_ERROR(FAIL, "MPI_Recv failed", mpi_code)
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_mpio_signal_right_neighbor
 *
 * Purpose:	Blocks until (empty) msg is received from immediately
 *		lower-rank neighbor. In conjunction with
 *		H5FD_mpio_wait_for_left_neighbor, useful for enforcing
 *		1-process-at-at-time access to critical regions to avoid race
 *		conditions (though it is overkill to require that the
 *		processes be allowed to proceed strictly in order of their
 *		rank).
 *
 * Note: 	This routine doesn't read or write any file, just performs
 *		interprocess coordination. It really should reside in a
 *		separate package of such routines.
 *
 * Return:	Success:	0
 *		Failure:	-1
 *
 * Programmer:	rky
 *              19981207
 *
 * Modifications:
 *		Robb Matzke, 1999-08-09
 *		Modified to work with the virtual file layer.
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_mpio_signal_right_neighbor(H5FD_t *_file)
{
    H5FD_mpio_t	*file = (H5FD_mpio_t*)_file;
    char msgbuf[1];
    int		mpi_code;		/* mpi return code */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(file);
    HDassert(H5FD_MPIO==file->pub.driver_id);

    if(file->mpi_rank != (file->mpi_size - 1))
        if(MPI_SUCCESS != (mpi_code=MPI_Send(&msgbuf, 0/*empty msg*/, MPI_CHAR, file->mpi_rank + 1, 0, file->comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Send failed", mpi_code)

done:
    FUNC_LEAVE_NOAPI(ret_value)
}
#endif /* NOT_YET */
#endif /* H5_HAVE_PARALLEL */

