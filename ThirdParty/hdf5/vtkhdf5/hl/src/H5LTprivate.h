/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef _H5LTprivate_H
#define _H5LTprivate_H

/* High-level library internal header file */
#include "H5HLprivate2.h"

/* public LT prototypes			*/
#include "H5LTpublic.h"

/*-------------------------------------------------------------------------
 * Private functions
 *-------------------------------------------------------------------------
 */

H5_HLDLL herr_t  H5LT_get_attribute_disk( hid_t obj_id,
                           const char *attr_name,
                           void *data );

H5_HLDLL herr_t  H5LT_set_attribute_numerical( hid_t loc_id,
                                     const char *obj_name,
                                     const char *attr_name,
                                     size_t size,
                                     hid_t type_id,
                                     const void *data );

H5_HLDLL herr_t  H5LT_set_attribute_string( hid_t dset_id,
                                 const char *name,
                                 const char *buf );

H5_HLDLL herr_t  H5LT_find_attribute( hid_t loc_id, const char *name );


H5_HLDLL char* H5LT_dtype_to_text(hid_t dtype, char *dt_str, H5LT_lang_t lang,
                                    size_t *slen, hbool_t no_user_buf);

H5_HLDLL int H5LTyyparse(void);

#endif
