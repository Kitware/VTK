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
* exgvtt - ex_get_var_tab
*
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid              exodus file id
*       int     num_blk            number of blocks
*       int     num_var            number of variables
*
* exit conditions - 
*       int*    var_tab            element variable truth table array
*
* revision history - 
*   20061002 - David Thompson - Added edge/face element support
*
*  Id
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

#include <ctype.h>

/*!
 * reads the EXODUS II specified variable truth table from the database
 */

int ex_get_var_tab (int  exoid,
                    const char *var_type,
                    int  num_blk,
                    int  num_var,
                    int *var_tab)
{
  int dimid, varid, tabid, i, j, iresult;
  long num_entity = -1;
  long num_var_db = -1;
  long start[2], count[2]; 
  nclong *longs;
  char errmsg[MAX_ERR_LENGTH];
  const char* routine = "ex_get_var_tab";

  /*
   * The ent_type and the var_name are used to build the netcdf
   * variables name.  Normally this is done via a macro defined in
   * exodusII_int.h
   */
  const char* ent_type = NULL;
  const char* var_name = NULL;
  int vartyp = tolower( *var_type );

  exerrval = 0; /* clear error code */

  switch (vartyp) {
  case 'l':
    dimid = ex_get_dimension(exoid, DIM_NUM_ED_BLK,   "edge", &num_entity, routine);
    varid = ex_get_dimension(exoid, DIM_NUM_EDG_VAR,  "edge variables", &num_var_db, routine);
    tabid = ncvarid (exoid, VAR_EBLK_TAB);
    var_name = "vals_edge_var";
    ent_type = "eb";
    break;
  case 'f':
    dimid = ex_get_dimension(exoid, DIM_NUM_FA_BLK,   "face", &num_entity, routine);
    varid = ex_get_dimension(exoid, DIM_NUM_FAC_VAR,  "face variables", &num_var_db, routine);
    tabid = ncvarid (exoid, VAR_FBLK_TAB);
    var_name = "vals_face_var";
    ent_type = "eb";
    break;
  case 'e':
    dimid = ex_get_dimension(exoid, DIM_NUM_EL_BLK,   "element", &num_entity, routine);
    varid = ex_get_dimension(exoid, DIM_NUM_ELE_VAR,  "element variables", &num_var_db, routine);
    tabid = ncvarid (exoid, VAR_ELEM_TAB);
    var_name = "vals_elem_var";
    ent_type = "eb";
    break;
  case 'm':
    dimid = ex_get_dimension(exoid, DIM_NUM_NS,       "nodeset", &num_entity, routine);
    varid = ex_get_dimension(exoid, DIM_NUM_NSET_VAR, "nodeset variables", &num_var_db, routine);
    tabid = ncvarid (exoid, VAR_NSET_TAB);
    var_name = "vals_nset_var";
    ent_type = "ns";
    break;
  case 'd':
    dimid = ex_get_dimension(exoid, DIM_NUM_ES,       "edgeset", &num_entity, routine);
    varid = ex_get_dimension(exoid, DIM_NUM_ESET_VAR, "edgeset variables", &num_var_db, routine);
    tabid = ncvarid (exoid, VAR_ESET_TAB);
    var_name = "vals_eset_var";
    ent_type = "ns";
    break;
  case 'a':
    dimid = ex_get_dimension(exoid, DIM_NUM_FS,       "faceset", &num_entity, routine);
    varid = ex_get_dimension(exoid, DIM_NUM_FSET_VAR, "faceset variables", &num_var_db, routine);
    tabid = ncvarid (exoid, VAR_FSET_TAB);
    var_name = "vals_fset_var";
    ent_type = "ns";
    break;
  case 's':
    dimid = ex_get_dimension(exoid, DIM_NUM_SS,       "sideset", &num_entity, routine);
    varid = ex_get_dimension(exoid, DIM_NUM_SSET_VAR, "sideset variables", &num_var_db, routine);
    tabid = ncvarid (exoid, VAR_SSET_TAB);
    var_name = "vals_sset_var";
    ent_type = "ss";
    break;
  case 't':
    dimid = ex_get_dimension(exoid, DIM_NUM_ELS,       "elemset", &num_entity, routine);
    varid = ex_get_dimension(exoid, DIM_NUM_ELSET_VAR, "elemset variables", &num_var_db, routine);
    tabid = ncvarid (exoid, VAR_ELSET_TAB);
    var_name = "vals_elset_var";
    ent_type = "els";
    break;
  default:
    exerrval = EX_BADPARAM;
    sprintf(errmsg,
      "Error: Invalid variable type %c specified in file id %d",
      *var_type, exoid);
    ex_err("ex_get_varid",errmsg,exerrval);
    return (EX_WARN);
  }

  if (dimid == -1) {
    exerrval = ncerr;
    return (EX_FATAL);
  }

  if (varid == -1) {
    exerrval = ncerr;
    return (EX_WARN);
  }

  if (num_entity != num_blk) {
    exerrval = EX_FATAL;
    sprintf(errmsg,
      "Error: # of blocks doesn't match those defined in file id %d", exoid);
    ex_err("ex_get_var_tab",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (num_var_db != num_var) {
    exerrval = EX_FATAL;
    sprintf(errmsg,
      "Error: # of variables doesn't match those defined in file id %d", exoid);
    ex_err("ex_get_var_tab",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (tabid == -1) {
    /* since truth table isn't stored in the data file, derive it dynamically */
    for (j=0; j<num_blk; j++) {

      for (i=0; i<num_var; i++) {
        /* NOTE: names are 1-based */
        if ((tabid = ncvarid (exoid, ex_catstr2(var_name, i+1, ent_type, j+1))) == -1) {

          /* variable doesn't exist; put a 0 in the truth table */
          var_tab[j*num_var+i] = 0;
        } else {

          /* variable exists; put a 1 in the truth table */
          var_tab[j*num_var+i] = 1;
        }
      }
    }
  } else {

    /* read in the truth table */

    /*
     * application code has allocated an array of ints but netcdf is
     * expecting a pointer to nclongs; if ints are different sizes
     * than nclongs, we must allocate an array of nclongs then
     * convert them to ints with ltoi
     */

    start[0] = 0;
    start[1] = 0;

    count[0] = num_blk;
    count[1] = num_var;

    if (sizeof(int) == sizeof(nclong)) {
      iresult = ncvarget (exoid, tabid, start, count, var_tab);
    } else {
      if (!(longs = malloc (num_blk*num_var * sizeof(nclong)))) {
        exerrval = EX_MEMFAIL;
        sprintf(errmsg,
          "Error: failed to allocate memory for truth table for file id %d",
          exoid);
        ex_err("ex_get_var_tab",errmsg,exerrval);
        return (EX_FATAL);
      }
      iresult = ncvarget (exoid, tabid, start, count, longs);
    }

    if (iresult == -1) {
      exerrval = ncerr;
      sprintf(errmsg,
        "Error: failed to get truth table from file id %d", exoid);
      ex_err("ex_get_var_tab",errmsg,exerrval);
      return (EX_FATAL);
    }

    if (sizeof(int) != sizeof(nclong)) {
      ltoi (longs, var_tab, num_blk*num_var);
      free (longs);
    }

  } 


  return (EX_NOERR);

}
