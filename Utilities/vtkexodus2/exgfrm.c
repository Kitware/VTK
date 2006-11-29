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
* author - Sandia National Laboratories
*          Garth Reese  - created this function. Nov 2002
*
* environment - UNIX
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
int ex_get_coordinate_frames( int exoid, int *nframes, int *cf_ids,
                              void* pt_coordinates, char* tags)
{
  int dimid;                       /* ID of the dimension of # frames */
  char errmsg[MAX_ERR_LENGTH];
  int exerrval;                    /* returned error value           */
  int varids;                      /* variable id for the frame ids  */
  long int start=0;                /* start value for varputs        */
  long int count;                  /* number vars to put in varput   */
  long int count9;                 /* ditto, but for coordinates     */
  void* pt_c=0;                    /* pointer to converted array     */

  /* get the dimensions */
  assert( nframes !=NULL );
  dimid = ncdimid(exoid,NUM_CFRAMES);
  if ( dimid<0 ){
    *nframes=0;
    return EX_NOERR;
  }
  ncdiminq(exoid,dimid,(char*)0,&count);
  *nframes=(int)count;
  count9=count*9;

  if ( count==0 )
    return (EX_NOERR);

  if ( cf_ids )
    if ( (varids=ncvarid(exoid,FRAME_IDS))==-1  ||
         ncvarget(exoid,varids,&start,&count,cf_ids)== -1 ) {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to read number coordinate ids from file id %d",
              exoid);
      ex_err((char*)PROCNAME,errmsg,exerrval);
      return (EX_FATAL);
    }

  if ( tags )
    if ( (varids=ncvarid(exoid,FRAME_TAGS))==-1  ||
         ncvarget(exoid,varids,&start,&count,tags)== -1 ) {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to read number coordinate tags from file id %d",
              exoid);
      ex_err((char*)PROCNAME,errmsg,exerrval);
      return (EX_FATAL);
    }

  if (pt_coordinates ){
    pt_c=ex_conv_array(exoid,RTN_ADDRESS,pt_coordinates,count9);
    assert(pt_c!=0);
    if ( (varids=ncvarid(exoid,FRAME_COORDS))==-1  ||
         ncvarget(exoid,varids,&start,&count9,pt_c)== -1 ) {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to read number coordinate tags from file id %d",
              exoid);
      ex_err((char*)PROCNAME,errmsg,exerrval);
      return (EX_FATAL);
    }
    else {
      pt_c=ex_conv_array( exoid, READ_CONVERT,pt_coordinates,count9);
      assert(pt_c==0);
    }
  }

  return (EX_NOERR);
}
