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
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          
* environment - UNIX
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
* 20060928   David Thompson   - Adapted from ex_put_elem_map
*
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"
#include <stdlib.h> /* for free() */

/*!
 * writes a map; this is a vector of integers of the same length as the
 * number of entries in the source object (nodes, edges, faces, or elements
 * in the file).
 */

int ex_put_num_map ( int exoid,
                     int map_type,
                     int map_id,
                     const int *map )
{
   int dimid, varid, iresult;
   long start[1]; 
   nclong ldum, *lptr;
   long num_maps, num_entries, count[1];
   int cur_num_maps;
   char *cdum;
   char errmsg[MAX_ERR_LENGTH];
   const char* tname;
   const char* dnumentries;
   const char* dnummaps;
   const char* vmapids;
   const char* vmap = 0;
   struct list_item** map_ctr_list;

   exerrval = 0; /* clear error code */

   cdum = 0;

   switch ( map_type ) {
   case EX_NODE_MAP:
     tname = "node";
     dnumentries = DIM_NUM_NODES;
     dnummaps = DIM_NUM_NM;
     vmapids = VAR_NM_PROP(1);
     map_ctr_list = &nm_ctr_list;
     break;
   case EX_EDGE_MAP:
     tname = "edge";
     dnumentries = DIM_NUM_EDGE;
     dnummaps = DIM_NUM_EDM;
     vmapids = VAR_EDM_PROP(1);
     map_ctr_list = &edm_ctr_list;
     break;
   case EX_FACE_MAP:
     tname = "face";
     dnumentries = DIM_NUM_FACE;
     dnummaps = DIM_NUM_FAM;
     vmapids = VAR_FAM_PROP(1);
     map_ctr_list = &fam_ctr_list;
     break;
   case EX_ELEM_MAP:
     tname = "element";
     dnumentries = DIM_NUM_ELEM;
     dnummaps = DIM_NUM_EM;
     vmapids = VAR_EM_PROP(1);
     map_ctr_list = &em_ctr_list;
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
   if ((dimid = (ncdimid (exoid, dnumentries))) == -1 )
   {
     return (EX_NOERR);
   }

/* first check if any maps are specified */

   if ((dimid = (ncdimid (exoid, dnummaps))) == -1 )
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: no %s maps specified in file id %d",
             tname,exoid);
     ex_err("ex_put_num_map",errmsg,exerrval);
     return (EX_FATAL);
   }

/* Check for duplicate map id entry */
   ex_id_lkup(exoid,vmapids,map_id); 
   if (exerrval != EX_LOOKUPFAIL)   /* found the map id */
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: %s map %d already defined in file id %d",
             tname,map_id,exoid);
     ex_err("ex_put_num_map",errmsg,exerrval);
     return(EX_FATAL);
   }

/* Get number of maps initialized for this file */
   if ((ncdiminq (exoid,dimid,cdum,&num_maps)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of %s maps in file id %d",
             tname,exoid);
     ex_err("ex_put_num_map",errmsg,exerrval);
     return (EX_FATAL);
   }

/* Keep track of the total number of maps defined using a counter stored
   in a linked list keyed by exoid.
   NOTE: ex_get_file_item  is used to find the number of maps
         for a specific file and returns that value.
*/
   cur_num_maps = ex_get_file_item(exoid, map_ctr_list );
   if (cur_num_maps >= num_maps)
   {
     exerrval = EX_FATAL;
     sprintf(errmsg,
          "Error: exceeded number of %s maps (%ld) specified in file id %d",
             tname,num_maps,exoid);
     ex_err("ex_put_num_map",errmsg,exerrval);
     return (EX_FATAL);
   }

/*   NOTE: ex_inc_file_item  is used to find the number of maps
         for a specific file and returns that value incremented. */

   cur_num_maps = ex_inc_file_item(exoid, map_ctr_list );

/* write out information to previously defined variable */

   /* first get id of variable */

   if ((varid = ncvarid (exoid, vmapids)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to locate %s map ids in file id %d",
             tname,exoid);
     ex_err("ex_put_num_map",errmsg,exerrval);
     return (EX_FATAL);
   }

   /* then, write out map id */

   start[0] = cur_num_maps;

   ldum = (nclong)map_id;
   if (ncvarput1 (exoid, varid, start, &ldum) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store %s map id %d in file id %d",
             tname,map_id,exoid);
     ex_err("ex_put_num_map",errmsg,exerrval);
     return (EX_FATAL);
   }

/* determine number of entries */

   if ((dimid = (ncdimid (exoid, dnumentries))) == -1 )
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: couldn't determine number of %s entries in file id %d",
             tname,exoid);
     ex_err("ex_put_num_map",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (ncdiminq (exoid, dimid, (char *) 0, &num_entries) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of %s entries in file id %d",
             tname,exoid);
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
   }

/* locate variable array in which to store the map */
   if ((varid = ncvarid(exoid,vmap)) == -1)
     {
#if 0
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: failed to locate %s map %d in file id %d",
               vmap,map_id,exoid);
       ex_err("ex_put_num_map",errmsg,exerrval);
       return (EX_FATAL);
#endif
       int dims[2];
       ncerr = 0;

       if ( ncredef( exoid ) == -1 ) {
         exerrval = ncerr;
         sprintf(errmsg, "Error: failed to place file id %d into define mode", exoid);
         ex_err("ex_put_num_map",errmsg,exerrval);
         return (EX_FATAL);
       }

       dims[0] = dimid;
       if ( (varid = ncvardef( exoid, vmap, NC_LONG, 1, dims )) == -1 ) {
         exerrval = ncerr;
         sprintf(errmsg, "Error: failed to define map %s in file id %d", vmap, exoid);
         ex_err("ex_put_num_map",errmsg,exerrval);
       }

       if ( ncendef( exoid ) == -1 ) { /* exit define mode */
         sprintf( errmsg, "Error: failed to complete definition for file id %d", exoid );
         ex_err( "ex_put_num_map", errmsg, exerrval );
         varid = -1; /* force early exit */
       }

       if ( varid == -1 ) /* we couldn't define variable and have prepared error message. */
         return (EX_FATAL);
     }

/* write out the map  */

/* this contortion is necessary because netCDF is expecting nclongs; fortunately
   it's necessary only when ints and nclongs aren't the same size */

   start[0] = 0;
   count[0] = num_entries;

   if (sizeof(int) == sizeof(nclong)) {
      iresult = ncvarput (exoid, varid, start, count, map);
   } else {
      lptr = itol (map, (int)num_entries);
      iresult = ncvarput (exoid, varid, start, count, lptr);
      free(lptr);
   }

   if (iresult == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store %s map in file id %d",
             tname,exoid);
     ex_err("ex_put_num_map",errmsg,exerrval);
     return (EX_FATAL);
   }

   return (EX_NOERR);
}
