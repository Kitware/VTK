/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5_TYPES_H
#define __PRIVATE_H5_TYPES_H

#include <vtk_hdf5.h>
#include "h5core/h5_types.h"

#define H5_INT8			H5T_NATIVE_INT8
#define H5_UINT8		H5T_NATIVE_UINT8
#define H5_INT16                H5T_NATIVE_INT16
#define H5_UINT16		H5T_NATIVE_UINT16
#define H5_INT32                H5T_NATIVE_INT32
#define H5_UINT32               H5T_NATIVE_UINT32
#define H5_INT64                H5T_NATIVE_INT64
#define H5_UINT64               H5T_NATIVE_UINT64
#define H5_FLOAT32              H5T_NATIVE_FLOAT
#define H5_FLOAT64              H5T_NATIVE_DOUBLE
#define H5_ID                   H5T_NATIVE_INT64
#define H5_STRING		H5T_NATIVE_CHAR

struct h5_prop {                        // generic property class
        h5_int64_t class;               // property class
        char pad[248];                  // sizeof (struct h5_prop) == 256
};

struct h5_prop_file {                   // file property
        h5_int64_t class;               // property class == H5_PROP_FILE
        h5_int64_t flags;               // file access mode (read-write, readonly ...
        h5_int64_t align;               // HDF5 alignment
	h5_int64_t increment;		// increment for core vfd
        h5_int64_t throttle;
#ifdef H5_HAVE_PARALLEL
        MPI_Comm comm;
#endif
	hid_t	xfer_prop;		// dataset transfer properties
	hid_t	access_prop;		// file access properties
	hid_t	create_prop;		// file create properties
	char*	prefix_iteration_name;	// Prefix of step name
	int	width_iteration_idx;	// pad iteration index with 0 up to this
	int     flush;                  // flush iteration after writing dataset
};
typedef struct h5_prop_file h5_prop_file_t;
typedef h5_prop_file_t* h5_prop_file_p;

/**
   \struct h5_file

   This is an essentially opaque datastructure that
   acts as a filehandle and is defined as type \c h5_file_t.
   It is created by \ref H5OpenFile and destroyed by
   \ref H5CloseFile.
*/
struct h5_file {
	hid_t		file;		// HDF5 file id
        h5_prop_file_t* props;          // file properties
	char		empty;          // flag (should be int?!)

	/* MPI */
	int             nprocs;		// number of processors
	int             myproc;		// index of my processor

	/* HDF5 */
	hid_t           root_gid;	// HDF5 group id of root
	hid_t           iteration_gid;	// HDF5 group id of current iteration

	/* iteration internal data					*/
	char*           iteration_name; // full current iteration name
	h5_int64_t      iteration_idx;	// current iteration index
	int             is_new_iteration;// :FIXME: ?

	struct h5u_fdata *u;            // pointer to unstructured data
	struct h5b_fdata *b;            // pointer to block data
};

struct h5_idxmap_el {
	h5_glb_idx_t	glb_idx;
	h5_loc_idx_t	loc_idx;
};
typedef struct h5_idxmap_el h5_idxmap_el_t;

struct h5_idxmap {
	h5_size_t	size;		/* allocated space in number of items */
	h5_size_t	num_items;	/* stored items	*/
	h5_idxmap_el_t*  items;
};

typedef struct {
	size_t size;
	size_t num_items;
	char* items[1];
} h5_strlist_t;

#endif
