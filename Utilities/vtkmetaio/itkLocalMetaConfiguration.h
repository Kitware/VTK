#ifdef METAIO_USE_NAMESPACE
  #undef METAIO_USE_NAMESPACE 
#endif
#ifdef METAIO_NAMESPACE
  #undef METAIO_NAMESPACE 
#endif

#include "itk_zlib.h"

#define METAIO_USE_NAMESPACE  0
#define METAIO_NAMESPACE      ITKMetaIO

#ifndef METAIO_STL
  #define METAIO_STL    std
#endif

#ifndef METAIO_STD
  #define METAIO_STREAM std
#endif

#include <iostream>
#include <fstream>

#ifndef METAIO_EXPORT
  #define METAIO_EXPORT 
#endif 


