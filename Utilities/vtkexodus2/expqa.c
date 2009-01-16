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
* expqa = ex_put_qa
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     num_qa_records          number of qa records to be written
*       char*   qa_record[][4])         qa record array
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
 * writes the QA records to the database
 * \param  exoid                   exodus file id
 * \param  num_qa_records          number of qa records to be written
 * \param *qa_record               qa record array
 */

int ex_put_qa (int   exoid,
               int   num_qa_records,
               char* qa_record[][4])
{
  int status;
  int i, j, strdim, num_qa_dim, varid, n4dim;
  int dims[3];
  size_t start[3], count[3];
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  /* only do this if there are records */

  if (num_qa_records > 0) {
    /*   inquire previously defined dimensions  */

    if ((status =  nc_inq_dimid(exoid, DIM_STR, &strdim)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to locate string length in file id %d", exoid);
      ex_err("ex_put_qa",errmsg,exerrval);
      return (EX_FATAL);
    }

    if ((status = nc_inq_dimid(exoid, DIM_N4, &n4dim)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to locate record length in file id %d", exoid);
      ex_err("ex_put_qa",errmsg,exerrval);
      return (EX_FATAL);
    }

    /*   put file into define mode  */
    if ((status = nc_redef(exoid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to put file id %d into define mode", exoid);
      ex_err("ex_put_qa",errmsg,exerrval);
      return (EX_FATAL);
    }


    /*   define dimensions */
    if ((status = nc_def_dim(exoid,DIM_NUM_QA,num_qa_records, &num_qa_dim)) != NC_NOERR) {
      if (status == NC_ENAMEINUSE) {     /* duplicate entry? */
	exerrval = status;
	sprintf(errmsg,
		"Error: qa records already exist in file id %d", exoid);
	ex_err("ex_put_qa",errmsg,exerrval);
      } else {
	exerrval = status;
	sprintf(errmsg,
		"Error: failed to define qa record array size in file id %d", exoid);
	ex_err("ex_put_qa",errmsg,exerrval);
      }

      goto error_ret;         /* exit define mode and return */
    }

    /*   define variable  */
    dims[0] = num_qa_dim;
    dims[1] = n4dim;
    dims[2] = strdim;

    if ((status = nc_def_var(exoid, VAR_QA_TITLE, NC_CHAR, 3, dims, &varid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to define qa record array in file id %d", exoid);
      ex_err("ex_put_qa",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

    /*   leave define mode  */
    if ((status = nc_enddef (exoid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to complete definition in file id %d", exoid);
      ex_err("ex_put_qa",errmsg,exerrval);
      return (EX_FATAL);
    }


    /*   write out QA records */

    for (i=0; i<num_qa_records; i++) {
      for (j=0; j<4; j++) {
	start[0] = i;
	start[1] = j;
	start[2] = 0;

	count[0] = 1;
	count[1] = 1;
	count[2] = strlen(qa_record[i][j]) + 1;

	if ((status = nc_put_vara_text(exoid, varid, start, count, qa_record[i][j])) != NC_NOERR) {
	  exerrval = status;
	  sprintf(errmsg,
		  "Error: failed to store qa record in file id %d", exoid);
	  ex_err("ex_put_qa",errmsg,exerrval);
	  return (EX_FATAL);
	}
      }
    }
  }
  return (EX_NOERR);

  /* Fatal error: exit definition mode and return */
 error_ret:
  if (nc_enddef (exoid) != NC_NOERR) {    /* exit define mode */
    sprintf(errmsg,
	    "Error: failed to complete definition for file id %d",
	    exoid);
    ex_err("ex_put_qa",errmsg,exerrval);
  }
  return (EX_FATAL);
}
