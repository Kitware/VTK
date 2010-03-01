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
* expem - ex_put_num_map
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     map_type                type of map (node,edge,face,elem)
*       int     map_id                  id to associate with new map
*       int     *map_data               map set value array
*
* exit conditions - 
*
* revision history - 
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"
#include <stdlib.h> /* for free() */

/*!
 * writes a map; this is a vector of integers of the same length as the
 * number of entries in the source object (nodes, edges, faces, or elements
 * in the file).
 * \param   exoid                   exodus file id
 * \param   map_type                type of map (node,edge,face,elem)
 * \param   map_id                  id to associate with new map
 * \param   map                    map set value array
 */

int ex_put_num_map ( int exoid,
                     ex_entity_type map_type,
                     int map_id,
                     const int *map )
{
   int dimid, varid;
   size_t start[1]; 
   int ldum;
   int num_maps;
   size_t num_entries;
   int cur_num_maps;
   char errmsg[MAX_ERR_LENGTH];
   const char* dnumentries;
   const char* dnummaps;
   const char* vmapids;
   const char* vmap;
   int status;
   
   exerrval = 0; /* clear error code */

   switch ( map_type ) {
   case EX_NODE_MAP:
     dnumentries = DIM_NUM_NODES;
     dnummaps = DIM_NUM_NM;
     vmapids = VAR_NM_PROP(1);
     break;
   case EX_EDGE_MAP:
     dnumentries = DIM_NUM_EDGE;
     dnummaps = DIM_NUM_EDM;
     vmapids = VAR_EDM_PROP(1);
     break;
   case EX_FACE_MAP:
     dnumentries = DIM_NUM_FACE;
     dnummaps = DIM_NUM_FAM;
     vmapids = VAR_FAM_PROP(1);
     break;
   case EX_ELEM_MAP:
     dnumentries = DIM_NUM_ELEM;
     dnummaps = DIM_NUM_EM;
     vmapids = VAR_EM_PROP(1);
     break;
   default:
     exerrval = EX_BADPARAM;
     sprintf( errmsg,
       "Error: Bad map type (%d) specified for file id %d",
       map_type, exoid );
     ex_err( "ex_put_num_map", errmsg, exerrval );
     return (EX_FATAL);
   }

   /* Make sure the file contains entries */
   if (nc_inq_dimid (exoid, dnumentries, &dimid) != NC_NOERR )
   {
     return (EX_NOERR);
   }

   /* first check if any maps are specified */
   if ((status = nc_inq_dimid (exoid, dnummaps, &dimid)) != NC_NOERR )
   {
     exerrval = status;
     sprintf(errmsg,
            "Error: no %ss specified in file id %d",
             ex_name_of_object(map_type),exoid);
     ex_err("ex_put_num_map",errmsg,exerrval);
     return (EX_FATAL);
   }

   /* Check for duplicate map id entry */
   ex_id_lkup(exoid,map_type,map_id); 
   if (exerrval != EX_LOOKUPFAIL)   /* found the map id */
   {
     sprintf(errmsg,
            "Error: %s %d already defined in file id %d",
             ex_name_of_object(map_type),map_id,exoid);
     ex_err("ex_put_num_map",errmsg,exerrval);
     return(EX_FATAL);
   }

   /* Get number of maps initialized for this file */
   if ((status = nc_inq_dimlen(exoid,dimid,&num_entries)) != NC_NOERR)
   {
     exerrval = status;
     sprintf(errmsg,
            "Error: failed to get number of %ss in file id %d",
             ex_name_of_object(map_type),exoid);
     ex_err("ex_put_num_map",errmsg,exerrval);
     return (EX_FATAL);
   }
   num_maps = (int)num_entries;

   /* Keep track of the total number of maps defined using a counter stored
      in a linked list keyed by exoid.
      NOTE: ex_get_file_item  is used to find the number of maps
      for a specific file and returns that value.
   */
   cur_num_maps = ex_get_file_item(exoid, ex_get_counter_list(map_type));
   if (cur_num_maps >= num_maps) {
     exerrval = EX_FATAL;
     sprintf(errmsg,
          "Error: exceeded number of %ss (%d) specified in file id %d",
             ex_name_of_object(map_type),num_maps,exoid);
     ex_err("ex_put_num_map",errmsg,exerrval);
     return (EX_FATAL);
   }

   /*   NOTE: ex_inc_file_item  is used to find the number of maps
        for a specific file and returns that value incremented. */
   cur_num_maps = ex_inc_file_item(exoid, ex_get_counter_list(map_type));

   /* write out information to previously defined variable */

   /* first get id of variable */
   if ((status = nc_inq_varid (exoid, vmapids, &varid)) == -1)
   {
     exerrval = status;
     sprintf(errmsg,
            "Error: failed to locate %s ids in file id %d",
             ex_name_of_object(map_type),exoid);
     ex_err("ex_put_num_map",errmsg,exerrval);
     return (EX_FATAL);
   }

   /* then, write out map id */
   start[0] = cur_num_maps;

   ldum = (int)map_id;
   if ((status = nc_put_var1_int(exoid, varid, start, &ldum)) != NC_NOERR)
   {
     exerrval = status;
     sprintf(errmsg,
            "Error: failed to store %s id %d in file id %d",
             ex_name_of_object(map_type),map_id,exoid);
     ex_err("ex_put_num_map",errmsg,exerrval);
     return (EX_FATAL);
   }

   switch ( map_type ) {
   case EX_NODE_MAP:
     vmap = VAR_NODE_MAP(cur_num_maps+1);
     break;
   case EX_EDGE_MAP:
     vmap = VAR_EDGE_MAP(cur_num_maps+1);
     break;
   case EX_FACE_MAP:
     vmap = VAR_FACE_MAP(cur_num_maps+1);
     break;
   case EX_ELEM_MAP:
     vmap = VAR_ELEM_MAP(cur_num_maps+1);
     break;
  default:
    exerrval = 1005;
    sprintf(errmsg,
            "Internal Error: unrecognized map type in switch: %d in file id %d",
            map_type,exoid);
    ex_err("ex_putt_n_one_attr",errmsg,EX_MSG);
    return (EX_FATAL);
   }

   /* locate variable array in which to store the map */
   if ((status = nc_inq_varid(exoid,vmap,&varid)) != NC_NOERR)
     {
       int dims[2];

       /* determine number of entries */
       if ((status = nc_inq_dimid (exoid, dnumentries, &dimid)) == -1 )
         {
           exerrval = status;
           sprintf(errmsg,
                   "Error: couldn't determine number of %s entries in file id %d",
                   ex_name_of_object(map_type),exoid);
           ex_err("ex_put_num_map",errmsg,exerrval);
           return (EX_FATAL);
         }
       
       status = 0;
       
       if ((status = nc_redef( exoid )) != NC_NOERR ) {
         exerrval = status;
         sprintf(errmsg, "Error: failed to place file id %d into define mode", exoid);
         ex_err("ex_put_num_map",errmsg,exerrval);
         return (EX_FATAL);
       }

       dims[0] = dimid;
       if ((status = nc_def_var( exoid, vmap, NC_INT, 1, dims, &varid )) == -1 ) {
         exerrval = status;
         sprintf(errmsg, "Error: failed to define map %s in file id %d", vmap, exoid);
         ex_err("ex_put_num_map",errmsg,exerrval);
       }

       if ((status = nc_enddef(exoid)) != NC_NOERR ) { /* exit define mode */
         sprintf( errmsg, "Error: failed to complete definition for file id %d", exoid );
         ex_err( "ex_put_num_map", errmsg, exerrval );
         varid = -1; /* force early exit */
       }

       if ( varid == -1 ) /* we couldn't define variable and have prepared error message. */
         return (EX_FATAL);
     }

   /* write out the map  */
   if ((status = nc_put_var_int(exoid, varid, map)) != NC_NOERR)
   {
     exerrval = status;
     sprintf(errmsg,
            "Error: failed to store %s in file id %d",
             ex_name_of_object(map_type),exoid);
     ex_err("ex_put_num_map",errmsg,exerrval);
     return (EX_FATAL);
   }

   return (EX_NOERR);
}
