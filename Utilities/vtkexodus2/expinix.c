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
* expinix - ex_put_init_ext
*
* entry conditions - 
*   input parameters:
*       int                   exoid     exodus file id
*       const ex_init_params* params    finite element model parameters
*
* exit conditions - 
*
* revision history - 
*          David Thompson  - Added edge/face blocks/sets
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define EX_WRITE_OBJECT_PARAMS(TNAME,DNAME,SNAME,INAME,NUM_BLK,DIMVAR) \
  /* Can have nonzero model->num_elem_blk even if model->num_elem == 0 */ \
  if ((NUM_BLK) > 0) { \
    if ((status = nc_def_dim(exoid, DNAME, (size_t)(NUM_BLK), &DIMVAR)) != NC_NOERR) { \
        exerrval = status; \
        sprintf(errmsg, \
                "Error: failed to define number of " TNAME "s in file id %d", \
                exoid); \
        ex_err("ex_put_init_ext",errmsg,exerrval); \
        goto error_ret;         /* exit define mode and return */ \
      } \
    /* ...and some variables */ \
    /* element block id status array */ \
    dim[0] = DIMVAR; \
    if ((status = nc_def_var (exoid, SNAME, NC_INT, 1, dim, &varid)) != NC_NOERR) {	\
        exerrval = status; \
        sprintf(errmsg, \
                "Error: failed to define " TNAME " status array in file id %d",exoid); \
        ex_err("ex_put_init_ext",errmsg,exerrval); \
        goto error_ret;         /* exit define mode and return */ \
      } \
    \
    /* TNAME id array */ \
    if ((status = nc_def_var (exoid, INAME, NC_INT, 1, dim, &varid)) != NC_NOERR) {	\
        exerrval = status; \
        sprintf(errmsg, \
                "Error: failed to define " TNAME " id array in file id %d",exoid); \
        ex_err("ex_put_init_ext",errmsg,exerrval); \
        goto error_ret;         /* exit define mode and return */ \
      } \
    \
    /*   store property name as attribute of property array variable */ \
    if ((status=nc_put_att_text(exoid, varid, ATT_PROP_NAME, 3, "ID")) != NC_NOERR) { \
        exerrval = status;							\
        sprintf(errmsg, \
                "Error: failed to store " TNAME " property name %s in file id %d", \
                "ID",exoid); \
        ex_err("ex_put_init_ext",errmsg,exerrval); \
        return (EX_FATAL); \
      } \
  }

#define EX_WRITE_MAP_PARAMS(TNAME,DNUMMAP,VMAPIDS,NUMMAPS,MAPDIM) \
  /* Can have nonzero model->num_XXXX_map even if model->num_XXXX == 0 */ \
  if ((NUMMAPS) > 0) { \
    if ((status = nc_def_dim(exoid, DNUMMAP, (size_t)(NUMMAPS), &MAPDIM)) != NC_NOERR) { \
        exerrval = status; \
        sprintf(errmsg, \
                "Error: failed to define number of " TNAME "s in file id %d", \
                exoid); \
        ex_err("ex_put_init_ext",errmsg,exerrval); \
        goto error_ret;         /* exit define mode and return */ \
      } \
    \
    dim[0] = MAPDIM; \
    \
    /* TNAME id array */ \
    if ((status = nc_def_var(exoid, VMAPIDS, NC_INT, 1, dim, &varid)) != NC_NOERR) {	\
        exerrval = status; \
        sprintf(errmsg, \
                "Error: failed to define " TNAME " id array in file id %d",exoid); \
        ex_err("ex_put_init_ext",errmsg,exerrval); \
        goto error_ret;         /* exit define mode and return */ \
      } \
    \
    /*   store property name as attribute of property array variable */ \
    if ((status=nc_put_att_text(exoid, varid, ATT_PROP_NAME, 3, "ID")) != NC_NOERR) { \
        exerrval = status; \
        sprintf(errmsg, \
                "Error: failed to store " TNAME " property name %s in file id %d", \
                "ID",exoid); \
        ex_err("ex_put_init_ext",errmsg,exerrval); \
        return (EX_FATAL); \
      } \
  }

#define EX_WRITE_OBJECT_NAMES(TNAME,DNAME,DIMVAR,DIMVAL) \
  /* Element block names... */ \
  if ((DIMVAL) > 0) { \
    dim[0] = (DIMVAR); \
    dim[1] = strdim; \
     \
    if ((status = nc_def_var (exoid, DNAME, NC_CHAR, 2, dim, &varid)) != NC_NOERR) { \
      exerrval = status; \
      sprintf(errmsg, \
	      "Error: failed to define %s name array in file id %d",TNAME,exoid); \
      ex_err("ex_put_init_ext",errmsg,exerrval); \
      goto error_ret;         /* exit define mode and return */ \
    } \
  }

static void zero_id_status(int exoid, const char *var_stat, const char *var_id,
			   int count, int *ids)
{
  int status;
  int i;
  int id_var, stat_var;
  
  if (count > 0) {
    for (i=0; i < count; i++) {
      ids[i] = 0;
    }

    status = nc_inq_varid(exoid, var_id,   &id_var);
    assert(status == NC_NOERR);
    status = nc_inq_varid(exoid, var_stat, &stat_var);
    assert(status == NC_NOERR);

    status = nc_put_var_int(exoid, id_var,   ids);
    assert(status == NC_NOERR);
    status = nc_put_var_int(exoid, stat_var, ids);
    assert(status == NC_NOERR);
  }
}

/*!
 * writes the initialization parameters to the EXODUS II file
 * \param     exoid     exodus file id
 * \param     model     finite element model parameters
 */

int ex_put_init_ext (int   exoid,
                     const ex_init_params *model)
{
  int numdimdim, numnoddim, elblkdim, edblkdim, fablkdim, esetdim, fsetdim, elsetdim, nsetdim, ssetdim, strdim, dim[2], varid, temp;
  int status;
  int nmapdim,edmapdim,famapdim,emapdim;
  int title_len;
#if 0
  /* used for header size calculations which are turned off for now */
  int header_size, fixed_var_size, iows;
#endif  
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  if (nc_inq_dimid (exoid, DIM_NUM_DIM, &temp) == NC_NOERR)
    {
      exerrval = EX_MSG;
      sprintf(errmsg,
              "Error: initialization already done for file id %d",exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      return (EX_FATAL);
    }


  /* put file into define mode */

  if ((status = nc_redef (exoid)) != NC_NOERR)
    {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to put file id %d into define mode", exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      return (EX_FATAL);
    }

  /* define some attributes... */
  title_len = strlen(model->title) < MAX_LINE_LENGTH ?
    strlen(model->title) : MAX_LINE_LENGTH;
  if ((status = nc_put_att_text(exoid, NC_GLOBAL, (const char*)ATT_TITLE, 
				title_len+1, model->title)) != NC_NOERR)
    {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to define model->title attribute to file id %d", exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

  /* ...and some dimensions... */

  if ((status = nc_def_dim(exoid, DIM_NUM_DIM, model->num_dim, &numdimdim)) != NC_NOERR)
    {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to define number of dimensions in file id %d",exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

  /*
   * Need to handle "empty file" that may be the result of a strange
   * load balance or some other strange run.  Note that if num_node
   * == 0, then model->num_elem must be zero since you cannot have elements
   * with no nodes. It *is* permissible to have zero elements with
   * non-zero node count.
   */
     
  if (model->num_nodes > 0) {
    if ((status = nc_def_dim(exoid, DIM_NUM_NODES, model->num_nodes, &numnoddim)) != NC_NOERR)
      {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to define number of nodes in file id %d",exoid);
        ex_err("ex_put_init_ext",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }
  }
  
  if (model->num_elem > 0) {
    if (model->num_nodes <=  0) {
      exerrval = EX_MSG;
      sprintf(errmsg,
              "Error: Cannot have non-zero element count if node count is zero.in file id %d",exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
    
    if ((status = nc_def_dim(exoid, DIM_NUM_ELEM, model->num_elem, &temp)) != NC_NOERR)
      {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to define number of elements in file id %d",exoid);
        ex_err("ex_put_init_ext",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }
  }

  if (model->num_edge > 0) {
    if (model->num_nodes <=  0) {
      exerrval = EX_MSG;
      sprintf(errmsg,
              "Error: Cannot have non-zero edge count if node count is zero.in file id %d",exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
    
    if ((status = nc_def_dim(exoid, DIM_NUM_EDGE, model->num_edge, &temp)) != NC_NOERR)
      {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to define number of edges in file id %d",exoid);
        ex_err("ex_put_init_ext",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }
  }

  if (model->num_face > 0) {
    if (model->num_nodes <=  0) {
      exerrval = EX_MSG;
      sprintf(errmsg,
              "Error: Cannot have non-zero face count if node count is zero.in file id %d",exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
    
    if ((status = nc_def_dim(exoid, DIM_NUM_FACE, model->num_face, &temp)) != NC_NOERR)
      {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to define number of faces in file id %d",exoid);
        ex_err("ex_put_init_ext",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }
  }

  EX_WRITE_OBJECT_PARAMS("element block", DIM_NUM_EL_BLK, VAR_STAT_EL_BLK, VAR_ID_EL_BLK, model->num_elem_blk, elblkdim);
  EX_WRITE_OBJECT_PARAMS("edge block",    DIM_NUM_ED_BLK, VAR_STAT_ED_BLK, VAR_ID_ED_BLK, model->num_edge_blk, edblkdim);
  EX_WRITE_OBJECT_PARAMS("face block",    DIM_NUM_FA_BLK, VAR_STAT_FA_BLK, VAR_ID_FA_BLK, model->num_face_blk, fablkdim);

  EX_WRITE_OBJECT_PARAMS("node set", DIM_NUM_NS,  VAR_NS_STAT,  VAR_NS_IDS, model->num_node_sets,  nsetdim);
  EX_WRITE_OBJECT_PARAMS("edge set", DIM_NUM_ES,  VAR_ES_STAT,  VAR_ES_IDS, model->num_edge_sets,  esetdim);
  EX_WRITE_OBJECT_PARAMS("face set", DIM_NUM_FS,  VAR_FS_STAT,  VAR_FS_IDS, model->num_face_sets,  fsetdim);
  EX_WRITE_OBJECT_PARAMS("side set", DIM_NUM_SS,  VAR_SS_STAT,  VAR_SS_IDS, model->num_side_sets,  ssetdim);
  EX_WRITE_OBJECT_PARAMS("elem set", DIM_NUM_ELS, VAR_ELS_STAT, VAR_ELS_IDS, model->num_elem_sets, elsetdim);

  EX_WRITE_MAP_PARAMS(   "node map", DIM_NUM_NM,  VAR_NM_PROP(1), model->num_node_maps, nmapdim);
  EX_WRITE_MAP_PARAMS(   "edge map", DIM_NUM_EDM, VAR_EDM_PROP(1), model->num_edge_maps, edmapdim);
  EX_WRITE_MAP_PARAMS(   "face map", DIM_NUM_FAM, VAR_FAM_PROP(1), model->num_face_maps, famapdim);
  EX_WRITE_MAP_PARAMS("element map", DIM_NUM_EM,  VAR_EM_PROP(1), model->num_elem_maps, emapdim);

  /*
   * To reduce the maximum dataset sizes, the storage of the nodal
   * coordinates and the nodal variables was changed from a single
   * dataset to a dataset per component or variable.  However, we
   * want to maintain some form of compatability with the old
   * exodusII version.  It is easy to do this on read; however, we
   * also want to be able to store in the old format using the new
   * library. 
   *
   * The mode is set in the ex_create call. The setting can be checked
   * via the ATT_FILESIZE attribute in the file (1=large,
   * 0=normal). Also handle old files that do not contain this
   * attribute.
   */

  if (model->num_nodes > 0) {
    if (ex_large_model(exoid) == 1) {
      /* node coordinate arrays -- separate storage... */

      dim[0] = numnoddim;
      if (model->num_dim > 0) {
        if ((status = nc_def_var (exoid, VAR_COORD_X, nc_flt_code(exoid), 1, dim, &temp)) != NC_NOERR)
          {
            exerrval = status;
            sprintf(errmsg,
                    "Error: failed to define node x coordinate array in file id %d",exoid);
            ex_err("ex_put_init_ext",errmsg,exerrval);
            goto error_ret;         /* exit define mode and return */
          }
      }
    
      if (model->num_dim > 1) {
        if ((status = nc_def_var(exoid, VAR_COORD_Y, nc_flt_code(exoid), 1, dim, &temp)) != NC_NOERR)
          {
            exerrval = status;
            sprintf(errmsg,
                    "Error: failed to define node y coordinate array in file id %d",exoid);
            ex_err("ex_put_init_ext",errmsg,exerrval);
            goto error_ret;         /* exit define mode and return */
          }
      }

      if (model->num_dim > 2) {
        if ((status = nc_def_var(exoid, VAR_COORD_Z, nc_flt_code(exoid), 1, dim, &temp)) != NC_NOERR)
          {
            exerrval = status;
            sprintf(errmsg,
                    "Error: failed to define node z coordinate array in file id %d",exoid);
            ex_err("ex_put_init_ext",errmsg,exerrval);
            goto error_ret;         /* exit define mode and return */
          }
      }
    } else {
      /* node coordinate arrays: -- all stored together (old method) */

      dim[0] = numdimdim;
      dim[1] = numnoddim;
      if ((status = nc_def_var(exoid, VAR_COORD, nc_flt_code(exoid), 2, dim, &temp)) != NC_NOERR)
        {
          exerrval = status;
          sprintf(errmsg,
                  "Error: failed to define node coordinate array in file id %d",exoid);
          ex_err("ex_put_init_ext",errmsg,exerrval);
          goto error_ret;         /* exit define mode and return */
        }
    }
  }
  
  /* inquire previously defined dimensions  */
  if ((status = nc_inq_dimid (exoid, DIM_STR, &strdim)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to get string length in file id %d",exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

  /* coordinate names array */

  dim[0] = numdimdim;
  dim[1] = strdim;

  if ((status=nc_def_var(exoid, VAR_NAME_COOR, NC_CHAR, 2, dim, &temp)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to define coordinate name array in file id %d",exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
  
  EX_WRITE_OBJECT_NAMES("element block",VAR_NAME_EL_BLK,elblkdim,model->num_elem_blk);
  EX_WRITE_OBJECT_NAMES("edge block",   VAR_NAME_ED_BLK,edblkdim,model->num_edge_blk);
  EX_WRITE_OBJECT_NAMES("face block",   VAR_NAME_FA_BLK,fablkdim,model->num_face_blk);
  EX_WRITE_OBJECT_NAMES("node set",     VAR_NAME_NS,    nsetdim, model->num_node_sets);
  EX_WRITE_OBJECT_NAMES("edge set",     VAR_NAME_ES,    esetdim, model->num_edge_sets);
  EX_WRITE_OBJECT_NAMES("face set",     VAR_NAME_FS,    fsetdim, model->num_face_sets);
  EX_WRITE_OBJECT_NAMES("side set",     VAR_NAME_SS,    ssetdim, model->num_side_sets);
  EX_WRITE_OBJECT_NAMES("element set",  VAR_NAME_ELS,   elsetdim,model->num_elem_sets);
  EX_WRITE_OBJECT_NAMES("node map",     VAR_NAME_NM,    nmapdim, model->num_node_maps);
  EX_WRITE_OBJECT_NAMES("edge map",     VAR_NAME_EDM,   edmapdim,model->num_edge_maps);
  EX_WRITE_OBJECT_NAMES("face map",     VAR_NAME_FAM,   famapdim,model->num_face_maps);
  EX_WRITE_OBJECT_NAMES("element map",  VAR_NAME_EM,    emapdim, model->num_elem_maps);
  
  /* leave define mode */
#if 1
  if ((status = nc_enddef (exoid)) != NC_NOERR)
    {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to complete variable definitions in file id %d",exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      return (EX_FATAL);
    }
  
#else
  /* estimate (guess) size of header of netCDF file */
  header_size = 1200 + 
    model->num_elem_blk * 800 + 
    model->num_node_sets * 220 + 
    model->num_side_sets * 300;

  if (header_size > MAX_HEADER_SIZE) header_size = MAX_HEADER_SIZE;

  /* estimate (guess) size of fixed size variable section of netCDF file */

  if (nc_flt_code(exoid) == NC_DOUBLE) 
    iows = 8;
  else
    iows = 4;

  fixed_var_size = model->num_dim * model->num_nodes * iows +
    model->num_nodes * sizeof(int) +
    model->num_elem * 16 * sizeof(int) +
    model->num_elem_blk * sizeof(int) +
    model->num_node_sets * model->num_nodes/100 * sizeof(int) +
    model->num_node_sets * model->num_nodes/100 * iows +
    model->num_node_sets * sizeof(int) +
    model->num_side_sets * model->num_elem/100 * 2 * sizeof(int) +
    model->num_side_sets * model->num_elem/100 * iows +
    model->num_side_sets * sizeof(int);

  /* With netcdf-3.4, this produces very large files on the
   * SGI.  Also with netcdf-3.5beta3
   */
  /*
   * This is also causing other problems on other systems .. disable for now
   */
  if ((status = nc__enddef (exoid, 
                  header_size, NC_ALIGN_CHUNK, 
			    fixed_var_size, NC_ALIGN_CHUNK)) != NC_NOERR)
    {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to complete variable definitions in file id %d",exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      return (EX_FATAL);
    }

#endif
  /* Fill the id and status arrays with zeros */
  {
    int *zeros = NULL;
    int maxset = model->num_elem_blk;
    if (maxset < model->num_elem_blk)  maxset = model->num_elem_blk;
    if (maxset < model->num_edge_blk)  maxset = model->num_edge_blk;
    if (maxset < model->num_face_blk)  maxset = model->num_face_blk;
    if (maxset < model->num_node_sets) maxset = model->num_node_sets;
    if (maxset < model->num_edge_sets) maxset = model->num_edge_sets;
    if (maxset < model->num_face_sets) maxset = model->num_face_sets;
    if (maxset < model->num_side_sets) maxset = model->num_side_sets;
    if (maxset < model->num_elem_sets) maxset = model->num_elem_sets;

    /* allocate space for id/status array */
    if (!(zeros = malloc(maxset*sizeof(int)))) {
      exerrval = EX_MEMFAIL;
      sprintf(errmsg,
             "Error: failed to allocate memory for id/status array for file id %d", exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      return (EX_FATAL);
    }
    
    zero_id_status(exoid, VAR_STAT_EL_BLK, VAR_ID_EL_BLK, model->num_elem_blk, zeros);
    zero_id_status(exoid, VAR_STAT_ED_BLK, VAR_ID_ED_BLK, model->num_edge_blk, zeros);
    zero_id_status(exoid, VAR_STAT_FA_BLK, VAR_ID_FA_BLK, model->num_face_blk, zeros);
    
    zero_id_status(exoid, VAR_NS_STAT,  VAR_NS_IDS,  model->num_node_sets, zeros);
    zero_id_status(exoid, VAR_ES_STAT,  VAR_ES_IDS,  model->num_edge_sets, zeros);
    zero_id_status(exoid, VAR_FS_STAT,  VAR_FS_IDS,  model->num_face_sets, zeros);
    zero_id_status(exoid, VAR_SS_STAT,  VAR_SS_IDS,  model->num_side_sets, zeros);
    zero_id_status(exoid, VAR_ELS_STAT, VAR_ELS_IDS, model->num_elem_sets, zeros);
    if (zeros != NULL) {
      free(zeros);
      zeros = NULL;
    }
  }  
  return (EX_NOERR);
  
  /* Fatal error: exit definition mode and return */
 error_ret:
  if (nc_enddef (exoid) != NC_NOERR)     /* exit define mode */
    {
      sprintf(errmsg,
              "Error: failed to complete definition for file id %d",
              exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
    }
  return (EX_FATAL);
}

