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
* exgcss - ex_get_concat_sets
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       ex_entity_type set_type                type of set
*
* exit conditions -
*       struct ex_set_specs* set_specs  set specs structure
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the set ID's, set entry count array, set entry pointers
 * array, set entry list, set extra list, and set distribution factors
 * for all sets of the specified type.
 */

int ex_get_concat_sets (int   exoid,
                        ex_entity_type set_type,
                        struct ex_set_specs* set_specs)
{
  int status, dimid;
  int  *set_ids = set_specs->sets_ids;
  int  *num_entries_per_set = set_specs->num_entries_per_set;
  int  *num_dist_per_set = set_specs->num_dist_per_set;
  int  *sets_entry_index = set_specs->sets_entry_index;
  int  *sets_dist_index = set_specs->sets_dist_index;
  int  *sets_entry_list = set_specs->sets_entry_list;
  int  *sets_extra_list = set_specs->sets_extra_list;
  void *sets_dist_fact = set_specs->sets_dist_fact;
  char *cdum;
  int num_sets, i;
  float fdum;
  float  *flt_dist_fact;
  double *dbl_dist_fact;
  char errmsg[MAX_ERR_LENGTH];
  ex_inquiry ex_inq_val;

  exerrval = 0; /* clear error code */

  cdum = 0; /* initialize even though it is not used */

  /* setup pointers based on set_type 
     NOTE: there is another block that sets more stuff later ... */

  if (set_type == EX_NODE_SET) {
    ex_inq_val = EX_INQ_NODE_SETS;
  }
  else if (set_type == EX_EDGE_SET) {
    ex_inq_val = EX_INQ_EDGE_SETS;
  }
  else if (set_type == EX_FACE_SET) {
    ex_inq_val = EX_INQ_FACE_SETS;
  }
  else if (set_type == EX_SIDE_SET) {
    ex_inq_val = EX_INQ_SIDE_SETS;
  }
  else if (set_type == EX_ELEM_SET) {
    ex_inq_val = EX_INQ_ELEM_SETS;
  }
  else {
    exerrval = EX_FATAL;
    sprintf(errmsg,
	    "Error: invalid set type (%d)", set_type);
    ex_err("ex_put_set_param",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* first check if any sets are specified */

  if ((status = nc_inq_dimid(exoid, ex_dim_num_objects(set_type), &dimid)) != NC_NOERR) {
    exerrval = status;
      if (status == NC_EBADDIM) {
	sprintf(errmsg,
		"Warning: no %ss defined for file id %d",
		ex_name_of_object(set_type), exoid);
	ex_err("ex_get_concat_sets",errmsg,exerrval);
	return (EX_WARN);
      } else {
	sprintf(errmsg,
		"Error: failed to locate %ss defined in file id %d", 
		ex_name_of_object(set_type), exoid);
	ex_err("ex_get_concat_sets",errmsg,exerrval);
	return (EX_FATAL);
      }
  }

  /* inquire how many sets have been stored */

  if (ex_inquire(exoid, ex_inq_val, &num_sets, &fdum, cdum) != NC_NOERR) {
    sprintf(errmsg,
            "Error: failed to get number of %ss defined for file id %d",
	    ex_name_of_object(set_type), exoid);
    /* use error val from inquire */
    ex_err("ex_get_concat_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (ex_get_ids (exoid, set_type, set_ids) != NC_NOERR) {
    sprintf(errmsg,
            "Error: failed to get %s ids for file id %d",
	    ex_name_of_object(set_type), exoid);
    /* use error val from inquire */
    ex_err("ex_get_concat_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  sets_entry_index[0] = 0;
  sets_dist_index[0] = 0;

  for (i=0; i<num_sets; i++) {
    if (ex_get_set_param(exoid, set_type, set_ids[i], 
			 &(num_entries_per_set[i]), &(num_dist_per_set[i])) != NC_NOERR)
      return(EX_FATAL); /* error will be reported by sub */

    if (i < num_sets-1) {
      /* fill in entry and dist factor index arrays */
      sets_entry_index[i+1] = sets_entry_index[i]+num_entries_per_set[i];
      sets_dist_index[i+1] = sets_dist_index[i]+num_dist_per_set[i];
    }

    if (num_entries_per_set[i] == 0) /* NULL  set? */
      continue;

    /* Now, use ExodusII call to get sets */

    if (ex_comp_ws(exoid) == sizeof(float)) {
      int *extra_sets = NULL;
      if (sets_extra_list != NULL)
	extra_sets = &(sets_extra_list[sets_entry_index[i]]);
       
      if (ex_get_set(exoid, set_type, set_ids[i],
		     &(sets_entry_list[sets_entry_index[i]]),
		     extra_sets) == -1)
	return(EX_FATAL); /* error will be reported by subroutine */

      /* get distribution factors for this set */
      flt_dist_fact = sets_dist_fact;
      if (num_dist_per_set[i] > 0) {      /* only get df if they exist */
	if (ex_get_set_dist_fact(exoid, set_type, set_ids[i],
				 &(flt_dist_fact[sets_dist_index[i]])) != NC_NOERR) {
	  sprintf(errmsg,
                  "Error: failed to get %s %d dist factors in file id %d",
		  ex_name_of_object(set_type), set_ids[i], exoid);
	  ex_err("ex_get_concat_sets",errmsg,exerrval);
	  return(EX_FATAL);
	}
      } else {  /* fill distribution factor array with 1's */
      }
    }
    else if (ex_comp_ws(exoid) == sizeof(double))
      {
	if (ex_get_set(exoid, set_type, set_ids[i],
		       &(sets_entry_list[sets_entry_index[i]]),
		       &(sets_extra_list[sets_entry_index[i]])) != NC_NOERR)
	  return(EX_FATAL); /* error will be reported by subroutine */

	/* get distribution factors for this set */
	dbl_dist_fact = sets_dist_fact;
	if (num_dist_per_set[i] > 0) {      /* only get df if they exist */
	  if (ex_get_set_dist_fact(exoid, set_type, set_ids[i],
				   &(dbl_dist_fact[sets_dist_index[i]])) != NC_NOERR) {
	    sprintf(errmsg,
		    "Error: failed to get %s %d dist factors in file id %d",
		    ex_name_of_object(set_type), set_ids[i], exoid);
	    ex_err("ex_get_concat_sets",errmsg,exerrval);
	    return(EX_FATAL);
	  }
	} else {  /* fill distribution factor array with 1's */
	}
      }
  }
  return(EX_NOERR);
}
