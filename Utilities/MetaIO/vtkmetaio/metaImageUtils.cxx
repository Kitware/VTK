/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/

#ifdef _MSC_VER
#  pragma warning(disable : 4702)
#  pragma warning(disable : 4996)
#endif


#include "metaImageUtils.h"

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#endif


bool
MET_StringToImageModality(const std::string& _str, MET_ImageModalityEnumType * _type)
{
  int i;

  for (i = 0; i < MET_NUM_IMAGE_MODALITY_TYPES; i++)
  {
    if (MET_ImageModalityTypeName[i] == _str)
    {
      *_type = static_cast<MET_ImageModalityEnumType>(i);
      return true;
    }
  }

  *_type = MET_MOD_UNKNOWN;

  return false;
}

bool
MET_ImageModalityToString(MET_ImageModalityEnumType _type, std::string & _str)
{
  _str = MET_ImageModalityTypeName[static_cast<int>(_type)];
  return true;
}

#if (METAIO_USE_NAMESPACE)
};
#endif
