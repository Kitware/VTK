/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include "private/h5_err.h"
#include "private/h5t_types.h"
#include "private/h5t_err.h"
#include "private/h5t_access.h"
#include "private/h5t_core.h"
#include "private/h5t_map.h"
#include "private/h5t_model.h"
#include "private/h5t_store.h"
#include "private/h5t_core.h"
#include "private/h5t_io.h"
#include "private/h5_file.h"
#include "private/h5_mpi.h"

#include "h5core/h5t_map.h"

#include <stdlib.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/*
  maximum elements per chunk.
  minimum is 4 for triangle meshes
  and 8 for thetrahedral meshes
 */
int max_num_elems_p_chunk = 120;

#ifdef WITH_PARALLEL_H5GRID
// that probably doesn't belong here... //TODO put in right place + print variables
h5_edge_list_t*
h5tpriv_init_edge_list (
	h5_int32_t num_alloc
	) {

	h5_edge_list_t* list = NULL;
	list = h5_calloc (1, sizeof (*list));
	list->num_alloc = num_alloc;
	list->num_items = 0;
	list->items = h5_calloc (num_alloc, sizeof (*list->items));
	return list;
}
h5_err_t
h5tpriv_free_edge_list (
	h5_edge_list_t* list
	) {
	H5_PRIV_API_ENTER (h5_err_t, "list=%p", list);
	TRY (h5_free (list->items));
	list->items = NULL;
	TRY (h5_free(list));
	list = NULL;
	H5_RETURN (H5_SUCCESS);
}
h5_err_t
h5tpriv_grow_edge_list (
	h5_edge_list_t* list,
	h5_int32_t size
	) {
	H5_PRIV_API_ENTER (h5_err_t, "list=%p", list);
	assert (list->num_alloc + size >= 0);
	if (size < 0) {
		h5_debug ("Warning: you are shrinking the edge_list!");
	}
	if (size == 0) {
		h5_debug ("Warning: you are not growing the edge_list!");
	}
	TRY ( list->items = h5_alloc(list->items, (list->num_alloc + size) * sizeof (*list->items)));
	H5_RETURN (H5_SUCCESS);
}

/*
 * compare edge_list_elem
 */
int compare_edge_list_elem(const void *p_a, const void *p_b) {
	h5t_edge_list_elem_t* elem_a = ((h5t_edge_list_elem_t*) p_a);
	h5t_edge_list_elem_t* elem_b = ((h5t_edge_list_elem_t*) p_b);

	if (elem_a->vtx1 - elem_b->vtx1 != 0) {
		return elem_a->vtx1 - elem_b->vtx1;
	} else {
		if (elem_a->vtx2 - elem_b->vtx2 != 0) {
			return elem_a->vtx2 - elem_b->vtx2;
		}
	}
	return elem_a->proc - elem_b->proc;
}
h5_err_t
h5tpriv_uniquify_edge_list (
	h5_edge_list_t* list
	) {
	H5_PRIV_API_ENTER (h5_err_t, "list=%p", list);
	if (list->num_items == 0) {
		H5_LEAVE (H5_SUCCESS);
	}
	h5t_edge_list_elem_t* old_elem = list->items;
	int num_old_elems = list->num_items;
	list->items = h5_calloc (list->num_alloc, sizeof (*list->items));
	memcpy (&list->items[0], &old_elem[0], sizeof (*list->items));
	list->num_items = 1;
	for (int i = 1; i < num_old_elems; i++) {
		int comp = compare_edge_list_elem (&list->items[list->num_items-1], &old_elem[i] );
		if (comp > 0) { // element in old_elem is smaller then last elem in list
			H5_LEAVE (H5_ERR_INVAL); // probably list wasn't sorted
		}
		if (comp < 0) {
			memcpy (&list->items[list->num_items++], &old_elem[i], sizeof (*list->items));
		}
	}
	TRY (h5_free (old_elem));

	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5tpriv_sort_edge_list (
	h5_edge_list_t* list
	) {
	H5_PRIV_API_ENTER (h5_err_t, "list=%p", list);
	qsort(list->items,list->num_items, sizeof (*list->items), compare_edge_list_elem);
	H5_RETURN (H5_SUCCESS);
}


h5_int32_t
h5tpriv_find_edge_list (
	h5_edge_list_t* list,
	h5t_edge_list_elem_t* elem
	) {
	H5_PRIV_API_ENTER (h5_err_t, "list=%p", list);

	comparison_fn_t comp_func;
	comp_func.compare = compare_edge_list_elem;
	// need linear search because we want to know first elem that hits.
	h5t_edge_list_elem_t* retval = linsearch (elem, list->items ,list->num_items, sizeof (*list->items), comp_func);
	if (retval == NULL) {
		H5_LEAVE (list->num_items);
	}
	H5_RETURN (retval - list->items); // TODO check if that works
}
h5_err_t
h5tpriv_add_edge_list (
	h5_edge_list_t* list,
	h5_glb_idx_t vtx1,
	h5_glb_idx_t vtx2,
	h5_loc_idx_t new_vtx,
	h5_int32_t proc
	) {
	H5_PRIV_API_ENTER (h5_err_t, "list=%p", list);
	h5t_edge_list_elem_t elem;
	elem.vtx1 = vtx1 < vtx2 ? vtx1 : vtx2; // vtx1 < vtx2 always!
	elem.vtx2 = vtx1 > vtx2 ? vtx1 : vtx2;
	elem.new_vtx = (h5_glb_idx_t) new_vtx; // problem => first we would like to store loc_idx and later the glb_idx therefore cast here
	// alternative would be to store two variables...
	elem.proc = -1;

	// add edge
	if (list->num_alloc == list->num_items) {
		H5_LEAVE (H5_ERR_INVAL);
	}
	list->items[list->num_items].vtx1	= elem.vtx1;
	list->items[list->num_items].vtx2	= elem.vtx2;
	list->items[list->num_items].new_vtx	= elem.new_vtx;
	list->items[list->num_items++].proc	= proc;

	H5_RETURN (H5_SUCCESS);
}

//END TODO put in right place + print variables

h5_err_t
h5tpriv_calc_chunk_statistic (
	h5t_mesh_t* const m,
	FILE* file
	) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p", m);
	if (file == NULL || m->chunks == NULL) {
		H5_LEAVE (H5_SUCCESS);
	}
	fprintf (file, "# printing chunk statistics of file \n");
	fprintf (file,
                 "# num_levels level max_elem num_chunks elems_p_level "
                 "minfill maxfill avg_p_level avg_fill_p_level \n");
	int counter = 0;
	h5_glb_idx_t* num_elems_p_level = NULL;
	TRY (num_elems_p_level = h5_calloc (m->chunks->num_levels +1, sizeof (*num_elems_p_level)));
	h5_glb_idx_t* min_elems_p_level = NULL;
	TRY (min_elems_p_level = h5_calloc (m->chunks->num_levels +1, sizeof (*min_elems_p_level)));
	h5_glb_idx_t* max_elems_p_level = NULL;
	TRY (max_elems_p_level = h5_calloc (m->chunks->num_levels +1, sizeof (*max_elems_p_level)));
	h5_float64_t* avg_p_level = NULL;
	TRY (avg_p_level = h5_calloc (m->chunks->num_levels +1, sizeof (*avg_p_level)));
	h5_float64_t* avgfill_p_level = NULL;
	TRY (avgfill_p_level = h5_calloc (m->chunks->num_levels +1, sizeof (*avg_p_level)));
	min_elems_p_level[ m->chunks->num_levels] = m->chunks->chunks[counter].num_elems;
	max_elems_p_level[ m->chunks->num_levels] = m->chunks->chunks[counter].num_elems;
	for (int i = 0; i < m->chunks->num_levels; i++) {
		// calc avg
		min_elems_p_level[i] = m->chunks->chunks[counter].num_elems;
		max_elems_p_level[i] = m->chunks->chunks[counter].num_elems;
		for (int j = 0; j < m->chunks->num_chunks_p_level[i]; j++) {
			num_elems_p_level[i] += m->chunks->chunks[counter].num_elems;
			num_elems_p_level[m->chunks->num_levels] += m->chunks->chunks[counter].num_elems;
                        min_elems_p_level[i] = 
                                (min_elems_p_level[i] > m->chunks->chunks[counter].num_elems) ?
                                m->chunks->chunks[counter].num_elems : 0;
                        max_elems_p_level[i] = 
                                max_elems_p_level[i] < m->chunks->chunks[counter].num_elems ?
                                m->chunks->chunks[counter].num_elems : 0;
			counter++;
		}
		avg_p_level[i] = num_elems_p_level[i] / (double) m->chunks->num_chunks_p_level[i];
		avgfill_p_level[i] = avg_p_level[i] / max_num_elems_p_chunk;
                min_elems_p_level[m->chunks->num_levels] = 
                        min_elems_p_level[m->chunks->num_levels] > min_elems_p_level[i] ?
                        min_elems_p_level[i] : 0;
                max_elems_p_level[m->chunks->num_levels] = 
                        max_elems_p_level[m->chunks->num_levels] < max_elems_p_level[i] ?
                        max_elems_p_level[i] : 0;
	}
	avg_p_level[m->chunks->num_levels] = num_elems_p_level[m->chunks->num_levels] / (double) counter;
	avgfill_p_level[m->chunks->num_levels] = avg_p_level[m->chunks->num_levels] / max_num_elems_p_chunk;
	for (int i = 0; i <= m->chunks->num_levels; i++) {
		if (i == m->chunks->num_levels) {
			fprintf (file, " %6d %6d %9d %9d %10lld %10lld %10lld %10.4f %10.4f #avg over whole mesh\n\n",
					m->chunks->num_levels, i, max_num_elems_p_chunk, counter,
					(long long int) num_elems_p_level[i],(long long int)min_elems_p_level[i],
					(long long int)max_elems_p_level[i],avg_p_level[i],avgfill_p_level[i]);
		} else {
			fprintf (file, " %6d %6d %9d %9d %10lld %10lld %10lld %10.4f %10.4f \n",
                                 m->chunks->num_levels, i, max_num_elems_p_chunk,
                                 m->chunks->num_chunks_p_level[i],
                                 (long long int) num_elems_p_level[i],(long long int)min_elems_p_level[i],
                                 (long long int)max_elems_p_level[i],avg_p_level[i],avgfill_p_level[i]);
		}

	}

	H5_RETURN (H5_SUCCESS);
}

/*
 * Calculate the global vertex range for the new vertices
 * range[i] = first glb idx of proc i
 * range[nproc] = next glb idx to assign
 */
h5_err_t
h5tpriv_get_vtx_ranges (
	h5t_mesh_t* const m,
	h5_glb_idx_t* range
	) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p, range=%p", m, range);
	h5_glb_idx_t sendbuf = m->last_stored_vid - m->last_stored_vid_before_ref;

	TRY (h5priv_mpi_allgather (
			&sendbuf,
			1,
			MPI_LONG,
			&range[1],
			1,
			MPI_LONG,
			m->f->props->comm));

	// set start of new vtx idx (because if leaf_level != 0 it was increased already)
	if (m->leaf_level == 0) {
		range[0] = 0;
	} else {
		range[0] = m->num_glb_vertices[m->leaf_level-1];
	}
	// calc range
	for (int i = 1; i <= m->f->nprocs; i++) {
		range[i] += range[i-1];
	}

	if (m->leaf_level == 0) {
		m->num_glb_vertices[0] = range[m->f->myproc];
	} else {
		m->num_glb_vertices[m->leaf_level-1] = range[m->f->myproc];
	}
	H5_RETURN (H5_SUCCESS);
}
#endif
/*
   Assign unique global indices to vertices.
 */
static h5_err_t
assign_global_vertex_indices (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER 	(h5_err_t, "m=%p", m);
	h5_loc_idx_t local_idx = (m->leaf_level == 0) ?
	                         0 : m->num_loc_vertices[m->leaf_level-1];
#if defined(WITH_PARALLEL_H5GRID)
        if (m->is_chunked && m->f->nprocs > 1) {
                // exchange num vertices and calc range
                h5_glb_idx_t* range = NULL;
                TRY (range = h5_calloc (m->f->nprocs + 1, sizeof (*range)));
                TRY (h5tpriv_get_vtx_ranges (m, range));
                int counter = 0;
                for (
                        ;
                        local_idx < m->num_loc_vertices[m->num_leaf_levels-1];
                        local_idx++, counter++) {
                        m->vertices[local_idx].idx = range[m->f->myproc] + counter;
                }
                if (counter + range[m->f->myproc] != range[m->f->myproc + 1]) {
                        H5_LEAVE (H5_ERR_INTERNAL);
                }
                TRY (h5_free (range));
        } else
#endif
	{
                // simple in serial runs: global_id = local_id
                for (
                        ;
                        local_idx < m->num_loc_vertices[m->num_leaf_levels-1];
                        local_idx++) {
                        m->vertices[local_idx].idx = local_idx;
                }
        }
	H5_RETURN (H5_SUCCESS);
}

#if defined(WITH_PARALLEL_H5GRID)
/*
 * there is a different version needed for after the refinement because
 * not all vertices need to get a glb_idx from this proc
 */

static h5_err_t
assign_global_vertex_indices_chk (
        h5t_mesh_t* const m,
        h5_loc_idxlist_t* vtx_list, // list with vertices that don't need to be assigned
        h5_glb_idx_t* vtx_range
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, vtx_list=%p, vtx_range=%p", m, vtx_list, vtx_range);
	h5_loc_idx_t local_idx = (m->leaf_level == 0) ?
	                         0 : m->num_loc_vertices[m->leaf_level-1]; // should not be 0 since only after ref...

	int counter = 0;
	for (;local_idx < m->num_loc_vertices[m->num_leaf_levels-1];
		     local_idx++) {

		h5_loc_idx_t retval = h5priv_find_in_loc_idxlist (vtx_list, local_idx);
		if (retval < 0) {
			// idx needs to be assigned.
			m->vertices[local_idx].idx = vtx_range[m->f->myproc] + counter;
			counter++;
		}
	}
	if (counter + vtx_range[m->f->myproc] != vtx_range[m->f->myproc + 1]) {
		H5_LEAVE (H5_ERR_INTERNAL);
	}

	H5_RETURN (H5_SUCCESS);
}

/*!
   Assign unique global indices to new elements.
 */
static h5_err_t
assign_glb_elem_indices_chk ( //TODO use ifdef instead of new func name
        h5t_mesh_t* const m,
        h5_glb_idx_t* range
        ) {
	H5_PRIV_FUNC_ENTER 	(h5_err_t, "m=%p, range=%p", m, range);

	h5_loc_idx_t loc_idx = (m->leaf_level == 0) ? 0 : m->num_interior_elems[m->leaf_level-1];
	int counter = 0;
	for (; loc_idx < m->num_interior_elems[m->leaf_level]; loc_idx++, counter++) {
		h5tpriv_set_loc_elem_glb_idx (m, loc_idx, range[m->f->myproc] + counter);
	}

	if (counter + range[m->f->myproc] != range[m->f->myproc + 1]) {
			H5_LEAVE (H5_ERR_INTERNAL);
		}
	H5_RETURN (H5_SUCCESS);

}


#endif

/*!
   Assign unique global indices to new elements.
 */
static h5_err_t
assign_glb_elem_indices (
        h5t_mesh_t* const m
        ) {
	/*
	   simple in serial runs: global index = local index
	 */
	h5_loc_idx_t loc_idx = (m->leaf_level == 0) ? 0 : m->num_interior_elems[m->leaf_level-1];

	for (; loc_idx < m->num_interior_elems[m->leaf_level]; loc_idx++) {
		h5tpriv_set_loc_elem_glb_idx (m, loc_idx, loc_idx);
	}

	return H5_SUCCESS;
}

h5_lvl_idx_t
h5tpriv_add_level (
        h5t_mesh_t* const m
        ) {
	H5_CORE_API_ENTER (h5_lvl_idx_t, "m=%p", m);
	CHECK_WRITABLE_MODE(m->f);

	m->leaf_level = m->num_leaf_levels++;
	m->num_loaded_levels = m->num_leaf_levels;

	TRY (m->num_glb_vertices = h5_alloc (
		     m->num_glb_vertices,
		     m->num_leaf_levels*sizeof (*m->num_glb_vertices)));
	TRY (m->num_loc_vertices = h5_alloc (
		     m->num_loc_vertices,
		     m->num_leaf_levels*sizeof (*m->num_loc_vertices)));

	TRY (m->num_b_vtx = h5_alloc (
		     m->num_b_vtx,
		     m->num_leaf_levels*sizeof (*m->num_b_vtx)));
	TRY (m->first_b_vtx = h5_alloc (
		     m->first_b_vtx,
		     m->num_leaf_levels*sizeof (*m->first_b_vtx)));

	TRY (m->num_glb_elems = h5_alloc (
		     m->num_glb_elems,
		     m->num_leaf_levels*sizeof (*m->num_glb_elems)));
	TRY (m->num_glb_leaf_elems = h5_alloc (
		     m->num_glb_leaf_elems,
		     m->num_leaf_levels*sizeof (*m->num_glb_leaf_elems)));
	TRY (m->num_interior_elems = h5_alloc (
		     m->num_interior_elems,
		     m->num_leaf_levels*sizeof (*m->num_interior_elems)));
	TRY (m->num_interior_leaf_elems = h5_alloc (
		     m->num_interior_leaf_elems,
		     m->num_leaf_levels*sizeof (*m->num_interior_leaf_elems)));
	TRY (m->num_ghost_elems = h5_alloc (
		     m->num_ghost_elems,
		     m->num_leaf_levels*sizeof (*m->num_ghost_elems)));

	m->num_glb_vertices[m->leaf_level] = -1;
	m->num_loc_vertices[m->leaf_level] = -1;

	m->num_glb_elems[m->leaf_level] = -1;
	m->num_glb_leaf_elems[m->leaf_level] = -1;
	m->num_interior_elems[m->leaf_level] = -1;
	m->num_interior_leaf_elems[m->leaf_level] = -1;
	m->num_ghost_elems[m->leaf_level] = 0;

	if (m->leaf_level == 0) {
		/* nothing stored yet */
		m->last_stored_vid = -1;
		m->last_stored_eid = -1;
		m->last_stored_vid_before_ref = -1;
		m->last_stored_eid_before_ref = -1;
	} else {
		assert (m->last_stored_vid == m->num_loc_vertices[m->leaf_level - 1] - 1);
		assert (m->last_stored_eid == m->num_interior_elems[m->leaf_level - 1] - 1);
	}

	H5_RETURN (m->leaf_level);
}

/*!
   Allocate memory for (more) vertices.
 */
h5_err_t
h5t_begin_store_vertices (
        h5t_mesh_t* const m,
        const h5_size_t num
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p, num=%llu", m, (long long unsigned)num);
	if (m->leaf_level < 0) {
		H5_LEAVE (h5tpriv_error_undef_level());
	}
	h5_size_t cur_num_loc_vertices = (m->leaf_level > 0 ?
	                                  m->num_loc_vertices[m->leaf_level-1] : 0);
	m->last_stored_vid = cur_num_loc_vertices - 1;
	m->last_stored_vid_before_ref = m->last_stored_vid;
	m->num_loc_vertices[m->leaf_level] = cur_num_loc_vertices+num;
	m->dsinfo_vertices.dims[0] = cur_num_loc_vertices+num;
	TRY (ret_value = h5tpriv_alloc_loc_vertices (m, cur_num_loc_vertices+num));
	H5_RETURN (ret_value);
}

h5_loc_idx_t
h5t_store_vertex (
        h5t_mesh_t* const m,            /*!< file handle		*/
        const h5_glb_idx_t glb_id,      /*!< global vertex id from mesher or -1	*/
        const h5_float64_t P[3]         /*!< coordinates		*/
        ) {
	H5_CORE_API_ENTER (h5_loc_idx_t,
	                   "m=%p, glb=id=%lld, P=%p",
	                   m,
	                   (long long)glb_id,
	                   P);

	// more than allocated
	if (m->last_stored_vid+1 >= m->num_loc_vertices[m->leaf_level])
		H5_LEAVE (HANDLE_H5_OVERFLOW_ERR(
		                           m->num_loc_vertices[m->leaf_level]));

	h5_loc_idx_t local_idx = ++m->last_stored_vid;
	h5_loc_vertex_t *vertex = &m->vertices[local_idx];
	vertex->idx = glb_id;     /* ID from mesher, replaced later!*/
	memcpy (&vertex->P, P, sizeof (vertex->P));
	H5_RETURN (local_idx);
}

h5_err_t
h5t_end_store_vertices (
        h5t_mesh_t* const m
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p", m);

	m->num_loc_vertices[m->leaf_level] = m->last_stored_vid+1;
	TRY (assign_global_vertex_indices (m));
	TRY (h5tpriv_rebuild_map_vertex_g2l_partial (m));
	m->last_stored_vid_before_ref = -1;
	H5_RETURN (H5_SUCCESS);
}

/*!
   Initialize everything so that we can begin to store elements.

   \param[in]	f	file handle
   \param[in]	num	number of elements to add
 */
h5_err_t
h5t_begin_store_elems (
        h5t_mesh_t* const m,
        const h5_size_t num
        ) {
	H5_CORE_API_ENTER (h5_err_t,
	                   "m=%p, num=%llu",
	                   m, (long long unsigned)num);

	h5_debug ("begin storing %llu elements", (long long)num);
	size_t cur = m->leaf_level > 0 ? m->num_interior_elems[m->leaf_level-1] : 0;
	m->last_stored_eid = cur - 1;
	size_t new = num + cur;
	m->dsinfo_elems.dims[0] = new;

	m->num_interior_elems[m->leaf_level] = new;

	m->num_interior_leaf_elems[m->leaf_level] = m->leaf_level > 0 ?
	                                            num + m->num_interior_leaf_elems[m->leaf_level-1] : num;

	if (m->leaf_level == 0) {
		TRY (m->weights = h5_calloc(m->num_weights * num, sizeof (* m->weights) ));
		if (m->num_weights < 1) {
			m->weights = NULL;
		}
	} else {
		// we don't now how many glb elems -> alloc needs to be later
	}

	m->last_stored_eid_before_ref = m->last_stored_eid;

	TRY (ret_value = h5tpriv_alloc_loc_elems (m, cur, new));
	H5_RETURN (ret_value);
}


/*!
   Store element. The vertices are given via their local indices.

   \param[in]	f			File handle.
   \param[in]	elem_idx_of_parent	Local indexd of the parent element
                                        or \c -1.
   \param[in]	vertices		Local vertex indices defining the
                                        tetrahedron.
 */
h5_loc_idx_t
h5tpriv_add_cell (
        h5t_mesh_t* const m,
        const h5_loc_idx_t parent_idx,
        const h5_loc_idx_t* vertex_indices,
        const h5_weight_t* weights
        ) {
	H5_PRIV_API_ENTER (h5_loc_idx_t,
	                   "m=%p, parent_idx=%lld, vertex_indices=%p, weights=%p",
	                   m,
	                   (long long)parent_idx,
	                   vertex_indices,
	                   weights);

	/*  more than allocated? */
	if ( m->last_stored_eid+1 >= m->num_interior_elems[m->leaf_level] )
		H5_LEAVE (
		        HANDLE_H5_OVERFLOW_ERR (m->num_interior_elems[m->leaf_level]));

	/* check parent id */
	if ((m->leaf_level == 0 && parent_idx != -1) ||
	    (m->leaf_level >  0 && parent_idx < 0) ||
	    (m->leaf_level >  0
	     && parent_idx >= m->num_interior_elems[m->leaf_level-1])
	    ) {
		H5_LEAVE (
		        HANDLE_H5_PARENT_ID_ERR (parent_idx));
	}

	/* store elem data (but neighbors) */
	h5_loc_idx_t elem_idx = ++m->last_stored_eid;
	h5tpriv_set_loc_elem_parent_idx (m, elem_idx, parent_idx);
	h5tpriv_set_loc_elem_child_idx (m, elem_idx, -1);
	h5tpriv_set_loc_elem_level_idx (m, elem_idx, m->leaf_level);

	// get ptr to local vertices store
	h5_loc_idx_t* loc_vertex_indices = h5tpriv_get_loc_elem_vertex_indices (
	        m, elem_idx);
	int num_vertices = h5tpriv_ref_elem_get_num_vertices (m);
	memcpy (loc_vertex_indices, vertex_indices,
	        sizeof (*vertex_indices)*num_vertices);


	if (m->leaf_level > 0) {
		/* add edges to map  edges -> elements */
		h5_loc_idx_t face_idx;
		int num_faces = h5tpriv_ref_elem_get_num_edges (m);
		for (face_idx = 0; face_idx < num_faces; face_idx++) {
			// add edges to neighbour struct
			TRY (h5tpriv_enter_te2 (m, face_idx, elem_idx, NULL));
		}
	}
	if (weights != NULL && m->leaf_level == 0) {
		memcpy (&m->weights[elem_idx * m->num_weights], weights, sizeof (*m->weights) * m->num_weights);
		for (int i = 0; i < m->num_weights; i++) {
			if (m->weights[elem_idx * m->num_weights + i] < 1) {
				m->weights[elem_idx * m->num_weights + i] = 1;
			}
		}
	}
	H5_RETURN (elem_idx);
}

h5_loc_idx_t
h5t_add_lvl0_cell (
        h5t_mesh_t* const m,
        const h5_loc_idx_t* vertex_indices,
        const h5_weight_t* weights
        ) {
	H5_CORE_API_ENTER (h5_loc_idx_t,
		                   "m=%p, vertex_indices=%p, weights=%p",
		                   m,
		                   vertex_indices,
		                   weights);
	h5tpriv_add_cell (m, -1, vertex_indices, weights);
	h5_loc_idx_t* loc_vertex_indices = h5tpriv_get_loc_elem_vertex_indices (
	        m, m->last_stored_eid);
	int num_vertices = h5tpriv_ref_elem_get_num_vertices (m);
	TRY (h5tpriv_sort_local_vertex_indices (m, loc_vertex_indices, num_vertices));
	H5_RETURN (m->last_stored_eid);
}
/*
   Rebuild mapping of global element indices to their local indices.
 */
static h5_err_t
rebuild_map_elem_g2l (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	if (m->num_leaf_levels <= 0) H5_LEAVE (H5_SUCCESS);

	h5_idxmap_t* map = &m->map_elem_g2l;
	h5_loc_idx_t loc_idx = m->leaf_level > 0 ? m->num_interior_elems[m->leaf_level-1] : 0;
	h5_loc_idx_t num_interior_elems = m->num_interior_elems[m->num_leaf_levels-1];

	/* (re-)alloc mem for global to local ID mapping */
	TRY (h5priv_grow_idxmap (map, num_interior_elems));

	h5_idxmap_el_t *item = &map->items[loc_idx];
	for (; loc_idx < num_interior_elems; loc_idx++, item++) {
		item->glb_idx = h5tpriv_get_loc_elem_glb_idx (m, loc_idx);
		item->loc_idx = loc_idx;
		map->num_items++;
	}
	h5priv_sort_idxmap (map);
	H5_RETURN (H5_SUCCESS);
}

#if defined(WITH_PARALLEL_H5GRID)
/*
   Rebuild mapping of global element indices to their local indices.
 */
static h5_err_t
rebuild_map_elem_g2l_partial (
	// we need that to update map for the refined elems before we have the
	// refined elements from the other proces
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	if (m->num_leaf_levels <= 0) H5_LEAVE (H5_SUCCESS);

	h5_idxmap_t* map = &m->map_elem_g2l;
	h5_loc_idx_t loc_idx = m->last_stored_eid_before_ref +1;
	h5_loc_idx_t num_interior_elems = m->last_stored_eid - m->last_stored_eid_before_ref;

	/* (re-)alloc mem for global to local ID mapping */
	TRY (h5priv_grow_idxmap (map, map->size + num_interior_elems));

	h5_idxmap_el_t *item = &map->items[loc_idx];
	for (; loc_idx <= m->last_stored_eid ; loc_idx++, item++) {
		item->glb_idx = h5tpriv_get_loc_elem_glb_idx (m, loc_idx);
		item->loc_idx = loc_idx;
		map->num_items++;
	}
	assert (map->size >= map->num_items);
	h5priv_sort_idxmap (map);
	H5_RETURN (H5_SUCCESS);
}
#endif

h5_err_t
h5t_end_store_elems (
        h5t_mesh_t* const m
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p", m);

        if (m->leaf_level == 0) {
                m->num_glb_leaf_elems[0] = m->num_glb_elems[0];
        }

	m->num_interior_elems[m->leaf_level] = m->last_stored_eid+1;
	m->num_glb_elems[m->leaf_level] = m->last_stored_eid+1;
	m->num_glb_leaf_elems[m->leaf_level] = m->num_interior_leaf_elems[m->leaf_level];
	m->last_stored_eid_before_ref = -1;

	/* assign global indices to new indices */
	TRY (assign_glb_elem_indices (m));

	/* rebuild map: global index -> local_index */
	TRY (rebuild_map_elem_g2l (m));

	/* mesh specific finalize */
	TRY (m->methods->store->end_store_elems (m));

	H5_RETURN (H5_SUCCESS);
}

#if defined(WITH_PARALLEL_H5GRID)

#if 0
/*
 * linear search trough chunks to find chk_idx which contains element
 */
static h5_err_t
find_chk_of_elem (
	h5t_mesh_t* m,
	h5_loc_id_t id,
	h5_chk_idx_t* chk_idx
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, id=%lld, chk_idx=%p", m, (long long int)id, chk_idx);
	h5_glb_idx_t glb_idx = h5tpriv_get_loc_elem_glb_idx (m, h5tpriv_get_elem_idx (id));

	// calc total num chunks
	h5_chk_idx_t total_num_chunks = 0;
	for (int i = 0; i < m->chunks->num_levels; i++) {
		total_num_chunks += m->chunks->num_chunks_p_level[i];
	}
	for (h5_chk_idx_t i = 0; i < total_num_chunks; i++) {
		if (glb_idx >= m->chunks->chunks[i].elem &&
			glb_idx < m->chunks->chunks[i].elem + m->chunks->chunks[i].num_elems) {
			// elem is contained in chunk
			*chk_idx = i;
			H5_LEAVE (H5_SUCCESS);
		}
	}
	H5_RETURN (H5_ERR_INVAL);
}
#endif

#if 0
/*
 * compare chk_idx used passed to qsort
 */
static int
compare_vtx_chk_list (
	const void *p_a,
	const void *p_b
	) {
	return ((h5t_vtx_chk_list_t*) p_a)->chk - ((h5t_vtx_chk_list_t*) p_b)->chk;
}
#endif

#ifdef CHUNKING_OF_VTX
static h5_err_t
h5tpriv_calc_vtx_permutation (
	h5t_mesh_t* m,
	h5t_vtx_chk_list_t* permut
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, permut=%p", m, permut);
	h5t_vtx_chk_list_t* b_vtx = NULL; // boundary vertices
	TRY (b_vtx = h5_calloc (m->num_loc_vertices[m->leaf_level], sizeof (*b_vtx)));
	memset (b_vtx, -1,m->num_loc_vertices[m->leaf_level]*sizeof (*b_vtx));

	h5_chk_idx_t* chk_idx_of_vtx = NULL; // allows us also to sort vtx acc to chunks
	TRY (chk_idx_of_vtx = h5_calloc (m->num_loc_vertices[m->leaf_level], sizeof (*chk_idx_of_vtx)));
	h5_loc_idx_t counter = 0;
	h5_loc_idx_t b_counter = 0;
	h5_loc_idlist_t* list = NULL;


	for (h5_loc_idx_t i = 0; i < m->num_loc_vertices[m->leaf_level]; i++) {
		// get list of elems for vertex i
		TRY (h5tpriv_find_tv3 (m, i, &list));
		h5_chk_idx_t old_chk_idx = -1;
		h5_chk_idx_t chk_idx = -1;
		int done = 0;
		for (int j = 0; j < list->num_items; j++) {
			TRY (find_chk_of_elem (m, list->items[j], &chk_idx));
			if (j == 0) {
				old_chk_idx = chk_idx;
			}
			if (old_chk_idx != chk_idx) {
				// vtx is a chunk boundary vtx
				b_vtx[b_counter++].vtx = i;
				done = 1;
				break;
			}

		}
		if (!done) {
			// vtx is a inner chunk vtx
			permut[counter].vtx = i;
			permut[counter++].chk = chk_idx;
		}
	}
	if (counter + b_counter != m->num_loc_vertices[m->leaf_level]) {
		H5_LEAVE (H5_ERR_INTERNAL);
	}
	// sort vtx acc to chunk
	qsort (permut, counter, sizeof (*permut), compare_vtx_chk_list);

	memcpy (&permut[counter], b_vtx, b_counter * sizeof (*permut));

	m->num_b_vtx[0] = b_counter;
	m->first_b_vtx[0] = counter;

	TRY (h5_free (b_vtx));
	H5_RETURN (H5_SUCCESS);
}

/*
 * calculate the reverse permutation access old_idx gives new_idx
 */
static h5_err_t
h5tpriv_calc_vtx_revpermutation (
	h5t_mesh_t* m,
	h5t_vtx_chk_list_t* permut,
	h5t_vtx_chk_list_t* rev_permut
	) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p, permut=%p, rev_permut=%p", m, permut, rev_permut);
	for (int i = 0; i < m->num_loc_vertices[m->leaf_level]; i++) {
		h5_loc_idx_t vtx = permut[i].vtx;
		rev_permut[vtx].vtx = i;
	}
	H5_RETURN (H5_SUCCESS);
}
#endif // CHUNKING_OF_VTX
#endif // WITH_PARALLEL_H5GRID

h5_err_t
h5t_end_store_ckd_elems (
        h5t_mesh_t* const m
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p", m);
	h5_debug ("end storing elements");
#ifdef WITH_PARALLEL_H5GRID
	m->num_interior_elems[m->leaf_level] = m->last_stored_eid+1;
	m->num_glb_elems[m->leaf_level] = m->last_stored_eid+1;// only works for serial case
	m->num_glb_leaf_elems[m->leaf_level] = m->num_interior_leaf_elems[m->leaf_level];

	if (m->leaf_level == 0) {
		// calculate midpoints of elements
		h5_oct_point_t* midpoints;
		TRY (midpoints = h5_calloc (m->num_glb_elems[0], sizeof (*midpoints)));
		h5_loc_idx_t curr_midp = 0;

		h5_float64_t bb[6]; // for calculating bounding box
		int num_faces = h5tpriv_ref_elem_get_num_edges (m);
		int num_vertices = h5tpriv_ref_elem_get_num_vertices (m);

		for (h5_loc_idx_t i = 0; i < m->num_glb_elems[0]; i++) {
			h5_loc_idx_t* indices; //[4] = {-1, -1, -1, -1};
			indices = h5tpriv_get_loc_elem_vertex_indices (m, i);
			h5_float64_t midpoint[3] = {0, 0, 0};
			h5_float64_t P[3];

			for (int j = 0; j < num_vertices; j++) {
				TRY (h5t_get_vertex_coords_by_index (m, indices[j], P));
				midpoint[0] += P[0];
				midpoint[1] += P[1];
				midpoint[2] += P[2];
				if (i == 0 && j == 0) {
					bb[0] = P[0];
					bb[1] = P[1];
					bb[2] = P[2];
					bb[3] = P[0];
					bb[4] = P[1];
					bb[5] = P[2];
				} else {
					bb[0] = (P[0] < bb[0]) ? P[0] : bb[0];
					bb[1] = (P[1] < bb[1]) ? P[1] : bb[1];
					bb[2] = (P[2] < bb[2]) ? P[2] : bb[2];
					bb[3] = (P[0] > bb[3]) ? P[0] : bb[3];
					bb[4] = (P[1] > bb[4]) ? P[1] : bb[4];
					bb[5] = (P[2] > bb[5]) ? P[2] : bb[5];
				}
			}
			midpoints[curr_midp].x = midpoint[0] / 3.0;
			midpoints[curr_midp].y = midpoint[1] / 3.0;
			midpoints[curr_midp].z = midpoint[2] / 3.0;
			midpoints[curr_midp].oct = -1;
			midpoints[curr_midp].elem = i;
			curr_midp++;
		}

		bb[3] += 0.1;

		bb[4] += 0.1;

		bb[5] += 0.1;

		TRY (H5t_set_bounding_box (m->octree, bb));

		TRY (H5t_set_maxpoints (m->octree, max_num_elems_p_chunk/ h5tpriv_get_num_new_elems(m)));

		TRY (H5t_refine_w_points(m->octree, midpoints, m->num_glb_elems[0], max_num_elems_p_chunk));

		TRY (H5t_add_points_to_leaf (m->octree, midpoints, m->num_glb_elems[0]));

		// set octree userlevel
		h5t_oct_iterator_t* iter = NULL;

		TRY (H5t_init_leafoct_iterator (m->octree, &iter));

		h5_oct_idx_t oct_idx;
		while ((oct_idx = H5t_iterate_oct (iter)) != -1) {
			TRY (H5t_set_userlevel (m->octree, oct_idx, 0));
		}
		TRY (H5t_end_iterate_oct (iter));

		TRY (H5t_update_internal (m->octree));
		// reorder elements
		// midpoints where already ordered according to octants (i.e. chunks)

		h5_loc_idx_t size = m->num_interior_elems[m->leaf_level];
		h5_loc_elem_t*  loc_elems = m->loc_elems;
		m->loc_elems = NULL;
		TRY (h5tpriv_alloc_loc_elems (m, 0, size));

		//TODO include ordering in subchunks
		h5_loc_idx_t* loc_vertex_indices;
		h5_loc_idx_t* old_loc_vertex_indices;
                // could get a problem is no element is added
		h5_chk_idx_t num_chunks = 1;
		h5_oct_idx_t old_idx = midpoints[0].oct;

		h5_weight_t* old_weights = m->weights;
		TRY (m->weights = h5_calloc(m->num_weights * m->num_glb_elems[0], sizeof (* m->weights) ));
		if (m->num_weights < 1) {
			m->weights = NULL;
		}
		h5t_oct_count_list_t oct_c_list;
		oct_c_list.num_items = 0;
		oct_c_list.size = size;
		TRY (oct_c_list.items = h5_calloc (size, sizeof (*oct_c_list.items)));
		oct_c_list.items[oct_c_list.num_items++].oct = midpoints[0].oct;
		int running_counter = 0;
		// copy the elements into the right order
		for (int i = 0; i < size; i++) {
			if (midpoints[i].oct != old_idx) {
				// this will be a new chunk
				num_chunks++;
				old_idx = midpoints[i].oct;
				oct_c_list.items[oct_c_list.num_items].oct = old_idx;
				oct_c_list.items[oct_c_list.num_items-1].count = i - running_counter;
				running_counter = i;
				oct_c_list.num_items++;
			}
			// permute weights
			memcpy(&m->weights[i*m->num_weights],
					&old_weights[midpoints[i].elem *m->num_weights],
					sizeof (*m->weights) * m->num_weights);

			loc_vertex_indices = h5tpriv_get_loc_elem_vertex_indices (m, i);

			old_loc_vertex_indices = h5tpriv_get_loc_elem_vertex_indices_of_array (m, midpoints[i].elem, loc_elems);

			h5tpriv_set_loc_elem_parent_idx (m, i, -1);
			h5tpriv_set_loc_elem_child_idx (m, i, -1);
			h5tpriv_set_loc_elem_level_idx (m, i, m->leaf_level);

			memcpy (loc_vertex_indices, old_loc_vertex_indices, num_vertices * sizeof (*loc_vertex_indices));

			/* add edges to map  edges -> elements */
			h5_loc_idx_t face_idx;

			for (face_idx = 0; face_idx < num_faces; face_idx++) {
			// add edges to neighbour struct
				TRY (h5tpriv_enter_te2 (m, face_idx, i, NULL));
			}

		}
		oct_c_list.items[oct_c_list.num_items-1].count = size - running_counter;

		// free loc_elems
		TRY (h5_free (loc_elems));

		// free old_weights
		TRY (h5_free (old_weights));

		// set up chunk structure
		TRY (h5tpriv_init_chunks (m));

		TRY (h5tpriv_grow_chunks (m, num_chunks));

		// create chunks
		h5_glb_idx_t elem_range[2];
		elem_range[0] = 0;
		elem_range[1] = m->num_glb_elems[m->leaf_level];
		h5_glb_idx_t chk_range[2];
		chk_range[0] = 0;
		chk_range[1] = num_chunks;
		TRY (h5tpriv_store_chunks (m, &oct_c_list, num_chunks, elem_range, chk_range));
		TRY (h5_free (oct_c_list.items));

		h5t_oct_userdata_t* userdata = NULL;
		//store userdata to chunks
		for (int j = 0; j < num_chunks; j++) {
			TRY (H5t_get_userdata_rw (m->octree, m->chunks->chunks[j].oct_idx,(void **) &userdata));
			userdata->idx[0] = (h5_chk_idx_t) j;
		}

		TRY (H5t_update_userdata (m->octree));
		TRY (h5_free (midpoints));
	}



	/* assign global indices to new indices */
	TRY (assign_glb_elem_indices (m));

	/* rebuild map: global index -> local_index */
	TRY (rebuild_map_elem_g2l (m));

	/* mesh specific finalize */
	TRY (m->methods->store->end_store_elems (m));


#ifdef CHUNKING_OF_VTX
	// abbandon chunking of vtx for time reasons
	if (m->leaf_level == 0) {
		// sort vertices
		//calculate permutation
		h5t_vtx_chk_list_t* permut = NULL;
		TRY (permut = h5_calloc (m->num_loc_vertices[m->leaf_level], sizeof (*permut)));
		memset (permut, -1,m->num_loc_vertices[m->leaf_level]*sizeof (*permut));
		TRY (h5tpriv_calc_vtx_permutation (m, permut));

		h5t_vtx_chk_list_t* rev_permut = NULL; // here is the reverse permutation
		TRY (rev_permut = h5_calloc (m->num_loc_vertices[m->leaf_level], sizeof (*rev_permut)));

		TRY (h5tpriv_calc_vtx_permutation (m, permut));

		TRY (h5tpriv_calc_vtx_revpermutation (m, permut, rev_permut));

		// permute vertices
		h5_loc_vertex_t* vertices = m->vertices;
		m->vertices = NULL;
		TRY (h5tpriv_alloc_loc_vertices (m, *m->num_loc_vertices));
		for (h5_loc_idx_t i = 0; i < *m->num_loc_vertices; i++) {
			memcpy (&m->vertices[i], &vertices[permut[i].vtx], sizeof (*vertices));
		}
		TRY (h5_free (vertices));
		TRY (assign_global_vertex_indices (m));
		TRY (h5_free (m->map_vertex_g2l.items));
		m->map_vertex_g2l.items = NULL;
		size_t size = m->num_loc_vertices[m->leaf_level]  + 128;
		TRY (h5priv_new_idxmap (&m->map_vertex_g2l, size));
		TRY (h5tpriv_rebuild_map_vertex_g2l (m, m->leaf_level, m->leaf_level));
		m->last_stored_vid_before_ref = -1;
		// update elements

		// permute vertex_indices
		for (h5_loc_idx_t i = 0; i < *m->num_glb_elems; i++) {
			h5_loc_idx_t* vertex_indices = NULL;
			int num_vtx = h5tpriv_ref_elem_get_num_vertices(m);
			vertex_indices = h5tpriv_get_loc_elem_vertex_indices (m, i);
			for (int j = 0; j < num_vtx; j++) {
				vertex_indices[j] = rev_permut[vertex_indices[j]].vtx;
			}
			TRY (h5tpriv_sort_local_vertex_indices (m, vertex_indices, num_vtx));

			// TODO is this necessary or already done with end store elems
			/* add edges to map  edges -> elements */
			h5_loc_idx_t face_idx;
			int num_faces = h5tpriv_ref_elem_get_num_edges (m);
			for (face_idx = 0; face_idx < num_faces; face_idx++) {
				// add edges to neighbour struct
				TRY (h5tpriv_enter_te2 (m, face_idx, i, NULL));
			}

		}
		// store vtx info to chunks
		TRY (h5tpriv_store_vtx_range_to_chk (m, permut));
//		/* mesh specific finalize */
//		TRY (m->methods->store->end_store_elems (m));
	}
#endif
#endif
	H5_RETURN (H5_SUCCESS);
}

#ifdef WITH_PARALLEL_H5GRID
h5_err_t
h5tpriv_find_oct_proc_of_point (
		h5t_mesh_t* m,
		h5_loc_idx_t loc_idx,
		h5_oct_point_t* point,
		h5_int32_t* proc
		) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p, loc_idx=%lld, point=%p, proc=%d", m,  (long long int)loc_idx, point,*proc);
	h5_loc_idx_t* indices = h5tpriv_get_loc_elem_vertex_indices (m, loc_idx);
	h5_float64_t midpoint[3] = {0, 0, 0};
	h5_float64_t P[3];
	int num_vertices = h5tpriv_ref_elem_get_num_vertices (m);

	for (int j = 0; j < num_vertices; j++) {
		TRY (h5t_get_vertex_coords_by_index (m, indices[j], P));
		midpoint[0] += P[0];
		midpoint[1] += P[1];
		midpoint[2] += P[2];
	}
	point->x = midpoint[0] / ((double) num_vertices);
	point->y = midpoint[1] / ((double) num_vertices);
	point->z = midpoint[2] / ((double) num_vertices);

	point->elem = -1;
	// check in which octant the new elems would be
	TRY (point->oct = H5t_find_leafoctant_of_point (m->octree, 0, H5t_get_bounding_box (m->octree), point));
	// get proc of octant
	*proc = H5t_get_proc (m->octree, point->oct);

	H5_RETURN (H5_SUCCESS);
}

static int
compare_midpoint_oct (
	const void *p_a,
	const void *p_b
	) {
	return ((h5_oct_point_t*) p_a)->oct - ((h5_oct_point_t*) p_b)->oct;
}
/*
 * Update the marked_entities list such that it contains all elements that are
 * going to be refined with the current proc
 */
h5_err_t
h5tpriv_mark_chk_elems_to_refine (
	h5t_mesh_t* const m,
	h5_glb_idxlist_t* glb_list,
	h5_oct_point_t* midpoint_list
	) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p, glb_list=%p", m, glb_list);
	// clear marked_entities list
	TRY (h5priv_free_loc_idlist (&m->marked_entities));
	TRY (h5priv_alloc_loc_idlist (&m->marked_entities, MAX_NUM_ELEMS_TO_REFINE_LOCALLY));

	h5_glb_idx_t glb_idx = -1;
	h5_loc_idx_t loc_idx = -1;
	h5_oct_point_t point;
	int counter = 0;
	// go through all elems
	for (int i = 0; i < glb_list->num_items; i++) {
		glb_idx = glb_list->items[i];

		// check if element is locally available (if not some other proc needs to refine it)
		loc_idx = h5t_map_glb_elem_idx2loc(m, glb_idx);
		if (loc_idx >= 0) {
			// check in which octant the element is
			h5_int32_t proc = -1;
			TRY (h5tpriv_find_oct_proc_of_point (m, loc_idx, &point, &proc));  //TODO maybe use hash table here!!!

			if (proc == m->f->myproc &&										// needs to be in my octant
					h5priv_find_in_loc_idlist(m->marked_entities, loc_idx) < 0 && // not already in list
					h5tpriv_get_loc_elem_child_idx (m, loc_idx) == -1) {  	// not refined already
				// element is in octant of this proc, add to marked list
				TRY (h5priv_search_in_loc_idlist (&m->marked_entities, loc_idx));

				midpoint_list[counter].x = point.x;
				midpoint_list[counter].y = point.y;
				midpoint_list[counter].z = point.z;
				midpoint_list[counter].oct = point.oct;
				midpoint_list[counter].elem = h5tpriv_get_loc_elem_glb_idx(m, loc_idx);
				counter++;
			}
		}
	}
	// sort midpoint list such that they are aligned according to octants
	qsort (midpoint_list, counter, sizeof (*midpoint_list), compare_midpoint_oct);
	H5_RETURN (H5_SUCCESS);
}
#endif
/*
   Mark entity for further processing (e.g. refinement).
 */
h5_err_t
h5t_mark_entity (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p, entity_id=%llu",
	                   m, (long long unsigned)entity_id);
	TRY (ret_value = h5priv_insert_into_loc_idlist (
		     &m->marked_entities, entity_id, -1));
	H5_RETURN (ret_value);
}

h5_err_t
h5t_pre_refine (
        h5t_mesh_t* const m
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p", m);
	H5_RETURN (m->methods->store->pre_refine (m));
}
#ifdef WITH_PARALLEL_H5GRID
//TODO maybe use ifdef to have name without _chk
h5_err_t
h5t_pre_refine_chk (
        h5t_mesh_t* const m,
        h5_glb_idxlist_t** glb_list,
        h5_oct_point_t** point_list
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p", m);
	// exchange list of marked entities
	TRY (*point_list = h5_calloc (
		     // alloc for maximal num elems to refine
		     m->num_glb_leaf_elems[m->leaf_level-1], sizeof (**point_list))); 

	TRY (h5priv_exchange_loc_list_to_glb (m, glb_list));
	h5_glb_idxlist_t* glb_marked_entities = *glb_list;

	// decide which elements this proc has to refine
	TRY (h5tpriv_mark_chk_elems_to_refine (m, glb_marked_entities, *point_list));

	/*
	  TODO maybe check that sum of m->marked_entities->num_items over
	  all proc is equal to glb_marked_entities->num_items this would
	  find out if there is a problem with loading neighboring chunks...
	*/
	  H5_RETURN (m->methods->store->pre_refine (m));
}
#endif

/*
   Refine previously marked elements.
 */
h5_err_t
h5t_refine_marked_elems (
        h5t_mesh_t* const m
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p", m);
	int i;
	for (i = 0; i < m->marked_entities->num_items; i++) {
		TRY (h5tpriv_refine_elem (m, m->marked_entities->items[i]));
	}
	H5_RETURN (H5_SUCCESS);
}

#ifdef WITH_PARALLEL_H5GRID
/*
 * Calculate the global entity range
 * range[i] = first glb entity of proc i
 * range[nproc] = next idx to assign
 */
h5_err_t
h5tpriv_get_ranges (
	h5t_mesh_t* const m,
	h5_glb_idx_t* range,
	h5_glb_idx_t mycount,
	h5_glb_idx_t glb_start
	) {
	H5_PRIV_API_ENTER (h5_err_t,
			    "m=%p, range=%p, mycount=%lld, glb_start=%lld",
			    m, range, (long long int) mycount, (long long int) glb_start);

	TRY (h5priv_mpi_allgather (
			&mycount,
			1,
			MPI_LONG,
			&range[1],
			1,
			MPI_LONG,
			m->f->props->comm));

	range[0] = glb_start;

	for (int i = 1; i <= m->f->nprocs; i++) {
		range[i] += range[i-1];
	}
	H5_RETURN (H5_SUCCESS);
}

/*
 * Calculate the global element range for the new elements
 * range[i] = first glb element of proc i
 * range[nproc] = next idx to assign
 */
h5_err_t
h5tpriv_get_elem_ranges (
	h5t_mesh_t* const m,
	h5_glb_idx_t* range
	) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p, range=%p", m, range);
	h5_glb_idx_t sendbuf = m->marked_entities->num_items * h5tpriv_get_num_new_elems (m);
	TRY (h5priv_mpi_allgather (
			&sendbuf,
			1,
			MPI_LONG,
			&range[1],
			1,
			MPI_LONG,
			m->f->props->comm));

	range[0] = m->leaf_level > 0 ? m->num_glb_elems[m->leaf_level-1] : 0; // TODo check if correct

	for (int i = 1; i <= m->f->nprocs; i++) {
		range[i] += range[i-1];
	}
	H5_RETURN (H5_SUCCESS);
}

#if 0
/*
 * Check if edge is on proc border. If yes also check that it hasn't been refined yet and the other proc
 * is also refining his element.
 */
static int
check_edge (
	h5t_mesh_t* const m,
	const h5_loc_idx_t face_idx,
	const h5_loc_idx_t elem_idx,
	h5_glb_idxlist_t* glb_elems
	) {
	H5_PRIV_FUNC_ENTER (h5_loc_idx_t,
			    "m=%p, face_idx=%lld, elem_idx=%lld, glb_elems=%p",
			    m, (long long)face_idx, (long long)elem_idx, glb_elems);
	h5_loc_idlist_t* retval;
		// get all elements sharing the given edge
		TRY (h5tpriv_find_te2 (m, face_idx, elem_idx, &retval));

		size_t i;
		for (i = 0; i < retval->num_items; i++) {
			// check weather the found element has been refined
			h5_loc_id_t kids[2] = {-1,-1};
			TRY (h5tpriv_get_loc_entity_children (m, retval->items[i], kids));
			if (kids[0] >= 0) {
				// element has been refined
				H5_LEAVE (0);
			}
			h5_loc_id_t el_id1 = retval->items[i];
			h5_chk_idx_t chk_idx1 = -1;
			h5_loc_id_t el_id2 = h5tpriv_build_entity_id(
					h5tpriv_ref_elem_get_entity_type(m, h5tpriv_ref_elem_get_dim (m) -1),
					face_idx,
					elem_idx );
			h5_chk_idx_t chk_idx2 = -2;
			TRY (find_chk_of_elem (m, el_id1, &chk_idx1));
			TRY (find_chk_of_elem (m, el_id2, &chk_idx2));
			if (chk_idx1 == chk_idx2) {
				H5_LEAVE (0);
			}
//			// BUG this works depending on what is my_proc interior elems or interior_chunks
////			// check weather the found element is owned by the same proc
////			if (m->loc_elems[elem_idx].my_proc == m->loc_elems[retval->items[i]].my_proc) {
////				// element is on same proc
////
////			}
//			// check if it is going to be refined
//			if (h5priv_find_in_glb_idxlist(glb_elems, m->loc_elems[h5tpriv_get_elem_idx(retval->items[i])].glb_idx) < 0 ) {
//				// element is not going to be refined
//				H5_LEAVE (0);
//			}
//			// check if it is on this proc
//			if (h5priv_find_in_loc_idlist(m->marked_entities, retval->items[i]) >= 0  ) {
//				// element is on same proc
//				H5_LEAVE (0);
//			}
		}
	H5_RETURN (1);
}
#endif

static h5_loc_idx_t
get_new_vtx_of_edge(
	h5t_mesh_t* const m,
	h5_loc_id_t loc_id
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, loc_id=%lld",
				m,  (long long int)loc_id);

	h5_loc_id_t kids[2] = {-1,-1};
	TRY (h5tpriv_get_loc_entity_children (m, loc_id, kids));
	if (kids[0] >= 0) {
		// element has been refined, return bisecting point
		h5_loc_idx_t edge0[2], edge1[2];
		TRY (h5t_get_loc_vertex_indices_of_edge (m, kids[0], edge0));
		TRY (h5t_get_loc_vertex_indices_of_edge (m, kids[1], edge1));
		if ((edge0[0] == edge1[0]) || (edge0[0] == edge1[1])) {
			H5_LEAVE (edge0[0]);
		} else {
			H5_LEAVE (edge0[1]);
		}
	}
	H5_RETURN (H5_ERR_INTERNAL); //edge that should be refined in not refined
}

/*
 * Go through elements and find boundary edges that were refined on this proc.
 * we try to find edges that are shared with non-local elements (they could have
 * been refined) or if one of the local elements was refined on a different proc
 */
h5_err_t
h5tpriv_find_boundary_edges ( // todo maybe put some part into another function...
	h5t_mesh_t* const m,
	h5_glb_idxlist_t* glb_elems,
	h5_edge_list_t* list
	) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p, glb_elems=%p, list=%p",
			m, glb_elems, list);
	// go through marked elements
		h5_loc_idx_t elem_idx = -1;
	for (int i = 0; i < m->marked_entities->num_items; i++) {
		elem_idx = h5tpriv_get_elem_idx( m->marked_entities->items[i]);
		int num_faces = h5tpriv_ref_elem_get_num_facets(m);
		for (int j = 0; j < num_faces; j++) {
			h5_loc_idlist_t* retval = NULL;
			// get all elements sharing the given edge
			TRY (h5tpriv_find_te2 (m, j, elem_idx, &retval));

			/*
			  check if it is a border edge
			  TODO does not work yet since flags are not set
			  properly but as long as we have all surounding
			  elems it's not a problem -> i.e. tetrahedrals
			*/
			if (retval->flags == H5_BORDER_ENTITY && 0) {
				// add to edgelist
				h5_glb_idx_t vertices[2];
				h5t_get_glb_vertex_indices_of_entity (m, retval->items[0], vertices);

				// we need to get the edge_id of elem_idx (other elems may not be refined
				// and therefore can not return the splitting vertex
				int l = 0;
				for (; l < retval->num_items; l++) {
					if (elem_idx == h5tpriv_get_elem_idx (retval->items[l])) {
						break;
					}
				}
				assert (l < retval->num_items);
				h5_loc_idx_t loc_new_vtx = get_new_vtx_of_edge(m, retval->items[l]);
				assert (loc_new_vtx > -1);

				TRY (h5tpriv_add_edge_list (
					     list ,vertices[0], vertices[1], loc_new_vtx, m->f->myproc));
				continue;
			}
			// check if one of the neighbors (locally available) was refined on a different proc
			for (int k = 0; k < retval->num_items; k++) {
				h5_loc_idx_t neigh_idx = h5tpriv_get_elem_idx (retval->items[k]);
				if (neigh_idx == elem_idx) {
					continue;
				}
				h5_glb_idx_t neigh_glb_idx = h5tpriv_get_loc_elem_glb_idx(m, neigh_idx);
				h5_loc_idx_t idx = h5priv_find_in_glb_idxlist(glb_elems, neigh_glb_idx);
				if (idx < 0) {
					// element has not been refined
					continue;
				} else {
					// check if it was refined on this proc
					int proc = -1;
					h5_oct_point_t point;
					TRY (h5tpriv_find_oct_proc_of_point (m, neigh_idx, &point, &proc));
					if (m->f->myproc != proc) {
						// element was refined on different proc
						// add to edgelist
						h5_glb_idx_t vertices[2];
						h5t_get_glb_vertex_indices_of_entity (m, retval->items[k], vertices);

						// we need to get the edge_id of elem_idx (other elems may not be refined
						// and therefore can not return the splitting vertex
						int l = 0;
						for (; l < retval->num_items; l++) {
							if (elem_idx == h5tpriv_get_elem_idx (retval->items[l])) {
								break;
							}
						}
						assert (l < retval->num_items);
						h5_loc_idx_t loc_new_vtx = get_new_vtx_of_edge(m, retval->items[l]);
						assert (loc_new_vtx > -1);
						TRY (h5tpriv_add_edge_list (list ,vertices[0], vertices[1],loc_new_vtx, m->f->myproc));
						break;
					}
				}

			}
		}
	}
	// sort & uniquify
	TRY (h5tpriv_sort_edge_list (list));
	TRY (h5tpriv_uniquify_edge_list (list));

	H5_RETURN (H5_SUCCESS);
}

/*
 * exchange boundary edges info
 */
static h5_err_t
exchange_boundary_edge_list (
	h5t_mesh_t* const m,
	h5_edge_list_t* b_edges,
	h5_edge_list_t* glb_b_edges
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m); // TODO

	int* recvcounts = NULL;
	TRY (recvcounts = h5_calloc (m->f->nprocs, sizeof (*recvcounts)));
	int* recvdisp = NULL;
	TRY (recvdisp = h5_calloc (m->f->nprocs + 1, sizeof (*recvdisp)));
	TRY (h5priv_mpi_allgather (
			&b_edges->num_items,
			1,
			MPI_INT,
			recvcounts,
			1,
			MPI_INT,
			m->f->props->comm));
	int tot_num_b_edges = 0;
	recvdisp[0] = 0;
	for (int i = 0; i < m->f->nprocs; i++) {
		tot_num_b_edges += recvcounts[i];
		recvdisp[i+1] = recvcounts[i] + recvdisp[i];
	}
	if (tot_num_b_edges > 0) {
		TRY (h5tpriv_grow_edge_list (glb_b_edges, tot_num_b_edges));

		TRY (h5priv_mpi_allgatherv (
			b_edges->items,
			b_edges->num_items,
			h5_dta_types.mpi_edge_list_elem,
			glb_b_edges->items,
			recvcounts,
			recvdisp,
			h5_dta_types.mpi_edge_list_elem,
			m->f->props->comm));

		glb_b_edges->num_items = tot_num_b_edges;
		h5tpriv_sort_edge_list(glb_b_edges);
	}
	TRY (h5_free(recvcounts));
	TRY (h5_free(recvdisp));

	H5_RETURN (H5_SUCCESS);
}
static h5_err_t
set_exchanged_glb_idx (
	h5t_mesh_t* const m,
	h5_edge_list_t* list,
	h5_edge_list_t* glb_list
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m); // TODO

	for (int i = 0; i < list->num_items; i++) {
		if ( list->items[i].proc != m->f->myproc) {
			h5_int32_t retval = h5tpriv_find_edge_list(glb_list,&list->items[i]);
			assert (retval != glb_list->num_items);
			m->vertices[list->items[i].new_vtx].idx = glb_list->items[retval].new_vtx;
		}
	}
	H5_RETURN (H5_SUCCESS);
}
/*
 * find local edges in glb list and find out which proc sets glb_idx
 */
static h5_err_t
find_edges_in_boundary_edge_list (
		h5_edge_list_t* list,
		h5_edge_list_t* glb_list
		) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "list=%p, glb_list=%p", list, glb_list);
	h5_int32_t idx = -1;
	for (int i = 0; i < list->num_items; i++) {
		h5t_edge_list_elem_t* retval = bsearch (&list->items[i], glb_list->items,glb_list->num_items, sizeof(*list->items), compare_edge_list_elem );
		assert (retval != NULL); //all items in list are copied from glb_list so retval can't be NULL

		idx =(int) (retval - glb_list->items);
		// if there was another proc with lower rank that refined the same edge, the edge would lie at position idx - 1
		// so we try to find lowest proc that has refined edge i
		h5t_edge_list_elem_t edge;
		edge.vtx1 = list->items[i].vtx1;
		edge.vtx2 = list->items[i].vtx2;
		edge.proc = 0;
		while (idx > 0 &&
				compare_edge_list_elem (&glb_list->items[idx-1], &edge) >= 0) {
			idx--;
		}
		list->items[i].proc = glb_list->items[idx].proc;
	}
	H5_RETURN (H5_SUCCESS);
}
/*
 * set glb_idx of new vertex into edge list -> will be exchanged to other procs
 */
static h5_err_t
set_glb_idx_edge_list (
		h5t_mesh_t* m,
		h5_edge_list_t* list
		) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, list=%p",m ,list);
	for (int i = 0; i < list->num_items; i++) {
		if (list->items[i].proc == m->f->myproc) {
			list->items[i].new_vtx = m->vertices[list->items[i].new_vtx].idx;
			assert (list->items[i].new_vtx != -1);
		}
	}
	H5_RETURN (H5_SUCCESS);
}
/*
 * at the moment either split weights equally to children or assign equal as parent
 */
static h5_err_t
update_weight_children (
		h5t_mesh_t* m,
		h5_weight_t* parent_weight,
		h5_weight_t* children_weight) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p",m );
	int num_new_elems =  h5tpriv_get_num_new_elems (m);

	if (UPDATE_WEIGHTS == 1) { // split
		for (int j = 0; j < m->num_weights; j++) {
			children_weight[j] = MAX (1, parent_weight[j]/num_new_elems);
		}

	}
	if (UPDATE_WEIGHTS == 2) { // copy
		for (int j = 0; j < m->num_weights; j++) {
			children_weight[j] = parent_weight[j];
		}
	}

	H5_RETURN (H5_SUCCESS);
}

/*
 * function to set weights after refinement automatically
 */
static h5_err_t
set_local_weights (
	h5t_mesh_t* m,
	h5_glb_idx_t* range
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, range=%p",m ,range);

	for (h5_glb_idx_t idx = range[m->f->myproc]; idx < range[m->f->myproc + 1]; idx++) {
		// get loc_idx of elem
		h5_loc_idx_t loc_idx = h5t_map_glb_elem_idx2loc (m, idx);
		assert (loc_idx >= 0);
		h5_loc_idx_t parent_idx = h5tpriv_get_loc_elem_parent_idx (m, loc_idx);
		h5_glb_idx_t parent_glb_idx = h5tpriv_get_loc_elem_glb_idx (m, parent_idx);
		h5_weight_t* parent_weight = NULL;
		h5_weight_t* children_weight = NULL;
		parent_weight = &m->weights[parent_glb_idx * m->num_weights];
		children_weight = &m->weights[idx * m->num_weights];
		TRY (update_weight_children (m, parent_weight, children_weight));
	}


	H5_RETURN (H5_SUCCESS);
}
/*
 * function to update weights after refinement
 */
static h5_err_t
exchange_weights (
	h5t_mesh_t* m,
	h5_glb_idx_t* range
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, range=%p",m ,range);

	int* recvcounts = h5_calloc (m->f->nprocs, sizeof (* recvcounts));
	int* recvdisp = h5_calloc (m->f->nprocs, sizeof (* recvdisp));

	for (int i = 0; i < m->f->nprocs; i++) {
			recvdisp[i] = (int) range[i] * m->num_weights;
			recvcounts[i] = (int) (range[i+1]- range[i]) * m->num_weights;

	}

	int sendcount = (range[m->f->myproc + 1] - range[m->f->myproc]) * m->num_weights;
	h5_weight_t* sendbuf  = h5_calloc (sendcount, sizeof (*sendbuf));

	memcpy (sendbuf, &m->weights[range[m->f->myproc] * m->num_weights], sendcount * sizeof (*sendbuf));

	TRY (h5priv_mpi_allgatherv (
				sendbuf,
				sendcount,
				MPI_INT,
				m->weights,
				recvcounts,
				recvdisp,
				MPI_INT,
				m->f->props->comm));
	for (h5_glb_idx_t i = 0; i < range[m->f->nprocs] * m->num_weights; i++ ) {
		assert (m->weights[i] > 0);
	}
	H5_RETURN (H5_SUCCESS);
}

/*
   Refine previously marked elements.
 */
h5_err_t
h5t_refine_marked_elems_chk (
        h5t_mesh_t* const m,
        h5_glb_idxlist_t* glb_elems,
        h5_oct_point_t* midpoints
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p", m);
	int num_midpoints = m->marked_entities->num_items;

	// refine octree
	TRY (H5t_refine_w_points (m->octree, midpoints, num_midpoints, H5t_get_maxpoints (m->octree)));

	// sort midpoint list such that they are aligned according to octants
	qsort (midpoints, num_midpoints, sizeof (*midpoints), compare_midpoint_oct);

	// set octree userlevel
	h5t_oct_iterator_t* iter = NULL;

	TRY (H5t_init_leafoct_iterator (m->octree, &iter));

	h5_oct_idx_t oct_idx;
	while ((oct_idx = H5t_iterate_oct (iter)) != -1) {
		if (H5t_get_proc (m->octree, oct_idx) == m->f->myproc) {
			TRY (H5t_set_userlevel (m->octree, oct_idx, m->leaf_level));
		}
	}
	TRY (H5t_end_iterate_oct (iter));
	TRY (H5t_update_internal (m->octree));

	// get elem ranges
	h5_glb_idx_t* elem_range = NULL;
	TRY (elem_range = h5_calloc (m->f->nprocs + 1, sizeof (*elem_range)));

	//TODO use generic get_range function
	TRY (h5tpriv_get_elem_ranges (m, elem_range));

	// CHUNKS
  	h5_chk_idx_t num_chunks = 1;

	h5t_oct_count_list_t oct_c_list; // list has contains all octants and number of elems per octant
	oct_c_list.num_items = 0;
	oct_c_list.size = num_midpoints;
	TRY (oct_c_list.items = h5_calloc (num_midpoints, sizeof (*oct_c_list.items)));

  	h5_oct_idx_t old_idx = -1;
  	if (num_midpoints > 0) {
  		old_idx = midpoints[0].oct;
  		oct_c_list.items[oct_c_list.num_items++].oct = old_idx;
  	} else {
  		num_chunks = 0;
  	}


	int running_counter = 0;
	// calc number of chunks
	for (int i = 0; i < num_midpoints; i++) {
		if (midpoints[i].oct != old_idx) {
			// point i will be in a new chunk
			num_chunks++;
			old_idx = midpoints[i].oct;
			oct_c_list.items[oct_c_list.num_items].oct = old_idx;
			oct_c_list.items[oct_c_list.num_items-1].count = i - running_counter;
			running_counter = i;
			oct_c_list.num_items++;
		}
	}
	oct_c_list.items[oct_c_list.num_items-1].count = num_midpoints - running_counter;
	// calc chunk range
	h5_glb_idx_t* chk_range = NULL;
	TRY (chk_range = h5_calloc (m->f->nprocs + 1, sizeof (*chk_range)));
	TRY (h5tpriv_get_ranges (m, chk_range, num_chunks, m->chunks->curr_idx + 1));

	// get total number of chunks
	int tot_num_chunks = 0;

	tot_num_chunks = chk_range[m->f->nprocs] - chk_range[0];
	// alloc mem for chunks
	TRY (h5tpriv_grow_chunks (m, tot_num_chunks));

	// create chunks
	TRY (h5tpriv_store_chunks (m, &oct_c_list, num_chunks, elem_range, chk_range));

	// update newly created chunks
	TRY (h5tpriv_update_chunks (m, chk_range));

	h5t_oct_userdata_t* userdata = NULL;
	//store userdata to octree
	for (int j = chk_range[m->f->myproc]; j < chk_range[m->f->myproc + 1]; j++) {
		assert ( H5t_get_proc (m->octree, m->chunks->chunks[j].oct_idx) == m->f->myproc);
		TRY (H5t_get_userdata_rw (m->octree, m->chunks->chunks[j].oct_idx,(void **) &userdata));
		if (userdata->idx[0] == -1) {
			userdata->idx[0] = (h5_chk_idx_t) j;
		} else if (userdata->idx[1] == -1) {
			userdata->idx[1] = (h5_chk_idx_t) j;
		} else if (userdata->idx[2] == -1) {
			userdata->idx[2] = (h5_chk_idx_t) j;
		} else if (userdata->idx[3] == -1) {
			userdata->idx[3] = (h5_chk_idx_t) j;
		} else {
			H5_LEAVE (H5_ERR_INTERNAL);
		}
	}

	TRY (H5t_update_userdata (m->octree));


	// refine elements
	for (int i = 0; i < num_midpoints; i++) {
                // needs to be ordered acc to octants
		TRY (h5tpriv_refine_elem (m, h5t_map_glb_elem_idx2loc(m,midpoints[i].elem)));
	}
	TRY (h5_free (oct_c_list.items));
	TRY (h5_free (elem_range));
	TRY (h5_free (chk_range));
	H5_RETURN (H5_SUCCESS);
}
/*
 * This function checks if there is the possibility to add another chunk to an octant
 */
int
h5tpriv_octant_is_full (
	h5t_octree_t* octree,
	h5_oct_idx_t oct_idx
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d", octree, oct_idx);
	h5t_oct_userdata_t* userdata = NULL;
	TRY (H5t_get_userdata_r (octree, oct_idx,(void **) &userdata));
	if (userdata->idx[3] == -1) {
		H5_LEAVE (0)
	}
	H5_RETURN (1);
}

/*
 * only compares the glb_idx of a vertex
 */
int compare_glb_vertex (const void *p_a, const void *p_b) {
	return ((h5_glb_vertex_t*) p_a)->idx - ((h5_glb_vertex_t*) p_b)->idx;
}
/*
 * find vertex in list and return num_vtx if not in list
 * list needs to be sorted
 */
static int
h5tpriv_find_vertex_in_list (
		h5t_mesh_t* const m,
		h5_glb_idx_t  vtx_idx,
		h5_glb_vertex_t* vtx_list,
        int num_vtx
	) {
	h5_glb_vertex_t key;
	key.idx = vtx_idx;

	h5_glb_vertex_t* retval = bsearch (&key, vtx_list, num_vtx, sizeof (*retval), compare_glb_vertex);
	if (retval == NULL) {
		return num_vtx;
	}
	return retval - vtx_list;
}
static h5_err_t
h5tpriv_sort_vertex_list (
		h5_glb_vertex_t* vtx_list,
		int num_vtx
		) {
	qsort (vtx_list, num_vtx, sizeof(*vtx_list), compare_glb_vertex);
	return (H5_SUCCESS);
}



int comp_vtx_coord (void* p_a, void* p_b) {
	h5_glb_vertex_t* a = (h5_glb_vertex_t*)p_a;
	h5_glb_vertex_t* b = (h5_glb_vertex_t*)p_b;

	if (a->idx != b->idx) {
		return a->idx != b->idx;
	} else {
		if (a->P[0] != b->P[0]) {
			return a->P[0] != b->P[0];
		} else {
			if (a->P[1] != b->P[1]) {
				return a->P[1] != b->P[1];
			} else {
				if (a->P[2] != b->P[2]) {
					return a->P[2] != b->P[2];
				} else {
					return 0;
				}
			}
		}
	}
}

static h5_err_t
add_glb_vertex_to_list (
        h5t_mesh_t* const m,
        h5_glb_idx_t  vtx_idx,
        h5_glb_vertex_t* glb_vtx,
        h5_glb_idx_t num_glb_vtx,
        h5_glb_vertex_t* vtx_list,
        int* num_vtx
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	h5_loc_idx_t loc_idx = h5tpriv_find_vertex_in_list (m, vtx_idx, glb_vtx, num_glb_vtx);
	assert (loc_idx > -1 );
	assert (loc_idx < num_glb_vtx);
	memcpy (&vtx_list[*num_vtx], &glb_vtx[loc_idx], sizeof (*vtx_list));
	(*num_vtx)++;
	H5_RETURN (H5_SUCCESS);
}


static h5_err_t
init_glb_vtx_struct_chk (
        h5t_mesh_t* const m,
        h5_glb_elem_t*  glb_elems,
        int num_glb_elems,
        h5_glb_vertex_t* vtx_list,
        int* num_vtx
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	// todo could be optimised by using a hashtab for glb_idx_list instead of map
	int num_vertices = h5tpriv_ref_elem_get_num_vertices(m);

	h5_idxmap_t map_s;
	TRY (h5priv_new_idxmap (&map_s, num_vertices * num_glb_elems));
	h5_idxmap_t* map = &map_s;

	h5_hashtable_t htab;
	TRY (h5priv_hcreate (((num_vertices * num_glb_elems) << 2) / 3, &htab,
		           hidxmap_cmp, hidxmap_compute_hval, NULL));

	for (int i = 0; i < num_glb_elems; i++) {
		h5_glb_idx_t* vtx_idx = h5tpriv_get_glb_elem_vertices(m, glb_elems, i);

		for (int j = 0; j < num_vertices; j++) {
			// add index temporarly to map ...
				map->items[map->num_items] = (h5_idxmap_el_t) {vtx_idx[j], 0};
				// ... and check whether it has already been added
				h5_idxmap_el_t* retval;
				h5priv_hsearch (&map->items[map->num_items],
				                H5_ENTER, (void**)&retval, &htab);

			if (retval == &map->items[map->num_items]) { // not in list
				// new entry in hash table thus in map
				map->num_items++;
				//todo optimise by copy in the end (copy consequtive vtx together)
				h5_loc_idx_t loc_idx = h5t_map_global_vertex_idx2local (m, vtx_idx[j]);
				assert (loc_idx > -1 && loc_idx <= m->last_stored_vid);
				memcpy (&vtx_list[*num_vtx], &m->vertices[loc_idx], sizeof (*vtx_list));
				(*num_vtx)++;
			}
		}
	}
	TRY (h5priv_hdestroy (&htab));
	TRY (h5_free (map->items));
	TRY (h5tpriv_sort_vertex_list (vtx_list, *num_vtx));
	H5_RETURN (H5_SUCCESS);
}
// little bit different funtion since it's used to store and not to load...
static h5_err_t
init_glb_vtx_struct_chk2 (
        h5t_mesh_t* const m,
        h5_glb_elem_t*  glb_elems,
        int num_glb_elems,
        h5_glb_vertex_t* glb_vtx,
        int num_glb_vtx,
        h5_glb_vertex_t* vtx_list,
        int* num_vtx
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	// todo could be optimised by using a hashtab for glb_idx_list instead of map
	int num_vertices = h5tpriv_ref_elem_get_num_vertices(m);
	h5_idxmap_t* map = &m->map_vertex_g2l;
	if (map->size <= map->num_items) {
		h5priv_grow_idxmap (map, map->size +10); // 1 should be enough
	}


	h5_hashtable_t htab;
	TRY (h5priv_hcreate (((num_vertices * num_glb_elems) << 2) / 3, &htab,
		           hidxmap_cmp, hidxmap_compute_hval, NULL));

	for (int i = 0; i < num_glb_elems; i++) {
		h5_glb_idx_t* vtx_idx = h5tpriv_get_glb_elem_vertices(m, glb_elems, i);

		for (int j = 0; j < num_vertices; j++) {
			// add index temporarly to map ...
				map->items[map->num_items] = (h5_idxmap_el_t) {vtx_idx[j], 0};
				// ... and check whether it has already been added
				h5_idxmap_el_t* retval;
				h5priv_hsearch (&map->items[map->num_items],
				                H5_ENTER, (void**)&retval, &htab);

			if (retval == &map->items[map->num_items]) { // not in list
				// new entry in hash table
				add_glb_vertex_to_list(m, vtx_idx[j], glb_vtx, num_glb_vtx, vtx_list, num_vtx);

			}
		}
	}
	TRY (h5priv_hdestroy (&htab));
	TRY (h5tpriv_sort_vertex_list (vtx_list, *num_vtx));
	H5_RETURN (H5_SUCCESS);
}

/*
 * there are already all local & neighbor chunks in the list
 * we remove all chunks that are not on m->leaf_level, and
 * from neighbors
 */

static h5_err_t
get_list_of_chunks_to_retrieve (
		h5t_mesh_t* const m,
		h5_chk_idx_t* list,
		int* num_list
		) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);

	// retrieve only chunks from this level
	int tmp_counter = 0;
	for (int i = 0; i < *num_list; i++) {
		if (list[i] > m->chunks->curr_idx - m->chunks->num_chunks_p_level[m->leaf_level] && // only the chunks stored on the last level
				H5t_get_proc (m->octree, m->chunks->chunks[list[i]].oct_idx) != m->f->myproc) {
			list[tmp_counter] = list[i];
			tmp_counter++;
		}
	}
	*num_list = tmp_counter;
	H5_RETURN (H5_SUCCESS);
}
static h5_err_t
exchange_glb_elem_glb_vtx (
		h5t_mesh_t* const m,
		h5_glb_elem_t* glb_elems,
		int num_glb_elems,
		h5_glb_elem_t** tot_glb_elems,
		int* num_tot_glb_elems,
		h5_glb_vertex_t* glb_vtx,
		int num_glb_vtx,
		h5_glb_vertex_t** tot_glb_vtx,
		int* num_tot_glb_vtx
		) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);

	h5_glb_idx_t* e_range = h5_calloc (m->f->nprocs +1, sizeof (*e_range));
	TRY (h5tpriv_get_ranges(m, e_range, (h5_glb_idx_t) num_glb_elems, 0));

	h5_glb_idx_t* v_range = h5_calloc (m->f->nprocs +1, sizeof (*v_range));
	TRY (h5tpriv_get_ranges(m, v_range, (h5_glb_idx_t) num_glb_vtx, 0));

	int* e_recvcounts = h5_calloc (m->f->nprocs, sizeof (* e_recvcounts));
	int* v_recvcounts = h5_calloc (m->f->nprocs, sizeof (* v_recvcounts));

	int* e_recvdisp = h5_calloc (m->f->nprocs, sizeof (* e_recvdisp));
	int* v_recvdisp = h5_calloc (m->f->nprocs, sizeof (* v_recvdisp));

	for (int i = 0; i < m->f->nprocs; i++) {
		e_recvdisp[i] = (int) e_range[i];
		v_recvdisp[i] = (int) v_range[i];
		e_recvcounts[i] = (int) (e_range[i+1]- e_range[i]);
		v_recvcounts[i] = (int) (v_range[i+1]- v_range[i]);
	}
	*num_tot_glb_elems = e_range[m->f->nprocs];
	TRY (*tot_glb_elems = h5tpriv_alloc_glb_elems(m, *num_tot_glb_elems));

	*num_tot_glb_vtx = v_range[m->f->nprocs];
	*tot_glb_vtx = h5_calloc (*num_tot_glb_vtx, sizeof (**tot_glb_vtx));


	TRY (h5priv_mpi_allgatherv (
			glb_elems,
			num_glb_elems,
			h5tpriv_get_mpi_type_of_glb_elem(m),
			*tot_glb_elems,
			e_recvcounts,
			e_recvdisp,
			h5tpriv_get_mpi_type_of_glb_elem(m),
			m->f->props->comm));

	TRY (h5priv_mpi_allgatherv (
			glb_vtx,
			num_glb_vtx,
			h5_dta_types.mpi_glb_vtx,
			*tot_glb_vtx,
			v_recvcounts,
			v_recvdisp,
			h5_dta_types.mpi_glb_vtx,
			m->f->props->comm));

	TRY (h5tpriv_sort_vertex_list (*tot_glb_vtx, *num_tot_glb_vtx));
	TRY (h5tpriv_sort_glb_elems (m, *tot_glb_elems, (size_t)*num_tot_glb_elems));

	// free mem
	TRY (h5_free (e_recvcounts));
	TRY (h5_free (v_recvcounts));
	TRY (h5_free (e_recvdisp));
	TRY (h5_free (v_recvdisp));
	TRY (h5_free (e_range));
	TRY (h5_free (v_range));

	H5_RETURN (H5_SUCCESS);
}

/*
 * compare chk_idx
 */
int compare_chk_list(const void *p_a, const void *p_b) {
	return ((h5t_chunk_t*) p_a)->idx - ((h5t_chunk_t*) p_b)->idx;
}

h5_err_t
store_exchanged_elems (
		h5t_mesh_t* const m,
		h5_chk_idx_t* chk_list,
		int num_chk_list,
		h5_glb_elem_t* glb_elems,
		int num_glb_elems,
		h5_glb_vertex_t* glb_vtx,
		int num_glb_vtx
		) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);

	qsort (chk_list, num_chk_list, sizeof (*chk_list), compare_chk_list);

	// calc how many new elems
	int num_new_elems = 0;
	for (int i = 0; i < num_chk_list; i++) {
		num_new_elems += m->chunks->chunks[chk_list[i]].num_elems;
	}
	TRY (h5tpriv_alloc_loc_elems (m, m->num_interior_elems[m->leaf_level], m->num_interior_elems[m->leaf_level] + num_new_elems));

	// extract glb elems that should be stored
	h5_glb_elem_t* new_elems = NULL;
	h5_loc_idx_t new_elems_c = 0; // counter

	// temporary idx list so we don't have to search in glb_elems. glb_elems should be sorted!
	h5_glb_idxlist_t* glb_list = NULL;
	h5priv_alloc_glb_idxlist (&glb_list, num_glb_elems);
	for (int i = 0; i < num_glb_elems; i++) {
		glb_list->items[i] = h5tpriv_get_glb_elem_idx (m, glb_elems, i);
	}
	glb_list->num_items = num_glb_elems;

	int* proc = h5_calloc (num_new_elems, sizeof (*proc));
	int proc_counter = 0;
	TRY (new_elems = h5tpriv_alloc_glb_elems(m, num_new_elems));
	for (int i = 0; i < num_chk_list; i++) {
		int num_elems = m->chunks->chunks[chk_list[i]].num_elems;
		int chk_proc = H5t_get_proc (m->octree, m->chunks->chunks[chk_list[i]].oct_idx );

		h5_glb_idx_t glb_idx = m->chunks->chunks[chk_list[i]].elem;
		h5_loc_idx_t loc_idx = h5priv_find_in_glb_idxlist (glb_list, glb_idx);
		assert (loc_idx > -1);
		assert (h5tpriv_get_glb_elem_idx (m,glb_elems, loc_idx) == glb_idx);

		TRY(h5tpriv_copy_glb_elems(m, new_elems, new_elems_c, glb_elems, loc_idx, num_elems));
		new_elems_c += num_elems;
		while (proc_counter < new_elems_c) {

			proc[proc_counter] = chk_proc; // TODO may need to be changed after we have lb
			proc_counter++;
		}
	}
	assert (new_elems_c == num_new_elems);


	// create list of new glb_vtx

        // TODO should be by far enough -> could be optimzed
	h5_glb_vertex_t* new_vtx = h5_calloc(new_elems_c * 4, sizeof (*new_vtx));
	int new_vtx_c = 0;


	// TODO maybe this function could be extended and used instead of the stuff below
	//TRY (init_glb_vtx_struct_chk (m, new_elems, new_elems_c, new_vtx, &new_vtx_c));
	// extract glb vertices that should be stored
	TRY (init_glb_vtx_struct_chk2 (m, new_elems, new_elems_c,
			glb_vtx, num_glb_vtx, new_vtx, &new_vtx_c));
//	int num_vertices = h5tpriv_ref_elem_get_num_vertices(m);
//	for (int i = 0; i < new_elems_c; i++) {
//		h5_glb_idx_t* vertices = h5tpriv_get_glb_elem_vertices(m, new_elems, i);
//		for (int j = 0; j < num_vertices; j++) {
//			// check if vertex is already locally available
//			int idx = -1;
//			TRY (idx = h5tpriv_find_glb_idx_in_map (&m->map_vertex_g2l, vertices[j]));
//			if (idx == m->map_vertex_g2l.num_items) { // locally not available
//				// add at end and sort later
//				add_glb_vertex_to_list(m, vertices[j], glb_vtx, num_glb_vtx, new_vtx, &new_vtx_c);
//			}
//		}
//	}
//	TRY (h5tpriv_sort_vertex_list (new_vtx, new_vtx_c));

	// store vertices

	TRY (h5tpriv_alloc_loc_vertices (m, new_vtx_c + m->last_stored_vid + 1)); // maybe somewhat more?
	memcpy (&m->vertices[m->last_stored_vid + 1], new_vtx, new_vtx_c * sizeof (*new_vtx));
	m->last_stored_vid += new_vtx_c;

	// rebuild map vtx
	TRY (h5priv_grow_idxmap (&m->map_vertex_g2l, new_vtx_c + m->map_vertex_g2l.size));
	TRY (h5tpriv_rebuild_map_vertex_g2l_partial (m));


	// store elems
	TRY (h5priv_grow_idxmap (&m->map_elem_g2l, num_new_elems + m->map_elem_g2l.size));
	TRY (h5tpriv_init_loc_elems_struct (m, new_elems, m->num_interior_elems[m->leaf_level], num_new_elems, 0, proc));

	// rebuild map elems
	TRY (rebuild_map_elem_g2l_partial (m));

	TRY (h5tpriv_init_elem_flags (m, m->num_interior_elems[m->leaf_level], num_new_elems));

	m->num_interior_elems[m->leaf_level] += num_new_elems;

	TRY (h5_free (new_elems));
	TRY (h5_free (new_vtx));
	TRY (h5_free (proc));
	H5_RETURN (H5_SUCCESS);
}
#endif

h5_err_t
h5t_post_refine (
        h5t_mesh_t* const m
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p", m);
	TRY (h5t_end_store_vertices (m));
	TRY (h5t_end_store_elems (m));
	H5_RETURN (h5priv_free_loc_idlist (&m->marked_entities));
}


#ifdef WITH_PARALLEL_H5GRID
h5_err_t
h5t_post_refine_chk (
        h5t_mesh_t* const m,
        h5_glb_idxlist_t* marked_glb_elems
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p", m);
	h5_debug("post_refine_chk");
	// get boundary edges
	h5_edge_list_t* b_edges = h5tpriv_init_edge_list (
		h5tpriv_ref_elem_get_num_edges(m) * m->marked_entities->num_items);
	TRY (h5tpriv_find_boundary_edges (m, marked_glb_elems, b_edges));

	// exchange boundary edges
	h5_edge_list_t* glb_b_edges = h5tpriv_init_edge_list(0);
	TRY (exchange_boundary_edge_list (m, b_edges, glb_b_edges));

	// find out which edges are split by other procs (i.e. with lower rank)
	// set proc in b_edges to proc who sets glb_idx
	TRY (find_edges_in_boundary_edge_list (b_edges, glb_b_edges));


	// calc vertex range, num loc vertices = (num new vertices - vertices to be set by other proc)
	int num_vtx_not_named = 0;
	for (int i = 0; i < b_edges->num_items; i++) {
		b_edges->items[i].proc != m->f->myproc ? num_vtx_not_named++ : 0;
	}
	h5_glb_idx_t* vtx_range = NULL;
	TRY (vtx_range = h5_calloc (m->f->nprocs + 1, sizeof (*vtx_range)));
	TRY (h5tpriv_get_ranges(m, vtx_range, m->last_stored_vid - m->last_stored_vid_before_ref - num_vtx_not_named, m->num_glb_vertices[m->leaf_level-1]));

	// assign glb vtx idx
	m->num_loc_vertices[m->leaf_level] = m->last_stored_vid+1;

	// make list of loc vtx that don't get a glb_idx from this proc
	h5_loc_idxlist_t* vtx_list = NULL;
	TRY (h5priv_alloc_loc_idxlist(&vtx_list, b_edges->num_items));
	for (int i = 0; i < b_edges->num_items; i++) {
		if (b_edges->items[i].proc != m->f->myproc) {
			TRY (h5priv_search_in_loc_idxlist (&vtx_list, (h5_loc_idx_t)b_edges->items[i].new_vtx));
		}
	}

	TRY (assign_global_vertex_indices_chk (m, vtx_list, vtx_range));

	// set glb_idx in b_edge list
	TRY (set_glb_idx_edge_list (m, b_edges));

	// exchange glb_idx of vertices
	TRY (exchange_boundary_edge_list (m, b_edges, glb_b_edges));
	// TODO could be more efficient by only sending the glb_idx around entries around
	// but one would need to create a new list...

	// set exchanged glb_idx
	TRY (set_exchanged_glb_idx (m, b_edges, glb_b_edges));

	// rebuild map g2l
	TRY (h5priv_grow_idxmap (&m->map_vertex_g2l, m->map_vertex_g2l.num_items + m->last_stored_vid - m->last_stored_vid_before_ref));
	TRY (h5tpriv_rebuild_map_vertex_g2l_partial (m));
	m->last_stored_vid_before_ref = m->last_stored_vid;
	// this is replacing TRY (h5t_end_store_vertices (m));
	// since we need a special assign glb_idx

	TRY (h5priv_mpi_barrier (m->f->props->comm));
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();

	// get elem ranges
	h5_glb_idx_t* elem_range = NULL;
	TRY (elem_range = h5_calloc (m->f->nprocs + 1, sizeof (*elem_range)));

	//TODO use generic get_range function
	TRY (h5tpriv_get_elem_ranges (m, elem_range));

	m->num_interior_elems[m->leaf_level] = m->last_stored_eid+1; // TODO needs to be reset after exchange
	m->num_glb_elems[m->leaf_level] = elem_range[m->f->nprocs];
	m->num_glb_leaf_elems[m->leaf_level] = m->num_glb_leaf_elems[m->leaf_level - 1] +
			(h5tpriv_get_num_new_elems(m) - 1) * (m->num_glb_elems[m->leaf_level]- m->num_glb_elems[m->leaf_level-1]);
	// idea: after ref we have the same number of leaf elems + all refined elems - elems that were refined

	/* assign global indices to new indices */
	TRY (assign_glb_elem_indices_chk (m, elem_range));


	/* rebuild map: global index -> local_index */
	TRY (rebuild_map_elem_g2l_partial (m));
	m->last_stored_eid_before_ref = m->last_stored_eid;


	// weights
	h5_debug("weights");
	if (m->num_weights < 1) {
		m->weights = NULL;
	} else {
		TRY ( m->weights = h5_alloc (m->weights, elem_range[m->f->nprocs] * m->num_weights * sizeof (*m->weights)));

		// set local weights
		TRY (set_local_weights (m, elem_range));

		// exchange weights
		TRY (exchange_weights (m, elem_range));
	}
	TRY (h5priv_mpi_barrier (m->f->props->comm));
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
	// get list of new chunks
	h5_chk_idx_t* chk_send_list = NULL;
	int counter = 0;
	TRY (h5tpriv_get_list_of_chunks_to_write (m, &chk_send_list, &counter));

	// send only chunks from this level
	int tmp_counter = 0;
	for (int i = 0; i < counter; i++) {
		if (chk_send_list[i] > m->chunks->curr_idx - m->chunks->num_chunks_p_level[m->leaf_level]) { // only the chunks stored on the last level
			chk_send_list[tmp_counter] = chk_send_list[i];
			tmp_counter++;
		}
	}
	// TODO this should be optimized only really needed chunks should be sent around

	counter = tmp_counter;
	// create glb_chunks
	h5_glb_elem_t* glb_elems = NULL;
	int num_glb_elems = m->num_interior_elems[m->leaf_level] - m->num_interior_elems[m->leaf_level-1];
	TRY (glb_elems = h5tpriv_alloc_glb_elems(m, num_glb_elems));
	TRY (h5tpriv_init_glb_elems_struct_chk(m, glb_elems, chk_send_list, counter));

	TRY (h5priv_mpi_barrier (m->f->props->comm));
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
	// create list of glb_vtx
	h5_glb_vertex_t* glb_vtx = NULL;
	h5_int32_t num_glb_vtx = 0;
	TRY (glb_vtx = h5_calloc (4 * num_glb_elems, sizeof (*glb_vtx))); // TODO should be by far enough -> could be optimzed
	TRY (init_glb_vtx_struct_chk (m, glb_elems, num_glb_elems, glb_vtx, &num_glb_vtx));

	// get list of chunks to retrieve
	h5_chk_idx_t* chk_list_read = NULL;
	int num_chk_list_read = 0;
	TRY (h5tpriv_get_list_of_chunks_to_read(m, &chk_list_read, &num_chk_list_read));
	TRY (get_list_of_chunks_to_retrieve (m, chk_list_read, &num_chk_list_read));

	TRY (h5priv_mpi_barrier (m->f->props->comm));
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
	// exchange cells and vertices
	h5_glb_elem_t* tot_glb_elems = NULL;
	h5_glb_vertex_t* tot_glb_vtx = NULL;
	int num_tot_glb_elems = 0;
	int num_tot_glb_vtx = 0;
	TRY (exchange_glb_elem_glb_vtx (m, glb_elems, num_glb_elems,
			&tot_glb_elems, &num_tot_glb_elems,
			glb_vtx, num_glb_vtx,
			&tot_glb_vtx, &num_tot_glb_vtx));
	TRY (h5_free (glb_elems)); // doesn't that create mem leak?
	TRY (h5priv_mpi_barrier (m->f->props->comm));
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();

	h5_debug("store exchanged elems");
	// store elems & vertices
	TRY (store_exchanged_elems (m, chk_list_read, num_chk_list_read, tot_glb_elems, num_tot_glb_elems, tot_glb_vtx, num_tot_glb_vtx));

	TRY (h5priv_mpi_barrier (m->f->props->comm));
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();

	// set variables elems
	m->num_glb_elems[m->leaf_level] = m->num_glb_elems[m->leaf_level - 1] + num_tot_glb_elems ;
	m->num_glb_leaf_elems[m->leaf_level] = m->num_glb_leaf_elems[m->leaf_level - 1] +
			num_tot_glb_elems /h5tpriv_get_num_new_elems(m) * (h5tpriv_get_num_new_elems(m) - 1) ;

	m->num_interior_elems[m->leaf_level] = m->last_stored_eid + 1;
	m->num_interior_leaf_elems[m->leaf_level] = m->num_interior_leaf_elems[m->leaf_level-1] +
			((m->num_interior_elems[m->leaf_level] - m->num_interior_elems[m->leaf_level-1])/h5tpriv_get_num_new_elems(m) ) *
			(h5tpriv_get_num_new_elems(m) - 1); // can it be calc easier?
	m->last_stored_eid_before_ref = -1;


	// set variables vtx
	m->num_glb_vertices[m->leaf_level] = vtx_range[m->f->nprocs];
	m->num_loc_vertices[m->leaf_level] = m->last_stored_vid + 1;
	m->last_stored_vid_before_ref = -1;

	// update parent elems
	// idea go through all refined elem (they know their parent) and set parent.child_idx to their idx
	// if it is done backwards always the first child will be stored finally -> is optimizable
	for (int i = m->last_stored_eid; i >= m->num_interior_elems[m->leaf_level - 1]; i--) {
		h5_loc_idx_t parent_idx = h5tpriv_get_loc_elem_parent_idx (m, i);
		if (parent_idx > -1) { // there can be elems with on the chunk border that don't have their parents locally available
			// those were refined on a different proc but exchanged to this proc.
			assert (h5tpriv_get_loc_elem_child_idx (m, parent_idx) == -1 ||
				h5tpriv_get_loc_elem_child_idx (m, parent_idx) == i + 1 ||
				h5tpriv_get_loc_elem_child_idx (m, parent_idx) > i - 4);
			TRY (h5tpriv_set_loc_elem_child_idx (m, parent_idx, i));
		}
	}
	h5_debug("end store elems");
	/* mesh specific finalize */
	TRY (m->methods->store->end_store_elems (m));



	// WARNING elements on boundary chunks that lay on proc boundary may not have enough information
	// to update the neighborhood correctly. a possibility would be to send them around again (the proc
	// who owns their chunk has all necessary neighboring chunks i.e. chan update them properly)


	//since we need special versions of this function, it was already implemented above
	// i don't know if there is a nice way to separate it also in parallel...
	//	TRY (h5t_end_store_elems (m));

	// memory cleanup

	TRY (h5_free (vtx_range));

	TRY (h5_free (chk_list_read));
	TRY (h5priv_free_glb_idxlist (&marked_glb_elems));


	marked_glb_elems = NULL;
	H5_RETURN (h5priv_free_loc_idlist (&m->marked_entities));

}

#endif
h5_err_t
h5t_begin_refine_elems (
        h5t_mesh_t* const m
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p", m);

	TRY (h5tpriv_add_level (m));
	/*
	   Pre-allocate space for items to avoid allocating small pieces of
	   memory.
	 */
	TRY (h5priv_alloc_loc_idlist (&m->marked_entities, MAX_NUM_ELEMS_TO_REFINE_LOCALLY));
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5t_end_refine_elems (
        h5t_mesh_t* const m
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p", m);
	if (m->is_chunked) {
#ifdef WITH_PARALLEL_H5GRID		
		TRY (h5priv_mpi_barrier (m->f->props->comm));
                m->timing.measure[m->timing.next_time++] = MPI_Wtime();
		h5_glb_idxlist_t* glb_list = NULL;
		h5_oct_point_t* midpoints = NULL;
		TRY (h5t_pre_refine_chk (m, &glb_list, &midpoints));
		TRY (h5priv_mpi_barrier (m->f->props->comm));
                m->timing.measure[m->timing.next_time++] = MPI_Wtime();
		TRY (h5t_refine_marked_elems_chk (m, glb_list, midpoints));
		TRY (h5priv_mpi_barrier (m->f->props->comm));
                m->timing.measure[m->timing.next_time++] = MPI_Wtime();
		TRY (h5_free (midpoints));
		midpoints = NULL;
		TRY (h5t_post_refine_chk (m, glb_list));
		m->mesh_changed = 1;
		TRY (h5priv_mpi_barrier (m->f->props->comm));
                m->timing.measure[m->timing.next_time++] = MPI_Wtime();
#endif
	} else {
		TRY (h5t_pre_refine (m));
		TRY (h5t_refine_marked_elems (m));
		TRY (h5t_post_refine (m));
		m->mesh_changed = 1;
	}

	H5_RETURN (H5_SUCCESS);
}

#if defined(WITH_PARALLEL_H5GRID)
h5_err_t
h5tpriv_init_chunks (
		h5t_mesh_t* const m
		) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);

	if (m->chunks != NULL) {
		H5_LEAVE (H5_ERR_INVAL);
	}
	TRY (m->chunks = h5_calloc (1, sizeof (*m->chunks)));
	m->chunks->curr_idx = -1;
	m->chunks->num_alloc = -1;
	m->chunks->num_levels = -1;
	m->chunks->num_chunks_p_level = NULL;
	m->chunks->chunks = NULL;

	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5tpriv_grow_chunks (
		h5t_mesh_t* const m,
		h5_chk_idx_t const size
		) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, size=%d", m, size);
	if (m->chunks->chunks == NULL ) {
		m->chunks->num_alloc = size;
		m->chunks->num_levels = 1;
		TRY (m->chunks->chunks = h5_calloc (size, sizeof (*m->chunks->chunks)));
		TRY (m->chunks->num_chunks_p_level = h5_calloc (
				m->chunks->num_levels,
				sizeof (*m->chunks->num_chunks_p_level)));
		m->chunks->num_chunks_p_level[0] = size;
	} else {
		m->chunks->num_alloc += size;
		m->chunks->num_levels++;
		TRY (m->chunks->chunks = h5_alloc (m->chunks->chunks, (m->chunks->num_alloc)* sizeof (*m->chunks->chunks)));
		TRY (m->chunks->num_chunks_p_level = h5_alloc (
				m->chunks->num_chunks_p_level,
				m->chunks->num_levels * sizeof (*m->chunks->num_chunks_p_level)));
		m->chunks->num_chunks_p_level[m->chunks->num_levels - 1] = size;
	}
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5tpriv_store_chunks (
		h5t_mesh_t* const m,
		h5t_oct_count_list_t* list,
		h5_chk_idx_t num_chunks,
		h5_glb_idx_t* elem_range,
		h5_glb_idx_t* chk_range
		) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, list=%p, num_chunks=%d, "
			"elem_range=%p, chk_range=%p", m, list, num_chunks, elem_range, chk_range);

	if (list->num_items <= 0) {
		assert (chk_range[m->f->myproc+1] - chk_range[m->f->myproc] == 0);
		assert (elem_range[m->f->myproc+1] - elem_range[m->f->myproc] == 0);
		assert (num_chunks == 0);
		H5_LEAVE (H5_SUCCESS);
	}
	int counter = 0;
	h5_chk_weight_t weight = 0;
	h5_oct_idx_t oct_idx = -1;
	//h5_lvl_idx_t level = m->leaf_level;
	int tot_loc_elem = 0;
	for (int i = 0; i < list->num_items; i++) {

		oct_idx = list->items[i].oct;
		counter = list->items[i].count;
		if (m->leaf_level > 0) {
			counter *= h5tpriv_get_num_new_elems (m);
		}
		TRY (h5tpriv_create_chunk (m, oct_idx, elem_range[m->f->myproc] + tot_loc_elem, weight, counter, chk_range));
		tot_loc_elem += counter;
	}

	if ((m->chunks->curr_idx + 1 != chk_range[m->f->myproc + 1]) ||
		(tot_loc_elem != elem_range[m->f->myproc + 1] - elem_range[m->f->myproc])) {
		H5_LEAVE (H5_ERR_INTERNAL);
	}

	H5_RETURN (H5_SUCCESS);
}
h5_err_t
h5tpriv_create_chunk (
		h5t_mesh_t* m,
		h5_oct_idx_t const oct_idx,
		h5_glb_idx_t const first_elem,
		h5_chk_weight_t const weight,
		h5_chk_size_t num_elems,
		h5_glb_idx_t* chk_range
		) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, oct_idx=%d, first_elem=%lld, weight=%lld, num_elems=%d",
			m, oct_idx,(long long) first_elem, (long long) weight, num_elems);

	if (m->chunks->curr_idx + 1 > m->chunks->num_alloc) {
		H5_LEAVE (H5_ERR_INTERNAL);
	}
	if (chk_range == NULL) {
	m->chunks->curr_idx++;
	} else {
		// set curr_idx to beginn of chk_range if not there
		if (m->chunks->curr_idx < chk_range[m->f->myproc]) {
			m->chunks->curr_idx = (h5_chk_idx_t) chk_range[m->f->myproc];
		} else {
			// otherwise already in right range, just add one
			m->chunks->curr_idx++;
		}
		// check that curr_idx doesn't leave range
		assert (m->chunks->curr_idx < chk_range[m->f->myproc + 1]);
	}


	m->chunks->chunks[m->chunks->curr_idx].idx = m->chunks->curr_idx;
	m->chunks->chunks[m->chunks->curr_idx].oct_idx = oct_idx;
	m->chunks->chunks[m->chunks->curr_idx].elem = first_elem;
//	m->chunks->chunks[m->chunks->curr_idx].elems_subchk = NULL;
	m->chunks->chunks[m->chunks->curr_idx].weight = weight;
	m->chunks->chunks[m->chunks->curr_idx].num_elems = num_elems;
//	m->chunks->chunks[m->chunks->curr_idx].vtx = -2; // TODO remove from chunks
//	m->chunks->chunks[m->chunks->curr_idx].num_vtx = -2; // TODO remove from chunks


	H5_RETURN (H5_SUCCESS);
}

/*
 * exchange newly created chunks
 */

h5_err_t
h5tpriv_update_chunks (
		h5t_mesh_t* m,
		h5_glb_idx_t* chk_range
		) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, chk_range=%p", m, chk_range);
	// range is already known
	int sendcount = chk_range[m->f->myproc + 1 ] - chk_range[m->f->myproc];

	// sendbuffer
	h5t_chunk_t* sendbuf = NULL;
	TRY (sendbuf = h5_calloc (sendcount, sizeof (*sendbuf)));
	memcpy(sendbuf, &m->chunks->chunks[chk_range[m->f->myproc]], sendcount * sizeof (*m->chunks->chunks));


	// recvbuf
	int* recvdisp = NULL;
	int* recvcount = NULL;
	TRY (recvdisp = h5_calloc (m->f->nprocs, sizeof (*recvdisp)));
	TRY (recvcount = h5_calloc (m->f->nprocs, sizeof (*recvcount)));
	recvdisp[0] = 0;
	recvcount[0] = chk_range[1] - chk_range[0];
	for (int i = 1; i < m->f->nprocs; i++) {
		recvdisp[i] = chk_range[i] - chk_range[0];
		recvcount[i] = chk_range[i + 1 ] - chk_range[i];
	}

	h5priv_mpi_allgatherv(
		    sendbuf,
		    sendcount,
		    h5_dta_types.mpi_chunk,
		    &m->chunks->chunks[chk_range[0]],
		    recvcount,
		    recvdisp,
		    h5_dta_types.mpi_chunk,
		    m->f->props->comm);

	m->chunks->curr_idx = chk_range[m->f->nprocs] -1;
	TRY (h5_free (sendbuf));
	TRY (h5_free (recvdisp));
	TRY (h5_free (recvcount));

	H5_RETURN (H5_SUCCESS);
}


h5_err_t
h5tpriv_free_chunks (
		h5t_mesh_t* const m
		) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);

	if (m->chunks != NULL) {
		TRY (h5_free (m->chunks->chunks));
		TRY (h5_free (m->chunks->num_chunks_p_level));
		TRY (h5_free (m->chunks));
	}
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5tpriv_print_chunks (
		h5t_mesh_t* const m
		) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	h5_debug ("\nPrinting chunks: \n   curr_idx: %d\n num_alloc: %d\n  num_levels: %d\n\n",
			m->chunks->curr_idx,
			m->chunks->num_alloc,
			m->chunks->num_levels);
	for (int i = 0; i <= m->chunks->curr_idx; i++) {
		h5_debug ("\nchunk: %d \n oct_idx: %d \n elem: %lld \n weight:%lld \n num_elems: %d\n\n",
				m->chunks->chunks[i].idx,
				m->chunks->chunks[i].oct_idx,
				(long long) m->chunks->chunks[i].elem,
				(long long) m->chunks->chunks[i].weight,
				m->chunks->chunks[i].num_elems
				);
	}


	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5tpriv_print_oct_userdata (
		h5t_mesh_t* const m
		) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	h5_debug ("\nPrinting oct_userdata: \n   curr_idx: %d\n",
			m->octree->current_oct_idx);
	for (int i = 0; i <= m->octree->current_oct_idx; i++) {
		h5_debug ("\n oct_idx: %d \n %d - %d - %d - %d \n\n",
				i,
				((h5t_oct_userdata_t*)m->octree->userdata)[i].idx[0],
				((h5t_oct_userdata_t*)m->octree->userdata)[i].idx[1],
				((h5t_oct_userdata_t*)m->octree->userdata)[i].idx[2],
				((h5t_oct_userdata_t*)m->octree->userdata)[i].idx[3]
				);
	}


	H5_RETURN (H5_SUCCESS);
}
#endif
#if 0
// index set for DUNE
h5_err_t
h5t_create_index_set (
        h5t_mesh_t* const m
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p", m);
	int codim;
	int dim = h5tpriv_ref_elem_get_dim (m);
	// todo: check tagset already exist
	TRY (h5t_add_mtagset (m, "__IndexSet__", H5_INT64_T);

	for (codim = 0; codim <= dim; codim++) {
		h5_glb_idx_t idx = 0;
		h5t_leaf_iterator_t it;
		h5_glb_id_t entity_id;
		TRY (h5t_init_leaf_iterator ((h5t_iterator_t*)&it, m, codim));
		while ((entity_id = it.iter(f, (h5t_iterator_t*)&it)) >= 0) {
			TRY (h5t_set_mtag_by_name (f, "__IndexSet__", entity_id, 1, &idx));
		}
	}
	H5_RETURN (H5_SUCCESS);
}
#endif
