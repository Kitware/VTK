/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayTemplateHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataArrayTemplateHelpers - Non-templated implementations for
// vtkDataArrayTemplate.
// .SECTION Description
// This namespace contains algorithms for internal use by vtkDataArrayTemplate.
// Its purpose is it to work around template instantiation issues caused by
// referring to concrete vtkDataArrayTemplate<T> in vtkDataArrayTemplate's
// inlined implementations.
// Methods that cause compiler warnings such as:
// "type attributes ignored after type is already defined [-Wattributes]"
// when declaring DLL interfaces in vtkIntArray.cxx, vtkFloatArray.cxx, etc.
// should go here.

#include "vtkDataArrayTemplate.h"

#ifndef VTKDATAARRAYTEMPLATEHELPER_H
#define VTKDATAARRAYTEMPLATEHELPER_H

class vtkDataArrayTemplateHelper
{
public:
  // Description:
  // Helper implementation for vtkDataArrayTemplate::InsertTuples
  static void InsertTuples(vtkDataArray *dst, vtkIdType dstStart, vtkIdType n,
                           vtkIdType srcStart, vtkAbstractArray *source);

};

#endif // VTKDATAARRAYTEMPLATEHELPER_H

// VTK-HeaderTest-Exclude: vtkDataArrayTemplateHelper.h
