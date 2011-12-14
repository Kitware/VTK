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
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void write_dummy_names(int exoid, ex_entity_type obj_type)
{
  const char *routine = "write_dummy_names";
  size_t  start[2], count[2];
  char *text = "";
  int varid;
  size_t num_entity;
  size_t i;
  
  ex_get_dimension(exoid, ex_dim_num_objects(obj_type),
       ex_name_of_object(obj_type),
       &num_entity, &varid, routine);
  
  for (i = 0; i < num_entity; i++) {
    start[0] = i;
    count[0] = 1;

    start[1] = 0;
    count[1] = strlen(text)+1;

    nc_put_vara_text(exoid, varid, start, count, text);
  }
}

static int ex_write_object_names(int exoid, const char *type, const char *dimension_name,
         int dimension_var, int string_dimension, int count)
{
  int dim[2];
  int status;
  int varid;
  char errmsg[MAX_ERR_LENGTH];

  if (count > 0) { 
    dim[0] = dimension_var;
    dim[1] = string_dimension;

    if ((status = nc_def_var (exoid, dimension_name, NC_CHAR, 2, dim, &varid)) != NC_NOERR) { 
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to define %s name array in file id %d",type,exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      return status;         /* exit define mode and return */
    }
  }
  return NC_NOERR;
}

static int ex_write_object_params(int exoid, const char *type, const char *dimension_name,
          const char *status_dim_name, const char *id_array_dim_name,
          int count, int *dimension)
{
  int dim[2];
  int varid;
  int status;
  char errmsg[MAX_ERR_LENGTH];
  
  /* Can have nonzero model->num_elem_blk even if model->num_elem == 0 */
  if (count > 0) {
    if ((status = nc_def_dim(exoid, dimension_name, (size_t)count, dimension)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to define number of %ss in file id %d",
        type, exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      return status;         /* exit define mode and return */
    }
    /* ...and some variables */
    /* element block id status array */
    dim[0] = *dimension;
    if ((status = nc_def_var (exoid, status_dim_name, NC_INT, 1, dim, &varid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to define %s status array in file id %d", type, exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      return status;         /* exit define mode and return */
    }
   
    /* type id array */
    if ((status = nc_def_var (exoid, id_array_dim_name, NC_INT, 1, dim, &varid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to define %s id array in file id %d", type, exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      return status;         /* exit define mode and return */
    }
   
    /*   store property name as attribute of property array variable */
    if ((status=nc_put_att_text(exoid, varid, ATT_PROP_NAME, 3, "ID")) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to store %s property name %s in file id %d",
        type, "ID", exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      return status;
    }
  }
  return NC_NOERR;
}

static int ex_write_map_params(int exoid, const char *map_name, const char *map_dim_name,
             const char *map_id_name, int map_count, int *map_dimension)
{
  int dim[2];
  int varid;
  int status;
  char errmsg[MAX_ERR_LENGTH];
  
  /* Can have nonzero model->num_XXXX_map even if model->num_XXXX == 0 */
  if ((map_count) > 0) {
    if ((status = nc_def_dim(exoid, map_dim_name, (size_t)(map_count), map_dimension)) != NC_NOERR) {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to define number of %ss in file id %d",
                map_name, exoid);
        ex_err("ex_put_init_ext",errmsg,exerrval);
        return status;         /* exit define mode and return */
      }

    dim[0] = *map_dimension;

    /* map_name id array */
    if ((status = nc_def_var(exoid, map_id_name, NC_INT, 1, dim, &varid)) != NC_NOERR) {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to define %s id array in file id %d", map_name, exoid);
        ex_err("ex_put_init_ext",errmsg,exerrval);
        return status;         /* exit define mode and return */
      }

    /*   store property name as attribute of property array variable */
    if ((status=nc_put_att_text(exoid, varid, ATT_PROP_NAME, 3, "ID")) != NC_NOERR) {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to store %s property name %s in file id %d",
                map_name, "ID",exoid);
        ex_err("ex_put_init_ext",errmsg,exerrval);
        return (EX_FATAL);
      }
  }
  return NC_NOERR;
}

static void invalidate_id_status(int exoid, const char *var_stat,
         const char *var_id, int count, int *ids)
{
  int status;
  int i;
  int id_var, stat_var;
  
  if (count > 0) {
    if (var_id != 0) {
      for (i=0; i < count; i++) {
  ids[i] = EX_INVALID_ID;
      }

      status = nc_inq_varid(exoid, var_id,   &id_var);
      assert(status == NC_NOERR);
      status = nc_put_var_int(exoid, id_var,   ids);
      assert(status == NC_NOERR);
    }

    if (var_stat != 0) {
      for (i=0; i < count; i++) {
  ids[i] = 0;
      }

      status = nc_inq_varid(exoid, var_stat, &stat_var);
      assert(status == NC_NOERR);
      status = nc_put_var_int(exoid, stat_var, ids);
      assert(status == NC_NOERR);
    }
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
  int numdimdim, numnoddim, elblkdim, edblkdim, fablkdim, esetdim,
    fsetdim, elsetdim, nsetdim, ssetdim, dim_str_name, dim[2], temp;
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

  /* create name string length dimension */
  {
    int max_name = ex_max_name_length < 32 ? 32 : ex_max_name_length;
    if ((status=nc_def_dim (exoid, DIM_STR_NAME, max_name+1, &dim_str_name)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to define name string length in file id %d",exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      goto error_ret;
    }
  }

  {
    int max_so_far = 32;
    if ((status=nc_put_att_int(exoid, NC_GLOBAL, ATT_MAX_NAME_LENGTH, NC_INT, 1, &max_so_far)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to add maximum_name_length attribute in file id %d",exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      goto error_ret;
    }
  }

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

  if (ex_write_object_params(exoid, "element block", DIM_NUM_EL_BLK, VAR_STAT_EL_BLK, VAR_ID_EL_BLK, model->num_elem_blk, &elblkdim)) goto error_ret;
  if (ex_write_object_params(exoid, "edge block",    DIM_NUM_ED_BLK, VAR_STAT_ED_BLK, VAR_ID_ED_BLK, model->num_edge_blk, &edblkdim)) goto error_ret;
  if (ex_write_object_params(exoid, "face block",    DIM_NUM_FA_BLK, VAR_STAT_FA_BLK, VAR_ID_FA_BLK, model->num_face_blk, &fablkdim)) goto error_ret;
  
  if (ex_write_object_params(exoid, "node set", DIM_NUM_NS,  VAR_NS_STAT,  VAR_NS_IDS, model->num_node_sets,  &nsetdim)) goto error_ret;
  if (ex_write_object_params(exoid, "edge set", DIM_NUM_ES,  VAR_ES_STAT,  VAR_ES_IDS, model->num_edge_sets,  &esetdim)) goto error_ret;
  if (ex_write_object_params(exoid, "face set", DIM_NUM_FS,  VAR_FS_STAT,  VAR_FS_IDS, model->num_face_sets,  &fsetdim)) goto error_ret;
  if (ex_write_object_params(exoid, "side set", DIM_NUM_SS,  VAR_SS_STAT,  VAR_SS_IDS, model->num_side_sets,  &ssetdim)) goto error_ret;
  if (ex_write_object_params(exoid, "elem set", DIM_NUM_ELS, VAR_ELS_STAT, VAR_ELS_IDS, model->num_elem_sets, &elsetdim)) goto error_ret;

  if (ex_write_map_params(exoid,   "node map",  DIM_NUM_NM,  VAR_NM_PROP(1),  model->num_node_maps, &nmapdim)  != NC_NOERR) goto error_ret;
  if (ex_write_map_params(exoid,   "edge map",  DIM_NUM_EDM, VAR_EDM_PROP(1), model->num_edge_maps, &edmapdim) != NC_NOERR) goto error_ret;
  if (ex_write_map_params(exoid,   "face map",  DIM_NUM_FAM, VAR_FAM_PROP(1), model->num_face_maps, &famapdim) != NC_NOERR) goto error_ret;
  if (ex_write_map_params(exoid, "element map", DIM_NUM_EM,  VAR_EM_PROP(1),  model->num_elem_maps, &emapdim)  != NC_NOERR) goto error_ret;

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
  
  if (ex_write_object_names(exoid, "element block",VAR_NAME_EL_BLK,elblkdim, dim_str_name, model->num_elem_blk) != NC_NOERR) goto error_ret;
  if (ex_write_object_names(exoid, "edge block",   VAR_NAME_ED_BLK,edblkdim, dim_str_name, model->num_edge_blk) != NC_NOERR) goto error_ret;
  if (ex_write_object_names(exoid, "face block",   VAR_NAME_FA_BLK,fablkdim, dim_str_name, model->num_face_blk) != NC_NOERR) goto error_ret;
  if (ex_write_object_names(exoid, "node set",     VAR_NAME_NS,    nsetdim,  dim_str_name, model->num_node_sets) != NC_NOERR) goto error_ret;
  if (ex_write_object_names(exoid, "edge set",     VAR_NAME_ES,    esetdim,  dim_str_name, model->num_edge_sets) != NC_NOERR) goto error_ret;
  if (ex_write_object_names(exoid, "face set",     VAR_NAME_FS,    fsetdim,  dim_str_name, model->num_face_sets) != NC_NOERR) goto error_ret;
  if (ex_write_object_names(exoid, "side set",     VAR_NAME_SS,    ssetdim,  dim_str_name, model->num_side_sets) != NC_NOERR) goto error_ret;
  if (ex_write_object_names(exoid, "element set",  VAR_NAME_ELS,   elsetdim, dim_str_name, model->num_elem_sets) != NC_NOERR) goto error_ret;
  if (ex_write_object_names(exoid, "node map",     VAR_NAME_NM,    nmapdim,  dim_str_name, model->num_node_maps) != NC_NOERR) goto error_ret;
  if (ex_write_object_names(exoid, "edge map",     VAR_NAME_EDM,   edmapdim, dim_str_name, model->num_edge_maps) != NC_NOERR) goto error_ret;
  if (ex_write_object_names(exoid, "face map",     VAR_NAME_FAM,   famapdim, dim_str_name, model->num_face_maps) != NC_NOERR) goto error_ret;
  if (ex_write_object_names(exoid, "element map",  VAR_NAME_EM,    emapdim,  dim_str_name, model->num_elem_maps) != NC_NOERR) goto error_ret;
  if (ex_write_object_names(exoid, "coordinate",   VAR_NAME_COOR,  numdimdim,dim_str_name, model->num_dim) != NC_NOERR) goto error_ret;

  /* leave define mode */
  if ((status = nc_enddef (exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to complete variable definitions in file id %d",exoid);
    ex_err("ex_put_init_ext",errmsg,exerrval);
    return (EX_FATAL);
  }
  
  /* Fill the id and status arrays with EX_INVALID_ID */
  {
    int *invalid_ids = NULL;
    int maxset = model->num_elem_blk;
    if (maxset < model->num_edge_blk)  maxset = model->num_edge_blk;
    if (maxset < model->num_face_blk)  maxset = model->num_face_blk;
    if (maxset < model->num_node_sets) maxset = model->num_node_sets;
    if (maxset < model->num_edge_sets) maxset = model->num_edge_sets;
    if (maxset < model->num_face_sets) maxset = model->num_face_sets;
    if (maxset < model->num_side_sets) maxset = model->num_side_sets;
    if (maxset < model->num_elem_sets) maxset = model->num_elem_sets;
    if (maxset < model->num_node_maps) maxset = model->num_node_maps;
    if (maxset < model->num_edge_maps) maxset = model->num_edge_maps;
    if (maxset < model->num_face_maps) maxset = model->num_face_maps;
    if (maxset < model->num_elem_maps) maxset = model->num_elem_maps;

    /* allocate space for id/status array */
    if (!(invalid_ids = malloc(maxset*sizeof(int)))) {
      exerrval = EX_MEMFAIL;
      sprintf(errmsg,
        "Error: failed to allocate memory for id/status array for file id %d", exoid);
      ex_err("ex_put_init_ext",errmsg,exerrval);
      return (EX_FATAL);
    }
    
    invalidate_id_status(exoid, VAR_STAT_EL_BLK, VAR_ID_EL_BLK,
       model->num_elem_blk, invalid_ids);
    invalidate_id_status(exoid, VAR_STAT_ED_BLK, VAR_ID_ED_BLK,
       model->num_edge_blk, invalid_ids);
    invalidate_id_status(exoid, VAR_STAT_FA_BLK, VAR_ID_FA_BLK,
       model->num_face_blk, invalid_ids);
    invalidate_id_status(exoid, VAR_NS_STAT,  VAR_NS_IDS,
       model->num_node_sets, invalid_ids);
    invalidate_id_status(exoid, VAR_ES_STAT,  VAR_ES_IDS,
       model->num_edge_sets, invalid_ids);
    invalidate_id_status(exoid, VAR_FS_STAT,  VAR_FS_IDS,
       model->num_face_sets, invalid_ids);
    invalidate_id_status(exoid, VAR_SS_STAT,  VAR_SS_IDS,
       model->num_side_sets, invalid_ids);
    invalidate_id_status(exoid, VAR_ELS_STAT, VAR_ELS_IDS,
       model->num_elem_sets, invalid_ids);

    invalidate_id_status(exoid, 0, VAR_NM_PROP(1),  model->num_node_maps, invalid_ids);
    invalidate_id_status(exoid, 0, VAR_EDM_PROP(1), model->num_edge_maps, invalid_ids);
    invalidate_id_status(exoid, 0, VAR_FAM_PROP(1), model->num_face_maps, invalid_ids);
    invalidate_id_status(exoid, 0, VAR_EM_PROP(1),  model->num_elem_maps, invalid_ids);

    if (invalid_ids != NULL) {
      free(invalid_ids);
      invalid_ids = NULL;
    }
  }

  /* Write dummy values to the names arrays to avoid corruption issues on some platforms */
  if (model->num_elem_blk > 0) write_dummy_names(exoid, EX_ELEM_BLOCK);
  if (model->num_edge_blk > 0) write_dummy_names(exoid, EX_EDGE_BLOCK);
  if (model->num_face_blk > 0) write_dummy_names(exoid, EX_FACE_BLOCK);
  if (model->num_node_sets> 0) write_dummy_names(exoid, EX_NODE_SET);
  if (model->num_edge_sets> 0) write_dummy_names(exoid, EX_EDGE_SET);
  if (model->num_face_sets> 0) write_dummy_names(exoid, EX_FACE_SET);
  if (model->num_side_sets> 0) write_dummy_names(exoid, EX_SIDE_SET);
  if (model->num_elem_sets> 0) write_dummy_names(exoid, EX_ELEM_SET);
  if (model->num_node_maps> 0) write_dummy_names(exoid, EX_NODE_MAP);
  if (model->num_edge_maps> 0) write_dummy_names(exoid, EX_EDGE_MAP);
  if (model->num_face_maps> 0) write_dummy_names(exoid, EX_FACE_MAP);
  if (model->num_elem_maps> 0) write_dummy_names(exoid, EX_ELEM_MAP);

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

