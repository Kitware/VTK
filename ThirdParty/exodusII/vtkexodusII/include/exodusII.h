/*
 * Copyright (c) 2005-2017 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of NTESS nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*****************************************************************************
 *
 * exodusII.h - Exodus II API include file
 *
 *****************************************************************************/

#ifndef EXODUS_II_HDR
#define EXODUS_II_HDR

#include "exodus_config.h"
#include "exodusII_cfg.h"
#include "vtk_exodusII_mangle.h"

#include "vtk_netcdf.h"

#if VTK_MODULE_USE_EXTERNAL_vtknetcdf
#if defined(NC_HAVE_META_H)
#include "netcdf_meta.h"
#if NC_HAS_PARALLEL
#ifndef PARALLEL_AWARE_EXODUS
#define PARALLEL_AWARE_EXODUS
#endif
#endif
#endif

#if defined(PARALLEL_AWARE_EXODUS)
#include "netcdf_par.h"
#endif
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef NC_INT64
#error "NetCDF version 4.1.2 or later is required."
#endif

/* EXODUS version number */
#define EX_API_VERS 7.13f
#define EX_API_VERS_NODOT 713
#define EX_VERS EX_API_VERS
#define NEMESIS_API_VERSION EX_API_VERS
#define NEMESIS_API_VERSION_NODOT EX_API_VERS_NODOT
#define NEMESIS_FILE_VERSION 2.6
/*
 * need following extern if this include file is used in a C++
 * program, to keep the C++ compiler from mangling the function names.
 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * The following are miscellaneous constants used in the EXODUS
 * API. They should already be defined, but are left over from the
 * old days...
 */
#ifndef EX_TRUE
#define EX_TRUE -1
#endif

#ifndef EX_FALSE
#define EX_FALSE 0
#endif

/**
 * \defgroup FileVars Variables controlling the file creation mode.
 *@{
 */
/* Modes for ex_open */
#define EX_WRITE 0x0001 /**< ex_open(): open existing file for appending. */
#define EX_READ 0x0002  /**< ex_open(): open file for reading (default) */

#define EX_NOCLOBBER 0x0004            /**< Don't overwrite existing database, default */
#define EX_CLOBBER 0x0008              /**< Overwrite existing database if it exists */
#define EX_NORMAL_MODEL 0x0010         /**< disable mods that permit storage of larger models */
#define EX_64BIT_OFFSET 0x0020         /**< enable mods that permit storage of larger models */
#define EX_LARGE_MODEL EX_64BIT_OFFSET /**< enable mods that permit storage of larger models */
#define EX_64BIT_DATA 0x400000 /**< CDF-5 format: classic model but 64 bit dimensions and sizes */
#define EX_NETCDF4 0x0040      /**< use the hdf5-based netcdf4 output */
#define EX_NOSHARE 0x0080      /**< Do not open netcdf file in "share" mode */
#define EX_SHARE 0x0100        /**< Do open netcdf file in "share" mode */
#define EX_NOCLASSIC 0x0200    /**< Do not force netcdf to classic mode in netcdf4 mode */

#define EX_DISKLESS 0x100000 /**< Experimental */
#define EX_MMAP 0x200000     /**< Experimental */

/* Need to distinguish between storage on database (DB in name) and
   passed through the API functions (API in name).
*/
#define EX_MAPS_INT64_DB 0x0400 /**< All maps (id, order, ...) store int64_t values */
#define EX_IDS_INT64_DB 0x0800  /**< All entity ids (sets, blocks, maps) are int64_t values */
#define EX_BULK_INT64_DB                                                                           \
  0x1000 /**< All integer bulk data (local indices, counts, maps); not ids                         \
          */
#define EX_ALL_INT64_DB                                                                            \
  (EX_MAPS_INT64_DB | EX_IDS_INT64_DB | EX_BULK_INT64_DB) /**< All of the above... */

#define EX_MAPS_INT64_API 0x2000 /**< All maps (id, order, ...) store int64_t values */
#define EX_IDS_INT64_API 0x4000  /**< All entity ids (sets, blocks, maps) are int64_t values */
#define EX_BULK_INT64_API                                                                          \
  0x8000 /**< All integer bulk data (local indices, counts, maps); not ids */
#define EX_INQ_INT64_API 0x10000 /**< Integers passed to/from ex_inquire are int64_t */
#define EX_ALL_INT64_API                                                                           \
  (EX_MAPS_INT64_API | EX_IDS_INT64_API | EX_BULK_INT64_API |                                      \
   EX_INQ_INT64_API) /**< All of the above... */

/* Parallel IO mode flags... */
#define EX_MPIIO 0x20000
#define EX_MPIPOSIX 0x40000 /**< \deprecated As of libhdf5 1.8.13. */
#define EX_PNETCDF 0x80000

/*@}*/

/*! \sa ex_inquire() */
enum ex_inquiry {
  EX_INQ_FILE_TYPE    = 1,  /**< inquire EXODUS file type*/
  EX_INQ_API_VERS     = 2,  /**< inquire API version number */
  EX_INQ_DB_VERS      = 3,  /**< inquire database version number */
  EX_INQ_TITLE        = 4,  /**< inquire database title     */
  EX_INQ_DIM          = 5,  /**< inquire number of dimensions */
  EX_INQ_NODES        = 6,  /**< inquire number of nodes    */
  EX_INQ_ELEM         = 7,  /**< inquire number of elements */
  EX_INQ_ELEM_BLK     = 8,  /**< inquire number of element blocks */
  EX_INQ_NODE_SETS    = 9,  /**< inquire number of node sets*/
  EX_INQ_NS_NODE_LEN  = 10, /**< inquire length of node set node list */
  EX_INQ_SIDE_SETS    = 11, /**< inquire number of side sets*/
  EX_INQ_SS_NODE_LEN  = 12, /**< inquire length of side set node list */
  EX_INQ_SS_ELEM_LEN  = 13, /**< inquire length of side set element list */
  EX_INQ_QA           = 14, /**< inquire number of QA records */
  EX_INQ_INFO         = 15, /**< inquire number of info records */
  EX_INQ_TIME         = 16, /**< inquire number of time steps in the database */
  EX_INQ_EB_PROP      = 17, /**< inquire number of element block properties */
  EX_INQ_NS_PROP      = 18, /**< inquire number of node set properties */
  EX_INQ_SS_PROP      = 19, /**< inquire number of side set properties */
  EX_INQ_NS_DF_LEN    = 20, /**< inquire length of node set distribution factor list*/
  EX_INQ_SS_DF_LEN    = 21, /**< inquire length of side set distribution factor list*/
  EX_INQ_LIB_VERS     = 22, /**< inquire API Lib vers number*/
  EX_INQ_EM_PROP      = 23, /**< inquire number of element map properties */
  EX_INQ_NM_PROP      = 24, /**< inquire number of node map properties */
  EX_INQ_ELEM_MAP     = 25, /**< inquire number of element maps */
  EX_INQ_NODE_MAP     = 26, /**< inquire number of node maps*/
  EX_INQ_EDGE         = 27, /**< inquire number of edges    */
  EX_INQ_EDGE_BLK     = 28, /**< inquire number of edge blocks */
  EX_INQ_EDGE_SETS    = 29, /**< inquire number of edge sets   */
  EX_INQ_ES_LEN       = 30, /**< inquire length of concat edge set edge list       */
  EX_INQ_ES_DF_LEN    = 31, /**< inquire length of concat edge set dist factor list*/
  EX_INQ_EDGE_PROP    = 32, /**< inquire number of properties stored per edge block    */
  EX_INQ_ES_PROP      = 33, /**< inquire number of properties stored per edge set      */
  EX_INQ_FACE         = 34, /**< inquire number of faces */
  EX_INQ_FACE_BLK     = 35, /**< inquire number of face blocks */
  EX_INQ_FACE_SETS    = 36, /**< inquire number of face sets */
  EX_INQ_FS_LEN       = 37, /**< inquire length of concat face set face list */
  EX_INQ_FS_DF_LEN    = 38, /**< inquire length of concat face set dist factor list*/
  EX_INQ_FACE_PROP    = 39, /**< inquire number of properties stored per face block */
  EX_INQ_FS_PROP      = 40, /**< inquire number of properties stored per face set */
  EX_INQ_ELEM_SETS    = 41, /**< inquire number of element sets */
  EX_INQ_ELS_LEN      = 42, /**< inquire length of concat element set element list       */
  EX_INQ_ELS_DF_LEN   = 43, /**< inquire length of concat element set dist factor list*/
  EX_INQ_ELS_PROP     = 44, /**< inquire number of properties stored per elem set      */
  EX_INQ_EDGE_MAP     = 45, /**< inquire number of edge maps                     */
  EX_INQ_FACE_MAP     = 46, /**< inquire number of face maps                     */
  EX_INQ_COORD_FRAMES = 47, /**< inquire number of coordinate frames */
  EX_INQ_DB_MAX_ALLOWED_NAME_LENGTH =
      48,                              /**< inquire size of MAX_NAME_LENGTH dimension on database */
  EX_INQ_DB_MAX_USED_NAME_LENGTH = 49, /**< inquire size of MAX_NAME_LENGTH dimension on database */
  EX_INQ_MAX_READ_NAME_LENGTH    = 50, /**< inquire client-specified max size of returned names */

  EX_INQ_DB_FLOAT_SIZE    = 51, /**< inquire size of floating-point values stored on database */
  EX_INQ_NUM_CHILD_GROUPS = 52, /**< inquire number of groups contained in this (exoid) group */
  EX_INQ_GROUP_PARENT =
      53, /**< inquire id of parent of this (exoid) group; returns exoid if at root */
  EX_INQ_GROUP_ROOT =
      54, /**< inquire id of root group "/" of this (exoid) group; returns exoid if at root */
  EX_INQ_GROUP_NAME_LEN      = 55, /**< inquire length of name of group exoid */
  EX_INQ_GROUP_NAME          = 56, /**< inquire name of group exoid. "/" returned for root group */
  EX_INQ_FULL_GROUP_NAME_LEN = 57, /**< inquire length of full path name of this (exoid) group */
  EX_INQ_FULL_GROUP_NAME = 58, /**< inquire full "/"-separated path name of this (exoid) group */
  EX_INQ_THREADSAFE      = 59, /**< Returns 1 if library is thread-safe; 0 otherwise */
  EX_INQ_INVALID         = -1
};

typedef enum ex_inquiry ex_inquiry;

/* Options */
/**
 * \defgroup FileOptions Variables controlling the compression, name size, and integer size.
 *@{
 */
/*! Modes for ex_set_option()

The compression-related options are only available on netcdf-4 files
since the underlying hdf5 compression functionality is used for the
implementation. The compression level indicates how much effort should
be expended in the compression and the computational expense increases
with higher levels; in many cases, a compression level of 1 is
sufficient.

 */
enum ex_option_type {
  EX_OPT_MAX_NAME_LENGTH =
      1, /**< Maximum length of names that will be returned/passed via api call. */
  EX_OPT_COMPRESSION_TYPE,    /**<  Not currently used; default is gzip	*/
  EX_OPT_COMPRESSION_LEVEL,   /**<  In the range [0..9]. A value of 0 indicates no compression */
  EX_OPT_COMPRESSION_SHUFFLE, /**<  1 if enabled, 0 if disabled	*/
  EX_OPT_INTEGER_SIZE_API, /**<  4 or 8 indicating byte size of integers used in api functions. */
  EX_OPT_INTEGER_SIZE_DB /**<  Query only, returns 4 or 8 indicating byte size of integers stored on
                            the database. */
};
typedef enum ex_option_type ex_option_type;
/*@}*/

enum ex_entity_type {
  EX_NODAL      = 14, /**< nodal "block" for variables*/
  EX_NODE_BLOCK = 14, /**< alias for EX_NODAL         */
  EX_NODE_SET   = 2,  /**< node set property code     */
  EX_EDGE_BLOCK = 6,  /**< edge block property code   */
  EX_EDGE_SET   = 7,  /**< edge set property code     */
  EX_FACE_BLOCK = 8,  /**< face block property code   */
  EX_FACE_SET   = 9,  /**< face set property code     */
  EX_ELEM_BLOCK = 1,  /**< element block property code*/
  EX_ELEM_SET   = 10, /**< face set property code     */

  EX_SIDE_SET = 3, /**< side set property code     */

  EX_ELEM_MAP = 4,  /**< element map property code  */
  EX_NODE_MAP = 5,  /**< node map property code     */
  EX_EDGE_MAP = 11, /**< edge map property code     */
  EX_FACE_MAP = 12, /**< face map property code     */

  EX_GLOBAL     = 13, /**< global "block" for variables*/
  EX_COORDINATE = 15, /**< kluge so some internal wrapper functions work */
  EX_INVALID    = -1
};
typedef enum ex_entity_type ex_entity_type;

/**
 * ex_opts() function codes - codes are OR'ed into exopts
 */
enum ex_options {
  EX_DEFAULT     = 0,
  EX_VERBOSE     = 1, /**< verbose mode message flag   */
  EX_DEBUG       = 2, /**< debug mode def             */
  EX_ABORT       = 4, /**< abort mode flag def        */
  EX_NULLVERBOSE = 8  /**< verbose mode for null entity detection warning */
};
typedef enum ex_options ex_options;

/** The value used to indicate that an entity (block, nset, sset)
    has not had its id set to a valid value
*/
#define EX_INVALID_ID -1

/**
 * \defgroup StringLengths maximum string lengths;
 * constants that are used as netcdf dimensions must be of type long
 * @{
 */
/** Maximum length of QA record, element type name */
#define MAX_STR_LENGTH 32L
/** Default maximum length of an entity name, attribute name, variable name.
    Can be changed via a call to ex_set_option() */
#define MAX_NAME_LENGTH MAX_STR_LENGTH

/** Maximum length of the database title or an information record */
#define MAX_LINE_LENGTH 80L
/** Maximum length of an error message passed to ex_err() function. Typically, internal use only */
#define MAX_ERR_LENGTH 256
/* @} */

/** Specifies that this argument is the id of an entity: element block, nodeset, sideset, ... */
typedef int64_t ex_entity_id;

/** The mechanism for passing double/float and int/int64_t both use a
   void*; to avoid some confusion as to whether a function takes an
   integer or a float/double, the following typedef is used for the
   integer argument
*/
typedef void void_int;

/**
 * \defgroup APIStructs Structures used by external API functions.
 * @{
 */
typedef struct ex_init_params
{
  char    title[MAX_LINE_LENGTH + 1];
  int64_t num_dim;
  int64_t num_nodes;
  int64_t num_edge;
  int64_t num_edge_blk;
  int64_t num_face;
  int64_t num_face_blk;
  int64_t num_elem;
  int64_t num_elem_blk;
  int64_t num_node_sets;
  int64_t num_edge_sets;
  int64_t num_face_sets;
  int64_t num_side_sets;
  int64_t num_elem_sets;
  int64_t num_node_maps;
  int64_t num_edge_maps;
  int64_t num_face_maps;
  int64_t num_elem_maps;
} ex_init_params;

typedef struct ex_block
{
  int64_t        id;
  ex_entity_type type;
  char           topology[MAX_STR_LENGTH + 1];
  int64_t        num_entry;
  int64_t        num_nodes_per_entry;
  int64_t        num_edges_per_entry;
  int64_t        num_faces_per_entry;
  int64_t        num_attribute;
} ex_block;

typedef struct ex_set
{
  int64_t        id;
  ex_entity_type type;
  int64_t        num_entry;
  int64_t        num_distribution_factor;
  void_int *     entry_list;
  void_int *     extra_list;
  void *         distribution_factor_list;
} ex_set;

typedef struct ex_block_params
{
  void_int *edge_blk_id;
  char **   edge_type;
  int *     num_edge_this_blk;
  int *     num_nodes_per_edge;
  int *     num_attr_edge;
  void_int *face_blk_id;
  char **   face_type;
  int *     num_face_this_blk;
  int *     num_nodes_per_face;
  int *     num_attr_face;
  void_int *elem_blk_id;
  char **   elem_type;
  int *     num_elem_this_blk;
  int *     num_nodes_per_elem;
  int *     num_edges_per_elem;
  int *     num_faces_per_elem;
  int *     num_attr_elem;
  int       define_maps;
} ex_block_params;

typedef struct ex_set_specs
{
  void_int *sets_ids;
  void_int *num_entries_per_set;
  void_int *num_dist_per_set;
  void_int *sets_entry_index;
  void_int *sets_dist_index;
  void_int *sets_entry_list;
  void_int *sets_extra_list;
  void *    sets_dist_fact;
} ex_set_specs;

typedef struct ex_var_params
{
  int  num_glob;
  int  num_node;
  int  num_edge;
  int  num_face;
  int  num_elem;
  int  num_nset;
  int  num_eset;
  int  num_fset;
  int  num_sset;
  int  num_elset;
  int *edge_var_tab;
  int *face_var_tab;
  int *elem_var_tab;
  int *nset_var_tab;
  int *eset_var_tab;
  int *fset_var_tab;
  int *sset_var_tab;
  int *elset_var_tab;
} ex_var_params;
/* @} */

#ifndef EXODUS_EXPORT
#define EXODUS_EXPORT extern
#endif /* EXODUS_EXPORT */

/* routines for file initialization i/o */
EXODUS_EXPORT int ex_close(int exoid);

EXODUS_EXPORT int ex_copy(int in_exoid, int out_exoid);
EXODUS_EXPORT int ex_copy_transient(int in_exoid, int out_exoid);

#define ex_create(path, mode, comp_ws, io_ws)                                                      \
  ex_create_int(path, mode, comp_ws, io_ws, EX_API_VERS_NODOT)

EXODUS_EXPORT int ex_create_int(const char *path, int cmode, int *comp_ws, int *io_ws,
                                int run_version);

#if defined(PARALLEL_AWARE_EXODUS)
#define ex_create_par(path, mode, comp_ws, io_ws, comm, info)                                      \
  ex_create_par_int(path, mode, comp_ws, io_ws, comm, info, EX_API_VERS_NODOT)

EXODUS_EXPORT int ex_create_par_int(const char *path, int cmode, int *comp_ws, int *io_ws,
                                    MPI_Comm comm, MPI_Info info, int my_version);
#endif

EXODUS_EXPORT int ex_create_group(int parent_id, const char *group_name);

EXODUS_EXPORT int ex_get_group_id(int parent_id, const char *group_name, int *group_id);

EXODUS_EXPORT int ex_get_group_ids(int parent_id, int *num_groups, int *group_ids);

EXODUS_EXPORT int ex_get_all_times(int exoid, void *time_values);

EXODUS_EXPORT int ex_get_coord_names(int exoid, char **coord_names);

EXODUS_EXPORT int ex_get_coord(int exoid, void *x_coor, void *y_coor, void *z_coor);

EXODUS_EXPORT int ex_get_partial_coord_component(int exoid, int64_t start_node_num,
                                                 int64_t num_nodes, int component, void *coor);

EXODUS_EXPORT int ex_get_partial_coord(int exoid, int64_t start_node_num, int64_t num_nodes,
                                       void *x_coor, void *y_coor, void *z_coor);

EXODUS_EXPORT int ex_get_ids(int exoid, ex_entity_type obj_type, void_int *ids);

EXODUS_EXPORT int ex_get_coordinate_frames(int exoid, int *nframes, void_int *cf_ids,
                                           void *pt_coordinates, char *tags);

EXODUS_EXPORT int ex_get_info(int exoid, char **info);

EXODUS_EXPORT int ex_put_init_ext(int exoid, const ex_init_params *model);

EXODUS_EXPORT int ex_get_init_ext(int exoid, ex_init_params *info);

EXODUS_EXPORT int ex_get_init(int exoid, char *title, void_int *num_dim, void_int *num_nodes,
                              void_int *num_elem, void_int *num_elem_blk, void_int *num_node_sets,
                              void_int *num_side_sets);

EXODUS_EXPORT int ex_put_init(int exoid, const char *title, int64_t num_dim, int64_t num_nodes,
                              int64_t num_elem, int64_t num_elem_blk, int64_t num_node_sets,
                              int64_t num_side_sets);

EXODUS_EXPORT int ex_get_map(int exoid, void_int *elem_map);

EXODUS_EXPORT int ex_get_map_param(int exoid, int *num_node_maps, int *num_elem_maps);

EXODUS_EXPORT int ex_get_name(int exoid, ex_entity_type obj_type, ex_entity_id entity_id,
                              char *name);

EXODUS_EXPORT int ex_get_names(int exoid, ex_entity_type obj_type, char **names);

EXODUS_EXPORT int ex_get_prop_array(int exoid, ex_entity_type obj_type, const char *prop_name,
                                    void_int *values);

EXODUS_EXPORT int ex_get_prop(int exoid, ex_entity_type obj_type, ex_entity_id obj_id,
                              const char *prop_name, void_int *value);

EXODUS_EXPORT int ex_get_partial_num_map(int exoid, ex_entity_type map_type, ex_entity_id map_id,
                                         int64_t ent_start, int64_t ent_count, void_int *map);

EXODUS_EXPORT int ex_get_prop_names(int exoid, ex_entity_type obj_type, char **prop_names);

EXODUS_EXPORT int ex_get_qa(int exoid, char *qa_record[][4]);

EXODUS_EXPORT int ex_get_time(int exoid, int time_step, void *time_value);

EXODUS_EXPORT int ex_get_variable_names(int exoid, ex_entity_type obj_type, int num_vars,
                                        char *var_names[]);
EXODUS_EXPORT int ex_get_variable_name(int exoid, ex_entity_type obj_type, int var_num,
                                       char *var_name);

EXODUS_EXPORT int ex_get_variable_param(int exoid, ex_entity_type obj_type, int *num_vars);

EXODUS_EXPORT int ex_get_object_truth_vector(int exoid, ex_entity_type obj_type,
                                             ex_entity_id entity_id, int num_var, int *var_vec);

EXODUS_EXPORT int ex_get_truth_table(int exoid, ex_entity_type obj_type, int num_blk, int num_var,
                                     int *var_tab);

#if defined(PARALLEL_AWARE_EXODUS)
#define ex_open_par(path, mode, comp_ws, io_ws, version, comm, info)                               \
  ex_open_par_int(path, mode, comp_ws, io_ws, version, comm, info, EX_API_VERS_NODOT)

EXODUS_EXPORT int ex_open_par_int(const char *path, int mode, int *comp_ws, int *io_ws,
                                  float *version, MPI_Comm comm, MPI_Info info, int my_version);
#endif

#define ex_open(path, mode, comp_ws, io_ws, version)                                               \
  ex_open_int(path, mode, comp_ws, io_ws, version, EX_API_VERS_NODOT)

EXODUS_EXPORT int ex_open_int(const char *path, int mode, int *comp_ws, int *io_ws, float *version,
                              int run_version);

EXODUS_EXPORT int ex_add_attr(int exoid, ex_entity_type obj_type, ex_entity_id obj_id,
                              int64_t num_attr_per_entry);

EXODUS_EXPORT int ex_put_attr_param(int exoid, ex_entity_type obj_type, ex_entity_id obj_id,
                                    int num_attrs);

EXODUS_EXPORT int ex_get_attr_param(int exoid, ex_entity_type obj_type, ex_entity_id obj_id,
                                    int *num_attrs);

EXODUS_EXPORT int ex_put_all_var_param(int exoid, int num_g, int num_n, int num_e,
                                       int *elem_var_tab, int num_m, int *nset_var_tab, int num_s,
                                       int *sset_var_tab);

EXODUS_EXPORT int ex_put_concat_elem_block(int exoid, const void_int *elem_blk_id,
                                           char *elem_type[], const void_int *num_elem_this_blk,
                                           const void_int *num_nodes_per_elem,
                                           const void_int *num_attr_this_blk, int define_maps);

EXODUS_EXPORT int ex_put_coord_names(int exoid, char *coord_names[]);

EXODUS_EXPORT int ex_put_coord(int exoid, const void *x_coor, const void *y_coor,
                               const void *z_coor);

EXODUS_EXPORT int ex_put_partial_coord_component(int exoid, int64_t start_node_num,
                                                 int64_t num_nodes, int component,
                                                 const void *coor);

EXODUS_EXPORT int ex_put_partial_coord(int exoid, int64_t start_node_num, int64_t num_nodes,
                                       const void *x_coor, const void *y_coor, const void *z_coor);

EXODUS_EXPORT int ex_put_map(int exoid, const void_int *elem_map);

EXODUS_EXPORT int ex_put_id_map(int exoid, ex_entity_type map_type, const void_int *map);

EXODUS_EXPORT int ex_put_partial_id_map(int exoid, ex_entity_type map_type,
                                        int64_t start_entity_num, int64_t num_entities,
                                        const void_int *map);

EXODUS_EXPORT int ex_get_id_map(int exoid, ex_entity_type map_type, void_int *map);

EXODUS_EXPORT int ex_get_partial_id_map(int exoid, ex_entity_type map_type,
                                        int64_t start_entity_num, int64_t num_entities,
                                        void_int *map);

EXODUS_EXPORT int ex_put_coordinate_frames(int exoid, int nframes, const void_int *cf_ids,
                                           void *pt_coordinates, const char *tags);

EXODUS_EXPORT int ex_put_info(int exoid, int num_info, char *info[]);

EXODUS_EXPORT int ex_put_map_param(int exoid, int num_node_maps, int num_elem_maps);

EXODUS_EXPORT int ex_put_name(int exoid, ex_entity_type obj_type, ex_entity_id entity_id,
                              const char *name);

EXODUS_EXPORT int ex_put_names(int exoid, ex_entity_type obj_type, char *names[]);

EXODUS_EXPORT int ex_put_partial_one_attr(int exoid, ex_entity_type obj_type, ex_entity_id obj_id,
                                          int64_t start_num, int64_t num_ent, int attrib_index,
                                          const void *attrib);

EXODUS_EXPORT int ex_put_prop(int exoid, ex_entity_type obj_type, ex_entity_id obj_id,
                              const char *prop_name, ex_entity_id value);

EXODUS_EXPORT int ex_put_prop_array(int exoid, ex_entity_type obj_type, const char *prop_name,
                                    const void_int *values);

EXODUS_EXPORT int ex_put_prop_names(int exoid, ex_entity_type obj_type, int num_props,
                                    char **prop_names);

EXODUS_EXPORT int ex_put_qa(int exoid, int num_qa_records, char *qa_record[][4]);

EXODUS_EXPORT int ex_put_time(int exoid, int time_step, const void *time_value);

EXODUS_EXPORT int ex_put_variable_name(int exoid, ex_entity_type obj_type, int var_num,
                                       const char *var_name);

EXODUS_EXPORT int ex_put_variable_names(int exoid, ex_entity_type obj_type, int num_vars,
                                        char *var_names[]);

EXODUS_EXPORT int ex_put_variable_param(int exoid, ex_entity_type obj_type, int num_vars);

EXODUS_EXPORT int ex_put_truth_table(int exoid, ex_entity_type obj_type, int num_blk, int num_var,
                                     int *var_tab);

EXODUS_EXPORT int ex_update(int exoid);
EXODUS_EXPORT int ex_get_num_props(int exoid, ex_entity_type obj_type);
EXODUS_EXPORT int ex_large_model(int exoid);
EXODUS_EXPORT size_t ex_header_size(int exoid);

EXODUS_EXPORT void        ex_err(const char *module_name, const char *message, int err_num);
EXODUS_EXPORT void        ex_set_err(const char *module_name, const char *message, int err_num);
EXODUS_EXPORT const char *ex_strerror(int err_num);
EXODUS_EXPORT void        ex_get_err(const char **msg, const char **func, int *err_num);
EXODUS_EXPORT int         ex_opts(int options);
EXODUS_EXPORT int ex_inquire(int exoid, int req_info, void_int * /*ret_int*/, float * /*ret_float*/,
                             char * /*ret_char*/);
EXODUS_EXPORT int64_t ex_inquire_int(int exoid, int req_info);
EXODUS_EXPORT int     ex_int64_status(int exoid);
EXODUS_EXPORT int     ex_set_int64_status(int exoid, int mode);

/** Note that the max name length setting is global at this time; not specific
 * to a particular database; however, the exoid option is passed to give
 * flexibility in the future to implement this on a database-by-database basis.
 */
EXODUS_EXPORT int ex_set_max_name_length(int exoid, int length);

EXODUS_EXPORT int ex_set_option(int exoid, ex_option_type option, int option_value);

/*  Write Node Edge Face or Element Number Map */
EXODUS_EXPORT int ex_put_num_map(int exoid, ex_entity_type map_type, ex_entity_id map_id,
                                 const void_int *map);

/*  Read Number Map */
EXODUS_EXPORT int ex_get_num_map(int exoid, ex_entity_type map_type, ex_entity_id map_id,
                                 void_int *map);

/*  Write Edge Face or Element Block Parameters */
EXODUS_EXPORT int ex_put_block(int exoid, ex_entity_type blk_type, ex_entity_id blk_id,
                               const char *entry_descrip, int64_t num_entries_this_blk,
                               int64_t num_nodes_per_entry, int64_t num_edges_per_entry,
                               int64_t num_faces_per_entry, int64_t num_attr_per_entry);

EXODUS_EXPORT int ex_get_block(int exoid, ex_entity_type blk_type, ex_entity_id blk_id,
                               char *elem_type, void_int *num_entries_this_blk,
                               void_int *num_nodes_per_entry, void_int *num_edges_per_entry,
                               void_int *num_faces_per_entry, void_int *num_attr_per_entry);

/*  Read Edge Face or Element Block Parameters */
EXODUS_EXPORT int ex_get_block_param(int exoid, ex_block *block);

EXODUS_EXPORT int ex_put_block_param(int exoid, ex_block block);

EXODUS_EXPORT int ex_get_block_params(int exoid, size_t block_count, struct ex_block **blocks);

EXODUS_EXPORT int ex_put_block_params(int exoid, size_t block_count, const struct ex_block *blocks);

/*  Write All Edge Face and Element Block Parameters */
EXODUS_EXPORT int ex_put_concat_all_blocks(int exoid, const ex_block_params *param);

EXODUS_EXPORT int ex_put_entity_count_per_polyhedra(int exoid, ex_entity_type blk_type,
                                                    ex_entity_id blk_id, const int *entity_counts);

EXODUS_EXPORT int ex_get_entity_count_per_polyhedra(int exoid, ex_entity_type blk_type,
                                                    ex_entity_id blk_id, int *entity_counts);

/*  Write Edge Face or Element Block Connectivity */
EXODUS_EXPORT int ex_put_conn(int exoid, ex_entity_type blk_type, ex_entity_id blk_id,
                              const void_int *node_conn, const void_int *elem_edge_conn,
                              const void_int *elem_face_conn);

/*  Read Edge Face or Element Block Connectivity */
EXODUS_EXPORT int ex_get_conn(int exoid, ex_entity_type blk_type, ex_entity_id blk_id,
                              void_int *nodeconn, void_int *edgeconn, void_int *faceconn);

EXODUS_EXPORT int ex_get_partial_conn(int exoid, ex_entity_type blk_type, ex_entity_id blk_id,
                                      int64_t start_num, int64_t num_ent, void_int *nodeconn,
                                      void_int *edgeconn, void_int *faceconn);

EXODUS_EXPORT int ex_put_partial_conn(int exoid, ex_entity_type blk_type, ex_entity_id blk_id,
                                      int64_t start_num, int64_t num_ent, const void_int *nodeconn,
                                      const void_int *edgeconn, const void_int *faceconn);

/*  Write Edge Face or Element Block Attributes */
EXODUS_EXPORT int ex_put_attr(int exoid, ex_entity_type blk_type, ex_entity_id blk_id,
                              const void *attrib);

EXODUS_EXPORT int ex_put_partial_attr(int exoid, ex_entity_type blk_type, ex_entity_id blk_id,
                                      int64_t start_entity, int64_t num_entity, const void *attrib);

/*  Read Edge Face or Element Block Attributes */
EXODUS_EXPORT int ex_get_attr(int exoid, ex_entity_type obj_type, ex_entity_id obj_id,
                              void *attrib);

EXODUS_EXPORT int ex_get_partial_attr(int exoid, ex_entity_type obj_type, ex_entity_id obj_id,
                                      int64_t start_num, int64_t num_ent, void *attrib);

/*  Write One Edge Face or Element Block Attribute */
EXODUS_EXPORT int ex_put_one_attr(int exoid, ex_entity_type obj_type, ex_entity_id obj_id,
                                  int attrib_index, const void *attrib);

/*  Read One Edge Face or Element Block Attribute */
EXODUS_EXPORT int ex_get_one_attr(int exoid, ex_entity_type obj_type, ex_entity_id obj_id,
                                  int attrib_index, void *attrib);

EXODUS_EXPORT int ex_get_partial_one_attr(int exoid, ex_entity_type obj_type, ex_entity_id obj_id,
                                          int64_t start_num, int64_t num_ent, int attrib_index,
                                          void *attrib);

/*  Write Edge Face or Element Block Attribute Names */
EXODUS_EXPORT int ex_put_attr_names(int exoid, ex_entity_type blk_type, ex_entity_id blk_id,
                                    char **names);

/*  Read Edge Face or Element Block Attribute Names */
EXODUS_EXPORT int ex_get_attr_names(int exoid, ex_entity_type obj_type, ex_entity_id obj_id,
                                    char **names);

/*  Write Node Edge Face or Side Set Parameters */
EXODUS_EXPORT int ex_put_set_param(int exoid, ex_entity_type set_type, ex_entity_id set_id,
                                   int64_t num_entries_in_set, int64_t num_dist_fact_in_set);

/*  Read Node Edge Face or Side Set Parameters */
EXODUS_EXPORT int ex_get_set_param(int exoid, ex_entity_type set_type, ex_entity_id set_id,
                                   void_int *num_entry_in_set, void_int *num_dist_fact_in_set);

/*  Write a Node Edge Face or Side Set */
EXODUS_EXPORT int ex_put_set(int exoid, ex_entity_type set_type, ex_entity_id set_id,
                             const void_int *set_entry_list, const void_int *set_extra_list);

EXODUS_EXPORT int ex_get_partial_set(int exoid, ex_entity_type set_type, ex_entity_id set_id,
                                     int64_t offset, int64_t num_to_get, void_int *set_entry_list,
                                     void_int *set_extra_list);

EXODUS_EXPORT int ex_put_partial_set(int exoid, ex_entity_type set_type, ex_entity_id set_id,
                                     int64_t offset, int64_t num_to_put,
                                     const void_int *set_entry_list,
                                     const void_int *set_extra_list);

/*  Read a Node Edge Face or Side Set */
EXODUS_EXPORT int ex_get_set(int exoid, ex_entity_type set_type, ex_entity_id set_id,
                             void_int *set_entry_list, void_int *set_extra_list);

/*  Write Node Edge Face or Side Set Distribution Factors */
EXODUS_EXPORT int ex_put_set_dist_fact(int exoid, ex_entity_type set_type, ex_entity_id set_id,
                                       const void *set_dist_fact);

/*  Read Node Edge Face or Side Set Distribution Factors */
EXODUS_EXPORT int ex_get_set_dist_fact(int exoid, ex_entity_type set_type, ex_entity_id set_id,
                                       void *set_dist_fact);

EXODUS_EXPORT int ex_get_partial_set_dist_fact(int exoid, ex_entity_type set_type,
                                               ex_entity_id set_id, int64_t offset,
                                               int64_t num_to_put, void *set_dist_fact);

/*  Write Concatenated Node Edge Face or Side Sets */
EXODUS_EXPORT int ex_put_concat_sets(int exoid, ex_entity_type set_type,
                                     const struct ex_set_specs *set_specs);

/*  Read Concatenated Node Edge Face or Side Sets */
EXODUS_EXPORT int ex_get_concat_sets(int exoid, ex_entity_type set_type,
                                     struct ex_set_specs *set_specs);

/*  Write Concatenated Node Edge Face or Side Sets */
EXODUS_EXPORT int ex_put_sets(int exoid, size_t set_count, const struct ex_set *sets);

/*  Read Concatenated Node Edge Face or Side Sets */
EXODUS_EXPORT int ex_get_sets(int exoid, size_t set_count, struct ex_set *sets);

/*  (MODIFIED) Write All Results Variables Parameters */
EXODUS_EXPORT int ex_put_all_var_param_ext(int exoid, const ex_var_params *vp);

/*  Write Edge Face or Element Variable Values on Blocks or Sets at a Time Step */
EXODUS_EXPORT int ex_put_var(int exoid, int time_step, ex_entity_type var_type, int var_index,
                             ex_entity_id obj_id, int64_t num_entries_this_obj,
                             const void *var_vals);

/*  Write Partial Edge Face or Element Variable Values on Blocks or Sets at a Time Step */
EXODUS_EXPORT int ex_put_partial_var(int exoid, int time_step, ex_entity_type var_type,
                                     int var_index, ex_entity_id obj_id, int64_t start_index,
                                     int64_t num_entities, const void *var_vals);

/*  Read Edge Face or Element Variable Values Defined On Blocks or Sets at a Time Step */
EXODUS_EXPORT int ex_get_var(int exoid, int time_step, ex_entity_type var_type, int var_index,
                             ex_entity_id obj_id, int64_t num_entry_this_obj, void *var_vals);

EXODUS_EXPORT int ex_get_partial_var(int exoid, int time_step, ex_entity_type var_type,
                                     int var_index, ex_entity_id obj_id, int64_t start_index,
                                     int64_t num_entities, void *var_vals);

/*  Read Edge Face or Element Variable Values Defined On Blocks or Sets Through Time */
EXODUS_EXPORT int ex_get_var_time(int exoid, ex_entity_type var_type, int var_index, int64_t id,
                                  int beg_time_step, int end_time_step, void *var_vals);

EXODUS_EXPORT int ex_cvt_nodes_to_sides(int exoid, void_int *num_elem_per_set,
                                        void_int *num_nodes_per_set, void_int *side_sets_elem_index,
                                        void_int *side_sets_node_index,
                                        void_int *side_sets_elem_list,
                                        void_int *side_sets_node_list,
                                        void_int *side_sets_side_list);

EXODUS_EXPORT int ex_put_partial_num_map(int exoid, ex_entity_type map_type, ex_entity_id map_id,
                                         int64_t ent_start, int64_t ent_count, const void_int *map);

EXODUS_EXPORT int ex_put_partial_set_dist_fact(int exoid, ex_entity_type set_type,
                                               ex_entity_id set_id, int64_t offset,
                                               int64_t num_to_put, const void *set_dist_fact);

/* TODO */
EXODUS_EXPORT int ex_get_concat_side_set_node_count(int exoid, int *side_set_node_cnt_list);

/* TODO */
EXODUS_EXPORT int ex_get_side_set_node_list_len(int exoid, ex_entity_id side_set_id,
                                                void_int *side_set_node_list_len);

/* TODO */
EXODUS_EXPORT int ex_get_side_set_node_count(int exoid, ex_entity_id side_set_id,
                                             int *side_set_node_cnt_list);

/* TODO */
EXODUS_EXPORT int ex_get_side_set_node_list(int exoid, ex_entity_id side_set_id,
                                            void_int *side_set_node_cnt_list,
                                            void_int *side_set_node_list);

/* ========================================================================
   Functions pulled from nemesis library and incorporated into exodus...
*/
/*=============================================================================
 *     Initial Information Routines
 *===========================================================================*/
EXODUS_EXPORT int ex_get_init_info(int   exoid,         /* NemesisI file ID */
                                   int * num_proc,      /* Number of processors */
                                   int * num_proc_in_f, /* Number of procs in this file */
                                   char *ftype);

EXODUS_EXPORT int ex_put_init_info(int   exoid,         /* NemesisI file ID */
                                   int   num_proc,      /* Number of processors */
                                   int   num_proc_in_f, /* Number of procs in this file */
                                   char *ftype);

EXODUS_EXPORT int ex_get_init_global(int       exoid,           /* NemesisI file ID */
                                     void_int *num_nodes_g,     /* Number of global FEM nodes */
                                     void_int *num_elems_g,     /* Number of global FEM elements */
                                     void_int *num_elem_blks_g, /* Number of global elem blocks */
                                     void_int *num_node_sets_g, /* Number of global node sets */
                                     void_int *num_side_sets_g  /* Number of global side sets */
);
EXODUS_EXPORT int ex_put_init_global(int     exoid,           /* NemesisI file ID */
                                     int64_t num_nodes_g,     /* Number of global FEM nodes */
                                     int64_t num_elems_g,     /* Number of global FEM elements */
                                     int64_t num_elem_blks_g, /* Number of global elem blocks */
                                     int64_t num_node_sets_g, /* Number of global node sets */
                                     int64_t num_side_sets_g  /* Number of global side sets */
);

/*=============================================================================
 *     Loadbalance Parameter Routines
 *===========================================================================*/
EXODUS_EXPORT int ex_get_loadbal_param(int       exoid,          /* NetCDF/Exodus file ID */
                                       void_int *num_int_nodes,  /* Number of internal FEM nodes */
                                       void_int *num_bor_nodes,  /* Number of border FEM nodes */
                                       void_int *num_ext_nodes,  /* Number of external FEM nodes */
                                       void_int *num_int_elems,  /* Number of internal FEM elems */
                                       void_int *num_bor_elems,  /* Number of border FEM elems */
                                       void_int *num_node_cmaps, /* Number of nodal comm maps */
                                       void_int *num_elem_cmaps, /* Number of elemental comm maps */
                                       int       processor       /* Processor ID */
);

EXODUS_EXPORT int ex_put_loadbal_param(int     exoid,          /* NemesisI file ID  */
                                       int64_t num_int_nodes,  /* Number of internal FEM nodes */
                                       int64_t num_bor_nodes,  /* Number of border FEM nodes */
                                       int64_t num_ext_nodes,  /* Number of external FEM nodes */
                                       int64_t num_int_elems,  /* Number of internal FEM elems */
                                       int64_t num_bor_elems,  /* Number of border FEM elems */
                                       int64_t num_node_cmaps, /* Number of nodal comm maps */
                                       int64_t num_elem_cmaps, /* Number of elemental comm maps */
                                       int     processor       /* Processor ID */
);

EXODUS_EXPORT int ex_put_loadbal_param_cc(int       exoid,         /* NetCDF/Exodus file ID */
                                          void_int *num_int_nodes, /* Number of internal node IDs */
                                          void_int *num_bor_nodes, /* Number of border node IDs */
                                          void_int *num_ext_nodes, /* Number of external node IDs */
                                          void_int *num_int_elems, /* Number of internal elem IDs */
                                          void_int *num_bor_elems, /* Number of border elem IDs */
                                          void_int *num_node_cmaps, /* Number of nodal comm maps */
                                          void_int *num_elem_cmaps  /* Number of elem comm maps */
);

/*=============================================================================
 *     NS, SS & EB Global Parameter Routines
 *===========================================================================*/
EXODUS_EXPORT int ex_get_ns_param_global(int       exoid,      /* NetCDF/Exodus file ID */
                                         void_int *global_ids, /* Global IDs of node sets */
                                         void_int *node_cnts,  /* Count of nodes in node sets */
                                         void_int *df_cnts     /* Count of dist. factors in ns */
);

EXODUS_EXPORT int
ex_put_ns_param_global(int       exoid,      /* NemesisI file ID */
                       void_int *global_ids, /* Vector of global node-set IDs */
                       void_int *node_cnts,  /* Vector of node counts in node-sets */
                       void_int *df_cnts     /* Vector of dist factor counts in node-sets */
);

EXODUS_EXPORT int ex_get_ss_param_global(int       exoid,      /* NetCDF/Exodus file ID */
                                         void_int *global_ids, /* Global side-set IDs */
                                         void_int *side_cnts,  /* Global side count */
                                         void_int *df_cnts     /* Global dist. factor count */
);

EXODUS_EXPORT int ex_put_ss_param_global(int       exoid,      /* NemesisI file ID */
                                         void_int *global_ids, /* Vector of global side-set IDs */
                                         void_int *side_cnts,  /* Vector of element/side */
                                                               /* counts in each side set */
                                         void_int *df_cnts     /* Vector of dist. factor */
                                                               /* counts in each side set */
);

EXODUS_EXPORT int ex_get_eb_info_global(int       exoid,      /* NemesisI file ID                 */
                                        void_int *el_blk_ids, /* Vector of global element IDs     */
                                        void_int *el_blk_cnts /* Vector of global element counts  */
);

EXODUS_EXPORT int ex_put_eb_info_global(int       exoid,      /* NemesisI file ID */
                                        void_int *el_blk_ids, /* Vector of global element IDs     */
                                        void_int *el_blk_cnts /* Vector of global element counts  */
);

/*=============================================================================
 *     NS, SS & EB Subset Routines
 *===========================================================================*/
EXODUS_EXPORT int ex_get_elem_type(int          exoid,       /* NetCDF/Exodus file ID */
                                   ex_entity_id elem_blk_id, /* Element block ID */
                                   char *       elem_type    /* The name of the element type */
);

/*=============================================================================
 *     Number Map Routines
 *===========================================================================*/
EXODUS_EXPORT int ex_get_processor_node_maps(int       exoid,     /* NetCDF/Exodus file ID */
                                             void_int *node_mapi, /* Internal FEM node IDs */
                                             void_int *node_mapb, /* Border FEM node IDs */
                                             void_int *node_mape, /* External FEM node IDs */
                                             int       processor  /* Processor IDs */
);

EXODUS_EXPORT int ex_put_processor_node_maps(int       exoid,     /* NetCDF/Exodus file ID */
                                             void_int *node_mapi, /* Internal FEM node IDs */
                                             void_int *node_mapb, /* Border FEM node IDs */
                                             void_int *node_mape, /* External FEM node IDs */
                                             int       proc_id    /* This processor ID */
);

EXODUS_EXPORT int ex_get_processor_elem_maps(int       exoid,     /* NetCDF/Exodus file ID */
                                             void_int *elem_mapi, /* Internal element IDs */
                                             void_int *elem_mapb, /* Border element IDs */
                                             int       processor  /* Processor ID */
);

EXODUS_EXPORT int ex_put_processor_elem_maps(int       exoid,     /* NetCDF/Exodus file ID */
                                             void_int *elem_mapi, /* Internal FEM element IDs */
                                             void_int *elem_mapb, /* Border FEM element IDs */
                                             int       processor  /* This processor ID */
);

/*=============================================================================
 *     Communications Maps Routines
 *===========================================================================*/

EXODUS_EXPORT int
ex_get_cmap_params(int       exoid,               /* NetCDF/Exodus file ID */
                   void_int *node_cmap_ids,       /* Nodal comm. map IDs */
                   void_int *node_cmap_node_cnts, /* Number of nodes in each map */
                   void_int *elem_cmap_ids,       /* Elemental comm. map IDs */
                   void_int *elem_cmap_elem_cnts, /* Number of elems in each map */
                   int       processor            /* This processor ID */
);

EXODUS_EXPORT int ex_put_cmap_params(int       exoid,               /* NetCDF/Exodus file ID */
                                     void_int *node_cmap_ids,       /* Node map IDs */
                                     void_int *node_cmap_node_cnts, /* Nodes in nodal comm */
                                     void_int *elem_cmap_ids,       /* Elem map IDs */
                                     void_int *elem_cmap_elem_cnts, /* Elems in elemental comm */
                                     int64_t   processor            /* This processor ID */
);

EXODUS_EXPORT int ex_put_cmap_params_cc(int       exoid,               /* NetCDF/Exodus file ID */
                                        void_int *node_cmap_ids,       /* Node map IDs */
                                        void_int *node_cmap_node_cnts, /* Nodes in nodal comm */
                                        void_int *node_proc_ptrs,      /* Pointer into array for */
                                                                       /* node maps		  */
                                        void_int *elem_cmap_ids,       /* Elem map IDs */
                                        void_int *elem_cmap_elem_cnts, /* Elems in elemental comm */
                                        void_int *elem_proc_ptrs       /* Pointer into array for */
                                                                       /* elem maps		  */
);

EXODUS_EXPORT int ex_get_node_cmap(int          exoid,    /* NetCDF/Exodus file ID */
                                   ex_entity_id map_id,   /* Map ID */
                                   void_int *   node_ids, /* FEM node IDs */
                                   void_int *   proc_ids, /* Processor IDs */
                                   int          processor /* This processor ID */
);

EXODUS_EXPORT int ex_put_node_cmap(int          exoid,    /* NetCDF/Exodus file ID */
                                   ex_entity_id map_id,   /* Nodal comm map ID */
                                   void_int *   node_ids, /* FEM node IDs */
                                   void_int *   proc_ids, /* Processor IDs */
                                   int          processor /* This processor ID */
);

EXODUS_EXPORT int ex_get_elem_cmap(int          exoid,    /* NetCDF/Exodus file ID */
                                   ex_entity_id map_id,   /* Elemental comm map ID */
                                   void_int *   elem_ids, /* Element IDs */
                                   void_int *   side_ids, /* Element side IDs */
                                   void_int *   proc_ids, /* Processor IDs */
                                   int          processor /* This processor ID */
);

EXODUS_EXPORT int ex_put_elem_cmap(int          exoid,    /* NetCDF/Exodus file ID */
                                   ex_entity_id map_id,   /* Elemental comm map ID */
                                   void_int *   elem_ids, /* Vector of element IDs */
                                   void_int *   side_ids, /* Vector of side IDs */
                                   void_int *   proc_ids, /* Vector of processor IDs */
                                   int          processor /* This processor ID */
);

/* Deprecated Code Handling Options:
 * 1. Ignore -- treat deprecated functions as normal non-deprecated functions (default)
 * 2. Delete -- the deprecated functions are not defined or compiled (SEACAS_HIDE_DEPRECATED_CODE is
 * defined)
 * 3. Warn   -- if used in client code, issue a warning. (SEACAS_WARN_DEPRECATED_CODE is defined)
 *
 * The symbols SEACAS_HIDE_DEPRECATED_CODE and SEACAS_DEPRECATED are defined in exodus_config.h
 * In a TriBITs-based system, this include file is generated from cmake-variable definitions.
 * In other build systems, the exodus_config.h file is hard-wired.
 */

#if !defined(SEACAS_HIDE_DEPRECATED_CODE)
/* ========================================================================
 * Deprecated functiona
 */

#ifndef SEACAS_DEPRECATED
#define SEACAS_DEPRECATED
#endif

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_glob_vars(int exoid, int time_step, int num_glob_vars,
                                                     void *glob_var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_glob_var_time(int exoid, int glob_var_index,
                                                         int beg_time_step, int end_time_step,
                                                         void *glob_var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_concat_node_sets(int exoid, void_int *node_set_ids, void_int *num_nodes_per_set,
                                          void_int *num_df_per_set, void_int *node_sets_node_index,
                                          void_int *node_sets_df_index, void_int *node_sets_node_list,
                                          void *node_sets_dist_fact);

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_concat_side_sets(int exoid, void_int *side_set_ids, void_int *num_elem_per_set,
                                          void_int *num_dist_per_set, void_int *side_sets_elem_index,
                                          void_int *side_sets_dist_index, void_int *side_sets_elem_list,
                                          void_int *side_sets_side_list, void *side_sets_dist_fact);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_elem_attr(int exoid, ex_entity_id elem_blk_id,
                                                     void *attrib);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_elem_attr_names(int exoid, ex_entity_id elem_blk_id,
                                                           char **names);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_elem_blk_ids(int exoid, void_int *ids);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_elem_block(int exoid, ex_entity_id elem_blk_id,
                                                      char *elem_type, void_int *num_elem_this_blk,
                                                      void_int *num_nodes_per_elem,
                                                      void_int *num_attr);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_elem_conn(int exoid, ex_entity_id elem_blk_id,
                                                     void_int *connect);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_elem_map(int exoid, ex_entity_id map_id,
                                                    void_int *elem_map);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_elem_num_map(int exoid, void_int *elem_map);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_nodal_var(int exoid, int time_step, int nodal_var_index,
                                                     int64_t num_nodes, void *nodal_var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_nodal_var(int exoid, int time_step, int nodal_var_index,
                                                     int64_t num_nodes, const void *nodal_var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_nodal_var_time(int exoid, int nodal_var_index,
                                                          int64_t node_number, int beg_time_step,
                                                          int end_time_step, void *nodal_var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_partial_nodal_var(int exoid, int time_step,
                                                             int     nodal_var_index,
                                                             int64_t start_node, int64_t num_nodes,
                                                             const void *nodal_var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_partial_nodal_var(int exoid, int time_step,
                                                             int     nodal_var_index,
                                                             int64_t start_node, int64_t num_nodes,
                                                             void *var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_elem_var(int exoid, int time_step, int elem_var_index,
                                                    ex_entity_id elem_blk_id,
                                                    int64_t num_elem_this_blk, void *elem_var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_elem_var_tab(int exoid, int num_elem_blk,
                                                        int num_elem_var, int *elem_var_tab);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_elem_var_time(int exoid, int elem_var_index,
                                                         int64_t elem_number, int beg_time_step,
                                                         int end_time_step, void *elem_var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_node_map(int exoid, ex_entity_id map_id,
                                                    void_int *node_map);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_node_num_map(int exoid, void_int *node_map);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_node_set_param(int exoid, ex_entity_id node_set_id,
                                                          void_int *num_nodes_in_set,
                                                          void_int *num_df_in_set);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_node_set(int exoid, ex_entity_id node_set_id,
                                                    void_int *node_set_node_list);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_node_set_dist_fact(int exoid, ex_entity_id node_set_id,
                                                              void *node_set_dist_fact);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_node_set_ids(int exoid, void_int *ids);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_nset_var_tab(int exoid, int num_nodesets,
                                                        int num_nset_var, int *nset_var_tab);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_nset_var(int exoid, int time_step, int nset_var_index,
                                                    ex_entity_id nset_id,
                                                    int64_t      num_node_this_nset,
                                                    void *       nset_var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_one_elem_attr(int exoid, ex_entity_id elem_blk_id,
                                                         int attrib_index, void *attrib);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_side_set(int exoid, ex_entity_id side_set_id,
                                                    void_int *side_set_elem_list,
                                                    void_int *side_set_side_list);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_side_set_dist_fact(int exoid, ex_entity_id side_set_id,
                                                              void *side_set_dist_fact);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_side_set_ids(int exoid, void_int *ids);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_side_set_param(int exoid, ex_entity_id side_set_id,
                                                          void_int *num_side_in_set,
                                                          void_int *num_dist_fact_in_set);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_sset_var(int exoid, int time_step, int sset_var_index,
                                                    ex_entity_id sset_id,
                                                    int64_t      num_side_this_sset,
                                                    void *       sset_var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_sset_var_tab(int exoid, int num_sidesets,
                                                        int num_sset_var, int *sset_var_tab);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_var_names(int exoid, const char *var_type, int num_vars,
                                                     char *var_names[]);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_var_name(int exoid, const char *var_type, int var_num,
                                                    char *var_name);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_var_param(int exoid, const char *var_type,
                                                     int *num_vars);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_var_tab(int exoid, const char *var_type, int num_blk,
                                                   int num_var, int *var_tab);

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_concat_node_sets(int exoid, void_int *node_set_ids, void_int *num_nodes_per_set,
                                          void_int *num_dist_per_set, void_int *node_sets_node_index,
                                          void_int *node_sets_df_index, void_int *node_sets_node_list,
                                          void *node_sets_dist_fact);

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_concat_side_sets(int exoid, void_int *side_set_ids, void_int *num_elem_per_set,
                                          void_int *num_dist_per_set, void_int *side_sets_elem_index,
                                          void_int *side_sets_dist_index, void_int *side_sets_elem_list,
                                          void_int *side_sets_side_list, void *side_sets_dist_fact);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_concat_var_param(int exoid, int num_g, int num_n,
                                                            int num_e, int num_elem_blk,
                                                            int *elem_var_tab);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_elem_attr_names(int exoid, ex_entity_id elem_blk_id,
                                                           char *names[]);
SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_elem_attr(int exoid, ex_entity_id elem_blk_id,
                                                     const void *attrib);

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_elem_block(int exoid, ex_entity_id elem_blk_id, const char *elem_type,
                                    int64_t num_elem_this_blk, int64_t num_nodes_per_elem, int64_t num_attr_per_elem);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_elem_conn(int exoid, ex_entity_id elem_blk_id,
                                                     const void_int *connect);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_elem_map(int exoid, ex_entity_id map_id,
                                                    const void_int *elem_map);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_elem_num_map(int exoid, const void_int *elem_map);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_elem_var(int exoid, int time_step, int elem_var_index,
                                                    ex_entity_id elem_blk_id,
                                                    int64_t      num_elem_this_blk,
                                                    const void * elem_var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_elem_var_tab(int exoid, int num_elem_blk,
                                                        int num_elem_var, int *elem_var_tab);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_glob_vars(int exoid, int time_step, int num_glob_vars,
                                                     const void *glob_var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_node_map(int exoid, ex_entity_id map_id,
                                                    const void_int *node_map);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_node_num_map(int exoid, const void_int *node_map);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_node_set(int exoid, ex_entity_id node_set_id,
                                                    const void_int *node_set_node_list);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_node_set_dist_fact(int exoid, ex_entity_id node_set_id,
                                                              const void *node_set_dist_fact);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_node_set_param(int exoid, ex_entity_id node_set_id,
                                                          int64_t num_nodes_in_set,
                                                          int64_t num_dist_in_set);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_nset_var(int exoid, int time_step, int nset_var_index,
                                                    ex_entity_id nset_id,
                                                    int64_t      num_nodes_this_nset,
                                                    const void * nset_var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_nset_var_tab(int exoid, int num_nset, int num_nset_var,
                                                        int *nset_var_tab);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_one_elem_attr(int exoid, ex_entity_id elem_blk_id,
                                                         int attrib_index, const void *attrib);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_side_set(int exoid, ex_entity_id side_set_id,
                                                    const void_int *side_set_elem_list,
                                                    const void_int *side_set_side_list);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_side_set_dist_fact(int exoid, ex_entity_id side_set_id,
                                                              const void *side_set_dist_fact);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_side_set_param(int exoid, ex_entity_id side_set_id,
                                                          int64_t num_side_in_set,
                                                          int64_t num_dist_fact_in_set);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_sset_var(int exoid, int time_step, int sset_var_index,
                                                    ex_entity_id sset_id,
                                                    int64_t      num_faces_this_sset,
                                                    const void * sset_var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_sset_var_tab(int exoid, int num_sset, int num_sset_var,
                                                        int *sset_var_tab);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_var_name(int exoid, const char *var_type, int var_num,
                                                    const char *var_name);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_var_names(int exoid, const char *var_type, int num_vars,
                                                     char *var_names[]);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_var_param(int exoid, const char *var_type, int num_vars);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_var_tab(int exoid, const char *var_type, int num_blk,
                                                   int num_var, int *var_tab);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_n_coord(int exoid, int64_t start_node_num,
                                                   int64_t num_nodes, void *x_coor, void *y_coor,
                                                   void *z_coor);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_n_nodal_var(int exoid, int time_step,
                                                       int nodal_var_index, int64_t start_node,
                                                       int64_t num_nodes, void *var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_n_conn(int exoid, ex_entity_type blk_type,
                                                  ex_entity_id blk_id, int64_t start_num,
                                                  int64_t num_ent, void_int *nodeconn,
                                                  void_int *edgeconn, void_int *faceconn);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_n_attr(int exoid, ex_entity_type obj_type,
                                                  ex_entity_id obj_id, int64_t start_num,
                                                  int64_t num_ent, void *attrib);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_n_one_attr(int exoid, ex_entity_type obj_type,
                                                      ex_entity_id obj_id, int64_t start_num,
                                                      int64_t num_ent, int attrib_index,
                                                      void *attrib);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_n_var(int exoid, int time_step, ex_entity_type var_type,
                                                 int var_index, ex_entity_id obj_id,
                                                 int64_t start_index, int64_t num_entities,
                                                 void *var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_n_elem_var(int exoid, int time_step, int elem_var_index,
                                                      ex_entity_id elem_blk_id,
                                                      int64_t      num_elem_this_blk,
                                                      int64_t start_elem_num, int64_t num_elem,
                                                      void *elem_var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_n_side_set(int          exoid,              /* NetCDF/Exodus file ID */
                                    ex_entity_id side_set_id,        /* Side-set ID to read */
                                    int64_t      start_side_num,     /* Starting element number */
                                    int64_t      num_sides,          /* Number of sides to read */
                                    void_int *   side_set_elem_list, /* List of element IDs */
                                    void_int *   side_set_side_list  /* List of side IDs */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_n_side_set(int             exoid,          /* NetCDF/Exodus file ID */
                                    ex_entity_id    side_set_id,    /* Side-set ID to write */
                                    int64_t         start_side_num, /* Starting element number */
                                    int64_t         num_sides,      /* Number of sides to write */
                                    const void_int *side_set_elem_list, /* List of element IDs */
                                    const void_int *side_set_side_list  /* List of side IDs */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_n_side_set_df(int          exoid,             /* NetCDF/Exodus file ID */
                                       ex_entity_id side_set_id,       /* Side-set ID */
                                       int64_t      start_num,         /* Starting df number */
                                       int64_t      num_df_to_get,     /* Number of df's to read */
                                       void *       side_set_dist_fact /* Distribution factors */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_n_side_set_df(int          exoid,             /* NetCDF/Exodus file ID */
                                       ex_entity_id side_set_id,       /* Side-set ID */
                                       int64_t      start_num,         /* Starting df number */
                                       int64_t      num_df_to_get,     /* Number of df's to write */
                                       void *       side_set_dist_fact /* Distribution factors */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_n_node_set(int          exoid,          /* NetCDF/Exodus file ID */
                                    ex_entity_id node_set_id,    /* Node set ID */
                                    int64_t      start_node_num, /* Node index to start reading at */
                                    int64_t      num_nodes,      /* Number of nodes to read */
                                    void_int *   node_set_node_list /* List of nodes in node set */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_n_node_set(int             exoid,       /* NetCDF/Exodus file ID */
                                    ex_entity_id    node_set_id, /* Node set ID */
                                    int64_t         start_node_num, /* Node index to start writing at */
                                    int64_t         num_nodes,      /* Number of nodes to write */
                                    const void_int *node_set_node_list /* List of nodes in node set */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_n_node_set_df(int          exoid,             /* NetCDF/Exodus file ID */
                                       ex_entity_id node_set_id,       /* Node-set ID */
                                       int64_t      start_num,         /* Starting df number */
                                       int64_t      num_df_to_get,     /* Number of df's to read */
                                       void *       node_set_dist_fact /* Distribution factors */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_n_node_set_df(int          exoid,             /* NetCDF/Exodus file ID */
                                       ex_entity_id node_set_id,       /* Node-set ID */
                                       int64_t      start_num,         /* Starting df number */
                                       int64_t      num_df_to_get,     /* Number of df's to write */
                                       void *       node_set_dist_fact /* Distribution factors */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_n_elem_conn(int          exoid,          /* NetCDF/Exodus file ID */
                                     ex_entity_id elem_blk_id,    /* Element block ID */
                                     int64_t      start_elem_num, /* Starting position to read from */
                                     int64_t      num_elems,      /* Number of elements to read */
                                     void_int *   connect         /* Connectivity vector */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_n_elem_conn(int             exoid,       /* NetCDF/Exodus file ID */
                                     ex_entity_id    elem_blk_id, /* Element block ID */
                                     int64_t         start_elem_num, /* Starting position to write to */
                                     int64_t         num_elems, /* Number of elements to write */
                                     const void_int *connect    /* Connectivity vector */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_n_elem_attr(int          exoid,          /* NetCDF/Exodus file ID */
                                     ex_entity_id elem_blk_id,    /* Element block ID */
                                     int64_t      start_elem_num, /* Starting position to read from */
                                     int64_t      num_elems,      /* Number of elements to read */
                                     void *       attrib          /* Attribute */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_n_elem_attr(int          exoid,          /* NetCDF/Exodus file ID */
                                     ex_entity_id elem_blk_id,    /* Element block ID */
                                     int64_t      start_elem_num, /* Starting position to write to */
                                     int64_t      num_elems,      /* Number of elements to write */
                                     void *       attrib          /* Attribute */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_n_elem_num_map(int       exoid,     /* NetCDF/Exodus file ID */
                                        int64_t   start_ent, /* Starting position to read from */
                                        int64_t   num_ents,  /* Number of elements to read */
                                        void_int *elem_map   /* element map numbers */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_n_node_num_map(int       exoid,     /* NetCDF/Exodus file ID */
                                        int64_t   start_ent, /* starting node number */
                                        int64_t   num_ents,  /* number of nodes to read */
                                        void_int *node_map   /* vector for node map */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_n_coord(int exoid, int64_t start_node_num,
                                                   int64_t num_nodes, const void *x_coor,
                                                   const void *y_coor, const void *z_coor);

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_n_elem_num_map(int exoid, int64_t start_ent, int64_t num_ents, const void_int *elem_map);

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_n_node_num_map(int exoid, int64_t start_ent, int64_t num_ents, const void_int *node_map);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_n_one_attr(int exoid, ex_entity_type obj_type,
                                                      ex_entity_id obj_id, int64_t start_num,
                                                      int64_t num_ent, int attrib_index,
                                                      const void *attrib);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_n_var(int exoid, int time_step, ex_entity_type var_type,
                                                 int var_index, ex_entity_id obj_id,
                                                 int64_t start_index, int64_t num_entities,
                                                 const void *var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_n_nodal_var(int exoid, int time_step,
                                                       int nodal_var_index, int64_t start_node,
                                                       int64_t     num_nodes,
                                                       const void *nodal_var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_partial_elem_var(int exoid, int time_step, int elem_var_index, ex_entity_id elem_blk_id,
                                          int64_t num_elem_this_blk, int64_t start_elem_num, int64_t num_elem,
                                          void *elem_var_vals);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_get_partial_elem_map(int exoid, ex_entity_id map_id,
                                                            int64_t ent_start, int64_t ent_count,
                                                            void_int *elem_map);

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_partial_elem_conn(int          exoid,       /* NetCDF/Exodus file ID */
                                           ex_entity_id elem_blk_id, /* Element block ID */
                                           int64_t      start_elem_num, /* Starting position to read from */
                                           int64_t      num_elems, /* Number of elements to read */
                                           void_int *   connect    /* Connectivity vector */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_partial_elem_attr(int          exoid,       /* NetCDF/Exodus file ID */
                                           ex_entity_id elem_blk_id, /* Element block ID */
                                           int64_t      start_elem_num, /* Starting position to read from */
                                           int64_t      num_elems, /* Number of elements to read */
                                           void *       attrib     /* Attribute */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_partial_elem_num_map(int       exoid,     /* NetCDF/Exodus file ID */
                                              int64_t   start_ent, /* Starting position to read from */
                                              int64_t   num_ents, /* Number of elements to read */
                                              void_int *elem_map  /* element map numbers */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_partial_node_num_map(int       exoid,     /* NetCDF/Exodus file ID */
                                              int64_t   start_ent, /* starting node number */
                                              int64_t   num_ents,  /* number of nodes to read */
                                              void_int *node_map   /* vector for node map */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_partial_node_set(int          exoid,       /* NetCDF/Exodus file ID */
                                          ex_entity_id node_set_id, /* Node set ID */
                                          int64_t      start_node_num, /* Node index to start reading at */
                                          int64_t      num_nodes, /* Number of nodes to read */
                                          void_int *   node_set_node_list /* List of nodes in node set */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_partial_node_set_df(int          exoid,         /* NetCDF/Exodus file ID */
                                             ex_entity_id node_set_id,   /* Node-set ID */
                                             int64_t      start_num,     /* Starting df number */
                                             int64_t      num_df_to_get, /* Number of df's to read */
                                             void *       node_set_dist_fact /* Distribution factors */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_partial_side_set(int          exoid,          /* NetCDF/Exodus file ID */
                                          ex_entity_id side_set_id,    /* Side-set ID to read */
                                          int64_t      start_side_num, /* Starting element number */
                                          int64_t      num_sides,      /* Number of sides to read */
                                          void_int *   side_set_elem_list, /* List of element IDs */
                                          void_int *   side_set_side_list  /* List of side IDs */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_get_partial_side_set_df(int          exoid,         /* NetCDF/Exodus file ID */
                                             ex_entity_id side_set_id,   /* Side-set ID */
                                             int64_t      start_num,     /* Starting df number */
                                             int64_t      num_df_to_get, /* Number of df's to read */
                                             void *       side_set_dist_fact /* Distribution factors */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_partial_node_num_map(int exoid, int64_t start_ent,
                                                                int64_t         num_ents,
                                                                const void_int *node_map);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_partial_elem_num_map(int exoid, int64_t start_ent,
                                                                int64_t         num_ents,
                                                                const void_int *elem_map);

SEACAS_DEPRECATED EXODUS_EXPORT int ex_put_partial_elem_map(int exoid, ex_entity_id map_id,
                                                            int64_t ent_start, int64_t ent_count,
                                                            const void_int *elem_map);

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_partial_side_set(int             exoid,       /* NetCDF/Exodus file ID */
                                          ex_entity_id    side_set_id, /* Side-set ID to write */
                                          int64_t         start_side_num, /* Starting element number */
                                          int64_t         num_sides, /* Number of sides to write */
                                          const void_int *side_set_elem_list, /* List of element IDs */
                                          const void_int *side_set_side_list /* List of side IDs */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_partial_side_set_df(int          exoid,         /* NetCDF/Exodus file ID */
                                             ex_entity_id side_set_id,   /* Side-set ID */
                                             int64_t      start_num,     /* Starting df number */
                                             int64_t      num_df_to_get, /* Number of df's to write */
                                             void *       side_set_dist_fact /* Distribution factors */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_partial_node_set(int             exoid,       /* NetCDF/Exodus file ID */
                                          ex_entity_id    node_set_id, /* Node set ID */
                                          int64_t         start_node_num, /* Node index to start writing at */
                                          int64_t         num_nodes, /* Number of nodes to write */
                                          const void_int *node_set_node_list /* List of nodes in node set */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_partial_node_set_df(int          exoid,         /* NetCDF/Exodus file ID */
                                             ex_entity_id node_set_id,   /* Node-set ID */
                                             int64_t      start_num,     /* Starting df number */
                                             int64_t      num_df_to_get, /* Number of df's to write */
                                             void *       node_set_dist_fact /* Distribution factors */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_partial_elem_conn(int             exoid,       /* NetCDF/Exodus file ID */
                                           ex_entity_id    elem_blk_id, /* Element block ID */
                                           int64_t         start_elem_num, /* Starting position to write to */
                                           int64_t         num_elems, /* Number of elements to write */
                                           const void_int *connect    /* Connectivity vector */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_partial_elem_attr(int          exoid,       /* NetCDF/Exodus file ID */
                                           ex_entity_id elem_blk_id, /* Element block ID */
                                           int64_t      start_elem_num, /* Starting position to write to */
                                           int64_t      num_elems, /* Number of elements to write */
                                           void *       attrib     /* Attribute */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_elem_var_slab(int          exoid,          /* NetCDF/Exodus file ID */
                                       int          time_step,      /* time index */
                                       int          elem_var_index, /* elemental variable index */
                                       ex_entity_id elem_blk_id,    /* elemental block id */
                                       int64_t      start_pos, /* Starting position to write to */
                                       int64_t      num_vals,  /* Number of elements to write */
                                       void *       elem_var_vals /* variable values */
                  );

SEACAS_DEPRECATED EXODUS_EXPORT int
                  ex_put_nodal_var_slab(int     exoid,           /* NetCDF/Exodus file ID */
                                        int     time_step,       /* The time step index */
                                        int     nodal_var_index, /* Nodal variable index */
                                        int64_t start_pos,       /* Start position for write */
                                        int64_t num_vals,        /* Number of nodal variables */
                                        void *  nodal_var_vals   /* Nodal variable values */
                  );

#endif
/* End of Deprecated functions and their replacements
 * ======================================================================== */

/* ERROR CODE DEFINITIONS AND STORAGE                                       */
EXODUS_EXPORT int exoptval; /**< error reporting flag (default is quiet)  */
#if defined(EXODUS_THREADSAFE)
#if !defined(exerrval)
/* In both exodusII.h and exodusII_int.h */
typedef struct EX_errval
{
  int  errval;
  char last_pname[MAX_ERR_LENGTH];
  char last_errmsg[MAX_ERR_LENGTH];
  int  last_err_num;
} EX_errval_t;

EXODUS_EXPORT EX_errval_t *ex_errval;
#define exerrval ex_errval->errval
#endif
#else
EXODUS_EXPORT int exerrval; /**< shared error return value                */
#endif

EXODUS_EXPORT char *ex_name_of_object(ex_entity_type obj_type);
EXODUS_EXPORT ex_entity_type ex_var_type_to_ex_entity_type(char var_type);

/* Should be internal use only, but was in external include file for
   nemesis and some codes are using the function
*/
EXODUS_EXPORT int ex_get_idx(int         exoid,       /* NetCDF/Exodus file ID */
                             const char *ne_var_name, /* Nemesis index variable name */
                             int64_t *   my_index,    /* array of length 2 to hold results */
                             int         pos          /* position of this proc/cmap in index */
);

/**
 * \defgroup ErrorReturnCodes Exodus error return codes - exerrval return values
 * @{
 */
#define EX_MEMFAIL 1000       /**< memory allocation failure flag def       */
#define EX_BADFILEMODE 1001   /**< bad file mode def                        */
#define EX_BADFILEID 1002     /**< bad file id def                          */
#define EX_WRONGFILETYPE 1003 /**< wrong file type for function             */
#define EX_LOOKUPFAIL 1004    /**< id table lookup failed                   */
#define EX_BADPARAM 1005      /**< bad parameter passed                     */
#define EX_INTERNAL 1006      /**< internal logic error                     */
#define EX_MSG -1000          /**< message print code - no error implied    */
#define EX_PRTLASTMSG -1001   /**< print last error message msg code        */
#define EX_NOTROOTID -1002    /**< file id is not the root id; it is a subgroup id */
#define EX_LASTERR -1003      /**< in ex_err, use existing err_num value */
#define EX_NULLENTITY -1006   /**< null entity found                        */
#define EX_DUPLICATEID -1007  /**< duplicate id found                        */
/* @} */

/* Exodus error return codes - function return values:                      */
#define EX_FATAL -1 /* fatal error flag def                     */
#define EX_NOERR 0  /* no error flag def                        */
#define EX_WARN 1   /* warning flag def                         */

#ifdef __cplusplus
} /* close brackets on extern "C" declaration */
#endif

#endif
