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
/*
 *  Id
 *
 *****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*
 *  reads the element block ids from the database
 */

int ex_get_ids (int  exoid,
    int obj_type, 
    int *ids)
{
  int dimid, varid, iresult;
  long numobj, start[1], count[1]; 
  nclong *longs;
  char errmsg[MAX_ERR_LENGTH];

  const char* dimnumobj;
  const char* varidobj;
  const char* tname;

  exerrval = 0; /* clear error code */

  switch (obj_type) {
  case EX_EDGE_BLOCK:
    tname = "edge block";
    dimnumobj = DIM_NUM_ED_BLK;
    varidobj = VAR_ID_ED_BLK;
    break;
  case EX_FACE_BLOCK:
    tname = "face block";
    dimnumobj = DIM_NUM_FA_BLK;
    varidobj = VAR_ID_FA_BLK;
    break;
  case EX_ELEM_BLOCK:
    tname = "element block";
    dimnumobj = DIM_NUM_EL_BLK;
    varidobj = VAR_ID_EL_BLK;
    break;
  case EX_NODE_SET:
    tname = "node set";
    dimnumobj = DIM_NUM_NS;
    varidobj = VAR_NS_IDS;
    break;
  case EX_EDGE_SET:
    tname = "edge set";
    dimnumobj = DIM_NUM_ES;
    varidobj = VAR_ES_IDS;
    break;
  case EX_FACE_SET:
    tname = "face set";
    dimnumobj = DIM_NUM_FS;
    varidobj = VAR_FS_IDS;
    break;
  case EX_SIDE_SET:
    tname = "side set";
    dimnumobj = DIM_NUM_SS;
    varidobj = VAR_SS_IDS;
    break;
  case EX_ELEM_SET:
    tname = "element set";
    dimnumobj = DIM_NUM_ELS;
    varidobj = VAR_ELS_IDS;
    break;
  case EX_NODE_MAP:
    tname = "node map";
    dimnumobj = DIM_NUM_NM;
    varidobj = VAR_NM_PROP(1);
    break;
  case EX_EDGE_MAP:
    tname = "edge map";
    dimnumobj = DIM_NUM_EDM;
    varidobj = VAR_EDM_PROP(1);
    break;
  case EX_FACE_MAP:
    tname = "face map";
    dimnumobj = DIM_NUM_FAM;
    varidobj = VAR_FAM_PROP(1);
    break;
  case EX_ELEM_MAP:
    tname = "element map";
    dimnumobj = DIM_NUM_EM;
    varidobj = VAR_EM_PROP(1);
    break;
  default:/* invalid variable type */
    exerrval = EX_BADPARAM;
    sprintf(errmsg, "Error: Invalid type specified in file id %d", exoid);
    ex_err("ex_get_ids",errmsg,exerrval);
    return(EX_FATAL);
  }

  /* inquire id's of previously defined dimensions and variables  */

  if ((dimid = ncdimid (exoid, dimnumobj)) == -1)
    {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: failed to locate dimension %s in file id %d",
      dimnumobj,exoid);
    ex_err("ex_get_ids",errmsg,exerrval);
    return (EX_FATAL);
    }

  if (ncdiminq (exoid, dimid, (char*)0, &numobj) == -1)
    {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: failed to return number of %ss in file id %d",
      tname,exoid);
    ex_err("ex_get_ids",errmsg,exerrval);
    return (EX_FATAL);
    }


  if ((varid = ncvarid (exoid, varidobj)) == -1)
    {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: failed to locate %s ids variable in file id %d",
      tname,exoid);
    ex_err("ex_get_ids",errmsg,exerrval);
    return (EX_FATAL);
    }


  /* read in the element block ids  */

  /* application code has allocated an array of ints but netcdf is expecting
     a pointer to nclongs;  if ints are different sizes than nclongs,
     we must allocate an array of nclongs then convert them to ints with ltoi */

  start[0] = 0;
  count[0] = numobj;

  if (sizeof(int) == sizeof(nclong)) {
    iresult = ncvarget (exoid, varid, start, count, ids);
  } else {
    if (!(longs = malloc(numobj * sizeof(nclong)))) {
      exerrval = EX_MEMFAIL;
      sprintf(errmsg,
        "Error: failed to allocate memory for %s ids for file id %d",
               tname,exoid);
       ex_err("ex_get_ids",errmsg,exerrval);
       return (EX_FATAL);
     }
     iresult = ncvarget (exoid, varid, start, count, longs);
   }

   if (iresult == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
       "Error: failed to return %s ids in file id %d",
             tname,exoid);
     ex_err("ex_get_ids",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (sizeof(int) != sizeof(nclong)) {
      ltoi (longs, ids, numobj);
      free (longs);
   }

   return(EX_NOERR);
}
