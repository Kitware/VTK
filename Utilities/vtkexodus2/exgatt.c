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
* exgatt - ex_get_attr
*
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          James A. Schutt - 8 byte float and standard C definitions
*          Vic Yarberry    - Added headers and error logging
*
*          
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     obj_type                object type (edge/face/element block)
*       int     obj_id                  object id (edge id, face id, elem id)
*
* exit conditions - 
*       float*  attrib                  array of attributes
*
* revision history - 
*   20061003 - David Thompson - Adapted from ex_get_elem_attr
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the attributes for an edge, face, or element block
 */

int ex_get_attr( int   exoid,
                 int   obj_type,
                 int   obj_id,
                 void* attrib )

{
  int numobjentdim, numattrdim, attrid, obj_id_ndx;
  long num_entries_this_obj, num_attr, start[2], count[2];
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
    sprintf( errmsg, "Error: Invalid object type (%d) specified for file id %d",
      obj_type, exoid );
    ex_err( "ex_get_attr", errmsg, exerrval );
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
              "Warning: no attributes found for NULL block %d in file id %d",
              obj_id,exoid);
      ex_err("ex_get_attr",errmsg,EX_MSG);
      return (EX_WARN);              /* no attributes for this object */
    }
    else
    {
      sprintf(errmsg,
      "Warning: failed to locate %s id %d in %s array in file id %d",
              tname,obj_id,vobjids, exoid);
      ex_err("ex_get_attr",errmsg,exerrval);
      return (EX_WARN);
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
    exerrval = ncerr;
    sprintf(errmsg,
        "Error: failed to locate number of entries for %s %d in file id %d",
            tname, obj_id, exoid);
    ex_err("ex_get_attr",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (ncdiminq (exoid, numobjentdim, (char *) 0, &num_entries_this_obj) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to get number of entries for %s %d in file id %d",
            tname,obj_id,exoid);
    ex_err("ex_get_attr",errmsg,exerrval);
    return (EX_FATAL);
  }


  if ((numattrdim = ncdimid(exoid, dnumobjatt)) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
            "Warning: no attributes found for block %d in file id %d",
            obj_id,exoid);
    ex_err("ex_get_attr",errmsg,EX_MSG);
    return (EX_WARN);              /* no attributes for this object */
  }

  if (ncdiminq (exoid, numattrdim, (char *) 0, &num_attr) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
         "Error: failed to get number of attributes for block %d in file id %d",
            obj_id,exoid);
    ex_err("ex_get_attr",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((attrid = ncvarid (exoid, vattrbname)) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
            "Error: failed to locate attributes for block %d in file id %d",
            obj_id,exoid);
    ex_err("ex_get_attr",errmsg,exerrval);
    return (EX_FATAL);
  }


/* read in the attributes */

  start[0] = 0;
  start[1] = 0;

  count[0] = num_entries_this_obj;
  count[1] = num_attr;

  if (ncvarget (exoid, attrid, start, count,
             ex_conv_array(exoid,RTN_ADDRESS,attrib,
                           (int)num_attr*num_entries_this_obj)) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
            "Error: failed to get attributes for block %d in file id %d",
            obj_id,exoid);
    ex_err("ex_get_attr",errmsg,exerrval);
    return (EX_FATAL);
  }


  ex_conv_array( exoid, READ_CONVERT, attrib, num_attr*num_entries_this_obj );

  return(EX_NOERR);

}
