/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5. The full HDF5 copyright notice, including      *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Programmer:  Raymond Lu <songyulu@hdfgroup.org>
 *              13 February 2013
 */

#ifndef _H5PLpublic_H
#define _H5PLpublic_H

/* Public headers needed by this file */
#include "H5public.h"          /* Generic Functions                    */

/*******************/
/* Public Typedefs */
/*******************/

/* Plugin type used by the plugin library */
typedef enum H5PL_type_t {
    H5PL_TYPE_ERROR        = -1,  /*error                    */
    H5PL_TYPE_FILTER       = 0,   /*filter                   */
    H5PL_TYPE_NONE         = 1    /*this must be last!       */
} H5PL_type_t;

/* Common dynamic plugin type flags used by the set/get_loading_state functions */
#define H5PL_FILTER_PLUGIN 0x0001
#define H5PL_ALL_PLUGIN 0xFFFF

#ifdef __cplusplus
extern "C" {
#endif

/* plugin state */
H5_DLL herr_t H5PLset_loading_state(unsigned int plugin_type);
H5_DLL herr_t H5PLget_loading_state(unsigned int *plugin_type/*out*/);
H5_DLL herr_t H5PLappend(const char *plugin_path);
H5_DLL herr_t H5PLprepend(const char *plugin_path);
H5_DLL herr_t H5PLreplace(const char *plugin_path, unsigned int index);
H5_DLL herr_t H5PLinsert(const char *plugin_path, unsigned int index);
H5_DLL herr_t H5PLremove(unsigned int index);
H5_DLL ssize_t H5PLget(unsigned int index, char *pathname/*out*/, size_t size);
H5_DLL herr_t H5PLsize(unsigned int *listsize/*out*/);

#ifdef __cplusplus
}
#endif

#endif /* _H5PLpublic_H */

