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
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*
*          
* environment - UNIX
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
                     int   map_type,
                     int   map_id,
                     int*  map )
{
   int dimid, var_id, id_ndx, iresult;
   long num_entries, start[1], count[1]; 
   nclong *longs;
   char errmsg[MAX_ERR_LENGTH];
   const char* dim_map_size;
   const char* dim_num_maps;
   const char* prop_map_id;
   const char* tname;

   switch (map_type) {
   case EX_NODE_MAP:
     tname = "node";
     dim_map_size = DIM_NUM_NODES;
     dim_num_maps = DIM_NUM_NM;
     prop_map_id = VAR_NM_PROP(1);
     break;
   case EX_EDGE_MAP:
     tname = "edge";
     dim_map_size = DIM_NUM_EDGE;
     dim_num_maps = DIM_NUM_EDM;
     prop_map_id = VAR_EDM_PROP(1);
     break;
   case EX_FACE_MAP:
     tname = "face";
     dim_map_size = DIM_NUM_FACE;
     dim_num_maps = DIM_NUM_FAM;
     prop_map_id = VAR_FAM_PROP(1);
     break;
   case EX_ELEM_MAP:
     tname = "elem";
     dim_map_size = DIM_NUM_ELEM;
     dim_num_maps = DIM_NUM_EM;
     prop_map_id = VAR_EM_PROP(1);
     break;
   default:
     exerrval = EX_BADPARAM;
     sprintf( errmsg, "Bad map type (%d) specified", map_type );
     ex_err( "ex_get_num_map", errmsg, exerrval );
     return (EX_FATAL);
   }

   exerrval = 0; /* clear error code */

   /* See if any entries are stored in this file */
   if ((dimid = ncdimid (exoid, dim_map_size)) == -1)
   {
     return (EX_NOERR);
   }

   if (ncdiminq (exoid, dimid, (char *) 0, &num_entries) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of entries in file id %d", exoid);
     ex_err("ex_get_map",errmsg,exerrval);
     return (EX_FATAL);
   }

/* first check if any maps have been defined */

   if ((dimid = ncdimid (exoid, dim_num_maps))  == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Warning: no %s maps defined in file id %d",
             tname,exoid);
     ex_err("ex_get_map",errmsg,exerrval);
     return (EX_WARN);
   }

/* Lookup index of map id property array */

   id_ndx = ex_id_lkup(exoid,prop_map_id,map_id);
   if (exerrval != 0) 
   {

      sprintf(errmsg,
              "Error: failed to locate %s map id %d in %s in file id %d",
               tname,map_id,prop_map_id,exoid);
      ex_err("ex_get_map",errmsg,exerrval);
      return (EX_FATAL);
   }

/* inquire id's of previously defined dimensions and variables */

   if ((var_id = ncvarid (exoid, ex_name_of_map(map_type,id_ndx))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to locate %s map %d in file id %d",
             tname,map_id,exoid);
     ex_err("ex_get_map",errmsg,exerrval);
     return (EX_FATAL);
   }


/* read in the map */

/* application code has allocated an array of ints but netcdf is expecting
   a pointer to nclongs;  if ints are different sizes than nclongs,
   we must allocate an array of nclongs then convert them to ints with ltoi */

   start[0] = 0;
   count[0] = num_entries;

   if (sizeof(int) == sizeof(nclong)) {
     iresult = ncvarget (exoid, var_id, start, count, map);
   } else {
     if (!(longs = malloc(num_entries * sizeof(nclong)))) {
       exerrval = EX_MEMFAIL;
       sprintf(errmsg,
               "Error: failed to allocate memory for %s map for file id %d",
               tname,exoid);
       ex_err("ex_get_map",errmsg,exerrval);
       return (EX_FATAL);
     }
     iresult = ncvarget (exoid, var_id, start, count, longs);
   }
   
   if (iresult == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get %s map in file id %d",
             tname,exoid);
     ex_err("ex_get_map",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (sizeof(int) != sizeof(nclong)) {
      ltoi (longs, map, num_entries);
      free (longs);
   }

   return (EX_NOERR);

}
