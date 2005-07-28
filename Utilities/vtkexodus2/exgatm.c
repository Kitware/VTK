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
* exgatm - get all time values
*
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          James A. Schutt - 8 byte float and standard C definitions
*          Vic Yarberry    - Added headers and error logging
*          
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*
* exit conditions - 
*       float*  time_values             array of simulation time values
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include <string.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads all the time values for history or whole time steps
 */

int ex_get_all_times (int   exoid,
                      void *time_values)

{
   int dimid, varid;
   long start[1], count[1];
   char var_name[MAX_VAR_NAME_LENGTH+1];
   char errmsg[MAX_ERR_LENGTH];

  exerrval = 0;

/* inquire previously defined dimensions  */

  strcpy (var_name, VAR_WHOLE_TIME);
  if (((dimid = ncdimid (exoid, DIM_TIME))) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg, 
           "Error: failed to locate whole time step dimension in file id %d",
            exoid);
    ex_err("ex_get_all_times",errmsg,exerrval);
    return(EX_FATAL);
  }

/* inquire previously defined variable */

  if ((varid = ncvarid (exoid, var_name)) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,"Error: failed to locate time variable %s in file id %d",
            var_name,exoid);
    ex_err("ex_get_all_times",errmsg,exerrval);
    return(EX_FATAL);
  }


/*read time values */

  start[0] = 0;

  if (ncdiminq (exoid, dimid, (char *) 0, count) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to get number of %s time values in file id %d",
            var_name,exoid);
    ex_err("ex_get_all_times",errmsg,exerrval);
    return(EX_FATAL);
  }

  if (ncvarget (exoid, varid, start, count,
             ex_conv_array(exoid,RTN_ADDRESS,time_values,(int)count[0])) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to get %s time values from file id %d",
            var_name,exoid);
    ex_err("ex_get_all_times",errmsg,exerrval);
    return(EX_FATAL);
  }

  ex_conv_array( exoid, READ_CONVERT, time_values, count[0] );

  return (EX_NOERR);
}
