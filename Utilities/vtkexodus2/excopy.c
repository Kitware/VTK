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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "exodusII.h"
#include "exodusII_int.h"

struct ncdim {                  /* dimension */
    char name[MAX_STR_LENGTH];
    size_t size;
};

struct ncvar {                  /* variable */
    char name[MAX_STR_LENGTH];
    nc_type type;
    int ndims;
    int dims[NC_MAX_VAR_DIMS];
    int natts;
};

struct ncatt {                  /* attribute */
    int var;
    char name[MAX_STR_LENGTH];
    nc_type type;
    size_t len;
    void *val;
};

static size_t type_size(nc_type type);
static int cpy_att    (int, int, int, int);
static int cpy_var_def(int, int, int, char*);
static int cpy_var_val(int, int, char*);
static int cpy_coord_def(int in_id,int out_id,int rec_dim_id,
			 char *var_nm, int in_large, int out_large);
static int cpy_coord_val(int in_id,int out_id,char *var_nm,
			 int in_large, int out_large);
static void update_internal_structs( int, ex_inquiry, struct list_item** );

/*!
 *  efficiently copies all non-transient information (attributes,
 * dimensions, and variables from an opened EXODUS file to another
 * opened EXODUS file.  Will not overwrite a dimension or variable
 * already defined in the new file.
 * \param      in_exoid     exodus file id for input file
 * \param      out_exoid    exodus file id for output file
 */

int ex_copy (int in_exoid, int out_exoid)
{
   int status;
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
   int i;
   size_t numrec;
   size_t dim_sz;
   char dim_nm[NC_MAX_NAME];
   int in_large, out_large;
   
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

   nc_inq(in_exoid, &ndims, &nvars, &ngatts, &recdimid);
   nc_inq_dimlen(in_exoid, recdimid, &numrec);

   /* put output file into define mode */

   nc_redef(out_exoid);

   /* copy global attributes */

   for (i = 0; i < ngatts; i++) {

     nc_inq_attname(in_exoid, NC_GLOBAL, i, att.name);
        
     /* if attribute exists in output file, don't overwrite it; compute 
      * word size, I/O word size etc. are global attributes stored when
      * file is created with ex_create;  we don't want to overwrite those
      */
     if ((status = nc_inq_att(out_exoid, NC_GLOBAL, att.name, &att.type, &att.len)) != NC_NOERR) {

        /* The "last_written_time" attribute is a special attribute
           used by the Sierra IO system to determine whether a
           timestep has been fully written to the database in order to
           try to detect a database crash that happens in the middle
           of a database output step. Don't want to copy that attribute.
        */
        if (strcmp(att.name,"last_written_time") != 0) {
          /* attribute doesn't exist in new file so OK to create it */
          nc_copy_att(in_exoid,NC_GLOBAL,att.name,out_exoid,NC_GLOBAL);
        }
      }
   }

   /* copy dimensions */

   /* Get the dimension sizes and names */

   for(dimid = 0; dimid < ndims; dimid++){

     nc_inq_dim(in_exoid,dimid,dim_nm,&dim_sz);

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
       
       /* See if the dimension has already been defined */
       status = nc_inq_dimid(out_exoid, dim_nm, &dim_out_id);
       
       if(status != NC_NOERR) {
	 if(dimid != recdimid) {
	   status = nc_def_dim(out_exoid, dim_nm, dim_sz,       &dim_out_id);
	 } else {
	   status = nc_def_dim(out_exoid, dim_nm, NC_UNLIMITED, &dim_out_id);
	 } /* end else */
       } /* end if */
     } /* end if */
   } /* end loop over dim */

   /* copy variable definitions and variable attributes */
   for (varid = 0; varid < nvars; varid++) {

      nc_inq_var(in_exoid, varid, var.name, &var.type, &var.ndims, 
		 var.dims, &var.natts);

      /* we don't want to copy some variables because there is not a
       * simple way to add to them;
       * QA records, info records and all results variables (nodal
       * element, and global results) are examples
       */

      if ( ( strcmp(var.name,VAR_QA_TITLE)      != 0) &&
           ( strcmp(var.name,VAR_INFO)          != 0) &&
           ( strcmp(var.name,VAR_EBLK_TAB)      != 0) &&
           ( strcmp(var.name,VAR_FBLK_TAB)      != 0) &&
           ( strcmp(var.name,VAR_ELEM_TAB)      != 0) &&
           ( strcmp(var.name,VAR_ELSET_TAB)     != 0) &&
           ( strcmp(var.name,VAR_SSET_TAB)      != 0) &&
           ( strcmp(var.name,VAR_FSET_TAB)      != 0) &&
           ( strcmp(var.name,VAR_ESET_TAB)      != 0) &&
           ( strcmp(var.name,VAR_NSET_TAB)      != 0) &&
           ( strcmp(var.name,VAR_NAME_GLO_VAR)  != 0) &&
           ( strcmp(var.name,VAR_GLO_VAR)       != 0) &&
           ( strcmp(var.name,VAR_NAME_NOD_VAR)  != 0) &&
           ( strcmp(var.name,VAR_NOD_VAR)       != 0) &&
           ( strcmp(var.name,VAR_NAME_EDG_VAR)  != 0) &&
           ( strcmp(var.name,VAR_NAME_FAC_VAR)  != 0) &&
           ( strcmp(var.name,VAR_NAME_ELE_VAR)  != 0) &&
           ( strcmp(var.name,VAR_NAME_NSET_VAR) != 0) &&
           ( strcmp(var.name,VAR_NAME_ESET_VAR) != 0) &&
           ( strcmp(var.name,VAR_NAME_FSET_VAR) != 0) &&
           ( strcmp(var.name,VAR_NAME_SSET_VAR) != 0) &&
           ( strcmp(var.name,VAR_NAME_ELSET_VAR) != 0)&&
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
   nc_enddef (out_exoid);

   /* output variable data */

   for (varid = 0; varid < nvars; varid++) {
      nc_inq_var(in_exoid, varid, var.name, &var.type, &var.ndims,
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
   update_internal_structs( out_exoid, EX_INQ_EDGE_BLK, ex_get_counter_list(EX_EDGE_BLOCK));
   update_internal_structs( out_exoid, EX_INQ_FACE_BLK, ex_get_counter_list(EX_FACE_BLOCK));
   update_internal_structs( out_exoid, EX_INQ_ELEM_BLK, ex_get_counter_list(EX_ELEM_BLOCK));

   /* if number of sets > 0 */
   update_internal_structs( out_exoid, EX_INQ_NODE_SETS, ex_get_counter_list(EX_NODE_SET));
   update_internal_structs( out_exoid, EX_INQ_EDGE_SETS, ex_get_counter_list(EX_EDGE_SET));
   update_internal_structs( out_exoid, EX_INQ_FACE_SETS, ex_get_counter_list(EX_FACE_SET));
   update_internal_structs( out_exoid, EX_INQ_SIDE_SETS, ex_get_counter_list(EX_SIDE_SET));
   update_internal_structs( out_exoid, EX_INQ_ELEM_SETS, ex_get_counter_list(EX_ELEM_SET));

   /* if number of maps > 0 */
   update_internal_structs( out_exoid, EX_INQ_NODE_MAP, ex_get_counter_list(EX_NODE_MAP));
   update_internal_structs( out_exoid, EX_INQ_EDGE_MAP, ex_get_counter_list(EX_EDGE_MAP));
   update_internal_structs( out_exoid, EX_INQ_FACE_MAP, ex_get_counter_list(EX_FACE_MAP));
   update_internal_structs( out_exoid, EX_INQ_ELEM_MAP, ex_get_counter_list(EX_ELEM_MAP));

   return(EX_NOERR);
}

int cpy_att(int in_id,int out_id,int var_in_id,int var_out_id)
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

  if(var_in_id == NC_GLOBAL) {
    nc_inq_natts(in_id,&nbr_att);
  } else {
    nc_inq_varnatts(in_id, var_in_id, &nbr_att);
  }

  /* Get the attributes names, types, lengths, and values */
  for (idx=0; idx<nbr_att; idx++) {
    char att_nm[MAX_STR_LENGTH];

    nc_inq_attname(in_id, var_in_id, idx, att_nm);
    nc_copy_att(in_id, var_in_id, att_nm, out_id, var_out_id);
  } 

  return(EX_NOERR);
} 

int cpy_coord_def(int in_id,int out_id,int rec_dim_id,char *var_nm,
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
  int status;
  size_t spatial_dim;
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
  ex_get_dimension(in_id, DIM_NUM_DIM, "dimension", &spatial_dim, &temp, routine);
  
  if (in_large == 0 && out_large == 1) {
    /* output file will have coordx, coordy, coordz (if 3d).  See if
       they are already defined in output file. Assume either all or
       none are defined. */

    {
      int var_out_idx, var_out_idy, var_out_idz;
      int status1 = nc_inq_varid(out_id, VAR_COORD_X, &var_out_idx);
      int status2 = nc_inq_varid(out_id, VAR_COORD_Y, &var_out_idy);
      int status3 = nc_inq_varid(out_id, VAR_COORD_Y, &var_out_idz);

      if (status1 == NC_NOERR && status2 == NC_NOERR &&
          (spatial_dim == 2 || status3 == NC_NOERR)) {
        return NC_NOERR; /* already defined in output file */
      }
    }

    /* Get dimid of the num_nodes dimension in output file... */
    nc_inq_dimid(out_id, DIM_NUM_NODES, &dim_out_id[0]);

    /* Define the variables in the output file */
    
    /* Define according to the EXODUS file's IO_word_size */
    nbr_dim = 1;
    nc_def_var(out_id, VAR_COORD_X, nc_flt_code(out_id), nbr_dim, dim_out_id, &var_out_id);
    nc_def_var(out_id, VAR_COORD_Y, nc_flt_code(out_id), nbr_dim, dim_out_id, &var_out_id);
    if (spatial_dim == 3)
      nc_def_var(out_id, VAR_COORD_Z, nc_flt_code(out_id), nbr_dim, dim_out_id, &var_out_id);
  }

  if (in_large == 1 && out_large == 0) {
    /* input file has coordx, coordy, coordz (if 3d); output will only
       have "coord".  See if is already defined in output file. */
    status = nc_inq_varid(out_id, VAR_COORD, &var_out_id);
    if (status == NC_NOERR)
      return NC_NOERR; /* already defined in output file */

    /* Get dimid of the spatial dimension and num_nodes dimensions in output file... */
    nc_inq_dimid(out_id, DIM_NUM_DIM,   &dim_out_id[0]);
    nc_inq_dimid(out_id, DIM_NUM_NODES, &dim_out_id[1]);

    /* Define the variable in the output file */
    
    /* Define according to the EXODUS file's IO_word_size */
    nbr_dim = 2;
    nc_def_var(out_id, VAR_COORD, nc_flt_code(out_id), nbr_dim, dim_out_id, &var_out_id);
  }
  return var_out_id;
}

int cpy_var_def(int in_id,int out_id,int rec_dim_id,char *var_nm)
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

  int status;
  int *dim_in_id;
  int *dim_out_id;
  int idx;
  int nbr_dim;
  int var_in_id;
  int var_out_id;

  nc_type var_type;

  /* See if the requested variable is already in the output file. */
  status = nc_inq_varid(out_id, var_nm, &var_out_id);
  if(status == NC_NOERR)
    return var_out_id;

  /* See if the requested variable is in the input file. */
  nc_inq_varid(in_id, var_nm, &var_in_id);

  /* Get the type of the variable and the number of dimensions. */
  nc_inq_vartype (in_id, var_in_id, &var_type);
  nc_inq_varndims(in_id, var_in_id, &nbr_dim);

  /* Recall:
     1. The dimensions must be defined before the variable.
     2. The variable must be defined before the attributes. */

  /* Allocate space to hold the dimension IDs */
  dim_in_id=malloc(nbr_dim*sizeof(int)); 
  dim_out_id=malloc(nbr_dim*sizeof(int));

  /* Get the dimension IDs */
  nc_inq_vardimid(in_id, var_in_id, dim_in_id);

  /* Get the dimension sizes and names */
  for(idx=0;idx<nbr_dim;idx++){
    char dim_nm[NC_MAX_NAME];
    size_t dim_sz;

    nc_inq_dim(in_id, dim_in_id[idx], dim_nm, &dim_sz);

    /* See if the dimension has already been defined */
    status = nc_inq_dimid(out_id, dim_nm, &dim_out_id[idx]);

    /* If the dimension hasn't been defined, copy it */
    if (status != NC_NOERR) {
      if (dim_in_id[idx] != rec_dim_id) {
        nc_def_dim(out_id, dim_nm, dim_sz, &dim_out_id[idx]);
      } else {
        nc_def_dim(out_id, dim_nm, NC_UNLIMITED, &dim_out_id[idx]);
      } 
    } 
  } 

  /* Define the variable in the output file */

  /* If variable is float or double, define it according to the EXODUS
     file's IO_word_size */

  if ((var_type == NC_FLOAT) || (var_type == NC_DOUBLE)) {
    nc_def_var(out_id, var_nm, nc_flt_code(out_id), nbr_dim, dim_out_id, &var_out_id);
  } else {
    nc_def_var(out_id, var_nm, var_type,            nbr_dim, dim_out_id, &var_out_id);
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
  size_t *dim_cnt;
  size_t *dim_sz;
  size_t *dim_srt;
  size_t var_sz=1L;
  nc_type var_type_in, var_type_out;

  void *void_ptr = NULL;

  /* Get the var_id for the requested variable from both files. */
  nc_inq_varid(in_id, var_nm, &var_in_id);
  nc_inq_varid(out_id,var_nm, &var_out_id);
 
  /* Get the number of dimensions for the variable. */
  nc_inq_vartype( out_id, var_out_id, &var_type_out);
  nc_inq_varndims(out_id, var_out_id, &nbr_dim);

  nc_inq_vartype( in_id,   var_in_id, &var_type_in);
  nc_inq_varndims(in_id,   var_in_id, &nbr_dim);
 
  /* Allocate space to hold the dimension IDs */
  dim_cnt = malloc(nbr_dim*sizeof(size_t));

  dim_id=malloc(nbr_dim*sizeof(int));

  dim_sz=malloc(nbr_dim*sizeof(size_t));

  dim_srt=malloc(nbr_dim*sizeof(size_t));
 
  /* Get the dimension IDs from the input file */
  nc_inq_vardimid(in_id, var_in_id, dim_id);
 
  /* Get the dimension sizes and names from the input file */
  for(idx=0;idx<nbr_dim;idx++){
  /* NB: For the unlimited dimension, ncdiminq() returns the maximum
     value used so far in writing data for that dimension.
     Thus if you read the dimension sizes from the output file, then
     the ncdiminq() returns dim_sz=0 for the unlimited dimension
     until a variable has been written with that dimension. This is
     the reason for always reading the input file for the dimension
     sizes. */

    nc_inq_dimlen(in_id,dim_id[idx],dim_cnt+idx);

    /* Initialize the indicial offset and stride arrays */
    dim_srt[idx]=0L;
    var_sz*=dim_cnt[idx];
  } /* end loop over dim */

  /* Allocate enough space to hold the variable */
  void_ptr=malloc(var_sz * type_size(var_type_in));

  /* Get the variable */

  /* if variable is float or double, convert if necessary */

  if(nbr_dim==0){  /* variable is a scalar */

    if (var_type_in == NC_INT && var_type_out == NC_INT) {
      nc_get_var1_int(in_id,  var_in_id,  0L, void_ptr);
      nc_put_var1_int(out_id, var_out_id, 0L, void_ptr);
    }

    else if (var_type_in == NC_FLOAT) {
      nc_get_var1_float(in_id,  var_in_id,  0L, void_ptr);
      nc_put_var1_float(out_id, var_out_id, 0L, void_ptr);
    }

    else if (var_type_in == NC_DOUBLE) {
      nc_get_var1_double(in_id,  var_in_id,  0L, void_ptr);
      nc_put_var1_double(out_id, var_out_id, 0L, void_ptr);
    }

    else if (var_type_in == NC_CHAR) {
      nc_get_var1_text(in_id,  var_in_id,  0L, void_ptr);
      nc_put_var1_text(out_id, var_out_id, 0L, void_ptr);
    }

    else {
      assert(1==0);
    }
  } else { /* variable is a vector */

    if (var_type_in == NC_INT && var_type_out == NC_INT) {
      nc_get_var_int(in_id,  var_in_id,  void_ptr);
      nc_put_var_int(out_id, var_out_id, void_ptr);
    }

    else if (var_type_in == NC_FLOAT) {
      nc_get_var_float(in_id,  var_in_id,  void_ptr);
      nc_put_var_float(out_id, var_out_id, void_ptr);
    }

    else if (var_type_in == NC_DOUBLE) {
      nc_get_var_double(in_id,  var_in_id,  void_ptr);
      nc_put_var_double(out_id, var_out_id, void_ptr);
    }

    else if (var_type_in == NC_CHAR) {
      nc_get_var_text(in_id,  var_in_id,  void_ptr);
      nc_put_var_text(out_id, var_out_id, void_ptr);
    }

    else {
      assert(1==0);
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
  size_t i;
  int temp;
  size_t spatial_dim, num_nodes;
  size_t start[2], count[2];
  nc_type var_type_in, var_type_out;

  void *void_ptr = NULL;

  /* Handle easiest situation first: in_large matches out_large */
  if (in_large == out_large)
    return cpy_var_val(in_id, out_id, var_nm);
  
  /* At this point, know that in_large != out_large, so will need to
     either copy a vector to multiple scalars or vice-versa.  Also
     will need a couple dimensions, so get them now.*/
  ex_get_dimension(in_id, DIM_NUM_DIM, "dimension", &spatial_dim, &temp, routine);
  ex_get_dimension(in_id, DIM_NUM_NODES, "nodes",   &num_nodes, &temp, routine);

  if (in_large == 0 && out_large == 1) {
    /* output file will have coordx, coordy, coordz (if 3d). */
    /* Get the var_id for the requested variable from both files. */
    int var_in_id, var_out_id[3];
    nc_inq_varid(in_id, VAR_COORD, &var_in_id);

    nc_inq_varid(out_id, VAR_COORD_X, &var_out_id[0]);
    nc_inq_varid(out_id, VAR_COORD_Y, &var_out_id[1]);
    nc_inq_varid(out_id, VAR_COORD_Z, &var_out_id[2]);

    nc_inq_vartype( in_id, var_in_id,     &var_type_in);
    nc_inq_vartype(out_id, var_out_id[0], &var_type_out);

    void_ptr=malloc(num_nodes * type_size(var_type_in));

    /* Copy each component of the variable... */
    for (i=0; i < spatial_dim; i++) {
      start[0] = i; start[1] = 0;
      count[0] = 1; count[1] = num_nodes;
      if (var_type_in == NC_FLOAT) {
	nc_get_vara_float(in_id, var_in_id,     start, count, void_ptr);
	nc_put_var_float(out_id, var_out_id[i],               void_ptr);
      } else {
	assert(var_type_in == NC_DOUBLE);
	nc_get_vara_double(in_id, var_in_id,    start, count, void_ptr);
	nc_put_var_double(out_id, var_out_id[i],              void_ptr);
      }
    }
  }

  if (in_large == 1 && out_large == 0) {
    /* input file will have coordx, coordy, coordz (if 3d); output has only "coord" */
    int var_in_id[3], var_out_id;
    nc_inq_varid(in_id,  VAR_COORD_X, &var_in_id[0]);
    nc_inq_varid(in_id,  VAR_COORD_Y, &var_in_id[1]);
    nc_inq_varid(in_id,  VAR_COORD_Z, &var_in_id[2]);
    nc_inq_varid(out_id, VAR_COORD,   &var_out_id);
    
    nc_inq_vartype(in_id,  var_in_id[0], &var_type_in);
    nc_inq_vartype(out_id, var_out_id,   &var_type_out);

    void_ptr=malloc(num_nodes * type_size(var_type_in));

    /* Copy each component of the variable... */
    for (i=0; i < spatial_dim; i++) {
      start[0] = i; start[1] = 0;
      count[0] = 1; count[1] = num_nodes;

      if (var_type_in == NC_FLOAT) {
        nc_get_var_float( in_id,  var_in_id[i],               void_ptr);
        nc_put_vara_float(out_id, var_out_id,   start, count, void_ptr);
      
      } else {
        nc_get_var_double( in_id,  var_in_id[i],               void_ptr);
        nc_put_vara_double(out_id, var_out_id,   start, count, void_ptr);
      }
    }
  }

  /* Free the space that held the variable */
  if (void_ptr)
    {
    (void)free(void_ptr);
    }
  return(EX_NOERR);
} /* end cpy_coord_val() */


void update_internal_structs( int out_exoid, ex_inquiry inqcode, struct list_item** ctr_list )
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

size_t type_size(nc_type type)
{
  if (type == NC_CHAR)
    return sizeof(char);
  else if (type == NC_INT)
    return sizeof(int);
  else if (type == NC_FLOAT)
    return sizeof(float);
  else if (type == NC_DOUBLE)
    return sizeof(double);
  else
    return 0;
}
