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
* expvtt - ex_put_truth_table
*
* entry conditions - 
*   input parameters:
*       int     exoid              exodus file id
*       int     obj_type           object type
*       int     num_blk            number of blocks
*       int     num_var            number of variables
*       int*    variable_table            variable truth table array
*
* exit conditions - 
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * writes the EXODUS II variable truth table to the database; also,
 * creates netCDF variables in which to store EXODUS II variable
 * values; although this table isn't required (because the netCDF
 * variables can also be created in ex_put_var), this call will save
 * tremendous time because all of the variables are defined at once
 * while the file is in define mode, rather than going in and out of
 * define mode (causing the entire file to be copied over and over)
 * which is what occurs when the variables are defined in ex_put_var
 * \param       exoid              exodus file id
 * \param       obj_type           object type
 * \param       num_blk            number of blocks
 * \param       num_var            number of variables
 * \param      *var_tab            variable truth table array
 */

int ex_put_truth_table (int  exoid,
			ex_entity_type obj_type,
			int  num_blk,
			int  num_var,
			int *var_tab)
{
  int numelblkdim, numelvardim, timedim, dims[2], varid;
  char *sta_type, *tab_type;
  size_t num_entity = 0;
  size_t num_var_db = 0;
  int *stat_vals;
  int i, j, k, id, *ids;
  int status;
  char errmsg[MAX_ERR_LENGTH];
  const char* routine = "ex_put_truth_table";
  
  /*
   * The ent_type and the var_name are used to build the netcdf
   * variables name.  Normally this is done via a macro defined in
   * exodusII_int.h
   */
  const char* ent_type = NULL;
  const char* var_name = NULL;
  const char* ent_size = NULL;
  exerrval = 0; /* clear error code */
   
  ex_get_dimension(exoid, ex_dim_num_objects(obj_type),
		   ex_name_of_object(obj_type), &num_entity, &numelblkdim, routine);

  if (obj_type == EX_ELEM_BLOCK) {
    ex_get_dimension(exoid, DIM_NUM_ELE_VAR,  "element variables",
		     &num_var_db, &numelvardim, routine);
    status = nc_inq_varid (exoid, VAR_ELEM_TAB, &varid);
    var_name = "vals_elem_var";
    ent_type = "eb";
    ent_size = "num_el_in_blk";
    sta_type = VAR_STAT_EL_BLK;
    tab_type = VAR_ELEM_TAB;
  }
  else if (obj_type == EX_EDGE_BLOCK) {
    ex_get_dimension(exoid, DIM_NUM_EDG_VAR, "edge block variables",
		     &num_var_db, &numelvardim, routine);
    status = nc_inq_varid (exoid, VAR_EBLK_TAB, &varid);
    var_name = "vals_edge_var";
    ent_type = "eb";
    ent_size = "num_ed_in_blk";
    sta_type = VAR_STAT_ED_BLK;
    tab_type = VAR_EBLK_TAB;
  }
  else if (obj_type  == EX_FACE_BLOCK) {
    ex_get_dimension(exoid, DIM_NUM_FAC_VAR, "face block variables",
		     &num_var_db, &numelvardim, routine);
    status = nc_inq_varid (exoid, VAR_FBLK_TAB, &varid);
    var_name = "vals_face_var";
    ent_type = "fb";
    ent_size = "num_fa_in_blk";
    sta_type = VAR_STAT_FA_BLK;
    tab_type = VAR_FBLK_TAB;
  }
  else if (obj_type == EX_SIDE_SET) {
    ex_get_dimension(exoid, DIM_NUM_SSET_VAR, "sideset variables",
		     &num_var_db, &numelvardim, routine);
    status = nc_inq_varid (exoid, VAR_SSET_TAB, &varid);
    var_name = "vals_sset_var";
    ent_type = "ss";
    ent_size = "num_side_ss";
    sta_type = VAR_SS_STAT;
    tab_type = VAR_SSET_TAB;
  }
  else if (obj_type == EX_NODE_SET) {
    ex_get_dimension(exoid, DIM_NUM_NSET_VAR, "nodeset variables",
		     &num_var_db, &numelvardim, routine);
    status = nc_inq_varid (exoid, VAR_NSET_TAB, &varid);
    var_name = "vals_nset_var";
    ent_type = "ns";
    ent_size = "num_nod_ns";
    sta_type = VAR_NS_STAT;
    tab_type = VAR_NSET_TAB;
  }
  else if (obj_type == EX_EDGE_SET) {
    ex_get_dimension(exoid, DIM_NUM_ESET_VAR, "edge set variables",
		     &num_var_db, &numelvardim, routine);
    status = nc_inq_varid (exoid, VAR_ESET_TAB, &varid);
    var_name = "vals_eset_var";
    ent_type = "es";
    ent_size = "num_edge_es";
    sta_type = VAR_ES_STAT;
    tab_type = VAR_ESET_TAB;
  }
  else if (obj_type == EX_FACE_SET) {
    ex_get_dimension(exoid, DIM_NUM_FSET_VAR, "face set variables",
		     &num_var_db, &numelvardim, routine);
    status = nc_inq_varid (exoid, VAR_FSET_TAB, &varid);
    var_name = "vals_fset_var";
    ent_type = "fs";
    ent_size = "num_face_fs";
    sta_type = VAR_FS_STAT;
    tab_type = VAR_FSET_TAB;
  }
  else if (obj_type == EX_ELEM_SET) {
    ex_get_dimension(exoid, DIM_NUM_ELSET_VAR, "element set variables",
		     &num_var_db, &numelvardim, routine);
    status = nc_inq_varid (exoid, VAR_ELSET_TAB, &varid);
    var_name = "vals_elset_var";
    ent_type = "es";
    ent_size = "num_ele_els";
    sta_type = VAR_ELS_STAT;
    tab_type = VAR_ELSET_TAB;
  }

  else {       /* invalid variable type */
    exerrval = EX_BADPARAM;
    sprintf(errmsg,
	    "Error: Invalid variable type %d specified in file id %d",
	    obj_type, exoid);
    ex_err("ex_get_varid",errmsg,exerrval);
    return (EX_WARN);
  }
   
  if (num_entity == (size_t)-1 || num_var_db == (size_t)-1)
    return (EX_FATAL);

  if (num_entity != (size_t)num_blk) {
    exerrval = EX_FATAL;
    sprintf(errmsg,
	    "Error: # of %s doesn't match those defined in file id %d",
	    ex_name_of_object(obj_type), exoid);
    ex_err("ex_get_var_tab",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (num_var_db != (size_t)num_var) {
    exerrval = EX_FATAL;
    sprintf(errmsg,
	    "Error: # of %s variables doesn't match those defined in file id %d",
	    ex_name_of_object(obj_type), exoid);
    ex_err("ex_get_var_tab",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* get block IDs */
  if (!(ids = malloc(num_blk*sizeof(int)))) {
    exerrval = EX_MEMFAIL;
    sprintf(errmsg,
	    "Error: failed to allocate memory for %s id array for file id %d",
	    ex_name_of_object(obj_type), exoid);
    ex_err(routine,errmsg,exerrval);
    return (EX_FATAL);
  }

  /* Get status array for later use */
  if (!(stat_vals = malloc(num_blk*sizeof(int)))) {
    exerrval = EX_MEMFAIL;
    free(ids);
    sprintf(errmsg,
	    "Error: failed to allocate memory for %s status array for file id %d",
            ex_name_of_object(obj_type), exoid);
    ex_err(routine,errmsg,exerrval);
    return (EX_FATAL);
  }

  ex_get_ids (exoid, obj_type, ids);
  status = nc_inq_varid (exoid, sta_type, &varid);

  /* get variable id of status array */
  if (status == NC_NOERR) {
    /* if status array exists (V 2.01+), use it, otherwise assume
       object exists to be backward compatible */

    if ((status = nc_get_var_int (exoid, varid, stat_vals)) != NC_NOERR) {
      exerrval = status;
      free(stat_vals);
      sprintf(errmsg,
	      "Error: failed to get %s status array from file id %d",
              ex_name_of_object(obj_type), exoid);
      ex_err("put_var_tab",errmsg,exerrval);
      return (EX_FATAL);
    }
  } else {
    /* status array doesn't exist (V2.00), dummy one up for later checking */
    for(i=0;i<num_blk;i++)
      stat_vals[i] = 1;
  }

  /* put netcdf file into define mode  */
  if ((status = nc_redef (exoid)) != NC_NOERR) {
    free(stat_vals);
    free (ids);
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to put file id %d into define mode",
	    exoid);
    ex_err(routine,errmsg,exerrval);
    return (EX_FATAL);
  }

  /* inquire previously defined dimensions */
  if ((status = nc_inq_dimid (exoid, DIM_TIME, &timedim)) != NC_NOERR) {
    exerrval = status;
    free(stat_vals);
    free (ids);
    sprintf(errmsg,
	    "Error: failed to locate time variable in file id %d",
	    exoid);
    ex_err(routine,errmsg,exerrval);
    goto error_ret;          /* exit define mode and return */
  }
  
  /* define netCDF variables in which to store EXODUS II element
   * variable values
   */

  k = 0;
  for (i=0; i<num_blk; i++) {
    for (j=1; j<=num_var; j++) {

      /* check if variables are to be put out for this entity */
      if (var_tab[k] != 0) {
	if (stat_vals[i] != 0) {/* check for NULL entity */
	  /* NOTE: This code used to zero out the var_tab entry
	     if the stat_vals[i] value was zero. However, in some
	     cases it is good to know that a variable was assigned to
	     an entity even if that entity is empty. The code was
	     changed to not modify the truth table.
	  */
	  dims[0] = timedim;
	  
	  /* Determine number of entities in block */
	  if ((status = nc_inq_dimid(exoid, ex_catstr(ent_size, (i+1)), &dims[1])) != NC_NOERR) {
	    exerrval = status;
	    id=ids[i];
	    free(stat_vals);
	    free (ids);
	    sprintf(errmsg,
		    "Error: failed to locate number of entities in %s %d in file id %d",
		    ex_name_of_object(obj_type), id,exoid);
	    ex_err(routine,errmsg,exerrval);
	    goto error_ret;          /* exit define mode and return */
	    }


	  /* define netCDF variable to store variable values; the j
	   * index cycles from 1 through the number of variables so
	   * that the index of the EXODUS II variable (which is part
	   * of the name of the netCDF variable) will begin at 1
	   * instead of 0
	   */

	  if ((status = nc_def_var(exoid, ex_catstr2(var_name, j, ent_type, i+1),
				  nc_flt_code(exoid), 2, dims, &varid)) != NC_NOERR) {
	    if (status != NC_ENAMEINUSE) {
	      exerrval = status;
	      id=ids[i];
	      free(stat_vals);
	      free (ids);
	      sprintf(errmsg,
		      "Error: failed to define variable for %s %d in file id %d",
		      ex_name_of_object(obj_type), id,exoid);
	      ex_err(routine,errmsg,exerrval);
	      goto error_ret;  /* exit define mode and return */
	    }
	  }
	}
      }  /* if */
      k++; /* increment element truth table pointer */
    }  /* for j */
  }  /* for i */

  free (stat_vals);
  free (ids);

  /* create a variable array in which to store the truth table
   */

  dims[0] = numelblkdim;
  dims[1] = numelvardim;
  status = nc_def_var (exoid, tab_type, NC_INT, 2, dims, &varid);
  
  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to define %s variable truth table in file id %d",
	    ex_name_of_object(obj_type), exoid);
    ex_err(routine,errmsg,exerrval);
    goto error_ret;          /* exit define mode and return */
  }

  /* leave define mode  */
  if ((status = nc_enddef (exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to complete definitions in file id %d",
	    exoid);
    ex_err(routine,errmsg,exerrval);
    return (EX_FATAL);
  }

  /* write out the element variable truth table */
  status = nc_put_var_int(exoid, varid, var_tab);

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to store variable truth table in file id %d",
	    exoid);
    ex_err(routine,errmsg,exerrval);
    return (EX_FATAL);
  }


  return (EX_NOERR);

  /* Fatal error: exit definition mode and return */
 error_ret:
  if (nc_enddef (exoid) != NC_NOERR)     /* exit define mode */
    {
      sprintf(errmsg,
	      "Error: failed to complete definition for file id %d",
	      exoid);
      ex_err(routine,errmsg,exerrval);
    }
  return (EX_FATAL);
}
