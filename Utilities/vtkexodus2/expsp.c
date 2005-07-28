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
* expsp - ex_put_side_set_param
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
*       int     side_set_id             side set id
*       int     num_side_in_set         number of sides in the side set
*       int     num_dist_fact_in_set    number of distribution factors in the
*                                       side set
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

struct list_item* ss_ctr_list = 0;       /* structure to hold number of
                                            side sets for each file id */
/*
 * writes the side set id and the number of sides (edges or faces) 
 * which describe a single side set
 */

int ex_put_side_set_param (int exoid,
                           int side_set_id,
                           int num_side_in_set,
                           int num_dist_fact_in_set)
{
   int dimid, varid, side_set_id_ndx, dims[1]; 
   long start[1], num_side_sets; 
   nclong ldum;
   int cur_num_side_sets, side_set_stat;
   char *cdum;
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

   cdum = 0;

/* first check if any side sets are specified */

   if ((dimid = ncdimid (exoid, DIM_NUM_SS)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: no side sets specified in file id %d",
             exoid);
     ex_err("ex_put_side_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

/* Check for duplicate side set id entry */
   ex_id_lkup(exoid,VAR_SS_IDS,side_set_id);
   if (exerrval != EX_LOOKUPFAIL)   /* found the side set id */
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: side set %d already defined in file id %d",
             side_set_id,exoid);
     ex_err("ex_put_side_set_param",errmsg,exerrval);
     return(EX_FATAL);
   }

/* Get number of side sets specified for this file */
   if ((ncdiminq (exoid,dimid,cdum,&num_side_sets)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of side sets in file id %d",
             exoid);
     ex_err("ex_put_side_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }


/* Keep track of the total number of side sets defined using a counter stored
   in a linked list keyed by exoid.
   NOTE: ex_get_file_item finds the maximum number of side sets defined
         for a specific file and returns that value.
*/
   cur_num_side_sets=ex_get_file_item(exoid,&ss_ctr_list);
   if (cur_num_side_sets >= num_side_sets)
   {
     exerrval = EX_FATAL;
     sprintf(errmsg,
            "Error: exceeded number of side sets (%ld) defined in file id %d",
             num_side_sets,exoid);
     ex_err("ex_put_side_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

/*   NOTE: ex_inc_file_item finds the current number of side sets defined
         for a specific file and returns that value incremented. */

   cur_num_side_sets=ex_inc_file_item(exoid,&ss_ctr_list);
   side_set_id_ndx = cur_num_side_sets + 1;

/* write out information to previously defined variable */

   /* first: get id of side set id variable */

   if ((varid = ncvarid (exoid, VAR_SS_IDS)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to locate side set %d in file id %d",
             side_set_id, exoid);
     ex_err("ex_put_side_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

   /* write out side set id */

   start[0] = cur_num_side_sets;

   ldum = (nclong)side_set_id;
   if (ncvarput1 (exoid, varid, start, &ldum) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store side set id %d in file id %d",
             side_set_id, exoid);
     ex_err("ex_put_side_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (num_side_in_set == 0) /* Is this a NULL side set? */
     side_set_stat = 0; /* change side set status to NULL */
   else
     side_set_stat = 1; /* change side set status to TRUE */

   if ((varid = ncvarid (exoid, VAR_SS_STAT)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
           "Error: failed to locate side set status in file id %d", exoid);
     ex_err("ex_put_side_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

   ldum = (nclong)side_set_stat;
   if (ncvarput1 (exoid, varid, start, &ldum) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store side set %d status to file id %d",
             side_set_id, exoid);
     ex_err("ex_put_side_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (num_side_in_set == 0) /* Is this a NULL side set? */
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
     ex_err("ex_put_side_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }


/* define dimensions and variables */

   if (num_side_in_set > 0)
   {
     if ((dimid = ncdimdef(exoid,DIM_NUM_SIDE_SS(side_set_id_ndx),
                           (long)num_side_in_set)) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to define number of sides in side set %d in file id %d",
               side_set_id,exoid);
       ex_err("ex_put_side_set_param",errmsg,exerrval);
       goto error_ret;
     }

     dims[0] = dimid;

     if (ncvardef (exoid, VAR_ELEM_SS(side_set_id_ndx), NC_LONG, 1, dims) == -1)
     {
       if (ncerr == NC_ENAMEINUSE)
       {
         exerrval = ncerr;
         sprintf(errmsg,
             "Error: element list already exists for side set %d in file id %d",
                 side_set_id,exoid);
         ex_err("ex_put_side_set_param",errmsg,exerrval);
       }
       else
       {
         exerrval = ncerr;
         sprintf(errmsg,
           "Error: failed to create element list for side set %d in file id %d",
                 side_set_id,exoid);
         ex_err("ex_put_side_set_param",errmsg,exerrval);
       }
       goto error_ret;            /* exit define mode and return */
     }

     if (ncvardef (exoid, VAR_SIDE_SS(side_set_id_ndx), NC_LONG, 1, dims) == -1)
     {
       if (ncerr == NC_ENAMEINUSE)
       {
         exerrval = ncerr;
         sprintf(errmsg,
                "Error: side list already exists for side set %d in file id %d",
                 side_set_id,exoid);
         ex_err("ex_put_side_set_param",errmsg,exerrval);
       }
       else
       {
         exerrval = ncerr;
         sprintf(errmsg,
              "Error: failed to create side list for side set %d in file id %d",
                 side_set_id,exoid);
         ex_err("ex_put_side_set_param",errmsg,exerrval);
       }
       goto error_ret;         /* exit define mode and return */

     }

   }

   if (num_dist_fact_in_set > 0)
   {
     if ((dimid = ncdimdef (exoid, DIM_NUM_DF_SS(side_set_id_ndx), 
                            (long)num_dist_fact_in_set)) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
  "Error: failed to define number of dist factors in side set %d in file id %d",
               side_set_id,exoid);
       ex_err("ex_put_side_set_param",errmsg,exerrval);
       goto error_ret;          /* exit define mode and return */
     }

/* create variable array in which to store the side set distribution factors
 */

     dims[0] = dimid;

     if (ncvardef (exoid, VAR_FACT_SS(side_set_id_ndx),
                       nc_flt_code(exoid), 1, dims) == -1)
     {
       if (ncerr == NC_ENAMEINUSE)
       {
         exerrval = ncerr;
         sprintf(errmsg,
        "Error: dist factors list already exists for side set %d in file id %d",
               side_set_id,exoid);
         ex_err("ex_put_side_set_param",errmsg,exerrval);
       }
       else
       {
         exerrval = ncerr;
         sprintf(errmsg,
      "Error: failed to create dist factors list for side set %d in file id %d",
               side_set_id,exoid);
         ex_err("ex_put_side_set_param",errmsg,exerrval);
       }
       goto error_ret;            /* exit define mode and return */
     }

   }

/* leave define mode  */

   if (ncendef (exoid) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to complete definition in file id %d", exoid);
     ex_err("ex_put_side_set_param",errmsg,exerrval);
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
         ex_err("ex_put_side_set_param",errmsg,exerrval);
       }
       return (EX_FATAL);
}
