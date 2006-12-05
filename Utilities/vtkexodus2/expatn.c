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
* expatn - ex_put_attr_names
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     blk_type                block type (edge, face, elem)
*       int     blk_id                  block id
*       char*   names                   ptr to array of attribute names

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
#include <string.h>

/*!
 * writes the attribute names for a block
 */
int ex_put_attr_names(int   exoid,
         int   blk_type,
         int   blk_id,
         char* names[])
{
   int varid = -1, numattrdim = -1, blk_id_ndx;
   long num_attr, start[2], count[2];
   char errmsg[MAX_ERR_LENGTH];
   int i;
   const char* tname;
   
   exerrval = 0; /* clear error code */

   switch (blk_type) {
   case EX_EDGE_BLOCK:
     tname = "edge";
     blk_id_ndx = ex_id_lkup(exoid,VAR_ID_ED_BLK,blk_id);
     break;
   case EX_FACE_BLOCK:
     tname = "face";
     blk_id_ndx = ex_id_lkup(exoid,VAR_ID_FA_BLK,blk_id);
     break;
   case EX_ELEM_BLOCK:
     tname = "element";
     blk_id_ndx = ex_id_lkup(exoid,VAR_ID_EL_BLK,blk_id);
     break;
   default:
     sprintf(errmsg, "Error: Bad block type (%d) specified for file id %d",
       blk_type,exoid);
     ex_err("ex_put_attr_names",errmsg,EX_FATAL);
     return (EX_FATAL);
     break;
   }

  /* Determine index of blk_id in VAR_ID_EL_BLK array */
  if (exerrval != 0) 
  {
    if (exerrval == EX_NULLENTITY)
    {
      sprintf(errmsg,
              "Warning: no attributes allowed for NULL %s block %d in file id %d",
              tname,blk_id,exoid);
      ex_err("ex_put_attr_names",errmsg,EX_MSG);
      return (EX_WARN);              /* no attributes for this block */
    }
    else
    {
      sprintf(errmsg,
             "Error: no %s block id %d in %s array in file id %d",
              tname, blk_id, VAR_ID_EL_BLK, exoid);
      ex_err("ex_put_attr_names",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  /* inquire id's of previously defined dimensions  */
  switch (blk_type) {
  case EX_EDGE_BLOCK:
    numattrdim = ncdimid(exoid, DIM_NUM_ATT_IN_EBLK(blk_id_ndx));
    break;
  case EX_FACE_BLOCK:
    numattrdim = ncdimid(exoid, DIM_NUM_ATT_IN_FBLK(blk_id_ndx));
    break;
  case EX_ELEM_BLOCK:
    numattrdim = ncdimid(exoid, DIM_NUM_ATT_IN_BLK(blk_id_ndx));
    break;
  }
  if (numattrdim == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: number of attributes not defined for %s block %d in file id %d",
            tname,blk_id,exoid);
    ex_err("ex_put_attr_names",errmsg,EX_MSG);
    return (EX_FATAL);              /* number of attributes not defined */
  }

  if (ncdiminq (exoid, numattrdim, (char *) 0, &num_attr) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
         "Error: failed to get number of attributes for block %d in file id %d",
            blk_id,exoid);
    ex_err("ex_put_attr_names",errmsg,exerrval);
    return (EX_FATAL);
  }

  switch (blk_type) {
  case EX_EDGE_BLOCK:
    varid = ncvarid (exoid, VAR_NAME_EATTRIB(blk_id_ndx));
    break;
  case EX_FACE_BLOCK:
    varid = ncvarid (exoid, VAR_NAME_FATTRIB(blk_id_ndx));
    break;
  case EX_ELEM_BLOCK:
    varid = ncvarid (exoid, VAR_NAME_ATTRIB(blk_id_ndx));
    break;
  }
  if (varid == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: failed to locate %s attribute names for block %d in file id %d",
      tname,blk_id, exoid);
    ex_err("ex_put_attr_names",errmsg,exerrval);
    return (EX_FATAL);
     }

  /* write out the attributes  */
  for (i = 0; i < num_attr; i++) {
    start[0] = i;
    start[1] = 0;

    count[0] = 1;
    count[1] = strlen(names[i])+1;

    if (ncvarput (exoid, varid, start, count, (void*) names[i]) == -1) {
      exerrval = ncerr;
      sprintf(errmsg,
        "Error: failed to put attribute namess for block %d in file id %d",
        blk_id,exoid);
      ex_err("ex_put_attr_names",errmsg,exerrval);
      return (EX_FATAL);
    }
  }
  return(EX_NOERR);
}
