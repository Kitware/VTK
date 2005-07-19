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
* expcns - ex_put_concat_node_sets
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
*       int*    node_set_ids            array of node set ids
*       int*    num_nodes_per_set       array of number of nodes per set
*       int*    num_dist_per_set        array of number of dist fact  per set
* ----------pass in NULL for remaining args if just want to set params -------------
*       int*    node_sets_node_index    array of set indices into node list
*       int*    node_sets_df_index      array of set indices into dist fact list
*       int*    node_set_node_list      array of node list #'s for node set
*       void*   node_set_dist_fact      array of dist factors for node set
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
 * writes the node set ID's, node set count array, node set pointers array, 
 * and node set node list for all of the node sets
 */

int ex_put_concat_node_sets (int   exoid,
                             int  *node_set_ids,
                             int  *num_nodes_per_set,
                             int  *num_dist_per_set,
                             int  *node_sets_node_index,
                             int  *node_sets_df_index,
                             int  *node_sets_node_list,
                             void *node_sets_dist_fact)
{
   int i, num_node_sets, cur_num_node_sets, dimid, varid, dims[1], *ns_stat;
   int iresult;
   long start[1], count[1]; 
   nclong *lptr;
   float fdum;
   char *cdum;
   float  *flt_dist_fact;
   double *dbl_dist_fact;
   char errmsg[MAX_ERR_LENGTH];

   (void)node_sets_df_index;

  exerrval = 0; /* clear error code */

  cdum = 0; /* initialize even though it is not used */

/* first check if any node sets are specified */

  if (ncdimid (exoid, DIM_NUM_NS)  == -1)
  { 
    if (ncerr == NC_EBADDIM)
    {
      exerrval = ncerr;
      sprintf(errmsg,
             "Error: no node sets defined for file id %d", exoid);
      ex_err("ex_put_concat_node_sets",errmsg,exerrval);
    }
    else
    {
      exerrval = ncerr;
      sprintf(errmsg,
             "Error: failed to locate node sets defined in file id %d", exoid);
      ex_err("ex_put_concat_node_sets",errmsg,exerrval);
    }
    return (EX_FATAL);
  }

/* inquire how many node sets are to be stored */

  if (ex_inquire(exoid, EX_INQ_NODE_SETS, &num_node_sets, &fdum, cdum) == -1)
  {
    sprintf(errmsg,
           "Error: failed to get number of node sets defined for file id %d",
            exoid);
    /* use error val from inquire */
    ex_err("ex_put_concat_node_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* Fill out node set status array */

  /* First, allocate space for the node set status list */
  if (!(ns_stat= malloc(num_node_sets*sizeof(int))))
  {
    exerrval = EX_MEMFAIL;
    sprintf(errmsg,
    "Error: failed to allocate space for node set status array in file id %d",
            exoid);
    ex_err("ex_put_concat_node_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  for (i=0;i<num_node_sets;i++)
  {
    if (num_nodes_per_set[i] == 0) /* Is this a NULL node set? */
      ns_stat[i] = 0; /* change node set status to NULL */
    else
      ns_stat[i] = 1; /* change node set status to TRUE */
  }

  /* Next, get variable id of status array */
  if ((varid = ncvarid (exoid, VAR_NS_STAT)) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to locate node set status in file id %d", exoid);
    ex_err("ex_put_concat_node_set",errmsg,exerrval);
    return (EX_FATAL);
  }

/* this contortion is necessary because netCDF is expecting nclongs; fortunately
   it's necessary only when ints and nclongs aren't the same size */

  start[0] = 0;
  count[0] = num_node_sets;

  if (sizeof(int) == sizeof(nclong)) {
     iresult = ncvarput (exoid, varid, start, count, ns_stat);
  } else {
     lptr = itol (ns_stat, num_node_sets);
     iresult = ncvarput (exoid, varid, start, count, lptr);
     free(lptr);
  }

  if (iresult == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to store node set status array to file id %d",
            exoid);
    ex_err("ex_put_concat_node_set",errmsg,exerrval);
    return (EX_FATAL);
  }
  free(ns_stat);

/* put netcdf file into define mode  */

  if (ncredef (exoid) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to put file id %d into define mode",
            exoid);
    ex_err("ex_put_concat_node_sets",errmsg,exerrval);
    return (EX_FATAL);
  }


/* create node set definitions */
  for (i=0; i<num_node_sets; i++)
  {

/* Keep track of the total number of node sets defined using a counter stored
   in a linked list keyed by exoid.
   NOTE: ex_get_file_item  is used to find the number of node sets
         for a specific file and returns that value.
*/
    cur_num_node_sets=ex_get_file_item(exoid, &ns_ctr_list );
    if (cur_num_node_sets >= num_node_sets)
    {
      exerrval = EX_FATAL;
      sprintf(errmsg,
             "Error: exceeded number of node sets (%d) defined in file id %d",
              num_node_sets,exoid);
      ex_err("ex_put_concat_node_sets",errmsg,exerrval);
      goto error_ret;
    }

/*  NOTE: ex_inc_file_item  is used to find the number of node sets
         for a specific file and returns that value incremented. */

    cur_num_node_sets=ex_inc_file_item(exoid, &ns_ctr_list );

/*  define dimension for number of nodes in node set */

    if (num_nodes_per_set[i] == 0) /* Is this a NULL node set? */
      continue; /* Do not create anything for NULL node sets! */


    if ((dimid = ncdimdef (exoid, DIM_NUM_NOD_NS(cur_num_node_sets+1),
                   (long)num_nodes_per_set[i])) == -1)
    {
      exerrval = ncerr;
      if (ncerr == NC_ENAMEINUSE)
      {
        sprintf(errmsg,
               "Error: node set %d already defined in file id %d",
                node_set_ids[i],exoid);
        ex_err("ex_put_concat_node_sets",errmsg,exerrval);
      }
      else
      {
        sprintf(errmsg,
             "Error: failed to define number of nodes for set %d in file id %d",
                node_set_ids[i],exoid);
        ex_err("ex_put_concat_node_sets",errmsg,exerrval);
      }
      goto error_ret;
    }


/*  define variable to store node set node list here instead of in expns */

    dims[0] = dimid;
    if (ncvardef(exoid,VAR_NODE_NS(cur_num_node_sets+1),NC_LONG,1,dims) == -1)
    {
      exerrval = ncerr;
      if (ncerr == NC_ENAMEINUSE)
      {
        sprintf(errmsg,
               "Error: node set %d node list already defined in file id %d",
                node_set_ids[i],exoid);
        ex_err("ex_put_concat_node_sets",errmsg,exerrval);
      }
      else
      {
        sprintf(errmsg,
               "Error: failed to create node set %d node list in file id %d",
                node_set_ids[i],exoid);
        ex_err("ex_put_concat_node_sets",errmsg,exerrval);
      }
      goto error_ret;            /* exit define mode and return */
    }


/*  Create variable for distribution factors if required */

    if (num_dist_per_set[i] > 0)        /* only define it if needed */
    {
    /* num_dist_per_set should equal num_nodes_per_set */
      if (num_dist_per_set[i] != num_nodes_per_set[i])
      {
        exerrval = EX_FATAL;
        sprintf(errmsg,
  "Error: # dist fact (%d) not equal to # nodes (%d) in node set %d file id %d",
            num_dist_per_set[i], num_nodes_per_set[i], node_set_ids[i],exoid);
        ex_err("ex_put_concat_node_sets",errmsg,exerrval);
        goto error_ret;          /* exit define mode and return */
      }
      else
      {
        /* create variable for distribution factors */

        if (ncvardef (exoid, VAR_FACT_NS(cur_num_node_sets+1),
                      nc_flt_code(exoid), 1, dims) == -1)
        {
          exerrval = ncerr;
          if (ncerr == NC_ENAMEINUSE)
          {
            sprintf(errmsg,
                  "Error: node set %d dist factors already exist in file id %d",
                    node_set_ids[i],exoid);
            ex_err("ex_put_concat_node_sets",errmsg,exerrval);
          }
          else
          {
            sprintf(errmsg,
               "Error: failed to create node set %d dist factors in file id %d",
                    node_set_ids[i],exoid);
            ex_err("ex_put_concat_node_sets",errmsg,exerrval);
          }
          goto error_ret;            /* exit define mode and return */
        }
      }
    }
  }

/* leave define mode  */

  if (ncendef (exoid) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to complete definition in file id %d",
            exoid);
    ex_err("ex_put_concat_node_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

/* Next, fill out node set ids array */

  /* first get id of node set ids array variable */

  if ((varid = ncvarid (exoid, VAR_NS_IDS)) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to locate node set ids array in file id %d",
            exoid);
    ex_err("ex_put_concat_node_sets",errmsg,exerrval);
    return (EX_FATAL);
  }
  /* then, write out node set ids */

/* this contortion is necessary because netCDF is expecting nclongs; fortunately
   it's necessary only when ints and nclongs aren't the same size */

  start[0] = 0;
  count[0] = num_node_sets;

  if (sizeof(int) == sizeof(nclong)) {
     iresult = ncvarput (exoid, varid, start, count, node_set_ids);
  } else {
     lptr = itol (node_set_ids, num_node_sets);
     iresult = ncvarput (exoid, varid, start, count, lptr);
     free(lptr);
  }

  if (iresult == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to store node set id array in file id %d",
            exoid);
    ex_err("ex_put_concat_node_sets",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* If the node_sets_node_index is passed in as a NULL pointer, then
   *  the user only wants us to define the nodesets and not populate
   *  the data structures.
   */
  if (node_sets_node_index == 0)
    return(EX_NOERR);
  
  /* Now, use ExodusII call to store node lists */

  for (i=0; i<num_node_sets; i++)
  {

    if (num_nodes_per_set[i] == 0) /* Is this a NULL node set? */
      continue; /* Do not create anything for NULL node sets! */

    if (ex_comp_ws(exoid) == sizeof(float))     /* 4-byte float word */
    {

      flt_dist_fact = node_sets_dist_fact;
      if (ex_put_node_set(exoid, node_set_ids[i],
                         &(node_sets_node_list[node_sets_node_index[i]])) == -1)
      {
        sprintf(errmsg,
               "Error: failed to store node set %d for file id %d",
                node_set_ids[i],exoid);
        /* use error val from exodusII routine */
        ex_err("ex_put_concat_node_sets",errmsg,exerrval);
        return (EX_FATAL);
      }
      if (num_dist_per_set[i] > 0)              /* only store if they exist */
      {
        if (ex_put_node_set_dist_fact(exoid, node_set_ids[i],
                         &(      flt_dist_fact[node_sets_node_index[i]])) == -1)
        {
          sprintf(errmsg,
               "Error: failed to store node set %d dist factors for file id %d",
                  node_set_ids[i],exoid);
          /* use error val from exodusII routine */
          ex_err("ex_put_concat_node_sets",errmsg,exerrval);
          return (EX_FATAL);
        }
      }
    } 
    else if( ex_comp_ws(exoid) == sizeof(double))       /* 8-byte float word */
    {
      dbl_dist_fact = node_sets_dist_fact;
      if (ex_put_node_set(exoid, node_set_ids[i],
                       &(node_sets_node_list[node_sets_node_index[i]])) == -1)
      {
        sprintf(errmsg,
               "Error: failed to store node set %d for file id %d",
                node_set_ids[i],exoid);
        /* use error val from exodusII routine */
        ex_err("ex_put_concat_node_sets",errmsg,exerrval);
        return (EX_FATAL);
      }
      if (num_dist_per_set[i] > 0)              /* only store if they exist */
      {
        if (ex_put_node_set_dist_fact(exoid, node_set_ids[i],
                         &(      dbl_dist_fact[node_sets_node_index[i]])) == -1)
        {
          sprintf(errmsg,
               "Error: failed to store node set %d dist factors for file id %d",
                  node_set_ids[i],exoid);
          /* use error val from exodusII routine */
          ex_err("ex_put_concat_node_sets",errmsg,exerrval);
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
      ex_err("ex_put_concat_node_sets", errmsg, exerrval);
      return (EX_FATAL);
    }
  }

  return (EX_NOERR);


/* Fatal error: exit definition mode and return */
error_ret:
  if (ncendef (exoid) == -1)     /* exit define mode */
  {
    sprintf(errmsg,
           "Error: failed to complete definition for file id %d",
            exoid);
    ex_err("ex_put_concat_node_sets",errmsg,exerrval);
  }
  return (EX_FATAL);

}
