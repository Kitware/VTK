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
#ifndef viskores_worklet_colorconversion_Portals_h
#define viskores_worklet_colorconversion_Portals_h

#include <viskores/VectorAnalysis.h>

namespace viskores
{
namespace worklet
{
namespace colorconversion
{

struct MagnitudePortal
{
  template <typename T, int N>
  VISKORES_EXEC auto operator()(const viskores::Vec<T, N>& values) const
    -> decltype(viskores::Magnitude(values))
  { //Should we be using RMag?
    return viskores::Magnitude(values);
  }
};

struct ComponentPortal
{
  viskores::IdComponent Component;

  ComponentPortal()
    : Component(0)
  {
  }

  ComponentPortal(viskores::IdComponent comp)
    : Component(comp)
  {
  }

  template <typename T>
  VISKORES_EXEC auto operator()(T&& value) const ->
    typename std::remove_reference<decltype(value[viskores::IdComponent{}])>::type
  {
    return value[this->Component];
  }
};
}
}
}
#endif
