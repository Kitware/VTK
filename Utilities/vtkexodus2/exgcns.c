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
* exgcns - read concatenated side sets
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
*
* exit conditions - 
*       int*    node_set_ids            array of node set IDs
*       int*    num_nodes_per_set       number of nodes for each set
*       int*    num_df_per_set          number of dist factors for each set
*       int*    node_sets_index         array of indices into node_set_node_list
*       int*    df_sets_index           array of indices into node_set_dist_fact
*       int*    node_sets_node_list     array of nodes for all sets
*       float*  node_sets_dist_fact     array of distribution factors for sets
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the node set ID's, node set count array, node set pointers array, 
 * node set node list, and node set distribution factors for all of the 
 * node sets
 */

int ex_get_concat_node_sets (int   exoid,
                             int  *node_set_ids,
                             int  *num_nodes_per_set, 
                             int  *num_df_per_set, 
                             int  *node_sets_node_index,
                             int  *node_sets_df_index,
                             int  *node_sets_node_list, 
                             void *node_sets_dist_fact)
{
   int i, num_node_sets, node_index_ctr, df_index_ctr;
   float fdum;
   char *cdum;
   float  *flt_dist_fact;
   double *dbl_dist_fact;
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

   cdum = 0; /* initialize even though it is not used */

/* first check if any node sets are specified */

   if (ncdimid (exoid, DIM_NUM_NS) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Warning: failed to locate number of node sets in file id %d",
            exoid);
     ex_err("ex_get_concat_node_sets",errmsg,exerrval);
     return (EX_WARN);         /* no node sets were defined */
   }

/* inquire how many node sets have been stored */

   if ((ex_inquire(exoid, EX_INQ_NODE_SETS, &num_node_sets, &fdum, cdum)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of node sets in file id %d",
            exoid);
     ex_err("ex_get_concat_node_sets",errmsg,exerrval);
     return(EX_FATAL);
   }

   if ((ex_get_node_set_ids (exoid, node_set_ids)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get node sets ids in file id %d",exoid);
     ex_err("ex_get_concat_node_sets",errmsg,exerrval);
     return(EX_FATAL);
   }

   node_index_ctr = 0;
   df_index_ctr = 0;

   for (i=0; i<num_node_sets; i++)
   {
     if ((ex_get_node_set_param(exoid, node_set_ids[i], 
                                &(num_nodes_per_set[i]),
                                &(num_df_per_set[i]))) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: failed to get node set parameters in file id %d",exoid);
       ex_err("ex_get_concat_node_sets",errmsg,exerrval);
       return(EX_FATAL);
     }

     /* get nodes for this set */
     if (num_nodes_per_set[i] > 0)
     {
       if (ex_get_node_set(exoid, node_set_ids[i],
                 &(node_sets_node_list[node_index_ctr])) == -1)
       {
         exerrval = ncerr;
         sprintf(errmsg,
                "Error: failed to get node set %d in file id %d",
                 node_set_ids[i], exoid);
         ex_err("ex_get_concat_node_sets",errmsg,exerrval);
         return(EX_FATAL);
       }

       if (ex_comp_ws(exoid) == sizeof(float) ) /* 4-byte float word */
       {

         /* get distribution factors for this set */
         flt_dist_fact = node_sets_dist_fact;
         if (num_df_per_set[i] > 0)     /* only get df if they exist */
         {
           if (ex_get_node_set_dist_fact(exoid, node_set_ids[i],
                             &(flt_dist_fact[df_index_ctr])) == -1)
           {
             exerrval = ncerr;
             sprintf(errmsg,
                  "Error: failed to get node set %d dist factors in file id %d",
                     node_set_ids[i], exoid);
             ex_err("ex_get_concat_node_sets",errmsg,exerrval);
             return(EX_FATAL);
           }
         }
       }
       else if (ex_comp_ws(exoid) == sizeof(double) )   /* 8-byte float word */
       {

         /* get distribution factors for this set */
         dbl_dist_fact = node_sets_dist_fact;
         if (num_df_per_set[i] > 0)             /* only get df if they exist */
         {
           if (ex_get_node_set_dist_fact(exoid, node_set_ids[i],
                               &(dbl_dist_fact[df_index_ctr])) == -1)
           {
             exerrval = ncerr;
             sprintf(errmsg,
                  "Error: failed to get node set %d dist factors in file id %d",
                     node_set_ids[i], exoid);
             ex_err("ex_get_concat_node_sets",errmsg,exerrval);
             return(EX_FATAL);
           }
         }
       }
       else
       {
         /* unknown floating point word size */
         exerrval = EX_BADPARAM;
         sprintf(errmsg,
                "Error: unsupported floating point word size %d for file id %d",
                 ex_comp_ws(exoid), exoid);
         ex_err("ex_get_concat_node_sets", errmsg, exerrval);
         return (EX_FATAL);
       }
     }

     /* update index arrays */

     if (i < num_node_sets)
     {
       node_sets_node_index[i] = node_index_ctr;
       node_index_ctr += num_nodes_per_set[i];  /* keep running count */

       if (num_df_per_set[i] > 0)               /* only get df if they exist */
       {
         node_sets_df_index[i] = df_index_ctr;
         df_index_ctr += num_df_per_set[i];     /* keep running count */
       }
       else
       {
         node_sets_df_index[i] = -1;            /* signal non-existence of df */
       }
     }
   }

   return(EX_NOERR);

}
