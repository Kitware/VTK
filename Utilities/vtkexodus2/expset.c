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
* expss - ex_put_set
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     set_type                set type
*       int     set_id                  set id
*       int*    set_entry_list          array of entries in set
*       int*    set_extra_list          array of extras in set

* exit conditions - 
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"
#include <stdlib.h> /* for free() */

/*!
 * writes the set entry list and set extra list for a single set
 * \param   exoid                   exodus file id
 * \param   set_type                set type
 * \param   set_id                  set id
 * \param  *set_entry_list          array of entries in set
 * \param  *set_extra_list          array of extras in set
 */

int ex_put_set (int   exoid,
		ex_entity_type set_type,
		int   set_id,
		const int  *set_entry_list,
		const int  *set_extra_list)
{
  int dimid, status;
  int entry_list_id, extra_list_id, set_id_ndx;
  char errmsg[MAX_ERR_LENGTH];
  char* entryptr = NULL;
  char* extraptr = NULL;

  exerrval = 0; /* clear error code */

  /* first check if any sets are specified */
  if ((status = nc_inq_dimid(exoid, ex_dim_num_objects(set_type), &dimid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: no %ss defined in file id %d",
	    ex_name_of_object(set_type), exoid);
    ex_err("ex_put_set",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* Lookup index of set id in VAR_*S_IDS array */
  set_id_ndx = ex_id_lkup(exoid, set_type,set_id);
  if (exerrval != 0) {
    if (exerrval == EX_NULLENTITY) {
      sprintf(errmsg,
              "Warning: no data allowed for NULL %s %d in file id %d",
	      ex_name_of_object(set_type),set_id,exoid);
      ex_err("ex_put_set",errmsg,EX_MSG);
      return (EX_WARN);
    } else {
      sprintf(errmsg,
	      "Error: failed to locate %s id %d in VAR_*S_IDS array in file id %d",
	      ex_name_of_object(set_type), set_id,exoid);
      ex_err("ex_put_set",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  /* setup more pointers based on set_type */
  if (set_type == EX_NODE_SET) {
    entryptr = VAR_NODE_NS(set_id_ndx);
    extraptr = NULL;
  }
  else if (set_type == EX_EDGE_SET) {
    entryptr = VAR_EDGE_ES(set_id_ndx);
    extraptr = VAR_ORNT_ES(set_id_ndx);
  }
  else if (set_type == EX_FACE_SET) {
    entryptr = VAR_FACE_FS(set_id_ndx);
    extraptr = VAR_ORNT_FS(set_id_ndx);
  }
  else if (set_type == EX_SIDE_SET) {
    entryptr = VAR_ELEM_SS(set_id_ndx);
    extraptr = VAR_SIDE_SS(set_id_ndx);
  }
  if (set_type == EX_ELEM_SET) {
    entryptr = VAR_ELEM_ELS(set_id_ndx);
    extraptr = NULL;
  }

  /* inquire id's of previously defined variables  */
  if ((status = nc_inq_varid(exoid, entryptr, &entry_list_id)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to locate entry list for %s %d in file id %d",
	    ex_name_of_object(set_type), set_id,exoid);
    ex_err("ex_put_set",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* only do for edge, face and side sets */
  if (extraptr) {
    if ((status = nc_inq_varid(exoid, extraptr, &extra_list_id)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to locate extra list for %s %d in file id %d",
	      ex_name_of_object(set_type), set_id,exoid);
      ex_err("ex_put_set",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  /* write out the entry list and extra list arrays */

  status = nc_put_var_int(exoid, entry_list_id, set_entry_list);

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to store entry list for %s %d in file id %d",
	    ex_name_of_object(set_type), set_id,exoid);
    ex_err("ex_put_set",errmsg,exerrval);
    return (EX_FATAL);
  }


  /* only do for edge, face and side sets */
  if (extraptr) {
    if ( set_extra_list == NULL ) {
      sprintf(errmsg, "Error: extra list NULL for %s %d in file id %d",
	      ex_name_of_object(set_type), set_id, exoid );
      ex_err("ex_put_set",errmsg,EX_BADPARAM);
      return (EX_FATAL);
    }

    status = nc_put_var_int(exoid, extra_list_id, set_extra_list);

    if (status != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to store extra list for %s %d in file id %d",
	      ex_name_of_object(set_type), set_id,exoid);
      ex_err("ex_put_set",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  /* warn if extra data was sent in for node sets and elem sets */
  if ((set_type == EX_NODE_SET || set_type == EX_ELEM_SET) &&
      set_extra_list != NULL) {
    sprintf(errmsg,
	    "Warning: extra list was ignored for %s %d in file id %d",
	    ex_name_of_object(set_type), set_id, exoid);
    ex_err("ex_put_set",errmsg,EX_MSG);
    return(EX_WARN); 
  }

  return (EX_NOERR);

}
