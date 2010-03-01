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

/*!
*
* expfrm - ex_put_coordinate_frames: write coordinate frames
*
* \param exoid          exodus file id
* \param nframes        number of coordinate frames in model
* \param cf_ids         coordinate ids
* \param pt_coordinates pointer to coordinates. 9 values per coordinate frame
* \param tags           character tag for each frame. 'r' - rectangular, 'c' - cylindrical, 's' - spherical
*
* returns -
*      EX_NOERR         for no error
*      EX_FATAL         for fatal errors
*      1                number frames < 0
* 
*****************************************************************************/

#include <assert.h>
#include <string.h>
#include "exodusII.h"
#include "exodusII_int.h"

/* -------------------- local defines --------------------------- */
#define PROCNAME "ex_put_coordinate_frames"
/* -------------------- end of local defines -------------------- */
int ex_put_coordinate_frames( int exoid, int nframes, const int cf_ids[], 
                              void* pt_coordinates, const char* tags)
{
  int status;
  int dim, dim9;                   /* dimension id for nframes, nframes*9 */
  char errmsg[MAX_ERR_LENGTH];     /* buffer for error messages      */
  int varcoords;                   /* variable id for the coordinates */
  int varids;                      /* variable id for the frame ids  */
  int vartags;                     /* variable id for the frame tags */
  int i;                           /* general indices */

  if ( exoid < 0 )
    return exoid;

  if ( nframes == 0 ) /* write nothing */
    return (EX_NOERR);

  if ( nframes<0 )
    return 1;

  assert( cf_ids!=0 );
  assert( pt_coordinates !=0 );
  assert( tags != 0 );

  /* make the definitions */
  /* go into define mode. define num_frames, num_frames9 */
  if ((status = nc_redef (exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,"Error: failed to place file id %d into define mode",
            exoid);
    ex_err(PROCNAME,errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((status = nc_def_dim(exoid, DIM_NUM_CFRAMES, nframes, &dim)) != NC_NOERR  ||
      (nc_def_dim(exoid, DIM_NUM_CFRAME9, nframes*9, &dim9) != NC_NOERR)) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to define number of coordinate frames in file id %d",
            exoid);
    ex_err(PROCNAME,errmsg,exerrval);
    goto error_ret;
  }
 
  /* define the variables. coordinates, tags and ids */
  if (nc_def_var (exoid, VAR_FRAME_COORDS,
                  nc_flt_code(exoid), 1, &dim9, &varcoords) != NC_NOERR ||
      (nc_def_var(exoid, VAR_FRAME_IDS,NC_INT, 1, &dim, &varids) != NC_NOERR) ||
      (nc_def_var(exoid, VAR_FRAME_TAGS,NC_CHAR,1,&dim, &vartags) != NC_NOERR) ) {
    exerrval = EX_FATAL;
    sprintf(errmsg,
            "Error:  failed to define coordinate frames in file id %d",
            exoid);
    ex_err(PROCNAME,errmsg,exerrval);
    goto error_ret;         /* exit define mode and return */
  }

  /* leave define mode */
  if ((status = nc_enddef (exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to complete coordinate frame definition in file id %d", 
            exoid);
    ex_err(PROCNAME,errmsg,exerrval);
    return (EX_FATAL);
  }

  /* check variables consistency */
  exerrval = EX_NOERR;
  for (i=0;i<nframes;i++)
    if ( strchr("RrCcSs",tags[i])==0 ){
      sprintf(errmsg,"Warning: Unrecognized coordinate frame tag: '%c'.",
              tags[i]);
      exerrval=2;
      ex_err(PROCNAME,errmsg,exerrval);
    }
  /* could also check vectors. Leave this up to the application */

  /* put the variables into the file */
  if (  nc_put_var_text(exoid, vartags, tags) != NC_NOERR ||
        nc_put_var_int(exoid, varids, cf_ids) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed writing frame data in file id %d",exoid);
    ex_err(PROCNAME,errmsg,exerrval);
    return (EX_FATAL);
  }

  if (ex_comp_ws(exoid) == 4) {
    status = nc_put_var_float(exoid, varcoords, pt_coordinates);
  } else {
    status = nc_put_var_double(exoid, varcoords, pt_coordinates);
  }

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed writing frame data in file id %d",exoid);
    ex_err(PROCNAME,errmsg,exerrval);
    return (EX_FATAL);
  }
  return (EX_NOERR);
 
 error_ret:
  if (nc_enddef (exoid) != NC_NOERR) {    /* exit define mode */
    sprintf(errmsg,
            "Error: failed to complete frame definition for file id %d",
            exoid);
    ex_err(PROCNAME,errmsg,exerrval);
  }
  return (EX_FATAL);


}


