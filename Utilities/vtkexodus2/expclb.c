/*
 * Copyright (c) 2005 Sandia Corporation. Under the terms of Contract
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

/*!
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
  int i, varid, dimid, dims[2], strdim, *eb_stat;
  int iblk;
  int iresult;
  long start[2], count[2], num_elem_blk;
  nclong *lptr;
  int cur_num_elem_blk, nelnoddim, numelbdim, numattrdim, connid, numelemdim, numnodedim;
  char *cdum;
  char errmsg[MAX_ERR_LENGTH];

  exerrval  = 0; /* clear error code */

  cdum = 0;

  /* first check if any element blocks are specified
   * OK if zero...
   */
  if ((dimid = (ncdimid (exoid, DIM_NUM_EL_BLK))) == -1 ) {
    return (EX_NOERR);
  }

  /* Get number of element blocks defined for this file */
  if ((ncdiminq (exoid,dimid,cdum,&num_elem_blk)) == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: failed to get number of element blocks in file id %d",
      exoid);
    ex_err("ex_put_concat_elem_block",errmsg,exerrval);
    return (EX_FATAL);
  }
  
  /* Fill out the element block status array */
  if (!(eb_stat = malloc(num_elem_blk*sizeof(int)))) {
    exerrval = EX_MEMFAIL;
    sprintf(errmsg,
      "Error: failed to allocate space for element block status array in file id %d",
      exoid);
    ex_err("ex_put_concat_elem_block",errmsg,exerrval);
    return (EX_FATAL);
  }

  for (i=0;i<num_elem_blk;i++) {
    if (num_elem_this_blk[i] == 0) /* Is this a NULL element block? */
      eb_stat[i] = 0; /* change element block status to NULL */
    else
      eb_stat[i] = 1; /* change element block status to TRUE */
  }

  /* Next, get variable id of status array */
  if ((varid = ncvarid (exoid, VAR_STAT_EL_BLK)) == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: failed to locate element block status in file id %d",
      exoid);
    ex_err("ex_put_concat_elem_block",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* this contortion is necessary because netCDF is expecting nclongs;
     fortunately it's necessary only when ints and nclongs aren't the
     same size */
  start[0] = 0;
  count[0] = num_elem_blk;

  if (sizeof(int) == sizeof(nclong)) {
    iresult = ncvarput (exoid, varid, start, count, eb_stat);
  } else {
    lptr = itol (eb_stat, num_elem_blk);
    iresult = ncvarput (exoid, varid, start, count, lptr);
    free(lptr);
  }

  if (iresult == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: failed to store element block status array to file id %d",
            exoid);
    ex_err("ex_put_concat_elem_block",errmsg,exerrval);
    return (EX_FATAL);
  }

  free(eb_stat);

  /* Next, fill out ids array */
  /* first get id of ids array variable */
  if ((varid = ncvarid (exoid, VAR_ID_EL_BLK)) == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: failed to locate element block ids array in file id %d",
            exoid);
    ex_err("ex_put_concat_elem_block",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* then, write out id list */
  /* this contortion is necessary because netCDF is expecting nclongs;
     fortunately it's necessary only when ints and nclongs aren't the
     same size */
  start[0] = 0;
  count[0] = num_elem_blk;

  if (sizeof(int) == sizeof(nclong)) {
    iresult = ncvarput (exoid, varid, start, count, elem_blk_id);
  } else {
    lptr = itol (elem_blk_id, num_elem_blk);
    iresult = ncvarput (exoid, varid, start, count, lptr);
    free(lptr);
  }

  if (iresult == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: failed to store element block id array in file id %d",
            exoid);
    ex_err("ex_put_concat_elem_block",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* inquire previously defined dimensions  */
  if ((strdim = ncdimid (exoid, DIM_STR)) < 0) {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: failed to get string length in file id %d",exoid);
    ex_err("ex_put_concat_elem_block",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* put netcdf file into define mode  */
  if (ncredef (exoid) == -1)  {
    exerrval = ncerr;
    sprintf(errmsg,"Error: failed to place file id %d into define mode",exoid);
    ex_err("ex_put_concat_elem_block",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* Iterate over element blocks ... */
  for (iblk = 0; iblk < num_elem_blk; iblk++) {

    cur_num_elem_blk=ex_get_file_item(exoid, &eb_ctr_list );
    if (cur_num_elem_blk >= num_elem_blk) {
      exerrval = EX_FATAL;
      sprintf(errmsg,
        "Error: exceeded number of element blocks (%ld) defined in file id %d",
              num_elem_blk,exoid);
      ex_err("ex_put_concat_elem_block",errmsg,exerrval);
      goto error_ret;
    }

    /* NOTE: ex_inc_file_item  is used to find the number of element blocks
       for a specific file and returns that value incremented. */
    cur_num_elem_blk=ex_inc_file_item(exoid, &eb_ctr_list );

    if (num_elem_this_blk[iblk] == 0) /* Is this a NULL element block? */
      continue;

    /* define some dimensions and variables*/
    if ((numelbdim = ncdimdef (exoid,
                               DIM_NUM_EL_IN_BLK(cur_num_elem_blk+1),
                               (long)num_elem_this_blk[iblk])) == -1) {
      exerrval = ncerr;
      if (ncerr == NC_ENAMEINUSE) {     /* duplicate entry */
  sprintf(errmsg,
    "Error: element block %d already defined in file id %d",
    elem_blk_id[iblk],exoid);
      } else {
  sprintf(errmsg,
    "Error: failed to define number of elements/block for block %d file id %d",
    elem_blk_id[iblk],exoid);
      }
      ex_err("ex_put_concat_elem_block",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

    if ((nelnoddim = ncdimdef (exoid,
                               DIM_NUM_NOD_PER_EL(cur_num_elem_blk+1),
                               (long)num_nodes_per_elem[iblk])) == -1) {
      exerrval = ncerr;
      sprintf(errmsg,
        "Error: failed to define number of nodes/element for block %d in file id %d",
        elem_blk_id[iblk],exoid);
      ex_err("ex_put_concat_elem_block",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

    /* element attribute array */

    if (num_attr[iblk] > 0) {
      if ((numattrdim = ncdimdef (exoid, 
          DIM_NUM_ATT_IN_BLK(cur_num_elem_blk+1),
          (long)num_attr[iblk])) == -1) {
  exerrval = ncerr;
  sprintf(errmsg,
    "Error: failed to define number of attributes in block %d in file id %d",
    elem_blk_id[iblk],exoid);
  ex_err("ex_put_concat_elem_block",errmsg,exerrval);
  goto error_ret;         /* exit define mode and return */
      }
      
      dims[0] = numelbdim;
      dims[1] = numattrdim;
      
      if ((ncvardef (exoid, VAR_ATTRIB(cur_num_elem_blk+1),
         nc_flt_code(exoid), 2, dims)) == -1) {
  exerrval = ncerr;
  sprintf(errmsg,
    "Error:  failed to define attributes for element block %d in file id %d",
    elem_blk_id[iblk],exoid);
  ex_err("ex_put_concat_elem_block",errmsg,exerrval);
  goto error_ret;         /* exit define mode and return */
      }

      /* Attribute names... */
      dims[0] = numattrdim;
      dims[1] = strdim;
      
      if (ncvardef (exoid, VAR_NAME_ATTRIB(cur_num_elem_blk+1), NC_CHAR, 2, dims) == -1) {
  exerrval = ncerr;
  sprintf(errmsg,
    "Error: failed to define element attribute name array in file id %d",exoid);
  ex_err("ex_put_concat_elem_block",errmsg,exerrval);
  goto error_ret;         /* exit define mode and return */
      }
    }
    
    /* element connectivity array */
    
    dims[0] = numelbdim;
    dims[1] = nelnoddim;

    if ((connid = ncvardef (exoid, VAR_CONN(cur_num_elem_blk+1),
          NC_LONG, 2, dims)) == -1) {
      exerrval = ncerr;
      sprintf(errmsg,
        "Error: failed to create connectivity array for block %d in file id %d",
        elem_blk_id[iblk],exoid);
      ex_err("ex_put_concat_elem_block",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

    /* store element type as attribute of connectivity variable */
    if ((ncattput (exoid, connid, ATT_NAME_ELB, NC_CHAR, strlen(elem_type[iblk])+1, 
                   (void*)elem_type[iblk])) == -1) {
      exerrval = ncerr;
      sprintf(errmsg,
        "Error: failed to store element type name %s in file id %d",
        elem_type[iblk],exoid);
      ex_err("ex_put_concat_elem_block",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
  }

  /* Define the element map here to avoid a later redefine call */
  if (define_maps != 0) {
    if (ncvarid(exoid, VAR_ELEM_NUM_MAP) == -1) { /* Map does not exist */
      /* Possible to have zero elements but >0 element blocks.
       * Only define map if there are nonzero elements
       */
      if ((numelemdim = ncdimid (exoid, DIM_NUM_ELEM)) != -1) {
  dims[0] = numelemdim;
  
  if ((ncvardef (exoid, VAR_ELEM_NUM_MAP, NC_LONG, 1, dims)) == -1) {
    exerrval = ncerr;
    if (ncerr == NC_ENAMEINUSE) {
      sprintf(errmsg,
        "Error: element numbering map already exists in file id %d",
        exoid);
    } else {
      sprintf(errmsg,
        "Error: failed to create element numbering map in file id %d",
        exoid);
    }
    ex_err("ex_put_concat_elem_block",errmsg,exerrval);
    goto error_ret;         /* exit define mode and return */
  }
      }
    }
    /* Do the same for the node numbering map */
    if (ncvarid(exoid, VAR_NODE_NUM_MAP) == -1) { /* Map does not exist */
      if ((numnodedim = ncdimid (exoid, DIM_NUM_NODES)) != -1) {
  dims[0] = numnodedim;
  if ((ncvardef (exoid, VAR_NODE_NUM_MAP, NC_LONG, 1, dims)) == -1) {
    exerrval = ncerr;
    if (ncerr == NC_ENAMEINUSE) {
      sprintf(errmsg,
        "Error: node numbering map already exists in file id %d",
        exoid);
    } else {
      sprintf(errmsg,
        "Error: failed to create node numbering map array in file id %d",
        exoid);
    }
    ex_err("ex_put_concat_elem_block",errmsg,exerrval);
    goto error_ret;         /* exit define mode and return */
  }
      }
    }
  }

  /* leave define mode  */
  if (ncendef (exoid) == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
      "Error: failed to complete element block definition in file id %d", 
      exoid);
    ex_err("ex_put_concat_elem_block",errmsg,exerrval);
    return (EX_FATAL);
  }

  return (EX_NOERR);
  
  /* Fatal error: exit definition mode and return */
 error_ret:
  if (ncendef (exoid) == -1) {     /* exit define mode */
    sprintf(errmsg,
      "Error: failed to complete definition for file id %d",
      exoid);
    ex_err("ex_put_concat_elem_block",errmsg,exerrval);
  }
  return (EX_FATAL);
}

