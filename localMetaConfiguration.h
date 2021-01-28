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

#if defined(METAIO_FOR_ITK) || !defined(METAIO_FOR_VTK)
// ITK

#  define METAIO_USE_NAMESPACE 0
#  define METAIO_NAMESPACE ITKMetaIO

#  include "itk_zlib.h"

#  include <iostream>
#  include <fstream>

#  define METAIO_EXPORT

#else
// VTK

#  define METAIO_USE_NAMESPACE 1
#  define METAIO_NAMESPACE vtkmetaio

#  include "vtk_zlib.h"

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
#    define METAIO_EXPORT
#  endif

// end VTK/ITK
#endif
