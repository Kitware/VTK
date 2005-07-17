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
* expnp - ex_put_node_set_parameters
*
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          James A. Schutt - 8 byte float and standard C definitions
*          Vic Yarberry    - Added headers and error logging
*
*          
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     node_set_id             node set id
*       int     num_nodes_in_set        number of nodes in set
*       int     num_dist_in_set         number of distribution factors in set
*
* exit conditions - 
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

struct list_item* ns_ctr_list = 0;

/*
 * writes the information which describes a single node set
 */

int ex_put_node_set_param (int exoid,
                           int node_set_id,
                           int num_nodes_in_set,
                           int num_dist_in_set)
{
   int dimid, varid, dims[1];
   long start[1]; 
   nclong ldum;
   long num_node_sets;
   int cur_num_node_sets, node_set_stat;
   char *cdum;
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

   cdum = 0;

/* first check if any node sets are specified */

   if ((dimid = (ncdimid (exoid, DIM_NUM_NS))) == -1 )
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: no node sets specified in file id %d",
             exoid);
     ex_err("ex_put_node_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

/* Check for duplicate node set id entry */
   ex_id_lkup(exoid,VAR_NS_IDS,node_set_id);
   if (exerrval != EX_LOOKUPFAIL)   /* found the node set id */
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: node set %d already defined in file id %d",
             node_set_id,exoid);
     ex_err("ex_put_node_set_param",errmsg,exerrval);
     return(EX_FATAL);
   }

/* Get number of node sets initialized for this file */
   if ((ncdiminq (exoid,dimid,cdum,&num_node_sets)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of node sets in file id %d",
             exoid);
     ex_err("ex_put_node_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }


/* Keep track of the total number of node sets defined using a counter stored
   in a linked list keyed by exoid.
   NOTE: ex_get_file_item  is used to find the number of node sets
         for a specific file and returns that value.
*/
   cur_num_node_sets=ex_get_file_item(exoid, &ns_ctr_list );
   if (cur_num_node_sets >= num_node_sets)
   {
     exerrval = EX_FATAL;
     sprintf(errmsg,
           "Error: exceeded number of node sets (%ld) specified in file id %d",
             num_node_sets,exoid);
     ex_err("ex_put_node_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

/*   NOTE: ex_inc_file_item  is used to find the number of node sets
         for a specific file and returns that value incremented. */

   cur_num_node_sets=ex_inc_file_item(exoid, &ns_ctr_list );

/* write out information to previously defined variable */

   /* first get id of variable */

   if ((varid = ncvarid (exoid, VAR_NS_IDS)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to locate node set ids in file id %d",
             exoid);
     ex_err("ex_put_node_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

   /* then, write out node set id */

   start[0] = cur_num_node_sets;

   ldum = (nclong)node_set_id;
   if (ncvarput1 (exoid, varid, start, &ldum) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store node set id %d in file id %d",
             node_set_id,exoid);
     ex_err("ex_put_node_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (num_nodes_in_set == 0) /* Is this a NULL side set? */
     node_set_stat = 0; /* change node set status to NULL */
   else
     node_set_stat = 1; /* change node set status to TRUE */

   if ((varid = ncvarid (exoid, VAR_NS_STAT)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
           "Error: failed to locate node set status in file id %d", exoid);
     ex_err("ex_put_node_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

   ldum = (nclong)node_set_stat;
   if (ncvarput1 (exoid, varid, start, &ldum) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store node set %d status to file id %d",
             node_set_id, exoid);
     ex_err("ex_put_node_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (num_nodes_in_set == 0) /* Is this a NULL side set? */
   {
     return(EX_NOERR);
   }


/* put netcdf file into define mode  */

   if (ncredef (exoid) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to put file id %d into define mode",
             exoid);
     ex_err("ex_put_node_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }


   if ((dimid = ncdimdef (exoid, DIM_NUM_NOD_NS(cur_num_node_sets+1),
                 (long)num_nodes_in_set)) == -1)
   {
     if (ncerr == NC_ENAMEINUSE)
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: node set %d size already defined in file id %d",
               node_set_id,exoid);
       ex_err("ex_put_node_set_param",errmsg,exerrval);
     }
     else
     {
       exerrval = ncerr;
       sprintf(errmsg,
             "Error: failed to define number of nodes for set %d in file id %d",
               node_set_id,exoid);
       ex_err("ex_put_node_set_param",errmsg,exerrval);
     }
     goto error_ret;
   }


/* create variable array in which to store the node set node list */

   dims[0] = dimid;

   if (ncvardef(exoid, VAR_NODE_NS(cur_num_node_sets+1), NC_LONG,1,dims) == -1)
   {
     if (ncerr == NC_ENAMEINUSE)
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: node set %d node list already defined in file id %d",
               node_set_id,exoid);
       ex_err("ex_put_node_set_param",errmsg,exerrval);
     }
     else
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: failed to create node set %d node list in file id %d",
               node_set_id,exoid);
       ex_err("ex_put_node_set_param",errmsg,exerrval);
     }
     goto error_ret;          /* exit define mode and return */
   }

/* Create distribution factors variable if required */

   if (num_dist_in_set > 0)
   {

     /* num_dist_in_set should equal num_nodes_in_set */
     if (num_dist_in_set != num_nodes_in_set)
     {
       exerrval = EX_FATAL;
       sprintf(errmsg, 
"Error: # dist fact (%d) not equal to # nodes (%d) in node set %d file id %d",
                 num_dist_in_set, num_nodes_in_set, node_set_id,exoid);
       ex_err("ex_put_node_set_param",errmsg,exerrval);
       goto error_ret;          /* exit define mode and return */
     }
     else
     {
/* create variable for distribution factors */

       if (ncvardef (exoid, VAR_FACT_NS(cur_num_node_sets+1),
                       nc_flt_code(exoid), 1, dims) == -1)
       {
         if (ncerr == NC_ENAMEINUSE)
         {
           exerrval = ncerr;
           sprintf(errmsg,
                  "Error: node set %d dist factors already exist in file id %d",
                   node_set_id,exoid);
           ex_err("ex_put_node_set_param",errmsg,exerrval);
         }
         else
         {
           exerrval = ncerr;
           sprintf(errmsg,
               "Error: failed to create node set %d dist factors in file id %d",
                node_set_id,exoid);
           ex_err("ex_put_node_set_param",errmsg,exerrval);
         }
         goto error_ret;            /* exit define mode and return */
       }
     }
   }

/* leave define mode  */

   if (ncendef (exoid) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to complete definition in file id %d",
             exoid);
     ex_err("ex_put_node_set_param",errmsg,exerrval);
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
     ex_err("ex_put_node_set_param",errmsg,exerrval);
   }
   return (EX_FATAL);
}
