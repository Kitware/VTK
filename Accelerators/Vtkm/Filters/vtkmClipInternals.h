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

#include "vtkDataArray.h"
#include "vtkmClip.h"
#include "vtkmlib/ImplicitFunctionConverter.h"

#include <vtkm/cont/DataSet.h>

struct vtkmClip::internals
{
  double ClipValue = .0;
  bool ComputeScalars = true;

  vtkImplicitFunction* ClipFunction = nullptr;
  std::unique_ptr<tovtkm::ImplicitFunctionConverter> ClipFunctionConverter;

  vtkm::cont::DataSet ExecuteClipWithImplicitFunction(vtkm::cont::DataSet&);
  vtkm::cont::DataSet ExecuteClipWithField(vtkm::cont::DataSet&, vtkDataArray*, int);
};

#endif
// VTK-HeaderTest-Exclude: vtkmClipInternals.h
