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

#ifndef viskores_worklet_gradient_Transpose_h
#define viskores_worklet_gradient_Transpose_h

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace gradient
{

template <typename T>
using TransposeType = viskores::List<viskores::Vec<viskores::Vec<T, 3>, 3>>;

template <typename T>
struct Transpose3x3 : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut field);

  template <typename FieldInVecType>
  VISKORES_EXEC void operator()(FieldInVecType& field) const
  {
    T tempA, tempB, tempC;
    tempA = field[0][1];
    field[0][1] = field[1][0];
    field[1][0] = tempA;
    tempB = field[0][2];
    field[0][2] = field[2][0];
    field[2][0] = tempB;
    tempC = field[1][2];
    field[1][2] = field[2][1];
    field[2][1] = tempC;
  }

  template <typename S>
  void Run(viskores::cont::ArrayHandle<viskores::Vec<viskores::Vec<T, 3>, 3>, S>& field,
           viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny())
  {
    viskores::worklet::DispatcherMapField<Transpose3x3<T>> dispatcher;
    dispatcher.SetDevice(device);
    dispatcher.Invoke(field);
  }
};
}
}
}

#endif
