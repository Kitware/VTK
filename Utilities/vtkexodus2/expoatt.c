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
* expoea - ex_put_one_attr
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     obj_type                object type (edge, face, elem block)
*       int     obj_id                  object id (edge, face, elem block ID)
*       int     attrib_index            index of attribute to write
*       float*  attrib                  array of attributes
*
* exit conditions - 
*
* revision history - 
*   20061003 - David Thompson - Adapted from ex_put_one_attr
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * writes the specified attribute for a block
 */

int ex_put_one_attr( int   exoid,
                     int   obj_type,
                     int   obj_id,
                     int   attrib_index,
                     const void *attrib )
{
  int numobjentdim, numattrdim, attrid, obj_id_ndx;
  long num_entries_this_obj, num_attr;
  size_t start[2], count[2];
  ptrdiff_t stride[2];
  int error;
  char errmsg[MAX_ERR_LENGTH];
  const char* tname;
  const char* vobjids;
  const char* dnumobjent = 0;
  const char* dnumobjatt = 0;
  const char* vattrbname = 0;

  switch (obj_type) {
  case EX_EDGE_BLOCK:
    tname = "edge block";
    vobjids = VAR_ID_ED_BLK;
    break;
  case EX_FACE_BLOCK:
    tname = "face block";
    vobjids = VAR_ID_FA_BLK;
    break;
  case EX_ELEM_BLOCK:
    tname = "element block";
    vobjids = VAR_ID_EL_BLK;
    break;
  default:
    exerrval = EX_BADPARAM;
    sprintf(errmsg, "Error: Bad block type (%d) specified for file id %d",
      obj_type, exoid );
    ex_err("ex_put_attr",errmsg,exerrval);
    return (EX_FATAL);
  }

  exerrval = 0; /* clear error code */

  /* Determine index of obj_id in vobjids array */
  obj_id_ndx = ex_id_lkup(exoid,vobjids,obj_id);
  if (exerrval != 0) 
    {
      if (exerrval == EX_NULLENTITY)
        {
          sprintf(errmsg,
                  "Warning: no attributes allowed for NULL %s %d in file id %d",
                  tname,obj_id,exoid);
          ex_err("ex_put_one_attr",errmsg,EX_MSG);
          return (EX_WARN);              /* no attributes for this element block */
        }
      else
        {
          sprintf(errmsg,
                  "Error: no %s id %d in %s array in file id %d",
                  tname, obj_id, vobjids, exoid);
          ex_err("ex_put_one_attr",errmsg,exerrval);
          return (EX_FATAL);
        }
    }

  switch (obj_type) {
  case EX_EDGE_BLOCK:
    dnumobjent = DIM_NUM_ED_IN_EBLK(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_EBLK(obj_id_ndx);
    vattrbname = VAR_EATTRIB(obj_id_ndx);
    break;
  case EX_FACE_BLOCK:
    dnumobjent = DIM_NUM_FA_IN_FBLK(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_FBLK(obj_id_ndx);
    vattrbname = VAR_FATTRIB(obj_id_ndx);
    break;
  case EX_ELEM_BLOCK:
    dnumobjent = DIM_NUM_EL_IN_BLK(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_BLK(obj_id_ndx);
    vattrbname = VAR_ATTRIB(obj_id_ndx);
    break;
  }

  /* inquire id's of previously defined dimensions  */
  if ((numobjentdim = ncdimid (exoid, dnumobjent)) == -1)
    {
      if (ncerr == NC_EBADDIM)
        {
          exerrval = ncerr;
          sprintf(errmsg,
                  "Error: no %s with id %d in file id %d",
                  tname, obj_id, exoid);
          ex_err("ex_put_one_attr",errmsg,exerrval);
          return (EX_FATAL);
        }
      else
        {
          exerrval = ncerr;
          sprintf(errmsg,
                  "Error: failed to locate number of entries for %s %d in file id %d",
                  tname, obj_id, exoid);
          ex_err("ex_put_one_attr",errmsg,exerrval);
          return (EX_FATAL);
        }
    }


  if (ncdiminq (exoid, numobjentdim, (char *) 0, &num_entries_this_obj) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to get number of entries for %s %d in file id %d",
              tname,obj_id,exoid);
      ex_err("ex_put_one_attr",errmsg,exerrval);
      return (EX_FATAL);
    }


  if ((numattrdim = ncdimid(exoid, dnumobjatt)) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: number of attributes not defined for %s %d in file id %d",
              tname,obj_id,exoid);
      ex_err("ex_put_one_attr",errmsg,EX_MSG);
      return (EX_FATAL);              /* number of attributes not defined */
    }

  if (ncdiminq (exoid, numattrdim, (char *) 0, &num_attr) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to get number of attributes for block %d in file id %d",
              obj_id,exoid);
      ex_err("ex_put_one_attr",errmsg,exerrval);
      return (EX_FATAL);
    }

  if (attrib_index < 1 || attrib_index > num_attr) {
    exerrval = EX_FATAL;
    sprintf(errmsg,
            "Error: Invalid attribute index specified: %d.  Valid range is 1 to %ld for %s %d in file id %d",
            attrib_index, num_attr, tname, obj_id, exoid);
    ex_err("ex_put_one_attr",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((attrid = ncvarid (exoid, vattrbname)) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to locate attribute variable for %s %d in file id %d",
              tname,obj_id,exoid);
      ex_err("ex_put_one_attr",errmsg,exerrval);
      return (EX_FATAL);
    }


  /* write out the attributes  */

  start[0] = 0;
  start[1] = attrib_index-1;

  count[0] = num_entries_this_obj;
  count[1] = 1;

  stride[0] = 1;
  stride[1] = num_attr;
  
  if (nc_flt_code(exoid) == NC_FLOAT) {
    error = nc_put_vars_float(exoid, attrid, start, count, stride,
                              ex_conv_array(exoid,WRITE_CONVERT,attrib,
                                            (int)num_attr*num_entries_this_obj));
  } else {
    error = nc_put_vars_double(exoid, attrid, start, count, stride,
                               ex_conv_array(exoid,WRITE_CONVERT,attrib,
                                             (int)num_attr*num_entries_this_obj));
  }
  if (error == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
            "Error: failed to put attribute %d for %s %d in file id %d",
            attrib_index, tname, obj_id, exoid);
    ex_err("ex_put_one_attr",errmsg,exerrval);
    return (EX_FATAL);
  }


  return(EX_NOERR);

}
