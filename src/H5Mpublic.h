/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * This file contains public declarations for the H5M module.
 *
 * NOTE:    This is an experimental API. Everything in the H5M package
 *          is subject to revision in a future release.
 */
#ifndef H5Mpublic_H
#define H5Mpublic_H

/* System headers needed by this file */

/* Public headers needed by this file */
#include "H5public.h"
#include "H5Ipublic.h"

/*****************/
/* Public Macros */
/*****************/

/* Macros defining operation IDs for map VOL callbacks (implemented using the
 * "optional" VOL callback) */
#define H5VL_MAP_CREATE   1
#define H5VL_MAP_OPEN     2
#define H5VL_MAP_GET_VAL  3
#define H5VL_MAP_EXISTS   4
#define H5VL_MAP_PUT      5
#define H5VL_MAP_GET      6
#define H5VL_MAP_SPECIFIC 7
#define H5VL_MAP_OPTIONAL 8
#define H5VL_MAP_CLOSE    9

/*******************/
/* Public Typedefs */
/*******************/

/* types for map GET callback */
typedef enum H5VL_map_get_t {
    H5VL_MAP_GET_MAPL,     /* map access property list */
    H5VL_MAP_GET_MCPL,     /* map creation property list */
    H5VL_MAP_GET_KEY_TYPE, /* key type                 */
    H5VL_MAP_GET_VAL_TYPE, /* value type               */
    H5VL_MAP_GET_COUNT     /* key count                */
} H5VL_map_get_t;

/* types for map SPECIFIC callback */
typedef enum H5VL_map_specific_t {
    H5VL_MAP_ITER,  /* H5Miterate               */
    H5VL_MAP_DELETE /* H5Mdelete                */
} H5VL_map_specific_t;

//! <!-- [H5M_iterate_t_snip] -->
/**
 * Callback for H5Miterate()
 */
typedef herr_t (*H5M_iterate_t)(hid_t map_id, const void *key, void *op_data);
//! <!-- [H5M_iterate_t_snip] -->

/********************/
/* Public Variables */
/********************/

/*********************/
/* Public Prototypes */
/*********************/
#ifdef __cplusplus
extern "C" {
#endif

/* The map API is only built when requested since there's no support in
 * the native file format at this time. It's only supported in a few VOL
 * connectors.
 */
#ifdef H5_HAVE_MAP_API

/**
 * \ingroup H5M
 *
 * \brief Creates a map object
 *
 * \fgdta_loc_id
 * \param[in] name Map object name
 * \type_id{key_type_id}
 * \type_id{val_type_id}
 * \lcpl_id
 * \mcpl_id
 * \mapl_id
 * \returns \hid_t{map object}
 *
 * \details H5Mcreate() creates a new map object for storing key-value
 *          pairs. The in-file datatype for keys is defined by \p key_type_id
 *          and the in-file datatype for values is defined by \p val_type_id. \p
 *          loc_id specifies the location to create the the map object and \p
 *          name specifies the name of the link to the map object relative to
 *          \p loc_id.
 *
 * \since 1.12.0
 *
 */
H5_DLL hid_t H5Mcreate(hid_t loc_id, const char *name, hid_t key_type_id, hid_t val_type_id, hid_t lcpl_id,
                       hid_t mcpl_id, hid_t mapl_id);

/**
 * \ingroup H5M
 *
 * \brief
 *
 * \details
 *
 * \since 1.12.0
 *
 */
H5_DLL hid_t H5Mcreate_anon(hid_t loc_id, hid_t key_type_id, hid_t val_type_id, hid_t mcpl_id, hid_t mapl_id);

/**
 * \ingroup H5M
 *
 * \brief Opens a map object
 *
 * \fgdta_loc_id{loc_id}
 * \param[in] name Map object name relative to \p loc_id
 * \mapl_id
 * \returns \hid_t{map object}
 *
 * \details H5Mopen() finds a map object specified by \p name under the location
 *          specified by \p loc_id. The map object should be close with
 *          H5Mclose() when the application is not longer interested in
 *          accessing it.
 *
 * \since 1.13.0
 *
 */
H5_DLL hid_t H5Mopen(hid_t loc_id, const char *name, hid_t mapl_id);

/**
 * \ingroup H5M
 *
 * \brief Terminates access to a map object
 *
 * \map_id
 * \returns \herr_t
 *
 * \details H5Mclose() closes access to a map object specified by \p map_id and
 *          releases resources used by it.
 *
 *          It is illegal to subsequently use that same map identifier in calls
 *          to other map functions.
 *
 * \since 1.12.0
 *
 */
H5_DLL herr_t H5Mclose(hid_t map_id);

/**
 * \ingroup H5M
 *
 * \brief Gets key datatype for a map object
 *
 * \map_id
 * \returns \hid_t{datatype}
 *
 * \details H5Mget_key_type() retrieves key datatype as stored in the file for a
 *          map object specified by \p map_id and returns identifier for the
 *          datatype.
 *
 * \since 1.12.0
 *
 */
H5_DLL hid_t H5Mget_key_type(hid_t map_id);

/**
 * \ingroup H5M
 *
 * \brief Gets value datatype for a map object
 *
 * \map_id
 * \returns \hid_t{datatype}
 *
 * \details H5Mget_val_type() retrieves value datatype as stored in the file for
 *          a map object specified by \p map_id and returns identifier for the
 *          datatype .
 *
 * \since 1.12.0
 *
 */
H5_DLL hid_t H5Mget_val_type(hid_t map_id);

/**
 * \ingroup H5M
 *
 * \brief Gets creation property list for a map object
 *
 * \map_id
 * \returns \hid_t{map creation property list}
 *
 * \details H5Mget_create_plist() returns an identifier for a copy of the
 *          creation property list for a map object specified by \p map_id.
 *
 * \since 1.13.0
 *
 */
H5_DLL hid_t H5Mget_create_plist(hid_t map_id);

/**
 * \ingroup H5M
 *
 * \brief Gets access property list for a map object
 *
 * \map_id
 * \returns \hid_t{map access property list}
 *
 * \details H5Mget_access_plist() returns an identifier for a copy of the access
 *          property list for a map object specified by \p map_id.
 *
 * \since 1.12.0
 *
 */
H5_DLL hid_t H5Mget_access_plist(hid_t map_id);

/**
 * \ingroup H5M
 *
 * \brief Retrieves the number of key-value pairs in a map object
 *
 * \map_id
 * \param[out] count The number of key-value pairs stored in the map object
 * \dxpl_id
 * \returns \herr_t
 *
 * \details H5Mget_count() retrieves the number of key-value pairs stored in a
 *          map specified by map_id.
 *
 * \since 1.12.0
 *
 */
H5_DLL herr_t H5Mget_count(hid_t map_id, hsize_t *count, hid_t dxpl_id);

/**
 * \ingroup H5M
 *
 * \brief Adds a key-value pair to a map object
 *
 * \map_id
 * \type_id{key_mem_type_id}
 * \param[in] key Pointer to key buffer
 * \type_id{val_mem_type_id}
 * \param[in] value Pointer to value buffer
 * \dxpl_id
 * \returns \herr_t
 *
 * \details H5Mput() adds a key-value pair to a map object specified by \p
 *          map_id, or updates the value for the specified key if one was set
 *          previously.
 *
 *          \p key_mem_type_id and \p val_mem_type_id specify the datatypes for
 *          the provided key and value buffers, and if different from those used
 *          to create the map object, the key and value will be internally
 *          converted to the datatypes for the map object.
 *
 *          Any further options can be specified through the property list
 *          \p dxpl_id.
 *
 * \since 1.12.0
 *
 */
H5_DLL herr_t H5Mput(hid_t map_id, hid_t key_mem_type_id, const void *key, hid_t val_mem_type_id,
                     const void *value, hid_t dxpl_id);

/**
 * \ingroup H5M
 *
 * \brief Retrieves a key-value pair from a map object
 *
 * \map_id
 * \type_id{key_mem_type_id}
 * \param[in] key Pointer to key buffer
 * \type_id{val_mem_type_id}
 * \param[out] value Pointer to value buffer
 * \dxpl_id
 * \returns \herr_t
 *
 * \details H5Mget() retrieves from a map object specified by \p map_id, the
 *          value associated with the provided key \p key. \p key_mem_type_id
 *          and \p val_mem_type_id specify the datatypes for the provided key
 *          and value buffers. If if the datatype specified by \p
 *          key_mem_type_id is different from that used to create the map object
 *          the key will be internally converted to the datatype for the map
 *          object for the query, and if the datatype specified by \p
 *          val_mem_type_id is different from that used to create the map object
 *          the returned value will be converted to have a datatype as specified
 *          by \p val_mem_type_id before the function returns.
 *
 *          Any further options can be specified through the property list
 *          \p dxpl_id.
 *
 * \since 1.12.0
 *
 */
H5_DLL herr_t H5Mget(hid_t map_id, hid_t key_mem_type_id, const void *key, hid_t val_mem_type_id, void *value,
                     hid_t dxpl_id);

/**
 * \ingroup H5M
 *
 * \brief Checks if provided key exists in a map object
 *
 * \map_id
 * \type_id{key_mem_type_id}
 * \param[in] key Pointer to key buffer
 * \param[out] exists Pointer to a buffer to return the existence status
 * \dxpl_id
 * \returns \herr_t
 *
 * \details H5Mexists() checks if the provided key is stored in the map object
 *          specified by \p map_id. If \p key_mem_type_id is different from that
 *          used to create the map object the key will be internally converted
 *          to the datatype for the map object for the query.
 *
 *          Any further options can be specified through the property list
 *          \p dxpl_id.
 *
 * \since 1.12.0
 *
 */
H5_DLL herr_t H5Mexists(hid_t map_id, hid_t key_mem_type_id, const void *key, hbool_t *exists, hid_t dxpl_id);

/**
 * \ingroup H5M
 *
 * \brief Iterates over all key-value pairs in a map object
 *
 * \map_id
 * \param[in,out] idx iteration index
 * \type_id{key_mem_type_id}
 * \param[in] op User-defined iterator function
 * \op_data
 * \dxpl_id
 * \returns \herr_t
 *
 * \details H5Miterate() iterates over all key-value pairs stored in the map
 *          object specified by \p map_id, making the callback specified by \p
 *          op for each. The \p idx parameter is an in/out parameter that may be
 *          used to restart a previously interrupted iteration. At the start of
 *          iteration \p idx should be set to 0, and to restart iteration at the
 *          same location on a subsequent call to H5Miterate(), \p idx should be
 *          the same value as returned by the previous call. Iterate callback is
 *          defined as:
 *          \snippet this H5M_iterate_t_snip
 *          The \p key parameter is the buffer for the key for this iteration,
 *          converted to the datatype specified by \p key_mem_type_id. The \p
 *          op_data parameter is a simple pass through of the value passed to
 *          H5Miterate(), which can be used to store application-defined data for
 *          iteration. A negative return value from this function will cause
 *          H5Miterate() to issue an error, while a positive return value will
 *          cause H5Miterate() to stop iterating and return this value without
 *          issuing an error. A return value of zero allows iteration to continue.
 *
 *          Any further options can be specified through the property list \p dxpl_id.
 *
 * \since 1.12.0
 *
 */
H5_DLL herr_t H5Miterate(hid_t map_id, hsize_t *idx, hid_t key_mem_type_id, H5M_iterate_t op, void *op_data,
                         hid_t dxpl_id);

/**
 * \ingroup H5M
 *
 * \brief Iterates over all key-value pairs in a map object
 *
 * \loc_id
 * \param[in] map_name Map object name relative to the location specified by \p loc_id
 * \param[in,out] idx Iteration index
 * \type_id{key_mem_type_id}
 * \param[in] op User-defined iterator function
 * \op_data
 * \dxpl_id
 * \lapl_id
 * \returns \herr_t
 *
 * \details H5Miterate_by_name() iterates over all key-value pairs stored in the
 *          map object specified by \p map_id, making the callback specified by
 *          \p op for each. The \p idx parameter is an in/out parameter that may
 *          be used to restart a previously interrupted iteration. At the start
 *          of iteration \p idx should be set to 0, and to restart iteration at
 *          the same location on a subsequent call to H5Miterate(), \p idx
 *          should be the same value as returned by the previous call. Iterate
 *          callback is defined as:
 *          \snippet this H5M_iterate_t_snip
 *          The\p key parameter is the buffer for the key for this iteration,
 *          converted to the datatype specified by \p key_mem_type_id. The \p
 *          op_data parameter is a simple pass through of the value passed to
 *          H5Miterate(), which can be used to store application-defined data
 *          for iteration. A negative return value from this function will cause
 *          H5Miterate() to issue an error, while a positive return value will cause
 *          H5Miterate() to stop iterating and return this value without issuing an
 *          error. A return value of zero allows iteration to continue.
 *
 *          Any further options can be specified through the property list \p dxpl_id.
 *
 * \since 1.12.0
 *
 */
H5_DLL herr_t H5Miterate_by_name(hid_t loc_id, const char *map_name, hsize_t *idx, hid_t key_mem_type_id,
                                 H5M_iterate_t op, void *op_data, hid_t dxpl_id, hid_t lapl_id);

/**
 * \ingroup H5M
 *
 * \brief Deletes a key-value pair from a map object
 *
 * \map_id
 * \type_id{key_mem_type_id}
 * \param[in] key Pointer to key buffer
 * \dxpl_id
 * \returns \herr_t
 *
 * \details H5Mdelete() deletes a key-value pair from the map object specified
 *          by \p map_id. \p key_mem_type_id specifies the datatype for the
 *          provided key buffer key, and if different from that used to create
 *          the map object, the key will be internally converted to the datatype
 *          for the map object.
 *
 *          Any further options can be specified through the property list \p dxpl_id.
 *
 * \since 1.12.0
 *
 */
H5_DLL herr_t H5Mdelete(hid_t map_id, hid_t key_mem_type_id, const void *key, hid_t dxpl_id);

/* Symbols defined for compatibility with previous versions of the HDF5 API.
 *
 * Use of these symbols is deprecated.
 */
#ifndef H5_NO_DEPRECATED_SYMBOLS
#endif /* H5_NO_DEPRECATED_SYMBOLS */

#endif /*  H5_HAVE_MAP_API */

#ifdef __cplusplus
}
#endif

#endif /* H5Mpublic_H */
