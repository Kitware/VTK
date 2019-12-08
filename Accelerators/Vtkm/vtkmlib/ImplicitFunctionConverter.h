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

#include "vtkAcceleratorsVTKmModule.h"
#include "vtkType.h"    // For vtkMTimeType
#include "vtkmConfig.h" //required for general vtkm setup

#include "vtkm/cont/ImplicitFunctionHandle.h"

class vtkImplicitFunction;

namespace tovtkm
{

class VTKACCELERATORSVTKM_EXPORT ImplicitFunctionConverter
{
public:
  ImplicitFunctionConverter();

  void Set(vtkImplicitFunction*);
  const vtkm::cont::ImplicitFunctionHandle& Get() const;

private:
  vtkImplicitFunction* InFunction;
  vtkm::cont::ImplicitFunctionHandle OutFunction;
  mutable vtkMTimeType MTime;
};

}

#endif // vtkmlib_ImplicitFunctionConverter_h
