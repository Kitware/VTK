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
* exgblk - read block parameters
*
* author - Victor R. Yarberry, Sandia National Laboratories
*
* environment - UNIX
*
* entry conditions -
*   input parameters:
*       int     idexo                   exodus file id
*       int     blk_type                block type (edge,face,element)
*       int     blk_id                  block id
*
* exit conditions -
*       char*   elem_type               element type name
*       int*    num_entries_this_blk    number of elements in this element block
*       int*    num_nodes_per_entry     number of nodes per element block
*       int*    num_attr_per_entry      number of attributes
*
* revision history -
*   20060929 - David Thompson - Edge/face elem support (adapted from ex_get_block)
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

int ex_get_block( int exoid,
  int blk_type,
  int blk_id,
  char* elem_type,
  int* num_entries_this_blk,
  int* num_nodes_per_entry,
  int* num_edges_per_entry,
  int* num_faces_per_entry,
  int* num_attr_per_entry )
{
   int dimid, connid, len, blk_id_ndx;
   long lnum_entries_this_blk, lnum_nodes_per_entry, lnum_attr_per_entry;
   long lnum_edges_per_entry, lnum_faces_per_entry;
   char *ptr;
   char  errmsg[MAX_ERR_LENGTH];
   nc_type dummy;
   const char* tname;
   const char* dnument;
   const char* dnumnod;
   const char* dnumedg;
   const char* dnumfac;
   const char* dnumatt;
   const char* ablknam;
   const char* vblkcon;
   const char* vblkids;

   exerrval = 0;

   /* First, locate index of element block id in VAR_ID_EL_BLK array */
   switch (blk_type) {
   case EX_EDGE_BLOCK:
     tname = "edge";
     vblkids = VAR_ID_ED_BLK;
     blk_id_ndx = ex_id_lkup(exoid,vblkids,blk_id);
     dnument = DIM_NUM_ED_IN_EBLK(blk_id_ndx);
     dnumnod = DIM_NUM_NOD_PER_ED(blk_id_ndx);
     dnumedg = 0;
     dnumfac = 0;
     dnumatt = DIM_NUM_ATT_IN_EBLK(blk_id_ndx);
     vblkcon = VAR_EBCONN(blk_id_ndx);
     ablknam = ATT_NAME_ELB;
     break;
   case EX_FACE_BLOCK:
     tname = "face";
     vblkids = VAR_ID_FA_BLK;
     blk_id_ndx = ex_id_lkup(exoid,vblkids,blk_id);
     dnument = DIM_NUM_FA_IN_FBLK(blk_id_ndx);
     dnumnod = DIM_NUM_NOD_PER_FA(blk_id_ndx);
     dnumedg = 0; /* it is possible this might be non-NULL some day */
     dnumfac = 0;
     dnumatt = DIM_NUM_ATT_IN_FBLK(blk_id_ndx);
     vblkcon = VAR_FBCONN(blk_id_ndx);
     ablknam = ATT_NAME_ELB;
     break;
   case EX_ELEM_BLOCK:
     tname = "element";
     vblkids = VAR_ID_EL_BLK;
     blk_id_ndx = ex_id_lkup(exoid,vblkids,blk_id);
     dnument = DIM_NUM_EL_IN_BLK(blk_id_ndx);
     dnumnod = DIM_NUM_NOD_PER_EL(blk_id_ndx);
     dnumedg = DIM_NUM_EDG_PER_EL(blk_id_ndx);
     dnumfac = DIM_NUM_FAC_PER_EL(blk_id_ndx);
     dnumatt = DIM_NUM_ATT_IN_BLK(blk_id_ndx);
     vblkcon = VAR_CONN(blk_id_ndx);
     ablknam = ATT_NAME_ELB;
     break;
   default:
     exerrval = EX_BADPARAM;
     sprintf( errmsg, "Bad block type parameter (%d) specified for file id %d.",
       blk_type, exoid );
     return (EX_FATAL);
   }

   if (exerrval != 0) 
     {
     if (exerrval == EX_NULLENTITY)     /* NULL element block?    */
       {
       if ( elem_type )
         strcpy(elem_type, "NULL");     /* NULL element type name */
       *num_entries_this_blk = 0;       /* no elements            */
       *num_nodes_per_entry = 0;        /* no nodes               */
       *num_attr_per_entry = 0;         /* no attributes          */
       return (EX_NOERR);
       }
     else
       {
       sprintf(errmsg,
        "Error: failed to locate element block id %d in %s array in file id %d",
               blk_id,vblkids,exoid);
       ex_err("ex_get_block",errmsg,exerrval);
       return (EX_FATAL);
       }
     }

   /* inquire values of some dimensions */
   if ( num_entries_this_blk )
     {
     if ((dimid = ncdimid (exoid, dnument)) == -1)
       {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate number of %ss in block %d in file id %d",
         tname,blk_id,exoid);
       ex_err("ex_get_block",errmsg, exerrval);
       return(EX_FATAL);
       }

     if (ncdiminq (exoid, dimid, (char *) 0, &lnum_entries_this_blk) == -1)
       {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to get number of %ss in block %d in file id %d",
         tname,blk_id, exoid);
       ex_err("ex_get_block",errmsg, exerrval);
       return(EX_FATAL);
       }
     *num_entries_this_blk = lnum_entries_this_blk;
     }

   if ( num_nodes_per_entry )
     {
     if ((dimid = ncdimid (exoid, dnumnod)) == -1)
       {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate number of nodes/%s in block %d in file id %d",
         tname,blk_id,exoid);
       ex_err("ex_get_block",errmsg, exerrval);
       return(EX_FATAL);
       }
     if (ncdiminq (exoid, dimid, (char *) 0, &lnum_nodes_per_entry) == -1)
       {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to get number of nodes/%s in block %d in file id %d",
         tname,blk_id, exoid);
       ex_err("ex_get_block",errmsg, exerrval);
       return(EX_FATAL);
       }
     *num_nodes_per_entry = lnum_nodes_per_entry;
     }

   if ( num_edges_per_entry )
     {
     if ( blk_type != EX_ELEM_BLOCK )
       {
       exerrval = (EX_WARN);
       sprintf(errmsg,
         "Warning: non-NULL pointer passed to num_edges_per_entry for %s block query in file id %d",
         tname,exoid);
       ex_err("ex_get_block",errmsg,exerrval);
       }
     else
       {
       if ((dimid = ncdimid (exoid, dnumedg)) == -1)
         {
         /* undefined => no edge entries per element */
         lnum_edges_per_entry = 0;
         }
       else
         {
         if (ncdiminq (exoid, dimid, (char *) 0, &lnum_edges_per_entry) == -1)
           {
           exerrval = ncerr;
           sprintf(errmsg,
             "Error: failed to get number of edges/%s in block %d in file id %d",
             tname,blk_id, exoid);
           ex_err("ex_get_block",errmsg, exerrval);
           return(EX_FATAL);
           }
         }
       *num_edges_per_entry = lnum_edges_per_entry;
       }
     }

   if ( num_faces_per_entry )
     {
     if ( blk_type != EX_ELEM_BLOCK )
       {
       exerrval = (EX_WARN);
       sprintf(errmsg,
         "Warning: non-NULL pointer passed to num_faces_per_entry for %s block query in file id %d",
         tname,exoid);
       ex_err("ex_get_block",errmsg,exerrval);
       }
     else
       {
       if ((dimid = ncdimid (exoid, dnumfac)) == -1)
         {
         /* undefined => no face entries per element */
         lnum_faces_per_entry = 0;
         }
       else
         {
         if (ncdiminq (exoid, dimid, (char *) 0, &lnum_faces_per_entry) == -1)
           {
           exerrval = ncerr;
           sprintf(errmsg,
             "Error: failed to get number of faces/%s in block %d in file id %d",
             tname,blk_id, exoid);
           ex_err("ex_get_block",errmsg, exerrval);
           return(EX_FATAL);
           }
         }
         *num_faces_per_entry = lnum_faces_per_entry;
       }
     }

   if ( num_attr_per_entry )
     {
     if ((dimid = ncdimid (exoid, dnumatt)) == -1)
       {
       /* dimension is undefined */
       *num_attr_per_entry = 0;
       }
     else
       {
       if (ncdiminq (exoid, dimid, (char *) 0, &lnum_attr_per_entry) == -1)
         {
         exerrval = ncerr;
         sprintf(errmsg,
           "Error: failed to get number of attributes in %s block %d in file id %d",
           tname,blk_id, exoid);
         ex_err("ex_get_block",errmsg, exerrval);
         return(EX_FATAL);
         }
       *num_attr_per_entry = lnum_attr_per_entry;
       }
     }

   if ( elem_type )
     {
     /* look up connectivity array for this element block id */
     if ((connid = ncvarid (exoid, vblkcon)) == -1)
       {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate connectivity array for element block %d in file id %d",
         blk_id,exoid);
       ex_err("ex_get_block",errmsg, exerrval);
       return(EX_FATAL);
       }

     if (ncattinq (exoid, connid, ablknam, &dummy, &len) == -1)
       {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to get element block %d type in file id %d",
         blk_id,exoid);
       ex_err("ex_get_block",errmsg, exerrval);
       return(EX_FATAL);
       }

     if (len > (MAX_STR_LENGTH+1))
       {
       len = MAX_STR_LENGTH;
       sprintf (errmsg,
         "Warning: element block %d type will be truncated to %d chars", 
         blk_id,len);
       ex_err("ex_get_block",errmsg,EX_MSG);
       }
     /* get the element type name */

     if (ncattget (exoid, connid, ablknam, elem_type) == -1)
       {
       exerrval = ncerr;
       sprintf(errmsg,"Error: failed to get element block %d type in file id %d",
         blk_id, exoid);
       ex_err("ex_get_block",errmsg, exerrval);
       return(EX_FATAL);
       }

     /* get rid of trailing blanks */
     ptr = elem_type;
     /* fprintf(stderr,"[exgblk] %s, len: %d\n",ptr,len); */
     while (ptr < elem_type + len && *ptr != ' ')
       {
       ptr++;
       }
     *(ptr) = '\0';
     }

   return (EX_NOERR);
}
