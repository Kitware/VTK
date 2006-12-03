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
* expblk - ex_put_block: write edge, face, or element block parameters
*
* entry conditions - 
*   input parameters:
*       int     idexo                   exodus file id
*       int     blk_type                type of block (edge, face, or element)
*       int     blk_id                  block identifer
*       char*   entry_descrip           string describing shape of entries in the block
*       int     num_entries_this_blk    number of entries(records) in the block
*       int     num_nodes_per_entry     number of nodes per block entry
*       int     num_edges_per_entry     number of edges per block entry
*       int     num_faces_per_entry     number of faces per block entry
*       int     num_attr_per_entry      number of attributes per block entry
*
* exit conditions - 
*
* revision history - 
*   20061001 - David Thompson - Adapted from ex_put_elem_block
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"
#include <string.h>

/*!
 * writes the parameters used to describe an element block
 */

int ex_put_block( int         exoid,
                  int         blk_type,
                  int         blk_id,
                  const char* entry_descrip,
                  int         num_entries_this_blk,
                  int         num_nodes_per_entry,
                  int         num_edges_per_entry,
                  int         num_faces_per_entry,
                  int         num_attr_per_entry )
{
   int varid, dimid, dims[2], blk_id_ndx, blk_stat, strdim;
   long start[2], num_blk;
   nclong ldum;
   int cur_num_blk, numblkdim, numattrdim;
   int nnodperentdim, nedgperentdim = -1, nfacperentdim = -1;
   int connid, econnid, fconnid;
   char *cdum;
   char errmsg[MAX_ERR_LENGTH];
   const char* tname;
   const char* dnumblk;
   const char* vblkids;
   const char* vblksta;
   const char* vnodcon = 0;
   const char* vedgcon = 0;
   const char* vfaccon = 0;
   const char* vattnam = 0;
   const char* vblkatt = 0;
   const char* dneblk = 0;
   const char* dnape = 0;
   const char* dnnpe = 0;
   const char* dnepe = 0;
   const char* dnfpe = 0;
   struct list_item** ctr_list;

   exerrval  = 0; /* clear error code */

   cdum = 0;

   switch (blk_type) {
   case EX_EDGE_BLOCK:
     tname = "edge";
     dnumblk = DIM_NUM_ED_BLK;
     vblkids = VAR_ID_ED_BLK;
     vblksta = VAR_STAT_ED_BLK;
     ctr_list = &ed_ctr_list;
     break;
   case EX_FACE_BLOCK:
     tname = "face";
     dnumblk = DIM_NUM_FA_BLK;
     vblkids = VAR_ID_FA_BLK;
     vblksta = VAR_STAT_FA_BLK;
     ctr_list = &fa_ctr_list;
     break;
   case EX_ELEM_BLOCK:
     tname = "element";
     dnumblk = DIM_NUM_EL_BLK;
     vblkids = VAR_ID_EL_BLK;
     vblksta = VAR_STAT_EL_BLK;
     ctr_list = &eb_ctr_list;
     break;
   default:
     exerrval = EX_BADPARAM;
     sprintf( errmsg, "Error: Bad block type (%d) specified for file id %d",
       blk_type, exoid );
     ex_err( "ex_put_block", errmsg, exerrval );
     return (EX_FATAL);
   }

/* first check if any element blocks are specified */

   if ((dimid = (ncdimid (exoid, dnumblk))) == -1 )
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: no element blocks defined in file id %d",
             exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     return (EX_FATAL);
   }

/* Get number of element blocks defined for this file */
   if ((ncdiminq (exoid,dimid,cdum,&num_blk)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of element blocks in file id %d",
             exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     return (EX_FATAL);
   }

/* Next: Make sure that this is not a duplicate element block id by
         searching the vblkids array.
   WARNING: This must be done outside of define mode because id_lkup accesses
            the database to determine the position
*/

   if ((varid = ncvarid (exoid, vblkids)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to locate element block ids in file id %d", exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);

   }

   blk_id_ndx = ex_id_lkup(exoid,vblkids,blk_id);
   if (exerrval != EX_LOOKUPFAIL)   /* found the element block id */
   {
     exerrval = EX_FATAL;
     sprintf(errmsg,
            "Error: element block id %d already exists in file id %d",
             blk_id,exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     return (EX_FATAL);
   }

/* Keep track of the total number of element blocks defined using a counter 
   stored in a linked list keyed by exoid.
   NOTE: ex_get_file_item  is a function that finds the number of element 
         blocks for a specific file and returns that value incremented.
*/
   cur_num_blk=ex_get_file_item(exoid, ctr_list);
   if (cur_num_blk >= num_blk)
   {
     exerrval = EX_FATAL;
     sprintf(errmsg,
          "Error: exceeded number of element blocks (%ld) defined in file id %d",
             num_blk,exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     return (EX_FATAL);
   }


/*   NOTE: ex_get_file_item  is a function that finds the number of element
         blocks for a specific file and returns that value incremented. */

   cur_num_blk=ex_inc_file_item(exoid, ctr_list);
   start[0] = (long)cur_num_blk;

/* write out element block id to previously defined id array variable*/

   ldum = (nclong)blk_id;
   if (ncvarput1 (exoid, varid, start, &ldum) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store element block id to file id %d",
             exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     return (EX_FATAL);
   }

   blk_id_ndx = start[0]+1; /* element id index into vblkids array*/

   if (num_entries_this_blk == 0) /* Is this a NULL element block? */
     blk_stat = 0; /* change element block status to NULL */
   else
     blk_stat = 1; /* change element block status to TRUE */

   if ((varid = ncvarid (exoid, vblksta)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
           "Error: failed to locate element block status in file id %d", exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     return (EX_FATAL);
   }

   ldum = (nclong)blk_stat;
   if (ncvarput1 (exoid, varid, start, &ldum) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store element id %d status to file id %d",
             blk_id, exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (num_entries_this_blk == 0) /* Is this a NULL element block? */
   {
     return(EX_NOERR);
   }


   /*
    * Check that storage required for connectivity array is less
    * than 2GB which is maximum size permitted by netcdf
    * (in large file mode). 1<<29 == max number of integer items.
    */
   if (num_entries_this_blk * num_nodes_per_entry  > (1<<29)) {
     exerrval = EX_BADPARAM;
     sprintf(errmsg,
             "Error: Size to store connectivity for element block %d exceeds 2GB in file id %d",
             blk_id, exoid);
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


   switch (blk_type) {
   case EX_EDGE_BLOCK:
     dneblk = DIM_NUM_ED_IN_EBLK(blk_id_ndx);
     dnnpe = DIM_NUM_NOD_PER_ED(blk_id_ndx);
     dnepe = 0;
     dnfpe = 0;
     dnape = DIM_NUM_ATT_IN_EBLK(blk_id_ndx);
     vblkatt = VAR_EATTRIB(blk_id_ndx);
     vattnam = VAR_NAME_EATTRIB(blk_id_ndx);
     vnodcon = VAR_EBCONN(blk_id_ndx);
     vedgcon = 0;
     vfaccon = 0;
     break;
   case EX_FACE_BLOCK:
     dneblk = DIM_NUM_FA_IN_FBLK(blk_id_ndx);
     dnnpe = DIM_NUM_NOD_PER_FA(blk_id_ndx);
     dnepe = 0;
     dnfpe = 0;
     dnape = DIM_NUM_ATT_IN_FBLK(blk_id_ndx);
     vblkatt = VAR_FATTRIB(blk_id_ndx);
     vattnam = VAR_NAME_FATTRIB(blk_id_ndx);
     vnodcon = VAR_FBCONN(blk_id_ndx);
     vedgcon = 0;
     vfaccon = 0;
     break;
   case EX_ELEM_BLOCK:
     dneblk = DIM_NUM_EL_IN_BLK(blk_id_ndx);
     dnnpe = DIM_NUM_NOD_PER_EL(blk_id_ndx);
     dnepe = DIM_NUM_EDG_PER_EL(blk_id_ndx);
     dnfpe = DIM_NUM_FAC_PER_EL(blk_id_ndx);
     dnape = DIM_NUM_ATT_IN_BLK(blk_id_ndx);
     vblkatt = VAR_ATTRIB(blk_id_ndx);
     vattnam = VAR_NAME_ATTRIB(blk_id_ndx);
     vnodcon = VAR_CONN(blk_id_ndx);
     vedgcon = VAR_ECONN(blk_id_ndx);
     vfaccon = VAR_FCONN(blk_id_ndx);
     break;
   }
/* define some dimensions and variables*/

   if ((numblkdim = ncdimdef (exoid,dneblk,(long)num_entries_this_blk)) == -1)
   {
     if (ncerr == NC_ENAMEINUSE)        /* duplicate entry */
     {
       exerrval = ncerr;
       sprintf(errmsg,
             "Error: element block %d already defined in file id %d",
              blk_id,exoid);
       ex_err("ex_put_elem_block",errmsg,exerrval);
     }
     else
     {
       exerrval = ncerr;
       sprintf(errmsg,
    "Error: failed to define number of elements/block for block %d file id %d",
              blk_id,exoid);
       ex_err("ex_put_elem_block",errmsg,exerrval);
     }
     goto error_ret;         /* exit define mode and return */
   }

   if ((nnodperentdim = ncdimdef (exoid,dnnpe,(long)num_nodes_per_entry)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
   "Error: failed to define number of nodes/element for block %d in file id %d",
             blk_id,exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     goto error_ret;         /* exit define mode and return */
   }

   if ( dnepe && num_edges_per_entry > 0 ) {
     if ((nedgperentdim = ncdimdef (exoid,dnepe,(long)num_edges_per_entry)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to define number of edges/element for block %d in file id %d",
         blk_id,exoid);
       ex_err("ex_put_elem_block",errmsg,exerrval);
       goto error_ret;         /* exit define mode and return */
     }
   }

   if ( dnfpe && num_faces_per_entry > 0 ) {
     if ((nfacperentdim = ncdimdef (exoid,dnfpe,(long)num_faces_per_entry)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to define number of faces/element for block %d in file id %d",
         blk_id,exoid);
       ex_err("ex_put_elem_block",errmsg,exerrval);
       goto error_ret;         /* exit define mode and return */
     }
   }

/* element attribute array */

   if (num_attr_per_entry > 0)
   {

     if ((numattrdim = ncdimdef (exoid, dnape, (long)num_attr_per_entry)) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
      "Error: failed to define number of attributes in block %d in file id %d",
               blk_id,exoid);
       ex_err("ex_put_elem_block",errmsg,exerrval);
       goto error_ret;         /* exit define mode and return */
     }

     dims[0] = numblkdim;
     dims[1] = numattrdim;

     if ((ncvardef (exoid, vblkatt, nc_flt_code(exoid), 2, dims)) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
       "Error:  failed to define attributes for element block %d in file id %d",
               blk_id,exoid);
       ex_err("ex_put_elem_block",errmsg,exerrval);
       goto error_ret;         /* exit define mode and return */
     }

     /* inquire previously defined dimensions  */
     if ((strdim = ncdimid (exoid, DIM_STR)) < 0) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to get string length in file id %d",exoid);
       ex_err("ex_put_elem_block",errmsg,exerrval);
       return (EX_FATAL);
     }
     
     /* Attribute names... */
     dims[0] = numattrdim;
     dims[1] = strdim;
      
     if (ncvardef (exoid, vattnam, NC_CHAR, 2, dims) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to define element attribute name array in file id %d",exoid);
       ex_err("ex_put_elem_block",errmsg,exerrval);
       goto error_ret;         /* exit define mode and return */
     }
     
   }

/* element connectivity array */

   dims[0] = numblkdim;
   dims[1] = nnodperentdim;

   if ((connid = ncvardef (exoid, vnodcon, NC_LONG, 2, dims)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
        "Error: failed to create connectivity array for block %d in file id %d",
             blk_id,exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     goto error_ret;         /* exit define mode and return */
   }

/* store element type as attribute of connectivity variable */

   if ((ncattput (exoid, connid, ATT_NAME_ELB, NC_CHAR, strlen(entry_descrip)+1, 
             (void*) entry_descrip)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store element type name %s in file id %d",
             entry_descrip,exoid);
     ex_err("ex_put_elem_block",errmsg,exerrval);
     goto error_ret;         /* exit define mode and return */
   }

   if ( vedgcon && num_edges_per_entry ) {
     dims[0] = numblkdim;
     dims[1] = nedgperentdim;

     if ((econnid = ncvardef (exoid, vedgcon, NC_LONG, 2, dims)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to create edge connectivity array for block %d in file id %d",
         blk_id,exoid);
       ex_err("ex_put_elem_block",errmsg,exerrval);
       goto error_ret;         /* exit define mode and return */
     }
   }

   if ( vfaccon && num_faces_per_entry ) {
     dims[0] = numblkdim;
     dims[1] = nfacperentdim;

     if ((fconnid = ncvardef (exoid, vfaccon, NC_LONG, 2, dims)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to create face connectivity array for block %d in file id %d",
         blk_id,exoid);
       ex_err("ex_put_elem_block",errmsg,exerrval);
       goto error_ret;         /* exit define mode and return */
     }
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

