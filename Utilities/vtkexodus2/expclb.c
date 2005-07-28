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
* expclb - ex_put_concat_elem_block: write element block parameters
*
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          James A. Schutt - 8 byte float and standard C definitions
*          Vic Yarberry    - Added headers and error logging
*          Greg Sjaardema  - created this function to avoid ncredef calls.
*
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     idexo                   exodus file id
*       char**  elem_type               element type string
*       int*    num_elem_this_blk       number of elements in the element blk
*       int*    num_nodes_per_elem      number of nodes per element block
*       int*    num_attr                number of attributes
*       int     define_maps             if != 0, write maps, else don't
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"
#include <string.h>

/*
 * writes the parameters used to describe an element block
 */

int ex_put_concat_elem_block (int    exoid,
                              const int*   elem_blk_id,
                              char *elem_type[],
                              const int*   num_elem_this_blk,
                              const int*   num_nodes_per_elem,
                              const int*   num_attr,
                              int    define_maps)
{
  int mapid, varid, dimid, dims[2], *elem_blk_id_ndx, elem_blk_stat;
  int iblk;
  int iresult;
  long start[2], count[2], num_elem_blk;
  nclong ldum, *lptr;
  int cur_num_elem_blk, nelnoddim, numelbdim, numattrdim, connid, numelemdim, numnodedim;
  char *cdum;
  char errmsg[MAX_ERR_LENGTH];

  exerrval  = 0; /* clear error code */

  cdum = 0;

  /* first check if any element blocks are specified
   * OK if zero...
   */
  
  if ((dimid = (ncdimid (exoid, DIM_NUM_EL_BLK))) == -1 )
    {
      return (EX_NOERR);
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
  
  if (!(elem_blk_id_ndx= malloc(num_elem_blk*sizeof(int))))
    {
      exerrval = EX_MEMFAIL;
      sprintf(errmsg,
              "Error: failed to allocate space for element block id index array in file id %d",
              exoid);
      ex_err("ex_put_concat_elem_block",errmsg,exerrval);
      return (EX_FATAL);
    }

  for (iblk = 0; iblk < num_elem_blk; iblk++) {
    elem_blk_id_ndx[iblk] = ex_id_lkup(exoid,VAR_ID_EL_BLK,elem_blk_id[iblk]);
    if (exerrval != EX_LOOKUPFAIL)   /* found the element block id */
      {
        exerrval = EX_FATAL;
        sprintf(errmsg,
                "Error: element block id %d already exists in file id %d",
                elem_blk_id[iblk],exoid);
        ex_err("ex_put_elem_block",errmsg,exerrval);
        free(elem_blk_id_ndx);
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
        free(elem_blk_id_ndx);
        return (EX_FATAL);
      }


    /*   NOTE: ex_get_file_item  is a function that finds the number of element
         blocks for a specific file and returns that value incremented. */

    cur_num_elem_blk=ex_inc_file_item(exoid, &eb_ctr_list);
    start[0] = (long)cur_num_elem_blk;

    /* write out element block id to previously defined id array variable*/

    ldum = (nclong)elem_blk_id[iblk];
    if (ncvarput1 (exoid, varid, start, &ldum) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to store element block id to file id %d",
                exoid);
        ex_err("ex_put_elem_block",errmsg,exerrval);
        free(elem_blk_id_ndx);
        return (EX_FATAL);
      }

    elem_blk_id_ndx[iblk] = start[0]+1; /* element id index into VAR_ID_EL_BLK array*/

    if (num_elem_this_blk[iblk] == 0) /* Is this a NULL element block? */
      elem_blk_stat = 0; /* change element block status to NULL */
    else
      elem_blk_stat = 1; /* change element block status to TRUE */

    if ((varid = ncvarid (exoid, VAR_STAT_EL_BLK)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to locate element block status in file id %d", exoid);
        ex_err("ex_put_elem_block",errmsg,exerrval);
        free(elem_blk_id_ndx);
        return (EX_FATAL);
      }

    ldum = (nclong)elem_blk_stat;
    if (ncvarput1 (exoid, varid, start, &ldum) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to store element id %d status to file id %d",
                elem_blk_id[iblk], exoid);
        ex_err("ex_put_elem_block",errmsg,exerrval);
        free(elem_blk_id_ndx);
        return (EX_FATAL);
      }
  }

  /* put netcdf file into define mode  */

  /*
    ncsync(exoid);
  */

  if (ncredef (exoid) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,"Error: failed to place file id %d into define mode",exoid);
      ex_err("ex_put_elem_block",errmsg,exerrval);
      return (EX_FATAL);
    }

  /* Iterate over element blocks ... */
  for (iblk = 0; iblk < num_elem_blk; iblk++) {

    if (num_elem_this_blk[iblk] == 0) /* Is this a NULL element block? */
      continue;

    /* define some dimensions and variables*/
    if ((numelbdim = ncdimdef (exoid,
                               DIM_NUM_EL_IN_BLK(elem_blk_id_ndx[iblk]),
                               (long)num_elem_this_blk[iblk])) == -1)
      {
        if (ncerr == NC_ENAMEINUSE)     /* duplicate entry */
          {
            exerrval = ncerr;
            sprintf(errmsg,
                    "Error: element block %d already defined in file id %d",
                    elem_blk_id[iblk],exoid);
            ex_err("ex_put_elem_block",errmsg,exerrval);
          }
        else
          {
            exerrval = ncerr;
            sprintf(errmsg,
                    "Error: failed to define number of elements/block for block %d file id %d",
                    elem_blk_id[iblk],exoid);
            ex_err("ex_put_elem_block",errmsg,exerrval);
          }
        goto error_ret;         /* exit define mode and return */
      }

    if ((nelnoddim = ncdimdef (exoid,
                               DIM_NUM_NOD_PER_EL(elem_blk_id_ndx[iblk]),
                               (long)num_nodes_per_elem[iblk])) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to define number of nodes/element for block %d in file id %d",
                elem_blk_id[iblk],exoid);
        ex_err("ex_put_elem_block",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }

    /* element attribute array */

    if (num_attr[iblk] > 0)
      {

        if ((numattrdim = ncdimdef (exoid, 
                                    DIM_NUM_ATT_IN_BLK(elem_blk_id_ndx[iblk]),
                                    (long)num_attr[iblk])) == -1)
          {
            exerrval = ncerr;
            sprintf(errmsg,
                    "Error: failed to define number of attributes in block %d in file id %d",
                    elem_blk_id[iblk],exoid);
            ex_err("ex_put_elem_block",errmsg,exerrval);
            goto error_ret;         /* exit define mode and return */
          }

        dims[0] = numelbdim;
        dims[1] = numattrdim;

        if ((ncvardef (exoid, 
                       VAR_ATTRIB(elem_blk_id_ndx[iblk]), nc_flt_code(exoid), 2, dims)) == -1)
          {
            exerrval = ncerr;
            sprintf(errmsg,
                    "Error:  failed to define attributes for element block %d in file id %d",
                    elem_blk_id[iblk],exoid);
            ex_err("ex_put_elem_block",errmsg,exerrval);
            goto error_ret;         /* exit define mode and return */
          }
      }

    /* element connectivity array */

    dims[0] = numelbdim;
    dims[1] = nelnoddim;

    if ((connid = ncvardef (exoid, 
                            VAR_CONN(elem_blk_id_ndx[iblk]), NC_LONG, 2, dims)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to create connectivity array for block %d in file id %d",
                elem_blk_id[iblk],exoid);
        ex_err("ex_put_elem_block",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }

    /* store element type as attribute of connectivity variable */

    if ((ncattput (exoid, connid, ATT_NAME_ELB, NC_CHAR, strlen(elem_type[iblk])+1, 
                   (void*)elem_type[iblk])) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to store element type name %s in file id %d",
                elem_type[iblk],exoid);
        ex_err("ex_put_elem_block",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }
 
  }
  free(elem_blk_id_ndx);

  /* Define the element map here to avoid a later redefine call */
  if (define_maps != 0) {
    if (ncvarid(exoid, VAR_ELEM_NUM_MAP) == -1) { /* Map does not exist */
      /* Possible to have zero elements but >0 element blocks.
       * Only define map if there are nonzero elements
       */
      if ((numelemdim = ncdimid (exoid, DIM_NUM_ELEM)) != -1)
        {
          dims[0] = numelemdim;
          
          if ((mapid = ncvardef (exoid, VAR_ELEM_NUM_MAP, NC_LONG, 1, dims)) == -1) {
            if (ncerr == NC_ENAMEINUSE)
              {
                exerrval = ncerr;
                sprintf(errmsg,
                        "Error: element numbering map already exists in file id %d",
                        exoid);
                ex_err("ex_put_elem_num_map",errmsg,exerrval);
              }
            else
              {
                exerrval = ncerr;
                sprintf(errmsg,
                        "Error: failed to create element numbering map in file id %d",
                        exoid);
                ex_err("ex_put_elem_num_map",errmsg,exerrval);
              }
            goto error_ret;         /* exit define mode and return */
          }
        }
    }
    /* Do the same for the node numbering map */
    if (ncvarid(exoid, VAR_NODE_NUM_MAP) == -1) { /* Map does not exist */
      if ((numnodedim = ncdimid (exoid, DIM_NUM_NODES)) != -1)
        {
          dims[0] = numnodedim;
          if ((mapid = ncvardef (exoid, VAR_NODE_NUM_MAP, NC_LONG, 1, dims)) == -1)
            {
              if (ncerr == NC_ENAMEINUSE)
                {
                  exerrval = ncerr;
                  sprintf(errmsg,
                          "Error: node numbering map already exists in file id %d",
                          exoid);
                  ex_err("ex_put_node_num_map",errmsg,exerrval);
                }
              else
                {
                  exerrval = ncerr;
                  sprintf(errmsg,
                          "Error: failed to create node numbering map array in file id %d",
                          exoid);
                  ex_err("ex_put_node_num_map",errmsg,exerrval);
                }
              goto error_ret;         /* exit define mode and return */
            }
        }
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

  /* first get id of element block ids array variable */

  if ((varid = ncvarid (exoid, VAR_ID_EL_BLK)) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to locate element block ids array in file id %d",
              exoid);
      ex_err("ex_put_concat_elem_block",errmsg,exerrval);
      return (EX_FATAL);
    }

  /* then, write out element block id list */

  /* this contortion is necessary because netCDF is expecting nclongs; fortunately
     it's necessary only when ints and nclongs aren't the same size */

  start[0] = 0;
  count[0] = num_elem_blk;

  if (sizeof(int) == sizeof(nclong)) {
    iresult = ncvarput (exoid, varid, start, count, elem_blk_id);
  } else {
    lptr = itol (elem_blk_id, num_elem_blk);
    iresult = ncvarput (exoid, varid, start, count, lptr);
    free(lptr);
  }

  if (iresult == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to element block id array in file id %d",
              exoid);
      ex_err("ex_put_concat_elem_block",errmsg,exerrval);
      return (EX_FATAL);
    }

  return (EX_NOERR);

  /* Fatal error: exit definition mode and return */
 error_ret:
  free(elem_blk_id_ndx);
  if (ncendef (exoid) == -1)     /* exit define mode */
    {
      sprintf(errmsg,
              "Error: failed to complete definition for file id %d",
              exoid);
      ex_err("ex_put_elem_block",errmsg,exerrval);
    }
  (void)mapid;
  return (EX_FATAL);
}

