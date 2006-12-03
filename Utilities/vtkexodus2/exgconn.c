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
* exgconn - exodusII read edge/face/element block connectivity
*
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          James A. Schutt - 8 byte float and standard C definitions
*          Vic Yarberry    - Added headers and error logging
*
* environment - UNIX
*
* entry conditions - 
*   expelb must be called first to establish element block parameters.
*   input parameters:
*       int     exoid           exodus file id
*       int     blk_type        block type (edge, face, element)
*       int     blk_id          block id
*
* exit conditions - 
*       int*    nodeconn        nodal connectivity array
*       int*    edgeconn        edge connectivity array (where applicable)
*       int*    faceconn        face connectivity array (where applicable)
*
* revision history - 
*   20061001 - David Thompson - Generalized from ex_get_conn.
*
* Id
*/
#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the connectivity array for an element block
 */

int ex_get_conn( int   exoid,
                 int   blk_type,
                 int   blk_id,
                 int*  nodeconn,
                 int*  edgeconn,
                 int*  faceconn )
{
   int numblkentriesdim, connid, econnid = -1, fconnid = -1, blk_id_ndx, iresult;
   int numnodperentdim, numedgperentdim = -1, numfacperentdim = -1;
   int iexit = (EX_NOERR); /* exit status */
   long num_entries_this_blk, num_nodes_per_entry, num_edges_per_entry, num_faces_per_entry;
   long start[2], count[2]; 
   nclong *longs;
   char errmsg[MAX_ERR_LENGTH];

   const char* tname;
   const char* vblkids;
   const char* dnumblkent = 0;
   const char* dnumnodent = 0;
   const char* dnumedgent = 0;
   const char* dnumfacent = 0;
   const char* vnodeconn = 0;
   const char* vedgeconn = 0;
   const char* vfaceconn = 0;

   /* Should we warn if edgeconn or faceconn are non-NULL?
    * No, fail silently so the same code can be used to read any type of block info.
    * However, we will warn if edgeconn or faceconn are NULL but num_edges_per_entry
    * or num_faces_per_entry (respectively) are positive.
    */
   switch (blk_type) {
   case EX_EDGE_BLOCK:
     tname = "edge";
     vblkids = VAR_ID_ED_BLK;
     break;
   case EX_FACE_BLOCK:
     tname = "face";
     vblkids = VAR_ID_FA_BLK;
     break;
   case EX_ELEM_BLOCK:
     tname = "element";
     vblkids = VAR_ID_EL_BLK;
     break;
   default:
     exerrval = EX_BADPARAM;
     sprintf( errmsg, "Error: Invalid block type (%d) specified in file id %d", blk_type, exoid );
     ex_err( "ex_get_conn", errmsg, exerrval );
     return (EX_FATAL);
   }

   exerrval = 0; /* clear error code */

   /* Locate index of element block id in VAR_ID_EL_BLK array */

   blk_id_ndx = ex_id_lkup(exoid,vblkids,blk_id);
   if (exerrval != 0) 
   {
     if (exerrval == EX_NULLENTITY)
     {
       sprintf(errmsg,
              "Warning: no connectivity array for NULL %s block %d in file id %d",
               tname,blk_id,exoid);
       ex_err("ex_get_conn",errmsg,EX_MSG);
       return (EX_WARN); /* no connectivity array for this element block */
     }
     else
     {
       sprintf(errmsg,
        "Error: failed to locate %s block id %d in %s array in file id %d",
               tname,blk_id,vblkids,exoid);
       ex_err("ex_get_conn",errmsg,exerrval);
       return (EX_FATAL);
     }
   }

   switch (blk_type) {
   case EX_EDGE_BLOCK:
     dnumblkent = DIM_NUM_ED_IN_EBLK(blk_id_ndx);
     dnumnodent = DIM_NUM_NOD_PER_ED(blk_id_ndx);
     dnumedgent = 0;
     dnumfacent = 0;
     vnodeconn = VAR_EBCONN(blk_id_ndx);
     vedgeconn = 0;
     vfaceconn = 0;
     break;
   case EX_FACE_BLOCK:
     dnumblkent = DIM_NUM_FA_IN_FBLK(blk_id_ndx);
     dnumnodent = DIM_NUM_NOD_PER_FA(blk_id_ndx);
     dnumedgent = 0;
     dnumfacent = 0;
     vnodeconn = VAR_FBCONN(blk_id_ndx);
     vedgeconn = 0;
     vfaceconn = 0;
     break;
   case EX_ELEM_BLOCK:
     dnumblkent = DIM_NUM_EL_IN_BLK(blk_id_ndx);
     dnumnodent = DIM_NUM_NOD_PER_EL(blk_id_ndx);
     dnumedgent = DIM_NUM_EDG_PER_EL(blk_id_ndx);
     dnumfacent = DIM_NUM_FAC_PER_EL(blk_id_ndx);
     vnodeconn = VAR_CONN(blk_id_ndx);
     vedgeconn = VAR_ECONN(blk_id_ndx);
     vfaceconn = VAR_FCONN(blk_id_ndx);
     break;
   }
/* inquire id's of previously defined dimensions  */

   if ((numblkentriesdim = ncdimid (exoid, dnumblkent)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
     "Error: failed to locate number of elements in %s block %d in file id %d",
              tname,blk_id,exoid);
     ex_err("ex_get_conn",errmsg, exerrval);
     return(EX_FATAL);
   }

   if (ncdiminq (exoid, numblkentriesdim, (char *) 0, &num_entries_this_blk) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
           "Error: failed to get number of entries in %s block %d in file id %d",
             tname,blk_id,exoid);
     ex_err("ex_get_conn",errmsg,exerrval);
     return(EX_FATAL);
   }


   if ((numnodperentdim = ncdimid (exoid, dnumnodent)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
      "Error: failed to locate number of nodes/elem for block %d in file id %d",
             blk_id,exoid);
     ex_err("ex_get_conn",errmsg,exerrval);
     return(EX_FATAL);
   }

   if (ncdiminq (exoid, numnodperentdim, (char *) 0, &num_nodes_per_entry) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
       "Error: failed to get number of nodes/elem for block %d in file id %d",
             blk_id,exoid);
     ex_err("ex_get_conn",errmsg, exerrval);
     return(EX_FATAL);
   }

   if ( dnumedgent ) {
     num_edges_per_entry = 0;
     if ((numedgperentdim = ncdimid (exoid, dnumedgent)) == -1) {
       numedgperentdim = -1;
     } else {
       if (ncdiminq (exoid, numedgperentdim, (char *) 0, &num_edges_per_entry) == -1) {
         exerrval = ncerr;
         sprintf(errmsg,
           "Error: failed to get number of edges/entry for %s block %d in file id %d",
           tname,blk_id,exoid);
         ex_err("ex_get_conn",errmsg, exerrval);
         return(EX_FATAL);
       }
       if ( num_edges_per_entry < 0 )
         num_edges_per_entry = 0;
     }
     if ( num_edges_per_entry > 0 && (!edgeconn) ) {
       exerrval = EX_BADPARAM;
       sprintf( errmsg, "Edge connectivity present but NULL pointer passed for file id %d", exoid );
       ex_err( "ex_get_conn", errmsg, exerrval );
       iexit = exerrval;
     }
   }

   if ( dnumfacent ) {
     num_faces_per_entry = 0;
     if ((numfacperentdim = ncdimid (exoid, dnumfacent)) == -1) {
       numfacperentdim = -1;
     } else {
       if (ncdiminq (exoid, numfacperentdim, (char *) 0, &num_faces_per_entry) == -1) {
         exerrval = ncerr;
         sprintf(errmsg,
           "Error: failed to get number of faces/entry for %s block %d in file id %d",
           tname,blk_id,exoid);
         ex_err("ex_get_conn",errmsg, exerrval);
         return(EX_FATAL);
       }
       if ( num_faces_per_entry < 0 )
         num_faces_per_entry = 0;
     }
     if ( num_faces_per_entry > 0 && (!faceconn) ) {
       exerrval = EX_BADPARAM;
       sprintf( errmsg, "Face connectivity present but NULL pointer passed for file id %d", exoid );
       ex_err( "ex_get_conn", errmsg, exerrval );
       iexit = exerrval;
     }
   }


   if ((connid = ncvarid (exoid, vnodeconn)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
        "Error: failed to locate connectivity array for block %d in file id %d",
             blk_id,exoid);
     ex_err("ex_get_conn",errmsg, exerrval);
     return(EX_FATAL);
   }

   if ( edgeconn && (numedgperentdim > 0) && ((econnid = ncvarid (exoid, vedgeconn)) == -1) )
   {
     exerrval = ncerr;
     sprintf(errmsg,
        "Error: failed to locate edge connectivity array for %s block %d in file id %d",
             tname,blk_id,exoid);
     ex_err("ex_get_conn",errmsg, exerrval);
     return(EX_FATAL);
   }

   if ( faceconn && (numfacperentdim > 0) && ((fconnid = ncvarid (exoid, vfaceconn)) == -1) )
   {
     exerrval = ncerr;
     sprintf(errmsg,
        "Error: failed to locate face connectivity array for %s block %d in file id %d",
             tname,blk_id,exoid);
     ex_err("ex_get_conn",errmsg, exerrval);
     return(EX_FATAL);
   }


/* read in the connectivity array */

/* application code has allocated an array of ints but netcdf is expecting
   a pointer to nclongs;  if ints are different sizes than nclongs,
   we must allocate an array of nclongs then convert them to ints with ltoi */

   start[0] = 0;
   start[1] = 0;

   if ( edgeconn && num_edges_per_entry ) {
     count[0] = num_entries_this_blk;
     count[1] = num_edges_per_entry;

     if (sizeof(int) == sizeof(nclong)) {
       iresult = ncvarget (exoid, econnid, start, count, edgeconn);
     } else {
       if (!(longs = malloc (num_entries_this_blk*num_edges_per_entry * sizeof(nclong)))) {
         exerrval = EX_MEMFAIL;
         sprintf(errmsg,
           "Error: failed to allocate memory for edge connectivity array for file id %d",
           exoid);
         ex_err("ex_get_conn",errmsg,exerrval);
         return (EX_FATAL);
       }
       iresult = ncvarget (exoid, econnid, start, count, longs);
     }

     if (iresult == -1)
       {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to get edge connectivity array for block %d in file id %d",
         blk_id,exoid);
       ex_err("ex_get_conn",errmsg, exerrval);
       return(EX_FATAL);
       }

     if (sizeof(int) != sizeof(nclong)) {
       ltoi (longs, edgeconn, num_entries_this_blk*num_edges_per_entry);
       free (longs);
     }
   }

   if ( faceconn && num_faces_per_entry ) {
     count[0] = num_entries_this_blk;
     count[1] = num_faces_per_entry;

     if (sizeof(int) == sizeof(nclong)) {
       iresult = ncvarget (exoid, fconnid, start, count, faceconn);
     } else {
       if (!(longs = malloc (num_entries_this_blk*num_faces_per_entry * sizeof(nclong)))) {
         exerrval = EX_MEMFAIL;
         sprintf(errmsg,
           "Error: failed to allocate memory for face connectivity array of %s blockfor file id %d",
           tname,exoid);
         ex_err("ex_get_conn",errmsg,exerrval);
         return (EX_FATAL);
       }
       iresult = ncvarget (exoid, fconnid, start, count, longs);
     }

     if (iresult == -1)
       {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to get face connectivity array for %s block %d in file id %d",
         tname,blk_id,exoid);
       ex_err("ex_get_conn",errmsg, exerrval);
       return(EX_FATAL);
       }

     if (sizeof(int) != sizeof(nclong)) {
       ltoi (longs, faceconn, num_entries_this_blk*num_faces_per_entry);
       free (longs);
     }
   }

   if ( nodeconn && num_nodes_per_entry ) {
     count[0] = num_entries_this_blk;
     count[1] = num_nodes_per_entry;

     if (sizeof(int) == sizeof(nclong)) {
       iresult = ncvarget (exoid, connid, start, count, nodeconn);
     } else {
       if (!(longs = malloc (num_entries_this_blk*num_nodes_per_entry * sizeof(nclong)))) {
         exerrval = EX_MEMFAIL;
         sprintf(errmsg,
           "Error: failed to allocate memory for element connectivity array for file id %d",
           exoid);
         ex_err("ex_get_conn",errmsg,exerrval);
         return (EX_FATAL);
       }
       iresult = ncvarget (exoid, connid, start, count, longs);
     }

     if (iresult == -1)
       {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to get connectivity array for block %d in file id %d",
         blk_id,exoid);
       ex_err("ex_get_conn",errmsg, exerrval);
       return(EX_FATAL);
       }

     if (sizeof(int) != sizeof(nclong)) {
       ltoi (longs, nodeconn, num_entries_this_blk*num_nodes_per_entry);
       free (longs);
     }
   }

   return iexit;
}
