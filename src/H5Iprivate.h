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

/*-----------------------------------------------------------------------------
 * File:    H5Iprivate.h
 * Purpose: header file for ID API
 *---------------------------------------------------------------------------*/

/* avoid re-inclusion */
#ifndef _H5Iprivate_H
#define _H5Iprivate_H

/* Include package's public header */
#include "H5Ipublic.h"

/* Private headers needed by this file */
#include "H5private.h"

/* Macro to determine if a H5I_type_t is a "library type" */
#define H5I_IS_LIB_TYPE( type ) (type > 0 && type < H5I_NTYPES)

/* Default sizes of the hash-tables for various atom types */
#define H5I_ERRSTACK_HASHSIZE		64
#define H5I_FILEID_HASHSIZE		64
#define H5I_TEMPID_HASHSIZE		64
#define H5I_DATATYPEID_HASHSIZE		64
#define H5I_DATASPACEID_HASHSIZE	64
#define H5I_DATASETID_HASHSIZE		64
#define H5I_OID_HASHSIZE		64
#define H5I_GROUPID_HASHSIZE		64
#define H5I_ATTRID_HASHSIZE		64
#define H5I_REFID_HASHSIZE		64
#define H5I_VFL_HASHSIZE		64
#define H5I_GENPROPCLS_HASHSIZE		64
#define H5I_GENPROPOBJ_HASHSIZE		128
#define H5I_ERRCLS_HASHSIZE		64
#define H5I_ERRMSG_HASHSIZE		64
#define H5I_ERRSTK_HASHSIZE		64

/* Private Functions in H5I.c */
H5_DLL H5I_type_t H5I_register_type(H5I_type_t type_id, size_t hash_size, unsigned reserved, H5I_free_t free_func);
H5_DLL int H5I_nmembers(H5I_type_t type);
H5_DLL herr_t H5I_clear_type(H5I_type_t type, hbool_t force, hbool_t app_ref);
H5_DLL int H5I_destroy_type(H5I_type_t type);
H5_DLL hid_t H5I_register(H5I_type_t type, const void *object, hbool_t app_ref);
H5_DLL void *H5I_subst(hid_t id, const void *new_object);
H5_DLL void *H5I_object(hid_t id);
H5_DLL void *H5I_object_verify(hid_t id, H5I_type_t id_type);
H5_DLL H5I_type_t H5I_get_type(hid_t id);
H5_DLL hid_t H5I_get_file_id(hid_t obj_id, hbool_t app_ref);
H5_DLL void *H5I_remove(hid_t id);
H5_DLL void *H5I_remove_verify(hid_t id, H5I_type_t id_type);
H5_DLL void *H5I_search(H5I_type_t type, H5I_search_func_t func, void *key, hbool_t app_ref);
H5_DLL int H5I_get_ref(hid_t id, hbool_t app_ref);
H5_DLL int H5I_inc_ref(hid_t id, hbool_t app_ref);
H5_DLL int H5I_dec_ref(hid_t id);
H5_DLL int H5I_dec_app_ref(hid_t id);
H5_DLL int H5I_dec_app_ref_always_close(hid_t id);
H5_DLL int H5I_inc_type_ref(H5I_type_t type);
H5_DLL herr_t H5I_dec_type_ref(H5I_type_t type);
H5_DLL int H5I_get_type_ref(H5I_type_t type);

#endif /* _H5Iprivate_H */

