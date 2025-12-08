/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include "private/h5_types.h"
#include "private/h5_hdf5.h"

#include "private/h5_model.h"
#include "private/h5_mpi.h"
#include "private/h5_io.h"
#include "private/h5b_types.h"
#include "private/h5b_model.h"

#include "h5core/h5_syscall.h"
#include "h5core/h5b_io.h"

#include <string.h>

/*!
  \ingroup h5_private

  \internal

  Initialize H5Block internal structure.

  TODO: Move to file "h5b_openclose.c"

  \return	H5_SUCCESS or error code
*/
h5_err_t
h5bpriv_open_file (
	const h5_file_p f		/*!< IN: file handle */
	) {
	H5_PRIV_API_ENTER (h5_err_t, "f=%p", f);
	h5b_fdata_t* b; 

	if (f->b)
		H5_LEAVE (H5_SUCCESS);

	TRY (f->b = (h5b_fdata_t*)h5_calloc (1, sizeof (*f->b)));

	b = f->b;
	memset (b, 0, sizeof (*b));

#ifdef H5_HAVE_PARALLEL
	size_t n = sizeof (struct h5b_partition) / sizeof (h5_int64_t);
	TRY (h5priv_mpi_type_contiguous(n, MPI_LONG_LONG, &b->partition_mpi_t));
#endif
	memset (b->user_layout, 0, sizeof(*b->user_layout));
	memset (b->write_layout, 0, sizeof(*b->write_layout));

	b->shape = -1;
	b->diskshape = -1;
	b->memshape = -1;
	b->block_gid = -1;
	b->field_gid = -1;
	b->have_layout = 0;

	TRY (b->dcreate_prop = hdf5_create_property (H5P_DATASET_CREATE));

	H5_RETURN (H5_SUCCESS);
}

/*!
  \ingroup h5block_private

  \internal

  De-initialize H5Block internal structure.  Open HDF5 objects are 
  closed and allocated memory freed.

  \return	H5_SUCCESS or error code
*/
h5_err_t
h5bpriv_close_file (
	const h5_file_p f	/*!< IN: file handle */
	) {
	H5_PRIV_API_ENTER (h5_err_t, "f=%p", f);
	struct h5b_fdata* b = f->b;
	TRY (hdf5_close_group (b->block_gid));
	TRY (hdf5_close_group (b->field_gid));
	TRY (hdf5_close_dataspace (b->shape));
	TRY (hdf5_close_dataspace (b->diskshape));
	TRY (hdf5_close_dataspace (b->memshape));
	TRY (hdf5_close_property (b->dcreate_prop));
#ifdef H5_HAVE_PARALLEL
	TRY (h5priv_mpi_type_free (&b->partition_mpi_t));
#endif
	TRY (h5_free (f->b));
	f->b = NULL;

	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
select_hyperslab_for_writing (
	const h5_file_p f		/*!< IN: file handle */
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "f=%p", f);
	/*
	   re-use existing hyperslab
	 */
	if ( f->b->shape >= 0 )
		H5_LEAVE (H5_SUCCESS);

	h5b_fdata_t *b = f->b;
	h5b_partition_t *p = b->write_layout;
	h5b_partition_t *q = b->user_layout;

	int rank = 3;

	hsize_t field_dims[3] = {
		b->k_max+1,
		b->j_max+1,
		b->i_max+1
	};

	hsize_t start[3] = {
		p->k_start,
		p->j_start,
		p->i_start
	};
	hsize_t stride[3] = { 1, 1, 1 };
	hsize_t part_dims[3] = {
		p->k_end - p->k_start + 1,
		p->j_end - p->j_start + 1,
		p->i_end - p->i_start + 1
	};


	TRY (b->shape = hdf5_create_dataspace(rank, field_dims, NULL));
	TRY (b->diskshape = hdf5_create_dataspace(rank,field_dims,NULL));
	h5_debug (
		"Select hyperslab on diskshape: "
		"start=(%lld,%lld,%lld), "
		"stride=(%lld,%lld,%lld), "
		"dims=(%lld,%lld,%lld)",
		(long long)start[2],
		(long long)start[1],
		(long long)start[0],
		(long long)stride[2],
		(long long)stride[1],
		(long long)stride[0],
		(long long)part_dims[2],
		(long long)part_dims[1],
		(long long)part_dims[0]  );

	TRY( hdf5_select_hyperslab_of_dataspace(
	             b->diskshape,
	             H5S_SELECT_SET,
	             start,
	             stride,
	             part_dims,
	             NULL) );

	field_dims[0] = q->k_end - q->k_start + 1;
	field_dims[1] = q->j_end - q->j_start + 1;
	field_dims[2] = q->i_end - q->i_start + 1;

	TRY (b->memshape = hdf5_create_dataspace(rank,field_dims,NULL));

	start[0] = p->k_start - q->k_start;
	start[1] = p->j_start - q->j_start;
	start[2] = p->i_start - q->i_start;

	h5_debug (
		"Select hyperslab on memshape:"
		"start=(%lld,%lld,%lld), "
		"stride=(%lld,%lld,%lld), "
		"dims=(%lld,%lld,%lld)",
		(long long)start[2],
		(long long)start[1],
		(long long)start[0],
		(long long)stride[2],
		(long long)stride[1],
		(long long)stride[0],
		(long long)part_dims[2],
		(long long)part_dims[1],
		(long long)part_dims[0]  );

	TRY (hdf5_select_hyperslab_of_dataspace(
	             b->memshape,
	             H5S_SELECT_SET,
	             start,
	             stride,
	             part_dims,
	             NULL));

	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
write_data (
	const h5_file_p f,		/*!< IN: file handle */
	const char *data_name,		/*!< IN: name of dataset */
	const void *data,		/*!< IN: data to write */
	const h5_types_t type		/*!< IN: data type */
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "f=%p, data_name=%s, data=%p type=%lld",
	                    f, data_name, data, (long long int)type);
	hid_t dataset;
	h5b_fdata_t *b = f->b;
	hid_t hdf5_data_type;
	TRY (hdf5_data_type = h5priv_map_enum_to_normalized_type (type));
	h5_err_t exists;
	TRY (exists = hdf5_link_exists (b->field_gid, data_name));
	if ( exists > 0 ) {
		TRY (dataset = hdf5_open_dataset_by_name (b->field_gid, data_name));
		hid_t type_of_dataset;
		TRY (type_of_dataset = h5priv_get_normalized_dataset_type (dataset));
		if (hdf5_data_type != type_of_dataset) {
			H5_RETURN_ERROR (
				H5_ERR_HDF5,
				"Field '%s' already has type '%s' "
				"but was written as '%s'.",
				hdf5_get_objname (b->field_gid),
				hdf5_get_type_name (type_of_dataset),
				hdf5_get_type_name (hdf5_data_type));
		}
	} else {
		TRY (dataset = hdf5_create_dataset(
		             b->field_gid,
		             data_name,
		             hdf5_data_type,
		             b->shape,
		             b->dcreate_prop));
	}
	TRY (h5priv_start_throttle (f));
	TRY (hdf5_write_dataset(
	             dataset,
	             hdf5_data_type,
	             b->memshape,
	             b->diskshape,
	             f->props->xfer_prop,
	             data));
	TRY (h5priv_end_throttle (f));
	TRY (hdf5_close_dataset (dataset));

	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5b_write_scalar_data (
	const h5_file_t fh,		/*!< IN: file handle */
	const char* const field_name,	/*!< IN: name of field */
	const void* const data,		/*!< IN: data to write */
	const h5_types_t type		/*!< IN: data type */
	) {
        h5_file_p f = (h5_file_p)fh;
	H5_CORE_API_ENTER (h5_err_t,
	                   "f=%p, field_name='%s', data=%p, type=%lld",
	                   f, field_name, data, (long long int)type);
	check_iteration_is_writable (f);
	CHECK_LAYOUT (f);

	TRY (h5bpriv_create_field_group (f, field_name));
	TRY (select_hyperslab_for_writing (f));
	TRY (write_data (f, H5_BLOCKNAME_X, data, type));

	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5b_write_vector3d_data (
	const h5_file_t fh,		/*!< IN: file handle */
	const char* const field_name,	/*!< IN: name of field */
	const void* const xdata,	/*!< IN: x data to write */
	const void* const ydata,	/*!< IN: y data to write */
	const void* const zdata,	/*!< IN: z data to write */
	const h5_types_t type		/*!< IN: data type */
	) {
        h5_file_p f = (h5_file_p)fh;
	H5_CORE_API_ENTER (h5_err_t,
	                   "f=%p, field_name='%s', "
	                   "xdata=%p, "
	                   "ydata=%p, "
	                   "zdata=%p, "
	                   "type=%lld",
	                   f, field_name, xdata, ydata, zdata, (long long int)type);
	check_iteration_is_writable (f);
	CHECK_LAYOUT (f);

	TRY (h5bpriv_create_field_group(f, field_name));
	TRY (select_hyperslab_for_writing(f));
	TRY (write_data (f, H5_BLOCKNAME_X, xdata, type));
	TRY (write_data (f, H5_BLOCKNAME_Y, ydata, type));
	TRY (write_data (f, H5_BLOCKNAME_Z, zdata, type));

	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
select_hyperslab_for_reading (
	const h5_file_p f,			/*!< IN: file handle */
	const hid_t dataset
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
			    "f=%p, dataset=%lld",
			    f, (long long int)dataset);
	h5b_fdata_t *b = f->b;
	h5b_partition_t *p = b->user_layout;
	int rank;
	hsize_t field_dims[3];
	hsize_t start[3] = {
		p->k_start,
		p->j_start,
		p->i_start
	};
	hsize_t stride[3] = { 1, 1, 1 };
	hsize_t part_dims[3] = {
		p->k_end - p->k_start + 1,
		p->j_end - p->j_start + 1,
		p->i_end - p->i_start + 1
	};

	TRY( h5bpriv_release_hyperslab(f) );

	TRY (b->diskshape = hdf5_get_dataset_space (dataset));

	TRY (rank = hdf5_get_dims_of_dataspace(b->diskshape, field_dims, NULL));
	if (rank != 3)
		H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"H5Block dataset has bad rank '%d' instead"
			" of rank 3! Is the file corrupt?",
			rank);

	if ( (field_dims[0] < (hsize_t)b->k_max) ||
	     (field_dims[1] < (hsize_t)b->j_max) ||
	     (field_dims[2] < (hsize_t)b->i_max) )
		H5_RETURN_ERROR (
			H5_ERR_VIEW,
			"%s",
			"H5Block dataset has invalid view. "
			"Is the file corrupt?");

	h5_debug (
		"field_dims: (%lld,%lld,%lld)",
		(long long)field_dims[2],
		(long long)field_dims[1],
		(long long)field_dims[0] );

	TRY (b->memshape = hdf5_create_dataspace (rank, part_dims, NULL));

	TRY (hdf5_select_hyperslab_of_dataspace(
	             b->diskshape,
	             H5S_SELECT_SET,
	             start,
	             stride,
	             part_dims,
	             NULL));

	h5_debug (
		"Select hyperslab: "
		"start=(%lld,%lld,%lld), "
		"stride=(%lld,%lld,%lld), "
		"dims=(%lld,%lld,%lld)",
		(long long)start[2],
		(long long)start[1],
		(long long)start[0],
		(long long)stride[2],
		(long long)stride[1],
		(long long)stride[0],
		(long long)part_dims[2],
		(long long)part_dims[1],
		(long long)part_dims[0]  );

	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
read_data (
	const h5_file_p f,		/*!< IN: file handle */
	const char* const dataset_name,	/*!< IN: name of dataset */
	void* const data,		/*!< OUT: ptr to read buffer */
	const hid_t type      		/*!< IN: data type */
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "f=%p, dataset_name=%s, data=%p, type=%lld",
	                    f, dataset_name, data, (long long int)type);
	h5b_fdata_t *b = f->b;
	hid_t hdf5_data_type;
	TRY (hdf5_data_type = h5priv_map_enum_to_normalized_type (type));
	hid_t dataset;
	TRY (dataset = hdf5_open_dataset_by_name (b->field_gid, dataset_name));
	hid_t type_of_dataset;
	TRY (type_of_dataset = h5priv_get_normalized_dataset_type (dataset));
	if (hdf5_data_type != type_of_dataset) {
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Field '%s' has type '%s', but requested type is '%s'.",
			hdf5_get_objname (b->field_gid),
			hdf5_get_type_name (type_of_dataset),
			hdf5_get_type_name (hdf5_data_type));
	}

	TRY (select_hyperslab_for_reading (f, dataset) );
	TRY (h5priv_start_throttle (f));
	TRY (hdf5_read_dataset(
	             dataset,
	             hdf5_data_type,
	             f->b->memshape,
	             f->b->diskshape,
	             f->props->xfer_prop,
	             data));
	TRY (h5priv_end_throttle (f));
	TRY (hdf5_close_dataset(dataset));

	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5b_read_scalar_data (
	const h5_file_t fh,		/*!< IN: file handle */
	const char* const field_name,	/*!< IN: name of field */
	void* const data,		/*!< OUT: read bufer */
	const h5_types_t type		/*!< IN: data type */
	) {
        h5_file_p f = (h5_file_p)fh;
	H5_CORE_API_ENTER (h5_err_t,
	                   "f=%p, field_name='%s', data=%p, type=%lld",
	                   f, field_name, data, (long long int)type);
	check_iteration_is_readable (f);
	CHECK_LAYOUT (f);

	TRY (h5bpriv_open_field_group(f, field_name));
	TRY (read_data(f, H5_BLOCKNAME_X, data, type));

	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5b_read_vector3d_data (
	const h5_file_t fh,		/*!< IN: file handle */
	const char* const field_name,	/*!< IN: name of field */
	void* const xdata,		/*!< IN: x data to write */
	void* const ydata,		/*!< IN: y data to write */
	void* const zdata,		/*!< IN: z data to write */
	const h5_types_t type		/*!< IN: data type */
	) {
        h5_file_p f = (h5_file_p)fh;
	H5_CORE_API_ENTER (h5_err_t,
	                   "f=%p, field_name='%s', "
	                   "xdata=%p, "
	                   "ydata=%p, "
	                   "zdata=%p, "
	                   "type=%lld",
	                   f, field_name, xdata, ydata, zdata, (long long int)type);
	check_iteration_is_readable (f);
	CHECK_LAYOUT (f);

	TRY (h5bpriv_open_field_group(f, field_name));
	TRY (read_data(f, H5_BLOCKNAME_X, xdata, type));
	TRY (read_data(f, H5_BLOCKNAME_Y, ydata, type));
	TRY (read_data(f, H5_BLOCKNAME_Z, zdata, type));

	H5_RETURN (H5_SUCCESS);
}

