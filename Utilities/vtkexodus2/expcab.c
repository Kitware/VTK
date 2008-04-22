/*
 * Copyright (c) 2006 Sandia Corporation. Under the terms of Contract
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
* expclb - ex_put_concat_all_blocks: write elem, edge, & face block parameters
*
* entry conditions - 
*   input parameters:
*       int                    exoid          exodus file id
*       const ex_block_params* bparam         block parameters structure
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"
#include <string.h>

/* Define this macro to get old-style number maps (node_num_map &
 * elem_num_map) when define_maps is true. Leave it undefined to
 * get new-style number maps (node_map#, edge_map#, face_map#, and
 * elem_map#) when define_maps is true.
 */
#undef EX_MAPSTYLE_OLD

/*!
 * writes the parameters used to describe all element, edge, and face blocks
 */
int ex_put_concat_all_blocks (int    exoid,
                              const ex_block_params *param)
{
  int i, varid, dimid, dims[2], strdim, *eb_stat, *ed_stat, *fa_stat;
  int iblk;
  int iresult;
  long start[2], count[2], num_elem_blk, num_edge_blk, num_face_blk;
  nclong *lptr;
  int cur_num_elem_blk, nelnoddim, numelbdim, numattrdim, connid=-1;
  int cur_num_edge_blk, numedbdim, nednoddim, cur_num_face_blk, numfabdim, nfanoddim;
  int neledgdim=-1, nelfacdim=-1;
  char *cdum;
  char errmsg[MAX_ERR_LENGTH];
  int elem_work = 0; /* is DIM_NUM_EL_BLK defined? If so, there's work to do */
  int edge_work = 0; /* is DIM_NUM_ED_BLK defined? If so, there's work to do */
  int face_work = 0; /* is DIM_NUM_FA_BLK defined? If so, there's work to do */
#ifdef EX_MAPSTYLE_OLD /* vv--old style number maps--vv */
  int numelemdim, numnodedim;
#else /* ^^--old style number maps--^^  vv--new style number maps--vv */
  static const char* dim_num_maps[] = {
    DIM_NUM_NM,
    DIM_NUM_EDM,
    DIM_NUM_FAM,
    DIM_NUM_EM,
  };
  static const char* dim_size_maps[] = {
    DIM_NUM_NODES,
    DIM_NUM_EDGE,
    DIM_NUM_FACE,
    DIM_NUM_ELEM,
  };
  static const int map_enums[] = {
    EX_NODE_MAP,
    EX_EDGE_MAP,
    EX_FACE_MAP,
    EX_ELEM_MAP
  };
  /* If param->define_maps is true, we must fill these with values from ex_put_init_ext
     before entering define mode */
  long num_maps[sizeof(dim_num_maps)/sizeof(dim_num_maps[0])];
  int num_map_dims = sizeof(dim_num_maps)/sizeof(dim_num_maps[0]);
#endif /* ^^--new style number maps--^^ */

  exerrval  = 0; /* clear error code */

  cdum = 0;

  /* inquire previously defined dimensions  */
  if ((strdim = ncdimid (exoid, DIM_STR)) < 0) {
    exerrval = ncerr;
    sprintf(errmsg,
            "Error: failed to get string length in file id %d",exoid);
    ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ( param->define_maps ) {
    for ( i = 0; i < num_map_dims; ++i ) {
      if ( (dimid = ncdimid( exoid, dim_num_maps[i] )) == -1 ) {
        exerrval = ncerr;
        sprintf( errmsg, "Error: failed to find node map size of file id %d", exoid );
        ex_err( "ex_put_concat_all_blocks", errmsg, exerrval );
        return (EX_FATAL);
      }
      if ( ncdiminq( exoid, dimid, cdum, num_maps + i ) == -1 ) {
        exerrval = ncerr;
        sprintf( errmsg, "Error: failed to retrieve node map size of file id %d", exoid );
        ex_err( "ex_put_concat_all_blocks", errmsg, exerrval );
        return (EX_FATAL);
      }
    }
  }

#define EX_PREPARE_BLOCK(TNAME,WNAME,DNUMNAME,VSTATNAME,VIDNAME,LNUMNAME,SNUMNAME,SIDNAME,GSTAT) \
  /* first check if any TNAME blocks are specified \
   * OK if zero... \
   */ \
  if ((dimid = (ncdimid (exoid, DNUMNAME))) != -1 ) { \
    WNAME = 1; \
    \
    /* Get number of TNAME blocks defined for this file */ \
    if ((ncdiminq (exoid,dimid,cdum,&LNUMNAME)) == -1) { \
      exerrval = ncerr; \
      sprintf(errmsg, \
        "Error: failed to get number of " TNAME " blocks in file id %d", \
        exoid); \
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval); \
      return (EX_FATAL); \
    } \
    \
    /* Fill out the TNAME block status array */ \
    if (!(GSTAT = malloc(LNUMNAME*sizeof(int)))) { \
      exerrval = EX_MEMFAIL; \
      sprintf(errmsg, \
        "Error: failed to allocate space for " TNAME " block status array in file id %d", \
        exoid); \
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval); \
      return (EX_FATAL); \
    } \
    \
    for (i=0;i<LNUMNAME;i++) { \
      if (SNUMNAME[i] == 0) /* Is this a NULL TNAME block? */ \
        GSTAT[i] = 0; /* change TNAME block status to NULL */ \
      else \
        GSTAT[i] = 1; /* change TNAME block status to TRUE */ \
    } \
    \
    /* Next, get variable id of status array */ \
    if ((varid = ncvarid (exoid, VSTATNAME)) == -1) { \
      exerrval = ncerr; \
      sprintf(errmsg, \
        "Error: failed to locate " TNAME " block status in file id %d", \
        exoid); \
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval); \
      return (EX_FATAL); \
    } \
    \
    /* this contortion is necessary because netCDF is expecting nclongs; \
       fortunately it's necessary only when ints and nclongs aren't the \
       same size */ \
    start[0] = 0; \
    count[0] = LNUMNAME; \
    \
    if (sizeof(int) == sizeof(nclong)) { \
      iresult = ncvarput (exoid, varid, start, count, GSTAT); \
    } else { \
      lptr = itol (GSTAT, LNUMNAME); \
      iresult = ncvarput (exoid, varid, start, count, lptr); \
      free(lptr); \
    } \
    \
    if (iresult == -1) { \
      exerrval = ncerr; \
      sprintf(errmsg, \
        "Error: failed to store " TNAME " block status array to file id %d", \
        exoid); \
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval); \
      return (EX_FATAL); \
    } \
    \
    free(GSTAT); \
    \
    /* Next, fill out ids array */ \
    /* first get id of ids array variable */ \
    if ((varid = ncvarid (exoid, VIDNAME)) == -1) { \
      exerrval = ncerr; \
      sprintf(errmsg, \
        "Error: failed to locate " TNAME " block ids array in file id %d", \
        exoid); \
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval); \
      return (EX_FATAL); \
    } \
    \
    /* then, write out id list */ \
    /* this contortion is necessary because netCDF is expecting nclongs; \
       fortunately it's necessary only when ints and nclongs aren't the \
       same size */ \
    start[0] = 0; \
    count[0] = LNUMNAME; \
    \
    if (sizeof(int) == sizeof(nclong)) { \
      iresult = ncvarput (exoid, varid, start, count, SIDNAME); \
    } else { \
      lptr = itol (SIDNAME, LNUMNAME); \
      iresult = ncvarput (exoid, varid, start, count, lptr); \
      free(lptr); \
    } \
    \
    if (iresult == -1) { \
      exerrval = ncerr; \
      sprintf(errmsg, \
        "Error: failed to store " TNAME " block id array in file id %d", \
        exoid); \
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval); \
      return (EX_FATAL); \
    } \
  }

  EX_PREPARE_BLOCK("element",elem_work,DIM_NUM_EL_BLK,VAR_STAT_EL_BLK,VAR_ID_EL_BLK,
    num_elem_blk,param->num_elem_this_blk,param->elem_blk_id,eb_stat);
  EX_PREPARE_BLOCK(   "edge",edge_work,DIM_NUM_ED_BLK,VAR_STAT_ED_BLK,VAR_ID_ED_BLK,
    num_edge_blk,param->num_edge_this_blk,param->edge_blk_id,ed_stat);
  EX_PREPARE_BLOCK(   "face",face_work,DIM_NUM_FA_BLK,VAR_STAT_FA_BLK,VAR_ID_FA_BLK,
    num_face_blk,param->num_face_this_blk,param->face_blk_id,fa_stat);

  if ( elem_work == 0 && edge_work == 0 && face_work == 0 && param->define_maps == 0 ) {
    /* Nothing to do. This is not an error, but we can save
     * ourselves from entering define mode by returning here.
     */
    return (EX_NOERR);
  }
  /* put netcdf file into define mode  */
  if (ncredef (exoid) == -1)  {
    exerrval = ncerr;
    sprintf(errmsg,"Error: failed to place file id %d into define mode",exoid);
    ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
    return (EX_FATAL);
  }

#define EX_PREPARE_ATTRIB_ARRAY(TNAME,CURBLK,DNAME,DVAL,ID,VANAME,VADIM0,VADIM1,VANNAME) \
    if (DVAL[iblk] > 0) { \
      if ((VADIM1 = ncdimdef (exoid,  \
                                  DNAME(CURBLK+1), \
                                  (long)DVAL[iblk])) == -1) { \
        exerrval = ncerr; \
        sprintf(errmsg, \
                "Error: failed to define number of attributes in " TNAME " block %d in file id %d", \
                ID[iblk],exoid); \
        ex_err("ex_put_concat_all_blocks",errmsg,exerrval); \
        goto error_ret;         /* exit define mode and return */ \
      } \
       \
      dims[0] = VADIM0; \
      dims[1] = VADIM1; \
       \
      if ((ncvardef (exoid, VANAME(CURBLK+1), \
                     nc_flt_code(exoid), 2, dims)) == -1) { \
        exerrval = ncerr; \
        sprintf(errmsg, \
                "Error:  failed to define attributes for " TNAME " block %d in file id %d", \
                ID[iblk],exoid); \
        ex_err("ex_put_concat_all_blocks",errmsg,exerrval); \
        goto error_ret;         /* exit define mode and return */ \
      } \
 \
      /* Attribute names... */ \
      dims[0] = VADIM1; \
      dims[1] = strdim; \
       \
      if (ncvardef (exoid, VANNAME(CURBLK+1), NC_CHAR, 2, dims) == -1) { \
        exerrval = ncerr; \
        sprintf(errmsg, \
                "Error: failed to define " TNAME " attribute name array in file id %d",exoid); \
        ex_err("ex_put_concat_all_blocks",errmsg,exerrval); \
        goto error_ret;         /* exit define mode and return */ \
      } \
    }

#define EX_PREPARE_CONN(TNAME,BLK,BLKID,BLKSZ,VNAME,DNAME) \
    if ( DNAME > 0 ) { \
      dims[0] = BLKSZ; \
      dims[1] = DNAME; \
      \
      if ((connid = ncvardef (exoid, VNAME(BLK+1), \
            NC_LONG, 2, dims)) == -1) { \
        exerrval = ncerr; \
        sprintf(errmsg, \
          "Error: failed to create " TNAME " connectivity array for block %d in file id %d", \
          BLKID[iblk],exoid); \
        ex_err("ex_put_concat_all_blocks",errmsg,exerrval); \
        goto error_ret;         /* exit define mode and return */ \
      } \
    }


  /* Iterate over edge blocks ... */
  for (iblk = 0; iblk < num_edge_blk; ++iblk) {

    cur_num_edge_blk=ex_get_file_item(exoid, &ed_ctr_list );
    if (cur_num_edge_blk >= num_edge_blk) {
      exerrval = EX_FATAL;
      sprintf(errmsg,
              "Error: exceeded number of edge blocks (%ld) defined in file id %d",
              num_edge_blk,exoid);
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
      goto error_ret;
    }

    /* NOTE: ex_inc_file_item  is used to find the number of edge blocks
       for a specific file and returns that value incremented. */
    cur_num_edge_blk=ex_inc_file_item(exoid, &ed_ctr_list );

    if (param->num_edge_this_blk[iblk] == 0) /* Is this a NULL edge block? */
      continue;

    /* define some dimensions and variables*/
    if ((numedbdim = ncdimdef (exoid,
                               DIM_NUM_ED_IN_EBLK(cur_num_edge_blk+1),
                               (long)param->num_edge_this_blk[iblk])) == -1) {
      exerrval = ncerr;
      if (ncerr == NC_ENAMEINUSE) {     /* duplicate entry */
        sprintf(errmsg,
                "Error: edge block %d already defined in file id %d",
                param->edge_blk_id[iblk],exoid);
      } else {
        sprintf(errmsg,
                "Error: failed to define number of edges/block for block %d file id %d",
                param->edge_blk_id[iblk],exoid);
      }
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

    if ((nednoddim = ncdimdef (exoid,
                               DIM_NUM_NOD_PER_ED(cur_num_edge_blk+1),
                               (long)param->num_nodes_per_edge[iblk])) == -1) {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to define number of nodes/edge for block %d in file id %d",
              param->edge_blk_id[iblk],exoid);
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

    /* edge attribute array */

    EX_PREPARE_ATTRIB_ARRAY("edge",cur_num_edge_blk,DIM_NUM_ATT_IN_EBLK,param->num_attr_edge,param->edge_blk_id,VAR_EATTRIB,numedbdim,numattrdim,VAR_NAME_EATTRIB);

    EX_PREPARE_CONN("edge block",cur_num_edge_blk,param->edge_blk_id,numedbdim,VAR_EBCONN,nednoddim);

    /* store edge type as attribute of connectivity variable */
    if ((ncattput (exoid, connid, ATT_NAME_ELB, NC_CHAR,
          (int)strlen(param->edge_type[iblk])+1,
          (void*)param->edge_type[iblk])) == -1) {
      exerrval = ncerr;
      sprintf(errmsg,
        "Error: failed to store edge type name %s in file id %d",
        param->edge_type[iblk],exoid);
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
  }

  /* Iterate over face blocks ... */
  for (iblk = 0; iblk < num_face_blk; ++iblk) {

    cur_num_face_blk=ex_get_file_item(exoid, &fa_ctr_list );
    if (cur_num_face_blk >= num_face_blk) {
      exerrval = EX_FATAL;
      sprintf(errmsg,
              "Error: exceeded number of face blocks (%ld) defined in file id %d",
              num_face_blk,exoid);
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
      goto error_ret;
    }

    /* NOTE: ex_inc_file_item  is used to find the number of edge blocks
       for a specific file and returns that value incremented. */
    cur_num_face_blk=ex_inc_file_item(exoid, &fa_ctr_list );

    if (param->num_face_this_blk[iblk] == 0) /* Is this a NULL face block? */
      continue;

    /* define some dimensions and variables*/
    if ((numfabdim = ncdimdef (exoid,
                               DIM_NUM_FA_IN_FBLK(cur_num_face_blk+1),
                               (long)param->num_face_this_blk[iblk])) == -1) {
      exerrval = ncerr;
      if (ncerr == NC_ENAMEINUSE) {     /* duplicate entry */
        sprintf(errmsg,
                "Error: face block %d already defined in file id %d",
                param->face_blk_id[iblk],exoid);
      } else {
        sprintf(errmsg,
                "Error: failed to define number of faces/block for block %d file id %d",
                param->face_blk_id[iblk],exoid);
      }
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

    if ((nfanoddim = ncdimdef (exoid,
                               DIM_NUM_NOD_PER_FA(cur_num_face_blk+1),
                               (long)param->num_nodes_per_face[iblk])) == -1) {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to define number of nodes/face for block %d in file id %d",
              param->face_blk_id[iblk],exoid);
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

    /* edge attribute array */

    EX_PREPARE_ATTRIB_ARRAY("face",cur_num_face_blk,DIM_NUM_ATT_IN_FBLK,param->num_attr_face,param->face_blk_id,VAR_FATTRIB,numfabdim,numattrdim,VAR_NAME_FATTRIB);

    EX_PREPARE_CONN("face block",cur_num_face_blk,param->face_blk_id,numfabdim,VAR_FBCONN,nfanoddim);

    /* store face type as attribute of connectivity variable */
    if ((ncattput (exoid, connid, ATT_NAME_ELB, NC_CHAR,
          (int)strlen(param->face_type[iblk])+1,
          (void*)param->face_type[iblk])) == -1) {
      exerrval = ncerr;
      sprintf(errmsg,
        "Error: failed to store face type name %s in file id %d",
        param->face_type[iblk],exoid);
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
  }

  /* Iterate over element blocks ... */
  for (iblk = 0; iblk < num_elem_blk; ++iblk) {

    cur_num_elem_blk=ex_get_file_item(exoid, &eb_ctr_list );
    if (cur_num_elem_blk >= num_elem_blk) {
      exerrval = EX_FATAL;
      sprintf(errmsg,
              "Error: exceeded number of element blocks (%ld) defined in file id %d",
              num_elem_blk,exoid);
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
      goto error_ret;
    }

    /* NOTE: ex_inc_file_item  is used to find the number of element blocks
       for a specific file and returns that value incremented. */
    cur_num_elem_blk=ex_inc_file_item(exoid, &eb_ctr_list );

    if (param->num_elem_this_blk[iblk] == 0) /* Is this a NULL element block? */
      continue;

    /* define some dimensions and variables*/
    if ((numelbdim = ncdimdef (exoid,
                               DIM_NUM_EL_IN_BLK(cur_num_elem_blk+1),
                               (long)param->num_elem_this_blk[iblk])) == -1) {
      exerrval = ncerr;
      if (ncerr == NC_ENAMEINUSE) {     /* duplicate entry */
        sprintf(errmsg,
                "Error: element block %d already defined in file id %d",
                param->elem_blk_id[iblk],exoid);
      } else {
        sprintf(errmsg,
                "Error: failed to define number of elements/block for block %d file id %d",
                param->elem_blk_id[iblk],exoid);
      }
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

    /* Always define DIM_NUM_NOD_PER_EL, even if zero.
     * Do not define DIM_NUM_EDG_PER_EL or DIM_NUM_FAC_PER_EL unless > 0.
     */
    if ((nelnoddim = ncdimdef (exoid,
                               DIM_NUM_NOD_PER_EL(cur_num_elem_blk+1),
                               (long)param->num_nodes_per_elem[iblk])) == -1) {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to define number of nodes/element for block %d in file id %d",
              param->elem_blk_id[iblk],exoid);
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

    if ( param->num_edges_per_elem[iblk] > 0 ) {
      if ((neledgdim = ncdimdef (exoid,
                                 DIM_NUM_EDG_PER_EL(cur_num_elem_blk+1),
                                 (long)param->num_edges_per_elem[iblk])) == -1) {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to define number of edges/element for block %d in file id %d",
                param->elem_blk_id[iblk],exoid);
        ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }
    }

    if ( param->num_faces_per_elem[iblk] > 0 ) {
      if ((nelfacdim = ncdimdef (exoid,
                                 DIM_NUM_FAC_PER_EL(cur_num_elem_blk+1),
                                 (long)param->num_faces_per_elem[iblk])) == -1) {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to define number of faces/element for block %d in file id %d",
                param->elem_blk_id[iblk],exoid);
        ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }
    }

    /* element attribute array */

    EX_PREPARE_ATTRIB_ARRAY("element",cur_num_elem_blk,DIM_NUM_ATT_IN_BLK,param->num_attr_elem,param->elem_blk_id,VAR_ATTRIB,numelbdim,numattrdim,VAR_NAME_ATTRIB);
    
    /* element connectivity array */

    EX_PREPARE_CONN("nodal",cur_num_elem_blk,param->elem_blk_id,numelbdim,VAR_CONN,nelnoddim);

    /* store element type as attribute of connectivity variable */
    if ((ncattput (exoid, connid, ATT_NAME_ELB, NC_CHAR,
          (int)strlen(param->elem_type[iblk])+1,
          (void*)param->elem_type[iblk])) == -1) {
      exerrval = ncerr;
      sprintf(errmsg,
        "Error: failed to store element type name %s in file id %d",
        param->elem_type[iblk],exoid);
      ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

    EX_PREPARE_CONN( "edge",cur_num_elem_blk,param->elem_blk_id,numelbdim,VAR_ECONN,neledgdim);
    EX_PREPARE_CONN( "face",cur_num_elem_blk,param->elem_blk_id,numelbdim,VAR_FCONN,nelfacdim);
  }

  /* Define the element map here to avoid a later redefine call */
  if ( param->define_maps != 0 ) {
#ifdef EX_MAPSTYLE_OLD /* vv=old style num map */
    if (ncvarid(exoid, VAR_ELEM_NUM_MAP) == -1) { /* Map does not exist */
      /* Possible to have zero elements but >0 element blocks.
       * Only define map if there are nonzero elements
       */
      if ((numelemdim = ncdimid (exoid, DIM_NUM_ELEM)) != -1) {
        dims[0] = numelemdim;
        
        if ((ncvardef (exoid, VAR_ELEM_NUM_MAP, NC_LONG, 1, dims)) == -1) {
          exerrval = ncerr;
          if (ncerr == NC_ENAMEINUSE) {
            sprintf(errmsg,
                    "Error: element numbering map already exists in file id %d",
                    exoid);
          } else {
            sprintf(errmsg,
                    "Error: failed to create element numbering map in file id %d",
                    exoid);
          }
          ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
          goto error_ret;         /* exit define mode and return */
        }
      }
    }
    /* Do the same for the node numbering map */
    if (ncvarid(exoid, VAR_NODE_NUM_MAP) == -1) { /* Map does not exist */
      if ((numnodedim = ncdimid (exoid, DIM_NUM_NODES)) != -1) {
        dims[0] = numnodedim;
        if ((ncvardef (exoid, VAR_NODE_NUM_MAP, NC_LONG, 1, dims)) == -1) {
          exerrval = ncerr;
          if (ncerr == NC_ENAMEINUSE) {
            sprintf(errmsg,
                    "Error: node numbering map already exists in file id %d",
                    exoid);
          } else {
            sprintf(errmsg,
                    "Error: failed to create node numbering map array in file id %d",
                    exoid);
          }
          ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
          goto error_ret;         /* exit define mode and return */
        }
      }
    }
#else /* ^^=old style num map  vv=new style num map */
    int map_type;
    for ( map_type = 0; map_type < num_map_dims; ++map_type ) {
      if ( (dims[0] = ncdimid( exoid, dim_size_maps[map_type] )) == -1 ) {
        exerrval = ncerr;
        sprintf( errmsg,
          "Error: could not find map size dimension %s in file id %d",
          dim_size_maps[map_type], exoid );
        ex_err( "ex_put_concat_all_blocks", errmsg, exerrval );
      }
      for ( i = 1; i <= num_maps[map_type]; ++i ) {
        const char* mapname = ex_name_of_map( map_enums[map_type], i );
        if ( ncvarid( exoid, mapname ) == -1 ) {
          if ( ncvardef( exoid, mapname, NC_LONG, 1, dims ) == -1 ) {
            exerrval = ncerr;
            if ( ncerr == NC_ENAMEINUSE ) {
              sprintf( errmsg, "Error: number map %s already exists in file id %d", mapname, exoid );
            } else {
              sprintf( errmsg, "Error: failed to create number map array %s in file id %d", mapname, exoid );
            }
            ex_err( "ex_put_concat_all_blocks", errmsg, exerrval );
            goto error_ret; /* exit define mode and return */
          }
        }
      }
    }
#endif /* ^^=new style num map */
  }

  /* leave define mode  */
  if (ncendef (exoid) == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
            "Error: failed to complete element block definition in file id %d", 
            exoid);
    ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
    return (EX_FATAL);
  }

  return (EX_NOERR);
  
  /* Fatal error: exit definition mode and return */
 error_ret:
  if (ncendef (exoid) == -1) {     /* exit define mode */
    sprintf(errmsg,
            "Error: failed to complete definition for file id %d",
            exoid);
    ex_err("ex_put_concat_all_blocks",errmsg,exerrval);
  }
  return (EX_FATAL);
}

