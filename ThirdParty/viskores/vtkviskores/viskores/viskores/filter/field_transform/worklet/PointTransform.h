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

#ifndef viskores_worklet_PointTransform_h
#define viskores_worklet_PointTransform_h

#include <viskores/Math.h>
#include <viskores/Matrix.h>
#include <viskores/Transform3D.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{

class PointTransform : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  VISKORES_CONT
  explicit PointTransform(const viskores::Matrix<viskores::FloatDefault, 4, 4>& m)
    : matrix(m)
  {
  }

  //Functor
  VISKORES_EXEC
  viskores::Vec<viskores::FloatDefault, 3> operator()(
    const viskores::Vec<viskores::FloatDefault, 3>& vec) const
  {
    return viskores::Transform3DPoint(matrix, vec);
  }

private:
  const viskores::Matrix<viskores::FloatDefault, 4, 4> matrix;
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_PointTransform_h
