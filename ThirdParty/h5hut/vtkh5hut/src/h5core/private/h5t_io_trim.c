/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include "private/h5t_types.h"
#include "private/h5t_map.h"
#include "private/h5t_model.h"
#include "private/h5t_io.h"
#include "private/h5t_access.h"

#include "h5core/h5t_map.h"

#define H5_GLB_ELEM_T   h5_glb_tri_t
#define H5_LOC_ELEM_T   h5_loc_tri_t

/*
   initialize local element structure
 */
static h5_err_t
init_loc_elems_struct (
        h5t_mesh_t* const m,
        const h5_glb_elem_t* const elems,
        const h5_loc_idx_t from_idx,
        const h5_loc_idx_t count,
        const h5_uint32_t flags,
        const h5_int32_t* my_proc
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, elems=%p, from_idx=%lld, count=%lld, flags=%xu",
	                    m, elems, (long long)from_idx, (long long)count, flags);

	int num_vertices = h5tpriv_ref_elem_get_num_vertices (m);
	int num_facets = h5tpriv_ref_elem_get_num_facets (m);
	H5_LOC_ELEM_T* loc_elem = (H5_LOC_ELEM_T*)m->loc_elems + from_idx;
	H5_GLB_ELEM_T* glb_elem = (H5_GLB_ELEM_T*)elems;
	H5_GLB_ELEM_T* last = glb_elem + count;
	int counter = 0;

	for (; glb_elem < last; loc_elem++, glb_elem++, counter++) {
		// global element index
		loc_elem->glb_idx = glb_elem->idx;

		// local parent index
		loc_elem->parent_idx = h5t_map_glb_elem_idx2loc (m, glb_elem->parent_idx);

		// local child index
		loc_elem->child_idx = h5t_map_glb_elem_idx2loc (m, glb_elem->child_idx);

		// level idx
		loc_elem->level_idx = glb_elem->level_idx;

		// refinement level
		loc_elem->refinement = glb_elem->refinement;

		// flags
		loc_elem->flags = glb_elem->flags | flags;

		// proc
		if (my_proc == NULL) {
			loc_elem->my_proc = -1;
		} else {
			loc_elem->my_proc = my_proc[counter];
		}

		// vertex indices
		TRY (h5t_map_global_vertex_indices2local (
		             m,
		             glb_elem->vertex_indices,
		             num_vertices,
		             loc_elem->vertex_indices));

		// neighbor indices
		h5t_map_glb_elem_indices2loc (
		        m,
		        glb_elem->neighbor_indices,
		        num_facets,
		        loc_elem->neighbor_indices);

	}
	m->last_stored_eid = from_idx + count -1;
	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
init_elem_flags (
        h5t_mesh_t* const m,
        const h5_loc_idx_t from,
        const h5_loc_idx_t count
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, from=%lld, count=%lld",
	                    m, (long long)from, (long long)count);

	int num_facets = h5tpriv_ref_elem_get_num_facets (m);

	H5_LOC_ELEM_T* elems = ((H5_LOC_ELEM_T*)m->loc_elems);
	H5_LOC_ELEM_T* elem = ((H5_LOC_ELEM_T*)m->loc_elems) + from;
	H5_LOC_ELEM_T* end = elem + count;

	while (elem < end) {
		for (int i = 0; i < num_facets; i++) {
			if (elem->neighbor_indices[i] == -1) {
				elem->flags |= H5_GEOBORDER_ENTITY;
			}
			if (elem->flags & H5_GHOST_ENTITY) {
				continue; // if ghost, we are done
			}
			// elem is not ghost so it must be interior (for the time
			// being we have no overlap
			elem->flags |= H5_INTERIOR_ENTITY;

			if (elem->neighbor_indices[i] <= -1 || 								// added <= istead of <
			    (elems[elem->neighbor_indices[i]].flags & H5_GHOST_ENTITY)) {  //TODO invalid access probably elem->neighbor_indices[i] == -1
				// if neighbor is < -1 we do not have ghost cells and cell
				// is border cell
				// if neighbor is ghost, cell is border cell
				elem->flags |= H5_BORDER_ENTITY;
			}
		}
		elem++;
	}
	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
init_map_elem_g2l (
        h5t_mesh_t* const m,
        h5_glb_elem_t* elems,
        h5_loc_idx_t count
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, count=%lld",
	                    m, (long long)count);
	h5_idxmap_t* map = &m->map_elem_g2l;

	/* alloc mem for global to local ID mapping */
	TRY (h5priv_grow_idxmap (map, map->num_items+count));
	h5_loc_idx_t offs = map->num_items;

	h5_idxmap_el_t* item = &map->items[map->num_items];
	H5_GLB_ELEM_T* elem = (H5_GLB_ELEM_T*)elems;

	for (h5_loc_idx_t i = 0; i < count; elem++, i++, item++) {
		item->glb_idx = elem->idx;
		item->loc_idx = i + offs;
		map->num_items++;
	}
	h5priv_sort_idxmap (map);

	H5_RETURN (H5_SUCCESS);
}

/*
   Setup data structure to be written on disk. We always write the hole mesh.
 */
static h5_err_t
init_glb_elems_struct (
        h5t_mesh_t* const m,
        const h5_glb_elem_t* const glb_elems
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	h5_loc_idx_t num_interior_elems = m->num_interior_elems[m->num_leaf_levels-1];

	// simple in serial runs: global index = local index
	H5_LOC_ELEM_T* loc_elem = (H5_LOC_ELEM_T*)m->loc_elems;
	H5_GLB_ELEM_T* glb_elem = (H5_GLB_ELEM_T*)glb_elems;
	H5_LOC_ELEM_T* end = loc_elem + num_interior_elems;

	while (loc_elem < end) {
		glb_elem->idx = loc_elem->glb_idx;
		glb_elem->parent_idx = loc_elem->parent_idx;
		glb_elem->child_idx = loc_elem->child_idx;
		glb_elem->level_idx = loc_elem->level_idx;
		glb_elem->refinement = loc_elem->refinement;
		glb_elem->flags = 0;
		for (int i = 0; i < 3; i++) {
			glb_elem->vertex_indices[i] = loc_elem->vertex_indices[i];
			glb_elem->neighbor_indices[i] = loc_elem->neighbor_indices[i];
		}
		loc_elem++;
		glb_elem++;
	}
	H5_RETURN (H5_SUCCESS);
}

//TODO maybe use ifdef to have name without _chk
static h5_err_t
init_glb_elems_struct_chk (
        h5t_mesh_t* const m,
        const h5_glb_elem_t* const glb_elems,
        h5_chk_idx_t* chk_list,
        int num_chk
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
#if defined(WITH_PARALLEL_H5GRID)
	h5_chk_idx_t chk_idx = -1;
	h5_chk_size_t num_elems = -1;
	h5_size_t counter = 0;
	for (int i = 0; i < num_chk; i++) {
		chk_idx = chk_list[i];
		num_elems = m->chunks->chunks[chk_idx].num_elems; // num_elems per chunk
		h5_glb_idx_t glb_idx = m->chunks->chunks[chk_idx].elem;
		for (h5_chk_size_t j = 0; j < num_elems; j++) {
			// get local element
			h5_loc_idx_t loc_idx = h5t_map_glb_elem_idx2loc(m, glb_idx);
			H5_LOC_ELEM_T* loc_elem = &((H5_LOC_ELEM_T*)m->loc_elems)[loc_idx];
			H5_GLB_ELEM_T* glb_elem = &((H5_GLB_ELEM_T*)glb_elems)[counter];
			glb_elem->idx = loc_elem->glb_idx;
			glb_elem->parent_idx = loc_elem->parent_idx > -1 ?
                                h5tpriv_get_loc_elem_glb_idx(m, loc_elem->parent_idx) : loc_elem->parent_idx;
			glb_elem->child_idx = loc_elem->child_idx > -1 ?
                                h5tpriv_get_loc_elem_glb_idx(m, loc_elem->child_idx) : loc_elem->child_idx;
			glb_elem->level_idx = loc_elem->level_idx;
			glb_elem->refinement = loc_elem->refinement;
			glb_elem->flags = 0; // TODO is that correct? why do we then store it?
			for (int i = 0; i < 3; i++) {
				glb_elem->vertex_indices[i] = loc_elem->vertex_indices[i] > -1 ?
						m->vertices[loc_elem->vertex_indices[i]].idx :loc_elem->vertex_indices[i];

				if (loc_elem->neighbor_indices[i] > -1) {
					glb_elem->neighbor_indices[i] =  ((H5_LOC_ELEM_T*)m->loc_elems)[loc_elem->neighbor_indices[i]].glb_idx;
				} else {
					if (loc_elem->neighbor_indices[i] < -1) {
						// if elem not locally available we store the - (glb_idx + 2)
						//TODO is there a func or makro that does that?
						glb_elem->neighbor_indices[i] = -( loc_elem->neighbor_indices[i] + 2);
					} else {
					glb_elem->neighbor_indices[i] = loc_elem->neighbor_indices[i]; // geometrical border
					}
				}
			}
			counter++;
			glb_idx++;
		}

	}
#endif
	H5_RETURN (H5_SUCCESS);
}





struct h5t_read_methods h5tpriv_read_trim_methods = {
	init_loc_elems_struct,
	init_elem_flags,
	init_map_elem_g2l,
	init_glb_elems_struct,
	init_glb_elems_struct_chk,
};
