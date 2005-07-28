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
* expcss - ex_put_concat_side_sets
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
*       int     *side_set_ids           array of side set ids
*       int     *num_elem_per_set       number of elements/sides/faces per set
*       int     *num_dist_per_set       number of distribution factors per set
* ----------pass in NULL for remaining args if just want to set params -------------
*       int     *side_sets_elem_index   index array of elements into elem list
*       int     *side_sets_dist_index   index array of df into df list
*       int     *side_sets_elem_list    array of elements
*       int     *side_sets_side_list    array of sides/faces
*       void    *side_sets_dist_fact    array of distribution factors
*
* exit conditions - 
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*
 * writes the side set ID's, side set element count array,
 * side set element pointers array, side set element list,
 * side set side list, and distribution factors list.
 */

int ex_put_concat_side_sets (int   exoid,
                             const int  *side_set_ids,
                             const int  *num_elem_per_set,
                             const int  *num_dist_per_set,
                             const int  *side_sets_elem_index,
                             const int  *side_sets_dist_index,
                             const int  *side_sets_elem_list,
                             const int  *side_sets_side_list,
                             const void *side_sets_dist_fact)
{
  char *cdum;
  int i, num_side_sets, cur_num_side_sets, dimid, varid, dims[1], *ss_stat;
  int iresult;
  long start[1], count[1]; 
  nclong *lptr;
  float fdum;
  const float *flt_dist_fact;
  const double *dbl_dist_fact;
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  cdum = 0; /* initialize even though it is not used */

/* first check if any side sets are specified */

  if (ncdimid (exoid, DIM_NUM_SS) == -1)
  {
    if (ncerr == NC_EBADDIM)
    {
      exerrval = ncerr;
      sprintf(errmsg,
             "Error: no side sets defined for file id %d", exoid);
      ex_err("ex_put_concat_side_sets",errmsg,exerrval);
    }
    else
    {
      exerrval = ncerr;
      sprintf(errmsg,
             "Error: failed to locate side sets defined in file id %d", exoid);
      ex_err("ex_put_concat_side_sets",errmsg,exerrval);
    }
    return (EX_FATAL);
  }

/* inquire how many side sets are to be stored */

  if (ex_inquire(exoid, EX_INQ_SIDE_SETS, &num_side_sets, &fdum, cdum) == -1)
  {
    sprintf(errmsg,
           "Error: failed to get number of side sets defined for file id %d",
            exoid);
    /* use error val from inquire */
    ex_err("ex_put_concat_side_sets",errmsg,exerrval);
    return (EX_FATAL);
  }
/* fill out side set status array */

  /* First, allocate space for the side set status list */
  if (!(ss_stat= malloc(num_side_sets*sizeof(int))))
  {
    exerrval = EX_MEMFAIL;
    sprintf(errmsg,
    "Error: failed to allocate space for side set status array in file id %d",
            exoid);
    ex_err("ex_put_concat_side_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  for (i=0;i<num_side_sets;i++)
  {
    if (num_elem_per_set[i] == 0) /* Is this a NULL side set? */
      ss_stat[i] = 0; /* change side set status to NULL */
    else
      ss_stat[i] = 1; /* change side set status to TRUE */
  }

  /* Next, get variable id of status array */
  if ((varid = ncvarid (exoid, VAR_SS_STAT)) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to locate side set status in file id %d", exoid);
    ex_err("ex_put_concat_node_set",errmsg,exerrval);
    return (EX_FATAL);
  }

/* this contortion is necessary because netCDF is expecting nclongs; fortunately
   it's necessary only when ints and nclongs aren't the same size */

  start[0] = 0;
  count[0] = num_side_sets;

  if (sizeof(int) == sizeof(nclong)) {
     iresult = ncvarput (exoid, varid, start, count, ss_stat);
  } else {
     lptr = itol (ss_stat, num_side_sets);
     iresult = ncvarput (exoid, varid, start, count, lptr);
     free(lptr);
  }

  if (iresult == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to store side set status array to file id %d",
            exoid);
    ex_err("ex_put_concat_side_set",errmsg,exerrval);
    return (EX_FATAL);
  }

  free(ss_stat);

/* put netcdf file into define mode  */

  if (ncredef (exoid) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to put file id %d into define mode",
            exoid);
    ex_err("ex_put_concat_side_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* create side set definitions */
  for (i=0; i<num_side_sets; i++)
  {
/* Keep track of the total number of side sets defined using a counter stored
   in a linked list keyed by exoid.
   NOTE: ex_get_file_item  is used to find the number of side sets
         for a specific file and returns that value.
*/
    cur_num_side_sets=ex_get_file_item(exoid, &ss_ctr_list );
    if (cur_num_side_sets >= num_side_sets)
    {
      exerrval = EX_FATAL;
      sprintf(errmsg,
             "Error: exceeded number of side sets (%d) defined in file id %d",
              num_side_sets,exoid);
      ex_err("ex_put_concat_side_sets",errmsg,exerrval);
      goto error_ret;
    }

/*   NOTE: ex_inc_file_item  is used to find the number of side sets
         for a specific file and returns that value incremented. */

    cur_num_side_sets=ex_inc_file_item(exoid, &ss_ctr_list );

/*  define dimension for number of sides/elements per side set */

    if (num_elem_per_set[i] == 0) /* Is this a NULL side set? */
      continue; /* Do not create anything for NULL side sets! */


    if ((dimid = ncdimdef (exoid, DIM_NUM_SIDE_SS(cur_num_side_sets+1),
                   (long)num_elem_per_set[i])) == -1)
    {
      if (ncerr == NC_ENAMEINUSE)
      {
        exerrval = ncerr;
        sprintf(errmsg,
               "Error: side set side count %d already defined in file id %d",
                side_set_ids[i],exoid);
        ex_err("ex_put_concat_side_sets",errmsg,exerrval);
      }
      else
      {
        exerrval = ncerr;
        sprintf(errmsg,
             "Error: failed to define number of sides for set %d in file id %d",
                side_set_ids[i],exoid);
        ex_err("ex_put_concat_side_sets",errmsg,exerrval);
      }
      goto error_ret;
    }

    /* create element list variable for side set */

    dims[0] = dimid;
    if (ncvardef (exoid,VAR_ELEM_SS(cur_num_side_sets+1),NC_LONG,1,dims) == -1)
    {
      if (ncerr == NC_ENAMEINUSE)
      {
        exerrval = ncerr;
        sprintf(errmsg,
             "Error: element list already exists for side set %d in file id %d",
                side_set_ids[i],exoid);
        ex_err("ex_put_concat_side_sets",errmsg,exerrval);
      }
      else
      {
        exerrval = ncerr;
        sprintf(errmsg,
           "Error: failed to create element list for side set %d in file id %d",
                side_set_ids[i],exoid);
        ex_err("ex_put_concat_side_sets",errmsg,exerrval);
      }
      goto error_ret;            /* exit define mode and return */
    }

    /* create side list variable for side set */

    if (ncvardef (exoid,VAR_SIDE_SS(cur_num_side_sets+1),NC_LONG,1,dims) == -1)
    {
      if (ncerr == NC_ENAMEINUSE)
      {
        exerrval = ncerr;
        sprintf(errmsg,
               "Error: side list already exists for side set %d in file id %d",
                side_set_ids[i],exoid);
        ex_err("ex_put_concat_side_sets",errmsg,exerrval);
      }
      else
      {
        exerrval = ncerr;
        sprintf(errmsg,
              "Error: failed to create side list for side set %d in file id %d",
                side_set_ids[i],exoid);
        ex_err("ex_put_concat_side_sets",errmsg,exerrval);
      }
      goto error_ret;         /* exit define mode and return */

     }

/*  define dimension for number of dist factors/nodes per side set */

/*  NOTE: only define df count if  the dist factors exist! */

    if (num_dist_per_set[i] > 0)
    {

      if ((dimid = ncdimdef (exoid, DIM_NUM_DF_SS(cur_num_side_sets+1),
                              (long)num_dist_per_set[i])) == -1)
      {
        if (ncerr == NC_ENAMEINUSE)
        {
          exerrval = ncerr;
          sprintf(errmsg,
                 "Error: side set df count %d already defined in file id %d",
                  side_set_ids[i],exoid);
          ex_err("ex_put_concat_side_sets",errmsg,exerrval);
        }
        else
        {
          exerrval = ncerr;
          sprintf(errmsg,
           "Error: failed to define side set df count for set %d in file id %d",
                  side_set_ids[i],exoid);
          ex_err("ex_put_concat_side_sets",errmsg,exerrval);
        }
        goto error_ret;
      }

      /* create distribution factor list variable for side set */

      dims[0] = dimid;
      if (ncvardef (exoid, VAR_FACT_SS(cur_num_side_sets+1),
                    nc_flt_code(exoid), 1, dims) == -1)
      {
        if (ncerr == NC_ENAMEINUSE)
        {
          exerrval = ncerr;
          sprintf(errmsg,
         "Error: dist factor list already exists for side set %d in file id %d",
                  side_set_ids[i],exoid);
          ex_err("ex_put_concat_side_sets",errmsg,exerrval);
        }
        else
        {
          exerrval = ncerr;
          sprintf(errmsg,
       "Error: failed to create dist factor list for side set %d in file id %d",
                  side_set_ids[i],exoid);
          ex_err("ex_put_concat_side_sets",errmsg,exerrval);
        }
        goto error_ret;            /* exit define mode and return */
      }
    } /* end define dist factors */
  }

/* leave define mode  */

  if (ncendef (exoid) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to complete definition in file id %d",
            exoid);
    ex_err("ex_put_concat_side_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

/* Next, fill out side set ids array */

  /* first get id of side set ids array variable */

  if ((varid = ncvarid (exoid, VAR_SS_IDS)) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to locate side set ids array in file id %d",
            exoid);
    ex_err("ex_put_concat_side_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* then, write out side set id list */

/* this contortion is necessary because netCDF is expecting nclongs; fortunately
   it's necessary only when ints and nclongs aren't the same size */

  start[0] = 0;
  count[0] = num_side_sets;

  if (sizeof(int) == sizeof(nclong)) {
     iresult = ncvarput (exoid, varid, start, count, side_set_ids);
  } else {
     lptr = itol (side_set_ids, num_side_sets);
     iresult = ncvarput (exoid, varid, start, count, lptr);
     free(lptr);
  }

  if (iresult == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to store side set id array in file id %d",
            exoid);
    ex_err("ex_put_concat_side_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* If the side_sets_elem_index is passed in as a NULL pointer, then
   *  the user only wants us to define the sidesets and not populate
   *  the data structures.
   */
  if (side_sets_elem_index == 0)
    return(EX_NOERR);
  
  /* Now, use ExodusII call to store side sets */
  for (i=0; i<num_side_sets; i++)
  {

    if (num_elem_per_set[i] == 0) /* Is this a NULL side set? */
      continue; /* Do not create anything for NULL side sets! */

    if (ex_comp_ws(exoid) == sizeof(float))
    {
      flt_dist_fact = side_sets_dist_fact;
      if (ex_put_side_set(exoid, side_set_ids[i], 
                         &(side_sets_elem_list[side_sets_elem_index[i]]),
                         &(side_sets_side_list[side_sets_elem_index[i]])) == -1)
         return(EX_FATAL); /* error will be reported by subroutine */
      if (num_dist_per_set[i] > 0)      /* store dist factors if required */
      {
        if (ex_put_side_set_dist_fact(exoid, side_set_ids[i],
                         &(flt_dist_fact[side_sets_dist_index[i]])) == -1)
        {
          sprintf(errmsg,
               "Error: failed to store side set %d dist factors for file id %d",
                  side_set_ids[i],exoid);
          /* use error val from exodusII routine */
          ex_err("ex_put_concat_side_sets",errmsg,exerrval);
          return (EX_FATAL);
        }
      }
    }
    else if (ex_comp_ws(exoid) == sizeof(double))
    {
      dbl_dist_fact = side_sets_dist_fact;
      if (ex_put_side_set(exoid, side_set_ids[i], 
                         &(side_sets_elem_list[side_sets_elem_index[i]]),
                         &(side_sets_side_list[side_sets_elem_index[i]])) == -1)
        return(EX_FATAL); /* error will be reported by subroutine */
      if (num_dist_per_set[i] > 0)             /* only store if they exist */
      {
        if (ex_put_side_set_dist_fact(exoid, side_set_ids[i],
                        &(dbl_dist_fact[side_sets_dist_index[i]])) == -1)
        {
          sprintf(errmsg,
               "Error: failed to store side set %d dist factors for file id %d",
                  side_set_ids[i],exoid);
          /* use error val from exodusII routine */
          ex_err("ex_put_concat_side_sets",errmsg,exerrval);
          return (EX_FATAL);
        }
      }
    }
    else
    {
      /* unknown floating point word size */
      exerrval = EX_BADPARAM;
      sprintf(errmsg,
             "Error: unsupported floating point word size %d for file id %d",
              ex_comp_ws(exoid), exoid);
      ex_err("ex_put_concat_side_sets", errmsg, exerrval);
      return (EX_FATAL);
    }
  }

  return(EX_NOERR);


/* Fatal error: exit definition mode and return */
error_ret:
  if (ncendef (exoid) == -1)     /* exit define mode */
  {
    sprintf(errmsg,
           "Error: failed to complete definition for file id %d",
            exoid);
    ex_err("ex_put_concat_side_sets",errmsg,exerrval);
  }
  return (EX_FATAL);
}
