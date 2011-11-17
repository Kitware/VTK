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
* exgsnl - ex_get_side_set_node_list_len
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     side_set_id             side set id
*
* exit conditions - 
*       int     *side_set_node_list_len length of node list
*
* revision history - 
*
*
*****************************************************************************/

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * This routine is designed to read the Exodus II V 2.0 side set side 
 * definition  and return the length of a ExodusI style side set node list.
 * \param           exoid                   exodus file id
 * \param           side_set_id             side set id
 * \param[out]     *side_set_node_list_len length of node list
 */

int ex_get_side_set_node_list_len(int exoid,
                          int side_set_id,
                          int *side_set_node_list_len)
{
  int i, j;
  size_t m;
  int  num_side_sets, num_elem_blks, num_df, ndim;
  int tot_num_elem = 0, tot_num_ss_elem = 0; 
  int *elem_blk_ids;
  int *ss_elem_ndx;
  int *side_set_elem_list, *side_set_side_list;
  int elem_ctr; 
  int num_elem_in_blk, num_nodes_per_elem, num_attr;
  float fdum;
  char *cdum, elem_type[MAX_STR_LENGTH+1];

  struct elem_blk_parm  *elem_blk_parms;

  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  cdum = 0; /* initialize even though it is not used */

  *side_set_node_list_len = 0; /* default value */
/* first check if any side sets are specified */
/* inquire how many side sets have been stored */

  /* get the dimensionality of the coordinates;  this is necessary to
     distinguish between 2d TRIs and 3d TRIs */

  if ((ex_inquire(exoid, EX_INQ_DIM, &ndim, &fdum, cdum)) == -1)
  {
    sprintf(errmsg,
           "Error: failed to get dimensionality in file id %d",exoid);
    ex_err("ex_get_side_set_node_list_len",errmsg,exerrval);
    return(EX_FATAL);
  }

  if ((ex_inquire(exoid, EX_INQ_ELEM, &tot_num_elem, &fdum, cdum)) == -1)
  {
    sprintf(errmsg,
           "Error: failed to get total number of elements in file id %d",exoid);
    ex_err("ex_get_side_set_node_list_len",errmsg,exerrval);
    return(EX_FATAL);
  }

  if ((ex_inquire(exoid, EX_INQ_ELEM_BLK, &num_elem_blks, &fdum, cdum)) == -1)
  {
    sprintf(errmsg,
           "Error: failed to get number of element blocks in file id %d",exoid);
    ex_err("ex_get_side_set_node_list_len",errmsg,exerrval);
    return(EX_FATAL);
  }

  if ((ex_inquire(exoid, EX_INQ_SIDE_SETS, &num_side_sets, &fdum, cdum)) == -1)
  {
    sprintf(errmsg,
           "Error: failed to get number of side sets in file id %d",exoid);
    ex_err("ex_get_side_set_node_list_len",errmsg,exerrval);
    return(EX_FATAL);
  }

  if (num_side_sets == 0)
  {
    sprintf(errmsg,
           "Warning: no side sets defined in file id %d",exoid);
    ex_err("ex_get_side_set_node_list_len",errmsg,EX_WARN);
    return(EX_WARN);
  }

  /* First determine the  # of elements in the side set*/
  if ((ex_get_side_set_param(exoid,side_set_id,&tot_num_ss_elem,&num_df)) == -1)
  {
    sprintf(errmsg,
         "Error: failed to get number of elements in side set %d in file id %d",
            side_set_id, exoid);
    ex_err("ex_get_side_set_node_list_len",errmsg,exerrval);
    return(EX_FATAL);
  }

  if (tot_num_ss_elem == 0) /* NULL side set? */
    return (EX_NOERR); /* return zero */

  /* Minor optimization/kluge -- If num_df is nonzero, or 1 per face
     then assume that it matches the number of nodes in the sideset... */
  if (num_df > 0 && num_df != tot_num_ss_elem) {
    *side_set_node_list_len = num_df;
    return(EX_NOERR);
  }

  /* Allocate space for the side set element list */
  if (!(side_set_elem_list=malloc(tot_num_ss_elem*sizeof(int))))
  {
    exerrval = EX_MEMFAIL;
    sprintf(errmsg,
    "Error: failed to allocate space for side set element list for file id %d",
            exoid);
    ex_err("ex_get_side_set_node_list_len",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* Allocate space for the side set side list */
  if (!(side_set_side_list=malloc(tot_num_ss_elem*sizeof(int))))
  {
    free(side_set_elem_list);
    exerrval = EX_MEMFAIL;
    sprintf(errmsg,
    "Error: failed to allocate space for side set side list for file id %d",
            exoid);
    ex_err("ex_get_side_set_node_list_len",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (ex_get_side_set(exoid, side_set_id, 
                      side_set_elem_list, side_set_side_list) == -1)
  {
    free(side_set_elem_list);
    free(side_set_side_list);
    sprintf(errmsg,
    "Error: failed to get side set %d in file id %d",
            side_set_id, exoid);
    ex_err("ex_get_side_set_node_list_len",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* Allocate space for the ss element index array */
  if (!(ss_elem_ndx=malloc(tot_num_ss_elem*sizeof(int))))
  {
    free(side_set_elem_list);
    free(side_set_side_list);
    exerrval = EX_MEMFAIL;
    sprintf(errmsg,
 "Error: failed to allocate space for side set elem sort array for file id %d",
            exoid);
    ex_err("ex_get_side_set_node_list_len",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* Sort side set element list into index array  - non-destructive */
  for (i=0;i<tot_num_ss_elem;i++)
    ss_elem_ndx[i] = i; /* init index array to current position */

  ex_iqsort(side_set_elem_list, ss_elem_ndx,tot_num_ss_elem);


  /* Allocate space for the element block ids */
  if (!(elem_blk_ids=malloc(num_elem_blks*sizeof(int))))
  {
    exerrval = EX_MEMFAIL;
    free(ss_elem_ndx);
    free(side_set_side_list);
    free(side_set_elem_list);
    sprintf(errmsg,
        "Error: failed to allocate space for element block ids for file id %d",
            exoid);
    ex_err("ex_get_side_set_node_list_len",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (ex_get_elem_blk_ids(exoid, elem_blk_ids))
  {
    free(elem_blk_ids);
    free(ss_elem_ndx);
    free(side_set_side_list);
    free(side_set_elem_list);
    sprintf(errmsg,
           "Error: failed to get element block ids in file id %d",
            exoid);
    ex_err("ex_get_side_set_node_list_len",errmsg,EX_MSG);
    return(EX_FATAL);
  } 

  /* Allocate space for the element block params */
  if (!(elem_blk_parms=malloc(num_elem_blks*sizeof(struct elem_blk_parm))))
  {
    free(elem_blk_ids);
    free(ss_elem_ndx);
    free(side_set_side_list);
    free(side_set_elem_list);
    exerrval = EX_MEMFAIL;
    sprintf(errmsg,
      "Error: failed to allocate space for element block params for file id %d",
            exoid);
    ex_err("ex_get_side_set_node_list_len",errmsg,exerrval);
    return (EX_FATAL);
  }

  elem_ctr = 0;
  for (i=0; i<num_elem_blks; i++)
  {
    /* read in an element block parameter */
    if ((ex_get_elem_block (exoid,
                            elem_blk_ids[i],
                            elem_type,
                            &num_elem_in_blk,
                            &num_nodes_per_elem,
                            &num_attr)) == -1)
    {
      free(elem_blk_parms);
      free(elem_blk_ids);
      free(ss_elem_ndx);
      free(side_set_side_list);
      free(side_set_elem_list);
      sprintf(errmsg,
             "Error: failed to get element block %d parameters in file id %d",
              elem_blk_ids[i], exoid);
      ex_err("ex_get_side_set_node_list_len",errmsg,EX_MSG);
      return(EX_FATAL);
    }

    elem_blk_parms[i].num_elem_in_blk = num_elem_in_blk;
    elem_blk_parms[i].num_nodes_per_elem = num_nodes_per_elem;
    elem_blk_parms[i].num_attr = num_attr;
    elem_blk_parms[i].elem_blk_id = elem_blk_ids[i];

    for (m=0; m < strlen(elem_type); m++)
      elem_blk_parms[i].elem_type[m] = toupper(elem_type[m]);
    elem_blk_parms[i].elem_type[m] = EX_EL_NULL_ELEMENT;

    if (strncmp(elem_blk_parms[i].elem_type,"CIRCLE",3) == 0)
    {
      elem_blk_parms[i].elem_type_val = EX_EL_CIRCLE;
      /* set side set node stride */
        elem_blk_parms[i].num_nodes_per_side[0] = 1;
    }
    else if (strncmp(elem_blk_parms[i].elem_type,"SPHERE",3) == 0)
    {
      elem_blk_parms[i].elem_type_val = EX_EL_SPHERE;
      /* set side set node stride */
        elem_blk_parms[i].num_nodes_per_side[0] = 1;
    }
    else if (strncmp(elem_blk_parms[i].elem_type,"QUAD",3) == 0)
    {
      elem_blk_parms[i].elem_type_val = EX_EL_QUAD;
      /* determine side set node stride */
      if (elem_blk_parms[i].num_nodes_per_elem == 4)
        elem_blk_parms[i].num_nodes_per_side[0] = 2;
      else if (elem_blk_parms[i].num_nodes_per_elem == 5)
        elem_blk_parms[i].num_nodes_per_side[0] = 2;
      else 
        elem_blk_parms[i].num_nodes_per_side[0] = 3;
    }
    else if (strncmp(elem_blk_parms[i].elem_type,"TRIANGLE",3) == 0)
    {
      elem_blk_parms[i].elem_type_val = EX_EL_TRIANGLE;
      /* determine side set node stride */
      if (ndim == 2) /* 2d TRIs */
      {
        if (elem_blk_parms[i].num_nodes_per_elem == 3)
          elem_blk_parms[i].num_nodes_per_side[0] = 2;
        else 
          elem_blk_parms[i].num_nodes_per_side[0] = 3;
      }
      else if (ndim == 3)  /* 3d TRIs */
      {   /* set the default number of nodes per side; catch exceptions later */
        if (elem_blk_parms[i].num_nodes_per_elem == 3)
          elem_blk_parms[i].num_nodes_per_side[0] = 3;
        else 
          elem_blk_parms[i].num_nodes_per_side[0] = 6;
      }
    }
    else if (strncmp(elem_blk_parms[i].elem_type,"SHELL",3) == 0)
    {
      elem_blk_parms[i].elem_type_val = EX_EL_SHELL;
      /* determine side set node stride */
      if (elem_blk_parms[i].num_nodes_per_elem == 2) /* KLUDGE for 2D Shells*/
        elem_blk_parms[i].num_nodes_per_side[0] = 2;
      else if (elem_blk_parms[i].num_nodes_per_elem == 4)
        elem_blk_parms[i].num_nodes_per_side[0] = 4;
      else
        elem_blk_parms[i].num_nodes_per_side[0] = 8;
    }
    else if (strncmp(elem_blk_parms[i].elem_type,"HEX",3) == 0)
    {
      elem_blk_parms[i].elem_type_val = EX_EL_HEX;
      /* determine side set node stride */
      if (elem_blk_parms[i].num_nodes_per_elem == 8)
        elem_blk_parms[i].num_nodes_per_side[0] = 4;
      else if (elem_blk_parms[i].num_nodes_per_elem == 9)
        elem_blk_parms[i].num_nodes_per_side[0] = 4;
      else if (elem_blk_parms[i].num_nodes_per_elem == 12) /* HEXSHELL */
        elem_blk_parms[i].num_nodes_per_side[0] = 4;
      else if (elem_blk_parms[i].num_nodes_per_elem == 27)
        elem_blk_parms[i].num_nodes_per_side[0] = 9;
      else
        elem_blk_parms[i].num_nodes_per_side[0] = 8;
    }
    else if (strncmp(elem_blk_parms[i].elem_type,"TETRA",3) == 0)
    {
      elem_blk_parms[i].elem_type_val = EX_EL_TETRA;
      /* determine side set node stride */
      if (elem_blk_parms[i].num_nodes_per_elem == 4)
        elem_blk_parms[i].num_nodes_per_side[0] = 3;
      else if (elem_blk_parms[i].num_nodes_per_elem == 8)
        elem_blk_parms[i].num_nodes_per_side[0] = 4;
      else
        elem_blk_parms[i].num_nodes_per_side[0] = 6;
    }
    else if (strncmp(elem_blk_parms[i].elem_type,"WEDGE",3) == 0)
    {
      elem_blk_parms[i].elem_type_val = EX_EL_WEDGE;
      /* determine side set node stride */
      if (elem_blk_parms[i].num_nodes_per_elem == 6)
        elem_blk_parms[i].num_nodes_per_side[0] = 4;
      else
        elem_blk_parms[i].num_nodes_per_side[0] = 8;
    }
    else if (strncmp(elem_blk_parms[i].elem_type,"PYRAMID",3) == 0)
    {
      elem_blk_parms[i].elem_type_val = EX_EL_PYRAMID;
      /* determine side set node stride */
      if (elem_blk_parms[i].num_nodes_per_elem == 5)
        elem_blk_parms[i].num_nodes_per_side[0] = 4;
      else
        elem_blk_parms[i].num_nodes_per_side[0] = 8;
    }
    else if (strncmp(elem_blk_parms[i].elem_type,"BEAM",3) == 0)
    {
      elem_blk_parms[i].elem_type_val = EX_EL_BEAM;
      /* determine side set node stride */
      if (elem_blk_parms[i].num_nodes_per_elem == 2)
        elem_blk_parms[i].num_nodes_per_side[0] = 2;
      else 
        elem_blk_parms[i].num_nodes_per_side[0] = 3;
    }
    else if ( (strncmp(elem_blk_parms[i].elem_type,"TRUSS",3) == 0) ||
              (strncmp(elem_blk_parms[i].elem_type,"BAR",3) == 0)  ||
              (strncmp(elem_blk_parms[i].elem_type,"EDGE",3) == 0))
    {
      elem_blk_parms[i].elem_type_val = EX_EL_TRUSS;
      /* determine side set node stride */
      if (elem_blk_parms[i].num_nodes_per_elem == 2)
        elem_blk_parms[i].num_nodes_per_side[0] = 2;
      else 
        elem_blk_parms[i].num_nodes_per_side[0] = 3;
    }
    else if (strncmp(elem_blk_parms[i].elem_type,"NULL",3) == 0)
    {
      elem_blk_parms[i].elem_type_val = EX_EL_NULL_ELEMENT;
      elem_blk_parms[i].num_nodes_per_side[0] = 0;
      elem_blk_parms[i].num_elem_in_blk = 0;
    }
    else
    { /* unsupported element type; no problem if no sides specified for
         this element block */
      elem_blk_parms[i].elem_type_val = EX_EL_UNK;
      elem_blk_parms[i].num_nodes_per_side[0] = 0;
    }

    elem_ctr += elem_blk_parms[i].num_elem_in_blk;
    elem_blk_parms[i].elem_ctr = elem_ctr;      /* save elem number max */
  }

/* Walk through element list and keep a running count of the node length */

  *side_set_node_list_len = 0;
  for (i=0;i<tot_num_ss_elem;i++)
  {
    for (j=0; j<num_elem_blks; j++)
    {
      if (elem_blk_parms[j].elem_type_val != EX_EL_NULL_ELEMENT)
        if (side_set_elem_list[i] <= elem_blk_parms[j].elem_ctr)
          break; /* stop because we found the parameters for this element */
    }
    if (j >= num_elem_blks)
    {
      exerrval = EX_BADPARAM;
      sprintf(errmsg,
             "Error: Invalid element number %d found in side set %d in file %d",
              side_set_elem_list[i], side_set_id, exoid);
      free(elem_blk_parms);
      free(elem_blk_ids);
      free(ss_elem_ndx);
      free(side_set_side_list);
      free(side_set_elem_list);
      ex_err("ex_get_side_set_node_list_len",errmsg,EX_MSG);
      return (EX_FATAL);
    }

    /* Update *side_set_node_list_len (which points to next node in chain */

    /* WEDGEs with 3 node sides (side 4 or 5) are special cases */
    if (elem_blk_parms[j].elem_type_val == EX_EL_WEDGE &&
        (side_set_side_list[i] == 4 || side_set_side_list[i] == 5))
    {
      if (elem_blk_parms[j].num_nodes_per_elem == 6)
        *side_set_node_list_len += 3;  /* 3 node side */
      else
        *side_set_node_list_len += 6;  /* 6 node side */
    }
    /* PYRAMIDSs with 3 node sides (sides 1,2,3,4) are also special */
    else if (elem_blk_parms[j].elem_type_val == EX_EL_PYRAMID &&
             (side_set_side_list[i] < 5))
    {
      if (elem_blk_parms[j].num_nodes_per_elem == 5)
        *side_set_node_list_len += 3;  /* 3 node side */
      else
        *side_set_node_list_len += 6;  /* 6 node side */
    }
    /* side numbers 3,4,5,6 for SHELLs are also special */
    else if (elem_blk_parms[j].elem_type_val == EX_EL_SHELL &&
        (side_set_side_list[i] > 2 ))
    {
      if (elem_blk_parms[j].num_nodes_per_elem == 4)
        *side_set_node_list_len += 2;  /* 2 node side */
      else
        *side_set_node_list_len += 3;  /* 3 node side */
    }
    /* sides 3, 4, and 5 of 3d TRIs are special cases */
    else if (elem_blk_parms[j].elem_type_val == EX_EL_TRIANGLE &&
             ndim == 3 &&
             side_set_side_list[i] > 2 )
    {
      if (elem_blk_parms[j].num_nodes_per_elem == 3)  /* 3-node TRI */
        *side_set_node_list_len += 2;  /* 2 node side */
      else  /* 6-node TRI */
        *side_set_node_list_len += 3;  /* 3 node side */
    }
    else if (elem_blk_parms[j].elem_type_val == EX_EL_UNK)
    {
      exerrval = EX_BADPARAM;
      sprintf(errmsg,
             "Error: %s in elem block %d is an unsupported element type",
              elem_blk_parms[i].elem_type, elem_blk_parms[i].elem_blk_id);
      free(elem_blk_parms);
      free(elem_blk_ids);
      free(ss_elem_ndx);
      free(side_set_side_list);
      free(side_set_elem_list);
      ex_err("ex_get_side_set_node_list_len",errmsg,EX_MSG);
      return (EX_FATAL);
    }
    else /* all other element types */
      *side_set_node_list_len += elem_blk_parms[j].num_nodes_per_side[0];
  }

  /* All done: release element block ids array,
     element block parameters array, and side set element index array */
  free(elem_blk_ids);
  free(elem_blk_parms);
  free(ss_elem_ndx);
  free(side_set_side_list);
  free(side_set_elem_list);

  return(EX_NOERR);
}
