// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#ifndef vtkmlib_ImplicitFunctionConverter_h
#define vtkmlib_ImplicitFunctionConverter_h

#include "vtkAcceleratorsVTKmDataModelModule.h"
#include "vtkType.h"             // For vtkMTimeType
#include "vtkmConfigDataModel.h" //required for general viskores setup

#include "viskores/ImplicitFunction.h"

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
  const viskores::ImplicitFunctionGeneral& Get();

private:
  vtkImplicitFunction* InFunction;
  viskores::ImplicitFunctionGeneral OutFunction;
  mutable vtkMTimeType MTime;
};

VTK_ABI_NAMESPACE_END
}

#endif // vtkmlib_ImplicitFunctionConverter_h
