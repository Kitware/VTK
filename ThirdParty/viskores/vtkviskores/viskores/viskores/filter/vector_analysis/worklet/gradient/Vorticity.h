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

#ifndef viskores_worklet_gradient_Vorticity_h
#define viskores_worklet_gradient_Vorticity_h

#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace gradient
{

using VorticityTypes =
  viskores::List<viskores::Vec<viskores::Vec3f_32, 3>, viskores::Vec<viskores::Vec3f_64, 3>>;


struct Vorticity : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn input, FieldOut output);

  template <typename InputType, typename OutputType>
  VISKORES_EXEC void operator()(const InputType& input, OutputType& vorticity) const
  {
    vorticity[0] = input[1][2] - input[2][1];
    vorticity[1] = input[2][0] - input[0][2];
    vorticity[2] = input[0][1] - input[1][0];
  }
};
}
}
}

#endif
