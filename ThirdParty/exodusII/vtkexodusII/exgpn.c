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

#include <string.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
  
The function ex_get_prop_names() returns names of integer properties
stored for an element block, node set, or side set. The number of
properties (needed to allocate space for the property names) can be
obtained via a call to ex_inquire() or ex_inquire_int().

\return In case of an error, ex_get_prop_names() returns a negative number; a
warning will return a positive number.  Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  invalid object type specified.


\param[in]   exoid        exodus file ID returned from a previous call to ex_create() or ex_open().
\param[in]   obj_type     Type of object; use one of the options in the table below.
\param[out]  prop_names   Returned array containing \c num_props (obtained from call to
                          ex_inquire() or ex_inquire_int()) names (of maximum length
        \p MAX_STR_LENGTH ) of properties to be stored. \b ID, a
        reserved property name, will be the first name in the array.

<table>
<tr><td> \c EX_NODE_SET   </td><td>  Node Set entity type     </td></tr>
<tr><td> \c EX_EDGE_BLOCK </td><td>  Edge Block entity type   </td></tr>
<tr><td> \c EX_EDGE_SET   </td><td>  Edge Set entity type     </td></tr>
<tr><td> \c EX_FACE_BLOCK </td><td>  Face Block entity type   </td></tr>
<tr><td> \c EX_FACE_SET   </td><td>  Face Set entity type     </td></tr>
<tr><td> \c EX_ELEM_BLOCK </td><td>  Element Block entity type</td></tr>
<tr><td> \c EX_ELEM_SET   </td><td>  Element Set entity type  </td></tr>
<tr><td> \c EX_SIDE_SET   </td><td>  Side Set entity type     </td></tr>
<tr><td> \c EX_ELEM_MAP   </td><td>  Element Map entity type  </td></tr>
<tr><td> \c EX_NODE_MAP   </td><td>  Node Map entity type     </td></tr>
<tr><td> \c EX_EDGE_MAP   </td><td>  Edge Map entity type     </td></tr>
<tr><td> \c EX_FACE_MAP   </td><td>  Face Map entity type     </td></tr>
</table>

As an example, the following code segment reads in properties assigned
to node sets:

\code
#include "exodusII.h";
int error, exoid, num_props, *prop_values;
char *prop_names[MAX_PROPS];

\comment{read node set properties}
num_props = ex_inquire_int(exoid, EX_INQ_NS_PROP);

for (i=0; i < num_props; i++) {
   prop_names[i] = (char *) malloc ((MAX_STR_LENGTH+1), sizeof(char));
   prop_values = (int *) malloc (num_node_sets, sizeof(int));
}

error = ex_get_prop_names(exoid,EX_NODE_SET,prop_names);

for (i=0; i < num_props; i++) {
   error = ex_get_prop_array(exoid, EX_NODE_SET, prop_names[i],
                             prop_values);
}
\endcode

*/

int ex_get_prop_names (int    exoid,
                       ex_entity_type obj_type,
                       char **prop_names)
{
  int status;
  int i, num_props, propid;
  char var_name[12];
  size_t att_len;
  nc_type att_type;

  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0;

  /* determine which type of object property names are desired for */

  num_props = ex_get_num_props (exoid, obj_type);

  for (i=0; i<num_props; i++) {
    switch (obj_type) {
    case EX_ELEM_BLOCK:
      strcpy (var_name, VAR_EB_PROP(i+1));
      break;
    case EX_FACE_BLOCK:
      strcpy (var_name, VAR_FA_PROP(i+1));
      break;
    case EX_EDGE_BLOCK:
      strcpy (var_name, VAR_ED_PROP(i+1));
      break;
    case EX_NODE_SET:
      strcpy (var_name, VAR_NS_PROP(i+1));
      break;
    case EX_SIDE_SET:
      strcpy (var_name, VAR_SS_PROP(i+1));
      break;
    case EX_EDGE_SET:
      strcpy (var_name, VAR_ES_PROP(i+1));
      break;
    case EX_FACE_SET:
      strcpy (var_name, VAR_FS_PROP(i+1));
      break;
    case EX_ELEM_SET:
      strcpy (var_name, VAR_ELS_PROP(i+1));
      break;
    case EX_ELEM_MAP:
      strcpy (var_name, VAR_EM_PROP(i+1));
      break;
    case EX_FACE_MAP:
      strcpy (var_name, VAR_FAM_PROP(i+1));
      break;
    case EX_EDGE_MAP:
      strcpy (var_name, VAR_EDM_PROP(i+1));
      break;
    case EX_NODE_MAP:
      strcpy (var_name, VAR_NM_PROP(i+1));
      break;
    default:
      exerrval = EX_BADPARAM;
      sprintf(errmsg, "Error: object type %d not supported; file id %d",
        obj_type, exoid);
      ex_err("ex_get_prop_names",errmsg,EX_BADPARAM);
      return(EX_FATAL);
    }

    if ((status = nc_inq_varid(exoid, var_name, &propid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to locate property array %s in file id %d",
        var_name, exoid);
      ex_err("ex_get_prop_names",errmsg,exerrval);
      return (EX_FATAL);
    }

    /*   for each property, read the "name" attribute of property array variable */
    if ((status = nc_inq_att(exoid, propid, ATT_PROP_NAME, &att_type, &att_len)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to get property attributes (type, len) in file id %d", exoid);
      ex_err("ex_get_prop_names",errmsg,exerrval);
      return (EX_FATAL);
    }

    if (att_len-1 <= ex_max_name_length) {
      /* Client has large enough char string to hold text... */
      if ((status = nc_get_att_text(exoid, propid, ATT_PROP_NAME, prop_names[i])) != NC_NOERR) {
  exerrval = status;
  sprintf(errmsg,
    "Error: failed to get property name in file id %d", exoid);
  ex_err("ex_get_prop_names",errmsg,exerrval);
  return (EX_FATAL);
      }
    }
    else {
      /* FIXME */
      exerrval = NC_ESTS;
      sprintf(errmsg,
        "Error: property name length exceeds space available to store it in file id %d", exoid);
      ex_err("ex_get_prop_names",errmsg,exerrval);
      return (EX_FATAL);
    }
  }
  return (EX_NOERR);
}
