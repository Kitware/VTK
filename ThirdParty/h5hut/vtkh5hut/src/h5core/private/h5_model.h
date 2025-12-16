/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5_MODEL_H
#define __PRIVATE_H5_MODEL_H


/* WARNING! Changing these values will alter the data model and introduce
 * file incompatibilities with previous versions. */

#define H5_DATANAME_LEN		H5_MAX_NAME_LEN
#define H5_ITERATION_NAME_LEN	H5_MAX_NAME_LEN
#define H5_ITERATION_NAME	"Step"
#define H5_ITERATION_NUM_WIDTH	1
#define H5BLOCK_GROUPNAME_BLOCK	"Block"
#define H5_BLOCKNAME_X		"0"
#define H5_BLOCKNAME_Y		"1"
#define H5_BLOCKNAME_Z		"2"
#define H5_ATTACHMENT		"Attachment"

#include "h5core/h5_types.h"
#include "h5core/h5_model.h"
#include "private/h5_const.h"
#include "private/h5_file.h"
#include "private/h5_mpi.h"
#include "private/h5_hdf5.h"

#ifdef H5_HAVE_PARALLEL
static inline h5_err_t
h5priv_start_throttle (
	const h5_file_p f
	) {
	H5_CORE_API_ENTER (h5_err_t, "f=%p", f);
	if (f->props->throttle > 0) {
		// throttle only if VFD is MPIO independent
		h5_int64_t mask = H5_VFD_MPIO_INDEPENDENT;
#if H5_VERSION_LE(1,8,12)
		//  or MPI POSIX - which has been removed in newer versions of hdf5
		mask |= H5_VFD_MPIO_POSIX;
#endif
		if (! (f->props->flags & mask)) {
			h5_warn (
				"Throttling is only permitted with the MPI-POSIX "
				"or MPI-IO Independent VFD." );
			H5_LEAVE (H5_SUCCESS);
		}

		int token = 1;
		h5_info (
			"Throttling with factor = %lld",
			(long long int)f->props->throttle);
		if (f->myproc / f->props->throttle > 0) {
			h5_debug (
				"throttle: waiting on token from %lld",
				(long long int)(f->myproc - f->props->throttle));
			// wait to receive token before continuing with read
			TRY( h5priv_mpi_recv(
				     &token, 1, MPI_INT,
				     // receive from previous proc
				     f->myproc - f->props->throttle,
				     // use this proc id as message tag
				     f->myproc,
				     f->props->comm
				     ) );
		}
		h5_debug ("throttle: received token");
	}
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
h5priv_end_throttle (
	const h5_file_p f
	) {
	H5_PRIV_API_ENTER (h5_err_t, "f=%p", f);
	if (f->props->throttle > 0) {
		int token;
		if (f->myproc + f->props->throttle < f->nprocs) {
			// pass token to next proc 
			h5_debug (
				"throttle: passing token to %lld",
				(long long int)(f->myproc + f->props->throttle));
			TRY (h5priv_mpi_send(
				     &token, 1, MPI_INT,
				      // send to next proc
				     f->myproc + f->props->throttle,
				      // use the id of the target as tag
				     f->myproc + f->props->throttle,
				     f->props->comm
				     ));
		}
	}
	H5_RETURN (H5_SUCCESS);
}
#else // H5_HAVE_PARALLEL
static inline h5_err_t
h5priv_start_throttle (const h5_file_p f) {
	UNUSED_ARGUMENT (f);
	return H5_SUCCESS;
}

static inline h5_err_t
h5priv_end_throttle (const h5_file_p f) {
	UNUSED_ARGUMENT (f);
	return H5_SUCCESS;
}

#endif // H5_HAVE_PARALLEL


h5_err_t
h5priv_close_iteration (
	const h5_file_p f
	);

/*
  Map given enumeration type to corresponding HDF5 type. We use this HDF5
  type for reading and writing datasets and attributes.
 */

static inline hid_t
h5priv_map_enum_to_normalized_type (
	h5_types_t type
	) {
	H5_PRIV_API_ENTER (hid_t,
			   "type=%lld",
			   (long long int)type);
	switch (type) {
	case H5_STRING_T:
		ret_value = H5_STRING;
		break;
	case H5_INT8_T: 
		ret_value = H5_INT8;
		break;
	case H5_UINT8_T:
		ret_value = H5_UINT8;
		break;
	case H5_INT16_T:
		ret_value = H5_INT16;
		break;
	case H5_UINT16_T:
		ret_value = H5_UINT16;
		break;
	case H5_INT32_T:
		ret_value = H5_INT32;
		break;
	case H5_UINT32_T:
		ret_value = H5_UINT32;
		break;
	case H5_INT64_T:
		ret_value = H5_INT64;
		break;
	case H5_UINT64_T:
		ret_value = H5_UINT64;
		break;
	case H5_FLOAT32_T:
		ret_value = H5_FLOAT32;
		break;
	case H5_FLOAT64_T:
		ret_value = H5_FLOAT64;
		break;
	default:
		H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Unknown type %d", (int)type);
	}
	H5_RETURN (ret_value);
}

/*
  Normaize a given HDF5 data type.
 */
static inline hid_t
h5priv_normalize_type (
	hid_t type
	) {
	H5_PRIV_API_ENTER (hid_t,
			   "type=%lld",
			   (long long int)type);
	H5T_class_t tclass;
	TRY (tclass = H5Tget_class (type));
	int tsize;
	TRY (tsize = H5Tget_size (type));
	H5T_sign_t tsign;
	TRY (tsign = H5Tget_sign (type));
	switch (tclass){
	case H5T_INTEGER:
		if (tsize==8) {
			if (tsign == H5T_SGN_2) {
				ret_value = H5_INT64;
			} else {
				ret_value = H5_UINT64;
			}
		} else if (tsize==4) {
			if (tsign == H5T_SGN_2) {
				ret_value = H5_INT32;
			} else {
				ret_value = H5_UINT32;
			}
		} else if (tsize==2) {
			if (tsign == H5T_SGN_2) {
				ret_value = H5_INT16;
			} else {
				ret_value = H5_UINT16;
			}
		} else if (tsize==1) {
			if (tsign == H5T_SGN_2) {
				ret_value = H5_INT8;
			} else {
				ret_value = H5_UINT8;
			}
		}
		break;
	case H5T_FLOAT:
		if (tsize==8) {
			ret_value = H5_FLOAT64;
		}
		else if (tsize==4) {
			ret_value = H5_FLOAT32;
		}
		break;
	case H5T_STRING:
		ret_value = H5_STRING;
		break;
	default:
		break;
	}
	if (ret_value < 0) {
		H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Unknown type %d", (int)type);
	}
	H5_RETURN (ret_value);
}

/*
  Map HDF5 type to H5hut type enumeration.
 */
static inline h5_int64_t
h5priv_map_hdf5_type_to_enum (
	hid_t type
	) {
	H5_PRIV_API_ENTER (h5_int64_t,
			   "type=%lld",
			   (long long int)type);
	H5T_class_t tclass;
	TRY (tclass = H5Tget_class (type));
	int tsize;
	TRY (tsize = H5Tget_size (type));
	H5T_sign_t tsign;
	TRY (tsign = H5Tget_sign (type));
	switch (tclass){
	case H5T_INTEGER:
		if (tsize==8) {
			if (tsign == H5T_SGN_2) {
				ret_value = H5_INT64_T;
			} else {
				ret_value = H5_UINT64_T;
			}
		} else if (tsize==4) {
			if (tsign == H5T_SGN_2) {
				ret_value = H5_INT32_T;
			} else {
				ret_value = H5_UINT32_T;
			}
		} else if (tsize==2) {
			if (tsign == H5T_SGN_2) {
				ret_value = H5_INT16_T;
			} else {
				ret_value = H5_UINT16_T;
			}
		} else if (tsize==1) {
			if (tsign == H5T_SGN_2) {
				ret_value = H5_INT8_T;
			}
			else {
				ret_value = H5_UINT8_T;
			}
		} else {
			ret_value = H5_STRING_T;
		}
		break;
	case H5T_FLOAT:
		if (tsize==8) {
			ret_value = H5_FLOAT64_T;
		}
		else if (tsize==4) {
			ret_value = H5_FLOAT32_T;
		}
		break;
	case H5T_STRING:
		ret_value = H5_STRING_T;
		break;
	default:
		H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Unknown type %d", (int)type);
	}
	H5_RETURN (ret_value);
}


static inline hid_t
h5priv_get_normalized_dataset_type (
	hid_t dataset
	) {
	H5_PRIV_API_ENTER (hid_t,
			   "dataset=%lld",
			   (long long)dataset);
	TRY (ret_value = hdf5_get_dataset_type (dataset));
	TRY (ret_value = h5priv_normalize_type (ret_value));
	H5_RETURN (ret_value);
}

#endif
