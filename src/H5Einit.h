/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Generated automatically by bin/make_err -- do not edit */
/* Add new errors to H5err.txt file */

#ifndef H5Einit_H
#define H5Einit_H

/*********************/
/* Major error codes */
/*********************/

/* H5E_ARGS */
assert(H5I_INVALID_HID == H5E_ARGS_g);
if((H5E_ARGS_g = H5I_register(H5I_ERROR_MSG, &H5E_ARGS_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Remember first major error code ID */
assert(H5E_first_maj_id_g==H5I_INVALID_HID);
H5E_first_maj_id_g = H5E_ARGS_g;

/* H5E_ATTR */
assert(H5I_INVALID_HID == H5E_ATTR_g);
if((H5E_ATTR_g = H5I_register(H5I_ERROR_MSG, &H5E_ATTR_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_BTREE */
assert(H5I_INVALID_HID == H5E_BTREE_g);
if((H5E_BTREE_g = H5I_register(H5I_ERROR_MSG, &H5E_BTREE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CACHE */
assert(H5I_INVALID_HID == H5E_CACHE_g);
if((H5E_CACHE_g = H5I_register(H5I_ERROR_MSG, &H5E_CACHE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CONTEXT */
assert(H5I_INVALID_HID == H5E_CONTEXT_g);
if((H5E_CONTEXT_g = H5I_register(H5I_ERROR_MSG, &H5E_CONTEXT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_DATASET */
assert(H5I_INVALID_HID == H5E_DATASET_g);
if((H5E_DATASET_g = H5I_register(H5I_ERROR_MSG, &H5E_DATASET_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_DATASPACE */
assert(H5I_INVALID_HID == H5E_DATASPACE_g);
if((H5E_DATASPACE_g = H5I_register(H5I_ERROR_MSG, &H5E_DATASPACE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_DATATYPE */
assert(H5I_INVALID_HID == H5E_DATATYPE_g);
if((H5E_DATATYPE_g = H5I_register(H5I_ERROR_MSG, &H5E_DATATYPE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_EARRAY */
assert(H5I_INVALID_HID == H5E_EARRAY_g);
if((H5E_EARRAY_g = H5I_register(H5I_ERROR_MSG, &H5E_EARRAY_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_EFL */
assert(H5I_INVALID_HID == H5E_EFL_g);
if((H5E_EFL_g = H5I_register(H5I_ERROR_MSG, &H5E_EFL_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_ERROR */
assert(H5I_INVALID_HID == H5E_ERROR_g);
if((H5E_ERROR_g = H5I_register(H5I_ERROR_MSG, &H5E_ERROR_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_EVENTSET */
assert(H5I_INVALID_HID == H5E_EVENTSET_g);
if((H5E_EVENTSET_g = H5I_register(H5I_ERROR_MSG, &H5E_EVENTSET_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_FARRAY */
assert(H5I_INVALID_HID == H5E_FARRAY_g);
if((H5E_FARRAY_g = H5I_register(H5I_ERROR_MSG, &H5E_FARRAY_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_FILE */
assert(H5I_INVALID_HID == H5E_FILE_g);
if((H5E_FILE_g = H5I_register(H5I_ERROR_MSG, &H5E_FILE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_FSPACE */
assert(H5I_INVALID_HID == H5E_FSPACE_g);
if((H5E_FSPACE_g = H5I_register(H5I_ERROR_MSG, &H5E_FSPACE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_FUNC */
assert(H5I_INVALID_HID == H5E_FUNC_g);
if((H5E_FUNC_g = H5I_register(H5I_ERROR_MSG, &H5E_FUNC_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_HEAP */
assert(H5I_INVALID_HID == H5E_HEAP_g);
if((H5E_HEAP_g = H5I_register(H5I_ERROR_MSG, &H5E_HEAP_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_ID */
assert(H5I_INVALID_HID == H5E_ID_g);
if((H5E_ID_g = H5I_register(H5I_ERROR_MSG, &H5E_ID_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_INTERNAL */
assert(H5I_INVALID_HID == H5E_INTERNAL_g);
if((H5E_INTERNAL_g = H5I_register(H5I_ERROR_MSG, &H5E_INTERNAL_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_IO */
assert(H5I_INVALID_HID == H5E_IO_g);
if((H5E_IO_g = H5I_register(H5I_ERROR_MSG, &H5E_IO_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_LIB */
assert(H5I_INVALID_HID == H5E_LIB_g);
if((H5E_LIB_g = H5I_register(H5I_ERROR_MSG, &H5E_LIB_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_LINK */
assert(H5I_INVALID_HID == H5E_LINK_g);
if((H5E_LINK_g = H5I_register(H5I_ERROR_MSG, &H5E_LINK_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_MAP */
assert(H5I_INVALID_HID == H5E_MAP_g);
if((H5E_MAP_g = H5I_register(H5I_ERROR_MSG, &H5E_MAP_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_NONE_MAJOR */
assert(H5I_INVALID_HID == H5E_NONE_MAJOR_g);
if((H5E_NONE_MAJOR_g = H5I_register(H5I_ERROR_MSG, &H5E_NONE_MAJOR_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_OHDR */
assert(H5I_INVALID_HID == H5E_OHDR_g);
if((H5E_OHDR_g = H5I_register(H5I_ERROR_MSG, &H5E_OHDR_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_PAGEBUF */
assert(H5I_INVALID_HID == H5E_PAGEBUF_g);
if((H5E_PAGEBUF_g = H5I_register(H5I_ERROR_MSG, &H5E_PAGEBUF_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_PLINE */
assert(H5I_INVALID_HID == H5E_PLINE_g);
if((H5E_PLINE_g = H5I_register(H5I_ERROR_MSG, &H5E_PLINE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_PLIST */
assert(H5I_INVALID_HID == H5E_PLIST_g);
if((H5E_PLIST_g = H5I_register(H5I_ERROR_MSG, &H5E_PLIST_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_PLUGIN */
assert(H5I_INVALID_HID == H5E_PLUGIN_g);
if((H5E_PLUGIN_g = H5I_register(H5I_ERROR_MSG, &H5E_PLUGIN_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_REFERENCE */
assert(H5I_INVALID_HID == H5E_REFERENCE_g);
if((H5E_REFERENCE_g = H5I_register(H5I_ERROR_MSG, &H5E_REFERENCE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_RESOURCE */
assert(H5I_INVALID_HID == H5E_RESOURCE_g);
if((H5E_RESOURCE_g = H5I_register(H5I_ERROR_MSG, &H5E_RESOURCE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_RS */
assert(H5I_INVALID_HID == H5E_RS_g);
if((H5E_RS_g = H5I_register(H5I_ERROR_MSG, &H5E_RS_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_SLIST */
assert(H5I_INVALID_HID == H5E_SLIST_g);
if((H5E_SLIST_g = H5I_register(H5I_ERROR_MSG, &H5E_SLIST_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_SOHM */
assert(H5I_INVALID_HID == H5E_SOHM_g);
if((H5E_SOHM_g = H5I_register(H5I_ERROR_MSG, &H5E_SOHM_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_STORAGE */
assert(H5I_INVALID_HID == H5E_STORAGE_g);
if((H5E_STORAGE_g = H5I_register(H5I_ERROR_MSG, &H5E_STORAGE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_SYM */
assert(H5I_INVALID_HID == H5E_SYM_g);
if((H5E_SYM_g = H5I_register(H5I_ERROR_MSG, &H5E_SYM_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_TST */
assert(H5I_INVALID_HID == H5E_TST_g);
if((H5E_TST_g = H5I_register(H5I_ERROR_MSG, &H5E_TST_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_VFL */
assert(H5I_INVALID_HID == H5E_VFL_g);
if((H5E_VFL_g = H5I_register(H5I_ERROR_MSG, &H5E_VFL_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_VOL */
assert(H5I_INVALID_HID == H5E_VOL_g);
if((H5E_VOL_g = H5I_register(H5I_ERROR_MSG, &H5E_VOL_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Remember last major error code ID */
assert(H5E_last_maj_id_g==H5I_INVALID_HID);
H5E_last_maj_id_g = H5E_VOL_g;


/*********************/
/* Minor error codes */
/*********************/


/* Argument errors */
/* H5E_BADRANGE */
assert(H5I_INVALID_HID == H5E_BADRANGE_g);
if((H5E_BADRANGE_g = H5I_register(H5I_ERROR_MSG, &H5E_BADRANGE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Remember first minor error code ID */
assert(H5E_first_min_id_g==H5I_INVALID_HID);
H5E_first_min_id_g = H5E_BADRANGE_g;

/* H5E_BADTYPE */
assert(H5I_INVALID_HID == H5E_BADTYPE_g);
if((H5E_BADTYPE_g = H5I_register(H5I_ERROR_MSG, &H5E_BADTYPE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_BADVALUE */
assert(H5I_INVALID_HID == H5E_BADVALUE_g);
if((H5E_BADVALUE_g = H5I_register(H5I_ERROR_MSG, &H5E_BADVALUE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_UNINITIALIZED */
assert(H5I_INVALID_HID == H5E_UNINITIALIZED_g);
if((H5E_UNINITIALIZED_g = H5I_register(H5I_ERROR_MSG, &H5E_UNINITIALIZED_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_UNSUPPORTED */
assert(H5I_INVALID_HID == H5E_UNSUPPORTED_g);
if((H5E_UNSUPPORTED_g = H5I_register(H5I_ERROR_MSG, &H5E_UNSUPPORTED_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Asynchronous operation errors */
/* H5E_CANTCANCEL */
assert(H5I_INVALID_HID == H5E_CANTCANCEL_g);
if((H5E_CANTCANCEL_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTCANCEL_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTWAIT */
assert(H5I_INVALID_HID == H5E_CANTWAIT_g);
if((H5E_CANTWAIT_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTWAIT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* B-tree related errors */
/* H5E_CANTDECODE */
assert(H5I_INVALID_HID == H5E_CANTDECODE_g);
if((H5E_CANTDECODE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTDECODE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTENCODE */
assert(H5I_INVALID_HID == H5E_CANTENCODE_g);
if((H5E_CANTENCODE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTENCODE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTFIND */
assert(H5I_INVALID_HID == H5E_CANTFIND_g);
if((H5E_CANTFIND_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTFIND_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTINSERT */
assert(H5I_INVALID_HID == H5E_CANTINSERT_g);
if((H5E_CANTINSERT_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTINSERT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTLIST */
assert(H5I_INVALID_HID == H5E_CANTLIST_g);
if((H5E_CANTLIST_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTLIST_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTMODIFY */
assert(H5I_INVALID_HID == H5E_CANTMODIFY_g);
if((H5E_CANTMODIFY_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTMODIFY_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTREDISTRIBUTE */
assert(H5I_INVALID_HID == H5E_CANTREDISTRIBUTE_g);
if((H5E_CANTREDISTRIBUTE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTREDISTRIBUTE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTREMOVE */
assert(H5I_INVALID_HID == H5E_CANTREMOVE_g);
if((H5E_CANTREMOVE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTREMOVE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTSPLIT */
assert(H5I_INVALID_HID == H5E_CANTSPLIT_g);
if((H5E_CANTSPLIT_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTSPLIT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTSWAP */
assert(H5I_INVALID_HID == H5E_CANTSWAP_g);
if((H5E_CANTSWAP_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTSWAP_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_EXISTS */
assert(H5I_INVALID_HID == H5E_EXISTS_g);
if((H5E_EXISTS_g = H5I_register(H5I_ERROR_MSG, &H5E_EXISTS_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_NOTFOUND */
assert(H5I_INVALID_HID == H5E_NOTFOUND_g);
if((H5E_NOTFOUND_g = H5I_register(H5I_ERROR_MSG, &H5E_NOTFOUND_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Cache related errors */
/* H5E_CANTCLEAN */
assert(H5I_INVALID_HID == H5E_CANTCLEAN_g);
if((H5E_CANTCLEAN_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTCLEAN_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTCORK */
assert(H5I_INVALID_HID == H5E_CANTCORK_g);
if((H5E_CANTCORK_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTCORK_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTDEPEND */
assert(H5I_INVALID_HID == H5E_CANTDEPEND_g);
if((H5E_CANTDEPEND_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTDEPEND_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTDIRTY */
assert(H5I_INVALID_HID == H5E_CANTDIRTY_g);
if((H5E_CANTDIRTY_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTDIRTY_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTEXPUNGE */
assert(H5I_INVALID_HID == H5E_CANTEXPUNGE_g);
if((H5E_CANTEXPUNGE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTEXPUNGE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTFLUSH */
assert(H5I_INVALID_HID == H5E_CANTFLUSH_g);
if((H5E_CANTFLUSH_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTFLUSH_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTINS */
assert(H5I_INVALID_HID == H5E_CANTINS_g);
if((H5E_CANTINS_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTINS_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTLOAD */
assert(H5I_INVALID_HID == H5E_CANTLOAD_g);
if((H5E_CANTLOAD_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTLOAD_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTMARKCLEAN */
assert(H5I_INVALID_HID == H5E_CANTMARKCLEAN_g);
if((H5E_CANTMARKCLEAN_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTMARKCLEAN_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTMARKDIRTY */
assert(H5I_INVALID_HID == H5E_CANTMARKDIRTY_g);
if((H5E_CANTMARKDIRTY_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTMARKDIRTY_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTMARKSERIALIZED */
assert(H5I_INVALID_HID == H5E_CANTMARKSERIALIZED_g);
if((H5E_CANTMARKSERIALIZED_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTMARKSERIALIZED_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTMARKUNSERIALIZED */
assert(H5I_INVALID_HID == H5E_CANTMARKUNSERIALIZED_g);
if((H5E_CANTMARKUNSERIALIZED_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTMARKUNSERIALIZED_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTNOTIFY */
assert(H5I_INVALID_HID == H5E_CANTNOTIFY_g);
if((H5E_CANTNOTIFY_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTNOTIFY_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTPIN */
assert(H5I_INVALID_HID == H5E_CANTPIN_g);
if((H5E_CANTPIN_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTPIN_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTPROTECT */
assert(H5I_INVALID_HID == H5E_CANTPROTECT_g);
if((H5E_CANTPROTECT_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTPROTECT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTRESIZE */
assert(H5I_INVALID_HID == H5E_CANTRESIZE_g);
if((H5E_CANTRESIZE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTRESIZE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTSERIALIZE */
assert(H5I_INVALID_HID == H5E_CANTSERIALIZE_g);
if((H5E_CANTSERIALIZE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTSERIALIZE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTTAG */
assert(H5I_INVALID_HID == H5E_CANTTAG_g);
if((H5E_CANTTAG_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTTAG_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTUNCORK */
assert(H5I_INVALID_HID == H5E_CANTUNCORK_g);
if((H5E_CANTUNCORK_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTUNCORK_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTUNDEPEND */
assert(H5I_INVALID_HID == H5E_CANTUNDEPEND_g);
if((H5E_CANTUNDEPEND_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTUNDEPEND_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTUNPIN */
assert(H5I_INVALID_HID == H5E_CANTUNPIN_g);
if((H5E_CANTUNPIN_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTUNPIN_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTUNPROTECT */
assert(H5I_INVALID_HID == H5E_CANTUNPROTECT_g);
if((H5E_CANTUNPROTECT_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTUNPROTECT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTUNSERIALIZE */
assert(H5I_INVALID_HID == H5E_CANTUNSERIALIZE_g);
if((H5E_CANTUNSERIALIZE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTUNSERIALIZE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_LOGGING */
assert(H5I_INVALID_HID == H5E_LOGGING_g);
if((H5E_LOGGING_g = H5I_register(H5I_ERROR_MSG, &H5E_LOGGING_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_NOTCACHED */
assert(H5I_INVALID_HID == H5E_NOTCACHED_g);
if((H5E_NOTCACHED_g = H5I_register(H5I_ERROR_MSG, &H5E_NOTCACHED_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_PROTECT */
assert(H5I_INVALID_HID == H5E_PROTECT_g);
if((H5E_PROTECT_g = H5I_register(H5I_ERROR_MSG, &H5E_PROTECT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_SYSTEM */
assert(H5I_INVALID_HID == H5E_SYSTEM_g);
if((H5E_SYSTEM_g = H5I_register(H5I_ERROR_MSG, &H5E_SYSTEM_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Dataspace errors */
/* H5E_BADSELECT */
assert(H5I_INVALID_HID == H5E_BADSELECT_g);
if((H5E_BADSELECT_g = H5I_register(H5I_ERROR_MSG, &H5E_BADSELECT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTAPPEND */
assert(H5I_INVALID_HID == H5E_CANTAPPEND_g);
if((H5E_CANTAPPEND_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTAPPEND_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTCLIP */
assert(H5I_INVALID_HID == H5E_CANTCLIP_g);
if((H5E_CANTCLIP_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTCLIP_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTCOMPARE */
assert(H5I_INVALID_HID == H5E_CANTCOMPARE_g);
if((H5E_CANTCOMPARE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTCOMPARE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTCOUNT */
assert(H5I_INVALID_HID == H5E_CANTCOUNT_g);
if((H5E_CANTCOUNT_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTCOUNT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTNEXT */
assert(H5I_INVALID_HID == H5E_CANTNEXT_g);
if((H5E_CANTNEXT_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTNEXT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTSELECT */
assert(H5I_INVALID_HID == H5E_CANTSELECT_g);
if((H5E_CANTSELECT_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTSELECT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_INCONSISTENTSTATE */
assert(H5I_INVALID_HID == H5E_INCONSISTENTSTATE_g);
if((H5E_INCONSISTENTSTATE_g = H5I_register(H5I_ERROR_MSG, &H5E_INCONSISTENTSTATE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Generic low-level file I/O errors */
/* H5E_CLOSEERROR */
assert(H5I_INVALID_HID == H5E_CLOSEERROR_g);
if((H5E_CLOSEERROR_g = H5I_register(H5I_ERROR_MSG, &H5E_CLOSEERROR_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_FCNTL */
assert(H5I_INVALID_HID == H5E_FCNTL_g);
if((H5E_FCNTL_g = H5I_register(H5I_ERROR_MSG, &H5E_FCNTL_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_OVERFLOW */
assert(H5I_INVALID_HID == H5E_OVERFLOW_g);
if((H5E_OVERFLOW_g = H5I_register(H5I_ERROR_MSG, &H5E_OVERFLOW_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_READERROR */
assert(H5I_INVALID_HID == H5E_READERROR_g);
if((H5E_READERROR_g = H5I_register(H5I_ERROR_MSG, &H5E_READERROR_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_SEEKERROR */
assert(H5I_INVALID_HID == H5E_SEEKERROR_g);
if((H5E_SEEKERROR_g = H5I_register(H5I_ERROR_MSG, &H5E_SEEKERROR_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_WRITEERROR */
assert(H5I_INVALID_HID == H5E_WRITEERROR_g);
if((H5E_WRITEERROR_g = H5I_register(H5I_ERROR_MSG, &H5E_WRITEERROR_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* File accessibility errors */
/* H5E_BADFILE */
assert(H5I_INVALID_HID == H5E_BADFILE_g);
if((H5E_BADFILE_g = H5I_register(H5I_ERROR_MSG, &H5E_BADFILE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTCLOSEFILE */
assert(H5I_INVALID_HID == H5E_CANTCLOSEFILE_g);
if((H5E_CANTCLOSEFILE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTCLOSEFILE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTCREATE */
assert(H5I_INVALID_HID == H5E_CANTCREATE_g);
if((H5E_CANTCREATE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTCREATE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTDELETEFILE */
assert(H5I_INVALID_HID == H5E_CANTDELETEFILE_g);
if((H5E_CANTDELETEFILE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTDELETEFILE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTLOCKFILE */
assert(H5I_INVALID_HID == H5E_CANTLOCKFILE_g);
if((H5E_CANTLOCKFILE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTLOCKFILE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTOPENFILE */
assert(H5I_INVALID_HID == H5E_CANTOPENFILE_g);
if((H5E_CANTOPENFILE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTOPENFILE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTUNLOCKFILE */
assert(H5I_INVALID_HID == H5E_CANTUNLOCKFILE_g);
if((H5E_CANTUNLOCKFILE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTUNLOCKFILE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_FILEEXISTS */
assert(H5I_INVALID_HID == H5E_FILEEXISTS_g);
if((H5E_FILEEXISTS_g = H5I_register(H5I_ERROR_MSG, &H5E_FILEEXISTS_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_FILEOPEN */
assert(H5I_INVALID_HID == H5E_FILEOPEN_g);
if((H5E_FILEOPEN_g = H5I_register(H5I_ERROR_MSG, &H5E_FILEOPEN_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_MOUNT */
assert(H5I_INVALID_HID == H5E_MOUNT_g);
if((H5E_MOUNT_g = H5I_register(H5I_ERROR_MSG, &H5E_MOUNT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_NOTHDF5 */
assert(H5I_INVALID_HID == H5E_NOTHDF5_g);
if((H5E_NOTHDF5_g = H5I_register(H5I_ERROR_MSG, &H5E_NOTHDF5_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_TRUNCATED */
assert(H5I_INVALID_HID == H5E_TRUNCATED_g);
if((H5E_TRUNCATED_g = H5I_register(H5I_ERROR_MSG, &H5E_TRUNCATED_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_UNMOUNT */
assert(H5I_INVALID_HID == H5E_UNMOUNT_g);
if((H5E_UNMOUNT_g = H5I_register(H5I_ERROR_MSG, &H5E_UNMOUNT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Free space errors */
/* H5E_CANTMERGE */
assert(H5I_INVALID_HID == H5E_CANTMERGE_g);
if((H5E_CANTMERGE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTMERGE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTREVIVE */
assert(H5I_INVALID_HID == H5E_CANTREVIVE_g);
if((H5E_CANTREVIVE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTREVIVE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTSHRINK */
assert(H5I_INVALID_HID == H5E_CANTSHRINK_g);
if((H5E_CANTSHRINK_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTSHRINK_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Function entry/exit interface errors */
/* H5E_ALREADYINIT */
assert(H5I_INVALID_HID == H5E_ALREADYINIT_g);
if((H5E_ALREADYINIT_g = H5I_register(H5I_ERROR_MSG, &H5E_ALREADYINIT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTINIT */
assert(H5I_INVALID_HID == H5E_CANTINIT_g);
if((H5E_CANTINIT_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTINIT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTRELEASE */
assert(H5I_INVALID_HID == H5E_CANTRELEASE_g);
if((H5E_CANTRELEASE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTRELEASE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Group related errors */
/* H5E_CANTCLOSEOBJ */
assert(H5I_INVALID_HID == H5E_CANTCLOSEOBJ_g);
if((H5E_CANTCLOSEOBJ_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTCLOSEOBJ_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTOPENOBJ */
assert(H5I_INVALID_HID == H5E_CANTOPENOBJ_g);
if((H5E_CANTOPENOBJ_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTOPENOBJ_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_COMPLEN */
assert(H5I_INVALID_HID == H5E_COMPLEN_g);
if((H5E_COMPLEN_g = H5I_register(H5I_ERROR_MSG, &H5E_COMPLEN_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_PATH */
assert(H5I_INVALID_HID == H5E_PATH_g);
if((H5E_PATH_g = H5I_register(H5I_ERROR_MSG, &H5E_PATH_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Heap errors */
/* H5E_CANTATTACH */
assert(H5I_INVALID_HID == H5E_CANTATTACH_g);
if((H5E_CANTATTACH_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTATTACH_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTCOMPUTE */
assert(H5I_INVALID_HID == H5E_CANTCOMPUTE_g);
if((H5E_CANTCOMPUTE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTCOMPUTE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTEXTEND */
assert(H5I_INVALID_HID == H5E_CANTEXTEND_g);
if((H5E_CANTEXTEND_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTEXTEND_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTOPERATE */
assert(H5I_INVALID_HID == H5E_CANTOPERATE_g);
if((H5E_CANTOPERATE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTOPERATE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTRESTORE */
assert(H5I_INVALID_HID == H5E_CANTRESTORE_g);
if((H5E_CANTRESTORE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTRESTORE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTUPDATE */
assert(H5I_INVALID_HID == H5E_CANTUPDATE_g);
if((H5E_CANTUPDATE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTUPDATE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Object ID related errors */
/* H5E_BADGROUP */
assert(H5I_INVALID_HID == H5E_BADGROUP_g);
if((H5E_BADGROUP_g = H5I_register(H5I_ERROR_MSG, &H5E_BADGROUP_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_BADID */
assert(H5I_INVALID_HID == H5E_BADID_g);
if((H5E_BADID_g = H5I_register(H5I_ERROR_MSG, &H5E_BADID_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTDEC */
assert(H5I_INVALID_HID == H5E_CANTDEC_g);
if((H5E_CANTDEC_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTDEC_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTINC */
assert(H5I_INVALID_HID == H5E_CANTINC_g);
if((H5E_CANTINC_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTINC_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTREGISTER */
assert(H5I_INVALID_HID == H5E_CANTREGISTER_g);
if((H5E_CANTREGISTER_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTREGISTER_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_NOIDS */
assert(H5I_INVALID_HID == H5E_NOIDS_g);
if((H5E_NOIDS_g = H5I_register(H5I_ERROR_MSG, &H5E_NOIDS_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Link related errors */
/* H5E_CANTMOVE */
assert(H5I_INVALID_HID == H5E_CANTMOVE_g);
if((H5E_CANTMOVE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTMOVE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTSORT */
assert(H5I_INVALID_HID == H5E_CANTSORT_g);
if((H5E_CANTSORT_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTSORT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_NLINKS */
assert(H5I_INVALID_HID == H5E_NLINKS_g);
if((H5E_NLINKS_g = H5I_register(H5I_ERROR_MSG, &H5E_NLINKS_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_NOTREGISTERED */
assert(H5I_INVALID_HID == H5E_NOTREGISTERED_g);
if((H5E_NOTREGISTERED_g = H5I_register(H5I_ERROR_MSG, &H5E_NOTREGISTERED_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_TRAVERSE */
assert(H5I_INVALID_HID == H5E_TRAVERSE_g);
if((H5E_TRAVERSE_g = H5I_register(H5I_ERROR_MSG, &H5E_TRAVERSE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Map related errors */
/* H5E_CANTPUT */
assert(H5I_INVALID_HID == H5E_CANTPUT_g);
if((H5E_CANTPUT_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTPUT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Parallel MPI errors */
/* H5E_CANTGATHER */
assert(H5I_INVALID_HID == H5E_CANTGATHER_g);
if((H5E_CANTGATHER_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTGATHER_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTRECV */
assert(H5I_INVALID_HID == H5E_CANTRECV_g);
if((H5E_CANTRECV_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTRECV_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_MPI */
assert(H5I_INVALID_HID == H5E_MPI_g);
if((H5E_MPI_g = H5I_register(H5I_ERROR_MSG, &H5E_MPI_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_MPIERRSTR */
assert(H5I_INVALID_HID == H5E_MPIERRSTR_g);
if((H5E_MPIERRSTR_g = H5I_register(H5I_ERROR_MSG, &H5E_MPIERRSTR_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_NO_INDEPENDENT */
assert(H5I_INVALID_HID == H5E_NO_INDEPENDENT_g);
if((H5E_NO_INDEPENDENT_g = H5I_register(H5I_ERROR_MSG, &H5E_NO_INDEPENDENT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* No error */
/* H5E_NONE_MINOR */
assert(H5I_INVALID_HID == H5E_NONE_MINOR_g);
if((H5E_NONE_MINOR_g = H5I_register(H5I_ERROR_MSG, &H5E_NONE_MINOR_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Object header related errors */
/* H5E_ALIGNMENT */
assert(H5I_INVALID_HID == H5E_ALIGNMENT_g);
if((H5E_ALIGNMENT_g = H5I_register(H5I_ERROR_MSG, &H5E_ALIGNMENT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_BADITER */
assert(H5I_INVALID_HID == H5E_BADITER_g);
if((H5E_BADITER_g = H5I_register(H5I_ERROR_MSG, &H5E_BADITER_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_BADMESG */
assert(H5I_INVALID_HID == H5E_BADMESG_g);
if((H5E_BADMESG_g = H5I_register(H5I_ERROR_MSG, &H5E_BADMESG_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTDELETE */
assert(H5I_INVALID_HID == H5E_CANTDELETE_g);
if((H5E_CANTDELETE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTDELETE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTPACK */
assert(H5I_INVALID_HID == H5E_CANTPACK_g);
if((H5E_CANTPACK_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTPACK_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTRENAME */
assert(H5I_INVALID_HID == H5E_CANTRENAME_g);
if((H5E_CANTRENAME_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTRENAME_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTRESET */
assert(H5I_INVALID_HID == H5E_CANTRESET_g);
if((H5E_CANTRESET_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTRESET_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_LINKCOUNT */
assert(H5I_INVALID_HID == H5E_LINKCOUNT_g);
if((H5E_LINKCOUNT_g = H5I_register(H5I_ERROR_MSG, &H5E_LINKCOUNT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_VERSION */
assert(H5I_INVALID_HID == H5E_VERSION_g);
if((H5E_VERSION_g = H5I_register(H5I_ERROR_MSG, &H5E_VERSION_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* I/O pipeline errors */
/* H5E_CALLBACK */
assert(H5I_INVALID_HID == H5E_CALLBACK_g);
if((H5E_CALLBACK_g = H5I_register(H5I_ERROR_MSG, &H5E_CALLBACK_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANAPPLY */
assert(H5I_INVALID_HID == H5E_CANAPPLY_g);
if((H5E_CANAPPLY_g = H5I_register(H5I_ERROR_MSG, &H5E_CANAPPLY_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTFILTER */
assert(H5I_INVALID_HID == H5E_CANTFILTER_g);
if((H5E_CANTFILTER_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTFILTER_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_NOENCODER */
assert(H5I_INVALID_HID == H5E_NOENCODER_g);
if((H5E_NOENCODER_g = H5I_register(H5I_ERROR_MSG, &H5E_NOENCODER_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_NOFILTER */
assert(H5I_INVALID_HID == H5E_NOFILTER_g);
if((H5E_NOFILTER_g = H5I_register(H5I_ERROR_MSG, &H5E_NOFILTER_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_SETLOCAL */
assert(H5I_INVALID_HID == H5E_SETLOCAL_g);
if((H5E_SETLOCAL_g = H5I_register(H5I_ERROR_MSG, &H5E_SETLOCAL_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Property list errors */
/* H5E_CANTGET */
assert(H5I_INVALID_HID == H5E_CANTGET_g);
if((H5E_CANTGET_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTGET_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTSET */
assert(H5I_INVALID_HID == H5E_CANTSET_g);
if((H5E_CANTSET_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTSET_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_DUPCLASS */
assert(H5I_INVALID_HID == H5E_DUPCLASS_g);
if((H5E_DUPCLASS_g = H5I_register(H5I_ERROR_MSG, &H5E_DUPCLASS_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_SETDISALLOWED */
assert(H5I_INVALID_HID == H5E_SETDISALLOWED_g);
if((H5E_SETDISALLOWED_g = H5I_register(H5I_ERROR_MSG, &H5E_SETDISALLOWED_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Plugin errors */
/* H5E_OPENERROR */
assert(H5I_INVALID_HID == H5E_OPENERROR_g);
if((H5E_OPENERROR_g = H5I_register(H5I_ERROR_MSG, &H5E_OPENERROR_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Resource errors */
/* H5E_ALREADYEXISTS */
assert(H5I_INVALID_HID == H5E_ALREADYEXISTS_g);
if((H5E_ALREADYEXISTS_g = H5I_register(H5I_ERROR_MSG, &H5E_ALREADYEXISTS_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTALLOC */
assert(H5I_INVALID_HID == H5E_CANTALLOC_g);
if((H5E_CANTALLOC_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTALLOC_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTCOPY */
assert(H5I_INVALID_HID == H5E_CANTCOPY_g);
if((H5E_CANTCOPY_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTCOPY_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTFREE */
assert(H5I_INVALID_HID == H5E_CANTFREE_g);
if((H5E_CANTFREE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTFREE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTGC */
assert(H5I_INVALID_HID == H5E_CANTGC_g);
if((H5E_CANTGC_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTGC_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTGETSIZE */
assert(H5I_INVALID_HID == H5E_CANTGETSIZE_g);
if((H5E_CANTGETSIZE_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTGETSIZE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTLOCK */
assert(H5I_INVALID_HID == H5E_CANTLOCK_g);
if((H5E_CANTLOCK_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTLOCK_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTUNLOCK */
assert(H5I_INVALID_HID == H5E_CANTUNLOCK_g);
if((H5E_CANTUNLOCK_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTUNLOCK_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_NOSPACE */
assert(H5I_INVALID_HID == H5E_NOSPACE_g);
if((H5E_NOSPACE_g = H5I_register(H5I_ERROR_MSG, &H5E_NOSPACE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_OBJOPEN */
assert(H5I_INVALID_HID == H5E_OBJOPEN_g);
if((H5E_OBJOPEN_g = H5I_register(H5I_ERROR_MSG, &H5E_OBJOPEN_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* System level errors */
/* H5E_SYSERRSTR */
assert(H5I_INVALID_HID == H5E_SYSERRSTR_g);
if((H5E_SYSERRSTR_g = H5I_register(H5I_ERROR_MSG, &H5E_SYSERRSTR_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Datatype conversion errors */
/* H5E_BADSIZE */
assert(H5I_INVALID_HID == H5E_BADSIZE_g);
if((H5E_BADSIZE_g = H5I_register(H5I_ERROR_MSG, &H5E_BADSIZE_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");
/* H5E_CANTCONVERT */
assert(H5I_INVALID_HID == H5E_CANTCONVERT_g);
if((H5E_CANTCONVERT_g = H5I_register(H5I_ERROR_MSG, &H5E_CANTCONVERT_msg_s, false)) < 0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message");

/* Remember last minor error code ID */
assert(H5E_last_min_id_g==H5I_INVALID_HID);
H5E_last_min_id_g = H5E_CANTCONVERT_g;

#endif /* H5Einit_H */
