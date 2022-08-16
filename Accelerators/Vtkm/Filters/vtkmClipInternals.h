/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkmClipInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkmClipInternals_h
#define vtkmClipInternals_h

#include <vtkm/cont/DataSet.h>

VTK_ABI_NAMESPACE_BEGIN

class vtkDataArray;
class vtkImplicitFunction;

struct vtkmClip::internals
{
  static vtkm::cont::DataSet ExecuteClipWithImplicitFunction(
    vtkm::cont::DataSet& in, vtkImplicitFunction* clipFunction, bool insideOut);
  static vtkm::cont::DataSet ExecuteClipWithField(vtkm::cont::DataSet& in, vtkDataArray* scalars,
    int assoc, double value, bool insideOut, bool computeScalars);
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkmClipInternals.h
