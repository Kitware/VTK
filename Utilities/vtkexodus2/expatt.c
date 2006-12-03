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
* expatt - ex_put_attr
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     blk_type                block type
*       int     blk_id                  block id
*       float*  attrib                  array of attributes
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
 * writes the attributes for an edge/face/element block
 */

int ex_put_attr (int   exoid,
                      int   blk_type,
                      int   blk_id,
                      const void *attrib)
{
  int numentriesbdim = -1, numattrdim = -1, attrid = -1, blk_id_ndx;
  long num_entries_this_blk, num_attr, start[2], count[2];
  char errmsg[MAX_ERR_LENGTH];
  const char* tname;
  const char* vobjids;

  switch (blk_type) {
  case EX_EDGE_BLOCK:
    tname = "edge";
    vobjids = VAR_ID_ED_BLK;
    break;
  case EX_FACE_BLOCK:
    tname = "face";
    vobjids = VAR_ID_FA_BLK;
    break;
  case EX_ELEM_BLOCK:
    tname = "element";
    vobjids = VAR_ID_EL_BLK;
    break;
  default:
    exerrval = EX_BADPARAM;
    sprintf(errmsg, "Error: Bad block type (%d) specified for file id %d",
      blk_type, exoid );
    ex_err("ex_put_attr",errmsg,exerrval);
    return (EX_FATAL);
  }

  exerrval = 0; /* clear error code */

  /* Determine index of blk_id in VAR_ID_EL_BLK array */
  blk_id_ndx = ex_id_lkup(exoid,vobjids,blk_id);
  if (exerrval != 0) 
  {
    if (exerrval == EX_NULLENTITY)
    {
      sprintf(errmsg,
            "Warning: no attributes allowed for NULL %s block %d in file id %d",
              tname,blk_id,exoid);
      ex_err("ex_put_attr",errmsg,EX_MSG);
      return (EX_WARN);              /* no attributes for this block */
    }
    else
    {
      sprintf(errmsg,
             "Error: no %s block id %d in %s array in file id %d",
              tname, blk_id, vobjids, exoid);
      ex_err("ex_put_attr",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

/* inquire id's of previously defined dimensions  */

  switch (blk_type) {
  case EX_EDGE_BLOCK:
    numentriesbdim = ncdimid (exoid, DIM_NUM_ED_IN_EBLK(blk_id_ndx));
    break;
  case EX_FACE_BLOCK:
    numentriesbdim = ncdimid (exoid, DIM_NUM_FA_IN_FBLK(blk_id_ndx));
    break;
  case EX_ELEM_BLOCK:
    numentriesbdim = ncdimid (exoid, DIM_NUM_EL_IN_BLK(blk_id_ndx));
    break;
  }
  if (numentriesbdim == -1)
  {
    if (ncerr == NC_EBADDIM)
    {
      exerrval = ncerr;
      sprintf(errmsg,
         "Error: no %s block with id %d in file id %d",
             tname, blk_id, exoid);
      ex_err("ex_put_attr",errmsg,exerrval);
      return (EX_FATAL);
    }
    else
    {
      exerrval = ncerr;
      sprintf(errmsg,
        "Error: failed to locate number of %ss for block %d in file id %d",
             tname, blk_id, exoid);
      ex_err("ex_put_attr",errmsg,exerrval);
      return (EX_FATAL);
    }
  }


  if (ncdiminq (exoid, numentriesbdim, (char *) 0, &num_entries_this_blk) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to get number of %ss for block %d in file id %d",
            tname, blk_id,exoid);
    ex_err("ex_put_attr",errmsg,exerrval);
    return (EX_FATAL);
  }


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
            tname, blk_id,exoid);
    ex_err("ex_put_attr",errmsg,EX_MSG);
    return (EX_FATAL);              /* number of attributes not defined */
  }

  if (ncdiminq (exoid, numattrdim, (char *) 0, &num_attr) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: failed to get number of attributes for %s block %d in file id %d",
            tname, blk_id,exoid);
    ex_err("ex_put_attr",errmsg,exerrval);
    return (EX_FATAL);
  }

  switch (blk_type) {
  case EX_EDGE_BLOCK:
    attrid = ncvarid (exoid, VAR_EATTRIB(blk_id_ndx));
    break;
  case EX_FACE_BLOCK:
    attrid = ncvarid (exoid, VAR_FATTRIB(blk_id_ndx));
    break;
  case EX_ELEM_BLOCK:
    attrid = ncvarid (exoid, VAR_ATTRIB(blk_id_ndx));
    break;
  }
  if (attrid == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
     "Error: failed to locate attribute variable for %s block %d in file id %d",
            tname,blk_id,exoid);
    ex_err("ex_put_attr",errmsg,exerrval);
    return (EX_FATAL);
  }


/* write out the attributes  */

  start[0] = 0;
  start[1] = 0;

  count[0] = num_entries_this_blk;
  count[1] = num_attr;

  if (ncvarput (exoid, attrid, start, count,
                ex_conv_array(exoid,WRITE_CONVERT,attrib,
                (int) num_attr * num_entries_this_blk)) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
            "Error: failed to put attributes for %s block %d in file id %d",
            tname,blk_id,exoid);
    ex_err("ex_put_attr",errmsg,exerrval);
    return (EX_FATAL);
  }


  return(EX_NOERR);

}
