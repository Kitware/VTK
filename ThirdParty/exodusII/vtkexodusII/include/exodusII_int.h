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
 * exodusII_int.h - ExodusII header file for internal Exodus call use only
 *
 */

#ifndef EXODUS_II_INT_HDR
#define EXODUS_II_INT_HDR

#include "exodus_config.h"

#if defined(EXODUS_THREADSAFE)
#include <pthread.h>
#endif

#include "vtk_netcdf.h"
#if VTK_MODULE_USE_EXTERNAL_vtknetcdf
#if defined(NC_HAVE_META_H)
#include "netcdf_meta.h"
#endif
#endif

#ifdef _WIN32
#define PRId64 "I64d"
#else
#include <inttypes.h>
#endif

#ifndef __APPLE__
#if defined __STDC__ || defined __cplusplus
#include <stdlib.h>
#endif
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4127)
#pragma warning(disable : 4706)
#pragma warning(disable : 4701)
#endif

#if defined(__BORLANDC__)
#pragma warn - 8004 /* "assigned a value that is never used" */
#endif

#include <stdio.h>

#if defined(_MSC_VER) && _MSC_VER < 1900
#define __func__ __FUNCTION__
#define snprintf _snprintf
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* A format string for outputting size_t ... */
#if defined(__STDC_VERSION__)
#if (__STDC_VERSION__ >= 199901L)
#define ST_ZU "zu"
#else
#define ST_ZU "lu"
#endif
#else
#define ST_ZU "lu"
#endif

#define MAX_VAR_NAME_LENGTH 32 /**< Internal use only */

/* Default "filesize" for newly created files.
 * Set to 0 for normal filesize setting.
 * Set to 1 for EXODUS_LARGE_MODEL setting to be the default
 */
#define EXODUS_DEFAULT_SIZE 1

/* Used to map between root (file id) and group ids when using groups */
#define EX_FILE_ID_MASK (0xffff0000) /* Must match FILE_ID_MASK in netcdf nc4internal.h */
#define EX_GRP_ID_MASK (0x0000ffff)  /* Must match GRP_ID_MASK in netcdf nc4internal.h */

void ex_reset_error_status();

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

extern pthread_once_t EX_first_init_g;

typedef struct EX_mutex_struct
{
  pthread_mutex_t     atomic_lock; /* lock for atomicity of new mechanism */
  pthread_mutexattr_t attribute;
} EX_mutex_t;

extern EX_mutex_t   EX_g;
extern int          ex_mutex_lock(EX_mutex_t *mutex);
extern int          ex_mutex_unlock(EX_mutex_t *mutex);
extern void         ex_pthread_first_thread_init(void);
extern EX_errval_t *exerrval_get();

#define EX_FUNC_ENTER()                                                                            \
  do {                                                                                             \
    /* Initialize the thread-safe code */                                                          \
    pthread_once(&EX_first_init_g, ex_pthread_first_thread_init);                                  \
                                                                                                   \
    /* Grab the mutex for the library */                                                           \
    ex_mutex_lock(&EX_g);                                                                          \
    ex_errval               = exerrval_get();                                                      \
    exerrval                = 0;                                                                   \
    ex_errval->last_err_num = 0;                                                                   \
  } while (0)

#define EX_FUNC_ENTER_INT()                                                                        \
  do {                                                                                             \
    /* Initialize the thread-safe code */                                                          \
    pthread_once(&EX_first_init_g, ex_pthread_first_thread_init);                                  \
                                                                                                   \
    /* Grab the mutex for the library */                                                           \
    ex_mutex_lock(&EX_g);                                                                          \
    ex_errval = exerrval_get();                                                                    \
  } while (0)

#define EX_FUNC_LEAVE(error)                                                                       \
  do {                                                                                             \
    ex_mutex_unlock(&EX_g);                                                                        \
    return error;                                                                                  \
  } while (0)

#define EX_FUNC_VOID()                                                                             \
  do {                                                                                             \
    ex_mutex_unlock(&EX_g);                                                                        \
    return;                                                                                        \
  } while (0)

#else

#if 0
EXODUS_EXPORT int indent;
#define EX_FUNC_ENTER()                                                                            \
  do {                                                                                             \
    ex_reset_error_status();                                                                       \
    fprintf(stderr, "%d Enter: %s\n", indent, __func__);                                           \
    indent++;                                                                                      \
  } while (0)
#define EX_FUNC_ENTER_INT()                                                                        \
  do {                                                                                             \
    fprintf(stderr, "%d Enter: %s\n", indent, __func__);                                           \
    indent++;                                                                                      \
  } while (0)
#define EX_FUNC_LEAVE(error)                                                                       \
  do {                                                                                             \
    indent--;                                                                                      \
    fprintf(stderr, "%d Leave: %s\n", indent, __func__);                                           \
    return error;                                                                                  \
  } while (0)
#define EX_FUNC_VOID()                                                                             \
  do {                                                                                             \
    indent--;                                                                                      \
    fprintf(stderr, "%d Leave: %s\n", indent, __func__);                                           \
    return;                                                                                        \
  } while (0)
#else
#define EX_FUNC_ENTER()                                                                            \
  {                                                                                                \
    ex_reset_error_status();                                                                       \
  }
#define EX_FUNC_ENTER_INT()
#define EX_FUNC_LEAVE(error) return error
#define EX_FUNC_VOID() return
#endif
#endif
/*
 * This file contains defined constants that are used internally in the
 * EXODUS API.
 *
 * The first group of constants refer to netCDF variables, attributes, or
 * dimensions in which the EXODUS data are stored.  Using the defined
 * constants will allow the names of the netCDF entities to be changed easily
 * in the future if needed.  The first three letters of the constant identify
 * the netCDF entity as a variable (VAR), dimension (DIM), or attribute (ATT).
 *
 * NOTE: The entity name should not have any blanks in it.  Blanks are
 *       technically legal but some netcdf utilities (ncgen in particular)
 *       fail when they encounter a blank in a name.
 *
 *      DEFINED CONSTANT        ENTITY NAME     DATA STORED IN ENTITY
 */
#define ATT_FILE_TYPE "type"                /* obsolete                  */
#define ATT_TITLE "title"                   /* the database title        */
#define ATT_API_VERSION "api_version"       /* the EXODUS api vers #   */
#define ATT_API_VERSION_BLANK "api version" /* the EXODUS api vers #   */
                                            /*  used for db version 2.01 */
                                            /*  and earlier              */
#define ATT_VERSION "version"               /* the EXODUS file vers # */
#define ATT_FILESIZE "file_size"            /* 1=large, 0=normal */
#define ATT_FLT_WORDSIZE "floating_point_word_size"
/* word size of floating     */
/* point numbers in file     */
#define ATT_FLT_WORDSIZE_BLANK "floating point word size"
/* word size of floating     */
/* point numbers in file     */
/* used for db version 2.01  */
/* and earlier               */
#define ATT_MAX_NAME_LENGTH "maximum_name_length"
#define ATT_INT64_STATUS "int64_status"

#define DIM_NUM_NODES "num_nodes"     /* # of nodes                */
#define DIM_NUM_DIM "num_dim"         /* # of dimensions; 2- or 3-d*/
#define DIM_NUM_EDGE "num_edge"       /* # of edges (over all blks)*/
#define DIM_NUM_FACE "num_face"       /* # of faces (over all blks)*/
#define DIM_NUM_ELEM "num_elem"       /* # of elements             */
#define DIM_NUM_EL_BLK "num_el_blk"   /* # of element blocks       */
#define DIM_NUM_ED_BLK "num_ed_blk"   /* # of edge blocks          */
#define DIM_NUM_FA_BLK "num_fa_blk"   /* # of face blocks          */
#define VAR_COORD "coord"             /* nodal coordinates         */
#define VAR_COORD_X "coordx"          /* X-dimension coordinate    */
#define VAR_COORD_Y "coordy"          /* Y-dimension coordinate    */
#define VAR_COORD_Z "coordz"          /* Z-dimension coordinate    */
#define VAR_NAME_COOR "coor_names"    /* names of coordinates      */
#define VAR_NAME_EL_BLK "eb_names"    /* names of element blocks   */
#define VAR_NAME_NS "ns_names"        /* names of node sets        */
#define VAR_NAME_SS "ss_names"        /* names of side sets        */
#define VAR_NAME_EM "emap_names"      /* names of element maps     */
#define VAR_NAME_EDM "edmap_names"    /* names of edge    maps     */
#define VAR_NAME_FAM "famap_names"    /* names of face    maps     */
#define VAR_NAME_NM "nmap_names"      /* names of node    maps     */
#define VAR_NAME_ED_BLK "ed_names"    /* names of edge    blocks   */
#define VAR_NAME_FA_BLK "fa_names"    /* names of face    blocks   */
#define VAR_NAME_ES "es_names"        /* names of edge    sets     */
#define VAR_NAME_FS "fs_names"        /* names of face    sets     */
#define VAR_NAME_ELS "els_names"      /* names of element sets     */
#define VAR_STAT_EL_BLK "eb_status"   /* element block status      */
#define VAR_STAT_ECONN "econn_status" /* element block edge status */
#define VAR_STAT_FCONN "fconn_status" /* element block face status */
#define VAR_STAT_ED_BLK "ed_status"   /* edge    block status      */
#define VAR_STAT_FA_BLK "fa_status"   /* face    block status      */
#define VAR_ID_EL_BLK "eb_prop1"      /* element block ids props   */
#define VAR_ID_ED_BLK "ed_prop1"      /* edge    block ids props   */
#define VAR_ID_FA_BLK "fa_prop1"      /* face    block ids props   */
#define ATT_NAME_ELB "elem_type"      /* element type names for    */
                                      /*   each element block      */
#define DIM_NUM_EL_IN_BLK(num) ex_catstr("num_el_in_blk", num)
/* # of elements in element  */
/*   block num               */
#define DIM_NUM_NOD_PER_EL(num) ex_catstr("num_nod_per_el", num)
/* # of nodes per element in */
/*   element block num       */
#define DIM_NUM_ATT_IN_BLK(num) ex_catstr("num_att_in_blk", num)
/* # of attributes in element*/
/*   block num               */
#define DIM_NUM_ED_IN_EBLK(num) ex_catstr("num_ed_in_blk", num)
/* # of edges in edge        */
/*   block num               */
#define DIM_NUM_NOD_PER_ED(num) ex_catstr("num_nod_per_ed", num)
/* # of nodes per edge in    */
/*   edge block num          */
#define DIM_NUM_EDG_PER_EL(num) ex_catstr("num_edg_per_el", num)
/* # of edges per element in */
/*   element block num       */
#define DIM_NUM_ATT_IN_EBLK(num) ex_catstr("num_att_in_eblk", num)
/* # of attributes in edge   */
/*   block num               */
#define DIM_NUM_FA_IN_FBLK(num) ex_catstr("num_fa_in_blk", num)
/* # of faces in face        */
/*   block num               */
#define DIM_NUM_NOD_PER_FA(num) ex_catstr("num_nod_per_fa", num)
/* # of nodes per face in    */
/*   face block num          */
#define DIM_NUM_FAC_PER_EL(num) ex_catstr("num_fac_per_el", num)
/* # of faces per element in */
/*   element block num       */
#define DIM_NUM_ATT_IN_FBLK(num) ex_catstr("num_att_in_fblk", num)
/* # of attributes in face   */
/*   block num               */
#define VAR_CONN(num) ex_catstr("connect", num)
/* element connectivity for  */
/*   element block num       */
#define VAR_EBEPEC(num) ex_catstr("ebepecnt", num)
/* array containing number of entity per */
/*  entity for n-sided face/element blocks */
#define VAR_ATTRIB(num) ex_catstr("attrib", num)
/* list of attributes for    */
/*   element block num       */
#define VAR_NAME_ATTRIB(num) ex_catstr("attrib_name", num)
/* list of attribute names   */
/* for element block num     */
#define VAR_EB_PROP(num) ex_catstr("eb_prop", num)
/* list of the numth property*/
/*   for all element blocks  */
#define VAR_ECONN(num) ex_catstr("edgconn", num)
/* edge connectivity for     */
/*   element block num       */
#define VAR_EBCONN(num) ex_catstr("ebconn", num)
/* edge connectivity for     */
/*   edge block num          */
#define VAR_EATTRIB(num) ex_catstr("eattrb", num)
/* list of attributes for    */
/*   edge block num          */
#define VAR_NAME_EATTRIB(num) ex_catstr("eattrib_name", num)
/* list of attribute names   */
/* for edge block num        */
#define VAR_NATTRIB "nattrb"
#define VAR_NAME_NATTRIB "nattrib_name"
#define DIM_NUM_ATT_IN_NBLK "num_att_in_nblk"

#define VAR_NSATTRIB(num) ex_catstr("nsattrb", num)
#define VAR_NAME_NSATTRIB(num) ex_catstr("nsattrib_name", num)
#define DIM_NUM_ATT_IN_NS(num) ex_catstr("num_att_in_ns", num)

#define VAR_SSATTRIB(num) ex_catstr("ssattrb", num)
#define VAR_NAME_SSATTRIB(num) ex_catstr("ssattrib_name", num)
#define DIM_NUM_ATT_IN_SS(num) ex_catstr("num_att_in_ss", num)

#define VAR_ESATTRIB(num) ex_catstr("esattrb", num)
#define VAR_NAME_ESATTRIB(num) ex_catstr("esattrib_name", num)
#define DIM_NUM_ATT_IN_ES(num) ex_catstr("num_att_in_es", num)

#define VAR_FSATTRIB(num) ex_catstr("fsattrb", num)
#define VAR_NAME_FSATTRIB(num) ex_catstr("fsattrib_name", num)
#define DIM_NUM_ATT_IN_FS(num) ex_catstr("num_att_in_fs", num)

#define VAR_ELSATTRIB(num) ex_catstr("elsattrb", num)
#define VAR_NAME_ELSATTRIB(num) ex_catstr("elsattrib_name", num)
#define DIM_NUM_ATT_IN_ELS(num) ex_catstr("num_att_in_els", num)

#define VAR_ED_PROP(num) ex_catstr("ed_prop", num)
/* list of the numth property*/
/*   for all edge blocks     */
#define VAR_FCONN(num) ex_catstr("facconn", num)
/* face connectivity for     */
/*   element block num       */
#define VAR_FBCONN(num) ex_catstr("fbconn", num)
/* face connectivity for     */
/*   face block num          */
#define VAR_FBEPEC(num) ex_catstr("fbepecnt", num)
/* array containing number of entity per */
/*  entity for n-sided face/element blocks */
#define VAR_FATTRIB(num) ex_catstr("fattrb", num)
/* list of attributes for    */
/*   face block num          */
#define VAR_NAME_FATTRIB(num) ex_catstr("fattrib_name", num)
/* list of attribute names   */
/* for face block num        */
#define VAR_FA_PROP(num) ex_catstr("fa_prop", num)
/* list of the numth property*/
/*   for all face blocks     */
#define ATT_PROP_NAME "name"       /* name attached to element  */
                                   /*   block, node set, side   */
                                   /*   set, element map, or    */
                                   /*   map properties          */
#define VAR_MAP "elem_map"         /* element order map         */
                                   /* obsolete, replaced by     */
                                   /* VAR_ELEM_MAP(num)         */
#define DIM_NUM_SS "num_side_sets" /* # of side sets            */
#define VAR_SS_STAT "ss_status"    /* side set status           */
#define VAR_SS_IDS "ss_prop1"      /* side set id properties    */
#define DIM_NUM_SIDE_SS(num) ex_catstr("num_side_ss", num)
/* # of sides in side set num*/
#define DIM_NUM_DF_SS(num) ex_catstr("num_df_ss", num)
/* # of distribution factors */
/* in side set num           */
/*#define DIM_NUM_NOD_SS(num)   ex_catstr("num_nod_ss",num) *** obsolete *** */
/* # of nodes in side set num*/
#define VAR_FACT_SS(num) ex_catstr("dist_fact_ss", num)
/* the distribution factors  */
/*   for each node in side   */
/*   set num                 */
#define VAR_ELEM_SS(num) ex_catstr("elem_ss", num)
/* list of elements in side  */
/*   set num                 */
#define VAR_SIDE_SS(num) ex_catstr("side_ss", num)
/* list of sides in side set */
#define VAR_SS_PROP(num) ex_catstr("ss_prop", num)
/* list of the numth property*/
/*   for all side sets       */
#define DIM_NUM_ES "num_edge_sets" /* # of edge sets            */
#define VAR_ES_STAT "es_status"    /* edge set status           */
#define VAR_ES_IDS "es_prop1"      /* edge set id properties    */
#define DIM_NUM_EDGE_ES(num) ex_catstr("num_edge_es", num)
/* # of edges in edge set num*/
#define DIM_NUM_DF_ES(num) ex_catstr("num_df_es", num)
/* # of distribution factors */
/* in edge set num           */
/*#define DIM_NUM_NOD_ES(num)    ex_catstr("num_nod_es",num) *** obsolete *** */
/* # of nodes in edge set num*/
#define VAR_FACT_ES(num) ex_catstr("dist_fact_es", num)
/* the distribution factors  */
/*   for each node in edge   */
/*   set num                 */
#define VAR_EDGE_ES(num) ex_catstr("edge_es", num)
/* list of edges in edge     */
/*   set num                 */
#define VAR_ORNT_ES(num) ex_catstr("ornt_es", num)
/* list of orientations in   */
/*  the edge set.            */
#define VAR_ES_PROP(num) ex_catstr("es_prop", num)
/* list of the numth property*/
/*   for all edge sets       */
#define DIM_NUM_FS "num_face_sets" /* # of face sets            */
#define VAR_FS_STAT "fs_status"    /* face set status           */
#define VAR_FS_IDS "fs_prop1"      /* face set id properties    */
#define DIM_NUM_FACE_FS(num) ex_catstr("num_face_fs", num)
/* # of faces in side set num*/
#define DIM_NUM_DF_FS(num) ex_catstr("num_df_fs", num)
/* # of distribution factors */
/* in face set num           */
/*#define DIM_NUM_NOD_FS(num)    ex_catstr("num_nod_ss",num) *** obsolete *** */
/* # of nodes in face set num*/
#define VAR_FACT_FS(num) ex_catstr("dist_fact_fs", num)
/* the distribution factors  */
/*   for each node in face   */
/*   set num                 */
#define VAR_FACE_FS(num) ex_catstr("face_fs", num)
/* list of elements in face  */
/*   set num                 */
#define VAR_ORNT_FS(num) ex_catstr("ornt_fs", num)
/* list of sides in side set */
#define VAR_FS_PROP(num) ex_catstr("fs_prop", num)
/* list of the numth property*/
/*   for all face sets       */
#define DIM_NUM_ELS "num_elem_sets" /* # of elem sets            */
#define DIM_NUM_ELE_ELS(num) ex_catstr("num_ele_els", num)
/* # of elements in elem set */
/*   num                     */
#define DIM_NUM_DF_ELS(num) ex_catstr("num_df_els", num)
/* # of distribution factors */
/* in element set num        */
#define VAR_ELS_STAT "els_status" /* elem set status           */
#define VAR_ELS_IDS "els_prop1"   /* elem set id properties    */
#define VAR_ELEM_ELS(num) ex_catstr("elem_els", num)
/* list of elements in elem  */
/*   set num                 */
#define VAR_FACT_ELS(num) ex_catstr("dist_fact_els", num)
/* list of distribution      */
/*   factors in elem set num */
#define VAR_ELS_PROP(num) ex_catstr("els_prop", num)
/* list of the numth property*/
/*   for all elem sets       */
#define DIM_NUM_NS "num_node_sets" /* # of node sets            */
#define DIM_NUM_NOD_NS(num) ex_catstr("num_nod_ns", num)
/* # of nodes in node set    */
/*   num                     */
#define DIM_NUM_DF_NS(num) ex_catstr("num_df_ns", num)
/* # of distribution factors */
/* in node set num           */
#define VAR_NS_STAT "ns_status" /* node set status           */
#define VAR_NS_IDS "ns_prop1"   /* node set id properties    */
#define VAR_NODE_NS(num) ex_catstr("node_ns", num)
/* list of nodes in node set */
/*   num                     */
#define VAR_FACT_NS(num) ex_catstr("dist_fact_ns", num)
/* list of distribution      */
/*   factors in node set num */
#define VAR_NS_PROP(num) ex_catstr("ns_prop", num)
/* list of the numth property*/
/*   for all node sets       */
#define DIM_NUM_QA "num_qa_rec"         /* # of QA records           */
#define VAR_QA_TITLE "qa_records"       /* QA records                */
#define DIM_NUM_INFO "num_info"         /* # of information records  */
#define VAR_INFO "info_records"         /* information records       */
#define VAR_HIS_TIME "time_hist"        /* obsolete                  */
#define VAR_WHOLE_TIME "time_whole"     /* simulation times for whole*/
                                        /*   time steps              */
#define VAR_ELEM_TAB "elem_var_tab"     /* element variable truth    */
                                        /*   table                   */
#define VAR_EBLK_TAB "edge_var_tab"     /* edge variable truth table */
#define VAR_FBLK_TAB "face_var_tab"     /* face variable truth table */
#define VAR_ELSET_TAB "elset_var_tab"   /* elemset variable truth    */
                                        /*   table                   */
#define VAR_SSET_TAB "sset_var_tab"     /* sideset variable truth    */
                                        /*   table                   */
#define VAR_FSET_TAB "fset_var_tab"     /* faceset variable truth    */
                                        /*   table                   */
#define VAR_ESET_TAB "eset_var_tab"     /* edgeset variable truth    */
                                        /*   table                   */
#define VAR_NSET_TAB "nset_var_tab"     /* nodeset variable truth    */
                                        /*   table                   */
#define DIM_NUM_GLO_VAR "num_glo_var"   /* # of global variables     */
#define VAR_NAME_GLO_VAR "name_glo_var" /* names of global variables */
#define VAR_GLO_VAR "vals_glo_var"      /* values of global variables*/
#define DIM_NUM_NOD_VAR "num_nod_var"   /* # of nodal variables      */
#define VAR_NAME_NOD_VAR "name_nod_var" /* names of nodal variables  */
#define VAR_NOD_VAR "vals_nod_var"      /* values of nodal variables */
#define VAR_NOD_VAR_NEW(num) ex_catstr("vals_nod_var", num)
/* values of nodal variables */
#define DIM_NUM_ELE_VAR "num_elem_var"   /* # of element variables    */
#define VAR_NAME_ELE_VAR "name_elem_var" /* names of element variables*/
#define VAR_ELEM_VAR(num1, num2) ex_catstr2("vals_elem_var", num1, "eb", num2)
/* values of element variable*/
/*   num1 in element block   */
/*   num2                    */
#define DIM_NUM_EDG_VAR "num_edge_var"   /* # of edge variables       */
#define VAR_NAME_EDG_VAR "name_edge_var" /* names of edge variables   */
#define VAR_EDGE_VAR(num1, num2) ex_catstr2("vals_edge_var", num1, "eb", num2)
/* values of edge variable   */
/*   num1 in edge block num2 */
#define DIM_NUM_FAC_VAR "num_face_var"   /* # of face variables       */
#define VAR_NAME_FAC_VAR "name_face_var" /* names of face variables   */
#define VAR_FACE_VAR(num1, num2) ex_catstr2("vals_face_var", num1, "fb", num2)
/* values of face variable   */
/*   num1 in face block num2 */

#define DIM_NUM_NSET_VAR "num_nset_var"   /* # of nodeset variables    */
#define VAR_NAME_NSET_VAR "name_nset_var" /* names of nodeset variables*/
#define VAR_NS_VAR(num1, num2) ex_catstr2("vals_nset_var", num1, "ns", num2)
/* values of nodeset variable*/
/*   num1 in nodeset num2    */
#define DIM_NUM_ESET_VAR "num_eset_var"   /* # of edgeset variables    */
#define VAR_NAME_ESET_VAR "name_eset_var" /* names of edgeset variables*/
#define VAR_ES_VAR(num1, num2) ex_catstr2("vals_eset_var", num1, "es", num2)
/* values of edgeset variable*/
/*   num1 in edgeset num2    */
#define DIM_NUM_FSET_VAR "num_fset_var"   /* # of faceset variables    */
#define VAR_NAME_FSET_VAR "name_fset_var" /* names of faceset variables*/
#define VAR_FS_VAR(num1, num2) ex_catstr2("vals_fset_var", num1, "fs", num2)
/* values of faceset variable*/
/*   num1 in faceset num2    */
#define DIM_NUM_SSET_VAR "num_sset_var"   /* # of sideset variables    */
#define VAR_NAME_SSET_VAR "name_sset_var" /* names of sideset variables*/
#define VAR_SS_VAR(num1, num2) ex_catstr2("vals_sset_var", num1, "ss", num2)
/* values of sideset variable*/
/*   num1 in sideset num2    */
#define DIM_NUM_ELSET_VAR "num_elset_var"   /* # of element set variables*/
#define VAR_NAME_ELSET_VAR "name_elset_var" /* names of elemset variables*/
#define VAR_ELS_VAR(num1, num2) ex_catstr2("vals_elset_var", num1, "es", num2)
/* values of elemset variable*/
/*   num1 in elemset num2    */

#define DIM_NUM_HIS_VAR "num_his_var"   /* obsolete                  */
#define VAR_NAME_HIS_VAR "name_his_var" /* obsolete                  */
#define VAR_HIS_VAR "vals_his_var"      /* obsolete                  */
#define DIM_STR "len_string"            /* general dimension of      */
                                        /*   length MAX_STR_LENGTH   */
                                        /*   used for some string lengths   */
#define DIM_STR_NAME "len_name"         /* general dimension of      */
                                        /*   length MAX_NAME_LENGTH  */
                                        /*   used for name lengths   */
#define DIM_LIN "len_line"              /* general dimension of      */
                                        /*   length MAX_LINE_LENGTH  */
                                        /*   used for long strings   */
#define DIM_N4 "four"                   /* general dimension of      */
                                        /*   length 4                */
#define DIM_TIME "time_step"            /* unlimited (expandable)    */
                                        /*   dimension for time steps*/
#define DIM_HTIME "hist_time_step"      /* obsolete                  */
#define VAR_ELEM_NUM_MAP "elem_num_map" /* element numbering map     */
                                        /* obsolete, replaced by     */
                                        /* VAR_ELEM_MAP(num)         */
#define VAR_FACE_NUM_MAP "face_num_map" /* face numbering map     */
                                        /* obsolete, replaced by     */
                                        /* VAR_FACE_MAP(num)         */
#define VAR_EDGE_NUM_MAP "edge_num_map" /* edge numbering map     */
                                        /* obsolete, replaced by     */
                                        /* VAR_EDGE_MAP(num)         */
#define VAR_NODE_NUM_MAP "node_num_map" /* node numbering map        */
                                        /* obsolete, replaced by     */
                                        /* VAR_NODE_MAP(num)         */
#define DIM_NUM_EM "num_elem_maps"      /* # of element maps         */
#define VAR_ELEM_MAP(num) ex_catstr("elem_map", num)
/* the numth element map     */
#define VAR_EM_PROP(num) ex_catstr("em_prop", num)
/* list of the numth property*/
/*   for all element maps    */
#define DIM_NUM_EDM "num_edge_maps" /* # of edge maps            */
#define VAR_EDGE_MAP(num) ex_catstr("edge_map", num)
/* the numth edge map        */
#define VAR_EDM_PROP(num) ex_catstr("edm_prop", num)
/* list of the numth property*/
/*   for all edge maps       */
#define DIM_NUM_FAM "num_face_maps" /* # of face maps            */
#define VAR_FACE_MAP(num) ex_catstr("face_map", num)
/* the numth face map        */
#define VAR_FAM_PROP(num) ex_catstr("fam_prop", num)
/* list of the numth property*/
/*   for all face maps       */
#define DIM_NUM_NM "num_node_maps" /* # of node maps            */
#define VAR_NODE_MAP(num) ex_catstr("node_map", num)
/* the numth node map        */
#define VAR_NM_PROP(num) ex_catstr("nm_prop", num)
/* list of the numth property*/
/*   for all node maps       */

#define DIM_NUM_CFRAMES "num_cframes"
#define DIM_NUM_CFRAME9 "num_cframes_9"
#define VAR_FRAME_COORDS "frame_coordinates"
#define VAR_FRAME_IDS "frame_ids"
#define VAR_FRAME_TAGS "frame_tags"

#define VAR_ELBLK_IDS_GLOBAL "el_blk_ids_global"
#define VAR_ELBLK_CNT_GLOBAL "el_blk_cnt_global"
#define VAR_NS_IDS_GLOBAL "ns_ids_global"
#define VAR_NS_NODE_CNT_GLOBAL "ns_node_cnt_global"
#define VAR_NS_DF_CNT_GLOBAL "ns_df_cnt_global"
#define VAR_SS_IDS_GLOBAL "ss_ids_global"
#define VAR_SS_SIDE_CNT_GLOBAL "ss_side_cnt_global"
#define VAR_SS_DF_CNT_GLOBAL "ss_df_cnt_global"
#define VAR_FILE_TYPE "nem_ftype"
#define VAR_COMM_MAP "comm_map"
#define VAR_NODE_MAP_INT "node_mapi"
#define VAR_NODE_MAP_INT_IDX "node_mapi_idx"
#define VAR_NODE_MAP_BOR "node_mapb"
#define VAR_NODE_MAP_BOR_IDX "node_mapb_idx"
#define VAR_NODE_MAP_EXT "node_mape"
#define VAR_NODE_MAP_EXT_IDX "node_mape_idx"
#define VAR_ELEM_MAP_INT "elem_mapi"
#define VAR_ELEM_MAP_INT_IDX "elem_mapi_idx"
#define VAR_ELEM_MAP_BOR "elem_mapb"
#define VAR_ELEM_MAP_BOR_IDX "elem_mapb_idx"
#define VAR_INT_N_STAT "int_n_stat"
#define VAR_BOR_N_STAT "bor_n_stat"
#define VAR_EXT_N_STAT "ext_n_stat"
#define VAR_INT_E_STAT "int_e_stat"
#define VAR_BOR_E_STAT "bor_e_stat"
#define VAR_N_COMM_IDS "n_comm_ids"
#define VAR_N_COMM_STAT "n_comm_stat"
#define VAR_N_COMM_INFO_IDX "n_comm_info_idx"
#define VAR_E_COMM_IDS "e_comm_ids"
#define VAR_E_COMM_STAT "e_comm_stat"
#define VAR_E_COMM_INFO_IDX "e_comm_info_idx"
#define VAR_N_COMM_NIDS "n_comm_nids"
#define VAR_N_COMM_PROC "n_comm_proc"
#define VAR_N_COMM_DATA_IDX "n_comm_data_idx"
#define VAR_E_COMM_EIDS "e_comm_eids"
#define VAR_E_COMM_SIDS "e_comm_sids"
#define VAR_E_COMM_PROC "e_comm_proc"
#define VAR_E_COMM_DATA_IDX "e_comm_data_idx"

#define DIM_NUM_INT_NODES "num_int_node"
#define DIM_NUM_BOR_NODES "num_bor_node"
#define DIM_NUM_EXT_NODES "num_ext_node"
#define DIM_NUM_INT_ELEMS "num_int_elem"
#define DIM_NUM_BOR_ELEMS "num_bor_elem"
#define DIM_NUM_PROCS "num_processors"
#define DIM_NUM_PROCS_F "num_procs_file"
#define DIM_NUM_NODES_GLOBAL "num_nodes_global"
#define DIM_NUM_ELEMS_GLOBAL "num_elems_global"
#define DIM_NUM_NS_GLOBAL "num_ns_global"
#define DIM_NUM_SS_GLOBAL "num_ss_global"
#define DIM_NUM_ELBLK_GLOBAL "num_el_blk_global"
#define DIM_NUM_N_CMAPS "num_n_cmaps"
#define DIM_NUM_E_CMAPS "num_e_cmaps"
#define DIM_NCNT_CMAP "ncnt_cmap"
#define DIM_ECNT_CMAP "ecnt_cmap"

enum ex_element_type {
  EX_EL_UNK          = -1, /**< unknown entity */
  EX_EL_NULL_ELEMENT = 0,
  EX_EL_TRIANGLE     = 1,  /**< Triangle entity */
  EX_EL_QUAD         = 2,  /**< Quad entity */
  EX_EL_HEX          = 3,  /**< Hex entity */
  EX_EL_WEDGE        = 4,  /**< Wedge entity */
  EX_EL_TETRA        = 5,  /**< Tetra entity */
  EX_EL_TRUSS        = 6,  /**< Truss entity */
  EX_EL_BEAM         = 7,  /**< Beam entity */
  EX_EL_SHELL        = 8,  /**< Shell entity */
  EX_EL_SPHERE       = 9,  /**< Sphere entity */
  EX_EL_CIRCLE       = 10, /**< Circle entity */
  EX_EL_TRISHELL     = 11, /**< Triangular Shell entity */
  EX_EL_PYRAMID      = 12  /**< Pyramid entity */
};
typedef enum ex_element_type ex_element_type;

enum ex_coordinate_frame_type { EX_CF_RECTANGULAR = 1, EX_CF_CYLINDRICAL = 2, EX_CF_SPHERICAL = 3 };
typedef enum ex_coordinate_frame_type ex_coordinate_frame_type;

/* Internal structure declarations */

struct ex_file_item
{
  int     file_id;
  nc_type netcdf_type_code;
  int     int64_status;
  int     maximum_name_length;
  int     time_varid; /* Store to avoid lookup each timestep */
  unsigned int
               compression_level : 4; /* 0 (disabled) to 9 (maximum) compression level; netcdf-4 only */
  unsigned int user_compute_wordsize : 1; /* 0 for 4 byte or 1 for 8 byte reals */
  unsigned int shuffle : 1;               /* 1 true, 0 false */
  unsigned int
                       file_type : 2; /* 0 - classic, 1 -- 64 bit classic, 2 --netcdf4,  3 --netcdf4 classic */
  unsigned int         is_parallel : 1; /* 1 true, 0 false */
  unsigned int         is_mpiio : 1;    /* 1 true, 0 false */
  unsigned int         is_pnetcdf : 1;  /* 1 true, 0 false */
  unsigned int         has_nodes : 1;   /* for input only at this time */
  unsigned int         has_edges : 1;   /* for input only at this time */
  unsigned int         has_faces : 1;   /* for input only at this time */
  unsigned int         has_elems : 1;   /* for input only at this time */
  struct ex_file_item *next;
};

struct elem_blk_parm
{
  char            elem_type[33];
  int64_t         elem_blk_id;
  int64_t         num_elem_in_blk;
  int             num_nodes_per_elem;
  int             num_sides;
  int             num_nodes_per_side[6];
  int             num_attr;
  int64_t         elem_ctr;
  ex_element_type elem_type_val;
};

struct list_item
{ /* for use with ex_get_file_item */
  int               exo_id;
  int               value;
  struct list_item *next;
};

struct obj_stats
{
  int64_t *         id_vals;
  int *             stat_vals;
  size_t            num;
  int               exoid;
  char              valid_ids;
  char              valid_stat;
  char              sequential;
  struct obj_stats *next;
};

void ex_iqsort(int v[], int iv[], int N);
void ex_iqsort64(int64_t v[], int64_t iv[], int64_t N);

char *ex_catstr(const char * /*string*/, int /*num*/);
char *ex_catstr2(const char * /*string1*/, int /*num1*/, const char * /*string2*/, int /*num2*/);
char *ex_dim_num_entries_in_object(ex_entity_type /*obj_type*/, int /*idx*/);
char *ex_dim_num_objects(ex_entity_type obj_type);
char *ex_name_var_of_object(ex_entity_type /*obj_type*/, int /*i*/, int /*j*/);
char *ex_name_of_map(ex_entity_type /*map_type*/, int /*map_index*/);

int  ex_conv_ini(int exoid, int *comp_wordsize, int *io_wordsize, int file_wordsize,
                 int int64_status, int is_parallel, int is_mpiio, int is_pnetcdf);
void ex_conv_exit(int exoid);

nc_type nc_flt_code(int exoid);
int     ex_comp_ws(int exoid);
int     ex_get_cpu_ws(void);
int     ex_is_parallel(int exoid);

struct list_item **ex_get_counter_list(ex_entity_type obj_type);
int                ex_get_file_item(int /*exoid*/, struct list_item ** /*list_ptr*/);
int                ex_inc_file_item(int /*exoid*/, struct list_item ** /*list_ptr*/);
void               ex_rm_file_item(int /*exoid*/, struct list_item ** /*list_ptr*/);

extern struct obj_stats *exoII_eb;
extern struct obj_stats *exoII_ed;
extern struct obj_stats *exoII_fa;
extern struct obj_stats *exoII_ns;
extern struct obj_stats *exoII_es;
extern struct obj_stats *exoII_fs;
extern struct obj_stats *exoII_ss;
extern struct obj_stats *exoII_els;
extern struct obj_stats *exoII_em;
extern struct obj_stats *exoII_edm;
extern struct obj_stats *exoII_fam;
extern struct obj_stats *exoII_nm;

struct ex_file_item *ex_find_file_item(int exoid);
struct ex_file_item *ex_add_file_item(int exoid);
struct obj_stats *   ex_get_stat_ptr(int exoid, struct obj_stats **obj_ptr);

void ex_rm_stat_ptr(int exoid, struct obj_stats **obj_ptr);

void ex_compress_variable(int exoid, int varid, int type);
int  ex_id_lkup(int exoid, ex_entity_type id_type, ex_entity_id num);
void ex_check_valid_file_id(int         exoid,
                            const char *func); /** Abort if exoid does not refer to valid file */
int  ex_check_file_type(const char *path, int *type);
int ex_get_dimension(int exoid, const char *DIMENSION, const char *label, size_t *count, int *dimid,
                     const char *routine);

int ex_get_nodal_var_int(int exoid, int time_step, int nodal_var_index, int64_t num_nodes,
                         void *nodal_var_vals);

int ex_put_nodal_var_int(int exoid, int time_step, int nodal_var_index, int64_t num_nodes,
                         const void *nodal_var_vals);

int ex_get_nodal_var_time_int(int exoid, int nodal_var_index, int64_t node_number,
                              int beg_time_step, int end_time_step, void *nodal_var_vals);

int ex_get_partial_nodal_var_int(int exoid, int time_step, int nodal_var_index, int64_t start_node,
                                 int64_t num_nodes, void *var_vals);

int ex_put_partial_nodal_var_int(int exoid, int time_step, int nodal_var_index, int64_t start_node,
                                 int64_t num_nodes, const void *nodal_var_vals);
int ex_get_glob_vars_int(int exoid, int time_step, int num_glob_vars, void *glob_var_vals);

int ex_get_glob_var_time_int(int exoid, int glob_var_index, int beg_time_step, int end_time_step,
                             void *glob_var_vals);

int  ex_get_name_internal(int exoid, int varid, size_t index, char *name, int name_size,
                          ex_entity_type obj_type, const char *routine);
int  ex_get_names_internal(int exoid, int varid, size_t num_entity, char **names,
                           ex_entity_type obj_type, const char *routine);
int  ex_put_name_internal(int exoid, int varid, size_t index, const char *name,
                          ex_entity_type obj_type, const char *subtype, const char *routine);
int  ex_put_names_internal(int exoid, int varid, size_t num_entity, char **names,
                           ex_entity_type obj_type, const char *subtype, const char *routine);
void ex_trim_internal(char *name);
void ex_update_max_name_length(int exoid, int length);
int  ex_leavedef(int         exoid,    /* NemesisI file ID         */
                 const char *call_rout /* Name of calling function */
 );

int ex_int_get_block_param(int exoid, ex_entity_id id, int ndim,
                           struct elem_blk_parm *elem_blk_parm);

int ex_get_file_type(int   exoid, /* NetCDF/Exodus file ID */
                     char *ftype  /* Nemesis file type */
);

int ex_put_nemesis_version(int exoid); /* NetCDF/Exodus file ID */

int ne_check_file_version(int neid /* NetCDF/Exodus file ID */
);

int ne_id_lkup(int          exoid,       /* NetCDF/Exodus file ID */
               const char * ne_var_name, /* Nemesis variable name */
               int64_t *    idx,         /* index variable for variable, length 2 */
               ex_entity_id ne_var_id    /* NetCDF variable ID */
);

/**
 * For output databases, the maximum length of any entity, variable,
 * property, attribute, or coordinate name to be written (not
 * including the NULL terminator). If a name is longer than this
 * value, a warning message will be output to stderr and the name
 * will be truncated.  Must be set (via call to
 * 'ex_set_max_name_length(exoid, int len)' prior to calling ex_create.
 *
 * For input databases, the size of the name arrays that the client
 * code will be passing to API routines that retrieve names (not
 * including the NULL terminator). This defaults to 32 for
 * compatibility with older clients. The value used at the time of
 * creation of the database can be queried by ex_inquire with the
 * EX_INQ_DB_MAX_NAME_LENGTH argument. The current value for this
 * variable can be queried with EX_INQ_CUR_MAX_NAME_LENGTH argument.
 *
 * Note that this is a global setting for all databases. If you are
 * accessing multiple databases, they will all use the same value.
 */
extern int ex_default_max_name_length;
#ifdef __cplusplus
}
#endif

#endif
