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
* exgsp - ex_get_side_set_param
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
*
* exit conditions - 
*       int*    num_side_in_set         number of sides in the side set
*       int*    num_dist_fact_in_set    number of distribution factors in the 
*                                       side set
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the number of sides and the number of distribution factors which 
 * describe a single side set
 */

int ex_get_side_set_param (int  exoid,
                           int  side_set_id,
                           int *num_side_in_set, 
                           int *num_dist_fact_in_set)
{
   int dimid, side_set_id_ndx;
   long lnum_side_in_set, lnum_dist_fact_in_set;
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

/* first check if any side sets are specified */

   if ((dimid = ncdimid (exoid, DIM_NUM_SS)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Warning: no side sets stored in file id %d",
             exoid);
     ex_err("ex_get_side_set_param",errmsg,exerrval);
     return (EX_WARN);
   }

/* Lookup index of side set id in VAR_SS_IDS array */

   side_set_id_ndx = ex_id_lkup(exoid,VAR_SS_IDS,side_set_id);
   if (exerrval != 0) 
   {
     if (exerrval == EX_NULLENTITY)     /* NULL side set? */
     {
       *num_side_in_set = 0;
       *num_dist_fact_in_set = 0;
       sprintf(errmsg,
              "Warning: side set %d is NULL in file id %d",
               side_set_id, exoid);
       ex_err("ex_get_side_set_param",errmsg,exerrval);
       return (EX_WARN);
     }
     else
     {

       sprintf(errmsg,
              "Error: failed to locate side set id %d in %s in file id %d",
               side_set_id,VAR_SS_IDS,exoid);
       ex_err("ex_get_side_set_param",errmsg,exerrval);
       return (EX_FATAL);
     }
   }


/* inquire values of dimensions */

   if ((dimid = ncdimid (exoid, DIM_NUM_SIDE_SS(side_set_id_ndx))) == -1)
   {
     *num_side_in_set = 0;
     exerrval = ncerr;
     sprintf(errmsg,
       "Error: failed to locate number of sides in side set %d in file id %d",
             side_set_id, exoid);
     ex_err("ex_get_side_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }
   else
   {
     if (ncdiminq (exoid, dimid, (char *) 0, &lnum_side_in_set) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: failed to get number of side sets in file id %d",
               exoid);
       ex_err("ex_get_side_set_param",errmsg,exerrval);
       return (EX_FATAL);
     }
     *num_side_in_set = lnum_side_in_set;
   }


   if ((dimid = ncdimid (exoid, DIM_NUM_DF_SS(side_set_id_ndx))) == -1)
   {
     *num_dist_fact_in_set = 0; /* no distribution factors for this side set*/
     if (ncerr == NC_EBADDIM)
       return (EX_NOERR);
     else
     {
       exerrval = ncerr;
       sprintf(errmsg,
  "Error: failed to locate number of dist factors in side set %d in file id %d",
               side_set_id, exoid);
       ex_err("ex_get_side_set_param",errmsg,exerrval);
       return (EX_FATAL);
     }
   }
   else
   {
     if (ncdiminq (exoid, dimid, (char *) 0, &lnum_dist_fact_in_set) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to get number of dist factors in side set %d in file id %d",
               side_set_id, exoid);
       ex_err("ex_get_side_set_param",errmsg,exerrval);
       return (EX_FATAL);
     }
     *num_dist_fact_in_set = lnum_dist_fact_in_set;
   }


   return (EX_NOERR);

}
