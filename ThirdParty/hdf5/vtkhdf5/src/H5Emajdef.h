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

#ifndef H5Emajdef_H
#define H5Emajdef_H

/***********************************/
/* Major error message definitions */
/***********************************/

/* clang-format off */
static const H5E_msg_t H5E_ARGS_msg_s = {false, "Invalid arguments to routine", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_ATTR_msg_s = {false, "Attribute", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_BTREE_msg_s = {false, "B-Tree node", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CACHE_msg_s = {false, "Object cache", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CONTEXT_msg_s = {false, "API Context", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_DATASET_msg_s = {false, "Dataset", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_DATASPACE_msg_s = {false, "Dataspace", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_DATATYPE_msg_s = {false, "Datatype", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_EARRAY_msg_s = {false, "Extensible Array", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_EFL_msg_s = {false, "External file list", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_ERROR_msg_s = {false, "Error API", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_EVENTSET_msg_s = {false, "Event Set", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_FARRAY_msg_s = {false, "Fixed Array", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_FILE_msg_s = {false, "File accessibility", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_FSPACE_msg_s = {false, "Free Space Manager", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_FUNC_msg_s = {false, "Function entry/exit", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_HEAP_msg_s = {false, "Heap", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_ID_msg_s = {false, "Object ID", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_INTERNAL_msg_s = {false, "Internal error (too specific to document in detail)", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_IO_msg_s = {false, "Low-level I/O", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_LIB_msg_s = {false, "General library infrastructure", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_LINK_msg_s = {false, "Links", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_MAP_msg_s = {false, "Map", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_NONE_MAJOR_msg_s = {false, "No error", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_OHDR_msg_s = {false, "Object header", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_PAGEBUF_msg_s = {false, "Page Buffering", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_PLINE_msg_s = {false, "Data filters", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_PLIST_msg_s = {false, "Property lists", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_PLUGIN_msg_s = {false, "Plugin for dynamically loaded library", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_REFERENCE_msg_s = {false, "References", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_RESOURCE_msg_s = {false, "Resource unavailable", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_RS_msg_s = {false, "Reference Counted Strings", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_SLIST_msg_s = {false, "Skip Lists", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_SOHM_msg_s = {false, "Shared Object Header Messages", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_STORAGE_msg_s = {false, "Data storage", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_SYM_msg_s = {false, "Symbol table", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_TST_msg_s = {false, "Ternary Search Trees", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_VFL_msg_s = {false, "Virtual File Layer", H5E_MAJOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_VOL_msg_s = {false, "Virtual Object Layer", H5E_MAJOR, &H5E_err_cls_s};
/* clang-format on */

#endif /* H5Emajdef_H */
