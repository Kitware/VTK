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
* exutils - utility routines
*
*  Id
*****************************************************************************/

#if defined(DEBUG_QSORT)
#include <assert.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "exodusII.h"
#include "exodusII_int.h"

struct obj_stats*  exoII_eb = 0;
struct obj_stats*  exoII_ed = 0;
struct obj_stats*  exoII_fa = 0;
struct obj_stats*  exoII_ns = 0;
struct obj_stats*  exoII_es = 0;
struct obj_stats*  exoII_fs = 0;
struct obj_stats*  exoII_ss = 0;
struct obj_stats* exoII_els = 0;
struct obj_stats*  exoII_em = 0;
struct obj_stats* exoII_edm = 0;
struct obj_stats* exoII_fam = 0;
struct obj_stats*  exoII_nm = 0;


/*****************************************************************************
*
* utility routines for string conversions
* ex_catstr  - concatenate  string/number (where number is converted to ASCII)
* ex_catstr2 - concatenate  string1/number1/string2/number2   "
*
* NOTE: these routines reuse the same storage over and over to build
*        concatenated strings, because the strings are just passed to netCDF
*        routines as names used to look up variables.  if the strings returned
*        by these routines are needed for any other purpose, they should
*        immediately be copied into other storage.
*****************************************************************************/

static char ret_string[10*(MAX_VAR_NAME_LENGTH+1)];
static char* cur_string = &ret_string[0];

/** ex_catstr  - concatenate  string/number (where number is converted to ASCII) */
char *ex_catstr (const char *string,
                 int   num)
{
  char* tmp_string = cur_string;
  cur_string += sprintf (cur_string, "%s%d", string, num) + 1;
  if ( cur_string - ret_string > 9*(MAX_VAR_NAME_LENGTH+1) )
    cur_string = ret_string;
  return (tmp_string);
}


/** ex_catstr2 - concatenate  string1num1string2num2   */
char *ex_catstr2 (const char *string1,
                  int   num1,
                  const char *string2,
                  int   num2)
{
  char* tmp_string = cur_string;
  cur_string += sprintf (cur_string, "%s%d%s%d", string1, num1, string2, num2) + 1;
  if ( cur_string - ret_string > 9*(MAX_VAR_NAME_LENGTH+1) )
    cur_string = ret_string;
  return (tmp_string);
}

const char* ex_name_of_object(ex_entity_type obj_type)
{
  switch (obj_type) {
  case EX_NODAL:
    return "nodal";
  case EX_EDGE_BLOCK:
    return "edge block";
  case EX_FACE_BLOCK:
    return "face block";
  case EX_ELEM_BLOCK:
    return "element block";
  case EX_NODE_SET:
    return "node set";
  case EX_EDGE_SET:
    return "edge set";
  case EX_FACE_SET:
    return "face set";
  case EX_SIDE_SET:
    return "side set";
  case EX_ELEM_SET:
    return "element set";
  case EX_ELEM_MAP:
    return "element map";
  case EX_NODE_MAP:
    return "node map";
  case EX_EDGE_MAP:
    return "edge map";
  case EX_FACE_MAP:
    return "face map";
  case EX_GLOBAL:
    return "global";
  default:
    return "invalid type";
  }
}

ex_entity_type ex_var_type_to_ex_entity_type(char var_type)
{
  char var_lower = tolower(var_type);
  if (var_lower == 'n')
    return EX_NODAL;
  else if (var_lower == 'l')
    return EX_EDGE_BLOCK;
  else if (var_lower == 'f')
    return EX_FACE_BLOCK;
  else if (var_lower == 'e')
    return EX_ELEM_BLOCK;
  else if (var_lower == 'm')
    return EX_NODE_SET;
  else if (var_lower == 'd')
    return EX_EDGE_SET;
  else if (var_lower == 'a')
    return EX_FACE_SET;
  else if (var_lower == 's')
    return EX_SIDE_SET;
  else if (var_lower == 't')
    return EX_ELEM_SET;
  else if (var_lower == 'g')
    return EX_GLOBAL;
  else
    return EX_INVALID;
}

const char* ex_dim_num_objects(ex_entity_type obj_type)
{
   exerrval = 0; /* clear error code */
   switch (obj_type)
   {
     case EX_NODAL:
       return DIM_NUM_NODES;
     case EX_ELEM_BLOCK:
       return DIM_NUM_EL_BLK;
     case EX_EDGE_BLOCK:
       return DIM_NUM_ED_BLK;
     case EX_FACE_BLOCK:
       return DIM_NUM_FA_BLK;
     case EX_NODE_SET:
       return DIM_NUM_NS;
     case EX_EDGE_SET:
       return DIM_NUM_ES;
     case EX_FACE_SET:
       return DIM_NUM_FS;
     case EX_ELEM_SET:
       return DIM_NUM_ELS;
     case EX_SIDE_SET:
       return DIM_NUM_SS;
     case EX_ELEM_MAP:
       return DIM_NUM_EM;
     case EX_FACE_MAP:
       return DIM_NUM_FAM;
     case EX_EDGE_MAP:
       return DIM_NUM_EDM;
     case EX_NODE_MAP:
       return DIM_NUM_NM;
     default:
       {
	 char errmsg[MAX_ERR_LENGTH];
	 exerrval = EX_BADPARAM;
	 sprintf(errmsg, "Error: object type %d not supported in call to ex_dim_num_objects",
		 obj_type);
	 ex_err("ex_dim_num_objects",errmsg,exerrval);
	 return (NULL);
       }
   }
}

const char* ex_dim_num_entries_in_object( ex_entity_type obj_type,
                                    int idx )
{
  switch (obj_type) {
  case EX_NODAL:
    return DIM_NUM_NODES;
  case EX_EDGE_BLOCK:
    return DIM_NUM_ED_IN_EBLK(idx);
  case EX_FACE_BLOCK:
    return DIM_NUM_FA_IN_FBLK(idx);
  case EX_ELEM_BLOCK:
    return DIM_NUM_EL_IN_BLK(idx);
  case EX_NODE_SET:
    return DIM_NUM_NOD_NS(idx);
  case EX_EDGE_SET:
    return DIM_NUM_EDGE_ES(idx);
  case EX_FACE_SET:
    return DIM_NUM_FACE_FS(idx);
  case EX_SIDE_SET:
    return DIM_NUM_SIDE_SS(idx);
  case EX_ELEM_SET:
    return DIM_NUM_ELE_ELS(idx);
  default:
    return 0;
  }
}

char* ex_name_var_of_object(ex_entity_type obj_type,
			    int i, int j)
{
  switch (obj_type) {
  case EX_EDGE_BLOCK:
    return VAR_EDGE_VAR(i,j);
  case EX_FACE_BLOCK:
    return VAR_FACE_VAR(i,j);
  case EX_ELEM_BLOCK:
    return VAR_ELEM_VAR(i,j);
  case EX_NODE_SET:
    return VAR_NS_VAR(i,j);
  case EX_EDGE_SET:
    return VAR_ES_VAR(i,j);
  case EX_FACE_SET:
    return VAR_FS_VAR(i,j);
  case EX_SIDE_SET:
    return VAR_SS_VAR(i,j);
  case EX_ELEM_SET:
    return VAR_ELS_VAR(i,j);
  default:
    return 0;
  }
}

char* ex_name_of_map(ex_entity_type map_type, int map_index)
{
  switch (map_type) {
  case EX_NODE_MAP:
    return VAR_NODE_MAP(map_index);
  case EX_EDGE_MAP:
    return VAR_EDGE_MAP(map_index);
  case EX_FACE_MAP:
    return VAR_FACE_MAP(map_index);
  case EX_ELEM_MAP:
    return VAR_ELEM_MAP(map_index);
  default:
    return 0;
  }
}

/*****************************************************************************
*
* ex_id_lkup - look up id
*
* entry conditions - 
*   input parameters:
*       int            exoid             exodus file id
*       ex_entity_type id_type           id type name:
*                                         elem_ss
*                                         node_ns
*                                         side_ss
*       int     num                     id value
*
* exit conditions - 
*       int     return                  index into table (1-based)
*
* revision history - 
*
*
*****************************************************************************/

int ex_id_lkup( int exoid,
                ex_entity_type id_type,
                int num)
{

  char id_table[MAX_VAR_NAME_LENGTH+1];
  char id_dim[MAX_VAR_NAME_LENGTH+1];
  char stat_table[MAX_VAR_NAME_LENGTH+1];
  int varid, dimid;
  size_t dim_len, i;
  int *id_vals=NULL, *stat_vals=NULL;

  static int filled=FALSE;
  struct obj_stats *tmp_stats;
  int status;
  char errmsg[MAX_ERR_LENGTH];

  exerrval  = 0; /* clear error code */

  switch(id_type) {
  case EX_NODAL:
    return 0;
  case EX_GLOBAL:
    return 0;
  case EX_ELEM_BLOCK:
    strcpy(id_table, VAR_ID_EL_BLK);            /* id array name */
    strcpy(id_dim, DIM_NUM_EL_BLK);             /* id array dimension name*/
    strcpy(stat_table, VAR_STAT_EL_BLK);        /* id status array name */
    tmp_stats = ex_get_stat_ptr (exoid, &exoII_eb);
    break;
  case EX_NODE_SET:
    strcpy(id_table, VAR_NS_IDS);
    strcpy(id_dim, DIM_NUM_NS);
    strcpy(stat_table, VAR_NS_STAT);
    tmp_stats = ex_get_stat_ptr (exoid, &exoII_ns);
    break;
  case EX_SIDE_SET:
    strcpy(id_table, VAR_SS_IDS);
    strcpy(id_dim, DIM_NUM_SS);
    strcpy(stat_table, VAR_SS_STAT);
    tmp_stats = ex_get_stat_ptr (exoid, &exoII_ss);
    break;
  case EX_ELEM_MAP:
    strcpy(id_table, VAR_EM_PROP(1));
    strcpy(id_dim, DIM_NUM_EM);
    strcpy(stat_table, "");
    tmp_stats = ex_get_stat_ptr (exoid, &exoII_em);
    break;
  case EX_NODE_MAP:
    strcpy(id_table, VAR_NM_PROP(1));
    strcpy(id_dim, DIM_NUM_NM);
    strcpy(stat_table, "");
    tmp_stats = ex_get_stat_ptr (exoid, &exoII_nm);
    break;
  case EX_EDGE_BLOCK:
    strcpy(id_table, VAR_ID_ED_BLK);
    strcpy(id_dim, DIM_NUM_ED_BLK);
    strcpy(stat_table, VAR_STAT_ED_BLK);
    tmp_stats = ex_get_stat_ptr (exoid, &exoII_ed);
    break;
  case EX_FACE_BLOCK:
    strcpy(id_table, VAR_ID_FA_BLK);
    strcpy(id_dim, DIM_NUM_FA_BLK);
    strcpy(stat_table, VAR_STAT_FA_BLK);
    tmp_stats = ex_get_stat_ptr (exoid, &exoII_fa);
    break;
  case EX_EDGE_SET:
    strcpy(id_table, VAR_ES_IDS);
    strcpy(id_dim, DIM_NUM_ES);
    strcpy(stat_table, VAR_ES_STAT);
    tmp_stats = ex_get_stat_ptr (exoid, &exoII_es);
    break;
  case EX_FACE_SET:
    strcpy(id_table, VAR_FS_IDS);
    strcpy(id_dim, DIM_NUM_FS);
    strcpy(stat_table, VAR_FS_STAT);
    tmp_stats = ex_get_stat_ptr (exoid, &exoII_fs);
    break;
  case EX_ELEM_SET:
    strcpy(id_table, VAR_ELS_IDS);
    strcpy(id_dim, DIM_NUM_ELS);
    strcpy(stat_table, VAR_ELS_STAT);
    tmp_stats = ex_get_stat_ptr (exoid, &exoII_els);
    break;
  case EX_EDGE_MAP:
    strcpy(id_table, VAR_EDM_PROP(1));
    strcpy(id_dim, DIM_NUM_EDM);
    strcpy(stat_table, "");
    tmp_stats = ex_get_stat_ptr (exoid, &exoII_edm);
    break;
  case EX_FACE_MAP:
    strcpy(id_table, VAR_FAM_PROP(1));
    strcpy(id_dim, DIM_NUM_FAM);
    strcpy(stat_table, "");
    tmp_stats = ex_get_stat_ptr (exoid, &exoII_fam);
    break;
  default:
    exerrval = EX_BADPARAM;
    sprintf(errmsg,
           "Error: unsupported id array type %d for file id %d",
            id_type, exoid);
    ex_err("ex_id_lkup",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ( (tmp_stats->id_vals == NULL) || (!(tmp_stats->valid_ids)) ) {

    /* first time thru or id arrays haven't been completely filled yet */ 

    /* get size of id array */

    /* First get dimension id of id array */
    if ((status = nc_inq_dimid(exoid,id_dim,&dimid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
           "Error: failed to locate id array dimension in file id %d",
            exoid);
      ex_err("ex_id_lkup",errmsg,exerrval);
      return (EX_FATAL);
    }


    /* Next get value of dimension */ 
    if ((status = nc_inq_dimlen(exoid,dimid,&dim_len)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
           "Error: failed to locate %s array length in file id %d",
            id_table,exoid);
      ex_err("ex_id_lkup",errmsg,exerrval);
      return (EX_FATAL);
    }

    /* get variable id of id array */
    if ((status = nc_inq_varid (exoid, id_table, &varid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
             "Error: failed to locate %s array in file id %d",
              id_table, exoid);
      ex_err("ex_id_lkup",errmsg,exerrval);
      return (EX_FATAL);
    }

    /* allocate space for id array */
    if (!(id_vals = malloc((int)dim_len*sizeof(int)))) {
      exerrval = EX_MEMFAIL;
      sprintf(errmsg,
             "Error: failed to allocate memory for %s array for file id %d",
              id_table,exoid);
      ex_err("ex_id_lkup",errmsg,exerrval);
      return (EX_FATAL);
    }

    if ((status = nc_get_var_int (exoid, varid, id_vals)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
             "Error: failed to get %s array from file id %d",
              id_table,exoid);
      ex_err("ex_id_lkup",errmsg,exerrval);
      free(id_vals);
      return (EX_FATAL);
    }

    /* check if values in stored arrays are filled with non-zeroes */
    filled = TRUE;
    for (i=0;i<dim_len;i++) {
      if (id_vals[i] == 0 || id_vals[i] == NC_FILL_INT) {
        filled = FALSE;
        break; /* id array hasn't been completely filled with valid ids yet */
      }
    }

    if (filled) {
      tmp_stats->valid_ids = TRUE;
      tmp_stats->num = dim_len;
      tmp_stats->id_vals = id_vals;
    }

  } else {
    id_vals = tmp_stats->id_vals;
    dim_len = tmp_stats->num;
  }

  /* Do a linear search through the id array to find the array value
     corresponding to the passed index number */

  for (i=0;i<dim_len;i++) {
    if (id_vals[i] == num)
      break; /* found the id requested */
  }
  if (i >= dim_len) /* failed to find id number */
  {
    if ( !(tmp_stats->valid_ids) ) {
       free (id_vals);
    }
    exerrval = EX_LOOKUPFAIL;
    return(EX_LOOKUPFAIL); /*if we got here, the id array value doesn't exist */
  }
  
  /* Now check status array to see if object is null */

  /* get variable id of status array */
  if (nc_inq_varid (exoid, stat_table, &varid) == NC_NOERR) {
    /* if status array exists, use it, otherwise assume object exists 
       to be backward compatible */

    if ( (tmp_stats->stat_vals == NULL) || (!(tmp_stats->valid_stat)) ) {
      /* first time thru or status arrays haven't been filled yet */ 

      /* allocate space for new status array */

      if (!(stat_vals = malloc((int)dim_len*sizeof(int)))) {
        exerrval = EX_MEMFAIL;
        sprintf(errmsg,
                 "Error: failed to allocate memory for %s array for file id %d",
                  id_table,exoid);
        ex_err("ex_id_lkup",errmsg,exerrval);
        return (EX_FATAL);
      }

      if ((status = nc_get_var_int (exoid, varid, stat_vals)) != NC_NOERR) {
        exerrval = status;
        free(stat_vals);
        sprintf(errmsg,
               "Error: failed to get %s array from file id %d",
                stat_table,exoid);
        ex_err("ex_id_lkup",errmsg,exerrval);
        return (EX_FATAL);
      }

      if (tmp_stats->valid_ids) {  
        /* status array is valid only if ids are valid */
        tmp_stats->valid_stat = TRUE;
        tmp_stats->stat_vals = stat_vals;
      }

    } else {
      stat_vals = tmp_stats->stat_vals;
    }

    if (stat_vals[i] == 0) /* is this object null? */ {
      exerrval =  EX_NULLENTITY;
      if ( !(tmp_stats->valid_stat) ) {
        free (stat_vals);
      }
      if ( !(tmp_stats->valid_ids) ) {
        if (id_vals) free (id_vals); 
      }
      return(-(((int)i)+1)); /* return index into id array (1-based) */
    }
  }
  if ( !(tmp_stats->valid_ids) ) {
    if (id_vals) free (id_vals);
    if (stat_vals) free (stat_vals);
  }
  return(i+1); /* return index into id array (1-based) */
}

/******************************************************************************
*
* ex_get_stat_ptr - returns a pointer to a structure of object ids
*
* revision history - 
*
*
*****************************************************************************/

/*! this routine returns a pointer to a structure containing the ids of 
 * element blocks, node sets, or side sets according to exoid;  if there
 * is not a structure that matches the exoid, one is created
 */

struct obj_stats *ex_get_stat_ptr (int exoid, struct obj_stats **obj_ptr)

{
  struct obj_stats *tmp_ptr;

  tmp_ptr = *obj_ptr;

  while (tmp_ptr) {
    if ( (tmp_ptr)->exoid == exoid) {
      break;
    }
    tmp_ptr = (tmp_ptr)->next;
  }

  if (!tmp_ptr) {    /* exoid not found */
    tmp_ptr = (struct obj_stats *) calloc (1, sizeof(struct obj_stats));
    tmp_ptr->exoid = exoid;
    tmp_ptr->next = *obj_ptr;
    tmp_ptr->id_vals = 0;
    tmp_ptr->stat_vals = 0;
    tmp_ptr->num = 0;
    tmp_ptr->valid_ids = 0;
    tmp_ptr->valid_stat = 0;
    *obj_ptr = tmp_ptr;
  }

  return (tmp_ptr);
}

/******************************************************************************
*
* ex_rm_stat_ptr - removes a pointer to a structure of object ids
*
* revision history - 
*
*
*****************************************************************************/

/*! this routine removes a pointer to a structure containing the ids of 
 * element blocks, node sets, or side sets according to exoid;  this
 * is necessary to clean up because netCDF reuses file ids;  should be
 * called from ex_close
 */

void ex_rm_stat_ptr (int exoid, struct obj_stats **obj_ptr)

{
  struct obj_stats *last_head_list_ptr, *tmp_ptr;

  tmp_ptr = *obj_ptr;
  last_head_list_ptr = *obj_ptr;        /* save last head pointer */

  while (tmp_ptr )                      /* Walk linked list of file ids/vals */
  {
    if (exoid == tmp_ptr->exoid )       /* linear search for exodus file id */
    {
      if (tmp_ptr == *obj_ptr)          /* Are we at the head of the list? */
        *obj_ptr = (*obj_ptr)->next;    /*   yes, reset ptr to head of list */
      else                              /*   no, remove this record from chain*/
        last_head_list_ptr->next=tmp_ptr->next;
      if (tmp_ptr->id_vals != NULL)
        free(tmp_ptr->id_vals);           /* free up memory */
      if (tmp_ptr->stat_vals != NULL)
        free(tmp_ptr->stat_vals);
      free(tmp_ptr);
      break;                            /* Quit if found */
    }
    last_head_list_ptr = tmp_ptr;       /* save last head pointer */
    tmp_ptr = tmp_ptr->next;            /* Loop back if not */
  }
}

/* structures to hold number of blocks of that type for each file id */ 
static struct list_item*  ed_ctr_list = 0; /* edge blocks */
static struct list_item*  fa_ctr_list = 0; /* face blocks */
static struct list_item*  eb_ctr_list = 0; /* element blocks */
/* structures to hold number of sets of that type for each file id */ 
static struct list_item*  ns_ctr_list = 0; /* node sets */
static struct list_item*  es_ctr_list = 0; /* edge sets */
static struct list_item*  fs_ctr_list = 0; /* face sets */
static struct list_item*  ss_ctr_list = 0; /* side sets */
static struct list_item* els_ctr_list = 0; /* element sets */
/* structures to hold number of maps of that type for each file id */ 
static struct list_item*  nm_ctr_list = 0; /* node maps */
static struct list_item* edm_ctr_list = 0; /* edge maps */
static struct list_item* fam_ctr_list = 0; /* face maps */
static struct list_item*  em_ctr_list = 0; /* element maps */

struct list_item** ex_get_counter_list(ex_entity_type obj_type)
{
  switch(obj_type) {
  case EX_ELEM_BLOCK:
    return &eb_ctr_list;
  case EX_NODE_SET:
    return &ns_ctr_list;
  case EX_SIDE_SET:
    return &ss_ctr_list;
  case EX_ELEM_MAP:
    return &em_ctr_list;
  case EX_NODE_MAP:
    return &nm_ctr_list;
  case EX_EDGE_BLOCK:
    return &ed_ctr_list;
  case EX_FACE_BLOCK:
    return &fa_ctr_list;
  case EX_EDGE_SET:
    return &es_ctr_list;
  case EX_FACE_SET:
    return &fs_ctr_list;
  case EX_ELEM_SET:
    return &els_ctr_list;
  case EX_EDGE_MAP:
    return &edm_ctr_list;
  case EX_FACE_MAP:
    return &fam_ctr_list;
  default:
    return (NULL);
  }
}

/******************************************************************************
*
* ex_inc_file_item - increment file item
*
*****************************************************************************/


/*! this routine sets up a structure to track and increment a counter for
 * each open exodus file.  it is designed to be used by the routines
 * ex_put_elem_block() and ex_put_set_param(),
 * to keep track of the number of element blocks, and each type of set,
 * respectively, for each open exodus II file.
 *
 * The list structure is used as follows:
 *
 *   ptr -----------> list item structure
 *                    -------------------
 *                    exodus file id
 *                    item value (int)
 *                    ptr to next (NULL if last)
 *
 *
 * NOTE: since netCDF reuses its file ids, and a user may open and close any
 *       number of files in one application, items must be taken out of the
 *       linked lists in each of the above routines.  these should be called
 *       after ncclose().
 */

int ex_inc_file_item( int exoid,                /* file id */
                      struct list_item **list_ptr)/* ptr to ptr to list_item */

{
  struct list_item* tlist_ptr;


  /* printf("[f] list: %ld, *list: %ld \n", list_ptr, *list_ptr); */
  tlist_ptr = *list_ptr;        /* use temp list ptr to walk linked list */

  while (tlist_ptr )                    /* Walk linked list of file ids/vals */
  {
    if (exoid == tlist_ptr->exo_id )    /* linear search for exodus file id */
      break;                            /* Quit if found */
    tlist_ptr = tlist_ptr->next;        /* Loop back if not */
  }

  if (!tlist_ptr )                      /* ptr NULL? */
  {                                     /*  yes, new file id */
    /* allocate space for new structure record */
    tlist_ptr = (struct list_item*) calloc(1,sizeof(struct list_item));
    tlist_ptr->exo_id = exoid;          /* insert file id */
    tlist_ptr->next = *list_ptr;        /* insert into head of list */
    *list_ptr = tlist_ptr;              /* fix up new head of list  */
  }

/*  printf("[f] tlist: %ld *tlist: %ld **tlist: %ld\n",
          tlist_ptr,*tlist_ptr,(*tlist_ptr)->value, (*tlist_ptr)->next); */

  
  return(tlist_ptr->value++);

}

/*****************************************************************************
*
* ex_get_file_item - increment file item
*
*****************************************************************************/


/*! this routine accesses a structure to track and increment a counter for
 * each open exodus file.  it is designed to be used by the routines
 * ex_put_elem_block(), and ex_put_set_param(),
 * to get the number of element blocks, or a type of set,
 * respectively, for an open exodus II file.
 *
 * The list structure is used as follows:
 *
 *   ptr -----------> list item structure
 *                    -------------------
 *                    exodus file id
 *                    item value (int)
 *                    ptr to next (NULL if last)
 *
 *
 * NOTE: since netCDF reuses its file ids, and a user may open and close any
 *       number of files in one application, items must be taken out of the
 *       linked lists in each of the above routines.  these should be called
 *       after ncclose().
 */

int ex_get_file_item( int exoid,                /* file id */
                      struct list_item **list_ptr)/* ptr to ptr to list_item */

{
  struct list_item* tlist_ptr;

  /* printf("[f] list: %ld, *list: %ld \n", list_ptr, *list_ptr); */
  tlist_ptr = *list_ptr;        /* use temp list ptr to walk linked list */


  while (tlist_ptr )                    /* Walk linked list of file ids/vals */
  {
    if (exoid == tlist_ptr->exo_id )    /* linear search for exodus file id */
      break;                            /* Quit if found */
    tlist_ptr = tlist_ptr->next;        /* Loop back if not */
  }

  if (!tlist_ptr )                      /* ptr NULL? */
  {                                     /*  yes, Error: file id not found*/
    return(-1);
  }

/*  printf("[f] list: %ld *list: %ld **list: %ld\n",
          list_ptr,*list_ptr,(*list_ptr)->value, (*list_ptr)->next); */

  return(tlist_ptr->value);
}

/*****************************************************************************
*
* ex_rm_file_item - remove file item
*
*****************************************************************************/


/*! this routine removes a structure to track and increment a counter for
 * each open exodus file.
 *
 * The list structure is used as follows:
 *
 *   ptr -----------> list item structure
 *                    -------------------
 *                    exodus file id
 *                    item value (int)
 *                    ptr to next (NULL if last)
 *
 *
 * NOTE: since netCDF reuses its file ids, and a user may open and close any
 *       number of files in one application, items must be taken out of the
 *       linked lists in each of the above routines.  these should be called
 *       after ncclose().
 */

void ex_rm_file_item( int exoid,                /* file id */
                      struct list_item **list_ptr)/* ptr to ptr to list_item */

{
  struct list_item *last_head_list_ptr, *tlist_ptr;

  /* printf("[f] list: %ld, *list: %ld \n", list_ptr, *list_ptr); */
  tlist_ptr = *list_ptr;
  last_head_list_ptr = *list_ptr; /* save last head pointer */
  /* printf("[f] last head list: %ld\n",last_head_list_ptr); */

  while (tlist_ptr )                    /* Walk linked list of file ids/vals */
  {
    if (exoid == tlist_ptr->exo_id )    /* linear search for exodus file id */
    {
      if (tlist_ptr == *list_ptr)       /* Are we at the head of the list? */
        *list_ptr = (*list_ptr)->next;  /*   yes, reset ptr to head of list */
      else                              /*   no, remove this record from chain*/
        last_head_list_ptr->next=tlist_ptr->next;
      free(tlist_ptr);                  /* free up memory */
      break;                            /* Quit if found */
    }
    last_head_list_ptr = tlist_ptr;     /* save last head pointer */
    tlist_ptr = tlist_ptr->next;        /* Loop back if not */
  }

/*  printf("[f] list: %ld *list: %ld **list: %ld\n",
          list_ptr,*list_ptr,(*list_ptr)->value, (*list_ptr)->next); */

}

/*****************************************************************************
*
* ex_get_num_props - get number of properties
*
* entry conditions -
*
* exit conditions -
*
* revision history -
*
*
*****************************************************************************/
int ex_get_num_props (int exoid, ex_entity_type obj_type)
{
  int cntr, varid;
  char var_name[MAX_VAR_NAME_LENGTH+1];
  char  errmsg[MAX_ERR_LENGTH];

  cntr = 0;

  /* loop until there is not a property variable defined; the name of */
  /* the variables begin with an increment of 1 ("xx_prop1") so use cntr+1 */

  while (TRUE)
    {
      switch (obj_type)
	{
	case EX_ELEM_BLOCK:
	  strcpy (var_name, VAR_EB_PROP(cntr+1));
	  break;
	case EX_EDGE_BLOCK:
	  strcpy (var_name, VAR_ED_PROP(cntr+1));
	  break;
	case EX_FACE_BLOCK:
	  strcpy (var_name, VAR_FA_PROP(cntr+1));
	  break;
	case EX_NODE_SET:
	  strcpy (var_name, VAR_NS_PROP(cntr+1));
	  break;
	case EX_EDGE_SET:
	  strcpy (var_name, VAR_ES_PROP(cntr+1));
	  break;
	case EX_FACE_SET:
	  strcpy (var_name, VAR_FS_PROP(cntr+1));
	  break;
	case EX_SIDE_SET:
	  strcpy (var_name, VAR_SS_PROP(cntr+1));
	  break;
	case EX_ELEM_SET:
	  strcpy (var_name, VAR_ELS_PROP(cntr+1));
	  break;
	case EX_ELEM_MAP:
	  strcpy (var_name, VAR_EM_PROP(cntr+1));
	  break;
	case EX_FACE_MAP:
	  strcpy (var_name, VAR_FAM_PROP(cntr+1));
	  break;
	case EX_EDGE_MAP:
	  strcpy (var_name, VAR_EDM_PROP(cntr+1));
	  break;
	case EX_NODE_MAP:
	  strcpy (var_name, VAR_NM_PROP(cntr+1));
	  break;
	default:
	  exerrval = EX_BADPARAM;
	  sprintf(errmsg, "Error: object type %d not supported; file id %d",
		  obj_type, exoid);
	  ex_err("ex_get_prop_names",errmsg,exerrval);
	  return(EX_FATAL);
	}

      if (nc_inq_varid (exoid, var_name, &varid) != NC_NOERR) {
	/*   no variable with this name; return cntr which is now the number of */
	/*   properties for this type of entity */
	return (cntr);
      }
      cntr++;
    }
}

int ex_get_cpu_ws(void)
{
  return(sizeof(float));
}


/* swap - interchange v[i] and v[j] */
static void ex_swap (int v[], int i, int j)
{
  int temp;

  temp = v[i];
  v[i] = v[j];
  v[j] = temp;
}

/*!
 * The following 'indexed qsort' routine is modified from Sedgewicks
 * algorithm It selects the pivot based on the median of the left,
 * right, and center values to try to avoid degenerate cases ocurring
 * when a single value is chosen.  It performs a quicksort on
 * intervals down to the EX_QSORT_CUTOFF size and then performs a final
 * insertion sort on the almost sorted final array.  Based on data in
 * Sedgewick, the EX_QSORT_CUTOFF value should be between 5 and 20.
 *
 * See Sedgewick for further details
 * Define DEBUG_QSORT at the top of this file and recompile to compile
 * in code that verifies that the array is sorted.
 */

#define EX_QSORT_CUTOFF 12

static int ex_int_median3(int v[], int iv[], int left, int right)
{
  int center;
  center = (left + right) / 2;

  if (v[iv[left]] > v[iv[center]])
    ex_swap(iv, left, center);
  if (v[iv[left]] > v[iv[right]])
    ex_swap(iv, left, right);
  if (v[iv[center]] > v[iv[right]])
    ex_swap(iv, center, right);

  ex_swap(iv, center, right-1);
  return iv[right-1];
}

static void ex_int_iqsort(int v[], int iv[], int left, int right)
{
  int pivot;
  int i, j;
  
  if (left + EX_QSORT_CUTOFF <= right) {
    pivot = ex_int_median3(v, iv, left, right);
    i = left;
    j = right - 1;

    for ( ; ; ) {
      while (v[iv[++i]] < v[pivot]);
      while (v[iv[--j]] > v[pivot]);
      if (i < j) {
        ex_swap(iv, i, j);
      } else {
        break;
      }
    }

    ex_swap(iv, i, right-1);
    ex_int_iqsort(v, iv, left, i-1);
    ex_int_iqsort(v, iv, i+1, right);
  }
}

static void ex_int_iisort(int v[], int iv[], int N)
{
  int i,j;
  int ndx = 0;
  int small;
  int tmp;
  
  small = v[iv[0]];
  for (i = 1; i < N; i++) {
    if (v[iv[i]] < small) {
      small = v[iv[i]];
      ndx = i;
    }
  }
  /* Put smallest value in slot 0 */
  ex_swap(iv, 0, ndx);

  for (i=1; i <N; i++) {
    tmp = iv[i];
    for (j=i; v[tmp] < v[iv[j-1]]; j--) {
      iv[j] = iv[j-1];
    }
    iv[j] = tmp;
  }
}

void ex_iqsort(int v[], int iv[], int N)
{
  ex_int_iqsort(v, iv, 0, N-1);
  ex_int_iisort(v, iv, N);

#if defined(DEBUG_QSORT)
  fprintf(stderr, "Checking sort of %d values\n", N+1);
  int i;
  for (i=1; i < N; i++) {
    assert(v[iv[i-1]] <= v[iv[i]]);
  }
#endif
}

/*!
 * Determine whether the new large model storage is being used in this
 * file, or old method. Basically, the difference is whether the
 * coordinates and nodal variables are stored in a blob (xyz
 * components together) or as a variable per component per
 * nodal_variable.
 */
int ex_large_model(int exoid)
{
  static int message_output = FALSE;
  if (exoid < 0) {
    /* If exoid not specified, then query is to see if user specified
     * the large model via an environment variable
     */
    char *option = getenv("EXODUS_LARGE_MODEL");
    if (option != NULL) {
      if (option[0] == 'n' || option[0] == 'N') {
	if (!message_output) {
	  fprintf(stderr,
		  "EXODUSII: Small model size selected via EXODUS_LARGE_MODEL environment variable\n");
	  message_output = TRUE;
	}
        return 0;
      } else {
	if (!message_output) {
	  fprintf(stderr,
		  "EXODUSII: Large model size selected via EXODUS_LARGE_MODEL environment variable\n");
	  message_output = TRUE;
	}
        return 1;
      }
    } else {
      return EXODUS_DEFAULT_SIZE; /* Specified in exodusII_int.h */
    }

  } else {
    /* See if the ATT_FILESIZE attribute is defined in the file */
    int file_size = 0; 
    if (nc_get_att_int(exoid, NC_GLOBAL, ATT_FILESIZE, &file_size) != NC_NOERR) {
      /* Variable not found; default is 0 */
      file_size = 0;
    }
    return file_size;
  }
}
  
int ex_get_dimension(int exoid, const char* DIMENSION, const char *label,
                     size_t *count, int *dimid, const char *routine)
{
  char errmsg[MAX_ERR_LENGTH];
  int status;
  
  *count = 0;
  *dimid = -1;
  
  if ((status = nc_inq_dimid(exoid, DIMENSION, dimid)) != NC_NOERR) {
    exerrval = status;
    if (routine != NULL) {
      if (status == NC_EBADDIM) {
        sprintf(errmsg,
                "Warning: no %s defined in file id %d",
                label, exoid);
        ex_err(routine, errmsg,exerrval);
        
      } else {
        sprintf(errmsg,
                "Error: failed to locate number of %s in file id %d",
                label, exoid);
        ex_err(routine,errmsg,exerrval);
      }
    }
    return status;
  }

  if ((status = nc_inq_dimlen (exoid, *dimid, count)) != NC_NOERR) {
    exerrval = status;
    if (routine != NULL) {
      sprintf(errmsg,
              "Error: failed to get number of %s in file id %d",
              label, exoid);
      ex_err(routine,errmsg,exerrval);
      return -1;
    }
  }
  return status;
}

/*! Calculate the number of words of storage required to store the
 * header information.  Total bytes can be obtained by multiplying
 * words by 4.  Size is slightly underestimated since it only
 * considers the bulk data storage...
 */
size_t ex_header_size(int exoid)
{
  const char *routine = NULL;
  int iows = 0;
  size_t ndim = 0;
  size_t num_nodes = 0;
  size_t num_elem = 0;
  size_t num_eblk = 0;
  size_t num_map  = 0;
  size_t num_nset = 0;
  size_t num_sset = 0;
  int mapid;
  int temp;
  
  size_t size = 0;
  /* Get word size (2 = 8-byte reals, 1 = 4-byte reals */
  
  if (nc_flt_code(exoid) == NC_DOUBLE) 
    iows = 2;
  else
    iows = 1;
  
  /* coordinates = (ndim * numnp)*iows + maps  */
  ex_get_dimension(exoid, DIM_NUM_DIM,   "dimension", &ndim,      &temp, routine);
  ex_get_dimension(exoid, DIM_NUM_NODES, "nodes",     &num_nodes, &temp, routine);
  size += iows * ndim * num_nodes;

  /* node maps */
  if (nc_inq_varid(exoid, VAR_NODE_NUM_MAP, &mapid) != -1)
    size += num_nodes;

  ex_get_dimension(exoid, DIM_NUM_NM,   "node maps", &num_map, &temp, routine);
  size += num_map * num_nodes;

  /* Element Data */
  ex_get_dimension(exoid, DIM_NUM_ELEM, "elements",  &num_elem, &temp, routine);
  
  /* Element order map */
  if (nc_inq_varid (exoid, VAR_MAP, &mapid) != -1)
    size += num_elem;
   
  if (nc_inq_varid (exoid, VAR_ELEM_NUM_MAP, &mapid) != -1)
    size += num_elem;

  /* Element map(s) */
  ex_get_dimension(exoid, DIM_NUM_EM,     "element maps",   &num_map, &temp, routine);
  size += num_map * num_elem;

  /* Element Blocks... */
  ex_get_dimension(exoid, DIM_NUM_EL_BLK, "element blocks", &num_eblk, &temp, routine);
  if (num_eblk > 0) {
    /* Allocate storage for element block parameters... */
    int *ids = malloc(num_eblk * sizeof(int));
    size_t i;

    size += 2*num_eblk; /* status + ids */
    
    ex_get_ids(exoid, EX_ELEM_BLOCK, ids);
    for (i=0; i < num_eblk; i++) {
      int num_elem_this_blk = 0;
      int num_nodes_per_elem = 0;
      int num_attr = 0;
      char elem_type[MAX_STR_LENGTH+1];
      ex_get_elem_block(exoid, ids[i], elem_type, &num_elem_this_blk,
                        &num_nodes_per_elem, &num_attr);
      size += num_elem_this_blk * num_nodes_per_elem;
      size += num_elem_this_blk * num_attr * iows;
    }
    free(ids);
  }
  
  /* Nodesets */
  ex_get_dimension(exoid, DIM_NUM_NS, "nodesets", &num_nset, &temp, routine);
  if (num_nset > 0) {
    /* Allocate storage for nodeset parameters... */
    int *ids = malloc(num_nset * sizeof(int));
    size_t i;

    size += 2*num_nset; /* Status + ids */
    ex_get_ids(exoid, EX_NODE_SET, ids);
    for (i=0; i < num_nset; i++) {
      int num_nodes_in_set = 0;
      int num_df_in_set = 0;
      ex_get_node_set_param(exoid, ids[i], &num_nodes_in_set, &num_df_in_set);
      size += num_nodes_in_set;
      size += num_df_in_set * iows;
    }
    free(ids);
  }

  /* Sidesets */
  ex_get_dimension(exoid, DIM_NUM_SS, "sidesets", &num_sset, &temp, routine);
  if (num_sset > 0) {
    /* Allocate storage for sideset parameters... */
    int *ids = malloc(num_sset * sizeof(int));
    size_t i;

    size += 2*num_sset; /* Status + ids */
    ex_get_ids(exoid, EX_SIDE_SET, ids);
    for (i=0; i < num_sset; i++) {
      int num_sides_in_set = 0;
      int num_df_in_set = 0;
      ex_get_side_set_param(exoid, ids[i], &num_sides_in_set, &num_df_in_set);
      size += num_sides_in_set * 2;
      size += num_df_in_set * iows;
    }
    free(ids);
  }

  if (ex_large_model(exoid) == 0 && size > (1<<29)) {

    fprintf(stderr, "ERROR: Size to store header information exceeds 2GB in file id %d\n       File is probably corrupt, rerun with environment variable EXODUS_LARGE_MODEL set.\n", exoid);
  }
  return size;
}
