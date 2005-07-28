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
* exutils - utility routines
*
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          James A. Schutt - 8 byte float and standard C definitions
*          Vic Yarberry    - Added headers and error logging
*          
* environment - UNIX
*
* entry conditions - 
*
* exit conditions - 
*
* revision history - 
*
*  Id
*****************************************************************************/

#if defined(DEBUG_QSORT)
#include <assert.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "exodusII.h"
#include "exodusII_int.h"

struct obj_stats* eb = 0;
struct obj_stats* ns = 0;
struct obj_stats* ss = 0;
struct obj_stats* em = 0;
struct obj_stats* nm = 0;

/*****************************************************************************
*
* utility routines for string conversions
* ex_catstr  - concatenate  string/number (where number is converted to ASCII)
* ex_catstr2 - concatenate  string1/number1/string2/number2   "
*
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          James A. Schutt - 8 byte float and standard C definitions
*          Vic Yarberry    - Added headers and error logging
*
* environment - UNIX
*
* entry conditions -
*
* exit conditions -
*
* revision history -
*
* NOTE: these routines reuse the same storage over and over to build
*        concatenated strings, because the strings are just passed to netCDF
*        routines as names used to look up variables.  if the strings returned
*        by these routines are needed for any other purpose, they should
*        immediately be copied into other storage.
*****************************************************************************/

char ret_string[MAX_VAR_NAME_LENGTH+1];

char *ex_catstr (const char *string,
                 int   num)
{
   sprintf (ret_string, "%s%d", string, num);
   return (ret_string);

}


char *ex_catstr2 (const char *string1,
                  int   num1,
                  const char *string2,
                  int   num2)
{
   sprintf (ret_string, "%s%d%s%d", string1, num1, string2, num2);
   return (ret_string);

}

/*****************************************************************************
*
* ex_id_lkup - look up id
*
* author - Sandia National Laboratories
*          Vic Yarberry    - Original
*
*          
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       char*   id_type                 id type name:
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
                char *id_type,
                int num)
{

  char id_table[MAX_VAR_NAME_LENGTH+1];
  char id_dim[MAX_VAR_NAME_LENGTH+1];
  char stat_table[MAX_VAR_NAME_LENGTH+1];
  char idtyp[MAX_VAR_NAME_LENGTH+1];
  int varid, dimid, i;
  long dim_len, start[2], count[2];
  nclong *id_vals=NULL, *stat_vals=NULL;

  static int filled=FALSE;
  struct obj_stats *tmp_stats;

   char errmsg[MAX_ERR_LENGTH];

  exerrval  = 0; /* clear error code */

  strcpy(idtyp,id_type);

  if (strcmp(idtyp,VAR_ID_EL_BLK) == 0)
  {
    strcpy(id_table, VAR_ID_EL_BLK);            /* id array name */
    strcpy(id_dim, DIM_NUM_EL_BLK);             /* id array dimension name*/
    strcpy(stat_table, VAR_STAT_EL_BLK);        /* id status array name */
    tmp_stats = get_stat_ptr (exoid, &eb);
  }
  else if (strcmp(idtyp,VAR_NS_IDS) == 0)
  {
    strcpy(id_table, VAR_NS_IDS);
    strcpy(id_dim, DIM_NUM_NS);
    strcpy(stat_table, VAR_NS_STAT);
    tmp_stats = get_stat_ptr (exoid, &ns);
  }
  else if (strcmp(idtyp,VAR_SS_IDS) == 0)
  {
    strcpy(id_table, VAR_SS_IDS);
    strcpy(id_dim, DIM_NUM_SS);
    strcpy(stat_table, VAR_SS_STAT);
    tmp_stats = get_stat_ptr (exoid, &ss);
  }
  else if (strcmp(idtyp,VAR_EM_PROP(1)) == 0)
  {
    strcpy(id_table, VAR_EM_PROP(1));
    strcpy(id_dim, DIM_NUM_EM);
    strcpy(stat_table, "");
    tmp_stats = get_stat_ptr (exoid, &em);
  }
  else if (strcmp(idtyp,VAR_NM_PROP(1)) == 0)
  {
    strcpy(id_table, VAR_NM_PROP(1));
    strcpy(id_dim, DIM_NUM_NM);
    strcpy(stat_table, "");
    tmp_stats = get_stat_ptr (exoid, &nm);
  }
  else
  {
    exerrval = EX_BADPARAM;
    sprintf(errmsg,
           "Error: unsupported id array type %s for file id %d",
            idtyp, exoid);
    ex_err("ex_id_lkup",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ( (tmp_stats->id_vals == NULL) || (!(tmp_stats->valid_ids)) ) {

    /* first time thru or id arrays haven't been completely filled yet */ 

    /* get size of id array */

    /* First get dimension id of id array */
    if ((dimid = ncdimid(exoid,id_dim)) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
           "Error: failed to locate id array dimension in file id %d",
            exoid);
      ex_err("ex_id_lkup",errmsg,exerrval);
      return (EX_FATAL);
    }


    /* Next get value of dimension */ 
    if (ncdiminq (exoid,dimid,(char *) 0,&dim_len) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
           "Error: failed to locate %s array length in file id %d",
            id_table,exoid);
      ex_err("ex_id_lkup",errmsg,exerrval);
      return (EX_FATAL);
    }

    /* get variable id of id array */
    if ((varid = ncvarid (exoid, id_table)) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
             "Error: failed to locate %s array in file id %d",
              id_table, exoid);
      ex_err("ex_id_lkup",errmsg,exerrval);
      return (EX_FATAL);
    }

    /* allocate space for id array */
    if (!(id_vals = malloc((int)dim_len*sizeof(nclong))))
    {
      exerrval = EX_MEMFAIL;
      sprintf(errmsg,
             "Error: failed to allocate memory for %s array for file id %d",
              id_table,exoid);
      ex_err("ex_id_lkup",errmsg,exerrval);
      return (EX_FATAL);
    }

    start[0] = 0;
    start[1] = 0;
    count[0] = dim_len;
    count[1] = 0;


    if (ncvarget (exoid, varid, start, count, (void *)id_vals) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
             "Error: failed to get %s array from file id %d",
              id_table,exoid);
      ex_err("ex_id_lkup",errmsg,exerrval);
      free(id_vals);
      return (EX_FATAL);
    }

    /* check if values in stored arrays are filled with non-zeroes */
    filled = TRUE;
    for (i=0;i<dim_len;i++)
    {
      if (id_vals[i] == 0) {
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

  for (i=0;i<dim_len;i++)
  {
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
  if ((varid = ncvarid (exoid, stat_table)) != -1)
  {
    /* if status array exists, use it, otherwise assume object exists 
       to be backward compatible */

    if ( (tmp_stats->stat_vals == NULL) || (!(tmp_stats->valid_stat)) ) {
      /* first time thru or status arrays haven't been filled yet */ 

      /* allocate space for new status array */

      if (!(stat_vals = malloc((int)dim_len*sizeof(nclong))))
      {
        exerrval = EX_MEMFAIL;
        sprintf(errmsg,
                 "Error: failed to allocate memory for %s array for file id %d",
                  id_table,exoid);
        ex_err("ex_id_lkup",errmsg,exerrval);
        return (EX_FATAL);
      }

      start[0] = 0;
      start[1] = 0;
      count[0] = dim_len;
      count[1] = 0;

      if (ncvarget (exoid, varid, start, count, (void *)stat_vals) == -1)
      {
        exerrval = ncerr;
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

    if (stat_vals[i] == 0) /* is this object null? */
    {
      exerrval =  EX_NULLENTITY;
      if ( !(tmp_stats->valid_stat) ) {
        free (stat_vals);
      }
      return(-(i+1)); /* return index into id array (1-based) */
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
* get_stat_ptr - returns a pointer to a structure of object ids
*
* author - Sandia National Laboratories
*          Larry Schoof
*
*          
* environment - UNIX
*
* revision history - 
*
*
*****************************************************************************/

/* this routine returns a pointer to a structure containing the ids of 
 * element blocks, node sets, or side sets according to exoid;  if there
 * is not a structure that matches the exoid, one is created
 */

struct obj_stats *get_stat_ptr (int exoid, struct obj_stats **obj_ptr)

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
    *obj_ptr = tmp_ptr;
  }

  return (tmp_ptr);
}

/******************************************************************************
*
* rm_stat_ptr - removes a pointer to a structure of object ids
*
* author - Sandia National Laboratories
*          Larry Schoof
*
*          
* environment - UNIX
*
* revision history - 
*
*
*****************************************************************************/

/* this routine removes a pointer to a structure containing the ids of 
 * element blocks, node sets, or side sets according to exoid;  this
 * is necessary to clean up because netCDF reuses file ids;  should be
 * called from ex_close
 */

void rm_stat_ptr (int exoid, struct obj_stats **obj_ptr)

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
/******************************************************************************
*
* ex_inc_file_item - increment file item
*
* author - Sandia National Laboratories
*          Vic Yarberry    
*
*          
* environment - UNIX
*
* revision history - 
*
*
*****************************************************************************/


/* this routine sets up a structure to track and increment a counter for
 * each open exodus file.  it is designed to be used by the routines
 * ex_put_elem_block(), ex_put_node_set_param(), and ex_put_side_set_param(),
 * to keep track of the number of element blocks, node sets, or side sets,
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
* author - Sandia National Laboratories
*          Vic Yarberry    
*
*          
* environment - UNIX
*
* revision history - 
*
*
*****************************************************************************/


/* this routine accesses a structure to track and increment a counter for
 * each open exodus file.  it is designed to be used by the routines
 * ex_put_elem_block(), ex_put_node_set_param(), and ex_put_side_set_param(),
 * to get the number of element blocks, node sets, or side sets,
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
* author - Sandia National Laboratories
*          Vic Yarberry    
*
*          
* environment - UNIX
*
* revision history - 
*
*
*****************************************************************************/


/* this routine removes a structure to track and increment a counter for
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
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*
* environment - UNIX
*
* entry conditions -
*
* exit conditions -
*
* revision history -
*
*
*****************************************************************************/
int ex_get_num_props (int exoid, int obj_type)
{
   int cntr;
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
       case EX_NODE_SET:
         strcpy (var_name, VAR_NS_PROP(cntr+1));
         break;
       case EX_SIDE_SET:
         strcpy (var_name, VAR_SS_PROP(cntr+1));
         break;
       case EX_ELEM_MAP:
         strcpy (var_name, VAR_EM_PROP(cntr+1));
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

     if ((ncvarid (exoid, var_name)) == -1)
     {

/*   no variable with this name; return cntr which is now the number of */
/*   properties for this type of entity */

       return (cntr);
     }
     cntr++;
   }
}
int ex_get_cpu_ws()
{
  return(sizeof(float));
}


/* swap - interchange v[i] and v[j] */
void ex_swap (int v[], int i, int j)
{
  int temp;

  temp = v[i];
  v[i] = v[j];
  v[j] = temp;
}

/*
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

int ex_int_median3(int v[], int iv[], int left, int right)
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

void ex_int_iqsort(int v[], int iv[], int left, int right)
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

void ex_int_iisort(int v[], int iv[], int N)
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

/*
 * Convert array of ints to packed array of "nclongs", in malloc'ed space.  
 * Returns pointer to "nclongs" or NULL if malloc failed.
 */
nclong* itol(const int *ints,              /* array of ints */
             int len)                /* length of array */
{
  nclong      *longs = malloc (len * sizeof (nclong));
  const int   *ip;
  nclong      *lp = longs;

  if (longs != NULL) {
    for (ip = ints; len > 0; len--) {
      *lp++ = (nclong) *ip++;
    }
  } else {
    char errmsg[MAX_ERR_LENGTH];
    exerrval = EX_MEMFAIL;
    sprintf(errmsg,
       "Error: failed to allocate memory for integer to long conversion");
    ex_err("ex_get_side_set_node_list",errmsg,exerrval);
  }
  return longs;
}

/*
 * Convert array of "nclongs" to array of ints
 */
int ltoi (const nclong *longs,   /* array of longs */
          int  *ints,      /* array of ints  */
          int  len)        /* length of array */
{
   int *ip;
   const nclong *lp;

   for (ip = ints, lp = longs; len > 0; len--)
      *ip++ = *lp++;

   return (0);
}

/*
 * Determine whether the new large model storage is being used in this
 * file, or old method. Basically, the difference is whether the
 * coordinates and nodal variables are stored in a blob (xyz
 * components together) or as a variable per component per
 * nodal_variable.
 */
int ex_large_model(int exoid)
{
  if (exoid < 0) {
    /* If exoid not specified, then query is to see if user specified
     * the large model via an environment variable
     */
    char *option = getenv("EXODUS_LARGE_MODEL");
    if (option != NULL) {
      fprintf(stderr, "EXODUSII: Large model size selected via EXODUS_LARGE_MODEL environment variable\n");
      return 1;
    } else {
      return 0;
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
  
