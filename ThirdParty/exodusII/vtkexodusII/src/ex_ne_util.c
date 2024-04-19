/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* Function(s) contained in this file:
 *
 *     ex__leavedef()
 *     ne__id_lkup()
 *     ex__get_file_type()
 *     ex__put_nemesis_version()
 *     ne__check_file_version()
 *     ex_get_idx()
 *
 *****************************************************************************
 * Much of this code is a modified version of what is found in NemesisI.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#include <exodusII.h>     // for ex_err, etc
#include <exodusII_int.h> // for EX_FATAL, EX_NOERR, etc

/* Global variables */
char *ne_ret_string;

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* Note: This function assumes a 1-d vector of data for "ne_var_name".
 */
/*****************************************************************************/
int ne__id_lkup(int exoid, const char *ne_var_name, int64_t *idx, ex_entity_id ne_var_id)
{
  int       status;
  int       varid, ndims, dimid[1], ret = -1;
  nc_type   var_type;
  size_t    length, start[1];
  int64_t   my_index, begin, end;
  long long id_val;

  char errmsg[MAX_ERR_LENGTH];

  if ((status = nc_inq_varid(exoid, ne_var_name, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
             ne_var_name, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    return (EX_FATAL);
  }

  /* check if I need the length for this variable */
  if (idx[1] == -1) {
    /* Get the dimension IDs for this variable */
    if ((status = nc_inq_var(exoid, varid, (char *)0, &var_type, &ndims, dimid, (int *)0)) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to find dimension ID for variable \"%s\" "
               "in file ID %d",
               ne_var_name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (EX_FATAL);
    }

    /* Get the length of this variable */
    if ((status = nc_inq_dimlen(exoid, dimid[0], &length)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to find dimension for variable \"%s\" in file ID %d", ne_var_name,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (EX_FATAL);
    }

    idx[1] = length;
  } /* End "if (idx[1] == -1)" */

  begin = idx[0];
  end   = idx[1];

  /* Find the index by looping over each entry */
  for (my_index = begin; my_index < end; my_index++) {
    start[0] = my_index;
    status   = nc_get_var1_longlong(exoid, varid, start, &id_val);

    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable \"%s\" in file ID %d",
               ne_var_name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (EX_FATAL);
    }

    if (id_val == ne_var_id) {
      ret = (int)my_index;
      break;
    }
  }
  return (ret);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* This function retrieves the file type from a Nemesis file.
 */
/*****************************************************************************/
int ex__get_file_type(int exoid, char *ftype)
{
  int status;
  int varid;
  int lftype;

  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();

  if ((status = nc_inq_varid(exoid, VAR_FILE_TYPE, &varid)) != NC_NOERR) {

    /* If no file type is found, assume parallel */
    ftype[0] = 'p';
    ftype[1] = '\0';

    EX_FUNC_LEAVE(EX_NOERR);
  }

  if ((status = nc_get_var1_int(exoid, varid, NULL, &lftype)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get variable \"%s\" from file ID %d",
             VAR_FILE_TYPE, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Set the appropriate character */
  if (lftype == 0) {
    ex_copy_string(ftype, "p", 2);
  }
  else if (lftype == 1) {
    ex_copy_string(ftype, "s", 2);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* This function outputs the Nemesis version information to the file.
 */
/*****************************************************************************/
int ex__put_nemesis_version(int exoid)
{
  int   status;
  float file_ver, api_ver;

  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();

  file_ver = NEMESIS_FILE_VERSION;
  api_ver  = NEMESIS_API_VERSION;

  /* Check to see if the nemesis file version is already in the file */
  if (nc_get_att_float(exoid, NC_GLOBAL, "nemesis_file_version", &file_ver) != NC_NOERR) {

    /* Output the Nemesis file version */
    if ((status = nc_put_att_float(exoid, NC_GLOBAL, ATT_NEM_FILE_VERSION, NC_FLOAT, 1,
                                   &file_ver)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output nemesis file version in file ID %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Output the Nemesis API version */
    if ((status = nc_put_att_float(exoid, NC_GLOBAL, ATT_NEM_API_VERSION, NC_FLOAT, 1, &api_ver)) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output nemesis api version in file ID %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* This function checks that the version info is correct.
 */
/*****************************************************************************/
int ne__check_file_version(int exoid)
{
#if 0
  float  file_ver;

  int    status;
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();

  /* Get the file version */
  if ((status = nc_get_att_float(exoid, NC_GLOBAL, "nemesis_file_version", &file_ver)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
            "ERROR: failed to get the nemesis file version from file ID %d",
            exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (fabs(NEMESIS_FILE_VERSION-file_ver) > 0.001) {
    snprintf(errmsg, MAX_ERR_LENGTH,
            "ERROR: Nemesis version mismatch in file ID %d!\n", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MSG);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
#else
  EX_UNUSED(exoid);
  return EX_NOERR;
#endif
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* This function gets the index for the given variable at the
 * position given.
 */
/*****************************************************************************/
int ex_get_idx(int exoid, const char *ne_var_name, int64_t *my_index, int pos)
{
  int    status;
  int    varid;
  size_t start[1], count[1];
#if NC_HAS_HDF5
  long long varidx[2];
#else
  int varidx[2];
#endif
  char errmsg[MAX_ERR_LENGTH];
  /*-----------------------------Execution begins-----------------------------*/
  EX_FUNC_ENTER();

  /* set default values for idx */
  my_index[0] = 0;
  my_index[1] = -1;

  /*
   * assume that if there is an error returned, that this
   * means that this is a parallel file, and the index does
   * not exists. This is not an error
   */
  if ((status = nc_inq_varid(exoid, ne_var_name, &varid)) == NC_NOERR) {
    /* check if we are at the beginning of the index vector */
    if (pos == 0) {
      start[0] = pos;
      count[0] = 1;
    }
    else {
      start[0] = pos - 1;
      count[0] = 2;
    }

#if NC_HAS_HDF5
    status = nc_get_vara_longlong(exoid, varid, start, count, varidx);
#else
    status = nc_get_vara_int(exoid, varid, start, count, varidx);
#endif
    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable \"%s\" in file ID %d",
               ne_var_name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(-1);
    }

    if (pos == 0) {
      my_index[0] = 0;
      my_index[1] = varidx[0];
    }
    else {
      my_index[0] = varidx[0];
      my_index[1] = varidx[1];
    }
  }
  EX_FUNC_LEAVE(1);
}
