/*
 * Copyright (c) 2006 Sandia Corporation. Under the terms of Contract
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
* exgnm - ex_get_map
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     map_type                type of map (node, edge, face, element)
*       int     map_id                  map id
*
* exit conditions - 
*       int*    map                     map
*
* revision history - 
*   20060930 - David Thompson - Adapted from ex_get_node_map
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the map with specified ID
 */

int ex_get_num_map ( int   exoid,
                     ex_entity_type map_type,
                     int   map_id,
                     int*  map )
{
   int dimid, var_id, id_ndx, status;
   char errmsg[MAX_ERR_LENGTH];
   const char* dim_map_size;
   const char* dim_num_maps;

   switch (map_type) {
   case EX_NODE_MAP:
     dim_map_size = DIM_NUM_NODES;
     dim_num_maps = DIM_NUM_NM;
     break;
   case EX_EDGE_MAP:
     dim_map_size = DIM_NUM_EDGE;
     dim_num_maps = DIM_NUM_EDM;
     break;
   case EX_FACE_MAP:
     dim_map_size = DIM_NUM_FACE;
     dim_num_maps = DIM_NUM_FAM;
     break;
   case EX_ELEM_MAP:
     dim_map_size = DIM_NUM_ELEM;
     dim_num_maps = DIM_NUM_EM;
     break;
   default:
     exerrval = EX_BADPARAM;
     sprintf( errmsg, "Bad map type (%d) specified", map_type );
     ex_err( "ex_get_num_map", errmsg, exerrval );
     return (EX_FATAL);
   }

   exerrval = 0; /* clear error code */

   /* See if any entries are stored in this file */
   if (nc_inq_dimid (exoid, dim_map_size, &dimid) != NC_NOERR) {
     return (EX_NOERR);
   }

   /* first check if any maps have been defined */
   if ((status = nc_inq_dimid(exoid, dim_num_maps, &dimid)) != NC_NOERR) {
     exerrval = status;
     sprintf(errmsg,
            "Warning: no %ss defined in file id %d",
             ex_name_of_object(map_type),exoid);
     ex_err("ex_get_map",errmsg,exerrval);
     return (EX_WARN);
   }

   /* Lookup index of map id property array */
   id_ndx = ex_id_lkup(exoid,map_type,map_id);
   if (exerrval != 0) {
      sprintf(errmsg,
              "Error: failed to locate %s id %d in id variable in file id %d",
               ex_name_of_object(map_type),map_id,exoid);
      ex_err("ex_get_map",errmsg,exerrval);
      return (EX_FATAL);
   }

   /* inquire id's of previously defined dimensions and variables */
   if ((status = nc_inq_varid(exoid, ex_name_of_map(map_type,id_ndx), &var_id)) != NC_NOERR) {
     exerrval = status;
     sprintf(errmsg,
            "Error: failed to locate %s %d in file id %d",
             ex_name_of_object(map_type),map_id,exoid);
     ex_err("ex_get_map",errmsg,exerrval);
     return (EX_FATAL);
   }

   /* read in the map */
   status = nc_get_var_int(exoid, var_id, map);
   
   if (status != NC_NOERR) {
     exerrval = status;
     sprintf(errmsg,
            "Error: failed to get %s in file id %d",
             ex_name_of_object(map_type),exoid);
     ex_err("ex_get_map",errmsg,exerrval);
     return (EX_FATAL);
   }

   return (EX_NOERR);
}
