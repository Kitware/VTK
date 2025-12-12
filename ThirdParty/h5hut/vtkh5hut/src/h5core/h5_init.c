/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include <stdlib.h>
#include <stdarg.h>     /* va_arg - System dependent ?! */
#include <string.h>
#include <assert.h>

#include "config.h"

#include "private/h5_hdf5.h"

#include "private/h5_mpi.h"
#include "private/h5t_types.h"
#include "private/h5t_tags.h"

int h5_initialized = 0;
h5_dta_types_t h5_dta_types;            // type ids' for base & compound data types
int h5_myproc = 0;                      // proc id
const char* H5_VER_STRING = PACKAGE_VERSION;
/*
   create several HDF5 types
 */
static inline h5_err_t
create_array_types (
        void
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");

	hsize_t dims[1] = { 3 };
	TRY(
	        h5_dta_types.h5_coord3d_t = hdf5_create_array_type (
	                H5_FLOAT64,
	                1,
	                dims)
	        );
	TRY(
	        h5_dta_types.h5_3glb_idx_t = hdf5_create_array_type (
	                H5_ID,
	                1,
	                dims)
	        );
	dims[0] = 4;
	TRY(
	        h5_dta_types.h5_4glb_idx_t = hdf5_create_array_type (
	                H5_ID,
	                1,
	                dims)
	        );
	TRY(
	        h5_dta_types.h5_4chk_idx_t = hdf5_create_array_type (
	                H5_INT32,
	                1,
	                dims)
	        );
	dims[0] = 6;
	TRY(
	        h5_dta_types.h5_coord6d_t = hdf5_create_array_type (
	                H5_FLOAT64,
	                1,
	                dims)
	        );
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
close_array_types (
	void
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	TRY (hdf5_close_type (h5_dta_types.h5_coord3d_t));
	TRY (hdf5_close_type (h5_dta_types.h5_3glb_idx_t));
	TRY (hdf5_close_type (h5_dta_types.h5_4glb_idx_t));
	TRY (hdf5_close_type (h5_dta_types.h5_4chk_idx_t));
	TRY (hdf5_close_type (h5_dta_types.h5_coord6d_t));
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
create_vertex_type (
        void
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");

	TRY(
	        h5_dta_types.h5_vertex_t = hdf5_create_type (
	                H5T_COMPOUND,
	                sizeof (h5_glb_vertex_t)) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_vertex_t,
	                "idx",
	                HOFFSET (h5_glb_vertex_t, idx),
	                H5_ID) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_vertex_t,
	                "P",
	                HOFFSET (h5_glb_vertex_t, P),
	                h5_dta_types.h5_coord3d_t) );

	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
close_vertex_type (
	void
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	TRY (hdf5_close_type (h5_dta_types.h5_vertex_t));
	H5_RETURN (H5_SUCCESS);
}


static inline h5_err_t
create_triangle_type (
        void
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");

	TRY(
	        h5_dta_types.h5_triangle_t = hdf5_create_type (
	                H5T_COMPOUND,
	                sizeof (h5_glb_tri_t)) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_triangle_t,
	                "idx",
	                HOFFSET (h5_glb_tri_t, idx),
	                H5_ID) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_triangle_t,
	                "parent_idx",
	                HOFFSET (h5_glb_tri_t, parent_idx),
	                H5_ID) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_triangle_t,
	                "child_idx",
	                HOFFSET(h5_glb_tri_t, child_idx),
	                H5_ID) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_triangle_t,
	                "level_idx",
	                HOFFSET(h5_glb_tri_t, level_idx),
	                H5_INT16) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_triangle_t,
	                "refinement",
	                HOFFSET(h5_glb_tri_t, refinement),
	                H5_INT16) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_triangle_t,
	                "flags",
	                HOFFSET(h5_glb_tri_t, flags),
	                H5_INT32) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_triangle_t,
	                "vertex_indices",
	                HOFFSET (h5_glb_tri_t, vertex_indices),
	                h5_dta_types.h5_3glb_idx_t) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_triangle_t,
	                "neighbor_indices",
	                HOFFSET(h5_glb_tri_t, neighbor_indices),
	                h5_dta_types.h5_3glb_idx_t) );

	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
close_triangle_type (
	void
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	TRY (hdf5_close_type (h5_dta_types.h5_triangle_t));
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
create_tag_type (
        void
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");

	TRY (
	        h5_dta_types.h5t_glb_tag_idx_t = hdf5_create_type (
	                H5T_COMPOUND,
	                sizeof (h5t_glb_tag_idx_t)) );
	TRY (
	        hdf5_insert_type (
	                h5_dta_types.h5t_glb_tag_idx_t,
	                "eid",
	                HOFFSET (h5t_glb_tag_idx_t, eid),
	                H5_ID) );
	TRY (
	        hdf5_insert_type (
	                h5_dta_types.h5t_glb_tag_idx_t,
	                "idx",
	                HOFFSET (h5t_glb_tag_idx_t, idx),
	                H5_ID) );

	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
close_tag_type (
	void
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	TRY (hdf5_close_type (h5_dta_types.h5t_glb_tag_idx_t));
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
create_tet_type (
        void
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");

	TRY(
	        h5_dta_types.h5_tet_t = hdf5_create_type (
	                H5T_COMPOUND,
	                sizeof (h5_glb_tet_t)) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_tet_t,
	                "idx",
	                HOFFSET (h5_glb_tet_t, idx),
	                H5_ID) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_tet_t,
	                "parent_idx",
	                HOFFSET (h5_glb_tet_t, parent_idx),
	                H5_ID) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_tet_t,
	                "child_idx",
	                HOFFSET (h5_glb_tet_t, child_idx),
	                H5_INT32) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_tet_t,
	                "level_idx",
	                HOFFSET(h5_glb_tet_t, level_idx),
	                H5_INT16) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_tet_t,
	                "refinement",
	                HOFFSET(h5_glb_tet_t, refinement),
	                H5_INT16) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_tet_t,
	                "flags",
	                HOFFSET(h5_glb_tet_t, flags),
	                H5_INT32) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_tet_t,
	                "vertex_indices",
	                HOFFSET (h5_glb_tet_t, vertex_indices),
	                h5_dta_types.h5_4glb_idx_t) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_tet_t,
	                "neighbor_indices",
	                HOFFSET (h5_glb_tet_t, neighbor_indices),
	                h5_dta_types.h5_4glb_idx_t) );

	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
close_tet_type (
	void
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	TRY (hdf5_close_type (h5_dta_types.h5_tet_t));
	H5_RETURN (H5_SUCCESS);
}

#if defined(WITH_PARALLEL_H5GRID)

static inline h5_err_t
create_chunk_type (
	void
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	TRY(
	        h5_dta_types.h5_chunk_t = hdf5_create_type (
	                H5T_COMPOUND,
	                sizeof (h5t_chunk_t)) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_chunk_t,
	                "idx",
	                HOFFSET (h5t_chunk_t, idx),
	                H5_INT32) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_chunk_t,
	                "oct_idx",
	                HOFFSET (h5t_chunk_t, oct_idx),
	                H5_INT32) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_chunk_t,
	                "elem",
	                HOFFSET (h5t_chunk_t, elem),
	                H5_ID) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_chunk_t,
	                "weight",
	                HOFFSET (h5t_chunk_t, weight),
	                H5_ID) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_chunk_t,
	                "num_elems",
	                HOFFSET (h5t_chunk_t, num_elems),
	                H5_UINT16) );
//	TRY(
//	        hdf5_insert_type (
//	                h5_dta_types.h5_chunk_t,
//	                "vtx",
//	                HOFFSET (h5t_chunk_t, vtx),
//	                H5_INT64) );
//	TRY(
//	        hdf5_insert_type (
//	                h5_dta_types.h5_chunk_t,
//	                "num_vtx",
//	                HOFFSET (h5t_chunk_t, num_vtx),
//	                H5_INT64) );
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
close_chunk_type (
	void
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	TRY (hdf5_close_type (h5_dta_types.h5_chunk_t));
	H5_RETURN (H5_SUCCESS);
}


static inline h5_err_t
create_octree_type (
		void
		) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	TRY(
	        h5_dta_types.h5_octree_t = hdf5_create_type (
	                H5T_COMPOUND,
	                sizeof (h5t_octant_t)) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_octree_t,
	                "idx",
	                HOFFSET (h5t_octant_t, idx),
	                H5_INT32) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_octree_t,
	                "parent_idx",
	                HOFFSET (h5t_octant_t, parent_idx),
	                H5_INT32) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_octree_t,
	                "child_idx",
	                HOFFSET (h5t_octant_t, child_idx),
	                H5_INT32) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_octree_t,
	                "level_idx",
	                HOFFSET (h5t_octant_t, level_idx),
	                H5_INT16) );
//	TRY(
//	        hdf5_insert_type (
//	                h5_dta_types.h5_octree_t,
//	                "bounding_box",
//	                HOFFSET (h5t_octant_t, bounding_box),
//	                h5_dta_types.h5_coord6d_t) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_octree_t,
	                "userlevels",
	                HOFFSET (h5t_octant_t, userlevels),
	                H5_INT32) );

	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
close_octree_type (
	void
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	TRY (hdf5_close_type (h5_dta_types.h5_octree_t));
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
create_userdata_type (
		void
		) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	TRY(
		        h5_dta_types.h5_userdata_t = hdf5_create_type (
		                H5T_COMPOUND,
		                sizeof (h5t_oct_userdata_t)) );
	TRY(
	        hdf5_insert_type (
	                h5_dta_types.h5_userdata_t,
	                "idx",
	                HOFFSET (h5t_oct_userdata_t, idx),
	                h5_dta_types.h5_4chk_idx_t) );

	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
close_userdata_type (
	void
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	TRY (hdf5_close_type (h5_dta_types.h5_userdata_t));
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
create_mpi_type_glb_tet (
        void
        ) {
	h5_glb_tet_t glb_elem;
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	int i = 0;
	const int count = 8;
	int blocklens[count];
	MPI_Aint indices[count];
	MPI_Datatype old_types[count];
	MPI_Aint base;
	MPI_Aint addr;
	TRY (mpi_get_address (&glb_elem, &base));

	// idx
	blocklens[i] = 1;
	TRY (mpi_get_address (&glb_elem.idx, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_LONG_LONG;

	// parent_idx
	blocklens[i] = 1;
	TRY (mpi_get_address (&glb_elem.parent_idx, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_LONG_LONG;

	// child_idx
	blocklens[i] = 1;
	TRY (mpi_get_address (&glb_elem.child_idx, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_LONG_LONG;

	// level_idx
	blocklens[i] = 1;
	TRY (mpi_get_address (&glb_elem.level_idx, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_SHORT;

	// refinement
	blocklens[i] = 1;
	TRY (mpi_get_address (&glb_elem.refinement, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_SHORT;

	// flags
	blocklens[i] = 1;
	TRY (mpi_get_address (&glb_elem.flags, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_UNSIGNED;

	// vertex_indices
	blocklens[i] = 4;
	TRY (mpi_get_address (&glb_elem.vertex_indices, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_LONG_LONG;

	// neighbor_indices
	blocklens[i] = 4;
	TRY (mpi_get_address (&glb_elem.neighbor_indices, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_LONG_LONG;

	// create new type
	assert (i==count);
	TRY (mpi_create_type_struct (count, blocklens, indices, old_types,
	                             &h5_dta_types.mpi_glb_tet));
	// commit new type
	TRY (h5priv_mpi_type_commit (&h5_dta_types.mpi_glb_tet));

	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
create_mpi_type_glb_tri (
        void
        ) {
	h5_glb_tri_t glb_elem;
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	int i = 0;
	const int count = 8;
	int blocklens[count];
	MPI_Aint indices[count];
	MPI_Datatype old_types[count];
	MPI_Aint base;
	MPI_Aint addr;
	TRY (mpi_get_address (&glb_elem, &base));

	// idx
	blocklens[i] = 1;
	TRY (mpi_get_address (&glb_elem.idx, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_LONG_LONG;

	// parent_idx
	blocklens[i] = 1;
	TRY (mpi_get_address (&glb_elem.parent_idx, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_LONG_LONG;

	// child_idx
	blocklens[i] = 1;
	TRY (mpi_get_address (&glb_elem.child_idx, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_LONG_LONG;

	// level_idx
	blocklens[i] = 1;
	TRY (mpi_get_address (&glb_elem.level_idx, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_SHORT;

	// refinement
	blocklens[i] = 1;
	TRY (mpi_get_address (&glb_elem.refinement, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_SHORT;

	// flags
	blocklens[i] = 1;
	TRY (mpi_get_address (&glb_elem.flags, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_UNSIGNED;

	// vertex_indices
	blocklens[i] = 3;
	TRY (mpi_get_address (&glb_elem.vertex_indices, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_LONG_LONG;

	// neighbor_indices
	blocklens[i] = 3;
	TRY (mpi_get_address (&glb_elem.neighbor_indices, &addr));
	indices[i] = addr - base;
	old_types[i++] =  MPI_LONG_LONG;

	// create new type
	assert (i==count);
	TRY (mpi_create_type_struct (count, blocklens, indices, old_types,
	                             &h5_dta_types.mpi_glb_triangle));
	// commit new type
	TRY (h5priv_mpi_type_commit (&h5_dta_types.mpi_glb_triangle));

	H5_RETURN (H5_SUCCESS);
}
static inline h5_err_t
create_mpi_type_glb_vtx (
        void
        ) {
	h5_glb_vertex_t glb_vtx;
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	int i = 0;
	const int count = 2;
	int blocklens[count];
	MPI_Aint indices[count];
	MPI_Datatype old_types[count];
	MPI_Aint base;
	MPI_Aint addr;
	TRY (mpi_get_address (&glb_vtx, &base));

	// idx
	blocklens[i] = 1;
	TRY (mpi_get_address (&glb_vtx.idx, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_LONG_LONG;

	// P
	blocklens[i] = 3;
	TRY (mpi_get_address (&glb_vtx.P, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_DOUBLE;

	// create new type
	assert (i==count);
	TRY (mpi_create_type_struct (count, blocklens, indices, old_types,
	                             &h5_dta_types.mpi_glb_vtx));
	// commit new type
	TRY (h5priv_mpi_type_commit (&h5_dta_types.mpi_glb_vtx));

	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
create_mpi_type_edge_list_elem (
        void
        ) {
	h5t_edge_list_elem_t elem;
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	int i = 0;
	const int count = 4;
	int blocklens[count];
	MPI_Aint indices[count];
	MPI_Datatype old_types[count];
	MPI_Aint base;
	MPI_Aint addr;
	TRY (mpi_get_address (&elem, &base));

	// vtx1
	blocklens[i] = 1;
	TRY (mpi_get_address (&elem.vtx1, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_LONG_LONG;

	// vtx2
	blocklens[i] = 1;
	TRY (mpi_get_address (&elem.vtx2, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_LONG_LONG;

	// new_vtx
	blocklens[i] = 1;
	TRY (mpi_get_address (&elem.new_vtx, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_LONG_LONG;

	// proc
	blocklens[i] = 1;
	TRY (mpi_get_address (&elem.proc, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_INT;

	// create new type
	assert (i==count);
	TRY (mpi_create_type_struct (count, blocklens, indices, old_types,
	                             &h5_dta_types.mpi_edge_list_elem));
	// commit new type
	TRY (h5priv_mpi_type_commit (&h5_dta_types.mpi_edge_list_elem));

	H5_RETURN (H5_SUCCESS);
}
static inline h5_err_t
create_mpi_type_chunk (
        void
        ) {
	h5t_chunk_t chunk;
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	int i = 0;
	const int count = 5;
	int blocklens[count];
	MPI_Aint indices[count];
	MPI_Datatype old_types[count];
	MPI_Aint base;
	MPI_Aint addr;
	TRY (mpi_get_address (&chunk, &base));

	// idx
	blocklens[i] = 1;
	TRY (mpi_get_address (&chunk.idx, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_INT;

	// oct_idx
	blocklens[i] = 1;
	TRY (mpi_get_address (&chunk.oct_idx, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_INT;

	// elem_idx
	blocklens[i] = 1;
	TRY (mpi_get_address (&chunk.elem, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_LONG_LONG;

	// chk_weight
	blocklens[i] = 1;
	TRY (mpi_get_address (&chunk.weight, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_LONG_LONG;

	// num_elems
	blocklens[i] = 1;
	TRY (mpi_get_address (&chunk.num_elems, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_SHORT;


	// create new type
	assert (i == count);
	TRY (mpi_create_type_struct (count, blocklens, indices, old_types,
	                             &h5_dta_types.mpi_chunk));
	// commit new type
	TRY (h5priv_mpi_type_commit (&h5_dta_types.mpi_chunk));

	H5_RETURN (H5_SUCCESS);
}
#endif

h5_err_t
_h5_exit (int status) {
	exit (status);
	return H5_ERR;
}

h5_err_t
h5_initialize (
        void
        ) {
	if (h5_initialized) return 0;
	memset (&h5_call_stack, 0, sizeof (h5_call_stack));
	// must be set here, otherwise next statement will fail!
	h5_initialized = 1;
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	ret_value = H5_SUCCESS;
#ifdef H5_HAVE_PARALLEL
	int mpi_is_initialized;
	MPI_Initialized (&mpi_is_initialized);
	if (!mpi_is_initialized) {
		MPI_Init (NULL, NULL);
	}
	if (h5priv_mpi_comm_rank (MPI_COMM_WORLD, &h5_myproc) < 0) {
		exit (42);
	}
#endif
	h5_dta_types.h5_glb_idx_t = H5_INT64;
	h5_dta_types.h5_int64_t = H5_INT64;
	h5_dta_types.h5_float64_t = H5_FLOAT64;
	h5_dta_types.h5_int32_t = H5_INT32;

	TRY (create_array_types ());
	TRY (create_vertex_type ());
	TRY (create_triangle_type ());
	TRY (create_tet_type ());
	TRY (create_tag_type ());

#if defined(WITH_PARALLEL_H5GRID)
	TRY (create_chunk_type ());
	TRY (create_octree_type ());
	TRY (create_userdata_type ());

	TRY (create_mpi_type_glb_tri ());
	TRY (create_mpi_type_glb_tet ());
	TRY (create_mpi_type_glb_vtx ());
	TRY (create_mpi_type_chunk ());
	TRY (create_mpi_type_edge_list_elem());
#endif
	H5_RETURN ((ret_value != H5_SUCCESS) ? _h5_exit (42) : H5_SUCCESS);
}

h5_err_t
h5_finalize (
	void
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	TRY (close_array_types ());
	TRY (close_vertex_type ());
	TRY (close_triangle_type ());
	TRY (close_tag_type ());
	TRY (close_tet_type ());
#if defined(WITH_PARALLEL_H5GRID)
	TRY (close_chunk_type ());
	TRY (close_octree_type ());
	TRY (close_userdata_type ());
#endif
	H5_RETURN (H5_SUCCESS);
}
