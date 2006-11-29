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
*  Id
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * writes the set ID's, set entry count array, set entry pointers array,
 * set entry list, set extra list, and distribution factors list for 
 * all the sets of the specified type.
 */

int ex_put_concat_sets (int   exoid,
      int   set_type,
      const struct ex_set_specs* set_specs)
{
  const int  *set_ids = set_specs->sets_ids;
  const int  *num_entries_per_set = set_specs->num_entries_per_set;
  const int  *num_dist_per_set = set_specs->num_dist_per_set;
  const int  *sets_entry_index = set_specs->sets_entry_index;
  const int  *sets_dist_index = set_specs->sets_dist_index;
  const int  *sets_entry_list = set_specs->sets_entry_list;
  const int  *sets_extra_list = set_specs->sets_extra_list;
  const void *sets_dist_fact = set_specs->sets_dist_fact;
  char *cdum;
  int i, num_sets, cur_num_sets, dimid, varid, set_id_ndx, dims[1], *set_stat;
  int iresult;
  long start[1], count[1]; 
  nclong *lptr;
  float fdum;
  const float *flt_dist_fact;
  const double *dbl_dist_fact;
  char errmsg[MAX_ERR_LENGTH];
  char* typeName;
  char* dimptr;
  char* idsptr;
  char* statptr;
  char* numentryptr;
  char* numdfptr;
  char* factptr;
  char* elemptr;
  char* extraptr;
  struct list_item** ctr_list_ptr;
  int ex_inq_val;
  const int *extra_list;   

  exerrval = 0; /* clear error code */

  cdum = 0; /* initialize even though it is not used */

  /* setup pointers based on set_type 
    NOTE: there is another block that sets more stuff later ... */

   if (set_type == EX_NODE_SET) {
     typeName = "node";
     ex_inq_val = EX_INQ_NODE_SETS;
     dimptr = DIM_NUM_NS;
     idsptr = VAR_NS_IDS;
     statptr = VAR_NS_STAT;
     ctr_list_ptr = &ns_ctr_list; 
   }
   else if (set_type == EX_EDGE_SET) {
     typeName = "edge";
     ex_inq_val = EX_INQ_EDGE_SETS;
     dimptr = DIM_NUM_ES;
     idsptr = VAR_ES_IDS;
     statptr = VAR_ES_STAT;
     ctr_list_ptr = &es_ctr_list;
   }
   else if (set_type == EX_FACE_SET) {
     typeName = "face";
     ex_inq_val = EX_INQ_FACE_SETS;
     dimptr = DIM_NUM_FS;
     idsptr = VAR_FS_IDS;
     statptr = VAR_FS_STAT;
     ctr_list_ptr = &fs_ctr_list;
   }
   else if (set_type == EX_SIDE_SET) {
     typeName = "side";
     ex_inq_val = EX_INQ_SIDE_SETS;
     dimptr = DIM_NUM_SS;
     idsptr = VAR_SS_IDS;
     statptr = VAR_SS_STAT;
     ctr_list_ptr = &ss_ctr_list;
   }
   else if (set_type == EX_ELEM_SET) {
     typeName = "elem";
     ex_inq_val = EX_INQ_ELEM_SETS;
     dimptr = DIM_NUM_ELS;
     idsptr = VAR_ELS_IDS;
     statptr = VAR_ELS_STAT;
     ctr_list_ptr = &els_ctr_list;
   }
   else {
     exerrval = EX_FATAL;
     sprintf(errmsg,
             "Error: invalid set type (%d)", set_type);
     ex_err("ex_put_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

/* first check if any sets are specified */

  if (ncdimid (exoid, dimptr) == -1)
  {
    if (ncerr == NC_EBADDIM)
    {
      exerrval = ncerr;
      sprintf(errmsg,
        "Error: no %s sets defined for file id %d", typeName, exoid);
      ex_err("ex_put_concat_sets",errmsg,exerrval);
    }
    else
    {
      exerrval = ncerr;
      sprintf(errmsg,
        "Error: failed to locate %s sets defined in file id %d",
        typeName, exoid);
      ex_err("ex_put_concat_sets",errmsg,exerrval);
    }
    return (EX_FATAL);
  }

/* inquire how many sets are to be stored */

  if (ex_inquire(exoid, ex_inq_val, &num_sets, &fdum, cdum) == -1)
  {
    sprintf(errmsg,
           "Error: failed to get number of %s sets defined for file id %d",
            typeName, exoid);
    /* use error val from inquire */
    ex_err("ex_put_concat_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* Fill out set status array */

  /* First, allocate space for the status list */
  if (!(set_stat= malloc(num_sets*sizeof(int))))
  {
    exerrval = EX_MEMFAIL;
    sprintf(errmsg,
    "Error: failed to allocate space for %s set status array in file id %d",
            typeName, exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  for (i=0;i<num_sets;i++)
  {
    if (num_entries_per_set[i] == 0) /* Is this a NULL set? */
      set_stat[i] = 0; /* change set status to NULL */
    else
      set_stat[i] = 1; /* change set status to TRUE */
  }

  /* Next, get variable id of status array */
  if ((varid = ncvarid (exoid, statptr)) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to locate %s set status in file id %d", 
      typeName, exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

/* this contortion is necessary because netCDF is expecting nclongs; fortunately
   it's necessary only when ints and nclongs aren't the same size */

  start[0] = 0;
  count[0] = num_sets;

  if (sizeof(int) == sizeof(nclong)) {
     iresult = ncvarput (exoid, varid, start, count, set_stat);
  } else {
     lptr = itol (set_stat, num_sets);
     iresult = ncvarput (exoid, varid, start, count, lptr);
     free(lptr);
  }

  if (iresult == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to store %s set status array to file id %d",
            typeName, exoid);
    ex_err("ex_put_concat_set",errmsg,exerrval);
    return (EX_FATAL);
  }

  free(set_stat);

/* put netcdf file into define mode  */

  if (ncredef (exoid) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to put file id %d into define mode",
            exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* create set definitions */
  for (i=0; i<num_sets; i++)
  {
/* Keep track of the total number of sets defined using a counter stored
   in a linked list keyed by exoid.
   NOTE: ex_get_file_item  is used to find the number of sets of type
         for a specific file and returns that value.
*/
    cur_num_sets=ex_get_file_item(exoid, ctr_list_ptr );
    if (cur_num_sets >= num_sets)
    {
      exerrval = EX_FATAL;
      sprintf(errmsg,
             "Error: exceeded number of %s sets (%d) defined in file id %d",
              typeName, num_sets,exoid);
      ex_err("ex_put_concat_sets",errmsg,exerrval);
      goto error_ret;
    }

/*   NOTE: ex_inc_file_item  is used to find the number of sets
         for a specific file and returns that value incremented. */

    cur_num_sets=ex_inc_file_item(exoid, ctr_list_ptr );
    set_id_ndx = cur_num_sets + 1;
    
    /* setup more pointers based on set_type */
    
    if (set_type == EX_NODE_SET) {
      numentryptr = DIM_NUM_NOD_NS(set_id_ndx);
      elemptr = VAR_NODE_NS(set_id_ndx);
      extraptr = NULL;
      /* note we are using DIM_NUM_NODE_NS instead of DIM_NUM_DF_NS */
      numdfptr = DIM_NUM_NOD_NS(set_id_ndx);
      factptr = VAR_FACT_NS(set_id_ndx);
    }
    else if (set_type == EX_EDGE_SET) {
      numentryptr = DIM_NUM_EDGE_ES(set_id_ndx);
      elemptr = VAR_EDGE_ES(set_id_ndx);
      extraptr = VAR_ORNT_ES(set_id_ndx);
      numdfptr = DIM_NUM_DF_ES(set_id_ndx);
      factptr = VAR_FACT_ES(set_id_ndx);
    }
    else if (set_type == EX_FACE_SET) {
      numentryptr = DIM_NUM_FACE_FS(set_id_ndx);
      elemptr = VAR_FACE_FS(set_id_ndx);
      extraptr = VAR_ORNT_FS(set_id_ndx);
      numdfptr = DIM_NUM_DF_FS(set_id_ndx);
      factptr = VAR_FACT_FS(set_id_ndx);
    }
    else if (set_type == EX_SIDE_SET) {
      numentryptr = DIM_NUM_SIDE_SS(set_id_ndx);
      elemptr = VAR_ELEM_SS(set_id_ndx);
      extraptr = VAR_SIDE_SS(set_id_ndx);
      numdfptr = DIM_NUM_DF_SS(set_id_ndx);
      factptr = VAR_FACT_SS(set_id_ndx);
    }
    if (set_type == EX_ELEM_SET) {
      numentryptr = DIM_NUM_ELE_ELS(set_id_ndx);
      elemptr = VAR_ELEM_ELS(set_id_ndx);
      extraptr = NULL;
      numdfptr = DIM_NUM_DF_ELS(set_id_ndx);
      factptr = VAR_FACT_ELS(set_id_ndx);
    }

/*  define dimension for number of entries per set */

    if (num_entries_per_set[i] == 0) /* Is this a NULL set? */
      continue; /* Do not create anything for NULL sets! */


    if ((dimid = ncdimdef (exoid, numentryptr,
                   (long)num_entries_per_set[i])) == -1)
    {
      if (ncerr == NC_ENAMEINUSE)
      {
        exerrval = ncerr;
        sprintf(errmsg,
               "Error: %s set entry count %d already defined in file id %d",
                typeName, set_ids[i],exoid);
        ex_err("ex_put_concat_sets",errmsg,exerrval);
      }
      else
      {
        exerrval = ncerr;
        sprintf(errmsg,
             "Error: failed to define number of entries for %s set %d in file id %d",
                typeName, set_ids[i],exoid);
        ex_err("ex_put_concat_sets",errmsg,exerrval);
      }
      goto error_ret;
    }

    /* create element list variable for set */

    dims[0] = dimid;
    if (ncvardef (exoid,elemptr,NC_LONG,1,dims) == -1)
    {
      if (ncerr == NC_ENAMEINUSE)
      {
        exerrval = ncerr;
        sprintf(errmsg,
             "Error: element list already exists for %s set %d in file id %d",
                typeName, set_ids[i],exoid);
        ex_err("ex_put_concat_sets",errmsg,exerrval);
      }
      else
      {
        exerrval = ncerr;
        sprintf(errmsg,
           "Error: failed to create element list for %s set %d in file id %d",
                typeName, set_ids[i],exoid);
        ex_err("ex_put_concat_sets",errmsg,exerrval);
      }
      goto error_ret;            /* exit define mode and return */
    }

    /* create extra list variable for set  (only for edge, face and side sets) */

    if (extraptr)
    {
      if (ncvardef (exoid,extraptr,NC_LONG,1,dims) == -1)
      { 
  if (ncerr == NC_ENAMEINUSE)
        {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: extra list already exists for %s set %d in file id %d",
      typeName, set_ids[i],exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
  }
  else
        {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: failed to create extra list for %s set %d in file id %d",
      typeName, set_ids[i],exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
  }
  goto error_ret;         /* exit define mode and return */
      }
    }

/*  define dimension for number of dist factors per set */

/*  NOTE: only define df count if the dist factors exist! */

    if (num_dist_per_set[i] > 0)
    {
      
      if (set_type == EX_NODE_SET)
      {
        if (num_dist_per_set[i] != num_entries_per_set[i])
        {
    exerrval = EX_FATAL;
    sprintf(errmsg,
  "Error: # dist fact (%d) not equal to # nodes (%d) in node set %d file id %d",
            num_dist_per_set[i], num_entries_per_set[i], set_ids[i],exoid);
        ex_err("ex_put_concat_sets",errmsg,exerrval);
        goto error_ret;          /* exit define mode and return */
      }

       /* resuse dimid from entry lists */

      }
      else 
      {
        if ((dimid = ncdimdef (exoid, numdfptr,
             (long)num_dist_per_set[i])) == -1)
  {
    if (ncerr == NC_ENAMEINUSE)
    {
      exerrval = ncerr;
      sprintf(errmsg,
        "Error: %s set df count %d already defined in file id %d",
        typeName, set_ids[i],exoid);
      ex_err("ex_put_concat_sets",errmsg,exerrval);
    }
    else
          {
      exerrval = ncerr;
      sprintf(errmsg,
        "Error: failed to define %s set df count for set %d in file id %d",
        typeName, set_ids[i],exoid);
      ex_err("ex_put_concat_sets",errmsg,exerrval);
    }
    goto error_ret;
  }
      }

      /* create distribution factor list variable for set */

      dims[0] = dimid;
      if (ncvardef (exoid, factptr,
                    nc_flt_code(exoid), 1, dims) == -1)
      {
        if (ncerr == NC_ENAMEINUSE)
        {
          exerrval = ncerr;
          sprintf(errmsg,
         "Error: dist factor list already exists for %s set %d in file id %d",
                  typeName, set_ids[i],exoid);
          ex_err("ex_put_concat_sets",errmsg,exerrval);
        }
        else
        {
          exerrval = ncerr;
          sprintf(errmsg,
       "Error: failed to create dist factor list for %s set %d in file id %d",
                  typeName, set_ids[i],exoid);
          ex_err("ex_put_concat_sets",errmsg,exerrval);
        }
        goto error_ret;            /* exit define mode and return */
      }
    } /* end define dist factors */
  }

/* leave define mode  */

  if (ncendef (exoid) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to complete definition in file id %d",
            exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

/* Next, fill out set ids array */

  /* first get id of set ids array variable */

  if ((varid = ncvarid (exoid, idsptr)) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to locate %s set ids array in file id %d",
            typeName, exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* then, write out set id list */

/* this contortion is necessary because netCDF is expecting nclongs; fortunately
   it's necessary only when ints and nclongs aren't the same size */

  start[0] = 0;
  count[0] = num_sets;

  if (sizeof(int) == sizeof(nclong)) {
     iresult = ncvarput (exoid, varid, start, count, set_ids);
  } else {
     lptr = itol (set_ids, num_sets);
     iresult = ncvarput (exoid, varid, start, count, lptr);
     free(lptr);
  }

  if (iresult == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to store %s set id array in file id %d",
            typeName, exoid);
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
  for (i=0; i<num_sets; i++)
  {

    if (num_entries_per_set[i] == 0) /* Is this a NULL set? */
      continue; /* Do not create anything for NULL sets! */

    /* set extra list */

    if (set_type == EX_EDGE_SET || set_type == EX_FACE_SET ||
  set_type == EX_SIDE_SET)
      extra_list = &(sets_extra_list[sets_entry_index[i]]);
    else
      extra_list = NULL;

    if (ex_comp_ws(exoid) == sizeof(float))
    {
      flt_dist_fact = sets_dist_fact;
      if (ex_put_set(exoid, set_type, set_ids[i], 
         &(sets_entry_list[sets_entry_index[i]]),
         extra_list) == -1)
         return(EX_FATAL); /* error will be reported by subroutine */
      if (num_dist_per_set[i] > 0)      /* store dist factors if required */
      {
        if (ex_put_set_dist_fact(exoid, set_type, set_ids[i],
                         &(flt_dist_fact[sets_dist_index[i]])) == -1)
        {
          sprintf(errmsg,
               "Error: failed to store %s set %d dist factors for file id %d",
                  typeName, set_ids[i],exoid);
          /* use error val from exodusII routine */
          ex_err("ex_put_concat_sets",errmsg,exerrval);
          return (EX_FATAL);
        }
      }
    }
    else if (ex_comp_ws(exoid) == sizeof(double))
    {
      dbl_dist_fact = sets_dist_fact;
      if (ex_put_set(exoid, set_type, set_ids[i], 
         &(sets_entry_list[sets_entry_index[i]]),
         extra_list) == -1)
        return(EX_FATAL); /* error will be reported by subroutine */
      if (num_dist_per_set[i] > 0)             /* only store if they exist */
      {
        if (ex_put_set_dist_fact(exoid, set_type, set_ids[i],
         &(dbl_dist_fact[sets_dist_index[i]])) == -1)
        {
          sprintf(errmsg,
               "Error: failed to store %s set %d dist factors for file id %d",
                  typeName, set_ids[i],exoid);
          /* use error val from exodusII routine */
          ex_err("ex_put_concat_sets",errmsg,exerrval);
          return (EX_FATAL);
        }
      }
    }
    else
    {
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
  if (ncendef (exoid) == -1)     /* exit define mode */
  {
    sprintf(errmsg,
           "Error: failed to complete definition for file id %d",
            exoid);
    ex_err("ex_put_concat_sets",errmsg,exerrval);
  }
  return (EX_FATAL);
}
