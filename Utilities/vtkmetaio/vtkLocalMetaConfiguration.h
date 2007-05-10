#ifdef METAIO_USE_NAMESPACE
  #undef METAIO_USE_NAMESPACE 
#endif
#ifdef METAIO_NAMESPACE
  #undef METAIO_NAMESPACE 
#endif
#ifdef METAIO_STD
  #undef METAIO_STD 
#endif

#include "vtkConfigure.h"

#define METAIO_USE_NAMESPACE  1
#define METAIO_NAMESPACE      vtkmetaio

#ifdef VTK_NO_STD_NAMESPACE
  #define METAIO_STL
#else
  #define METAIO_STL  std
#endif

#ifdef VTK_USE_ANSI_STDLIB
  #define METAIO_STREAM std
  #include <iostream>
  #include <fstream>
#else
  #define METAIO_STREAM 
  #include <iostream.h>
  #include <fstream.h>
#endif

#ifdef METAIO_EXPORT
  #undef METAIO_EXPORT
#endif 

#if ((defined(_WIN32) || defined(WIN32)) && defined(vtkmetaio_BUILD_SHARED_LIBS))
  #ifdef vtkmetaio_EXPORTS
    #define METAIO_EXPORT __declspec(dllexport)
    #define METAIO_EXTERN
  #else
    #define METAIO_EXPORT __declspec(dllimport)
    #define METAIO_EXTERN extern
  #endif
#else
  #define METAIO_EXPORT 
#endif

