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
* expvp - ex_put_all_var_param_ext
*
* entry conditions - 
*   input parameters:
*       int                  exoid    exodus file id
*       const ex_var_params* vp       pointer to variable parameter info
*
* exit conditions - 
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

#include <ctype.h>
static void *safe_free(void *array);
static int define_dimension(int exoid, const char *DIMENSION, int count, const char *label, int *dimid);
static int define_variable_name_variable(int exoid, const char *VARIABLE, int dimension,
                                         const char *label);
static int *get_status_array(int exoid, int count, const char *VARIABLE, const char *label);
static int put_truth_table(int exoid, int varid, int *table, const char *label);
static int define_truth_table(ex_entity_type obj_type, int exoid, int num_ent, int num_var,
                              int *var_tab, int *status, int *ids, const char *label);

#define EX_GET_IDS_STATUS(TNAME,NUMVAR,DNAME,DID,DVAL,VIDS,EIDS,VSTAT,VSTATVAL) \
  if (NUMVAR > 0) {             \
    status = ex_get_dimension(exoid, DNAME, TNAME "s", &DVAL, &DID, routine); \
    if (status != NC_NOERR)           \
      goto error_ret;             \
                  \
    /* get element block IDs */           \
    if (!(VIDS = malloc(DVAL*sizeof(int)))) {       \
      exerrval = EX_MEMFAIL;            \
      sprintf(errmsg,             \
              "Error: failed to allocate memory for " TNAME " id array for file id %d", \
              exoid);             \
      ex_err("ex_put_all_var_param_ext",errmsg,exerrval);   \
      goto error_ret;             \
    }                 \
    ex_get_ids (exoid, EIDS, VIDS);         \
                  \
    /* Get element block status array for later use (allocates memory) */ \
    VSTATVAL = get_status_array(exoid, DVAL, VSTAT, TNAME);   \
    if (VSTATVAL == NULL) {           \
      goto error_ret;             \
    }                 \
  }

/*!
 * writes the number of global, nodal, element, nodeset, and sideset variables 
 * that will be written to the database
 * \param      exoid    exodus file id
 * \param      *vp       pointer to variable parameter info
 */

int ex_put_all_var_param_ext ( int   exoid,
                               const ex_var_params* vp )
{
  int in_define = 0;
  int status;
  int temp;
  int time_dim, num_nod_dim, dimid;
  size_t num_elem_blk, num_edge_blk, num_face_blk;
  size_t num_nset, num_eset, num_fset, num_sset, num_elset;
  int numelblkdim, numelvardim, numedvardim, numedblkdim,
    numfavardim, numfablkdim,  numnsetdim,  nsetvardim,
    numesetdim,  esetvardim,   numfsetdim,  fsetvardim,
    numssetdim,  ssetvardim,  numelsetdim, elsetvardim;
  int i;

  int edblk_varid, fablk_varid, eblk_varid, nset_varid,
    eset_varid, fset_varid, sset_varid, elset_varid, varid;
  
  int* eblk_ids = 0;
  int* edblk_ids = 0;
  int* fablk_ids = 0;
  int* nset_ids = 0;
  int* eset_ids = 0;
  int* fset_ids = 0;
  int* sset_ids = 0;
  int* elset_ids = 0;

  int* eblk_stat = 0;
  int* edblk_stat = 0;
  int* fablk_stat = 0;
  int* nset_stat = 0;
  int* eset_stat = 0;
  int* fset_stat = 0;
  int* sset_stat = 0;
  int* elset_stat = 0;
  
  int dims[3];
  char errmsg[MAX_ERR_LENGTH];
  const char* routine = "ex_put_all_var_param_ext";

  exerrval = 0; /* clear error code */

  /* inquire previously defined dimensions  */

  if ((status = nc_inq_dimid(exoid, DIM_TIME, &time_dim)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to locate time dimension in file id %d", exoid);
    ex_err("ex_put_all_var_param_ext",errmsg,exerrval);
    goto error_ret;
  }

  if ((status = nc_inq_dimid (exoid, DIM_NUM_NODES, &num_nod_dim)) != NC_NOERR) {
    if (vp->num_node > 0) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to locate number of nodes in file id %d",
              exoid);
      ex_err("ex_put_all_var_param_ext",errmsg,exerrval);
      goto error_ret;
    }
  }

  /* Check this now so we can use it later without checking for errors */
  if ((status = nc_inq_dimid(exoid, DIM_STR_NAME, &temp)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to get string length in file id %d",exoid);
    ex_err("ex_put_all_var_param_ext",errmsg,exerrval);
    goto error_ret;
  }

  EX_GET_IDS_STATUS(   "edge block",vp->num_edge, DIM_NUM_ED_BLK,numedblkdim,num_edge_blk,edblk_ids,EX_EDGE_BLOCK,VAR_STAT_ED_BLK,edblk_stat);
  EX_GET_IDS_STATUS(   "face block",vp->num_face, DIM_NUM_FA_BLK,numfablkdim,num_face_blk,fablk_ids,EX_FACE_BLOCK,VAR_STAT_FA_BLK,fablk_stat);
  EX_GET_IDS_STATUS("element block",vp->num_elem, DIM_NUM_EL_BLK,numelblkdim,num_elem_blk, eblk_ids,EX_ELEM_BLOCK,VAR_STAT_EL_BLK, eblk_stat);
  EX_GET_IDS_STATUS(     "node set",vp->num_nset, DIM_NUM_NS,    numnsetdim, num_nset,     nset_ids,EX_NODE_SET,  VAR_NS_STAT,     nset_stat);
  EX_GET_IDS_STATUS(     "edge set",vp->num_eset, DIM_NUM_ES,    numesetdim, num_eset,     eset_ids,EX_EDGE_SET,  VAR_ES_STAT,     eset_stat);
  EX_GET_IDS_STATUS(     "face set",vp->num_fset, DIM_NUM_FS,    numfsetdim, num_fset,     fset_ids,EX_FACE_SET,  VAR_FS_STAT,     fset_stat);
  EX_GET_IDS_STATUS(     "side set",vp->num_sset, DIM_NUM_SS,    numssetdim, num_sset,     sset_ids,EX_SIDE_SET,  VAR_SS_STAT,     sset_stat);
  EX_GET_IDS_STATUS(  "element set",vp->num_elset,DIM_NUM_ELS,   numelsetdim,num_elset,   elset_ids,EX_ELEM_SET,  VAR_ELS_STAT,   elset_stat);

  /* put file into define mode  */
  if ((status = nc_redef (exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to put file id %d into define mode", exoid);
    ex_err("ex_put_all_var_param_ext",errmsg,exerrval);
    goto error_ret;
  }
  in_define = 1;

  /* define dimensions and variables */
  if (vp->num_glob > 0) {
    if (define_dimension(exoid, DIM_NUM_GLO_VAR, vp->num_glob, "global", &dimid) != NC_NOERR)
      goto error_ret;
    
    dims[0] = time_dim;
    dims[1] = dimid;
    if ((status = nc_def_var (exoid, VAR_GLO_VAR, nc_flt_code(exoid), 2, dims, &varid)) != NC_NOERR)
      {
  exerrval = status;
  sprintf(errmsg,
    "Error: failed to define global variables in file id %d",
    exoid);
  ex_err("ex_put_all_var_param_ext",errmsg,exerrval);
  goto error_ret;          /* exit define mode and return */
      }

    /* Now define global variable name variable */
    if (define_variable_name_variable(exoid, VAR_NAME_GLO_VAR, dimid, "global") != NC_NOERR)
      goto error_ret;
  }

  if (vp->num_node > 0) {
    /*
     * There are two ways to store the nodal variables. The old way *
     * was a blob (#times,#vars,#nodes), but that was exceeding the
     * netcdf maximum dataset size for large models. The new way is
     * to store #vars separate datasets each of size (#times,#nodes)
     *
     * We want this routine to be capable of storing both formats
     * based on some external flag.  Since the storage format of the
     * coordinates have also been changed, we key off of their
     * storage type to decide which method to use for nodal
     * variables. If the variable 'coord' is defined, then store old
     * way; otherwise store new.
     */
    if (define_dimension(exoid, DIM_NUM_NOD_VAR, vp->num_node, "nodal", &dimid) != NC_NOERR)
      goto error_ret;

    if (ex_large_model(exoid) == 0) { /* Old way */
      dims[0] = time_dim;
      dims[1] = dimid;
      dims[2] = num_nod_dim;
      if ((status = nc_def_var(exoid, VAR_NOD_VAR,
             nc_flt_code(exoid), 3, dims, &varid)) != NC_NOERR) {
  exerrval = status;
  sprintf(errmsg,
    "Error: failed to define nodal variables in file id %d",
    exoid);
  ex_err("ex_put_all_var_param_ext",errmsg,exerrval);
  goto error_ret;          /* exit define mode and return */
      }
    } else { /* Store new way */
      for (i = 1; i <= vp->num_node; i++) {
  dims[0] = time_dim;
  dims[1] = num_nod_dim;
  if ((status = nc_def_var(exoid, VAR_NOD_VAR_NEW(i),
         nc_flt_code(exoid), 2, dims, &varid)) != NC_NOERR)
    {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to define nodal variable %d in file id %d",
        i, exoid);
      ex_err("ex_put_var_param",errmsg,exerrval);
      goto error_ret;          /* exit define mode and return */
    }
      }
    }

    /* Now define nodal variable name variable */
    if (define_variable_name_variable(exoid, VAR_NAME_NOD_VAR, dimid, "nodal") != NC_NOERR)
      goto error_ret;
  }

#define EX_DEFINE_VARS(TID,STNAME,TNAME,NUMVAR,DNAME,DID1,DID2,DVAL,VIDS,VNOV,VTV,VSTATVAL,VTABVAL,VTABVAR) \
  if (NUMVAR > 0) {             \
    status = define_dimension(exoid, DNAME, NUMVAR, STNAME, &DID2); \
    if (status != NC_NOERR) goto error_ret;       \
                  \
    /* Now define STNAME variable name variable */      \
    if (define_variable_name_variable(exoid, VNOV, DID2, STNAME) != NC_NOERR) \
      goto error_ret;             \
                  \
    if (define_truth_table(TID, exoid, DVAL, NUMVAR, VTABVAL, VSTATVAL, VIDS, TNAME) != NC_NOERR) \
      goto error_ret;             \
                  \
    VSTATVAL = safe_free (VSTATVAL);          \
    VIDS  = safe_free (VIDS);           \
                  \
    /* create a variable array in which to store the STNAME variable truth \
     * table                \
     */                 \
                  \
    dims[0] = DID1;             \
    dims[1] = DID2;             \
                  \
    if ((status = nc_def_var(exoid, VTV, NC_INT, 2, dims, &VTABVAR)) != NC_NOERR) { \
      exerrval = status;            \
      sprintf(errmsg,             \
              "Error: failed to define " STNAME " variable truth table in file id %d", \
              exoid);             \
      ex_err("ex_put_all_var_param_ext",errmsg,exerrval);   \
      goto error_ret;          /* exit define mode and return */  \
    }                 \
  }
  EX_DEFINE_VARS(EX_EDGE_BLOCK,   "edge",   "edge block",vp->num_edge, DIM_NUM_EDG_VAR,  numedblkdim,numedvardim,num_edge_blk,edblk_ids,VAR_NAME_EDG_VAR,  VAR_EBLK_TAB, edblk_stat,vp->edge_var_tab,edblk_varid);
  EX_DEFINE_VARS(EX_FACE_BLOCK,   "face",   "face block",vp->num_face, DIM_NUM_FAC_VAR,  numfablkdim,numfavardim,num_face_blk,fablk_ids,VAR_NAME_FAC_VAR,  VAR_FBLK_TAB, fablk_stat,vp->face_var_tab,fablk_varid);
  EX_DEFINE_VARS(EX_ELEM_BLOCK,"element","element block",vp->num_elem, DIM_NUM_ELE_VAR,  numelblkdim,numelvardim,num_elem_blk, eblk_ids,VAR_NAME_ELE_VAR,  VAR_ELEM_TAB,  eblk_stat,vp->elem_var_tab,eblk_varid);
  EX_DEFINE_VARS(EX_NODE_SET,  "nodeset",     "node set",vp->num_nset, DIM_NUM_NSET_VAR, numnsetdim, nsetvardim, num_nset,     nset_ids,VAR_NAME_NSET_VAR, VAR_NSET_TAB,  nset_stat,vp->nset_var_tab, nset_varid);
  EX_DEFINE_VARS(EX_EDGE_SET,  "edgeset",     "edge set",vp->num_eset, DIM_NUM_ESET_VAR, numesetdim, esetvardim, num_eset,     eset_ids,VAR_NAME_ESET_VAR, VAR_ESET_TAB,  eset_stat,vp->eset_var_tab, eset_varid);
  EX_DEFINE_VARS(EX_FACE_SET,  "faceset",     "face set",vp->num_fset, DIM_NUM_FSET_VAR, numfsetdim, fsetvardim, num_fset,     fset_ids,VAR_NAME_FSET_VAR, VAR_FSET_TAB,  fset_stat,vp->fset_var_tab, fset_varid);
  EX_DEFINE_VARS(EX_SIDE_SET,  "sideset",     "side set",vp->num_sset, DIM_NUM_SSET_VAR, numssetdim, ssetvardim, num_sset,     sset_ids,VAR_NAME_SSET_VAR, VAR_SSET_TAB,  sset_stat,vp->sset_var_tab, sset_varid);
  EX_DEFINE_VARS(EX_ELEM_SET,  "elemset",  "element set",vp->num_elset,DIM_NUM_ELSET_VAR,numelsetdim,elsetvardim,num_elset,   elset_ids,VAR_NAME_ELSET_VAR,VAR_ELSET_TAB,elset_stat,vp->elset_var_tab,elset_varid);

  /* leave define mode  */

  in_define = 0;
  if ((status = nc_enddef (exoid)) != NC_NOERR)
    {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to complete definition in file id %d",
              exoid);
      ex_err("ex_put_all_var_param_ext",errmsg,exerrval);
      goto error_ret;
    }

  /* write out the variable truth tables */
  if (vp->num_edge > 0) {
    if (put_truth_table(exoid, edblk_varid, vp->edge_var_tab, "edge") != NC_NOERR)
      goto error_ret;
  }

  if (vp->num_face > 0) {
    if (put_truth_table(exoid, fablk_varid, vp->face_var_tab, "face") != NC_NOERR)
      goto error_ret;
  }

  if (vp->num_elem > 0) {
    if (put_truth_table(exoid, eblk_varid, vp->elem_var_tab, "element") !=  NC_NOERR)
      goto error_ret;
  }

  if (vp->num_nset > 0) {
    if (put_truth_table(exoid, nset_varid, vp->nset_var_tab, "nodeset") != NC_NOERR)
      goto error_ret;
  }

  if (vp->num_eset > 0) {
    if (put_truth_table(exoid, eset_varid, vp->eset_var_tab, "edgeset") != NC_NOERR)
      goto error_ret;
  }

  if (vp->num_fset > 0) {
    if (put_truth_table(exoid, fset_varid, vp->fset_var_tab, "faceset") != NC_NOERR)
      goto error_ret;
  }

  if (vp->num_sset > 0) {
    if (put_truth_table(exoid, sset_varid, vp->sset_var_tab, "sideset") != NC_NOERR)
      goto error_ret;
  }

  if (vp->num_elset > 0) {
    if (put_truth_table(exoid, elset_varid, vp->elset_var_tab, "elemset") != NC_NOERR)
      goto error_ret;
  }

  return(EX_NOERR);
  
  /* Fatal error: exit definition mode and return */
 error_ret:
  if (in_define == 1) {
    if (nc_enddef (exoid) != NC_NOERR)     /* exit define mode */
      {
        sprintf(errmsg,
                "Error: failed to complete definition for file id %d",
                exoid);
        ex_err("ex_put_all_var_param_ext",errmsg,exerrval);
      }
  }
  safe_free(eblk_ids);
  safe_free(edblk_ids);
  safe_free(fablk_ids);
  safe_free(nset_ids);
  safe_free(eset_ids);
  safe_free(fset_ids);
  safe_free(sset_ids);
  safe_free(elset_ids);

  safe_free(eblk_stat);
  safe_free(edblk_stat);
  safe_free(fablk_stat);
  safe_free(nset_stat);
  safe_free(eset_stat);
  safe_free(fset_stat);
  safe_free(sset_stat);
  safe_free(elset_stat);
  return(EX_FATAL);
}

int define_dimension(int exoid, const char *DIMENSION, int count, const char *label, int *dimid)
{
  char errmsg[MAX_ERR_LENGTH];
  int status;
  if ((status = nc_def_dim(exoid, DIMENSION, count, dimid)) != NC_NOERR) {
    exerrval = status;
    if (status == NC_ENAMEINUSE) {
      sprintf(errmsg,
              "Error: %s variable name parameters are already defined in file id %d",
              label, exoid);
      ex_err("ex_put_all_var_param_ext",errmsg,exerrval);
    } else {
      sprintf(errmsg,
              "Error: failed to define number of %s variables in file id %d",
              label, exoid);
      ex_err("ex_put_all_var_param_ext",errmsg,exerrval);
    }
  }
  return status;
}

int define_variable_name_variable(int exoid, const char *VARIABLE, int dimension, const char *label)
{
  char errmsg[MAX_ERR_LENGTH];
  int dims[2];
  int variable;
  int status;
  
  dims[0] = dimension;
  nc_inq_dimid(exoid, DIM_STR_NAME, &dims[1]); /* Checked earlier, so known to exist */

  if ((status=nc_def_var(exoid, VARIABLE, NC_CHAR, 2, dims, &variable)) != NC_NOERR) {
    if (status == NC_ENAMEINUSE) {
      exerrval = status;
      sprintf(errmsg,
              "Error: %s variable names are already defined in file id %d",
              label, exoid);
      ex_err("ex_put_all_var_param_ext",errmsg,exerrval);

    } else {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to define %s variable names in file id %d",
              label, exoid);
      ex_err("ex_put_all_var_param_ext",errmsg,exerrval);
    }
  }
  return status;
}

int *get_status_array(int exoid, int var_count, const char *VARIABLE, const char *label)
{
  char errmsg[MAX_ERR_LENGTH];
  int varid;
  int status;
  int *stat_vals = NULL;
  
  if (!(stat_vals = malloc(var_count*sizeof(int)))) {
    exerrval = EX_MEMFAIL;
    sprintf(errmsg,
            "Error: failed to allocate memory for %s status array for file id %d",
            label, exoid);
    ex_err("ex_put_all_var_param_ext",errmsg,exerrval);
    return (NULL);
  }

  /* get variable id of status array */
  if ((nc_inq_varid (exoid, VARIABLE, &varid)) == NC_NOERR) {
    /* if status array exists (V 2.01+), use it, otherwise assume
       object exists to be backward compatible */
     
    if ((status = nc_get_var_int(exoid, varid, stat_vals)) != NC_NOERR) {
      exerrval = status;
      stat_vals = safe_free(stat_vals);
      sprintf(errmsg,
              "Error: failed to get %s status array from file id %d",
              label, exoid);
      ex_err("ex_put_all_var_param_ext",errmsg,exerrval);
      return (NULL);
    }
  } else {
    /* status array doesn't exist (V2.00), dummy one up for later checking */
    int i;
    for(i=0; i<var_count; i++)
      stat_vals[i] = 1;
  }
 return stat_vals;
}

void *safe_free(void *array)
{
  if (array != 0) free(array);
  return 0;
}

int put_truth_table(int exoid, int varid, int *table, const char *label)
{
  int  iresult = 0;
  char errmsg[MAX_ERR_LENGTH];
  
  iresult = nc_put_var_int(exoid, varid, table);
    
  if (iresult != NC_NOERR) {
    exerrval = iresult;
    sprintf(errmsg,
            "Error: failed to store %s variable truth table in file id %d",
            label, exoid);
    ex_err("ex_put_all_var_param_ext",errmsg,exerrval);
   }
  return iresult;
}

int define_truth_table(ex_entity_type obj_type, int exoid, int num_ent, int num_var,
                       int *var_tab, int *status_tab, int *ids, const char *label)
{
  char errmsg[MAX_ERR_LENGTH];
  int k = 0;
  int i, j;
  int time_dim;
  int dims[2];
  int varid;
  int status;

  nc_inq_dimid(exoid, DIM_TIME, &time_dim);

  if (var_tab == NULL) {
    exerrval = EX_NULLENTITY;
    sprintf(errmsg,
            "Error: %s variable truth table is NULL in file id %d", label, exoid);
    ex_err("ex_put_all_var_param_ext",errmsg, exerrval);
    return -1;
  }
  
  for (i=0; i<num_ent; i++) {
    for (j=1; j<=num_var; j++) {
      
      /* check if variables are to be put out for this block */
      if (var_tab[k] != 0) {
        if (status_tab[i] != 0) {/* only define variable if active */
          dims[0] = time_dim;
                
          /* Determine number of entities in entity */
          /* Need way to make this more generic... */
          status = nc_inq_dimid(exoid, ex_dim_num_entries_in_object( obj_type, i+1 ), &dims[1]);
          if (status != NC_NOERR) {
            exerrval = status;
            sprintf(errmsg,
                    "Error: failed to locate number of entities in %s %d in file id %d",
                    label, ids[i], exoid);
            ex_err("ex_put_all_var_param_ext",errmsg,exerrval);
            return status;
          }
          
          /* define netCDF variable to store variable values;
           * the j index cycles from 1 through the number of variables so 
           * that the index of the EXODUS II variable (which is part of 
           * the name of the netCDF variable) will begin at 1 instead of 0
           */
          status = nc_def_var(exoid, ex_name_var_of_object( obj_type, j, i+1 ),
            nc_flt_code(exoid), 2, dims, &varid);
          if (status != NC_NOERR) {
            if (status != NC_ENAMEINUSE) {
              exerrval = status;
              sprintf(errmsg,
                      "Error: failed to define %s variable for %s %d in file id %d",
                      label, label, ids[i], exoid);
              ex_err("ex_put_all_var_param_ext",errmsg,exerrval);
              return status;
            }
          }
        }
      }  /* if */
      k++; /* increment truth table pointer */
    }  /* for j */
  }  /* for i */
  return NC_NOERR;
}
