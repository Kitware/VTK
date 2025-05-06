//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/PointLocatorBase.h>

namespace viskores
{
namespace cont
{

void PointLocatorBase::Update() const
{
  if (this->Modified)
  {
    // Although the data of the derived class may change, the logical state
    // of the class should not. Thus, we will instruct the compiler to relax
    // the const constraint.
    const_cast<PointLocatorBase*>(this)->Build();
    this->Modified = false;
  }
}

}
} // namespace viskores::cont
