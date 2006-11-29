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
* excopy - ex_copy
*
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     in_exoid                input exodus file id
*
* exit conditions - 
*       int     out_exoid               output exodus file id
*
* revision history - 
*
*
*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "exodusII.h"
#include "exodusII_int.h"

struct ncdim {                  /* dimension */
    char name[MAX_STR_LENGTH];
    long size;
};

struct ncvar {                  /* variable */
    char name[MAX_STR_LENGTH];
    nc_type type;
    int ndims;
    int dims[MAX_VAR_DIMS];
    int natts;
};

struct ncatt {                  /* attribute */
    int var;
    char name[MAX_STR_LENGTH];
    nc_type type;
    int len;
    void *val;
};

void update_internal_structs( int, int, struct list_item** );

/*
 * copies all information (attributes, dimensions, and variables from
 * an opened EXODUS file to another opened EXODUS file
 */

int ex_copy (int in_exoid, int out_exoid)
{
   int ndims;                   /* number of dimensions */
   int nvars;                   /* number of variables */
   int ngatts;                  /* number of global attributes */
   int recdimid;                /* id of unlimited dimension */
   int dimid;                   /* dimension id */
   int dim_out_id;              /* dimension id */
   int varid;                   /* variable id */
   int var_out_id;              /* variable id */
   struct ncvar var;            /* variable */
   struct ncatt att;            /* attribute */
   int i, temp;
   long numrec;
   long dim_sz;
   char dim_nm[MAX_NC_NAME];
   int in_large, out_large;
   
   extern int ncopts;

   exerrval = 0; /* clear error code */

   /*
    * Get exodus_large_model setting on both input and output
    * databases so know how to handle coordinates.
    */
   in_large  = ex_large_model(in_exoid);
   out_large = ex_large_model(out_exoid);
   
   /*
    * get number of dimensions, number of variables, number of global
    * atts, and dimension id of unlimited dimension, if any
    */

   ncinquire(in_exoid, &ndims, &nvars, &ngatts, &recdimid);
   ncdiminq (in_exoid, recdimid, (char *) 0, &numrec);

   /* put output file into define mode */

   ncredef(out_exoid);

   /* copy global attributes */

   for (i = 0; i < ngatts; i++) {

      ncattname(in_exoid, NC_GLOBAL, i, att.name);
        
      ncattinq(in_exoid, NC_GLOBAL, att.name, &att.type, &att.len);

      /* if attribute exists in output file, don't overwrite it; compute 
       * word size, I/O word size etc. are global attributes stored when
       * file is created with ex_create;  we don't want to overwrite those
       */

      if (ncattinq (out_exoid, NC_GLOBAL, att.name, &att.type, &att.len) == -1){

        /* The "last_written_time" attribute is a special attribute
           used by the Sierra IO system to determine whether a
           timestep has been fully written to the database in order to
           try to detect a database crash that happens in the middle
           of a database output step. Don't want to copy that attribute.
        */
        if (strcmp(att.name,"last_written_time") != 0) {
          /* attribute doesn't exist in new file so OK to create it */
          ncattcopy (in_exoid,NC_GLOBAL,att.name,out_exoid,NC_GLOBAL);
        }
      }
   }

   /* copy dimensions */

   /* Get the dimension sizes and names */

   for(dimid = 0; dimid < ndims; dimid++){

      ncdiminq(in_exoid,dimid,dim_nm,&dim_sz);

      /* See if the dimension has already been defined */

      temp = ncopts;
      ncopts = 0;
      dim_out_id = ncdimid(out_exoid,dim_nm);
      ncopts = temp;

      /* If the dimension isn't one we specifically don't want 
       * to copy (ie, number of QA or INFO records) and it 
       * hasn't been defined, copy it */

      if ( ( strcmp(dim_nm,DIM_NUM_QA)        != 0) &&
           ( strcmp(dim_nm,DIM_NUM_INFO)      != 0) &&
           ( strcmp(dim_nm,DIM_NUM_NOD_VAR)   != 0) &&
           ( strcmp(dim_nm,DIM_NUM_EDG_VAR)   != 0) &&
           ( strcmp(dim_nm,DIM_NUM_FAC_VAR)   != 0) &&
           ( strcmp(dim_nm,DIM_NUM_ELE_VAR)   != 0) &&
           ( strcmp(dim_nm,DIM_NUM_NSET_VAR)  != 0) &&
           ( strcmp(dim_nm,DIM_NUM_ESET_VAR)  != 0) &&
           ( strcmp(dim_nm,DIM_NUM_FSET_VAR)  != 0) &&
           ( strcmp(dim_nm,DIM_NUM_SSET_VAR)  != 0) &&
           ( strcmp(dim_nm,DIM_NUM_ELSET_VAR) != 0) &&
           ( strcmp(dim_nm,DIM_NUM_GLO_VAR)   != 0) ) {

         if(dim_out_id == -1){
            if(dimid != recdimid){
               dim_out_id=ncdimdef(out_exoid,dim_nm,dim_sz);
            }else{
               dim_out_id=ncdimdef(out_exoid,dim_nm,NC_UNLIMITED);
            } /* end else */
         } /* end if */
      } /* end if */
   } /* end loop over dim */

   /* copy variable definitions and variable attributes */

   for (varid = 0; varid < nvars; varid++) {

      ncvarinq(in_exoid, varid, var.name, &var.type, &var.ndims, 
                      var.dims, &var.natts);

      /* we don't want to copy some variables because there is not a
       * simple way to add to them;
       * QA records, info records and all results variables (nodal
       * element, and global results) are examples
       */

      if ( ( strcmp(var.name,VAR_QA_TITLE) != 0)     &&
           ( strcmp(var.name,VAR_INFO) != 0)         &&
           ( strcmp(var.name,VAR_EBLK_TAB) != 0)     &&
           ( strcmp(var.name,VAR_FBLK_TAB) != 0)     &&
           ( strcmp(var.name,VAR_ELEM_TAB) != 0)     &&
           ( strcmp(var.name,VAR_ELSET_TAB) != 0)     &&
           ( strcmp(var.name,VAR_SSET_TAB) != 0)     &&
           ( strcmp(var.name,VAR_FSET_TAB) != 0)     &&
           ( strcmp(var.name,VAR_ESET_TAB) != 0)     &&
           ( strcmp(var.name,VAR_NSET_TAB) != 0)     &&
           ( strcmp(var.name,VAR_NAME_GLO_VAR) != 0) &&
           ( strcmp(var.name,VAR_GLO_VAR) != 0)      &&
           ( strcmp(var.name,VAR_NAME_NOD_VAR) != 0) &&
           ( strcmp(var.name,VAR_NOD_VAR) != 0)      &&
           ( strcmp(var.name,VAR_NAME_EDG_VAR) != 0) &&
           ( strcmp(var.name,VAR_NAME_FAC_VAR) != 0) &&
           ( strcmp(var.name,VAR_NAME_ELE_VAR) != 0) &&
           ( strcmp(var.name,VAR_NAME_NSET_VAR) != 0)   &&
           ( strcmp(var.name,VAR_NAME_ESET_VAR) != 0)   &&
           ( strcmp(var.name,VAR_NAME_FSET_VAR) != 0)   &&
           ( strcmp(var.name,VAR_NAME_SSET_VAR) != 0)   &&
           ( strcmp(var.name,VAR_NAME_ELSET_VAR) != 0)  &&
           ( strncmp(var.name,"vals_elset_var", 14) != 0) &&
           ( strncmp(var.name,"vals_sset_var",  13) != 0) &&
           ( strncmp(var.name,"vals_fset_var",  13) != 0) &&
           ( strncmp(var.name,"vals_eset_var",  13) != 0) &&
           ( strncmp(var.name,"vals_nset_var",  13) != 0) &&
           ( strncmp(var.name,"vals_nod_var",   12) != 0) &&
           ( strncmp(var.name,"vals_edge_var",  13) != 0) &&
           ( strncmp(var.name,"vals_face_var",  13) != 0) &&
           ( strncmp(var.name,"vals_elem_var",  13) != 0) ) {

        if (strncmp(var.name,VAR_COORD,5) == 0) {
          var_out_id = cpy_coord_def (in_exoid, out_exoid, recdimid, var.name,
                                      in_large, out_large);
        } else {
          var_out_id = cpy_var_def (in_exoid, out_exoid, recdimid, var.name);
        }

         /* copy the variable's attributes */
         (void) cpy_att (in_exoid, out_exoid, varid, var_out_id);

      }
   }

   /* take the output file out of define mode */
   ncendef (out_exoid);

   /* output variable data */

   for (varid = 0; varid < nvars; varid++) {
      ncvarinq(in_exoid, varid, var.name, &var.type, &var.ndims,
                    var.dims, &var.natts);

      /* we don't want to copy some variable values;
       * QA records and info records shouldn't be copied because there
       * isn't an easy way to add to them;
       * the time value array ("time_whole") and any results variables
       * (nodal, elemental, or global) shouldn't be copied 
       */

      if ( ( strcmp(var.name,VAR_QA_TITLE) != 0)        &&
           ( strcmp(var.name,VAR_INFO) != 0)            &&
           ( strcmp(var.name,VAR_EBLK_TAB) != 0)        &&
           ( strcmp(var.name,VAR_FBLK_TAB) != 0)        &&
           ( strcmp(var.name,VAR_ELEM_TAB) != 0)        &&
           ( strcmp(var.name,VAR_ELSET_TAB) != 0)       &&
           ( strcmp(var.name,VAR_SSET_TAB) != 0)        &&
           ( strcmp(var.name,VAR_FSET_TAB) != 0)        &&
           ( strcmp(var.name,VAR_ESET_TAB) != 0)        &&
           ( strcmp(var.name,VAR_NSET_TAB) != 0)        &&
           ( strcmp(var.name,VAR_NAME_GLO_VAR) != 0)    &&
           ( strcmp(var.name,VAR_GLO_VAR) != 0)         &&
           ( strcmp(var.name,VAR_NAME_NOD_VAR) != 0)    &&
           ( strcmp(var.name,VAR_NOD_VAR) != 0)         &&
           ( strcmp(var.name,VAR_NAME_EDG_VAR) != 0)    &&
           ( strcmp(var.name,VAR_NAME_FAC_VAR) != 0)    &&
           ( strcmp(var.name,VAR_NAME_ELE_VAR) != 0)    &&
           ( strcmp(var.name,VAR_NAME_NSET_VAR) != 0)   &&
           ( strcmp(var.name,VAR_NAME_ESET_VAR) != 0)   &&
           ( strcmp(var.name,VAR_NAME_FSET_VAR) != 0)   &&
           ( strcmp(var.name,VAR_NAME_SSET_VAR) != 0)   &&
           ( strcmp(var.name,VAR_NAME_ELSET_VAR) != 0)  &&
           ( strncmp(var.name,"vals_elset_var", 14) != 0)&&
           ( strncmp(var.name,"vals_sset_var", 13) != 0)&&
           ( strncmp(var.name,"vals_fset_var", 13) != 0)&&
           ( strncmp(var.name,"vals_eset_var", 13) != 0)&&
           ( strncmp(var.name,"vals_nset_var", 13) != 0)&&
           ( strncmp(var.name,"vals_nod_var", 12) != 0) &&
           ( strncmp(var.name,"vals_edge_var",13) != 0) &&
           ( strncmp(var.name,"vals_face_var",13) != 0) &&
           ( strncmp(var.name,"vals_elem_var",13) != 0) &&
           ( strcmp(var.name,VAR_WHOLE_TIME) != 0) ) {

        if (strncmp(var.name,VAR_COORD,5) == 0) {
          (void) cpy_coord_val (in_exoid, out_exoid, var.name,
                                in_large, out_large);
        } else {
          (void) cpy_var_val (in_exoid, out_exoid, var.name);
        }

      }
   }

   /* ensure internal data structures are updated */

   /* if number of blocks > 0 */
   update_internal_structs( out_exoid, EX_INQ_EDGE_BLK, &ed_ctr_list );
   update_internal_structs( out_exoid, EX_INQ_FACE_BLK, &fa_ctr_list );
   update_internal_structs( out_exoid, EX_INQ_ELEM_BLK, &eb_ctr_list );

   /* if number of sets > 0 */
   update_internal_structs( out_exoid, EX_INQ_NODE_SETS, &ns_ctr_list );
   update_internal_structs( out_exoid, EX_INQ_EDGE_SETS, &es_ctr_list );
   update_internal_structs( out_exoid, EX_INQ_FACE_SETS, &fs_ctr_list );
   update_internal_structs( out_exoid, EX_INQ_SIDE_SETS, &ss_ctr_list );
   update_internal_structs( out_exoid, EX_INQ_ELEM_SETS, &els_ctr_list );

   /* if number of maps > 0 */
   update_internal_structs( out_exoid, EX_INQ_NODE_MAP, &nm_ctr_list );
   update_internal_structs( out_exoid, EX_INQ_EDGE_MAP, &edm_ctr_list );
   update_internal_structs( out_exoid, EX_INQ_FACE_MAP, &fam_ctr_list );
   update_internal_structs( out_exoid, EX_INQ_ELEM_MAP, &em_ctr_list );

   return(EX_NOERR);
}

int
cpy_att(int in_id,int out_id,int var_in_id,int var_out_id)
/*
   int in_id: input netCDF input-file ID
   int out_id: input netCDF output-file ID
   int var_in_id: input netCDF input-variable ID
   int var_out_id: input netCDF output-variable ID
 */
{
  /* Routine to copy all the attributes from the input netCDF
     file to the output netCDF file. If var_in_id == NC_GLOBAL,
     then the global attributes are copied. Otherwise the variable's
     attributes are copied. */

  int idx;
  int nbr_att;

  if(var_in_id == NC_GLOBAL){
    ncinquire(in_id,(int *)NULL,(int *)NULL,&nbr_att,(int *)NULL);

  }else{
    ncvarinq(in_id,var_in_id,(char *)NULL,(nc_type *)NULL,
                   (int *)NULL,(int*)NULL,&nbr_att);
  } /* end else */

  /* Get the attributes names, types, lengths, and values */
  for(idx=0;idx<nbr_att;idx++){
    char att_nm[MAX_STR_LENGTH];

    ncattname(in_id,var_in_id,idx,att_nm);

    ncattcopy(in_id,var_in_id,att_nm,out_id,var_out_id);
  } /* end loop over attributes */

  return(EX_NOERR);

} /* end cpy_att() */

int
cpy_coord_def(int in_id,int out_id,int rec_dim_id,char *var_nm,
              int in_large, int out_large)
/*
   int in_id: input netCDF input-file ID
   int out_id: input netCDF output-file ID
   int rec_dim_id: input input-file record dimension ID
   char *var_nm: input variable name
   int in_large: large file setting for input file
   int out_large: large file setting for output file
   int cpy_var_def(): output output-file variable ID
 */
{
  const char *routine = NULL;
  long spatial_dim;
  int nbr_dim;
  int temp;

  int dim_out_id[2];
  int var_out_id = -1;
  
  /* Handle easiest situation first: in_large matches out_large */
  if (in_large == out_large) {
    return cpy_var_def(in_id, out_id, rec_dim_id, var_nm);
  }

  /* At this point, know that in_large != out_large, so some change to
     the coord variable definition is needed. Also will need the
     spatial dimension, so get that now.*/
  ex_get_dimension(in_id, DIM_NUM_DIM, "dimension", &spatial_dim, routine);
  
  if (in_large == 0 && out_large == 1) {
    /* output file will have coordx, coordy, coordz (if 3d).  See if
       they are already defined in output file. Assume either all or
       none are defined. */
    temp = ncopts;
    ncopts=0;

    {
      int var_out_idx = ncvarid(out_id, VAR_COORD_X);
      int var_out_idy = ncvarid(out_id, VAR_COORD_Y);
      int var_out_idz = ncvarid(out_id, VAR_COORD_Z);
      ncopts = temp;
      if (var_out_idx != -1 && var_out_idy != -1 &&
          (spatial_dim == 2 || var_out_idz != -1)) {
        return var_out_idx;
      }
    }

    /* Get dimid of the num_nodes dimension in output file... */
    dim_out_id[0]=ncdimid(out_id,DIM_NUM_NODES);

    /* Define the variables in the output file */
    
    /* Define according to the EXODUS file's IO_word_size */
    nbr_dim = 1;
    var_out_id=ncvardef(out_id,VAR_COORD_X,nc_flt_code(out_id),
                        nbr_dim, dim_out_id);
    var_out_id=ncvardef(out_id,VAR_COORD_Y,nc_flt_code(out_id),
                        nbr_dim, dim_out_id);
    if (spatial_dim == 3)
      var_out_id=ncvardef(out_id,VAR_COORD_Z,nc_flt_code(out_id),
                          nbr_dim, dim_out_id);
    
  }

  if (in_large == 1 && out_large == 0) {
    /* input file has coordx, coordy, coordz (if 3d); output will only
       have "coord".  See if is already defined in output file. */
    temp = ncopts;
    ncopts=0;
    var_out_id = ncvarid(out_id, VAR_COORD);
    ncopts = temp;
    
    if (var_out_id != -1)
      return var_out_id;

    /* Get dimid of the spatial dimension and num_nodes dimensions in output file... */
    dim_out_id[0]=ncdimid(out_id,DIM_NUM_DIM);
    dim_out_id[1]=ncdimid(out_id,DIM_NUM_NODES);

    /* Define the variable in the output file */
    
    /* Define according to the EXODUS file's IO_word_size */
    nbr_dim = 2;
    var_out_id=ncvardef(out_id,VAR_COORD,nc_flt_code(out_id),
                        nbr_dim, dim_out_id);
  }
  return var_out_id;
}

int
cpy_var_def(int in_id,int out_id,int rec_dim_id,char *var_nm)
/*
   int in_id: input netCDF input-file ID
   int out_id: input netCDF output-file ID
   int rec_dim_id: input input-file record dimension ID
   char *var_nm: input variable name
   int cpy_var_def(): output output-file variable ID
 */
{
  /* Routine to copy the variable metadata from an input netCDF file
   * to an output netCDF file. 
   */

  int *dim_in_id;
  int *dim_out_id;
  int idx;
  int nbr_dim;
  int var_in_id;
  int var_out_id;
  int temp;

  extern int ncopts;

  nc_type var_type;

  /* See if the requested variable is already in the output file. */
  temp = ncopts;
  ncopts=0;
  var_out_id=ncvarid(out_id,var_nm);
  ncopts = temp;
  if(var_out_id != -1) return var_out_id;

  /* See if the requested variable is in the input file. */
  var_in_id=ncvarid(in_id,var_nm);

  /* Get the type of the variable and the number of dimensions. */
  ncvarinq(in_id,var_in_id,(char *)NULL,&var_type,&nbr_dim,
        (int *)NULL,(int *)NULL);

  /* Recall:
     1. The dimensions must be defined before the variable.
     2. The variable must be defined before the attributes. */

  /* Allocate space to hold the dimension IDs */
  dim_in_id=malloc(nbr_dim*sizeof(int)); 
  dim_out_id=malloc(nbr_dim*sizeof(int));

  /* Get the dimension IDs */
  ncvarinq(in_id,var_in_id,(char *)NULL,(nc_type *)NULL,
                (int *)NULL,dim_in_id,(int *)NULL);

  /* Get the dimension sizes and names */
  for(idx=0;idx<nbr_dim;idx++){
    char dim_nm[MAX_NC_NAME];
    long dim_sz;

    ncdiminq(in_id,dim_in_id[idx],dim_nm,&dim_sz);

    /* See if the dimension has already been defined */
    temp = ncopts;
    ncopts = 0;
    dim_out_id[idx]=ncdimid(out_id,dim_nm);
    ncopts = temp;

    /* If the dimension hasn't been defined, copy it */
    if(dim_out_id[idx] == -1){
      if(dim_in_id[idx] != rec_dim_id){
        dim_out_id[idx]=ncdimdef(out_id,dim_nm,dim_sz);
      }else{
        dim_out_id[idx]=ncdimdef(out_id,dim_nm,NC_UNLIMITED);
      } /* end else */
    } /* end if */
  } /* end loop over dim */

  /* Define the variable in the output file */

  /* If variable is float or double, define it according to the EXODUS
     file's IO_word_size */

  if ((var_type == NC_FLOAT) || (var_type == NC_DOUBLE)) {
     var_out_id=ncvardef(out_id,var_nm,nc_flt_code(out_id),
                             nbr_dim,dim_out_id);
  } else {
     var_out_id=ncvardef(out_id,var_nm,var_type,nbr_dim,dim_out_id);
  }

  /* Free the space holding the dimension IDs */
  (void)free(dim_in_id);
  (void)free(dim_out_id);

  return var_out_id;
} /* end cpy_var_def() */

int
cpy_var_val(int in_id,int out_id,char *var_nm)
/*
   int in_id: input netCDF input-file ID
   int out_id: input netCDF output-file ID
   char *var_nm: input variable name
 */
{
  /* Routine to copy the variable data from an input netCDF file
   * to an output netCDF file. 
   */

  int *dim_id;
  int idx;
  int nbr_dim;
  int var_in_id;
  int var_out_id;
  long *dim_cnt;
  long *dim_sz;
  long *dim_srt;
  long var_sz=1L;

  nc_type var_type_in, var_type_out;

  void *void_ptr;

  /* Get the var_id for the requested variable from both files. */
  var_in_id=ncvarid(in_id,var_nm);

  var_out_id=ncvarid(out_id,var_nm);
 
  /* Get the number of dimensions for the variable. */

  ncvarinq(out_id,var_out_id,(char *)NULL,&var_type_out,&nbr_dim,
                (int *)NULL,(int *)NULL);

  ncvarinq(in_id,var_in_id,(char *)NULL,&var_type_in,&nbr_dim,
                (int *)NULL,(int *)NULL);
 
  /* Allocate space to hold the dimension IDs */
  dim_cnt = malloc(nbr_dim*sizeof(long));

  dim_id=malloc(nbr_dim*sizeof(int));

  dim_sz=malloc(nbr_dim*sizeof(long));

  dim_srt=malloc(nbr_dim*sizeof(long));
 
  /* Get the dimension IDs from the input file */
  ncvarinq(in_id,var_in_id,(char *)NULL,(nc_type *)NULL,
                (int *)NULL,dim_id,(int *)NULL);
 
  /* Get the dimension sizes and names from the input file */
  for(idx=0;idx<nbr_dim;idx++){
  /* NB: For the unlimited dimension, ncdiminq() returns the maximum
     value used so far in writing data for that dimension.
     Thus if you read the dimension sizes from the output file, then
     the ncdiminq() returns dim_sz=0 for the unlimited dimension
     until a variable has been written with that dimension. This is
     the reason for always reading the input file for the dimension
     sizes. */

    ncdiminq(in_id,dim_id[idx],(char *)NULL,dim_cnt+idx);

    /* Initialize the indicial offset and stride arrays */
    dim_srt[idx]=0L;
    var_sz*=dim_cnt[idx];
  } /* end loop over dim */

  /* Allocate enough space to hold the variable */
  void_ptr=malloc(var_sz*nctypelen(var_type_in));

  /* Get the variable */

  /* if variable is float or double, convert if necessary */

  if(nbr_dim==0){  /* variable is a scalar */

    ncvarget1(in_id,var_in_id,0L,void_ptr);

    if ( ( (var_type_in == NC_FLOAT) && (var_type_out == NC_FLOAT) ) ||
         ( (var_type_in == NC_DOUBLE) && (var_type_out == NC_DOUBLE) ) ) {
      /* no conversion necessary */

      ncvarput1(out_id,var_out_id,0L,void_ptr);

    } else if ( (var_type_in == NC_FLOAT) && (var_type_out == NC_DOUBLE) ) {
      /* convert up */

      ncvarput1(out_id,var_out_id,0L,
                ex_conv_array (out_id, WRITE_CONVERT_UP, void_ptr, 1));

    } else if ( (var_type_in == NC_DOUBLE) && (var_type_out == NC_FLOAT) ) {
      /* convert down */

      ncvarput1(out_id,var_out_id,0L,
                ex_conv_array (out_id, WRITE_CONVERT_DOWN, void_ptr, 1));

    } else {  /* variable isn't float or double */

      /* no conversion necessary */

      ncvarput1(out_id,var_out_id,0L,void_ptr);

    }

  } else { /* variable is a vector */

    ncvarget(in_id,var_in_id,dim_srt,dim_cnt,void_ptr);

    if ( ( (var_type_in == NC_FLOAT) && (var_type_out == NC_FLOAT) ) ||
         ( (var_type_in == NC_DOUBLE) && (var_type_out == NC_DOUBLE) ) ) {
      /* no conversion necessary */

      ncvarput(out_id,var_out_id,dim_srt,dim_cnt,void_ptr);

    } else if ( (var_type_in == NC_FLOAT) && (var_type_out == NC_DOUBLE) ) {
      /* convert up */

      ncvarput(out_id,var_out_id,dim_srt,dim_cnt,
               ex_conv_array (out_id,WRITE_CONVERT_UP,void_ptr,var_sz));

    } else if ( (var_type_in == NC_DOUBLE) && (var_type_out == NC_FLOAT) ) {
      /* convert down */

      ncvarput(out_id,var_out_id,dim_srt,dim_cnt,
               ex_conv_array (out_id,WRITE_CONVERT_DOWN,void_ptr,var_sz));

    } else {  /* variable isn't float or double */

      /* no conversion necessary */

      ncvarput(out_id,var_out_id,dim_srt,dim_cnt,void_ptr);

    }

  } /* end if variable is an array */

  /* Free the space that held the dimension IDs */
  (void)free(dim_cnt);
  (void)free(dim_id);
  (void)free(dim_sz);
  (void)free(dim_srt);

  /* Free the space that held the variable */
  (void)free(void_ptr);

  return(EX_NOERR);

} /* end cpy_var_val() */

int
cpy_coord_val(int in_id,int out_id,char *var_nm,
              int in_large, int out_large)
/*
   int in_id: input netCDF input-file ID
   int out_id: input netCDF output-file ID
   char *var_nm: input variable name
 */
{
  /* Routine to copy the coordinate data from an input netCDF file
   * to an output netCDF file. 
   */

  const char *routine = NULL;
  int i;
  long spatial_dim, num_nodes;
  long start[2], count[2];
  nc_type var_type_in, var_type_out;

  void *void_ptr;

  /* Handle easiest situation first: in_large matches out_large */
  if (in_large == out_large)
    return cpy_var_val(in_id, out_id, var_nm);
  
  /* At this point, know that in_large != out_large, so will need to
     either copy a vector to multiple scalars or vice-versa.  Also
     will a couple dimensions, so get them now.*/
  ex_get_dimension(in_id, DIM_NUM_DIM, "dimension", &spatial_dim, routine);
  ex_get_dimension(in_id, DIM_NUM_NODES, "nodes",   &num_nodes, routine);

  if (in_large == 0 && out_large == 1) {
    /* output file will have coordx, coordy, coordz (if 3d). */
    /* Get the var_id for the requested variable from both files. */
    int var_in_id, var_out_id[3];
    var_in_id = ncvarid(in_id, VAR_COORD);
    var_out_id[0] = ncvarid(out_id,VAR_COORD_X);
    var_out_id[1] = ncvarid(out_id,VAR_COORD_Y);
    var_out_id[2] = ncvarid(out_id,VAR_COORD_Z);

    ncvarinq(in_id,var_in_id,(char *)NULL,&var_type_in,(int*)NULL,
                (int *)NULL,(int *)NULL);
    ncvarinq(out_id,var_out_id[0],(char *)NULL,&var_type_out,(int *)NULL,
                (int *)NULL,(int *)NULL);

    void_ptr=malloc(num_nodes * nctypelen(var_type_in));

    /* Copy each component of the variable... */
    for (i=0; i < spatial_dim; i++) {
      start[0] = i; start[1] = 0;
      count[0] = 1; count[1] = num_nodes;
      ncvarget(in_id, var_in_id, start, count, void_ptr);
      
      if (var_type_in == var_type_out) {
        if (var_type_out == NC_FLOAT) {
          nc_put_var_float(out_id, var_out_id[i], void_ptr);
        } else {
          nc_put_var_double(out_id, var_out_id[i], void_ptr);
        }
      } else if (var_type_in == NC_FLOAT && var_type_out == NC_DOUBLE) {
        nc_put_var_double(out_id, var_out_id[i],
                          ex_conv_array(out_id, WRITE_CONVERT_UP, void_ptr, num_nodes));
      } else if (var_type_in == NC_DOUBLE && var_type_out == NC_FLOAT) {
        nc_put_var_float(out_id, var_out_id[i],
                          ex_conv_array(out_id, WRITE_CONVERT_DOWN, void_ptr, num_nodes));
      }
    }
  }

  if (in_large == 1 && out_large == 0) {
    /* input file will have coordx, coordy, coordz (if 3d); output has
       only "coord" */
    int var_in_id[3], var_out_id;
    var_in_id[0] = ncvarid(in_id,  VAR_COORD_X);
    var_in_id[1] = ncvarid(in_id,  VAR_COORD_Y);
    var_in_id[2] = ncvarid(in_id,  VAR_COORD_Z);
    var_out_id   = ncvarid(out_id, VAR_COORD);
    
    ncvarinq(in_id,var_in_id[0],(char *)NULL,&var_type_in,(int *)NULL,
                (int *)NULL,(int *)NULL);

    ncvarinq(out_id,var_out_id,(char *)NULL,&var_type_out,(int*)NULL,
                (int *)NULL,(int *)NULL);

    void_ptr=malloc(num_nodes * nctypelen(var_type_in));

    /* Copy each component of the variable... */
    for (i=0; i < spatial_dim; i++) {
      if (var_type_in == NC_FLOAT) {
        nc_get_var_float(in_id, var_in_id[i], void_ptr);
      } else {
        nc_get_var_double(in_id, var_in_id[i], void_ptr);
      }

      start[0] = i; start[1] = 0;
      count[0] = 1; count[1] = num_nodes;
      if (var_type_in == var_type_out) {
        ncvarput(out_id, var_out_id, start, count, void_ptr);
      } else if (var_type_in == NC_FLOAT && var_type_out == NC_DOUBLE) {
        ncvarput(out_id, var_out_id, start, count,
                 ex_conv_array(out_id, WRITE_CONVERT_UP, void_ptr, num_nodes));
      } else if (var_type_in == NC_DOUBLE && var_type_out == NC_FLOAT) {
        ncvarput(out_id, var_out_id, start, count,
                 ex_conv_array(out_id, WRITE_CONVERT_DOWN, void_ptr, num_nodes));
      }
    }
  }

  /* Free the space that held the variable */
  (void)free(void_ptr);

  return(EX_NOERR);

} /* end cpy_coord_val() */


void update_internal_structs( int out_exoid, int inqcode, struct list_item** ctr_list )
{
  int i;
  int number;
  double fdum;
  char* cdum = 0;

  ex_inquire (out_exoid, inqcode, &number, &fdum, cdum);

  if (number > 0) {
    for (i=0; i<number; i++)
      ex_inc_file_item (out_exoid, ctr_list);
  }
}
