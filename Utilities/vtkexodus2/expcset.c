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
* expcss - ex_put_concat_sets
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     set_type                type of set
*       struct ex_set_specs* set_specs  set specs structure
*
* exit conditions - 
*
* revision history - 
*
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * writes the set ID's, set entry count array, set entry pointers array,
 * set entry list, set extra list, and distribution factors list for 
 * all the sets of the specified type.
 * \param  exoid      exodus file id
 * \param  set_type   type of set
 * \param  set_specs  set specs structure
 */

int ex_put_concat_sets (int   exoid,
      ex_entity_type set_type,
      const struct ex_set_specs* set_specs)
{
  int status;
  int temp;
  const int  *set_ids = set_specs->sets_ids;
  const int  *num_entries_per_set = set_specs->num_entries_per_set;
  const int  *num_dist_per_set = set_specs->num_dist_per_set;
  const int  *sets_entry_index = set_specs->sets_entry_index;
  const int  *sets_dist_index = set_specs->sets_dist_index;
  const int  *sets_entry_list = set_specs->sets_entry_list;
  const int  *sets_extra_list = set_specs->sets_extra_list;
  const void *sets_dist_fact = set_specs->sets_dist_fact;
  char *cdum = NULL;
  int i, num_sets, cur_num_sets, dimid, varid, set_id_ndx, dims[1];
  int  *set_stat = NULL;
  float fdum;
  const float *flt_dist_fact = NULL;
  const double *dbl_dist_fact = NULL;
  char errmsg[MAX_ERR_LENGTH];
  char* idsptr = NULL;
  char* statptr = NULL;
  char* numdfptr = NULL;
  char* factptr = NULL;
  char* elemptr = NULL;
  char* extraptr = NULL;
  ex_inquiry ex_inq_val;
  const int *extra_list = NULL;   

  exerrval = 0; /* clear error code */

  cdum = 0; /* initialize even though it is not used */

  /* setup pointers based on set_type 
     NOTE: there is another block that sets more stuff later ... */

  if (set_type == EX_NODE_SET) {
    ex_inq_val = EX_INQ_NODE_SETS;
    idsptr = VAR_NS_IDS;
    statptr = VAR_NS_STAT;
  }
  else if (set_type == EX_EDGE_SET) {
    ex_inq_val = EX_INQ_EDGE_SETS;
    idsptr = VAR_ES_IDS;
    statptr = VAR_ES_STAT;
  }
  else if (set_type == EX_FACE_SET) {
    ex_inq_val = EX_INQ_FACE_SETS;
    idsptr = VAR_FS_IDS;
    statptr = VAR_FS_STAT;
  }
  else if (set_type == EX_SIDE_SET) {
    ex_inq_val = EX_INQ_SIDE_SETS;
    idsptr = VAR_SS_IDS;
    statptr = VAR_SS_STAT;
  }
  else if (set_type == EX_ELEM_SET) {
    ex_inq_val = EX_INQ_ELEM_SETS;
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

  /* first check if any sets are specified */
  if ((status = nc_inq_dimid(exoid, ex_dim_num_objects(set_type), &temp)) != NC_NOERR) {
    if (status == NC_EBADDIM) {
      exerrval = status;
      sprintf(errmsg,
        "Error: no %ss defined for file id %d", ex_name_of_object(set_type), exoid);
      ex_err("ex_put_concat_sets",errmsg,exerrval);
    } else {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to locate %ss defined in file id %d",
        ex_name_of_object(set_type), exoid);
      ex_err("ex_put_concat_sets",errmsg,exerrval);
    }
    return (EX_FATAL);
  }
   
  /* inquire how many sets are to be stored */
  if (ex_inquire(exoid, ex_inq_val, &num_sets, &fdum, cdum) != NC_NOERR)  {
    sprintf(errmsg,
      "Error: failed to get number of %ss defined for file id %d",
      ex_name_of_object(set_type), exoid);
    /* use error val from inquire */
    ex_err("ex_put_concat_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* Fill out set status array */

  /* First, allocate space for the status list */
  if (!(set_stat= malloc(num_sets*sizeof(int)))) {
    exerrval = EX_MEMFAIL;
    sprintf(errmsg,
      "Error: failed to allocate space for %s status array in file id %d",
      ex_name_of_object(set_type), exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  for (i=0;i<num_sets;i++)  {
    if (num_entries_per_set[i] == 0) /* Is this a NULL set? */
      set_stat[i] = 0; /* change set status to NULL */
    else
      set_stat[i] = 1; /* change set status to TRUE */
  }

  /* Next, get variable id of status array */
  if ((status = nc_inq_varid(exoid, statptr, &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to locate %s status in file id %d", 
      ex_name_of_object(set_type), exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  status = nc_put_var_int(exoid, varid, set_stat);

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to store %s status array to file id %d",
      ex_name_of_object(set_type), exoid);
    ex_err("ex_put_concat_set",errmsg,exerrval);
    return (EX_FATAL);
  }

  free(set_stat);

  /* put netcdf file into define mode  */
  if ((status = nc_redef (exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to put file id %d into define mode",
      exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* create set definitions */
  for (i=0; i<num_sets; i++) {
    /* Keep track of the total number of sets defined using a counter stored
       in a linked list keyed by exoid.
       NOTE: ex_get_file_item  is used to find the number of sets of type
       for a specific file and returns that value.
    */
    cur_num_sets=ex_get_file_item(exoid, ex_get_counter_list(set_type));
    if (cur_num_sets >= num_sets) {
      exerrval = EX_FATAL;
      sprintf(errmsg,
        "Error: exceeded number of %ss (%d) defined in file id %d",
        ex_name_of_object(set_type), num_sets,exoid);
      ex_err("ex_put_concat_sets",errmsg,exerrval);
      goto error_ret;
    }

    /*   NOTE: ex_inc_file_item  is used to find the number of sets
   for a specific file and returns that value incremented. */

    cur_num_sets=ex_inc_file_item(exoid, ex_get_counter_list(set_type));
    set_id_ndx = cur_num_sets + 1;
    
    /* setup more pointers based on set_type */
    if (set_type == EX_NODE_SET) {
      elemptr = VAR_NODE_NS(set_id_ndx);
      extraptr = NULL;
      /* note we are using DIM_NUM_NODE_NS instead of DIM_NUM_DF_NS */
      numdfptr = DIM_NUM_NOD_NS(set_id_ndx);
      factptr = VAR_FACT_NS(set_id_ndx);
    }
    else if (set_type == EX_EDGE_SET) {
      elemptr = VAR_EDGE_ES(set_id_ndx);
      extraptr = VAR_ORNT_ES(set_id_ndx);
      numdfptr = DIM_NUM_DF_ES(set_id_ndx);
      factptr = VAR_FACT_ES(set_id_ndx);
    }
    else if (set_type == EX_FACE_SET) {
      elemptr = VAR_FACE_FS(set_id_ndx);
      extraptr = VAR_ORNT_FS(set_id_ndx);
      numdfptr = DIM_NUM_DF_FS(set_id_ndx);
      factptr = VAR_FACT_FS(set_id_ndx);
    }
    else if (set_type == EX_SIDE_SET) {
      elemptr = VAR_ELEM_SS(set_id_ndx);
      extraptr = VAR_SIDE_SS(set_id_ndx);
      numdfptr = DIM_NUM_DF_SS(set_id_ndx);
      factptr = VAR_FACT_SS(set_id_ndx);
    }
    if (set_type == EX_ELEM_SET) {
      elemptr = VAR_ELEM_ELS(set_id_ndx);
      extraptr = NULL;
      numdfptr = DIM_NUM_DF_ELS(set_id_ndx);
      factptr = VAR_FACT_ELS(set_id_ndx);
    }

    /*  define dimension for number of entries per set */
    if (num_entries_per_set[i] == 0) /* Is this a NULL set? */
      continue; /* Do not create anything for NULL sets! */

    if ((status = nc_def_dim(exoid, ex_dim_num_entries_in_object(set_type, set_id_ndx),
           num_entries_per_set[i], &dimid)) != NC_NOERR) {
      if (status == NC_ENAMEINUSE) {
  exerrval = status;
  sprintf(errmsg,
    "Error: %s entry count %d already defined in file id %d",
    ex_name_of_object(set_type), set_ids[i],exoid);
  ex_err("ex_put_concat_sets",errmsg,exerrval);
      } else {
  exerrval = status;
  sprintf(errmsg,
    "Error: failed to define number of entries for %s %d in file id %d",
    ex_name_of_object(set_type), set_ids[i],exoid);
  ex_err("ex_put_concat_sets",errmsg,exerrval);
      }
      goto error_ret;
    }

    /* create element list variable for set */

    dims[0] = dimid;
    if ((status = nc_def_var(exoid,elemptr,NC_INT,1,dims, &temp)) != NC_NOERR) {
      if (status == NC_ENAMEINUSE) {
  exerrval = status;
  sprintf(errmsg,
    "Error: element list already exists for %s %d in file id %d",
    ex_name_of_object(set_type), set_ids[i],exoid);
  ex_err("ex_put_concat_sets",errmsg,exerrval);
      } else {
  exerrval = status;
  sprintf(errmsg,
    "Error: failed to create element list for %s %d in file id %d",
    ex_name_of_object(set_type), set_ids[i],exoid);
  ex_err("ex_put_concat_sets",errmsg,exerrval);
      }
      goto error_ret;            /* exit define mode and return */
    }

    /* create extra list variable for set  (only for edge, face and side sets) */
    if (extraptr) {
      if ((status = nc_def_var(exoid,extraptr,NC_INT,1,dims, &temp)) != NC_NOERR) { 
  if (status == NC_ENAMEINUSE) {
    exerrval = status;
    sprintf(errmsg,
      "Error: extra list already exists for %s %d in file id %d",
      ex_name_of_object(set_type), set_ids[i],exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
  } else {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to create extra list for %s %d in file id %d",
      ex_name_of_object(set_type), set_ids[i],exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
  }
  goto error_ret;         /* exit define mode and return */
      }
    }

    /*  define dimension for number of dist factors per set */
    /*  NOTE: only define df count if the dist factors exist! */
    if (num_dist_per_set[i] > 0) {
      
      if (set_type == EX_NODE_SET) {
  if (num_dist_per_set[i] != num_entries_per_set[i]) {
    exerrval = EX_FATAL;
    sprintf(errmsg,
      "Error: # dist fact (%d) not equal to # nodes (%d) in node set %d file id %d",
      num_dist_per_set[i], num_entries_per_set[i], set_ids[i],exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
    goto error_ret;          /* exit define mode and return */
  }

  /* resuse dimid from entry lists */
      } else  {
  if ((status = nc_def_dim(exoid, numdfptr,
         num_dist_per_set[i], &dimid)) != NC_NOERR) {
    if (status == NC_ENAMEINUSE) {
      exerrval = status;
      sprintf(errmsg,
        "Error: %s df count %d already defined in file id %d",
        ex_name_of_object(set_type), set_ids[i],exoid);
      ex_err("ex_put_concat_sets",errmsg,exerrval);
    } else {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to define %s df count for set %d in file id %d",
        ex_name_of_object(set_type), set_ids[i],exoid);
      ex_err("ex_put_concat_sets",errmsg,exerrval);
    }
    goto error_ret;
  }
      }

      /* create distribution factor list variable for set */
      dims[0] = dimid;
      if ((status = nc_def_var(exoid, factptr, nc_flt_code(exoid), 1, dims, &temp)) != NC_NOERR) {
  if (status == NC_ENAMEINUSE) {
    exerrval = status;
    sprintf(errmsg,
      "Error: dist factor list already exists for %s %d in file id %d",
      ex_name_of_object(set_type), set_ids[i],exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
  } else {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to create dist factor list for %s %d in file id %d",
      ex_name_of_object(set_type), set_ids[i],exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
  }
  goto error_ret;            /* exit define mode and return */
      }
    } /* end define dist factors */
  }

  /* leave define mode  */
  if ((status = nc_enddef (exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to complete definition in file id %d",
      exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* Next, fill out set ids array */

  /* first get id of set ids array variable */
  if ((status = nc_inq_varid(exoid, idsptr, &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to locate %s ids array in file id %d",
      ex_name_of_object(set_type), exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* then, write out set id list */
  status = nc_put_var_int(exoid, varid, set_ids);

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to store %s id array in file id %d",
      ex_name_of_object(set_type), exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* If the sets_entry_index is passed in as a NULL pointer, then
   *  the user only wants us to define the sets and not populate
   *  the data structures.
   */
  if (sets_entry_index == 0)
    return(EX_NOERR);
  
  /* Now, use ExodusII call to store sets */
  for (i=0; i<num_sets; i++) {

      if (num_entries_per_set[i] == 0) /* Is this a NULL set? */
  continue; /* Do not create anything for NULL sets! */

      /* set extra list */
      if (set_type == EX_EDGE_SET || set_type == EX_FACE_SET ||
    set_type == EX_SIDE_SET)
  extra_list = &(sets_extra_list[sets_entry_index[i]]);
      else
  extra_list = NULL;

      if (ex_put_set(exoid, set_type, set_ids[i], 
         &(sets_entry_list[sets_entry_index[i]]),
         extra_list) == -1)
  return(EX_FATAL); /* error will be reported by subroutine */

      if (ex_comp_ws(exoid) == sizeof(float)) {
  flt_dist_fact = sets_dist_fact;
  if (num_dist_per_set[i] > 0) {     /* store dist factors if required */
    if (ex_put_set_dist_fact(exoid, set_type, set_ids[i],
           &(flt_dist_fact[sets_dist_index[i]])) == -1) {
        sprintf(errmsg,
          "Error: failed to store %s %d dist factors for file id %d",
          ex_name_of_object(set_type), set_ids[i],exoid);
        /* use error val from exodusII routine */
        ex_err("ex_put_concat_sets",errmsg,exerrval);
        return (EX_FATAL);
      }
  }
      } else if (ex_comp_ws(exoid) == sizeof(double)) {
  dbl_dist_fact = sets_dist_fact;
  if (num_dist_per_set[i] > 0) {             /* only store if they exist */
    if (ex_put_set_dist_fact(exoid, set_type, set_ids[i],
           &(dbl_dist_fact[sets_dist_index[i]])) == -1) {
      sprintf(errmsg,
        "Error: failed to store %s %d dist factors for file id %d",
        ex_name_of_object(set_type), set_ids[i],exoid);
      /* use error val from exodusII routine */
      ex_err("ex_put_concat_sets",errmsg,exerrval);
      return (EX_FATAL);
    }
  }
      } else {
  /* unknown floating point word size */
  exerrval = EX_BADPARAM;
  sprintf(errmsg,
    "Error: unsupported floating point word size %d for file id %d",
    ex_comp_ws(exoid), exoid);
  ex_err("ex_put_concat_sets", errmsg, exerrval);
  return (EX_FATAL);
      }
  }
  return(EX_NOERR);


  /* Fatal error: exit definition mode and return */
 error_ret:
  if (nc_enddef (exoid) != NC_NOERR)     /* exit define mode */
    {
      sprintf(errmsg,
        "Error: failed to complete definition for file id %d",
        exoid);
      ex_err("ex_put_concat_sets",errmsg,exerrval);
    }
  return (EX_FATAL);
}
