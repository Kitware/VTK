/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include "private/h5t_types.h"
#include "h5core/h5t_map.h"

#include "private/h5_va_macros.h"

#include "private/h5_attribs.h"
#include "private/h5_hdf5.h"

#include "private/h5_model.h"
#include "private/h5t_access.h"
#include "private/h5t_map.h"
#include "private/h5t_model.h"
#include "private/h5t_tags.h"

#include <assert.h>

static h5_err_t
read_dataset (
        h5t_mesh_t* const m,
        const h5_file_p f,
        hid_t dset_id,
        h5_dsinfo_t* dsinfo,
        hid_t (*set_mspace)(h5t_mesh_t*,hid_t),
        hid_t (*set_dspace)(h5t_mesh_t*,hid_t),
        void* const data
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                   "f=%p, dset_id=%lld (%s), dsinfo=%p, set_mspace=%p, "
	                   "set_dspace=%p, data=%p",
	                   f, (long long int)dset_id, hdf5_get_objname(dset_id),
	                   dsinfo,
	                   set_mspace, set_dspace, data);

	hid_t mspace_id;
	hid_t dspace_id;

	TRY (mspace_id = (*set_mspace)(m, dset_id));
	TRY (dspace_id = (*set_dspace)(m, dset_id));
	TRY (h5priv_start_throttle (f));
	TRY (hdf5_read_dataset (
	             dset_id,
	             dsinfo->type_id,
	             mspace_id,
	             dspace_id,
	             f->props->xfer_prop,
	             data));
	TRY (h5priv_end_throttle (f));
	TRY (hdf5_close_dataspace (dspace_id));
	TRY (hdf5_close_dataspace (mspace_id));

	H5_RETURN (H5_SUCCESS);
}

static hid_t
open_space_all (
        h5t_mesh_t* const m,
        const hid_t dataset_id
        ) {
	UNUSED_ARGUMENT (m);
	UNUSED_ARGUMENT (dataset_id);
	return H5S_ALL;
}

/*!
   Get number of tagsets
 */
h5_ssize_t
h5t_get_num_mtagsets (
        h5t_mesh_t* const m
        ) {
	H5_CORE_API_ENTER (h5_ssize_t, "m=%p", m);
	h5_ssize_t num_mtagsets = 0;
	h5_err_t exists = 0;
	TRY (exists = h5priv_link_exists (m->mesh_gid, "Tags"));
	if (!exists) H5_LEAVE (0);

	hid_t loc_id;
	TRY (loc_id = h5priv_open_group (m->mesh_gid, "Tags"));
	TRY (num_mtagsets = hdf5_get_num_groups (loc_id));
	TRY (hdf5_close_group (loc_id));

	H5_RETURN (num_mtagsets);

}

static h5_err_t
get_tagset_info (
        const hid_t loc_id,
        const h5_size_t idx,
        char name[],
        const h5_size_t len_name,
        h5_int64_t* const type
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "loc_id=%lld, idx=%llu, name=%p, len_name=%llu, type=%p",
	                    (long long int)loc_id, (long long unsigned)idx, name,
	                    (long long unsigned)len_name, type);

	hid_t tags_id, tag_id, dset_id;
	// open ctn with all tags
	TRY (tags_id = hdf5_open_group (loc_id, "Tags"));
	// get name of tag given by idx
	TRY (hdf5_get_name_of_group_by_idx (tags_id, idx, name, len_name));
	// open this tag
	TRY (tag_id = hdf5_open_group (tags_id, name));
	// determine type of dataset with values
	TRY (dset_id = hdf5_open_dataset_by_name (tag_id, "values"));
	TRY (*type = h5priv_get_normalized_dataset_type (dset_id));

	TRY (hdf5_close_dataset (dset_id));
	TRY (hdf5_close_group (tag_id));
	TRY (hdf5_close_group (tags_id));
	H5_RETURN (H5_SUCCESS);
}

/*!
   Get information about tagset given by index:
   name
   type
 */
h5_err_t
h5t_get_mtagset_info (
        h5t_mesh_t* const m,
        const h5_size_t idx,
        char name[],
        const h5_size_t len_name,
        h5_int64_t* const type
        ) {
	H5_CORE_API_ENTER (h5_err_t,
	                   "m=%p, idx=%llu, name=%p, len_name=%llu, type=%p",
	                   m, (long long unsigned)idx, name,
	                   (long long unsigned)len_name, type);
	TRY (ret_value = get_tagset_info(m->mesh_gid, idx, name, len_name, type));
	H5_RETURN (ret_value);
}

/*!
   Check whether taget exists.
 */
h5_err_t
h5t_mtagset_exists (
        h5t_mesh_t* const m,
        const char name[]
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p, name=%s", m, name);
	TRY (ret_value = h5priv_link_exists (m->mesh_gid, "Tags", name));
	H5_RETURN (ret_value);
}

static h5_err_t
new_tagset (
        h5t_mesh_t* const m,
        hid_t parent_gid,
        const char name[],
        h5_id_t type,
        h5t_tagset_t** rtagset
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, name='%s', type=%llu, rtagset=%p",
	                    m, name, (long long unsigned)type, rtagset);

	h5t_tagset_t* tagset = NULL;
	size_t size = (m->num_interior_elems[m->num_leaf_levels-1] - 1) * sizeof(*tagset->elems)
	              + sizeof(*tagset);
	TRY (tagset = h5_calloc (1, size));

	TRY (tagset->name = h5_strdup (name));
	tagset->m = m;
	tagset->parent_gid = parent_gid;
	tagset->type = type;
	tagset->num_interior_elems = m->num_interior_elems[m->num_leaf_levels-1];
	tagset->scope.min_level = 32767;
	tagset->scope.max_level = -1;
	TRY (h5priv_search_strlist (&m->mtagsets, name));

	*rtagset = tagset;
	H5_RETURN (H5_SUCCESS);
}

/*!
   Create a new tagset
 */
h5_err_t
h5t_create_mtagset (
        h5t_mesh_t* const m,
        const char name[],
        const h5_types_t type,
        h5t_tagset_t** set
        ) {
	H5_CORE_API_ENTER (h5_err_t,
	                   "m=%p, name='%s', type=%llu, set=%p",
	                   m, name, (long long unsigned)type, set);
	// validate name
	if (name == NULL || name[0] == '\0') {
		H5_RETURN_ERROR (
		        H5_ERR_INVAL,
			"%s",
			"Invalid name");
	}

	// validate type
	if (type != H5_INT64_T && type != H5_FLOAT64_T) {
		H5_RETURN_ERROR (
		        H5_ERR_INVAL,
			"%s",
			"Unsupported data type.");
	}

	// check if a tagset with given name already exists
	h5_err_t exists;
	TRY (exists = h5priv_link_exists (m->mesh_gid, "Tags", name));
	if (exists || h5priv_find_strlist (m->mtagsets, name) >= 0)
		H5_RETURN_ERROR (
			H5_ERR_H5FED,
			"Cannot create tagset '%s': Tagset exists", name);
	TRY (ret_value = new_tagset (m, m->mesh_gid, name, type, set));
	H5_RETURN (ret_value);
}

static int
find_face_id (
        h5t_tageleminfo_t* eleminfo,
        h5_loc_idx_t face_id
        ) {
	if (eleminfo->num_tags == 0) return -1;
	h5t_taginfo_t* taginfo = eleminfo->ti;
	register int low = 0;
	register int high = eleminfo->num_tags-1;
	while (low <= high) {
		register int mid = (low + high) / 2;
		register int diff = taginfo[mid].face_id - face_id;
		if (diff > 0)
			high = mid -1;
		else if (diff < 0)
			low = mid + 1;
		else
			return mid;
	}
	return -(low+1);
}


static h5_err_t
remove_tag (
        h5t_tagset_t* tagset,
        const h5_loc_idx_t face_id,
        const h5_loc_idx_t elem_idx
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "tagset=%p, face_id=%lld, elem_idx=%lld",
	                    tagset, (long long)face_id, (long long)elem_idx);
	if (tagset->elems[elem_idx] == NULL) {
		H5_LEAVE (
		        h5_warn (
		                "Tag %s not set for face %llx of element %lld",
		                tagset->name,
		                (long long)face_id,
		                (long long)elem_idx));
	}
	h5t_tageleminfo_t* eleminfo = tagset->elems[elem_idx];

	// remove values
	int idx = find_face_id (eleminfo, face_id);
	if (idx < 0) {
		H5_LEAVE (
		        h5_warn (
		                "Tag %s not set for face %llx of element %lld",
		                tagset->name,
		                (long long)face_id,
		                (long long)elem_idx));
	}
	h5t_taginfo_t* ti = &eleminfo->ti[idx];

	tagset->num_values -= ti->val_dim;
	memmove (tagset->values + ti->val_idx,
	         tagset->values + ti->val_idx + ti->val_dim,
	         (tagset->num_values - idx) * sizeof (tagset->values[0]));

	// remove tag info for this entity
	memmove (ti,
	         ti + 1,
	         (eleminfo->num_tags-idx-1)*sizeof (ti[0]) );

	// we don't resize the eleminfo structure!!!
	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
add_tag (
        h5t_tagset_t *tagset,
        const int idx,
        const h5_loc_idx_t face_id,
        const h5_loc_idx_t elem_idx,
        const size_t dim,
        void* val
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "tagset=%p, idx=%d, face_id=%lld, elem_idx=%lld, dim=%zu, val=%p",
	                    tagset, idx, (long long)face_id, (long long)elem_idx, dim, val);
	// insert new taginfo
	h5t_tageleminfo_t* eleminfo = tagset->elems[elem_idx];
	TRY (eleminfo = tagset->elems[elem_idx] = h5_alloc (
	                        tagset->elems[elem_idx],
	                        sizeof (*eleminfo)
	                        + eleminfo->num_tags * sizeof (eleminfo->ti[0])));
	h5t_taginfo_t* ti = &eleminfo->ti[idx];
	memmove (ti + 1,
	         ti,
	         (eleminfo->num_tags - idx) * sizeof(*ti));
	eleminfo->num_tags++;
	ti->face_id = (short)face_id;
	ti->val_dim = dim;

	// append values
	TRY (tagset->values = h5_alloc (
	             tagset->values,
	             (tagset->num_values+dim) * sizeof (*tagset->values)));
	memcpy (tagset->values + tagset->num_values,
	        val,
	        dim*sizeof (*tagset->values));
	ti->val_idx = tagset->num_values;
	tagset->num_values += dim;
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
overwrite_tag (
        h5t_tagset_t* tagset,
        const int idx,
        const h5_loc_idx_t elem_idx,
        void* val
        ) {
	h5t_tageleminfo_t* eleminfo = tagset->elems[elem_idx];
	h5t_taginfo_t* ti = &eleminfo->ti[idx];

	memcpy (tagset->values + ti->val_idx,
	        val,
	        ti->val_dim * sizeof (*ti) );
	return H5_SUCCESS;
}

/*!
   Set tag for entity in current mesh.
 */
static h5_err_t
set_tag (
        h5t_tagset_t* tagset,
        const h5_loc_idx_t face_id,
        const h5_loc_idx_t elem_idx,
        const size_t dim,
        void* val
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "tagset=%p, face_id=%lld, elem_idx=%lld, dim=%zu, val=%p",
	                    tagset, (long long)face_id, (long long)elem_idx, dim, val);
	if (tagset->elems[elem_idx] == NULL) {
		TRY (tagset->elems[elem_idx] = h5_calloc (
		             1, sizeof (*tagset->elems)));
	}
	h5t_tageleminfo_t* eleminfo = tagset->elems[elem_idx];
	int i = find_face_id (eleminfo, face_id);
	h5t_taginfo_t* ti = eleminfo->ti + i;
	if (i >= 0 && dim != ti->val_dim) {
		/*
		   Overwrite existing value with new dimension.
		   This is a very unusual case.
		 */
		TRY (remove_tag (tagset, face_id, elem_idx));
		TRY (add_tag (tagset, i, face_id, elem_idx, dim, val));
	} else if (i >= 0 && dim == ti->val_dim) {
		TRY (overwrite_tag (tagset, i, elem_idx, val));
	} else { // i < 0
		TRY (add_tag (tagset, -i-1, face_id, elem_idx, dim, val));
		tagset->num_entities++;
	}
	if (tagset->m->leaf_level < tagset->scope.min_level) {
		tagset->scope.min_level = tagset->m->leaf_level;
	}
	if (tagset->m->leaf_level > tagset->scope.max_level) {
		tagset->scope.max_level = tagset->m->leaf_level;
	}
	H5_RETURN (H5_SUCCESS);
}


static h5_err_t
read_tagset (
        h5t_tagset_t* const tagset
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "tagset=%p", tagset);
	hid_t loc_id = 0;

	// open HDF5 group
	TRY (loc_id = h5priv_open_group_with_intermediates (
		     tagset->parent_gid,
		     "Tags", tagset->name, NULL));

	// read datasets:

	// "elems"
	h5t_glb_tag_idx_t* elems;
	size_t num_interior_elems = 0;

	hid_t dset_id;
	TRY (dset_id = hdf5_open_dataset_by_name (loc_id, "elems"));
	TRY (num_interior_elems = hdf5_get_npoints_of_dataset (dset_id));
	TRY (elems = h5_calloc (num_interior_elems, sizeof(*elems)));

	h5_dsinfo_t dsinfo;
	memset (&dsinfo, 0, sizeof (dsinfo));
	dsinfo.type_id = h5_dta_types.h5t_glb_tag_idx_t;
	TRY (read_dataset (
                     tagset->m,
                     tagset->m->f,
		     dset_id,
		     &dsinfo,
		     open_space_all,
                     open_space_all,
		     elems));
	TRY (hdf5_close_dataset (dset_id));
	num_interior_elems--;

	// "entities"
	h5t_glb_tag_idx_t* entities;
	size_t ent_idx = 0;
	size_t num_entities = 0;
	TRY (dset_id = hdf5_open_dataset_by_name (loc_id, "entities"));
	TRY (num_entities = hdf5_get_npoints_of_dataset (dset_id));
	TRY (entities = h5_calloc (num_entities, sizeof(*entities)));
	TRY (read_dataset (
                     tagset->m,
	             tagset->m->f,
	             dset_id,
	             &dsinfo,
	             open_space_all, open_space_all,
	             entities));
	TRY (hdf5_close_dataset (dset_id));
	num_entities--;

	//  "values"
	h5_int64_t* vals;
	size_t num_vals = 0;
	TRY (dset_id = hdf5_open_dataset_by_name (loc_id, "values"));
	TRY (num_vals = hdf5_get_npoints_of_dataset (dset_id));
	TRY (vals = h5_calloc (num_vals, sizeof (*vals)));
	TRY (dsinfo.type_id = h5priv_get_normalized_dataset_type (dset_id));
	TRY (read_dataset (
				 tagset->m,
	             tagset->m->f,
	             dset_id,
	             &dsinfo,
	             open_space_all, open_space_all,
	             vals));
	TRY (hdf5_close_dataset (dset_id));
	tagset->type = dsinfo.type_id;

	/*
	   add tagset and set values
	 */

	h5_int64_t scope;
	TRY (h5priv_read_attrib (loc_id, "__scope_min__", H5_INT64_T, &scope));
	tagset->scope.min_level = scope;
	TRY (h5priv_read_attrib (loc_id, "__scope_max__", H5_INT64_T, &scope));
	tagset->scope.max_level = scope;

	for (ent_idx = 0; ent_idx < num_entities; ent_idx++) {
		h5t_glb_tag_idx_t *entity = &entities[ent_idx];
		size_t dim = (entity+1)->idx - entity->idx;
		// map global face id and global element idx to local
		h5_loc_idx_t face_id;
		h5_loc_idx_t elem_idx;
		h5_glb_idx_t glb_elem_idx = h5tpriv_get_elem_idx (entity->eid);
		elem_idx = h5t_map_glb_elem_idx2loc (tagset->m, glb_elem_idx);
		assert (elem_idx >= 0);
		face_id = h5tpriv_get_face_id (entity->eid);
		TRY (set_tag (
		             tagset,
		             face_id,
		             elem_idx,
		             dim,
		             &vals[entity->idx]));
	}
	TRY (hdf5_close_group (loc_id));
	TRY (h5_free (elems));
	TRY (h5_free (entities));
	TRY (h5_free (vals));
	H5_RETURN (H5_SUCCESS);
}

/*!
   Open existing tagset given by \c name.
 */
h5_err_t
h5t_open_mtagset (
        h5t_mesh_t* const m,
        const char name[],
        h5t_tagset_t** set
        ) {
	H5_CORE_API_ENTER (h5_err_t,
	                   "m=%p, name='%s', set=%p",
	                   m, name, set);
	// validate name
	if (name == NULL || name[0] == '\0') {
		H5_RETURN_ERROR (
		        H5_ERR_INVAL,
			"%s",
			"Invalid name");
	}

	// check if a tagset with given name exists
	h5_err_t exists;
	TRY (exists = h5priv_link_exists (m->mesh_gid, "Tags", name));
	if (!exists)
		H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Cannot open tagset '%s': No such tagset ",
			name);

	// check if tagset has already been opened
	if (h5priv_find_strlist (m->mtagsets, name) >= 0)
		H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Cannot open tagset '%s': Already open ",
			name);


	TRY (new_tagset (m, m->mesh_gid, name, -1, set));
	TRY (read_tagset (*set));
	H5_RETURN (H5_SUCCESS);
}

/*
   Write tagset to disk.
 */
static h5_err_t
write_tagset (
        h5t_tagset_t* tagset
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "tagset=%p", tagset);
	h5t_tageleminfo_t** eleminfos = tagset->elems;
	hid_t group_id;
	h5t_glb_tag_idx_t* elems = NULL;   // in memory dataset
	h5t_glb_tag_idx_t* elem = NULL;    // reference an element in elems
	h5_loc_idx_t num_interior_elems = 0;
	h5t_glb_tag_idx_t* entities = NULL; // in memory dataset
	h5t_glb_tag_idx_t* entity = NULL;  // reference an element in entities
	h5t_tagval_t* values = NULL;       // in memory dataset

	h5_loc_idx_t elem_idx = 0;
	h5_loc_idx_t entity_idx = 0;
	h5_loc_idx_t val_idx = 0;

	h5t_mesh_t* m = tagset->m;
	if (m->num_leaf_levels <= 0) {
		H5_LEAVE (H5_SUCCESS); // nothing to do
	}
	num_interior_elems = m->num_interior_elems[m->num_leaf_levels-1];
	if (num_interior_elems == 0 || tagset->num_entities == 0) {
		H5_LEAVE (H5_SUCCESS); // nothing to do
	}
	// allocate memory per element (plus 1)
	TRY (elems = h5_calloc (num_interior_elems+1, sizeof(*elems)));
	elem = elems;

	// allocate memory per entity (plus 1)
	TRY (entities = h5_calloc (tagset->num_entities+1, sizeof(*entities)) );
	entity = entities;

	// allocate memory for all values
	TRY (values = h5_calloc (tagset->num_values, sizeof(*values)) );

	// build data structures in memory
	while (elem < elems+num_interior_elems) {
		elem->eid = elem_idx;
		elem->idx = entity_idx;
		h5t_tageleminfo_t* eleminfo = *eleminfos;

		// loop over all tagged faces of this element
		int ti_idx;
		for (ti_idx = 0; eleminfo && ti_idx < eleminfo->num_tags; ti_idx++) {
			h5t_taginfo_t* ti = eleminfo->ti+ti_idx;
			h5_glb_idx_t glb_elem_idx = h5tpriv_get_loc_elem_glb_idx (
			        m, elem_idx);
			entity->eid = h5tpriv_build_entity_id (
			        0, (h5_glb_id_t)ti->face_id, glb_elem_idx);
			entity->idx = val_idx;

			// copy values
			memcpy (values + val_idx,
			        &tagset->values[ti->val_idx],
			        ti->val_dim * sizeof (*values));
			val_idx += ti->val_dim;
			entity_idx++;
			entity++;
		}
		elem_idx++;
		eleminfos++;
		elem++;
	}
	elem->eid = -1;         // last entry
	tagset->num_entities = elem->idx = entity_idx;
	entity->eid = -1;
	tagset->num_values = entity->idx = val_idx;

	// write data
	TRY (group_id = h5priv_create_group_with_intermediates (
	             tagset->parent_gid,
		     "Tags", tagset->name, NULL));
	h5_dsinfo_t dsinfo;
	memset (&dsinfo, 0, sizeof(dsinfo));
	dsinfo.rank = 1;
	dsinfo.max_dims[0] = H5S_UNLIMITED;
	dsinfo.chunk_dims[0] = 4096;
	dsinfo.access_prop = H5P_DEFAULT;

	strcpy (dsinfo.name, "elems");
	dsinfo.dims[0] = num_interior_elems + 1;
	dsinfo.type_id = h5_dta_types.h5t_glb_tag_idx_t;
	TRY (dsinfo.create_prop = hdf5_create_property (H5P_DATASET_CREATE));
	TRY (hdf5_set_chunk_property (dsinfo.create_prop, dsinfo.rank,
	                              dsinfo.chunk_dims));

	TRY (h5priv_write_dataset_by_name (
				 m,
	             m->f,
	             group_id,
	             &dsinfo,
	             open_space_all, open_space_all,
	             elems));

	strcpy (dsinfo.name, "entities");
	dsinfo.dims[0] = tagset->num_entities + 1;

	TRY (h5priv_write_dataset_by_name (
				 m,
	             m->f,
	             group_id,
	             &dsinfo,
	             open_space_all, open_space_all,
	             entities));

	strcpy (dsinfo.name, "values");
	dsinfo.dims[0] = tagset->num_values;
	dsinfo.type_id = h5_dta_types.h5_int64_t;

	TRY (h5priv_write_dataset_by_name (
				 m,
	             m->f,
	             group_id,
	             &dsinfo,
	             open_space_all, open_space_all,
	             values));
	h5_int64_t scope = tagset->scope.min_level;
	TRY (h5priv_write_attrib (group_id, "__scope_min__", H5_INT64_T, &scope, 1));
	scope = tagset->scope.max_level;
	TRY (h5priv_write_attrib (group_id, "__scope_max__", H5_INT64_T, &scope, 1));

	TRY (hdf5_close_group (group_id));
	TRY (h5_free (elems));
	TRY (h5_free (entities));
	TRY (h5_free (values));
	H5_RETURN (H5_SUCCESS);
}


/*
   Release a tag-set
 */
static inline h5_err_t
release_mtagset (
        h5t_tagset_t* tagset
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "tagset=%p", tagset);
	unsigned int i;
	// release per element structures
	for (i = 0; i < tagset->num_interior_elems; i++) {
		if (tagset->elems[i] != NULL) {
			TRY (h5_free (tagset->elems[i]));
		}
	}
	// remove from book-keeping list in mesh
	TRY (h5priv_remove_strlist(tagset->m->mtagsets, tagset->name));

	// release other memory
	TRY (h5_free (tagset->name));
	TRY (h5_free (tagset->values));
	TRY (h5_free (tagset));

	H5_RETURN (H5_SUCCESS);
}

/*!
   Close tagset.

   Write data to disk, if something as been changed, and release memory.
 */
h5_err_t
h5t_close_mtagset (
        h5t_tagset_t* tagset
        ) {
	H5_CORE_API_ENTER (h5_err_t, "tagset=%p", tagset);
	if (tagset->changed) {
		TRY (write_tagset (tagset));
	}
	TRY (release_mtagset (tagset));
	H5_RETURN (H5_SUCCESS);
}

/*!
   Delete tagset on disk.

   Note:
   There may be a copy in memory! This copy is still accessable, even
   changes are possible. Since data are written on close, the tagset will
   be created again.
   We should implement something to avoid this! We have a similiar problem,
   if the user opens the dataset more than once.
 */

/*!
   Remove a tagset from the current mesh.

   \param[in]	f	file handle
   \param[in]	name	name of tagset to remove

   \return	H5_SUCCESS or error code
 */
static h5_err_t
remove_tagset (
        const hid_t tagsets_id,
        const char name[]
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
			    "tagsets_id=%lld, name=%s",
			    (long long int)tagsets_id, name);
	hid_t loc_id;
	TRY (loc_id = hdf5_open_group (tagsets_id, name));
	TRY (hdf5_delete_link (loc_id, "elems", H5P_DEFAULT));
	TRY (hdf5_delete_link (loc_id, "entities", H5P_DEFAULT));
	TRY (hdf5_delete_link (loc_id, "values", H5P_DEFAULT));
	TRY (hdf5_close_group (loc_id));
	TRY (hdf5_delete_link (tagsets_id, name, H5P_DEFAULT));
	H5_RETURN (H5_SUCCESS);
}

/*!
   Remove a tagset from the current mesh.

   \param[in]	f	file handle
   \param[in]	name	name of tagset to remove

   \return	H5_SUCCESS or error code
 */
h5_err_t
h5t_remove_mtagset (
        h5t_mesh_t* const m,
        const char name[]
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p, name='%s'", m, name);

	// check if tagset has a copy in memory
	if (h5priv_find_strlist (m->mtagsets, name) >= 0)
		H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Cannot remove tagset '%s': Still open ",
			name);

	hid_t loc_id;
	TRY (loc_id = hdf5_open_group (m->mesh_gid, "Tags"));
	TRY (remove_tagset (loc_id, name));
	TRY (hdf5_close_group (loc_id));
	H5_RETURN (H5_SUCCESS);
}

/*!
   Set tag.
 */
h5_err_t
h5t_set_tag (
        h5t_tagset_t* const tagset,
        const h5_loc_id_t entity_id,
        const h5_size_t size,
        void* val
        ) {
	H5_CORE_API_ENTER (h5_err_t,
	                   "tagset=%p, entity_id=%lld, size=%llu, val=%p",
	                   tagset,
	                   (long long)entity_id,
	                   (long long unsigned)size,
	                   val);
	h5_loc_idx_t face_id = h5tpriv_get_face_id (entity_id);
	h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (entity_id);
	TRY (set_tag (tagset, face_id, elem_idx, size, val));
	tagset->changed = 1;
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
get_idx_of_tagval (
        const h5t_tagset_t* tagset,
        const h5_loc_id_t entity_id,
        int* taginfo_idx,
        h5_loc_idx_t* val_idx
        ) {
	h5_loc_idx_t face_id = h5tpriv_get_face_id (entity_id);
	h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (entity_id);
	h5t_tageleminfo_t* eleminfo = tagset->elems[elem_idx];
	if (eleminfo == NULL) {
		return H5_NOK; // not tagged
	}
	*taginfo_idx = find_face_id (eleminfo, face_id);
	if (*taginfo_idx < 0) {
		return H5_NOK; // not tagged
	}
	*val_idx = eleminfo->ti[*taginfo_idx].val_idx;
	return H5_SUCCESS;
}

/*!
   Get tag for entity in given tagset. Untagged entities inherit tags from
   their closest parent.

   \param[in]	tagset		tagset
   \param[in]	entity_id	id of entity
   \param[out]	size		size of value
   \param[out]	vals		tag value

   \return	H5_SUCCESS or error code
 */
h5_loc_id_t
h5t_get_tag (
        const h5t_tagset_t* tagset,
        const h5_loc_id_t entity_id,
        h5_size_t* const dim,
        void* const values
        ) {
	H5_CORE_API_ENTER (h5_err_t,
	                   "tagset=%p, entity_id=%lld, dim=%p, values=%p",
	                   tagset,
	                   (long long)entity_id,
	                   dim,
	                   values);
	if (tagset->m->leaf_level < tagset->scope.min_level) {
		H5_LEAVE (H5_NOK); // entity not tagged
	}
	h5_loc_id_t id = entity_id;
	h5_err_t h5err;
	int ti_idx = 0;
	h5_loc_idx_t val_idx = 0;
	// query entity while not tagged and parent exists
	while ((h5err = get_idx_of_tagval (tagset, id, &ti_idx, &val_idx)) < 0 &&
	       (id = h5tpriv_get_loc_entity_parent (tagset->m, id)) >= 0) ;

	if (h5err < 0)
		H5_LEAVE (H5_NOK);  // entity not tagged

	h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (id);
	h5t_taginfo_t* ti = &tagset->elems[elem_idx]->ti[ti_idx];
	h5t_tagval_t* v = tagset->values;
	if (*dim > ti->val_dim || values == NULL) {
		*dim = ti->val_dim;
	}
	if (values != NULL) {
		memcpy (values, v + val_idx, *dim*sizeof(*v) );
	}
	H5_RETURN (id);
}

/*!
   Remove tag from entity

   \param[in]	tagset		pointer to tagset
   \param[in]	entity_id	local entity id
 */
h5_err_t
h5t_remove_tag (
        h5t_tagset_t* tagset,
        const h5_loc_id_t entity_id
        ) {
	H5_CORE_API_ENTER (h5_err_t,
	                   "tagset=%p, entity_id=%lld",
	                   tagset, (long long)entity_id);
	h5_loc_idx_t face_id = h5tpriv_get_face_id (entity_id);
	h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (entity_id);
	TRY (ret_value = remove_tag (tagset, face_id, elem_idx));
	H5_RETURN (ret_value);
}

