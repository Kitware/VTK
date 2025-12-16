/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include "h5core/h5_log.h"
#include "h5core/h5_syscall.h"

#include "private/h5_file.h"
#include "private/h5_types.h"
#include "private/h5t_types.h"
#include "private/h5t_model.h"
#include "private/h5t_access.h"

#include <string.h>
#include <stdlib.h>

/*** op's on local elements ***/

#if defined(WITH_PARALLEL_H5GRID)
static MPI_Datatype
get_mpi_type_of_glb_elem (
        h5t_mesh_t* const m
        ) {
	return h5_dta_types.mpi_glb_tet;
}
#endif

/*** op's on local elements ***/
static h5_loc_elem_t*
get_loc_elem (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return (h5_loc_elem_t*)(((h5_loc_tet_t*)m->loc_elems) + elem_idx);
}

static h5_glb_idx_t
get_loc_elem_glb_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_loc_tet_t*)m->loc_elems)[elem_idx].glb_idx;
}

static h5_glb_idx_t
set_loc_elem_glb_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_glb_idx_t glb_idx
        ) {
	((h5_loc_tet_t*)m->loc_elems)[elem_idx].glb_idx = glb_idx;
	return glb_idx;
}

static h5_loc_idx_t
get_loc_elem_parent_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_loc_tet_t*)m->loc_elems)[elem_idx].parent_idx;
}

static h5_loc_idx_t
set_loc_elem_parent_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t parent_idx
        ) {
	((h5_loc_tet_t*)m->loc_elems)[elem_idx].parent_idx = parent_idx;
	return parent_idx;
}

static h5_loc_idx_t
get_loc_elem_child_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_loc_tet_t*)m->loc_elems)[elem_idx].child_idx;
}

static h5_loc_idx_t
set_loc_elem_child_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t child_idx
        ) {
	((h5_loc_tet_t*)m->loc_elems)[elem_idx].child_idx = child_idx;
	return child_idx;
}

static h5_lvl_idx_t
get_loc_elem_level_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_loc_tet_t*)m->loc_elems)[elem_idx].level_idx;
}

static h5_lvl_idx_t
set_loc_elem_level_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_lvl_idx_t level_idx
        ) {
	((h5_loc_tet_t*)m->loc_elems)[elem_idx].level_idx = level_idx;
	return level_idx;
}

static h5_loc_idx_t*
get_loc_elem_vertex_indices (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_loc_tet_t*)m->loc_elems)[elem_idx].vertex_indices;
}

static h5_loc_idx_t*
get_loc_elem_vertex_indices_of_array (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_loc_elem_t* loc_elems
        ) {
	return ((h5_loc_tet_t*)loc_elems)[elem_idx].vertex_indices;
}

static h5_loc_idx_t
get_loc_elem_vertex_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t face_idx
        ) {
	return ((h5_loc_tet_t*)m->loc_elems)[elem_idx].vertex_indices[face_idx];
}

static h5_loc_idx_t
set_loc_elem_vertex_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t face_idx,
        const h5_loc_idx_t vertex_idx
        ) {
	((h5_loc_tet_t*)m->loc_elems)[elem_idx].vertex_indices[face_idx] = vertex_idx;
	return vertex_idx;
}

static h5_loc_idx_t*
get_loc_elem_neighbor_indices (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_loc_tet_t*)m->loc_elems)[elem_idx].neighbor_indices;
}

static h5_loc_idx_t
get_loc_elem_neighbor_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t face_idx
        ) {
	return ((h5_loc_tet_t*)m->loc_elems)[elem_idx].neighbor_indices[face_idx];
}

static h5_loc_idx_t
set_loc_elem_neighbor_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t face_idx,
        const h5_loc_idx_t neighbor_idx
        ) {
	((h5_loc_tet_t*)m->loc_elems)[elem_idx].neighbor_indices[face_idx] = neighbor_idx;
	return neighbor_idx;
}

/*** op's on global elements ***/
static h5_glb_elem_t*
alloc_glb_elems (
        h5t_mesh_t* const m, const size_t size
        ) {
	H5_PRIV_FUNC_ENTER (h5_glb_elem_p, "m=%p, size=%zu", m, size);
	h5_glb_elem_p buf;
	TRY (buf = h5_calloc (size, sizeof(h5_glb_tet_t)));
	H5_RETURN (buf);
}

static h5_glb_elem_t*
get_glb_elem (
        h5_glb_elem_t* __elems,
        const h5_loc_idx_t idx
        ) {
	h5_glb_tet_t* elems = (h5_glb_tet_t*)__elems;
	return (h5_glb_elem_t*)(elems+idx);
}

static h5_glb_elem_t*
copy_glb_elems (
        h5_glb_elem_t* dstbuf,
        h5_loc_idx_t dstidx,
        h5_glb_elem_t* srcbuf,
        h5_loc_idx_t srcidx,
        size_t count
        ) {
	h5_glb_tet_t* dst = (h5_glb_tet_t*)dstbuf + dstidx;
	h5_glb_tet_t* src = (h5_glb_tet_t*)srcbuf + srcidx;
	memcpy (dst, src, count*sizeof(*src));
	return (h5_glb_elem_t*)dst;
}

static int
compare_glb_elems(const void *p_a, const void *p_b) {
	return ((h5_glb_tet_t*) p_a)->idx - ((h5_glb_tet_t*) p_b)->idx;
}


static h5_err_t
sort_glb_elems (
        h5_glb_elem_t* elems,
        size_t count
        ) {
	qsort(elems,(int) count, sizeof (h5_glb_tet_t), compare_glb_elems);
	return H5_SUCCESS;
}


static h5_glb_idx_t
get_glb_elem_idx (
        h5_glb_elem_t* const elems,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_glb_tet_t*)elems)[elem_idx].idx;
}

static h5_lvl_idx_t
get_glb_elem_level (
        h5_glb_elem_t* const elems,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_glb_tet_t*)elems)[elem_idx].level_idx;
}

static h5_glb_idx_t*
get_glb_elem_vertices (
        h5_glb_elem_t* const elems,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_glb_tet_t*)elems)[elem_idx].vertex_indices;
}

static h5_glb_idx_t*
get_glb_elem_neighbors (
        h5_glb_elem_t* const elems,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_glb_tet_t*)elems)[elem_idx].neighbor_indices;
}

static h5_err_t
set_geom_boundary_elem_flag (
        h5t_mesh_t* const m,
        h5_loc_idx_t elem_idx
        ) {
	((h5_loc_tet_t*)m->loc_elems)[elem_idx].flags |= H5_GEOBORDER_ENTITY;
	return H5_SUCCESS;
}

static int
is_geom_boundary_elem  (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return (((h5_loc_tet_t*)m->loc_elems)[elem_idx].flags & H5_GEOBORDER_ENTITY) ? 1 : 0;
}

static int
is_boundary_facet (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t facet_idx
        ) {
	return (((h5_loc_tet_t*)m->loc_elems)[elem_idx].neighbor_indices[facet_idx] == -1);
}

static int
is_boundary_face (
        h5t_mesh_t* const m,
        const int dim,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t facet_idx
        ) {
	UNUSED_ARGUMENT (m);
	UNUSED_ARGUMENT (dim);
	UNUSED_ARGUMENT (elem_idx);
	UNUSED_ARGUMENT (facet_idx);

	return h5_error_internal ();
}

/*
   From time to time we need the parent entity of a given entity. This is
   simple  for cells but requires some knowledge about the mesh refinement
   method for all other entities. Of course not all entities have a parent
   anyway.

   Just as an reminder: New vertices in a refined tetrahedron P:

   3
 **
 ** *
 * *  *
 *  *   *
 *   *    *
 *    9     *
 *     *      *
   7      *       8
 *       *        *
 *        2         *
 *       * *  *       *
 *     *          *     *
 *   5                6   *
 * *                      * *
 **                          **
   0 * * * * * * * 4 * * * * * * *1

   The new cells (with n_P: number of children):

   n_P	vertices
   0	0,4,5,7
   1	4,1,6,8
   2	5,6,2,9
   3	7,8,9,3
   4	4,5,6,8
   5	4,5,7,8
   6	5,6,8,9
   7	5,7,8,9

   Given an entity E we know:

 * C: the cell E belongs to,
 * e_C: the id of E in C and E = [e_C,C]
 * P: the parent of C, if any
 * n_P: the number of child C

 */

/*
   Parent of tetrahedron
   ---------------------
   This is the simplest case: each children knows his parent.


   Parent of triangle
   ------------------
   Given an triangle E, we want to know the parent triangle of E.

   Refining a tetrahedron gives 4*4 new triangles on the surface of the
   tetrahedron. We don't care about the inner triangles.

   The new triangles on the surface of P are:

   n_P	vertices	triangles
   0	0,4,5,7		(0,4,5)*  (0,4,7)*  (0,5,7)*  (4,5,7)
   1	4,1,6,8		(4,1,6)*  (4,1,8)*  (4,6,8)   (1,6,8)*
   2	5,6,2,9		(5,6,2)*  (5,6,9)   (5,2,9)*  (6,2,9)*
   3	7,8,9,3		(7,8,9)   (7,8,3)*  (7,9,3)*  (8,9,3)*
   4	4,5,6,8		(4,5,6)*  (4,5,8)   (4,6,8)   (5,6,8)
   5	4,5,7,8		(4,5,7)   (4,5,8)   (4,7,8)*  (5,7,8)
   6	5,6,8,9		(5,6,8)   (5,6,9)   (5,8,9)   (6,8,9)*
   7	5,7,8,9		(5,7,8)   (5,7,9)*  (5,8,9)   (7,8,9)

   Now let's have a look on triangle id's. We want to map an triangle id e_C
   of cell C to the triangle id e_P of its parent P. As we can see in above
   table, the triangle (0,4,5) has id 0 and belongs to child 0. The parent
   triangle of (0,4,5) is (0,1,2) also with id 0. It is simple to prove, that
   the triangle id of child and parent are identical in all cases.

   triangle	tetrahedron	triangle id	parent triangle id
   (0,4,5)	(0,4,5,7)	0		0
   (0,4,7)	(0,4,5,7)	1		1
   (0,5,7)	(0,4,5,7)	2		2
   (4,1,6)	(4,1,6,8)	0		0
   (4,1,8)	(4,1,6,8)	1		1
   (1,6,8)	(4,1,6,8)	3		3
   (5,6,2)	(5,6,2,9)	0		0
   (5,2,9)	(5,6,2,9)	2		2
   (6,2,9)	(5,6,2,9)	3		3
   (7,8,3)	(7,8,9,3)	1		1
   (7,9,3)	(7,8,9,3)	2		2
   (8,9,3)	(7,8,9,3)	3		3
   (4,5,6)	(4,5,6,8)	0		0
   (4,7,8)	(4,5,7,8)	2		1
   (6,8,9)	(5,6,8,9)	3		3
   (5,7,9)	(5,7,8,9)	1		2

   This gives us the following matrix:

             t r i a n g l e   i d
             0   1   2   3

     c  0    0   1   2  -1
     h	1    0   1  -1   3
     i	2    0  -1   2   3
     l	3   -1   1   2   3
     d	4    0  -1  -1  -1
        5   -1  -1   1  -1
     n	6   -1  -1  -1   3
     o	7   -1   2  -1  -1

 */
int map_tri_to_parent_face[8][4] = {
	{ 0, 1, 2,-1},  // 0
	{ 0, 1,-1, 3},  // 1
	{ 0,-1, 2, 3},  // 2
	{-1, 1, 2, 3},  // 3
	{ 0,-1,-1,-1},  // 4
	{-1,-1, 1,-1},  // 5
	{-1,-1,-1, 3},  // 6
	{-1, 2,-1,-1}   // 7
};

/*
   Parent of edge
   --------------
   Given an edge E, we want to know the parent edge of E.

   Refining P we get 25 new edges. But only 12 edges are intersecting with
   edges of P. Only for these 12 edges we have a parent edges in P.  So,
   we don't have to care about the 13 other edges. It is obvious, that only
   the first for children of P have edges intersecting with edges of P.
   These edges are marked in the following table.

   Edges of the first four childern of P:

   n_P	vertices	edges
   0	0,4,5,7		(0,4)*  (0,5)*  (4,5)   (0,7)*  (4,7)   (5,7)
   1	4,1,6,8		(4,1)*  (4,6)   (1,6)*  (4,8)   (1,8)*  (6,8)
   2	5,6,2,9		(5,6)   (5,2)*  (6,2)*  (5,9)   (6,9)   (2,9)*
   3	7,8,9,3		(7,8)   (7,9)   (8,9)   (7,3)*  (8,3)*  (9,3)*

   Now let's have a look on edge id's. We want to map an edge id e_C of cell
   C to the edge id e_P of its parent P. As we can see in above table, the
   edge (0,4) has id 0 and belongs to child 0. The parent edge of (0,4) is
   (0,1) also with id 0. It is simple to prove, that the edge id in child
   and parent are identical in all cases. This gives us the following matrix:

             e d g e   i d
             0   1   2   3   4   5

     c  0    0   1  -1   3  -1  -1
     h	1    0  -1   2  -1   4  -1
     i	2   -1   1   2  -1  -1   5
     l	3   -1  -1  -1   3   4   5
     d	4   -1  -1  -1  -1  -1  -1
        5   -1  -1  -1  -1  -1  -1
     n	6   -1  -1  -1  -1  -1  -1
     o	7   -1  -1  -1  -1  -1  -1

   How to read it? Given an entity E, we know the element/cell C
   entity E is belonging to and the edge number e_C. We know the parent P of
   C and we know which number of child C is, let this be n_P.

   The matrix maps child number and edge id of E to the corresponding edge
   id of the parent.

   f (n_P, e_C) -> e_P

   If the result is -1, no parent edge exists. Otherwise the parent edge is
   [e_P,P]

 */
int map_edge_to_parent_face[8][6] = {
	{ 0, 1,-1, 3,-1,-1},
	{ 0,-1, 2,-1, 4,-1},
	{-1, 1, 2,-1,-1, 5},
	{-1,-1,-1, 3, 4, 5},
	{-1,-1,-1,-1,-1,-1},
	{-1,-1,-1,-1,-1,-1},
	{-1,-1,-1,-1,-1,-1},
	{-1,-1,-1,-1,-1,-1},
};
/*
   Parent of vertex
   ----------------
   This case is trivial.
 */
int map_vertex_to_parent_face[8][4] = {
	{ 0,-1,-1,-1},
	{-1, 1,-1,-1},
	{-1,-1, 2,-1},
	{-1,-1,-1, 3},
	{-1,-1,-1,-1},
	{-1,-1,-1,-1},
	{-1,-1,-1,-1},
	{-1,-1,-1,-1},
};

static h5_loc_id_t
get_loc_entity_parent (
        h5t_mesh_t* const m,
        h5_loc_id_t entity_id
        ) {
	// extract type ID and face index
	h5_loc_id_t type_id = h5tpriv_get_entity_type (entity_id);
	h5_loc_idx_t face_idx = h5tpriv_get_face_idx (entity_id);
	h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (entity_id);

	// # of children
	h5_loc_idx_t parent_idx = get_loc_elem_parent_idx (m, elem_idx);
	if (parent_idx < 0)
		return H5_NOK;
	h5_loc_idx_t firstborn_idx = get_loc_elem_child_idx (m, parent_idx);
	h5_loc_idx_t num_child = elem_idx - firstborn_idx;

	switch (type_id) {
	case H5T_TYPE_VERTEX: {
		face_idx = map_vertex_to_parent_face[num_child][face_idx];
		break;
	}
	case H5T_TYPE_EDGE: {
		face_idx = map_edge_to_parent_face[num_child][face_idx];
		break;
	}
	case H5T_TYPE_TRIANGLE: {
		face_idx = map_edge_to_parent_face[num_child][face_idx];
		break;
	}
	case H5T_TYPE_TET: {
		break;
	}
	default:
		h5_error_internal ();
	}
	return (face_idx != -1) ?
	       h5tpriv_build_entity_id (type_id, face_idx, parent_idx) : -1;
}

static h5_err_t
get_children_of_loc_elem (
        h5t_mesh_t* const m,
        h5_loc_idx_t face_idx,          // in
        h5_loc_idx_t elem_idx,          // in
        h5_loc_id_t* children           // out
        ) {
	if (face_idx != 0) {
		return h5_error_internal ();
	}
	h5_loc_idx_t idx = ((h5_loc_tet_t*)m->loc_elems)[elem_idx].child_idx;
	children[0] = h5tpriv_build_tet_id ((h5_loc_idx_t)0, idx++);
	children[1] = h5tpriv_build_tet_id ((h5_loc_idx_t)0, idx++);
	children[2] = h5tpriv_build_tet_id ((h5_loc_idx_t)0, idx++);
	children[3] = h5tpriv_build_tet_id ((h5_loc_idx_t)0, idx++);
	children[4] = h5tpriv_build_tet_id ((h5_loc_idx_t)0, idx++);
	children[5] = h5tpriv_build_tet_id ((h5_loc_idx_t)0, idx++);
	children[6] = h5tpriv_build_tet_id ((h5_loc_idx_t)0, idx++);
	children[7] = h5tpriv_build_tet_id ((h5_loc_idx_t)0, idx);

	return H5_SUCCESS;
}
/*
   Compute direct children of a triangle.

   Face 0	      Face 1		  Face 2	      Face 3
   1		      1	                  2		      1
 +		       +	           +		       +
 |\		       |\	           |\		       |\
 | \		       | \	           | \		       | \
 |  \		       |  \	           |  \		       |  \
 |   \	       |   \	           |   \	       |   \
   4+----+6	      4+----+8            5+----+9	      6+----+8
 |\   |\	       |\   |\             |\   |\	       |\   |\
 | \  | \	       | \  | \            | \  | \	       | \  | \
 |  \ |  \	       |  \ |  \           |  \ |  \	       |  \ |  \
 |   \|   \	       |   \|   \          |   \|   \	       |   \|   \
 |+----+----+	       +----+----+         +----+----+	       +----+----+
   0     5     2	      0     7     3	  0     7     3	      2     9     3

   z ^                 z ^                 y ^                 z ^
 |                   |                   |                   |
 |+-->                +-->                +-->                +-->
       y                    x                  x                   x

   Triangle: face idx, #child

   [0,4,5]: 0, 0       [0,4,7]: 1, 0       [0,5,7]: 2, 0       [1,6,8]: 3, 1
   [1,4,6]: 0, 1       [1,4,8]: 1, 1       [2,5,9]: 2, 2       [2,6,9]: 3, 2
   [2,5,6]: 0, 2       [3,7,8]: 1, 3       [3,7,9]: 2, 3       [3,8,9]: 3, 3
   [4,5,6]: 0, 4       [4,7,8]: 2, 5       [5,7,9]: 1, 7       [6,8,9]: 3, 6

 */
static inline h5_err_t
get_children_of_loc_triangle (
        h5t_mesh_t* const m,
        h5_loc_idx_t face_idx,          // in
        h5_loc_idx_t elem_idx,          // in
        h5_loc_id_t* children           // out
        ) {
	h5_loc_idx_t map[4][4][2] = {
		{{0,0},{0,1},{0,2},{0,4}},      // face 0
		{{1,0},{1,1},{1,3},{2,5}},      // face 1
		{{2,0},{2,2},{2,3},{1,7}},      // face 2
		{{3,1},{3,2},{3,3},{3,6}}       // face 3
	};
	int num_faces = h5tpriv_ref_elem_get_num_facets (m);
	if ((face_idx < 0) || (face_idx >= num_faces)) {
		return h5_error_internal ();
	}
	h5_loc_idx_t idx = ((h5_loc_tet_t*)m->loc_elems)[elem_idx].child_idx;
	children[0] = h5tpriv_build_triangle_id (
	        map[face_idx][0][0], idx+map[face_idx][0][1]);
	children[1] = h5tpriv_build_triangle_id (
	        map[face_idx][1][0], idx+map[face_idx][1][1]);
	children[2] = h5tpriv_build_triangle_id (
	        map[face_idx][2][0], idx+map[face_idx][2][1]);
	children[3] = h5tpriv_build_triangle_id (
	        map[face_idx][3][0], idx+map[face_idx][3][1]);
	return H5_SUCCESS;
}

/*
   Return the two direct children of the edge given by face and
   element index of first child.
 */
static h5_err_t
get_children_of_loc_edge (
        h5t_mesh_t* const m,
        h5_loc_idx_t face_idx,
        h5_loc_idx_t elem_idx,
        h5_loc_id_t* children
        ) {
	/*
	   Please read the note about the offsets in the corresponding file
	   for triangle meshes.
	 */
	int offs[6][2] = { {0,1}, // edge 0
			   {0,2}, // edge 1
			   {1,2}, // edge 2
			   {0,3}, // edge 3
			   {1,3}, // edge 4
			   {2,3}  // edge 5
	};
	h5_loc_idx_t num_faces = h5tpriv_ref_elem_get_num_edges (m);
	if ((face_idx < 0) || (face_idx >= num_faces)) {
		return h5_error_internal ();
	}
	h5_loc_idx_t idx = ((h5_loc_tet_t*)m->loc_elems)[elem_idx].child_idx;
	children[0] = h5tpriv_build_edge_id (face_idx, idx+offs[face_idx][0]);
	children[1] = h5tpriv_build_edge_id (face_idx, idx+offs[face_idx][1]);
	return H5_SUCCESS;
}

static h5_err_t
get_loc_entity_children (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_loc_id_t* const children
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, entity_id=%llx, children=%p",
	                    m, (long long)entity_id, children);

	const h5_loc_id_t type_id = h5tpriv_get_entity_type (entity_id);
	const h5_loc_idx_t face_idx = h5tpriv_get_face_idx (entity_id);
	const h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (entity_id);

	if (h5tpriv_is_leaf_elem (m, &((h5_loc_tet_t*)m->loc_elems)[elem_idx])) {
		H5_LEAVE (H5_NOK);            // not refined
	}
	switch (type_id) {
	case H5T_TYPE_TET: {
		H5_LEAVE (
		        get_children_of_loc_elem (m, face_idx, elem_idx, children));
		break;
	}
	case H5T_TYPE_TRIANGLE: {
		H5_LEAVE (
		        get_children_of_loc_triangle (m, face_idx, elem_idx, children));
		break;
	}
	case H5T_TYPE_EDGE: {
		H5_LEAVE (
		        get_children_of_loc_edge (m, face_idx, elem_idx, children));
	}
	default:
		H5_LEAVE (h5_error_internal ());
	}
	H5_RETURN (H5_SUCCESS);
}

struct h5t_access_methods h5tpriv_access_tetm_methods = {
#if defined(WITH_PARALLEL_H5GRID)
	get_mpi_type_of_glb_elem,
#endif
	get_loc_elem,
	get_loc_elem_glb_idx,
	set_loc_elem_glb_idx,
	get_loc_elem_parent_idx,
	set_loc_elem_parent_idx,
	get_loc_elem_child_idx,
	set_loc_elem_child_idx,
	get_loc_elem_level_idx,
	set_loc_elem_level_idx,
	get_loc_elem_vertex_indices,
	get_loc_elem_vertex_indices_of_array,
	get_loc_elem_vertex_idx,
	set_loc_elem_vertex_idx,
	get_loc_elem_neighbor_indices,
	get_loc_elem_neighbor_idx,
	set_loc_elem_neighbor_idx,
	get_loc_entity_parent,
	get_loc_entity_children,
	alloc_glb_elems,
	get_glb_elem,
	copy_glb_elems,
	sort_glb_elems,
	get_glb_elem_idx,
	get_glb_elem_level,
	get_glb_elem_vertices,
	get_glb_elem_neighbors,
	set_geom_boundary_elem_flag,
	is_geom_boundary_elem,
	is_boundary_facet,
	is_boundary_face,
};

