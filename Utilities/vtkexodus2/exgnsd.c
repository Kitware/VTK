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
* exgnsd - ex_get_node_set_dist_fact
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
*
* exit conditions - 
*       int*    node_set_dist_fact      node distribution factors for node set
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the distribution factors for a single node set
 */

int ex_get_node_set_dist_fact  (int   exoid,
                                int   node_set_id,
                                void *node_set_dist_fact)
{
   int dimid, dist_id, node_set_id_ndx;
   long num_nodes_in_set, start[1], count[1];
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

/* first check if any node sets are specified */

   if ((dimid = ncdimid (exoid, DIM_NUM_NS))  == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Warning: no node sets defined in file id %d",
             exoid);
     ex_err("ex_get_node_set_dist_fact",errmsg,exerrval);
     return (EX_WARN);
   }

/* Lookup index of node set id in VAR_NS_IDS array */

   node_set_id_ndx = ex_id_lkup(exoid,VAR_NS_IDS,node_set_id);
   if (exerrval != 0) 
   {
     if (exerrval == EX_NULLENTITY)
     {
       sprintf(errmsg,
              "Warning: node set %d is NULL in file id %d",
               node_set_id,exoid);
       ex_err("ex_get_node_set_dist_fact",errmsg,EX_MSG);
       return (EX_WARN);
     }
     else
     {

       sprintf(errmsg,
              "Error: failed to locate node set id %d in %s in file id %d",
               node_set_id,VAR_NS_IDS,exoid);
       ex_err("ex_get_node_set_dist_fact",errmsg,exerrval);
       return (EX_FATAL);
     }
   }

/* inquire id's of previously defined dimensions and variables */

   if ((dimid = ncdimid (exoid, DIM_NUM_NOD_NS(node_set_id_ndx))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
         "Error: failed to locate number of nodes in node set %d in file id %d",
             node_set_id,exoid);
     ex_err("ex_get_node_set_dist_fact",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (ncdiminq (exoid, dimid, (char *) 0, &num_nodes_in_set) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of nodes in node set %d in file id %d",
             node_set_id, exoid);
     ex_err("ex_get_node_set_dist_fact",errmsg,exerrval);
     return (EX_FATAL);
   }

   if ((dist_id = ncvarid (exoid, VAR_FACT_NS(node_set_id_ndx))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
             "Warning: dist factors not stored for node set %d in file id %d",
             node_set_id,exoid);
     ex_err("ex_get_node_set_dist_fact",errmsg,exerrval);
     return (EX_WARN);          /* complain - but not too loud */
   }


/* read in the distribution factors array */

   start[0] = 0;

   count[0] = num_nodes_in_set;

   if (ncvarget (exoid, dist_id, start, count,
             ex_conv_array(exoid,RTN_ADDRESS,node_set_dist_fact,
                           num_nodes_in_set)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get distribution factors in file id %d",
             exoid);
     ex_err("ex_get_node_set_dist_fact",errmsg,exerrval);
     return (EX_FATAL);
   }


   ex_conv_array( exoid, READ_CONVERT, node_set_dist_fact, num_nodes_in_set );

   return (EX_NOERR);

}
