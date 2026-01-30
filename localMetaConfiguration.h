/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#ifdef METAIO_USE_NAMESPACE
#  undef METAIO_USE_NAMESPACE
#endif
#ifdef METAIO_NAMESPACE
#  undef METAIO_NAMESPACE
#endif
#ifdef METAIO_EXPORT
#  undef METAIO_EXPORT
#endif

#include "metaIOConfig.h"

#if defined(METAIO_FOR_ITK)
// ITK

#  define METAIO_USE_NAMESPACE 0
#  define METAIO_NAMESPACE ITKMetaIO
#  define METAIO_STREAM itksys

#  include <itksys/FStream.hxx>
#  include <itk_zlib.h>

#  include <iostream>
#  include <fstream>

#  if defined(_WIN32) && defined(itkmetaio_BUILD_SHARED_LIBS)
#    ifdef metaio_EXPORTS
#      define METAIO_EXPORT __declspec(dllexport)
#      define METAIO_EXTERN
#    else
#      define METAIO_EXPORT __declspec(dllimport)
#      define METAIO_EXTERN extern
#    endif
#  else
#    if defined(itkmetaio_BUILD_SHARED_LIBS)
#      define METAIO_EXPORT __attribute__((visibility ("default")))
#    else
#      define METAIO_EXPORT
#    endif
#  endif

#elif defined(METAIO_FOR_VTK)
// VTK

#  define METAIO_USE_NAMESPACE 1
#  define METAIO_NAMESPACE vtkmetaio
#  define METAIO_STREAM vtksys

#  include <vtksys/FStream.hxx>
#  include <vtk_zlib.h>

#  include <iostream>
#  include <fstream>

#  if defined(_WIN32) && defined(vtkmetaio_BUILD_SHARED_LIBS)
#    ifdef metaio_EXPORTS
#      define METAIO_EXPORT __declspec(dllexport)
#      define METAIO_EXTERN
#    else
#      define METAIO_EXPORT __declspec(dllimport)
#      define METAIO_EXTERN extern
#    endif
#  else
#    if defined(vtkmetaio_BUILD_SHARED_LIBS)
#      define METAIO_EXPORT __attribute__((visibility ("default")))
#    else
#      define METAIO_EXPORT
#    endif
#  endif

#else
// Independent of ITK and VTK

#  define METAIO_USE_NAMESPACE 0
#  define METAIO_NAMESPACE metaio
#  define METAIO_STREAM std

#  include "itk_zlib.h"

#  include <iostream>
#  include <fstream>

#  if defined(_WIN32) && defined(metaio_BUILD_SHARED_LIBS)
#    ifdef metaio_EXPORTS
#      define METAIO_EXPORT __declspec(dllexport)
#      define METAIO_EXTERN
#    else
#      define METAIO_EXPORT __declspec(dllimport)
#      define METAIO_EXTERN extern
#    endif
#  else
#    if defined(metaio_BUILD_SHARED_LIBS)
#      define METAIO_EXPORT __attribute__((visibility ("default")))
#    else
#      define METAIO_EXPORT
#    endif
#  endif

#endif
