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
* exgfrm - ex_get_coordinate_frames: read coordinate frames
*
* entry conditions - 
*   input parameters:
*       int         exoid               exodus file id
*
* NOTE:
*    cf_ids, pt_coordinates and tags may all be NULL, otherwise, they
*    must have sufficient memory to store the output data. Two calls
*    to this routine are anticipated (one to get nframes, the second
*    to fill the data. i.e.
*      ex_get_coordinate_frames(exoid,&nframes,0,0,0);
*        ... allocate memory ...
*      ex_get_coordinate_frames(exoid,&nframe,id,coords,tags);
*
*
* output conditions -
*       int*         nframes             number of coordinate frames in model
*       const int*   cf_ids             coordinate ids
*                                       dimension int[nframes]
*       const void*  pt_coordinates     pointer to coordinates. 9 values per
*                                       coordinate frame
*                                       dimension float[9*nframes]
*       const char*  tags               character tag for each frame.
*                                        'r' - rectangular
*                                        'c' - cylindrical
*                                        's' - spherical
*                                        dimension char[nframes]
*
* returns -
*      EX_NOERR         for no error
*      EX_FATAL         for fatal errors
* 
*****************************************************************************/

#include <assert.h>
#include "exodusII.h"
#include "exodusII_int.h"

/* -------------------- local defines --------------------------- */
#define PROCNAME "ex_get_coordinate_frames"
/* -------------------- end of local defines -------------------- */
/*!
 * Coordinate frames are stored in the database as a series of three
 * points (defined in the basic cartesian coordinate system). The
 * first of these points describes the origin of the new system.  The
 * second point lies on the 3 axis (or Z axis) of the frame. The third
 * point is in the 1-3 (xz) plane. Each coordinate frame is identified
 * by a unique, integer coordinate ID, and by a character tag
 * indicating whether the frame is rectangular cartesian
 * "R", cylindrical "C, or spherical "S".
 * Because the coordinates are floating point values,
 * the application code must declare the arrays passed to be the
 * appropriate type "float" or "double" to match the
 * compute word size passed in ex_create() or
 * ex_open().
 * \param        exoid    exodus file id
 * \param[in,out] nframes  if 'cf_ids' is NULL, then nframes is returned with the number
 *                        of defined coordinate frames. Else it is the number of coordinate
 *                        frames to read.
 * \param[out] cf_ids The (nframes) coordinate frame Ids. If cf_ids is
 *                    NULL, no data will be returned in this or any other array. Only
 *                    nframes will be modified. Otherwise, space must be allocated to
 *                    store 'nframes' integers before making this call.
 * \param[out] pt_coordinates The (9*nframes) coordinates of the three
 *                            points defining each coordinate axis. The first three values are
 *                            the origin of the first frame. The next three values are the
 *                            coordinates of a point on the 3rd axis of the first frame. The next
 *                            three values are the coordinates of a point in the plane of the 1-3
 *                            axis. The pattern is repeated for each frame. If 'cf_ids'
 *                            is null, no data will be returned in this array. Otherwise, space
 *                            must be allocated for 9*nframes floating point values. The size of
 *                            the allocation depends upon the compute word size.
 * \param[out] tags The (nframes) character tags associated with each
 *                  coordinate frame. If 'cf_ids' is NULL, no data will be
 *                  returned in this array. Otherwise, space must be allocated for
 *                  'nframes' characters.
 */

 int ex_get_coordinate_frames( int exoid, int *nframes, int *cf_ids, void* pt_coordinates,
             char* tags)

{
  int status;
  int dimid; /* ID of the dimension of # frames */
  char errmsg[MAX_ERR_LENGTH];
  int varids;                      /* variable id for the frame ids  */
  size_t start=0;                /* start value for varputs        */
  size_t count;                  /* number vars to put in varput   */

  /* get the dimensions */
  assert( nframes !=NULL );
  status = nc_inq_dimid(exoid, DIM_NUM_CFRAMES, &dimid);
  if (status != NC_NOERR){
    *nframes=0;
    return EX_NOERR;
  }

  nc_inq_dimlen(exoid,dimid,&count);
  *nframes=(int)count;

  if ( count==0 )
    return (EX_NOERR);

  if ( cf_ids )
    if ((status = nc_inq_varid(exoid,VAR_FRAME_IDS, &varids))!= NC_NOERR  ||
  (nc_get_var_int(exoid,varids,cf_ids)!= NC_NOERR)) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to read number coordinate ids from file id %d",
              exoid);
      ex_err(PROCNAME,errmsg,exerrval);
      return (EX_FATAL);
    }

  if ( tags )
    if ( (status = nc_inq_varid(exoid,VAR_FRAME_TAGS,&varids))!= NC_NOERR  ||
         (nc_get_vara_text(exoid,varids,&start,&count,tags) != NC_NOERR)) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to read number coordinate tags from file id %d",
              exoid);
      ex_err(PROCNAME,errmsg,exerrval);
      return (EX_FATAL);
    }

  if (pt_coordinates ){
    if ( (status = nc_inq_varid(exoid,VAR_FRAME_COORDS,&varids))!= NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to read number coordinate tags from file id %d",
              exoid);
      ex_err(PROCNAME,errmsg,exerrval);
      return (EX_FATAL);
    }

    if (ex_comp_ws(exoid) == 4) {
      status = nc_get_var_float(exoid,varids,pt_coordinates);
    } else {
      status = nc_get_var_double(exoid,varids,pt_coordinates);
    }

    if (status != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to read number coordinate tags from file id %d",
              exoid);
      ex_err(PROCNAME,errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  return (EX_NOERR);
}
