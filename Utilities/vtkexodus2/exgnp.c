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
* exgnp - ex_get_node_set_parameters
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
*       int*    num_nodes_in_set        number of nodes in set
*       int*    num_df_in_set           number of distribution factors in set
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the information which describe a single node set
 */

int ex_get_node_set_param (int  exoid,
                           int  node_set_id,
                           int *num_nodes_in_set,
                           int *num_df_in_set)
{
   int dimid, node_set_id_ndx;
   long lnum_nodes_in_set, lnum_df_in_set;
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

/* first check if any node sets are specified */

   if ((dimid = ncdimid (exoid, DIM_NUM_NS))  == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Warning: no node sets defined in file id %d",
             exoid);
     ex_err("ex_get_node_set_param",errmsg,exerrval);
     return (EX_WARN);
   }

/* Lookup index of node set id in VAR_NS_IDS array */

   node_set_id_ndx = ex_id_lkup(exoid,VAR_NS_IDS,node_set_id);
   if (exerrval != 0) 
   {
     if (exerrval == EX_NULLENTITY)     /* NULL node set ? */
     {
       *num_nodes_in_set = 0;
       *num_df_in_set = 0;
       return (EX_NOERR);
     }
     else
     {
       sprintf(errmsg,
     "Error: failed to locate node set id %d in VAR_NS_IDS array in file id %d",
             node_set_id,exoid);
       ex_err("ex_get_node_set_param",errmsg,exerrval);
       return (EX_FATAL);
     }

   }
/* inquire value of dimension of number of nodes for this node set */

   if ((dimid = ncdimid (exoid, DIM_NUM_NOD_NS(node_set_id_ndx))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
         "Error: failed to locate number of nodes in node set %d in file id %d",
             node_set_id,exoid);
     ex_err("ex_get_node_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (ncdiminq (exoid, dimid, (char *) 0, &lnum_nodes_in_set) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of nodes in node set %d in file id %d",
             node_set_id,exoid);
     ex_err("ex_get_node_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }
   *num_nodes_in_set = lnum_nodes_in_set;

/* Inquire if the dist factors variable have been defined for this node set.
   NOTE: If the dist factor variable for this node set index doesn't exist,
         it is assumed to be zero, otherwise the dist factor count will be
         the same as the number of nodes in the set. */
   if ((ncvarid (exoid, VAR_FACT_NS(node_set_id_ndx))) == -1)
   {
     *num_df_in_set = 0;        /* signal dist factor doesn't exist */
     if (ncerr == NC_ENOTVAR)
       return (EX_NOERR);
     else
     {
       exerrval = ncerr;
       sprintf(errmsg,
     "Error: failed to locate the dist factors for node set %d in file id %d",
               node_set_id,exoid);
       ex_err("ex_get_node_set_param",errmsg,exerrval);
       return (EX_FATAL);
     }
   }
   else
   {
     /* dimension exists, retrieve value */
     if (ncdiminq (exoid, dimid, (char *) 0, &lnum_df_in_set) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
        "Error: failed to get number of dist fact in node set %d in file id %d",
               node_set_id,exoid);
       ex_err("ex_get_node_set_param",errmsg,exerrval);
       return (EX_FATAL);
     }
     *num_df_in_set = lnum_nodes_in_set;        /* # of df = # of nodes */
   }

   return (EX_NOERR);
}
