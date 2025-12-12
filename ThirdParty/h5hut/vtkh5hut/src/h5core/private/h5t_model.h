/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/
#ifndef __PRIVATE_H5T_MODEL_H
#define __PRIVATE_H5T_MODEL_H

#include "h5core/h5_types.h"
#include "h5core/h5t_model.h"
#include "private/h5t_types.h"

#define H5T_CONTAINER_GRPNAME	"Topo"
#define TETRAHEDRAL_MESHES_GRPNAME "TetMeshes"
#define TRIANGLE_MESHES_GRPNAME "TriangleMeshes"

#if !(defined(ULLONG_MAX))
#define ULLONG_MAX  (LLONG_MAX * 2ULL + 1ULL)
#endif

/*
 ID's: 64bit

 Vertices:
   000100vv tttttttt tttttttt tttttttt tttttttt tttttttt tttttttt tttttttt
   3V TT TT TT TT TT TT

 Edges:
   00100eee tttttttt tttttttt tttttttt tttttttt tttttttt tttttttt tttttttt
   2E TT TT TT TT TT TT

 Trinagles:
   001100dd tttttttt tttttttt tttttttt tttttttt tttttttt tttttttt tttttttt
   1D TT TT TT TT TT TT

 Tets:
   01000000 tttttttt tttttttt tttttttt tttttttt tttttttt tttttttt tttttttt
   00 TT TT TT TT TT TT

*/

#define BITS_OF(x)		(sizeof(x)*CHAR_BIT)

#define H5T_TYPE_VERTEX		(1<<4)
#define H5T_TYPE_EDGE		(2<<4)
#define H5T_TYPE_TRIANGLE	(3<<4)
#define H5T_TYPE_TET		(4<<4)

#define H5T_FACE_MASK		(0x0f)
#define H5T_TYPE_MASK		(0x70)

#if 0
enum elem_types {
	vertex = 1,	// 1 vertex
	edge,		// 2 vertices
	triangle,	// 3 vertices
	quadrangle,	// 4 vertices
	tetrahedron,	// 4 vertices
	pyramid,	// 5 vertices
	prism,		// 6 vertices
	hexahedron	// 8 vertices
};
#endif

#define h5tpriv_build_entity_id( type, face_idx, elem_idx )	\
	(((type) | (face_idx)) << (BITS_OF(elem_idx)-8) | (elem_idx))

#define h5tpriv_build_vertex_id( face_idx, elem_idx )			\
	(h5tpriv_build_entity_id (H5T_TYPE_VERTEX, face_idx, elem_idx))

#define h5tpriv_build_edge_id( face_idx, elem_idx )			\
	(h5tpriv_build_entity_id (H5T_TYPE_EDGE, face_idx, elem_idx))

#define h5tpriv_build_triangle_id( face_idx, elem_idx )			\
	(h5tpriv_build_entity_id (H5T_TYPE_TRIANGLE, face_idx, elem_idx))

#define h5tpriv_build_tet_id( face_idx, elem_idx )			\
	(h5tpriv_build_entity_id (H5T_TYPE_TET, face_idx, elem_idx))

#define h5tpriv_get_entity_type( entity_id )	\
	((entity_id >> (BITS_OF(entity_id)-8)) & H5T_TYPE_MASK)

#define h5tpriv_get_face_idx( entity_id )				\
	(((entity_id) >> (BITS_OF(entity_id)-8)) & H5T_FACE_MASK)

#define h5tpriv_get_face_id( entity_id )				\
	(((entity_id) >> (BITS_OF(entity_id)-8)) & (H5T_TYPE_MASK|H5T_FACE_MASK))

#define h5tpriv_get_elem_idx( entity_id )			\
	(((entity_id) << 8) >> 8)


/*
  Test whether given element is on current level. This is the case, if
  - the level_id of the element is <= the current level
  - and, if any, the direct children is on a level > the current level
*/
#define h5tpriv_is_leaf_elem(m, el)					\
	( ((el)->level_idx <= m->leaf_level) &&				\
	  ((el)->child_idx < 0 || (el)->child_idx >= m->num_interior_elems[m->leaf_level]) )


#define H5T_BOUNDARY_ELEM_FLAG 1
#define H5T_BOUNDARY_FACET_FLAG 2

h5_err_t h5tpriv_alloc_loc_vertices (h5t_mesh_t* const, const h5_size_t);

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
        );

h5_err_t
h5tpriv_alloc_num_vertices (
	h5t_mesh_t* const m,
	const h5_size_t num
	);
#endif
