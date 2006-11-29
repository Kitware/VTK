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
* exppem - ex_put_partial_elem_map
*
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     map_id                  element map id
*       int     ent_start               first entry in map
*       int     ent_count               number of entries in map
*       int     *elem_map               element map
*
* exit conditions - 
*
* revision history - 
*
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"
#include <stdlib.h> /* for free() */

/*!
 * writes an element map; this is a vector of integers of length number
 * of elements
 */
int ex_put_partial_elem_map (int exoid,
           int map_id,
           int ent_start,
           int ent_count, 
           const int *elem_map)
{
  int dimid, varid, iresult, map_ndx, map_exists;
  long start[1]; 
  nclong ldum, *lptr;
  long num_elem_maps, num_elem, count[1];
  int cur_num_elem_maps;
  char *cdum;
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */
  map_exists = 0;
  cdum = 0;


  /* Make sure the file contains elements */
  if ((dimid = (ncdimid (exoid, DIM_NUM_ELEM))) == -1 )
    {
      return (EX_NOERR);
    }

  /* first check if any element maps are specified */

  if ((dimid = (ncdimid (exoid, DIM_NUM_EM))) == -1 )
    {
      exerrval = ncerr;
      sprintf(errmsg,
        "Error: no element maps specified in file id %d",
        exoid);
      ex_err("ex_put_partial_elem_map",errmsg,exerrval);
      return (EX_FATAL);
    }

  /* Check for duplicate element map id entry */
  map_ndx = ex_id_lkup(exoid,VAR_EM_PROP(1),map_id); 
  if (exerrval == EX_LOOKUPFAIL) {   /* did not find the element map id */
    map_exists = 0; /* Map is being defined */
    map_ndx    = -1;
  } else {
    map_exists = 1; /* A portion of this map has already been written */
  }

  if (!map_exists) {
    /* Get number of element maps initialized for this file */
    if ((ncdiminq (exoid,dimid,cdum,&num_elem_maps)) == -1)
      {
  exerrval = ncerr;
  sprintf(errmsg,
    "Error: failed to get number of element maps in file id %d",
    exoid);
  ex_err("ex_put_partial_elem_map",errmsg,exerrval);
  return (EX_FATAL);
      }

    /* Keep track of the total number of element maps defined using a
       counter stored in a linked list keyed by exoid.  NOTE:
       ex_get_file_item is used to find the number of element maps for a
       specific file and returns that value.
    */
    cur_num_elem_maps = ex_get_file_item(exoid, &em_ctr_list );
    if (cur_num_elem_maps >= num_elem_maps)
      {
  exerrval = EX_FATAL;
  sprintf(errmsg,
    "Error: exceeded number of element maps (%ld) specified in file id %d",
    num_elem_maps,exoid);
  ex_err("ex_put_partial_elem_map",errmsg,exerrval);
  return (EX_FATAL);
      }
    
    /*   NOTE: ex_inc_file_item  is used to find the number of element maps
   for a specific file and returns that value incremented. */
    
    cur_num_elem_maps = ex_inc_file_item(exoid, &em_ctr_list );
  } else {
    cur_num_elem_maps = map_ndx-1;
  }

  /* determine number of elements */
  if ((dimid = (ncdimid (exoid, DIM_NUM_ELEM))) == -1 )
    {
      exerrval = ncerr;
      sprintf(errmsg,
        "Error: couldn't determine number of elements in file id %d",
        exoid);
      ex_err("ex_put_partial_elem_map",errmsg,exerrval);
      return (EX_FATAL);
    }

  if (ncdiminq (exoid, dimid, (char *) 0, &num_elem) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
        "Error: failed to get number of elements in file id %d",
        exoid);
      ex_err("ex_put_partial_elem_map",errmsg,exerrval);
      return (EX_FATAL);
    }

  /* Check input parameters for a valid range of numbers */
  if (ent_start <= 0 || ent_start > num_elem) {
    exerrval = EX_FATAL;
    sprintf(errmsg,
      "Error: start count is invalid in file id %d",
      exoid);
    ex_err("ex_put_partial_elem_map",errmsg,exerrval);
    return (EX_FATAL);
  }
  if (ent_count < 0) {
    exerrval = EX_FATAL;
    sprintf(errmsg,
      "Error: Invalid count value in file id %d",
      exoid);
    ex_err("ex_put_partial_elem_map",errmsg,exerrval);
    return (EX_FATAL);
  }
  if (ent_start+ent_count-1 > num_elem) {
    exerrval = EX_FATAL;
    sprintf(errmsg,
      "Error: start+count-1 is larger than element count in file id %d",
      exoid);
    ex_err("ex_put_partial_elem_map",errmsg,exerrval);
    return (EX_FATAL);
  }
  

  /* write out information to previously defined variable */

  /* first get id of variable */
  if ((varid = ncvarid (exoid, VAR_EM_PROP(1))) == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: failed to locate element map ids in file id %d",
      exoid);
    ex_err("ex_put_partial_elem_map",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* then, write out element map id */
  if (!map_exists) {
    start[0] = cur_num_elem_maps;

    ldum = (nclong)map_id;
    if (ncvarput1 (exoid, varid, start, &ldum) == -1)
      {
  exerrval = ncerr;
  sprintf(errmsg,
    "Error: failed to store element map id %d in file id %d",
    map_id,exoid);
  ex_err("ex_put_partial_elem_map",errmsg,exerrval);
  return (EX_FATAL);
      }
  }
  
  /* locate variable array in which to store the element map */
  if ((varid = 
       ncvarid(exoid,VAR_ELEM_MAP(cur_num_elem_maps+1))) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to locate element map %d in file id %d",
        map_id,exoid);
      ex_err("ex_put_partial_elem_map",errmsg,exerrval);
      return (EX_FATAL);
    }

  /* write out the element map  */

  /* this contortion is necessary because netCDF is expecting nclongs;
     fortunately it's necessary only when ints and nclongs aren't the
     same size */

  start[0] = ent_start-1;
  count[0] = ent_count;

  if (sizeof(int) == sizeof(nclong)) {
    iresult = ncvarput (exoid, varid, start, count, elem_map);
  } else {
    lptr = itol (elem_map, (int)ent_count);
    iresult = ncvarput (exoid, varid, start, count, lptr);
    free(lptr);
  }

  if (iresult == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
        "Error: failed to store element map in file id %d",
        exoid);
      ex_err("ex_put_partial_elem_map",errmsg,exerrval);
      return (EX_FATAL);
    }

  return (EX_NOERR);
}
