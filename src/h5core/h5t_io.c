/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include "private/h5_types.h"

#include <stdlib.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#define sleep(seconds) Sleep(seconds * 1000)
#endif

#ifdef WITH_PARALLEL_H5GRID
#include <parmetis.h>
#endif

#include "private/h5_attribs.h"
#include "private/h5_hdf5.h"

#include "private/h5_model.h"
#include "private/h5t_types.h"
#include "private/h5t_access.h"
#include "private/h5t_core.h"
#include "h5core/h5t_map.h"

int preferred_direction = 0;
// 0 x direction
// 1 y direction
// 2 z direction

int dont_use_parmetis = 0;
// 0 use parmetis
// 1 distribute with morton ordering
// 2 distribute geometrically with preferred direction


#include "private/h5t_types.h"
#include "private/h5t_model.h"
#include "private/h5t_map.h"
#include "private/h5t_adjacencies.h"
#include "private/h5t_io.h"
#include "private/h5t_store.h"
#include "private/h5t_core.h"
#include "private/h5_mpi.h"

#include <assert.h>

static hid_t
open_space_all (
		h5t_mesh_t* const m,
        const hid_t dataset_id
        ) {
	UNUSED_ARGUMENT (m);
	UNUSED_ARGUMENT (dataset_id);
	return H5S_ALL;
}

int
hidxmap_cmp (
        const void* __a,
        const void* __b
        ) {
	h5_idxmap_el_t* a = (h5_idxmap_el_t*)__a;
	h5_idxmap_el_t* b = (h5_idxmap_el_t*)__b;
	return b->glb_idx - a->glb_idx;
}

unsigned int
hidxmap_compute_hval (
        const void* __item
        ) {
	h5_idxmap_el_t* item = (h5_idxmap_el_t*)__item;
	uint16_t* key =  (uint16_t*)&item->glb_idx;
	unsigned int count = sizeof (item->glb_idx) / sizeof (uint16_t);
	unsigned int hval = count;
	while (count--) {
		if (*key) {
			hval <<= 6;
			hval += *key;
		}
		key++;
	}
	return hval;
}

/*
   Write vertices:
 * either we write a new dataset
 * or we append data to this dataset
 * appending means, a new level has been added
 * existing vertices will never be changed!

 */
static h5_err_t
write_vertices (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	assert (m->num_leaf_levels > 0);

	// quick hack for serial case (for the time being writes are serial anyway)
	for (size_t i = 0; i < m->num_leaf_levels; i++) {
		m->num_glb_vertices[i] = m->num_loc_vertices[i];
	}

	m->dsinfo_vertices.dims[0] = m->num_loc_vertices[m->num_leaf_levels-1];
	TRY( h5priv_write_dataset_by_name (
				 m,
	             m->f,
	             m->mesh_gid,
	             &m->dsinfo_vertices,
	             open_space_all,
	             open_space_all,
	             m->vertices) );

	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__num_vertices__",
	             H5_INT64_T,
	             m->num_glb_vertices,
	             m->num_leaf_levels));
	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__num_b_vertices__",
	             H5_INT64_T,
	             m->num_b_vtx,
	             m->num_leaf_levels));
	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__first_b_vertices__",
	             H5_INT64_T,
	             m->first_b_vtx,
	             m->num_leaf_levels));
	H5_RETURN (H5_SUCCESS);
}
#ifdef WITH_PARALLEL_H5GRID

static h5_err_t
add_chunk_to_list (
	h5t_mesh_t* const m,
	h5_loc_idxlist_t** list,
	h5_oct_idx_t oct_idx
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, list=%p, oct_idx=%d", m, list, oct_idx);
	h5t_oct_userdata_t* userdata;
	TRY (H5t_get_userdata_r (m->octree, oct_idx,(void **) &userdata));
		for (int i = 0; i < OCT_USERDATA_SIZE; i++) {
			if (userdata->idx[i] > -1) {
				h5priv_search_in_loc_idxlist(list, (h5_loc_idx_t)userdata->idx[i]);
			}
		}
	H5_RETURN (H5_SUCCESS);
}

#if 0
static int
compare_chk_idx (
	const void *p_a,
	const void *p_b
	) {
	return ((*(h5_chk_idx_t*)p_a) - (*(h5_chk_idx_t*)p_b));
}
#endif

h5_err_t
h5tpriv_get_list_of_chunks_to_write (
	h5t_mesh_t* const m,
	h5_chk_idx_t** list,
	int* counter
	) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p list=%p, counter=%p", m, list, counter);
	h5_chk_idx_t num_chunks = m->chunks->curr_idx + 1; // Is that ok? yes if update is correct

	int rank = m->f->myproc;
	int size_list = num_chunks;
	TRY ( *list = h5_calloc (num_chunks, sizeof (**list)));
	*counter = 0;
	h5_loc_idxlist_t* loc_list = NULL; //WARNING works only if chk_idx == loc_idx
	TRY (h5priv_alloc_loc_idxlist (&loc_list, size_list));

	// go through chunks and get those that belong to this proc
	for (h5_loc_idx_t i = 0; i < num_chunks; i++) {
		if (H5t_get_proc (m->octree, m->chunks->chunks[i].oct_idx) == rank ) {
			TRY (h5priv_search_in_loc_idxlist (&loc_list, i));
		}
	}
	*counter = loc_list->num_items;
	for (int i = 0; i< loc_list->num_items; i++) {
		(*list)[i] = (h5_chk_idx_t)loc_list->items[i];
	}
//	memcpy (*list, loc_list->items, sizeof(**list) * loc_list->num_items);
	TRY (h5priv_free_loc_idxlist (&loc_list));

	if (size_list < *counter) {
		h5_debug ("Overflow of list_of_chunks");
		H5_LEAVE (H5_ERR_INTERNAL);
	}
	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
exchange_g2l_vtx_map (
	h5t_mesh_t* const m,
	h5_idxmap_t* map,
	h5_glb_idx_t** range,
	h5_glb_idx_t** glb_vtx
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p map=%p, range=%p, glb_vtx=%p", m, map, range, glb_vtx);
	// alloc/get range
	TRY (*range = h5_calloc (m->f->nprocs + 1, sizeof (**range)));
	TRY (h5tpriv_get_ranges (m,*range, map->num_items, 0));

	//alloc glb_vtx
	TRY (*glb_vtx = h5_calloc ((*range)[m->f->nprocs], sizeof (**glb_vtx)));
	h5_glb_idx_t* sendbuf = NULL;
	TRY (sendbuf = h5_calloc (map->num_items, sizeof (*sendbuf)));
	int* recvcount = NULL;
	int* recvdisp = NULL;
	TRY (recvcount = h5_calloc (m->f->nprocs , sizeof (*recvcount)));
	TRY (recvdisp = h5_calloc (m->f->nprocs , sizeof (*recvdisp)));
	for (int i = 0; i < m->f->nprocs; i++) {
		recvdisp[i] = (int) (*range)[i];
		recvcount[i] = (int) ((*range)[i+1] - (*range)[i]);
	}
	for (int i = 0; i < map->num_items; i++) {
		sendbuf[i] = map->items[i].glb_idx;
	}

	TRY (h5priv_mpi_allgatherv (
				sendbuf,
				(int)map->num_items,
				MPI_LONG,
				*glb_vtx,
				recvcount,
				recvdisp,
				MPI_LONG,
				m->f->props->comm));
	H5_RETURN (H5_SUCCESS);
}
#endif

/*
 * instead of bsearch it returns the first element that fulfills compare(key,element) == 0 in an unsorted array
 * we don't want to sort array since it's also containing a permutation
 */
void*
linsearch (
	const void *key,
	void *array,
	size_t count,
	size_t size,
	comparison_fn_t compare
	) {
	void* pointer = array;
	for (int i = 0; i < count; i++) {
		if (compare.compare(key,pointer) == 0) {
			return pointer;
			break;
		}
	        pointer = (char*)pointer + size;
	}
	return NULL;
}

#if defined(WITH_PARALLEL_H5GRID)
static int
sort_glb_idx (
	const void *p_a,
	const void *p_b
	) {
	return (*(h5_glb_idx_t*)p_a) - (*(h5_glb_idx_t*)p_b);
}

static h5_err_t
remove_item_from_idxmap (
	h5_idxmap_t* map,
	int item_idx
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "map=%p, item_idx=%d", map, item_idx);
	assert(item_idx < map->num_items);

	memmove(&map->items[item_idx], &map->items[item_idx + 1], (map->num_items - 1 - item_idx) * sizeof (*map->items));
	map->num_items--;
	H5_RETURN (H5_SUCCESS);
}

/*
 * Check if any proc with lower rank already writes a vtx that this proc has planed to write
 * if so remove it from the map. Only the proc with the lowest rank writes the vertex
 */
static h5_err_t
check_multiple_vtx_writes (
	h5t_mesh_t* const m,
	h5_idxmap_t* map,
	h5_glb_idx_t* range,
	h5_glb_idx_t* glb_vtx
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p map=%p, range=%p, glb_vtx=%p", m, map, range, glb_vtx);
	if (m->f->myproc == 0) {
		H5_LEAVE (H5_SUCCESS);
	}
	h5_glb_idx_t num_glb_vtx = range[m->f->myproc];
	// sort glb_vtx up to my vtx
	qsort (glb_vtx,num_glb_vtx, sizeof (*glb_vtx), sort_glb_idx);

	h5_glb_idx_t* key = NULL;

//	comparison_fn_t comp_func;
//	comp_func.compare = sort_glb_idx;
	for (int i = 0; i < map->num_items; i++) {
		h5_glb_idx_t* retval = NULL;
		key = &map->items[i].glb_idx;
		retval = bsearch (key, glb_vtx, num_glb_vtx, sizeof (*glb_vtx), sort_glb_idx);
		if (retval != NULL) {
			// vertex already exist on proc with lower rank
			TRY (remove_item_from_idxmap (map, i));
			i--; // same position should be check again
		}
	}
	H5_RETURN (H5_SUCCESS);
}
#endif

h5_int32_t
h5priv_find_proc_to_write (
	h5t_mesh_t* const m,
	h5_loc_idx_t elem_idx
	) {
	H5_PRIV_API_ENTER (int, "m=%p ", m);
#ifdef WITH_PARALLEL_H5GRID
	h5_glb_idx_t glb_idx = h5tpriv_get_loc_elem_glb_idx (m, elem_idx);
	for (int i = 0; i < m->chunks->num_alloc; i++) {
		if ( glb_idx >= m->chunks->chunks[i].elem &&
				glb_idx < m->chunks->chunks[i].elem + m->chunks->chunks[i].num_elems) {
			H5_LEAVE (H5t_get_proc (m->octree, m->chunks->chunks[i].oct_idx));
		}
	}
#endif
	H5_RETURN (H5_SUCCESS);
}

#ifdef WITH_PARALLEL_H5GRID
/*
 * function returns a map with all the vtx that should be written by this proc
 *
 * first get all vtx that span elems that are written by this proc
 *
 * exchange the map and find which ones have to be written locally
 */
static h5_err_t
get_map_vertices_write (
	h5t_mesh_t* const m,
	h5_idxmap_t* map
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, map=%p", m, map);
	h5_chk_idx_t* list_of_chunks;
	int num_chunks = 0;
	TRY (h5tpriv_get_list_of_chunks_to_write (m, &list_of_chunks, &num_chunks));

	/* for the time being we use the hash table only for a fast test
	   whether a global index has already been added to the map or not. */
	h5_hashtable_t htab;
	TRY (h5priv_hcreate ((map->size << 2) / 3, &htab,
	                     hidxmap_cmp, hidxmap_compute_hval, NULL));

	int num_vertices = h5tpriv_ref_elem_get_num_vertices (m);

	// go through chunks
	for (int i = 0; i < num_chunks; i++) {
		h5_chk_idx_t chk_idx = list_of_chunks[i];
		h5_glb_idx_t glb_elem_idx = m->chunks->chunks[chk_idx].elem;
		h5_chk_size_t num_elem = m->chunks->chunks[chk_idx].num_elems;
		for (int j = 0; j < num_elem; j++) { // go through elements
			h5_loc_idx_t loc_elem_idx = h5t_map_glb_elem_idx2loc(m, glb_elem_idx + j);
			h5_loc_idx_t* vertices = h5tpriv_get_loc_elem_vertex_indices(m, loc_elem_idx);
			for (int k = 0; k < num_vertices; k++) {
				// get glb vertices
				h5_glb_idx_t glb_vidx = m->vertices[vertices[k]].idx;

				// add index temporarly to map ...
				map->items[map->num_items] = (h5_idxmap_el_t) {glb_vidx, vertices[k]};
				// ... and check whether it has already been added
				h5_idxmap_el_t* retval;
				h5priv_hsearch (&map->items[map->num_items],
				                H5_ENTER, (void**)&retval, &htab);
				if (retval == &map->items[map->num_items]) {
					// new entry in hash table thus in map
					map->num_items++;
				}

			}
		}
	}
	TRY (h5priv_hdestroy (&htab));
	h5priv_sort_idxmap (map);

	h5_glb_idx_t* range = NULL;
	h5_glb_idx_t* glb_vtx = NULL;
	// do exchange map
	TRY (exchange_g2l_vtx_map(m, map, &range, &glb_vtx));

	// check your vertices if they already appear on a proc with lower rank
	// if they appear, delete them from this map
	TRY (check_multiple_vtx_writes (m, map, range, glb_vtx));


	h5priv_sort_idxmap (map);
	TRY (h5_free (range));
	TRY (h5_free (glb_vtx));

	H5_RETURN (H5_SUCCESS);
}

//TODO maybe use ifdef to have name without _chk
static h5_err_t
write_vertices_chk (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	assert (m->num_leaf_levels > 0);
	hid_t dset_id;
	TRY (dset_id = hdf5_open_dataset_by_name (m->mesh_gid, m->dsinfo_vertices.name));
	hid_t mspace_id;
	hid_t dspace_id;


	h5_idxmap_t map_r;
	TRY (h5priv_new_idxmap (&map_r, m->num_loc_vertices[m->num_leaf_levels-1] + 128));
	h5_idxmap_t* map = &map_r;
	get_map_vertices_write (m, map);
	TRY (h5priv_mpi_barrier (m->f->props->comm));
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
	// create memspace
	hsize_t num_loc_vertices = m->num_loc_vertices[m->leaf_level];
	TRY (mspace_id = hdf5_create_dataspace(1, &num_loc_vertices, NULL));
	// add memspace
	hsize_t hstride = 1;
	H5S_seloper_t seloper = H5S_SELECT_SET; // first selection
	for (hsize_t i = 0; i < map->num_items; i++) {
		hsize_t hstart = map->items[i].loc_idx;
		hsize_t hcount = 1;
		while (map->items[i].loc_idx+1 == map->items[i+1].loc_idx &&
			       i+1 < map->num_items) {// WARINIG make sure +1 is right here
				i++; hcount++;
		}
		TRY (hdf5_select_hyperslab_of_dataspace (
			             mspace_id,
			             seloper,
			             &hstart, &hstride, &hcount,
			             NULL));
		seloper = H5S_SELECT_OR;
	}

	// create diskspace and select subset
	hsize_t num_glb_vertices = m->num_glb_vertices[m->num_leaf_levels-1];
	m->dsinfo_vertices.dims[0] = num_glb_vertices;
	TRY (dspace_id = hdf5_get_dataset_space (dset_id));
	TRY (hdf5_set_dataset_extent (dset_id, &num_glb_vertices));
	H5Sset_extent_simple(dspace_id, 1,m->dsinfo_vertices.dims, NULL); //TODO WRITE WRAPPER

	seloper = H5S_SELECT_SET; // first selection
	for (hsize_t i = 0; i < map->num_items; i++) {
		hsize_t hstart = map->items[i].glb_idx;
		hsize_t hcount = 1;
		while (map->items[i].glb_idx+1 == map->items[i+1].glb_idx &&
			       i+1 < map->num_items) { // WARINIG make sure +1 is right here
				i++; hcount++;
		}
		TRY (hdf5_select_hyperslab_of_dataspace (
			             dspace_id,
			             seloper,
			             &hstart, &hstride, &hcount,
			             NULL));
		seloper = H5S_SELECT_OR;
	}
	TRY (h5priv_mpi_barrier (m->f->props->comm));
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
	TRY (h5priv_start_throttle (m->f));

	TRY( h5priv_write_dataset_by_name_id (
	             m->f,
	             m->mesh_gid,
	             &m->dsinfo_vertices,
	             dset_id,
	             mspace_id,
	             dspace_id,
	             m->vertices) );


	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__num_vertices__",
	             H5_INT64_T,
	             m->num_glb_vertices,
	             m->num_leaf_levels));
	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__num_b_vertices__",
	             H5_INT64_T,
	             m->num_b_vtx,
	             m->num_leaf_levels));
	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__first_b_vertices__",
	             H5_INT64_T,
	             m->first_b_vtx,
	             m->num_leaf_levels));

	TRY (h5priv_end_throttle (m->f));
	TRY (hdf5_close_dataspace (dspace_id));
	TRY (hdf5_close_dataspace (mspace_id));
	TRY (hdf5_close_dataset (dset_id));
	m->f->empty = 0;

	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
write_elems (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	H5_RETURN (h5_error_not_implemented ());
}

#else

// SERIAL WRITE
static h5_err_t
write_elems (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	assert (m->num_leaf_levels > 0);

	h5_loc_idx_t num_interior_elems = m->num_interior_elems[m->num_leaf_levels-1];
	// alloc and inititalize data in memory
	h5_glb_elem_t* glb_elems;
	TRY (glb_elems = h5tpriv_alloc_glb_elems (m, num_interior_elems));
	TRY (h5tpriv_init_glb_elems_struct (m, glb_elems));

	m->dsinfo_elems.dims[0] = num_interior_elems;
	TRY (h5priv_write_dataset_by_name (
				 m,
	             m->f,
	             m->mesh_gid,
	             &m->dsinfo_elems,
	             open_space_all,
	             open_space_all,
	             glb_elems));

	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__num_elems__",
	             H5_INT64_T,
	             m->num_glb_elems,
	             m->num_leaf_levels));

	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__num_leaf_elems__",
	             H5_INT64_T,
	             m->num_glb_leaf_elems,
	             m->num_leaf_levels));

	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__num_leaf_levels__",
	             H5_INT16_T,
	             &m->num_leaf_levels,
	             1));

	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__is_chunked__",
	             H5_INT16_T,
	             &m->is_chunked,
	             1));
	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__num_weights__",
	             H5_INT32_T,
	             &m->num_weights,
	             1));
	// release mem
	TRY (h5_free (glb_elems));
	H5_RETURN (H5_SUCCESS);
}
#endif
#ifdef WITH_PARALLEL_H5GRID
//TODO maybe use ifdef to have name without _chk
static h5_err_t
write_elems_chk (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	assert (m->num_leaf_levels > 0);

	h5_chk_idx_t* chk_list = NULL;
	int num_chk = 0;
	// get my chunks to write
	TRY (h5tpriv_get_list_of_chunks_to_write(m, &chk_list, &num_chk));

	hsize_t num_elems = 0;
	for (int i = 0; i < num_chk; i++) {
		num_elems += m->chunks->chunks[chk_list[i]].num_elems;
	}
	// alloc and inititalize data in memory
	h5_glb_elem_t* glb_elems;
	TRY (glb_elems = h5tpriv_alloc_glb_elems (m, num_elems));
	TRY (h5tpriv_init_glb_elems_struct_chk (m, glb_elems, chk_list, num_chk));

	// could check here that glb_elems are in correct order

	hid_t dset_id;
	TRY (dset_id = hdf5_open_dataset_by_name (m->mesh_gid, m->dsinfo_elems.name));
	hid_t dspace_id;
	hid_t mspace_id;
	hsize_t hstart = 0;
	hsize_t hstride = 1;
	hsize_t hcount = num_elems;
	TRY (mspace_id = hdf5_create_dataspace (1, &num_elems, NULL));
	TRY (hdf5_select_hyperslab_of_dataspace (
				             mspace_id,
				             H5S_SELECT_SET,
				             &hstart, &hstride, &hcount,
				             NULL));


	// create diskspace and select subset
	hsize_t num_glb_elems = m->num_glb_elems[m->leaf_level];
	m->dsinfo_elems.dims[0] = num_glb_elems;
	TRY (dspace_id = hdf5_get_dataset_space (dset_id));
	TRY (hdf5_set_dataset_extent (dset_id, &num_glb_elems));
	H5Sset_extent_simple(dspace_id, 1,m->dsinfo_elems.dims, NULL); //TODO WRITE WRAPPER


	hsize_t hnext = h5tpriv_get_glb_elem_idx(m, glb_elems, 0);
	hsize_t hcurr = 0;
	// with those two variables the number of func calls can be reduced 3 times!
	H5S_seloper_t seloper = H5S_SELECT_SET; // first selection
	for (hsize_t i = 0; i < num_elems; i++) {
		hstart = hnext;
		hcurr = hnext;
		hcount = 1;
		while (i + 1 < num_elems &&
				(hnext = h5tpriv_get_glb_elem_idx(m, glb_elems, i + 1)) == hcurr + 1) {
				i++; hcount++; hcurr++;
		}
		TRY (hdf5_select_hyperslab_of_dataspace (
			             dspace_id,
			             seloper,
			             &hstart, &hstride, &hcount,
			             NULL));
		seloper = H5S_SELECT_OR;
	}
	// TODO add throttle

	TRY (h5priv_write_dataset_by_name_id(
	             m->f,
	             m->mesh_gid,
	             &m->dsinfo_elems,
	             dset_id,
	             mspace_id,
	             dspace_id,
	             glb_elems));

	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__num_elems__",
	             H5_INT64_T,
	             m->num_glb_elems,
	             m->num_leaf_levels));

	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__num_leaf_elems__",
	             H5_INT64_T,
	             m->num_glb_leaf_elems,
	             m->num_leaf_levels));

	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__num_leaf_levels__",
	             H5_INT16_T,
	             &m->num_leaf_levels,
	             1));

	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__is_chunked__",
	             H5_INT16_T,
	             &m->is_chunked,
	             1));

	// release mem
	TRY (h5_free (glb_elems));
	TRY (h5_free (chk_list));

	TRY (hdf5_close_dataspace (dspace_id));
	TRY (hdf5_close_dataspace (mspace_id));
	TRY (hdf5_close_dataset (dset_id));
	H5_RETURN (H5_SUCCESS);
}

static hid_t
set_chk_memspace (
	h5t_mesh_t* m,
	hid_t dataspace_id) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
			    "m=%p, dataspace_id=%lld",
			    m, (long long int)dataspace_id);
	hid_t mspace_id;
	TRY (mspace_id = hdf5_create_dataspace (1, m->dsinfo_chunks.dims, NULL));

	hsize_t hstride = 1;
	hsize_t hstart = 0;
	hsize_t hcount = m->dsinfo_chunks.dims[0];
	if (m->f->myproc == 0) {
		TRY (hdf5_select_hyperslab_of_dataspace (
				        mspace_id,
				        H5S_SELECT_SET,
				        &hstart, &hstride, &hcount,
				        NULL));
	} else {
		hcount = 0;
		TRY (hdf5_select_hyperslab_of_dataspace (
						        mspace_id,
						        H5S_SELECT_SET,
						        &hstart, &hstride, &hcount,
						        NULL));
	}
	H5_RETURN (mspace_id);
}

static hid_t
set_chk_diskspace (
	h5t_mesh_t* m,
	hid_t dspace_id
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
			    "m=%p, dataspace_id=%lld",
			    m, (long long int)dspace_id);

	H5Sset_extent_simple(dspace_id, 1,m->dsinfo_chunks.dims, NULL); //TODO WRITE WRAPPER

	hsize_t hstride = 1;
	hsize_t hstart = 0;
	hsize_t hcount = m->dsinfo_chunks.dims[0];
	if (m->f->myproc == 0) {
		TRY (hdf5_select_hyperslab_of_dataspace (
				        dspace_id,
				        H5S_SELECT_SET,
				        &hstart, &hstride, &hcount,
				        NULL));
	} else {
		hcount = 0;
		TRY (hdf5_select_hyperslab_of_dataspace (
						        dspace_id,
						        H5S_SELECT_SET,
						        &hstart, &hstride, &hcount,
						        NULL));
	}
	H5_RETURN (dspace_id);
}

static h5_err_t
write_chunks (
	h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	m->dsinfo_chunks.dims[0] = m->chunks->num_alloc;


	TRY (h5priv_write_dataset_by_name (
			m,
			m->f,
			m->mesh_gid,
			&m->dsinfo_chunks,
			set_chk_memspace,
			set_chk_diskspace,
			m->chunks->chunks));

	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__num_chunks__",
	             H5_INT32_T,
	             &m->chunks->num_alloc,
	             1));

	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__num_chk_levels__",
	             H5_INT16_T, 	// note: better uint16?
	             &m->chunks->num_levels,
	             1));

	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__num_chk_p_level__",
	             H5_INT32_T,
	             m->chunks->num_chunks_p_level,
	             m->chunks->num_levels));

	H5_RETURN (H5_SUCCESS);
}

static hid_t
set_oct_memspace (
	h5t_mesh_t* m,
	hid_t dataspace_id
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
			    "m=%p, dataspace_id=%lld",
			    m, (long long int)dataspace_id);
	hid_t mspace_id;
	TRY (mspace_id = hdf5_create_dataspace (1, m->dsinfo_octree.dims, NULL));

	hsize_t hstride = 1;
	hsize_t hstart = 0;
	hsize_t hcount = m->dsinfo_octree.dims[0];
	if (m->f->myproc == 0) {
		TRY (hdf5_select_hyperslab_of_dataspace (
				        mspace_id,
				        H5S_SELECT_SET,
				        &hstart, &hstride, &hcount,
				        NULL));
	} else {
		hcount = 0;
		TRY (hdf5_select_hyperslab_of_dataspace (
						        mspace_id,
						        H5S_SELECT_SET,
						        &hstart, &hstride, &hcount,
						        NULL));
	}
	H5_RETURN (mspace_id);
}

static hid_t
set_oct_diskspace (
	h5t_mesh_t* m,
	hid_t dspace_id
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
			    "m=%p, dataspace_id=%lld",
			    m, (long long int)dspace_id);

	H5Sset_extent_simple(dspace_id, 1,m->dsinfo_octree.dims, NULL); //TODO WRITE WRAPPER

	hsize_t hstride = 1;
	hsize_t hstart = 0;
	hsize_t hcount = m->dsinfo_octree.dims[0];
	if (m->f->myproc == 0) {
		TRY (hdf5_select_hyperslab_of_dataspace (
				        dspace_id,
				        H5S_SELECT_SET,
				        &hstart, &hstride, &hcount,
				        NULL));
	} else {
		hcount = 0;
		TRY (hdf5_select_hyperslab_of_dataspace (
						        dspace_id,
						        H5S_SELECT_SET,
						        &hstart, &hstride, &hcount,
						        NULL));
	}
	H5_RETURN (dspace_id);
}

static h5_err_t
write_octree (
	h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);

	TRY (H5t_update_internal (m->octree));
	TRY (H5t_update_userdata (m->octree));

	m->dsinfo_octree.dims[0] = m->octree->current_oct_idx + 1;

	TRY (h5priv_write_dataset_by_name (
		     m,
		     m->f,
		     m->mesh_gid,
		     &m->dsinfo_octree,
		     set_oct_memspace,
		     set_oct_diskspace,
		     m->octree->octants));

	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__curr_oct_idx__",
	             H5_INT32_T,
	             &m->octree->current_oct_idx,
	             1));

	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__oct_maxpoints__",
	             H5_INT32_T,
	             &m->octree->maxpoints,
	             1));
	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__oct_size_userdata__",
	             H5_INT32_T,
	             &m->octree->size_userdata,
	             1));
	TRY (h5priv_write_attrib (
		     m->mesh_gid,
		     "__oct_bounding_box__",
		     H5_FLOAT64_T,
		     m->octree->bounding_box,
		     6));

	if (m->octree->size_userdata > 0) {
		m->dsinfo_userdata.dims[0] = m->octree->current_oct_idx + 1;

		TRY (h5priv_write_dataset_by_name (
					m,
					m->f,
					m->mesh_gid,
					&m->dsinfo_userdata,
					set_oct_memspace,    // should work fine same size as octree
					set_oct_diskspace,
					m->octree->userdata));


	}

	H5_RETURN (H5_SUCCESS);
}

static hid_t
set_weight_memspace (
	h5t_mesh_t* m,
	hid_t dataspace_id) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
			    "m=%p, dataspace_id=%lld",
			    m, (long long int)dataspace_id);
	hid_t mspace_id;
	TRY (mspace_id = hdf5_create_dataspace (1, m->dsinfo_weights.dims, NULL));

	hsize_t hstride = 1;
	hsize_t hstart = 0;
	hsize_t hcount = m->dsinfo_weights.dims[0];
	if (m->f->myproc == 0) {
		TRY (hdf5_select_hyperslab_of_dataspace (
				        mspace_id,
				        H5S_SELECT_SET,
				        &hstart, &hstride, &hcount,
				        NULL));
	} else {
		hcount = 0;
		TRY (hdf5_select_hyperslab_of_dataspace (
						        mspace_id,
						        H5S_SELECT_SET,
						        &hstart, &hstride, &hcount,
						        NULL));
	}
	H5_RETURN (mspace_id);
}

static hid_t
set_weight_diskspace (
	h5t_mesh_t* m,
	hid_t dspace_id
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
			    "m=%p, dataspace_id=%lld",
			    m, (long long int)dspace_id);

	H5Sset_extent_simple(dspace_id, 1,m->dsinfo_weights.dims, NULL); //TODO WRITE WRAPPER

	hsize_t hstride = 1;
	hsize_t hstart = 0;
	hsize_t hcount = m->dsinfo_weights.dims[0];
	if (m->f->myproc == 0) {
		TRY (hdf5_select_hyperslab_of_dataspace (
				        dspace_id,
				        H5S_SELECT_SET,
				        &hstart, &hstride, &hcount,
				        NULL));
	} else {
		hcount = 0;
		TRY (hdf5_select_hyperslab_of_dataspace (
						        dspace_id,
						        H5S_SELECT_SET,
						        &hstart, &hstride, &hcount,
						        NULL));
	}
	H5_RETURN (dspace_id);
}

/*
 * weights is an array that stores the element weights. c weights per element with n elements, gives a size of c*n.
 * where the weights of an element are stored contiguosly. i.e. first weight of second elem is at weights[c*1].
 */
static h5_err_t
write_weights (
	h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);

	m->dsinfo_weights.dims[0] = m->num_glb_elems[m->leaf_level] * m->num_weights;

	TRY (h5priv_write_dataset_by_name (
			m,
			m->f,
			m->mesh_gid,
			&m->dsinfo_weights,
			set_weight_memspace,
			set_weight_diskspace,
			m->weights));

	TRY (h5priv_write_attrib (
	             m->mesh_gid,
	             "__num_weights__",
	             H5_INT32_T,
	             &m->num_weights,
	             1));

	H5_RETURN (H5_SUCCESS);
}
#endif

h5_err_t
h5tpriv_write_mesh (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p", m);
	if (m->mesh_changed) {
		if (m->is_chunked) {
#ifdef WITH_PARALLEL_H5GRID
			if (m->num_weights > 0) {
				TRY (write_weights (m));
			}
			TRY (h5priv_mpi_barrier (m->f->props->comm));
			m->timing.measure[m->timing.next_time++] = MPI_Wtime();
			TRY (write_chunks (m));
			TRY (h5priv_mpi_barrier (m->f->props->comm));
			m->timing.measure[m->timing.next_time++] = MPI_Wtime();
			TRY (write_octree (m));
			TRY (h5priv_mpi_barrier (m->f->props->comm));
			m->timing.measure[m->timing.next_time++] = MPI_Wtime();
			if (m->f->nprocs > 1) {
				TRY (write_vertices_chk (m));
				TRY (h5priv_mpi_barrier (m->f->props->comm));
				m->timing.measure[m->timing.next_time++] = MPI_Wtime();
				TRY (write_elems_chk (m));
				TRY (h5priv_mpi_barrier (m->f->props->comm));
				m->timing.measure[m->timing.next_time++] = MPI_Wtime();
			} else {
				TRY (write_vertices (m));
				TRY (h5priv_mpi_barrier (m->f->props->comm));
				m->timing.measure[m->timing.next_time++] = MPI_Wtime();
				TRY (write_elems (m));
				TRY (h5priv_mpi_barrier (m->f->props->comm));
				m->timing.measure[m->timing.next_time++] = MPI_Wtime();
			}
#endif			
		} else {
			TRY (write_vertices (m));
			TRY (write_elems (m));
		}



	}
	H5_RETURN (H5_SUCCESS);
}
// ANCHOR WRITE
/*
   Read vertices from file. If map is NULL, read *all* vertices otherwise the
   vertices specified in the map.
 */
static h5_err_t
read_vertices (
        h5t_mesh_t* m,
        h5_idxmap_t* map
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	hid_t dset_id;
	TRY (dset_id = hdf5_open_dataset_by_name (m->mesh_gid, m->dsinfo_vertices.name));
	hid_t mspace_id = H5S_ALL;
	hid_t dspace_id = H5S_ALL;

	if (map) {
		m->num_loc_vertices[m->num_leaf_levels-1] = map->num_items;
		m->last_stored_vid = m->num_loc_vertices[m->num_leaf_levels-1] - 1;
		TRY (h5tpriv_alloc_loc_vertices (m, map->num_items));

		// create memspace
		hsize_t num_loc_vertices = map->num_items;
		TRY (mspace_id = hdf5_create_dataspace(1, &num_loc_vertices, NULL));

		// create diskspace and select subset
		hsize_t num_glb_vertices = m->num_glb_vertices[m->num_leaf_levels-1];
		TRY (dspace_id = hdf5_create_dataspace (1, &num_glb_vertices, NULL));
		hsize_t hstride = 1;
		H5S_seloper_t seloper = H5S_SELECT_SET; // first selection
		for (hsize_t i = 0; i < map->num_items; i++) {
			hsize_t hstart = map->items[i].glb_idx;
			hsize_t hcount = 1;
			while (map->items[i].glb_idx+1 == map->items[i+1].glb_idx &&
			       i + 1 < map->num_items) {// WARNING check if +1 is correct here
				i++; hcount++;
			}
			TRY (hdf5_select_hyperslab_of_dataspace (
			             dspace_id,
			             seloper,
			             &hstart, &hstride, &hcount,
			             NULL));
			seloper = H5S_SELECT_OR;
		}
	} else {
		size_t num_vertices =  m->num_glb_vertices[m->num_leaf_levels-1];
		m->last_stored_vid = m->num_glb_vertices[m->num_leaf_levels-1] - 1;
		TRY (h5tpriv_alloc_loc_vertices (m, num_vertices));
	}

	TRY (h5priv_start_throttle (m->f));
	TRY (hdf5_read_dataset (
	             dset_id,
	             m->dsinfo_vertices.type_id,
	             mspace_id,
	             dspace_id,
	             m->f->props->xfer_prop,
	             m->vertices));
	TRY (h5priv_end_throttle (m->f));
	TRY (hdf5_close_dataspace (dspace_id));
	TRY (hdf5_close_dataspace (mspace_id));
	TRY (hdf5_close_dataset (dset_id));

	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
read_elems (
        h5t_mesh_t* const m,
        h5_loc_idx_t start,
        h5_loc_idx_t count,
        h5_glb_elem_t* glb_elems
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, start=%lld, count=%lld",
	                    m, (long long)start, (long long)count);

	hid_t dset_id;
	TRY (dset_id = hdf5_open_dataset_by_name (m->mesh_gid, m->dsinfo_elems.name));
	hid_t mspace_id;
	hsize_t hcount = (hsize_t)count;
	TRY (mspace_id = hdf5_create_dataspace (1, &hcount, NULL));

	hid_t dspace_id;
	hsize_t hstart = (hsize_t)start;
	hsize_t hstride = 1;
	hsize_t num_glb_elems = m->num_glb_elems[m->num_leaf_levels-1];
	TRY (dspace_id = hdf5_create_dataspace (1, &num_glb_elems, NULL));
	TRY (hdf5_select_hyperslab_of_dataspace (
	             dspace_id,
	             H5S_SELECT_SET,
	             &hstart, &hstride, &hcount,
	             NULL));

	TRY (h5priv_start_throttle (m->f));
	TRY (hdf5_read_dataset (
	             dset_id,
	             m->dsinfo_elems.type_id,
	             mspace_id,
	             dspace_id,
	             m->f->props->xfer_prop,
	             glb_elems));
	TRY (h5priv_end_throttle (m->f));

	TRY (hdf5_close_dataspace (dspace_id));
	TRY (hdf5_close_dataspace (mspace_id));
	TRY (hdf5_close_dataset (dset_id));

	H5_RETURN (H5_SUCCESS);
}



#if defined(WITH_PARALLEL_H5GRID)
// READ MESH PARALLEL
/*
   Partition mesh via dual graph partitioning

   Step 1: Partiton dual graph of mesh, all procs have their global
          cell data loaded.
   Step 2: Handle ghost- and border cells
   Step 3: Read vertices
 */
static h5_err_t
part_kway  (
        h5t_mesh_t* const m,
        h5_glb_elem_t** glb_elems
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	/*
	   Step 1:
	   - read num cell/num procs cells
	   - call partitioner
	   - each proc must know the number of cells assigned to his
	    partition on foreign procs.
	   - send global cell data to right proc
	 */

	// compute initial distribution of cells on all procs
	int n = m->num_glb_elems[0] / m->f->nprocs;
	int r = m->num_glb_elems[0] % m->f->nprocs;

	idx_t* vtxdist;
	TRY (vtxdist = h5_calloc (m->f->nprocs+1, sizeof (*vtxdist)));
	vtxdist[0] = 0;
	h5_debug ("vtxdist[%d]: %d", 0, 0);
	for (int i = 1; i < m->f->nprocs+1; i++) {
		if (r > 0) {
			vtxdist[i] = vtxdist[i-1] + n + 1;
			r--;
		} else {
			vtxdist[i] = vtxdist[i-1] + n;
		}
		h5_debug ("vtxdist[%d]: %d", i, vtxdist[i]);
	}
	// read cells only
	idx_t start = vtxdist[m->f->myproc];
	idx_t num_interior_elems = vtxdist[m->f->myproc+1] - start;
	m->num_interior_elems[0] = m->num_interior_leaf_elems[0] = num_interior_elems;

	h5_glb_elem_t* elems;
	TRY (elems = h5tpriv_alloc_glb_elems (m, num_interior_elems));

	TRY (read_elems (m, start, num_interior_elems, elems));


	// setup input for ParMETIS
	idx_t* xadj;
	idx_t* adjncy;
	idx_t* part;
	TRY (xadj = h5_calloc (num_interior_elems, sizeof(*xadj)));
	TRY (part = h5_calloc (num_interior_elems, sizeof(*part)));
	//  TODO: 4*num_interior_elems will work for meshes with up to 4 facets only!
	TRY (adjncy = h5_calloc (4*num_interior_elems, sizeof(*adjncy)));
	idx_t i, j;
	int num_facets = h5tpriv_ref_elem_get_num_facets (m);
	for (i = 0, j = 0; i < num_interior_elems; i++) {
		h5_glb_idx_t* neighbors = h5tpriv_get_glb_elem_neighbors(m, elems, i);
		xadj[i] = j;
		h5_debug ("xadj[%d]: %d", i, j);
		// for all facets
		for (int l = 0; l < num_facets; l++) {
			if (neighbors[l] < 0) continue;
			adjncy[j] = neighbors[l];
			h5_debug ("adjncy[%d]: %d", j, adjncy[j]);
			j++;
		}
	}
	xadj[num_interior_elems] = j;
	h5_debug ("xadj[%lld]: %d", (long long)num_interior_elems, j);
	// now we can call the partitioner
	idx_t wgtflag = 0;
	idx_t numflag = 0;
	idx_t ncon = 1;
	idx_t nparts = m->f->nprocs;
	real_t* tpwgts;
	real_t* ubvec;
	idx_t options[] = {1,127,42};
	idx_t edgecut;
	h5_debug ("nparts: %d", nparts);
	TRY (tpwgts = h5_calloc (nparts, sizeof(*tpwgts)));
	TRY (ubvec = h5_calloc (nparts, sizeof(*ubvec)));
	for (i = 0; i < nparts; i++) {
		tpwgts[i] = 1.0 / (real_t)nparts;
		ubvec[i] = 1.05;
	}
	int rc = ParMETIS_V3_PartKway (
	        vtxdist,
	        xadj,
	        adjncy,
	        NULL,           // vwgt
	        NULL,           // adjwgt
	        &wgtflag,
	        &numflag,
	        &ncon,
	        &nparts,
	        tpwgts,
	        ubvec,
	        options,
	        &edgecut,
	        part,
	        &m->f->props->comm
	        );
	if (rc != METIS_OK) {
		H5_RETURN_ERROR (
		        H5_ERR,
			"ParMETIS failed");
	}
	TRY (h5_free (vtxdist));
	TRY (h5_free (xadj));
	TRY (h5_free (adjncy));
	TRY (h5_free (tpwgts));
	TRY (h5_free (ubvec));

#if !defined(NDEBUG)
	for (i = 0; i < num_interior_elems; i++) {
		h5_debug ("part[%d]: %llu", i, (unsigned long long)part[i]);
	}
#endif
	/*
	   Now we know the partitioning, but the cells aren't distributed
	   accordingly.

	   Eeach processor knows the number of cells he has to send the other
	   procs in the group, but still don't how many cells he will receive
	   from the other procs in the group.
	 */

	// So, let's  count the number of cells we have to send to each proc.
	int* sendcounts;
	int* recvcounts;
	TRY (sendcounts = h5_calloc (nparts, sizeof(sendcounts[0])));
	TRY (recvcounts = h5_calloc (nparts, sizeof(recvcounts[0])));
	for (i = 0; i < num_interior_elems; i++) {
		sendcounts[part[i]]++;
	}
	// send these numbers to according procs
	TRY (h5priv_mpi_alltoall (
	             sendcounts, 1, MPI_INT,
	             recvcounts, 1, MPI_INT,
	             m->f->props->comm));

	for (i = 0; i < nparts; i++) {
		h5_debug ("sendcounts[%d]: %d", i, sendcounts[i]);
		h5_debug ("recvcounts[%d]: %d", i, recvcounts[i]);
	}
	/*
	   next step is to scatter the cells to their procs
	   via an all-to-all communication:
	   - allocate and setup send buffer
	   - allocate and setup recv buffer
	   - setup MPI data type
	   - scatter all-to-all
	 */

	// allocate and initialize send buffer num_interior_elems*sizeof(cell)
	h5_glb_elem_t* sendbuf;
	TRY (sendbuf = h5tpriv_alloc_glb_elems (m, num_interior_elems));
	int* senddispls;
	TRY (senddispls = h5_calloc (m->f->nprocs, sizeof (senddispls[0])));

	senddispls[0] = 0;
	for (i = 0; i < m->f->nprocs-1; i++) {
		senddispls[i+1] = senddispls[i] + sendcounts[i];
		sendcounts[i] = 0;
	}
	sendcounts[i] = 0;
	for (i = 0; i < num_interior_elems; i++) {
		size_t sendidx = senddispls[part[i]] + sendcounts[part[i]];
		h5tpriv_copy_glb_elems (
		        m,
		        sendbuf, sendidx,
		        elems, i, 1);
		sendcounts[part[i]]++;
	}
	for (i = 0; i < num_interior_elems; i++) {
		h5_debug ("sendbuf[%d]: %lld", i, h5tpriv_get_glb_elem_idx (m, sendbuf, i));
	}
	TRY (h5_free (part));

	// allocate and initialize recv buffer
	int* recvdispls;
	recvdispls = h5_calloc (m->f->nprocs, sizeof (recvdispls[0]));
	recvdispls[0] = 0;
	for (i = 0; i < m->f->nprocs-1; i++) {
		recvdispls[i+1] = recvdispls[i] + recvcounts[i];
	}
	num_interior_elems = recvdispls[i] + recvcounts[i];
	h5_glb_elem_t* recvbuf;
	TRY (recvbuf = h5tpriv_alloc_glb_elems (m, num_interior_elems));
	// scatter elems all to all
	MPI_Datatype type = h5tpriv_get_mpi_type_of_glb_elem (m);
	TRY (h5priv_mpi_alltoallv (
	             sendbuf, sendcounts, senddispls, type,
	             recvbuf, recvcounts, recvdispls, type,
	             m->f->props->comm));
	for (i = 0; i < num_interior_elems; i++) {
		h5_debug ("global cell ID[%d]: %lld",
		          i, h5tpriv_get_glb_elem_idx (m, recvbuf, i));
	}
	TRY (h5_free (sendbuf));
	TRY (h5_free (sendcounts));
	TRY (h5_free (senddispls));
	TRY (h5_free (recvcounts));
	TRY (h5_free (recvdispls));

	TRY (h5_free (elems));
	*glb_elems = recvbuf;

	H5_RETURN (H5_SUCCESS);
}


/*
   exchange ghost cells:
   idea: send my ghost cells to all procs
   1. compute ghost cells
   2. allgather ghost cell IDs: each proc knows the ghost cell IDs of all procs
   3. compute number of border cells to scatter to each proc
   4. scatter these numbers
   5. build array with (border)cells to scatter (note: we may have to scatter
     the same cell to multiple procs)
   6. scatter border cells with alltoallv
 */
static h5_err_t
exchange_ghost_cells (
        h5t_mesh_t* const m,
        h5_glb_elem_t* glb_elems,
        h5_glb_elem_t** ghost_elems,
        size_t* num_ghost_elems
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, ghost_elems=%p", m, ghost_elems);
	int* sendcounts;
	int* senddispls;
	int* recvcounts;
	int* recvdispls;
	int nprocs = m->f->nprocs;
	TRY (sendcounts = h5_calloc (nprocs, sizeof(*sendcounts)));
	TRY (senddispls = h5_calloc (nprocs, sizeof(*senddispls)));
	TRY (recvcounts = h5_calloc (nprocs, sizeof(*recvcounts)));
	TRY (recvdispls = h5_calloc (nprocs, sizeof(*recvdispls)));

	// determine my ghost cells
	h5_glb_idxlist_t* loc_ghostcell_ids = NULL;
	int num_facets = h5tpriv_ref_elem_get_num_facets (m);
	for (int i = 0; i < m->num_interior_elems[0]; i++) {
		h5_glb_idx_t* neighbors = h5tpriv_get_glb_elem_neighbors(m, glb_elems, i);
		for (int facet = 0; facet < num_facets; facet++) {
			if (neighbors[facet] == -1) {
				// geometric boundary
				continue;
			}
			if (h5priv_search_idxmap (&m->map_elem_g2l, neighbors[facet]) >= 0) {
				// neighbor is local
				continue;
			}
			// neighbor is *not* local
			TRY (h5priv_search_in_glb_idxlist (&loc_ghostcell_ids, neighbors[facet]));
			h5_debug ("ghost cell: %lld", neighbors[facet]);
		}
	}
	// allgather number of ghost cells
	int* num_ghostcells;
	TRY (num_ghostcells = h5_calloc (nprocs, sizeof(num_ghostcells[0])));
	TRY (mpi_allgather (&loc_ghostcell_ids->num_items, 1, MPI_INT,
	                    num_ghostcells, 1, MPI_INT, m->f->props->comm));
	for (int i = 0; i < nprocs; i++) {
		h5_debug ("num_ghostcells[%d] = %d", i, num_ghostcells[i]);
	}

	// allgather ghost cell IDs
	int num_ghostcells_total = num_ghostcells[0];
	recvdispls[0] = 0;
	for (int i = 1; i < nprocs; i++) {
		recvdispls[i] = num_ghostcells_total;
		num_ghostcells_total += num_ghostcells[i];
	}
	h5_glb_id_t* ghostcells;
	TRY (ghostcells = h5_calloc (num_ghostcells_total, sizeof(*ghostcells)));
	TRY (mpi_allgatherv (
	             loc_ghostcell_ids->items, (int)loc_ghostcell_ids->num_items,
	             MPI_LONG_LONG,
	             ghostcells, num_ghostcells, recvdispls,
	             MPI_LONG_LONG,
	             m->f->props->comm));

	for (int i = 0; i < num_ghostcells_total; i++) {
		h5_debug ("ghostcells[%d] = %lld", i, ghostcells[i]);
	}

	// scatter my border cells
	// - count and collect border cells we have to scatter
	h5_loc_idxlist_t** belem_lists;
	TRY (belem_lists = h5_calloc (nprocs, sizeof (*belem_lists)));
	h5_loc_idx_t num_elems_to_scatter_total = 0;
	for (int proc = 0; proc < nprocs; proc++) {
		h5_loc_idxlist_t** list = &belem_lists[proc];
		h5_loc_idx_t last = recvdispls[proc] + num_ghostcells[proc] - 1;
		for (int i = recvdispls[proc]; i <= last; i++) {
			// is cell with ID local?
			h5_loc_idx_t idx;
			idx = h5priv_search_idxmap (&m->map_elem_g2l, ghostcells[i]);
			if (idx >= 0) {
				// yes: we have to send this cell to proc
				// add to collection
				TRY (h5priv_insert_into_loc_idxlist (list, idx, -1));
				num_elems_to_scatter_total++;
			}
		}
	}
	TRY (h5_free (num_ghostcells));
	TRY (h5_free (ghostcells));

	// - allocate memory for border cells we have to scatter
	h5_glb_elem_t* sendbuf;
	TRY (sendbuf = h5tpriv_alloc_glb_elems (m, num_elems_to_scatter_total));

	// - setup send buffer
	for (int sendidx = 0, proc = 0; proc < nprocs; proc++) {
		h5_loc_idxlist_t* list = belem_lists[proc];
		if (list) {
			sendcounts[proc] = list->num_items;
			senddispls[proc] = sendidx;
			for (size_t i = 0; i < list->num_items; i++, sendidx++) {
				// copy glb cell at list->item[i] to sendbuf
				h5tpriv_copy_glb_elems (
				        m,
				        sendbuf, sendidx,
				        glb_elems,
				        list->items[i],
				        1);
			}
			TRY (h5_free (list));
		} else { // nothing to to for this proc
			sendcounts[proc] = 0;
			senddispls[proc] = sendidx;
		}
	}
	TRY (h5_free (belem_lists));

	// - scatter send counts
	TRY (h5priv_mpi_alltoall (
	             sendcounts, 1, MPI_INT,
	             recvcounts, 1, MPI_INT,
	             m->f->props->comm));

	// compute receive displacements and number of local ghost cells
	recvdispls[0] = 0;
	for (int proc = 0; proc < nprocs-1; proc++) {
		recvdispls[proc+1] = recvdispls[proc] + recvcounts[proc];
	}
	int num_loc_ghost_elems = recvdispls[nprocs-1] + recvcounts[nprocs-1];

	// - scatter ghost cells alltoall
	h5_glb_elem_t* recvbuf;
	TRY (recvbuf = h5tpriv_alloc_glb_elems (m, num_loc_ghost_elems));
	MPI_Datatype type = h5tpriv_get_mpi_type_of_glb_elem (m);

	TRY (h5priv_mpi_alltoallv (
	             sendbuf, sendcounts, senddispls, type,
	             recvbuf, recvcounts, recvdispls, type,
	             m->f->props->comm));

	for (int i = 0; i < num_loc_ghost_elems; i++) {
		h5_debug ("global ghost cell ID[%d]: %lld",
		          i, h5tpriv_get_glb_elem_idx (m, recvbuf, i));
	}
	TRY (h5_free (sendbuf));

	TRY (h5_free (sendcounts));
	TRY (h5_free (senddispls));
	TRY (h5_free (recvcounts));
	TRY (h5_free (recvdispls));

	*ghost_elems = recvbuf;
	*num_ghost_elems = num_loc_ghost_elems;
	H5_RETURN (H5_SUCCESS);
}


h5_err_t
h5tpriv_read_mesh (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p", m);
	h5_glb_elem_t* glb_elems = NULL;
	TRY (part_kway (m, &glb_elems));
	h5_loc_idx_t num_interior_elems = m->num_interior_elems[0];

	// add interior elements to global -> local index map
	TRY (h5tpriv_init_map_elem_g2l (m, glb_elems, num_interior_elems));

	// gather ghost cells
	h5_glb_elem_t* ghost_elems;
	size_t num_ghost_elems;
	TRY (exchange_ghost_cells (m, glb_elems, &ghost_elems, &num_ghost_elems));
	m->num_ghost_elems[0] = num_ghost_elems;

	// add ghost cells to global -> local index map
	TRY (h5tpriv_init_map_elem_g2l (m, ghost_elems, num_ghost_elems));

	// define local indices for all vertices of all local elements
	size_t size = num_interior_elems+num_ghost_elems;
	TRY (h5priv_new_idxmap (&m->map_vertex_g2l, size+128));

	h5_idxmap_t* map = &m->map_vertex_g2l;

	/* for the time being we use the hash table only for a fast test
	   whether a global index has already been added to the map or not. */
	h5_hashtable_t htab;
	TRY (h5priv_hcreate ((size << 2) / 3, &htab,
	                     hidxmap_cmp, hidxmap_compute_hval, NULL));

	int num_vertices = h5tpriv_ref_elem_get_num_vertices (m);
	for (size_t idx = 0; idx < num_interior_elems; idx++) {
		h5_glb_idx_t* vertices = h5tpriv_get_glb_elem_vertices (m, glb_elems, idx);
		for (int i = 0; i < num_vertices; i++) {
			// add index temporarly to map ...
			map->items[map->num_items] = (h5_idxmap_el_t) {vertices[i], 0};
			// ... and check whether it has already been added
			h5_idxmap_el_t* retval;
			h5priv_hsearch (&map->items[map->num_items],
			                H5_ENTER, (void**)&retval, &htab);
			if (retval == &map->items[map->num_items]) {
				// new entry in hash table thus in map
				map->num_items++;
			}
		}
	}
	// same for ghost cells
	for (size_t idx = 0; idx < num_ghost_elems; idx++) {
		h5_glb_idx_t* vertices = h5tpriv_get_glb_elem_vertices (m, ghost_elems, idx);
		for (int i = 0; i < num_vertices; i++) {
			// add index temporarly to map ...
			map->items[map->num_items] = (h5_idxmap_el_t) {vertices[i], 0};
			// ... and check whether it has already been added
			h5_idxmap_el_t* retval;
			h5priv_hsearch (&map->items[map->num_items],
			                H5_ENTER, (void**)&retval, &htab);
			if (retval == &map->items[map->num_items]) {
				// new entry in hash table thus in map
				map->num_items++;
			}
		}
	}
	TRY (h5priv_hdestroy (&htab));
	h5priv_sort_idxmap (map);
	for (h5_loc_idx_t i = 0; i < map->num_items; i++) {
		map->items[i].loc_idx = i;
	}
	TRY (read_vertices (m, map));

	TRY (h5tpriv_alloc_loc_elems (m, 0, num_interior_elems+num_ghost_elems));
	m->num_loaded_levels = 1;

	TRY (h5tpriv_init_loc_elems_struct (m, glb_elems, 0, num_interior_elems, 0, NULL));
	TRY (h5tpriv_init_loc_elems_struct (
		     m, ghost_elems, num_interior_elems, num_ghost_elems, H5_GHOST_ENTITY, NULL));

	TRY (h5_free (glb_elems));
	TRY (h5_free (ghost_elems));

	TRY (h5tpriv_init_elem_flags (m, 0, num_interior_elems+num_ghost_elems));

	TRY (h5tpriv_update_internal_structs (m, 0));
	H5_RETURN (H5_SUCCESS);
}
#else

// THE SERIAL VERSION ...
h5_err_t
h5tpriv_read_mesh (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p", m);
	// local and global counts are identical in serial case
	for (size_t lvl = 0; lvl < m->num_leaf_levels; lvl++) {
		m->num_loc_vertices[lvl] = m->num_glb_vertices[lvl];
		m->num_interior_elems[lvl] = m->num_glb_elems[lvl];
		m->num_interior_leaf_elems[lvl] = m->num_glb_leaf_elems[lvl];
	}

	TRY (read_vertices (m, NULL));
	TRY (h5tpriv_rebuild_map_vertex_g2l (m, 0, m->num_leaf_levels-1));

	h5_loc_idx_t num_interior_elems = m->num_interior_elems[m->num_leaf_levels-1];
	h5_glb_elem_t* glb_elems;
	TRY (glb_elems = h5tpriv_alloc_glb_elems (m, num_interior_elems));
	TRY (read_elems (m, 0, num_interior_elems, glb_elems));
	TRY (h5tpriv_alloc_loc_elems (m, 0, num_interior_elems));
	m->num_loaded_levels = m->num_leaf_levels;

	TRY (h5tpriv_init_map_elem_g2l (m, glb_elems, num_interior_elems));
	TRY (h5tpriv_init_loc_elems_struct (m, glb_elems, 0, num_interior_elems, 0, NULL));
	TRY (h5tpriv_update_internal_structs (m, 0));
	TRY (h5tpriv_init_elem_flags (m, 0, num_interior_elems));
	TRY (h5_free (glb_elems));

	H5_RETURN (H5_SUCCESS);
}
#endif


#ifdef WITH_PARALLEL_H5GRID
static h5_err_t
read_octree (
        h5t_mesh_t* m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	hid_t dset_id, dset_id2;
	TRY (dset_id = hdf5_open_dataset_by_name (m->mesh_gid, m->dsinfo_octree.name));
	hid_t mspace_id = H5S_ALL;
	hid_t dspace_id = H5S_ALL;


	int oct_size = -1;
	int maxpoints = -1;
	int size_userdata = -1;
	h5_float64_t bounding_box[6];
	TRY (h5priv_read_attrib (
		     m->mesh_gid,
		     "__curr_oct_idx__",
		     H5_INT32_T,
		     &oct_size));
	TRY (h5priv_read_attrib (
		     m->mesh_gid,
		     "__oct_maxpoints__",
		     H5_INT32_T,
		     &maxpoints));
	TRY (h5priv_read_attrib (
		     m->mesh_gid,
		     "__oct_size_userdata__",
		     H5_INT32_T,
		     &size_userdata));
	TRY (h5priv_read_attrib (
		     m->mesh_gid,
		     "__oct_bounding_box__",
		     H5_FLOAT64_T,
		     bounding_box))
	h5t_octant_t*   octants;
	h5t_oct_userdata_t*  userdata;
	TRY (H5t_read_octree (
                     &m->octree,
                     oct_size,
                     size_userdata,
                     maxpoints,
                     &octants, 
                     (void **) &userdata,
                     m->f->props->comm));
	TRY (H5t_set_bounding_box (m->octree, bounding_box));
	TRY (h5priv_start_throttle (m->f));

	TRY (hdf5_read_dataset (
             dset_id,
             m->dsinfo_octree.type_id,
             mspace_id,
             dspace_id,
             m->f->props->xfer_prop,
             octants));

	if (size_userdata > 0) {
		TRY (dset_id2 = hdf5_open_dataset_by_name (
			     m->mesh_gid, m->dsinfo_userdata.name));
		TRY (hdf5_read_dataset (
	             dset_id2,
	             m->dsinfo_userdata.type_id,
	             mspace_id,
	             dspace_id,
	             m->f->props->xfer_prop,
	             userdata));
		TRY (hdf5_close_dataset (dset_id2));
	}
	TRY (h5priv_end_throttle (m->f));


	TRY (hdf5_close_dataspace (dspace_id));
	TRY (hdf5_close_dataspace (mspace_id));
	TRY (hdf5_close_dataset (dset_id));



	H5_RETURN (H5_SUCCESS);
}
static h5_err_t
read_weights (
	h5t_mesh_t* m
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	hid_t dset_id;
	TRY (dset_id = hdf5_open_dataset_by_name (m->mesh_gid, m->dsinfo_weights.name));
	hid_t mspace_id = H5S_ALL;
	hid_t dspace_id = H5S_ALL;


	TRY (h5priv_read_attrib (
		     m->mesh_gid,
		     "__num_weights__",
		     H5_INT32_T,
		     &m->num_weights));
	TRY (m->weights =
	     h5_calloc (m->num_weights * m->num_glb_elems[m->num_leaf_levels-1], sizeof (*m->weights)));
	if (m->num_weights < 1) {
		m->weights = NULL;
	}
	TRY (h5priv_start_throttle (m->f));

	TRY (hdf5_read_dataset (
		             dset_id,
		             m->dsinfo_weights.type_id,
		             mspace_id,
		             dspace_id,
		             m->f->props->xfer_prop,
		             m->weights));
	TRY (h5priv_end_throttle (m->f));

	// check that weights are > 0
	for (h5_glb_idx_t i = 0; i < m->num_weights * m->num_glb_elems[m->num_leaf_levels-1]; i++) {
		if (m->weights[i] < 1) {
			h5_debug ("Warning: weight %d from elem %lld was %d ",
				  (int) i%m->num_weights,
				  (long long int) i/m->num_weights,
				  m->weights[i]);
			m->weights[i] = 1;

		}
	}

	TRY (hdf5_close_dataspace (dspace_id));
	TRY (hdf5_close_dataspace (mspace_id));
	TRY (hdf5_close_dataset (dset_id));
	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
read_chunks (
        h5t_mesh_t* m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	hid_t dset_id;
	TRY (dset_id = hdf5_open_dataset_by_name (m->mesh_gid, m->dsinfo_chunks.name));
	hid_t mspace_id = H5S_ALL;
	hid_t dspace_id = H5S_ALL;

	TRY (m->chunks = h5_calloc (1, sizeof (*m->chunks)));
	TRY (h5priv_read_attrib (
		     m->mesh_gid,
		     "__num_chunks__",
		     H5_INT32_T,
		     &m->chunks->num_alloc));
	m->chunks->curr_idx = m->chunks->num_alloc -1;
	TRY (h5priv_read_attrib (
		     m->mesh_gid,
		     "__num_chk_levels__",
		     H5_INT16_T,
		     &m->chunks->num_levels));

	TRY (m->chunks->num_chunks_p_level =
	     h5_calloc (m->chunks->num_levels, sizeof (*m->chunks->num_chunks_p_level)));

	TRY (h5priv_read_attrib (
		     m->mesh_gid,
		     "__num_chk_p_level__",
		     H5_INT32_T,
		     m->chunks->num_chunks_p_level));
	TRY (m->chunks->chunks =
	     h5_calloc (m->chunks->num_alloc, sizeof (*m->chunks->chunks)));

	TRY (h5priv_start_throttle (m->f));

		TRY (hdf5_read_dataset (
	             dset_id,
	             m->dsinfo_chunks.type_id,
	             mspace_id,
	             dspace_id,
	             m->f->props->xfer_prop,
	             m->chunks->chunks));
	TRY (h5priv_end_throttle (m->f));


	TRY (hdf5_close_dataspace (dspace_id));
	TRY (hdf5_close_dataspace (mspace_id));
	TRY (hdf5_close_dataset (dset_id));

	H5_RETURN (H5_SUCCESS);
}

/*
  get weights of octant (i.e. of all elements that belong to a chunk that
  belongs to the octant or its parents) for parent octants a factor is used.
  therefore the weight of an octant is divided equally onto it's children
*/

static h5_err_t
get_weights_of_octant (
	h5t_mesh_t* const m,
	h5t_octree_t* octree,
	h5_oct_idx_t oct_idx,
	double factor,
	idx_t* weights
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, octree=%p, oct_idx=%d, factor=%4.4f, weights=%p",
			m, octree, oct_idx, factor, weights);

	h5t_oct_userdata_t* userdata = NULL;

	TRY (H5t_get_userdata_r (octree, oct_idx, (void**) &userdata));
	for (int i = 0; i < MAX_CHUNKS_PER_OCTANT; i++) { // iterate through all chunks in octant
		if (userdata->idx[i] != -1) {
			h5_chk_idx_t chk_idx = userdata->idx[i];
			h5_glb_idx_t first_elem = m->chunks->chunks[chk_idx].elem;
			h5_glb_idx_t num_elems = m->chunks->chunks[chk_idx].num_elems;
			// iterate through all elements in chunk
			for (h5_glb_idx_t j = first_elem; j < first_elem + num_elems; j++) {
				for (h5_weight_t k = 0; k < m->num_weights; k++) {
					weights[k] += (h5_weight_t) (m->weights[j * m->num_weights + k] * factor);
				}
			}

		}
	} // add weights of parents proportionally
	while ((oct_idx = H5t_get_parent (octree, oct_idx)) != -1) {
		TRY (get_weights_of_octant (m, octree, oct_idx, factor/((double) NUM_OCTANTS), weights));
	}
	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
calc_weights_oct_leaflevel (
	h5t_mesh_t* const m,
	idx_t** weights,
	h5_oct_idx_t** new_numbering,
	h5_oct_idx_t* num_tot_leaf_oct
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, weights=%p, new_numbering=%p", m, weights, new_numbering);
	int size = m->f->nprocs;
	int rank = m->f->myproc;

	// get number of leaf octants

	TRY (*num_tot_leaf_oct = H5t_get_num_oct_leaflevel (m->octree));
	int counter = 0;
	TRY (*new_numbering = h5_calloc (*num_tot_leaf_oct + 1 , sizeof (h5_oct_idx_t)));

	// get range per core
	h5_oct_idx_t my_first_octant = -1;
	h5_oct_idx_t num_leaf_octants = -1;
	num_leaf_octants = *num_tot_leaf_oct / size;
	if (rank < (*num_tot_leaf_oct % size)) {
		num_leaf_octants++;
	}
	my_first_octant = (*num_tot_leaf_oct / size) * rank;
	if (rank < (*num_tot_leaf_oct % size)) {
		my_first_octant += rank;
	} else {
		my_first_octant +=  (*num_tot_leaf_oct % size);
	}
	// iterate through level and get weights
	h5t_oct_iterator_t* iter = NULL;
	TRY (H5t_init_oct_iterator (m->octree, &iter, m->leaf_level));
	for (int i = 0; i < my_first_octant; i++) {
		TRY ((*new_numbering)[counter] = H5t_iterate_oct (iter));
		counter++;
	}
	TRY (*weights = h5_calloc (num_leaf_octants * m->num_weights, sizeof (**weights)));
	idx_t* weights_p = *weights;
	h5_oct_idx_t curr_oct_idx = -1;
	for (int i = 0; i < num_leaf_octants; i++) {
		TRY (curr_oct_idx = H5t_iterate_oct (iter));
		(*new_numbering)[counter] = curr_oct_idx;
		counter++;
		TRY (get_weights_of_octant (m, m->octree, curr_oct_idx, 1, &weights_p[i * m->num_weights]));

		// make sure weights are at least 1
		for (int j = 0;j < m->num_weights; j++) {
			if (weights_p[i * m->num_weights + j] < 1) {
				weights_p[i * m->num_weights + j] = 1;
			}
		}
	}
	while (((*new_numbering)[counter] = H5t_iterate_oct (iter)) != -1) {
		counter++;
	}
	H5t_end_iterate_oct (iter);


	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
distribute_octree_parmetis (
	h5t_mesh_t* const m,
	idx_t* weights,
	h5_oct_idx_t* new_numbering,
	h5_oct_idx_t num_tot_leaf_oct
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, weights=%p, new_numbering=%p", m, weights, new_numbering);

	if (num_tot_leaf_oct < m->f->nprocs) {
		h5_debug ("Number of leaf octants %d is smaller then number of procs %d ",num_tot_leaf_oct, m->f->nprocs);
		assert (num_tot_leaf_oct > m->f->nprocs);
	}

	// compute initial distribution of cells on all procs
	int n = num_tot_leaf_oct / m->f->nprocs;
	int r = num_tot_leaf_oct % m->f->nprocs;

	idx_t* vtxdist = NULL;
	idx_t* glb_part = NULL;

	TRY (vtxdist = h5_calloc (m->f->nprocs+1, sizeof (*vtxdist)));
	vtxdist[0] = 0;
#if !defined(NDEBUG)
	if (__h5_debug_mask & (1<<5) ) {
		h5_debug ("vtxdist[%d]: %d", 0, 0);
	}
#endif
	for (int i = 1; i < m->f->nprocs+1; i++) {
		if (r > 0) {
			vtxdist[i] = vtxdist[i-1] + n + 1;
			r--;
		} else {
			vtxdist[i] = vtxdist[i-1] + n;
		}
#if !defined(NDEBUG)
		if (__h5_debug_mask & (1<<5) ) {
			h5_debug ("vtxdist[%d]: %d", i, vtxdist[i]);
		}
#endif
	}
	TRY (glb_part = h5_calloc (vtxdist[m->f->nprocs], sizeof(*glb_part)));
	if (!dont_use_parmetis) {
		//	// read cells only
		idx_t start = vtxdist[m->f->myproc];
		idx_t num_interior_oct = vtxdist[m->f->myproc+1] - start;


		// setup input for ParMETIS
		idx_t* xadj;
		idx_t* adjncy;
		idx_t* part;
		TRY (xadj = h5_calloc (num_interior_oct + 1, sizeof(*xadj)));
		TRY (part = h5_calloc (num_interior_oct, sizeof(*part)));
		int num_alloc_adj = 4*6*num_interior_oct;
		TRY (adjncy = h5_calloc (num_alloc_adj, sizeof(*adjncy)));

		h5_oct_idx_t* neighbors = NULL;
		h5_oct_idx_t num_neigh;
		h5_oct_idx_t* ancestor_of_neigh = NULL;
		h5_oct_idx_t num_anc_of_neigh;

		//	h5priv_plot_octants (m->octree);
		idx_t i;
		idx_t counter = 0;
		for (i = 0; i < num_interior_oct; i++) {
			TRY (
				H5t_get_neighbors (
					m->octree,
					new_numbering[start + i],
					&neighbors,
					&num_neigh,
					&ancestor_of_neigh,
					&num_anc_of_neigh,
					1,
					m->leaf_level));
			if (counter + num_neigh >= num_alloc_adj) {
				 // WARNING may alloc too much mem (minimal would be counter + num_neigh)
				adjncy = realloc (adjncy, (num_alloc_adj + counter + num_neigh) * sizeof(*adjncy));
			}
			xadj[i+1] = xadj[i] + num_neigh;
#if !defined(NDEBUG)
			if (__h5_debug_mask & (1<<5) ) {
				h5_debug ("xadj[%d]: %d", i+1, xadj[i+1]);
			}
#endif
			for (int k = 0; k < num_neigh; k++) {
				int found = 0;
				for (int j = 0; j < num_tot_leaf_oct; j++) {
					if (new_numbering[j] == neighbors[k]) {
						adjncy[counter] = j;
#if !defined(NDEBUG)
						if (__h5_debug_mask & (1<<5) ) {
							h5_debug ("adjncy[%d]: %d", counter, adjncy[counter]);
						}
#endif
						counter++;
						found = 1;
					}
				}
				if (found == 0) {
					H5_LEAVE (H5_ERR_INTERNAL);
				}
			}
		}


		// now we can call the partitioner
		idx_t wgtflag = 0;
		idx_t numflag = 0;
		idx_t ncon = 1;
		idx_t* vwgt = NULL;
		if (m->num_weights > 0) {
			wgtflag = 2;
			ncon = m->num_weights;
			vwgt = weights;
		}

		idx_t nparts = m->f->nprocs;
		real_t* tpwgts;
		real_t* ubvec;
		idx_t options[] = {1,0,42};
		idx_t edgecut;
		h5_debug ("nparts: %d", nparts);
		TRY (tpwgts = h5_calloc (nparts * ncon, sizeof(*tpwgts)));
		TRY (ubvec = h5_calloc (nparts * ncon, sizeof(*ubvec)));
		for (i = 0; i < nparts * ncon; i++) {
			tpwgts[i] = 1.0 / (real_t)nparts;
			ubvec[i] = 1.05;
		}


		int rc = ParMETIS_V3_PartKway (
				vtxdist,
				xadj,
				adjncy,
				vwgt,           // vwgt vertex weights
				NULL,           // adjwgt
				&wgtflag,
				&numflag,
				&ncon,			// number of balance constraints
				&nparts,
				tpwgts,
				ubvec,
				options,
				&edgecut,
				part,
				&m->f->props->comm
		);
		if (rc != METIS_OK) {
			H5_RETURN_ERROR (
				H5_ERR,
				"ParMETIS failed");
		}

		TRY (h5_free (xadj));
		TRY (h5_free (adjncy));
		TRY (h5_free (tpwgts));
		TRY (h5_free (ubvec));

#if !defined(NDEBUG)
		if (__h5_debug_mask & (1<<5) ) {
			for (i = 0; i < num_interior_oct; i++) {
				h5_debug ("part[%d]: %llu", i, (unsigned long long)part[i]);
			}
		}
#endif
		// instead of updating the whole octree, we just update the new procs locally
		// therefore we need the glb_part array
		int* recvcounts = NULL;
		TRY (recvcounts = h5_calloc (m->f->nprocs, sizeof (*recvcounts)));
		for (int i = 0; i < m->f->nprocs; i++) {
			recvcounts[i] = vtxdist[i+1]-vtxdist[i];
		}

		TRY (h5priv_mpi_allgatherv (part,
				num_interior_oct,
				MPI_INT,
				glb_part,
				recvcounts,
				vtxdist,
				MPI_INT,
				m->f->props->comm));
		TRY (h5_free (part));
	}
	if (dont_use_parmetis == 1 ) { // not use parmetis and just distribute octants with morton ordering
		int curr_proc = 0;
		for (int i = 0; i < vtxdist[m->f->nprocs]; i++) {
			while(i >= vtxdist[curr_proc +1]) {
				curr_proc++;
			}
			glb_part[i] = curr_proc;
		}

	}
	if (dont_use_parmetis == 2) { // distribute geometrically in slices acc to preferred direction.
		assert (preferred_direction > -1 && preferred_direction <3);
		// calculate slices
		h5_float64_t* bb = H5t_get_bounding_box(m->octree);
		h5_float64_t glb_min = bb[preferred_direction];
		h5_float64_t glb_max = bb[preferred_direction + 3];
		h5_float64_t slice_length = (glb_max - glb_min) / (h5_float64_t)m->f->nprocs;
		h5_float64_t newbb[6];
		// iterate through leave level octants and decide to whom they belong
		// WARING this could be more efficient if every proc would take care of
		// num_leaf_oct/nproc octants and exchange results
		for (int i = 0; i < vtxdist[m->f->nprocs]; i++) {
			h5_oct_idx_t oct_idx = new_numbering[i];

			TRY (H5t_get_bounding_box_of_octant (m->octree, oct_idx, newbb));
			h5_float64_t loc_min = newbb[preferred_direction];
			h5_float64_t loc_max = newbb[preferred_direction + 3];
			h5_float64_t loc_mid = (loc_min + loc_max) / 2.0;
			glb_part[i] = (int) ((loc_mid - glb_min)/slice_length);
		}
	}
	assert (dont_use_parmetis > -1 && dont_use_parmetis <3);

	for (int i = 0; i < vtxdist[m->f->nprocs]; i++) {
		h5_oct_idx_t oct_idx = new_numbering[i];
		TRY (H5t_set_proc_int (m->octree, oct_idx, glb_part[i]));
		h5_oct_idx_t parent = oct_idx;
		while ((parent = H5t_get_parent (m->octree, parent)) != -1) {
			if (H5t_get_children(m->octree, parent) == oct_idx) {
				// oct_idx is the first children - so set
				// the proc of the parent to the same as
				// the oct_idx
				TRY (H5t_set_proc_int (m->octree, parent, glb_part[i]));
				oct_idx = parent;
			} else {
				// nothing further to do
				break;
			}
		}

	}
	//should not be necessary anymore but doesn't matter since it just checks if update is necessary
	TRY (H5t_update_internal (m->octree));
	TRY (h5_free (vtxdist));

	TRY (h5_free (glb_part));
	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
add_oct_children_to_list (
	h5t_mesh_t* const m,
	h5_loc_idxlist_t** oct_list,
	h5_oct_idx_t oct_idx
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p list=%p, oct_idx=%d", m, oct_list, oct_idx);
	assert (oct_idx > 0); // otherwise we add the whole octree!
	h5_oct_idx_t children = H5t_get_children (m->octree, oct_idx);
	if (children == -1 ) {
		H5_LEAVE (H5_SUCCESS);
	}
	// get siblings
	h5_oct_idx_t sibling_idx = H5t_get_sibling (m->octree, children);
	for (int i = 0; i < NUM_OCTANTS; i++) {
		h5priv_search_in_loc_idxlist(oct_list,(h5_loc_idx_t) sibling_idx + i);
		TRY (add_oct_children_to_list (m, oct_list,(h5_loc_idx_t) sibling_idx + i));
		// TODO it may could be faster to do this
		// is a second loop because adding elems just after each other could be beneficial -> maybe extend
		// insert to multiple insert...
	}


	H5_RETURN (H5_SUCCESS);
}

/*
  Idea: we need to get all octants that belong to this proc including all their parent and children.
  (octants that dont have a userlevel should not be added)
  This gives us a a domain per proc (which will probably already overlap somewhat).
  Concerning the neighbors: we need to get all the neighbors (of the domain) on level 0 and all their children.
 */
h5_err_t
h5tpriv_get_list_of_chunks_to_read (
	h5t_mesh_t* const m,
	h5_chk_idx_t** list,
	int* counter
	) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p list=%p, counter=%p", m, list, counter);
	int rank = m->f->myproc;
	int size_list = m->chunks->curr_idx + 1;
	TRY ( *list = h5_calloc (size_list + 1, sizeof (**list))); // +1 to be on save side for accesses
	h5_loc_idxlist_t* loc_list = NULL;
	TRY (h5priv_alloc_loc_idxlist (&loc_list, size_list+1));
	*counter = 0;

	//WARNING works only if oct_idx = loc_idx
	h5_loc_idxlist_t* oct_list = NULL;
	TRY (h5priv_alloc_loc_idxlist (&oct_list, H5t_get_num_octants (m->octree)* 2 /m->f->nprocs));
	// go through octree level, get own chunks and parent chunks
	h5t_oct_iterator_t* iter = NULL;
	for (int i = 0; i < m->num_leaf_levels; i++) {
		TRY (H5t_init_oct_iterator (m->octree, &iter, i));

		// get all octants that belong to this proc
		h5_oct_idx_t oct_idx = -1;
		while ((oct_idx = H5t_iterate_oct (iter)) != -1) {
			if (H5t_get_proc (m->octree, oct_idx) == rank) {
				h5priv_search_in_loc_idxlist(&oct_list, (h5_loc_idx_t)oct_idx);
				h5_oct_idx_t parent = oct_idx;
				// add parent chunks
				while ((parent = H5t_get_parent (m->octree, parent)) != -1) {
					h5_oct_idx_t parent_idx = -1;
					if (H5t_get_userlevel(m->octree, parent) == 0) {
						// check if there are any parents that still have a level.
						parent_idx = parent;
						while ((parent_idx = H5t_get_parent (m->octree, parent_idx)) != -1) {
							if (H5t_get_userlevel(m->octree, parent) != 0) {
								// parent_idx still has a level ->
								//so need parent needs to be added
								break;
							}
						}
						if (parent_idx == -1) {
							// parent_idx is -1 so all parents don't
							// have a level anymore -> don't add
							break;
						}
					}
					if (parent_idx != -1) {
						h5priv_search_in_loc_idxlist(&oct_list, (h5_loc_idx_t)parent);
					}
				}
				// add children
				TRY (add_oct_children_to_list (m, &oct_list, (h5_loc_idx_t)oct_idx));
			}

		}
	}
	TRY (H5t_end_iterate_oct (iter));
	// get all neighbors
	//WARNING works only if oct_idx = loc_idx
	h5_loc_idxlist_t* neigh_oct_list = NULL;
	TRY (h5priv_alloc_loc_idxlist (&neigh_oct_list, H5t_get_num_octants (m->octree)* 2 /m->f->nprocs));
	h5_oct_idx_t* neigh = NULL;
	h5_oct_idx_t* anc = NULL;
	for (int i = 0; i < oct_list->num_items; i++) {

		h5_oct_idx_t  num_neigh = 0;
		h5_oct_idx_t  num_anc = 0;

		if ((H5t_get_userlevel (m->octree,(h5_oct_idx_t) oct_list->items[i]) & 1) == 1) {
			// get neighbors
			TRY (H5t_get_neighbors (
					m->octree,
					(h5_oct_idx_t) oct_list->items[i],
					&neigh,
					&num_neigh,
					&anc,
					&num_anc,
					3,
					0));
		}
		// TODO avoid any that have no level themselfs and above!
		for (int i = 0; i < num_neigh; i++) {
			h5priv_search_in_loc_idxlist(&neigh_oct_list, (h5_loc_idx_t)neigh[i]);
			// add children
			TRY (add_oct_children_to_list (m, &neigh_oct_list, (h5_loc_idx_t)neigh[i]));
		}
		// add octants to chunk_list
		add_chunk_to_list (m, &loc_list,(h5_oct_idx_t) oct_list->items[i]);
	}
	// add neighbors to chunk_list
	for (int i = 0; i < neigh_oct_list->num_items; i++) {
		add_chunk_to_list (m, &loc_list,(h5_oct_idx_t) neigh_oct_list->items[i]);
	}

	for (int i = 0; i < loc_list->num_items; i++) {
		(*list)[i] = (h5_chk_idx_t) loc_list->items[i];
	}
	*counter = loc_list->num_items;
	h5priv_free_loc_idxlist(&loc_list);

	TRY (h5_free (neigh));
	TRY (h5_free (anc));
	TRY (h5priv_free_loc_idxlist (&oct_list));
	TRY (h5priv_free_loc_idxlist (&neigh_oct_list));
	if (size_list < *counter) {
		h5_debug ("Overflow of list_of_chunks");
		H5_LEAVE (H5_ERR_INTERNAL);
	}

	H5_RETURN (H5_SUCCESS);
}

/*
 * returns a list of processors for assigning proc to each element
 */
static h5_err_t
get_list_of_proc (
	h5t_mesh_t* const m,
	h5_int32_t* my_procs,
	h5_chk_idx_t* list,
	int num_chunks
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, my_procs=%p, list=%p", m, my_procs, list);
	int counter = 0;
	for (int i = 0; i < num_chunks; i++) {
		h5_glb_idx_t num = m->chunks->chunks[list[i]].num_elems;
		int proc = H5t_get_proc(m->octree, m->chunks->chunks[list[i]].oct_idx);
		for (int j = 0; j < num; j++) {
			my_procs[counter] = proc;
			counter++;
		}
	}


	H5_RETURN (H5_SUCCESS);
}

static h5_lvl_idx_t
get_level_of_chk_idx (
	h5t_mesh_t* const m,
	h5_chk_idx_t chk_idx
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, chk_idx=%d", m, chk_idx);
	assert (chk_idx > -1);
	int nbr_chunks = m->chunks->num_chunks_p_level[0];
	for (int i = 0; i < m->chunks->num_levels; i++) {
		if (chk_idx < nbr_chunks) {
			H5_LEAVE (i);
		} else {
			if (i+1 < m->chunks->num_levels) {
				nbr_chunks += m->chunks->num_chunks_p_level[i + 1];
			}
		}
	}
	H5_RETURN (-2);
}

static h5_err_t
read_chunked_elements (
	h5t_mesh_t* const m,
	h5_glb_elem_t** glb_elems,
	h5_int32_t** my_procs
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	// find chunks to read
	h5_chk_idx_t* list_of_chunks = NULL;
	int num_interior_chunks = 0;

	TRY (h5tpriv_get_list_of_chunks_to_read (m, &list_of_chunks, &num_interior_chunks));


	for (int i = 0; i < num_interior_chunks; i++) {
		// set number of vertices and elements
		h5_lvl_idx_t level = get_level_of_chk_idx (m, list_of_chunks[i]);
		assert (level > -1);
		m->num_interior_elems[level] += m->chunks->chunks[list_of_chunks[i]].num_elems;
	}
	m->num_interior_leaf_elems[0] = m->num_interior_elems[0];
	for (int i = 1; i < m->num_leaf_levels; i++) {
		m->num_interior_leaf_elems[i] = m->num_interior_leaf_elems[i-1] +
				m->num_interior_elems[i] -
				m->num_interior_elems[i] / h5tpriv_get_num_new_elems (m);
		m->num_interior_elems[i] += m->num_interior_elems[i-1];
	}
	int num_interior_elems = m->num_interior_elems[m->num_leaf_levels -1];


	TRY (*glb_elems = h5tpriv_alloc_glb_elems (m, num_interior_elems));

	TRY (*my_procs = h5_calloc (num_interior_elems, sizeof (**my_procs)));

	// get list of proc to assign to each element
	TRY (get_list_of_proc (m, *my_procs, list_of_chunks, num_interior_chunks));

	hid_t dset_id;
	TRY (dset_id = hdf5_open_dataset_by_name (m->mesh_gid, m->dsinfo_elems.name));
	hid_t mspace_id;

	// create memspace
	hsize_t hcount = (hsize_t) num_interior_elems;
	TRY (mspace_id = hdf5_create_dataspace (1, &hcount, NULL));

	//create dspace
	hid_t dspace_id;
	H5S_seloper_t seloper = H5S_SELECT_SET; // first selection
	hsize_t hstride = 1;
	hsize_t num_glb_elems = m->num_glb_elems[m->num_leaf_levels-1];
	TRY (dspace_id = hdf5_create_dataspace (1, &num_glb_elems, NULL));
	hsize_t hstart = -1;
	m->dsinfo_elems.dims[0] = num_interior_elems;

	for (hsize_t i = 0; i < num_interior_chunks; i++) {
		hstart = (hsize_t)m->chunks->chunks[list_of_chunks[i]].elem;
		hcount = (hsize_t)m->chunks->chunks[list_of_chunks[i]].num_elems;
		while (i + 1 < num_interior_chunks &&
				hstart + hcount  == m->chunks->chunks[list_of_chunks[i+1]].elem) {
			// WARNING make sure list has one free element in the back otherwise seg fault
			hcount += (hsize_t)m->chunks->chunks[list_of_chunks[i+1]].num_elems;
			i++;
		}
		TRY (hdf5_select_hyperslab_of_dataspace (
		             dspace_id,
		             seloper,
		             &hstart, &hstride, &hcount,
		             NULL));
		seloper = H5S_SELECT_OR;
	}

	TRY (h5priv_start_throttle (m->f));
	TRY (hdf5_read_dataset (
	             dset_id,
	             m->dsinfo_elems.type_id,
	             mspace_id,
	             dspace_id,
	             m->f->props->xfer_prop,
	             *glb_elems));
	TRY (h5priv_end_throttle (m->f));
	TRY (hdf5_close_dataspace (dspace_id));
	TRY (hdf5_close_dataspace (mspace_id));
	TRY (hdf5_close_dataset (dset_id));


#if NDEBUG == 0
	if (__h5_debug_mask & (1<<6) ) {
		sleep (m->f->myproc*2);
		for (int i = 0; i < num_interior_elems;i++) {
			h5_debug ("\n"
				"[proc %d] D:     ELEM\n"
				"[proc %d] D:     idx:          %d \n"
				"[proc %d] D:     parent_idx:   %d \n"
				"[proc %d] D:     child_idx:    %d \n"
				"[proc %d] D:     level_idx:    %d \n"
				"[proc %d] D:    refinement:    %d \n"
				"[proc %d] D:         flags:    %d \n"
				"[proc %d] D:       indices:    %d %d %d\n"
				"[proc %d] D:    neigh_indi:    %d %d %d\n\n",m->f->myproc,
				m->f->myproc, (int)((h5_glb_tri_t*)(*glb_elems))[i].idx,
				m->f->myproc, (int)((h5_glb_tri_t*)(*glb_elems))[i].parent_idx,
				m->f->myproc, (int)((h5_glb_tri_t*)(*glb_elems))[i].child_idx,
				m->f->myproc, (int)((h5_glb_tri_t*)(*glb_elems))[i].level_idx,
				m->f->myproc, (int)((h5_glb_tri_t*)(*glb_elems))[i].refinement,
				m->f->myproc, (int)((h5_glb_tri_t*)(*glb_elems))[i].flags,
				m->f->myproc, (int)((h5_glb_tri_t*)(*glb_elems))[i].vertex_indices[0],
				(int)((h5_glb_tri_t*)(*glb_elems))[i].vertex_indices[1],
				(int)((h5_glb_tri_t*)(*glb_elems))[i].vertex_indices[2],
				m->f->myproc, (int)((h5_glb_tri_t*)(*glb_elems))[i].neighbor_indices[0],
				(int)((h5_glb_tri_t*)(*glb_elems))[i].neighbor_indices[1],
				(int)((h5_glb_tri_t*)(*glb_elems))[i].neighbor_indices[2]);
		}
	}

#endif


	H5_RETURN (H5_SUCCESS);
}

#endif
// The chunked version
h5_err_t
h5tpriv_read_chunked_mesh (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p", m);
#ifdef WITH_PARALLEL_H5GRID

	TRY (read_octree (m));
	TRY (h5priv_mpi_barrier (m->f->props->comm));
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
	TRY (read_chunks (m));
	TRY (h5priv_mpi_barrier (m->f->props->comm));
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
	if (m->num_weights > 0) {
		TRY (read_weights (m));
	} else {
		m->weights = NULL;
	}
	TRY (h5priv_mpi_barrier (m->f->props->comm));
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
	h5_oct_idx_t* new_numbering = NULL;
	idx_t* weights = NULL;
	h5_oct_idx_t num_tot_leaf_oct = -1;
	TRY (calc_weights_oct_leaflevel (m, &weights, &new_numbering, &num_tot_leaf_oct));

	TRY (distribute_octree_parmetis (m, weights, new_numbering, num_tot_leaf_oct));
	TRY (h5_free (weights));

	TRY (h5priv_mpi_barrier (m->f->props->comm));
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();

	h5_glb_elem_t* glb_elems = NULL;
	h5_int32_t* my_procs = NULL;
	TRY (read_chunked_elements (m, &glb_elems, &my_procs));

	TRY (h5priv_mpi_barrier (m->f->props->comm));
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();

	h5_loc_idx_t num_interior_elems = m->num_interior_elems[m->leaf_level];
	// add interior elements to global -> local index map
	TRY (h5tpriv_init_map_elem_g2l (m, glb_elems, num_interior_elems));

	TRY (h5tpriv_alloc_loc_elems (m, 0, num_interior_elems));
	m->num_loaded_levels = m->num_leaf_levels;

	// define local indices for all vertices of all local elements
	size_t size = m->num_interior_elems[m->leaf_level] *4  + 128; // TODO do we know smth about how many vtx?
	TRY (h5priv_new_idxmap (&m->map_vertex_g2l, size));

	h5_idxmap_t* map = &m->map_vertex_g2l;

	/* for the time being we use the hash table only for a fast test
		   whether a global index has already been added to the map or not. */
	h5_hashtable_t htab;
	TRY (h5priv_hcreate ((size << 2) / 3, &htab,
			hidxmap_cmp, hidxmap_compute_hval, NULL));

	int num_vertices = h5tpriv_ref_elem_get_num_vertices (m);
	for (size_t idx = 0; idx < num_interior_elems; idx++) {
		h5_glb_idx_t* vertices = h5tpriv_get_glb_elem_vertices (m, glb_elems, idx);

		for (int i = 0; i < num_vertices; i++) {
			// add index temporarly to map ...
			map->items[map->num_items] = (h5_idxmap_el_t) {vertices[i], 0};
			// ... and check whether it has already been added
			h5_idxmap_el_t* retval = NULL;
			h5priv_hsearch (&map->items[map->num_items],
					H5_ENTER, (void**)&retval, &htab);
			if (retval == &map->items[map->num_items]) {
				// new entry in hash table thus in map
				map->num_items++;
				h5_lvl_idx_t level = h5tpriv_get_glb_elem_level (m, glb_elems, idx);
				m->num_loc_vertices[level]++;
			}
		}
	}
	TRY (h5priv_hdestroy (&htab));

	for (int i = 1; i < m->num_leaf_levels; i++) {
		m->num_loc_vertices[i] += m->num_loc_vertices[i-1];
	}

	h5priv_sort_idxmap (map);
	for (h5_loc_idx_t i = 0; i < map->num_items; i++) {
		map->items[i].loc_idx = i;
	}
	TRY (read_vertices (m, map));

	TRY (h5priv_mpi_barrier (m->f->props->comm));
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();

	m->num_loaded_levels = m->num_leaf_levels;
	// calculate which elem belongs to which proc
	TRY (h5tpriv_init_loc_elems_struct (m, glb_elems, 0, num_interior_elems, 0, my_procs));
	TRY (h5tpriv_init_elem_flags (m, 0, num_interior_elems));

	TRY (h5tpriv_update_internal_structs (m, 0)); //TODO check if that should be 0 or m->leaf_level
	TRY (h5_free (glb_elems));

	TRY (h5priv_mpi_barrier (m->f->props->comm));
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
#endif	
	H5_RETURN (H5_SUCCESS);
}



static int
cmp_glb_idx (
        const void* x,
        const void* y
        ) {
	if (*(h5_glb_idx_t*)x < *(h5_glb_idx_t*)y) return -1;
	if (*(h5_glb_idx_t*)x > *(h5_glb_idx_t*)y) return 1;

	return 0;
}


static h5_err_t
read_elems_part (
        h5t_mesh_t* const m,
        h5_glb_elem_t** glb_elems,
        h5_glb_idx_t* elem_indices,     // in
        h5_glb_idx_t dim                // in
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);

	m->num_interior_elems[0] = dim;
	m->num_interior_leaf_elems[0] = dim;

	h5_glb_elem_t* elems;
	TRY (elems = h5tpriv_alloc_glb_elems (m, dim));

	qsort (elem_indices, dim, sizeof(*elem_indices), cmp_glb_idx);

	// create memspace
	hsize_t num_elems = dim;
	hid_t mspace_id;
	TRY (mspace_id = hdf5_create_dataspace(1, &num_elems, NULL));

	// create diskspace and select subset
	hsize_t num_glb_elems = m->num_glb_elems[m->num_leaf_levels-1];
	hid_t dspace_id;
	TRY (dspace_id = hdf5_create_dataspace (1, &num_glb_elems, NULL));
	hsize_t hstride = 1;
	H5S_seloper_t seloper = H5S_SELECT_SET; // first selection
	for (hsize_t i = 0; i < dim; i++) {
		hsize_t hstart = elem_indices[i];
		hsize_t hcount = 1;
		while (elem_indices[i]+1 == elem_indices[i+1] && i < dim) {
			i++; hcount++;
		}
		if (hstart+hcount > num_glb_elems) {
			H5_RETURN_ERROR (
				H5_ERR_H5FED,
				"invalid selection: start=%lld, count=%lld",
				(long long)hstart, (long long)hcount);
		}
		TRY (hdf5_select_hyperslab_of_dataspace (
		             dspace_id,
		             seloper,
		             &hstart, &hstride, &hcount,
		             NULL));
		seloper = H5S_SELECT_OR;
	}
	hid_t dset_id;
	TRY (dset_id = hdf5_open_dataset_by_name (m->mesh_gid, m->dsinfo_elems.name));
	TRY (hdf5_read_dataset (
	             dset_id,
	             m->dsinfo_elems.type_id,
	             mspace_id,
	             dspace_id,
	             m->f->props->xfer_prop,
	             elems));
	TRY (hdf5_close_dataspace (dspace_id));
	TRY (hdf5_close_dataspace (mspace_id));
	TRY (hdf5_close_dataset (dset_id));

	*glb_elems = elems;
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5tpriv_read_mesh_part (
        h5t_mesh_t* const m,
        h5_glb_idx_t* elem_indices,
        h5_glb_idx_t num_elems
        ) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p", m);
#ifdef WITH_PARALLEL_H5GRID
	TRY (h5priv_mpi_barrier (m->f->props->comm)); // octree
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
	TRY (h5priv_mpi_barrier (m->f->props->comm)); // chunks
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
	TRY (h5priv_mpi_barrier (m->f->props->comm)); // weights
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
	TRY (h5priv_mpi_barrier (m->f->props->comm)); // distribute chunks
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
#endif
	h5_glb_elem_t* glb_elems = NULL;
	TRY (read_elems_part (m, &glb_elems, elem_indices, num_elems));
	h5_loc_idx_t num_interior_elems = m->num_interior_elems[0];
	h5_loc_idx_t num_ghost_elems = m->num_ghost_elems[0] = 0;
#ifdef WITH_PARALLEL_H5GRID
	TRY (h5priv_mpi_barrier (m->f->props->comm)); // read elems
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
#endif
	// add interior elements to global -> local index map
	TRY (h5tpriv_init_map_elem_g2l (m, glb_elems, num_interior_elems));

	// define local indices for all vertices of all local elements
	size_t size = 3*(num_interior_elems + num_ghost_elems) + 128;  // added times 3 for random read of elems
	TRY (h5priv_new_idxmap (&m->map_vertex_g2l, size));

	h5_idxmap_t* map = &m->map_vertex_g2l;

	/* for the time being we use the hash table only for a fast test
	   whether a global index has already been added to the map or not. */
	h5_hashtable_t htab;
	TRY (h5priv_hcreate ((size << 2) / 3, &htab,
	                     hidxmap_cmp, hidxmap_compute_hval, NULL));

	int num_vertices = h5tpriv_ref_elem_get_num_vertices (m);
	for (size_t idx = 0; idx < num_interior_elems; idx++) {
		h5_glb_idx_t* vertices = h5tpriv_get_glb_elem_vertices (m, glb_elems, idx);
		for (int i = 0; i < num_vertices; i++) {
			// add index temporarly to map ...
			map->items[map->num_items] = (h5_idxmap_el_t) {vertices[i], 0};
			// ... and check whether it has already been added
			h5_idxmap_el_t* retval;
			h5priv_hsearch (&map->items[map->num_items],
			                H5_ENTER, (void**)&retval, &htab);
			if (retval == &map->items[map->num_items]) {
				// new entry in hash table thus in map
				map->num_items++;
			}
		}
	}
	TRY (h5priv_hdestroy (&htab));
	h5priv_sort_idxmap (map);
	for (h5_loc_idx_t i = 0; i < map->num_items; i++) {
		map->items[i].loc_idx = i;
	}
	TRY (read_vertices (m, map));
	TRY (h5tpriv_alloc_loc_elems (m, 0, num_interior_elems+num_ghost_elems));
	m->num_loaded_levels = 1;
#ifdef WITH_PARALLEL_H5GRID
	TRY (h5priv_mpi_barrier (m->f->props->comm)); // read vtx
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
#endif
	TRY (h5tpriv_init_loc_elems_struct (m, glb_elems, 0, num_interior_elems, 0, NULL));
	TRY (h5tpriv_init_elem_flags (m, 0, num_interior_elems+num_ghost_elems));
	TRY (h5tpriv_update_internal_structs (m, 0));

	TRY (h5_free (glb_elems));
#ifdef WITH_PARALLEL_H5GRID
	TRY (h5priv_mpi_barrier (m->f->props->comm)); // init update
	m->timing.measure[m->timing.next_time++] = MPI_Wtime();
#endif
	H5_RETURN (H5_SUCCESS);
}

