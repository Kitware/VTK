/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgfrm - ex_get_coordinate_frames: read coordinate frames
 *
 * entry conditions -
 *   input parameters:
 *       int         exoid               exodus file id
 *
 * NOTE:
 *    cf_ids, pt_coordinates and tags may all be NULL, otherwise, they
 *    must have sufficient memory to store the output data. Two calls
 *    to this routine are anticipated (one to get nframes, the second
 *    to fill the data. i.e.
 *      ex_get_coordinate_frames(exoid,&nframes,0,0,0);
 *        ... allocate memory ...
 *      ex_get_coordinate_frames(exoid,&nframe,id,coords,tags);
 *
 *
 * output conditions -
 *       int*         nframes             number of coordinate frames in model
 *       const int*   cf_ids             coordinate ids
 *                                       dimension int[nframes]
 *       const void*  pt_coordinates     pointer to coordinates. 9 values per
 *                                       coordinate frame
 *                                       dimension float[9*nframes]
 *       const char*  tags               character tag for each frame.
 *                                        'r' - rectangular
 *                                        'c' - cylindrical
 *                                        's' - spherical
 *                                        dimension char[nframes]
 *
 * returns -
 *      EX_NOERR         for no error
 *      EX_FATAL         for fatal errors
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, EX_NOERR, etc

/*!
 * Coordinate frames are stored in the database as a series of three
 * points (defined in the basic cartesian coordinate system). The
 * first of these points describes the origin of the new system.  The
 * second point lies on the 3 axis (or Z axis) of the frame. The third
 * point is in the 1-3 (xz) plane. Each coordinate frame is identified
 * by a unique, integer coordinate ID, and by a character tag
 * indicating whether the frame is rectangular cartesian
 * "R", cylindrical "C, or spherical "S".
 * Because the coordinates are floating point values,
 * the application code must declare the arrays passed to be the
 * appropriate type "float" or "double" to match the
 * compute word size passed in ex_create() or
 * ex_open().
 * \param        exoid    exodus file id
 * \param[in,out] nframes  if 'cf_ids' is NULL, then nframes is returned with
 * the number
 *                        of defined coordinate frames. Else it is the number of
 * coordinate
 *                        frames to read.
 * \param[out] cf_ids The (nframes) coordinate frame Ids. If cf_ids is
 *                    NULL, no data will be returned in this or any other array.
 * Only
 *                    nframes will be modified. Otherwise, space must be
 * allocated to
 *                    store 'nframes' integers before making this call.
 * \param[out] pt_coordinates The (9*nframes) coordinates of the three
 *                            points defining each coordinate axis. The first
 * three values are
 *                            the origin of the first frame. The next three
 * values are the
 *                            coordinates of a point on the 3rd axis of the
 * first frame. The next
 *                            three values are the coordinates of a point in the
 * plane of the 1-3
 *                            axis. The pattern is repeated for each frame. If
 * 'cf_ids'
 *                            is null, no data will be returned in this array.
 * Otherwise, space
 *                            must be allocated for 9*nframes floating point
 * values. The size of
 *                            the allocation depends upon the compute word size.
 * \param[out] tags The (nframes) character tags associated with each
 *                  coordinate frame. If 'cf_ids' is NULL, no data will be
 *                  returned in this array. Otherwise, space must be allocated
 * for
 *                  'nframes' characters.
 */

int ex_get_coordinate_frames(int exoid, int *nframes, void_int *cf_ids, void *pt_coordinates,
                             char *tags)

{
  int    status;
  int    dimid; /* ID of the dimension of # frames */
  char   errmsg[MAX_ERR_LENGTH];
  int    varids;    /* variable id for the frame ids  */
  size_t start = 0; /* start value for varputs        */
  size_t count = 0; /* number vars to put in varput   */

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* get the dimensions */
  assert(nframes != NULL);
  status = nc_inq_dimid(exoid, DIM_NUM_CFRAMES, &dimid);
  if (status != NC_NOERR) {
    *nframes = 0;
    EX_FUNC_LEAVE(EX_NOERR);
  }

  (void)nc_inq_dimlen(exoid, dimid, &count);
  *nframes = (int)count;

  if (count == 0) {
    EX_FUNC_LEAVE(EX_NOERR);
  }

  if (cf_ids) {
    if ((status = nc_inq_varid(exoid, VAR_FRAME_IDS, &varids)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to read number coordinate ids from file id %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
      status = nc_get_var_longlong(exoid, varids, cf_ids);
    }
    else {
      status = nc_get_var_int(exoid, varids, cf_ids);
    }

    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to read coordinate frame ids from file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  if (tags) {
    if ((status = nc_inq_varid(exoid, VAR_FRAME_TAGS, &varids)) != NC_NOERR ||
        (nc_get_vara_text(exoid, varids, &start, &count, tags) != NC_NOERR)) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to read number coordinate tags from file id %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  if (pt_coordinates) {
    if ((status = nc_inq_varid(exoid, VAR_FRAME_COORDS, &varids)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to read number coordinate tags from file id %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (exi_comp_ws(exoid) == 4) {
      status = nc_get_var_float(exoid, varids, pt_coordinates);
    }
    else {
      status = nc_get_var_double(exoid, varids, pt_coordinates);
    }

    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to read number coordinate tags from file id %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  EX_FUNC_LEAVE(EX_NOERR);
}
