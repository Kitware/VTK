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
* expvtt - ex_put_var_tab
*
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       char*   type                    'e', 'm', 's' element, nodeset, sideset
*       int     num_blk            number of blocks
*       int     num_var            number of variables
*       int*    var_tab            variable truth table array
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
 * variables can also be created in ex_put_*_var), this call will save
 * tremendous time because all of the variables are defined at once
 * while the file is in define mode, rather than going in and out of
 * define mode (causing the entire file to be copied over and over)
 * which is what occurs when the variables are defined in ex_put_*_var
 */

int ex_put_var_tab (int  exoid,
        const char *var_type,
        int  num_blk,
        int  num_var,
        int *var_tab)
{
  int numelblkdim, numelvardim, timedim, dims[2], varid, iresult;
  int obj_type;
  char *sta_type, *tab_type;
  long num_entity = -1;
  long num_var_db = -1;
  long start[2], count[2]; 
  nclong *stat_vals, *lptr;
  int i, j, k, id, *ids;
  char errmsg[MAX_ERR_LENGTH];
  const char* routine = "ex_get_var_tab";
  
  /*
   * The ent_type and the var_name are used to build the netcdf
   * variables name.  Normally this is done via a macro defined in
   * exodusII_int.h
   */
  const char* ent_type = NULL;
  const char* var_name = NULL;
  const char* ent_size = NULL;
  exerrval = 0; /* clear error code */
   
  if (*var_type == 'e' || *var_type == 'E') {
    numelblkdim = ex_get_dimension(exoid, DIM_NUM_EL_BLK,   "element blocks",
           &num_entity, routine);
    numelvardim = ex_get_dimension(exoid, DIM_NUM_ELE_VAR,  "element variables",
           &num_var_db, routine);
    varid = ncvarid (exoid, VAR_ELEM_TAB);
    var_name = "vals_elem_var";
    ent_type = "eb";
    ent_size = "num_el_in_blk";
    obj_type = EX_ELEM_BLOCK;
    sta_type = VAR_STAT_EL_BLK;
    tab_type = VAR_ELEM_TAB;
  }
  else if (*var_type == 'm' || *var_type == 'M') {
    numelblkdim = ex_get_dimension(exoid, DIM_NUM_NS,       "nodesets",
           &num_entity, routine);
    numelvardim = ex_get_dimension(exoid, DIM_NUM_NSET_VAR, "nodeset variables",
           &num_var_db, routine);
    varid = ncvarid (exoid, VAR_NSET_TAB);
    var_name = "vals_nset_var";
    ent_type = "ns";
    ent_size = "num_nod_ns";
    obj_type = EX_NODE_SET;
    sta_type = VAR_NS_STAT;
    tab_type = VAR_NSET_TAB;
  }
  else if (*var_type == 's' || *var_type == 'S') {
    numelblkdim = ex_get_dimension(exoid, DIM_NUM_SS,       "sidesets",
           &num_entity, routine);
    numelvardim = ex_get_dimension(exoid, DIM_NUM_SSET_VAR, "sideset variables",
           &num_var_db, routine);
    varid = ncvarid (exoid, VAR_SSET_TAB);
    var_name = "vals_sset_var";
    ent_type = "ss";
    ent_size = "num_side_ss";
    obj_type = EX_SIDE_SET;
    sta_type = VAR_SS_STAT;
    tab_type = VAR_SSET_TAB;
  }
  else {       /* invalid variable type */
    exerrval = EX_BADPARAM;
    sprintf(errmsg,
      "Error: Invalid variable type %c specified in file id %d",
      *var_type, exoid);
    ex_err("ex_get_varid",errmsg,exerrval);
    return (EX_WARN);
  }
   
  if (num_entity == -1 || num_var_db == -1)
    return (EX_FATAL);

  if (num_entity != num_blk) {
    exerrval = EX_FATAL;
    sprintf(errmsg,
      "Error: # of blocks doesn't match those defined in file id %d", exoid);
    ex_err("ex_get_var_tab",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (num_var_db != num_var) {
    exerrval = EX_FATAL;
    sprintf(errmsg,
      "Error: # of variables doesn't match those defined in file id %d", exoid);
    ex_err("ex_get_var_tab",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* get block IDs */
  if (!(ids = malloc(num_blk*sizeof(int)))) {
    exerrval = EX_MEMFAIL;
    sprintf(errmsg,
      "Error: failed to allocate memory for id array for file id %d",
      exoid);
    ex_err("ex_put_var_tab",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* Get status array for later use */
  if (!(stat_vals = malloc(num_blk*sizeof(nclong)))) {
    exerrval = EX_MEMFAIL;
    free(ids);
    sprintf(errmsg,
      "Error: failed to allocate memory for status array for file id %d",
            exoid);
    ex_err("ex_put_var_tab",errmsg,exerrval);
    return (EX_FATAL);
  }

  ex_get_ids (exoid, obj_type, ids);
  varid = ncvarid (exoid, sta_type);

  /* get variable id of status array */
  if (varid != -1) {
    /* if status array exists (V 2.01+), use it, otherwise assume
       object exists to be backward compatible */

    start[0] = 0;
    start[1] = 0;
    count[0] = num_blk;
    count[1] = 0;

    if (ncvarget (exoid, varid, start, count, (void *)stat_vals) == -1) {
      exerrval = ncerr;
      free(stat_vals);
      sprintf(errmsg,
        "Error: failed to get status array from file id %d",
              exoid);
      ex_err("put_var_tab",errmsg,exerrval);
      return (EX_FATAL);
    }
  } else {
    /* status array doesn't exist (V2.00), dummy one up for later checking */
    for(i=0;i<num_blk;i++)
      stat_vals[i] = 1;
  }

  /* put netcdf file into define mode  */
  if (ncredef (exoid) == -1) {
    free(stat_vals);
    free (ids);
    exerrval = ncerr;
    sprintf(errmsg,
            "Error: failed to put file id %d into define mode",
      exoid);
    ex_err("ex_put_var_tab",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* inquire previously defined dimensions */
  if ((timedim = ncdimid (exoid, DIM_TIME)) == -1) {
    exerrval = ncerr;
    free(stat_vals);
    free (ids);
    sprintf(errmsg,
      "Error: failed to locate time variable in file id %d",
      exoid);
    ex_err("ex_put_var_tab",errmsg,exerrval);
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
    if ((dims[1] = ncdimid (exoid, ex_catstr(ent_size, (i+1)))) == -1) {
      exerrval = ncerr;
      id=ids[i];
      free(stat_vals);
      free (ids);
      sprintf(errmsg,
        "Error: failed to locate number of entities in block %d in file id %d",
        id,exoid);
      ex_err("ex_put_var_tab",errmsg,exerrval);
      goto error_ret;          /* exit define mode and return */
      }


    /* define netCDF variable to store variable values; the j
     * index cycles from 1 through the number of variables so
     * that the index of the EXODUS II variable (which is part
     * of the name of the netCDF variable) will begin at 1
     * instead of 0
     */

    if ((varid = ncvardef (exoid, ex_catstr2(var_name, j, ent_type, i+1),
         nc_flt_code(exoid), 2, dims)) == -1) {
      if (ncerr != NC_ENAMEINUSE) {
        exerrval = ncerr;
        id=ids[i];
        free(stat_vals);
        free (ids);
        sprintf(errmsg,
          "Error: failed to define variable for block %d in file id %d",
          id,exoid);
        ex_err("ex_put_var_tab",errmsg,exerrval);
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
  varid = ncvardef (exoid, tab_type, NC_LONG, 2, dims);
  
  if (varid == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: failed to define variable truth table in file id %d",
      exoid);
    ex_err("ex_put_var_tab",errmsg,exerrval);
    goto error_ret;          /* exit define mode and return */
  }

  /* leave define mode  */
  if (ncendef (exoid) == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: failed to complete definitions in file id %d",
      exoid);
    ex_err("ex_put_var_tab",errmsg,exerrval);
    return (EX_FATAL);
  }


  /* write out the element variable truth table */

  /* this contortion is necessary because netCDF is expecting nclongs;
     fortunately it's necessary only when ints and nclongs aren't the
     same size */

  start[0] = 0;
  start[1] = 0;

  count[0] = num_blk;
  count[1] = num_var;

  if (sizeof(int) == sizeof(nclong)) {
    iresult = ncvarput (exoid, varid, start, count, var_tab);
  } else {
    lptr = itol (var_tab, (int)(num_blk*num_var));
    iresult = ncvarput (exoid, varid, start, count, lptr);
    free(lptr);
  }

  if (iresult == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: failed to store variable truth table in file id %d",
      exoid);
    ex_err("ex_put_var_tab",errmsg,exerrval);
    return (EX_FATAL);
  }


  return (EX_NOERR);

  /* Fatal error: exit definition mode and return */
 error_ret:
  if (ncendef (exoid) == -1)     /* exit define mode */
    {
      sprintf(errmsg,
        "Error: failed to complete definition for file id %d",
        exoid);
      ex_err("ex_put_var_tab",errmsg,exerrval);
    }
  return (EX_FATAL);
}
