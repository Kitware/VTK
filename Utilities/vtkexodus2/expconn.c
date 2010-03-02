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

/*! write out the connectivity array */
#define EX_WRITE_CONN(TNAME,VARCONN,VARCONNVAL) \
   status = nc_put_var_int(exoid, VARCONN, VARCONNVAL); \
   if (status != NC_NOERR) { \
      exerrval = status; \
      sprintf(errmsg, \
      "Error: failed to write connectivity array for %s block %d in file id %d", \
                TNAME,blk_id,exoid); \
      ex_err("ex_put_conn",errmsg, exerrval); \
      return(EX_FATAL); \
   }

/*!
 * writes the connectivity array for a block
 * \param exoid           exodus file id
 * \param blk_type        type of block
 * \param blk_id          id of block
 * \param node_conn       node-element connectivity
 * \param elem_edge_conn  element-edge connectivity (NULL if none)
 * \param elem_face_conn  element-face connectivity (NULL if none)
 */

int ex_put_conn (int   exoid,
                 ex_entity_type blk_type,
                 int   blk_id,
                 const int  *node_conn,
                 const int  *elem_edge_conn,
                 const int  *elem_face_conn)
{
   int connid=-1, blk_id_ndx, status;
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

   blk_id_ndx = ex_id_lkup(exoid,blk_type,blk_id);
   if (exerrval != 0) 
     {
     if (exerrval == EX_NULLENTITY)
       {
       sprintf(errmsg,
         "Warning: connectivity array not allowed for NULL %s block %d in file id %d",
               ex_name_of_object(blk_type),blk_id,exoid);
       ex_err("ex_put_conn",errmsg,EX_MSG);
       return (EX_WARN);
       }
     else
       {
       sprintf(errmsg,
         "Error: failed to locate %s block id %d in id array in file id %d",
         ex_name_of_object(blk_type),blk_id, exoid);
       ex_err("ex_put_conn",errmsg,exerrval);
       return (EX_FATAL);
       }
     }

/* inquire id's of previously defined dimensions  */
   if (node_conn) {
     switch (blk_type) {
     case EX_ELEM_BLOCK:
       status = nc_inq_varid (exoid, VAR_CONN(blk_id_ndx), &connid);
       break;
     case EX_FACE_BLOCK:
       status = nc_inq_varid (exoid, VAR_FBCONN(blk_id_ndx), &connid);
       break;
     case EX_EDGE_BLOCK:
       status = nc_inq_varid (exoid, VAR_EBCONN(blk_id_ndx), &connid);
       break;
     default:
       exerrval = 1005;
       sprintf(errmsg,
               "Internal Error: unrecognized block type in switch: %d in file id %d",
               blk_type,exoid);
       ex_err("ex_putt_conn",errmsg,EX_MSG);
       return (EX_FATAL);
     }
     if (status != NC_NOERR) {
       exerrval = status;
       sprintf(errmsg,
               "Error: failed to locate connectivity array for %s block %d in file id %d",
               ex_name_of_object(blk_type),blk_id,exoid);
       ex_err("ex_put_conn",errmsg, exerrval);
       return(EX_FATAL);
     }
     
     EX_WRITE_CONN(ex_name_of_object(blk_type),connid,node_conn);
   }

   /* If there are edge and face connectivity arrays that belong with the element
    * block, write them now. Warn if they are required but not specified or
    * specified but not required.
    */
   if ( blk_type == EX_ELEM_BLOCK ) {
     int nedpereldim, nfapereldim;
     size_t num_ed_per_elem, num_fa_per_elem;

     status = nc_inq_dimid (exoid, DIM_NUM_EDG_PER_EL(blk_id_ndx), &nedpereldim);
     if (status != NC_NOERR && elem_edge_conn != 0)
       {
       exerrval = status;
       sprintf(errmsg,
         "Error: edge connectivity specified but failed to "
         "locate number of edges/element in block %d in file id %d",
         blk_id,exoid);
       ex_err("ex_put_conn",errmsg,exerrval);
       return(EX_FATAL);
       }

     status = nc_inq_dimid (exoid, DIM_NUM_FAC_PER_EL(blk_id_ndx), &nfapereldim);
     if (status != NC_NOERR && elem_face_conn != 0)
       {
       exerrval = status;
       sprintf(errmsg,
         "Error: face connectivity specified but failed to "
         "locate number of faces/element in block %d in file id %d",
         blk_id,exoid);
       ex_err("ex_put_conn",errmsg,exerrval);
       return(EX_FATAL);
       }

     num_ed_per_elem = 0;
     if ((elem_edge_conn != 0) &&
         (status = nc_inq_dimlen(exoid, nedpereldim, &num_ed_per_elem) != NC_NOERR))
       {
       exerrval = status;
       sprintf(errmsg,
         "Error: failed to get number of edges/elem in block %d in file id %d",
         blk_id,exoid);
       ex_err("ex_put_conn",errmsg,exerrval);
       return(EX_FATAL);
       }

     num_fa_per_elem = 0;
     if ((elem_face_conn != 0) &&
         (status = nc_inq_dimlen(exoid, nfapereldim, &num_fa_per_elem) != NC_NOERR))
       {
       exerrval = status;
       sprintf(errmsg,
         "Error: failed to get number of faces/elem in block %d in file id %d",
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
               (long)num_ed_per_elem, (void*)elem_edge_conn );
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
               (long)num_fa_per_elem, (void*)elem_face_conn );
       ex_err("ex_put_conn",errmsg,exerrval);
       return (EX_FATAL);
       }

     if ( num_ed_per_elem != 0 ) {
       status = nc_inq_varid(exoid, VAR_ECONN(blk_id_ndx), &connid);
       if (status != NC_NOERR)
         {
         exerrval = status;
         sprintf(errmsg,
           "Error: failed to locate connectivity array for "
           "element edge block %d in file id %d",
           blk_id,exoid);
         ex_err("ex_put_conn",errmsg, exerrval);
         return(EX_FATAL);
         }
       EX_WRITE_CONN("element edge",connid,elem_edge_conn);
     }

     if ( num_fa_per_elem != 0 ) {
       status = nc_inq_varid (exoid, VAR_FCONN(blk_id_ndx), &connid);
       if (status != NC_NOERR)
         {
         exerrval = status;
         sprintf(errmsg,
           "Error: failed to locate connectivity array for "
           "element face block %d in file id %d",
           blk_id,exoid);
         ex_err("ex_put_conn",errmsg, exerrval);
         return(EX_FATAL);
         }
       EX_WRITE_CONN("element face",connid,elem_face_conn);
     }
   }

   return (EX_NOERR);

}
