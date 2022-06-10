//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
#ifndef vtkmlib_ImplicitFunctionConverter_h
#define vtkmlib_ImplicitFunctionConverter_h

#include "vtkAcceleratorsVTKmDataModelModule.h"
#include "vtkType.h"             // For vtkMTimeType
#include "vtkmConfigDataModel.h" //required for general vtkm setup

#include "vtkm/ImplicitFunction.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkImplicitFunction;
VTK_ABI_NAMESPACE_END

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

class VTKACCELERATORSVTKMDATAMODEL_EXPORT ImplicitFunctionConverter
{
public:
  ImplicitFunctionConverter();

  void Set(vtkImplicitFunction*);
  const vtkm::ImplicitFunctionGeneral& Get();

private:
  vtkImplicitFunction* InFunction;
  vtkm::ImplicitFunctionGeneral OutFunction;
  mutable vtkMTimeType MTime;
};

VTK_ABI_NAMESPACE_END
}

#endif // vtkmlib_ImplicitFunctionConverter_h
