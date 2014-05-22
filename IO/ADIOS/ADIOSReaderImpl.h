/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ADIOSReaderImpl.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef _ADIOSReaderImpl_h
#define _ADIOSReaderImpl_h

#include <map>
#include <utility>
#include <vector>

#include <adios_read.h>

#include "ADIOSReader.h"
#include "ADIOSVarInfo.h"
#include "ADIOSScalar.h"
#include "ADIOSAttribute.h"

struct ADIOSReader::ADIOSReaderImpl
{
  ADIOSReaderImpl(void)
  : File(NULL)
  { }

  ADIOS_FILE* File;

  std::pair<int, int> StepRange;
  std::vector<ADIOSAttribute*> Attributes;
  std::vector<ADIOSScalar*> Scalars;
  std::vector<ADIOSVarInfo*> Arrays;
  std::map<std::string, int> ArrayIds;
};
#endif
// VTK-HeaderTest-Exclude: ADIOSReaderImpl.h
