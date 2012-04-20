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

#ifndef ITKMetaIO_METAIMAGETYPES_H
#define ITKMetaIO_METAIMAGETYPES_H

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

typedef enum { MET_MOD_CT, MET_MOD_MR, MET_MOD_NM, MET_MOD_US, MET_MOD_OTHER,
       MET_MOD_UNKNOWN } MET_ImageModalityEnumType;

#define MET_NUM_IMAGE_MODALITY_TYPES 6

const char MET_ImageModalityTypeName[MET_NUM_IMAGE_MODALITY_TYPES][17] = {
   {'M','E','T','_','M','O','D','_','C','T','\0',' ',' ',' ',' ',' ',' '},
   {'M','E','T','_','M','O','D','_','M','R','\0',' ',' ',' ',' ',' ',' '},
   {'M','E','T','_','M','O','D','_','N','M','\0',' ',' ',' ',' ',' ',' '},
   {'M','E','T','_','M','O','D','_','U','S','\0',' ',' ',' ',' ',' ',' '},
   {'M','E','T','_','M','O','D','_','O','T','H','E','R','\0',' ',' ',' '},
   {'M','E','T','_','M','O','D','_','U','N','K','N','O','W','N','\0',' '}};

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif
