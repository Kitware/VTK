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
/*  Id */

#include "exodusII.h"
#include "exodusII_int.h"
#include <stdlib.h> /* for free() */

/*!
 * writes the connectivity array for a block
 */

int ex_put_conn (int   exoid,
                 int   blk_type,
                 int   blk_id,
                 const int  *node_conn,
                 const int  *elem_edge_conn,
                 const int  *elem_face_conn)
{
   int numelbdim=-1, nelnoddim=-1, connid=-1, blk_id_ndx, iresult;
   char* var_id_blk;
   long num_entry_this_blk, num_id_per_entry, start[2], count[2]; 
   nclong *lptr;
   char errmsg[MAX_ERR_LENGTH];
   const char* blk_typename;

   exerrval = 0; /* clear error code */

   switch (blk_type) {
   case EX_ELEM_BLOCK:
     /* Determine index of elem_blk_id in VAR_ID_EL_BLK array */
     var_id_blk = VAR_ID_EL_BLK;
     blk_typename = "element";
     break;
   case EX_FACE_BLOCK:
     var_id_blk = VAR_ID_FA_BLK;
     blk_typename = "face";
     break;
   case EX_EDGE_BLOCK:
     var_id_blk = VAR_ID_ED_BLK;
     blk_typename = "edge";
     break;
   default:
     sprintf(errmsg,"Error: Invalid block type %d passed for file id %d",
             blk_type,exoid);
     ex_err("ex_put_conn",errmsg,EX_MSG);
     return (EX_FATAL);
   }

   blk_id_ndx = ex_id_lkup(exoid,var_id_blk,blk_id);
   if (exerrval != 0) 
     {
     if (exerrval == EX_NULLENTITY)
       {
       sprintf(errmsg,
         "Warning: connectivity array not allowed for NULL %s block %d in file id %d",
         blk_typename,blk_id,exoid);
       ex_err("ex_put_conn",errmsg,EX_MSG);
       return (EX_WARN);
       }
     else
       {
       sprintf(errmsg,
         "Error: failed to locate %s block id %d in %s array in file id %d",
         blk_typename,blk_id,var_id_blk, exoid);
       ex_err("ex_put_conn",errmsg,exerrval);
       return (EX_FATAL);
       }
     }

/* inquire id's of previously defined dimensions  */

   switch (blk_type) {
   case EX_ELEM_BLOCK:
     /* Determine index of elem_blk_id in VAR_ID_EL_BLK array */
     numelbdim = ncdimid (exoid, DIM_NUM_EL_IN_BLK(blk_id_ndx));
     break;
   case EX_FACE_BLOCK:
     numelbdim = ncdimid (exoid, DIM_NUM_FA_IN_FBLK(blk_id_ndx));
     break;
   case EX_EDGE_BLOCK:
     numelbdim = ncdimid (exoid, DIM_NUM_ED_IN_EBLK(blk_id_ndx));
     break;
   }
   if (numelbdim == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
     "Error: failed to locate number of %ss in block %d in file id %d",
              blk_typename,blk_id,exoid);
     ex_err("ex_put_conn",errmsg, exerrval);
     return(EX_FATAL);
   }

   if (ncdiminq(exoid, numelbdim, NULL, &num_entry_this_blk) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of elements in block %d in file id %d",
             blk_id,exoid);
     ex_err("ex_put_conn",errmsg,exerrval);
     return(EX_FATAL);
   }


   switch (blk_type) {
   case EX_ELEM_BLOCK:
     nelnoddim = ncdimid (exoid, DIM_NUM_NOD_PER_EL(blk_id_ndx));
     break;
   case EX_FACE_BLOCK:
     nelnoddim = ncdimid (exoid, DIM_NUM_NOD_PER_FA(blk_id_ndx));
     break;
   case EX_EDGE_BLOCK:
     nelnoddim = ncdimid (exoid, DIM_NUM_NOD_PER_ED(blk_id_ndx));
     break;
   }
   if (nelnoddim == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
       "Error: failed to locate number of nodes/%s in block %d in file id %d",
             blk_typename,blk_id,exoid);
     ex_err("ex_put_conn",errmsg,exerrval);
     return(EX_FATAL);
   }

   if (ncdiminq (exoid, nelnoddim, NULL, &num_id_per_entry) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
       "Error: failed to get number of nodes/elem in block %d in file id %d",
             blk_id,exoid);
     ex_err("ex_put_conn",errmsg,exerrval);
     return(EX_FATAL);
   }


   switch (blk_type) {
   case EX_ELEM_BLOCK:
     connid = ncvarid (exoid, VAR_CONN(blk_id_ndx));
     break;
   case EX_FACE_BLOCK:
     connid = ncvarid (exoid, VAR_FBCONN(blk_id_ndx));
     break;
   case EX_EDGE_BLOCK:
     connid = ncvarid (exoid, VAR_EBCONN(blk_id_ndx));
     break;
   }
   if (connid == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
"Error: failed to locate connectivity array for %s block %d in file id %d",
             blk_typename,blk_id,exoid);
     ex_err("ex_put_conn",errmsg, exerrval);
     return(EX_FATAL);
   }


/* write out the connectivity array */

/* this contortion is necessary because netCDF is expecting nclongs; fortunately
   it's necessary only when ints and nclongs aren't the same size */
#define EX_WRITE_CONN(TNAME,ARRDIM0,ARRDIM1,VARCONN,VARCONNVAL) \
   start[0] = 0; \
   start[1] = 0; \
 \
   count[0] = (ARRDIM0); \
   count[1] = (ARRDIM1); \
 \
   if (sizeof(int) == sizeof(nclong)) { \
      iresult = ncvarput (exoid, VARCONN, start, count, VARCONNVAL); \
   } else { \
      lptr = itol (VARCONNVAL, (int)((ARRDIM0)*(ARRDIM1))); \
      iresult = ncvarput (exoid, VARCONN, start, count, lptr); \
      free(lptr); \
   } \
 \
   if (iresult == -1) \
   { \
      exerrval = ncerr; \
      sprintf(errmsg, \
      "Error: failed to write connectivity array for %s block %d in file id %d", \
                TNAME,blk_id,exoid); \
      ex_err("ex_put_conn",errmsg, exerrval); \
      return(EX_FATAL); \
   }

   EX_WRITE_CONN(blk_typename,num_entry_this_blk,num_id_per_entry,connid,node_conn);

   /* If there are edge and face connectivity arrays that belong with the element
    * block, write them now. Warn if they are required but not specified or
    * specified but not required.
    */
   if ( blk_type == EX_ELEM_BLOCK ) {
     int nedpereldim, nfapereldim;
     long num_ed_per_elem, num_fa_per_elem;

     nedpereldim = ncdimid (exoid, DIM_NUM_EDG_PER_EL(blk_id_ndx));
     if (nedpereldim == -1 && elem_edge_conn != 0)
       {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: edge connectivity specified but failed to "
         "locate number of edges/element in block %d in file id %d",
         blk_id,exoid);
       ex_err("ex_put_conn",errmsg,exerrval);
       return(EX_FATAL);
       }

     nfapereldim = ncdimid (exoid, DIM_NUM_FAC_PER_EL(blk_id_ndx));
     if (nfapereldim == -1 && elem_face_conn != 0)
       {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: face connectivity specified but failed to "
         "locate number of faces/element in block %d in file id %d",
         blk_id,exoid);
       ex_err("ex_put_conn",errmsg,exerrval);
       return(EX_FATAL);
       }

     if (ncdiminq (exoid, nedpereldim, NULL, &num_ed_per_elem) == -1)
       {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to get number of edges/elem in block %d in file id %d",
         blk_id,exoid);
       ex_err("ex_put_conn",errmsg,exerrval);
       return(EX_FATAL);
       }

     if (ncdiminq (exoid, nfapereldim, NULL, &num_fa_per_elem) == -1)
       {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to get number of edges/elem in block %d in file id %d",
         blk_id,exoid);
       ex_err("ex_put_conn",errmsg,exerrval);
       return(EX_FATAL);
       }

     if ( (num_ed_per_elem == 0 && elem_edge_conn != 0) ||
          (num_ed_per_elem != 0 && elem_edge_conn == 0) )
       {
       exerrval = (EX_FATAL);
       sprintf(errmsg,
         "Error: number of edges per element (%ld) doesn't "
         "agree with elem_edge_conn (0x%p)",
         num_ed_per_elem, elem_edge_conn );
       ex_err("ex_put_conn",errmsg,exerrval);
       return (EX_FATAL);
       }

     if ( (num_fa_per_elem == 0 && elem_face_conn != 0) ||
          (num_fa_per_elem != 0 && elem_face_conn == 0) )
       {
       exerrval = (EX_FATAL);
       sprintf(errmsg,
         "Error: number of faces per element (%ld) doesn't "
         "agree with elem_face_conn (0x%p)",
         num_fa_per_elem, elem_face_conn );
       ex_err("ex_put_conn",errmsg,exerrval);
       return (EX_FATAL);
       }

     if ( num_ed_per_elem != 0 ) {
       connid = ncvarid (exoid, VAR_ECONN(blk_id_ndx));
       if (connid == -1)
         {
         exerrval = ncerr;
         sprintf(errmsg,
           "Error: failed to locate connectivity array for "
           "element edge block %d in file id %d",
           blk_id,exoid);
         ex_err("ex_put_conn",errmsg, exerrval);
         return(EX_FATAL);
         }
       EX_WRITE_CONN("element edge",num_entry_this_blk,num_ed_per_elem,connid,elem_edge_conn);
     }

     if ( num_fa_per_elem != 0 ) {
       connid = ncvarid (exoid, VAR_FCONN(blk_id_ndx));
       if (connid == -1)
         {
         exerrval = ncerr;
         sprintf(errmsg,
           "Error: failed to locate connectivity array for "
           "element face block %d in file id %d",
           blk_id,exoid);
         ex_err("ex_put_conn",errmsg, exerrval);
         return(EX_FATAL);
         }
       EX_WRITE_CONN("element face",num_entry_this_blk,num_fa_per_elem,connid,elem_face_conn);
     }
   }

   return (EX_NOERR);

}
