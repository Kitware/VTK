/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/
#include "private/h5t_types.h"

#include "private/h5_file.h"
#include "private/h5_hdf5.h"
#include "private/h5_mpi.h"
#include "private/h5_va_macros.h"

#include "private/h5t_model.h"
#include "private/h5t_access.h"
#include "private/h5t_adjacencies.h"
#include "private/h5t_io.h"
#include "private/h5t_retrieve.h"
#include "private/h5t_store.h"

#include "private/h5t_core.h"

static struct h5t_methods tri_funcs = {
	&h5tpriv_read_trim_methods,
	&h5tpriv_trim_store_methods,
	&h5tpriv_trim_retrieve_methods,
	&h5tpriv_access_trim_methods,
	&h5tpriv_trim_adjacency_methods,
	&h5tpriv_trim_core_methods
};

/*
   open tetrahedral mesh
 */
h5_err_t
h5t_open_triangle_mesh_by_idx (
	const h5_file_t fh,
	const h5_id_t idx,
	h5t_mesh_t** mesh
	) {
        h5_file_p f = (h5_file_p)fh;
	H5_CORE_API_ENTER (h5_err_t, "f=%p, idx=%lld, mesh=%p", f, (long long)idx, mesh);
	hid_t ctn_hid;
	char name[1024];

	TRY (ctn_hid = h5priv_open_group_with_intermediates (
	             f->root_gid,
	             H5T_CONTAINER_GRPNAME,
	             TRIANGLE_MESHES_GRPNAME, NULL));
	TRY (hdf5_get_name_of_group_by_idx (ctn_hid, idx, name, sizeof (name)));
	TRY (hdf5_close_group (ctn_hid));

	H5_RETURN (h5t_open_triangle_mesh ((h5_file_t)f, name, mesh));
}

h5_err_t
h5t_open_triangle_mesh (
	const h5_file_t fh,
	const char* name,
	h5t_mesh_t** mesh
	) {
        h5_file_p f = (h5_file_p)fh;
	H5_CORE_API_ENTER (
                h5_err_t,
                "f=%p, name=%s, mesh=%p",
                f, name, mesh);
#ifdef WITH_PARALLEL_H5GRID
	MPI_Barrier (f->props->comm);
	double start = MPI_Wtime();
#endif
	hid_t mesh_hid;
	TRY (mesh_hid = h5priv_open_group_with_intermediates (
	             f->root_gid,
	             H5T_CONTAINER_GRPNAME,
	             TRIANGLE_MESHES_GRPNAME,
	             name, NULL));

	TRY (*mesh = h5_calloc (1, sizeof(**mesh)));
	h5t_mesh_t* m = *mesh;
	TRY (h5tpriv_init_mesh (
	             m,
	             f,
	             name,
	             mesh_hid,
	             h5_dta_types.h5_triangle_t,
	             &h5t_tri_ref_elem,
	             &tri_funcs,
	             0));
#ifdef WITH_PARALLEL_H5GRID
	MPI_Barrier (m->f->props->comm);
	m->timing.measure[m->timing.next_time++] = start;
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
#endif
	TRY (
                m->is_chunked && m->f->nprocs > 1 ?
                h5tpriv_read_chunked_mesh (m) : h5tpriv_read_mesh (m)
                );
	H5_RETURN (H5_SUCCESS);
}


h5_err_t
h5t_open_triangle_mesh_part (
        const h5_file_t fh,
        const char* name,
        h5t_mesh_t** mesh,
        h5_glb_idx_t* elem_indices,
        h5_glb_idx_t dim
        ) {
        h5_file_p f = (h5_file_p)fh;
	H5_CORE_API_ENTER (h5_err_t, "f=%p, name=%s, mesh=%p", f, name, mesh);
	hid_t mesh_hid;
#ifdef WITH_PARALLEL_H5GRID
	MPI_Barrier (f->props->comm);
	double start = MPI_Wtime();
#endif
	TRY (mesh_hid = h5priv_open_group_with_intermediates (
	             f->root_gid,
	             H5T_CONTAINER_GRPNAME,
	             TRIANGLE_MESHES_GRPNAME,
	             name, NULL));

	TRY (*mesh = h5_calloc (1, sizeof(**mesh)));
	h5t_mesh_t* m = *mesh;
	TRY (h5tpriv_init_mesh (
	             m,
	             f,
	             name,
	             mesh_hid,
	             h5_dta_types.h5_triangle_t,
	             &h5t_tri_ref_elem,
	             &tri_funcs,
	             0));
#ifdef WITH_PARALLEL_H5GRID  // reason: even if we have a chunked mesh, if h5hut is not parallel
	MPI_Barrier (m->f->props->comm);
	m->timing.measure[m->timing.next_time++] = start;
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
#endif

	TRY (h5tpriv_read_mesh_part (m, elem_indices, dim));

	H5_RETURN (H5_SUCCESS);
}

/*!
   Add new mesh

   \return mesh id
 */
h5_err_t
h5t_add_triangle_mesh (
	const h5_file_t fh,
	const char* name,
        const h5_weight_t num_weights,
	h5t_mesh_t** mesh
	) {
        h5_file_p f = (h5_file_p)fh;
	H5_CORE_API_ENTER (h5_err_t, "f=%p, name=%s, mesh=%p", f, name, mesh);
	CHECK_WRITABLE_MODE (f);
	h5_err_t exists;
	TRY (exists = h5priv_link_exists (
	             f->root_gid,
	             H5T_CONTAINER_GRPNAME,
	             TRIANGLE_MESHES_GRPNAME,
	             name));
	if (exists) {
		H5_RETURN_ERROR (
			H5_ERR,
			"Tetrahedral mesh '%s' already exists!",
			name);
	}
	hid_t mesh_hid;
	TRY (mesh_hid = h5priv_create_group_with_intermediates (
	             f->root_gid,
	             H5T_CONTAINER_GRPNAME,
	             TRIANGLE_MESHES_GRPNAME,
	             name, NULL));

	TRY (*mesh = h5_calloc (1, sizeof(**mesh)));
	h5t_mesh_t* m = *mesh;
	TRY (h5tpriv_init_mesh (
	             m,
	             f,
	             name,
	             mesh_hid,
	             h5_dta_types.h5_triangle_t,
	             &h5t_tri_ref_elem,
	             &tri_funcs,
	             1));

	m->leaf_level = 0;
	m->num_leaf_levels = 0;
	TRY (h5tpriv_add_level (m));
	m->mesh_changed = 1;

	H5_RETURN (H5_SUCCESS);
}
/*!
   Add new chunked mesh

   \return mesh id
 */
h5_err_t
h5t_add_chunked_triangle_mesh(
        const h5_file_t fh,
        const char* name,
        const h5_weight_t num_weights,
        h5t_mesh_t** mesh
        ) {
	H5_CORE_API_ENTER (h5_err_t, "f=%p, name=%s, mesh=%p", (void*)fh, name, mesh);
#ifdef WITH_PARALLEL_H5GRID
        h5_file_p f = (h5_file_p)fh;
	int size = -1;
	TRY (h5priv_mpi_comm_size (f->props->comm, &size));
	if (size != 1) {
		H5_RETURN_ERROR (
			H5_ERR,
			"Trying to create a chunked mesh with '%d' procs instead of 1!",
			size);
	}

	CHECK_WRITABLE_MODE (f);
	h5_err_t exists;
	TRY (exists = h5priv_link_exists (
	             f->root_gid,
	             H5T_CONTAINER_GRPNAME,
	             TRIANGLE_MESHES_GRPNAME,
	             name));
	if (exists) {
		H5_RETURN_ERROR (
			H5_ERR,
			"Triangle mesh '%s' already exists!",
			name);
	}
	hid_t mesh_hid;
	TRY (mesh_hid = h5priv_create_group_with_intermediates (
	             f->root_gid,
	             H5T_CONTAINER_GRPNAME,
	             TRIANGLE_MESHES_GRPNAME,
	             name, NULL));

	TRY (*mesh = h5_calloc (1, sizeof(**mesh)));
	h5t_mesh_t* m = *mesh;
	TRY (h5tpriv_init_mesh (
	             m,
	             f,
	             name,
	             mesh_hid,
	             h5_dta_types.h5_triangle_t,
	             &h5t_tri_ref_elem,
	             &tri_funcs,
	             1));
	m->is_chunked = 1;
	TRY (H5t_init_octree(&m->octree, sizeof (h5t_oct_userdata_t), NULL, -1, m->f->props->comm));
	m->leaf_level = 0;
	m->num_leaf_levels = 0;
	TRY (h5tpriv_add_level (m));
	m->mesh_changed = 1;
#endif
	H5_RETURN (H5_SUCCESS);
}
