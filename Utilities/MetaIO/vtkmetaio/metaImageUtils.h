/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "metaTypes.h"

#ifndef ITKMetaIO_METAIMAGEUTILS_H
#define ITKMetaIO_METAIMAGEUTILS_H

#include "metaImageTypes.h"

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

METAIO_EXPORT bool MET_StringToImageModality(const char * _str,
                                      MET_ImageModalityEnumType * _type);

METAIO_EXPORT bool MET_ImageModalityToString(MET_ImageModalityEnumType _type,
                                      char * _str);

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif
