#ifndef __proj_config_h
#define __proj_config_h

#include "vtk_libproj4_mangle.h"

#define PROJ_VERSION_MAJOR @PROJ_VERSION_MAJOR@
#define PROJ_VERSION_MINOR @PROJ_VERSION_MINOR@
#define PROJ_VERSION_PATCH @PROJ_VERSION_PATCH@

#define PROJ_VERSION_STRING "@PROJ_VERSION@"

#cmakedefine PROJ_LIST_EXTERNAL
#cmakedefine PROJ_USE_PTHREADS
#cmakedefine PROJ_USE_GSL
#ifdef PROJ_USE_GSL
#  define PROJ_HAVE_GSL
#endif /* PROJ_USE_GSL */

#cmakedefine PROJ_HAVE_COMPLEX
#cmakedefine PROJ_HAVE_ATANH

#ifdef BUILD_SHARED_LIBS
#  define PROJ_TMP_SHARED_LIBS BUILD_SHARED_LIBS
#  undef BUILD_SHARED_LIBS
#endif
#cmakedefine BUILD_SHARED_LIBS
#ifdef BUILD_SHARED_LIBS
#  ifndef PROJ_SHARED
#    define PROJ_SHARED
#  endif /* PROJ_SHARED */
#endif /* BUILD_SHARED_LIBS */
#undef BUILD_SHARED_LIBS
#ifdef PROJ_TMP_SHARED_LIBS
#  define BUILD_SHARED_LIBS PROJ_TMP_SHARED_LIBS
#endif

#if ( defined(WIN32) || defined(_WIN32) ) && defined(PROJ_SHARED)
#  if defined(vtkproj4_EXPORTS)
#    define PROJ_EXPORT __declspec( dllexport )
#  else
#    define PROJ_EXPORT __declspec( dllimport ) 
#  endif
#else
#  define PROJ_EXPORT
#endif

#endif /* __proj_config_h */
