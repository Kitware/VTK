/*
 * Copyright (c) 2005 Sandia Corporation. Under the terms of Contract
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
* exodusII_int.h - ExodusII header file for internal Exodus call use only
*
*/

#ifndef EXODUS_II_INT_HDR
#define EXODUS_II_INT_HDR

#include "vtk_netcdf.h"

#ifndef __APPLE__
#if defined __STDC__ || defined __cplusplus
#include <stdlib.h>
#endif
#endif

#ifdef _MSC_VER
#  pragma warning ( disable : 4127 )
#  pragma warning ( disable : 4706 )
#  pragma warning ( disable : 4701 )
#endif

#if defined(__BORLANDC__)
#pragma warn -8004 /* "assigned a value that is never used" */
#endif


#include <stdio.h>

#define MAX_VAR_NAME_LENGTH     32   /**< Internal use only */

/* this should be defined in ANSI C and C++, but just in case ... */
#ifndef NULL
#define NULL 0
#endif

/* Default "filesize" for newly created files.
 * Set to 0 for normal filesize setting.
 * Set to 1 for EXODUS_LARGE_MODEL setting to be the default
 */
#define EXODUS_DEFAULT_SIZE 1

/* Exodus error return codes - function return values:                      */
#define EX_FATAL        -1      /* fatal error flag def                     */
#define EX_NOERR         0      /* no error flag def                        */
#define EX_WARN          1      /* warning flag def                         */

/*
 * This file contains defined constants that are used internally in the
 * EXODUS II API.
 *
 * The first group of constants refer to netCDF variables, attributes, or 
 * dimensions in which the EXODUS II data are stored.  Using the defined 
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
#define ATT_FILE_TYPE           "type"          /* obsolete                  */
#define ATT_TITLE               "title"         /* the database title        */
#define ATT_API_VERSION         "api_version"   /* the EXODUS II api vers #   */
#define ATT_API_VERSION_BLANK   "api version"   /* the EXODUS II api vers #   */
                                                /*  used for db version 2.01 */
                                                /*  and earlier              */
#define ATT_VERSION             "version"       /* the EXODUS II file vers # */
#define ATT_FILESIZE            "file_size"     /* 1=large, 0=normal */
#define ATT_FLT_WORDSIZE        "floating_point_word_size"
                                                /* word size of floating     */
                                                /* point numbers in file     */
#define ATT_FLT_WORDSIZE_BLANK  "floating point word size"
                                                /* word size of floating     */
                                                /* point numbers in file     */
                                                /* used for db version 2.01  */
                                                /* and earlier               */
#define ATT_MAX_NAME_LENGTH "maximum_name_length"

#define DIM_NUM_NODES           "num_nodes"     /* # of nodes                */
#define DIM_NUM_DIM             "num_dim"       /* # of dimensions; 2- or 3-d*/
#define DIM_NUM_EDGE            "num_edge"      /* # of edges (over all blks)*/
#define DIM_NUM_FACE            "num_face"      /* # of faces (over all blks)*/
#define DIM_NUM_ELEM            "num_elem"      /* # of elements             */
#define DIM_NUM_EL_BLK          "num_el_blk"    /* # of element blocks       */
#define DIM_NUM_ED_BLK          "num_ed_blk"    /* # of edge blocks          */
#define DIM_NUM_FA_BLK          "num_fa_blk"    /* # of face blocks          */
#define VAR_COORD               "coord"         /* nodal coordinates         */
#define VAR_COORD_X             "coordx"        /* X-dimension coordinate    */
#define VAR_COORD_Y             "coordy"        /* Y-dimension coordinate    */
#define VAR_COORD_Z             "coordz"        /* Z-dimension coordinate    */
#define VAR_NAME_COOR           "coor_names"    /* names of coordinates      */
#define VAR_NAME_EL_BLK         "eb_names"      /* names of element blocks   */
#define VAR_NAME_NS             "ns_names"      /* names of node sets        */
#define VAR_NAME_SS             "ss_names"      /* names of side sets        */
#define VAR_NAME_EM             "emap_names"    /* names of element maps     */
#define VAR_NAME_EDM            "edmap_names"   /* names of edge    maps     */
#define VAR_NAME_FAM            "famap_names"   /* names of face    maps     */
#define VAR_NAME_NM             "nmap_names"    /* names of node    maps     */
#define VAR_NAME_ED_BLK         "ed_names"      /* names of edge    blocks   */
#define VAR_NAME_FA_BLK         "fa_names"      /* names of face    blocks   */
#define VAR_NAME_ES             "es_names"      /* names of edge    sets     */
#define VAR_NAME_FS             "fs_names"      /* names of face    sets     */
#define VAR_NAME_ELS            "els_names"     /* names of element sets     */
#define VAR_STAT_EL_BLK         "eb_status"     /* element block status      */
#define VAR_STAT_ECONN          "econn_status"  /* element block edge status */
#define VAR_STAT_FCONN          "fconn_status"  /* element block face status */
#define VAR_STAT_ED_BLK         "ed_status"     /* edge    block status      */
#define VAR_STAT_FA_BLK         "fa_status"     /* face    block status      */
#define VAR_ID_EL_BLK           "eb_prop1"      /* element block ids props   */
#define VAR_ID_ED_BLK           "ed_prop1"      /* edge    block ids props   */
#define VAR_ID_FA_BLK           "fa_prop1"      /* face    block ids props   */
#define ATT_NAME_ELB            "elem_type"     /* element type names for    */
                                                /*   each element block      */
#define DIM_NUM_EL_IN_BLK(num)  ex_catstr("num_el_in_blk",num)
                                                /* # of elements in element  */
                                                /*   block num               */
#define DIM_NUM_NOD_PER_EL(num) ex_catstr("num_nod_per_el",num)
                                                /* # of nodes per element in */
                                                /*   element block num       */
#define DIM_NUM_ATT_IN_BLK(num) ex_catstr("num_att_in_blk",num)
                                                /* # of attributes in element*/
                                                /*   block num               */
#define DIM_NUM_ED_IN_EBLK(num) ex_catstr("num_ed_in_blk",num)
                                                /* # of edges in edge        */
                                                /*   block num               */
#define DIM_NUM_NOD_PER_ED(num)  ex_catstr("num_nod_per_ed",num)
                                                /* # of nodes per edge in    */
                                                /*   edge block num          */
#define DIM_NUM_EDG_PER_EL(num)  ex_catstr("num_edg_per_el",num)
                                                /* # of edges per element in */
                                                /*   element block num       */
#define DIM_NUM_ATT_IN_EBLK(num) ex_catstr("num_att_in_eblk",num)
                                                /* # of attributes in edge   */
                                                /*   block num               */
#define DIM_NUM_FA_IN_FBLK(num)  ex_catstr("num_fa_in_blk",num)
                                                /* # of faces in face        */
                                                /*   block num               */
#define DIM_NUM_NOD_PER_FA(num)  ex_catstr("num_nod_per_fa",num)
                                                /* # of nodes per face in    */
                                                /*   face block num          */
#define DIM_NUM_FAC_PER_EL(num)  ex_catstr("num_fac_per_el",num)
                                                /* # of faces per element in */
                                                /*   element block num       */
#define DIM_NUM_ATT_IN_FBLK(num) ex_catstr("num_att_in_fblk",num)
                                                /* # of attributes in face   */
                                                /*   block num               */
#define DIM_NUM_ATT_IN_NBLK      "num_att_in_nblk"

#define VAR_CONN(num)            ex_catstr("connect",num)
                                                /* element connectivity for  */
                                                /*   element block num       */
#define VAR_EBEPEC(num)          ex_catstr("ebepecnt",num)
                                                /* array containing number of entity per */
            /*  entity for n-sided face/element blocks */
#define VAR_ATTRIB(num)         ex_catstr("attrib",num)
                                                /* list of attributes for    */
                                                /*   element block num       */
#define VAR_NAME_ATTRIB(num)    ex_catstr("attrib_name",num)
                                                /* list of attribute names   */
                                                /* for element block num     */
#define VAR_EB_PROP(num)        ex_catstr("eb_prop",num)
                                                /* list of the numth property*/
                                                /*   for all element blocks  */
#define VAR_ECONN(num)            ex_catstr("edgconn",num)
                                                /* edge connectivity for     */
                                                /*   element block num       */
#define VAR_EBCONN(num)           ex_catstr("ebconn",num)
                                                /* edge connectivity for     */
                                                /*   edge block num          */
#define VAR_EATTRIB(num)          ex_catstr("eattrb",num)
                                                /* list of attributes for    */
                                                /*   edge block num          */
#define VAR_NAME_EATTRIB(num)    ex_catstr("eattrib_name",num)
                                                /* list of attribute names   */
                                                /* for edge block num        */
#define VAR_NATTRIB              "nattrb"
#define VAR_NAME_NATTRIB         "nattrib_name"
#define DIM_NUM_ATT_IN_NBLK      "num_att_in_nblk"

#define VAR_NSATTRIB(num)        ex_catstr("nsattrb",num)
#define VAR_NAME_NSATTRIB(num)   ex_catstr("nsattrib_name",num)
#define DIM_NUM_ATT_IN_NS(num)   ex_catstr("num_att_in_ns",num)

#define VAR_SSATTRIB(num)        ex_catstr("ssattrb",num)
#define VAR_NAME_SSATTRIB(num)   ex_catstr("ssattrib_name",num)
#define DIM_NUM_ATT_IN_SS(num)   ex_catstr("num_att_in_ss",num)

#define VAR_ESATTRIB(num)        ex_catstr("esattrb",num)
#define VAR_NAME_ESATTRIB(num)   ex_catstr("esattrib_name",num)
#define DIM_NUM_ATT_IN_ES(num)   ex_catstr("num_att_in_es",num)

#define VAR_FSATTRIB(num)        ex_catstr("fsattrb",num)
#define VAR_NAME_FSATTRIB(num)   ex_catstr("fsattrib_name",num)
#define DIM_NUM_ATT_IN_FS(num)   ex_catstr("num_att_in_fs",num)

#define VAR_ELSATTRIB(num)       ex_catstr("elsattrb",num)
#define VAR_NAME_ELSATTRIB(num)  ex_catstr("elsattrib_name",num)
#define DIM_NUM_ATT_IN_ELS(num)  ex_catstr("num_att_in_els",num)

#define VAR_ED_PROP(num)         ex_catstr("ed_prop",num)
                                                /* list of the numth property*/
                                                /*   for all edge blocks     */
#define VAR_FCONN(num)            ex_catstr("facconn",num)
                                                /* face connectivity for     */
                                                /*   element block num       */
#define VAR_FBCONN(num)           ex_catstr("fbconn",num)
                                                /* face connectivity for     */
                                                /*   face block num          */
#define VAR_FBEPEC(num)           ex_catstr("fbepecnt",num)
                                                /* array containing number of entity per */
            /*  entity for n-sided face/element blocks */
#define VAR_FATTRIB(num)          ex_catstr("fattrb",num)
                                                /* list of attributes for    */
                                                /*   face block num          */
#define VAR_NAME_FATTRIB(num)    ex_catstr("fattrib_name",num)
                                                /* list of attribute names   */
                                                /* for face block num        */
#define VAR_FA_PROP(num)         ex_catstr("fa_prop",num)
                                                /* list of the numth property*/
                                                /*   for all face blocks     */
#define ATT_PROP_NAME           "name"          /* name attached to element  */
                                                /*   block, node set, side   */
                                                /*   set, element map, or    */
                                                /*   map properties          */
#define VAR_MAP                 "elem_map"      /* element order map         */
                                                /* obsolete, replaced by     */
                                                /* VAR_ELEM_MAP(num)         */
#define DIM_NUM_SS              "num_side_sets" /* # of side sets            */
#define VAR_SS_STAT             "ss_status"     /* side set status           */
#define VAR_SS_IDS              "ss_prop1"      /* side set id properties    */
#define DIM_NUM_SIDE_SS(num)    ex_catstr("num_side_ss",num)
                                                /* # of sides in side set num*/
#define DIM_NUM_DF_SS(num)      ex_catstr("num_df_ss",num)
                                               /* # of distribution factors */
                                               /* in side set num           */
/*#define DIM_NUM_NOD_SS(num)   ex_catstr("num_nod_ss",num) *** obsolete *** */
                                                /* # of nodes in side set num*/
#define VAR_FACT_SS(num)        ex_catstr("dist_fact_ss",num)
                                                /* the distribution factors  */
                                                /*   for each node in side   */
                                                /*   set num                 */
#define VAR_ELEM_SS(num)        ex_catstr("elem_ss",num)
                                                /* list of elements in side  */
                                                /*   set num                 */
#define VAR_SIDE_SS(num)        ex_catstr("side_ss",num)
                                                /* list of sides in side set */
#define VAR_SS_PROP(num)        ex_catstr("ss_prop",num)
                                                /* list of the numth property*/
                                                /*   for all side sets       */
#define DIM_NUM_ES               "num_edge_sets"/* # of edge sets            */
#define VAR_ES_STAT              "es_status"    /* edge set status           */
#define VAR_ES_IDS               "es_prop1"     /* edge set id properties    */
#define DIM_NUM_EDGE_ES(num)     ex_catstr("num_edge_es",num)
                                                /* # of edges in edge set num*/
#define DIM_NUM_DF_ES(num)       ex_catstr("num_df_es",num)
                                                /* # of distribution factors */
                                                /* in edge set num           */
/*#define DIM_NUM_NOD_ES(num)    ex_catstr("num_nod_es",num) *** obsolete *** */
                                                /* # of nodes in edge set num*/
#define VAR_FACT_ES(num)         ex_catstr("dist_fact_es",num)
                                                /* the distribution factors  */
                                                /*   for each node in edge   */
                                                /*   set num                 */
#define VAR_EDGE_ES(num)         ex_catstr("edge_es",num)
                                                /* list of edges in edge     */
                                                /*   set num                 */
#define VAR_ORNT_ES(num)         ex_catstr("ornt_es",num)
                                                /* list of orientations in   */
                                                /*  the edge set.            */
#define VAR_ES_PROP(num)         ex_catstr("es_prop",num)
                                                /* list of the numth property*/
                                                /*   for all edge sets       */
#define DIM_NUM_FS               "num_face_sets"/* # of face sets            */
#define VAR_FS_STAT              "fs_status"    /* face set status           */
#define VAR_FS_IDS               "fs_prop1"     /* face set id properties    */
#define DIM_NUM_FACE_FS(num)     ex_catstr("num_face_fs",num)
                                                /* # of faces in side set num*/
#define DIM_NUM_DF_FS(num)       ex_catstr("num_df_fs",num)
                                                /* # of distribution factors */
                                                /* in face set num           */
/*#define DIM_NUM_NOD_FS(num)    ex_catstr("num_nod_ss",num) *** obsolete *** */
                                                /* # of nodes in face set num*/
#define VAR_FACT_FS(num)         ex_catstr("dist_fact_fs",num)
                                                /* the distribution factors  */
                                                /*   for each node in face   */
                                                /*   set num                 */
#define VAR_FACE_FS(num)         ex_catstr("face_fs",num)
                                                /* list of elements in face  */
                                                /*   set num                 */
#define VAR_ORNT_FS(num)         ex_catstr("ornt_fs",num)
                                                /* list of sides in side set */
#define VAR_FS_PROP(num)         ex_catstr("fs_prop",num)
                                                /* list of the numth property*/
                                                /*   for all face sets       */
#define DIM_NUM_ELS              "num_elem_sets"/* # of elem sets            */
#define DIM_NUM_ELE_ELS(num)     ex_catstr("num_ele_els",num)
                                                /* # of elements in elem set */
                                                /*   num                     */
#define DIM_NUM_DF_ELS(num)     ex_catstr("num_df_els",num)
                                               /* # of distribution factors */
                                               /* in element set num        */
#define VAR_ELS_STAT            "els_status"    /* elem set status           */
#define VAR_ELS_IDS             "els_prop1"     /* elem set id properties    */
#define VAR_ELEM_ELS(num)        ex_catstr("elem_els",num)
                                                /* list of elements in elem  */
                                                /*   set num                 */
#define VAR_FACT_ELS(num)       ex_catstr("dist_fact_els",num)
                                                /* list of distribution      */
                                                /*   factors in elem set num */
#define VAR_ELS_PROP(num)       ex_catstr("els_prop",num)
                                                /* list of the numth property*/
                                                /*   for all elem sets       */
#define DIM_NUM_NS               "num_node_sets"/* # of node sets            */
#define DIM_NUM_NOD_NS(num)      ex_catstr("num_nod_ns",num)
                                                /* # of nodes in node set    */
                                                /*   num                     */
#define DIM_NUM_DF_NS(num)      ex_catstr("num_df_ns",num)
                                               /* # of distribution factors */
                                               /* in node set num           */
#define VAR_NS_STAT             "ns_status"     /* node set status           */
#define VAR_NS_IDS              "ns_prop1"      /* node set id properties    */
#define VAR_NODE_NS(num)        ex_catstr("node_ns",num)
                                                /* list of nodes in node set */
                                                /*   num                     */
#define VAR_FACT_NS(num)        ex_catstr("dist_fact_ns",num)
                                                /* list of distribution      */
                                                /*   factors in node set num */
#define VAR_NS_PROP(num)        ex_catstr("ns_prop",num)
                                                /* list of the numth property*/
                                                /*   for all node sets       */
#define DIM_NUM_QA              "num_qa_rec"    /* # of QA records           */
#define VAR_QA_TITLE            "qa_records"    /* QA records                */
#define DIM_NUM_INFO            "num_info"      /* # of information records  */
#define VAR_INFO                "info_records"  /* information records       */
#define VAR_HIS_TIME            "time_hist"     /* obsolete                  */
#define VAR_WHOLE_TIME          "time_whole"    /* simulation times for whole*/
                                                /*   time steps              */
#define VAR_ELEM_TAB            "elem_var_tab"  /* element variable truth    */
                                                /*   table                   */
#define VAR_EBLK_TAB            "edge_var_tab"  /* edge variable truth table */
#define VAR_FBLK_TAB            "face_var_tab"  /* face variable truth table */
#define VAR_ELSET_TAB           "elset_var_tab" /* elemset variable truth    */
                                                /*   table                   */
#define VAR_SSET_TAB            "sset_var_tab"  /* sideset variable truth    */
                                                /*   table                   */
#define VAR_FSET_TAB            "fset_var_tab"  /* faceset variable truth    */
                                                /*   table                   */
#define VAR_ESET_TAB            "eset_var_tab"  /* edgeset variable truth    */
                                                /*   table                   */
#define VAR_NSET_TAB            "nset_var_tab"  /* nodeset variable truth    */
                                                /*   table                   */
#define DIM_NUM_GLO_VAR         "num_glo_var"   /* # of global variables     */
#define VAR_NAME_GLO_VAR        "name_glo_var"  /* names of global variables */
#define VAR_GLO_VAR             "vals_glo_var"  /* values of global variables*/
#define DIM_NUM_NOD_VAR         "num_nod_var"   /* # of nodal variables      */
#define VAR_NAME_NOD_VAR        "name_nod_var"  /* names of nodal variables  */
#define VAR_NOD_VAR             "vals_nod_var"  /* values of nodal variables */
#define VAR_NOD_VAR_NEW(num)    ex_catstr("vals_nod_var",num)
                                                /* values of nodal variables */
#define DIM_NUM_ELE_VAR         "num_elem_var"  /* # of element variables    */
#define VAR_NAME_ELE_VAR        "name_elem_var" /* names of element variables*/
#define VAR_ELEM_VAR(num1,num2) ex_catstr2("vals_elem_var",num1,"eb",num2)
                                                /* values of element variable*/
                                                /*   num1 in element block   */
                                                /*   num2                    */
#define DIM_NUM_EDG_VAR         "num_edge_var"  /* # of edge variables       */
#define VAR_NAME_EDG_VAR        "name_edge_var" /* names of edge variables   */
#define VAR_EDGE_VAR(num1,num2) ex_catstr2("vals_edge_var",num1,"eb",num2)
                                                /* values of edge variable   */
                                                /*   num1 in edge block num2 */
#define DIM_NUM_FAC_VAR         "num_face_var"  /* # of face variables       */
#define VAR_NAME_FAC_VAR        "name_face_var" /* names of face variables   */
#define VAR_FACE_VAR(num1,num2) ex_catstr2("vals_face_var",num1,"fb",num2)
                                                /* values of face variable   */
                                                /*   num1 in face block num2 */

#define DIM_NUM_NSET_VAR        "num_nset_var"  /* # of nodeset variables    */
#define VAR_NAME_NSET_VAR       "name_nset_var" /* names of nodeset variables*/
#define VAR_NS_VAR(num1,num2) ex_catstr2("vals_nset_var",num1,"ns",num2)
                                                /* values of nodeset variable*/
                                                /*   num1 in nodeset num2    */
#define DIM_NUM_ESET_VAR        "num_eset_var"  /* # of edgeset variables    */
#define VAR_NAME_ESET_VAR       "name_eset_var" /* names of edgeset variables*/
#define VAR_ES_VAR(num1,num2) ex_catstr2("vals_eset_var",num1,"es",num2)
                                                /* values of edgeset variable*/
                                                /*   num1 in edgeset num2    */
#define DIM_NUM_FSET_VAR        "num_fset_var"  /* # of faceset variables    */
#define VAR_NAME_FSET_VAR       "name_fset_var" /* names of faceset variables*/
#define VAR_FS_VAR(num1,num2) ex_catstr2("vals_fset_var",num1,"fs",num2)
                                                /* values of faceset variable*/
                                                /*   num1 in faceset num2    */
#define DIM_NUM_SSET_VAR        "num_sset_var"  /* # of sideset variables    */
#define VAR_NAME_SSET_VAR       "name_sset_var" /* names of sideset variables*/
#define VAR_SS_VAR(num1,num2) ex_catstr2("vals_sset_var",num1,"ss",num2)
                                                /* values of sideset variable*/
                                                /*   num1 in sideset num2    */
#define DIM_NUM_ELSET_VAR       "num_elset_var" /* # of element set variables*/
#define VAR_NAME_ELSET_VAR      "name_elset_var"/* names of elemset variables*/
#define VAR_ELS_VAR(num1,num2) ex_catstr2("vals_elset_var",num1,"es",num2)
                                                /* values of elemset variable*/
                                                /*   num1 in elemset num2    */

#define DIM_NUM_HIS_VAR         "num_his_var"   /* obsolete                  */
#define VAR_NAME_HIS_VAR        "name_his_var"  /* obsolete                  */
#define VAR_HIS_VAR             "vals_his_var"  /* obsolete                  */
#define DIM_STR                 "len_string"    /* general dimension of      */
                                                /*   length MAX_STR_LENGTH   */
                                                /*   used for some string lengths   */
#define DIM_STR_NAME            "len_name"      /* general dimension of      */
                                                /*   length MAX_NAME_LENGTH  */
                                                /*   used for name lengths   */
#define DIM_LIN                 "len_line"      /* general dimension of      */
                                                /*   length MAX_LINE_LENGTH  */
                                                /*   used for long strings   */
#define DIM_N4                  "four"          /* general dimension of      */
                                                /*   length 4                */
#define DIM_TIME                "time_step"     /* unlimited (expandable)    */
                                                /*   dimension for time steps*/
#define DIM_HTIME               "hist_time_step"/* obsolete                  */
#define VAR_ELEM_NUM_MAP        "elem_num_map"  /* element numbering map     */
                                                /* obsolete, replaced by     */
                                                /* VAR_ELEM_MAP(num)         */
#define VAR_FACE_NUM_MAP        "face_num_map"  /* face numbering map     */
                                                /* obsolete, replaced by     */
                                                /* VAR_FACE_MAP(num)         */
#define VAR_EDGE_NUM_MAP        "edge_num_map"  /* edge numbering map     */
                                                /* obsolete, replaced by     */
                                                /* VAR_EDGE_MAP(num)         */
#define VAR_NODE_NUM_MAP        "node_num_map"  /* node numbering map        */
                                                /* obsolete, replaced by     */
                                                /* VAR_NODE_MAP(num)         */
#define DIM_NUM_EM              "num_elem_maps" /* # of element maps         */
#define VAR_ELEM_MAP(num)       ex_catstr("elem_map",num)
                                                /* the numth element map     */
#define VAR_EM_PROP(num)        ex_catstr("em_prop",num)
                                                /* list of the numth property*/
                                                /*   for all element maps    */
#define DIM_NUM_EDM             "num_edge_maps" /* # of edge maps            */
#define VAR_EDGE_MAP(num)       ex_catstr("edge_map",num)
                                                /* the numth edge map        */
#define VAR_EDM_PROP(num)       ex_catstr("edm_prop",num)
                                                /* list of the numth property*/
                                                /*   for all edge maps       */
#define DIM_NUM_FAM             "num_face_maps" /* # of face maps            */
#define VAR_FACE_MAP(num)       ex_catstr("face_map",num)
                                                /* the numth face map        */
#define VAR_FAM_PROP(num)       ex_catstr("fam_prop",num)
                                                /* list of the numth property*/
                                                /*   for all face maps       */
#define DIM_NUM_NM              "num_node_maps" /* # of node maps            */
#define VAR_NODE_MAP(num)       ex_catstr("node_map",num)
                                                /* the numth node map        */
#define VAR_NM_PROP(num)        ex_catstr("nm_prop",num)
                                                /* list of the numth property*/
                                                /*   for all node maps       */

#define DIM_NUM_CFRAMES  "num_cframes"
#define DIM_NUM_CFRAME9  "num_cframes_9"
#define VAR_FRAME_COORDS "frame_coordinates"
#define VAR_FRAME_IDS    "frame_ids"
#define VAR_FRAME_TAGS   "frame_tags"

enum ex_element_type {
  EX_EL_UNK         =  -1,     /**< unknown entity */
  EX_EL_NULL_ELEMENT=   0,     
  EX_EL_TRIANGLE    =   1,     /**< Triangle entity */
  EX_EL_QUAD        =   2,     /**< Quad entity */
  EX_EL_HEX         =   3,     /**< Hex entity */
  EX_EL_WEDGE       =   4,     /**< Wedge entity */
  EX_EL_TETRA       =   5,     /**< Tetra entity */
  EX_EL_TRUSS       =   6,     /**< Truss entity */
  EX_EL_BEAM        =   7,     /**< Beam entity */
  EX_EL_SHELL       =   8,     /**< Shell entity */
  EX_EL_SPHERE      =   9,     /**< Sphere entity */
  EX_EL_CIRCLE      =  10,     /**< Circle entity */
  EX_EL_TRISHELL    =  11,     /**< Triangular Shell entity */
  EX_EL_PYRAMID     =  12      /**< Pyramid entity */
}; 
typedef enum ex_element_type ex_element_type;

enum ex_coordinate_frame_type {
  EX_CF_RECTANGULAR =   1,
  EX_CF_CYLINDRICAL =   2,
  EX_CF_SPHERICAL   =   3
}; 
typedef enum ex_coordinate_frame_type ex_coordinate_frame_type;

/* Internal structure declarations */

struct elem_blk_parm
{
  char elem_type[33];
  int elem_blk_id;
  int num_elem_in_blk;
  int num_nodes_per_elem;
  int num_sides;
  int num_nodes_per_side[6];
  int num_attr;
  int elem_ctr;
  ex_element_type elem_type_val;
};

struct list_item {              /* for use with ex_get_file_item */

  int exo_id;
  int value;
  struct list_item* next;
};

struct obj_stats {
  int *id_vals;
  int *stat_vals;
  long num;
  int exoid;
  int valid_ids;
  int valid_stat;
  struct obj_stats *next;
};

void  ex_iqsort(int v[], int iv[], int count );
char* ex_catstr(const char*, int);
char* ex_catstr2(const char*, int, const char*, int);
char* ex_dim_num_entries_in_object(ex_entity_type, int);
char* ex_dim_num_objects(ex_entity_type obj_type);
char* ex_name_var_of_object( ex_entity_type, int, int );
char* ex_name_of_map( ex_entity_type, int );

int ex_conv_ini  (int exoid, int* comp_wordsize, int* io_wordsize, int file_wordsize);
void ex_conv_exit  (int exoid);
nc_type nc_flt_code  (int exoid);
int ex_comp_ws  (int exoid);
int ex_get_cpu_ws(void);

struct list_item** ex_get_counter_list(ex_entity_type obj_type);
int ex_get_file_item  (int, struct list_item**);
int ex_inc_file_item  (int, struct list_item**);
void ex_rm_file_item  (int, struct list_item**);

extern struct obj_stats* exoII_eb;
extern struct obj_stats* exoII_ed;
extern struct obj_stats* exoII_fa;
extern struct obj_stats* exoII_ns;
extern struct obj_stats* exoII_es;
extern struct obj_stats* exoII_fs;
extern struct obj_stats* exoII_ss;
extern struct obj_stats* exoII_els;
extern struct obj_stats* exoII_em;
extern struct obj_stats* exoII_edm;
extern struct obj_stats* exoII_fam;
extern struct obj_stats* exoII_nm;


struct obj_stats *ex_get_stat_ptr  ( int exoid, struct obj_stats** obj_ptr);
void ex_rm_stat_ptr  (int exoid, struct obj_stats** obj_ptr);

int ex_id_lkup  (int exoid, ex_entity_type id_type, int num);
int ex_get_dimension(int exoid, const char *dimtype, const char *label,
         size_t *count, int *dimid, const char *routine);

int ex_get_name_internal(int exoid, int varid, size_t ind, char *name,
       ex_entity_type type, const char *routine);
int ex_get_names_internal(int exoid, int varid, size_t count, char**names,
        ex_entity_type type, const char *routine);
int ex_put_name_internal(int exoid, int varid, size_t ind, const char *name,
        ex_entity_type type, const char *subtype, const char *routine);
int ex_put_names_internal(int exoid, int varid, size_t count, char**names,
        ex_entity_type type, const char *subtype, const char *routine);
void ex_trim_internal(char *name);
void ex_update_max_name_length(int exoid, int length);
#endif
