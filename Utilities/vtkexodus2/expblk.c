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
* expblk - ex_put_block: write edge, face, or element block parameters
*
* entry conditions - 
*   input parameters:
*       int     idexo                   exodus file id
*       int     blk_type                type of block (edge, face, or element)
*       int     blk_id                  block identifer
*       char*   entry_descrip           string describing shape of entries in the block
*       int     num_entries_this_blk    number of entries(records) in the block
*       int     num_nodes_per_entry     number of nodes per block entry
*       int     num_edges_per_entry     number of edges per block entry
*       int     num_faces_per_entry     number of faces per block entry
*       int     num_attr_per_entry      number of attributes per block entry
*
* exit conditions - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"
#include <string.h>

/*!
 * writes the parameters used to describe an element block
 * \param   exoid                   exodus file id
 * \param   blk_type                type of block (edge, face, or element)
 * \param   blk_id                  block identifer
 * \param   entry_descrip           string describing shape of entries in the block
 * \param   num_entries_this_blk    number of entries(records) in the block
 * \param   num_nodes_per_entry     number of nodes per block entry
 * \param   num_edges_per_entry     number of edges per block entry
 * \param   num_faces_per_entry     number of faces per block entry
 * \param   num_attr_per_entry      number of attributes per block entry
 */

int ex_put_block( int         exoid,
                  ex_entity_type blk_type,
                  int         blk_id,
                  const char* entry_descrip,
                  int         num_entries_this_blk,
                  int         num_nodes_per_entry,
                  int         num_edges_per_entry,
                  int         num_faces_per_entry,
                  int         num_attr_per_entry )
{
  int status;
  int varid, dimid, dims[2], blk_id_ndx, blk_stat, strdim;
  size_t start[2];
  int num_blk;
  size_t temp;
  int cur_num_blk, numblkdim, numattrdim;
  int nnodperentdim, nedgperentdim, nfacperentdim;
  int connid;
  char *cdum;
  char errmsg[MAX_ERR_LENGTH];
  const char* dnumblk;
  const char* vblkids;
  const char* vblksta;
  const char* vnodcon;
  const char* vedgcon;
  const char* vfaccon;
  const char* vattnam;
  const char* vblkatt;
  const char* dneblk;
  const char* dnape;
  const char* dnnpe;
  const char* dnepe;
  const char* dnfpe;

  exerrval  = 0; /* clear error code */

  cdum = 0;

  switch (blk_type) {
  case EX_EDGE_BLOCK:
    dnumblk = DIM_NUM_ED_BLK;
    vblkids = VAR_ID_ED_BLK;
    vblksta = VAR_STAT_ED_BLK;
    break;
  case EX_FACE_BLOCK:
    dnumblk = DIM_NUM_FA_BLK;
    vblkids = VAR_ID_FA_BLK;
    vblksta = VAR_STAT_FA_BLK;
    break;
  case EX_ELEM_BLOCK:
    dnumblk = DIM_NUM_EL_BLK;
    vblkids = VAR_ID_EL_BLK;
    vblksta = VAR_STAT_EL_BLK;
    break;
  default:
    exerrval = EX_BADPARAM;
    sprintf( errmsg, "Error: Bad block type (%d) specified for file id %d",
	     blk_type, exoid );
    ex_err( "ex_put_block", errmsg, exerrval );
    return (EX_FATAL);
  }

  /* first check if any element blocks are specified */

  if ((status = ex_get_dimension(exoid, dnumblk, ex_name_of_object(blk_type),
				 &temp, &dimid, "ex_put_block")) != NC_NOERR) {
    return EX_FATAL;
  }
  num_blk = temp;
  
  /* Next: Make sure that this is not a duplicate element block id by
     searching the vblkids array.
     WARNING: This must be done outside of define mode because id_lkup accesses
     the database to determine the position
  */

  if ((status = nc_inq_varid(exoid, vblkids, &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to locate %s ids in file id %d",
	    ex_name_of_object(blk_type), exoid);
    ex_err("ex_put_block",errmsg,exerrval);
  }

  blk_id_ndx = ex_id_lkup(exoid,blk_type,blk_id);
  if (exerrval != EX_LOOKUPFAIL) {   /* found the element block id */
    exerrval = EX_FATAL;
    sprintf(errmsg,
            "Error: %s id %d already exists in file id %d",
	    ex_name_of_object(blk_type), blk_id,exoid);
    ex_err("ex_put_block",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* Keep track of the total number of element blocks defined using a counter 
     stored in a linked list keyed by exoid.
     NOTE: ex_get_file_item  is a function that finds the number of element 
     blocks for a specific file and returns that value incremented.
  */
  cur_num_blk=ex_get_file_item(exoid, ex_get_counter_list(blk_type));
  if (cur_num_blk >= num_blk) {
    exerrval = EX_FATAL;
    sprintf(errmsg,
	    "Error: exceeded number of %ss (%d) defined in file id %d",
	    ex_name_of_object(blk_type), num_blk,exoid);
    ex_err("ex_put_block",errmsg,exerrval);
    return (EX_FATAL);
  }


  /*   NOTE: ex_get_file_item  is a function that finds the number of element
       blocks for a specific file and returns that value incremented. */
  cur_num_blk=ex_inc_file_item(exoid, ex_get_counter_list(blk_type));
  start[0] = cur_num_blk;

  /* write out element block id to previously defined id array variable*/
  if ((status = nc_put_var1_int(exoid, varid, start, &blk_id)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to store %s id to file id %d",
	    ex_name_of_object(blk_type), exoid);
    ex_err("ex_put_block",errmsg,exerrval);
    return (EX_FATAL);
  }

  blk_id_ndx = start[0]+1; /* element id index into vblkids array*/

  if (num_entries_this_blk == 0) /* Is this a NULL element block? */
    blk_stat = 0; /* change element block status to NULL */
  else
    blk_stat = 1; /* change element block status to TRUE */

  if ((status = nc_inq_varid (exoid, vblksta, &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to locate %s status in file id %d",
	    ex_name_of_object(blk_type), exoid);
    ex_err("ex_put_block",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((status = nc_put_var1_int(exoid, varid, start, &blk_stat)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to store %s id %d status to file id %d",
	    ex_name_of_object(blk_type), blk_id, exoid);
    ex_err("ex_put_block",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (num_entries_this_blk == 0) {/* Is this a NULL element block? */
    return(EX_NOERR);
  }


  /* put netcdf file into define mode  */
  if ((status=nc_redef (exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,"Error: failed to place file id %d into define mode",exoid);
    ex_err("ex_put_block",errmsg,exerrval);
    return (EX_FATAL);
  }

  switch (blk_type) {
  case EX_EDGE_BLOCK:
    dneblk = DIM_NUM_ED_IN_EBLK(blk_id_ndx);
    dnnpe = DIM_NUM_NOD_PER_ED(blk_id_ndx);
    dnepe = 0;
    dnfpe = 0;
    dnape = DIM_NUM_ATT_IN_EBLK(blk_id_ndx);
    vblkatt = VAR_EATTRIB(blk_id_ndx);
    vattnam = VAR_NAME_EATTRIB(blk_id_ndx);
    vnodcon = VAR_EBCONN(blk_id_ndx);
    vedgcon = 0;
    vfaccon = 0;
    break;
  case EX_FACE_BLOCK:
    dneblk = DIM_NUM_FA_IN_FBLK(blk_id_ndx);
    dnnpe = DIM_NUM_NOD_PER_FA(blk_id_ndx);
    dnepe = 0;
    dnfpe = 0;
    dnape = DIM_NUM_ATT_IN_FBLK(blk_id_ndx);
    vblkatt = VAR_FATTRIB(blk_id_ndx);
    vattnam = VAR_NAME_FATTRIB(blk_id_ndx);
    vnodcon = VAR_FBCONN(blk_id_ndx);
    vedgcon = 0;
    vfaccon = 0;
    break;
  case EX_ELEM_BLOCK:
    dneblk = DIM_NUM_EL_IN_BLK(blk_id_ndx);
    dnnpe = DIM_NUM_NOD_PER_EL(blk_id_ndx);
    dnepe = DIM_NUM_EDG_PER_EL(blk_id_ndx);
    dnfpe = DIM_NUM_FAC_PER_EL(blk_id_ndx);
    dnape = DIM_NUM_ATT_IN_BLK(blk_id_ndx);
    vblkatt = VAR_ATTRIB(blk_id_ndx);
    vattnam = VAR_NAME_ATTRIB(blk_id_ndx);
    vnodcon = VAR_CONN(blk_id_ndx);
    vedgcon = VAR_ECONN(blk_id_ndx);
    vfaccon = VAR_FCONN(blk_id_ndx);
    break;
  default:
    sprintf(errmsg,
      "Error: Called with invalid blk_type %d", blk_type );
    ex_err("ex_put_block",errmsg,exerrval);
    return (EX_FATAL);
  }
  /* define some dimensions and variables*/

  if ((status = nc_def_dim(exoid,dneblk,num_entries_this_blk, &numblkdim )) != NC_NOERR) {
    if (status == NC_ENAMEINUSE) {        /* duplicate entry */
      exerrval = status;
      sprintf(errmsg,
	      "Error: %s %d already defined in file id %d",
	      ex_name_of_object(blk_type), blk_id,exoid);
      ex_err("ex_put_block",errmsg,exerrval);
    } else {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to define number of entities/block for %s %d file id %d",
	      ex_name_of_object(blk_type), blk_id,exoid);
      ex_err("ex_put_block",errmsg,exerrval);
    }
    goto error_ret;         /* exit define mode and return */
  }

  if ((status = nc_def_dim(exoid,dnnpe,num_nodes_per_entry, &nnodperentdim)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to define number of nodes/entity for %s %d in file id %d",
	    ex_name_of_object(blk_type), blk_id,exoid);
    ex_err("ex_put_block",errmsg,exerrval);
    goto error_ret;         /* exit define mode and return */
  }

  if (dnepe && num_edges_per_entry > 0 ) {
    if ((status = nc_def_dim (exoid,dnepe,num_edges_per_entry, &nedgperentdim)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to define number of edges/entity for %s %d in file id %d",
	      ex_name_of_object(blk_type), blk_id,exoid);
      ex_err("ex_put_block",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
  }

  if ( dnfpe && num_faces_per_entry > 0 ) {
    if ((status = nc_def_dim(exoid,dnfpe,num_faces_per_entry, &nfacperentdim)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to define number of faces/entity for %s %d in file id %d",
	      ex_name_of_object(blk_type), blk_id,exoid);
      ex_err("ex_put_block",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
  }

  /* element attribute array */
  if (num_attr_per_entry > 0) {

    if ((status = nc_def_dim(exoid, dnape, num_attr_per_entry, &numattrdim)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to define number of attributes in %s %d in file id %d",
	      ex_name_of_object(blk_type), blk_id,exoid);
      ex_err("ex_put_block",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

    dims[0] = numblkdim;
    dims[1] = numattrdim;

    if ((status = nc_def_var(exoid, vblkatt, nc_flt_code(exoid), 2, dims, &varid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error:  failed to define attributes for %s %d in file id %d",
	      ex_name_of_object(blk_type), blk_id,exoid);
      ex_err("ex_put_block",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

    /* inquire previously defined dimensions  */
    if ((status = nc_inq_dimid(exoid, DIM_STR, &strdim)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to get string length in file id %d",exoid);
      ex_err("ex_put_block",errmsg,exerrval);
      return (EX_FATAL);
    }
     
    /* Attribute names... */
    dims[0] = numattrdim;
    dims[1] = strdim;
	    
    if ((status = nc_def_var(exoid, vattnam, NC_CHAR, 2, dims, &varid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to define %s attribute name array in file id %d",
	      ex_name_of_object(blk_type), exoid);
      ex_err("ex_put_block",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
  }

  /* element connectivity array */
  dims[0] = numblkdim;
  dims[1] = nnodperentdim;

  if ((status = nc_def_var(exoid, vnodcon, NC_INT, 2, dims, &connid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to create connectivity array for %s %d in file id %d",
	    ex_name_of_object(blk_type), blk_id,exoid);
    ex_err("ex_put_block",errmsg,exerrval);
    goto error_ret;         /* exit define mode and return */
  }

  /* store element type as attribute of connectivity variable */
  if ((status = nc_put_att_text(exoid, connid, ATT_NAME_ELB, strlen(entry_descrip)+1, 
				entry_descrip)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to store %s type name %s in file id %d",
	    ex_name_of_object(blk_type), entry_descrip,exoid);
    ex_err("ex_put_block",errmsg,exerrval);
    goto error_ret;         /* exit define mode and return */
  }

  if (vedgcon && num_edges_per_entry ) {
    dims[0] = numblkdim;
    dims[1] = nedgperentdim;

    if ((status = nc_def_var(exoid, vedgcon, NC_INT, 2, dims, &varid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to create edge connectivity array for %s %d in file id %d",
	      ex_name_of_object(blk_type), blk_id,exoid);
      ex_err("ex_put_block",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
  }

  if ( vfaccon && num_faces_per_entry ) {
    dims[0] = numblkdim;
    dims[1] = nfacperentdim;

    if ((status = nc_def_var(exoid, vfaccon, NC_INT, 2, dims, &varid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to create face connectivity array for %s %d in file id %d",
	      ex_name_of_object(blk_type), blk_id,exoid);
      ex_err("ex_put_block",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
  }

  /* leave define mode  */

  if ((exerrval=nc_enddef (exoid)) != NC_NOERR) {
    sprintf(errmsg,
	    "Error: failed to complete %s definition in file id %d", 
	    ex_name_of_object(blk_type), exoid);
    ex_err("ex_put_block",errmsg,exerrval);
    return (EX_FATAL);
  }

  return (EX_NOERR);

  /* Fatal error: exit definition mode and return */
 error_ret:
  if (nc_enddef (exoid) != NC_NOERR) {    /* exit define mode */
    sprintf(errmsg,
	    "Error: failed to complete definition for file id %d",
	    exoid);
    ex_err("ex_put_block",errmsg,exerrval);
  }
  return (EX_FATAL);
}

