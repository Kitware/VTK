/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5T_IO_H
#define __PRIVATE_H5T_IO_H

#include "h5core/h5_types.h"
#include "private/h5t_types.h"

struct h5t_read_methods {
	h5_err_t (*init_loc_elems_struct)(h5t_mesh_t* const,
	                                  const h5_glb_elem_t*cosnt,
	                                  const h5_loc_idx_t,
	                                  const h5_loc_idx_t,
	                                  const h5_uint32_t,
	                                  const h5_int32_t* my_proc);
	h5_err_t (*init_elem_flags)(h5t_mesh_t* const, h5_loc_idx_t, h5_loc_idx_t);
	h5_err_t (*init_map_elem_g2l)(h5t_mesh_t* const, h5_glb_elem_t*
	                              const, const h5_loc_idx_t);
	h5_err_t (*init_glb_elems_struct)(h5t_mesh_t* const, const h5_glb_elem_t*const);
	h5_err_t (*init_glb_elems_struct_chk)(h5t_mesh_t* const, const h5_glb_elem_t*const, h5_chk_idx_t* ,int);
};

extern struct h5t_read_methods h5tpriv_read_trim_methods;
extern struct h5t_read_methods h5tpriv_read_tetm_methods;

h5_err_t
h5tpriv_read_mesh (
        h5t_mesh_t* const m
        );
h5_err_t
h5tpriv_read_chunked_mesh (
        h5t_mesh_t* const m
        );
h5_err_t
h5tpriv_read_mesh_part (
        h5t_mesh_t* const m,
        h5_glb_idx_t* elem_indices,
        h5_glb_idx_t dim
        );

h5_err_t
h5tpriv_write_mesh (
        h5t_mesh_t* const m
        );

static inline h5_err_t
h5tpriv_init_loc_elems_struct (
        h5t_mesh_t* const m,
        const h5_glb_elem_t* const elems,
        const h5_loc_idx_t from_idx,
        const h5_loc_idx_t count,
        const h5_uint32_t flags,
        const h5_int32_t* my_proc
        ) {
	return m->methods->read->init_loc_elems_struct (m, elems, from_idx, count, flags, my_proc);
}

static inline h5_err_t
h5tpriv_init_elem_flags (
        h5t_mesh_t* const m,
        const h5_loc_idx_t from,
        const h5_loc_idx_t count
        ) {
	return m->methods->read->init_elem_flags (m, from, count);
}

static inline h5_err_t
h5tpriv_init_map_elem_g2l (
        h5t_mesh_t* const m,
        h5_glb_elem_t* elems,
        const h5_loc_idx_t count
        ) {
	return m->methods->read->init_map_elem_g2l (m, elems, count);
}

static inline h5_err_t
h5tpriv_init_glb_elems_struct (
        h5t_mesh_t* const m,
        const h5_glb_elem_t* const glb_elems
        ) {
	return m->methods->read->init_glb_elems_struct (m, glb_elems);
}

static inline h5_err_t
h5tpriv_init_glb_elems_struct_chk (
        h5t_mesh_t* const m,
        const h5_glb_elem_t* const glb_elems,
        h5_chk_idx_t* list,
        int num_items
        ) {
	return m->methods->read->init_glb_elems_struct_chk (m, glb_elems, list, num_items);
}


h5_err_t
h5tpriv_get_list_of_chunks_to_write (
		h5t_mesh_t* const m,
		h5_chk_idx_t** list,
		int* counter
		);

h5_err_t
h5tpriv_get_list_of_chunks_to_read (
		h5t_mesh_t* const m,
		h5_chk_idx_t** list,
		int* counter
		);

h5_int32_t
h5priv_find_proc_to_write (
		h5t_mesh_t* const m,
		h5_loc_idx_t elem_idx
		);

// TODO move this to appropriate place
typedef struct {
	int (*compare) (const void *p_a, const void *p_b);
} comparison_fn_t;

void * linsearch (const void *key, void *array, size_t count, size_t size,
 comparison_fn_t compare);

unsigned int
hidxmap_compute_hval (
        const void* __item
        );

int
hidxmap_cmp (
        const void* __a,
        const void* __b
        );

#endif
