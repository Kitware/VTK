/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5FED_TAGS_H
#define __H5FED_TAGS_H

#include "h5core/h5_types.h"
#include "h5core/h5_log.h"
#include "h5core/h5t_tags.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
   Get number of tagsets assocciated with the mesh.

   \param[in]	m	mesh
 */
static inline h5_ssize_t
H5FedGetNumMTagsets (
        h5t_mesh_t* const m
        ) {
	H5_API_ENTER (h5_ssize_t, "m=%p", m);
	H5_API_RETURN (h5t_get_num_mtagsets(m));
}

/*!
   Get some information about the tagset \c name.

   \param[in]	m		mesh
   \param[in]	idx		index of tagset to query
   \param[out]	name		name of tagset
   \param[in]	len_name	len of buffer \c name
   \param[out]	type		type of tagset
 */
static inline h5_err_t
H5FedGetMTagsetInfo (
        h5t_mesh_t* const m,
        const h5_size_t idx,
        char name[],
        const h5_size_t len_name,
        h5_int64_t* const type
        ) {
	H5_API_ENTER (h5_err_t,
	              "m=%p, idx=%llu, name=%p, len_name=%llu, type=%p",
	              m, (long long unsigned)idx, name,
	              (long long unsigned)len_name, type);
	H5_API_RETURN (h5t_get_mtagset_info (m, idx, name, len_name, type));
}

/*!
   Test whether tagset \c name exists.

   \param[in]	m		mesh
   \param[out]	name		name of tagset to test existance
 */
static inline h5_err_t
H5FedMTagsetExists (
        h5t_mesh_t* const m,
        const char name[]
        ) {
	H5_API_ENTER (h5_err_t, "m=%p, name=%s", m, name);
	H5_API_RETURN (h5t_mtagset_exists (m, name));
}

/*!
   Add a tagset to the current mesh.

   \param[in]	m		mesh to add a tagset to
   \param[in]	name		name of tagset
   \param[in]	type		data type of tagset
   \param[out]	tagset		new tagset

   \return	H5_SUCCESS or error code
 */
static inline h5_err_t
H5FedAddMTagset (
        h5t_mesh_t* const m,
        const char name[],
        const h5_types_t type,
        h5t_tagset_t** tagset
        ) {
	H5_API_ENTER (h5_err_t,
	              "m=%p, name='%s', type=%lld tagset=%p",
	              m, name, (long long)type, tagset);
	H5_API_RETURN (h5t_create_mtagset (m, name, type, tagset));
}

/*!
   Open tagset \c name.

   \param[in]	m	mesh
   \param[in]	name	name of tagset to open
   \param[out]	tagset	open tagset

   \return	H5_SUCCESS or error code
 */
static inline h5_err_t
H5FedOpenMTagset (
        h5t_mesh_t* const m,
        const char name[],
        h5t_tagset_t** tagset
        ) {
	H5_API_ENTER (h5_err_t,
	              "m=%p, name='%s', tagset=%p",
	              m, name, tagset);
	H5_API_RETURN (h5t_open_mtagset (m, name, tagset));
}

/*!
   Close tagset.

   \param[in]	tagset	open tagset

   \return	H5_SUCCESS or error code
 */
static inline h5_err_t
H5FedCloseMTagset (
        h5t_tagset_t* tagset
        ) {
	H5_API_ENTER (h5_err_t, "tagset=%p", tagset);
	H5_API_RETURN (h5t_close_mtagset(tagset));
}

/**
   Remove tagset from mesh

   \return	H5_SUCCESS or error code
 */
static inline h5_err_t
H5FedRemoveMTagset (
        h5t_mesh_t* m,	    ///< mesh object
        const char name[]   ///< [in] name of tagset to remove
        ) {
	H5_API_ENTER (h5_err_t, "m=%p, name='%s'", m, name);
	H5_API_RETURN (h5t_remove_mtagset (m, name));
}

/**
   Set tag for entity in current mesh.

   \return	H5_SUCCESS or error code
 */
static inline h5_err_t
H5FedSetTag (
        h5t_tagset_t* const tagset, ///< [in,out] tagset object
        h5_loc_id_t entity_id,	    ///< [in] entity the tag is assigned to
        const h5_size_t size,	    ///< [in] byte-size of value
        void* val		    ///< [in] value of tag
        ) {
	H5_API_ENTER (h5_err_t,
	              "tagset=%p, entity_id=%lld, size=%lld, val=%p",
	              tagset, (long long)entity_id, (long long)size, val);
	H5_API_RETURN (h5t_set_tag (tagset, entity_id, size, val));
}

/*!
   Get tag for entity in current mesh. If entity is not tagged, return tag of
   the next coarser tagged entity. The corresponding entity ID is returned.

   \param[in]	tagset		ptr to tagset
   \param[in]	entity_id	id of entity to tag
   \param[out]	size		size of value
   \param[out]	val		tag value

   \return	entity id
   \return	H5_ERR on error
 */
static inline h5_loc_id_t
H5FedGetTag (
        h5t_tagset_t* const tagset,
        const h5_loc_id_t entity_id,
        h5_size_t* size,
        void* val
        ) {
	H5_API_ENTER (h5_err_t,
	              "tagset=%p, entity_id=%lld, size=%p, val=%p",
	              tagset, (long long)entity_id, size, val);
	H5_API_RETURN (h5t_get_tag (tagset, entity_id, size, val));
}

/*!
   Remove tag for entity in current mesh.

   \param[in]	tagset		ptr to tagset tagset
   \param[in]	entity_id	id of entity from which the tag must be removed.

   \return	H5_SUCCESS or error code
 */
static inline h5_err_t
H5FedRemoveMTag (
        h5t_tagset_t* const tagset,
        const h5_loc_id_t entity_id
        ) {
	H5_API_ENTER (h5_err_t,
	              "tagset=%p, entity_id=%lld",
	              tagset, (long long)entity_id);
	H5_API_RETURN (h5t_remove_tag (tagset, entity_id));
}

#ifdef __cplusplus
}
#endif

#endif
