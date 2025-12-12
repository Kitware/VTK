/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include "private/h5_fcmp.h"
#include "private/h5_hdf5.h"

#include "private/h5t_types.h"
#include "private/h5t_access.h"
#include "private/h5t_map.h"
#include "h5core/h5t_map.h"
#include "private/h5t_model.h"
#include "private/h5_mpi.h"

#include <stdlib.h>

/*
   Mapping of global to local id's:

   Before adding a new level or closing the mesh, we must define global id's
   for the vertices and elements. This we have to do only for the last stored
   level.
 */


/*!
   Compare to vertices given by their 3-dimensional coordinates
 */
static int
cmp_vertices (
        h5_float64_t P0[3],
        h5_float64_t P1[3]
        ) {
	int i;
	for (i = 0; i < 3; i++) {
		h5_int64_t diff = h5priv_fcmp (P0[i], P1[i], 10);
		if (diff < 0) return -1;
		else if (diff > 0) return 1;
	}
	return 0;
}

/*!
   Sort (small) array of local vertex indices geometrically.
 */
h5_err_t
h5tpriv_sort_local_vertex_indices (
        h5t_mesh_t* const m,
        h5_loc_idx_t* const indices,    /* IN/OUT: local vertex indices */
        const h5_size_t size            /* size of array */
        ) {
	H5_PRIV_API_ENTER (h5_err_t,
	                   "m=%p, indices=%p, size=%llu",
	                   m, indices, (long long unsigned)size);

	h5_size_t i;
	for (i = 1; i < size; ++i) {
		h5_loc_idx_t idx = indices[i];

		h5_size_t j = i;
		while ((j >= 1 ) && cmp_vertices (
		               m->vertices[idx].P,
		               m->vertices[indices[j-1]].P
		               ) < 0 ) {
			indices[j] = indices[j-1];
			--j;
		}
		indices[j] = idx;
	}
	H5_RETURN (H5_SUCCESS);
}

/*!
   find if there is an entry with glb_idx in the map and return position
   if not in map return the position last_position +1
 */
#define h5tpriv_error_global_id_nexist( name, id )		\
	h5_error(						\
		H5_ERR_NOENTRY,					\
		"%s with global id %lld does not exist!",	\
		name, (long long)id );

h5_loc_idx_t
h5tpriv_find_glb_idx_in_map (
		h5_idxmap_t*  map,
        const h5_glb_idx_t glb_idx
        ) {
	H5_CORE_API_ENTER (h5_loc_idx_t, "m=%p, glb_idx=%lld", map, (long long)glb_idx);
	if (glb_idx < 0) return -1;

	h5_loc_idx_t loc_idx = h5priv_search_idxmap (map, glb_idx); // loc_idx is position in map
	if (loc_idx < 0) { // set to next position
		loc_idx = map->num_items;
	}
	H5_RETURN (loc_idx);
}

/*!
   Map a global vertex index to corresponding local index.
 */
h5_loc_idx_t
h5t_map_global_vertex_idx2local (
        h5t_mesh_t* const m,
        const h5_glb_idx_t glb_idx
        ) {
	H5_CORE_API_ENTER (h5_loc_idx_t, "m=%p, glb_idx=%lld", m, (long long)glb_idx);
	if (glb_idx < 0) return -1;

	// loc_idx is position in map
	h5_loc_idx_t loc_idx = h5priv_search_idxmap (&m->map_vertex_g2l, glb_idx);
	if (loc_idx < 0) {
		H5_LEAVE (
			h5tpriv_error_global_id_nexist ("vertex", glb_idx));
	}
	// loc_idx is position in m->vertices!
	TRY (ret_value = m->map_vertex_g2l.items[loc_idx].loc_idx);
	H5_RETURN (ret_value);
}

h5_err_t
h5t_map_global_vertex_indices2local (
        h5t_mesh_t* const m,
        const h5_glb_idx_t* const glb_indices,
        const h5_size_t size,
        h5_loc_idx_t* const loc_indices
        ) {
	H5_CORE_API_ENTER (h5_err_t,
	                   "m=%p, glb_indices=%p, size=%llu, loc_indices=%p",
	                   m, glb_indices, (long long unsigned)size, loc_indices);
	h5_size_t i;
	for (i = 0; i < size; i++) {
		TRY (loc_indices[i] =
		             h5t_map_global_vertex_idx2local (m, glb_indices[i]));
	}
	H5_RETURN (H5_SUCCESS);
}

/*!
   Get local element idx of element given by its global idx.

   \param[in]	f		File handle
   \param[in]	glb_idx		Global element index

   \return	Local element index
                or -1, if \c glb_idx is -1
                or \c -glb_idx-2, if cell is on another proc
 */
h5_loc_idx_t
h5t_map_glb_elem_idx2loc (
        h5t_mesh_t* const m,
        const h5_glb_idx_t glb_idx
        ) {
	H5_CORE_API_ENTER (h5_loc_idx_t,
	                   "m=%p, glb_idx=%lld",
	                   m, (long long)glb_idx);

	// global index is -1, if the cell is at the geometric border
	if (glb_idx < 0) H5_LEAVE (-1);

	h5_loc_idx_t i = h5priv_search_idxmap (&m->map_elem_g2l, glb_idx);
	// global index >= 0 && negative result means: element is on other proc
	if (i < 0) H5_LEAVE (-glb_idx-2);

	H5_RETURN (m->map_elem_g2l.items[i].loc_idx);
}


h5_err_t
h5t_map_glb_elem_indices2loc (
        h5t_mesh_t* const m,
        const h5_glb_idx_t*  glb_indices,
        const h5_size_t size,
        h5_loc_idx_t* loc_indices
        ) {
	H5_CORE_API_ENTER (h5_err_t,
	                   "m=%p, glb_indices=%p, size=%llu, loc_indices=%p",
	                   m, glb_indices, (long long unsigned)size, loc_indices);
	const h5_glb_idx_t*  end = glb_indices+size;

	while (glb_indices < end) {
		*loc_indices = h5t_map_glb_elem_idx2loc (m, *glb_indices);
		loc_indices++;
		glb_indices++;
	}
	H5_RETURN (H5_SUCCESS);
}


/*
   rebuild mapping of global vertex indices to their local indices
 */
h5_err_t
h5tpriv_rebuild_map_vertex_g2l (
        h5t_mesh_t* const m,
        h5_lvl_idx_t from_lvl,  // from level
        h5_lvl_idx_t to_lvl             // up to and including this level
        ) {
	H5_CORE_API_ENTER (h5_loc_idx_t, "m=%p, from_lvl=%d, to_lvl=%d",
	                   m, from_lvl, to_lvl);

	h5_loc_idx_t loc_idx = from_lvl > 0 ? m->num_loc_vertices[from_lvl-1] : 0;
	h5_loc_idx_t num_loc_vertices = m->num_loc_vertices[to_lvl];
	h5_idxmap_el_t *item = &m->map_vertex_g2l.items[loc_idx];

	for (; loc_idx < num_loc_vertices; loc_idx++, item++) {
		item->glb_idx = m->vertices[loc_idx].idx;
		item->loc_idx = loc_idx;
		m->map_vertex_g2l.num_items++;
	}
	h5priv_sort_idxmap (&m->map_vertex_g2l);
	H5_RETURN (H5_SUCCESS);
}
/*
   rebuild mapping of global vertex indices to their local indices
 */
h5_err_t
h5tpriv_rebuild_map_vertex_g2l_partial (
        h5t_mesh_t* const m
        ) {
	H5_CORE_API_ENTER (h5_loc_idx_t, "m=%p",
	                   m);

	h5_loc_idx_t loc_idx = m->last_stored_vid_before_ref + 1;
	h5_loc_idx_t start = loc_idx;
	h5_loc_idx_t num_loc_vertices = m->last_stored_vid - m->last_stored_vid_before_ref;
	h5_idxmap_el_t *item = &m->map_vertex_g2l.items[loc_idx];

	for (; loc_idx < start + num_loc_vertices; loc_idx++, item++) {
		item->glb_idx = m->vertices[loc_idx].idx;
		item->loc_idx = loc_idx;
		m->map_vertex_g2l.num_items++;
	}
	h5priv_sort_idxmap (&m->map_vertex_g2l);
	H5_RETURN (H5_SUCCESS);
}
/*
   Get local vertex indices of entity given by it's local ID.
 */
h5_err_t
h5t_get_loc_vertex_indices_of_entity (
        h5t_mesh_t* const m,            // in
        const h5_loc_id_t entity_id,    // in
        h5_loc_idx_t* vertex_indices    // out
        ) {
	H5_CORE_API_ENTER (h5_err_t,
	                   "m=%p, entity_id=%llu, vertex_indices=%p",
	                   m,
	                   (long long unsigned)entity_id,
	                   vertex_indices);
	h5_loc_idx_t type = h5tpriv_get_entity_type (entity_id);
	h5_loc_idx_t face_idx = h5tpriv_get_face_idx (entity_id);
	h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (entity_id);
	int dim = 0;
	switch (type) {
	case H5T_TYPE_VERTEX:   dim = 0; break;
	case H5T_TYPE_EDGE:     dim = 1; break;
	case H5T_TYPE_TRIANGLE: dim = 2; break;
	case H5T_TYPE_TET:      dim = 3; break;
	default:
		H5_LEAVE (h5_error_internal ());
	}
	h5_loc_idx_t* indices = h5tpriv_get_loc_elem_vertex_indices (m, elem_idx);
	const h5t_ref_elem_t* ref_elem = m->ref_elem;
	int num_vertices = ref_elem->num_vertices_of_face[dim][face_idx];
	for (int i = 0; i < num_vertices; i++) {
		int idx = h5tpriv_ref_elem_get_vertex_idx(m, dim, face_idx, i);
		vertex_indices[i] = indices[idx];
	}
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5t_get_glb_vertex_indices_of_entity (
        h5t_mesh_t* const m,            // in
        const h5_loc_id_t entity_id,    // in
        h5_glb_idx_t* vertex_indices    // out
        ) {
	H5_CORE_API_ENTER (h5_err_t,
	                   "m=%p, entity_id=%llu, vertex_indices=%p",
	                   m,
	                   (long long unsigned)entity_id,
	                   vertex_indices);
	h5_loc_idx_t type = h5tpriv_get_entity_type (entity_id);
	h5_loc_idx_t face_idx = h5tpriv_get_face_idx (entity_id);
	h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (entity_id);
	int dim = 0;
	switch (type) {
	case H5T_TYPE_VERTEX:   dim = 0; break;
	case H5T_TYPE_EDGE:     dim = 1; break;
	case H5T_TYPE_TRIANGLE: dim = 2; break;
	case H5T_TYPE_TET:      dim = 3; break;
	default:
		H5_LEAVE (h5_error_internal ());
	}
	h5_loc_idx_t* indices = h5tpriv_get_loc_elem_vertex_indices (m, elem_idx);
	const h5t_ref_elem_t* ref_elem = m->ref_elem;
	int num_vertices = ref_elem->num_vertices_of_face[dim][face_idx];
	for (int i = 0; i < num_vertices; i++) {
		int idx = h5tpriv_ref_elem_get_vertex_idx(m, dim, face_idx, i);
		h5_loc_idx_t loc_idx = indices[idx];
		vertex_indices[i] = m->vertices[loc_idx].idx;
	}
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5tpriv_get_loc_vtx_idx_of_vtx (
	h5t_mesh_t* const m,
	const h5_loc_id_t entity_id,
	h5_loc_idx_t* vertex_index
	) {
	H5_CORE_API_ENTER (h5_loc_idx_t,
	                   "m=%p, entity_id=%llu, vertex_index=%llu",
	                   m,
	                   (long long unsigned)entity_id,
	                   (long long unsigned)*vertex_index);
	h5_loc_idx_t face_idx = h5tpriv_get_face_idx (entity_id);
	h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (entity_id);
	TRY (ret_value = h5tpriv_get_loc_vtx_idx_of_vtx2 (
		     m, face_idx, elem_idx, vertex_index));
	H5_RETURN (ret_value);
}

h5_err_t
h5tpriv_get_loc_vtx_idx_of_vtx2 (
	h5t_mesh_t* const m,
	const h5_loc_idx_t face_idx,	// vertex index according ref. element
	const h5_loc_idx_t elem_idx,	// local element index
	h5_loc_idx_t* vertex_indices	// OUT: vertex ID's
	) {
	H5_CORE_API_ENTER (h5_err_t,
	                   "m=%p, face_idx=%llu, elem_idx=%llu, vertex_indices=%p",
	                   m,
	                   (long long unsigned)face_idx,
	                   (long long unsigned)elem_idx,
	                   vertex_indices);
	vertex_indices[0] = h5tpriv_get_loc_elem_vertex_idx (m, elem_idx, face_idx);
	H5_RETURN (H5_SUCCESS);
}

/*
   Get the local ID of the vertices of an elemet.
 */
h5_err_t
h5t_get_loc_vertex_indices_of_edge (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_loc_idx_t* vertex_indices
        ) {
	H5_CORE_API_ENTER (h5_loc_idx_t,
	                   "m=%p, entity_id=%llu, vertex_indices=%p",
	                   m,
	                   (long long unsigned)entity_id,
	                   vertex_indices);
	h5_loc_idx_t face_idx = h5tpriv_get_face_idx (entity_id);
	h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (entity_id);
	
	TRY (ret_value = h5t_get_loc_vertex_indices_of_edge2 (
		     m, face_idx, elem_idx, vertex_indices));
	H5_RETURN (ret_value);
}

/*!
   Get local vertex indices of an edge. The edge is specified by the local
   element index and the face number of the edge according the reference
   element.

   This function can be used with tetrahedral and triangle meshes.
 */
h5_err_t
h5t_get_loc_vertex_indices_of_edge2 (
        h5t_mesh_t* const m,
        const h5_loc_idx_t face_idx,    // edge index according ref. element
        const h5_loc_idx_t elem_idx,    // local element index
        h5_loc_idx_t* vertex_indices    // OUT: vertex indices
        ) {
	H5_CORE_API_ENTER (h5_err_t,
	                   "m=%p, face_idx=%llu, elem_idx=%llu, vertex_indices=%p",
	                   m,
	                   (long long unsigned)face_idx,
	                   (long long unsigned)elem_idx,
	                   vertex_indices);
	const h5_loc_idx_t* indices = h5tpriv_get_loc_elem_vertex_indices (m, elem_idx);

	h5_loc_idx_t idx;
	idx = h5tpriv_ref_elem_get_vertex_idx (m, 1, face_idx, 0);
	vertex_indices[0] = indices[idx];
	idx = h5tpriv_ref_elem_get_vertex_idx (m, 1, face_idx, 1);
	vertex_indices[1] = indices[idx];
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5t_get_loc_vertex_indices_of_triangle (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_loc_idx_t* vertex_indices
        ) {
	H5_CORE_API_ENTER (h5_loc_idx_t,
	                   "m=%p, entity_id=%llu, vertex_indices=%p",
	                   m,
	                   (long long unsigned)entity_id,
	                   vertex_indices);
	h5_loc_idx_t face_idx = h5tpriv_get_face_idx (entity_id);
	h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (entity_id);
	TRY (ret_value = h5t_get_loc_vertex_indices_of_triangle2 (
		     m, face_idx, elem_idx, vertex_indices));
	H5_RETURN (ret_value);
}

h5_err_t
h5t_get_loc_vertex_indices_of_triangle2 (
        h5t_mesh_t* const m,
        const h5_loc_idx_t face_idx,
        const h5_loc_idx_t elem_idx,
        h5_loc_idx_t* vertex_indices
        ) {
	H5_CORE_API_ENTER (h5_err_t,
	                   "m=%p, face_idx=%llu, elem_idx=%llu, vertex_indices=%p",
	                   m,
	                   (long long unsigned)face_idx,
	                   (long long unsigned)elem_idx,
	                   vertex_indices);
	const h5_loc_idx_t* indices = h5tpriv_get_loc_elem_vertex_indices (m, elem_idx);

	h5_loc_idx_t idx;
	idx = h5tpriv_ref_elem_get_vertex_idx (m, 2, face_idx, 0);
	vertex_indices[0] = indices[idx];
	idx = h5tpriv_ref_elem_get_vertex_idx (m, 2, face_idx, 1);
	vertex_indices[1] = indices[idx];
	idx = h5tpriv_ref_elem_get_vertex_idx (m, 2, face_idx, 2);
	vertex_indices[2] = indices[idx];
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5t_get_loc_vertex_indices_of_tet (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_loc_idx_t* vertex_indices
        ) {
	H5_CORE_API_ENTER (h5_loc_idx_t,
	                   "m=%p, entity_id=%llu, vertex_indices=%p",
	                   m,
	                   (long long unsigned)entity_id,
	                   vertex_indices);

	const h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (entity_id);
	const h5_loc_idx_t* indices = h5tpriv_get_loc_elem_vertex_indices (
	        m, elem_idx);

	h5_loc_idx_t idx;
	idx = h5tpriv_ref_elem_get_vertex_idx (m, 3, 0, 0);
	vertex_indices[0] = indices[idx];
	idx = h5tpriv_ref_elem_get_vertex_idx (m, 3, 0, 1);
	vertex_indices[1] = indices[idx];
	idx = h5tpriv_ref_elem_get_vertex_idx (m, 3, 0, 2);
	vertex_indices[2] = indices[idx];
	idx = h5tpriv_ref_elem_get_vertex_idx (m, 3, 0, 3);
	vertex_indices[3] = indices[idx];
	H5_RETURN (H5_SUCCESS);
}

#ifdef WITH_PARALLEL_H5GRID
int
compare_glb_idx_oct (const void *  p_a,const void*  p_b) {
	return *(h5_glb_idx_t*) p_a - *(h5_glb_idx_t*) p_b;
}

/*
 *  takes all local element idx from m->marked_entities gets their glb_idx
 *  and exchanges them with all proc and adds them to the glb_list
 */
h5_err_t
h5priv_exchange_loc_list_to_glb (
	h5t_mesh_t* const m,
	h5_glb_idxlist_t** glb_list
	) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p, glb_list=%p", m, glb_list);
	int* num_elems = NULL;
	TRY (num_elems = h5_calloc (m->f->nprocs, sizeof (*num_elems)));

	// exchange size of marked_entities
	TRY (h5priv_mpi_allgather (
			&m->marked_entities->num_items,
			1,
			MPI_INT,
			num_elems,
			1,
			MPI_INT,
			m->f->props->comm));
	// calc sendbuf
	h5_glb_idx_t* sendbuf;
	TRY (sendbuf = h5_calloc (m->marked_entities->num_items, sizeof (*sendbuf)));
	// loc -> glb
	for (int i = 0; i < m->marked_entities->num_items; i++) {
		if (m->marked_entities->items[i] > m->last_stored_eid) {
			H5_RETURN_ERROR (
				H5_ERR_INVAL,
				"Element chosen to be refined is %d but there are only %d elements",
				m->marked_entities->items[i],
				m->last_stored_eid + 1);
		}

		sendbuf[i] = h5tpriv_get_loc_elem_glb_idx (m, m->marked_entities->items[i]);

	}


	int* recvdispls = NULL;
	TRY (recvdispls = h5_calloc (m->f->nprocs, sizeof (*recvdispls)));
	int num_tot_elems = num_elems[0];
	for (int i = 1; i < m->f->nprocs; i++) {
		recvdispls[i] = recvdispls[i-1] + num_elems[i-1];
		num_tot_elems += num_elems[i];
	}
	TRY (h5priv_alloc_glb_idxlist (glb_list, num_tot_elems));
	(*glb_list)->num_items = num_tot_elems;
	// exchange
	TRY (mpi_allgatherv (
			sendbuf,
			m->marked_entities->num_items,
			MPI_LONG_LONG,
			(*glb_list)->items,
			num_elems,
			recvdispls,
			MPI_LONG_LONG,
			m->f->props->comm
			))


	qsort ((*glb_list)->items, num_tot_elems, sizeof (*(*glb_list)->items), compare_glb_idx_oct);
	TRY (h5_free (num_elems));
	TRY (h5_free (sendbuf));
	TRY (h5_free (recvdispls));
	H5_RETURN (H5_SUCCESS);
}
#endif

/*
  Find ID in sorted list
*/
h5_loc_id_t
h5priv_find_idlist (
	h5_loc_idlist_t* list,
	const h5_loc_id_t item
	) {
	H5_PRIV_API_ENTER (h5_err_t,
			   "list=%p, item=%llu",
			   list, (long long unsigned)item);
	if (!list) {
		H5_LEAVE (-1);
	}
	register size_t low = 0;
	register size_t mid;
	register size_t high = list->num_items - 1;
	register h5_loc_idx_t diff;
	const h5_loc_id_t face_id =  h5tpriv_get_face_id(item);
	const h5_loc_idx_t elem_idx =  h5tpriv_get_elem_idx(item);
	while (low <= high) {
		mid = (low + high) / 2;
		diff = h5tpriv_get_elem_idx(list->items[mid]) - elem_idx;
		// if element indices are equal, we decide on the face indices
		if (diff == 0) {
			diff = h5tpriv_get_face_id (list->items[mid]) - face_id;
		}
           	if ( diff > 0 )
               		high = mid - 1;
           	else if ( diff < 0 )
               		low = mid + 1;
           	else
               		H5_LEAVE (mid); // found
       	}
       	H5_RETURN (-(low+1));  // not found
}

