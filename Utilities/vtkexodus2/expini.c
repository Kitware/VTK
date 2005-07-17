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
* expini - ex_put_init
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
*       char*   title                   title of file
*       int     num_dim                 number of dimensions (per node)
*       int     num_nodes               number of nodes
*       int     num_elem                number of elements
*       int     num_elem_blk            number of element blocks
*       int     num_node_sets           number of node sets
*       int     num_side_sets           number of side sets
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
 * writes the initialization parameters to the EXODUS II file
 */

int ex_put_init (int   exoid,
                 const char *title,
                 int   num_dim,
                 int   num_nodes,
                 int   num_elem,
                 int   num_elem_blk,
                 int   num_node_sets,
                 int   num_side_sets)
{
  int numdimdim, numnoddim, elblkdim, strdim, dim[2], dimid, varid;
  int header_size, fixed_var_size, iows;
  
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  if ((ncdimid (exoid, DIM_NUM_DIM)) != -1)
    {
      exerrval = EX_MSG;
      sprintf(errmsg,
              "Error: initialization already done for file id %d",exoid);
      ex_err("ex_put_init",errmsg,exerrval);
      return (EX_FATAL);
    }


  /* put file into define mode */

  if (ncredef (exoid) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to put file id %d into define mode", exoid);
      ex_err("ex_put_init",errmsg,exerrval);
      return (EX_FATAL);
    }


  /* define some attributes... */

  if (ncattput (exoid, NC_GLOBAL, (const char*) ATT_TITLE, 
                NC_CHAR, strlen(title)+1, (void *)title) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to define title attribute to file id %d", exoid);
      ex_err("ex_put_init",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

  /* ...and some dimensions... */

  if ((numdimdim = ncdimdef (exoid, DIM_NUM_DIM, (long)num_dim)) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to define number of dimensions in file id %d",exoid);
      ex_err("ex_put_init",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

  /*
   * Need to handle "empty file" that may be the result of a strange
   * load balance or some other strange run.  Note that if num_node
   * == 0, then num_elem must be zero since you cannot have elements
   * with no nodes. It *is* permissible to have zero elements with
   * non-zero node count.
   */
     
  if (num_nodes > 0) {
    if ((numnoddim = ncdimdef (exoid, DIM_NUM_NODES, (long)num_nodes)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to define number of nodes in file id %d",exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }
  }
  
  if (num_elem > 0) {
    if (num_nodes <=  0) {
      exerrval = EX_MSG;
      sprintf(errmsg,
              "Error: Cannot have non-zero element count if node count is zero.in file id %d",exoid);
      ex_err("ex_put_init",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
    
    if ((ncdimdef (exoid, DIM_NUM_ELEM, (long)num_elem)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to define number of elements in file id %d",exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }
  }

  /* Can have nonzero num_elem_blk even if num_elem == 0 */
  if (num_elem_blk > 0) {
    if ((elblkdim = ncdimdef (exoid, DIM_NUM_EL_BLK, (long)num_elem_blk)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to define number of element blocks in file id %d",
                exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }
    
    /* ...and some variables */
    
    /* element block id status array */
    
    dim[0] = elblkdim;
    if ((varid = ncvardef (exoid, VAR_STAT_EL_BLK, NC_LONG, 1, dim)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to define element block status array in file id %d",exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }
    
#if 0
    /*   store property name as attribute of property array variable */
    if ((ncattput (exoid, varid, ATT_PROP_NAME, NC_CHAR, 7, "STATUS")) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to store element block property name %s in file id %d",
                "STATUS",exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        return (EX_FATAL);
      }
#endif
    
    /* element block id array */
    
    if ((varid = ncvardef (exoid, VAR_ID_EL_BLK, NC_LONG, 1, dim)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to define element block id array in file id %d",exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }
    
    /*   store property name as attribute of property array variable */
    if ((ncattput (exoid, varid, ATT_PROP_NAME, NC_CHAR, 3, "ID")) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to store element block property name %s in file id %d",
                "ID",exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        return (EX_FATAL);
      }
  }

  /* node set id array: */

  if (num_node_sets > 0) {

    if ((dimid = ncdimdef (exoid, DIM_NUM_NS, (long)num_node_sets)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to define number of node sets in file id %d",exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }

    /* node set id status array: */

    dim[0] = dimid;
    if ((varid = ncvardef (exoid, VAR_NS_STAT, NC_LONG, 1, dim)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to create node sets status array in file id %d",exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }


#if 0
    /*   store property name as attribute of property array variable */
    if ((ncattput (exoid, varid, ATT_PROP_NAME, NC_CHAR, 7, "STATUS")) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to store node set property name %s in file id %d",
                "ID",exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        return (EX_FATAL);
      }
#endif

    /* node set id array: */

    dim[0] = dimid;
    if ((varid = ncvardef (exoid, VAR_NS_IDS, NC_LONG, 1, dim)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to create node sets property array in file id %d",exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }


    /*   store property name as attribute of property array variable */
    if ((ncattput (exoid, varid, ATT_PROP_NAME, NC_CHAR, 3, "ID")) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to store node set property name %s in file id %d",
                "ID",exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        return (EX_FATAL);
      }


  }

  /* side set id array: */

  if (num_side_sets > 0) {

    if ((dimid = ncdimdef (exoid, DIM_NUM_SS, (long)num_side_sets)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to define number of side sets in file id %d",exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }

    /* side set id status array: */

    dim[0] = dimid;
    if ((varid = ncvardef (exoid, VAR_SS_STAT, NC_LONG, 1, dim)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to define side set status in file id %d",exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }

#if 0
    /*   store property name as attribute of property array variable */
    if ((ncattput (exoid, varid, ATT_PROP_NAME, NC_CHAR, 7, "STATUS")) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to store side set property name %s in file id %d",
                "ID",exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        return (EX_FATAL);
      }
#endif
    
    /* side set id array: */

    if ((varid = ncvardef (exoid, VAR_SS_IDS, NC_LONG, 1, dim)) == -1) 
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to define side set property in file id %d",exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }

    /*   store property name as attribute of property array variable */
    if ((ncattput (exoid, varid, ATT_PROP_NAME, NC_CHAR, 3, "ID")) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to store side set property name %s in file id %d",
                "ID",exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        return (EX_FATAL);
      }

  }

  /*
   * To reduce the maximum dataset sizes, the storage of the nodal
   * coordinates and the nodal variables was changed from a single
   * dataset to a dataset per component or variable.  However, we
   * want to maintain some form of compatability with the old
   * exodusII version.  It is easy to do this on read; however, we
   * also want to be able to store in the old format using the new
   * library. 
   *
   * The mode is set in the ex_create call. The setting can be checked
   * via the ATT_FILESIZE attribute in the file (1=large,
   * 0=normal). Also handle old files that do not contain this
   * attribute.
   */

  if (num_nodes > 0) {
    if (ex_large_model(exoid) == 1) {
      /* node coordinate arrays -- separate storage... */

      /*
       * Check that storage required for coordinates  is less
       * than 2GB which is maximum size permitted by netcdf
       * (in large file mode). 1<<29 == max number of integer items.
       */
      int shift = nc_flt_code(exoid) == NC_DOUBLE ? 28 : 29; 
      if (num_nodes  > (1<<shift)) {
        exerrval = EX_BADPARAM;
        sprintf(errmsg,
                "Error: Size to store nodal coordinates exceeds 2GB in file id %d",
                exoid);
        ex_err("ex_put_init",errmsg,exerrval);
        return (EX_FATAL);
      }
    
      dim[0] = numnoddim;
      if (num_dim > 0) {
        if (ncvardef (exoid, VAR_COORD_X, nc_flt_code(exoid), 1, dim) == -1)
          {
            exerrval = ncerr;
            sprintf(errmsg,
                    "Error: failed to define node x coordinate array in file id %d",exoid);
            ex_err("ex_put_init",errmsg,exerrval);
            goto error_ret;         /* exit define mode and return */
          }
      }
    
      if (num_dim > 1) {
        if (ncvardef (exoid, VAR_COORD_Y, nc_flt_code(exoid), 1, dim) == -1)
          {
            exerrval = ncerr;
            sprintf(errmsg,
                    "Error: failed to define node y coordinate array in file id %d",exoid);
            ex_err("ex_put_init",errmsg,exerrval);
            goto error_ret;         /* exit define mode and return */
          }
      }

      if (num_dim > 2) {
        if (ncvardef (exoid, VAR_COORD_Z, nc_flt_code(exoid), 1, dim) == -1)
          {
            exerrval = ncerr;
            sprintf(errmsg,
                    "Error: failed to define node z coordinate array in file id %d",exoid);
            ex_err("ex_put_init",errmsg,exerrval);
            goto error_ret;         /* exit define mode and return */
          }
      }
    } else {
      /* node coordinate arrays: -- all stored together (old method) */

      dim[0] = numdimdim;
      dim[1] = numnoddim;
      if (ncvardef (exoid, VAR_COORD, nc_flt_code(exoid), 2, dim) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
                  "Error: failed to define node coordinate array in file id %d",exoid);
          ex_err("ex_put_init",errmsg,exerrval);
          goto error_ret;         /* exit define mode and return */
        }
    }
  }
  
  /* inquire previously defined dimensions  */
  if ((strdim = ncdimid (exoid, DIM_STR)) < 0)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to get string length in file id %d",exoid);
      ex_err("ex_put_init",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

  /* coordinate names array */

  dim[0] = numdimdim;
  dim[1] = strdim;

  if (ncvardef (exoid, VAR_NAME_COOR, NC_CHAR, 2, dim) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to define coordinate name array in file id %d",exoid);
      ex_err("ex_put_init",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
  

  /* leave define mode */
#if 1
  if (ncendef (exoid) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to complete variable definitions in file id %d",exoid);
      ex_err("ex_put_init",errmsg,exerrval);
      return (EX_FATAL);
    }
  
#else
  /* estimate (guess) size of header of netCDF file */
  header_size = 1200 + 
    num_elem_blk * 800 + 
    num_node_sets * 220 + 
    num_side_sets * 300;

  if (header_size > MAX_HEADER_SIZE) header_size = MAX_HEADER_SIZE;

  /* estimate (guess) size of fixed size variable section of netCDF file */

  if (nc_flt_code(exoid) == NC_DOUBLE) 
    iows = 8;
  else
    iows = 4;

  fixed_var_size = num_dim * num_nodes * iows +
    num_nodes * sizeof(int) +
    num_elem * 16 * sizeof(int) +
    num_elem_blk * sizeof(int) +
    num_node_sets * num_nodes/100 * sizeof(int) +
    num_node_sets * num_nodes/100 * iows +
    num_node_sets * sizeof(int) +
    num_side_sets * num_elem/100 * 2 * sizeof(int) +
    num_side_sets * num_elem/100 * iows +
    num_side_sets * sizeof(int);



  /* With netcdf-3.4, this produces very large files on the
   * SGI.  Also with netcdf-3.5beta3
   */
  /*
   * This is also causing other problems on other systems .. disable for now
   */
  if (nc__enddef (exoid, 
                  header_size, NC_ALIGN_CHUNK, 
                  fixed_var_size, NC_ALIGN_CHUNK) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to complete variable definitions in file id %d",exoid);
      ex_err("ex_put_init",errmsg,exerrval);
      return (EX_FATAL);
    }

#endif
  return (EX_NOERR);

  /* Fatal error: exit definition mode and return */
 error_ret:
  if (ncendef (exoid) == -1)     /* exit define mode */
    {
      sprintf(errmsg,
              "Error: failed to complete definition for file id %d",
              exoid);
      ex_err("ex_put_init",errmsg,exerrval);
    }
  return (EX_FATAL);
}
