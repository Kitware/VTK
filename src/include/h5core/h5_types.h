/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5CORE_H5_TYPES_H
#define __H5CORE_H5_TYPES_H

#include <stdarg.h>
#include <stdint.h>

#include <vtk_hdf5.h>
#include "vtk_h5hut_mangle.h"
#include "vtkh5hut_export.h"

#ifndef H5_HAVE_PARALLEL
/*
  If someone want's to use serial H5hut with MPI, he must include 
  the header file "mpi.h" before including H5hut's header file. According
  to the MPI standard the macro MPI_VERSION must be defined after reading
  "mpi.h".
 */
#ifndef MPI_VERSION
typedef int MPI_Comm;
typedef int MPI_Datatype;
#endif
#endif

typedef enum  {
	H5_STRING_T,
	H5_INT8_T,
	H5_UINT8_T,
	H5_INT16_T,
	H5_UINT16_T,
	H5_INT32_T,
	H5_UINT32_T,
	H5_INT64_T,
	H5_UINT64_T,
	H5_FLOAT32_T,
	H5_FLOAT64_T,
	H5_ID_T,
} h5_types_t;

/*
   file modes:

   H5_O_RDONLY:
	read data from existing file

   H5_O_WRONLY:
	create new file, if file not exists; write new or overwrite
        existing data

   H5_O_APPENDONLY:
	allows to append new data to an existing file

   H5_O_RDWR:
	create new file, if file not exists; read and (over-)write data
 */
typedef enum {
	H5_O_RDWR =	    0x01,
	H5_O_RDONLY =	    0x02,
	H5_O_WRONLY =	    0x04,
	H5_O_APPENDONLY =   0x08
} h5_file_modes_t;

#ifdef   _WIN32
typedef __int64                 int64_t;
#endif /* _WIN32 */

typedef int64_t                 h5_int64_t;
typedef int32_t                 h5_int32_t;
typedef uint64_t                h5_uint64_t;
typedef uint32_t                h5_uint32_t;
typedef int64_t                 h5_id_t;
typedef int16_t                 h5_lvl_idx_t;
typedef int64_t                 h5_glb_idx_t;   // type for a global index
typedef int64_t                 h5_glb_id_t;    // type for a global ID

#if defined(USE_LARGE_INDICES)
typedef int64_t                 h5_loc_idx_t;   // type for a local index
typedef int64_t                 h5_loc_id_t;    // type for a local ID
#else
typedef int32_t                 h5_loc_idx_t;   // type for a local index
typedef int32_t                 h5_loc_id_t;    // type for a local ID
#endif

typedef int32_t                 h5_chk_idx_t;	// type for a chunk index

typedef uint64_t                h5_size_t;      // size in number of elements
typedef int64_t                 h5_ssize_t;     // size in number of elements */
typedef int64_t                 h5_err_t;
typedef int64_t                 h5_chk_weight_t;// type for a chunk weight
typedef uint16_t                h5_chk_size_t;	// type for number of elements in chunk
typedef int32_t                 h5_weight_t;	// type for weights
typedef double                  h5_time_t;	// type for storing a time

typedef char*                   char_p;
typedef void*                   void_p;
typedef double                  h5_float64_t;
typedef float                   h5_float32_t;

typedef struct h5_complex {
	h5_float64_t r,i;
} h5_complex_t;

typedef h5_float64_t            h5_coord3d_t[3];

struct h5_prop;
typedef struct h5_prop* h5_prop_p;
typedef uintptr_t h5_prop_t;

struct h5_file;
typedef struct h5_file* h5_file_p;
typedef uintptr_t h5_file_t;

struct h5t_mesh;
typedef struct h5t_mesh h5t_mesh_t;
typedef h5t_mesh_t* h5t_mesh_p;

typedef h5_err_t (*h5_errorhandler_t)(
        const char*,
        va_list ap );

typedef struct h5_loc_idlist {
	int32_t size;                   /* allocated space in number of items */
	int32_t num_items;              /* stored items	*/
	int32_t flags;
	h5_loc_id_t items[1];
} h5_loc_idlist_t;

typedef struct h5_glb_idlist {
	int32_t size;                   /* allocated space in number of items */
	int32_t num_items;              /* stored items	*/
	h5_glb_id_t items[1];
} h5_glb_idlist_t;

typedef struct h5_loc_idxlist {
	int32_t size;                   /* allocated space in number of items */
	int32_t num_items;              /* stored items	*/
	h5_loc_idx_t items[1];
} h5_loc_idxlist_t;

typedef struct h5_glb_idxlist {
	int32_t size;                   /* allocated space in number of items */
	int32_t num_items;              /* stored items	*/
	h5_glb_idx_t items[1];
} h5_glb_idxlist_t;

enum h5_iterators {
	iteration_iterator
};

struct h5_iterator;
typedef struct {
	enum h5_iterators it_type;
	h5_file_t file;
	h5_int64_t (*iter)(struct h5_iterator*);
} h5_iterator_t;

struct h5_idxmap;
typedef struct h5_idxmap h5_idxmap_t;

#define H5_PROP_DEFAULT (0)
#define H5_PROP_FILE    (1)

#endif
