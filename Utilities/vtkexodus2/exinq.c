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

#include <stdlib.h>
#include <string.h>

#include "exodusII.h"
#include "exodusII_int.h"

/*!
\fn{int ex_inquire (int   exoid,
        int   req_info,
        int  *ret_int,
        void *ret_float,
        char *ret_char)}

The function ex_inquire() is used to inquire values of certain
data entities in an exodus file. Memory must be allocated for the
returned values before this function is invoked.query database. \sa ex_inquire_int().

\return In case of an error, ex_inquire() returns a negative
  number; a warning will return a positive number.
  Possible causes of errors include:
  -  data file not properly opened with call to ex_create() or ex_open().
  -  requested information not stored in the file.
  -  invalid request flag.

\param[in] exoid     exodus file ID returned from a previous call to ex_create() or ex_open().
\param[in] req_info  A flag which designates what information is requested. It must be one
         of the following constants in the table below.

\param[out]  ret_int   Returned integer, if an integer value is requested (according
           to \c req_info); otherwise, supply a dummy argument.

\param[out]  ret_float Returned float, if a float value is requested
           (according to \c req_info); otherwise, supply a dummy
           argument. This argument is always a float even if the database IO
           and/or CPU word size is a double.

\param[out]  ret_char  Returned character string, if a character value is requested (according
           to \c req_info); otherwise, supply a dummy argument.

<table>
<tr><td>\c EX_INQ_API_VERS</td><td> The exodus API version number is returned
 in \c ret_float and an undotted version number is returned in
 \c ret_int. The API version number reflects the release of the
 function library (i.e., function names, argument list, etc.). The API
 and LIB version numbers are synchronized and will always
 match. Initially, it was thought that maintaining the two versions
 separately would be a benefit, but that was more confusing than
 helpful, so the numbers were made the same.</td></tr>

 <tr><td> \c EX_INQ_DB_VERS </td><td> The exodus database version number is
 returned in \c ret_float and an ``undotted'' version number is
 returned in \c ret_int. The database version number reflects the
 version of the library that was used to \e write the file pointed to by
 \c exoid. </td></tr>

 <tr><td> \c EX_INQ_LIB_VERS  </td><td>The exodus library version number is
 returned in \c ret_float and an undotted version number is
 returned in \c ret_int. The API library version number reflects
 the version number of the exodus library linked with this
 application. </td></tr>

 <tr><td> \c EX_INQ_TITLE  </td><td>The title stored in the database is returned in \c ret_char. </td></tr>

 <tr><td> \c EX_INQ_DIM  </td><td>The dimensionality, or number of coordinates
 per node (1, 2 or 3), of the database is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_NODES  </td><td>The number of nodes is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_ELEM  </td><td>The number of elements is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_ELEM_BLK  </td><td>The number of element blocks is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_NODE_SETS  </td><td>The number of node sets is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_NS_NODE_LEN  </td><td>The length of the concatenated node
 sets node list is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_NS_DF_LEN  </td><td>The length of the concatenated node
 sets distribution list is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_SIDE_SETS  </td><td>The number of side sets is returned in
 \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_SS_ELEM_LEN  </td><td>The length of the concatenated side
 sets element list is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_SS_DF_LEN  </td><td>The length of the concatenated side
 sets distribution factor list is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_SS_NODE_LEN  </td><td>The aggregate length of all of the
 side sets node lists is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_EB_PROP  </td><td>The number of integer properties stored
 for each element block is returned in \c ret_int; this number
 includes the property named \c ID. </td></tr>

 <tr><td> \c EX_INQ_NS_PROP  </td><td>The number of integer properties stored
 for each node set is returned in \c ret_int; this number includes
 the property named \c ID. </td></tr>

 <tr><td> \c EX_INQ_SS_PROP  </td><td>The number of integer properties stored
 for each side set is returned in \c ret_int; this number includes
 the property named \c ID. </td></tr>

 <tr><td> \c EX_INQ_QA  </td><td>The number of QA records is returned in
 \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_INFO  </td><td>The number of information records is returned
 in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_TIME  </td><td>The number of time steps stored in the
 database is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_EDGE_BLK   </td><td>The number of edge blocks is returned in
 \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_EDGE_MAP   </td><td>The number of edge maps is returned in
 \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_EDGE_PROP   </td><td>The number of properties stored per
 edge blockis returned in \c ret_int.  </td></tr>

 <tr><td> \c EX_INQ_EDGE_SETS   </td><td>The number of edge sets is returned in
 \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_EDGE   </td><td>The number of edges is returned in
 \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_FACE   </td><td>The number of faces is returned in
 \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_EB_PROP   </td><td>The number of element block properties is
 returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_ELEM_MAP   </td><td>The number of element maps is returned
 in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_ELEM_SETS   </td><td>The number of element sets is returned
 in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_ELS_DF_LEN   </td><td>The length of the concatenated
 element set distribution factor list is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_ELS_LEN   </td><td>The length of the concatenated element
 set element list is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_ELS_PROP   </td><td>The number of properties stored per elem
 set is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_EM_PROP   </td><td>The number of element map properties is
 returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_ES_DF_LEN   </td><td>The length of the concatenated edge
 set distribution factor list is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_ES_LEN   </td><td>The length of the concatenated edge set
 edge list is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_ES_PROP   </td><td>The number of properties stored per edge
 set is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_FACE_BLK   </td><td>The number of face blocks is returned in
 \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_FACE_MAP   </td><td>The number of face maps is returned in
 \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_FACE_PROP   </td><td>The number of properties stored per
 face block is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_FACE_SETS   </td><td>The number of face sets is returned in
 \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_FS_DF_LEN   </td><td>The length of the concatenated face
 set distribution factor list is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_FS_LEN   </td><td>The length of the concatenated face set
 face list is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_FS_PROP   </td><td>The number of properties stored per face
 set is returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_NM_PROP   </td><td>The number of node map properties is
 returned in \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_NODE_MAP   </td><td>The number of node maps is returned in
 \c ret_int. </td></tr>

 <tr><td> \c EX_INQ_COORD_FRAMES </td><td>The number of coordinate frames is returned in \c ret_int. </td></tr>
</table>

As an example, the following will return the number of element
block properties stored in the exodus file :

\code
#include "exodusII.h"
int error, exoid, num_props;
float fdum;
char *cdum;

\comment{determine the number of element block properties}
error = ex_inquire (exoid, EX_INQ_EB_PROP, &num_props,
        &fdum, cdum);
...Another way to get the same information
num_props = ex_inquire_int(exoid, EX_INQ_EB_PROP);
\endcode

*/

/*! \cond INTERNAL */
static int ex_get_dimension_value(int exoid, int *var, int default_value, const char *dimension_name, int missing_ok)
{
  int status;
  char  errmsg[MAX_ERR_LENGTH];
  size_t idum;
  int dimid;

  if ((status = nc_inq_dimid( exoid, dimension_name, &dimid)) != NC_NOERR) {
    *var = default_value;
    if ( missing_ok ) {
      return (EX_NOERR);
    } else {
      exerrval = status;
      sprintf( errmsg,
         "Error: failed to retrieve dimension %s for file id %d",
         dimension_name, exoid);
      ex_err("ex_inquire",errmsg,exerrval);
      return (EX_FATAL);
    }
  }
  if ((status = nc_inq_dimlen( exoid, dimid, &idum)) != NC_NOERR) {
    *var = default_value;
    exerrval = status;
    sprintf( errmsg,
       "Error: failed to retrieve value for dimension %s for file id %d",
       dimension_name, exoid);
    ex_err("ex_inquire",errmsg,exerrval);
    return (EX_FATAL);
  }
  *var = (int)idum;
  return (EX_NOERR);
}

static int ex_get_concat_set_len(int exoid, int *set_length, const char *set_name,
         ex_entity_type set_type, const char *set_num_dim,
         const char *set_stat_var, const char *set_size_root,
         int missing_ok)
{
  int i;
  int status;
  char  errmsg[MAX_ERR_LENGTH];
  size_t idum;
  int dimid, varid;
  size_t num_sets;
  int *stat_vals = NULL;
  int *ids = NULL;

  *set_length = 0;     /* default return value */

  if ((status = nc_inq_dimid (exoid, set_num_dim, &dimid)) == NC_NOERR)
    {
      if ((status = nc_inq_dimlen (exoid, dimid, &num_sets)) != NC_NOERR) {
  exerrval = status;
  sprintf(errmsg,
    "Error: failed to get number of %s sets in file id %d",
    set_name, exoid);
  ex_err("ex_inquire",errmsg,exerrval);
  return (EX_FATAL);
      }

      if (!(ids = malloc(num_sets*sizeof(int)))) {
  exerrval = EX_MEMFAIL;
  sprintf(errmsg,
    "Error: failed to allocate memory for %s set ids for file id %d",
    set_name, exoid);
  ex_err("ex_inquire",errmsg,exerrval);
  return (EX_FATAL);
      }

      if (ex_get_ids (exoid, set_type, ids) == EX_FATAL) {
  sprintf(errmsg,
    "Error: failed to get %s set ids in file id %d",
    set_name, exoid);
  ex_err("ex_inquire",errmsg,exerrval);
  free(ids);
  return (EX_FATAL);
      }

      /* Allocate space for stat array */
      if (!(stat_vals = malloc((int)num_sets*sizeof(int)))) {
  exerrval = EX_MEMFAIL;
  free (ids);
  sprintf(errmsg,
    "Error: failed to allocate memory for %s set status array for file id %d",
    set_name, exoid);
  ex_err("ex_inquire",errmsg,exerrval);
  return (EX_FATAL);
      }

      /* get variable id of status array */
      if ((status = nc_inq_varid (exoid, set_stat_var, &varid)) == NC_NOERR) {
  /* if status array exists, use it, otherwise assume, object exists
     to be backward compatible */
  if ((status = nc_get_var_int(exoid, varid, stat_vals)) != NC_NOERR) {
    exerrval = status;
    free (ids);
    free(stat_vals);
    sprintf(errmsg,
      "Error: failed to get %s set status array from file id %d",
      set_name, exoid);
    ex_err("ex_inquire",errmsg,exerrval);
    return (EX_FATAL);
  }
      } else /* default: status is true */
  for(i=0;i<num_sets;i++)
    stat_vals[i]=1;

      for (i=0; i<num_sets; i++) {
  if (stat_vals[i] == 0) /* is this object null? */
    continue;


  if ((status = nc_inq_dimid (exoid, ex_catstr(set_size_root,i+1), &dimid)) != NC_NOERR) {
    if ( missing_ok ) {
      idum = 0;
    } else {
      *set_length = 0;
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to locate %s set %d in file id %d",
        set_name, ids[i],exoid);
      ex_err("ex_inquire",errmsg,exerrval);
      free(stat_vals);
      free(ids);
      return (EX_FATAL);
    }
  } else {
    if ((status = nc_inq_dimlen (exoid, dimid, &idum)) != NC_NOERR) {
      *set_length = 0;
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to get size of %s set %d in file id %d",
        set_name, ids[i], exoid);
      ex_err("ex_inquire",errmsg,exerrval);
      free(stat_vals);
      free(ids);
      return (EX_FATAL);
    }
  }

  *set_length += (int)idum;
      }

      free(stat_vals);
      free (ids);
    }
  return (EX_NOERR);
}

static void flt_cvt(float *xptr,double x)
{
  *xptr = (float)x;
}
/*! \endcond */

/*!
  A variant of ex_inquire() which queries integer-valued information only. \see ex_inquire().
  \param[in] exoid     exodus file ID returned from a previous call to ex_create() or ex_open().
  \param[in] req_info  A flag which designates what information is requested.
           (See ex_inquire() documentation)
  \return    result of inquiry.

 As an example, the following will return the number of nodes,
 elements, and element blocks stored in the exodus file :

\code
#include "exodusII.h"
int exoid;
int num_nodes = ex_inquire_int(exoid, EX_INQ_NODES);
int num_elems = ex_inquire_int(exoid, EX_INQ_ELEM);
int num_block = ex_inquire_int(exoid, EX_INQ_ELEM_BLK);
\endcode

*/

int ex_inquire_int (int exoid, int req_info)
{
  char *cdummy = NULL; /* Needed just for function call, unused. */
  float fdummy = 0;    /* Needed just for function call, unused. */
  int   ret_val = 0;
  int error = ex_inquire(exoid, req_info, &ret_val, &fdummy, cdummy);
  if (error < 0)
    ret_val = error;

  return ret_val;
}

int ex_inquire (int   exoid,
    int   req_info,
    int  *ret_int,
    void *ret_float,
    char *ret_char)
{
  int dimid, varid, tmp_num, *ids;
  size_t i;
  size_t ldum = 0;
  size_t num_sets, idum;
  int *stat_vals;
  char  errmsg[MAX_ERR_LENGTH];
  int status;
  char tmp_title[2048];

  exerrval = 0; /* clear error code */

  if (ret_char)  *ret_char  = '\0';
  if (ret_int)   *ret_int   = 0;

  switch (req_info) {
  case EX_INQ_FILE_TYPE:

    /* obsolete call */
    /*returns "r" for regular EXODUS II file or "h" for history EXODUS file*/
    exerrval = EX_BADPARAM;
    sprintf(errmsg,
      "Warning: file type inquire is obsolete");
    ex_err("ex_inquire",errmsg,exerrval);
    return (EX_WARN);

  case EX_INQ_API_VERS:
    /* returns the EXODUS II API version number */
    if (nc_get_att_float(exoid, NC_GLOBAL, ATT_API_VERSION, ret_float) != NC_NOERR)
      {  /* try old (prior to db version 2.02) attribute name */
  if ((status = nc_get_att_float (exoid, NC_GLOBAL, ATT_API_VERSION_BLANK,ret_float)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to get EXODUS API version for file id %d", exoid);
    ex_err("ex_inquire",errmsg,exerrval);
    return (EX_FATAL);
  }
      }

    break;

  case EX_INQ_DB_VERS:
    /* returns the EXODUS II database version number */
    if ((status = nc_get_att_float (exoid, NC_GLOBAL, ATT_VERSION, ret_float)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to get EXODUS database version for file id %d", exoid);
      ex_err("ex_inquire",errmsg,exerrval);
      return (EX_FATAL);
    }
    break;

  case EX_INQ_LIB_VERS:
    /* returns the EXODUS II Library version number */
    if (ret_float)
      flt_cvt((float *)ret_float, EX_API_VERS);

    if (ret_int)
      *ret_int = EX_API_VERS_NODOT;
    break;

  case EX_INQ_DB_MAX_ALLOWED_NAME_LENGTH:
    /* Return the MAX_NAME_LENGTH size for this database
       It will not include the space for the trailing null, so if it
       is defined as 33 on the database, 32 will be returned.
    */
    if ((status = nc_inq_dimid(exoid, DIM_STR_NAME, &dimid)) != NC_NOERR) {
      /* If not found, then an older database */
      *ret_int = 32;
    }
    else {
      /* Get the name string length */
      size_t name_length = 0;
      if ((status = nc_inq_dimlen(exoid,dimid,&name_length)) != NC_NOERR) {
  exerrval = status;
  sprintf(errmsg,
    "Error: failed to get name string length in file id %d", exoid);
  ex_err("ex_inquire",errmsg,exerrval);
  return (EX_FATAL);
      }
      else {
  *ret_int = name_length-1;
      }
    }
    break;

  case EX_INQ_DB_MAX_USED_NAME_LENGTH:
    /* Return the value of the ATT_MAX_NAME_LENGTH attribute (if it
       exists) which is the maximum length of any entity, variable,
       attribute, property name written to this database.  If the
       attribute does not exist, then '32' is returned.  The length
       does not include the trailing null.
    */
    {
      nc_type att_type = NC_NAT;
      size_t att_len = 0;
  
      *ret_int = 32; /* Default size consistent with older databases */

      status = nc_inq_att(exoid, NC_GLOBAL, ATT_MAX_NAME_LENGTH, &att_type, &att_len);
      if (status == NC_NOERR && att_type == NC_INT) {
  /* The attribute exists, return it... */
  status = nc_get_att_int(exoid, NC_GLOBAL, ATT_MAX_NAME_LENGTH, ret_int);
      }
    }
    break;

  case EX_INQ_MAX_READ_NAME_LENGTH:
    /* Returns the user-specified maximum size of names that will be
     * returned to the user by any of the ex_get_ routines.  If the
     * name is longer than this value, it will be truncated. The
     * default if not set by the client is 32 characters. The value
     * does not include the trailing null.
     */
    *ret_int = ex_max_name_length;
    break;

  case EX_INQ_TITLE:
    /* returns the title of the database */
    if ((status = nc_get_att_text (exoid, NC_GLOBAL, ATT_TITLE, tmp_title)) != NC_NOERR) {
      *ret_char = '\0';
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to get database title for file id %d", exoid);
      ex_err("ex_inquire",errmsg,exerrval);
      return (EX_FATAL);
    } else {
      strncpy(ret_char, tmp_title, MAX_LINE_LENGTH+1);
      ret_char[MAX_LINE_LENGTH] = '\0';
    }
    break;

  case EX_INQ_DIM:
    /* returns the dimensionality (2 or 3, for 2-d or 3-d) of the database */
    if (ex_get_dimension(exoid, DIM_NUM_DIM, "database dimensionality", &ldum, &dimid, "ex_inquire") != NC_NOERR)
      return EX_FATAL;
    *ret_int = ldum;
    break;

  case EX_INQ_NODES:
    /* returns the number of nodes */
    if (ex_get_dimension(exoid, DIM_NUM_NODES, "nodes", &ldum, &dimid, NULL) != NC_NOERR)
      *ret_int = 0;
    else
      *ret_int = ldum;
    break;

  case EX_INQ_ELEM:
    /* returns the number of elements */
    if (ex_get_dimension(exoid, DIM_NUM_ELEM, "elements", &ldum, &dimid, NULL) != NC_NOERR)
      *ret_int = 0;
    else
      *ret_int = ldum;
    break;

  case EX_INQ_ELEM_BLK:
    /* returns the number of element blocks */
    if (ex_get_dimension(exoid, DIM_NUM_EL_BLK, "element blocks", &ldum, &dimid, NULL) != NC_NOERR)
      *ret_int = 0;
    else
      *ret_int = ldum;
    break;

  case EX_INQ_NODE_SETS:
    /* returns the number of node sets */
    if (ex_get_dimension(exoid, DIM_NUM_NS, "node sets", &ldum, &dimid, NULL) != NC_NOERR)
      *ret_int = 0;
    else
      *ret_int = ldum;
    break;

  case EX_INQ_NS_NODE_LEN:
    /* returns the length of the concatenated node sets node list */
    ex_get_concat_set_len(exoid, ret_int,"node",EX_NODE_SET,DIM_NUM_NS,VAR_NS_STAT,"num_nod_ns",0);
    break;

  case EX_INQ_NS_DF_LEN:
    /*     returns the length of the concatenated node sets dist factor list */

    /*
      Determine the concatenated node sets distribution factor length:

      1. Get the node set ids list.
      2. Check see if the dist factor variable for a node set id exists.
      3. If it exists, goto step 4, else the length is zero.
      4. Get the dimension of the number of nodes in the node set -0
      use this value as the length as by definition they are the same.
      5. Sum the individual lengths for the total list length.
    */

    *ret_int = 0;    /* default value if no node sets defined */

    if (nc_inq_dimid (exoid, DIM_NUM_NS, &dimid) == NC_NOERR) {
      if ((status = nc_inq_dimlen(exoid, dimid, &num_sets)) != NC_NOERR) {
  exerrval = status;
  sprintf(errmsg,
    "Error: failed to get number of node sets in file id %d",
    exoid);
  ex_err("ex_inquire",errmsg,exerrval);
  return (EX_FATAL);
      }


      if (!(ids = malloc(num_sets*sizeof(int))))
  {
    exerrval = EX_MEMFAIL;
    sprintf(errmsg,
      "Error: failed to allocate memory for node set ids for file id %d",
      exoid);
    ex_err("ex_inquire",errmsg,exerrval);
    return (EX_FATAL);
  }

      if (ex_get_node_set_ids (exoid, ids) == EX_FATAL)
  {
    sprintf(errmsg,
      "Error: failed to get node sets in file id %d",
      exoid);
    /* pass back error code from ex_get_node_set_ids (in exerrval) */
    ex_err("ex_inquire",errmsg,exerrval);
    free (ids);
    return (EX_FATAL);
  }

      for (i=0; i<num_sets; i++) {
  if ((status = nc_inq_varid (exoid, VAR_FACT_NS(i+1), &varid)) != NC_NOERR) {
    if (status == NC_ENOTVAR) {
      idum = 0;        /* this dist factor doesn't exist */
    } else {
      *ret_int = 0;
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to locate number of dist fact for node set %d in file id %d",
        ids[i], exoid);
      ex_err("ex_inquire",errmsg,exerrval);
      free (ids);
      return (EX_FATAL);
    }
  } else {
    if ((status = nc_inq_dimid (exoid, DIM_NUM_NOD_NS(i+1), &dimid)) != NC_NOERR) {
      *ret_int = 0;
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to locate number of nodes in node set %d in file id %d",
        ids[i], exoid);
      ex_err("ex_inquire",errmsg,exerrval);
      free (ids);
      return (EX_FATAL);
    }
    if ((status = nc_inq_dimlen (exoid, dimid, &idum)) != NC_NOERR) {
      *ret_int = 0;
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to get number of nodes in node set %d in file id %d",
        ids[i],exoid);
      ex_err("ex_inquire",errmsg,exerrval);
      free(ids);
      return (EX_FATAL);
    }
  }
  *ret_int += idum;
      }
      free(ids);
    }

    break;

  case EX_INQ_SIDE_SETS:
    /* returns the number of side sets */
    if (ex_get_dimension(exoid, DIM_NUM_SS, "side sets", &ldum, &dimid, NULL) != NC_NOERR)
      *ret_int = 0;
    else
      *ret_int = ldum;
    break;

  case EX_INQ_SS_NODE_LEN:

    /*     returns the length of the concatenated side sets node list */

    *ret_int = 0;     /* default return value */

    if (nc_inq_dimid (exoid, DIM_NUM_SS, &dimid) == NC_NOERR) {
      if ((status = nc_inq_dimlen(exoid, dimid, &num_sets)) != NC_NOERR) {
  exerrval = status;
  sprintf(errmsg,
    "Error: failed to get number of side sets in file id %d",
    exoid);
  ex_err("ex_inquire",errmsg,exerrval);
  return (EX_FATAL);
      }


      if (!(ids = malloc(num_sets*sizeof(int)))) {
  exerrval = EX_MEMFAIL;
  sprintf(errmsg,
    "Error: failed to allocate memory for side set ids for file id %d",
    exoid);
  ex_err("ex_inquire",errmsg,exerrval);
  return (EX_FATAL);
      }

      if (ex_get_side_set_ids (exoid, ids) == EX_FATAL) {
  sprintf(errmsg,
    "Error: failed to get side set ids in file id %d",
    exoid);
  ex_err("ex_inquire",errmsg,exerrval);
  free(ids);
  return (EX_FATAL);
      }

      /* allocate space for stat array */
      if (!(stat_vals = malloc((int)num_sets*sizeof(int)))) {
  exerrval = EX_MEMFAIL;
  free (ids);
  sprintf(errmsg,
    "Error: failed to allocate memory for side set status array for file id %d",
    exoid);
  ex_err("ex_inquire",errmsg,exerrval);
  return (EX_FATAL);
      }
      /* get variable id of status array */
      if ((status = nc_inq_varid (exoid, VAR_SS_STAT, &varid)) == NC_NOERR) {
  /* if status array exists, use it, otherwise assume, object exists
     to be backward compatible */

  if ((status = nc_get_var_int(exoid, varid, stat_vals)) != NC_NOERR) {
    exerrval = status;
    free (ids);
    free(stat_vals);
    sprintf(errmsg,
      "Error: failed to get element block status array from file id %d",
      exoid);
    ex_err("ex_inquire",errmsg,exerrval);
    return (EX_FATAL);
  }
      }
      else /* default: status is true */
  for(i=0;i<num_sets;i++)
    stat_vals[i]=1;

      /* walk id list, get each side set node length and sum for total */

      for (i=0; i<num_sets; i++) {
  if (stat_vals[i] == 0) /* is this object null? */
    continue;

  if ((status = ex_get_side_set_node_list_len(exoid, ids[i], &tmp_num)) != NC_NOERR) {
    *ret_int = 0;
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to side set %d node length in file id %d",
      ids[i],exoid);
    ex_err("ex_inquire",errmsg,exerrval);
    free(stat_vals);
    free(ids);
    return (EX_FATAL);
  }
  *ret_int += tmp_num;
      }

      free(stat_vals);
      free (ids);
    }

    break;

  case EX_INQ_SS_ELEM_LEN:
    /*     returns the length of the concatenated side sets element list */
    ex_get_concat_set_len(exoid, ret_int,"side",EX_SIDE_SET,DIM_NUM_SS,VAR_SS_STAT,"num_side_ss",0);
    break;

  case EX_INQ_SS_DF_LEN:

    /*     returns the length of the concatenated side sets dist factor list */

    /*
      Determine the concatenated side sets distribution factor length:

      1. Get the side set ids list.
      2. Check see if the dist factor dimension for a side set id exists.
      3. If it exists, goto step 4, else set the individual length to zero.
      4. Sum the dimension value into the running total length.
    */

    *ret_int = 0;

    /* first check see if any side sets exist */

    if (nc_inq_dimid (exoid, DIM_NUM_SS, &dimid) == NC_NOERR) {
      if ((status = nc_inq_dimlen (exoid, dimid, &num_sets)) != NC_NOERR) {
  exerrval = status;
  sprintf(errmsg,
    "Error: failed to get number of side sets in file id %d",
    exoid);
  ex_err("ex_inquire",errmsg,exerrval);
  return (EX_FATAL);
      }


      if (!(ids = malloc(num_sets*sizeof(int)))) {
  exerrval = EX_MEMFAIL;
  sprintf(errmsg,
    "Error: failed to allocate memory for side set ids for file id %d",
    exoid);
  ex_err("ex_inquire",errmsg,exerrval);
  return (EX_FATAL);
      }

      if (ex_get_side_set_ids (exoid, ids) == EX_FATAL) {
  sprintf(errmsg,
    "Error: failed to get side sets in file id %d",
    exoid);
  /* pass back error code from ex_get_side_set_ids (in exerrval) */
  ex_err("ex_inquire",errmsg,exerrval);
  free (ids);
  return (EX_FATAL);
      }

      for (i=0; i<num_sets; i++) {
  if ((status = nc_inq_dimid (exoid, DIM_NUM_DF_SS(i+1), &dimid)) != NC_NOERR) {
    if (status == NC_EBADDIM) {
      ldum = 0;        /* this dist factor doesn't exist */
    } else {
      *ret_int = 0;
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to locate number of dist fact for side set %d in file id %d",
        ids[i], exoid);
      ex_err("ex_inquire",errmsg,exerrval);
      free (ids);
      return (EX_FATAL);
    }
  } else {
    if ((status = nc_inq_dimlen (exoid, dimid, &ldum)) != NC_NOERR) {
      *ret_int = 0;
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to get number of dist factors in side set %d in file id %d",
        ids[i], exoid);
      ex_err("ex_inquire",errmsg,exerrval);
      free (ids);
      return (EX_FATAL);
    }
  }
  *ret_int += ldum;
      }
      free (ids);
    }

    break;

  case EX_INQ_QA:
    /* returns the number of QA records */
    if (ex_get_dimension(exoid, DIM_NUM_QA, "QA records", &ldum, &dimid, NULL) != NC_NOERR)
      *ret_int = 0;
    else
      *ret_int = ldum;
    break;

  case EX_INQ_INFO:
    /* returns the number of information records */
    if (ex_get_dimension(exoid, DIM_NUM_INFO, "info records", &ldum, &dimid, NULL) != NC_NOERR)
      *ret_int = 0;
    else
      *ret_int = ldum;
    break;

  case EX_INQ_TIME:
    /*     returns the number of time steps stored in the database */
    if (ex_get_dimension(exoid, DIM_TIME, "time dimension", &ldum, &dimid, "ex_inquire") != NC_NOERR)
      return EX_FATAL;
    *ret_int = ldum;
    break;

  case EX_INQ_EB_PROP:
    /* returns the number of element block properties */
    *ret_int = ex_get_num_props (exoid, EX_ELEM_BLOCK);
    break;

  case EX_INQ_NS_PROP:
    /* returns the number of node set properties */
    *ret_int = ex_get_num_props (exoid, EX_NODE_SET);
    break;

  case EX_INQ_SS_PROP:
    /* returns the number of side set properties */
    *ret_int = ex_get_num_props (exoid, EX_SIDE_SET);
    break;

  case EX_INQ_ELEM_MAP:
    /* returns the number of element maps */
    if (ex_get_dimension(exoid, DIM_NUM_EM, "element maps", &ldum, &dimid, NULL) != NC_NOERR)
      *ret_int = 0;
    else
      *ret_int = ldum;
    break;

  case EX_INQ_EM_PROP:
    /* returns the number of element map properties */
    *ret_int = ex_get_num_props (exoid, EX_ELEM_MAP);
    break;

  case EX_INQ_NODE_MAP:
    /* returns the number of node maps */
    if (ex_get_dimension(exoid, DIM_NUM_NM, "node maps", &ldum, &dimid, NULL) != NC_NOERR)
      *ret_int = 0;
    else
      *ret_int = ldum;
    break;

  case EX_INQ_NM_PROP:
    /* returns the number of node map properties */
    *ret_int = ex_get_num_props (exoid, EX_NODE_MAP);
    break;

  case EX_INQ_EDGE:
    /* returns the number of edges (defined across all edge blocks). */
    if (ex_get_dimension_value(exoid, ret_int, 0, DIM_NUM_EDGE, 1) != EX_NOERR) return EX_FATAL;
    break;

  case EX_INQ_EDGE_BLK:
    /* returns the number of edge blocks. */
    if (ex_get_dimension_value(exoid, ret_int, 0, DIM_NUM_ED_BLK, 1) != EX_NOERR) return EX_FATAL;
    break;

  case EX_INQ_EDGE_SETS:
    /* returns the number of edge sets. */
    if (ex_get_dimension_value(exoid, ret_int, 0, DIM_NUM_ES, 1) != EX_NOERR) return EX_FATAL;
    break;

  case EX_INQ_ES_LEN:
    /* returns the length of the concatenated edge set edge list. */
    ex_get_concat_set_len(exoid, ret_int,"edge",EX_EDGE_SET,DIM_NUM_ES,VAR_ES_STAT,"num_edge_es",0);
    break;

  case EX_INQ_ES_DF_LEN:
    /* returns the length of the concatenated edge set distribution factor list. */
    ex_get_concat_set_len(exoid, ret_int,"edge",EX_EDGE_SET,DIM_NUM_ES,VAR_ES_STAT,"num_df_es",1);
    break;

  case EX_INQ_EDGE_PROP:
    /* returns the number of integer properties stored for each edge block. This includes the "ID" property. */
    *ret_int = ex_get_num_props( exoid, EX_EDGE_BLOCK );
    break;

  case EX_INQ_ES_PROP:
    /* returns the number of integer properties stored for each edge set.. This includes the "ID" property */
    *ret_int = ex_get_num_props( exoid, EX_EDGE_SET );
    break;

  case EX_INQ_FACE:
    /* returns the number of faces (defined across all face blocks). */
    if (ex_get_dimension_value(exoid, ret_int, 0, DIM_NUM_FACE, 1) != EX_NOERR) return EX_FATAL;
    break;

  case EX_INQ_FACE_BLK:
    /* returns the number of face blocks. */
    if (ex_get_dimension_value(exoid, ret_int, 0, DIM_NUM_FA_BLK, 1) != EX_NOERR) return EX_FATAL;
    break;

  case EX_INQ_FACE_SETS:
    /* returns the number of face sets. */
    if (ex_get_dimension_value(exoid, ret_int, 0, DIM_NUM_FS, 1) != EX_NOERR) return EX_FATAL;
    break;

  case EX_INQ_FS_LEN:
    /* returns the length of the concatenated edge set edge list. */
    ex_get_concat_set_len(exoid, ret_int,"face",EX_FACE_SET,DIM_NUM_FS,VAR_FS_STAT,"num_face_fs",0);
    break;

  case EX_INQ_FS_DF_LEN:
    /* returns the length of the concatenated edge set distribution factor list. */
    ex_get_concat_set_len(exoid, ret_int,"face",EX_FACE_SET,DIM_NUM_FS,VAR_FS_STAT,"num_df_fs",1);
    break;

  case EX_INQ_FACE_PROP:
    /* returns the number of integer properties stored for each edge block. This includes the "ID" property. */
    *ret_int = ex_get_num_props( exoid, EX_FACE_BLOCK );
    break;

  case EX_INQ_FS_PROP:
    /* returns the number of integer properties stored for each edge set.. This includes the "ID" property */
    *ret_int = ex_get_num_props( exoid, EX_FACE_SET );
    break;

  case EX_INQ_ELEM_SETS:
    /* returns the number of element sets. */
    if (ex_get_dimension_value(exoid, ret_int, 0, DIM_NUM_ELS, 1) != EX_NOERR) return EX_FATAL;
    break;

  case EX_INQ_ELS_LEN:
    /* returns the length of the concatenated element set element list. */
    ex_get_concat_set_len(exoid, ret_int,"element",EX_ELEM_SET,DIM_NUM_ELS,VAR_ELS_STAT,"num_ele_els",0);
    break;

  case EX_INQ_ELS_DF_LEN:
    /* returns the length of the concatenated element set distribution factor list. */
    ex_get_concat_set_len(exoid, ret_int,"element",EX_ELEM_SET,DIM_NUM_ELS,VAR_ELS_STAT,"num_df_els",1);
    break;

  case EX_INQ_ELS_PROP:
    /* returns the number of integer properties stored for each element set. */
    *ret_int = ex_get_num_props( exoid, EX_ELEM_SET );
    break;

  case EX_INQ_EDGE_MAP:
    /* returns the number of edge maps. */
    if (ex_get_dimension_value(exoid, ret_int, 0, DIM_NUM_EDM, 1) != EX_NOERR) return EX_FATAL;
    break;

  case EX_INQ_FACE_MAP:
    /*     returns the number of face maps. */
    if (ex_get_dimension_value(exoid, ret_int, 0, DIM_NUM_FAM, 1) != EX_NOERR) return EX_FATAL;
    break;

  case EX_INQ_COORD_FRAMES:
    /* return the number of coordinate frames */
    if (ex_get_dimension_value(exoid, ret_int, 0, DIM_NUM_CFRAMES, 1) != EX_NOERR) return EX_FATAL;
    break;

  default:
    *ret_int = 0;
    exerrval = EX_FATAL;
    sprintf(errmsg, "Error: invalid inquiry %d", req_info);
    ex_err("ex_inquire",errmsg,exerrval);
    return(EX_FATAL);
  }
  return (EX_NOERR);
}
