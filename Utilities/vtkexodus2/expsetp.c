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
* expsetp - ex_put_set_param
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     set_type                the type of set
*       int     set_id                  set id
*       int     num_entries_in_set       number of entries in the set
*       int     num_dist_fact_in_set    number of distribution factors in the
*                                       set
*
* exit conditions - 
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * writes the set id and the number of entries which describe a single set
 * \param  exoid                   exodus file id
 * \param  set_type                the type of set
 * \param  set_id                  set id
 * \param  num_entries_in_set      number of entries in the set
 * \param  num_dist_fact_in_set    number of distribution factors in the set
 */

int ex_put_set_param (int exoid,
                      ex_entity_type set_type,
                      int set_id,
                      int num_entries_in_set,
                      int num_dist_fact_in_set)
{
  int status;
  size_t temp;
  int dimid, varid, set_id_ndx, dims[1]; 
  size_t start[1]; 
  int num_sets;
  int ldum;
  int cur_num_sets, set_stat;
  char errmsg[MAX_ERR_LENGTH];
  char* dimptr;
  char* idsptr;
  char* statptr;
  char* numentryptr;
  char* numdfptr;
  char* factptr;
  char* entryptr;
  char* extraptr;

  exerrval = 0; /* clear error code */

  /* setup pointers based on set_type 
     NOTE: there is another block that sets more stuff later ... */
  if (set_type == EX_NODE_SET) {
    dimptr = DIM_NUM_NS;
    idsptr = VAR_NS_IDS;
    statptr = VAR_NS_STAT;
  }
  else if (set_type == EX_EDGE_SET) {
    dimptr = DIM_NUM_ES;
    idsptr = VAR_ES_IDS;
    statptr = VAR_ES_STAT;
  }
  else if (set_type == EX_FACE_SET) {
    dimptr = DIM_NUM_FS;
    idsptr = VAR_FS_IDS;
    statptr = VAR_FS_STAT;
  }
  else if (set_type == EX_SIDE_SET) {
    dimptr = DIM_NUM_SS;
    idsptr = VAR_SS_IDS;
    statptr = VAR_SS_STAT;
  }
  else if (set_type == EX_ELEM_SET) {
    dimptr = DIM_NUM_ELS;
    idsptr = VAR_ELS_IDS;
    statptr = VAR_ELS_STAT;
  }
  else {
    exerrval = EX_FATAL;
    sprintf(errmsg,
            "Error: invalid set type (%d)", set_type);
    ex_err("ex_put_set_param",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* first check if any of that set type is specified */

  if ((status = nc_inq_dimid(exoid, dimptr, &dimid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: no %ss specified in file id %d", ex_name_of_object(set_type),
            exoid);
    ex_err("ex_put_set_param",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* Check for duplicate set id entry */
  ex_id_lkup(exoid, set_type, set_id);
  if (exerrval != EX_LOOKUPFAIL) {  /* found the side set id */
    sprintf(errmsg,
            "Error: %s %d already defined in file id %d", ex_name_of_object(set_type),
            set_id,exoid);
    ex_err("ex_put_set_param",errmsg,exerrval);
    return(EX_FATAL);
  }

  /* Get number of sets specified for this file */
  if ((status = nc_inq_dimlen(exoid,dimid,&temp)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to get number of %ss in file id %d",
            ex_name_of_object(set_type), exoid);
    ex_err("ex_put_set_param",errmsg,exerrval);
    return (EX_FATAL);
  }
  num_sets = (int)temp;


  /* Keep track of the total number of sets defined using a counter stored
     in a linked list keyed by exoid.
     NOTE: ex_get_file_item finds the maximum number of sets defined
     for a specific file and returns that value.
  */
  cur_num_sets=ex_get_file_item(exoid, ex_get_counter_list(set_type));
  if (cur_num_sets >= num_sets) {
    exerrval = EX_FATAL;
    sprintf(errmsg,
            "Error: exceeded number of %ss (%d) defined in file id %d",
            ex_name_of_object(set_type), num_sets,exoid);
    ex_err("ex_put_set_param",errmsg,exerrval);
    return (EX_FATAL);
  }

  /*   NOTE: ex_inc_file_item finds the current number of sets defined
       for a specific file and returns that value incremented. */

  cur_num_sets=ex_inc_file_item(exoid, ex_get_counter_list(set_type));
  set_id_ndx = cur_num_sets + 1;

  /* setup more pointers based on set_type */
  if (set_type == EX_NODE_SET) {
    numentryptr = DIM_NUM_NOD_NS(set_id_ndx);
    entryptr = VAR_NODE_NS(set_id_ndx);
    extraptr = NULL;
    /* note we are using DIM_NUM_NODE_NS instead of DIM_NUM_DF_NS */
    numdfptr = DIM_NUM_NOD_NS(set_id_ndx);
    factptr = VAR_FACT_NS(set_id_ndx);
  }
  else if (set_type == EX_EDGE_SET) {
    numentryptr = DIM_NUM_EDGE_ES(set_id_ndx);
    entryptr = VAR_EDGE_ES(set_id_ndx);
    extraptr = VAR_ORNT_ES(set_id_ndx);
    numdfptr = DIM_NUM_DF_ES(set_id_ndx);
    factptr = VAR_FACT_ES(set_id_ndx);
  }
  else if (set_type == EX_FACE_SET) {
    numentryptr = DIM_NUM_FACE_FS(set_id_ndx);
    entryptr = VAR_FACE_FS(set_id_ndx);
    extraptr = VAR_ORNT_FS(set_id_ndx);
    numdfptr = DIM_NUM_DF_FS(set_id_ndx);
    factptr = VAR_FACT_FS(set_id_ndx);
  }
  else if (set_type == EX_SIDE_SET) {
    numentryptr = DIM_NUM_SIDE_SS(set_id_ndx);
    entryptr = VAR_ELEM_SS(set_id_ndx);
    extraptr = VAR_SIDE_SS(set_id_ndx);
    numdfptr = DIM_NUM_DF_SS(set_id_ndx);
    factptr = VAR_FACT_SS(set_id_ndx);
  }
  if (set_type == EX_ELEM_SET) {
    numentryptr = DIM_NUM_ELE_ELS(set_id_ndx);
    entryptr = VAR_ELEM_ELS(set_id_ndx);
    extraptr = NULL;
    numdfptr = DIM_NUM_DF_ELS(set_id_ndx);
    factptr = VAR_FACT_ELS(set_id_ndx);
  }

  /* write out information to previously defined variable */

  /* first: get id of set id variable */
  if ((status = nc_inq_varid(exoid, idsptr, &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to locate %s %d in file id %d", ex_name_of_object(set_type),
            set_id, exoid);
    ex_err("ex_put_set_param",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* write out set id */
  start[0] = cur_num_sets;

  ldum = (int)set_id;
  if ((status = nc_put_var1_int(exoid, varid, start, &ldum)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to store %s id %d in file id %d", ex_name_of_object(set_type),
            set_id, exoid);
    ex_err("ex_put_set_param",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (num_entries_in_set == 0) /* Is this a NULL  set? */
    set_stat = 0; /* change set status to NULL */
  else
    set_stat = 1; /* change set status to TRUE */

  if ((status = nc_inq_varid(exoid, statptr, &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to locate %s status in file id %d", ex_name_of_object(set_type),
            exoid);
    ex_err("ex_put_set_param",errmsg,exerrval);
    return (EX_FATAL);
  }

  ldum = (int)set_stat;
  if ((status = nc_put_var1_int(exoid, varid, start, &ldum)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to store %s %d status to file id %d", ex_name_of_object(set_type),
            set_id, exoid);
    ex_err("ex_put_set_param",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (num_entries_in_set == 0) {/* Is this a NULL set? */
    return(EX_NOERR);
  }

  /* put netcdf file into define mode  */
  if ((status = nc_redef (exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to put file id %d into define mode",
            exoid);
    ex_err("ex_put_set_param",errmsg,exerrval);
    return (EX_FATAL);
  }


  /* define dimensions and variables */
  if ((status = nc_def_dim(exoid, numentryptr,
                           num_entries_in_set, &dimid)) != NC_NOERR) {
    exerrval = status;
    if (status == NC_ENAMEINUSE)
      {
        sprintf(errmsg,
                "Error: %s %d size already defined in file id %d",
                ex_name_of_object(set_type), set_id,exoid);
        ex_err("ex_put_set_param",errmsg,exerrval);
      }
    else {
      sprintf(errmsg,
              "Error: failed to define number of entries in %s %d in file id %d",
              ex_name_of_object(set_type), set_id,exoid);
      ex_err("ex_put_set_param",errmsg,exerrval);
    }
    goto error_ret;
  }

  /* create variable array in which to store the entry lists */

  dims[0] = dimid;
  if ((status = nc_def_var(exoid, entryptr, NC_INT, 1, dims, &varid)) != NC_NOERR) {
    exerrval = status;
    if (status == NC_ENAMEINUSE) {
      sprintf(errmsg,
              "Error: entry list already exists for %s %d in file id %d",
              ex_name_of_object(set_type), set_id,exoid);
      ex_err("ex_put_set_param",errmsg,exerrval);
    } else {
      sprintf(errmsg,
              "Error: failed to create entry list for %s %d in file id %d",
              ex_name_of_object(set_type), set_id,exoid);
      ex_err("ex_put_set_param",errmsg,exerrval);
    }
    goto error_ret;            /* exit define mode and return */
  }

  if (extraptr) {
    if ((status = nc_def_var(exoid, extraptr, NC_INT, 1, dims, &varid)) != NC_NOERR) {
      exerrval = status;
      if (status == NC_ENAMEINUSE) {
        sprintf(errmsg,
                "Error: extra list already exists for %s %d in file id %d",
                ex_name_of_object(set_type), set_id, exoid);
        ex_err("ex_put_set_param",errmsg,exerrval);
      } else {
        sprintf(errmsg,
                "Error: failed to create extra list for %s %d in file id %d",
                ex_name_of_object(set_type), set_id,exoid);
        ex_err("ex_put_set_param",errmsg,exerrval);
      }
      goto error_ret;         /* exit define mode and return */
           
    }
  }

  /* Create distribution factors variable if required */

  if (num_dist_fact_in_set > 0) {

    if (set_type == EX_NODE_SET) {
      /* but num_dist_fact_in_set must equal number of nodes */
      if (num_dist_fact_in_set != num_entries_in_set) {
        exerrval = EX_FATAL;
        sprintf(errmsg,
                "Error: # dist fact (%d) not equal to # nodes (%d) in node  set %d file id %d",
                num_dist_fact_in_set, num_entries_in_set, set_id, exoid);
        ex_err("ex_put_set_param",errmsg,exerrval);
        goto error_ret;    /* exit define mode and return */
      }

      /* resuse dimid from entry lists */

    } else {
      if ((status = nc_def_dim(exoid, numdfptr, 
                               num_dist_fact_in_set, &dimid)) != NC_NOERR) {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to define number of dist factors in %s %d in file id %d",
                ex_name_of_object(set_type), set_id,exoid);
        ex_err("ex_put_set_param",errmsg,exerrval);
        goto error_ret;          /* exit define mode and return */
      }
    }

    /* create variable array in which to store the set distribution factors
     */
    dims[0] = dimid;
    if ((status = nc_def_var(exoid, factptr, nc_flt_code(exoid), 1, dims, &varid)) != NC_NOERR) {
      exerrval = status;
      if (status == NC_ENAMEINUSE) {
        sprintf(errmsg,
                "Error: dist factors list already exists for %s %d in file id %d",
                ex_name_of_object(set_type), set_id,exoid);
        ex_err("ex_put_set_param",errmsg,exerrval);
      } else {
        sprintf(errmsg,
                "Error: failed to create dist factors list for %s %d in file id %d",
                ex_name_of_object(set_type), set_id,exoid);
        ex_err("ex_put_set_param",errmsg,exerrval);
      }
      goto error_ret;            /* exit define mode and return */
    }
  }

  /* leave define mode  */
  if ((status = nc_enddef (exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to complete definition in file id %d", exoid);
    ex_err("ex_put_set_param",errmsg,exerrval);
    return (EX_FATAL);
  }

  return (EX_NOERR);

  /* Fatal error: exit definition mode and return */
 error_ret:
  if (nc_enddef (exoid) != NC_NOERR) {    /* exit define mode */
    sprintf(errmsg,
            "Error: failed to complete definition for file id %d",
            exoid);
    ex_err("ex_put_set_param",errmsg,exerrval);
  }
  return (EX_FATAL);
}
