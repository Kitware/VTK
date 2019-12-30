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
 * exutils - exodus utilities
 *
 * entry conditions -
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for ex__file_item, EX_FATAL, etc

/*! \file
 * this file contains code needed to support the various floating point word
 * size combinations for computation and i/o that applications might want to
 * use. See the netcdf documentation for more details on the floating point
 * conversion capabilities.
 *
 * netCDF supports two floating point word sizes for its files:
 *   - NC_FLOAT  - 32 bit IEEE
 *   - NC_DOUBLE - 64 bit IEEE
 *
 */

#define NC_FLOAT_WORDSIZE 4

static struct ex__file_item *file_list = NULL;

struct ex__file_item *ex__find_file_item(int exoid)
{
  /* Find base filename in case exoid refers to a group */
  int                   base_exoid = (unsigned)exoid & EX_FILE_ID_MASK;
  struct ex__file_item *ptr        = file_list;
  while (ptr) {
    if (ptr->file_id == base_exoid) {
      break;
    }
    ptr = ptr->next;
  }
  return (ptr);
}

void ex__check_valid_file_id(int exoid, const char *func)
{
  int error = 0;
  if (exoid <= 0) {
    error = 1;
  }
#if !defined BUILT_IN_SIERRA
  else {
    struct ex__file_item *file = ex__find_file_item(exoid);

    if (!file) {
      error = 1;
    }
  }
#endif

  if (error) {
    ex_opts(EX_ABORT | EX_VERBOSE);
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: In \"%s\", the file id %d was not obtained via a call "
             "to \"ex_open\" or \"ex_create\".\n\t\tIt does not refer to a "
             "valid open exodus file.\n\t\tAborting to avoid file "
             "corruption or data loss or other potential problems.",
             func, exoid);
    ex_err(__func__, errmsg, EX_BADFILEID);
  }
}

int ex__conv_init(int exoid, int *comp_wordsize, int *io_wordsize, int file_wordsize,
                  int int64_status, int is_parallel, int is_hdf5, int is_pnetcdf)
{
  char                  errmsg[MAX_ERR_LENGTH];
  struct ex__file_item *new_file;
  int                   filetype = 0;

  /*! ex__conv_init() initializes the floating point conversion process.
   *
   * \param exoid         an integer uniquely identifying the file of interest.
   *
   * \param comp_wordsize compute floating point word size in the user's code.
   *                      a zero value indicates that the user is requesting the
   *                      default float size for the machine. The appropriate
   *                      value is chosen and returned in comp_wordsize, and
   *                      used in subsequent conversions.  a valid but
   *                      inappropriate for this parameter cannot be detected.
   *
   * \param io_wordsize   the desired floating point word size for a netCDF
   *                      file. For an existing file, if this parameter doesn't
   *                      match the word size of data already stored in the file,
   *                      a fatal error is generated.  a value of 0 for an
   *                      existing file indicates that the word size of the file
   *                      was not known a priori, so use whatever is in the file.
   *                      a value of 0 for a new file means to use the default
   *                      size, an NC_FLOAT (4 bytes).  when a value of 0 is
   *                      specified the actual value used is returned in
   *                      io_wordsize.
   *
   * \param file_wordsize floating point word size in an existing netCDF file.
   *                      a value of 0 should be passed in for a new netCDF
   *                      file.
   *
   * \param int64_status  the flags specifying how integer values should be
   *                      stored on the database and how they should be
   *                      passes through the api functions.
   *
   * \param is_parallel   1 if parallel file; 0 if serial
   *
   * \param is_hdf5      1 if parallel netcdf-4 mode; 0 if not.
   *
   * \param is_pnetcdf    1 if parallel PNetCDF file; 0 if not.
   *
   * word size parameters are specified in bytes. valid values are 0, 4, and 8:
   */

  EX_FUNC_ENTER();

  /* check to make sure machine word sizes are sane */
/* If the following line causes a compile-time error, then there is a problem
 * which will cause exodus to not work correctly on this platform.
 *
 * Contact Greg Sjaardema, gdsjaar@sandia.gov for asisstance.
 */
#define CT_ASSERT(e) extern char(*ct_assert(void))[sizeof(char[1 - 2 * !(e)])]
  CT_ASSERT((sizeof(float) == 4 || sizeof(float) == 8) &&
            (sizeof(double) == 4 || sizeof(double) == 8));

  /* check to see if requested word sizes are valid */
  if (!*io_wordsize) {
    if (!file_wordsize) {
      *io_wordsize = NC_FLOAT_WORDSIZE;
    }
    else {
      *io_wordsize = file_wordsize;
    }
  }

  else if (*io_wordsize != 4 && *io_wordsize != 8) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unsupported I/O word size for file id: %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  else if (file_wordsize && *io_wordsize != file_wordsize) {
    *io_wordsize = file_wordsize;
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: invalid I/O word size specified for existing file id: "
             "%d, Requested I/O word size overridden.",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
  }

  if (!*comp_wordsize) {
    *comp_wordsize = sizeof(float);
  }
  else if (*comp_wordsize != 4 && *comp_wordsize != 8) {
    ex_err_fn(exoid, __func__, "ERROR: invalid compute wordsize specified", EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Check that the int64_status contains only valid bits... */
  {
    int valid_int64 = EX_ALL_INT64_API | EX_ALL_INT64_DB;
    if ((int64_status & valid_int64) != int64_status) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "Warning: invalid int64_status flag (%d) specified for "
               "existing file id: %d. Ignoring invalids",
               int64_status, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    }
    int64_status &= valid_int64;
  }

  /* Verify filetype
   *  0 -- classic format   (NC_FORMAT_CLASSIC -1)
   *  1 -- 64 bit classic   (NC_FORMAT_64BIT   -1)
   *  2 -- netcdf4          (NC_FORMAT_NETCDF4 -1)
   *  3 -- netcdf4 classic  (NC_FORMAT_NETCDF4_CLASSIC -1)
   */

  nc_inq_format(exoid, &filetype);

  if (!(new_file = malloc(sizeof(struct ex__file_item)))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate memory for internal file "
             "structure storage file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  new_file->file_id               = exoid;
  new_file->user_compute_wordsize = *comp_wordsize == 4 ? 0 : 1;
  new_file->int64_status          = int64_status;
  new_file->maximum_name_length   = ex__default_max_name_length;
  new_file->time_varid            = -1;
  new_file->compression_level     = 0;
  new_file->shuffle               = 0;
  new_file->file_type             = filetype - 1;
  new_file->is_parallel           = is_parallel;
  new_file->is_hdf5               = is_hdf5;
  new_file->is_pnetcdf            = is_pnetcdf;
  new_file->has_nodes             = 1; /* default to yes in case not set */
  new_file->has_edges             = 1;
  new_file->has_faces             = 1;
  new_file->has_elems             = 1;

  new_file->next = file_list;
  file_list      = new_file;

  if (*io_wordsize == NC_FLOAT_WORDSIZE) {
    new_file->netcdf_type_code = NC_FLOAT;
  }
  else {
    new_file->netcdf_type_code = NC_DOUBLE;
  }

  EX_FUNC_LEAVE(EX_NOERR);
}

/*............................................................................*/
/*............................................................................*/

/*! ex__conv_exit() takes the structure identified by "exoid" out of the linked
 * list which describes the files that ex_conv_array() knows how to convert.
 *
 * \note it is absolutely necessary for ex__conv_exit() to be called after
 *       ncclose(), if the parameter used as "exoid" is the id returned from
 *       an ncopen() or nccreate() call, as netCDF reuses file ids!
 *       the best place to do this is ex_close(), which is where I did it.
 *
 * \param exoid  integer which uniquely identifies the file of interest.
 */
void ex__conv_exit(int exoid)
{

  char                  errmsg[MAX_ERR_LENGTH];
  struct ex__file_item *file = file_list;
  struct ex__file_item *prev = NULL;

  EX_FUNC_ENTER();
  while (file) {
    if (file->file_id == exoid) {
      break;
    }

    prev = file;
    file = file->next;
  }

  if (!file) {
    snprintf(errmsg, MAX_ERR_LENGTH, "Warning: failure to clear file id %d - not in list.", exoid);
    ex_err(__func__, errmsg, EX_BADFILEID);
    EX_FUNC_VOID();
  }

  if (prev) {
    prev->next = file->next;
  }
  else {
    file_list = file->next;
  }

  free(file);
  EX_FUNC_VOID();
}

/*............................................................................*/
/*............................................................................*/

nc_type nc_flt_code(int exoid)
{
  /*!
   * \ingroup Utilities
   * nc_flt_code() returns either NC_FLOAT or NC_DOUBLE, based on the parameters
   * with which ex__conv_init() was called.  nc_flt_code() is used as the nc_type
   * parameter on ncvardef() calls that define floating point variables.
   *
   * "exoid" is some integer which uniquely identifies the file of interest.
   */
  EX_FUNC_ENTER();
  struct ex__file_item *file = ex__find_file_item(exoid);

  if (!file) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unknown file id %d for nc_flt_code().", exoid);
    ex_err(__func__, errmsg, EX_BADFILEID);
    return ((nc_type)-1);
  }
  EX_FUNC_LEAVE(file->netcdf_type_code);
}

int ex_int64_status(int exoid)
{
  /*!
   * \ingroup Utilities
     ex_int64_status() returns an int that can be tested
     against the defines listed below to determine which, if any,
     'types' in the database are to be stored as int64 types and which, if any,
     types are passed/returned as int64 types in the API

     | Defines: | |
     |----------|-|
     | #EX_MAPS_INT64_DB | All maps (id, order, ...) store int64_t values |
     | #EX_IDS_INT64_DB  | All entity ids (sets, blocks, maps) are int64_t values |
     | #EX_BULK_INT64_DB | All integer bulk data (local indices, counts, maps); not ids |
     | #EX_ALL_INT64_DB  | (#EX_MAPS_INT64_DB \| #EX_IDS_INT64_DB \| #EX_BULK_INT64_DB) |
     | #EX_MAPS_INT64_API| All maps (id, order, ...) passed as int64_t values |
     | #EX_IDS_INT64_API | All entity ids (sets, blocks, maps) are passed as int64_t values |
     | #EX_BULK_INT64_API| All integer bulk data (local indices, counts, maps); not ids|
     | #EX_INQ_INT64_API | Integers passed to/from ex_inquire() are int64_t |
     | #EX_ALL_INT64_API | (#EX_MAPS_INT64_API \| #EX_IDS_INT64_API \| #EX_BULK_INT64_API \|
   #EX_INQ_INT64_API) |
  */
  EX_FUNC_ENTER();
  struct ex__file_item *file = ex__find_file_item(exoid);

  if (!file) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unknown file id %d for ex_int64_status().", exoid);
    ex_err(__func__, errmsg, EX_BADFILEID);
    EX_FUNC_LEAVE(0);
  }
  EX_FUNC_LEAVE(file->int64_status);
}

int ex_set_int64_status(int exoid, int mode)
{
  /*!
    \ingroup Utilities

     ex_set_int64_status() sets the value of the INT64_API flags which
     specify how integer types are passed/returned as int64 types in
     the API

     | Mode can be one of: | |
     |----------|-|
     | 0                 | All integers are passed as int32_t values. |
     | #EX_MAPS_INT64_API| All maps (id, order, ...) passed as int64_t values |
     | #EX_IDS_INT64_API | All entity ids (sets, blocks, maps) are passed as int64_t values |
     | #EX_BULK_INT64_API| All integer bulk data (local indices, counts, maps); not ids|
     | #EX_INQ_INT64_API | Integers passed to/from ex_inquire() are int64_t |
     | #EX_ALL_INT64_API | (#EX_MAPS_INT64_API \| #EX_IDS_INT64_API \| #EX_BULK_INT64_API \|
    #EX_INQ_INT64_API) |
  */

  int api_mode = 0;
  int db_mode  = 0;

  EX_FUNC_ENTER();
  struct ex__file_item *file = ex__find_file_item(exoid);

  if (!file) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unknown file id %d for ex_int64_status().", exoid);
    ex_err(__func__, errmsg, EX_BADFILEID);
    EX_FUNC_LEAVE(0);
  }

  /* Strip of all non-INT64_API values */
  api_mode = mode & EX_ALL_INT64_API;
  db_mode  = file->int64_status & EX_ALL_INT64_DB;

  file->int64_status = api_mode | db_mode;
  EX_FUNC_LEAVE(file->int64_status);
}

/*!
  \ingroup Utilities
  \undoc
*/
int ex_set_option(int exoid, ex_option_type option, int option_value)
{
  EX_FUNC_ENTER();
  struct ex__file_item *file = ex__find_file_item(exoid);
  if (!file) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unknown file id %d for ex_set_option().", exoid);
    ex_err(__func__, errmsg, EX_BADFILEID);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  switch (option) {
  case EX_OPT_MAX_NAME_LENGTH: file->maximum_name_length = option_value; break;
  case EX_OPT_COMPRESSION_TYPE: /* Currently not used. GZip by default */ break;
  case EX_OPT_COMPRESSION_LEVEL: /* 0 (disabled/fastest) ... 9 (best/slowest) */
    /* Check whether file type supports compression... */
    if (file->is_hdf5) {
      int value = option_value;
      if (value > 9) {
        value = 9;
      }
      if (value < 0) {
        value = 0;
      }
      file->compression_level = value;
    }
    else {
      file->compression_level = 0;
    }
    break;
  case EX_OPT_COMPRESSION_SHUFFLE: /* 0 (disabled); 1 (enabled) */
    file->shuffle = option_value != 0 ? 1 : 0;
    break;
  case EX_OPT_INTEGER_SIZE_API: /* See *_INT64_* values above */
    ex_set_int64_status(exoid, option_value);
    break;
  case EX_OPT_INTEGER_SIZE_DB: /* (query only) */ break;
  default: {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: invalid option %d for ex_set_option().", (int)option);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}

/*!
 * \ingroup Utilities
 * ex__comp_ws() returns 4 (i.e. sizeof(float)) or 8 (i.e. sizeof(double)),
 * depending on the value of floating point word size used to initialize
 * the conversion facility for this file id (exoid).
 * \param exoid  integer which uniquely identifies the file of interest.
 */
int ex__comp_ws(int exoid)
{
  struct ex__file_item *file = ex__find_file_item(exoid);

  if (!file) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unknown file id %d", exoid);
    ex_err(__func__, errmsg, EX_BADFILEID);
    return (EX_FATAL);
  }
  /* Stored as 0 for 4-byte; 1 for 8-byte */
  return ((file->user_compute_wordsize + 1) * 4);
}

/*!
 * \ingroup Utilities
 * ex__is_parallel() returns 1 (true) or 0 (false) depending on whether
 * the file was opened in parallel or serial/file-per-processor mode.
 * Note that in this case parallel assumes the output of a single file,
 * not a parallel run using file-per-processor.
 * \param exoid  integer which uniquely identifies the file of interest.
 */
int ex__is_parallel(int exoid)
{
  EX_FUNC_ENTER();
  struct ex__file_item *file = ex__find_file_item(exoid);

  if (!file) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unknown file id %d", exoid);
    ex_err(__func__, errmsg, EX_BADFILEID);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  /* Stored as 1 for parallel, 0 for serial or file-per-processor */
  EX_FUNC_LEAVE(file->is_parallel);
}

/*!
 * \ingroup Utilities
 * \note
 * Do not use this unless you know what you are doing and why you
 * are doing it.  One use is if calling ex_get_partial_set() in a
 * serial mode (proc 0 only) on a file opened in parallel.
 * Make sure to reset the value to original value after done with
 * special case...
 *
 * ex_set_parallel() sets the parallel setting for a file.
 * returns 1 (true) or 0 (false) depending on the current setting.
 * \param exoid  integer which uniquely identifies the file of interest.
 * \param is_parallel 1 if parallel, 0 if serial.
 */
int ex_set_parallel(int exoid, int is_parallel)
{
  EX_FUNC_ENTER();
  int                   old_value = 0;
  struct ex__file_item *file      = ex__find_file_item(exoid);

  if (!file) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unknown file id %d", exoid);
    ex_err(__func__, errmsg, EX_BADFILEID);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  old_value         = file->is_parallel;
  file->is_parallel = is_parallel;
  /* Stored as 1 for parallel, 0 for serial or file-per-processor */
  EX_FUNC_LEAVE(old_value);
}
