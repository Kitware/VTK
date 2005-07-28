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
* exgevid - ex_get_elem_varid
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     num_elem_blk            number of element blocks
*       int     num_elem_var            number of element variables
*
* exit conditions - 
*       int*    elem_varid              element variable truth table array
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the EXODUS II element variable truth table from the database
 */

int ex_get_elem_varid (int  exoid,
                       int *varid)
{
  int  dimid, evarid, i, j;
  long num_elem_blk, num_elem_var;
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */
 
  /* inquire id's of previously defined dimensions  */
  if ((dimid = ncdimid (exoid, DIM_NUM_EL_BLK)) == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
            "Error: failed to locate number of element blocks in file id %d",
            exoid);
    ex_err("ex_get_elem_varid",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (ncdiminq (exoid, dimid, (char *) 0, &num_elem_blk) == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
            "Error: failed to get number of element blocks in file id %d",
            exoid);
    ex_err("ex_get_elem_varid",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((dimid = ncdimid (exoid, DIM_NUM_ELE_VAR)) == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
            "Warning: no element variables stored in file id %d",
            exoid);
    ex_err("ex_get_elem_varid",errmsg,exerrval);
    return (EX_WARN);
  }

  if (ncdiminq (exoid, dimid, (char *) 0, &num_elem_var) == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
            "Error: failed to get number of element variables in file id %d",
            exoid);
    ex_err("ex_get_elem_varid",errmsg,exerrval);
    return (EX_FATAL);
  }


  /* since truth table isn't stored in the data file, derive it dynamically */
  for (j=0; j<num_elem_blk; j++) {
    for (i=0; i<num_elem_var; i++) {
      /* NOTE: names are 1-based */
      if ((evarid = ncvarid (exoid, VAR_ELEM_VAR(i+1,j+1))) == -1)
        /* variable doesn't exist; put a 0 in the varid table */
        varid[j*num_elem_var+i] = 0;
      else
        /* variable exists; put varid in the table */
        varid[j*num_elem_var+i] = evarid;
    }
  }
  return (EX_NOERR);
}
