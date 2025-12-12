/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include "private/h5t_types.h"
#include "private/h5t_err.h"
#include "private/h5t_access.h"
#include "private/h5t_adjacencies.h"
#include "private/h5t_core.h"
#include "private/h5t_map.h"
#include "private/h5t_model.h"
#include "private/h5t_io.h"
#include "private/h5t_store.h"

#include "h5core/h5t_map.h"

static h5_err_t
alloc_loc_elems (
        h5t_mesh_t* const m,
        const size_t cur,       // currently allocated
        const size_t new        // new size
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, cur=%zu, new=%zu", m, cur, new);

	/* alloc mem for local data of elements */
	TRY ( m->loc_elems = h5_alloc (
	              m->loc_elems,
	              new * sizeof (h5_loc_tet_t) ) );
	memset (
	        (h5_loc_tet_t*)m->loc_elems + cur,
	        -1,
	        (new-cur) * sizeof (h5_loc_tet_t) );

	H5_RETURN (H5_SUCCESS);
}


/*!
   Refine edge. Store vertex, if new.

   Function can be used with tetrahedral and triangle meshes.

   \return local index of vertex
 */
static h5_loc_idx_t
bisect_edge (
        h5t_mesh_t* const m,
        const h5_loc_idx_t face_idx,
        const h5_loc_idx_t elem_idx
        ) {
	H5_PRIV_FUNC_ENTER (
                h5_loc_idx_t,
                "m=%p, face_idx=%lld, elem_idx=%lld",
                m, (long long)face_idx, (long long)elem_idx);
	h5_loc_idlist_t* retval;
	/*
	   get all elements sharing the given edge
	 */
	TRY (h5tpriv_find_te2 (m, face_idx, elem_idx, &retval));
	/*
	   check wether one of the cells in retval has been refined
	 */
	size_t i;
	for (i = 0; i < retval->num_items; i++) {
		h5_loc_id_t kids[2] = {-1,-1};
		TRY (h5tpriv_get_loc_entity_children (m, retval->items[i], kids));
		if (kids[0] >= 0) {
			// element has been refined, return bisecting point
			h5_loc_idx_t edge0[2], edge1[2];
			TRY( h5t_get_loc_vertex_indices_of_edge (m, kids[0], edge0) );
			TRY( h5t_get_loc_vertex_indices_of_edge (m, kids[1], edge1) );
			if ((edge0[0] == edge1[0]) || (edge0[0] == edge1[1])) {
				H5_LEAVE (edge0[0]); // return first vertex
			} else {
				H5_LEAVE (edge0[1]); // return second vertex
			}
		}
	}
	/*
	   None of the elements has been refined -> add new vertex.
	 */
	h5_loc_idx_t indices[2];
	TRY( h5t_get_loc_vertex_indices_of_edge2 (m, face_idx, elem_idx, indices) );
	h5_float64_t* P0 = m->vertices[indices[0]].P;
	h5_float64_t* P1 = m->vertices[indices[1]].P;
	h5_float64_t P[3];

	P[0] = (P0[0] + P1[0]) / 2.0;
	P[1] = (P0[1] + P1[1]) / 2.0;
	P[2] = (P0[2] + P1[2]) / 2.0;

	H5_RETURN (h5t_store_vertex (m, -1, P));  // return idx of new vertex
}

/*
   When calling this function, we know the number of elements to refine. But
   we don't now the number of new vertices we will get. We have to compute
   this number or just to guess it.

   Let n be the number of elements to refine and L the number of boundary faces of the
   disconnected areas to be refined. Let l be the number of edges on

   For triangle grids the upper limit of new vertices is 3n and the lower limit
   3/2n + 3/2. The exact number is 3/2n + 1/2L.
   The number of inner vertices is (3n-L)/2 and the boundary vertices L.

  similar for tetrahedrals but calc also include boundary edges...
//   For tetrahedral grids the upper limit is 6n and the lower limit is 3n+3.
//   The exact number is 3n + 3l.

   To get the real number of vertices to add, we either have to compute the
   number of disconnected areas (which is quiet expensive), try to guess it
   (which is impossible) or just set a limit. In most cases the number of
   disconnected areas will be "small".

   For the time being we set the maximum number of boundary  to 64.
 */
static h5_err_t
pre_refine_tet (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	unsigned int num_interior_elems_to_refine = m->marked_entities->num_items;
	TRY (h5t_begin_store_vertices (m, num_interior_elems_to_refine*3 + 192));
	TRY (h5t_begin_store_elems (m, num_interior_elems_to_refine*8));
	H5_RETURN (H5_SUCCESS);
}

/*!
   Refine tetrahedron \c elem_idx. This function implements a "red refinement"
   as described by J. Bey in "Tetrahedral grid refinement", Computing 55
   (1995), pp. 355-378

   \return Local id of first new tetrahedron or \c -1
 */
static h5_loc_idx_t
refine_tet (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	H5_PRIV_FUNC_ENTER (h5_loc_idx_t,
	                    "m=%p, elem_idx=%lld",
	                    m, (long long)elem_idx);
	h5_loc_idx_t vertices[10];
	h5_loc_idx_t elem_idx_of_first_child;
	h5_loc_tet_t* el = (h5_loc_tet_t*)m->loc_elems + elem_idx;

	if ( el->child_idx >= 0 )
		H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Tetrahedron %lld already refined.",
			(long long)elem_idx);
	vertices[0] = el->vertex_indices[0];
	vertices[1] = el->vertex_indices[1];
	vertices[2] = el->vertex_indices[2];
	vertices[3] = el->vertex_indices[3];

	vertices[4] = bisect_edge (m, 0, elem_idx);     // edge (0,1)
	vertices[5] = bisect_edge (m, 1, elem_idx);     // edge (0,2)
	vertices[6] = bisect_edge (m, 2, elem_idx);     // edge (1,2)
	vertices[7] = bisect_edge (m, 3, elem_idx);     // edge (0,3)
	vertices[8] = bisect_edge (m, 4, elem_idx);     // edge (1,3)
	vertices[9] = bisect_edge (m, 5, elem_idx);     // edge (2,3)

	// add new tets
	h5_loc_idx_t new_elem[4];

        /*
          Add refined cells with pre-sorted vertices!
          This is actually importend: sorting refined cells may produce wrong
          orientated cells. This can easily be shown with triangle:

         0
          * 
               
                     *4
          2*                 
                               *2
                     *
            *        5
            1

          The parent triangle is (0,1,2) with a counter-clockwise orientation.
          Since we must use an epsilon comparision, we run into a problem, if
          the comparision returns X(0) < X(1), but X(0) == X(2). This happens,
          if
             X(0) - X(1) < 2*epsilon
          The refined triangle (0,2,4) would be stored as (2,0,4), if we try 
          to sort the vertices. The orientation of (2,0,4) is clockwise! But
          we assume that (0,2,4), (2,1,5) and (4,5,2) have the same orientation
          as the parent and (2,4,5) the opposite orientation.
        */

	// child 0
	new_elem[0] = vertices[0];      // vertex 0
	new_elem[1] = vertices[4];      // split point (0,1)
	new_elem[2] = vertices[5];      // split point (0,2)
	new_elem[3] = vertices[7];      // split point (0,3)
	TRY (elem_idx_of_first_child = h5tpriv_add_cell (m, elem_idx, new_elem, NULL));

	// child 1
	new_elem[0] = vertices[4];      // split point (0,1)
	new_elem[1] = vertices[1];      // vertex 1
	new_elem[2] = vertices[6];      // split point (1,2)
	new_elem[3] = vertices[8];      // split point (1,3)
	TRY (h5tpriv_add_cell (m, elem_idx, new_elem, NULL));

	// child 2
	new_elem[0] = vertices[5];      // split point (0,2)
	new_elem[1] = vertices[6];      // split point (1,2)
	new_elem[2] = vertices[2];      // vertex 2
	new_elem[3] = vertices[9];      // split point (2,3)
	TRY (h5tpriv_add_cell (m, elem_idx, new_elem, NULL));

	// child 3
	new_elem[0] = vertices[7];      // split point (0,3)
	new_elem[1] = vertices[8];      // split point (1,3)
	new_elem[2] = vertices[9];      // split point (2,3)
	new_elem[3] = vertices[3];      // vertex 3
	TRY (h5tpriv_add_cell (m, elem_idx, new_elem, NULL));

	// child 4
	new_elem[0] = vertices[4];      // split point (0,1)
	new_elem[1] = vertices[5];      // split point (0,2)
	new_elem[2] = vertices[6];      // split point (1,2)
	new_elem[3] = vertices[8];      // split point (1,3)
	TRY (h5tpriv_add_cell (m, elem_idx, new_elem, NULL));

	// child 5
	new_elem[0] = vertices[4];      // split point (0,1)
	new_elem[1] = vertices[5];      // split point (0,2)
	new_elem[2] = vertices[7];      // split point (0,3)
	new_elem[3] = vertices[8];      // split point (1,3)
	TRY (h5tpriv_add_cell (m, elem_idx, new_elem, NULL));

	// child 6
	new_elem[0] = vertices[5];      // split point (0,2)
	new_elem[1] = vertices[6];      // split point (1,2)
	new_elem[2] = vertices[8];      // split point (1,3)
	new_elem[3] = vertices[9];      // split point (2,3)
	TRY (h5tpriv_add_cell (m, elem_idx, new_elem, NULL));

	// child 7
	new_elem[0] = vertices[5];      // split point (0,2)
	new_elem[1] = vertices[7];      // split point (0,3)
	new_elem[2] = vertices[8];      // split point (1,3)
	new_elem[3] = vertices[9];      // split point (2,3)
	TRY (h5tpriv_add_cell (m, elem_idx, new_elem, NULL));

	((h5_loc_tet_t*)m->loc_elems)[elem_idx].child_idx = elem_idx_of_first_child;
	m->num_interior_leaf_elems[m->leaf_level]--;

	H5_RETURN (elem_idx_of_first_child);
}

static inline h5_loc_idx_t
compute_neighbor_of_face (
        h5t_mesh_t* const m,
        h5_loc_idx_t elem_idx,
        const h5_loc_idx_t face_idx
        ) {
	H5_PRIV_FUNC_ENTER (h5_loc_idx_t,
	                    "m=%p, elem_idx=%lld, face_idx=%lld",
	                    m, (long long)elem_idx, (long long)face_idx);
	h5_loc_idlist_t* td;
	h5_loc_idx_t neighbor_idx = -2;

	do {
		TRY( h5tpriv_find_td2 (
		             m,
		             face_idx,
		             elem_idx,
		             &td) );
		if (td == NULL) {
			H5_LEAVE (h5_error_internal ());
		}
		if (td->num_items == 1) {
			// neighbor is coarser or face is on the boundary
			elem_idx = ((h5_loc_tet_t*)m->loc_elems)[elem_idx].parent_idx;
			if (elem_idx == -1) {
				// we are on the level of the macro grid
				neighbor_idx = -1;
			}
		} else if (td->num_items == 2) {
			// neighbor has same level of coarsness
			if (h5tpriv_get_elem_idx(td->items[0]) == elem_idx) {
				neighbor_idx = h5tpriv_get_elem_idx (td->items[1]);
			} else {
				neighbor_idx = h5tpriv_get_elem_idx (td->items[0]);
			}

		} else {
			H5_LEAVE (h5_error_internal ());
		}
	} while (neighbor_idx < -1);
	H5_RETURN (neighbor_idx);
}

/*
   Compute neighbors for elements on given level.
 */
static inline h5_err_t
compute_neighbors_of_elems (
        h5t_mesh_t* const m,
        h5_lvl_idx_t level
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, level=%d", m, level);
	if (level < 0 || level >= m->num_leaf_levels) {
		H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"level idx %lld out of bound, must be in [%lld,%lld]",
			(long long)level,
			(long long)0,
			(long long)m->num_leaf_levels);
	}
	h5_loc_idx_t elem_idx = level == 0 ? 0 : m->num_interior_elems[level-1];
	const h5_loc_idx_t last_idx = m->num_interior_elems[level] - 1;
	h5_loc_tet_t *el = (h5_loc_tet_t*)m->loc_elems + elem_idx;
	while (elem_idx <= last_idx) {
		h5_loc_idx_t face_idx = 0;
		for (; face_idx < 4; face_idx++) {
			el->neighbor_indices[face_idx] =
			        compute_neighbor_of_face (m, elem_idx, face_idx);
		}
		elem_idx++;
		el++;
	}

	H5_RETURN (H5_SUCCESS);
}
/*
 * returns number of newly created tetrahedra when refining a tetrahedral
 */
static int
get_num_new_tetrahedra (
        void
        ) {
	return 8;
}

static h5_err_t
end_store_elems (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);

	h5_loc_idx_t start_idx = (m->leaf_level > 0) ? m->num_interior_elems[m->leaf_level-1] : 0;
	h5_loc_idx_t count = m->num_interior_elems[m->leaf_level] - start_idx;

	TRY( h5tpriv_update_internal_structs (m, m->leaf_level) );
	TRY( compute_neighbors_of_elems (m, m->leaf_level) );
	TRY( h5tpriv_init_elem_flags (m, start_idx, count) );
	H5_RETURN (H5_SUCCESS);
}

struct h5t_store_methods h5tpriv_tetm_store_methods = {
	alloc_loc_elems,
	pre_refine_tet,
	refine_tet,
	get_num_new_tetrahedra,
	end_store_elems,
};
