/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5. The full HDF5 copyright notice, including      *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic document set and is     *
 * linked from the top-level documents page.  It can also be found at        *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have access   *
 * to either file, you may request a copy from help@hdfgroup.org.            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Programmer:  Raymond Lu <songyulu@hdfgroup.org>
 *              13 February 2013
 */
#ifndef _H5PLextern_H
#define _H5PLextern_H

/* Include HDF5 header */
#include "hdf5.h"

#ifndef H5_VMS

/*******************/
/* Public Typedefs */
/*******************/

/* Plugin type */
typedef enum H5PL_type_t {
    H5PL_TYPE_ERROR        = -1,  /*error                    */
    H5PL_TYPE_FILTER       = 0,   /*filter                   */
    H5PL_TYPE_NONE         = 1    /*this must be last!       */
} H5PL_type_t;


#ifdef H5_BUILT_AS_DYNAMIC_LIB

  #if defined (hdf5_EXPORTS)
    /* hdf5 library imports from plugin */
    #if defined (_MSC_VER)  /* MSVC Compiler Case */
      #define H5PLUGIN_DLL __declspec(dllimport)
    #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
      #define H5PLUGIN_DLL __attribute__ ((visibility("default")))
    #endif
  #else
    /* plugins always export */
    #if defined (_MSC_VER)  /* MSVC Compiler Case */
      #define H5PLUGIN_DLL __declspec(dllexport)
    #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
      #define H5PLUGIN_DLL __attribute__ ((visibility("default")))
    #endif
  #endif

#elif defined(H5_BUILT_AS_STATIC_LIB)
  #define H5PLUGIN_DLL
#else

  #if defined(H5_HAVE_WIN32_API)
    #if defined(_HDF5DLL_)
      #pragma warning(disable: 4273)  /* Disable the dll linkage warnings */
      #define H5PLUGIN_DLL __declspec(dllimport)
    #elif defined(_HDF5USEDLL_)
      #define H5PLUGIN_DLL __declspec(dllexport)
    #endif /* _HDF5DLL_ */
  #else /*H5_HAVE_WIN32_API*/
    #define H5PLUGIN_DLL
  #endif /*H5_HAVE_WIN32_API*/

#endif /* H5_BUILT_AS_xxx_LIB */

#ifdef __cplusplus
extern "C" {
#endif

H5PLUGIN_DLL H5PL_type_t H5PLget_plugin_type(void);
H5PLUGIN_DLL const void *H5PLget_plugin_info(void);

#ifdef __cplusplus
}
#endif
#endif /*H5_VMS*/

#endif /* _H5PLextern_H */

