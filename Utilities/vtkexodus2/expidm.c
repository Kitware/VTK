/*
 * Copyright (c) 2007 Sandia Corporation. Under the terms of Contract
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
* expenm - ex_put_id_map
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       ex_entity_type obj_type
*       int*    elem_map                element numbering map array
*
* exit conditions - 
*
* revision history - 
*
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"
#include <stdlib.h> /* for free() */

/*!
 * writes out the entity numbering map to the database; this allows
 * the entity numbers to be non-contiguous.  This map is used for
 * mapping between local and global entity ids.
 * \param    exoid                   exodus file id
 * \param    map_type
 * \param    map                element numbering map array
 */

int ex_put_id_map (int  exoid,
                   ex_entity_type map_type,
                   const int *map)
{
  int dimid, mapid, status, dims[1];
  char errmsg[MAX_ERR_LENGTH];
  const char* tname;
  const char* dnumentries;
  const char* vmap;

  exerrval = 0; /* clear error code */

  switch ( map_type ) {
  case EX_NODE_MAP:
    tname = "node";
    dnumentries = DIM_NUM_NODES;
    vmap = VAR_NODE_NUM_MAP;
    break;
  case EX_EDGE_MAP:
    tname = "edge";
    dnumentries = DIM_NUM_EDGE;
    vmap = VAR_EDGE_NUM_MAP;
    break;
  case EX_FACE_MAP:
    tname = "face";
    dnumentries = DIM_NUM_FACE;
    vmap = VAR_FACE_NUM_MAP;
    break;
  case EX_ELEM_MAP:
    tname = "element";
    dnumentries = DIM_NUM_ELEM;
    vmap = VAR_ELEM_NUM_MAP;
    break;
  default:
    exerrval = EX_BADPARAM;
    sprintf( errmsg,
             "Error: Bad map type (%d) specified for file id %d",
             map_type, exoid );
    ex_err( "ex_put_id_map", errmsg, exerrval );
    return (EX_FATAL);
  }

  /* Make sure the file contains entries */
  if (nc_inq_dimid (exoid, dnumentries, &dimid) != NC_NOERR) {
    return (EX_NOERR);
  }
   
  /* put netcdf file into define mode  */
  if (nc_inq_varid (exoid, vmap, &mapid) != NC_NOERR) {
    if ((status = nc_redef (exoid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to put file id %d into define mode",
              exoid);
      ex_err("ex_put_id_map",errmsg,exerrval);
      return (EX_FATAL);
    }
    

    /* create a variable array in which to store the id map  */
    dims[0] = dimid;
    
    if ((status = nc_def_var(exoid, vmap, NC_INT, 1, dims, &mapid)) != NC_NOERR) {
      if (status == NC_ENAMEINUSE) {
        exerrval = status;
        sprintf(errmsg,
                "Error: %s numbering map already exists in file id %d",
                tname, exoid);
        ex_err("ex_put_id_map",errmsg,exerrval);
      } else {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to create %s id map in file id %d",
                tname, exoid);
        ex_err("ex_put_id_map",errmsg,exerrval);
      }
      goto error_ret;         /* exit define mode and return */
    }
    

    /* leave define mode  */
    if ((status = nc_enddef (exoid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to complete definition in file id %d",
              exoid);
      ex_err("ex_put_id_map",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  /* write out the entity numbering map  */
  status = nc_put_var_int(exoid, mapid, map);

  if (status!= NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to store %s numbering map in file id %d",
            tname, exoid);
    ex_err("ex_put_id_map",errmsg,exerrval);
    return (EX_FATAL);
  }


  return (EX_NOERR);

  /* Fatal error: exit definition mode and return */
 error_ret:
  if (nc_enddef (exoid) != NC_NOERR)     /* exit define mode */
    {
      sprintf(errmsg,
              "Error: failed to complete definition for file id %d",
              exoid);
      ex_err("ex_put_id_map",errmsg,exerrval);
    }
  return (EX_FATAL);
}

