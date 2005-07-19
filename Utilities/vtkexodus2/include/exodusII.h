/*
 * Copyright (c) 1994 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Governement
 * retains certain rights in this software.
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
 *     * Neither the name of Sandia Corporation nor the names of its
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
 * exodusII.h - Exodus II include file, for general use
 *
 * author - Sandia National Laboratories
 *          Larry A. Schoof - Original
 *          James A. Schutt - 8 byte float and standard C definitions
 *          Vic Yarberry    - Added headers and error logging
 *
 *          
 * environment - UNIX
 *
 * exit conditions - 
 *
 * revision history - 
 *
 *  Id
 *****************************************************************************/

#include "vtk_netcdf.h"
#include "exodusII_cfg.h"
#ifndef TRUE
#define TRUE -1
#endif

#ifndef FALSE
#define FALSE 0 
#endif

#ifndef EXODUS_II_HDR
#define EXODUS_II_HDR

/*
 * need following extern if this include file is used in a C++ program, to
 * keep the C++ compiler from mangling the function names.
 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * The following are miscellaneous constants used in the EXODUS II API.
 */

#define EX_NOCLOBBER            0
#define EX_CLOBBER              1
#define EX_NORMAL_MODEL         2 /* disable mods that permit storage of larger models */
#define EX_LARGE_MODEL          4 /* enable mods that permit storage of larger models */

#define EX_READ                 0
#define EX_WRITE                1

#define EX_INQ_FILE_TYPE        1               /* inquire EXODUS II file type*/
#define EX_INQ_API_VERS         2               /* inquire API version number */
#define EX_INQ_DB_VERS          3               /* inquire database version   */
                                                /*   number                   */
#define EX_INQ_TITLE            4               /* inquire database title     */
#define EX_INQ_DIM              5               /* inquire number of          */
                                                /*   dimensions               */
#define EX_INQ_NODES            6               /* inquire number of nodes    */
#define EX_INQ_ELEM             7               /* inquire number of elements */
#define EX_INQ_ELEM_BLK         8               /* inquire number of element  */
                                                /*   blocks                   */
#define EX_INQ_NODE_SETS        9               /* inquire number of node sets*/
#define EX_INQ_NS_NODE_LEN      10              /* inquire length of node set */
                                                /*   node list                */
#define EX_INQ_SIDE_SETS        11              /* inquire number of side sets*/
#define EX_INQ_SS_NODE_LEN      12              /* inquire length of side set */
                                                /*   node list                */
#define EX_INQ_SS_ELEM_LEN      13              /* inquire length of side set */
                                                /*   element list             */
#define EX_INQ_QA               14              /* inquire number of QA       */
                                                /*   records                  */
#define EX_INQ_INFO             15              /* inquire number of info     */
                                                /*   records                  */
#define EX_INQ_TIME             16              /* inquire number of time     */
                                                /*   steps in the database    */
#define EX_INQ_EB_PROP          17              /* inquire number of element  */
                                                /*   block properties         */
#define EX_INQ_NS_PROP          18              /* inquire number of node set */
                                                /*   properties               */
#define EX_INQ_SS_PROP          19              /* inquire number of side set */
#define EX_INQ_NS_DF_LEN        20              /* inquire length of node set */
                                                /*   distribution factor  list*/
#define EX_INQ_SS_DF_LEN        21              /* inquire length of node set */
                                                /*   distribution factor  list*/
#define EX_INQ_LIB_VERS         22              /* inquire API Lib vers number*/
#define EX_INQ_EM_PROP          23              /* inquire number of element  */
                                                /*   map properties           */
#define EX_INQ_NM_PROP          24              /* inquire number of node     */
                                                /*   map properties           */
#define EX_INQ_ELEM_MAP         25              /* inquire number of element  */
                                                /*   maps                     */
#define EX_INQ_NODE_MAP         26              /* inquire number of node     */
                                                /*   maps                     */

/*   properties               */
#define EX_ELEM_BLOCK           1               /* element block property code*/
#define EX_NODE_SET             2               /* node set property code     */
#define EX_SIDE_SET             3               /* side set property code     */
#define EX_ELEM_MAP             4               /* element map property code  */
#define EX_NODE_MAP             5               /* node map property code     */

/*   max string lengths; constants that are used as netcdf dimensions must be
     of type long       */
#define MAX_STR_LENGTH          32L
#define MAX_VAR_NAME_LENGTH     20
#define MAX_LINE_LENGTH         80L
#define MAX_ERR_LENGTH          256

/*   for netCDF 3.4, we estimate the size of the header; 
     if estimate is larger than this max, set the estimate to this max;
     I've never measured a header larger than 20K   */
#define MAX_HEADER_SIZE         30000

/* routines for file initialization i/o */

EXODUS_EXPORT int ex_create             (const char*, int, int*, int*);
EXODUS_EXPORT int ex_open               (const char*, int, int*, int*, float*);
EXODUS_EXPORT int ex_close              (int);
EXODUS_EXPORT void ex_err               (const char*, const char*, int);
EXODUS_EXPORT void ex_opts              (int);
EXODUS_EXPORT int ex_update             (int);

EXODUS_EXPORT int ex_put_init           (int, const char*, int, int, int, int, int, int);
EXODUS_EXPORT int ex_get_init           (int, char*, int*, int*, int*, int*, int*, int*);

  EXODUS_EXPORT int ex_put_qa           (int,int, char*[][4]);
EXODUS_EXPORT int ex_get_qa             (int, char*[][4]);

EXODUS_EXPORT int ex_put_info           (int, int, char*[]);
EXODUS_EXPORT int ex_get_info           (int, char*[]);

/* routines for model description i/o */

EXODUS_EXPORT int ex_put_coord          (int, const void*, const void*, const void*);
EXODUS_EXPORT int ex_get_coord          (int, void*, void*, void*);

EXODUS_EXPORT int ex_put_coord_names    (int, char*[]);
EXODUS_EXPORT int ex_get_coord_names    (int, char*[]);

EXODUS_EXPORT int ex_put_map            (int, const int*);
EXODUS_EXPORT int ex_get_map            (int, int*);

EXODUS_EXPORT int ex_put_elem_block     (int, int, const char*, int, int, int);
EXODUS_EXPORT int ex_get_elem_block     (int, int, char*, int*, int*, int*);
EXODUS_EXPORT int ex_put_concat_elem_block (int, const int*, char*[],
                                     const int*, const int*, const int*, int);

EXODUS_EXPORT int ex_get_elem_blk_ids   (int, int*);

EXODUS_EXPORT int ex_put_elem_conn      (int, int, const int*);
EXODUS_EXPORT int ex_get_elem_conn      (int, int, int*);

EXODUS_EXPORT int ex_put_elem_attr      (int, int, const void*);
EXODUS_EXPORT int ex_get_elem_attr      (int, int, void*);

EXODUS_EXPORT int ex_put_node_set_param (int, int, int, int);
EXODUS_EXPORT int ex_get_node_set_param (int, int, int*, int*);

EXODUS_EXPORT int ex_put_node_set       (int, int, const int*);
EXODUS_EXPORT int ex_get_node_set       (int, int, int*);

EXODUS_EXPORT int ex_put_node_set_dist_fact (int, int, const void*);
EXODUS_EXPORT int ex_get_node_set_dist_fact (int, int, void*);

EXODUS_EXPORT int ex_get_node_set_ids   (int, int*);

EXODUS_EXPORT int ex_put_concat_node_sets (int, int*, int*, int*, int*, int*, int*, void*);
EXODUS_EXPORT int ex_get_concat_node_sets(int, int*, int*, int*, int*, int*, int*, void*);

EXODUS_EXPORT int ex_put_side_set_param (int, int, int, int);
EXODUS_EXPORT int ex_get_side_set_param (int, int, int*, int*);

EXODUS_EXPORT int ex_put_side_set       (int, int, const int*, const int*);
EXODUS_EXPORT int ex_get_side_set       (int, int, int*, int*);
EXODUS_EXPORT int ex_put_side_set_dist_fact (int, int, const void*);
EXODUS_EXPORT int ex_get_side_set_dist_fact (int, int, void*);
EXODUS_EXPORT int ex_get_side_set_ids   (int, int*);
EXODUS_EXPORT int ex_get_side_set_node_list (int, int, int*, int*);
EXODUS_EXPORT int ex_get_side_set_node_count(int, int, int*);

EXODUS_EXPORT int ex_put_prop_names     (int, int, int, char**);
EXODUS_EXPORT int ex_get_prop_names     (int, int, char**);

EXODUS_EXPORT int ex_put_prop           (int, int, int, const char*, int);
EXODUS_EXPORT int ex_get_prop           (int, int, int, const char*, int*);

EXODUS_EXPORT int ex_put_prop_array     (int, int, const char*, const int*);
EXODUS_EXPORT int ex_get_prop_array     (int, int, const char*, int*);

EXODUS_EXPORT int ex_put_concat_side_sets (int, const int*, const int*, const int*, const int*,
                                    const int*, const int*, const int*, const void* );
EXODUS_EXPORT int ex_get_concat_side_sets (int, int*, int*, int*, int*, int*, int*, int*, void* );
EXODUS_EXPORT int ex_cvt_nodes_to_sides   (int, int*, int*, int*, int*, int*, int*, int*);

EXODUS_EXPORT int ex_put_coordinate_frames(int, int, const int*, void*, const char*);

EXODUS_EXPORT int ex_get_coordinate_frames(int, int*, int*, void*, char*);

/* routines for analysis results i/o */

EXODUS_EXPORT int ex_put_var_param      (int, const char*, int);
EXODUS_EXPORT int ex_get_var_param      (int, const char*, int*);

EXODUS_EXPORT int ex_put_concat_var_param(int, int, int, int, int, int*);
                                                      
EXODUS_EXPORT int ex_put_var_names      (int, const char*, int, char*[]);
EXODUS_EXPORT int ex_get_var_names      (int, const char*, int, char*[]);

EXODUS_EXPORT int ex_put_var_name       (int, const char*, int, const char*);
EXODUS_EXPORT int ex_get_var_name       (int, const char*, int, char*);

EXODUS_EXPORT int ex_put_elem_var_tab   (int, int, int, int*);
EXODUS_EXPORT int ex_get_elem_var_tab   (int, int, int, int*);

EXODUS_EXPORT int ex_put_glob_vars      (int, int, int, const void*);
EXODUS_EXPORT int ex_get_glob_vars      (int, int, int, void*);

EXODUS_EXPORT int ex_get_glob_var_time  (int, int, int, int, void*);

EXODUS_EXPORT int ex_put_nodal_var      (int, int, int, int, const void*);
EXODUS_EXPORT int ex_get_nodal_var      (int, int, int, int, void*);

EXODUS_EXPORT int ex_get_nodal_var_time (int, int, int, int, int, void*);

EXODUS_EXPORT int ex_put_elem_var       (int, int, int, int, int, const void*);
EXODUS_EXPORT int ex_get_elem_var       (int, int, int, int, int, void*);

EXODUS_EXPORT int ex_get_elem_var_time  (int, int, int, int, int, void*);

EXODUS_EXPORT int ex_put_time           (int, int, const void*);
EXODUS_EXPORT int ex_get_time           (int, int, void*);

EXODUS_EXPORT int ex_get_all_times      (int, void*);

EXODUS_EXPORT int ex_inquire            (int, int, int*, void*, char*);
EXODUS_EXPORT int ex_get_num_props      (int, int);

EXODUS_EXPORT int ex_put_elem_num_map   (int, const int*);
EXODUS_EXPORT int ex_get_elem_num_map   (int, int*);

EXODUS_EXPORT int ex_put_node_num_map   (int, const int*);
EXODUS_EXPORT int ex_get_node_num_map   (int, int*);

EXODUS_EXPORT int ex_put_map_param      (int, int, int);
EXODUS_EXPORT int ex_get_map_param      (int, int*, int*);

EXODUS_EXPORT int ex_put_elem_map       (int, int, const int*);
EXODUS_EXPORT int ex_get_elem_map       (int, int, int*);

EXODUS_EXPORT int ex_put_node_map       (int, int, const int*);
EXODUS_EXPORT int ex_get_node_map       (int, int, int*);

EXODUS_EXPORT int *itol              (const int*, int); 

EXODUS_EXPORT int ltoi                  (const int*, int*, int);

EXODUS_EXPORT int ex_copy               (int, int);

EXODUS_EXPORT int cpy_att               (int, int, int, int);

EXODUS_EXPORT int cpy_var_def           (int, int, int, char*);

EXODUS_EXPORT int cpy_var_val           (int, int, char*);

EXODUS_EXPORT int ex_get_elem_varid  (int  exoid, int *varid);
EXODUS_EXPORT int ex_get_nodal_varid (int  exoid, int *varid);

EXODUS_EXPORT int ex_get_nodal_varid_var(int, int, int, int, int,       void *nodal_var_vals);
EXODUS_EXPORT int ex_put_nodal_varid_var(int, int, int, int, int, const void *nodal_var_vals);

EXODUS_EXPORT int ex_get_varid_var(int, int, int, int,       void *var_vals);
EXODUS_EXPORT int ex_put_varid_var(int, int, int, int, const void *var_vals);

/* ERROR CODE DEFINITIONS AND STORAGE                                       */
EXODUS_EXPORT int exerrval;            /* shared error return value                */
EXODUS_EXPORT int exoptval;            /* error reporting flag (default is quiet)  */

#ifdef __cplusplus
}                               /* close brackets on extern "C" declaration */
#endif

#endif

/* ex_opts function codes - codes are OR'ed into exopts                     */
#define EX_VERBOSE      1       /* verbose mode message flag                */
#define EX_DEBUG        2       /* debug mode def                           */
#define EX_ABORT        4       /* abort mode flag def                      */

/* Exodus error return codes - exerrval return values:                      */
#define EX_MEMFAIL       1000   /* memory allocation failure flag def       */
#define EX_BADFILEMODE   1001   /* bad file mode def                        */
#define EX_BADFILEID     1002   /* bad file id def                          */
#define EX_WRONGFILETYPE 1003   /* wrong file type for function             */
#define EX_LOOKUPFAIL    1004   /* id table lookup failed                   */
#define EX_BADPARAM      1005   /* bad parameter passed                     */
#define EX_NULLENTITY   -1006   /* null entity found                        */
#define EX_MSG          -1000   /* message print code - no error implied    */
#define EX_PRTLASTMSG   -1001   /* print last error message msg code        */

