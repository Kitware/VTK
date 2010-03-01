/*
 * Copyright (c) 2005 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Governement
 * retains certain rights in this software.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.  
 * 
 *     * Neither the name of Sandia Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

/*****************************************************************************
 *
 * exodusII.h - Exodus II API include file
 *
 *****************************************************************************/

#ifndef EXODUS_II_HDR
#define EXODUS_II_HDR

#include "vtk_netcdf.h"
#include "vtk_exodus2_mangle.h"
#include "exodusII_cfg.h"
/* #include "stddef.h" */


/* EXODUS II version number */
#define EX_API_VERS 4.93
#define EX_API_VERS_NODOT 493
#define EX_VERS EX_API_VERS


/*
 * need following extern if this include file is used in a C++
 * program, to keep the C++ compiler from mangling the function names.
 */
#ifdef __cplusplus
extern "C" {
#endif

  /*
   * The following are miscellaneous constants used in the EXODUS II
   * API. They should already be defined, but are left over from the
   * old days...
   */
#ifndef TRUE
#define TRUE -1
#endif

#ifndef FALSE
#define FALSE 0 
#endif

  /**
   * \defgroup FileVars Variables controlling the file creation mode.
   *@{
   */
#define EX_NOCLOBBER            0 /**< Don't overwrite existing database, default */
#define EX_CLOBBER              1 /**< Overwrite existing database if it exists */
#define EX_NORMAL_MODEL         2 /**< disable mods that permit storage of larger models */
#define EX_LARGE_MODEL          4 /**< enable mods that permit storage of larger models */
#define EX_NETCDF4              8 /**< use the hdf5-based netcdf4 output */
#define EX_NOSHARE             16 /**< Do not open netcdf file in "share" mode */
#define EX_SHARE               32 /**< Do open netcdf file in "share" mode */

#define EX_READ                 0
#define EX_WRITE                1
  /*@}*/
  
  /*! \sa ex_inquire() */
  enum ex_inquiry {
    EX_INQ_FILE_TYPE       =  1,     /**< inquire EXODUS II file type*/
    EX_INQ_API_VERS        =  2,     /**< inquire API version number */
    EX_INQ_DB_VERS         =  3,     /**< inquire database version number */
    EX_INQ_TITLE           =  4,     /**< inquire database title     */
    EX_INQ_DIM             =  5,     /**< inquire number of dimensions */
    EX_INQ_NODES           =  6,     /**< inquire number of nodes    */
    EX_INQ_ELEM            =  7,     /**< inquire number of elements */
    EX_INQ_ELEM_BLK        =  8,     /**< inquire number of element blocks */
    EX_INQ_NODE_SETS       =  9,     /**< inquire number of node sets*/
    EX_INQ_NS_NODE_LEN     = 10,     /**< inquire length of node set node list */
    EX_INQ_SIDE_SETS       = 11,     /**< inquire number of side sets*/
    EX_INQ_SS_NODE_LEN     = 12,     /**< inquire length of side set node list */
    EX_INQ_SS_ELEM_LEN     = 13,     /**< inquire length of side set element list */
    EX_INQ_QA              = 14,     /**< inquire number of QA records */
    EX_INQ_INFO            = 15,     /**< inquire number of info records */
    EX_INQ_TIME            = 16,     /**< inquire number of time steps in the database */
    EX_INQ_EB_PROP         = 17,     /**< inquire number of element block properties */
    EX_INQ_NS_PROP         = 18,     /**< inquire number of node set properties */
    EX_INQ_SS_PROP         = 19,     /**< inquire number of side set properties */
    EX_INQ_NS_DF_LEN       = 20,     /**< inquire length of node set distribution factor list*/
    EX_INQ_SS_DF_LEN       = 21,     /**< inquire length of side set distribution factor list*/
    EX_INQ_LIB_VERS        = 22,     /**< inquire API Lib vers number*/
    EX_INQ_EM_PROP         = 23,     /**< inquire number of element map properties */
    EX_INQ_NM_PROP         = 24,     /**< inquire number of node map properties */
    EX_INQ_ELEM_MAP        = 25,     /**< inquire number of element maps */
    EX_INQ_NODE_MAP        = 26,     /**< inquire number of node maps*/
    EX_INQ_EDGE            = 27,     /**< inquire number of edges    */
    EX_INQ_EDGE_BLK        = 28,     /**< inquire number of edge blocks */
    EX_INQ_EDGE_SETS       = 29,     /**< inquire number of edge sets   */
    EX_INQ_ES_LEN          = 30,     /**< inquire length of concat edge set edge list       */
    EX_INQ_ES_DF_LEN       = 31,     /**< inquire length of concat edge set dist factor list*/
    EX_INQ_EDGE_PROP       = 32,     /**< inquire number of properties stored per edge block    */
    EX_INQ_ES_PROP         = 33,     /**< inquire number of properties stored per edge set      */
    EX_INQ_FACE            = 34,     /**< inquire number of faces */
    EX_INQ_FACE_BLK        = 35,     /**< inquire number of face blocks */
    EX_INQ_FACE_SETS       = 36,     /**< inquire number of face sets */
    EX_INQ_FS_LEN          = 37,     /**< inquire length of concat face set face list */
    EX_INQ_FS_DF_LEN       = 38,     /**< inquire length of concat face set dist factor list*/
    EX_INQ_FACE_PROP       = 39,     /**< inquire number of properties stored per face block */
    EX_INQ_FS_PROP         = 40,     /**< inquire number of properties stored per face set */
    EX_INQ_ELEM_SETS       = 41,     /**< inquire number of element sets */
    EX_INQ_ELS_LEN         = 42,     /**< inquire length of concat element set element list       */
    EX_INQ_ELS_DF_LEN      = 43,     /**< inquire length of concat element set dist factor list*/
    EX_INQ_ELS_PROP        = 44,     /**< inquire number of properties stored per elem set      */
    EX_INQ_EDGE_MAP        = 45,     /**< inquire number of edge maps                     */
    EX_INQ_FACE_MAP        = 46,     /**< inquire number of face maps                     */
    EX_INQ_COORD_FRAMES    = 47,     /**< inquire number of coordinate frames */
    EX_INQ_INVALID         = -1};

  typedef enum ex_inquiry ex_inquiry;
  
  /*   properties               */
  enum ex_entity_type {
    EX_NODAL       = 14,          /**< nodal "block" for variables*/
    EX_NODE_BLOCK  = 14,          /**< alias for EX_NODAL         */
    EX_NODE_SET    =  2,          /**< node set property code     */
    EX_EDGE_BLOCK  =  6,          /**< edge block property code   */
    EX_EDGE_SET    =  7,          /**< edge set property code     */
    EX_FACE_BLOCK  =  8,          /**< face block property code   */
    EX_FACE_SET    =  9,          /**< face set property code     */
    EX_ELEM_BLOCK  =  1,          /**< element block property code*/
    EX_ELEM_SET    = 10,          /**< face set property code     */
    
    EX_SIDE_SET    =  3,          /**< side set property code     */
    
    EX_ELEM_MAP    =  4,          /**< element map property code  */
    EX_NODE_MAP    =  5,          /**< node map property code     */
    EX_EDGE_MAP    = 11,          /**< edge map property code     */
    EX_FACE_MAP    = 12,          /**< face map property code     */
    
    EX_GLOBAL      = 13,          /**< global "block" for variables*/
    EX_INVALID     = -1};             
  typedef enum ex_entity_type ex_entity_type;
  
  /**
   * ex_opts() function codes - codes are OR'ed into exopts
   */
  enum ex_options {
    EX_DEFAULT  = 0,
    EX_VERBOSE  = 1,  /**< verbose mode message flag   */
    EX_DEBUG    = 2,  /**< debug mode def             */
    EX_ABORT    = 4   /**< abort mode flag def        */
  };
  typedef enum ex_options ex_options;
  
  /**
   * \defgroup StringLengths maximum string lengths;
   * constants that are used as netcdf dimensions must be of type long
   * @{ 
   */
  /** Maximum length of an entity name, attribute name, variable name,
      QA record, element type name */
#define MAX_STR_LENGTH          32L 
  /** Maximum length of the database title or an information record */
#define MAX_LINE_LENGTH         80L
  /** Maximum length of an error message passed to ex_err() function. Typically, internal use only */
#define MAX_ERR_LENGTH          256
  /* @} */

#ifndef EXODUS_EXPORT
#define EXODUS_EXPORT extern
#endif /* EXODUS_EXPORT */

  /* routines for file initialization i/o */
  EXODUS_EXPORT int ex_close (int exoid);
  EXODUS_EXPORT int ex_cvt_nodes_to_sides(int exoid, int *num_elem_per_set,
                                          int *num_nodes_per_set, int *side_sets_elem_index,
                                          int *side_sets_node_index, int *side_sets_elem_list,
                                          int *side_sets_node_list, int *side_sets_side_list);
  EXODUS_EXPORT int ex_copy (int in_exoid, int out_exoid);
#define ex_create(path, mode, comp_ws, io_ws) ex_create_int(path, mode, comp_ws, io_ws, EX_API_VERS_NODOT)  
  EXODUS_EXPORT int ex_create_int (const char *path, int cmode, int *comp_ws, int *io_ws, int my_version);
  EXODUS_EXPORT int ex_get_all_times (int   exoid, void *time_values);
  EXODUS_EXPORT int ex_get_concat_node_sets (int   exoid,
                                             int  *node_set_ids,
                                             int  *num_nodes_per_set, 
                                             int  *num_df_per_set, 
                                             int  *node_sets_node_index,
                                             int  *node_sets_df_index,
                                             int  *node_sets_node_list, 
                                             void *node_sets_dist_fact);
  EXODUS_EXPORT int ex_get_coord_names (int    exoid,
                                        char **coord_names);
  EXODUS_EXPORT int ex_get_coord (int exoid,
                                  void *x_coor,
                                  void *y_coor,
                                  void *z_coor);
  EXODUS_EXPORT int ex_get_n_coord (int exoid,
                                    int start_node_num,
                                    int num_nodes,
                                    void *x_coor,
                                    void *y_coor,
                                    void *z_coor);
  EXODUS_EXPORT int ex_get_concat_side_sets (int   exoid,
                                             int  *side_set_ids,
                                             int  *num_elem_per_set,
                                             int  *num_dist_per_set,
                                             int  *side_sets_elem_index,
                                             int  *side_sets_dist_index,
                                             int  *side_sets_elem_list,
                                             int  *side_sets_side_list,
                                             void *side_sets_dist_fact);
  EXODUS_EXPORT int ex_get_elem_attr_names (int   exoid,
                                            int   elem_blk_id,
                                            char **names);
  EXODUS_EXPORT int ex_get_elem_attr (int   exoid,
                                      int   elem_blk_id,
                                      void *attrib);
  EXODUS_EXPORT int ex_get_ids (int  exoid, ex_entity_type obj_type, int *ids);
  EXODUS_EXPORT int ex_get_elem_blk_ids (int  exoid, int *ids);
  EXODUS_EXPORT int ex_get_elem_block (int   exoid,
                                       int   elem_blk_id,
                                       char *elem_type,
                                       int  *num_elem_this_blk, 
                                       int  *num_nodes_per_elem,
                                       int  *num_attr);

  EXODUS_EXPORT int ex_get_elem_conn (int   exoid,
                                      int   elem_blk_id,
                                      int  *connect);

  EXODUS_EXPORT int ex_get_elem_map (int   exoid,
                                     int   map_id,
                                     int  *elem_map);
  EXODUS_EXPORT int ex_get_elem_num_map (int  exoid,
                                         int *elem_map);
  EXODUS_EXPORT int ex_get_elem_var (int   exoid,
                                     int   time_step,
                                     int   elem_var_index,
                                     int   elem_blk_id, 
                                     int   num_elem_this_blk,
                                     void *elem_var_vals);
  EXODUS_EXPORT int ex_get_elem_varid (int  exoid,
                                       int *varid);
  EXODUS_EXPORT int ex_get_elem_var_time (int   exoid,
                                          int   elem_var_index,
                                          int   elem_number,
                                          int   beg_time_step, 
                                          int   end_time_step,
                                          void *elem_var_vals);
  EXODUS_EXPORT int ex_get_coordinate_frames(int exoid, int *nframes, int *cf_ids,
                                             void* pt_coordinates, char* tags);

  EXODUS_EXPORT int ex_get_glob_vars (int   exoid,
                                      int   time_step,
                                      int   num_glob_vars,
                                      void *glob_var_vals);

  EXODUS_EXPORT int ex_get_glob_var_time (int   exoid,
                                          int   glob_var_index,
                                          int   beg_time_step,
                                          int   end_time_step,
                                          void *glob_var_vals);

  EXODUS_EXPORT int ex_get_info (int exoid, char **info);

  EXODUS_EXPORT int ex_get_init (int   exoid,
                                 char *title,
                                 int  *num_dim,
                                 int  *num_nodes,
                                 int  *num_elem, 
                                 int  *num_elem_blk,
                                 int  *num_node_sets,
                                 int  *num_side_sets);

  EXODUS_EXPORT int ex_get_map (int  exoid, int *elem_map);

  EXODUS_EXPORT int ex_get_map_param (int   exoid,
                                      int  *num_node_maps,
                                      int  *num_elem_maps);

  EXODUS_EXPORT int ex_get_name (int   exoid,
                                 ex_entity_type   obj_type,
                                 int   entity_id, 
                                 char *name);

  EXODUS_EXPORT int ex_get_names (int exoid,
                                  ex_entity_type obj_type,
                                  char **names);

  EXODUS_EXPORT int ex_get_node_map (int   exoid,
                                     int   map_id,
                                     int  *node_map);

  EXODUS_EXPORT int ex_get_node_num_map (int  exoid,
                                         int *node_map);

  EXODUS_EXPORT int ex_get_node_set_param (int  exoid,
                                           int  node_set_id,
                                           int *num_nodes_in_set,
                                           int *num_df_in_set);

  EXODUS_EXPORT int ex_get_node_set (int   exoid,
                                     int   node_set_id,
                                     int  *node_set_node_list);

  EXODUS_EXPORT int ex_get_node_set_dist_fact  (int   exoid,
                                                int   node_set_id,
                                                void *node_set_dist_fact);

  EXODUS_EXPORT int ex_get_node_set_ids (int  exoid,
                                         int *ids);

  EXODUS_EXPORT int ex_get_nset_var_tab (int  exoid,
                                         int  num_nodesets,
                                         int  num_nset_var,
                                         int *nset_var_tab);

  EXODUS_EXPORT int ex_get_nset_var (int   exoid,
                                     int   time_step,
                                     int   nset_var_index,
                                     int   nset_id, 
                                     int   num_node_this_nset,
                                     void *nset_var_vals);

  EXODUS_EXPORT int ex_get_nset_varid (int  exoid,
                                       int *varid);

  EXODUS_EXPORT int ex_get_nodal_var (int   exoid,
                                      int   time_step,
                                      int   nodal_var_index,
                                      int   num_nodes, 
                                      void *nodal_var_vals);

  EXODUS_EXPORT int ex_get_n_nodal_var (int   exoid,
                                        int   time_step,
                                        int   nodal_var_index,
                                        int   start_node, 
                                        int   num_nodes, 
                                        void *nodal_var_vals);

  EXODUS_EXPORT int ex_get_nodal_varid(int exoid, int *varid);

  EXODUS_EXPORT int ex_get_nodal_var_time (int   exoid,
                                           int   nodal_var_index,
                                           int   node_number,
                                           int   beg_time_step, 
                                           int   end_time_step,
                                           void *nodal_var_vals);

  EXODUS_EXPORT int ex_get_nodal_varid_var(int   exoid,
                                           int   time_step,
                                           int   nodal_var_index,
                                           int   num_nodes, 
                                           int   varid,
                                           void *nodal_var_vals);

  EXODUS_EXPORT int ex_get_one_elem_attr (int   exoid,
                                          int   elem_blk_id,
                                          int   attrib_index,
                                          void *attrib);

  EXODUS_EXPORT int ex_get_prop_array (int   exoid,
                                       ex_entity_type obj_type,
                                       const char *prop_name,
                                       int  *values);

  EXODUS_EXPORT int ex_get_prop (int   exoid,
                                 ex_entity_type obj_type,
                                 int   obj_id,
                                 const char *prop_name,
                                 int  *value);

  EXODUS_EXPORT int ex_get_partial_elem_map (int   exoid,
                                             int   map_id,
                                             int ent_start,
                                             int ent_count, 
                                             int  *elem_map);

  EXODUS_EXPORT int ex_get_prop_names (int    exoid,
                                       ex_entity_type obj_type,
                                       char **prop_names);

  EXODUS_EXPORT int ex_get_qa (int exoid,
                               char *qa_record[][4]);
  EXODUS_EXPORT int ex_get_side_set_node_list_len(int exoid,
                                                  int side_set_id,
                                                  int *side_set_node_list_len);
  EXODUS_EXPORT int ex_get_side_set_param (int  exoid,
                                           int  side_set_id,
                                           int *num_side_in_set, 
                                           int *num_dist_fact_in_set);
  EXODUS_EXPORT int ex_get_side_set (int   exoid,
                                     int   side_set_id,
                                     int  *side_set_elem_list, 
                                     int  *side_set_side_list);
  EXODUS_EXPORT int ex_get_side_set_node_count(int exoid,
                                               int side_set_id,
                                               int *side_set_node_cnt_list);
  EXODUS_EXPORT int ex_get_concat_side_set_node_count(int exoid,
                                                      int *side_set_node_cnt_list);
  EXODUS_EXPORT int ex_get_side_set_dist_fact (int   exoid,
                                               int   side_set_id,
                                               void *side_set_dist_fact);
  EXODUS_EXPORT int ex_get_side_set_ids (int  exoid,
                                         int *ids);
  EXODUS_EXPORT int ex_get_side_set_node_list(int exoid,
                                              int side_set_id,
                                              int *side_set_node_cnt_list,
                                              int *side_set_node_list);
  EXODUS_EXPORT int ex_get_sset_var (int   exoid,
                                     int   time_step,
                                     int   sset_var_index,
                                     int   sset_id, 
                                     int   num_side_this_sset,
                                     void *sset_var_vals);

  EXODUS_EXPORT int ex_get_sset_var_tab (int  exoid,
                                         int  num_sidesets,
                                         int  num_sset_var,
                                         int *sset_var_tab);
  EXODUS_EXPORT int ex_get_sset_varid (int  exoid,
                                       int *varid);
  EXODUS_EXPORT int ex_get_time (int   exoid,
                                 int   time_step,
                                 void *time_value);
  EXODUS_EXPORT int ex_get_variable_names (int   exoid,
                                           ex_entity_type obj_type,
                                           int   num_vars,
                                           char *var_names[]);
  EXODUS_EXPORT int ex_get_var_names (int   exoid,
                                      const char *var_type,
                                      int   num_vars,
                                      char *var_names[]);
  EXODUS_EXPORT int ex_get_varid (int  exoid, ex_entity_type obj_type,
                                  int *varid_arr);
  EXODUS_EXPORT int ex_get_variable_name (int   exoid,
                                          ex_entity_type obj_type,
                                          int   var_num,
                                          char *var_name);
  EXODUS_EXPORT int ex_get_var_name (int   exoid,
                                     const char *var_type,
                                     int   var_num,
                                     char *var_name);
  EXODUS_EXPORT int ex_get_var_param (int   exoid,
                                      const char *var_type,
                                      int  *num_vars);
  EXODUS_EXPORT int ex_get_variable_param (int   exoid,
                                           ex_entity_type obj_type,
                                           int  *num_vars);

  EXODUS_EXPORT int ex_get_object_truth_vector (int  exoid,
                                                ex_entity_type var_type,
                                                int  object_id,
                                                int  num_var,
                                                int *var_vector);
  EXODUS_EXPORT int ex_get_truth_table (int  exoid,
                                        ex_entity_type obj_type,
                                        int  num_blk,
                                        int  num_var,
                                        int *var_tab);
  EXODUS_EXPORT int ex_get_var_tab (int  exoid,
                                    const char *var_type,
                                    int  num_blk,
                                    int  num_var,
                                    int *var_tab);
  
  EXODUS_EXPORT int ex_get_elem_var_tab (int  exoid,
                                         int  num_elem_blk,
                                         int  num_elem_var,
                                         int *elem_var_tab);

#define ex_open(path, mode, comp_ws, io_ws, version) ex_open_int(path, mode, comp_ws, io_ws, version, EX_API_VERS_NODOT)  
  EXODUS_EXPORT int ex_open_int (const char  *path,
                                 int    mode,
                                 int   *comp_ws,
                                 int   *io_ws,
                                 float *version, int my_version);
  
  EXODUS_EXPORT int ex_put_attr_param (int   exoid,
                                       ex_entity_type obj_type,
                                       int   obj_id,
                                       int   num_attrs);

  EXODUS_EXPORT int ex_get_attr_param (int   exoid,
                                       ex_entity_type obj_type,
                                       int   obj_id,
                                       int   *num_attrs);

  EXODUS_EXPORT int ex_put_all_var_param (int exoid,
                                          int num_g, int num_n,
                                          int num_e, int *elem_var_tab,
                                          int num_m, int *nset_var_tab,
                                          int num_s, int *sset_var_tab);

  EXODUS_EXPORT int ex_put_concat_elem_block (int    exoid,
                                              const int*   elem_blk_id,
                                              char *elem_type[],
                                              const int*   num_elem_this_blk,
                                              const int*   num_nodes_per_elem,
                                              const int*   num_attr,
                                              int    define_maps);

  EXODUS_EXPORT int ex_put_concat_node_sets (int   exoid,
                                             int  *node_set_ids,
                                             int  *num_nodes_per_set,
                                             int  *num_dist_per_set,
                                             int  *node_sets_node_index,
                                             int  *node_sets_df_index,
                                             int  *node_sets_node_list,
                                             void *node_sets_dist_fact);

  EXODUS_EXPORT int ex_put_concat_side_sets (int   exoid,
                                             int  *side_set_ids,
                                             int  *num_elem_per_set,
                                             int  *num_dist_per_set,
                                             int  *side_sets_elem_index,
                                             int  *side_sets_dist_index,
                                             int  *side_sets_elem_list,
                                             int  *side_sets_side_list,
                                             void *side_sets_dist_fact);

  EXODUS_EXPORT int ex_put_concat_var_param (int exoid, int num_g, int num_n,
                                             int num_e, int num_elem_blk, int  *elem_var_tab);
  
  EXODUS_EXPORT int ex_put_coord_names (int   exoid,
                                        char *coord_names[]);
  EXODUS_EXPORT int ex_put_coord (int   exoid,
                                  const void *x_coor,
                                  const void *y_coor,
                                  const void *z_coor);
  EXODUS_EXPORT int ex_put_n_coord (int   exoid,
                                    int   start_node_num,
                                    int   num_nodes,
                                    const void *x_coor,
                                    const void *y_coor,
                                    const void *z_coor);
  EXODUS_EXPORT int ex_put_elem_attr_names(int   exoid,
                                           int   elem_blk_id,
                                           char *names[]);
  EXODUS_EXPORT int ex_put_elem_attr (int   exoid,
                                      int   elem_blk_id,
                                      const void *attrib);
  EXODUS_EXPORT int ex_put_elem_block (int   exoid,
                                       int   elem_blk_id,
                                       const char *elem_type,
                                       int   num_elem_this_blk,
                                       int   num_nodes_per_elem,
                                       int   num_attr);

  EXODUS_EXPORT int ex_put_elem_conn (int   exoid,
                                      int   elem_blk_id,
                                      const int  *connect);
  EXODUS_EXPORT int ex_put_elem_map (int exoid,
                                     int map_id,
                                     const int *elem_map);
  EXODUS_EXPORT int ex_put_id_map(int exoid,
                                  ex_entity_type obj_type,
                                  const int *map);
  
  EXODUS_EXPORT int ex_get_id_map(int exoid,
                                  ex_entity_type obj_type,
                                  int *map);
  
  EXODUS_EXPORT int ex_put_elem_num_map (int  exoid,
                                         const int *elem_map);
  EXODUS_EXPORT int ex_put_elem_var (int   exoid,
                                     int   time_step,
                                     int   elem_var_index,
                                     int   elem_blk_id,
                                     int   num_elem_this_blk,
                                     const void *elem_var_vals);

  EXODUS_EXPORT int ex_put_coordinate_frames(int exoid, int nframes, const int cf_ids[], 
                                             void* pt_coordinates, const char* tags);
  EXODUS_EXPORT int ex_put_glob_vars (int   exoid,
                                      int   time_step,
                                      int   num_glob_vars,
                                      const void *glob_var_vals);
  EXODUS_EXPORT int ex_put_info (int   exoid, 
                                 int   num_info,
                                 char *info[]);
  EXODUS_EXPORT int ex_put_init (int   exoid,
                                 const char *title,
                                 int   num_dim,
                                 int   num_nodes,
                                 int   num_elem,
                                 int   num_elem_blk,
                                 int   num_node_sets,
                                 int   num_side_sets);

  EXODUS_EXPORT int ex_put_map (int  exoid,
                                const int *elem_map);
  EXODUS_EXPORT int ex_put_map_param (int   exoid,
                                      int   num_node_maps,
                                      int   num_elem_maps);
  EXODUS_EXPORT int ex_put_name (int   exoid,
                                 ex_entity_type obj_type,
                                 int   entity_id,
                                 const char *name);
  EXODUS_EXPORT int ex_put_names (int   exoid,
                                  ex_entity_type obj_type,
                                  char *names[]);
  EXODUS_EXPORT int ex_put_nodal_var (int   exoid,
                                      int   time_step,
                                      int   nodal_var_index,
                                      int   num_nodes, 
                                      const void *nodal_var_vals);

  EXODUS_EXPORT int ex_put_n_nodal_var (int   exoid,
                                        int   time_step,
                                        int   nodal_var_index,
                                        int   start_node, 
                                        int   num_nodes, 
                                        const void *nodal_var_vals);

  EXODUS_EXPORT int ex_put_nodal_varid_var(int   exoid,
                                           int   time_step,
                                           int   nodal_var_index,
                                           int   num_nodes, 
                                           int   varid,
                                           const void *nodal_var_vals);

  EXODUS_EXPORT int ex_put_node_map (int exoid,
                                     int map_id,
                                     const int *node_map);
  EXODUS_EXPORT int ex_put_node_num_map (int  exoid,
                                         const int *node_map);
  EXODUS_EXPORT int ex_put_node_set_param (int exoid,
                                           int node_set_id,
                                           int num_nodes_in_set,
                                           int num_dist_in_set);
  EXODUS_EXPORT int ex_put_node_set (int   exoid,
                                     int   node_set_id,
                                     const int  *node_set_node_list);
  EXODUS_EXPORT int ex_put_node_set_dist_fact  (int   exoid,
                                                int   node_set_id,
                                                const void *node_set_dist_fact);
  EXODUS_EXPORT int ex_put_nset_var (int   exoid,
                                     int   time_step,
                                     int   nset_var_index,
                                     int   nset_id,
                                     int   num_nodes_this_nset,
                                     const void *nset_var_vals);

  EXODUS_EXPORT int ex_put_nset_var_tab (int  exoid,
                                         int  num_nset,
                                         int  num_nset_var,
                                         int *nset_var_tab);
  EXODUS_EXPORT int ex_put_one_elem_attr (int   exoid,
                                          int   elem_blk_id,
                                          int   attrib_index,
                                          const void *attrib);

  EXODUS_EXPORT int ex_put_n_one_attr( int   exoid,
                                       ex_entity_type obj_type,
                                       int   obj_id,
                                       int   start_num,
                                       int   num_ent,
                                       int   attrib_index,
                                       const void *attrib );

  EXODUS_EXPORT int ex_put_partial_elem_map (int   exoid,
                                             int   map_id,
                                             int ent_start,
                                             int ent_count, 
                                             const int  *elem_map);

  EXODUS_EXPORT int ex_put_partial_set_dist_fact (int   exoid,
                                                  ex_entity_type set_type,
                                                  int   set_id,
                                                  int   offset,
                                                  int   num_to_put,
                                                  const void *set_dist_fact);

  EXODUS_EXPORT int ex_put_prop (int   exoid,
                                 ex_entity_type obj_type,
                                 int   obj_id,
                                 const char *prop_name,
                                 int   value);

  EXODUS_EXPORT int ex_put_prop_array (int   exoid,
                                       ex_entity_type obj_type,
                                       const char *prop_name,
                                       const int  *values);
  EXODUS_EXPORT int ex_put_prop_names (int   exoid,
                                       ex_entity_type obj_type,
                                       int   num_props,
                                       char **prop_names);
  EXODUS_EXPORT int ex_put_qa (int   exoid,
                               int   num_qa_records,
                               char* qa_record[][4]);
  EXODUS_EXPORT int ex_put_side_set_param (int exoid,
                                           int side_set_id,
                                           int num_side_in_set,
                                           int num_dist_fact_in_set);
  EXODUS_EXPORT int ex_put_side_set (int   exoid,
                                     int   side_set_id,
                                     const int  *side_set_elem_list,
                                     const int  *side_set_side_list);
  EXODUS_EXPORT int ex_put_side_set_dist_fact (int   exoid,
                                               int   side_set_id,
                                               const void *side_set_dist_fact);
  EXODUS_EXPORT int ex_put_sset_var (int   exoid,
                                     int   time_step,
                                     int   sset_var_index,
                                     int   sset_id,
                                     int   num_faces_this_sset,
                                     const void *sset_var_vals);

  EXODUS_EXPORT int ex_put_sset_var_tab (int  exoid,
                                         int  num_sset,
                                         int  num_sset_var,
                                         int *sset_var_tab);
  EXODUS_EXPORT int ex_put_time (int   exoid,
                                 int   time_step,
                                 const void *time_value);
  EXODUS_EXPORT int ex_put_varid_var(int   exoid,
                                     int   time_step,
                                     int   varid,
                                     int   num_entity,
                                     const void *var_vals);

  EXODUS_EXPORT int ex_put_var_names (int   exoid,
                                      const char *var_type,
                                      int   num_vars,
                                      char *var_names[]);
  EXODUS_EXPORT int ex_put_var_name (int   exoid,
                                     const char *var_type,
                                     int   var_num,
                                     const char *var_name);
  EXODUS_EXPORT int ex_put_var_param (int   exoid,
                                      const char *var_type,
                                      int   num_vars);
  EXODUS_EXPORT int ex_put_variable_names (int   exoid,
                                           ex_entity_type obj_type,
                                           int   num_vars,
                                           char* var_names[]);
  EXODUS_EXPORT int ex_put_variable_name (int   exoid,
                                          ex_entity_type obj_type,
                                          int   var_num,
                                          const char *var_name);
  EXODUS_EXPORT int ex_put_variable_param (int exoid,
                                           ex_entity_type obj_type,
                                           int num_vars);
  EXODUS_EXPORT int ex_put_truth_table (int  exoid,
                                        ex_entity_type obj_type,
                                        int  num_blk,
                                        int  num_var,
                                        int *var_tab);
  EXODUS_EXPORT int ex_put_var_tab (int  exoid,
                                    const char *var_type,
                                    int  num_blk,
                                    int  num_var,
                                    int *var_tab);
  
  EXODUS_EXPORT int ex_put_elem_var_tab (int  exoid,
                                         int  num_elem_blk,
                                         int  num_elem_var,
                                         int *elem_var_tab);
  EXODUS_EXPORT int ex_update (int exoid);
  EXODUS_EXPORT int ex_get_num_props (int exoid, ex_entity_type obj_type);
  EXODUS_EXPORT int ex_large_model(int exoid);
  EXODUS_EXPORT size_t ex_header_size(int exoid);

  EXODUS_EXPORT void ex_err(const char*, const char*, int);
  EXODUS_EXPORT void ex_get_err(const char** msg, const char** func, int* errcode);
  EXODUS_EXPORT void ex_opts(int options);
  EXODUS_EXPORT int ex_inquire(int exoid, int inquiry, int*, void*, char*);
  EXODUS_EXPORT int ex_inquire_int(int exoid, int inquiry);

  EXODUS_EXPORT int ex_get_varid_var(int   exoid,
                                     int   time_step,
                                     int   varid,
                                     int   num_entity,
                                     void *var_vals);
  
  /* ERROR CODE DEFINITIONS AND STORAGE                                       */
  extern int exerrval;     /**< shared error return value                */
  extern int exoptval;     /**< error reporting flag (default is quiet)  */
  
  char* ex_name_of_object(ex_entity_type obj_type);
  ex_entity_type ex_var_type_to_ex_entity_type(char var_type);

#ifdef __cplusplus
}                               /* close brackets on extern "C" declaration */
#endif

/**
 * \defgroup ErrorReturnCodes Exodus error return codes - exerrval return values
 * @{
 */
#define EX_MEMFAIL       1000   /**< memory allocation failure flag def       */
#define EX_BADFILEMODE   1001   /**< bad file mode def                        */
#define EX_BADFILEID     1002   /**< bad file id def                          */
#define EX_WRONGFILETYPE 1003   /**< wrong file type for function             */
#define EX_LOOKUPFAIL    1004   /**< id table lookup failed                   */
#define EX_BADPARAM      1005   /**< bad parameter passed                     */
#define EX_MSG          -1000   /**< message print code - no error implied    */
#define EX_PRTLASTMSG   -1001   /**< print last error message msg code        */
#define EX_NULLENTITY   -1006   /**< null entity found                        */
/* @} */

#include "exodusII_ext.h"
#endif

