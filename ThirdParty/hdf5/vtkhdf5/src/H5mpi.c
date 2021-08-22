/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Purpose:     Common MPI routines
 *
 */

#include "H5private.h"   /* Generic Functions                        */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5MMprivate.h" /* Memory Management                        */

#ifdef H5_HAVE_PARALLEL

/****************/
/* Local Macros */
/****************/
#define TWO_GIG_LIMIT INT32_MAX
#ifndef H5_MAX_MPI_COUNT
#define H5_MAX_MPI_COUNT (1 << 30)
#endif

/*******************/
/* Local Variables */
/*******************/
static hsize_t bigio_count_g = H5_MAX_MPI_COUNT;

/*-------------------------------------------------------------------------
 * Function:  H5_mpi_set_bigio_count
 *
 * Purpose:   Allow us to programatically change the switch point
 *            when we utilize derived datatypes.  This is of
 *            particular interest for allowing nightly testing
 *
 * Return:    The current/previous value of bigio_count_g.
 *
 * Programmer: Richard Warren,  March 10, 2017
 *
 *-------------------------------------------------------------------------
 */
hsize_t
H5_mpi_set_bigio_count(hsize_t new_count)
{
    hsize_t orig_count = bigio_count_g;

    if ((new_count > 0) && (new_count < (hsize_t)TWO_GIG_LIMIT)) {
        bigio_count_g = new_count;
    }
    return orig_count;
} /* end H5_mpi_set_bigio_count() */

/*-------------------------------------------------------------------------
 * Function:  H5_mpi_get_bigio_count
 *
 * Purpose:   Allow other HDF5 library functions to access
 *            the current value for bigio_count_g.
 *
 * Return:    The current/previous value of bigio_count_g.
 *
 * Programmer: Richard Warren,  October 7, 2019
 *
 *-------------------------------------------------------------------------
 */
hsize_t
H5_mpi_get_bigio_count(void)
{
    return bigio_count_g;
}

/*-------------------------------------------------------------------------
 * Function:    H5_mpi_comm_dup
 *
 * Purpose:     Duplicate an MPI communicator.
 *
 *              Does not duplicate MPI_COMM_NULL. Instead, comm_new will
 *              be set to MPI_COMM_NULL directly.
 *
 *              The new communicator is returned via the comm_new pointer.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5_mpi_comm_dup(MPI_Comm comm, MPI_Comm *comm_new)
{
    herr_t   ret_value = SUCCEED;
    MPI_Comm comm_dup  = MPI_COMM_NULL;
    int      mpi_code;

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    if (!comm_new)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADVALUE, FAIL, "comm_new cannot be NULL")

    /* Handle MPI_COMM_NULL separately */
    if (MPI_COMM_NULL == comm) {
        /* Don't duplicate MPI_COMM_NULL since that's an error in MPI */
        comm_dup = MPI_COMM_NULL;
    }
    else {

        /* Duplicate the MPI communicator */
        if (MPI_SUCCESS != (mpi_code = MPI_Comm_dup(comm, &comm_dup)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Comm_dup failed", mpi_code)

        /* Set MPI_ERRORS_RETURN on comm_dup so that MPI failures are not fatal,
         * and return codes can be checked and handled.
         */
        if (MPI_SUCCESS != (mpi_code = MPI_Comm_set_errhandler(comm_dup, MPI_ERRORS_RETURN)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Errhandler_set failed", mpi_code)
    }

    /* Copy the new communicator to the return argument */
    *comm_new = comm_dup;

done:
    if (FAIL == ret_value) {
        /* need to free anything created here */
        if (MPI_COMM_NULL != comm_dup)
            MPI_Comm_free(&comm_dup);
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5_mpi_comm_dup() */

/*-------------------------------------------------------------------------
 * Function:    H5_mpi_info_dup
 *
 * Purpose:     Duplicate an MPI info.
 *
 *              If the info object is MPI_INFO_NULL, no duplicate
 *              is made but the same value assigned to the new info object
 *              handle.
 *
 *              The new info is returned via the info_new pointer.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5_mpi_info_dup(MPI_Info info, MPI_Info *info_new)
{
    herr_t   ret_value = SUCCEED;
    MPI_Info info_dup  = MPI_INFO_NULL;
    int      mpi_code;

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    if (!info_new)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADVALUE, FAIL, "info_new cannot be NULL")

    /* Duplicate the MPI info */
    if (info == MPI_INFO_NULL) {
        /* Don't duplicate MPI_INFO_NULL. Just copy it. */
        info_dup = MPI_INFO_NULL;
    }
    else {
        /* Duplicate the info */
        if (MPI_SUCCESS != (mpi_code = MPI_Info_dup(info, &info_dup)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Info_dup failed", mpi_code)
    }

    /* Copy the new info to the return argument */
    *info_new = info_dup;

done:
    if (FAIL == ret_value) {
        /* need to free anything created here */
        if (MPI_INFO_NULL != info_dup)
            MPI_Info_free(&info_dup);
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5_mpi_info_dup() */

/*-------------------------------------------------------------------------
 * Function:    H5_mpi_comm_free
 *
 * Purpose:     Free an MPI communicator.
 *
 *              If comm is MPI_COMM_NULL this call does nothing.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5_mpi_comm_free(MPI_Comm *comm)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    if (!comm)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADVALUE, FAIL, "comm pointer cannot be NULL")

    /* Free the communicator */
    if (MPI_COMM_WORLD != *comm && MPI_COMM_NULL != *comm)
        MPI_Comm_free(comm);

    *comm = MPI_COMM_NULL;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* End H5_mpi_comm_free() */

/*-------------------------------------------------------------------------
 * Function:    H5_mpi_info_free
 *
 * Purpose:     Free the MPI info.
 *
 *              If info is MPI_INFO_NULL this call does nothing.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5_mpi_info_free(MPI_Info *info)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    if (!info)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADVALUE, FAIL, "info pointer cannot be NULL")

    /* Free the info */
    if (MPI_INFO_NULL != *info)
        MPI_Info_free(info);

    *info = MPI_INFO_NULL;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* End H5_mpi_info_free() */

/*-------------------------------------------------------------------------
 * Function:    H5_mpi_comm_cmp
 *
 * Purpose:     Compares two MPI communicators.
 *
 *              Note that passing MPI_COMM_NULL to this function will not
 *              throw errors, unlike MPI_Comm_compare().
 *
 *              We consider MPI communicators to be the "same" when the
 *              groups are identical. We don't care about the context
 *              since that will always be different as we call MPI_Comm_dup
 *              when we store the communicator in the fapl.
 *
 *              The out parameter is a value like strcmp. The value is
 *              undefined when the return value is FAIL.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5_mpi_comm_cmp(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
    int    mpi_code;
    int    mpi_result = MPI_IDENT;
    herr_t ret_value  = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    if (!result)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADVALUE, FAIL, "result cannot be NULL")

    /* Set out parameter to something reasonable in case something goes wrong */
    *result = 0;

    /* Can't pass MPI_COMM_NULL to MPI_Comm_compare() so we have to handle
     * it in special cases.
     *
     * MPI_Comm can either be an integer type or a pointer. We cast them
     * to intptr_t so we can compare them with < and > when needed.
     */
    if (MPI_COMM_NULL == comm1 && MPI_COMM_NULL == comm2) {
        /* Special case of both communicators being MPI_COMM_NULL */
        *result = 0;
    }
    else if (MPI_COMM_NULL == comm1 || MPI_COMM_NULL == comm2) {

        /* Special case of one communicator being MPI_COMM_NULL */
        *result = (intptr_t)comm1 < (intptr_t)comm2 ? -1 : 1;
    }
    else {

        /* Normal communicator compare */

        /* Compare the MPI communicators */
        if (MPI_SUCCESS != (mpi_code = MPI_Comm_compare(comm1, comm2, &mpi_result)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Comm_compare failed", mpi_code)

        /* Set the result */
        if (MPI_IDENT == mpi_result || MPI_CONGRUENT == mpi_result)
            *result = 0;
        else
            *result = (intptr_t)comm1 < (intptr_t)comm2 ? -1 : 1;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5_mpi_comm_cmp() */

/*-------------------------------------------------------------------------
 * Function:    H5_mpi_info_cmp
 *
 * Purpose:     Compares two MPI info objects.
 *
 *              For our purposes, two mpi info objects are the "same" if
 *              they contain the same key-value pairs or are both
 *              MPI_INFO_NULL.
 *
 *              The out parameter is a value like strcmp. The value is
 *              undefined when the return value is FAIL.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5_mpi_info_cmp(MPI_Info info1, MPI_Info info2, int *result)
{
    hbool_t same      = FALSE;
    char *  key       = NULL;
    char *  value1    = NULL;
    char *  value2    = NULL;
    herr_t  ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    if (!result)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADVALUE, FAIL, "result cannot be NULL")

    /* Check for MPI_INFO_NULL */
    if (MPI_INFO_NULL == info1 && MPI_INFO_NULL == info2) {
        /* Special case of both info objects being MPI_INFO_NULL */
        same = TRUE;
    }
    else if (MPI_INFO_NULL == info1 || MPI_INFO_NULL == info2) {

        /* Special case of one info object being MPI_INFO_NULL */
        same = FALSE;
    }
    else {
        int mpi_code;
        int nkeys_1;
        int nkeys_2;

        /* Check if the number of keys is the same */
        if (MPI_SUCCESS != (mpi_code = MPI_Info_get_nkeys(info1, &nkeys_1)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Info_get_nkeys failed", mpi_code)
        if (MPI_SUCCESS != (mpi_code = MPI_Info_get_nkeys(info2, &nkeys_2)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Info_get_nkeys failed", mpi_code)

        if (nkeys_1 != nkeys_2)
            same = FALSE;
        else if (0 == nkeys_1 && 0 == nkeys_2)
            same = TRUE;
        else {
            int i;
            int flag1 = -1;
            int flag2 = -1;

            /* Allocate buffers for iteration */
            if (NULL == (key = (char *)H5MM_malloc(MPI_MAX_INFO_KEY * sizeof(char))))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
            if (NULL == (value1 = (char *)H5MM_malloc(MPI_MAX_INFO_VAL * sizeof(char))))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
            if (NULL == (value2 = (char *)H5MM_malloc(MPI_MAX_INFO_VAL * sizeof(char))))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

            /* Iterate over the keys, comparing them */
            for (i = 0; i < nkeys_1; i++) {

                same = TRUE;

                /* Memset the buffers to zero */
                HDmemset(key, 0, MPI_MAX_INFO_KEY);
                HDmemset(value1, 0, MPI_MAX_INFO_VAL);
                HDmemset(value2, 0, MPI_MAX_INFO_VAL);

                /* Get the nth key */
                if (MPI_SUCCESS != (mpi_code = MPI_Info_get_nthkey(info1, i, key)))
                    HMPI_GOTO_ERROR(FAIL, "MPI_Info_get_nthkey failed", mpi_code)

                /* Get the values */
                if (MPI_SUCCESS != (mpi_code = MPI_Info_get(info1, key, MPI_MAX_INFO_VAL, value1, &flag1)))
                    HMPI_GOTO_ERROR(FAIL, "MPI_Info_get failed", mpi_code)
                if (MPI_SUCCESS != (mpi_code = MPI_Info_get(info2, key, MPI_MAX_INFO_VAL, value2, &flag2)))
                    HMPI_GOTO_ERROR(FAIL, "MPI_Info_get failed", mpi_code)

                /* Compare values and flags */
                if (!flag1 || !flag2 || HDmemcmp(value1, value2, MPI_MAX_INFO_VAL)) {
                    same = FALSE;
                    break;
                }

            } /* end for */
        }     /* end else */
    }         /* end else */

    /* Set the output value
     *
     * MPI_Info can either be an integer type or a pointer. We cast them
     * to intptr_t so we can compare them with < and > when needed.
     */
    if (same)
        *result = 0;
    else
        *result = (intptr_t)info1 < (intptr_t)info2 ? -1 : 1;

done:
    if (key)
        H5MM_xfree(key);
    if (value1)
        H5MM_xfree(value1);
    if (value2)
        H5MM_xfree(value2);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5_mpi_info_cmp() */

/*-------------------------------------------------------------------------
 * Function:    H5_mpio_create_large_type
 *
 * Purpose:     Create a large datatype of size larger than what a 32 bit integer
 *              can hold.
 *
 * Return:      Non-negative on success, negative on failure.
 *
 *              *new_type    the new datatype created
 *
 * Programmer:  Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5_mpio_create_large_type(hsize_t num_elements, MPI_Aint stride_bytes, MPI_Datatype old_type,
                          MPI_Datatype *new_type)
{
    int          num_big_types;   /* num times the 2G datatype will be repeated */
    int          remaining_bytes; /* the number of bytes left that can be held in an int value */
    hsize_t      leftover;
    int          block_len[2];
    int          mpi_code; /* MPI return code */
    MPI_Datatype inner_type, outer_type, leftover_type, type[2];
    MPI_Aint     disp[2], old_extent;
    herr_t       ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Calculate how many Big MPI datatypes are needed to represent the buffer */
    num_big_types = (int)(num_elements / bigio_count_g);
    leftover      = (hsize_t)num_elements - (hsize_t)num_big_types * bigio_count_g;
    H5_CHECKED_ASSIGN(remaining_bytes, int, leftover, hsize_t);

    /* Create a contiguous datatype of size equal to the largest
     * number that a 32 bit integer can hold x size of old type.
     * If the displacement is 0, then the type is contiguous, otherwise
     * use type_hvector to create the type with the displacement provided
     */
    if (0 == stride_bytes) {
        if (MPI_SUCCESS != (mpi_code = MPI_Type_contiguous((int)bigio_count_g, old_type, &inner_type)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_contiguous failed", mpi_code)
    } /* end if */
    else if (MPI_SUCCESS !=
             (mpi_code = MPI_Type_create_hvector((int)bigio_count_g, 1, stride_bytes, old_type, &inner_type)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_create_hvector failed", mpi_code)

    /* Create a contiguous datatype of the buffer (minus the remaining < 2GB part)
     * If a stride is present, use hvector type
     */
    if (0 == stride_bytes) {
        if (MPI_SUCCESS != (mpi_code = MPI_Type_contiguous(num_big_types, inner_type, &outer_type)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_contiguous failed", mpi_code)
    } /* end if */
    else if (MPI_SUCCESS !=
             (mpi_code = MPI_Type_create_hvector(num_big_types, 1, stride_bytes, inner_type, &outer_type)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_create_hvector failed", mpi_code)

    MPI_Type_free(&inner_type);

    /* If there is a remaining part create a contiguous/vector datatype and then
     * use a struct datatype to encapsulate everything.
     */
    if (remaining_bytes) {
        if (stride_bytes == 0) {
            if (MPI_SUCCESS != (mpi_code = MPI_Type_contiguous(remaining_bytes, old_type, &leftover_type)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Type_contiguous failed", mpi_code)
        } /* end if */
        else if (MPI_SUCCESS != (mpi_code = MPI_Type_create_hvector(
                                     (int)(num_elements - (hsize_t)num_big_types * bigio_count_g), 1,
                                     stride_bytes, old_type, &leftover_type)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_create_hvector failed", mpi_code)

        /* As of version 4.0, OpenMPI now turns off MPI-1 API calls by default,
         * so we're using the MPI-2 version even though we don't need the lb
         * value.
         */
        {
            MPI_Aint unused_lb_arg;
            MPI_Type_get_extent(old_type, &unused_lb_arg, &old_extent);
        }

        /* Set up the arguments for MPI_Type_struct constructor */
        type[0]      = outer_type;
        type[1]      = leftover_type;
        block_len[0] = 1;
        block_len[1] = 1;
        disp[0]      = 0;
        disp[1]      = (old_extent + stride_bytes) * num_big_types * (MPI_Aint)bigio_count_g;

        if (MPI_SUCCESS != (mpi_code = MPI_Type_create_struct(2, block_len, disp, type, new_type)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_create_struct failed", mpi_code)

        MPI_Type_free(&outer_type);
        MPI_Type_free(&leftover_type);
    } /* end if */
    else
        /* There are no remaining bytes so just set the new type to
         * the outer type created */
        *new_type = outer_type;

    if (MPI_SUCCESS != (mpi_code = MPI_Type_commit(new_type)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code)

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5_mpio_create_large_type() */

#endif /* H5_HAVE_PARALLEL */
