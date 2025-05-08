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
#ifndef viskores_worklet_Normalize_h
#define viskores_worklet_Normalize_h

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/VectorAnalysis.h>

namespace viskores
{
namespace worklet
{

class Normal : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);

  template <typename T, typename T2>
  VISKORES_EXEC void operator()(const T& inValue, T2& outValue) const
  {
    outValue = viskores::Normal(inValue);
  }
};

class Normalize : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut);

  template <typename T>
  VISKORES_EXEC void operator()(T& value) const
  {
    viskores::Normalize(value);
  }
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_Normalize_h
