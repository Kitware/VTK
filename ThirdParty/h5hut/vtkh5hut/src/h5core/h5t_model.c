/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include "h5core/h5_log.h"

#include "private/h5_file.h"
#include "private/h5_mpi.h"
#include "private/h5_err.h"

#include "private/h5_attribs.h"
#include "private/h5_hdf5.h"

#include "private/h5t_types.h"
#include "private/h5t_err.h"
#include "private/h5t_model.h"
#include "private/h5t_adjacencies.h"
#include "private/h5t_io.h"
#include "private/h5t_tags.h"

#include "h5core/h5_model.h"
#include "private/h5t_core.h"
#include "private/h5t_store.h"

#include <string.h>
#include <assert.h>


/*!
   \ingroup h5_private

   \internal

   Initialize topo internal structure. The structure has already be initialized
   with zero's.

   \return	H5_SUCCESS or error code
 */
h5_err_t
h5tpriv_init_mesh (
        h5t_mesh_t* const m,
        const h5_file_p f,
        const char* name,
        const hid_t mesh_hid,
        const hid_t elem_type,
        const h5t_ref_elem_t* ref_elem,
        h5t_methods_t* methods,
        const int create_mesh
        ) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p", m);

	m->f = f;
	strncpy (m->mesh_name, name, sizeof (m->mesh_name)-1);
	m->mesh_name[sizeof(m->mesh_name)-1] = '\0';
	m->mesh_gid = mesh_hid;
	m->ref_elem = ref_elem;
	m->methods = methods;

	m->mesh_changed = 0;
	m->num_leaf_levels = -1;
	m->leaf_level = 0;
	m->last_stored_vid = -1;
	m->last_stored_eid = -1;
	m->last_stored_vid_before_ref = -1;
	m->last_stored_eid_before_ref = -1;
	m->timing.num_timing = NUM_TIMING;
	m->timing.next_time = 0;
	m->timing.f = NULL;
	m->is_chunked = 0;
#if defined(WITH_PARALLEL_H5GRID)

	/* initialize pointers */
	m->octree = 					NULL;
	m->chunks = 					NULL;
#endif
	m->loc_elems =                  NULL;
	m->num_interior_elems =         NULL;
	m->num_glb_elems =              NULL;
	m->num_interior_leaf_elems =    NULL;
	m->num_glb_leaf_elems =         NULL;
	m->map_elem_g2l.items =         NULL;
	m->vertices =                   NULL;
	m->num_loc_vertices =           NULL;
	m->map_vertex_g2l.items =       NULL;
	m->num_b_vtx = 					NULL;
	m->first_b_vtx = 				NULL;
//	m->weights = 					NULL;

#if defined(WITH_PARALLEL_H5GRID)
	/* chunks */
	strcpy (m->dsinfo_chunks.name, "Chunks");
	m->dsinfo_chunks.rank = 1;
	m->dsinfo_chunks.dims[0] = 0;
	m->dsinfo_chunks.max_dims[0] = H5S_UNLIMITED;
	m->dsinfo_chunks.chunk_dims[0] = 4096;
	m->dsinfo_chunks.type_id = h5_dta_types.h5_chunk_t;
	TRY (m->dsinfo_chunks.create_prop = hdf5_create_property (
	             H5P_DATASET_CREATE) );
	TRY (hdf5_set_chunk_property (
	             m->dsinfo_chunks.create_prop,
	             m->dsinfo_chunks.rank,
	             m->dsinfo_chunks.chunk_dims) );
	m->dsinfo_chunks.access_prop = H5P_DEFAULT;

	/* octree */
	strcpy (m->dsinfo_octree.name, "Octants");
	m->dsinfo_octree.rank = 1;
	m->dsinfo_octree.dims[0] = 0;
	m->dsinfo_octree.max_dims[0] = H5S_UNLIMITED;
	m->dsinfo_octree.chunk_dims[0] = 4096;
	m->dsinfo_octree.type_id = h5_dta_types.h5_octree_t;
	TRY (m->dsinfo_octree.create_prop = hdf5_create_property (
	             H5P_DATASET_CREATE) );
	TRY (hdf5_set_chunk_property (
	             m->dsinfo_octree.create_prop,
	             m->dsinfo_octree.rank,
	             m->dsinfo_octree.chunk_dims) );
	m->dsinfo_octree.access_prop = H5P_DEFAULT;

	/* octree userdata */
	strcpy (m->dsinfo_userdata.name, "Oct_userdata");
	m->dsinfo_userdata.rank = 1;
	m->dsinfo_userdata.dims[0] = 0;
	m->dsinfo_userdata.max_dims[0] = H5S_UNLIMITED;
	m->dsinfo_userdata.chunk_dims[0] = 4096;
	m->dsinfo_userdata.type_id = h5_dta_types.h5_userdata_t;
	TRY (m->dsinfo_userdata.create_prop = hdf5_create_property (
	             H5P_DATASET_CREATE) );
	TRY (hdf5_set_chunk_property (
	             m->dsinfo_userdata.create_prop,
	             m->dsinfo_userdata.rank,
	             m->dsinfo_userdata.chunk_dims) );
	m->dsinfo_userdata.access_prop = H5P_DEFAULT;
#endif
	
	/* vertices */
	strcpy (m->dsinfo_vertices.name, "Vertices");
	m->dsinfo_vertices.rank = 1;
	m->dsinfo_vertices.dims[0] = 0;
	m->dsinfo_vertices.max_dims[0] = H5S_UNLIMITED;
	m->dsinfo_vertices.chunk_dims[0] = 4096;
	m->dsinfo_vertices.type_id = h5_dta_types.h5_vertex_t;
	TRY (m->dsinfo_vertices.create_prop = hdf5_create_property (
	             H5P_DATASET_CREATE) );
	TRY (hdf5_set_chunk_property (
	             m->dsinfo_vertices.create_prop,
	             m->dsinfo_vertices.rank,
	             m->dsinfo_vertices.chunk_dims) );
	m->dsinfo_vertices.access_prop = H5P_DEFAULT;

	/* Elems */
	strcpy (m->dsinfo_elems.name, "Elems");
	m->dsinfo_elems.rank = 1;
	m->dsinfo_elems.dims[0] = 0;
	m->dsinfo_elems.max_dims[0] = H5S_UNLIMITED;
	m->dsinfo_elems.chunk_dims[0] = 4096;
	m->dsinfo_elems.type_id = elem_type;
	TRY (m->dsinfo_elems.create_prop = hdf5_create_property (
	             H5P_DATASET_CREATE) );
	TRY (hdf5_set_chunk_property (
	             m->dsinfo_elems.create_prop,
	             m->dsinfo_elems.rank,
	             m->dsinfo_elems.chunk_dims) );
	m->dsinfo_elems.access_prop = H5P_DEFAULT;

	/* Weights */
	strcpy (m->dsinfo_weights.name, "weights");
	m->dsinfo_weights.rank = 1;
//	m->dsinfo_weights.rank = 2;
	m->dsinfo_weights.dims[0] = 0;
//	m->dsinfo_weights.dims[1] = 0;
	m->dsinfo_weights.max_dims[0] = H5S_UNLIMITED;
//	m->dsinfo_weights.max_dims[1] = H5S_UNLIMITED;
	m->dsinfo_weights.chunk_dims[0] = 4096;
//	m->dsinfo_weights.chunk_dims[0] = 4096;
	m->dsinfo_weights.type_id = h5_dta_types.h5_int32_t;
	TRY (m->dsinfo_weights.create_prop = hdf5_create_property (
	             H5P_DATASET_CREATE) );
	TRY (hdf5_set_chunk_property (
	             m->dsinfo_weights.create_prop,
	             m->dsinfo_weights.rank,
	             m->dsinfo_weights.chunk_dims) );
	m->dsinfo_weights.access_prop = H5P_DEFAULT;

	if (create_mesh) {
		m->num_leaf_levels = 0;
	} else {
		TRY (h5priv_read_attrib (
		             m->mesh_gid,
		             "__num_leaf_levels__",
		             H5_INT16_T,
		             &m->num_leaf_levels));
                //seems not to be set otherwise but for reading vtx it should be...
		m->leaf_level = m->num_leaf_levels-1;
		TRY (m->num_glb_elems = h5_calloc (m->num_leaf_levels, sizeof(*m->num_glb_elems)));
		TRY (m->num_glb_leaf_elems = h5_calloc (m->num_leaf_levels, sizeof(*m->num_glb_leaf_elems)));
		TRY (m->num_glb_vertices = h5_calloc (m->num_leaf_levels, sizeof (*m->num_glb_vertices)));

		TRY (m->num_b_vtx = h5_alloc (m->num_b_vtx, m->num_leaf_levels*sizeof (*m->num_b_vtx)));
		TRY (m->first_b_vtx = h5_alloc (m->first_b_vtx, m->num_leaf_levels*sizeof (*m->first_b_vtx)));

		TRY (m->num_interior_elems = h5_calloc (m->num_leaf_levels, sizeof(*m->num_interior_elems)));
		TRY (m->num_interior_leaf_elems = h5_calloc (m->num_leaf_levels, sizeof(*m->num_interior_leaf_elems)));
		TRY (m->num_ghost_elems = h5_calloc (m->num_leaf_levels, sizeof(*m->num_ghost_elems)));
		TRY (m->num_loc_vertices = h5_calloc (m->num_leaf_levels, sizeof (*m->num_loc_vertices)));

		TRY (h5priv_read_attrib (
		             m->mesh_gid,
		             "__num_elems__",
		             H5_INT64_T,
		             m->num_glb_elems));

		TRY (h5priv_read_attrib (
		             m->mesh_gid,
		             "__num_leaf_elems__",
		             H5_INT64_T,
		             m->num_glb_leaf_elems));

		TRY (h5priv_read_attrib (
		             m->mesh_gid,
		             "__num_vertices__",
		             H5_INT64_T,
		             m->num_glb_vertices));
		// if the file version is lower the following attributes are missing:
		hid_t exists;
		TRY (exists = hdf5_attribute_exists (
		        	m->mesh_gid,
		        	"__is_chunked__"));
		if (exists) {

			TRY (h5priv_read_attrib (
		             m->mesh_gid,
		             "__num_b_vertices__",
		             H5_INT64_T,
		             m->num_b_vtx));
			TRY (h5priv_read_attrib (
		             m->mesh_gid,
		             "__first_b_vertices__",
		             H5_INT64_T,
		             m->first_b_vtx));
			TRY (h5priv_read_attrib (
		             m->mesh_gid,
		             "__is_chunked__",
		             H5_INT16_T,
		             &m->is_chunked));
			TRY (h5priv_read_attrib (
		             m->mesh_gid,
		             "__num_weights__",
		             H5_INT32_T,
		             &m->num_weights));
		} else {
			memset (m->num_b_vtx, -1, m->num_leaf_levels * sizeof (m->num_b_vtx));
			memset (m->first_b_vtx, -1, m->num_leaf_levels * sizeof (m->first_b_vtx));
			m->is_chunked = 0;
			m->num_weights = 0;
		}


	}
	H5_RETURN (H5_SUCCESS);
}


/*
   Open HDF5 group with specific mesh
 */
static h5_err_t
release_elems (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	TRY (h5_free (m->loc_elems));                   m->loc_elems = NULL;
	TRY (h5_free (m->num_glb_elems));               m->num_glb_elems = NULL;
	TRY (h5_free (m->num_glb_leaf_elems));          m->num_glb_leaf_elems = NULL;
	TRY (h5_free (m->num_interior_elems));          m->num_interior_elems = NULL;
	TRY (h5_free (m->num_interior_leaf_elems));     m->num_interior_leaf_elems = NULL;
	TRY (h5_free (m->num_ghost_elems));             m->num_ghost_elems = NULL;
	TRY (h5_free (m->map_elem_g2l.items));          m->map_elem_g2l.items = NULL;
	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
release_vertices (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	TRY (h5_free (m->vertices));                    m->vertices = NULL;
	TRY (h5_free (m->num_glb_vertices));            m->num_glb_vertices = NULL;
	TRY (h5_free (m->num_loc_vertices));            m->num_loc_vertices = NULL;
	TRY (h5_free (m->map_vertex_g2l.items));        m->map_vertex_g2l.items = NULL;
	TRY (h5_free (m->first_b_vtx)); 				m->first_b_vtx = NULL;
	TRY (h5_free (m->num_b_vtx)); 					m->num_b_vtx = NULL;
	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
release_memory (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	TRY (h5tpriv_release_adjacency_structs (m));
	TRY (release_elems (m));
	TRY (release_vertices (m));
#ifdef WITH_PARALLEL_H5GRID
	if (m->is_chunked) {
		TRY (h5tpriv_free_chunks (m));
		TRY (H5t_free_octree (m->octree));
		TRY (h5_free (m->weights));
	}
#endif
	TRY (h5_free (m));
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
write_timing (
		h5t_mesh_t* const m
) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p", m);
	if (m->f->myproc == 0 && m->timing.f != NULL) {
		FILE* file = fopen (m->timing.f,"a");
		h5_time_t tmp =m->timing.measure[m->timing.num_timing-1]- m->timing.measure[0];
		fprintf (file,"#writing timing \n "
				" nprocs max_chunks num_elems tot_time init_mesh "
				" reading_octree reading_chunks reading_weights "
				" distributing_chunks reading_elems reading_vtx "
				" internal_update ref pre_ref ref  boundary weights "
				" init_glb_elems init_glb_vtx exchange_glb_structs "
				" store_glb_struct post_ref close "
				" write_weights write_chunks write_octree "
				" calc_vtx_map vtx_hyperslap write_vertices write_elems "
				"\n"
				" %d %d %lld %4.4f",
				m->f->nprocs, max_num_elems_p_chunk,
				(long long int) m->num_glb_elems[m->num_leaf_levels-1],
				tmp >=0 ? tmp: 0);

		for (int i = 1; i < m->timing.num_timing; i++) {
			tmp = m->timing.measure[i]- m->timing.measure[i-1] ;
			fprintf (file," %4.4f ", tmp >=0 ? tmp: 0 );
		}
		fprintf (file,"\n");
		fprintf (file, "# nprocs reading_octree_chunks distribute_chunks reading_elems"
				" reading_vtx refinement postrefinement write_oct_chunks "
				" vtx_hyperslaps write_vtx write_elems\n");
		fprintf (file,"# %d %4.4f %4.4f %4.4f %4.4f %4.4f %4.4f %4.4f %4.4f %4.4f %4.4f \n\n",
				m->f->nprocs,
				m->timing.measure[4] - m->timing.measure[0],
				m->timing.measure[5] - m->timing.measure[4],
				m->timing.measure[6] - m->timing.measure[5],
				m->timing.measure[8] - m->timing.measure[6],
				m->timing.measure[11] - m->timing.measure[8],
				m->timing.measure[18] - m->timing.measure[11],
				m->timing.measure[22] - m->timing.measure[18],
				m->timing.measure[24] - m->timing.measure[22],
				m->timing.measure[25] - m->timing.measure[24],
				m->timing.measure[26] - m->timing.measure[25]);
		fclose (file);
	}
	H5_RETURN (H5_SUCCESS);
}
h5_err_t
set_timing_file (
		h5t_mesh_t* const m,
		char* time_f
		) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p", m);
	m->timing.f = time_f;
	H5_RETURN (H5_SUCCESS);
}
h5_err_t
h5t_close_mesh (
        h5t_mesh_t* const m
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p", m);
#ifdef WITH_PARALLEL_H5GRID
	TRY (h5priv_mpi_barrier (m->f->props->comm));
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
#endif
	// check if tagsets are still open
	if (m->mtagsets && m->mtagsets->num_items > 0)
		H5_RETURN_ERROR (
			H5_ERR_H5FED,
			"%s",
			"Mesh cannot be closed: Mesh is referenced by open tagsets");

	if (!(m->f->props->flags & H5_O_RDONLY)) {
		TRY (h5tpriv_write_mesh (m));
	}
	TRY (hdf5_close_group (m->mesh_gid));
#ifdef WITH_PARALLEL_H5GRID
	TRY (write_timing (m));
#endif
	TRY (release_memory (m));
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5t_set_level (
        h5t_mesh_t* const m,
        const h5_lvl_idx_t level_id
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p, level_id=%d", m, level_id);

	if ((level_id < 0) || (level_id >= m->num_leaf_levels))
		H5_LEAVE (HANDLE_H5_OUT_OF_RANGE_ERR ("Level", level_id));

	h5_lvl_idx_t prev_level = m->leaf_level;
	m->leaf_level = level_id;

	if (level_id >= m->num_loaded_levels) {
		TRY (h5tpriv_update_internal_structs (m, ++prev_level));
	}
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5t_set_mesh_changed (
        h5t_mesh_t* const m
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p",m);
	m->mesh_changed = 1;
	H5_RETURN (H5_SUCCESS);
}

/*
   Allocate \c num additional vertices.
 */
h5_err_t
h5tpriv_alloc_loc_vertices (
        h5t_mesh_t* const m,
        const h5_size_t num
        ) {
	H5_PRIV_API_ENTER (h5_err_t,
	                    "m=%p, num=%llu",
	                    m,
	                    (long long unsigned)num);
	ssize_t size = num * sizeof (m->vertices[0]);
	TRY (m->vertices = h5_alloc (m->vertices, size));
	TRY (h5priv_grow_idxmap (&m->map_vertex_g2l, num));
	H5_RETURN (H5_SUCCESS);
}


/*!
  Get number of meshes of given type.

  \param[in]	f	File handle
  \param[in]	type_id	Type of mesh we want the number of.

  \return	Number of meshes of type \c type_id or error code.
 */
static inline h5_ssize_t
get_num_meshes (
	const h5_file_t fh,
	const char* grpname
	) {
        h5_file_p f = (h5_file_p)fh;
	H5_PRIV_FUNC_ENTER (h5_ssize_t, "f=%p, grpname=%s", f, grpname);
	hid_t topo_gid = -1;
	hid_t meshes_gid = -1;

	h5_err_t exists;
	TRY (exists = hdf5_link_exists (f->root_gid, H5T_CONTAINER_GRPNAME));
	if (!exists) H5_LEAVE (0);

	TRY (topo_gid = hdf5_open_group (f->root_gid, H5T_CONTAINER_GRPNAME));

	TRY (exists = hdf5_link_exists (topo_gid, grpname));
	if (!exists) H5_LEAVE (0);

	TRY (meshes_gid = hdf5_open_group (topo_gid, grpname));
	h5_ssize_t num_meshes;
	TRY (num_meshes = hdf5_get_num_groups (meshes_gid));
	TRY (hdf5_close_group (meshes_gid) );
	TRY (hdf5_close_group (topo_gid) );

	H5_RETURN (num_meshes);
}

h5_ssize_t
h5t_get_num_tetmeshes (
	const h5_file_t fh
	) {
	H5_CORE_API_ENTER (h5_ssize_t, "f=%p", (h5_file_p)fh);
	TRY (ret_value = get_num_meshes (fh, TETRAHEDRAL_MESHES_GRPNAME));
	H5_RETURN (ret_value);
}

h5_ssize_t
h5t_get_num_trimeshes (
	const h5_file_t fh
	) {
	H5_CORE_API_ENTER (h5_ssize_t, "f=%p", (h5_file_p)fh);
	TRY (ret_value = get_num_meshes (fh, TRIANGLE_MESHES_GRPNAME));
	H5_RETURN (ret_value);
}

/*!
  Get the number of hierarchical mesh levels for the current mesh.

  \param[in]	f	File handle

  \return	Number of hierarchical mesh levels or error code.
 */
h5_ssize_t
h5t_get_num_leaf_levels (
	h5t_mesh_t* const m
	) {
	H5_CORE_API_ENTER (h5_ssize_t, "m=%p", m);
	H5_RETURN (m->num_leaf_levels);
}

/*!
  Get current level.

  \param[in]	f	File handle.

  \return	Current level ID.
*/
h5_lvl_idx_t
h5t_get_level (
	h5t_mesh_t* const m
	) {
	H5_CORE_API_ENTER (h5_lvl_idx_t, "m=%p", m);
	H5_RETURN (m->leaf_level);
}

/*!
  Return number of elems on compute node \c cnode_id on
  current level. If \cnode_id is equal \c -1 return the 
  number of elements in the entire mesh.

  \remark
  Refined elems are *not* counted.

  \param[in]	f	File handle.
  \param[in]	cnode	Compute node

  \return	Number of elements or error code.
 */

h5_ssize_t
h5t_get_num_leaf_elems (
        h5t_mesh_t* const m,
        const h5_id_t cnode
        ) {
	H5_CORE_API_ENTER (h5_ssize_t, "m=%p, cnode=%llu",
	                   m, (long long unsigned)cnode);
	UNUSED_ARGUMENT (cnode);

	if (m->leaf_level < 0) {
		H5_LEAVE (h5tpriv_error_undef_level ());
	}
	H5_RETURN (m->num_interior_leaf_elems[m->leaf_level]);
}
/*!
  Return number of vertices on compute node \c cnode_id
  on current level.  If \cnode_id is equal \c -1 return the 
  number of vertices in the entire mesh.

  \remark
  There is nothing like "refined vertices".

  \param[in]	f	File handle.
  \param[in]	cnode	Compute node

  \return	Number of vertices or error code.
 */
h5_ssize_t
h5t_get_num_vertices (
        h5t_mesh_t* const m,
        h5_id_t cnode
        ) {
	H5_CORE_API_ENTER (h5_ssize_t,
	                   "m=%p, cnode=%llu",
	                   m, (long long unsigned)cnode);
	UNUSED_ARGUMENT (cnode);

	if (m->leaf_level < 0) {
		H5_LEAVE (h5tpriv_error_undef_level ());
	}
	H5_RETURN (m->num_loc_vertices[m->leaf_level]);
}

/*!
   Return if mesh is chunked

   \param[in]	f	File handle.

   \return	0 if not chunked
 */
h5_lvl_idx_t
h5t_is_chunked (
        h5t_mesh_t* const m
        ) {
	H5_CORE_API_ENTER (h5_lvl_idx_t, "m=%p", m);
	H5_RETURN (m->is_chunked);
}

