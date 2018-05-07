/*
 * Copyright (c) 2005-2017 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
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
 *     * Neither the name of NTESS nor the names of its
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
 *****************************************************************************/

#if defined(DEBUG_QSORT)
#include <assert.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "exodusII.h"
#include "exodusII_int.h"

struct obj_stats *exoII_eb  = 0;
struct obj_stats *exoII_ed  = 0;
struct obj_stats *exoII_fa  = 0;
struct obj_stats *exoII_ns  = 0;
struct obj_stats *exoII_es  = 0;
struct obj_stats *exoII_fs  = 0;
struct obj_stats *exoII_ss  = 0;
struct obj_stats *exoII_els = 0;
struct obj_stats *exoII_em  = 0;
struct obj_stats *exoII_edm = 0;
struct obj_stats *exoII_fam = 0;
struct obj_stats *exoII_nm  = 0;

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

static char  ret_string[10 * (MAX_VAR_NAME_LENGTH + 1)];
static char *cur_string = &ret_string[0];

int ex_check_file_type(const char *path, int *type)
{
/* Based on (stolen from?) NC_check_file_type from netcdf sources.

Type is set to:
1 if this is a netcdf classic file,
2 if this is a netcdf 64-bit offset file,
4 pnetcdf cdf5 file.
5 if this is an hdf5 file
*/

#define MAGIC_NUMBER_LEN 4

  char magic[MAGIC_NUMBER_LEN];
  EX_FUNC_ENTER();

  *type = 0;

  /* Get the 4-byte magic from the beginning of the file. */
  {
    FILE *fp;
    int   i;

    if (!(fp = fopen(path, "r"))) {
      EX_FUNC_LEAVE(errno);
    }
    i = fread(magic, MAGIC_NUMBER_LEN, 1, fp);
    fclose(fp);
    if (i != 1) {
      EX_FUNC_LEAVE(errno);
    }
  }

  /* Ignore the first byte for HDF */
  if (magic[1] == 'H' && magic[2] == 'D' && magic[3] == 'F') {
    *type = 5;
  }
  else if (magic[0] == 'C' && magic[1] == 'D' && magic[2] == 'F') {
    if (magic[3] == '\001') {
      *type = 1;
    }
    else if (magic[3] == '\002') {
      *type = 2;
    }
    else if (magic[3] == '\005') {
      *type = 4; /* cdf5 (including pnetcdf) file */
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}

int ex_set_max_name_length(int exoid, int length)
{
  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  ex_check_valid_file_id(exoid, __func__);
  if (length <= 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Max name length must be positive.");
    ex_err(__func__, errmsg, NC_EMAXNAME);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (length > NC_MAX_NAME) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: Max name length (%d) exceeds netcdf max name size (%d).", length, NC_MAX_NAME);
    ex_err(__func__, errmsg, NC_EMAXNAME);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  ex_set_option(exoid, EX_OPT_MAX_NAME_LENGTH, length);

  EX_FUNC_LEAVE(EX_NOERR);
}

void ex_update_max_name_length(int exoid, int length)
{
  int status;
  int db_length = 0;
  int rootid    = exoid & EX_FILE_ID_MASK;

  EX_FUNC_ENTER();
  ex_check_valid_file_id(exoid, __func__);

  /* Get current value of the maximum_name_length attribute... */
  if ((status = nc_get_att_int(rootid, NC_GLOBAL, ATT_MAX_NAME_LENGTH, &db_length)) != NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to update 'max_name_length' attribute in file id %d", exoid);
    ex_err(__func__, errmsg, status);
  }

  if (length > db_length) {
    /* Update with new value... */
    ex_set_max_name_length(exoid, length);
    nc_put_att_int(rootid, NC_GLOBAL, ATT_MAX_NAME_LENGTH, NC_INT, 1, &length);
    nc_sync(rootid);
  }
  EX_FUNC_VOID();
}

int ex_put_names_internal(int exoid, int varid, size_t num_entity, char **names,
                          ex_entity_type obj_type, const char *subtype, const char *routine)
{
  size_t i;
  int    status;
  char   errmsg[MAX_ERR_LENGTH];
  int    max_name_len = 0;
  size_t name_length;
  size_t length;
  char * int_names  = NULL;
  size_t idx        = 0;
  int    found_name = 0;

  EX_FUNC_ENTER();
  ex_check_valid_file_id(exoid, __func__);
  /* inquire previously defined dimensions  */
  name_length = ex_inquire_int(exoid, EX_INQ_DB_MAX_ALLOWED_NAME_LENGTH) + 1;

  int_names = calloc(num_entity * name_length, 1);

  for (i = 0; i < num_entity; i++) {
    if (names != NULL && *names != NULL && *names[i] != '\0') {
      found_name = 1;
      strncpy(&int_names[idx], names[i], name_length - 1);
      int_names[idx + name_length - 1] = '\0';
      length                           = strlen(names[i]) + 1;
      if (length > name_length) {
        fprintf(stderr,
                "Warning: The %s %s name '%s' is too long.\n\tIt will "
                "be truncated from %d to %d characters\n",
                ex_name_of_object(obj_type), subtype, names[i], (int)length - 1,
                (int)name_length - 1);
        length = name_length;
      }

      if (length > max_name_len) {
        max_name_len = length;
      }
    }
    idx += name_length;
  }

  if ((status = nc_put_var_text(exoid, varid, int_names)) != NC_NOERR) {
    free(int_names);
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s names in file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err(__func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (found_name) {

    /* Update the maximum_name_length attribute on the file. */
    ex_update_max_name_length(exoid, max_name_len - 1);
  }
  free(int_names);

  EX_FUNC_LEAVE(EX_NOERR);
}

int ex_put_name_internal(int exoid, int varid, size_t index, const char *name,
                         ex_entity_type obj_type, const char *subtype, const char *routine)
{
  int    status;
  size_t start[2], count[2];
  char   errmsg[MAX_ERR_LENGTH];
  size_t name_length;

  ex_check_valid_file_id(exoid, __func__);

  /* inquire previously defined dimensions  */
  name_length = ex_inquire_int(exoid, EX_INQ_DB_MAX_ALLOWED_NAME_LENGTH) + 1;

  if (name != NULL && *name != '\0') {
    int too_long = 0;
    start[0]     = index;
    start[1]     = 0;

    count[0] = 1;
    count[1] = strlen(name) + 1;

    if (count[1] > name_length) {
      fprintf(stderr,
              "Warning: The %s %s name '%s' is too long.\n\tIt will be "
              "truncated from %d to %d characters\n",
              ex_name_of_object(obj_type), subtype, name, (int)strlen(name), (int)name_length - 1);
      count[1] = name_length;
      too_long = 1;
    }

    if ((status = nc_put_vara_text(exoid, varid, start, count, name)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s name in file id %d",
               ex_name_of_object(obj_type), exoid);
      ex_err(__func__, errmsg, status);
      return (EX_FATAL);
    }

    /* Add the trailing null if the variable name was too long */
    if (too_long) {
      start[1] = name_length - 1;
      nc_put_var1_text(exoid, varid, start, "\0");
    }

    /* Update the maximum_name_length attribute on the file. */
    ex_update_max_name_length(exoid, count[1] - 1);
  }
  return (EX_NOERR);
}

int ex_get_names_internal(int exoid, int varid, size_t num_entity, char **names,
                          ex_entity_type obj_type, const char *routine)
{
  size_t i;
  int    status;

  /* Query size of names on file
   * Use the smaller of the size on file or user-specified length
   */
  int db_name_size  = ex_inquire_int(exoid, EX_INQ_DB_MAX_ALLOWED_NAME_LENGTH);
  int api_name_size = ex_inquire_int(exoid, EX_INQ_MAX_READ_NAME_LENGTH);
  int name_size     = db_name_size < api_name_size ? db_name_size : api_name_size;

  for (i = 0; i < num_entity; i++) {
    status = ex_get_name_internal(exoid, varid, i, names[i], name_size, obj_type, routine);
    if (status != NC_NOERR) {
      return (status);
    }
  }
  return (EX_NOERR);
}

int ex_get_name_internal(int exoid, int varid, size_t index, char *name, int name_size,
                         ex_entity_type obj_type, const char *routine)
{
  size_t start[2], count[2];
  int    status;
  char   errmsg[MAX_ERR_LENGTH];
  int    api_name_size = 0;

  api_name_size = ex_inquire_int(exoid, EX_INQ_MAX_READ_NAME_LENGTH);

  /* read the name */
  start[0] = index;
  count[0] = 1;
  start[1] = 0;
  count[1] = name_size + 1;

  status = nc_get_vara_text(exoid, varid, start, count, name);
  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get %s name at index %d from file id %d",
             ex_name_of_object(obj_type), (int)index, exoid);
    ex_err(__func__, errmsg, status);
    return (EX_FATAL);
  }

  name[api_name_size] = '\0';

  ex_trim_internal(name);
  return (EX_NOERR);
}

void ex_trim_internal(char *name)
{
  /* Thread-safe, reentrant */
  /* Trim trailing spaces... */
  size_t size;
  char * end;

  if (name == NULL) {
    return;
  }

  size = strlen(name);
  if (size == 0) {
    return;
  }

  end = name + size - 1;
  while (end >= name && isspace(*end)) {
    end--;
  }

  *(end + 1) = '\0';
}

/** ex_catstr  - concatenate  string/number (where number is converted to ASCII)
 */
char *ex_catstr(const char *string, int num)
{
  /* Only called from an already locked function */
  char *tmp_string = cur_string;
  cur_string += sprintf(cur_string, "%s%d", string, num) + 1;
  if (cur_string - ret_string > 9 * (MAX_VAR_NAME_LENGTH + 1)) {
    cur_string = ret_string;
  }
  return (tmp_string);
}

/** ex_catstr2 - concatenate  string1num1string2num2   */
char *ex_catstr2(const char *string1, int num1, const char *string2, int num2)
{
  /* Only called from an already locked function */
  char *tmp_string = cur_string;
  cur_string += sprintf(cur_string, "%s%d%s%d", string1, num1, string2, num2) + 1;
  if (cur_string - ret_string > 9 * (MAX_VAR_NAME_LENGTH + 1)) {
    cur_string = ret_string;
  }
  return (tmp_string);
}

char *ex_name_of_object(ex_entity_type obj_type)
{
  /* Thread-safe and reentrant */
  switch (obj_type) {
  case EX_COORDINATE: /* kluge so some wrapper functions work */ return "coordinate";
  case EX_NODAL: return "nodal";
  case EX_EDGE_BLOCK: return "edge block";
  case EX_FACE_BLOCK: return "face block";
  case EX_ELEM_BLOCK: return "element block";
  case EX_NODE_SET: return "node set";
  case EX_EDGE_SET: return "edge set";
  case EX_FACE_SET: return "face set";
  case EX_SIDE_SET: return "side set";
  case EX_ELEM_SET: return "element set";
  case EX_ELEM_MAP: return "element map";
  case EX_NODE_MAP: return "node map";
  case EX_EDGE_MAP: return "edge map";
  case EX_FACE_MAP: return "face map";
  case EX_GLOBAL: return "global";
  default: return "invalid type";
  }
}

ex_entity_type ex_var_type_to_ex_entity_type(char var_type)
{
  /* Thread-safe and reentrant */
  char var_lower = tolower(var_type);
  if (var_lower == 'n') {
    return EX_NODAL;
  }
  if (var_lower == 'l') {
    return EX_EDGE_BLOCK;
  }
  if (var_lower == 'f') {
    return EX_FACE_BLOCK;
  }
  if (var_lower == 'e') {
    return EX_ELEM_BLOCK;
  }
  else if (var_lower == 'm') {
    return EX_NODE_SET;
  }
  else if (var_lower == 'd') {
    return EX_EDGE_SET;
  }
  else if (var_lower == 'a') {
    return EX_FACE_SET;
  }
  else if (var_lower == 's') {
    return EX_SIDE_SET;
  }
  else if (var_lower == 't') {
    return EX_ELEM_SET;
  }
  else if (var_lower == 'g') {
    return EX_GLOBAL;
  }
  else {
    return EX_INVALID;
  }
}

char *ex_dim_num_objects(ex_entity_type obj_type)
{
  switch (obj_type) {
  case EX_NODAL: return DIM_NUM_NODES;
  case EX_ELEM_BLOCK: return DIM_NUM_EL_BLK;
  case EX_EDGE_BLOCK: return DIM_NUM_ED_BLK;
  case EX_FACE_BLOCK: return DIM_NUM_FA_BLK;
  case EX_NODE_SET: return DIM_NUM_NS;
  case EX_EDGE_SET: return DIM_NUM_ES;
  case EX_FACE_SET: return DIM_NUM_FS;
  case EX_ELEM_SET: return DIM_NUM_ELS;
  case EX_SIDE_SET: return DIM_NUM_SS;
  case EX_ELEM_MAP: return DIM_NUM_EM;
  case EX_FACE_MAP: return DIM_NUM_FAM;
  case EX_EDGE_MAP: return DIM_NUM_EDM;
  case EX_NODE_MAP: return DIM_NUM_NM;
  default: {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: object type %d not supported in call to ex_dim_num_objects", obj_type);
    ex_err(__func__, errmsg, EX_BADPARAM);
    return (NULL);
  }
  }
}

char *ex_dim_num_entries_in_object(ex_entity_type obj_type, int idx)
{
  switch (obj_type) {
  case EX_NODAL: return DIM_NUM_NODES;
  case EX_EDGE_BLOCK: return DIM_NUM_ED_IN_EBLK(idx);
  case EX_FACE_BLOCK: return DIM_NUM_FA_IN_FBLK(idx);
  case EX_ELEM_BLOCK: return DIM_NUM_EL_IN_BLK(idx);
  case EX_NODE_SET: return DIM_NUM_NOD_NS(idx);
  case EX_EDGE_SET: return DIM_NUM_EDGE_ES(idx);
  case EX_FACE_SET: return DIM_NUM_FACE_FS(idx);
  case EX_SIDE_SET: return DIM_NUM_SIDE_SS(idx);
  case EX_ELEM_SET: return DIM_NUM_ELE_ELS(idx);
  default: return 0;
  }
}

char *ex_name_var_of_object(ex_entity_type obj_type, int i, int j)
{
  switch (obj_type) {
  case EX_EDGE_BLOCK: return VAR_EDGE_VAR(i, j);
  case EX_FACE_BLOCK: return VAR_FACE_VAR(i, j);
  case EX_ELEM_BLOCK: return VAR_ELEM_VAR(i, j);
  case EX_NODE_SET: return VAR_NS_VAR(i, j);
  case EX_EDGE_SET: return VAR_ES_VAR(i, j);
  case EX_FACE_SET: return VAR_FS_VAR(i, j);
  case EX_SIDE_SET: return VAR_SS_VAR(i, j);
  case EX_ELEM_SET: return VAR_ELS_VAR(i, j);
  default: return 0;
  }
}

char *ex_name_of_map(ex_entity_type map_type, int map_index)
{
  switch (map_type) {
  case EX_NODE_MAP: return VAR_NODE_MAP(map_index);
  case EX_EDGE_MAP: return VAR_EDGE_MAP(map_index);
  case EX_FACE_MAP: return VAR_FACE_MAP(map_index);
  case EX_ELEM_MAP: return VAR_ELEM_MAP(map_index);
  default: return 0;
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
2*                                         side_ss
*       int     num                     id value
*
* exit conditions -
*       int     return                  index into table (1-based)
*
*****************************************************************************/

int ex_id_lkup(int exoid, ex_entity_type id_type, ex_entity_id num)
{
  char *   id_table;
  char *   id_dim;
  char *   stat_table;
  int      varid, dimid;
  size_t   dim_len, i, j;
  int64_t *id_vals   = NULL;
  int *    stat_vals = NULL;

  static int        filled     = EX_FALSE;
  static int        sequential = EX_FALSE;
  struct obj_stats *tmp_stats;
  int               status;
  char              errmsg[MAX_ERR_LENGTH];

  switch (id_type) {
  case EX_NODAL: return (0);
  case EX_GLOBAL: return (0);
  case EX_ELEM_BLOCK:
    id_table   = VAR_ID_EL_BLK;   /* id array name */
    id_dim     = DIM_NUM_EL_BLK;  /* id array dimension name*/
    stat_table = VAR_STAT_EL_BLK; /* id status array name */
    tmp_stats  = ex_get_stat_ptr(exoid, &exoII_eb);
    break;
  case EX_NODE_SET:
    id_table   = VAR_NS_IDS;
    id_dim     = DIM_NUM_NS;
    stat_table = VAR_NS_STAT;
    tmp_stats  = ex_get_stat_ptr(exoid, &exoII_ns);
    break;
  case EX_SIDE_SET:
    id_table   = VAR_SS_IDS;
    id_dim     = DIM_NUM_SS;
    stat_table = VAR_SS_STAT;
    tmp_stats  = ex_get_stat_ptr(exoid, &exoII_ss);
    break;
  case EX_EDGE_BLOCK:
    id_table   = VAR_ID_ED_BLK;
    id_dim     = DIM_NUM_ED_BLK;
    stat_table = VAR_STAT_ED_BLK;
    tmp_stats  = ex_get_stat_ptr(exoid, &exoII_ed);
    break;
  case EX_FACE_BLOCK:
    id_table   = VAR_ID_FA_BLK;
    id_dim     = DIM_NUM_FA_BLK;
    stat_table = VAR_STAT_FA_BLK;
    tmp_stats  = ex_get_stat_ptr(exoid, &exoII_fa);
    break;
  case EX_EDGE_SET:
    id_table   = VAR_ES_IDS;
    id_dim     = DIM_NUM_ES;
    stat_table = VAR_ES_STAT;
    tmp_stats  = ex_get_stat_ptr(exoid, &exoII_es);
    break;
  case EX_FACE_SET:
    id_table   = VAR_FS_IDS;
    id_dim     = DIM_NUM_FS;
    stat_table = VAR_FS_STAT;
    tmp_stats  = ex_get_stat_ptr(exoid, &exoII_fs);
    break;
  case EX_ELEM_SET:
    id_table   = VAR_ELS_IDS;
    id_dim     = DIM_NUM_ELS;
    stat_table = VAR_ELS_STAT;
    tmp_stats  = ex_get_stat_ptr(exoid, &exoII_els);
    break;
  case EX_NODE_MAP:
    id_table   = VAR_NM_PROP(1);
    id_dim     = DIM_NUM_NM;
    stat_table = "";
    tmp_stats  = ex_get_stat_ptr(exoid, &exoII_nm);
    break;
  case EX_EDGE_MAP:
    id_table   = VAR_EDM_PROP(1);
    id_dim     = DIM_NUM_EDM;
    stat_table = "";
    tmp_stats  = ex_get_stat_ptr(exoid, &exoII_edm);
    break;
  case EX_FACE_MAP:
    id_table   = VAR_FAM_PROP(1);
    id_dim     = DIM_NUM_FAM;
    stat_table = "";
    tmp_stats  = ex_get_stat_ptr(exoid, &exoII_fam);
    break;
  case EX_ELEM_MAP:
    id_table   = VAR_EM_PROP(1);
    id_dim     = DIM_NUM_EM;
    stat_table = "";
    tmp_stats  = ex_get_stat_ptr(exoid, &exoII_em);
    break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unsupported id array type %d for file id %d", id_type,
             exoid);
    ex_err(__func__, errmsg, EX_BADPARAM);
    return (EX_FATAL);
  }

  if ((tmp_stats->id_vals == NULL) || (!(tmp_stats->valid_ids))) {

    /* first time through or id arrays haven't been completely filled yet */

    /* get size of id array */

    /* First get dimension id of id array */
    if ((status = nc_inq_dimid(exoid, id_dim, &dimid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate id array dimension in file id %d",
               exoid);
      ex_err(__func__, errmsg, status);
      return (EX_FATAL);
    }

    /* Next get value of dimension */
    if ((status = nc_inq_dimlen(exoid, dimid, &dim_len)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s array length in file id %d",
               id_table, exoid);
      ex_err(__func__, errmsg, status);
      return (EX_FATAL);
    }

    /* get variable id of id array */
    if ((status = nc_inq_varid(exoid, id_table, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s array in file id %d", id_table,
               exoid);
      ex_err(__func__, errmsg, status);
      return (EX_FATAL);
    }

    /* allocate space for id array and initialize to zero to ensure
       that the higher bits don't contain garbage while copy from ints */
    if (!(id_vals = calloc(dim_len, sizeof(int64_t)))) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to allocate memory for %s array for file id %d", id_table, exoid);
      ex_err(__func__, errmsg, EX_MEMFAIL);
      return (EX_FATAL);
    }

    if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
      status = nc_get_var_longlong(exoid, varid, (long long *)id_vals);
    }
    else {
      int *id_vals_int;
      if (!(id_vals_int = malloc(dim_len * sizeof(int)))) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to allocate memory for temporary array "
                 "id_vals_int for file id %d",
                 exoid);
        ex_err(__func__, errmsg, EX_MEMFAIL);
        free(id_vals);
        return (EX_FATAL);
      }
      status = nc_get_var_int(exoid, varid, id_vals_int);
      if (status == NC_NOERR) {
        for (i = 0; i < dim_len; i++) {
          id_vals[i] = (int64_t)id_vals_int[i];
        }
      }
      free(id_vals_int);
    }

    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get %s array from file id %d", id_table,
               exoid);
      ex_err(__func__, errmsg, status);
      free(id_vals);
      return (EX_FATAL);
    }

    /* check if values in stored arrays are filled with non-zeroes */
    filled     = EX_TRUE;
    sequential = EX_TRUE;
    for (i = 0; i < dim_len; i++) {
      if (id_vals[i] != i + 1) {
        sequential = EX_FALSE;
      }
      if (id_vals[i] == EX_INVALID_ID || id_vals[i] == NC_FILL_INT) {
        filled     = EX_FALSE;
        sequential = EX_FALSE;
        break; /* id array hasn't been completely filled with valid ids yet */
      }
    }

    if (filled) {
      tmp_stats->valid_ids  = EX_TRUE;
      tmp_stats->sequential = sequential;
      tmp_stats->num        = dim_len;
      tmp_stats->id_vals    = id_vals;
    }
  }
  else {
    id_vals    = tmp_stats->id_vals;
    dim_len    = tmp_stats->num;
    sequential = tmp_stats->sequential;
  }

  if (sequential && num < dim_len) {
    i = num - 1;
  }
  else {
    /* Do a linear search through the id array to find the array value
       corresponding to the passed index number */
    for (i = 0; i < dim_len; i++) {
      if (id_vals[i] == num) {
        break; /* found the id requested */
      }
    }
  }
  if (i >= dim_len) /* failed to find id number */
  {
    if (!(tmp_stats->valid_ids)) {
      free(id_vals);
    }
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate id %" PRId64 " for file id %d", num,
             exoid);
    ex_set_err(__func__, errmsg, EX_LOOKUPFAIL);
    return (-EX_LOOKUPFAIL); /*if we got here, the id array value doesn't exist */
  }

  /* Now check status array to see if object is null */
  if ((tmp_stats->stat_vals == NULL) || (!(tmp_stats->valid_stat))) {

    /* allocate space for new status array */
    if (!(stat_vals = malloc(dim_len * sizeof(int)))) {
      free(id_vals);
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to allocate memory for %s array for file id %d", id_table, exoid);
      ex_err(__func__, errmsg, EX_MEMFAIL);
      return (EX_FATAL);
    }

    /* first time through or status arrays haven't been filled yet */
    if (nc_inq_varid(exoid, stat_table, &varid) == NC_NOERR) {
      /* get variable id of status array */
      /* if status array exists, use it, otherwise assume object exists
         to be backward compatible */

      if ((status = nc_get_var_int(exoid, varid, stat_vals)) != NC_NOERR) {
        free(id_vals);
        free(stat_vals);
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get %s array from file id %d",
                 stat_table, exoid);
        ex_err(__func__, errmsg, status);
        return (EX_FATAL);
      }
    }
    else {
      for (j = 0; j < dim_len; j++) {
        stat_vals[j] = 1;
      }
    }

    if (tmp_stats->valid_ids) {
      /* status array is valid only if ids are valid */
      tmp_stats->valid_stat = EX_TRUE;
      tmp_stats->stat_vals  = stat_vals;
    }
  }
  else {
    stat_vals = tmp_stats->stat_vals;
  }

  if (stat_vals[i] == 0) /* is this object null? */ {
    ex_err(__func__, "", EX_NULLENTITY);
    if (!(tmp_stats->valid_stat)) {
      free(stat_vals);
    }
    if (!(tmp_stats->valid_ids)) {
      free(id_vals);
    }
    return (-((int)i + 1)); /* return index into id array (1-based) */
  }
  if (!(tmp_stats->valid_ids)) {
    free(id_vals);
    free(stat_vals);
  }
  return (i + 1); /* return index into id array (1-based) */
}

/******************************************************************************
 *
 * ex_get_stat_ptr - returns a pointer to a structure of object ids
 *
 *****************************************************************************/

/*! this routine returns a pointer to a structure containing the ids of
 * element blocks, node sets, or side sets according to exoid;  if there
 * is not a structure that matches the exoid, one is created
 */

struct obj_stats *ex_get_stat_ptr(int exoid, struct obj_stats **obj_ptr)
{
  struct obj_stats *tmp_ptr;

  tmp_ptr = *obj_ptr;

  while (tmp_ptr) {
    if ((tmp_ptr)->exoid == exoid) {
      break;
    }
    tmp_ptr = (tmp_ptr)->next;
  }

  if (!tmp_ptr) { /* exoid not found */
    tmp_ptr             = (struct obj_stats *)calloc(1, sizeof(struct obj_stats));
    tmp_ptr->exoid      = exoid;
    tmp_ptr->next       = *obj_ptr;
    tmp_ptr->id_vals    = 0;
    tmp_ptr->stat_vals  = 0;
    tmp_ptr->num        = 0;
    tmp_ptr->valid_ids  = 0;
    tmp_ptr->valid_stat = 0;
    *obj_ptr            = tmp_ptr;
  }
  return tmp_ptr;
}

/******************************************************************************
 *
 * ex_rm_stat_ptr - removes a pointer to a structure of object ids
 *
 *****************************************************************************/

/*! this routine removes a pointer to a structure containing the ids of
 * element blocks, node sets, or side sets according to exoid;  this
 * is necessary to clean up because netCDF reuses file ids;  should be
 * called from ex_close
 */

void ex_rm_stat_ptr(int exoid, struct obj_stats **obj_ptr)
{
  struct obj_stats *last_head_list_ptr, *tmp_ptr;

  tmp_ptr            = *obj_ptr;
  last_head_list_ptr = *obj_ptr; /* save last head pointer */

  while (tmp_ptr) /* Walk linked list of file ids/vals */
  {
    if (exoid == tmp_ptr->exoid) /* linear search for exodus file id */
    {
      if (tmp_ptr == *obj_ptr) {     /* Are we at the head of the list? */
        *obj_ptr = (*obj_ptr)->next; /*   yes, reset ptr to head of list */
      }
      else { /*   no, remove this record from chain*/
        last_head_list_ptr->next = tmp_ptr->next;
      }
      free(tmp_ptr->id_vals); /* free up memory */
      free(tmp_ptr->stat_vals);
      free(tmp_ptr);
      break; /* Quit if found */
    }
    last_head_list_ptr = tmp_ptr;       /* save last head pointer */
    tmp_ptr            = tmp_ptr->next; /* Loop back if not */
  }
}

/* structures to hold number of blocks of that type for each file id */
static struct list_item *ed_ctr_list = 0; /* edge blocks */
static struct list_item *fa_ctr_list = 0; /* face blocks */
static struct list_item *eb_ctr_list = 0; /* element blocks */
/* structures to hold number of sets of that type for each file id */
static struct list_item *ns_ctr_list  = 0; /* node sets */
static struct list_item *es_ctr_list  = 0; /* edge sets */
static struct list_item *fs_ctr_list  = 0; /* face sets */
static struct list_item *ss_ctr_list  = 0; /* side sets */
static struct list_item *els_ctr_list = 0; /* element sets */
/* structures to hold number of maps of that type for each file id */
static struct list_item *nm_ctr_list  = 0; /* node maps */
static struct list_item *edm_ctr_list = 0; /* edge maps */
static struct list_item *fam_ctr_list = 0; /* face maps */
static struct list_item *em_ctr_list  = 0; /* element maps */

struct list_item **ex_get_counter_list(ex_entity_type obj_type)
{
  /* Thread-safe, but is dealing with globals */
  /* Only called from a routine which will be using locks */
  switch (obj_type) {
  case EX_ELEM_BLOCK: return &eb_ctr_list;
  case EX_NODE_SET: return &ns_ctr_list;
  case EX_SIDE_SET: return &ss_ctr_list;
  case EX_ELEM_MAP: return &em_ctr_list;
  case EX_NODE_MAP: return &nm_ctr_list;
  case EX_EDGE_BLOCK: return &ed_ctr_list;
  case EX_FACE_BLOCK: return &fa_ctr_list;
  case EX_EDGE_SET: return &es_ctr_list;
  case EX_FACE_SET: return &fs_ctr_list;
  case EX_ELEM_SET: return &els_ctr_list;
  case EX_EDGE_MAP: return &edm_ctr_list;
  case EX_FACE_MAP: return &fam_ctr_list;
  default: return (NULL);
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

int ex_inc_file_item(int                exoid,    /* file id */
                     struct list_item **list_ptr) /* ptr to ptr to list_item */
{
  struct list_item *tlist_ptr = *list_ptr; /* use temp list ptr to walk linked list */
  while (tlist_ptr) {                      /* Walk linked list of file ids/vals */
    if (exoid == tlist_ptr->exo_id) {      /* linear search for exodus file id */
      break;                               /* Quit if found */
    }
    tlist_ptr = tlist_ptr->next; /* Loop back if not */
  }

  if (!tlist_ptr) { /* ptr NULL? */
    /* allocate space for new structure record */
    tlist_ptr         = (struct list_item *)calloc(1, sizeof(struct list_item));
    tlist_ptr->exo_id = exoid;     /* insert file id */
    tlist_ptr->next   = *list_ptr; /* insert into head of list */
    *list_ptr         = tlist_ptr; /* fix up new head of list  */
  }
  return (tlist_ptr->value++);
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
 *       after nc_close().
 */

int ex_get_file_item(int                exoid,    /* file id */
                     struct list_item **list_ptr) /* ptr to ptr to list_item */
{
  /* Not thread-safe: list_ptr passed in is a global
   * Would probably work ok with multiple threads since read-only,
   * but possible that list_ptr will be modified while being used
   */
  struct list_item *tlist_ptr = *list_ptr; /* use temp list ptr to walk linked list */
  while (tlist_ptr) {                      /* Walk linked list of file ids/vals */
    if (exoid == tlist_ptr->exo_id) {      /* linear search for exodus file id */
      break;                               /* Quit if found */
    }
    tlist_ptr = tlist_ptr->next; /* Loop back if not */
  }

  if (!tlist_ptr) { /* ptr NULL? */
    return (-1);
  }

  return (tlist_ptr->value);
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

void ex_rm_file_item(int                exoid,    /* file id */
                     struct list_item **list_ptr) /* ptr to ptr to list_item */

{
  struct list_item *last_head_list_ptr = *list_ptr; /* save last head pointer */

  struct list_item *tlist_ptr = *list_ptr;
  while (tlist_ptr) {                  /* Walk linked list of file ids/vals */
    if (exoid == tlist_ptr->exo_id) {  /* linear search for exodus file id */
      if (tlist_ptr == *list_ptr) {    /* Are we at the head of the list? */
        *list_ptr = (*list_ptr)->next; /*   yes, reset ptr to head of list */
      }
      else { /*   no, remove this record from chain*/
        last_head_list_ptr->next = tlist_ptr->next;
      }
      free(tlist_ptr); /* free up memory */
      break;           /* Quit if found */
    }
    last_head_list_ptr = tlist_ptr;       /* save last head pointer */
    tlist_ptr          = tlist_ptr->next; /* Loop back if not */
  }
}

/*****************************************************************************
 *
 * ex_get_num_props - get number of properties
 *
 *****************************************************************************/
int ex_get_num_props(int exoid, ex_entity_type obj_type)
{
  int   cntr, varid;
  char *var_name;
  char  errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  cntr = 0;

  /* loop until there is not a property variable defined; the name of */
  /* the variables begin with an increment of 1 ("xx_prop1") so use cntr+1 */
  while (EX_TRUE) {
    switch (obj_type) {
    case EX_ELEM_BLOCK: var_name = VAR_EB_PROP(cntr + 1); break;
    case EX_EDGE_BLOCK: var_name = VAR_ED_PROP(cntr + 1); break;
    case EX_FACE_BLOCK: var_name = VAR_FA_PROP(cntr + 1); break;
    case EX_NODE_SET: var_name = VAR_NS_PROP(cntr + 1); break;
    case EX_EDGE_SET: var_name = VAR_ES_PROP(cntr + 1); break;
    case EX_FACE_SET: var_name = VAR_FS_PROP(cntr + 1); break;
    case EX_SIDE_SET: var_name = VAR_SS_PROP(cntr + 1); break;
    case EX_ELEM_SET: var_name = VAR_ELS_PROP(cntr + 1); break;
    case EX_ELEM_MAP: var_name = VAR_EM_PROP(cntr + 1); break;
    case EX_FACE_MAP: var_name = VAR_FAM_PROP(cntr + 1); break;
    case EX_EDGE_MAP: var_name = VAR_EDM_PROP(cntr + 1); break;
    case EX_NODE_MAP: var_name = VAR_NM_PROP(cntr + 1); break;
    default:
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: object type %d not supported; file id %d", obj_type,
               exoid);
      ex_err(__func__, errmsg, EX_BADPARAM);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (nc_inq_varid(exoid, var_name, &varid) != NC_NOERR) {
      /*   no variable with this name; return cntr which is now the number of */
      /*   properties for this type of entity */
      EX_FUNC_LEAVE(cntr);
    }
    cntr++;
  }
}

int ex_get_cpu_ws(void) { return (sizeof(float)); }

/* swap - interchange v[i] and v[j] */
static void ex_swap(int v[], int64_t i, int64_t j)
{
  /* Thread-safe, reentrant */
  int temp;

  temp = v[i];
  v[i] = v[j];
  v[j] = temp;
}

static void ex_swap64(int64_t v[], int64_t i, int64_t j)
{
  /* Thread-safe, reentrant */
  int64_t temp;

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
   *
   * NOTE: The 'int' implementation below assumes that *both* the items
   *       being sorted and the *number* of items being sorted are both
   *       representable as 'int'.
   */

#define EX_QSORT_CUTOFF 12

static int ex_int_median3(int v[], int iv[], int64_t left, int64_t right)
{
  /* Thread-safe, reentrant */
  int64_t center;
  center = (left + right) / 2;

  if (v[iv[left]] > v[iv[center]]) {
    ex_swap(iv, left, center);
  }
  if (v[iv[left]] > v[iv[right]]) {
    ex_swap(iv, left, right);
  }
  if (v[iv[center]] > v[iv[right]]) {
    ex_swap(iv, center, right);
  }

  ex_swap(iv, center, right - 1);
  return iv[right - 1];
}

static int64_t ex_int_median3_64(int64_t v[], int64_t iv[], int64_t left, int64_t right)
{
  /* Thread-safe, reentrant */
  int64_t center;
  center = (left + right) / 2;

  if (v[iv[left]] > v[iv[center]]) {
    ex_swap64(iv, left, center);
  }
  if (v[iv[left]] > v[iv[right]]) {
    ex_swap64(iv, left, right);
  }
  if (v[iv[center]] > v[iv[right]]) {
    ex_swap64(iv, center, right);
  }

  ex_swap64(iv, center, right - 1);
  return iv[right - 1];
}

static void ex_int_iqsort(int v[], int iv[], int left, int right)
{
  /* Thread-safe, reentrant */
  int pivot;
  int i, j;

  if (left + EX_QSORT_CUTOFF <= right) {
    pivot = ex_int_median3(v, iv, left, right);
    i     = left;
    j     = right - 1;

    for (;;) {
      while (v[iv[++i]] < v[pivot]) {
        ;
      }
      while (v[iv[--j]] > v[pivot]) {
        ;
      }
      if (i < j) {
        ex_swap(iv, i, j);
      }
      else {
        break;
      }
    }

    ex_swap(iv, i, right - 1);
    ex_int_iqsort(v, iv, left, i - 1);
    ex_int_iqsort(v, iv, i + 1, right);
  }
}

static void ex_int_iqsort64(int64_t v[], int64_t iv[], int64_t left, int64_t right)
{
  /* Thread-safe, reentrant */
  int64_t pivot;
  int64_t i, j;

  if (left + EX_QSORT_CUTOFF <= right) {
    pivot = ex_int_median3_64(v, iv, left, right);
    i     = left;
    j     = right - 1;

    for (;;) {
      while (v[iv[++i]] < v[pivot]) {
        ;
      }
      while (v[iv[--j]] > v[pivot]) {
        ;
      }
      if (i < j) {
        ex_swap64(iv, i, j);
      }
      else {
        break;
      }
    }

    ex_swap64(iv, i, right - 1);
    ex_int_iqsort64(v, iv, left, i - 1);
    ex_int_iqsort64(v, iv, i + 1, right);
  }
}

static void ex_int_iisort(int v[], int iv[], int N)
{
  /* Thread-safe, reentrant */
  int i, j;
  int ndx = 0;
  int small;
  int tmp;

  small = v[iv[0]];
  for (i = 1; i < N; i++) {
    if (v[iv[i]] < small) {
      small = v[iv[i]];
      ndx   = i;
    }
  }
  /* Put smallest value in slot 0 */
  ex_swap(iv, 0, ndx);

  for (i = 1; i < N; i++) {
    tmp = iv[i];
    for (j = i; v[tmp] < v[iv[j - 1]]; j--) {
      iv[j] = iv[j - 1];
    }
    iv[j] = tmp;
  }
}

static void ex_int_iisort64(int64_t v[], int64_t iv[], int64_t N)
{
  /* Thread-safe, reentrant */
  int64_t i, j;
  int64_t ndx = 0;
  int64_t small;
  int64_t tmp;

  small = v[iv[0]];
  for (i = 1; i < N; i++) {
    if (v[iv[i]] < small) {
      small = v[iv[i]];
      ndx   = i;
    }
  }
  /* Put smallest value in slot 0 */
  ex_swap64(iv, 0, ndx);

  for (i = 1; i < N; i++) {
    tmp = iv[i];
    for (j = i; v[tmp] < v[iv[j - 1]]; j--) {
      iv[j] = iv[j - 1];
    }
    iv[j] = tmp;
  }
}

void ex_iqsort(int v[], int iv[], int N)
{
  /* Thread-safe, reentrant */
  ex_int_iqsort(v, iv, 0, N - 1);
  ex_int_iisort(v, iv, N);

#if defined(DEBUG_QSORT)
  fprintf(stderr, "Checking sort of %d values\n", N + 1);
  int i;
  for (i = 1; i < N; i++) {
    assert(v[iv[i - 1]] <= v[iv[i]]);
  }
#endif
}

void ex_iqsort64(int64_t v[], int64_t iv[], int64_t N)
{
  /* Thread-safe, reentrant */
  ex_int_iqsort64(v, iv, 0, N - 1);
  ex_int_iisort64(v, iv, N);

#if defined(DEBUG_QSORT)
  fprintf(stderr, "Checking sort of %d values\n", N + 1);
  int i;
  for (i = 1; i < N; i++) {
    assert(v[iv[i - 1]] <= v[iv[i]]);
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
  static int message_output = EX_FALSE;
  EX_FUNC_ENTER();
  if (exoid < 0) {
    /* If exoid not specified, then query is to see if user specified
     * the large model via an environment variable
     */
    char *option = getenv("EXODUS_LARGE_MODEL");
    if (option != NULL) {
      if (option[0] == 'n' || option[0] == 'N') {
        if (!message_output) {
          fprintf(stderr, "EXODUS: Small model size selected via "
                          "EXODUS_LARGE_MODEL environment variable\n");
          message_output = EX_TRUE;
        }
        EX_FUNC_LEAVE(0);
      }
      if (!message_output) {
        fprintf(stderr, "EXODUS: Large model size selected via "
                        "EXODUS_LARGE_MODEL environment variable\n");
        message_output = EX_TRUE;
      }
      EX_FUNC_LEAVE(1);
    }

    EX_FUNC_LEAVE(EXODUS_DEFAULT_SIZE); /* Specified in exodusII_int.h */
  }

  /* See if the ATT_FILESIZE attribute is defined in the file */
  int file_size = 0;
  int rootid    = exoid & EX_FILE_ID_MASK;
  if (nc_get_att_int(rootid, NC_GLOBAL, ATT_FILESIZE, &file_size) != NC_NOERR) {
    /* Variable not found; default is 0 */
    file_size = 0;
  }
  EX_FUNC_LEAVE(file_size);
}

int ex_get_dimension(int exoid, const char *DIMENSION, const char *label, size_t *count, int *dimid,
                     const char *routine)
{
  char errmsg[MAX_ERR_LENGTH];
  int  status;

  *count = 0;
  *dimid = -1;

  if ((status = nc_inq_dimid(exoid, DIMENSION, dimid)) != NC_NOERR) {
    if (routine != NULL) {
      if (status == NC_EBADDIM) {
        snprintf(errmsg, MAX_ERR_LENGTH, "Warning: no %s defined in file id %d", label, exoid);
        ex_err(__func__, errmsg, status);
      }
      else {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate number of %s in file id %d",
                 label, exoid);
        ex_err(__func__, errmsg, status);
      }
    }
    return (status);
  }

  if ((status = nc_inq_dimlen(exoid, *dimid, count)) != NC_NOERR) {
    if (routine != NULL) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of %s in file id %d", label,
               exoid);
      ex_err(__func__, errmsg, status);
      return -1;
    }
  }
  return (status);
}

/* Deprecated. do not use */
size_t ex_header_size(int exoid) { return 0; }

/* type = 1 for integer, 2 for real, 3 for character */
void ex_compress_variable(int exoid, int varid, int type)
{
#if NC_HAS_HDF5

  struct ex_file_item *file = ex_find_file_item(exoid);

  if (!file) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unknown file id %d for ex_compress_variable().",
             exoid);
    ex_err(__func__, errmsg, EX_BADFILEID);
  }
  else {
    int deflate_level = file->compression_level;
    int compress      = 1;
    int shuffle       = file->shuffle;
    if (!file->is_parallel && deflate_level > 0 && (file->file_type == 2 || file->file_type == 3)) {
      nc_def_var_deflate(exoid, varid, shuffle, compress, deflate_level);
    }
#if defined(PARALLEL_AWARE_EXODUS)
    if (type != 3 && file->is_parallel && file->is_mpiio) {
      nc_var_par_access(exoid, varid, NC_COLLECTIVE);
    }
#endif
  }
#endif
}
