/*
 * Copyright (c) 1994 Sandia Corporation. Under the terms of Contract
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
* exgeat - ex_get_elem_attr
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
*       int     elem_blk_id             element block id
*
* exit conditions - 
*       float*  attrib                  array of attributes
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the attributes for an element block
 */

int ex_get_elem_attr (int   exoid,
                      int   elem_blk_id,
                      void *attrib)

{
  int numelbdim, numattrdim, attrid, elem_blk_id_ndx;
  long num_elem_this_blk, num_attr, start[2], count[2];
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  /* Determine index of elem_blk_id in VAR_ID_EL_BLK array */
  elem_blk_id_ndx = ex_id_lkup(exoid,VAR_ID_EL_BLK,elem_blk_id);
  if (exerrval != 0) 
  {
    if (exerrval == EX_NULLENTITY)
    {
      sprintf(errmsg,
              "Warning: no attributes found for NULL block %d in file id %d",
              elem_blk_id,exoid);
      ex_err("ex_get_elem_attr",errmsg,EX_MSG);
      return (EX_WARN);              /* no attributes for this element block */
    }
    else
    {
      sprintf(errmsg,
      "Warning: failed to locate element block id %d in %s array in file id %d",
              elem_blk_id,VAR_ID_EL_BLK, exoid);
      ex_err("ex_get_elem_attr",errmsg,exerrval);
      return (EX_WARN);
    }
  }


/* inquire id's of previously defined dimensions  */

  if ((numelbdim = ncdimid (exoid, DIM_NUM_EL_IN_BLK(elem_blk_id_ndx))) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
        "Error: failed to locate number of elements for block %d in file id %d",
            elem_blk_id, exoid);
    ex_err("ex_get_elem_attr",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (ncdiminq (exoid, numelbdim, (char *) 0, &num_elem_this_blk) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to get number of elements for block %d in file id %d",
            elem_blk_id,exoid);
    ex_err("ex_get_elem_attr",errmsg,exerrval);
    return (EX_FATAL);
  }


  if ((numattrdim = ncdimid(exoid, DIM_NUM_ATT_IN_BLK(elem_blk_id_ndx))) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
            "Warning: no attributes found for block %d in file id %d",
            elem_blk_id,exoid);
    ex_err("ex_get_elem_attr",errmsg,EX_MSG);
    return (EX_WARN);              /* no attributes for this element block */
  }

  if (ncdiminq (exoid, numattrdim, (char *) 0, &num_attr) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
         "Error: failed to get number of attributes for block %d in file id %d",
            elem_blk_id,exoid);
    ex_err("ex_get_elem_attr",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((attrid = ncvarid (exoid, VAR_ATTRIB(elem_blk_id_ndx))) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
            "Error: failed to locate attributes for block %d in file id %d",
            elem_blk_id,exoid);
    ex_err("ex_get_elem_attr",errmsg,exerrval);
    return (EX_FATAL);
  }


/* read in the attributes */

  start[0] = 0;
  start[1] = 0;

  count[0] = num_elem_this_blk;
  count[1] = num_attr;

  if (ncvarget (exoid, attrid, start, count,
             ex_conv_array(exoid,RTN_ADDRESS,attrib,
                           (int)num_attr*num_elem_this_blk)) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
            "Error: failed to get attributes for block %d in file id %d",
            elem_blk_id,exoid);
    ex_err("ex_get_elem_attr",errmsg,exerrval);
    return (EX_FATAL);
  }


  ex_conv_array( exoid, READ_CONVERT, attrib, num_attr*num_elem_this_blk );

  return(EX_NOERR);

}
