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
#include "vtkType.h" // For vtkMTimeType

#include <memory> // For std::shared_ptr


class vtkImplicitFunction;

namespace vtkm {
namespace cont {

class ImplicitFunction;

}} // vtkm::cont

namespace tovtkm {

class VTKACCELERATORSVTKM_EXPORT ImplicitFunctionConverter
{
public:
  ImplicitFunctionConverter();

  void Set(vtkImplicitFunction *);
  const std::shared_ptr<vtkm::cont::ImplicitFunction>& Get() const;

private:
  vtkImplicitFunction *InFunction;
  std::shared_ptr<vtkm::cont::ImplicitFunction> OutFunction;
  mutable vtkMTimeType MTime;
};

}

#endif // vtkmlib_ImplicitFunctionConverter_h
