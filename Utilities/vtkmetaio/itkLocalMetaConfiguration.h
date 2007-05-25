#ifdef METAIO_USE_NAMESPACE
  #undef METAIO_USE_NAMESPACE 
#endif
#ifdef METAIO_NAMESPACE
  #undef METAIO_NAMESPACE 
#endif
#ifdef METAIO_STL
  #undef METAIO_STL 
#endif
#ifdef METAIO_EXPORT
  #undef METAIO_EXPORT
#endif 

#include "itk_zlib.h"

#define METAIO_USE_NAMESPACE  0
#define METAIO_NAMESPACE      ITKMetaIO

#define METAIO_STL    std

#ifndef METAIO_STREAM
#define METAIO_STREAM std
#endif

#include <iostream>
#include <fstream>

#ifndef METAIO_EXPORT
  #define METAIO_EXPORT 
#endif

