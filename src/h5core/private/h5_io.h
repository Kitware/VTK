/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5_IO_H
#define __PRIVATE_H5_IO_H

#include "h5core/h5_types.h"

/*
   information about HDF5 dataset
 */
typedef struct h5_dataset_info {
	char name[256];
	int rank;
	hsize_t dims[4];
	hsize_t max_dims[4];
	hsize_t chunk_dims[4];
	hid_t type_id;
	hid_t create_prop;
	hid_t access_prop;
} h5_dsinfo_t;

h5_err_t
h5priv_write_dataset_by_name (
		h5t_mesh_t* const m,
	 	const h5_file_p f,
        hid_t loc_id,
        h5_dsinfo_t* ds_info,
        hid_t (*set_memspace)(h5t_mesh_t*,hid_t),
        hid_t (*set_diskspace)(h5t_mesh_t*,hid_t),
        const void* const data
        );

h5_err_t
h5priv_write_dataset_by_name_id (
	 	const h5_file_p f,
        const hid_t loc_id,
        h5_dsinfo_t* dsinfo,
        hid_t dset_id,
        hid_t memspace_id,
        hid_t diskspace_id,
        const void* const data
        );

h5_err_t
h5priv_read_dataset (
	const h5_file_p,
	const hid_t, h5_dsinfo_t*,
        hid_t (*)(h5t_mesh_t* const, const hid_t),
        hid_t (*)(h5t_mesh_t* const, const hid_t),
	void* const);

h5_err_t
h5priv_normalize_dataset_name (
	char* const name
	);

#endif
