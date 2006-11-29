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
* expvar - ex_put_var
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     time_step               time step number
*       int     var_type                type (edge block, face block, edge set, ... )
*       int     var_index               element variable index
*       int     obj_id                  element block id
*       int     num_entries_this_obj    number of entries in this block/set
*
* exit conditions -
*
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

/*!
 * writes the values of a single element variable for one element block at 
 * one time step to the database; assume the first time step and 
 * element variable index are 1
 */

int ex_put_var (int   exoid,
                int   time_step,
                int   var_type,
                int   var_index,
                int   obj_id,
                int   num_entries_this_obj,
                const void *var_vals)
{
  int varid, dimid,time_dim, numobjdim, dims[2], obj_id_ndx;
  long num_obj, num_obj_var, start[2], count[2];
  nclong *obj_var_truth_tab;
  char errmsg[MAX_ERR_LENGTH];
  const char* tname;

  exerrval = 0; /* clear error code */

#define EX_LOOK_UP_VAR(TNAME,VOBJID,VVAR,VOBJTAB,DNUMOBJ,DNUMOBJENT,DNUMOBJVAR) \
  /* Determine index of obj_id in VOBJID array */ \
  tname = TNAME; \
  obj_id_ndx = ex_id_lkup(exoid,VOBJID,obj_id); \
  if (exerrval != 0)  \
  { \
    if (exerrval == EX_NULLENTITY) \
    { \
      sprintf(errmsg, \
              "Warning: no variables allowed for NULL block %d in file id %d", \
              obj_id,exoid); \
      ex_err("ex_put_var",errmsg,EX_MSG); \
      return (EX_WARN); \
    } \
    else \
    { \
    sprintf(errmsg, \
        "Error: failed to locate " TNAME " id %d in %s array in file id %d", \
            obj_id, VOBJID, exoid); \
    ex_err("ex_put_var",errmsg,exerrval); \
    return (EX_FATAL); \
    } \
  } \
 \
  if ((varid = ncvarid (exoid, \
                        VVAR(var_index,obj_id_ndx))) == -1) \
  { \
    if (ncerr == NC_ENOTVAR) /* variable doesn't exist, create it! */ \
    { \
 \
/*    inquire previously defined dimensions */ \
 \
      /* check for the existance of an TNAME variable truth table */ \
      if ((varid = ncvarid (exoid, VOBJTAB)) != -1) \
      { \
        /* find out number of TNAMEs and TNAME variables */ \
        if ((dimid = ncdimid (exoid, DNUMOBJ)) == -1) \
        { \
          exerrval = ncerr; \
          sprintf(errmsg, \
               "Error: failed to locate number of " TNAME "s in file id %d", \
                  exoid); \
          ex_err("ex_put_var",errmsg,exerrval); \
          return (EX_FATAL); \
        } \
 \
        if (ncdiminq (exoid, dimid, (char *) 0, &num_obj) == -1) \
        { \
          exerrval = ncerr; \
          sprintf(errmsg, \
                 "Error: failed to get number of " TNAME "s in file id %d", \
                  exoid); \
          ex_err("ex_put_var",errmsg,exerrval); \
          return (EX_FATAL); \
        } \
 \
        if ((dimid = ncdimid (exoid, DNUMOBJVAR)) == -1) \
        { \
          exerrval = EX_BADPARAM; \
          sprintf(errmsg, \
               "Error: no " TNAME " variables stored in file id %d", \
                  exoid); \
          ex_err("ex_put_var",errmsg,exerrval); \
          return (EX_FATAL); \
        } \
 \
        if (ncdiminq (exoid, dimid, (char *) 0, &num_obj_var) == -1) \
        { \
          exerrval = ncerr; \
          sprintf(errmsg, \
               "Error: failed to get number of " TNAME " variables in file id %d", \
                  exoid); \
          ex_err("ex_put_var",errmsg,exerrval); \
          return (EX_FATAL); \
        } \
 \
        if (!(obj_var_truth_tab = malloc(num_obj*num_obj_var*sizeof(nclong)))) \
        { \
          exerrval = EX_MEMFAIL; \
          sprintf(errmsg, \
                 "Error: failed to allocate memory for " TNAME " variable truth table in file id %d", \
                  exoid); \
          ex_err("ex_put_var",errmsg,exerrval); \
          return (EX_FATAL); \
        } \
 \
        /*   read in the TNAME variable truth table */ \
 \
        start[0] = 0; \
        start[1] = 0; \
 \
        count[0] = num_obj; \
        count[1] = num_obj_var; \
 \
        if (ncvarget (exoid, varid, start, count, obj_var_truth_tab) == -1) \
        { \
          exerrval = ncerr; \
          sprintf(errmsg, \
                 "Error: failed to get truth table from file id %d", exoid); \
          ex_err("ex_put_var",errmsg,exerrval); \
          return (EX_FATAL); \
        } \
 \
        if(obj_var_truth_tab[num_obj_var*(obj_id_ndx-1)+var_index-1]  \
           == 0L) \
        { \
          free(obj_var_truth_tab); \
          exerrval = EX_BADPARAM; \
          sprintf(errmsg, \
              "Error: Invalid " TNAME " variable %d, " TNAME " %d in file id %d", \
                  var_index, obj_id, exoid); \
          ex_err("ex_put_var",errmsg,exerrval); \
          return (EX_FATAL); \
        } \
        free(obj_var_truth_tab); \
      } \
 \
      if ((time_dim = ncdimid (exoid, DIM_TIME)) == -1) \
      { \
        exerrval = ncerr; \
        sprintf(errmsg, \
               "Error: failed to locate time dimension in file id %d", exoid); \
        ex_err("ex_put_var",errmsg,exerrval); \
        goto error_ret;         /* exit define mode and return */ \
      } \
 \
      if ((numobjdim=ncdimid(exoid, DNUMOBJENT(obj_id_ndx))) == -1) \
      { \
        if (ncerr == NC_EBADDIM) \
        { \
          exerrval = ncerr; \
          sprintf(errmsg, \
      "Error: number of entries in " TNAME " %d not defined in file id %d", \
                  obj_id, exoid); \
          ex_err("ex_put_var",errmsg,exerrval); \
        } \
        else \
        { \
          exerrval = ncerr; \
          sprintf(errmsg, \
 "Error: failed to locate number of entries in " TNAME " %d in file id %d", \
                  obj_id, exoid); \
          ex_err("ex_put_var",errmsg,exerrval); \
        } \
        goto error_ret; \
      } \
 \
/*    variable doesn't exist so put file into define mode  */ \
 \
      if (ncredef (exoid) == -1) \
      { \
        exerrval = ncerr; \
        sprintf(errmsg, \
               "Error: failed to put file id %d into define mode", exoid); \
        ex_err("ex_put_var",errmsg,exerrval); \
        return (EX_FATAL); \
      } \
 \
 \
/*    define netCDF variable to store TNAME variable values */ \
 \
      dims[0] = time_dim; \
      dims[1] = numobjdim; \
      if ((varid = ncvardef(exoid,VVAR(var_index,obj_id_ndx), \
                            nc_flt_code(exoid), 2, dims)) == -1) \
      { \
        exerrval = ncerr; \
        sprintf(errmsg, \
               "Error: failed to define " TNAME " variable %d in file id %d", \
                var_index,exoid); \
        ex_err("ex_put_var",errmsg,exerrval); \
        goto error_ret; \
      } \
 \
 \
/*    leave define mode  */ \
 \
      if (ncendef (exoid) == -1) \
      { \
        exerrval = ncerr; \
        sprintf(errmsg, \
       "Error: failed to complete " TNAME " variable %s definition to file id %d", \
                VVAR(var_index,obj_id_ndx), exoid); \
        ex_err("ex_put_var",errmsg,exerrval); \
        return (EX_FATAL); \
      } \
    } \
    else \
    { \
      exerrval = ncerr; \
      sprintf(errmsg, \
             "Error: failed to locate " TNAME " variable %s in file id %d", \
              VVAR(var_index,obj_id_ndx),exoid); \
      ex_err("ex_put_var",errmsg,exerrval); \
      return (EX_FATAL); \
    } \
  }

  switch (var_type) {
  case EX_GLOBAL:
    if ( num_entries_this_obj <= 0 ) {
     exerrval = EX_MSG;
     sprintf(errmsg,
            "Warning: no global variables specified for file id %d",
             exoid);
     ex_err("ex_put_glob_vars",errmsg,exerrval);

     return (EX_WARN);
    }
    /* inquire previously defined variable */

    if ((varid = ncvarid (exoid, VAR_GLO_VAR)) == -1) {
      if (ncerr == NC_ENOTVAR) {
        exerrval = ncerr;
        sprintf(errmsg,
          "Error: no global variables defined in file id %d",
          exoid);
        ex_err("ex_put_glob_vars",errmsg,exerrval);
      } else {
        exerrval = ncerr;
        sprintf(errmsg,
          "Error: failed to get global variables parameters in file id %d",
          exoid);
        ex_err("ex_put_glob_vars",errmsg,exerrval);
      }
      return (EX_FATAL);
    } 
    break;
  case EX_EDGE_BLOCK:
    EX_LOOK_UP_VAR("edge block",VAR_ID_ED_BLK,VAR_EDGE_VAR,VAR_EBLK_TAB,DIM_NUM_ED_BLK,DIM_NUM_ED_IN_EBLK,DIM_NUM_EDG_VAR);
    break;
  case EX_FACE_BLOCK:
    EX_LOOK_UP_VAR("face block",VAR_ID_FA_BLK,VAR_FACE_VAR,VAR_FBLK_TAB,DIM_NUM_FA_BLK,DIM_NUM_FA_IN_FBLK,DIM_NUM_FAC_VAR);
    break;
  case EX_ELEM_BLOCK:
    EX_LOOK_UP_VAR("element block",VAR_ID_EL_BLK,VAR_ELEM_VAR,VAR_ELEM_TAB,DIM_NUM_EL_BLK,DIM_NUM_EL_IN_BLK,DIM_NUM_ELE_VAR);
    break;
  case EX_NODE_SET:
    EX_LOOK_UP_VAR("node set",VAR_NS_IDS,VAR_NS_VAR,VAR_NSET_TAB,DIM_NUM_NS,DIM_NUM_NOD_NS,DIM_NUM_NSET_VAR);
    break;
  case EX_EDGE_SET:
    EX_LOOK_UP_VAR("edge set",VAR_ES_IDS,VAR_ES_VAR,VAR_ESET_TAB,DIM_NUM_ES,DIM_NUM_EDGE_ES,DIM_NUM_ESET_VAR);
    break;
  case EX_FACE_SET:
    EX_LOOK_UP_VAR("face set",VAR_FS_IDS,VAR_FS_VAR,VAR_FSET_TAB,DIM_NUM_FS,DIM_NUM_FACE_FS,DIM_NUM_FSET_VAR);
    break;
  case EX_SIDE_SET:
    EX_LOOK_UP_VAR("side set",VAR_SS_IDS,VAR_SS_VAR,VAR_SSET_TAB,DIM_NUM_SS,DIM_NUM_SIDE_SS,DIM_NUM_SSET_VAR);
    break;
  case EX_ELEM_SET:
    EX_LOOK_UP_VAR("element set",VAR_ELS_IDS,VAR_ELS_VAR,VAR_ELSET_TAB,DIM_NUM_ELS,DIM_NUM_ELE_ELS,DIM_NUM_ELSET_VAR);
    break;
  default:
    exerrval = EX_MSG;
    sprintf( errmsg, "Error: invalid variable type (%d) specified for file id %d",
      var_type, exoid );
    ex_err( "ex_put_var", errmsg, exerrval );
    return (EX_FATAL);
  }
/* store element variable values */

  start[0] = --time_step;
  start[1] = 0;

  if ( var_type == EX_GLOBAL ) {
    /* global variables may be written
     * - all at once (by setting var_index to 1 and num_entries_this_obj to num_glob, or
     * - one at a time (by setting var_index to the desired index and num_entries_this_obj to 1.
     */
    count[0] = var_index;
  } else {
    count[0] = 1;
  }
  count[1] = num_entries_this_obj;

  if (ncvarput (exoid, varid, start, count, 
                ex_conv_array(exoid,WRITE_CONVERT,var_vals,
                num_entries_this_obj)) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to store %s variable %d in file id %d", 
            tname,var_index,exoid);
    ex_err("ex_put_var",errmsg,exerrval);
    return (EX_FATAL);
  }

  return (EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  if (ncendef (exoid) == -1)     /* exit define mode */
  {
    sprintf(errmsg,
           "Error: failed to complete definition for file id %d",
            exoid);
    ex_err("ex_put_var",errmsg,exerrval);
  }
  return (EX_FATAL);
}
