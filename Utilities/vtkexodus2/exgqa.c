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
* exgqa - ex_get_qa
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*
* exit conditions - 
*       char*   qa_record[8][4]         ptr to qa record ptr array
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the QA records from the database
 */

int ex_get_qa (int exoid,
               char *qa_record[][4])
{
  int status;
  int j, k, dimid, varid;
  size_t i;
  size_t num_qa_records, start[3];

  char *ptr;
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  /* inquire previously defined dimensions and variables  */
  if ((status = nc_inq_dimid(exoid, DIM_NUM_QA, &dimid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Warning: no qa records stored in file id %d", 
            exoid);
    ex_err("ex_get_qa",errmsg,exerrval);
    return (EX_WARN);
  }

  if ((status = nc_inq_dimlen(exoid, dimid, &num_qa_records)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to get number of qa records in file id %d",
            exoid);
    ex_err("ex_get_qa",errmsg,exerrval);
    return (EX_FATAL);
  }


  /* do this only if there are any QA records */
  if (num_qa_records > 0) {
    if ((status = nc_inq_varid(exoid, VAR_QA_TITLE, &varid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to locate qa record data in file id %d", exoid);
      ex_err("ex_get_qa",errmsg,exerrval);
      return (EX_FATAL);
    }


    /* read the QA records */
    for (i=0; i<num_qa_records; i++) {
      for (j=0; j<4; j++) {
        start[0] = i;
        start[1] = j;
        start[2] = 0;

        k = 0;
        ptr = qa_record[i][j];

        if ((status = nc_get_var1_text(exoid, varid, start, ptr)) != NC_NOERR) {
          exerrval = status;
          sprintf(errmsg,
                  "Error: failed to get qa record data in file id %d", exoid);
          ex_err("ex_get_qa",errmsg,exerrval);
          return (EX_FATAL);
        }


        while ((*(ptr++) != '\0') && (k < MAX_STR_LENGTH)) {
          start[2] = ++k;
          if ((status = nc_get_var1_text(exoid, varid, start, ptr)) != NC_NOERR) {
            exerrval = status;
            sprintf(errmsg,
                    "Error: failed to get qa record data in file id %d", exoid);
            ex_err("ex_get_qa",errmsg,exerrval);
            return (EX_FATAL);
          }
        }

        /* remove trailing blanks */

        if(start[2] != 0) {
          --ptr;
          while ( --ptr >= qa_record[i][j] && *ptr == ' ' );
          *(++ptr) = '\0';
        }
      }
    }
  }
  return (EX_NOERR);
}
