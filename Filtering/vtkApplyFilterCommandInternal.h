/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkApplyFilterCommandInternal.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkApplyFilterCommandInternal_h
#define __vtkApplyFilterCommandInternal_h

#include <vtkstd/map>
#include <vtkstd/vector>

#include "vtkStdString.h"

class vtkApplyFilterCommandInternal
{
public:
  typedef vtkstd::vector<vtkStdString> FilterTypesVector;
  typedef vtkstd::map<vtkStdString, FilterTypesVector> FilterTypesMap;

  FilterTypesMap FilterTypes;
};


#endif
