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
  int arbitrary_polyhedra = 0; /* 1 if block is arbitrary 2d polyhedra type; 2 if 3d polyhedra */
  int att_name_varid, varid, dimid, dims[2], blk_id_ndx, blk_stat, strdim;
  size_t start[2];
  int num_blk;
  size_t temp;
  int cur_num_blk, numblkdim, numattrdim;
  int nnodperentdim, nedgperentdim, nfacperentdim;
  int connid;
  int npeid;
  char errmsg[MAX_ERR_LENGTH];
  char entity_type1[5];
  char entity_type2[5];
  const char* dnumblk = NULL;
  const char* vblkids = NULL;
  const char* vblksta = NULL;
  const char* vnodcon = NULL;
  const char* vnpecnt = NULL;
  const char* vedgcon = NULL;
  const char* vfaccon = NULL;
  const char* vconn   = NULL;
  const char* vattnam = NULL;
  const char* vblkatt = NULL;
  const char* dneblk  = NULL;
  const char* dnape   = NULL;
  const char* dnnpe   = NULL;
  const char* dnepe   = NULL;
  const char* dnfpe   = NULL;

  exerrval  = 0; /* clear error code */

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
    vnpecnt = VAR_FBEPEC(blk_id_ndx);
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
    vnpecnt = VAR_EBEPEC(blk_id_ndx);
    vedgcon = VAR_ECONN(blk_id_ndx);
    vfaccon = VAR_FCONN(blk_id_ndx);
    break;
  default:
    exerrval = 1005;
    sprintf(errmsg,
      "Internal Error: unrecognized block type in switch: %d in file id %d",
      blk_type,exoid);
    ex_err("ex_put_block",errmsg,EX_MSG);
    return (EX_FATAL);              /* number of attributes not defined */
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

  if ( dnnpe && num_nodes_per_entry > 0) {
    /* A nfaced block would not have any nodes defined... */
    if ((status = nc_def_dim(exoid,dnnpe,num_nodes_per_entry, &nnodperentdim)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to define number of nodes/entity for %s %d in file id %d",
        ex_name_of_object(blk_type), blk_id,exoid);
      ex_err("ex_put_block",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
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
    if ((status = nc_inq_dimid(exoid, DIM_STR_NAME, &strdim)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to get string length in file id %d",exoid);
      ex_err("ex_put_block",errmsg,exerrval);
      return (EX_FATAL);
    }
     
    /* Attribute names... */
    dims[0] = numattrdim;
    dims[1] = strdim;
      
    if ((status = nc_def_var(exoid, vattnam, NC_CHAR, 2, dims, &att_name_varid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to define %s attribute name array in file id %d",
        ex_name_of_object(blk_type), exoid);
      ex_err("ex_put_block",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
  }

  /* See if storing an 'nsided' element block (arbitrary 2d polyhedra or super element) */
  if (strlen(entry_descrip) >= 3) {
    if ((entry_descrip[0] == 'n' || entry_descrip[0] == 'N') &&
  (entry_descrip[1] == 's' || entry_descrip[1] == 'S') &&
  (entry_descrip[2] == 'i' || entry_descrip[2] == 'I'))
      arbitrary_polyhedra = 1;
    else if ((entry_descrip[0] == 'n' || entry_descrip[0] == 'N') &&
       (entry_descrip[1] == 'f' || entry_descrip[1] == 'F') &&
       (entry_descrip[2] == 'a' || entry_descrip[2] == 'A'))
      /* If a FACE_BLOCK, then we are dealing with the faces of the nfaced block. */
      arbitrary_polyhedra = blk_type == EX_FACE_BLOCK ? 1 : 2;
  }

  /* element connectivity array */
  if (arbitrary_polyhedra > 0) {
    if (blk_type != EX_FACE_BLOCK && blk_type != EX_ELEM_BLOCK) {
      exerrval = EX_BADPARAM;
      sprintf( errmsg, "Error: Bad block type (%d) for nsided/nfaced block in file id %d",
         blk_type, exoid );
      ex_err( "ex_put_block", errmsg, exerrval );
      return (EX_FATAL);
    }

    if (arbitrary_polyhedra == 1) {
      dims[0] = nnodperentdim;
      vconn = vnodcon;

      /* store entity types as attribute of npeid variable -- node/elem, node/face, face/elem*/
      strcpy(entity_type1, "NODE");
      if (blk_type == EX_ELEM_BLOCK)
  strcpy(entity_type2, "ELEM");
      else
  strcpy(entity_type2, "FACE");
    } else if (arbitrary_polyhedra == 2) {
      dims[0] = nfacperentdim;
      vconn = vfaccon;

      /* store entity types as attribute of npeid variable -- node/elem, node/face, face/elem*/
      strcpy(entity_type1, "FACE");
      strcpy(entity_type2, "ELEM");
    }

    if ((status = nc_def_var(exoid, vconn, NC_INT, 1, dims, &connid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to create connectivity array for %s %d in file id %d",
        ex_name_of_object(blk_type), blk_id,exoid);
      ex_err("ex_put_block",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
    
    /* element face-per-element or node-per-element count array */
    dims[0] = numblkdim;
    
    if ((status = nc_def_var(exoid, vnpecnt, NC_INT, 1, dims, &npeid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to create face- or node- per-entity count array for %s %d in file id %d",
        ex_name_of_object(blk_type), blk_id, exoid);
      ex_err("ex_put_block",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

    if ((status = nc_put_att_text(exoid, npeid, "entity_type1", strlen(entity_type1)+1,
          entity_type1)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to store entity type attribute text for %s %d in file id %d",
        ex_name_of_object(blk_type), blk_id, exoid);
      ex_err("ex_put_block",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
    if ((status = nc_put_att_text(exoid, npeid, "entity_type2", strlen(entity_type2)+1,
          entity_type2)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to store entity type attribute text for %s %d in file id %d",
        ex_name_of_object(blk_type), blk_id, exoid);
      ex_err("ex_put_block",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }
  } else {
    /* "Normal" (non-polyhedra) element block type */
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

  if (arbitrary_polyhedra == 0) {
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
  }
  /* leave define mode  */

  if ((exerrval=nc_enddef (exoid)) != NC_NOERR) {
    sprintf(errmsg,
      "Error: failed to complete %s definition in file id %d", 
      ex_name_of_object(blk_type), exoid);
    ex_err("ex_put_block",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* Output a dummy empty attribute name in case client code doesn't
     write anything; avoids corruption in some cases.
  */
  if (num_attr_per_entry > 0) {
    size_t  count[2];
    char *text = "";
    size_t i;

    count[0] = 1;
    start[1] = 0;
    count[1] = strlen(text)+1;
  
    for (i = 0; i < num_attr_per_entry; i++) {
      start[0] = i;
      nc_put_vara_text(exoid, att_name_varid, start, count, text);
    }
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

