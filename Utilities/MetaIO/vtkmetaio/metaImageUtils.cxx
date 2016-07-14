/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include <stdio.h>

#ifdef _MSC_VER
#pragma warning(disable:4702)
#pragma warning(disable:4996)
#endif


#include "metaImageUtils.h"
#include <string.h>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif


bool MET_StringToImageModality(const char * _str,
                               MET_ImageModalityEnumType * _type)
  {
  int i;

  for(i=0; i<MET_NUM_IMAGE_MODALITY_TYPES; i++)
    if(!strcmp(MET_ImageModalityTypeName[i], _str))
      {
      *_type = (MET_ImageModalityEnumType)i;
      return true;
      }

  *_type = MET_MOD_UNKNOWN;

  return false;
  }

bool MET_ImageModalityToString(MET_ImageModalityEnumType _type,
                               char * _str)
  {
  strcpy(_str, MET_ImageModalityTypeName[(int)_type]);
  return true;
  }

#if (METAIO_USE_NAMESPACE)
};
#endif
