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
* expnm - ex_put_node_map
*
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     map_id                  node map id
*       int     *node_map               node map
*
* exit conditions - 
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

#include <stdlib.h>

/* initialize this once */

struct list_item* nm_ctr_list = 0;

/*
 * writes an node map; this is a vector of integers of length number
 * of nodes
 */

int ex_put_node_map (int exoid,
                     int map_id,
                     const int *node_map)
{
   int dimid, varid, dims[1], iresult;
   long start[1]; 
   nclong ldum, *lptr;
   long num_node_maps, num_nodes, count[1];
   int cur_num_node_maps;
   char *cdum;
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

   cdum = 0;


   /* Return if node nodes stored in this file */
   if ((dimid = (ncdimid (exoid, DIM_NUM_NODES))) == -1 ) {
     return (EX_NOERR);
   }

/* first check if any node maps are specified */

   if ((dimid = (ncdimid (exoid, DIM_NUM_NM))) == -1 )
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: no node maps specified in file id %d",
             exoid);
     ex_err("ex_put_node_map",errmsg,exerrval);
     return (EX_FATAL);
   }

/* Check for duplicate node map id entry */
   ex_id_lkup(exoid,VAR_NM_PROP(1),map_id);
   if (exerrval != EX_LOOKUPFAIL)   /* found the node map id */
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: node map %d already defined in file id %d",
             map_id,exoid);
     ex_err("ex_put_node_map",errmsg,exerrval);
     return(EX_FATAL);
   }

/* Get number of node maps initialized for this file */
   if ((ncdiminq (exoid,dimid,cdum,&num_node_maps)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of node maps in file id %d",
             exoid);
     ex_err("ex_put_node_map",errmsg,exerrval);
     return (EX_FATAL);
   }

/* Keep track of the total number of node maps defined using a counter stored
   in a linked list keyed by exoid.
   NOTE: ex_get_file_item  is used to find the number of node maps
         for a specific file and returns that value.
*/
   cur_num_node_maps = ex_get_file_item(exoid, &nm_ctr_list );
   if (cur_num_node_maps >= num_node_maps)
   {
     exerrval = EX_FATAL;
     sprintf(errmsg,
          "Error: exceeded number of node maps (%ld) specified in file id %d",
             num_node_maps,exoid);
     ex_err("ex_put_node_map",errmsg,exerrval);
     return (EX_FATAL);
   }

/*   NOTE: ex_inc_file_item  is used to find the number of node maps
         for a specific file and returns that value incremented. */

   cur_num_node_maps = ex_inc_file_item(exoid, &nm_ctr_list );

/* write out information to previously defined variable */

   /* first get id of variable */

   if ((varid = ncvarid (exoid, VAR_NM_PROP(1))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to locate node map ids in file id %d",
             exoid);
     ex_err("ex_put_node_map",errmsg,exerrval);
     return (EX_FATAL);
   }

   /* then, write out node map id */

   start[0] = cur_num_node_maps;

   ldum = (nclong)map_id;
   if (ncvarput1 (exoid, varid, start, &ldum) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store node map id %d in file id %d",
             map_id,exoid);
     ex_err("ex_put_node_map",errmsg,exerrval);
     return (EX_FATAL);
   }

/* determine number of nodes */

   if ((dimid = (ncdimid (exoid, DIM_NUM_NODES))) == -1 )
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: couldn't determine number of nodes in file id %d",
             exoid);
     ex_err("ex_put_node_map",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (ncdiminq (exoid, dimid, (char *) 0, &num_nodes) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of nodes in file id %d",
             exoid);
     ex_err("ex_put_node_map",errmsg,exerrval);
     return (EX_FATAL);
   }

/* put netcdf file into define mode  */

   if (ncredef (exoid) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to put file id %d into define mode",
             exoid);
     ex_err("ex_put_elem_map",errmsg,exerrval);
     return (EX_FATAL);
   }

/* create variable array in which to store the node map */

   dims[0] = dimid;

   if ((varid = 
        ncvardef(exoid,VAR_NODE_MAP(cur_num_node_maps+1),NC_LONG,1,dims)) == -1)
   {
     if (ncerr == NC_ENAMEINUSE)
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: node map %d already defined in file id %d",
               map_id,exoid);
       ex_err("ex_put_node_map",errmsg,exerrval);
     }
     else
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: failed to create node map %d in file id %d",
               map_id,exoid);
       ex_err("ex_put_node_map",errmsg,exerrval);
     }
     goto error_ret;          /* exit define mode and return */
   }

/* leave define mode  */

   if (ncendef (exoid) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to complete definition in file id %d",
             exoid);
     ex_err("ex_put_node_map",errmsg,exerrval);
     return (EX_FATAL);
   }

/* write out the node map  */

/* this contortion is necessary because netCDF is expecting nclongs; fortunately
   it's necessary only when ints and nclongs aren't the same size */

   start[0] = 0;
   count[0] = num_nodes;

   if (sizeof(int) == sizeof(nclong)) {
      iresult = ncvarput (exoid, varid, start, count, node_map);
   } else {
      lptr = itol (node_map, (int)num_nodes);
      iresult = ncvarput (exoid, varid, start, count, lptr);
      free(lptr);
   }

   if (iresult == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store node map in file id %d",
             exoid);
     ex_err("ex_put_node_map",errmsg,exerrval);
     return (EX_FATAL);
   }

   return (EX_NOERR);


/* Fatal error: exit definition mode and return */
error_ret:
   if (ncendef (exoid) == -1)     /* exit define mode */
   {
     sprintf(errmsg,
            "Error: failed to complete definition for file id %d",
             exoid);
     ex_err("ex_put_node_map",errmsg,exerrval);
   }
   return (EX_FATAL);
}
