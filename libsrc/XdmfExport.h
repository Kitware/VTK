#ifndef __xdmf_export_h
#define __xdmf_export_h

#include "XdmfConfig.h"

#if defined(_WIN32) && !defined(WIN32)
# define WIN32
#endif

#if defined(WIN32) && !defined(XDMFSTATIC)
#  if defined(Xdmf_EXPORTS)
#    define XDMF_EXPORT __declspec( dllexport ) 
#  else
#    define XDMF_EXPORT __declspec( dllimport ) 
#  endif
#else
#  define XDMF_EXPORT
#endif

#endif
