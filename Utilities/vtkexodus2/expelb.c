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
* expelb - ex_put_elem_block: write element block parameters
*
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          James A. Schutt - 8 byte float and standard C definitions
*          Vic Yarberry    - Added headers and error logging
*
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     idexo                   exodus file id
*       char*   elem_type               element type string
*       int     num_elem_this_blk       number of elements in the element blk
*       int     num_nodes_per_elem      number of nodes per element block
*       int     num_attr                number of attributes
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
#include <string.h>

/*
 * writes the parameters used to describe an element block
 */

int ex_put_elem_block (int   exoid,
                       int   elem_blk_id,
                       const char *elem_type,
                       int   num_elem_this_blk,
                       int   num_nodes_per_elem,
                       int   num_attr)
{
   int varid, dimid, dims[2], elem_blk_id_ndx, elem_blk_stat;
   long start[2], num_elem_blk;
   nclong ldum;
   int cur_num_elem_blk, nelnoddim, numelbdim, numattrdim, connid;
   char *cdum;
   char errmsg[MAX_ERR_LENGTH];

   exerrval  = 0; /* clear error code */

   cdum = 0;

/* first check if any element blocks are specified */

   if ((dimid = (ncdimid (exoid, DIM_NUM_EL_BLK))) == -1 )
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: no element blocks defined in file id %d",
             exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     return (EX_FATAL);
   }

/* Get number of element blocks defined for this file */
   if ((ncdiminq (exoid,dimid,cdum,&num_elem_blk)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of element blocks in file id %d",
             exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     return (EX_FATAL);
   }

/* Next: Make sure that this is not a duplicate element block id by
         searching the VAR_ID_EL_BLK array.
   WARNING: This must be done outside of define mode because id_lkup accesses
            the database to determine the position
*/

   if ((varid = ncvarid (exoid, VAR_ID_EL_BLK)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to locate element block ids in file id %d", exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     return (EX_FATAL);
   }

   elem_blk_id_ndx = ex_id_lkup(exoid,VAR_ID_EL_BLK,elem_blk_id);
   if (exerrval != EX_LOOKUPFAIL)   /* found the element block id */
   {
     exerrval = EX_FATAL;
     sprintf(errmsg,
            "Error: element block id %d already exists in file id %d",
             elem_blk_id,exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     return (EX_FATAL);
   }

/* Keep track of the total number of element blocks defined using a counter 
   stored in a linked list keyed by exoid.
   NOTE: ex_get_file_item  is a function that finds the number of element 
         blocks for a specific file and returns that value incremented.
*/
   cur_num_elem_blk=ex_get_file_item(exoid, &eb_ctr_list);
   if (cur_num_elem_blk >= num_elem_blk)
   {
     exerrval = EX_FATAL;
     sprintf(errmsg,
          "Error: exceeded number of element blocks (%ld) defined in file id %d",
             num_elem_blk,exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     return (EX_FATAL);
   }


/*   NOTE: ex_get_file_item  is a function that finds the number of element
         blocks for a specific file and returns that value incremented. */

   cur_num_elem_blk=ex_inc_file_item(exoid, &eb_ctr_list);
   start[0] = (long)cur_num_elem_blk;

/* write out element block id to previously defined id array variable*/

   ldum = (nclong)elem_blk_id;
   if (ncvarput1 (exoid, varid, start, &ldum) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store element block id to file id %d",
             exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     return (EX_FATAL);
   }

   elem_blk_id_ndx = start[0]+1; /* element id index into VAR_ID_EL_BLK array*/

   if (num_elem_this_blk == 0) /* Is this a NULL element block? */
     elem_blk_stat = 0; /* change element block status to NULL */
   else
     elem_blk_stat = 1; /* change element block status to TRUE */

   if ((varid = ncvarid (exoid, VAR_STAT_EL_BLK)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
           "Error: failed to locate element block status in file id %d", exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     return (EX_FATAL);
   }

   ldum = (nclong)elem_blk_stat;
   if (ncvarput1 (exoid, varid, start, &ldum) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store element id %d status to file id %d",
             elem_blk_id, exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (num_elem_this_blk == 0) /* Is this a NULL element block? */
   {
     return(EX_NOERR);
   }


   /*
    * Check that storage required for connectivity array is less
    * than 2GB which is maximum size permitted by netcdf
    * (in large file mode). 1<<29 == max number of integer items.
    */
   if (num_elem_this_blk * num_nodes_per_elem  > (1<<29)) {
     exerrval = EX_BADPARAM;
     sprintf(errmsg,
             "Error: Size to store connectivity for element block %d exceeds 2GB in file id %d",
             elem_blk_id, exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     return (EX_FATAL);
   }

   /* put netcdf file into define mode  */
   if (ncredef (exoid) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,"Error: failed to place file id %d into define mode",exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     return (EX_FATAL);
   }


/* define some dimensions and variables*/

   if ((numelbdim = ncdimdef (exoid,
        DIM_NUM_EL_IN_BLK(elem_blk_id_ndx), (long)num_elem_this_blk)) == -1)
   {
     if (ncerr == NC_ENAMEINUSE)        /* duplicate entry */
     {
       exerrval = ncerr;
       sprintf(errmsg,
             "Error: element block %d already defined in file id %d",
              elem_blk_id,exoid);
       ex_err("ex_put_elem_block",errmsg,exerrval);
     }
     else
     {
       exerrval = ncerr;
       sprintf(errmsg,
    "Error: failed to define number of elements/block for block %d file id %d",
              elem_blk_id,exoid);
       ex_err("ex_put_elem_block",errmsg,exerrval);
     }
     goto error_ret;         /* exit define mode and return */
   }

   if ((nelnoddim = ncdimdef (exoid,
        DIM_NUM_NOD_PER_EL(elem_blk_id_ndx), (long)num_nodes_per_elem)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
   "Error: failed to define number of nodes/element for block %d in file id %d",
             elem_blk_id,exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     goto error_ret;         /* exit define mode and return */
   }

/* element attribute array */

   if (num_attr > 0)
   {

     if ((numattrdim = ncdimdef (exoid, 
                DIM_NUM_ATT_IN_BLK(elem_blk_id_ndx), (long)num_attr)) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
      "Error: failed to define number of attributes in block %d in file id %d",
               elem_blk_id,exoid);
       ex_err("ex_put_elem_block",errmsg,exerrval);
       goto error_ret;         /* exit define mode and return */
     }

     dims[0] = numelbdim;
     dims[1] = numattrdim;

     if ((ncvardef (exoid, 
                VAR_ATTRIB(elem_blk_id_ndx), nc_flt_code(exoid), 2, dims)) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
       "Error:  failed to define attributes for element block %d in file id %d",
               elem_blk_id,exoid);
       ex_err("ex_put_elem_block",errmsg,exerrval);
       goto error_ret;         /* exit define mode and return */
     }
   }

/* element connectivity array */

   dims[0] = numelbdim;
   dims[1] = nelnoddim;

   if ((connid = ncvardef (exoid, 
                VAR_CONN(elem_blk_id_ndx), NC_LONG, 2, dims)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
        "Error: failed to create connectivity array for block %d in file id %d",
             elem_blk_id,exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     goto error_ret;         /* exit define mode and return */
   }

/* store element type as attribute of connectivity variable */

   if ((ncattput (exoid, connid, ATT_NAME_ELB, NC_CHAR, strlen(elem_type)+1, 
             (void*) elem_type)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store element type name %s in file id %d",
             elem_type,exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     goto error_ret;         /* exit define mode and return */
   }
 

/* leave define mode  */

   if (ncendef (exoid) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
       "Error: failed to complete element block definition in file id %d", 
        exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
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
         ex_err("ex_put_elem_block",errmsg,exerrval);
       }
       return (EX_FATAL);
}

