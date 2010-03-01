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
* exgcon - ex_get_coord_names
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*
* exit conditions - 
*       char*   coord_names[]           ptr array of coord names
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * reads the names (#MAX_STR_LENGTH characters in length) of the
 * coordinate arrays from the database. Memory must be allocated for
 * the character strings before this function is invoked.
 * \param      exoid  exodus file id
 * \param[out] coord_names Returned pointer to a vector containing
 *             'num_dim' names of the nodal coordinate arrays.
 */

int ex_get_coord_names (int    exoid,
                        char **coord_names)
{
  int status;
  size_t i;
  int j, ndimdim, varid;
  size_t num_dim, start[2];
  char *ptr;
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0;

  /* inquire previously defined dimensions and variables  */

  if ((status = nc_inq_dimid(exoid, DIM_NUM_DIM, &ndimdim)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to locate number of dimensions in file id %d",
            exoid);
    ex_err("ex_get_coord_names",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((status = nc_inq_dimlen(exoid, ndimdim, &num_dim)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to get number of dimensions in file id %d",
            exoid);
    ex_err("ex_get_coord_names",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((status = nc_inq_varid(exoid, VAR_NAME_COOR, &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Warning: failed to locate coordinate names in file id %d",
            exoid);
    ex_err("ex_get_coord_names",errmsg,exerrval);
    return (EX_WARN);
  }


  /* read the coordinate names */
  for (i=0; i<num_dim; i++) {
    start[0] = i;
    start[1] = 0;

    j = 0;
    ptr = coord_names[i];

    if ((status = nc_get_var1_text(exoid, varid, start, ptr)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to get coordinate names in file id %d", exoid);
      ex_err("ex_get_coord_names",errmsg,exerrval);
      return (EX_FATAL);
    }

 
    while ((*ptr++ != '\0') && (j < MAX_STR_LENGTH)) {
      start[1] = ++j;
      if ((status = nc_get_var1_text(exoid, varid, start, ptr)) != NC_NOERR) {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to get coordinate names in file id %d", exoid);
        ex_err("ex_get_coord_names",errmsg,exerrval);
        return (EX_FATAL);
      }
    }
    --ptr;
    if (ptr > coord_names[i]) {
      /*    get rid of trailing blanks */
      while (--ptr >= coord_names[i] && *ptr == ' ');
    }
    *(++ptr) = '\0';
  }
  return (EX_NOERR);
}
