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
* exgelb - read element block parameters
*
* author - Victor R. Yarberry, Sandia National Laboratories
*
* environment - UNIX
*
* entry conditions -
*   input parameters:
*       int     idexo                   exodus file id
*       int     elem_blk_id             element block id
*
* exit conditions -
*       char*   elem_type               element type name
*       int*    num_elem_this_blk       number of elements in this element block
*       int*    num_nodes_per_elem      number of nodes per element block
*       int*    num_attr                number of attributes
*
* revision history -
*
*  Id
*
*/

#include <string.h>
#include <stdio.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the parameters used to describe an element block
 */

int ex_get_elem_block (int   exoid,
                       int   elem_blk_id,
                       char *elem_type,
                       int  *num_elem_this_blk, 
                       int  *num_nodes_per_elem,
                       int  *num_attr)

{
   int dimid, connid, len, elem_blk_id_ndx;
   long lnum_elem_this_blk, lnum_nodes_per_elem, lnum_attr;
   char *ptr;
   char  errmsg[MAX_ERR_LENGTH];
   nc_type dummy;

   exerrval = 0;

/* First, locate index of element block id in VAR_ID_EL_BLK array */

   elem_blk_id_ndx = ex_id_lkup(exoid,VAR_ID_EL_BLK,elem_blk_id);
   if (exerrval != 0) 
   {
     if (exerrval == EX_NULLENTITY)     /* NULL element block?    */
     {
       strcpy(elem_type, "NULL");       /* NULL element type name */
       *num_elem_this_blk = 0;          /* no elements            */
       *num_nodes_per_elem = 0;         /* no nodes               */
       *num_attr = 0;                   /* no attributes          */
       return (EX_NOERR);
     }
     else
     {
       sprintf(errmsg,
        "Error: failed to locate element block id %d in %s array in file id %d",
               elem_blk_id,VAR_ID_EL_BLK,exoid);
       ex_err("ex_get_elem_block",errmsg,exerrval);
       return (EX_FATAL);
     }
   }

/* inquire values of some dimensions */

   if ((dimid = ncdimid (exoid, DIM_NUM_EL_IN_BLK(elem_blk_id_ndx))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
         "Error: failed to locate number of elements in block %d in file id %d",
             elem_blk_id,exoid);
     ex_err("ex_get_elem_block",errmsg, exerrval);
     return(EX_FATAL);
   }

   if (ncdiminq (exoid, dimid, (char *) 0, &lnum_elem_this_blk) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of elements in block %d in file id %d",
             elem_blk_id, exoid);
     ex_err("ex_get_elem_block",errmsg, exerrval);
     return(EX_FATAL);
   }
   *num_elem_this_blk = lnum_elem_this_blk;
   if ((dimid = ncdimid (exoid, DIM_NUM_NOD_PER_EL(elem_blk_id_ndx))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
    "Error: failed to locate number of nodes/element in block %d in file id %d",
             elem_blk_id,exoid);
     ex_err("ex_get_elem_block",errmsg, exerrval);
     return(EX_FATAL);
   }
   if (ncdiminq (exoid, dimid, (char *) 0, &lnum_nodes_per_elem) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
       "Error: failed to get number of nodes/element in block %d in file id %d",
             elem_blk_id, exoid);
     ex_err("ex_get_elem_block",errmsg, exerrval);
     return(EX_FATAL);
   }
   *num_nodes_per_elem = lnum_nodes_per_elem;

   if ((dimid = ncdimid (exoid, DIM_NUM_ATT_IN_BLK(elem_blk_id_ndx))) == -1)
      *num_attr = 0;            /* dimension is undefined */
   else
   {
     if (ncdiminq (exoid, dimid, (char *) 0, &lnum_attr) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
          "Error: failed to get number of attributes in block %d in file id %d",
               elem_blk_id, exoid);
       ex_err("ex_get_elem_block",errmsg, exerrval);
       return(EX_FATAL);
     }
     *num_attr = lnum_attr;
   }

   /* look up connectivity array for this element block id */

   if ((connid = ncvarid (exoid, VAR_CONN(elem_blk_id_ndx))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
"Error: failed to locate connectivity array for element block %d in file id %d",
             elem_blk_id,exoid);
     ex_err("ex_get_elem_block",errmsg, exerrval);
     return(EX_FATAL);
   }

   if (ncattinq (exoid, connid, ATT_NAME_ELB, &dummy, &len) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
    "Error: failed to get element block %d type in file id %d",
             elem_blk_id,exoid);
     ex_err("ex_get_elem_block",errmsg, exerrval);
     return(EX_FATAL);
   }

   if (len > (MAX_STR_LENGTH+1))
   {
     len = MAX_STR_LENGTH;
     sprintf (errmsg,
             "Warning: element block %d type will be truncated to %d chars", 
              elem_blk_id,len);
     ex_err("ex_get_elem_block",errmsg,EX_MSG);
   }
/* get the element type name */

   if (ncattget (exoid, connid, ATT_NAME_ELB, elem_type) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,"Error: failed to get element block %d type in file id %d",
              elem_blk_id, exoid);
     ex_err("ex_get_elem_block",errmsg, exerrval);
     return(EX_FATAL);
   }

/* get rid of trailing blanks */
   ptr = elem_type;
   /* fprintf(stderr,"[exgelb] %s, len: %d\n",ptr,len); */
   while (ptr < elem_type + len && *ptr != ' ')
   {
     ptr++;
   }
   *(ptr) = '\0';

   return (EX_NOERR);
}
