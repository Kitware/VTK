/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoJSONProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGeoJSONProperty - Generic feature property
// .SECTION Description

#ifndef __vtkGeoJSONProperty_h
#define __vtkGeoJSONProperty_h

#include "vtkIOGeoJSONModule.h" // For export macro
#include "vtkVariant.h"
#include <string>

class  VTKIOGEOJSON_EXPORT vtkGeoJSONProperty
{
public:
  std::string Name;
  vtkVariant Value;
};

#endif  // __vtkGeoJSONProperty_h
