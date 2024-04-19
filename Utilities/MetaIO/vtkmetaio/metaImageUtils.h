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
#  define ITKMetaIO_METAIMAGEUTILS_H

#  include "metaImageTypes.h"

#  include <memory>

#  if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#  endif

METAIO_EXPORT bool
MET_StringToImageModality(const std::string& _str, MET_ImageModalityEnumType * _type);

METAIO_EXPORT bool
MET_ImageModalityToString(MET_ImageModalityEnumType _type, std::string & _str);

// C++11 friendly wrapper of snprintf
template <typename... Args>
std::string
string_format(const std::string & format, Args... args)
{
  auto                  size = static_cast<size_t>(snprintf(nullptr, 0, format.c_str(), args...) + 1); // Extra space for '\0'
  std::unique_ptr<char[]> buf(new char[size]);
  snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

#  if (METAIO_USE_NAMESPACE)
};
#  endif

#endif
